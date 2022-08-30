/*
 * banner.c - Banner text
 *
 * Copyright (C) 2022 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stddef.h>
#include <stdbool.h>

#include "linzhi/alloc.h"
#include "linzhi/mqtt.h"

#include "mqtt.h"
#include "banner.h"


static char *banner_part[2][3]; /* 0/1: user/factory, slot + 1 */
static char *last_banner = NULL;


void banner(bool factory, int slot, const char *msg)
{
	char *new_banner = NULL;
	unsigned cfg, idx;

	free(banner_part[factory][slot + 1]);
	banner_part[factory][slot + 1] = msg ? stralloc(msg) : NULL;

	for (idx = 0; idx != 3; idx++) {
		const char *b;

		/* select user or factory, for each component */
		for (cfg = 0; cfg != 2; cfg++) {
			b = banner_part[cfg][idx];
			if (b)
				break;
		}

		/* combine components */
		if (!b)
			continue;
		if (new_banner)
			new_banner = stralloc_append(new_banner, "\n");
		new_banner = stralloc_append(new_banner, b);
	}

	/* both are NULL */
	if (last_banner == new_banner)
		return;
	/* no change */
	if (last_banner && new_banner && !strcmp(last_banner, new_banner))
		return;

	free(last_banner);
	last_banner = new_banner;
	mqtt_printf(MQTT_TOPIC_BANNER, qos_ack, 1, "%s",
	    new_banner ? new_banner : "");
}
