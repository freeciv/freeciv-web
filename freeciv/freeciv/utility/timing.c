/***********************************************************************
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

/********************************************************************** 
  Measuring times; original author: David Pfitzner <dwp@mso.anu.edu.au>

  We assume we have at least ANSI/ISO C timing functions, so
  that we can use:
     clock_t clock() for CPU times
     time_t time() for user-time
  If we have HAVE_GETTIMEOFDAY we use gettimeofday() for user-time
  to get (usually) better resolution than time().

  As well as measuring single time intervals, these functions
  support accumulating the time from multiple separate intervals.

  Notice the struct timer is an opaque type: modules outside timing.c
  can only use it as a pointer (cf FILE type).  This is done for two
  main reasons:

   1. General principle of data hiding and encapsulation

   2. Means we don't have to include fc_config.h and possibly system
      specific header files in timing.h.  Such stuff is confined
      inside timing.c.

  However there is a disadvantage: any code using a timer must do
  memory allocation and deallocation for it.  Some of the functions
  below are intended to make this reasonably convenient; see function
  comments.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <time.h>

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef HAVE_FTIME
# include <sys/timeb.h>
#endif

/* utility */
#include "log.h"
#include "mem.h"
#include "shared.h"		/* TRUE, FALSE */
#include "support.h"

#include "timing.h"

#ifndef CLOCKS_PER_SEC
#ifdef CLOCKS_PER_SECOND
#define CLOCKS_PER_SEC CLOCKS_PER_SECOND
#else
#define CLOCKS_PER_SEC 1000000	/* wild guess!! */
#endif
#endif

#define N_USEC_PER_SEC 1000000L	  /* not 1000! :-) */

enum timer_state {
  TIMER_STARTED,
  TIMER_STOPPED
};

struct timer {
  /* type: */
  enum timer_timetype type;
  enum timer_use use;
  enum timer_state state;

  /* this is accumulated time for previous timings: */
  double sec;
  long usec;            /* not always used, in which case zero,
                           or if used may be negative, but >= -1000000 */

  /* this is start of current timing, if state == TIMER_STARTED: */
  union {
    clock_t c;
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
#elif HAVE_FTIME
    struct timeb tp;
#else
    time_t t;
#endif
  } start;
};

/*******************************************************************//**
  Report if clock() returns -1, but only the first time.
  Ignore this timer from now on.
***********************************************************************/
static void report_clock_failed(struct timer *t)
{
  static bool first = TRUE;

  if (first) {
    log_test("clock() returned -1, ignoring timer");
    first = FALSE;
  }
  t->use = TIMER_IGNORE;
}

#ifdef HAVE_GETTIMEOFDAY
/*******************************************************************//**
  Report if gettimeofday() returns -1, but only the first time.
  Ignore this timer from now on.
***********************************************************************/
static void report_gettimeofday_failed(struct timer *t)
{
  static bool first = TRUE;

  if (first) {
    log_test("gettimeofday() returned -1, ignoring timer");
    first = FALSE;
  }
  t->use = TIMER_IGNORE;
}
#elif !defined HAVE_FTIME
/*******************************************************************//**
  Report if time() returns -1, but only the first time.
  Ignore this timer from now on.
***********************************************************************/
static void report_time_failed(struct timer *t)
{
  static bool first = TRUE;

  if (first) {
    log_test("time() returned -1, ignoring timer");
    first = FALSE;
  }
  t->use = TIMER_IGNORE;
}
#endif


/*******************************************************************//**
  Allocate a new timer with specified "type" and "use".
  The timer is created as cleared, and stopped.
***********************************************************************/
struct timer *timer_new(enum timer_timetype type, enum timer_use use)
{
  return timer_renew(NULL, type, use);
}

/*******************************************************************//**
  Allocate a new timer, or reuse t, with specified "type" and "use".
  The timer is created as cleared, and stopped.
  If t is NULL, allocate and return a new timer, else
  just re-initialise t and return t.
  This is intended to be useful to allocate a static t just once, eg:
  {
     static struct timer *t = NULL; 
     t = timer_renew(t, TIMER_CPU, TIMER_USE);
     ... stuff ...
     log_verbose("That took %g seconds.", timer_read_seconds(t));
     ... never free t ...
  }
***********************************************************************/
struct timer *timer_renew(struct timer *t, enum timer_timetype type,
			  enum timer_use use)
{
  if (!t) {
    t = (struct timer *)fc_malloc(sizeof(struct timer));
  }
  t->type = type;
  t->use = use;
  timer_clear(t);
  return t;
}

/*******************************************************************//**
  Free the memory associated with a timer.
***********************************************************************/
void timer_destroy(struct timer *t)
{
  if (t != NULL) {
    free(t);
  }
}

/*******************************************************************//**
  Return whether timer is in use.
  t may be NULL, in which case returns 0
***********************************************************************/
bool timer_in_use(struct timer *t)
{
  return (t && t->use != TIMER_IGNORE);
}

/*******************************************************************//**
  Reset accumulated time to zero, and stop timer if going.
  That is, this may be called whether t is started or stopped;
  in either case the timer is in the stopped state after this function.
***********************************************************************/
void timer_clear(struct timer *t)
{
  fc_assert_ret(NULL != t);
  t->state = TIMER_STOPPED;
  t->sec = 0.0;
  t->usec = 0;
}

/*******************************************************************//**
  Start timing, adding to previous accumulated time if timer has not
  been cleared.  A warning is printed if the timer is already started.
***********************************************************************/
void timer_start(struct timer *t)
{
  fc_assert_ret(NULL != t);

  if (t->use == TIMER_IGNORE) {
    return;
  }
  if (t->state == TIMER_STARTED) {
    log_error("tried to start already started timer");
    return;
  }
  if (t->type == TIMER_CPU) {
    t->start.c = clock();
    if (t->start.c == (clock_t) -1) {
      report_clock_failed(t);
      return;
    }
  } else {
#ifdef HAVE_GETTIMEOFDAY
    int ret = gettimeofday(&t->start.tv, NULL);
    if (ret == -1) {
      report_gettimeofday_failed(t);
      return;
    }
#elif defined HAVE_FTIME
    ftime(&t->start.tp);
#else
    t->start.t = time(NULL);
    if (t->start.t == (time_t) -1) {
      report_time_failed(t);
      return;
    }
#endif
  }
  t->state = TIMER_STARTED;
}

/*******************************************************************//**
  Stop timing, and accumulate time so far.
  (The current time is stored in t->start, so that timer_read_seconds
  can call this to take a point reading if the timer is active.)
  A warning is printed if the timer is already stopped.
***********************************************************************/
void timer_stop(struct timer *t)
{
  fc_assert_ret(NULL != t);

  if (t->use == TIMER_IGNORE) {
    return;
  }
  if (t->state == TIMER_STOPPED) {
    log_error("tried to stop already stopped timer");
    return;
  }
  if (t->type == TIMER_CPU) {
    clock_t now = clock();
    if (now == (clock_t) -1) {
      report_clock_failed(t);
      return;
    }
    t->sec += (now - t->start.c) / (double)CLOCKS_PER_SEC;
    t->start.c = now;
  } else {
#ifdef HAVE_GETTIMEOFDAY
    struct timeval now;
    int ret = gettimeofday(&now, NULL);
    if (ret == -1) {
      report_gettimeofday_failed(t);
      return;
    }
    t->usec += (now.tv_usec - t->start.tv.tv_usec);
    t->sec += (now.tv_sec - t->start.tv.tv_sec);
    if (t->usec < 0) {
      t->usec += N_USEC_PER_SEC;
      t->sec -= 1.0;
    } else if (t->usec >= N_USEC_PER_SEC) {
      long sec = t->usec / N_USEC_PER_SEC;
      t->sec += sec;
      t->usec -= sec * N_USEC_PER_SEC;
    }
    t->start.tv = now;
#elif defined HAVE_FTIME
    struct timeb now;

    ftime(&now);
    t->usec += 1000 * ((long)now.millitm - (long)t->start.tp.millitm);
    t->sec += now.time - t->start.tp.time;
    if (t->usec < 0) {
      t->usec += N_USEC_PER_SEC;
      t->sec -= 1.0;
    } else if (t->usec >= N_USEC_PER_SEC) {
      long sec = t->usec / N_USEC_PER_SEC;
      t->sec += sec;
      t->usec -= sec * N_USEC_PER_SEC;
    }
    t->start.tp = now;
#else
    time_t now = time(NULL);
    if (now == (time_t) -1) {
      report_time_failed(t);
      return;
    }
    t->sec += difftime(now, t->start.t);
    t->start.t = now;
#endif
  }
  t->state = TIMER_STOPPED;
}

/*******************************************************************//**
  Read value from timer.  If the timer is not stopped, this stops the
  timer, reads it (and accumulates), and then restarts it.
  Returns 0.0 for unused timers.
***********************************************************************/
double timer_read_seconds(struct timer *t)
{
  fc_assert_ret_val(NULL != t, -1.0);

  if (t->use == TIMER_IGNORE) {
    return 0.0;
  }
  if (t->state == TIMER_STARTED) {
    timer_stop(t);
    t->state = TIMER_STARTED;
  }
  return t->sec + t->usec / (double)N_USEC_PER_SEC;
}

/*******************************************************************//**
  Sleeps until the given number of microseconds have elapsed since the
  timer was started.  Leaves the timer running.
  Must be called with an active, running user timer.
  (If timer is broken or in wrong state, just sleep for entire interval.)
***********************************************************************/
void timer_usleep_since_start(struct timer *t, long usec)
{
#ifdef HAVE_GETTIMEOFDAY
  int ret;
  struct timeval tv_now;
  long elapsed_usec;
  long wait_usec;

  fc_assert_ret(NULL != t);

  ret = gettimeofday(&tv_now, NULL);

  if ((ret == -1) ||
      (t->type != TIMER_USER) ||
      (t->use != TIMER_ACTIVE) ||
      (t->state != TIMER_STARTED)) {
    fc_usleep(usec);
    return;
  }

  elapsed_usec =
    (tv_now.tv_sec - t->start.tv.tv_sec) * N_USEC_PER_SEC +
    (tv_now.tv_usec - t->start.tv.tv_usec);
  wait_usec = usec - elapsed_usec;

  if (wait_usec > 0)
    fc_usleep(wait_usec);
#elif HAVE_FTIME
  struct timeb now;
  long elapsed_usec, wait_usec;

  ftime(&now);

  elapsed_usec = (now.time - t->start.tp.time) * N_USEC_PER_SEC
    + (now.millitm - t->start.tp.millitm);
  wait_usec = usec - elapsed_usec;

  if (wait_usec > 0) {
    fc_usleep(wait_usec);
  }
#else
  fc_usleep(usec);
#endif
}
