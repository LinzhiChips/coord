/*
 * fan.c - Fan control
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "linzhi/alloc.h"
#include "linzhi/dtime.h"
#include "linzhi/mqtt.h"
#include "linzhi/dtime.h"

#include "coord.h"
#include "mqtt.h"
#include "tsense.h"
#include "fan.h"


#define	MAX_TEMP_SILENCE_S	60


struct point {
	long		temp;
	unsigned	duty;
};

static bool have_profile = 0;
static long max_temp[SLOTS];
static uint64_t temp_skip[SLOTS] = { 0, 0 };
static bool have_temp = 0;
static struct dtime last_temp;
static struct point *points = NULL;
static unsigned n_points;


static void set_duty(unsigned duty)
{
	mqtt_printf(MQTT_TOPIC_ALL_PWM_SET, qos_ack, 1, "%u", duty);
}


static void update_pwm(void)
{
	long temp = 0;
	uint8_t slot;
	const struct point *last = NULL;
	unsigned i, duty;

	assert(have_profile);

	if (!have_temp || tsense_unreliable) {
		set_duty(100);
		return;
	}

	for (slot = 0; slot != SLOTS; slot++)
		if (max_temp[slot] > temp)
			temp = max_temp[slot];

	for (i = 0; i != n_points; i++) {
		if (points[i].temp > temp)
			break;
		last = points + i;
	}
	if (i == n_points) {
		if (verbose)
			fprintf(stderr, "update_pwm: last entry\n");
		set_duty(100); 
		return;
	}
	if (!last) {
		if (verbose)
			fprintf(stderr, "update_pwm: first entry\n");
		set_duty(points->duty);
		return;
	}
	assert(last->temp != points[i].temp);
	duty = last->duty + (points[i].duty - last->duty) *
            ((double) (temp - last->temp) / (points[i].temp - last->temp)) +
	    0.5;
	if (verbose)
		fprintf(stderr, "%ld at (%u to %u) over (%ld to %ld) = %u\n",
		    temp, last->duty, points[i].duty,
		    last->temp, points[i].temp, duty);
	set_duty(duty);
}


void fan_temp(bool slot, const char *s)
{
	struct timespec t;
	bool good = 0;
	long max = 0;
	const char *next;
	int i = -1;

	if (now)
		t = *now;
	else
		dtime_get(&t);
	do {
		char *end;
		long temp;

		i++;
		next = strchr(s, ' ');
		if (!next)
			next = strchr(s, 0);
		temp = strtod(s, &end);
		if (end == next) {
			if (((temp_skip[slot] >> i) & 1) == 0) {
				if (temp > max)
					max = temp;
				tsense_update(slot, &t, i);
				good = 1;
			}
		} else {
			if (verbose)
				fprintf(stderr, "bad temperature \"%.*s\"\n",
				    (int) (next - s), s);
		}
		s = next + 1;
	} while (*next == ' ');
	if (verbose) {
		if (good)
			fprintf(stderr, "maximum is %ld C\n", max);
		else
			fprintf(stderr, "no good temperature\n");
	}
	if (good) {
		have_temp = 1;
		max_temp[slot] = max;
		dtime_set(&last_temp, now);
	}
	if (have_profile)
		update_pwm();
}


void fan_skip(bool slot, const char *msg)
{
	char *tmp, *s;

	temp_skip[slot] = 0;
	if (!*msg)
		return;
	tmp = stralloc(msg);
	for (s = strtok(tmp, ","); s; s = strtok(NULL, ",")) {
		unsigned bus, dev, chan;
		int i;

		if (sscanf(s, "%u:%x:%u", &bus, &dev, &chan) != 3) {
			fprintf(stderr, "invalid tsense skip \"%s\"\n", s);
			continue;
		}
		i = (bus * 4 + dev - 0x48) * 8 + chan - 1;
		temp_skip[slot] |= (uint64_t) 1 << i;
	}
	free(tmp);
}


void fan_profile(const char *s)
{
	struct point *last = NULL;
	const char *next;

	if (!have_profile)
		dtime_set(&last_temp, now);
	have_profile = 1;
	free(points);
	points = NULL;
	n_points = 0;

	do {
		unsigned temp, duty;

		next = strchr(s, ' ');
		if (sscanf(s, "%u:%u", &temp, &duty) == 2 &&
		    (!last || last->temp < temp) &&
		    (!last || last->duty <= duty)) {
			points = realloc_type_n(points, n_points + 1);
			last = points + n_points;
			n_points++;
			last->temp = temp;
			last->duty = duty;
		} else {
			fprintf(stderr, "bad temp:duty entry at \"%s\"\n", s);
		}
		s = next + 1;
	} while (next);
	
	update_pwm();
}


void fan_idle(void)
{
	if (!have_profile)
		return;
	if (dtime_s(&last_temp, now) > MAX_TEMP_SILENCE_S) {
		set_duty(100);
		have_profile = 0;
	}
}


void fan_init(void)
{
	mqtt_last_will(MQTT_TOPIC_ALL_PWM_SET, qos_ack, 1, "100");
}
