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
#include <sys/types.h>
#include <unistd.h>

#include <gtk/gtk.h>

/* utility */
#include "fc_dirent.h"
#include "mem.h"
#include "string_vector.h"

/* client */
#include "themes_common.h"

/* gui-gtk-3.0 */
#include "gui_main.h"

#include "themes_g.h"

/*************************************************************************//**
  Loads a gtk theme directory/theme_name
*****************************************************************************/
void gui_load_theme(const char *directory, const char *theme_name)
{
  static GtkCssProvider *fc_css_provider = NULL;
  GError *error = NULL;
  char buf[strlen(directory) + strlen(theme_name) + 32];

  if (fc_css_provider == NULL) {
    fc_css_provider = gtk_css_provider_new();
    gtk_style_context_add_provider(gtk_widget_get_style_context(toplevel),
        GTK_STYLE_PROVIDER(fc_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }

  /* Gtk theme is a directory containing gtk-3.0/gtk.css file */
  fc_snprintf(buf, sizeof(buf), "%s/%s/gtk-3.0/gtk.css", directory,
              theme_name);

  gtk_css_provider_load_from_file(fc_css_provider, g_file_new_for_path(buf), &error);

  if (error) {
    g_warning("%s\n", error->message);
  }

  gtk_style_context_invalidate(gtk_widget_get_style_context(toplevel));
}

/*************************************************************************//**
  Clears a theme (sets default system theme)
*****************************************************************************/
void gui_clear_theme(void)
{
  bool theme_loaded;

  /* try to load user defined theme */
  theme_loaded = load_theme(GUI_GTK_OPTION(default_theme_name));

  /* no user defined theme loaded -> try to load Freeciv default theme */
  if (!theme_loaded) {
    theme_loaded = load_theme(GUI_GTK_DEFAULT_THEME_NAME);
    if (theme_loaded) {
      sz_strlcpy(GUI_GTK_OPTION(default_theme_name), GUI_GTK_DEFAULT_THEME_NAME);
    }
  }

  /* still no theme loaded -> load system default theme */
  if (!theme_loaded) {
    static GtkCssProvider *default_provider = NULL;

    if (default_provider == NULL) {
      default_provider = gtk_css_provider_new();
    }
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(toplevel),
        GTK_STYLE_PROVIDER(default_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
}

/*************************************************************************//**
  Each gui has its own themes directories.
  For gtk3 these are:
  - /usr/share/themes
  - ~/.themes
  Returns an array containing these strings and sets array size in count.
  The caller is responsible for freeing the array and the paths.
*****************************************************************************/
char **get_gui_specific_themes_directories(int *count)
{
  gchar *standard_dir;
  char *home_dir;
  const struct strvec *data_dirs = get_data_dirs();
  char **directories = fc_malloc((2 + strvec_size(data_dirs))
                                 * sizeof(char *));

  *count = 0;

  /* Freeciv-specific GTK3 themes directories */
  strvec_iterate(data_dirs, dir_name) {
    char buf[strlen(dir_name) + strlen("/themes/gtk3") + 1];

    fc_snprintf(buf, sizeof(buf), "%s/themes/gtk3", dir_name);

    directories[(*count)++] = fc_strdup(buf);
  } strvec_iterate_end;

  /* standard GTK+ themes directory */
#ifdef CROSSER
  standard_dir = "../share/themes";
#else  /* CROSSER */
  standard_dir = "/usr/share/themes";
#endif /* CROSSER */
  directories[(*count)++] = fc_strdup(standard_dir);

  /* user GTK+ themes directory (~/.themes) */
  home_dir = user_home_dir();
  if (home_dir) {
    char buf[strlen(home_dir) + 16];

    fc_snprintf(buf, sizeof(buf), "%s/.themes/", home_dir);
    directories[(*count)++] = fc_strdup(buf);
  }

  return directories;
}

/*************************************************************************//**
  Return an array of names of usable themes in the given directory.
  Array size is stored in count.
  Useable theme for gtk+ is a directory which contains file gtk-3.0/gtk.css.
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
                "%s/%s/gtk-3.0/gtk.css", directory, entry->d_name);

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
