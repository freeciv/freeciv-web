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
#endif

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "fcthread.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

#include "log.h"
#include "deprecations.h"

#define MAX_LEN_LOG_LINE 5120

static void log_write(FILE *fs, enum log_level level, bool print_from_where,
                      const char *where, const char *message);
static void log_real(enum log_level level, bool print_from_where,
                     const char *where, const char *msg);

static char *log_filename = NULL;
static log_pre_callback_fn log_pre_callback = log_real;
static log_callback_fn log_callback = NULL;
static log_prefix_fn log_prefix = NULL;

static fc_mutex logfile_mutex;

#ifdef FREECIV_DEBUG
static const enum log_level max_level = LOG_DEBUG;
#else
static const enum log_level max_level = LOG_VERBOSE;
#endif /* FREECIV_DEBUG */

static enum log_level fc_log_level = LOG_NORMAL;
static int fc_fatal_assertions = -1;

#ifdef FREECIV_DEBUG
struct log_fileinfo {
  char *name;
  enum log_level level;
  int min;
  int max;
};
static int log_num_files = 0;
static struct log_fileinfo *log_files = NULL;
#endif /* FREECIV_DEBUG */

static char *log_level_names[] = {
  "Fatal", "Error", "Warning", "Normal", "Verbose", "Debug", NULL
};

/* A helper variable to indicate that there is no log message. The '%s' is
 * added to use it as format string as well as the argument. */
const char *nologmsg = "nologmsg:%s";

/**********************************************************************//**
  level_str should be either "0", "1", "2", "3", "4" or
  "4:filename" or "4:file1:file2" or "4:filename,100,200" etc

  If everything goes ok, returns TRUE. If there was a parsing problem,
  prints to stderr, and returns FALSE.

  Return in ret_level the requested level only if level_str is a simple
  number (like "0", "1", "2").

  Also sets up the log_files data structure. Does _not_ set fc_log_level.
**************************************************************************/
bool log_parse_level_str(const char *level_str, enum log_level *ret_level)
{
  const char *c;
  int n = 0;                    /* number of filenames */
  int level;
  int ln;
  int first_len = -1;
#ifdef FREECIV_DEBUG
  const char *tok;
  int i;
  char *dupled;
  bool ret = TRUE;
#endif /* FREECIV_DEBUG */

  c = level_str;
  n = 0;
  while ((c = strchr(c, ':'))) {
    if (first_len < 0) {
      first_len = c - level_str;
    }
    c++;
    n++;
  }
  if (n == 0) {
    /* Global log level. */
    if (!str_to_int(level_str, &level)) {
      level = -1;
      for (ln = 0; log_level_names[ln] != NULL && level < 0; ln++) {
        if (!fc_strncasecmp(level_str, log_level_names[ln], strlen(level_str))) {
          level = ln;
        }
      }
      if (level < 0) {
        fc_fprintf(stderr, _("Bad log level \"%s\".\n"), level_str);
        return FALSE;
      }
    } else {
        log_deprecation( _("Do not provide log level with a numerical value."
            " Use one of the levels Fatal, Error, Warning, Normal, Verbose, Debug") );
    }
    if (level >= LOG_FATAL && level <= max_level) {
      if (NULL != ret_level) {
        *ret_level = level;
      }
      return TRUE;
    } else {
      fc_fprintf(stderr, _("Bad log level %d in \"%s\".\n"),
                 level, level_str);
#ifndef FREECIV_DEBUG
      if (level == max_level + 1) {
        fc_fprintf(stderr,
                   _("Freeciv must be compiled with the FREECIV_DEBUG flag "
                     "to use debug level %d.\n"), max_level + 1);
      }
#endif /* FREECIV_DEBUG */
      return FALSE;
    }
  }

#ifdef FREECIV_DEBUG
  c = level_str;
  level = -1;
  if (first_len > 0) {
    for (ln = 0; log_level_names[ln] != NULL && level < 0; ln++) {
      if (!fc_strncasecmp(level_str, log_level_names[ln], first_len)) {
        level = ln;
      }
    }
  }
  if (level < 0) {
    level = c[0] - '0';
    if (c[1] == ':') {
      if (level < LOG_FATAL || level > max_level) {
        fc_fprintf(stderr, _("Bad log level %c in \"%s\".\n"),
                   c[0], level_str);
        return FALSE;
      }
    } else {
      fc_fprintf(stderr, _("Badly formed log level argument \"%s\".\n"),
                 level_str);
      return FALSE;
    }
  }
  i = log_num_files;
  log_num_files += n;
  log_files = fc_realloc(log_files,
                         log_num_files * sizeof(struct log_fileinfo));

  dupled = fc_strdup(c + 2);
  tok = strtok(dupled, ":");

  if (!tok) {
    fc_fprintf(stderr, _("Badly formed log level argument \"%s\".\n"),
               level_str);
    ret = FALSE;
    goto out;
  }
  do {
    struct log_fileinfo *pfile = log_files + i;
    char *d = strchr(tok, ',');

    pfile->min = 0;
    pfile->max = 0;
    pfile->level = level;
    if (d) {
      char *pc = d + 1;

      d[0] = '\0';
      d = strchr(d + 1, ',');
      if (d && *pc != '\0' && d[1] != '\0') {
        d[0] = '\0';
        if (!str_to_int(pc, &pfile->min)) {
          fc_fprintf(stderr, _("Not an integer: '%s'\n"), pc);
          ret = FALSE;
          goto out;
        }
        if (!str_to_int(d + 1, &pfile->max)) {
          fc_fprintf(stderr, _("Not an integer: '%s'\n"), d + 1);
          ret = FALSE;
          goto out;
        }
      }
    }
    if (strlen(tok) == 0) {
      fc_fprintf(stderr, _("Empty filename in log level argument \"%s\".\n"),
                 level_str);
      ret = FALSE;
      goto out;
    }
    pfile->name = fc_strdup(tok);
    i++;
    tok = strtok(NULL, ":");
  } while (tok);

  if (i != log_num_files) {
    fc_fprintf(stderr, _("Badly formed log level argument \"%s\".\n"),
               level_str);
    ret = FALSE;
    goto out;
  }

out:
  free(dupled);
  return ret;
#else  /* FREECIV_DEBUG */
  fc_fprintf(stderr,
             _("Freeciv must be compiled with the FREECIV_DEBUG flag "
               "to use advanced log levels based on files.\n"));
  return FALSE;
#endif /* FREECIV_DEBUG */
}

/**********************************************************************//**
  Initialise the log module. Either 'filename' or 'callback' may be NULL.
  If both are NULL, print to stderr. If both are non-NULL, both callback, 
  and fprintf to file.  Pass -1 for fatal_assertions to don't raise any
  signal on failed assertion.
**************************************************************************/
void log_init(const char *filename, enum log_level initial_level,
              log_callback_fn callback, log_prefix_fn prefix,
              int fatal_assertions)
{
  fc_log_level = initial_level;
  if (log_filename) {
    free(log_filename);
    log_filename = NULL;
  }
  if (filename && strlen(filename) > 0) {
    log_filename = fc_strdup(filename);
  } else {
    log_filename = NULL;
  }
  log_callback = callback;
  log_prefix = prefix;
  fc_fatal_assertions = fatal_assertions;
  fc_init_mutex(&logfile_mutex);
  log_verbose("log started");
  log_debug("LOG_DEBUG test");
}

/**********************************************************************//**
   Deinitialize logging module.
**************************************************************************/
void log_close(void)
{
  fc_destroy_mutex(&logfile_mutex);
}

/**********************************************************************//**
  Adjust the log preparation callback function.
**************************************************************************/
log_pre_callback_fn log_set_pre_callback(log_pre_callback_fn precallback)
{
  log_pre_callback_fn old = log_pre_callback;

  log_pre_callback = precallback;

  return old;
}

/**********************************************************************//**
  Adjust the callback function after initial log_init().
**************************************************************************/
log_callback_fn log_set_callback(log_callback_fn callback)
{
  log_callback_fn old = log_callback;

  log_callback = callback;

  return old;
}

/**********************************************************************//**
  Adjust the prefix callback function after initial log_init().
**************************************************************************/
log_prefix_fn log_set_prefix(log_prefix_fn prefix)
{
  log_prefix_fn old = log_prefix;

  log_prefix = prefix;

  return old;
}

/**********************************************************************//**
  Adjust the logging level after initial log_init().
**************************************************************************/
void log_set_level(enum log_level level)
{
  fc_log_level = level;
}

/**********************************************************************//**
  Returns the current log level.
**************************************************************************/
enum log_level log_get_level(void)
{
  return fc_log_level;
}

/**********************************************************************//**
  Return name of the given log level
**************************************************************************/
const char *log_level_name(enum log_level lvl)
{
  if (lvl < LOG_FATAL || lvl > LOG_DEBUG) {
    return NULL;
  }

  return log_level_names[lvl];
}

#ifdef FREECIV_DEBUG
/**********************************************************************//**
  Returns wether we should do an output for this level, in this file,
  at this line.
**************************************************************************/
bool log_do_output_for_level_at_location(enum log_level level,
                                         const char *file, int line)
{
  struct log_fileinfo *pfile;
  int i;

  for (i = 0, pfile = log_files; i < log_num_files; i++, pfile++) {
    if (pfile->level >= level
        && 0 == strcmp(pfile->name, file)
        && ((0 == pfile->min && 0 == pfile->max)
            || (pfile->min <= line && pfile->max >= line))) {
      return TRUE;
    }
  }
  return (fc_log_level >= level);
}
#endif /* FREECIV_DEBUG */

/**********************************************************************//**
  Unconditionally print a simple string.
  Let the callback do its own level formatting and add a '\n' if it wants.
**************************************************************************/
static void log_write(FILE *fs, enum log_level level, bool print_from_where,
                      const char *where, const char *message)
{
  if (log_filename || (!log_callback)) {
    char prefix[128];

    if (log_prefix) {
      /* Get the log prefix. */
      fc_snprintf(prefix, sizeof(prefix), "[%s] ", log_prefix());
    } else {
      prefix[0] = '\0';
    }

    if (log_filename || (print_from_where && where)) {
      fc_fprintf(fs, "%d: %s%s%s\n", level, prefix, where, message);
    } else {
      fc_fprintf(fs, "%d: %s%s\n", level, prefix, message);
    }
    fflush(fs);
  }

  if (log_callback) {
    if (print_from_where) {
      char buf[MAX_LEN_LOG_LINE];

      fc_snprintf(buf, sizeof(buf), "%s%s", where, message);
      log_callback(level, buf, log_filename != NULL);
    } else {
      log_callback(level, message, log_filename != NULL);
    }
  }
}

/**********************************************************************//**
  Unconditionally print a log message. This function is usually protected
  by do_log_for().
**************************************************************************/
void vdo_log(const char *file, const char *function, int line,
             bool print_from_where, enum log_level level,
             char *buf, int buflen, const char *message, va_list args)
{
  char buf_where[MAX_LEN_LOG_LINE];

  /* There used to be check against recursive logging here, but
   * the way it worked prevented any kind of simultaneous logging,
   * not just recursive. Multiple threads should be able to log
   * simultaneously. */

  fc_vsnprintf(buf, buflen, message, args);
  fc_snprintf(buf_where, sizeof(buf_where), "in %s() [%s::%d]: ",
              function, file, line);

  /* In the default configuration log_pre_callback is equal to log_real(). */
  if (log_pre_callback) {
    log_pre_callback(level, print_from_where, buf_where, buf);
  }
}

/**********************************************************************//**
  Really print a log message.
  For repeat message, may wait and print instead "last message repeated ..."
  at some later time.
  Calls log_callback if non-null, else prints to stderr.
**************************************************************************/
static void log_real(enum log_level level, bool print_from_where,
                     const char *where, const char *msg)
{
  static char last_msg[MAX_LEN_LOG_LINE] = "";
  static unsigned int repeated = 0; /* total times current message repeated */
  static unsigned int next = 2; /* next total to print update */
  static unsigned int prev = 0; /* total on last update */
  /* only count as repeat if same level */
  static enum log_level prev_level = -1;
  char buf[MAX_LEN_LOG_LINE];
  FILE *fs;

  if (log_filename) {
    fc_allocate_mutex(&logfile_mutex);
    if (!(fs = fc_fopen(log_filename, "a"))) {
      fc_fprintf(stderr,
                 _("Couldn't open logfile: %s for appending \"%s\".\n"), 
                 log_filename, msg);
      exit(EXIT_FAILURE);
    }
  } else {
    fs = stderr;
  }

  if (level == prev_level && 0 == strncmp(msg, last_msg,
                                          MAX_LEN_LOG_LINE - 1)){
    repeated++;
    if (repeated == next) {
      fc_snprintf(buf, sizeof(buf),
                  PL_("last message repeated %d time",
                      "last message repeated %d times",
                      repeated-prev), repeated-prev);
      if (repeated > 2) {
        cat_snprintf(buf, sizeof(buf), 
                     PL_(" (total %d repeat)",
                         " (total %d repeats)",
                         repeated), repeated);
      }
      log_write(fs, prev_level, print_from_where, where, buf);
      prev = repeated;
      next *= 2;
    }
  } else {
    if (repeated > 0 && repeated != prev) {
      if (repeated == 1) {
        /* just repeat the previous message: */
        log_write(fs, prev_level, print_from_where, where, last_msg);
      } else {
        fc_snprintf(buf, sizeof(buf),
                    PL_("last message repeated %d time", 
                        "last message repeated %d times",
                        repeated - prev), repeated - prev);
        if (repeated > 2) {
          cat_snprintf(buf, sizeof(buf), 
                       PL_(" (total %d repeat)", " (total %d repeats)",
                           repeated),  repeated);
        }
        log_write(fs, prev_level, print_from_where, where, buf);
      }
    }
    prev_level = level;
    repeated = 0;
    next = 2;
    prev = 0;
    log_write(fs, level, print_from_where, where, msg);
  }
  /* Save last message. */
  sz_strlcpy(last_msg, msg);

  fflush(fs);
  if (log_filename) {
    fclose(fs);
    fc_release_mutex(&logfile_mutex);
  }
}

/**********************************************************************//**
  Unconditionally print a log message. This function is usually protected
  by do_log_for().
  For repeat message, may wait and print instead
  "last message repeated ..." at some later time.
  Calls log_callback if non-null, else prints to stderr.
**************************************************************************/
void do_log(const char *file, const char *function, int line,
            bool print_from_where, enum log_level level,
            const char *message, ...)
{
  char buf[MAX_LEN_LOG_LINE];
  va_list args;

  va_start(args, message);
  vdo_log(file, function, line, print_from_where, level,
          buf, MAX_LEN_LOG_LINE, message, args);
  va_end(args);
}

/**********************************************************************//**
  Set what signal the fc_assert* macros should raise on failed assertion
  (-1 to disable).
**************************************************************************/
void fc_assert_set_fatal(int fatal_assertions)
{
  fc_fatal_assertions = fatal_assertions;
}

#ifndef FREECIV_NDEBUG
/**********************************************************************//**
  Returns wether the fc_assert* macros should raise a signal on failed
  assertion.
**************************************************************************/
void fc_assert_fail(const char *file, const char *function, int line,
                    const char *assertion, const char *message, ...)
{
  enum log_level level = (0 <= fc_fatal_assertions ? LOG_FATAL : LOG_ERROR);

  if (NULL != assertion) {
    do_log(file, function, line, TRUE, level,
           "assertion '%s' failed.", assertion);
  }

  if (NULL != message && NOLOGMSG != message) {
    /* Additional message. */
    char buf[MAX_LEN_LOG_LINE];
    va_list args;

    va_start(args, message);
    vdo_log(file, function, line, FALSE, level, buf, MAX_LEN_LOG_LINE,
            message, args);
    va_end(args);
  }

  do_log(file, function, line, FALSE, level,
         /* TRANS: No full stop after the URL, could cause confusion. */
         _("Please report this message at %s"), BUG_URL);

  if (0 <= fc_fatal_assertions) {
    /* Emit a signal. */
    raise(fc_fatal_assertions);
  }
}
#endif /* FREECIV_NDEBUG */
