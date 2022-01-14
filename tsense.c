/*
 * tsense.c - Temperature sensing channel health
 *
 * Copyright (C) 2021, 2022 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "linzhi/dtime.h"
#include "linzhi/mqtt.h"

#include "coord.h"
#include "mqtt.h"
#include "fsm.h"
#include "tsense.h"


#define	TCHAN_LIMIT_S	20	/* report all channels idle this long */
#define	TCHAN_TRIGGER_S	30	/* threshold to trigger a report */


struct tsense_ops {
	unsigned (*chip2die_a)(unsigned chip);
	unsigned (*chip2die_b)(unsigned chip);
	unsigned (*die2chip)(unsigned die);
	unsigned (*pos2die)(unsigned pos);
};


bool tsense_unreliable = 0;

static struct timespec last[SLOTS][MAX_CHIPS];
static bool overdue[SLOTS][MAX_CHIPS];
static unsigned chips[SLOTS];
static struct tsense_ops *ops[SLOTS];


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


/* ----- Board-specific mappings: LH --------------------------------------- */


static unsigned lh_chip2die_a(unsigned chip)
{
	return chip << 1;
}


static unsigned lh_chip2die_b(unsigned chip)
{
	return (chip << 1) ^ 3;
}


static unsigned lh_die2chip(unsigned die)
{
	return (die & ~2) >> 1 | (((die >> 1) ^ die) & 1);
}


static unsigned lh_pos2die(unsigned pos)
{
	static const unsigned map[2][8] = {
	    { 13, 15,  9, 11,  5,  7,  1,  3 },
	    { 2,  0,  6,  4, 10,  8, 14, 12 }
	};

	return map[pos >> 5][pos & 7] + 16 * (3 - ((pos >> 3) & 3));
}


static struct tsense_ops lh_ops = {
	.chip2die_a	= lh_chip2die_a,
	.chip2die_b	= lh_chip2die_b,
	.die2chip	= lh_die2chip,
	.pos2die	= lh_pos2die,
};


/* ----- Board-specific mappings: LB --------------------------------------- */


static unsigned lb_chip2die_a(unsigned chip)
{
	return 0;
}


static unsigned lb_chip2die_b(unsigned chip)
{
	return 1;
}


static unsigned lb_die2chip(unsigned die)
{
	return 0;
}


static unsigned lb_pos2die(unsigned pos)
{
	return pos;
}


static struct tsense_ops lb_ops = {
	.chip2die_a	= lb_chip2die_a,
	.chip2die_b	= lb_chip2die_b,
	.die2chip	= lb_die2chip,
	.pos2die	= lb_pos2die,
};


/* ----- Monitor temperature sensing --------------------------------------- */


static void tsense_warn(bool slot, const struct timespec *t)
{
	static bool warning[SLOTS] = { 0, };
	char buf[MAX_CHIPS * (2 * 3 + 30) + 1];	/* 30 is for the delta time */
	char *p = buf;
	unsigned chip;

	for (chip = 0; chip != chips[slot]; chip++) {
		if (!overdue[slot][chip])
			continue;
		if (p != buf)
			*p++ = ' ';
		p += sprintf(p, "%d,%d,%d",
		    ops[slot]->chip2die_a(chip), ops[slot]->chip2die_b(chip),
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
	unsigned die = ops[slot]->pos2die(pos);
	unsigned chip = ops[slot]->die2chip(die);

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
		for (chip = 0; chip != chips[slot]; chip++) {
			dt = delta(last[slot] + chip, &t);
			if (dt >= TCHAN_TRIGGER_S)
				break;
		}
		if (dt < TCHAN_TRIGGER_S)
			return;
		for (chip = 0; chip != chips[slot]; chip++) {
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
		char var[] = "CFG_SLOTn_SERIAL";
		char *sn;

		var[8] = slot + '0';
		sn = getenv(var);
		if (sn && !strncmp(sn, "BC", 2)) {
			ops[slot] = &lb_ops;
			chips[slot] = 1;
		} else {
			ops[slot] = &lh_ops;
			chips[slot] = 32;
		}

		for (chip = 0; chip != chips[slot]; chip++) {
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
	for (i = 0; i != 2 * MAX_CHIPS; i++)
		printf("%d: %u\n", i, lh_ops.pos2die(i));
	printf("--- chip -> dies ---\n");
	for (i = 0; i != MAX_CHIPS; i++) {
		int a = lh_ops.chip2die_a(i);
		int b = lh_ops.chip2die_b(i);

		printf("%d: %d,%d, %d.%d\n",
		    i, a, b, lh_ops.die2chip(a), lh_ops.die2chip(b));
	}
}
