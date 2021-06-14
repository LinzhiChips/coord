/*
 * tsense.h - Temperature sensing channel health
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_TSENSE_H
#define	COORD_TSENSE_H

#include <stdbool.h>


void tsense_update(bool slot, const struct timespec *now, unsigned pos);
void tsense_tick(void);
void tsense_init(void);

void tsense_test(void);

#endif /* !COORD_TSENSE_H */
