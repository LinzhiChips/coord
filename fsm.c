/*
 * fsm.c - Layered state machines to determine system condition
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "linzhi/mqtt.h"
#include "linzhi/dtime.h"

#include "coord.h"
#include "mqtt.h"
#include "led.h"
#include "action.h"
#include "fsm.h"


/* ----- Summary and LED updating ----------------------------------------- */


static enum led highlight_color(void);
static enum led abnormal_color(void);
static enum led pool_color(void);
static enum led mining_color(void);


static void update_led(void)
{
	led_set(highlight_color());
}


/* ----- Command/script execution ------------------------------------------ */


static void run(const char *cmd)
{
	if (testing)
		printf("RUN %s\n", cmd);
	else
		system(cmd);
}


/* ----- HIGHLIGHT FSM ----------------------------------------------------- */


enum highlight_state {
	hs_background,
	hs_highlight,
	hs_recovery,
	hs_user,
};


static enum highlight_state highlight_state = hs_background;


static enum led highlight_color(void)
{
	switch (highlight_state) {
	case hs_background:
		return abnormal_color();
	case hs_highlight:
		if (verbose)
			fprintf(stderr, "HIGHLIGHT=highlight -> BLUE+BLINK\n");
		return BLUE | BLINK;
	case hs_recovery:
		if (verbose)
			fprintf(stderr, "HIGHLIGHT=recovery -> BLUE\n");
		return BLUE;
	case hs_user:
		if (verbose)
			fprintf(stderr, "HIGHLIGHT=user -> BLUE\n");
		return BLUE;
	default:
		abort();
	}
}


void ev_highlight(bool on)
{
	switch (highlight_state) {
	case hs_background:
	case hs_user:
		if (on) {
			highlight_state = hs_highlight;
			update_led();
		}
		break;
	case hs_highlight:
		if (!on) {
			highlight_state = hs_background;
			update_led();
		}
		break;
	case hs_recovery:
		break;
	default:
		abort();
	}
}


void ev_recovery(bool on)
{
	if (on) {
		highlight_state = hs_recovery;
		update_led();
	}
}


/* ----- ABNORMAL FSM ------------------------------------------------------ */


enum abnormal_state {
	as_normal,
	as_warn,
	as_nowarn,
	as_shutdown,
};

enum abnormal_cond {
	TW	= 1 << 0,	/* temperature warning */
	TS	= 1 << 1,	/* temperature shutdown */
	PS	= 1 << 2,	/* power shutdown */
	I2CS	= 1 << 3,	/* I2C shutdown */
	I2CW	= 1 << 4,	/* I2C warning */
	CARD	= 1 << 5,	/* card (uSD) warning */
	BOOT	= 1 << 6,	/* boot problem warning */
};


static enum abnormal_state abnormal_state = as_normal;
static enum abnormal_cond abnormal_cond = 0;


static enum led abnormal_color(void)
{
	switch (abnormal_state) {
	case as_normal:
		return pool_color();
	case as_warn:
		if (verbose)
			fprintf(stderr, "ABNORMAL=warn -> YELLOW+BLINK\n");
		return YELLOW | BLINK;
	case as_nowarn:
		return pool_color();
	case as_shutdown:
		if (verbose)
			fprintf(stderr, "ABNORMAL=shutdown -> RED+BLINK\n");
		return RED | BLINK;
	default:
		abort();
	}
}


static void update_cond(void)
{
	switch (abnormal_state) {
	case as_normal:
		if (abnormal_cond & (TS | PS | I2CS)) {
			abnormal_state = as_shutdown;
			update_led();
		} else if (abnormal_cond & (TW | CARD | I2CW | BOOT)) {
			abnormal_state = as_warn;
			update_led();
		}
		break;
	case as_warn:
	case as_nowarn:
		if (!abnormal_cond) {
			abnormal_state = as_normal;
			update_led();
		}
		if (abnormal_cond & (TS | PS | I2CS)) {
			abnormal_state = as_shutdown;
			update_led();
		}
		break;
	case as_shutdown:
		if (!abnormal_cond) {
			abnormal_state = as_normal;
			update_led();
		}
		if ((abnormal_cond & (TW | CARD | I2CW | BOOT)) &&
		    !(abnormal_cond & ~(TW | CARD | I2CW | BOOT))) {
			abnormal_state = as_warn;
			update_led();
		}
		break;
	default:
		abort();
	}
}


void ev_twarn(bool on)
{
	if (on)
		abnormal_cond |= TW;
	else
		abnormal_cond &= ~TW;
	update_cond();
}


void ev_tshut(bool on)
{
	if (on)
		abnormal_cond |= TS;
	else
		abnormal_cond &= ~TS;
	update_cond();
}


void ev_pshut(bool on)
{
	if (on)
		abnormal_cond |= PS;
	else
		abnormal_cond &= ~PS;
	update_cond();
}


void ev_i2cwarn(bool on)
{
	if (on)
		abnormal_cond |= I2CW;
	else
		abnormal_cond &= ~I2CW;
	update_cond();
}


void ev_i2cshut(bool on)
{
	if (on)
		abnormal_cond |= I2CS;
	else
		abnormal_cond &= ~I2CS;
	update_cond();
}


void ev_card_warn(bool on)
{
	if (on)
		abnormal_cond |= CARD;
	else
		abnormal_cond &= ~CARD;
	update_cond();
}


void ev_boot_problem(const char *s)
{
	if (*s)
		abnormal_cond |= BOOT;
	else
		abnormal_cond &= ~BOOT;
	update_cond();
}



void ev_clear(void)
{
	if (abnormal_cond & BOOT) {
		abnormal_cond &= ~BOOT;
		update_cond();
	}
	if (abnormal_cond & TS)
		run("tshut clear");
	if (abnormal_cond & PS)
		run("pshut clear");
	if (abnormal_cond & I2CS)
		run("i2c-troubled clear");
	if (abnormal_cond & CARD)
		run("card-troubled clear");
}


/* ----- POOL FSM ---------------------------------------------------------- */


enum pool_state {
	ps_disconnected,
	ps_connected,
};


static enum pool_state pool_state = ps_disconnected;


static enum led pool_color(void)
{
	switch (pool_state) {
	case ps_disconnected:
		if (verbose)
			fprintf(stderr, "POOL=disconnected -> YELLOW\n");
		return YELLOW;
	case ps_connected:
		return mining_color();
	default:
		abort();
	}
}


void ev_got_job(void)
{
	switch (pool_state) {
	case ps_disconnected:
		pool_state = ps_connected;
		update_led();
		break;
	case ps_connected:
		break;
	default:
		abort();
	}
}


void ev_disconnect(void)
{
	switch (pool_state) {
	case ps_disconnected:
		break;
	case ps_connected:
		pool_state = ps_disconnected;
		update_led();
		break;
	default:
		abort();
	}
}


/* ----- MINING FSM -------------------------------------------------------- */


enum mining_state {
	ms_idle,
	ms_mining,
};


static enum mining_state mining_state = ms_idle;


static enum led mining_color(void)
{
	switch (mining_state) {
	case ms_idle:
		if (verbose)
			fprintf(stderr, "MINING=idle -> YELLOW\n");
		return YELLOW;
	case ms_mining:
		if (verbose)
			fprintf(stderr, "MINING=mining-> GREEN\n");
		return GREEN;
	default:
		abort();
	}
}


void ev_got_ack(void)
{
	switch (mining_state) {
	case ms_idle:
		mining_state = ms_mining;
		update_led();
		break;
	case ms_mining:
		break;
	default:
		abort();
	}
}


void ev_stop(void)
{
	switch (mining_state) {
	case ms_idle:
		break;
	case ms_mining:
		mining_state = ms_idle;
		update_led();
		break;
	default:
		abort();
	}
}


/* ----- mined last will (reset) ------------------------------------------- */


void ev_reset(void)
{
	pool_state = ps_disconnected;
	mining_state = ms_idle;
	update_led();
}


/* ----- USER button ------------------------------------------------------- */


static bool button_down = 0;
static struct dtime button_down_since;
static bool user_long = 0;


void ev_user(bool down)
{
	if (down && !button_down)
		dtime_set(&button_down_since, now);
	button_down = down;
	if (down) {
		dark_mode_cancel();
		/* if we use MQTT_SUB_OPT_NO_LOCAL, call ev_clear(); */
		mqtt_printf(MQTT_TOPIC_CLEAR, qos_ack, 1, "x");
		if (abnormal_state == as_warn)
			abnormal_state = as_nowarn;
	} else {
		user_long = 0;
	}
	if (highlight_state == hs_highlight) {
		mqtt_printf(MQTT_TOPIC_HIGHLIGHT, qos_ack, 1, "0");
		highlight_state = hs_background;
	} else {
		highlight_state = down ? hs_user : hs_background;
	}
	update_led();
}


/* ----- Periodic tick ----------------------------------------------------- */


void ev_tick(void)
{
	if (!button_down || dtime_s(&button_down_since, now) < USER_LONG_S)
		return;
	if (user_long)
		return;
	user_long = 1;
	run_event_action("user-long");
}


/* ----- Initialization and result ----------------------------------------- */


void fsm_init(void)
{
	highlight_state = hs_background;
	update_led();
}
