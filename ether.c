/*
 * ether.c - Ethernet link bounce detection
 *
 * Copyright (C) 2022 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

#include "linzhi/dtime.h"
#include "linzhi/mqtt.h"

#include "coord.h"
#include "mqtt.h"
#include "ether.h"


#define	UP_QUEUE	10
#define	MIN_INTERVAL_S	120


static time_t up_queue[UP_QUEUE];
static unsigned up_queue_n = 0;
static bool ether_is_up = 1;


static time_t ether_update(void)
{
	static bool last_bounce = 0;
	static bool first = 1;
	struct timespec ts;
	bool bounce;

	if (now)
		ts = *now;
	else
		dtime_get(&ts);

	bounce = up_queue_n >= UP_QUEUE &&
	    ts.tv_sec < up_queue[0] + MIN_INTERVAL_S &&
	    ts.tv_sec < up_queue[UP_QUEUE - 1] + UP_QUEUE * MIN_INTERVAL_S;
	if (first || bounce != last_bounce)
		mqtt_printf(MQTT_TOPIC_ETHER_BOUNCE, qos_ack, 1, "%u", bounce);
	first = false;
	last_bounce = bounce;
	return ts.tv_sec;
}


void ether_up(bool up)
{
	time_t t;

	ether_is_up = up;
	if (!up)
		return;
	t = ether_update();
	memmove(up_queue + 1, up_queue, (UP_QUEUE - 1) * sizeof(time_t));
	up_queue[0] = t;
	up_queue_n++;
}


void ether_tick(void)
{
	if (ether_is_up)
		ether_update();
}
