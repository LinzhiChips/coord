/*
 * fan.h - Fan control
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_FAN_H
#define	COORD_FAN_H

void fan_temp(bool slot, const char *s);
void fan_profile(const char *s);
void fan_idle(void);

void fan_init(void);

#endif /* !COORD_FAN_H */
