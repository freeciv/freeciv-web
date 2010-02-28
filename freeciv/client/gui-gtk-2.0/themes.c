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
#include <sys/types.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "mem.h"
#include "support.h"

#include "gui_main.h"

#include "themes_common.h"
#include "themes_g.h"

/* Array of default files. First num_default_files positions
 * are files returned by gtk_rc_get_default_files() on client startup.
 * There are two extra postions allocated in the array - one for
 * specific Freeciv file and one for NULL. */
static char** default_files;
static int num_default_files;

/****************************************************************************
  Initialize the default_files array.
****************************************************************************/
static void load_default_files()
{
  int i;
  gchar** f;
  
  if (default_files != NULL) {
    return;
  }
  
  f = gtk_rc_get_default_files();
  
  for (i = 0; f[i] ; i++) {
    /* nothing */
  }
  num_default_files = i;
  default_files = fc_malloc(sizeof(char*) * (i + 2));
  
  for (i = 0; i < num_default_files; i++) {
    default_files[i] = mystrdup(f[i]);
  }
  default_files[i] = default_files[i + 1] = NULL;
}

/*****************************************************************************
  Loads a gtk theme directory/theme_name
*****************************************************************************/
void gui_load_theme(const char *directory, const char *theme_name)
{
  GtkStyle *style;
  char buf[strlen(directory) + strlen(theme_name) + 32];
  
  load_default_files();
  default_files[num_default_files] = buf;
  default_files[num_default_files + 1] = NULL;
  
  /* Gtk theme is a directory containing gtk-2.0/gtkrc file */
  my_snprintf(buf, sizeof(buf), "%s/%s/gtk-2.0/gtkrc", directory,
	      theme_name);

  gtk_rc_set_default_files(default_files);

  gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);
    
  /* the turn done button must have its own style. otherwise when we flash
     the turn done button other widgets may flash too. */
  if (!(style = gtk_rc_get_style(turn_done_button))) {
    style = turn_done_button->style;
  }
  gtk_widget_set_style(turn_done_button, gtk_style_copy(style));
}

/*****************************************************************************
  Clears a theme (sets default system theme)
*****************************************************************************/
void gui_clear_theme(void)
{
  GtkStyle *style;
  bool theme_loaded;

  /* try to load user defined theme */
  theme_loaded = load_theme(default_theme_name);

  /* no user defined theme loaded -> try to load Freeciv default theme */
  if (!theme_loaded) {
    theme_loaded = load_theme(FC_GTK_DEFAULT_THEME_NAME);
    if (theme_loaded) {
      sz_strlcpy(default_theme_name, FC_GTK_DEFAULT_THEME_NAME);
    }
  }
    
  /* still no theme loaded -> load system default theme */
  if (!theme_loaded) {
    load_default_files();
    default_files[num_default_files] = NULL;
    gtk_rc_set_default_files(default_files);
    gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);
      
    /* the turn done button must have its own style. otherwise when we flash
       the turn done button other widgets may flash too. */
    if (!(style = gtk_rc_get_style(turn_done_button))) {
      style = turn_done_button->style;
    }
    gtk_widget_set_style(turn_done_button, gtk_style_copy(style));
  }
}

/*****************************************************************************
  Each gui has its own themes directories.
  For gtk2 these are:
  - /usr/share/themes
  - ~/.themes
  Returns an array containing these strings and sets array size in count.
  The caller is responsible for freeing the array and the paths.
*****************************************************************************/
char **get_gui_specific_themes_directories(int *count)
{
  gchar *standard_dir;
  char *home_dir;
  int i;

  const char **data_directories = get_data_dirs(count);
    
  char **directories = fc_malloc(sizeof(char *) * (*count + 2));
    
  /* Freeciv-specific GTK+ themes directories */
  for (i = 0; i < *count; i++) {
    char buf[strlen(data_directories[i]) + strlen("/themes/gui-gtk-2.0") + 1];
    
    my_snprintf(buf, sizeof(buf), "%s/themes/gui-gtk-2.0", data_directories[i]);

    directories[i] = mystrdup(buf);
  }
    
  /* standard GTK+ themes directory (e.g. /usr/share/themes) */
  standard_dir = gtk_rc_get_theme_dir();
  directories[*count] = mystrdup(standard_dir);
  (*count)++;
  g_free(standard_dir);

  /* user GTK+ themes directory (~/.themes) */
  home_dir = user_home_dir();
  if (home_dir) {
    char buf[strlen(home_dir) + 16];
    
    my_snprintf(buf, sizeof(buf), "%s/.themes/", home_dir);
    directories[*count] = mystrdup(buf);
    (*count)++;
  }

  return directories;
}

/*****************************************************************************
  Return an array of names of usable themes in the given directory.
  Array size is stored in count.
  Useable theme for gtk+ is a directory which contains file gtk-2.0/gtkrc.
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
		"%s/%s/gtk-2.0/gtkrc", directory, entry->d_name);

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
