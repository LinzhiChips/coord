/*
 * onoff.c - On/off control
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linzhi/mqtt.h"

#include "coord.h"
#include "mqtt.h"
#include "fsm.h"
#include "onoff.h"


enum state {
	s_on,		/* running / powered on */
	s_stop,		/* stop requested */
	s_off,		/* stopped / powered down */
	s_start,	/* start requested */
};

static enum state state[SLOTS] = { s_off, s_off };
	/* What we think mined is doing. Only state[0] is used if single
	   mined. */
static bool is_running[SLOTS] = { 0, 0 };
	/* What kunai says mined is doing. Only running[0] is used if single
	   mined. */
static bool powered[SLOTS] = { 0, 0 };
	/* Slot state. Not needed when using separate mineds. */
static bool master_goal = 0;		/* Master switch */
static bool slot_goal[SLOTS] = { 0, 0 };/* Per-slot switches */

static const char *state_name[] = {
	[s_on]		= "ON",
	[s_stop]	= "STOP",
	[s_off]		= "OFF",
	[s_start]	= "START",
};


/* ----- Helper functions -------------------------------------------------- */


static bool have_slot(bool slot)
{
	return (slots >> slot) & 1;
}


/* ----- Effect changes ---------------------------------------------------- */


static void start(const char *name)
{
	mqtt_printf_arg(MQTT_TOPIC_DAEMON_START, qos_ack, 0, name, "x");
}


static void stop(const char *name)
{
	mqtt_printf_arg(MQTT_TOPIC_DAEMON_STOP, qos_ack, 0, name, "x");
	ev_stop();
}


static void down(const char *arg)
{
	char buf[100];
	int ret;

	if (testing) {
		printf("LHADM %s down\n", arg);
	} else {
		sprintf(buf, "lhadm %s down", arg);
		ret = system(buf);
		if (ret)
			fprintf(stderr, "lhadm: exit code %d\n", ret);
	}
}


static void up(const char *arg)
{
	if (testing) {
		if (arg && *arg)
			printf("UP %s\n", arg);
		else
			printf("UP\n");
	}
	/*
	 * We don't have any real "up" action, since the launch script is
	 * expected to clean up on its own.
	 */
}


/* ----- State machines ---------------------------------------------------- */


static void step_separate(bool slot, const char *daemon, const char *arg,
    bool goal)
{
	if (verbose)
		fprintf(stderr, "step(%u, %s, %s, %u) state %s, running %u\n",
		    slot, daemon, arg, goal, state_name[state[slot]],
		    is_running[slot]);
	switch (state[slot]) {
	case s_on:
		if (goal) {
			if (!is_running[slot]) {
				if (testing)
					printf("MANUAL STOP ?\n");
				// manual stop ? crash ?
				//start("mined");
				//state[slot] = s_start;
			}
		} else {
			if (is_running[slot]) {
				stop(daemon);
				state[slot] = s_stop;
			} else {
				if (arg)
					down(arg);
				state[slot] = s_off;
			}
		}
		break;
	case s_stop:
		if (is_running[slot])
			break;
		if (goal) {
			start(daemon);
			state[slot] = s_start;
		} else {
			if (arg)
				down(arg);
			state[slot] = s_off;
		}
		break;
	case s_off:
		if (goal) {
			if (is_running[slot]) {
				up(arg);
				state[slot] = s_on;
			} else {
				start(daemon);
				state[slot] = s_start;
			}
		} else {
			if (is_running[slot]) {
				if (testing)
					printf("MANUAL START ?\n");
				// manual start ?
			}
		}
		break;
	case s_start:
		if (!is_running[slot])
			break;
		if (goal) {
			up(arg);
			state[slot] = s_on;
		} else {
			stop(daemon);
			state[slot] = s_stop;
		}
		break;
	default:
		abort();
	}
	if (verbose)
		fprintf(stderr, "-> %s\n", state_name[state[slot]]);
}


static void lhadm_adjust(bool slot, const char *s, bool want)
{
	if (want == powered[slot])
		return;
	if (!want)
		down(s);
	powered[slot] = want;
}


static void step_single(void)
{
	bool want_slot0 = have_slot(0) && master_goal && slot_goal[0];
	bool want_slot1 = have_slot(1) && master_goal && slot_goal[1];

	/* before we can change the power settings, we need to shut down
	   mined */
	if ((want_slot0 != powered[0]) || (want_slot1 != powered[1])) {
		switch (state[0]) {
		case s_on:
			if (!is_running[0]) {
				if (testing)
					printf("MANUAL STOP ?\n");
				return;
			}
			stop("mined");
			state[0] = s_stop;
			return;
		case s_stop:
			if (is_running[0])
				return;
			state[0] = s_off;
			break;
		case s_off:
			if (is_running[0]) {
				if (testing)
					printf("MANUAL START ?\n");
				return;
			}
			/* we're good, proceed */
			break;
		case s_start:
			if (is_running[0]) {
				stop("mined");
				state[0] = s_stop;
			}
			return;
		default:
			abort();

		}
		lhadm_adjust(0, "0", want_slot0);
		lhadm_adjust(1, "1", want_slot1);
	}

	step_separate(0, "mined", NULL, want_slot0 || want_slot1);
}


static void action(void)
{
	if (separate_mined) {
		if (have_slot(0))
			step_separate(0, "mined0", "0",
			    master_goal && slot_goal[0]);
		if (have_slot(1))
			step_separate(1, "mined1", "1",
			    master_goal && slot_goal[1]);
	} else {
		step_single();
	}
}


/* ----- Inputs from MQTT -------------------------------------------------- */


void onoff_mined(bool slot, bool running)
{
	is_running[slot] = running;
	action();
}


void onoff_master_switch(bool on)
{
	if (master_goal != on)
		mqtt_printf(MQTT_TOPIC_CFG_MASTER, qos_ack, 0, on ? "" : "off");
	master_goal = on;
	action();
}


void onoff_slot_switch(bool slot, bool on)
{
	if (slot_goal[slot] != on)
		mqtt_printf(slot ? MQTT_TOPIC_CFG_SLOT1 : MQTT_TOPIC_CFG_SLOT0,
		    qos_ack, 0, on ? "" : "off");
	slot_goal[slot] = on;
	action();
}


/* ----- Initialization ---------------------------------------------------- */


static bool getenv_on(const char *name)
{
	const char *s = getenv(name);

	return !s || strcmp(s, "off");
}


void onoff_init(void)
{
	master_goal = getenv_on("CFG_SWITCH_LAST");
	slot_goal[0] = getenv_on("CFG_SWITCH_0_LAST");
	slot_goal[1] = getenv_on("CFG_SWITCH_1_LAST");
}
