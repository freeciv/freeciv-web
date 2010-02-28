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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "fcintl.h"

#include "options.h"

#include "dialogs_g.h"

#include "gui_main.h"

static bool load_theme = FALSE;

static void theme_suggestion_callback(GtkWidget *dlg, gint arg);

/****************************************************************
  Callback deciding if the theme may be loaded or not
*****************************************************************/
static void theme_suggestion_callback(GtkWidget *dlg, gint arg)
{
  load_theme = (arg == GTK_RESPONSE_YES);
}

/****************************************************************
  Popup dialog asking if tileset suggested theme should be
  used.
*****************************************************************/
bool popup_theme_suggestion_dialog(const char *theme_name)
{
  GtkWidget *dialog, *label;
  char buf[1024];
  char *current_name = default_theme_name;

  if (current_name == NULL) {
    /* default_theme_name is not yet set.
     * This can happen when we load tileset requested at command line and
     * user has not saved theme information to .civclientrc. */
    current_name = FC_GTK_DEFAULT_THEME_NAME;
  }

  dialog = gtk_dialog_new_with_buttons(_("Theme suggested"),
                                       NULL,
                                       0,
                                       _("Load theme"),
                                       GTK_RESPONSE_YES,
                                       _("Keep current theme"),
                                       GTK_RESPONSE_NO,
                                       NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

  my_snprintf(buf, sizeof(buf),
              _("Tileset suggests using %s theme.\n"
              "You are currently using %s."),
              theme_name, current_name);

  label = gtk_label_new(buf);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  gtk_widget_show(label);

  g_signal_connect(dialog, "response",
                   G_CALLBACK(theme_suggestion_callback), NULL);

  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
  
  return load_theme;
}
