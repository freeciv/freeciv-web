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

#include "fcintl.h"

/********************************************************************** 
Some strings are ambiguous for translation.  For example, "Game" is
something you play (like Freeciv!) or animals that can be hunted.
To distinguish strings for translation, we qualify them with a prefix
string of the form "?qualifier:".  So, the above two cases might be:
  "Game"           -- when used as meaning something you play
  "?animals:Game"  -- when used as animales to be hunted
Notice that only the second is qualified; the first is processed in
the normal gettext() manner (as at most one ambiguous string can be).

This function tests for, and removes if found, the qualifier prefix part
of a string.

This function should only be called by the Q_() macro.  This macro also
should, if NLS is enabled, have called gettext() to get the argument
to pass to this function.
***********************************************************************/
const char *skip_intl_qualifier_prefix(const char *str)
{
  const char *ptr;

  if (*str != '?') {
    return str;
  } else if ((ptr = strchr(str, ':'))) {
    return (ptr + 1);
  } else {
    return str;			/* may be something wrong */
  }
}
