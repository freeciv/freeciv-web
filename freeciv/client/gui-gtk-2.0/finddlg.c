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

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "player.h"

/* client */
#include "options.h"

/* gui-gtk-2.0 */
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"

#include "finddlg.h"

static struct gui_dialog *find_dialog_shell;
static GtkWidget *find_view;

static void update_find_dialog(GtkListStore *store);

static void find_response(struct gui_dialog *dlg, int response, gpointer data);
static void find_destroy_callback(GtkWidget *w, gpointer data);
static void find_selection_callback(GtkTreeSelection *selection,
				    GtkTreeModel *model);

static struct tile *pos;

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_find_dialog(void)
{
  if (!find_dialog_shell) {
    GtkWidget         *label;
    GtkWidget         *sw;
    GtkListStore      *store;
    GtkTreeSelection  *selection;
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;

    pos = get_center_tile_mapcanvas();

    gui_dialog_new(&find_dialog_shell, GTK_NOTEBOOK(bottom_notebook), NULL);
    gui_dialog_set_title(find_dialog_shell, _("Find City"));
    gui_dialog_set_default_size(find_dialog_shell, -1, 240);

    gui_dialog_add_button(find_dialog_shell,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    gui_dialog_add_button(find_dialog_shell,
	GTK_STOCK_FIND, GTK_RESPONSE_ACCEPT);

    gui_dialog_set_default_response(find_dialog_shell, GTK_RESPONSE_ACCEPT);

    gui_dialog_response_set_callback(find_dialog_shell, find_response);

    g_signal_connect(find_dialog_shell->vbox, "destroy",
	G_CALLBACK(find_destroy_callback), NULL);

    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
	0, GTK_SORT_ASCENDING);

    find_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(find_view));
    g_object_unref(store);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(find_view));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(find_view), FALSE);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
	"text", 0, NULL);
    gtk_tree_view_column_set_sort_order(column, GTK_SORT_ASCENDING);
    gtk_tree_view_append_column(GTK_TREE_VIEW(find_view), column);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
	GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(sw), find_view);

    label = g_object_new(GTK_TYPE_LABEL,
	"use-underline", TRUE,
	"mnemonic-widget", find_view,
	"label", _("Ci_ties:"),
	"xalign", 0.0, "yalign", 0.5, NULL);
    gtk_box_pack_start(GTK_BOX(find_dialog_shell->vbox), label,
	FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(find_dialog_shell->vbox), sw,
	TRUE, TRUE, 2);

    g_signal_connect(selection, "changed",
	G_CALLBACK(find_selection_callback), store);

    update_find_dialog(store);
    gtk_tree_view_focus(GTK_TREE_VIEW(find_view));

    gui_dialog_show_all(find_dialog_shell);
  }

  gui_dialog_raise(find_dialog_shell);
}



/**************************************************************************
...
**************************************************************************/
static void update_find_dialog(GtkListStore *store)
{
  GtkTreeIter it;

  gtk_list_store_clear(store);

  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
	GValue value = { 0, };

	gtk_list_store_append(store, &it);

	g_value_init(&value, G_TYPE_STRING);
	g_value_set_static_string(&value, city_name(pcity));
	gtk_list_store_set_value(store, &it, 0, &value);
	g_value_unset(&value);

	gtk_list_store_set(store, &it, 1, pcity, -1);
    } city_list_iterate_end;
  } players_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
static void find_response(struct gui_dialog *dlg, int response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT) {
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(find_view));

    if (gtk_tree_selection_get_selected(selection, &model, &it)) {
      struct city *pcity;

      gtk_tree_model_get(model, &it, 1, &pcity, -1);

      if (pcity) {
	pos = pcity->tile;
      }
    }
  }
  gui_dialog_destroy(dlg);
}

/**************************************************************************
...
**************************************************************************/
static void find_destroy_callback(GtkWidget *w, gpointer data)
{
  can_slide = FALSE;
  center_tile_mapcanvas(pos);
  can_slide = TRUE;
}

/**************************************************************************
...
**************************************************************************/
static void find_selection_callback(GtkTreeSelection *selection,
				    GtkTreeModel *model)
{
  GtkTreeIter it;
  struct city *pcity;

  if (!gtk_tree_selection_get_selected(selection, NULL, &it))
    return;

  gtk_tree_model_get(model, &it, 1, &pcity, -1);

  if (pcity) {
    can_slide = FALSE;
    center_tile_mapcanvas(pcity->tile);
    can_slide = TRUE;
  }
}
