/*
 * tsense.c - Temperature sensing channel health
 *
 * Copyright (C) 2021, 2022 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

#include "linzhi/dtime.h"
#include "linzhi/mqtt.h"

#include "coord.h"
#include "mqtt.h"
#include "fsm.h"
#include "tsense.h"


#define	TCHAN_LIMIT_S	20	/* report all channels idle this long */
#define	TCHAN_TRIGGER_S	30	/* threshold to trigger a report */

#define	CHIP2DIE_A(c)	((c) << 1)
#define	CHIP2DIE_B(c)	(((c) << 1) ^ 3)
#define	DIE2CHIP(d)	(((d) & ~2) >> 1 | ((((d) >> 1) ^ (d)) & 1))


bool tsense_unreliable = 0;

static struct timespec last[SLOTS][CHIPS];
static bool overdue[SLOTS][CHIPS];


/* ----- Helper functions -------------------------------------------------- */


static double time_s(const struct timespec *t)
{
	return t->tv_sec + t->tv_nsec * 1e-9;
}


static double delta(const struct timespec *t0, const struct timespec *t)
{
	return (t->tv_sec - t0->tv_sec) +
	    (t->tv_nsec - t0->tv_nsec) * 1e-9;
}


static unsigned pos2die(unsigned pos)
{
	static const unsigned map[2][8] = {
	    { 13, 15,  9, 11,  5,  7,  1,  3 },
	    { 2,  0,  6,  4, 10,  8, 14, 12 }
	};

	return map[pos >> 5][pos & 7] + 16 * (3 - ((pos >> 3) & 3));
}


/* ----- Monitor temperature sensing --------------------------------------- */


static void tsense_warn(bool slot, const struct timespec *t)
{
	static bool warning[SLOTS] = { 0, };
	char buf[CHIPS * (2 * 3 + 30) + 1];	/* 30 is for the delta time */
	char *p = buf;
	unsigned chip;

	for (chip = 0; chip != CHIPS; chip++) {
		if (!overdue[slot][chip])
			continue;
		if (p != buf)
			*p++ = ' ';
		p += sprintf(p, "%d,%d,%d",
		    CHIP2DIE_A(chip), CHIP2DIE_B(chip),
		    (int) delta(last[slot] + chip, t));
	}
	*p = 0;
	if (p == buf) {
		warning[slot] = 0;
		if (tsense_unreliable && !(warning[0] || warning[1])) {
			ev_tsense(0);
			tsense_unreliable = 0;
		}
		mqtt_printf_arg(MQTT_TOPIC_TEMP_OVERDUE, qos_ack, 1,
		    slot ? "1" : "0",  "%s", "");
	} else {
		warning[slot] = 1;
		if (!tsense_unreliable) {
			ev_tsense(1);
			tsense_unreliable = 1;
		}
		mqtt_printf_arg(MQTT_TOPIC_TEMP_OVERDUE, qos_ack, 1,
		    slot ? "1" : "0",  "%d %s", (int) time_s(t), buf);
	}
}


void tsense_update(bool slot, const struct timespec *t, unsigned pos)
{
	unsigned die = pos2die(pos);
	unsigned chip = DIE2CHIP(die);

	last[slot][chip] = *t;
	if (overdue[slot][chip]) {
		overdue[slot][chip] = 0;
		tsense_warn(slot, t);
	}
}


void tsense_tick(void)
{
	struct timespec t;
	unsigned slot;

	if (now)
		t = *now;
	else
		dtime_get(&t);
	for (slot = 0; slot != SLOTS; slot++) {
		unsigned chip;
		double dt;
		bool new = 0;

		if (!((slots >> slot) & 1))
			continue;
		for (chip = 0; chip != CHIPS; chip++) {
			dt = delta(last[slot] + chip, &t);
			if (dt >= TCHAN_TRIGGER_S)
				break;
		}
		if (dt < TCHAN_TRIGGER_S)
			return;
		for (chip = 0; chip != CHIPS; chip++) {
			dt = delta(last[slot] + chip, &t);
			if (dt > TCHAN_LIMIT_S) {
				if (!overdue[slot][chip])
					new = 1;
				overdue[slot][chip] = 1;
			}
		}
		if (new)
			tsense_warn(slot, &t);
	}
}


void tsense_init(void)
{
	struct timespec t;
	unsigned slot, chip;

	if (now)
		t = *now;
	else
		dtime_get(&t);
	for (slot = 0; slot != SLOTS; slot++) {
		for (chip = 0; chip != CHIPS; chip++) {
			last[slot][chip] = t;
			overdue[slot][chip] = 0;
		}
		if (!testing)
			tsense_warn(slot, &t);
	}
}


/* ----- For development --------------------------------------------------- */


/*
 * We never call tsense_test from anywhere in coord. To run it, use gdb:
 * start
 * call tsense_test()
 */

void tsense_test(void)
{
	int i;

	printf("--- pos -> die ---\n");
	for (i = 0; i != 2 * CHIPS; i++)
		printf("%d: %u\n", i, pos2die(i));
	printf("--- chip -> dies ---\n");
	for (i = 0; i != CHIPS; i++) {
		int a = CHIP2DIE_A(i);
		int b = CHIP2DIE_B(i);

		printf("%d: %d,%d, %d.%d\n",
		    i, a, b, DIE2CHIP(a), DIE2CHIP(b));
	}
}
