/*
 * coord.c - Coordination and monitoring
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#define	_GNU_SOURCE	/* for vasprintf */
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "linzhi/mqtt.h"

#include "led.h"
#include "action.h"
#include "fsm.h"
#include "tsense.h"
#include "fan.h"
#include "mqtt.h"
#include "coord.h"


bool verbose = 0;
bool testing = 0;
const struct timespec *now = NULL;
unsigned slots;


/* ----- Testing ----------------------------------------------------------- */


static struct timespec test_time;


static void process_line(char *s)
{
	char *p, *end;

	if (*s == '@') {
		double t;

		t = strtod(s+ 1, &end);
		if (*end) {
			fprintf(stderr, "bad time \"%s\"\n", s + 1);
			exit(1);
		}
		test_time.tv_sec = trunc(t);
		test_time.tv_nsec = (t - test_time.tv_sec) * 1e9;
		polled_actions();
		return;
	}

	if (*s == '>') {
		printf("%s\n", s + 1);
		return;
	}

	p = strchr(s, ':');
	if (!p) {
		fprintf(stderr, "don't understand \"%s\"\n", s);
		exit(1);
	}
	*p = 0;
	mqtt_deliver(s, p + 1);
}


static void run_test(void)
{
	char buf[1000];

	while (fgets(buf, sizeof(buf), stdin)) {
		char *p, *s;

		p = strchr(buf, '\n');
		if (p)
			*p = 0;

		p = strchr(buf, '#');
		if (p)
			*p = 0;
		if (!*buf)
			continue;

		s = buf;
		while (1) {
			p = strchr(s, ';');
			if (!p || p[1] != '/')
				break;
			*p = 0;
			process_line(s);
			s = p + 1;
		}
		process_line(s);
	}
}


/* ----- Periodically polled actions (every TICK_MS) ----------------------- */


void polled_actions(void)
{
	dark_mode_idle();
	ev_tick();
	tsense_tick();
}


/* ----- Slot detection ---------------------------------------------------- */


static const char *getenvf(const char *fmt, ...)
{
	va_list ap;
	char *s;
	const char *res;

	va_start(ap, fmt);
	if (vasprintf(&s, fmt, ap) < 0) {
		perror("vasprintf");
		exit(1);
	}
	va_end(ap);
	res = getenv(s);
	free(s);
	return res && *res ? res : NULL; /* treat empty as unset */
}


static bool have_slot(unsigned slot)
{
	if (!getenvf("CFG_SLOT%u_SERIAL", slot))
		return 0;
	if (getenvf("CFG_SLOT%u_IGNORE", slot))
		return 0;
	return 1;
}


/* ----- Command-line processing ------------------------------------------- */


static void usage(const char *name)
{
	fprintf(stderr,
"usage: %s [-t] [-v] [-x event:command]\n\n"
"  -t  run in test mode, with fake MQTT input and output\n"
"  -v  verbose operation\n"
"  -x event:script\n"
"      execute a shell command when an event occurs. Available events:\n"
"      user-long  USER button is being pressed for %g seconds or longer\n"
	    , name, USER_LONG_S);
	exit(1);
}


int main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "tvx:")) != EOF)
		switch (c) {
		case 't':
			testing = 1;
			now = &test_time;
			break;
		case 'v':
			verbose = 1;
			mqtt_verbose = 2;
			break;
		case 'x':
			add_event_action(optarg);
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

	slots = have_slot(0) | have_slot(1) << 1;

	fan_init();
	mqtt_setup();
	fsm_init();
	tsense_init();
	if (testing)
		run_test();
	else
		mqtt_loop();
	return 0;
}
