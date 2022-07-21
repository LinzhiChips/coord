/*
 * ether.h - Ethernet link bounce detection
 *
 * Copyright (C) 2022 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_ETHER_H
#define COORD_ETHER_H

#include <stdbool.h>


void ether_up(bool up);
void ether_tick(void);

#endif /* !COORD_ETHER_H */
