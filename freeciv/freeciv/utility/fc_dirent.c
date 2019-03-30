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

#include <stdlib.h>

/* utility */
#include "fciconv.h"

#include "fc_dirent.h"

/**************************************************************//**
  Wrapper function for opendir() with filename conversion to local
  encoding on Windows.
******************************************************************/
DIR *fc_opendir(const char *dir_to_open)
{
#ifdef FREECIV_MSWINDOWS
  DIR *result;
  char *dirname_in_local_encoding =
    internal_to_local_string_malloc(dir_to_open);

  result = opendir(dirname_in_local_encoding);
  free(dirname_in_local_encoding);
  return result;
#else  /* FREECIV_MSWINDOWS */
  return opendir(dir_to_open);
#endif /* FREECIV_MSWINDOWS */
}
