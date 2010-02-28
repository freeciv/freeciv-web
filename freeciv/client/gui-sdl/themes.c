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

#include <dirent.h>
#include <sys/stat.h>

/* utility */
#include "fcintl.h"
#include "log.h"

/* gui-sdl */
#include "themespec.h"

#include "themes_common.h"
#include "themes_g.h"

/*****************************************************************************
  Loads a gui-sdl theme directory/theme_name
*****************************************************************************/
void gui_load_theme(const char *directory, const char *theme_name)
{
  char buf[strlen(directory) + strlen("/") + strlen(theme_name) + strlen("/theme") + 1];

  /* free previous loaded theme, if any */
  theme_free(theme);
  
  my_snprintf(buf, sizeof(buf), "%s/%s/theme", directory, theme_name);
  
  themespec_try_read(buf);
  theme_load_sprites(theme);
}

/*****************************************************************************
  Clears a theme (sets default system theme)
*****************************************************************************/
void gui_clear_theme(void)
{
  theme_free(theme);
  if (!load_theme(default_theme_name)) {
    freelog(LOG_FATAL,
            /* TRANS: No full stop after the URL, could cause confusion. */
            _("No gui-sdl theme was found. For instructions on how to get one,"
              " please visit %s"),
            WIKI_URL);
    exit(EXIT_FAILURE);
  }
}

/*****************************************************************************
  Each gui has its own themes directories.

  Returns an array containing these strings and sets array size in count.
  The caller is responsible for freeing the array and the paths.
*****************************************************************************/
char **get_gui_specific_themes_directories(int *count)
{
  int i;

  const char **data_directories = get_data_dirs(count);

  char **directories = fc_malloc(sizeof(char *) * *count);  

  for (i = 0; i < *count; i++) {
    char buf[strlen(data_directories[i]) + strlen("/themes/gui-sdl") + 1];
    
    my_snprintf(buf, sizeof(buf), "%s/themes/gui-sdl", data_directories[i]);

    directories[i] = mystrdup(buf);
  }

  return directories;
}

/*****************************************************************************
  Return an array of names of usable themes in the given directory.
  Array size is stored in count.
  Useable theme for gui-sdl is a directory which contains file theme.themespec.
  The caller is responsible for freeing the array and the names
*****************************************************************************/
char **get_useable_themes_in_directory(const char *directory, int *count)
{
  DIR *dir;
  struct dirent *entry;
  
  char **theme_names = fc_malloc(sizeof(char *) * 2);
  /* Allocated memory size */
  int t_size = 2;

  *count = 0;

  dir = opendir(directory);
  if (!dir) {
    /* This isn't directory or we can't list it */
    return theme_names;
  }

  while ((entry = readdir(dir))) {
    char buf[strlen(directory) + strlen(entry->d_name) + 32];
    struct stat stat_result;

    my_snprintf(buf, sizeof(buf),
		"%s/%s/theme.themespec", directory, entry->d_name);
    
    if (stat(buf, &stat_result) != 0) {
      /* File doesn't exist */
      continue;
    }
    
    if (!S_ISREG(stat_result.st_mode)) {
      /* Not a regular file */
      continue;
    }
    
    /* Otherwise it's ok */

    /* Increase array size if needed */
    if (*count == t_size) {
      theme_names = fc_realloc(theme_names, t_size * 2 * sizeof(char *));
      t_size *= 2;
    }

    theme_names[*count] = mystrdup(entry->d_name);
    (*count)++;
  }

  closedir(dir);

  return theme_names;
}
