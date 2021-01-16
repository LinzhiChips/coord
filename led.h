/*
 * led.h - LED control (interface to rgbd)
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_LED_H
#define	COORD_LED_H

enum led {
	RED	= 1 << 0,
	GREEN	= 1 << 1,
	BLUE	= 1 << 2,
	YELLOW	= RED | GREEN,
	BLINK	= 1 << 3,
};


void led_set(enum led led);

void dark_mode_cancel(void);
void dark_mode_idle(void);
void dark_mode_set(unsigned timeout_s);

#endif /* !COORD_LED_H */
