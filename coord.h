/*
 * coord.h - Coordination and monitoring
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_COORD_H
#define	COORD_COORD_H

#include <stdbool.h>
#include <sys/time.h>


#define	TICK_MS		100
#define	USER_LONG_S	2.0	/* long press */

#define	SLOTS		2
#define	CHIPS		32


extern bool verbose;
extern bool testing;
extern const struct timespec *now;
extern unsigned slots; /* bitmask for available slots */


void polled_actions(void);

#endif /* !COORD_COORD_H */
