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

#include <string.h>

/* utility */
#include "mem.h"

#include "fcintl.h"

static bool autocap = FALSE;

/*******************************************************************//**
  Some strings are ambiguous for translation.  For example, "Game" is
  something you play (like Freeciv!) or animals that can be hunted.
  To distinguish strings for translation, we qualify them with a prefix
  string of the form "?qualifier:".  So, the above two cases might be:
    "Game"           -- when used as meaning something you play
    "?animals:Game"  -- when used as animals to be hunted
  Notice that only the second is qualified; the first is processed in
  the normal gettext() manner (as at most one ambiguous string can be).

  This function tests for, and removes if found, the qualifier prefix part
  of a string.

  This function is called by the Q_() macro and specenum.  If used in the
  Q_() macro it should, if NLS is enabled, have called gettext() to get the
  argument to pass to this function. Specenum use it untranslated.
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

/*******************************************************************//**
  This function tries to capitalize first letter of the string.
  Currently this handles just single byte UTF-8 characters, since
  those are same as ASCII.
***********************************************************************/
char *capitalized_string(const char *str)
{
  int len = strlen(str);
  char *result = fc_malloc(len + 1);

  fc_strlcpy(result, str, len + 1);

  if (autocap) {
    if ((unsigned char) result[0] < 128) {
      result[0] = fc_toupper(result[0]);
    }
  }

  return result;
}

/*******************************************************************//**
  Free capitalized string.
***********************************************************************/
void free_capitalized(char *str)
{
  FC_FREE(str);
}

/*******************************************************************//**
  Translation opts in to automatic capitalization features.
***********************************************************************/
void capitalization_opt_in(bool opt_in)
{
  autocap = opt_in;
}

/*******************************************************************//**
  Automatic capitalization features requested.
***********************************************************************/
bool is_capitalization_enabled(void)
{
  return autocap;
}

/*******************************************************************//**
  Return directory containing locales.
***********************************************************************/
const char *get_locale_dir(void)
{
  return LOCALEDIR;
}
