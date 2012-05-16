/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/* Measuring times; original author: David Pfitzner <dwp@mso.anu.edu.au> */

#ifndef FC__TIMING_H
#define FC__TIMING_H

#include "shared.h"		/* bool type */

enum timer_timetype {
  TIMER_CPU,			/* time spent by the CPU */
  TIMER_USER			/* time as seen by the user ("wall clock") */
};

enum timer_use {
  TIMER_ACTIVE,			/* use this timer */
  TIMER_IGNORE			/* ignore this timer */
};
/*
 * TIMER_IGNORE is to leave a timer in the code, but not actually
 * use it, and not make any time-related system calls for it.
 * It is also used internally to turn off timers if the system
 * calls indicate that timing is not available.
 * Also note TIMER_DEBUG below.
 */

#ifdef DEBUG
#define TIMER_DEBUG TIMER_ACTIVE
#else
#define TIMER_DEBUG TIMER_IGNORE
#endif

struct timer;		/* opaque type; see comments in timing.c */

struct timer *new_timer(enum timer_timetype type, enum timer_use use);
struct timer *new_timer_start(enum timer_timetype type, enum timer_use use);

struct timer *renew_timer(struct timer *t, enum timer_timetype type,
			  enum timer_use use);
struct timer *renew_timer_start(struct timer *t, enum timer_timetype type,
				enum timer_use use);

void free_timer(struct timer *t);
bool timer_in_use(struct timer *t);

void clear_timer(struct timer *t);
void start_timer(struct timer *t);
void stop_timer(struct timer *t);
void clear_timer_start(struct timer *t);

double read_timer_seconds(struct timer *t);
double read_timer_seconds_free(struct timer *t);

void usleep_since_timer_start(struct timer *t, long usec);
void usleep_since_timer_start_free(struct timer *t, long usec);

#endif  /* FC__TIMER_H */
