/********************************************************************** 
 Freeciv - Copyright (C) 2005 The Freeciv Team
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

#include "themes_common.h"
#include "themes_g.h"

/*****************************************************************************
  Loads a gtk theme directory/theme_name
*****************************************************************************/
void gui_load_theme(const char *directory, const char *theme_name)
{
  /* Nothing */
}

/*****************************************************************************
  Clears a theme (sets default system theme)
*****************************************************************************/
void gui_clear_theme(void)
{
  /* Nothing */
}

/*****************************************************************************
  Each gui has its own themes directories.

  Returns an array containing these strings and sets array size in count.
  The caller is responsible for freeing the array and the paths.
*****************************************************************************/
char **get_gui_specific_themes_directories(int *count)
{
  *count = 0;
  
  return fc_malloc(sizeof(char*) * 0);
}

/*****************************************************************************
  Return an array of names of usable themes in the given directory.
  Array size is stored in count.
  Useable theme for gtk+ is a directory which contains file gtk-2.0/gtkrc.
  The caller is responsible for freeing the array and the names
*****************************************************************************/
char **get_useable_themes_in_directory(const char *directory, int *count)
{
  *count = 0;
  return fc_malloc(sizeof(char*) * 0);
}
