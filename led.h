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
