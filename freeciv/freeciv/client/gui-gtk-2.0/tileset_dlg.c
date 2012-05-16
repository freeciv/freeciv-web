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

#include "game.h"
#include "unitlist.h"

#include "tilespec.h"

#include "dialogs_g.h"

static void tileset_suggestion_callback(GtkWidget *dlg, gint arg);

/****************************************************************
  Callback either loading suggested tileset or doing nothing
*****************************************************************/
static void tileset_suggestion_callback(GtkWidget *dlg, gint arg)
{
  if (arg == GTK_RESPONSE_YES) {
    /* User accepted tileset loading */
    tilespec_reread(game.control.prefered_tileset);
  }
}

/****************************************************************
  Popup dialog asking if ruleset suggested tileset should be
  used.
*****************************************************************/
void popup_tileset_suggestion_dialog(void)
{
  GtkWidget *dialog, *label;
  char buf[1024];

  dialog = gtk_dialog_new_with_buttons(_("Prefered tileset"),
                                       NULL,
                                       0,
                                       _("Load tileset"),
                                       GTK_RESPONSE_YES,
                                       _("Keep current tileset"),
                                       GTK_RESPONSE_NO,
                                       NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

  sprintf(buf,
          _("Modpack suggest using %s tileset.\n"
            "It might not work with other tilesets.\n"
            "You are currently using tileset %s."),
          game.control.prefered_tileset, tileset_get_name(tileset));

  label = gtk_label_new(buf);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  gtk_widget_show(label);

  g_signal_connect(dialog, "response",
                   G_CALLBACK(tileset_suggestion_callback), NULL);

  /* In case incoming rulesets are incompatible with current tileset
   * we need to block their receive before user has accepted loading
   * of the correct tileset. */
  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
}
