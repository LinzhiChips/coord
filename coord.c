/*
 * coord.c - Coordination and monitoring
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "linzhi/mqtt.h"

#include "fsm.h"
#include "fan.h"
#include "mqtt.h"
#include "coord.h"


bool verbose = 0;


static void usage(const char *name)
{
	fprintf(stderr,
"usage: %s [-v]\n\n"
"  -v  verbose operation\n"
	    , name);
	exit(1);
}


int main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "v")) != EOF)
		switch (c) {
		case 'v':
			verbose = 1;
			mqtt_verbose = 2;
			break;
		default:
			usage(*argv);
		}

	switch (argc - optind) {
	case 0:
		break;
	default:
		usage(*argv);
	}

	fan_init();
	mqtt_setup();
	fsm_init();
	mqtt_loop();
	return 0;
}
