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

#include "linzhi/mqtt.h"

#include "coord.h"
#include "mqtt.h"
#include "fsm.h"
#include "onoff.h"


enum state {
	s_on,		/* powered on */
	s_stop,		/* stop requested */
	s_off,		/* powered down */
	s_start,	/* start requested */
};

static enum state state[SLOTS] = { s_off, s_off };
static bool master_goal = 0;
static bool slot_goal[SLOTS] = { 0, 0 };
static bool is_running[SLOTS] = { 0, 0 };

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
		if (*arg)
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


static void step(bool slot, const char *daemon, const char *arg, bool goal)
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


static void action(void)
{
	if (separate_mined) {
		if (have_slot(0))
			step(0, "mined0", "0", master_goal && slot_goal[0]);
		if (have_slot(1))
			step(1, "mined1", "1", master_goal && slot_goal[1]);
	} else {
		step(0, "mined", "", master_goal && slot_goal[0]);
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
	master_goal = on;
	action();
	mqtt_printf(MQTT_TOPIC_CFG_MASTER, qos_ack, 0, on ? "" : "off");
}


void onoff_slot_switch(bool slot, bool on)
{
	slot_goal[slot] = on;
	action();
	mqtt_printf(slot ? MQTT_TOPIC_CFG_SLOT1 : MQTT_TOPIC_CFG_SLOT0,
	    qos_ack, 0, on ? "" : "off");
}
