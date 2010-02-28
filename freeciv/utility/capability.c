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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "shared.h"		/* TRUE, FALSE */
#include "support.h"		/* my_is* */

#include "capability.h"

#define	GET_TOKEN(start, end)	\
  {									\
    /* skip leading whitespace */					\
    while (my_isspace(*start)) {					\
      start++;								\
    }									\
    /* skip to end of token */						\
    for (end = start; *end != '\0' && !my_isspace(*end) && *end != ','; end++) { \
      /* nothing */							\
    }                                                                   \
  }

/* This routine returns true if the capability in cap appears
 * in the capability list in capstr.  The capabilities in capstr
 * are allowed to start with a "+", but the capability in cap must not.
 */
static bool my_has_capability(const char *cap, const char *capstr,
			     const size_t cap_len)
{
  const char *next;

  for (;;) {
    GET_TOKEN(capstr, next);

    if (*capstr == '+') {
      capstr++;
    }
    if ((next-capstr == cap_len) && strncmp(cap, capstr, cap_len)==0) {
      return TRUE;
    }
    if (*next == '\0') {
      return FALSE;
    }

    capstr = next+1;
  }
}

/* Wrapper for my_has_capability() for NUL terminated strings.
 */
bool has_capability(const char *cap, const char *capstr)
{
  return my_has_capability(cap, capstr, strlen(cap));
}

/* This routine returns true if all the mandatory capabilities in
 * us appear in them.
 */
bool has_capabilities(const char *us, const char *them)
{
  const char *next;

  for (;;) {
    GET_TOKEN(us, next);

    if (*us == '+' && !my_has_capability(us+1, them, next-(us+1))) {
      return FALSE;
    }
    if (*next == '\0') {
      return TRUE;
    }

    us = next+1;
  }
}
