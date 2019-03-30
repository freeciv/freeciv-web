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

#ifndef FC__DEPRECATIONS_H
#define FC__DEPRECATIONS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "log.h"

typedef void (*deprecation_warn_callback)(const char *msg);

#define LOG_DEPRECATION LOG_WARN

void deprecation_warn_cb_set(deprecation_warn_callback new_cb);
void deprecation_warnings_enable(void);
bool are_deprecation_warnings_enabled(void);

void do_log_deprecation(const char *format, ...);

#define log_deprecation(message, ...) \
  do { \
    if (are_deprecation_warnings_enabled()) { \
      do_log_deprecation(message, ## __VA_ARGS__); \
    } \
  } while (FALSE);

#define log_deprecation_alt(altlvl, message, ...) \
  do { \
    if (are_deprecation_warnings_enabled()) { \
      do_log_deprecation(message, ## __VA_ARGS__); \
    } else { \
      log_base(altlvl, message, ## __VA_ARGS__); \
    } \
  } while (FALSE);

#define log_deprecation_always(message, ...) \
  do_log_deprecation(message, ## __VA_ARGS__);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__DEPRECATIONS_H */
