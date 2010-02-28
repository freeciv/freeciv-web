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

#include <stdio.h>

#include <gtk/gtk.h>  

/* utility */
#include "fcintl.h"
#include "log.h"

/* gui-gtk-2.0 */
#include "gui_main.h"
#include "gui_stuff.h"

#include "inputdlg.h"

/****************************************************************
...
*****************************************************************/
const char *input_dialog_get_input(GtkWidget *button)
{
  const char *dp;
  GtkWidget *winput;
      
  winput=g_object_get_data(G_OBJECT(button->parent->parent->parent),
	"iinput");
  
  dp=gtk_entry_get_text(GTK_ENTRY(winput));
 
  return dp;
}


/****************************************************************
...
*****************************************************************/
void input_dialog_destroy(GtkWidget *button)
{
  gtk_widget_destroy(button->parent->parent->parent);
}


/****************************************************************
...
*****************************************************************/
GtkWidget *input_dialog_create(GtkWindow *parent, const char *dialogname, 
			       const char *text, const char *postinputtest,
			       GCallback ok_callback, gpointer ok_cli_data, 
			       GCallback cancel_callback,
			       gpointer cancel_cli_data)
{
  GtkWidget *shell, *label, *input, *ok, *cancel;
  
  shell = gtk_dialog_new_with_buttons(dialogname,
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        NULL);
  setup_dialog(shell, GTK_WIDGET(parent));
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);
  label = gtk_frame_new(text);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(shell)->vbox), label, TRUE, TRUE, 0);

  input = gtk_entry_new();
  gtk_container_add(GTK_CONTAINER(label), input);
  gtk_entry_set_text(GTK_ENTRY(input), postinputtest);

  g_signal_connect(input, "activate", ok_callback, ok_cli_data);

  cancel = gtk_dialog_add_button(GTK_DIALOG(shell), GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL);
  ok = gtk_dialog_add_button(GTK_DIALOG(shell), GTK_STOCK_OK,
    GTK_RESPONSE_OK);

  g_signal_connect(ok, "clicked", ok_callback, ok_cli_data);
  g_signal_connect(cancel, "clicked", cancel_callback, cancel_cli_data);

  gtk_widget_grab_focus(input);

  g_object_set_data(G_OBJECT(shell), "iinput", input);

  gtk_widget_show_all(GTK_DIALOG(shell)->vbox);
  gtk_widget_show_all(GTK_DIALOG(shell)->action_area);
  gtk_window_present(GTK_WINDOW(shell));

  return shell;
}
