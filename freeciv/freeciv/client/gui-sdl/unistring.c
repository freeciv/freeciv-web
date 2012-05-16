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

/***************************************************************************
                          unistring.c  -  description
                             -------------------
    begin                : Mon Jul 08 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "SDL_types.h"

/* utility */
#include "mem.h"

/* gui-sdl */

#include "unistring.h"

/**************************************************************************
  ...
**************************************************************************/
size_t unistrlen(const Uint16 *pUniString)
{
  size_t ret = 0;
  if (pUniString) {
    while (*pUniString) {
      pUniString++;
      ret++;
    }
  }
  return ret;
}

/**************************************************************************
  ...
**************************************************************************/
Uint16 *unistrcpy(Uint16 *pToUniString, const Uint16 *pFromUniString)
{
  size_t size = (unistrlen(pFromUniString) + 1) << 1;
  if (!pToUniString) {
    pToUniString = fc_calloc(1, size);
  }
  return memcpy(pToUniString, pFromUniString, size);
}

/**************************************************************************
  ...
**************************************************************************/
Uint16 *unistrcat(Uint16 *pToUniString,
		  const Uint16 *pFromUniString)
{
  size_t src_size = (unistrlen(pFromUniString) + 1) << 1;
  size_t dst_size = unistrlen(pToUniString) << 1;
  pToUniString = fc_realloc(pToUniString, src_size + dst_size);
  return memcpy((Uint8 *) pToUniString + dst_size,
		pFromUniString, src_size);
}

/**************************************************************************
  ...
**************************************************************************/
Uint16 *unistrdup(const Uint16 *pUniString)
{
  size_t size = (unistrlen(pUniString) + 1) << 1;
  Uint16 *pNewUniString = fc_calloc(1, size);
  return memcpy(pNewUniString, pUniString, size);
}

/**************************************************************************
  Don't free return array, only arrays members
**************************************************************************/
Uint16 ** create_new_line_unistrings(const Uint16 *pUnistring)
{
  static Uint16 *pBuf[64];
  Uint16 *pFromUnistring = (Uint16 *)pUnistring;
  size_t len = 0, count = 0;
  
  while (*pUnistring != 0) {
    if (*pUnistring == 10) {	/* find new line char */
      if (len) {
	pBuf[count] = fc_calloc(len + 1, sizeof(Uint16));
	memcpy(pBuf[count], pFromUnistring, len * sizeof(Uint16));
      } else {
	pBuf[count] = fc_calloc(2, sizeof(Uint16));
	pBuf[count][0] = 32;
      }
      pFromUnistring = (Uint16 *)pUnistring + 1;
      len = 0;
      count++;
    } else {
      len++;
    }

    pUnistring++;
        
    if ((*pUnistring == 0) && len) {
      pBuf[count] = fc_calloc(len + 1, sizeof(Uint16));
      memcpy(pBuf[count], pFromUnistring, len * sizeof(Uint16));
    }
  }
  
  return pBuf;
}
