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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "support.h"            /* bool type */

/* Undefine this if you don't want timing measurements to appear in logs.
   This is useful if you want to compare logs of two freeciv runs and
   want to see only differences in control flow, and not diffs full of
   different timing results.
*/
#define LOG_TIMERS

/* Timing logging happens so often only in debug builds that it makes
   sense to have macro defined for it once here and to have all the
   checks against that single macro instead of two separate. */
#if defined(LOG_TIMERS) && defined(FREECIV_DEBUG)
#define DEBUG_TIMERS
#endif

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

#ifdef FREECIV_DEBUG
#define TIMER_DEBUG TIMER_ACTIVE
#else
#define TIMER_DEBUG TIMER_IGNORE
#endif

struct timer;		/* opaque type; see comments in timing.c */

#define SPECLIST_TAG timer
#define SPECLIST_TYPE struct timer
#include "speclist.h"
#define timer_list_iterate(ARG_list, NAME_item) \
  TYPED_LIST_ITERATE(struct timer, (ARG_list), NAME_item)
#define timer_list_iterate_end LIST_ITERATE_END


struct timer *timer_new(enum timer_timetype type, enum timer_use use);
struct timer *timer_renew(struct timer *t, enum timer_timetype type,
			  enum timer_use use);

void timer_destroy(struct timer *t);
bool timer_in_use(struct timer *t);

void timer_clear(struct timer *t);
void timer_start(struct timer *t);
void timer_stop(struct timer *t);

double timer_read_seconds(struct timer *t);

void timer_usleep_since_start(struct timer *t, long usec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__TIMER_H */
