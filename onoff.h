/*
 * onoff.h - On/off control
 *
 * Copyright (C) 2021, 2022 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_ONOFF_H
#define	COORD_ONOFF_H

#include <stdbool.h>
#include <stdint.h>


void onoff_shutdown(bool on);

void onoff_mined(bool slot, bool running);

void onoff_master_switch(bool on);
void onoff_slot_switch(bool slot, bool on);

void onoff_ops(uint32_t value, uint32_t mask);
void onoff_enable_ops(bool set);

void onoff_trip_master(bool set);

void onoff_init(void);
void onoff_init_late(void); /* after MQTT is operational */

#endif /* !COORD_ONOFF_H */
