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
#ifndef FC__LOG_H
#define FC__LOG_H

#include <stdarg.h>

#include "shared.h"		/* bool type and fc__attribute */

#define LOG_FATAL   0
#define LOG_ERROR   1		/* non-fatal errors */
#define LOG_NORMAL  2
#define LOG_VERBOSE 3		/* not shown by default */
#define LOG_DEBUG   4		/* suppressed unless DEBUG defined;
				   may be enabled on file/line basis */

/* Used by game debug command */
#define LOG_TEST LOG_NORMAL
#define LOG_PACKET LOG_VERBOSE

/* Some variables local to each file which includes log.h,
   to record whether LOG_DEBUG messages apply for that file
   and if so for which lines (min,max) :
*/
struct logdebug_afile_info {
  int tthis;
  int min;
  int max;
};
#ifdef DEBUG
static int logdebug_this_init;
static struct logdebug_afile_info logdebug_thisfile;
#endif

extern int logd_init_counter;   /* increment this to force re-init */
extern int fc_log_level;

/* Return an updated struct logdebug_afile_info: */
struct logdebug_afile_info logdebug_update(const char *file);


/* A function type to enable custom output of log messages other than
 * via fputs(stderr).  Eg, to the server console while handling prompts,
 * rfcstyle, client notifications; Eg, to the client window output window?
 */
typedef void (*log_callback_fn)(int, const char*, bool file_too);

int log_parse_level_str(const char *level_str);
void log_init(const char *filename, int initial_level,
	      log_callback_fn callback);
void log_set_level(int level);
log_callback_fn log_set_callback(log_callback_fn callback);

void real_freelog(int level, const char *message, ...)
                  fc__attribute((__format__ (__printf__, 2, 3)));
void vreal_freelog(int level, const char *message, va_list ap);


#ifdef DEBUG
/* A static (per-file) function to use/update the above per-file vars.
 * This should only be called for LOG_DEBUG messages.
 * It returns whether such a LOG_DEBUG message should be sent on
 * to real_freelog.
 */
static inline int logdebug_check(const char *file, int line)
{
  if (logdebug_this_init < logd_init_counter) {  
    logdebug_thisfile = logdebug_update(file);
    logdebug_this_init = logd_init_counter;
  } 
  return (logdebug_thisfile.tthis && (logdebug_thisfile.max==0 
				      || (line >= logdebug_thisfile.min 
					  && line <= logdebug_thisfile.max))); 
}
#endif

#ifdef DEBUG
#  define freelog(level, ...)                                             \
  do {                                                                      \
    if ((level) != LOG_DEBUG || logdebug_check(__FILE__, __LINE__)) {       \
      real_freelog((level), __VA_ARGS__);                                   \
    }                                                                       \
  } while(FALSE)
#else
#  define freelog(level, ...)                                             \
  do {                                                                      \
    if ((level) != LOG_DEBUG) {                                             \
      real_freelog((level), __VA_ARGS__);                                   \
    }                                                                       \
  } while(FALSE) 
#endif  /* DEBUG */

#define RETURN_IF_FAIL(condition)                                           \
if (!(condition)) {                                                         \
  freelog(LOG_ERROR, "In %s() (%s, line %d): assertion '%s' failed.",       \
          __FUNCTION__, __FILE__, __LINE__, #condition);                    \
  return;                                                                   \
}

#define RETURN_VAL_IF_FAIL(condition, val)                                  \
if (!(condition)) {                                                         \
  freelog(LOG_ERROR, "In %s() (%s, line %d): assertion '%s' failed.",       \
          __FUNCTION__, __FILE__, __LINE__, #condition);                    \
  return val;                                                               \
}

#define RETURN_IF_FAIL_MSG(condition, format, ...)                          \
if (!(condition)) {                                                         \
  freelog(LOG_ERROR, "In %s() (%s, line %d): " format,                      \
          __FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__);                \
  return;                                                                   \
}

#define RETURN_VAL_IF_FAIL_MSG(condition, val, format, ...)                 \
if (!(condition)) {                                                         \
  freelog(LOG_ERROR, "In %s() (%s, line %d): " format,                      \
          __FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__);                \
  return val;                                                               \
}

#endif  /* FC__LOG_H */
