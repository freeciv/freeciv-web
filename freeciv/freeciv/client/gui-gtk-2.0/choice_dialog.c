/********************************************************************** 
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
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

#include <stdarg.h>

#include <gtk/gtk.h>

#include "support.h"

#include "gui_main.h"
#include "gui_stuff.h"

#include "choice_dialog.h"

/****************************************************************
  Choice dialog: A dialog with a label and a list of buttons
  placed vertically
****************************************************************/

/****************************************************************
...
*****************************************************************/
static GtkWidget* choice_dialog_get_nth_button(GtkWidget *cd,
                                               int button)
{
  char button_name[512];
  GtkWidget *b;

  my_snprintf(button_name, sizeof(button_name), "button%d", button);

  b = g_object_get_data(G_OBJECT(cd), button_name);

  return b;
}

/****************************************************************
...
*****************************************************************/
void choice_dialog_button_set_sensitive(GtkWidget *cd, int button,
					 gboolean state)
{
  gtk_widget_set_sensitive(choice_dialog_get_nth_button(cd, button), state);
}

/****************************************************************
...
*****************************************************************/
void choice_dialog_button_set_label(GtkWidget *cd, int number,
                                    const char* label)
{
  GtkWidget* button = choice_dialog_get_nth_button(cd, number);
  gtk_button_set_label(GTK_BUTTON(button), label);
}

/****************************************************************
...
*****************************************************************/
GtkWidget *choice_dialog_start(GtkWindow *parent, const gchar *name,
				const gchar *text)
{
  GtkWidget *dshell, *dlabel, *vbox, *bbox;

  dshell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  setup_dialog(dshell, toplevel);
  gtk_window_set_position (GTK_WINDOW(dshell), GTK_WIN_POS_MOUSE);

  gtk_window_set_title(GTK_WINDOW(dshell), name);

  gtk_window_set_transient_for(GTK_WINDOW(dshell), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dshell), TRUE);

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(dshell),vbox);

  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

  dlabel = gtk_label_new(text);
  gtk_container_add(GTK_CONTAINER(vbox), dlabel);

  bbox = gtk_vbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(bbox), 2);
  gtk_container_add(GTK_CONTAINER(vbox), bbox);
  
  g_object_set_data(G_OBJECT(dshell), "bbox", bbox);
  g_object_set_data(G_OBJECT(dshell), "nbuttons", GINT_TO_POINTER(0));
  g_object_set_data(G_OBJECT(dshell), "hide", GINT_TO_POINTER(FALSE));
  
  gtk_widget_show(vbox);
  gtk_widget_show(dlabel);
  
  return dshell;
}

/****************************************************************
...
*****************************************************************/
static void choice_dialog_clicked(GtkWidget *w, gpointer data)
{
  if (g_object_get_data(G_OBJECT(data), "hide")) {
    gtk_widget_hide(GTK_WIDGET(data));
  } else {
    gtk_widget_destroy(GTK_WIDGET(data));
  }
}

/****************************************************************
...
*****************************************************************/
void choice_dialog_add(GtkWidget *dshell, const gchar *label,
			GCallback handler, gpointer data)
{
  GtkWidget *button, *bbox;
  char name[512];
  int nbuttons;

  bbox = g_object_get_data(G_OBJECT(dshell), "bbox");
  nbuttons = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dshell), "nbuttons"));
  g_object_set_data(G_OBJECT(dshell), "nbuttons", GINT_TO_POINTER(nbuttons+1));

  my_snprintf(name, sizeof(name), "button%d", nbuttons);

  button = gtk_button_new_from_stock(label);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_object_set_data(G_OBJECT(dshell), name, button);

  if (handler) {
    g_signal_connect(button, "clicked", handler, data);
  }

  g_signal_connect_after(button, "clicked",
			 G_CALLBACK(choice_dialog_clicked), dshell);
}

/****************************************************************
...
*****************************************************************/
void choice_dialog_end(GtkWidget *dshell)
{
  GtkWidget *bbox;
  
  bbox = g_object_get_data(G_OBJECT(dshell), "bbox");
  
  gtk_widget_show_all(bbox);
  gtk_widget_show(dshell);  
}

/****************************************************************
...
*****************************************************************/
void choice_dialog_set_hide(GtkWidget *dshell, gboolean setting)
{
  g_object_set_data(G_OBJECT(dshell), "hide", GINT_TO_POINTER(setting));
}

/****************************************************************
...
*****************************************************************/
GtkWidget *popup_choice_dialog(GtkWindow *parent, const gchar *dialogname,
				const gchar *text, ...)
{
  GtkWidget *dshell;
  va_list args;
  gchar *name;
  int i;

  dshell = choice_dialog_start(parent, dialogname, text);
  
  i = 0;
  va_start(args, text);

  while ((name = va_arg(args, gchar *))) {
    GCallback handler;
    gpointer data;

    handler = va_arg(args, GCallback);
    data = va_arg(args, gpointer);

    choice_dialog_add(dshell, name, handler, data);
  }

  va_end(args);

  choice_dialog_end(dshell);

  return dshell;
}
