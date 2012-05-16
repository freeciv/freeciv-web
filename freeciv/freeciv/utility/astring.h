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

/********************************************************************** 
  Allocated/allocatable strings (and things?)
  See comments in astring.c
***********************************************************************/

#ifndef FC__ASTRING_H
#define FC__ASTRING_H

#include <stddef.h>		/* size_t */

#include "shared.h"		/* ADD_TO_POINTER */

struct astring {
  char *str;			/* the string */
  size_t n;			/* size most recently requested */
  size_t n_alloc;		/* total allocated */
};

/* Can assign this in variable declaration to initialize:
 * Notice a static astring var is exactly this already.
 * For athing need to call ath_init() due to size.
 */
#define ASTRING_INIT  { NULL, 0, 0 }

void astr_init(struct astring *astr);
void astr_minsize(struct astring *astr, size_t n);
void astr_free(struct astring *astr);
void astr_clear(struct astring *astr);
void astr_add(struct astring *astr, const char *format, ...)
      fc__attribute((__format__(__printf__, 2, 3)));
void astr_add_line(struct astring *astr, const char *format, ...)
      fc__attribute((__format__(__printf__, 2, 3)));

#endif  /* FC__ASTRING_H */
