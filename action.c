#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linzhi/alloc.h"

#include "coord.h"
#include "action.h"


struct event_action {
	const char *event;	/* NULL for the last entry */
	const char *action;
};


static struct event_action events[] = {
	{ "user-long",	NULL },
	{ NULL, NULL }
};


void run_event_action(const char *event)
{
	struct event_action *e;
	int ret;

	for (e = events; e->event; e++)
		if (!strcmp(event, e->event)) {
			if (e->action) {
				if (verbose)
					fprintf(stderr, "running %s:%s\n",
					    e->event, e->action);
				ret = system(e->action);
				if (ret)
					fprintf(stderr, "%s: exit code %d\n",
					    event, ret);
			}
			return;
		}
}


void add_event_action(const char *arg)
{
	struct event_action *e;
	const char *colon;
	size_t len;

	colon = strchr(arg, ':');
	if (!colon) {
		fprintf(stderr, "format should be \"event:command\": %s\n",
		    arg);
		exit(1);
	}
	len = colon - arg;
	for (e = events; e->event; e++)
		if (strlen(e->event) == len && !strncmp(e->event, arg, len)) {
			free((void *) e->action);
			e->action = stralloc(colon + 1);
			return;
		}
	fprintf(stderr, "event \"%.*s\" not found\n", (int) len, arg);
	exit(1);
}
