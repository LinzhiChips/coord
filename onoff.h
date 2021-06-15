/*
 * onoff.h - On/off control
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_ONOFF_H
#define	COORD_ONOFF_H

#include <stdbool.h>


void onoff_mined(bool slot, bool running);

void onoff_master_switch(bool on);
void onoff_slot_switch(bool slot, bool on);

#endif /* !COORD_ONOFF_H */
