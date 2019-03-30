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
#ifndef FC__PREHDRS_H
#define FC__PREHDRS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* gen_headers */
#include "freeciv_config.h"

/* If winsock stuff is to be included, it must be included
 * before <windows.h> and sometimes before any msys2 provided standard
 * headers ( <unistd.h> ). It's hard to try to include these *before*
 * standard headers only if they will be needed *later*, so we just
 * include them always. */
#ifdef FREECIV_HAVE_WINSOCK
#ifdef FREECIV_HAVE_WINSOCK2
#include <winsock2.h>
#else  /* FREECIV_HAVE_WINSOCK2 */
#include <winsock.h>
#endif /* FREECIV_HAVE_WINSOCK2 */
#endif /* FREECIV_HAVE_WINSOCK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__PREHDRS_H */
