#include <stdbool.h>

#include "linzhi/dtime.h"
#include "linzhi/mqtt.h"

#include "mqtt.h"
#include "led.h"


static enum led curr_led = 0;
static bool dark = 0;
static unsigned dark_timeout_s = 0;	/* 0 to disable dark mode */
static double dark_t0;


static void do_led_set(enum led led)
{
	char color[4];
	char *c = color;
	unsigned on_ms = 0, off_ms = 0;

	if (led & RED)
		*c++ = 'R';
	if (led & GREEN)
		*c++ = 'G';
	if (led & BLUE)
		*c++ = 'B';
	if (c == color)
		*c++ = '-';
	if (led & BLINK) {
		switch (led & ~BLINK) {
		case RED | GREEN: /* warning */
			on_ms = off_ms = 100;
			break;
		case BLUE: /* HIGHLIGHT */
			on_ms = 1450;
			off_ms = 50;
			break;
		default: /* includes RED = shutdown */
			on_ms = off_ms = 500;
		}
	}
	*c++ = 0;
	mqtt_printf(MQTT_TOPIC_LED_RGB, qos_ack, 1, "%s %u %u",
	    color, on_ms, off_ms);
}


static void ether_on(bool on)
{
	mqtt_printf(MQTT_TOPIC_LED_ETHER, qos_ack, 1, "%u", on);
}


void led_set(enum led led)
{
	if (dark && led == GREEN)
		return;
	curr_led = led;
	dark_mode_cancel();
}


void dark_mode_cancel(void)
{
	do_led_set(curr_led);
	ether_on(1);
	dark = 0;
	dark_t0 = dtime_s(NULL, NULL);
}


void dark_mode_idle(void)
{
	double t;

	if (dark || curr_led != GREEN || !dark_timeout_s)
		return;
	t = dtime_s(NULL, NULL);
	if (t - dark_t0 < dark_timeout_s)
		return;
	do_led_set(0);
	ether_on(0);
	dark = 1;
}


void dark_mode_set(unsigned timeout_s)
{
	dark_mode_cancel();
	dark_timeout_s = timeout_s;
}
