#ifndef COORD_FSM_H
#define	COORD_FSM_H

#include <stdbool.h>

#include "led.h"


/* HIGHLIGHT FSM */

void ev_highlight(bool on);
void ev_recovery(bool on);

/* ABNORMAL FSM */

void ev_twarn(bool on);
void ev_tshut(bool on);
void ev_pshut(bool on);
void ev_i2cwarn(bool on);
void ev_i2cshut(bool on);
void ev_card_warn(bool on);
void ev_boot_problem(const char *s);
void ev_clear(void);

/* POOL FSM */

void ev_got_job(void);
void ev_disconnect(void);

/* MINING FSM */

void ev_got_ack(void);
void ev_stop(void);

/* mined last will (reset) */

void ev_reset(void);

/* USER button */

void ev_user(bool down);

/* initialization */

void fsm_init(void);

#endif /* !COORD_FSM_H */
