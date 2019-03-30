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

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL_types.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL_types.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "mem.h"

#include "utf8string.h"

/**********************************************************************//**
  Return number of characters in UTF-8 string
**************************************************************************/
size_t utf8_strlen(const char *pstr)
{
  size_t ret = 0;
  int i = 0;

  while (pstr[i] != '\0') {
    if (pstr[i++] >= 128) {
      while ((pstr[i] & (128 + 64)) == 128) {
        i++;
      }
    }
    ret++;
  }

  return ret;
}

/**********************************************************************//**
  Don't free return array, only array's members. This is not re-entrant.
**************************************************************************/
char **create_new_line_utf8strs(const char *pstr)
{
  static char *buf[512];
  const char *start = pstr;
  size_t len = 0, count = 0;

  while (*start != '\0') {
    if (*pstr == '\n') { /* find a new line char */
      if (len) {
        buf[count] = fc_calloc(len + 1, 1);
        memcpy(buf[count], start, len);
      } else {
        buf[count] = fc_calloc(2, 1);
        buf[count][0] = ' ';
      }
      start = pstr + 1;
      len = 0;
      count++;
    } else {
      len++;
    }

    pstr++;

    if (*pstr == '\0') {
      if (len != 0) {
        buf[count] = fc_calloc(len + 1, 1);
        memcpy(buf[count], start, len);
        count++;
      }

      buf[count] = NULL;
      start = pstr;
    }
  }

  return buf;
}
