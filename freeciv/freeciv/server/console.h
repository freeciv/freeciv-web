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
#ifndef FC__CONSOLE_H
#define FC__CONSOLE_H

#include "shared.h"		/* bool type and fc__attribute */

#define MAX_LEN_CONSOLE_LINE 512	/* closing \0 included */

/* 
 * A note on "rfc-style":
 *
 * This style of server output, started with the /rfcstyle server
 * command, prefixes all output with a status number. This is similar
 * to how some common ascii based internet protocols like FTP and SMTP
 * work. A parser can check these numbers to determine whether an
 * action was successful or not, instead of attempting to parse the
 * text (which can be translated into various languages and easily
 * change between versions). This status number is given to the output
 * functions below as their first parameter, or to cmd_reply* as their
 * third parameter.
 */

enum rfc_status {
  C_IGNORE = -1,                /* never print RFC-style number prefix */
  C_COMMENT = 0,                /* for human eyes only */
  C_VERSION = 1,                /* version info */
  C_DEBUG = 2,                  /* debug info */
  C_LOG_BASE = 10,              /* 10, 11, 12 depending on log level */
  C_OK = 100,                   /* success of requested operation */
  C_CONNECTION = 101,           /* new client */
  C_DISCONNECTED = 102,         /* client gone */
  C_REJECTED = 103,             /* client rejected */
  C_FAIL = 200,                 /* failure of requested operation */
  C_METAERROR = 201,            /* failure of meta server */
  C_SYNTAX = 300,               /* syntax error or value out of range */
  C_BOUNCE = 301,               /* option no longer available */
  C_GENFAIL = 400,              /* failure not caused by a requested operation */
  C_WARNING = 500,              /* something may be wrong */
  C_READY = 999                 /* waiting for input */
};

/* initialize logging via console */
void con_log_init(const char *log_filename, int log_level);

/* write to console and add line-break, and show prompt if required. */
void con_write(enum rfc_status rfc_status, const char *message, ...)
     fc__attribute((__format__ (__printf__, 2, 3)));

/* write to console and add line-break, and show prompt if required.
   ie, same as con_write, but without the format string stuff. */
void con_puts(enum rfc_status rfc_status, const char *str);
     
/* ensure timely update */
void con_flush(void);

/* initialize prompt; display initial message */
void con_prompt_init(void);

/* make sure a prompt is printed, and re-printed after every message */
void con_prompt_on(void);

/* do not print a prompt after every message */
void con_prompt_off(void);

/* user pressed enter: will need a new prompt */
void con_prompt_enter(void);

/* clear "user pressed enter" state (used in special cases) */
void con_prompt_enter_clear(void);

/* set server output style */
void con_set_style(bool i);

/* return server output style */
bool con_get_style(void);

#endif  /* FC__CONSOLE_H */
