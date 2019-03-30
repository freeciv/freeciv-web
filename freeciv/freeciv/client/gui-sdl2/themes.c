/***********************************************************************
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
#include <fc_config.h>
#endif

#ifdef FREECIV_HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <sys/stat.h>

/* utility */
#include "fc_dirent.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "string_vector.h"

/* client/gui-sdl2 */
#include "themespec.h"

#include "themes_common.h"
#include "themes_g.h"

/*************************************************************************//**
  Loads a gui-sdl2 theme directory/theme_name
*****************************************************************************/
void gui_load_theme(const char *directory, const char *theme_name)
{
  char buf[strlen(directory) + strlen(DIR_SEPARATOR) + strlen(theme_name)
           + strlen(DIR_SEPARATOR "theme") + 1];

  if (theme != NULL) {
    /* We don't support changing theme once it has been loaded */
    return;
  }

  /* free previous loaded theme, if any */
  theme_free(theme);
  theme = NULL;

  fc_snprintf(buf, sizeof(buf), "%s" DIR_SEPARATOR "%s" DIR_SEPARATOR "theme",
              directory, theme_name);

  themespec_try_read(buf);
  theme_load_sprites(theme);
}

/*************************************************************************//**
  Clears a theme (sets default system theme)
*****************************************************************************/
void gui_clear_theme(void)
{
  if (!load_theme(gui_options.gui_sdl2_default_theme_name)) {
    /* TRANS: No full stop after the URL, could cause confusion. */
    log_fatal(_("No Sdl2-client theme was found. For instructions on how to "
                "get one, please visit %s"), WIKI_URL);
    exit(EXIT_FAILURE);
  }
}

/*************************************************************************//**
  Each gui has its own themes directories.

  Returns an array containing these strings and sets array size in count.
  The caller is responsible for freeing the array and the paths.
*****************************************************************************/
char **get_gui_specific_themes_directories(int *count)
{
  const struct strvec *data_dirs = get_data_dirs();
  char **directories = fc_malloc(strvec_size(data_dirs) * sizeof(char *));
  int i = 0;

  *count = strvec_size(data_dirs);
  strvec_iterate(data_dirs, data_dir) {
    char buf[strlen(data_dir) + strlen("/themes/gui-sdl2") + 1];

    fc_snprintf(buf, sizeof(buf), "%s/themes/gui-sdl2", data_dir);

    directories[i++] = fc_strdup(buf);
  } strvec_iterate_end;

  return directories;
}

/*************************************************************************//**
  Return an array of names of usable themes in the given directory.
  Array size is stored in count.
  Useable theme for gui-sdl2 is a directory which contains file theme.themespec.
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

  dir = fc_opendir(directory);
  if (!dir) {
    /* This isn't directory or we can't list it */
    return theme_names;
  }

  while ((entry = readdir(dir))) {
    char buf[strlen(directory) + strlen(entry->d_name) + 32];
    struct stat stat_result;

    fc_snprintf(buf, sizeof(buf),
		"%s/%s/theme.themespec", directory, entry->d_name);

    if (fc_stat(buf, &stat_result) != 0) {
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

    theme_names[*count] = fc_strdup(entry->d_name);
    (*count)++;
  }

  closedir(dir);

  return theme_names;
}
