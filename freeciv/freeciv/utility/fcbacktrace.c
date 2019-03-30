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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif /* HAVE_CONFIG_H */

#include "fc_prehdrs.h"

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

/* utility */
#include "log.h"
#include "shared.h"

#include "fcbacktrace.h"

/* We don't want backtrace-spam to testmatic logs */
#if defined(FREECIV_DEBUG) && defined(HAVE_BACKTRACE) && !defined(FREECIV_TESTMATIC)
#define BACKTRACE_ACTIVE 1
#endif

#ifdef BACKTRACE_ACTIVE

/* We write always in level LOG_NORMAL and not in higher one since those
 * interact badly with server callback to send error messages to local
 * client. */
#define LOG_BACKTRACE LOG_NORMAL

#define MAX_NUM_FRAMES 64

static log_pre_callback_fn previous = NULL;

static void write_backtrace_line(enum log_level level, bool print_from_where,
                                 const char *where, const char *msg);
static void backtrace_log(enum log_level level, bool print_from_where,
                          const char *where, const char *msg);
#endif /* BACKTRACE_ACTIVE */

/********************************************************************//**
  Take backtrace log callback to use
************************************************************************/
void backtrace_init(void)
{
#ifdef BACKTRACE_ACTIVE
  previous = log_set_pre_callback(backtrace_log);
#endif
}

/********************************************************************//**
  Remove backtrace log callback from use
************************************************************************/
void backtrace_deinit(void)
{
#ifdef BACKTRACE_ACTIVE
  log_pre_callback_fn active;

  active = log_set_pre_callback(previous);

  if (active != backtrace_log) {
    /* We were not the active callback!
     * Restore the active callback and log error */
    log_set_pre_callback(active);
    log_error("Backtrace log (pre)callback cannot be removed");
  }
#endif /* BACKTRACE_ACTIVE */
}

#ifdef BACKTRACE_ACTIVE
/********************************************************************//**
  Write one line of backtrace
************************************************************************/
static void write_backtrace_line(enum log_level level, bool print_from_where,
                                 const char *where, const char *msg)
{
  /* Current behavior of this function is to write to chained callback,
   * nothing more, nothing less. */
  if (previous != NULL) {
    previous(level, print_from_where, where, msg);
  }
}

/********************************************************************//**
  Main backtrace callback called from logging code.
************************************************************************/
static void backtrace_log(enum log_level level, bool print_from_where,
                          const char *where, const char *msg)
{
  if (previous != NULL) {
    /* Call chained callback first */
    previous(level, print_from_where, where, msg);
  }

  if (level <= LOG_ERROR) {
    backtrace_print(LOG_BACKTRACE);
  }
}

#endif /* BACKTRACE_ACTIVE */

/********************************************************************//**
  Print backtrace
************************************************************************/
void backtrace_print(enum log_level level)
{
#ifdef BACKTRACE_ACTIVE
  void *buffer[MAX_NUM_FRAMES];
  int frames;
  char **names;

  frames = backtrace(buffer, ARRAY_SIZE(buffer));
  names = backtrace_symbols(buffer, frames);

  if (names == NULL) {
    write_backtrace_line(level, FALSE, NULL, "No backtrace");
  } else {
    int i;

    write_backtrace_line(level, FALSE, NULL, "Backtrace:");

    for (i = 0; i < MIN(frames, MAX_NUM_FRAMES); i++) {
      char linestr[256];

      fc_snprintf(linestr, sizeof(linestr), "%5d: %s", i, names[i]);

      write_backtrace_line(level, FALSE, NULL, linestr);
    }

    free(names);
  }
#endif /* BACKTRACE_ACTIVE */
}
