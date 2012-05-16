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

#include <assert.h>

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>


#include "mem.h"
#include "shared.h"
#include "support.h"

#include "themes_common.h"

#include "themes_g.h"

/***************************************************************************
  A theme is a portion of client data, which for following reasons should
  be separated from a tileset:
  - Theme is not only graphic related
  - Theme can be changed independently from tileset
  - Theme implementation is gui specific and most themes can not be shared
    between different guis.
  Theme is recognized by its name.
  
  Theme is stored in a directory called like the theme. The directory contains
  some data files. Each gui defines its own format in the
  get_useable_themes_in_directory() function.
****************************************************************************/

/* A directory containing a list of usable themes */
struct theme_directory {
  /* Path on the filesystem */
  char *path; 
  /* Array of theme names */
  char **themes;
  /* Themes array length */
  int num_themes;
};

/* List of all directories with themes */
static int num_directories;
struct theme_directory *directories;

/****************************************************************************
  Initialized themes data
****************************************************************************/
void init_themes(void)
{
  int i;
    
  /* get GUI-specific theme directories */
  char **gui_directories =
      get_gui_specific_themes_directories(&num_directories);
  
  directories = 
      fc_malloc(sizeof(struct theme_directory) * num_directories);
          
  for (i = 0; i < num_directories; i++) {
    directories[i].path = gui_directories[i];
    
    /* get useable themes in this directory */
    directories[i].themes =
	get_useable_themes_in_directory(directories[i].path,
					&(directories[i].num_themes));
  }
}

/****************************************************************************
  Return an array of useable theme names. The array is NULL terminated and 
  the caller is responsible for freeing the array.
****************************************************************************/
const char **get_themes_list(void)
{
  int size = 0;
  int i, j, k, c;
  const char **themes;

  for (i = 0; i < num_directories; i++) {
    size += directories[i].num_themes;
  }

  themes = fc_malloc(sizeof(char *) * (size + 1));

  /* Copy theme names from all directories, but remove duplicates */
  c = 0;
  for (i = 0; i < num_directories; i++) {
    for (j = 0; j < directories[i].num_themes; j++) {
      for (k = 0; k < c; k++) {
	if (strcmp(themes[k], directories[i].themes[j]) == 0) {
	  break;
	}
      }
      if (k == c) {
	themes[c++] = directories[i].themes[j];
      }
    }
  }
  themes[c] = NULL;
  return themes;
}

/****************************************************************************
  Loads a theme with the given name. First matching directory will be used.
  If there's no such theme the function returns FALSE.
****************************************************************************/
bool load_theme(const char *theme_name)
{
  int i, j;
  for (i = 0; i < num_directories; i++) {
    for (j = 0; j < directories[i].num_themes; j++) {
      if (strcmp(theme_name, directories[i].themes[j]) == 0) {
	gui_load_theme(directories[i].path, directories[i].themes[j]);
	return TRUE;
      }
    }
  }
  return FALSE;
}

/****************************************************************************
  Wrapper for load_theme. It's is used by local options dialog
****************************************************************************/
void theme_reread_callback(struct client_option *option)
{
  assert(option->string.pvalue && *option->string.pvalue!= '\0');
  load_theme(option->string.pvalue);
}
