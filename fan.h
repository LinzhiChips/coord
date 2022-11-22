/*
 * fan.h - Fan control
 *
 * Copyright (C) 2021, 2022 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_FAN_H
#define	COORD_FAN_H

void fan_temp(bool slot, const char *s);
void fan_skip(bool slot, const char *s);

void fan_profile_set(const char *s);
void fan_profile_factory(const char *s);
void fan_profile_user(const char *s);

void fan_idle(void);

void fan_init(void);
void fan_late_init(void);

#endif /* !COORD_FAN_H */
