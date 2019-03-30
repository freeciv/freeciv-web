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

#include <stdio.h>

#include <gtk/gtk.h>  

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"

/* client/gui-gtk-4.0 */
#include "gui_main.h"
#include "gui_stuff.h"

#include "inputdlg.h"

struct input_dialog_data {
  input_dialog_callback_t response_callback;
  gpointer response_cli_data;
};

/**********************************************************************//**
  Called when user dismisses dialog -- either to accept or to cancel.
**************************************************************************/
static void input_dialog_response(GtkDialog *shell, gint response,
                                  gpointer data)
{
  GtkWidget *winput = g_object_get_data(G_OBJECT(shell), "iinput");
  struct input_dialog_data *cb = data;

  cb->response_callback(cb->response_cli_data,
                        response, gtk_entry_get_text(GTK_ENTRY(winput)));

  /* Any response is final */
  gtk_widget_destroy(GTK_WIDGET(shell));
  FC_FREE(cb);
}

/**********************************************************************//**
  Create a popup with a text entry box and "OK" and "Cancel" buttons.
**************************************************************************/
GtkWidget *input_dialog_create(GtkWindow *parent, const char *dialogname, 
                               const char *text, const char *postinputtest,
                               input_dialog_callback_t response_callback,
                               gpointer response_cli_data)
{
  GtkWidget *shell, *label, *input;
  struct input_dialog_data *cb = fc_malloc(sizeof(struct input_dialog_data));

  cb->response_callback = response_callback;
  cb->response_cli_data = response_cli_data;

  shell = gtk_dialog_new_with_buttons(dialogname,
                                      parent,
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      _("Cancel"), GTK_RESPONSE_CANCEL,
                                      _("OK"), GTK_RESPONSE_OK,
                                      NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_OK);
  setup_dialog(shell, GTK_WIDGET(parent));
  g_signal_connect(shell, "response", G_CALLBACK(input_dialog_response), cb);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);

  label = gtk_frame_new(text);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(shell))),
                     label);

  input = gtk_entry_new();
  gtk_container_add(GTK_CONTAINER(label), input);
  gtk_entry_set_text(GTK_ENTRY(input), postinputtest);
  gtk_entry_set_activates_default(GTK_ENTRY(input), TRUE);
  g_object_set_data(G_OBJECT(shell), "iinput", input);

  gtk_widget_show(GTK_WIDGET(shell));
  gtk_window_present(GTK_WINDOW(shell));

  return shell;
}
