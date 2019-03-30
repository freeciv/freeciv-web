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

#ifndef FC__DIRENT_H
#define FC__DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <freeciv_config.h>

#ifdef FREECIV_HAVE_DIRENT_H
#include <dirent.h>
#endif

DIR *fc_opendir(const char *dir_to_open);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__DIRENT_H */
