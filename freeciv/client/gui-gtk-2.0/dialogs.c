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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* common & utility */
#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "log.h"
#include "mem.h"
#include "packets.h"
#include "player.h"
#include "rand.h"
#include "support.h"
#include "unitlist.h"

/* client */
#include "chatline.h"
#include "choice_dialog.h"
#include "citydlg.h"
#include "client_main.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "control.h"
#include "goto.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "options.h"
#include "packhand.h"
#include "plrdlg.h"
#include "text.h"
#include "tilespec.h"

#include "dialogs.h"
#include "editprop.h"
#include "wldlg.h"

/******************************************************************/
static GtkWidget  *races_shell;
struct player *races_player;
static GtkWidget  *races_nation_list[MAX_NUM_NATION_GROUPS + 1];
static GtkWidget  *races_leader;
static GList      *races_leader_list;
static GtkWidget  *races_sex[2];
static GtkWidget  *races_city_style_list;
static GtkTextBuffer *races_text;

/******************************************************************/
#define SELECT_UNIT_READY  1
#define SELECT_UNIT_SENTRY 2
#define SELECT_UNIT_ALL    3

static GtkWidget *unit_select_dialog_shell;
static GtkTreeStore *unit_select_store;
static GtkWidget *unit_select_view;
static GtkTreePath *unit_select_path;
static struct tile *unit_select_ptile;

static void create_races_dialog(struct player *pplayer);
static void races_destroy_callback(GtkWidget *w, gpointer data);
static void races_response(GtkWidget *w, gint response, gpointer data);
static void races_nation_callback(GtkTreeSelection *select, gpointer data);
static void races_leader_callback(void);
static void races_sex_callback(GtkWidget *w, gpointer data);
static void races_city_style_callback(GtkTreeSelection *select, gpointer data);
static gboolean races_selection_func(GtkTreeSelection *select,
				     GtkTreeModel *model, GtkTreePath *path,
				     gboolean selected, gpointer data);

static int selected_nation;
static int selected_sex;
static int selected_city_style;

static int is_showing_pillage_dialog = FALSE;
static int unit_to_use_to_pillage;

/**************************************************************************
  Popup a generic dialog to display some generic information.
**************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
			 const char *lines)
{
  static struct gui_dialog *shell;
  GtkWidget *vbox, *label, *headline_label, *sw;

  gui_dialog_new(&shell, GTK_NOTEBOOK(bottom_notebook), NULL);
  gui_dialog_set_title(shell, caption);

  gui_dialog_add_button(shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE); 
  gui_dialog_set_default_response(shell, GTK_RESPONSE_CLOSE);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(shell->vbox), vbox, TRUE, TRUE, 0);

  headline_label = gtk_label_new(headline);   
  gtk_box_pack_start(GTK_BOX(vbox), headline_label, FALSE, FALSE, 0);
  gtk_widget_set_name(headline_label, "notify_label");

  gtk_label_set_justify(GTK_LABEL(headline_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(headline_label), 0.0, 0.0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  label = gtk_label_new(lines);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), label);

  gtk_widget_set_name(label, "notify_label");
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);

  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  gui_dialog_show_all(shell);

  gui_dialog_set_default_size(shell, -1, 265);
  gui_dialog_present(shell);

  shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void notify_goto_response(GtkWidget *w, gint response)
{
  struct city *pcity = NULL;
  struct tile *ptile = g_object_get_data(G_OBJECT(w), "tile");

  switch (response) {
  case 1:
    center_tile_mapcanvas(ptile);
    break;
  case 2:
    pcity = tile_city(ptile);

    if (center_when_popup_city) {
      center_tile_mapcanvas(ptile);
    }

    if (pcity) {
      popup_city_dialog(pcity);
    }
    break;
  }
  gtk_widget_destroy(w);
}

/****************************************************************
  User clicked close for connect message dialog
*****************************************************************/
static void notify_connect_msg_response(GtkWidget *w, gint response)
{
  gtk_widget_destroy(w);
}

/**************************************************************************
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
**************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  GtkWidget *shell, *label, *goto_command, *popcity_command;
  
  shell = gtk_dialog_new_with_buttons(headline,
        NULL,
        0,
        NULL);
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);

  label = gtk_label_new(lines);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), label);
  gtk_widget_show(label);
  
  goto_command = gtk_stockbutton_new(GTK_STOCK_JUMP_TO,
	_("Goto _Location"));
  gtk_dialog_add_action_widget(GTK_DIALOG(shell), goto_command, 1);
  gtk_widget_show(goto_command);

  popcity_command = gtk_stockbutton_new(GTK_STOCK_ZOOM_IN,
	_("Inspect _City"));
  gtk_dialog_add_action_widget(GTK_DIALOG(shell), popcity_command, 2);
  gtk_widget_show(popcity_command);

  gtk_dialog_add_button(GTK_DIALOG(shell), GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE);

  if (!ptile) {
    gtk_widget_set_sensitive(goto_command, FALSE);
    gtk_widget_set_sensitive(popcity_command, FALSE);
  } else {
    struct city *pcity;

    pcity = tile_city(ptile);
    gtk_widget_set_sensitive(popcity_command,
      (NULL != pcity && city_owner(pcity) == client.conn.playing));
  }

  g_object_set_data(G_OBJECT(shell), "tile", ptile);

  g_signal_connect(shell, "response", G_CALLBACK(notify_goto_response), NULL);
  gtk_widget_show(shell);
}

/**************************************************************************
  Popup a dialog to display connection message from server.
**************************************************************************/
void popup_connect_msg(const char *headline, const char *message)
{
  GtkWidget *shell, *label;
  
  shell = gtk_dialog_new_with_buttons(headline,
        NULL,
        0,
        NULL);
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);

  label = gtk_label_new(message);
  gtk_label_set_selectable(GTK_LABEL(label), 1);

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), label);
  gtk_widget_show(label);

  gtk_dialog_add_button(GTK_DIALOG(shell), GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE);

  g_signal_connect(shell, "response", G_CALLBACK(notify_connect_msg_response),
                   NULL);
  gtk_widget_show(shell);
}

/****************************************************************
...
*****************************************************************/
static void revolution_response(GtkWidget *w, gint response, gpointer data)
{
  struct government *government = data;

  if (response == GTK_RESPONSE_YES) {
    if (!government) {
      start_revolution();
    } else {
      set_government_choice(government);
    }
  }
  if (w) {
    gtk_widget_destroy(w);
  }
}

/****************************************************************
...
*****************************************************************/
void popup_revolution_dialog(struct government *government)
{
  static GtkWidget *shell = NULL;

  if (0 > client.conn.playing->revolution_finishes) {
    if (!shell) {
      shell = gtk_message_dialog_new(NULL,
	  0,
	  GTK_MESSAGE_WARNING,
	  GTK_BUTTONS_YES_NO,
	  _("You say you wanna revolution?"));
      gtk_window_set_title(GTK_WINDOW(shell), _("Revolution!"));
      setup_dialog(shell, toplevel);

      g_signal_connect(shell, "destroy",
	  G_CALLBACK(gtk_widget_destroyed), &shell);
    }
    g_signal_connect(shell, "response",
	G_CALLBACK(revolution_response), government);

    gtk_window_present(GTK_WINDOW(shell));
  } else {
    revolution_response(shell, GTK_RESPONSE_YES, government);
  }
}


/***********************************************************************
  NB: 'data' is a value of enum tile_special_type casted to a pointer.
***********************************************************************/
static void pillage_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit;
  int what = GPOINTER_TO_INT(data);

  punit = game_find_unit_by_number(unit_to_use_to_pillage);
  if (punit) {
    Base_type_id pillage_base = -1;

    if (what > S_LAST) {
      pillage_base = what - S_LAST - 1;
      what = S_LAST;
    }

    request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE,
                                       what, pillage_base);
  }
}

/****************************************************************
...
*****************************************************************/
static void pillage_destroy_callback(GtkWidget *w, gpointer data)
{
  is_showing_pillage_dialog = FALSE;
}

/****************************************************************
  Opens pillage dialog listing possible pillage targets.
*****************************************************************/
void popup_pillage_dialog(struct unit *punit,
			  bv_special may_pillage,
                          bv_bases bases)
{
  GtkWidget *shl;
  int what;
  enum tile_special_type prereq;

  if (!is_showing_pillage_dialog) {
    is_showing_pillage_dialog = TRUE;
    unit_to_use_to_pillage = punit->id;

    shl = choice_dialog_start(GTK_WINDOW(toplevel),
			       _("What To Pillage"),
			       _("Select what to pillage:"));

    while ((what = get_preferred_pillage(may_pillage, bases)) != S_LAST) {
      bv_special what_bv;
      bv_bases what_base;

      BV_CLR_ALL(what_bv);
      BV_CLR_ALL(what_base);

      if (what > S_LAST) {
        BV_SET(what_base, what - S_LAST - 1);
      } else {
        BV_SET(what_bv, what);
      }

      choice_dialog_add(shl, get_infrastructure_text(what_bv, what_base),
                        G_CALLBACK(pillage_callback), GINT_TO_POINTER(what));

      if (what > S_LAST) {
        BV_CLR(bases, what - S_LAST - 1);
      } else {
        clear_special(&may_pillage, what);
        prereq = get_infrastructure_prereq(what);
        if (prereq != S_LAST) {
          clear_special(&may_pillage, prereq);
        }
      }
    }

    choice_dialog_add(shl, GTK_STOCK_CANCEL, 0, 0);

    choice_dialog_end(shl);

    g_signal_connect(shl, "destroy", G_CALLBACK(pillage_destroy_callback),
		     NULL);   
  }
}

/**************************************************************************
...
**************************************************************************/
static void unit_select_row_activated(GtkTreeView *view, GtkTreePath *path)
{
  GtkTreeIter it;
  struct unit *punit;
  gint id;

  gtk_tree_model_get_iter(GTK_TREE_MODEL(unit_select_store), &it, path);
  gtk_tree_model_get(GTK_TREE_MODEL(unit_select_store), &it, 0, &id, -1);
 
  if ((punit = player_find_unit_by_id(client.conn.playing, id))) {
    set_unit_focus(punit);
  }

  gtk_widget_destroy(unit_select_dialog_shell);
}

/**************************************************************************
...
**************************************************************************/
static void unit_select_append(struct unit *punit, GtkTreeIter *it,
    			       GtkTreeIter *parent)
{
  GdkPixbuf *pix;

  pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
      tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));

  {
    struct canvas canvas_store;

    canvas_store.type = CANVAS_PIXBUF;
    canvas_store.v.pixbuf = pix;   

    gdk_pixbuf_fill(pix, 0x00000000);
    put_unit(punit, &canvas_store, 0, 0);
  }

  gtk_tree_store_append(unit_select_store, it, parent);
  gtk_tree_store_set(unit_select_store, it,
      0, punit->id,
      1, pix,
      2, unit_name_translation(punit),
      -1);
  g_object_unref(pix);

  if (unit_is_in_focus(punit)) {
    unit_select_path =
      gtk_tree_model_get_path(GTK_TREE_MODEL(unit_select_store), it);
  }
}

/**************************************************************************
...
**************************************************************************/
static void unit_select_recurse(int root_id, GtkTreeIter *it_root)
{
  unit_list_iterate(unit_select_ptile->units, pleaf) {
    GtkTreeIter it_leaf;

    if (pleaf->transported_by == root_id) {
      unit_select_append(pleaf, &it_leaf, it_root);
      if (pleaf->occupy > 0) {
	unit_select_recurse(pleaf->id, &it_leaf);
      }
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
static void refresh_unit_select_dialog(void)
{
  if (unit_select_dialog_shell) {
    gtk_tree_store_clear(unit_select_store);

    unit_select_recurse(-1, NULL);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(unit_select_view));

    if (unit_select_path) {
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(unit_select_view),
	  unit_select_path, NULL, FALSE);
      gtk_tree_path_free(unit_select_path);
      unit_select_path = NULL;
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_select_destroy_callback(GtkObject *object, gpointer data)
{
  unit_select_dialog_shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void unit_select_cmd_callback(GtkWidget *w, gint rid, gpointer data)
{
  struct tile *ptile = unit_select_ptile;

  switch (rid) {
  case SELECT_UNIT_READY:
    {
      struct unit *pmyunit = NULL;

      unit_list_iterate(ptile->units, punit) {
        if (unit_owner(punit) == client.conn.playing) {
          pmyunit = punit;

          /* Activate this unit. */
	  punit->focus_status = FOCUS_AVAIL;
	  if (unit_has_orders(punit)) {
	    request_orders_cleared(punit);
	  }
	  if (punit->activity != ACTIVITY_IDLE || punit->ai.control) {
	    punit->ai.control = FALSE;
	    request_new_unit_activity(punit, ACTIVITY_IDLE);
	  }
        }
      } unit_list_iterate_end;

      if (pmyunit) {
        /* Put the focus on one of the activated units. */
        set_unit_focus(pmyunit);
      }
    }
    break;

  case SELECT_UNIT_SENTRY:
    {
      unit_list_iterate(ptile->units, punit) {
        if (unit_owner(punit) == client.conn.playing) {
          if ((punit->activity == ACTIVITY_IDLE) &&
              !punit->ai.control &&
              can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
            request_new_unit_activity(punit, ACTIVITY_SENTRY);
          }
        }
      } unit_list_iterate_end;
    }
    break;

  case SELECT_UNIT_ALL:
    {
      unit_list_iterate(ptile->units, punit) {
        if (unit_owner(punit) == client.conn.playing) {
          if (punit->activity == ACTIVITY_IDLE &&
              !punit->ai.control) {
            /* Give focus to it */
            add_unit_focus(punit);
          }
        }
      } unit_list_iterate_end;
    }
    break;

  default:
    break;
  }
  
  gtk_widget_destroy(unit_select_dialog_shell);
}

/****************************************************************
...
*****************************************************************/
#define NUM_UNIT_SELECT_COLUMNS 2

void popup_unit_select_dialog(struct tile *ptile)
{
  if (!unit_select_dialog_shell) {
    GtkTreeStore *store;
    GtkWidget *shell, *view, *sw, *hbox;
    GtkWidget *ready_cmd, *sentry_cmd, *select_all_cmd, *close_cmd;

    static const char *titles[NUM_UNIT_SELECT_COLUMNS] = {
      N_("Unit"),
      N_("Name")
    };
    static bool titles_done;

    GType types[NUM_UNIT_SELECT_COLUMNS+1] = {
      G_TYPE_INT,
      GDK_TYPE_PIXBUF,
      G_TYPE_STRING
    };
    int i;


    shell = gtk_dialog_new_with_buttons(_("Unit selection"),
      NULL,
      0,
      NULL);
    unit_select_dialog_shell = shell;
    setup_dialog(shell, toplevel);
    g_signal_connect(shell, "destroy",
      G_CALLBACK(unit_select_destroy_callback), NULL);
    gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_MOUSE);
    g_signal_connect(shell, "response",
      G_CALLBACK(unit_select_cmd_callback), NULL);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), hbox);

    intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

    store = gtk_tree_store_newv(ARRAY_SIZE(types), types);
    unit_select_store = store;

    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    unit_select_view = view;
    g_object_unref(store);
 
    for (i = 1; i < ARRAY_SIZE(types); i++) {
      GtkTreeViewColumn *column;
      GtkCellRenderer *render;

      column = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title(column, titles[i-1]);

      switch (types[i]) {
	case G_TYPE_STRING:
	  render = gtk_cell_renderer_text_new();
	  gtk_tree_view_column_pack_start(column, render, TRUE);
	  gtk_tree_view_column_set_attributes(column, render, "text", i, NULL);
	  break;
	default:
	  render = gtk_cell_renderer_pixbuf_new();
	  gtk_tree_view_column_pack_start(column, render, FALSE);
	  gtk_tree_view_column_set_attributes(column, render,
	      "pixbuf", i, NULL);
	  break;
      }
      gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    }

    g_signal_connect(view, "row_activated",
	G_CALLBACK(unit_select_row_activated), NULL);


    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, -1, 300);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
	GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), view);
    gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);


    ready_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      _("_Ready all"), SELECT_UNIT_READY);

    gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area),
      ready_cmd, TRUE);

    sentry_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      _("_Sentry idle"), SELECT_UNIT_SENTRY);

    gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area),
      sentry_cmd, TRUE);

    select_all_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      _("Select _all"), SELECT_UNIT_ALL);

    gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area),
      select_all_cmd, TRUE);

    close_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

    gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);

    gtk_widget_show_all(GTK_DIALOG(shell)->vbox);
    gtk_widget_show_all(GTK_DIALOG(shell)->action_area);
  }

  unit_select_ptile = ptile;
  refresh_unit_select_dialog();

  gtk_window_present(GTK_WINDOW(unit_select_dialog_shell));
}
/****************************************************************
  NATION SELECTION DIALOG
****************************************************************/
/****************************************************************
  Creates a list of nation of given group
  Inserts apropriate gtk_tree_view into races_nation_list[i]
  If group == NULL, create a list of all nations
+****************************************************************/
static GtkWidget* create_list_of_nations_in_group(struct nation_group* group,
						  int index)
{
  GtkWidget *sw;
  GtkListStore *store;
  GtkWidget *list;
  GtkTreeSelection *select;
  GtkCellRenderer *render;
  GtkTreeViewColumn *column;

  store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_BOOLEAN,
      GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
      3, GTK_SORT_ASCENDING);

  list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  races_nation_list[index] = list;
  g_object_unref(store);

  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
  g_signal_connect(select, "changed", G_CALLBACK(races_nation_callback), NULL);
  gtk_tree_selection_set_select_function(select, races_selection_func,
      NULL, NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
      GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(sw), list);
 
  render = gtk_cell_renderer_pixbuf_new();
  column = gtk_tree_view_column_new_with_attributes(_("Flag"), render,
      "pixbuf", 2, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
  render = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("Nation"), render,
      "text", 3, "strikethrough", 1, NULL);
  gtk_tree_view_column_set_sort_column_id(column, 3);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
  render = gtk_cell_renderer_text_new();
  g_object_set(render, "style", PANGO_STYLE_ITALIC, NULL);

  /* Populate nation list store. */
  nations_iterate(pnation) {
    bool used;
    GdkPixbuf *img;
    GtkTreeIter it;
    GValue value = { 0, };

    if (!is_nation_playable(pnation) || !pnation->is_available) {
      continue;
    }

    if (group != NULL && !is_nation_in_group(pnation, group)) {
      continue;
    }

    gtk_list_store_append(store, &it);

    used = (pnation->player != NULL && pnation->player != races_player);
    gtk_list_store_set(store, &it, 0, nation_number(pnation), 1, used, -1);
    img = get_flag(pnation);
    if (img != NULL) {
      gtk_list_store_set(store, &it, 2, img, -1);
      g_object_unref(img);
    }
    if (pnation->player == races_player) {
      /* FIXME: should select this one by default. */
    }

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, nation_adjective_translation(pnation));
    gtk_list_store_set_value(store, &it, 3, &value);
    g_value_unset(&value);
  } nations_iterate_end;
  return sw;
}

/****************************************************************
  Creates left side of nation selection dialog
****************************************************************/
static GtkWidget* create_nation_selection_list(void)
{
  GtkWidget *vbox;
  GtkWidget *notebook;
  
  GtkWidget *label;
  GtkWidget *nation_list;
  GtkWidget *group_name_label;
  
  int i;
  
  vbox = gtk_vbox_new(FALSE, 2);
  
  nation_list = create_list_of_nations_in_group(NULL, 0);  
  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", nation_list,
      "label", _("Nation _Groups:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);  
  
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);  
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
  
  for (i = 0; i <= nation_group_count(); i++) {
    struct nation_group* group = (i == 0 ? NULL: nation_group_by_number(i - 1));
    nation_list = create_list_of_nations_in_group(group, i);
    group_name_label = gtk_label_new(group ? Q_(group->name) : _("All"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), nation_list, group_name_label);
  }

  return vbox;
}


/****************************************************************
...
*****************************************************************/
static void create_races_dialog(struct player *pplayer)
{
  GtkWidget *shell;
  GtkWidget *cmd;
  GtkWidget *vbox, *hbox, *table;
  GtkWidget *frame, *label, *combo;
  GtkWidget *text;
  GtkWidget *notebook;
  GtkWidget* nation_selection_list;
  
  
  GtkWidget *sw;
  GtkWidget *list;  
  GtkListStore *store;
  GtkCellRenderer *render;
  GtkTreeViewColumn *column;
  
  int i;
  char *title;

  if (C_S_RUNNING == client_state()) {
    title = _("Edit Nation");
  } else if (NULL != pplayer && pplayer == client.conn.playing) {
    title = _("What Nation Will You Be?");
  } else {
    title = _("Pick Nation");
  }

  shell = gtk_dialog_new_with_buttons(title,
				      NULL,
				      0,
				      GTK_STOCK_CANCEL,
				      GTK_RESPONSE_CANCEL,
				      _("_Random Nation"),
				      GTK_RESPONSE_NO, /* arbitrary */
				      GTK_STOCK_OK,
				      GTK_RESPONSE_ACCEPT,
				      NULL);
  races_shell = shell;
  races_player = pplayer;
  setup_dialog(shell, toplevel);

  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_default_size(GTK_WINDOW(shell), -1, 590);

  frame = gtk_frame_new(_("Select a nation"));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), frame);

  hbox = gtk_hbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  /* Nation list */
  nation_selection_list = create_nation_selection_list();
  gtk_container_add(GTK_CONTAINER(hbox), nation_selection_list);


  /* Right side. */
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
  gtk_container_add(GTK_CONTAINER(hbox), notebook);

  /* Properties pane. */
  label = gtk_label_new_with_mnemonic(_("_Properties"));

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

  table = gtk_table_new(3, 4, FALSE); 
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacing(GTK_TABLE(table), 0, 12);
  gtk_table_set_col_spacing(GTK_TABLE(table), 1, 12);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

  /* Leader. */ 
  combo = gtk_combo_new();
  races_leader = combo;
  gtk_combo_disable_activate(GTK_COMBO(combo));
  gtk_combo_set_value_in_list(GTK_COMBO(combo), FALSE, FALSE);
  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", GTK_COMBO(combo)->entry,
      "label", _("_Leader:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 2, 0, 0, 0, 0);
  gtk_table_attach(GTK_TABLE(table), combo, 1, 3, 0, 1, 0, 0, 0, 0);

  cmd = gtk_radio_button_new_with_mnemonic(NULL, _("_Female"));
  races_sex[0] = cmd;
  gtk_table_attach(GTK_TABLE(table), cmd, 1, 2, 1, 2, 0, 0, 0, 0);

  cmd = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(cmd),
      _("_Male"));
  races_sex[1] = cmd;
  gtk_table_attach(GTK_TABLE(table), cmd, 2, 3, 1, 2, 0, 0, 0, 0);

  /* City style. */
  store = gtk_list_store_new(3, G_TYPE_INT,
      GDK_TYPE_PIXBUF, G_TYPE_STRING);

  list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  races_city_style_list = list;
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)), "changed",
      G_CALLBACK(races_city_style_callback), NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), list);
  gtk_table_attach(GTK_TABLE(table), sw, 1, 3, 2, 4,
      GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);

  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", list,
      "label", _("City _Styles:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, 0, 0, 0, 0);

  render = gtk_cell_renderer_pixbuf_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, render,
      "pixbuf", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
  render = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, render,
      "text", 2, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

  gtk_table_set_row_spacing(GTK_TABLE(table), 1, 12);

  /* Populate city style store. */
  for (i = 0; i < game.control.styles_count; i++) {
    GdkPixbuf *img;
    struct sprite *s;
    GtkTreeIter it;

    if (city_style_has_requirements(&city_styles[i])) {
      continue;
    }

    gtk_list_store_append(store, &it);

    s = crop_blankspace(get_sample_city_sprite(tileset, i));
    img = sprite_get_pixbuf(s);
    g_object_ref(img);
    free_sprite(s);
    gtk_list_store_set(store, &it, 0, i, 1, img, 2,
                       city_style_name_translation(i), -1);
    g_object_unref(img);
  }

  /* Legend pane. */
  label = gtk_label_new_with_mnemonic(_("_Description"));

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

  text = gtk_text_view_new();
  races_text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 6);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 6);

  gtk_box_pack_start(GTK_BOX(vbox), text, TRUE, TRUE, 0);

  /* Signals. */
  g_signal_connect(shell, "destroy",
      G_CALLBACK(races_destroy_callback), NULL);
  g_signal_connect(shell, "response",
      G_CALLBACK(races_response), NULL);

  g_signal_connect(GTK_COMBO(races_leader)->list, "select_child",
      G_CALLBACK(races_leader_callback), NULL);

  g_signal_connect(races_sex[0], "toggled",
      G_CALLBACK(races_sex_callback), GINT_TO_POINTER(0));
  g_signal_connect(races_sex[1], "toggled",
      G_CALLBACK(races_sex_callback), GINT_TO_POINTER(1));

  /* Init. */
  selected_nation = -1;

  /* Finish up. */
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CANCEL);

  /* Don't allow ok without a selection */
  gtk_dialog_set_response_sensitive(GTK_DIALOG(shell), GTK_RESPONSE_ACCEPT,
                                    FALSE);                                          
  /* You can't assign NO_NATION during a running game. */
  if (C_S_RUNNING == client_state()) {
    gtk_dialog_set_response_sensitive(GTK_DIALOG(shell), GTK_RESPONSE_NO,
                                      FALSE);
  }

  gtk_widget_show_all(GTK_DIALOG(shell)->vbox);
}

/****************************************************************
  popup the dialog 10% inside the main-window 
 *****************************************************************/
void popup_races_dialog(struct player *pplayer)
{
  if (!pplayer) {
    return;
  }

  if (!races_shell) {
    create_races_dialog(pplayer);
    gtk_window_present(GTK_WINDOW(races_shell));
  }
}

/****************************************************************
  ...
 *****************************************************************/
void popdown_races_dialog(void)
{
  if (races_shell) {
    gtk_widget_destroy(races_shell);
  }

  /* We're probably starting a new game, maybe with a new ruleset.
     So we warn the worklist dialog. */
  blank_max_unit_size();
}


/****************************************************************
  ...
 *****************************************************************/
static void races_destroy_callback(GtkWidget *w, gpointer data)
{
  g_list_free(races_leader_list);
  races_leader_list = NULL;

  races_shell = NULL;
}

/****************************************************************
  ...
 *****************************************************************/
static gint cmp_func(gconstpointer ap, gconstpointer bp)
{
  return strcmp((const char *)ap, (const char *)bp);
}

/****************************************************************
  Selects a leader.
  Updates the gui elements.
 *****************************************************************/
static void select_random_leader(void)
{
  struct nation_leader *leaders;
  int i, nleaders;
  GList *items = NULL;
  GtkWidget *text, *list;
  gchar *name;
  bool unique;

  text = GTK_COMBO(races_leader)->entry;
  list = GTK_COMBO(races_leader)->list;
  name = gtk_editable_get_chars(GTK_EDITABLE(text), 0, -1);

  if (name[0] == '\0'
      || g_list_find_custom(races_leader_list, name, cmp_func)) {
    unique = FALSE;
  } else {
    unique = TRUE;
  }

  leaders
    = get_nation_leaders(nation_by_number(selected_nation), &nleaders);
  for (i = 0; i < nleaders; i++) {
    items = g_list_prepend(items, leaders[i].name);
  }

  /* Populate combo box with minimum signal noise. */
  g_signal_handlers_block_by_func(list, races_leader_callback, NULL);
  gtk_combo_set_popdown_strings(GTK_COMBO(races_leader), items);
  gtk_entry_set_text(GTK_ENTRY(text), "");
  g_signal_handlers_unblock_by_func(list, races_leader_callback, NULL);

  g_list_free(races_leader_list);
  races_leader_list = items;

  if (unique) {
    gtk_entry_set_text(GTK_ENTRY(text), name);
  } else {
    i = myrand(nleaders);
    gtk_entry_set_text(GTK_ENTRY(text), g_list_nth_data(items, i));
  }

  g_free(name);
}

/**************************************************************************
  ...
 **************************************************************************/
void races_toggles_set_sensitive(void)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  GtkTreePath *path;
  gboolean chosen;
  int i;
  gboolean changed;

  if (!races_shell) {
    return;
  }

  for (i = 0; i <= nation_group_count(); i++) {
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(races_nation_list[i]));
    if (gtk_tree_model_get_iter_first(model, &it)) {
      do {
        int nation_no;
	struct nation_type *nation;

        gtk_tree_model_get(model, &it, 0, &nation_no, -1);
	nation = nation_by_number(nation_no);

        chosen = !nation->is_available || nation->player;

        gtk_list_store_set(GTK_LIST_STORE(model), &it, 1, chosen, -1);

      } while (gtk_tree_model_iter_next(model, &it));
    }
  }
  
  changed = false;
  for (i = 0; i <= nation_group_count(); i++) {
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(races_nation_list[i]), &path, NULL);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(races_nation_list[i]));    
    if (path) {
      gtk_tree_model_get_iter(model, &it, path);
      gtk_tree_model_get(model, &it, 1, &chosen, -1);

      if (chosen) {
          GtkTreeSelection* select = gtk_tree_view_get_selection(GTK_TREE_VIEW(races_nation_list[i]));
	  gtk_tree_selection_unselect_all(select);
	  changed = true;
      }

      gtk_tree_path_free(path);
    }
  }
}

/**************************************************************************
  Called whenever a user selects a nation in nation list
 **************************************************************************/
static void races_nation_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gboolean chosen;
    struct nation_type *nation;

    gtk_tree_model_get(model, &it, 0, &selected_nation, 1, &chosen, -1);
    nation = nation_by_number(selected_nation);

    if (!chosen) {
      int cs, i, j;
      GtkTreePath *path;
     

      /* Unselect other nations in other pages 
       * This can set selected_nation to -1, so we have to copy it
       */
      int selected_nation_copy = selected_nation;      
      for (i = 0; i <= nation_group_count(); i++) {
        gtk_tree_view_get_cursor(GTK_TREE_VIEW(races_nation_list[i]), &path, NULL);
        model = gtk_tree_view_get_model(GTK_TREE_VIEW(races_nation_list[i]));    
        if (path) {
          int other_nation;
          gtk_tree_model_get_iter(model, &it, path);
          gtk_tree_model_get(model, &it, 0, &other_nation, -1);
          if (other_nation != selected_nation_copy) {
            GtkTreeSelection* select = gtk_tree_view_get_selection(GTK_TREE_VIEW(races_nation_list[i]));
            gtk_tree_selection_unselect_all(select);
          }

          gtk_tree_path_free(path);
        }
      }
      selected_nation = selected_nation_copy;
      
      select_random_leader();
      
      /* Select city style for chosen nation. */
      cs = city_style_of_nation(nation_by_number(selected_nation));
      for (i = 0, j = 0; i < game.control.styles_count; i++) {
        if (city_style_has_requirements(&city_styles[i])) {
	  continue;
	}

	if (i < cs) {
	  j++;
	} else {
	  break;
	}
      }

      path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, j);
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(races_city_style_list), path,
			       NULL, FALSE);
      gtk_tree_path_free(path);

      /* Update nation legend text. */
      gtk_text_buffer_set_text(races_text, nation->legend , -1);
    }

    /* Once we've made a selection, allow user to ok */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(races_shell), 
                                      GTK_RESPONSE_ACCEPT, TRUE);
  } else {
    selected_nation = -1;
  }
}

/**************************************************************************
...
**************************************************************************/
static void races_leader_callback(void)
{
  const gchar *name;

  name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(races_leader)->entry));

  if (check_nation_leader_name(nation_by_number(selected_nation), name)) {
    selected_sex = get_nation_leader_sex(nation_by_number(selected_nation),
					 name);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(races_sex[selected_sex]),
				 TRUE);
  }
}

/**************************************************************************
...
**************************************************************************/
static void races_sex_callback(GtkWidget *w, gpointer data)
{
  selected_sex = GPOINTER_TO_INT(data);
}

/**************************************************************************
...
**************************************************************************/
static gboolean races_selection_func(GtkTreeSelection *select,
				     GtkTreeModel *model, GtkTreePath *path,
				     gboolean selected, gpointer data)
{
  GtkTreeIter it;
  gboolean chosen;

  gtk_tree_model_get_iter(model, &it, path);
  gtk_tree_model_get(model, &it, 1, &chosen, -1);
  return (!chosen || selected);
}

/**************************************************************************
...
**************************************************************************/
static void races_city_style_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_tree_model_get(model, &it, 0, &selected_city_style, -1);
  } else {
    selected_city_style = -1;
  }
}

/**************************************************************************
...
**************************************************************************/
static void races_response(GtkWidget *w, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT) {
    const char *s;

    /* This shouldn't be possible but... */
    if (selected_nation == -1) {
      return;
    }

    if (selected_sex == -1) {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("You must select your sex."));
      return;
    }

    if (selected_city_style == -1) {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("You must select your city style."));
      return;
    }

    s = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(races_leader)->entry));

    /* Perform a minimum of sanity test on the name. */
    /* This could call is_allowed_player_name if it were available. */
    if (strlen(s) == 0) {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("You must type a legal name."));
      return;
    }

    dsend_packet_nation_select_req(&client.conn,
				   player_number(races_player), selected_nation,
				   selected_sex, s, selected_city_style);
  } else if (response == GTK_RESPONSE_NO) {
    dsend_packet_nation_select_req(&client.conn,
				   player_number(races_player),
				   -1, FALSE, "", 0);
  }

  popdown_races_dialog();
}


/**************************************************************************
  Adjust tax rates from main window
**************************************************************************/
gboolean taxrates_callback(GtkWidget * w, GdkEventButton * ev, gpointer data)
{
  common_taxrates_callback((size_t) data);
  return TRUE;
}

/****************************************************************************
  Pops up a dialog to confirm upgrading of the unit.
****************************************************************************/
void popup_upgrade_dialog(struct unit_list *punits)
{
  GtkWidget *shell;
  char buf[512];

  if (!punits || unit_list_size(punits) == 0) {
    return;
  }

  if (!get_units_upgrade_info(buf, sizeof(buf), punits)) {
    shell = gtk_message_dialog_new(NULL, 0,
				   GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
				   "%s", buf);
    gtk_window_set_title(GTK_WINDOW(shell), _("Upgrade Unit!"));
    setup_dialog(shell, toplevel);
    g_signal_connect(shell, "response", G_CALLBACK(gtk_widget_destroy),
                    NULL);
    gtk_window_present(GTK_WINDOW(shell));
  } else {
    shell = gtk_message_dialog_new(NULL, 0,
				   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				   "%s", buf);
    gtk_window_set_title(GTK_WINDOW(shell), _("Upgrade Obsolete Units"));
    setup_dialog(shell, toplevel);
    gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_YES);

    if (gtk_dialog_run(GTK_DIALOG(shell)) == GTK_RESPONSE_YES) {
      unit_list_iterate(punits, punit) {
	request_unit_upgrade(punit);
      } unit_list_iterate_end;
    }
    gtk_widget_destroy(shell);
  }
}

/********************************************************************** 
  This function is called when the client disconnects or the game is
  over.  It should close all dialog windows for that game.
***********************************************************************/
void popdown_all_game_dialogs(void)
{
  gui_dialog_destroy_all();
  property_editor_popdown(editprop_get_property_editor());
}
