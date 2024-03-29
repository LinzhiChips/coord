/*
 * mqtt.c - MQTT setup and input processing
 *
 * Copyright (C) 2021, 2022 Linzhi Ltd.
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
#include "onoff.h"
#include "ether.h"
#include "banner.h"
#include "mqtt.h"


/* --- Inbound ------------------------------------------------------------- */


static void process_pool_stats(const char *msg, int slot)
{
	static unsigned last_j[2] = { UINT_MAX, UINT_MAX };
	static unsigned last_a[2] = { 0, 0 };
	unsigned long long c;
	unsigned j, a, r, s;

	if (slot == -1) {
		slot = 0;
		last_j[1] = UINT_MAX;
		last_a[1] = 0;
	}
	if (sscanf(msg, "C:%llu J:%u A:%u R:%u S:%u", &c, &j, &a, &r, &s)
	    != 5) {
		fprintf(stderr, "bad statistics: \"%s\"\n", msg);
		return;
	}
	/*
	 * @@@ mined doesn't reset J on disconnect yet
	 */
	if (j < last_j[slot]) {
		ev_disconnect();
	} else {
		if (j > last_j[slot])
			ev_got_job();
		if (a > last_a[slot])
			ev_got_ack();
	}
	last_j[slot] = j;
	last_a[slot] = a;
}


static void process_pool_stats_common(const char *msg)
{
	process_pool_stats(msg, -1);
}


static void process_pool_stats_0(const char *msg)
{
	process_pool_stats(msg, 0);
}


static void process_pool_stats_1(const char *msg)
{
	process_pool_stats(msg, 1);
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


static void process_mined(const char *s)
{
	bool running = strcmp(s, "0");

	/*
	 * We consider the whole system as "slot 0".
	 * @@@ This would need changing if we ever want to allow switching
	 * between single and separate mineds without a reboot.
	 */
	onoff_mined(0, running);
}


static void process_mined0(const char *s)
{
	onoff_mined(0, strcmp(s, "0"));
}


static void process_mined1(const char *s)
{
	onoff_mined(1, strcmp(s, "0"));
}


static void process_onoff_master(const char *s)
{
	onoff_master_switch(strcmp(s, "0"));
}


static void process_onoff_slot0(const char *s)
{
	onoff_slot_switch(0, strcmp(s, "0"));
}


static void process_onoff_slot1(const char *s)
{
	onoff_slot_switch(1, strcmp(s, "0"));
}


static void process_trip_master(const char *s)
{
	onoff_trip_master(!strcmp(s, "y"));
}


static void process_onoff_ops_set(const char *s)
{
	uint32_t value, mask = 0xffffffff;
	char *end;

	value = strtoul(s, &end, 0);
	if (*end != ' ')
		goto invalid;
	mask = strtoul(end + 1, &end, 0);
	if (*end)
		goto invalid;
	onoff_ops(value, mask);
	return;

invalid:
	fprintf(stderr, "don't understand \"%s\"\n", s);
}


static void process_switch_ops_set(const char *s)
{
	onoff_enable_ops(!!strcmp(s, "n"));
}


static void process_banner(void *user, const char *topic, const char *msg)
{
	if (!strcmp(topic, MQTT_TOPIC_CFG_BANNER_USER))
		banner(0, -1, msg);
	else if (!strcmp(topic, MQTT_TOPIC_CFG_BANNER_U0))
		banner(0, 0, msg);
	else if (!strcmp(topic, MQTT_TOPIC_CFG_BANNER_U1))
		banner(0, 1, msg);
	else if (!strcmp(topic, MQTT_TOPIC_CFG_BANNER_FACTORY))
		banner(1, -1, msg);
	else if (!strcmp(topic, MQTT_TOPIC_CFG_BANNER_F0))
		banner(1, 0, msg);
	else if (!strcmp(topic, MQTT_TOPIC_CFG_BANNER_F1))
		banner(1, 1, msg);
	else
		fprintf(stderr, "don't recognize topic \"%s\"\n", topic);
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


static void sub_generic(const char *topic,
    void (*fn)(void *user, const char *topic, const char *msg), void *user)
{
	mqtt_subscribe("%s", qos_ack, fn, user, topic);
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
	sub_bool(MQTT_TOPIC_ETHER_UP, ether_up);

	sub_string(MQTT_TOPIC_BOOT_PROBLEM, ev_boot_problem);
	sub_string(MQTT_TOPIC_POOL_STATS, process_pool_stats_common);
	sub_string(MQTT_TOPIC_POOL_SLOT0_STATS, process_pool_stats_0);
	sub_string(MQTT_TOPIC_POOL_SLOT1_STATS, process_pool_stats_1);
	sub_string(MQTT_TOPIC_MINED_STATE, process_mined_state);
	sub_n(MQTT_TOPIC_DARK_MODE, dark_mode_set);

	sub_string(MQTT_TOPIC_0_TEMP, process_temp_0);
	sub_string(MQTT_TOPIC_1_TEMP, process_temp_1);
	sub_string(MQTT_TOPIC_0_SKIP, process_skip_0);
	sub_string(MQTT_TOPIC_1_SKIP, process_skip_1);

	sub_string(MQTT_TOPIC_FAN_PROFILE_SET, fan_profile_set);
	sub_string(MQTT_TOPIC_CFG_PROFILE_FACTORY, fan_profile_factory);
	sub_string(MQTT_TOPIC_CFG_PROFILE_USER, fan_profile_user);

	sub_string(MQTT_TOPIC_MINED_TIME, process_mined);
	sub_string(MQTT_TOPIC_MINED0_TIME, process_mined0);
	sub_string(MQTT_TOPIC_MINED1_TIME, process_mined1);
	sub_string(MQTT_TOPIC_ONOFF_MASTER, process_onoff_master);
	sub_string(MQTT_TOPIC_ONOFF_SLOT0, process_onoff_slot0);
	sub_string(MQTT_TOPIC_ONOFF_SLOT1, process_onoff_slot1);

	sub_generic(MQTT_TOPIC_CFG_BANNER_USER, process_banner, NULL);
	sub_generic(MQTT_TOPIC_CFG_BANNER_U0, process_banner, NULL);
	sub_generic(MQTT_TOPIC_CFG_BANNER_U1, process_banner, NULL);
	sub_generic(MQTT_TOPIC_CFG_BANNER_FACTORY, process_banner, NULL);
	sub_generic(MQTT_TOPIC_CFG_BANNER_F0, process_banner, NULL);
	sub_generic(MQTT_TOPIC_CFG_BANNER_F1, process_banner, NULL);

	sub_string(MQTT_TOPIC_ONOFF_OPS_SET, process_onoff_ops_set);
	sub_string(MQTT_TOPIC_CFG_SWITCH_OPS, process_switch_ops_set);

	sub_string(MQTT_TOPIC_CFG_TRIP_MASTER, process_trip_master);

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
