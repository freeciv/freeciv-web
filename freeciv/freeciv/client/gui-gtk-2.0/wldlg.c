/********************7************************************************** 
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "city.h"
#include "citydlg_common.h"
#include "client_main.h"
#include "fcintl.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mem.h"
#include "packets.h"
#include "worklist.h"
#include "support.h"
#include "climisc.h"
#include "options.h"
#include "tilespec.h"

#include "canvas.h"
#include "citydlg.h"
#include "graphics.h"
#include "wldlg.h"

static GtkWidget *worklists_shell;
static GtkWidget *worklists_list;

enum {
  WORKLISTS_NEW,
  WORKLISTS_DELETE,
  WORKLISTS_PROPERTIES,
  WORKLISTS_CLOSE
};

static GtkListStore *worklists_store;

static int max_unit_height = -1, max_unit_width = -1;

static void popup_worklist(struct worklist *pwl);
static void popdown_worklist(struct worklist *pwl);
static void dst_row_callback(GtkTreeView *view, GtkTreePath *path,
			     GtkTreeViewColumn *col, gpointer data);

/****************************************************************
...
*****************************************************************/
void blank_max_unit_size(void)
{
  max_unit_height = -1;
  max_unit_width = -1;
}

/****************************************************************
...
*****************************************************************/
static void update_max_unit_size(void)
{
  max_unit_height = 0;
  max_unit_width = 0;

  unit_type_iterate(i) {
    int x1, x2, y1, y2;
    struct sprite *sprite = get_unittype_sprite(tileset, i);

    sprite_get_bounding_box(sprite, &x1, &y1, &x2, &y2);
    max_unit_width = MAX(max_unit_width, x2 - x1);
    max_unit_height = MAX(max_unit_height, y2 - y1);
  } unit_type_iterate_end;
}


/****************************************************************
...
*****************************************************************/
static void worklists_destroy_callback(GtkWidget *w, gpointer data)
{
  worklists_shell = NULL;
}


/****************************************************************
...
*****************************************************************/
void update_worklist_report_dialog(void)
{
  int i;
  GtkTreeIter it;

  if (!worklists_shell) {
    return;
  }

  gtk_list_store_clear(worklists_store);

  for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
    if (client.worklists[i].is_valid) {
      gtk_list_store_append(worklists_store, &it);
      
       gtk_list_store_set(worklists_store, &it,
			  0, client.worklists[i].name,
			  1, i,
			  -1); 
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void worklists_response(GtkWidget *w, gint response)
{
  int i, pos;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;

  for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
    if (!client.worklists[i].is_valid) {
      break;
    }
  }
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(worklists_list));

  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    gtk_tree_model_get(model, &it, 1, &pos, -1);
  } else {
    pos = -1;
  }

 switch (response) {
    case WORKLISTS_NEW:
      /* No more worklist slots free.  (!!!Maybe we should tell the user?) */
      if (i == MAX_NUM_WORKLISTS) {
        return;
      }

      /* Validate this slot. */
      worklist_init(&client.worklists[i]);
      client.worklists[i].is_valid = TRUE;
      strcpy(client.worklists[i].name, _("new"));

      update_worklist_report_dialog();
      return;

    case WORKLISTS_DELETE:
      if (pos == -1) {
	return;
      }

      popdown_worklist(&client.worklists[pos]);

      client.worklists[pos].is_valid = FALSE;
      client.worklists[pos].name[0] = '\0';

      update_worklist_report_dialog();
      return;

    case WORKLISTS_PROPERTIES:
      if (pos == -1) {
	return;
      }

      popup_worklist(&client.worklists[pos]);
      return;

    default:
      gtk_widget_destroy(worklists_shell);
      return;
  }
}

/****************************************************************
...
*****************************************************************/
static void cell_edited(GtkCellRendererText *cell,
			const gchar *spath,
			const gchar *text, gpointer data)
{
  GtkTreePath *path;
  GtkTreeIter it;
  int pos;

  path = gtk_tree_path_new_from_string(spath);
  gtk_tree_model_get_iter(GTK_TREE_MODEL(worklists_store), &it, path);
  
  gtk_tree_model_get(GTK_TREE_MODEL(worklists_store), &it, 1, &pos, -1);

  sz_strlcpy(client.worklists[pos].name, text);
  gtk_list_store_set(worklists_store, &it, 0, text, -1);

  gtk_tree_path_free(path);
}

/****************************************************************
  Bring up the global worklist report.
*****************************************************************/
static GtkWidget *create_worklists_report(void)
{
  GtkWidget *shell, *list;
  GtkWidget *vbox, *label, *sw;
  GtkCellRenderer *rend;

  shell = gtk_dialog_new_with_buttons(_("Edit worklists"),
				      NULL,
				      0,
				      GTK_STOCK_NEW,
				      WORKLISTS_NEW,
				      GTK_STOCK_DELETE,
				      WORKLISTS_DELETE,
				      GTK_STOCK_PROPERTIES,
				      WORKLISTS_PROPERTIES,
				      GTK_STOCK_CLOSE,
				      WORKLISTS_CLOSE,
				      NULL);
  setup_dialog(shell, toplevel);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_MOUSE);
  
  g_signal_connect(shell, "response",
		   G_CALLBACK(worklists_response), NULL);
  g_signal_connect(shell, "destroy",
		   G_CALLBACK(worklists_destroy_callback), NULL);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), vbox);

  worklists_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

  list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(worklists_store));
  g_object_unref(worklists_store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);

  worklists_list = list;
  
  rend = gtk_cell_renderer_text_new();
  g_object_set(rend, "editable", TRUE, NULL);
  g_signal_connect(rend, "edited",
		   G_CALLBACK(cell_edited), NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(list), -1, NULL,
    rend, "text", 0, NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(sw), list);

  gtk_widget_set_size_request(sw, -1, 200);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", list,
		       "label", _("_Worklists:"),
		       "xalign", 0.0, "yalign", 0.5, NULL);

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
  gtk_widget_show_all(vbox);

  return shell;
}

/****************************************************************
...
*****************************************************************/
void popup_worklists_report(void)
{
  if (!worklists_shell) {
    worklists_shell = create_worklists_report();

    update_worklist_report_dialog();
  }

  gtk_window_present(GTK_WINDOW(worklists_shell));
}




/****************************************************************
...
*****************************************************************/
struct worklist_data {
  struct worklist *pwl;
  struct city *pcity;

  GtkWidget *editor;

  GtkListStore *src, *dst;
  GtkWidget *src_view, *dst_view;
  GtkTreeSelection *src_selection, *dst_selection;

  GtkTreeViewColumn *src_col, *dst_col;

  GtkWidget *add_cmd, *change_cmd, *help_cmd;
  GtkWidget *up_cmd, *down_cmd, *prepend_cmd, *append_cmd, *remove_cmd;

  bool future;
};

static GHashTable *hash;

static void commit_worklist(struct worklist_data *ptr);


enum {
  TARGET_GTK_TREE_MODEL_ROW
};

static GtkTargetEntry wl_dnd_targets[] = {
  { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_APP, TARGET_GTK_TREE_MODEL_ROW },
};



/****************************************************************
...
*****************************************************************/
void add_worklist_dnd_target(GtkWidget *w)
{
  gtk_drag_dest_set(w, GTK_DEST_DEFAULT_ALL,
		    wl_dnd_targets, G_N_ELEMENTS(wl_dnd_targets),
		    GDK_ACTION_COPY);
}

/****************************************************************
...
*****************************************************************/
static GtkWidget *get_worklist(struct worklist *pwl)
{
  if (hash) {
    gpointer ret;
    
    ret = g_hash_table_lookup(hash, pwl);
    return ret;
  } else {
    return NULL;
  }
}

/****************************************************************
...
*****************************************************************/
static void insert_worklist(struct worklist *pwl, GtkWidget *editor)
{
  if (!hash) {
    hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  }
  g_hash_table_insert(hash, pwl, editor);
}

/****************************************************************
...
*****************************************************************/
static void delete_worklist(struct worklist *pwl)
{
  if (hash) {
    g_hash_table_remove(hash, pwl);
  }
}

/****************************************************************
...
*****************************************************************/
static void worklist_response(GtkWidget *shell, gint response)
{
  gtk_widget_destroy(shell);
}

/****************************************************************
  Worklist editor window used by the global worklist report.
*****************************************************************/
static void popup_worklist(struct worklist *pwl)
{
  GtkWidget *shell;

  if (!(shell = get_worklist(pwl))) {
    GtkWidget *editor;
    
    shell = gtk_dialog_new_with_buttons(pwl->name,
					GTK_WINDOW(worklists_shell),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CLOSE,
					GTK_RESPONSE_CLOSE,
					NULL);
    gtk_window_set_role(GTK_WINDOW(shell), "worklist");
    gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_MOUSE);
    g_signal_connect(shell, "response", G_CALLBACK(worklist_response), NULL);
    gtk_window_set_default_size(GTK_WINDOW(shell), 500, 400);
  
    editor = create_worklist();
    reset_worklist(editor, pwl, NULL);
    insert_worklist(pwl, editor);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), editor);
    gtk_widget_show(editor);

    refresh_worklist(editor);
  }

  gtk_window_present(GTK_WINDOW(shell));
}

/****************************************************************
...
*****************************************************************/
static void popdown_worklist(struct worklist *pwl)
{
  GtkWidget *shell;
  
  if ((shell = get_worklist(pwl))) {
    GtkWidget *parent;

    parent = gtk_widget_get_toplevel(shell);
    gtk_widget_destroy(parent);
  }
}

/****************************************************************
...
*****************************************************************/
static void worklist_destroy(GtkWidget *editor, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;
  
  if (ptr->pwl) {
    delete_worklist(ptr->pwl);
  }

  free(ptr);
}

/****************************************************************
...
*****************************************************************/
static void menu_item_callback(GtkMenuItem *item, struct worklist_data *ptr)
{
  gint pos;
  struct worklist *pwl;

  if (NULL == client.conn.playing) {
    return;
  }

  pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "pos"));

  pwl = &client.worklists[pos];

  if (pwl->is_valid) {
    int i;

    for (i = 0; i < worklist_length(pwl); i++) {
      GtkTreeIter it;
      cid cid;

      cid = cid_encode(pwl->entries[i]);

      gtk_list_store_append(ptr->dst, &it);
      gtk_list_store_set(ptr->dst, &it, 0, (gint) cid, -1);
    }
  }

  commit_worklist(ptr);
}

/****************************************************************
...
*****************************************************************/
static void popup_add_menu(GtkMenuShell *menu, gpointer data)
{
  int i;
  GtkWidget *item;

  gtk_container_foreach(GTK_CONTAINER(menu),
			(GtkCallback) gtk_widget_destroy, NULL);

  for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
    if (client.worklists[i].is_valid) {
      item = gtk_menu_item_new_with_label(client.worklists[i].name);
      g_object_set_data(G_OBJECT(item), "pos", GINT_TO_POINTER(i));
      gtk_widget_show(item);

      gtk_container_add(GTK_CONTAINER(menu), item);
      g_signal_connect(item, "activate",
		       G_CALLBACK(menu_item_callback), data);
    }
  }

  item = gtk_separator_menu_item_new();
  gtk_widget_show(item);

  gtk_container_add(GTK_CONTAINER(menu), item);

  item = gtk_menu_item_new_with_mnemonic(_("Edit Global _Worklists"));
  gtk_widget_show(item);

  gtk_container_add(GTK_CONTAINER(menu), item);
  g_signal_connect(item, "activate",
  		   G_CALLBACK(popup_worklists_report), NULL);
}

/****************************************************************
...
*****************************************************************/
static void help_callback(GtkWidget *w, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;

  ptr = data;
  selection = ptr->src_selection;

  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    gint cid;
    struct universal target;

    gtk_tree_model_get(model, &it, 0, &cid, -1);
    target = cid_decode(cid);

    if (VUT_UTYPE == target.kind) {
      popup_help_dialog_typed(utype_name_translation(target.value.utype),
			      HELP_UNIT);
    } else if (is_great_wonder(target.value.building)) {
      popup_help_dialog_typed(improvement_name_translation(target.value.building),
			      HELP_WONDER);
    } else {
      popup_help_dialog_typed(improvement_name_translation(target.value.building),
			      HELP_IMPROVEMENT);
    }
  } else {
    popup_help_dialog_string(HELP_WORKLIST_EDITOR_ITEM);
  }
}

/****************************************************************
...
*****************************************************************/
static void change_callback(GtkWidget *w, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;

  ptr = data;
  selection = ptr->src_selection;

  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    gint cid;

    gtk_tree_model_get(model, &it, 0, &cid, -1);

    city_change_production(ptr->pcity, cid_production(cid));
  }
}

/****************************************************************
...
*****************************************************************/
static void future_callback(GtkToggleButton *toggle, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;
  ptr->future = !ptr->future;

  refresh_worklist(ptr->editor);
}

/****************************************************************
...
*****************************************************************/
static void queue_bubble_up(struct worklist_data *ptr)
{
  GtkTreePath *path;
  GtkTreeViewColumn *col;
  GtkTreeModel *model;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }

  model = GTK_TREE_MODEL(ptr->dst);
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(ptr->dst_view), &path, &col);
  if (path) {
    GtkTreeIter it, it_prev;

    if (gtk_tree_path_prev(path)) {
      gtk_tree_model_get_iter(model, &it_prev, path);
      it = it_prev;
      gtk_tree_model_iter_next(model, &it);

      gtk_list_store_swap(GTK_LIST_STORE(model), &it, &it_prev);

      gtk_tree_view_set_cursor(GTK_TREE_VIEW(ptr->dst_view), path, col, FALSE);
      commit_worklist(ptr);
    }
  }
  gtk_tree_path_free(path);
}

/****************************************************************
...
*****************************************************************/
static void queue_remove(struct worklist_data *ptr)
{
  GtkTreePath *path;
  GtkTreeViewColumn *col;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(ptr->dst_view), &path, &col);
  if (path) {
    dst_row_callback(GTK_TREE_VIEW(ptr->dst_view), path, col, ptr);
    gtk_tree_path_free(path);
  }
}

/****************************************************************
...
*****************************************************************/
static void queue_bubble_down(struct worklist_data *ptr)
{
  GtkTreePath *path;
  GtkTreeViewColumn *col;
  GtkTreeModel *model;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }

  model = GTK_TREE_MODEL(ptr->dst);
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(ptr->dst_view), &path, &col);
  if (path) {
    GtkTreeIter it, it_next;

    gtk_tree_model_get_iter(model, &it, path);
    it_next = it;
    if (gtk_tree_model_iter_next(model, &it_next)) {
      gtk_list_store_swap(GTK_LIST_STORE(model), &it, &it_next);

      gtk_tree_path_next(path);
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(ptr->dst_view), path, col, FALSE);
      commit_worklist(ptr);
    }
  }
  gtk_tree_path_free(path);
}

/****************************************************************
...
*****************************************************************/
static void queue_insert(struct worklist_data *ptr, bool prepend)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  GtkTreePath *path;

  GtkTreeModel *src_model, *dst_model;
  GtkTreeIter src_it, dst_it;
  gint i, ncols;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }

  if (!gtk_tree_selection_get_selected(ptr->src_selection, &model, &it)) {
    return;
  }

  path = gtk_tree_model_get_path(model, &it);

  src_model = GTK_TREE_MODEL(ptr->src);
  dst_model = GTK_TREE_MODEL(ptr->dst);

  gtk_tree_model_get_iter(src_model, &src_it, path);
  if (prepend) {
    gtk_list_store_prepend(GTK_LIST_STORE(dst_model), &dst_it);
  } else {
    gtk_list_store_append(GTK_LIST_STORE(dst_model), &dst_it);
  }

  ncols = gtk_tree_model_get_n_columns(src_model);

  for (i = 0; i < ncols; i++) {
    GValue value = { 0, };

    gtk_tree_model_get_value(src_model, &src_it, i, &value);
    gtk_list_store_set_value(GTK_LIST_STORE(dst_model), &dst_it, i, &value);
  }
  commit_worklist(ptr);

  gtk_tree_path_free(path);
}

/****************************************************************
...
*****************************************************************/
static void queue_prepend(struct worklist_data *ptr)
{
  queue_insert(ptr, TRUE);
}

/****************************************************************
...
*****************************************************************/
static void queue_append(struct worklist_data *ptr)
{
  queue_insert(ptr, FALSE);
}

/****************************************************************
...
*****************************************************************/
static void src_row_callback(GtkTreeView *view, GtkTreePath *path,
			     GtkTreeViewColumn *col, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeModel *src_model, *dst_model;
  GtkTreeIter src_it, dst_it;
  gint i, ncols;

  ptr = data;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }
  
  src_model = GTK_TREE_MODEL(ptr->src);
  dst_model = GTK_TREE_MODEL(ptr->dst);

  gtk_tree_model_get_iter(src_model, &src_it, path);
  gtk_list_store_append(GTK_LIST_STORE(dst_model), &dst_it);

  ncols = gtk_tree_model_get_n_columns(src_model);

  for (i = 0; i < ncols; i++) {
    GValue value = { 0, };

    gtk_tree_model_get_value(src_model, &src_it, i, &value);
    gtk_list_store_set_value(GTK_LIST_STORE(dst_model), &dst_it, i, &value);
  }
  commit_worklist(ptr);
}

/****************************************************************
...
*****************************************************************/
static void dst_row_callback(GtkTreeView *view, GtkTreePath *path,
			     GtkTreeViewColumn *col, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeModel *dst_model;
  GtkTreeIter it;

  ptr = data;
  dst_model = GTK_TREE_MODEL(ptr->dst);

  gtk_tree_model_get_iter(dst_model, &it, path);

  gtk_list_store_remove(GTK_LIST_STORE(dst_model), &it);
  commit_worklist(ptr);
}

/****************************************************************
...
*****************************************************************/
static gboolean src_key_press_callback(GtkWidget *w, GdkEventKey *ev,
				       gpointer data)
{
  struct worklist_data *ptr;
    
  ptr = data;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return FALSE;
  }
  
  if ((ev->state & GDK_SHIFT_MASK) && ev->keyval == GDK_Insert) {
    queue_prepend(ptr);
    return TRUE;
  } else if (ev->keyval == GDK_Insert) {
    queue_append(ptr);
    return TRUE;
  } else {
    return FALSE;
  }
}

/****************************************************************
...
*****************************************************************/
static gboolean dst_key_press_callback(GtkWidget *w, GdkEventKey *ev,
				       gpointer data)
{
  GtkTreeModel *model;
  struct worklist_data *ptr;

  ptr = data;
  model = GTK_TREE_MODEL(ptr->dst);

  if (ev->keyval == GDK_Delete) {
    GtkTreeIter it, it_next;
    bool deleted = FALSE;

    if (gtk_tree_model_get_iter_first(model, &it)) {
      bool more;

      do {
	it_next = it;
	more = gtk_tree_model_iter_next(model, &it_next);

	if (gtk_tree_selection_iter_is_selected(ptr->dst_selection, &it)) {
	  gtk_list_store_remove(GTK_LIST_STORE(model), &it);
	  deleted = TRUE;
	}
	it = it_next;

      } while (more);
    }

    if (deleted) {
      commit_worklist(ptr);
    }
    return TRUE;

  } else if ((ev->state & GDK_MOD1_MASK) && ev->keyval == GDK_Up) {
    queue_bubble_up(ptr);
    return TRUE;

  } else if ((ev->state & GDK_MOD1_MASK) && ev->keyval == GDK_Down) {
    queue_bubble_down(ptr);
    return TRUE;

  } else {
    return FALSE;
  }
}

/****************************************************************
...
*****************************************************************/
static void src_selection_callback(GtkTreeSelection *selection, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;

  /* update widget sensitivity. */
  if (gtk_tree_selection_get_selected(selection, NULL, NULL)) {
    if (can_client_issue_orders()
      && (!ptr->pcity || city_owner(ptr->pcity) == client.conn.playing)) {
      /* if ptr->pcity is NULL, this is a global worklist */
      gtk_widget_set_sensitive(ptr->change_cmd, TRUE);
      gtk_widget_set_sensitive(ptr->prepend_cmd, TRUE);
      gtk_widget_set_sensitive(ptr->append_cmd, TRUE);
    } else {
      gtk_widget_set_sensitive(ptr->change_cmd, FALSE);
      gtk_widget_set_sensitive(ptr->prepend_cmd, FALSE);
      gtk_widget_set_sensitive(ptr->append_cmd, FALSE);
    }
    gtk_widget_set_sensitive(ptr->help_cmd, TRUE);
  } else {
    gtk_widget_set_sensitive(ptr->change_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->help_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->prepend_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->append_cmd, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void dst_selection_callback(GtkTreeSelection *selection, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;

  /* update widget sensitivity. */
  if (gtk_tree_selection_count_selected_rows(selection) > 0) {
    int num_rows = 0;
    GtkTreeIter it;
    
    gtk_widget_set_sensitive(ptr->up_cmd, TRUE);
    gtk_widget_set_sensitive(ptr->down_cmd, TRUE);
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ptr->dst), &it)) {
      do {
	num_rows++;
      } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ptr->dst), &it));
    }
    if (num_rows > 1) {
      gtk_widget_set_sensitive(ptr->remove_cmd, TRUE);      
    } else {
      gtk_widget_set_sensitive(ptr->remove_cmd, FALSE);
    }
  } else {
    gtk_widget_set_sensitive(ptr->up_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->down_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->remove_cmd, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static gboolean dst_dnd_callback(GtkWidget *w, GdkDragContext *context,
				 struct worklist_data *ptr)
{
  commit_worklist(ptr);
  return FALSE;
}

/****************************************************************
...
*****************************************************************/
static void cell_render_func(GtkTreeViewColumn *col, GtkCellRenderer *rend,
			     GtkTreeModel *model, GtkTreeIter *it,
			     gpointer data)
{
  gint cid;
  struct universal target;

  gtk_tree_model_get(model, it, 0, &cid, -1);
  target = cid_production(cid);

  if (GTK_IS_CELL_RENDERER_PIXBUF(rend)) {
    GdkPixbuf *pix;

    if (VUT_UTYPE == target.kind) {
      struct canvas store;

      pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
	  max_unit_width, max_unit_height);

      store.type = CANVAS_PIXBUF;
      store.v.pixbuf = pix;
      create_overlay_unit(&store, target.value.utype);

      g_object_set(rend, "pixbuf", pix, NULL);
      g_object_unref(pix);
    } else {
      struct sprite *sprite = get_building_sprite(tileset, target.value.building);

      pix = sprite_get_pixbuf(sprite);
      g_object_set(rend, "pixbuf", pix, NULL);
    }
  } else {
    struct city **pcity = data;
    gint column;
    char *row[4];
    char  buf[4][64];
    int   i;
    gboolean useless;

    for (i = 0; i < ARRAY_SIZE(row); i++) {
      row[i] = buf[i];
    }
    column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(rend), "column"));

    get_city_dialog_production_row(row, sizeof(buf[0]), target, *pcity);
    g_object_set(rend, "text", row[column], NULL);

    if (NULL != *pcity  &&  VUT_IMPROVEMENT == target.kind) {
      struct player *plr = city_owner(*pcity);

      useless = improvement_obsolete(plr, target.value.building)
	|| (! impr_provides_buildable_units(plr, target.value.building)
           && is_building_replaced(*pcity, target.value.building, RPT_CERTAIN));
      /* Mark building redundant if we are really certain that there is
       * no use for it. */
      g_object_set(rend, "strikethrough", useless, NULL);
    } else {
      g_object_set(rend, "strikethrough", FALSE, NULL);
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void populate_view(GtkTreeView *view, struct city **ppcity,
			  GtkTreeViewColumn **pcol)
{
  static const char *titles[] =
  { N_("Type"), N_("Name"), N_("Info"), N_("Cost"), N_("Turns") };

  static bool titles_done;
  gint i;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

  /* Case i == 0 taken out of the loop to workaround gcc-4.2.1 bug
   * http://gcc.gnu.org/PR33381
   * Some values would 'stick' from i == 0 round. */
  i = 0;

  rend = gtk_cell_renderer_pixbuf_new();

  gtk_tree_view_insert_column_with_data_func(view,
                   i, titles[i], rend, cell_render_func, ppcity, NULL);
  col = gtk_tree_view_get_column(view, i);

  if (gui_gtk2_show_task_icons) {
    if (max_unit_width == -1 || max_unit_height == -1) {
      update_max_unit_size();
    }
  } else {
    g_object_set(col, "visible", FALSE, NULL);
  }
  if (gui_gtk2_show_task_icons) {
    g_object_set(rend, "height", max_unit_height, NULL);
  }

  for (i = 1; i < ARRAY_SIZE(titles); i++) {

    gint pos = i-1;

    rend = gtk_cell_renderer_text_new();
    g_object_set_data(G_OBJECT(rend), "column", GINT_TO_POINTER(pos));

    gtk_tree_view_insert_column_with_data_func(view,
	i, titles[i], rend, cell_render_func, ppcity, NULL); 
    col = gtk_tree_view_get_column(view, i);

    if (pos >= 2) {
      g_object_set(G_OBJECT(rend), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_alignment(col, 1.0);
    }

    if (pos == 3) {
      *pcol = col;
    }
    if (gui_gtk2_show_task_icons) {
      g_object_set(rend, "height", max_unit_height, NULL);
    }
  }
}

/****************************************************************
  Worklist editor shell.
*****************************************************************/
GtkWidget *create_worklist(void)
{
  GtkWidget *editor, *table, *sw, *bbox;
  GtkWidget *src_view, *dst_view, *label, *button;
  GtkWidget *menubar, *item, *menu, *image;
  GtkWidget *table2, *arrow, *check;
  GtkSizeGroup *group;

  GtkListStore *src_store, *dst_store;

  struct worklist_data *ptr;


  ptr = fc_malloc(sizeof(*ptr));

  src_store = gtk_list_store_new(1, G_TYPE_INT);
  dst_store = gtk_list_store_new(1, G_TYPE_INT);

  ptr->pwl = NULL;
  ptr->pcity = NULL;
  ptr->src = src_store;
  ptr->dst = dst_store;
  ptr->future = FALSE;


  /* create shell. */ 
  editor = gtk_vbox_new(FALSE, 6);
  g_signal_connect(editor, "destroy", G_CALLBACK(worklist_destroy), ptr);
  g_object_set_data(G_OBJECT(editor), "data", ptr);

  ptr->editor = editor;

  /* add source and target lists.  */
  table = gtk_table_new(2, 5, FALSE);
  gtk_box_pack_start(GTK_BOX(editor), table, TRUE, TRUE, 0);

  group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_table_attach(GTK_TABLE(table), sw, 3, 5, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  src_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(src_store));
  g_object_unref(src_store);
  gtk_size_group_add_widget(group, src_view);
  gtk_widget_set_name(src_view, "small_font");

  populate_view(GTK_TREE_VIEW(src_view), &ptr->pcity, &ptr->src_col);
  gtk_container_add(GTK_CONTAINER(sw), src_view);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", src_view,
		       "label", _("Source _Tasks:"),
		       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0);

  check = gtk_check_button_new_with_mnemonic(_("Show _Future Targets"));
  gtk_table_attach(GTK_TABLE(table), check, 4, 5, 0, 1,
		   0, GTK_FILL, 0, 0);
  g_signal_connect(check, "toggled", G_CALLBACK(future_callback), ptr);


  table2 = gtk_table_new(5, 1, FALSE);
  gtk_table_attach(GTK_TABLE(table), table2, 2, 3, 1, 2,
		   GTK_FILL, GTK_FILL, 0, 0);

  button = gtk_button_new();
  ptr->prepend_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 0, 1,
      0, GTK_EXPAND|GTK_FILL, 0, 24);

  arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
			   G_CALLBACK(queue_prepend), ptr);
  gtk_widget_set_sensitive(ptr->prepend_cmd, FALSE);

  button = gtk_button_new();
  ptr->up_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 1, 2, 0, 0, 0, 0);

  arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
			   G_CALLBACK(queue_bubble_up), ptr);
  gtk_widget_set_sensitive(ptr->up_cmd, FALSE);

  button = gtk_button_new();
  ptr->down_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 2, 3, 0, 0, 0, 0);

  arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
			   G_CALLBACK(queue_bubble_down), ptr);
  gtk_widget_set_sensitive(ptr->down_cmd, FALSE);

  button = gtk_button_new();
  ptr->append_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 3, 4,
      0, GTK_EXPAND|GTK_FILL, 0, 24);

  arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
			   G_CALLBACK(queue_append), ptr);
  gtk_widget_set_sensitive(ptr->append_cmd, FALSE);

  button = gtk_button_new();
  ptr->remove_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 4, 5,
      0, GTK_EXPAND|GTK_FILL, 0, 24);
  
  arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
                           G_CALLBACK(queue_remove), ptr);
  gtk_widget_set_sensitive(ptr->remove_cmd, FALSE);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_table_attach(GTK_TABLE(table), sw, 0, 2, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  dst_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dst_store));
  g_object_unref(dst_store);
  gtk_size_group_add_widget(group, dst_view);
  gtk_widget_set_name(dst_view, "small_font");

  populate_view(GTK_TREE_VIEW(dst_view), &ptr->pcity, &ptr->dst_col);
  gtk_container_add(GTK_CONTAINER(sw), dst_view);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", dst_view,
		       "label", _("Target _Worklist:"),
		       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0);

  /* add bottom menu and buttons. */
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 10);
  gtk_box_pack_start(GTK_BOX(editor), bbox, FALSE, FALSE, 0);

  menubar = gtk_menu_bar_new();
  gtk_container_add(GTK_CONTAINER(bbox), menubar);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), menubar, TRUE);

  menu = gtk_menu_new();

  image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  item = gtk_image_menu_item_new_with_mnemonic(_("_Add Global Worklist"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
  g_signal_connect(menu, "show",
		   G_CALLBACK(popup_add_menu), ptr);
  ptr->add_cmd = item;
  gtk_widget_set_sensitive(ptr->add_cmd, FALSE);

  button = gtk_button_new_from_stock(GTK_STOCK_HELP);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
		   G_CALLBACK(help_callback), ptr);
  ptr->help_cmd = button;
  gtk_widget_set_sensitive(ptr->help_cmd, FALSE);

  button = gtk_button_new_with_mnemonic(_("Change Prod_uction"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
		   G_CALLBACK(change_callback), ptr);
  ptr->change_cmd = button;
  gtk_widget_set_sensitive(ptr->change_cmd, FALSE);

  ptr->src_view = src_view;
  ptr->dst_view = dst_view;
  ptr->src_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(src_view));
  ptr->dst_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dst_view));
  gtk_tree_selection_set_mode(ptr->dst_selection, GTK_SELECTION_MULTIPLE);

  /* DND and other state changing callbacks. */
  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(dst_view), TRUE);
  g_signal_connect(dst_view, "drag_end",
		   G_CALLBACK(dst_dnd_callback), ptr);

  g_signal_connect(src_view, "row_activated",
		   G_CALLBACK(src_row_callback), ptr);
  g_signal_connect(src_view, "key_press_event",
		   G_CALLBACK(src_key_press_callback), ptr);

  g_signal_connect(dst_view, "row_activated",
		   G_CALLBACK(dst_row_callback), ptr);
  g_signal_connect(dst_view, "key_press_event",
		   G_CALLBACK(dst_key_press_callback), ptr);

  g_signal_connect(ptr->src_selection, "changed",
      		   G_CALLBACK(src_selection_callback), ptr);
  g_signal_connect(ptr->dst_selection, "changed",
      		   G_CALLBACK(dst_selection_callback), ptr);


  gtk_widget_show_all(table);
  gtk_widget_show_all(bbox);

  return editor;
}


/****************************************************************
...
*****************************************************************/
void reset_worklist(GtkWidget *editor, struct worklist *pwl, struct city *pcity)
{
  struct worklist_data *ptr;

  ptr = g_object_get_data(G_OBJECT(editor), "data");

  ptr->pwl = pwl;
  ptr->pcity = pcity;

  gtk_list_store_clear(ptr->src);
  gtk_list_store_clear(ptr->dst);

  /* DND callbacks and object visibility settings. */
  if (pcity) {
    g_object_set(ptr->src_col, "visible", TRUE, NULL);
    g_object_set(ptr->dst_col, "visible", TRUE, NULL);

    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(ptr->src_view),
					   GDK_BUTTON1_MASK,
					   wl_dnd_targets,
					   G_N_ELEMENTS(wl_dnd_targets),
					   GDK_ACTION_COPY);
  } else {
    gtk_widget_hide(ptr->change_cmd);
    g_object_set(ptr->src_col, "visible", FALSE, NULL);
    g_object_set(ptr->dst_col, "visible", FALSE, NULL);

    gtk_tree_view_unset_rows_drag_source(GTK_TREE_VIEW(ptr->src_view));
  }
}

/****************************************************************
...
*****************************************************************/
void refresh_worklist(GtkWidget *editor)
{
  struct worklist_data *ptr;
  struct worklist *pwl, queue;

  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  int i, targets_used;
  struct item items[MAX_NUM_PRODUCTION_TARGETS];

  bool selected;
  gint id;
  GtkTreeIter it;

  GtkTreePath *path;

  GtkTreeModel *model;
  gboolean exists;

  ptr = g_object_get_data(G_OBJECT(editor), "data");
  pwl = ptr->pwl;
  
  if (!pwl) {
    return;
  }

  /* refresh source tasks. */
  if (gtk_tree_selection_get_selected(ptr->src_selection, NULL, &it)) {
    gtk_tree_model_get(GTK_TREE_MODEL(ptr->src), &it, 0, &id, -1);
    selected = TRUE;
  } else {
    selected = FALSE;
  }
  gtk_list_store_clear(ptr->src);

  targets_used = collect_eventually_buildable_targets(targets, ptr->pcity, ptr->future);
  name_and_sort_items(targets, targets_used, items, FALSE, ptr->pcity);

  path = NULL;
  for (i = 0; i < targets_used; i++) {
    gtk_list_store_append(ptr->src, &it);
    gtk_list_store_set(ptr->src, &it, 0, (gint) cid_encode(items[i].item), -1);

    if (selected && cid_encode(items[i].item) == id) {
      path = gtk_tree_model_get_path(GTK_TREE_MODEL(ptr->src), &it);
    }
  }
  if (path) {
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(ptr->src_view), path, NULL, FALSE);
    gtk_tree_path_free(path);
  }


  /* refresh target worklist. */
  model = GTK_TREE_MODEL(ptr->dst);
  exists = gtk_tree_model_get_iter_first(model, &it);

  /* dance around worklist braindamage. */
  if (ptr->pcity) {
    city_get_queue(ptr->pcity, &queue);
  } else {
    worklist_copy(&queue, pwl);
  }

  for (i = 0; i < worklist_length(&queue); i++) {
    struct universal target = queue.entries[i];

    if (!exists) {
      gtk_list_store_append(ptr->dst, &it);
    }

    gtk_list_store_set(ptr->dst, &it, 0, (gint) cid_encode(target), -1);

    if (exists) {
      exists = gtk_tree_model_iter_next(model, &it);
    }
  }

  if (exists) {
    GtkTreeIter it_next;
    bool more;

    do {
      it_next = it;
      more = gtk_tree_model_iter_next(model, &it_next);

      gtk_list_store_remove(ptr->dst, &it);
      it = it_next;
    } while (more);
  }

  /* update widget sensitivity. */
  if (ptr->pcity) {
    if ((can_client_issue_orders() &&
	 city_owner(ptr->pcity) == client.conn.playing)) {
      gtk_widget_set_sensitive(ptr->add_cmd, TRUE);
      gtk_widget_set_sensitive(ptr->dst_view, TRUE);
    } else {
      gtk_widget_set_sensitive(ptr->add_cmd, FALSE);
      gtk_widget_set_sensitive(ptr->dst_view, FALSE);
    }
  } else {
    gtk_widget_set_sensitive(ptr->add_cmd, TRUE);
    gtk_widget_set_sensitive(ptr->dst_view, TRUE);
  }
}

/****************************************************************
...
*****************************************************************/
static void commit_worklist(struct worklist_data *ptr)
{
  struct worklist *pwl, queue;
  GtkTreeModel *model;
  GtkTreeIter it;
  int i;

  pwl = ptr->pwl;
  
  if (!pwl) {
    return;
  }

  model = GTK_TREE_MODEL(ptr->dst);
  
  worklist_init(&queue);
  sz_strlcpy(queue.name, pwl->name);

  i = 0;
  if (gtk_tree_model_get_iter_first(model, &it)) {
    do {
      gint cid;

      /* oops, the player has a worklist longer than what we can store. */
      if (i >= MAX_LEN_WORKLIST) {
	break;
      }

      gtk_tree_model_get(model, &it, 0, &cid, -1);

      worklist_append(&queue, cid_production(cid));

      i++;
    } while (gtk_tree_model_iter_next(model, &it));
  }

  /* dance around worklist braindamage. */
  if (ptr->pcity) {
    if (!city_set_queue(ptr->pcity, &queue)) {
      /* Failed to change worklist. This means worklist visible
       * on screen is not true. */
      refresh_worklist(ptr->editor);
    }
  } else {
    worklist_copy(pwl, &queue);
  }
}
