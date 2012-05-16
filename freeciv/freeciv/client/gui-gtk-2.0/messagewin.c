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
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "events.h"
#include "fcintl.h"
#include "game.h"
#include "map.h"
#include "mem.h"
#include "packets.h"
#include "player.h"
#include "chatline.h"
#include "citydlg.h"
#include "colors.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "options.h"

#include "messagewin.h"

static struct gui_dialog *meswin_shell;
static GtkListStore *meswin_store;
static GtkTreeSelection *meswin_selection;

static void create_meswin_dialog(void);
static void meswin_selection_callback(GtkTreeSelection *selection,
                                      gpointer data);
static void meswin_row_activated_callback(GtkTreeView *view,
					  GtkTreePath *path,
					  GtkTreeViewColumn *col,
					  gpointer data);
static void meswin_response_callback(struct gui_dialog *dlg, int response,
                                     gpointer data);

enum {
  CMD_GOTO = 1, CMD_POPCITY
};

#define N_MSG_VIEW 24	       /* max before scrolling happens */

/****************************************************************
popup the dialog 10% inside the main-window, and optionally
raise it.
*****************************************************************/
void popup_meswin_dialog(bool raise)
{
  if (!meswin_shell) {
    create_meswin_dialog();
  }

  update_meswin_dialog();

  gui_dialog_present(meswin_shell);
  if (raise) {
    gui_dialog_raise(meswin_shell);
  }
}

/**************************************************************************
 Closes the message window dialog.
**************************************************************************/
void popdown_meswin_dialog(void)
{
  if (meswin_shell) {
    gui_dialog_destroy(meswin_shell);
  }
}

/****************************************************************
...
*****************************************************************/
bool is_meswin_open(void)
{
  return (meswin_shell != NULL);
}

/****************************************************************
...
*****************************************************************/
static void meswin_set_visited(GtkTreeIter *it, bool visited)
{
  GtkListStore *store;
  gint row;

  store = meswin_store;
  g_return_if_fail(store != NULL);

  gtk_list_store_set(store, it, 1, visited, -1);
  gtk_tree_model_get(GTK_TREE_MODEL(store), it, 2, &row, -1);
  set_message_visited_state(row, visited);
}

/****************************************************************
...
*****************************************************************/
static void meswin_cell_data_func(GtkTreeViewColumn *col,
				  GtkCellRenderer *cell,
				  GtkTreeModel *model, GtkTreeIter *it,
				  gpointer data)
{
  gboolean b;

  gtk_tree_model_get(model, it, 1, &b, -1);

  if (b) {
    g_object_set(G_OBJECT(cell), "style", PANGO_STYLE_ITALIC,
		 "weight", PANGO_WEIGHT_NORMAL, NULL);
  } else {
    g_object_set(G_OBJECT(cell), "style", PANGO_STYLE_NORMAL,
		 "weight", PANGO_WEIGHT_BOLD, NULL);
  }
}
					     
/**************************************************************************
  Mouse button press handler for the message window treeview. We only
  care about right clicks on a row; this action centers on the tile
  associated with the event at that row (if applicable).
**************************************************************************/
static gboolean meswin_button_press_callback(GtkWidget *widget,
                                             GdkEventButton *ev,
                                             gpointer data)
{
  GtkTreePath *path = NULL;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint row;

  g_return_val_if_fail(GTK_IS_TREE_VIEW(widget), FALSE);

  if (ev->type != GDK_BUTTON_PRESS || ev->button != 3) {
    return FALSE;
  }

  if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                                     (gint) ev->x, (gint) ev->y,
                                     &path, NULL, NULL, NULL)) {
    return TRUE;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gtk_tree_model_get(model, &iter, 2, &row, -1);
    meswin_goto(row);
  }
  gtk_tree_path_free(path);

  return TRUE;
}

/****************************************************************
...
*****************************************************************/
static void create_meswin_dialog(void)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkWidget *view, *sw, *cmd, *notebook;

  if (gui_gtk2_split_bottom_notebook) {
    notebook = right_notebook;
  } else {
    notebook = bottom_notebook;
  }
  gui_dialog_new(&meswin_shell, GTK_NOTEBOOK(notebook), NULL);
  gui_dialog_set_title(meswin_shell, _("Messages"));

  meswin_store = gtk_list_store_new(3, G_TYPE_STRING,
                                    G_TYPE_BOOLEAN,
                                    G_TYPE_INT);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                          GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(meswin_shell->vbox), sw, TRUE, TRUE, 0);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(meswin_store));
  meswin_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_object_unref(meswin_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, renderer,
  	"text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  gtk_tree_view_column_set_cell_data_func(col, renderer,
	meswin_cell_data_func, NULL, NULL);
  gtk_container_add(GTK_CONTAINER(sw), view);

  g_signal_connect(meswin_selection, "changed",
		   G_CALLBACK(meswin_selection_callback), NULL);
  g_signal_connect(view, "row_activated",
		   G_CALLBACK(meswin_row_activated_callback), NULL);
  g_signal_connect(view, "button-press-event",
                   G_CALLBACK(meswin_button_press_callback), NULL);

  if (gui_gtk2_show_message_window_buttons) {
    cmd = gui_dialog_add_stockbutton(meswin_shell, GTK_STOCK_JUMP_TO,
                                     _("Goto _Location"), CMD_GOTO);
    gtk_widget_set_sensitive(cmd, FALSE);

    cmd = gui_dialog_add_stockbutton(meswin_shell, GTK_STOCK_ZOOM_IN,
                                     _("Inspect _City"), CMD_POPCITY);
    gtk_widget_set_sensitive(cmd, FALSE);
  }

  gui_dialog_add_button(meswin_shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  gui_dialog_response_set_callback(meswin_shell, meswin_response_callback);
  gui_dialog_set_default_size(meswin_shell, 520, 300);

  gui_dialog_show_all(meswin_shell);
}

/**************************************************************************
...
**************************************************************************/
void real_update_meswin_dialog(void)
{
  int i, num, num_not_visited = 0;
  struct message *pmsg;
  GtkListStore *store;
  GtkTreeIter it;

  store = meswin_store;
  g_return_if_fail(store != NULL);

  gtk_list_store_clear(store);
  num = get_num_messages();

  for (i = 0; i < num; i++) {
    pmsg = get_message(i);

    if (gui_gtk2_new_messages_go_to_top) {
      gtk_list_store_prepend(store, &it);
    } else {
      gtk_list_store_append(store, &it);
    }
    gtk_list_store_set(store, &it, 0, pmsg->descr, 2, i, -1);
    meswin_set_visited(&it, pmsg->visited);

    if (!pmsg->visited) {
      num_not_visited++;
    }
  }

  gui_dialog_set_response_sensitive(meswin_shell, CMD_GOTO, FALSE);
  gui_dialog_set_response_sensitive(meswin_shell, CMD_POPCITY, FALSE);

  if (num_not_visited > 0) {
    gui_dialog_alert(meswin_shell);
  }
}

/**************************************************************************
...
**************************************************************************/
static void meswin_selection_callback(GtkTreeSelection *selection,
				      gpointer data)
{
  struct message *pmsg;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gint row;

  if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
    return;
  }

  gtk_tree_model_get(model, &iter, 2, &row, -1);
  pmsg = get_message(row);

  if (pmsg) {
    gui_dialog_set_response_sensitive(meswin_shell, CMD_GOTO,
                                      pmsg->location_ok);
    gui_dialog_set_response_sensitive(meswin_shell, CMD_POPCITY,
                                      pmsg->city_ok);
  }
}

/**************************************************************************
...
**************************************************************************/
static void meswin_row_activated_callback(GtkTreeView *view,
					  GtkTreePath *path,
					  GtkTreeViewColumn *col,
					  gpointer data)
{
  struct message *pmsg;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint row;

  model = gtk_tree_view_get_model(view);
  if (!gtk_tree_model_get_iter(model, &iter, path)) {
    return;
  }

  gtk_tree_model_get(model, &iter, 2, &row, -1);
  pmsg = get_message(row);

  meswin_double_click(row);
  meswin_set_visited(&iter, TRUE);

  gui_dialog_set_response_sensitive(meswin_shell, CMD_GOTO,
                                    pmsg->location_ok);
  gui_dialog_set_response_sensitive(meswin_shell, CMD_POPCITY,
                                    pmsg->city_ok);
}

/**************************************************************************
...
**************************************************************************/
static void meswin_response_callback(struct gui_dialog *dlg, int response,
                                     gpointer data)
{
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint row;

  if (response != CMD_GOTO && response != CMD_POPCITY) {
    gui_dialog_destroy(dlg);
    return;
  }

  sel = meswin_selection;
  g_return_if_fail(sel != NULL);
  if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
    return;
  }

  gtk_tree_model_get(model, &iter, 2, &row, -1);

  if (response == CMD_GOTO) {
    meswin_goto(row);
  } else {
    meswin_popup_city(row);
  }
  meswin_set_visited(&iter, TRUE);
}

