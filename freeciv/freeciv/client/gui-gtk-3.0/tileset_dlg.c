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

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"

/* common */
#include "game.h"
#include "unitlist.h"

/* client */
#include "tilespec.h"

/* client/gui-gtk-3.0 */
#include "gui_main.h"
#include "gui_stuff.h"

#include "dialogs_g.h"
extern char forced_tileset_name[512];
static void tileset_suggestion_callback(GtkWidget *dlg, gint arg);

/************************************************************************//**
  Callback either loading suggested tileset or doing nothing
****************************************************************************/
static void tileset_suggestion_callback(GtkWidget *dlg, gint arg)
{
  if (arg == GTK_RESPONSE_YES) {
    /* User accepted tileset loading */
    sz_strlcpy(forced_tileset_name, game.control.preferred_tileset);
    if (!tilespec_reread(game.control.preferred_tileset, FALSE, 1.0)) {
      tileset_error(LOG_ERROR, _("Can't load requested tileset %s."),
                    game.control.preferred_tileset);
    }
  }
}

/************************************************************************//**
  Popup dialog asking if ruleset suggested tileset should be
  used.
****************************************************************************/
void popup_tileset_suggestion_dialog(void)
{
  GtkWidget *dialog, *label;
  char buf[1024];

  dialog = gtk_dialog_new_with_buttons(_("Preferred tileset"),
                                       NULL,
                                       0,
                                       _("Load tileset"),
                                       GTK_RESPONSE_YES,
                                       _("Keep current tileset"),
                                       GTK_RESPONSE_NO,
                                       NULL);
  setup_dialog(dialog, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

  fc_snprintf(buf, sizeof(buf),
              _("Modpack suggests using %s tileset.\n"
                "It might not work with other tilesets.\n"
                "You are currently using tileset %s."),
              game.control.preferred_tileset, tileset_basename(tileset));

  label = gtk_label_new(buf);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
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
