/*
 * mqtt.c - MQTT setup and input processing
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "linzhi/mqtt.h"

#include "coord.h"
#include "fsm.h"
#include "fan.h"
#include "mqtt.h"


/* --- Inbound ------------------------------------------------------------- */


static void process_pool_stats(const char *msg)
{
	static unsigned last_j = UINT_MAX;
	static unsigned last_a = 0;
	unsigned long long c;
	unsigned j, a, r, s;

	if (sscanf(msg, "C:%llu J:%u A:%u R:%u S:%u", &c, &j, &a, &r, &s)
	    != 5) {
		fprintf(stderr, "bad statistics: \"%s\"\n", msg);
		return;
	}
	/*
	 * @@@ mined doesn't reset J on disconnect yet
	 */
	if (j < last_j) {
		ev_disconnect();
	} else {
		if (j > last_j)
			ev_got_job();
		if (a > last_a)
			ev_got_ack();
	}
	last_j = j;
	last_a = a;
}


static void process_mined_state(const char *s)
{
	/*
	 * @@@ There are more ways the miner could go "dead".
	 */
	if (!strcmp(s, "dead"))
		ev_stop();
}


static void process_temp_0(const char *s)
{
	fan_temp(0, s);
}


static void process_temp_1(const char *s)
{
	fan_temp(1, s);
}


static void process_skip_0(const char *s)
{
	fan_skip(0, s);
}


static void process_skip_1(const char *s)
{
	fan_skip(1, s);
}


/* ----- Callback wrappers ------------------------------------------------- */


static void cb_void(void *user, const char *topic, const char *msg)
{
	void (*fn)(void) = user;

	fn();
}


static void cb_bool(void *user, const char *topic, const char *msg)
{
	void (*fn)(bool) = user;
	unsigned long long n;
	char *end;

	n = strtoull(msg, &end, 0);
	if (*end && *end != '.' && *end != ' ')
		n = 0;
	fn(n);
}


static void cb_n(void *user, const char *topic, const char *msg)
{
	void (*fn)(unsigned) = user;
	unsigned long long n;
	char *end;

	n = strtoull(msg, &end, 0);
	if (*end && *end != '.')
		n = 0;
	fn(n);
}


static void cb_s(void *user, const char *topic, const char *msg)
{
	void (*fn)(const char *s) = user;

	fn(msg);
}


/* sub_* are to ensure type compatibility */


static void sub_void(const char *topic, void (*fn)(void))
{
	mqtt_subscribe("%s", qos_ack, cb_void, fn, topic);
}


static void sub_bool(const char *topic, void (*fn)(bool))
{
	mqtt_subscribe("%s", qos_ack, cb_bool, fn, topic);
}


static void sub_n(const char *topic, void (*fn)(unsigned))
{
	mqtt_subscribe("%s", qos_ack, cb_n, fn, topic);
}


static void sub_string(const char *topic, void (*fn)(const char *))
{
	mqtt_subscribe("%s", qos_ack, cb_s, fn, topic);
}


/* ----- Loop and setup ---------------------------------------------------- */


void mqtt_setup(void)
{
	sub_void(MQTT_TOPIC_CLEAR, ev_clear);
	sub_bool(MQTT_TOPIC_HIGHLIGHT, ev_highlight);
	sub_bool(MQTT_TOPIC_PSHUT, ev_pshut);
	sub_bool(MQTT_TOPIC_I2CSHUT, ev_i2cshut);
	sub_bool(MQTT_TOPIC_I2CWARN, ev_i2cwarn);
	sub_bool(MQTT_TOPIC_TSHUT, ev_tshut);
	sub_bool(MQTT_TOPIC_TWARN, ev_twarn);
	sub_bool(MQTT_TOPIC_CARD_WARN, ev_card_warn);
	sub_bool(MQTT_TOPIC_USER, ev_user);
	sub_bool(MQTT_TOPIC_RECOVERY, ev_recovery);
	sub_string(MQTT_TOPIC_BOOT_PROBLEM, ev_boot_problem);
	sub_string(MQTT_TOPIC_POOL_STATS, process_pool_stats);
	sub_string(MQTT_TOPIC_MINED_STATE, process_mined_state);
	sub_n(MQTT_TOPIC_DARK_MODE, dark_mode_set);

	sub_string(MQTT_TOPIC_0_TEMP, process_temp_0);
	sub_string(MQTT_TOPIC_1_TEMP, process_temp_1);
	sub_string(MQTT_TOPIC_0_SKIP, process_skip_0);
	sub_string(MQTT_TOPIC_1_SKIP, process_skip_1);
	sub_string(MQTT_TOPIC_FAN_PROFILE, fan_profile);

	if (testing)
		mqtt_testing();
	else
		mqtt_init(NULL, 0);
}


void mqtt_loop(void)
{
	assert(!testing);
	while (1) {
		mqtt_loop_once(TICK_MS);
		polled_actions();
	}
}
