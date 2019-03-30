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

/* client */
#include "audio.h"
#include "client_main.h"

/* client/gui-gtk-3.0 */
#include "gui_main.h"
#include "gui_stuff.h"

#include "dialogs_g.h"

static void soundset_suggestion_callback(GtkWidget *dlg, gint arg);
static void musicset_suggestion_callback(GtkWidget *dlg, gint arg);

/************************************************************************//**
  Callback either loading suggested soundset or doing nothing
****************************************************************************/
static void soundset_suggestion_callback(GtkWidget *dlg, gint arg)
{
  if (arg == GTK_RESPONSE_YES) {
    /* User accepted soundset loading */
    audio_restart(game.control.preferred_soundset,
                  music_set_name);
  }
}

/************************************************************************//**
  Popup dialog asking if ruleset suggested soundset should be
  used.
****************************************************************************/
void popup_soundset_suggestion_dialog(void)
{
  GtkWidget *dialog, *label;
  char buf[1024];

  dialog = gtk_dialog_new_with_buttons(_("Preferred soundset"),
                                       NULL,
                                       0,
                                       _("Load soundset"),
                                       GTK_RESPONSE_YES,
                                       _("Keep current soundset"),
                                       GTK_RESPONSE_NO,
                                       NULL);
  setup_dialog(dialog, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

  fc_snprintf(buf, sizeof(buf),
              _("Modpack suggests using %s soundset.\n"
                "It might not work with other soundsets.\n"
                "You are currently using soundset %s."),
              game.control.preferred_soundset, sound_set_name);

  label = gtk_label_new(buf);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  gtk_widget_show(label);

  g_signal_connect(dialog, "response",
                   G_CALLBACK(soundset_suggestion_callback), NULL);

  /* In case incoming rulesets are incompatible with current soundset
   * we need to block their receive before user has accepted loading
   * of the correct soundset. */
  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
}

/************************************************************************//**
  Callback either loading suggested musicset or doing nothing
****************************************************************************/
static void musicset_suggestion_callback(GtkWidget *dlg, gint arg)
{
  if (arg == GTK_RESPONSE_YES) {
    /* User accepted musicset loading */
    audio_restart(sound_set_name, game.control.preferred_musicset);
  }
}

/************************************************************************//**
  Popup dialog asking if ruleset suggested musicset should be
  used.
****************************************************************************/
void popup_musicset_suggestion_dialog(void)
{
  GtkWidget *dialog, *label;
  char buf[1024];

  dialog = gtk_dialog_new_with_buttons(_("Preferred musicset"),
                                       NULL,
                                       0,
                                       _("Load musicset"),
                                       GTK_RESPONSE_YES,
                                       _("Keep current musicset"),
                                       GTK_RESPONSE_NO,
                                       NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

  fc_snprintf(buf, sizeof(buf),
              _("Modpack suggests using %s musicset.\n"
                "It might not work with other musicsets.\n"
                "You are currently using musicset %s."),
              game.control.preferred_musicset, music_set_name);

  label = gtk_label_new(buf);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  gtk_widget_show(label);

  g_signal_connect(dialog, "response",
                   G_CALLBACK(musicset_suggestion_callback), NULL);

  /* In case incoming rulesets are incompatible with current musicset
   * we need to block their receive before user has accepted loading
   * of the correct soundset. */
  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
}
