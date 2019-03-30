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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* utility */
#include "astring.h"
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"

/* common */
#include "game.h"
#include "government.h"
#include "map.h"
#include "packets.h"
#include "player.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "control.h"
#include "helpdata.h"  /* for helptext_nation() */
#include "goto.h"
#include "options.h"
#include "packhand.h"
#include "text.h"
#include "tilespec.h"

/* client/gui-gtk-4.0 */
#include "chatline.h"
#include "choice_dialog.h"
#include "citydlg.h"
#include "editprop.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "plrdlg.h"
#include "wldlg.h"
#include "unitselect.h"
#include "unitselextradlg.h"

#include "dialogs.h"

/******************************************************************/
static GtkWidget  *races_shell;
static GtkWidget  *nationsets_chooser;
struct player *races_player;
/* One entry per nation group, plus one at the end for 'all nations' */
static GtkWidget  *races_nation_list[MAX_NUM_NATION_GROUPS + 1];
static GtkWidget  *races_notebook;
static GtkWidget  *races_properties;
static GtkWidget  *races_leader;
static GtkWidget  *races_sex[2];
static GtkWidget  *races_style_list;
static GtkTextBuffer *races_text;

static void create_races_dialog(struct player *pplayer);
static void races_response(GtkWidget *w, gint response, gpointer data);
static void races_nation_callback(GtkTreeSelection *select, gpointer data);
static void races_leader_callback(void);
static void races_sex_callback(GtkWidget *w, gpointer data);
static void races_style_callback(GtkTreeSelection *select, gpointer data);
static gboolean races_selection_func(GtkTreeSelection *select,
				     GtkTreeModel *model, GtkTreePath *path,
				     gboolean selected, gpointer data);

static int selected_nation;
static int selected_sex;
static int selected_style;

static int is_showing_pillage_dialog = FALSE;

/**********************************************************************//**
  Popup a generic dialog to display some generic information.
**************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
                         const char *lines)
{
  static struct gui_dialog *shell;
  GtkWidget *vbox, *label, *headline_label, *sw;

  gui_dialog_new(&shell, GTK_NOTEBOOK(bottom_notebook), NULL, TRUE);
  gui_dialog_set_title(shell, caption);

  gui_dialog_add_button(shell, "window-close", _("Close"),
                        GTK_RESPONSE_CLOSE);
  gui_dialog_set_default_response(shell, GTK_RESPONSE_CLOSE);

  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing(GTK_GRID(vbox), 2);
  gtk_container_add(GTK_CONTAINER(shell->vbox), vbox);

  headline_label = gtk_label_new(headline);   
  gtk_container_add(GTK_CONTAINER(vbox), headline_label);
  gtk_widget_set_name(headline_label, "notify_label");

  gtk_label_set_justify(GTK_LABEL(headline_label), GTK_JUSTIFY_LEFT);
  gtk_widget_set_halign(headline_label, GTK_ALIGN_START);
  gtk_widget_set_valign(headline_label, GTK_ALIGN_START);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  label = gtk_label_new(lines);
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_vexpand(label, TRUE);
  gtk_container_add(GTK_CONTAINER(sw), label);

  gtk_widget_set_name(label, "notify_label");
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_valign(label, GTK_ALIGN_START);

  gtk_container_add(GTK_CONTAINER(vbox), sw);

  gui_dialog_show_all(shell);

  gui_dialog_set_default_size(shell, -1, 265);
  gui_dialog_present(shell);

  shell = NULL;
}

/**********************************************************************//**
  User has responded to notify dialog with possibility to
  center (goto) on event location.
**************************************************************************/
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

    if (gui_options.center_when_popup_city) {
      center_tile_mapcanvas(ptile);
    }

    if (pcity) {
      popup_city_dialog(pcity);
    }
    break;
  }
  gtk_widget_destroy(w);
}

/**********************************************************************//**
  User clicked close for connect message dialog
**************************************************************************/
static void notify_connect_msg_response(GtkWidget *w, gint response)
{
  gtk_widget_destroy(w);
}

/**********************************************************************//**
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
**************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  GtkWidget *shell, *label;

  if (ptile == NULL) {
    shell = gtk_dialog_new_with_buttons(headline, NULL, 0,
                                        _("Close"), GTK_RESPONSE_CLOSE,
                                        NULL);
  } else {
    struct city *pcity = tile_city(ptile);

    if (pcity != NULL && city_owner(pcity) == client.conn.playing) {
      shell = gtk_dialog_new_with_buttons(headline, NULL, 0,
                                          _("Goto _Location"), 1,
                                          _("I_nspect City"), 2,
                                          _("Close"), GTK_RESPONSE_CLOSE,
                                          NULL);
    } else {
      shell = gtk_dialog_new_with_buttons(headline, NULL, 0,
                                          _("Goto _Location"), 1,
                                          _("Close"), GTK_RESPONSE_CLOSE,
                                          NULL);
    }
  }
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);

  label = gtk_label_new(lines);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(shell))), label);
  gtk_widget_show(label);

  g_object_set_data(G_OBJECT(shell), "tile", ptile);

  g_signal_connect(shell, "response", G_CALLBACK(notify_goto_response), NULL);
  gtk_widget_show(shell);
}

/**********************************************************************//**
  Popup a dialog to display connection message from server.
**************************************************************************/
void popup_connect_msg(const char *headline, const char *message)
{
  GtkWidget *shell, *label;

  shell = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(shell), headline);
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);

  label = gtk_label_new(message);
  gtk_label_set_selectable(GTK_LABEL(label), 1);

  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(shell))), label);
  gtk_widget_show(label);

  gtk_dialog_add_button(GTK_DIALOG(shell), _("Close"),GTK_RESPONSE_CLOSE);

  g_signal_connect(shell, "response", G_CALLBACK(notify_connect_msg_response),
                   NULL);
  gtk_widget_show(shell);
}

/**********************************************************************//**
  User has responded to revolution dialog
**************************************************************************/
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

/**********************************************************************//**
  Popup revolution dialog for user
**************************************************************************/
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

/**********************************************************************//**
  Callback for pillage dialog.
**************************************************************************/
static void pillage_callback(GtkWidget *dlg, gint arg)
{
  is_showing_pillage_dialog = FALSE;

  if (arg == GTK_RESPONSE_YES) {
    int au_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dlg),
                                                  "actor"));
    struct unit *actor = game_unit_by_number(au_id);

    int tgt_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dlg),
                                                   "target"));
    struct extra_type *tgt_extra = extra_by_number(tgt_id);

    if (actor && tgt_extra) {
      request_new_unit_activity_targeted(actor, ACTIVITY_PILLAGE,
                                         tgt_extra);
    }
  }

  gtk_widget_destroy(dlg);
}

/**********************************************************************//**
  Opens pillage dialog listing possible pillage targets.
**************************************************************************/
void popup_pillage_dialog(struct unit *punit, bv_extras extras)
{
  if (!is_showing_pillage_dialog) {
    /* Possibly legal target extras. */
    bv_extras alternative;
    /* Selected by default. */
    struct extra_type *preferred_tgt;
    /* Current target to check. */
    struct extra_type *tgt;

    is_showing_pillage_dialog = TRUE;

    BV_CLR_ALL(alternative);
    preferred_tgt = get_preferred_pillage(extras);

    while ((tgt = get_preferred_pillage(extras))) {
      int what;

      what = extra_index(tgt);
      BV_CLR(extras, what);
      BV_SET(alternative, what);
    }

    select_tgt_extra(punit, unit_tile(punit), alternative, preferred_tgt,
                     /* TRANS: Pillage dialog title. */
                     _("What To Pillage"),
                     /* TRANS: Pillage dialog actor text. */
                     _("Looking for target extra:"),
                     /* TRANS: Pillage dialog target text. */
                     _("Select what to pillage:"),
                     /* TRANS: Pillage dialog do button text. */
                     _("Pillage"), G_CALLBACK(pillage_callback));
  }
}

/**********************************************************************//**
  Popup unit selection dialog. It is a wrapper for the main function; see
  unitselect.c:unit_select_dialog_popup_main().
**************************************************************************/
void unit_select_dialog_popup(struct tile *ptile)
{
  unit_select_dialog_popup_main(ptile, TRUE);
}

/**********************************************************************//**
  Update unit selection dialog. It is a wrapper for the main function; see
  unitselect.c:unit_select_dialog_popup_main().
**************************************************************************/
void unit_select_dialog_update_real(void *unused)
{
  unit_select_dialog_popup_main(NULL, FALSE);
}

/**************************************************************************
  NATION SELECTION DIALOG
**************************************************************************/
/**********************************************************************//**
  Return the GtkTreePath for a given nation on the specified list, or NULL
  if it's not there at all.
  Caller must free with gtk_tree_path_free().
**************************************************************************/
static GtkTreePath *path_to_nation_on_list(Nation_type_id nation,
                                           GtkTreeView *list)
{
  if (nation == -1 || list == NULL) {
    return NULL;
  } else {
    GtkTreeModel *model = gtk_tree_view_get_model(list);
    GtkTreeIter iter;
    GtkTreePath *path = NULL;

    gtk_tree_model_get_iter_first(model, &iter);
    do {
      int nation_of_row;

      gtk_tree_model_get(model, &iter, 0, &nation_of_row, -1);
      if (nation == nation_of_row) {
        path = gtk_tree_model_get_path(model, &iter);
        break;
      }
    } while (gtk_tree_model_iter_next(model, &iter));
    return path;
  }
}

/**********************************************************************//**
  Make sure the given nation is selected in the list on a given groups
  notebook tab, if it's present on that tab.
  Intended for synchronising the tabs to the current selection, so does not
  disturb the controls on the right-hand side.
**************************************************************************/
static void select_nation_on_tab(GtkWidget *tab_list, int nation)
{
  /* tab_list is a GtkTreeView (not its enclosing GtkScrolledWindow). */
  GtkTreeView *list = GTK_TREE_VIEW(tab_list);
  GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
  GtkTreePath *path = path_to_nation_on_list(nation, list);

  /* Suppress normal effects of selection change to avoid loops. */
  g_signal_handlers_block_by_func(select, races_nation_callback, NULL);
  if (path) {
    /* Found nation on this list. */
    /* Avoid disturbing tabs that already have the correct selection. */
    if (!gtk_tree_selection_path_is_selected(select, path)) {
      /* Set cursor -- this will cause the nation to be selected */
      gtk_tree_view_set_cursor(list, path, NULL, FALSE);
      /* Make sure selected nation is visible in list */
      gtk_tree_view_scroll_to_cell(list, path, NULL, FALSE, 0, 0);
    }
  } else {
    /* Either no nation was selected, or the nation is not mentioned in
     * this tab. Either way we want to end up with no selection. */
    GtkTreePath *cursorpath;

    /* If there is no cursor, Gtk tends to focus and select the first row
     * at the first opportunity, disturbing any existing state. We want to
     * allow the no-rows-selected state, so detect this case and defuse
     * it by setting a cursor. */
    gtk_tree_view_get_cursor(list, &cursorpath, NULL);
    /* Set the cursor in the case above, or if there was a previous
     * selection */
    if (!cursorpath || gtk_tree_selection_get_selected(select, NULL, NULL)) {
      cursorpath = gtk_tree_path_new_first();
      gtk_tree_view_set_cursor(list, cursorpath, NULL, FALSE);
    }
    gtk_tree_selection_unselect_all(select);
    gtk_tree_path_free(cursorpath);
  }
  gtk_tree_path_free(path);
  /* Re-enable selection change side-effects */
  g_signal_handlers_unblock_by_func(select, races_nation_callback, NULL);
}

/**********************************************************************//**
  Select the given nation in the nation lists in the left-hand-side notebook.
**************************************************************************/
static void sync_tabs_to_nation(int nation)
{
  /* Ensure that all tabs are in sync with the new selection */
  int i;

  for (i = 0; i <= nation_group_count(); i++) {
    if (races_nation_list[i]) {
      select_nation_on_tab(races_nation_list[i], nation);
    }
  }
}

/**********************************************************************//**
  Populates leader list.
  If no nation selected, blanks it.
**************************************************************************/
static void populate_leader_list(void)
{
  int i;
  GtkListStore *model =
      GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(races_leader)));

  i = 0;
  gtk_list_store_clear(model);
  if (selected_nation >= 0) {
    nation_leader_list_iterate(nation_leaders(nation_by_number
                                              (selected_nation)), pleader) {
      const char *leader_name = nation_leader_name(pleader);
      GtkTreeIter iter; /* unused */

      gtk_list_store_insert_with_values(model, &iter, i, 0, leader_name, -1);
      i++;
    } nation_leader_list_iterate_end;
  }
}

/**********************************************************************//**
  Update dialog state by selecting a nation and choosing values for its
  parameters, and update the right-hand side of the dialog accordingly.
  If 'leadername' is NULL, pick a random leader name and sex from the
  nation's list (ignoring the 'is_male' parameter).
**************************************************************************/
static void select_nation(int nation,
                          const char *leadername, bool is_male,
                          int style_id)
{
  selected_nation = nation;

  /* Refresh the available leaders. */
  populate_leader_list();

  if (selected_nation != -1) {

    /* Select leader name and sex. */
    if (leadername) {
      gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(races_leader))),
                         leadername);
      /* Assume is_male is valid too. */
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(races_sex[is_male]),
                                   TRUE);
    } else {
      int idx = fc_rand(nation_leader_list_size(
                        nation_leaders(nation_by_number(selected_nation))));

      gtk_combo_box_set_active(GTK_COMBO_BOX(races_leader), idx);
      /* This also updates the leader sex, eventually. */
    }

    /* Select the appropriate city style entry. */
    {
      int i;
      int j = 0;
      GtkTreePath *path;

      styles_iterate(pstyle) {
        i = basic_city_style_for_style(pstyle);

        if (i >= 0 && i < style_id) {
          j++;
        } else {
          break;
        }
      } styles_iterate_end;

      path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, j);
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(races_style_list), path,
                               NULL, FALSE);
      gtk_tree_path_free(path);
    }

    /* Update nation description. */
    {
      char buf[4096];

      helptext_nation(buf, sizeof(buf),
                      nation_by_number(selected_nation), NULL);
      gtk_text_buffer_set_text(races_text, buf, -1);
    }

    gtk_widget_set_sensitive(races_properties, TRUE);
    /* Once we've made a nation selection, allow user to ok */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(races_shell),
                                      GTK_RESPONSE_ACCEPT, TRUE);
  } else {
    /* No nation selected. Blank properties and make controls insensitive. */
    /* Leader name */
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(races_leader))),
                       "");
    /* Leader sex (*shrug*) */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(races_sex[0]), TRUE);
    /* City style */
    {
      GtkTreeSelection* select
        = gtk_tree_view_get_selection(GTK_TREE_VIEW(races_style_list));

      gtk_tree_selection_unselect_all(select);
    }
    /* Nation description */
    gtk_text_buffer_set_text(races_text, "", 0);

    gtk_widget_set_sensitive(races_properties, FALSE);
    /* Don't allow OK without a selection
     * (user can still do "Random Nation") */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(races_shell),
                                      GTK_RESPONSE_ACCEPT, FALSE);
  }

  /* Update notebook to reflect the current selection */
  sync_tabs_to_nation(selected_nation);
}

/**********************************************************************//**
  Creates a list of currently-pickable nations in the given group
  Inserts appropriate gtk_tree_view into races_nation_list[index] (or NULL if
  the group has no nations)
  If group == NULL, create a list of all nations
**************************************************************************/
static GtkWidget* create_list_of_nations_in_group(struct nation_group* group,
                                                  int index)
{
  GtkWidget *sw = NULL;
  GtkListStore *store = NULL;
  GtkWidget *list = NULL;

  /* Populate nation list store. */
  nations_iterate(pnation) {
    bool used;
    GdkPixbuf *img;
    GtkTreeIter it;
    GValue value = { 0, };

    if (!is_nation_playable(pnation) || !is_nation_pickable(pnation)) {
      continue;
    }

    if (NULL != group && !nation_is_in_group(pnation, group)) {
      continue;
    }

    /* Only create tab on demand -- we don't want it if there aren't any
     * currently pickable nations in this group. */
    if (sw == NULL) {
      GtkTreeSelection *select;
      GtkCellRenderer *render;
      GtkTreeViewColumn *column;

      store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_BOOLEAN,
          GDK_TYPE_PIXBUF, G_TYPE_STRING);
      gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
          3, GTK_SORT_ASCENDING);

      list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
      gtk_widget_set_hexpand(list, TRUE);
      gtk_widget_set_vexpand(list, TRUE);
      gtk_tree_view_set_search_column(GTK_TREE_VIEW(list), 3);
      gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
      g_object_unref(store);

      select = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
      g_signal_connect(select, "changed", G_CALLBACK(races_nation_callback),
                       NULL);
      gtk_tree_selection_set_select_function(select, races_selection_func,
          NULL, NULL);

      sw = gtk_scrolled_window_new(NULL, NULL);
      gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
          GTK_SHADOW_ETCHED_IN);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
          GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
      gtk_container_add(GTK_CONTAINER(sw), list);
     
      render = gtk_cell_renderer_pixbuf_new();
      column = gtk_tree_view_column_new_with_attributes("Flag", render,
          "pixbuf", 2, NULL);
      gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
      render = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Nation", render,
          "text", 3, "strikethrough", 1, NULL);
      gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
    }

    gtk_list_store_append(store, &it);

    used = (pnation->player != NULL && pnation->player != races_player);
    gtk_list_store_set(store, &it, 0, nation_number(pnation), 1, used, -1);
    img = get_flag(pnation);
    if (img != NULL) {
      gtk_list_store_set(store, &it, 2, img, -1);
      g_object_unref(img);
    }

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, nation_adjective_translation(pnation));
    gtk_list_store_set_value(store, &it, 3, &value);
    g_value_unset(&value);
  } nations_iterate_end;

  races_nation_list[index] = list;
  return sw;
}

/**********************************************************************//**
  Creates lists of nations for left side of nation selection dialog
**************************************************************************/
static void create_nation_selection_lists(void)
{
  GtkWidget *nation_list;
  GtkWidget *group_name_label;
  int i;
  
  for (i = 0; i < nation_group_count(); i++) {
    struct nation_group* group = (nation_group_by_number(i));

    if (is_nation_group_hidden(group)) {
      races_nation_list[i] = NULL;
      continue;
    }
    nation_list = create_list_of_nations_in_group(group, i);
    if (nation_list) {
      group_name_label = gtk_label_new(nation_group_name_translation(group));
      gtk_notebook_append_page(GTK_NOTEBOOK(races_notebook), nation_list,
                               group_name_label);
    }
  }
  
  nation_list = create_list_of_nations_in_group(NULL, nation_group_count());
  /* Even this list can be empty if there are no pickable nations (due to
   * a combination of start position and nationset restrictions). */
  if (nation_list) {
    group_name_label = gtk_label_new(_("All"));
    gtk_notebook_append_page(GTK_NOTEBOOK(races_notebook), nation_list,
                             group_name_label);
  }
}

/**********************************************************************//**
  The server has changed the set of selectable nations.
  Update any current nations dialog accordingly.
**************************************************************************/
void races_update_pickable(bool nationset_change)
{
  int tab, groupidx;

  if (!races_shell) {
    return;
  }

  /* Save selected tab */
  tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(races_notebook));
  if (tab != -1) {
    int i = 0;

    groupidx = 0;
    /* Turn tab index into a nation group index (they're not always equal,
     * as some groups may not currently have tabs). */
    do {
      while (groupidx <= nation_group_count()
             && races_nation_list[groupidx] == NULL) {
        groupidx++;
      }
      fc_assert_action(groupidx <= nation_group_count(), break);
      /* Nation group 'groupidx' is what's displayed on the i'th tab */
      if (i == tab) {
        break;
      }
      i++;
      groupidx++;
    } while (TRUE);
  } else {
    /* No tabs currently */
    groupidx = -1;
  }

  /* selected_nation already contains currently selected nation; however,
   * it may no longer be a valid choice */
  if (selected_nation != -1
      && !is_nation_pickable(nation_by_number(selected_nation))) {
    select_nation(-1, NULL, FALSE, 0);
  }

  /* Delete all list stores, treeviews, tabs */
  while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(races_notebook)) > 0) {
    gtk_notebook_remove_page(GTK_NOTEBOOK(races_notebook), -1);
  }

  /* (Re)create all of them */
  create_nation_selection_lists();

  /* Can't set current tab before child widget is visible */
  gtk_widget_show(GTK_WIDGET(races_notebook));

  /* Restore selected tab */
  if (groupidx != -1 && races_nation_list[groupidx] != NULL) {
    int i;

    tab = 0;
    for (i = 0; i < groupidx; i++) {
      if (races_nation_list[i] != NULL) {
        tab++;
      }
    }
    gtk_notebook_set_current_page(GTK_NOTEBOOK(races_notebook), tab);
  }

  /* Restore selected nation */
  sync_tabs_to_nation(selected_nation);
}

/**********************************************************************//**
  Sync nationset control with the current state of the server.
**************************************************************************/
void nationset_sync_to_server(const char *nationset)
{
  if (nationsets_chooser) {
    struct nation_set *set = nation_set_by_setting_value(nationset);

    gtk_combo_box_set_active(GTK_COMBO_BOX(nationsets_chooser),
                             nation_set_index(set));
  }
}

/**********************************************************************//**
  Called when the nationset control's value has changed.
**************************************************************************/
static void nationset_callback(GtkComboBox *b, gpointer data)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter(b, &iter)) {
    struct option *poption = optset_option_by_name(server_optset, "nationset");
    gchar *rule_name;

    gtk_tree_model_get(gtk_combo_box_get_model(b), &iter,
                       0, &rule_name, -1);
    /* Suppress propagation of an option value equivalent to the current
     * server state, after canonicalisation, to avoid loops from
     * nationset_sync_to_server().
     * (HACK: relies on local Gtk "changed" signal getting here before
     * server response.) */
    if (nation_set_by_setting_value(option_str_get(poption))
        != nation_set_by_rule_name(rule_name)) {
      option_str_set(poption, rule_name);
    }
    FC_FREE(rule_name);
  }
}

/**********************************************************************//**
  Create nations dialog
**************************************************************************/
static void create_races_dialog(struct player *pplayer)
{
  GtkWidget *shell;
  GtkWidget *cmd;
  GtkWidget *hbox, *table;
  GtkWidget *frame, *label, *combo;
  GtkWidget *text;
  GtkWidget *notebook;
  GtkWidget *sw;
  GtkWidget *list;  
  GtkListStore *store;
  GtkCellRenderer *render;
  GtkTreeViewColumn *column;
  int i;
  char *title;

  /* Init. */
  selected_nation = -1;

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
                                      _("Cancel"),
                                      GTK_RESPONSE_CANCEL,
                                      _("_Random Nation"),
                                      GTK_RESPONSE_NO, /* arbitrary */
                                      _("Ok"),
                                      GTK_RESPONSE_ACCEPT,
                                      NULL);
  races_shell = shell;
  races_player = pplayer;
  setup_dialog(shell, toplevel);

  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_default_size(GTK_WINDOW(shell), -1, 590);

  frame = gtk_frame_new(_("Select a nation"));
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(shell))), frame);

  hbox = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(hbox), 18);
  gtk_widget_set_margin_start(hbox, 3);
  gtk_widget_set_margin_end(hbox, 3);
  gtk_widget_set_margin_top(hbox, 3);
  gtk_widget_set_margin_bottom(hbox, 3);

  gtk_container_add(GTK_CONTAINER(frame), hbox);

  /* Left side: nation list */
  {
    GtkWidget* nation_selection_list = gtk_grid_new();

    nationsets_chooser = NULL;

    gtk_grid_set_row_spacing(GTK_GRID(nation_selection_list), 2);

    /* Nationset selector dropdown */
    /* Only present this if there is more than one choice.
     * (If ruleset is changed, possibly changing the number of available sets
     * and invalidating this decision, then dialog will be popped down.) */
    if (nation_set_count() > 1) {
      GtkListStore *sets_model = gtk_list_store_new(4, G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING);
      GtkCellRenderer *renderer;

      nation_sets_iterate(pset) {
        /* Index in list store must match nation_set_index(). */
        gchar *escaped;
        struct astring s = ASTRING_INIT;
        int num_nations = 0;

        nations_iterate(pnation) {
          if (is_nation_playable(pnation) && nation_is_in_set(pnation, pset)) {
            num_nations++;
          }
        } nations_iterate_end;
        escaped = g_markup_escape_text(nation_set_name_translation(pset), -1);
        /* TRANS: nation set name followed by number of playable nations;
         * <b> and </b> are Pango markup and should be left alone */
        astr_set(&s, PL_("<b>%s</b> (%d nation)",
                         "<b>%s</b> (%d nations)", num_nations),
                 escaped, num_nations);
        g_free(escaped);
        if (strlen(nation_set_description(pset)) > 0) {
          /* While in principle it would be better to get Gtk to wrap the
           * drop-down (e.g. via "wrap-width" property), there's no way
           * to specify the indentation we want. So we do it ourselves. */
          char *desc = fc_strdup(_(nation_set_description(pset)));
          char *p = desc;

          fc_break_lines(desc, 70);
          astr_add(&s, "\n");
          while (*p) {
            int len = strcspn(p, "\n");

            if (p[len] == '\n') {
              len++;
            }
            escaped = g_markup_escape_text(p, len);
            astr_add(&s, "\t%s", escaped);
            g_free(escaped);
            p += len;
          }
          FC_FREE(desc);
        }
        gtk_list_store_insert_with_values(sets_model, NULL, -1,
                                          0, nation_set_rule_name(pset),
                                          1, astr_str(&s),
                                          2, nation_set_name_translation(pset),
                                          -1);
        astr_free(&s);
      } nation_sets_iterate_end;

      /* We want a combo box where the button displays just the set name,
       * but the dropdown displays the expanded description. */
      nationsets_chooser
        = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(sets_model));
      g_object_unref(G_OBJECT(sets_model));
      {
        /* Do our best to turn the text-entry widget into something more
         * like a cell-view: disable editing, and focusing (which removes
         * the caret). */
        GtkWidget *entry = gtk_bin_get_child(GTK_BIN(nationsets_chooser));

        gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
        gtk_widget_set_can_focus(entry, FALSE);
      }
      /* The entry displays the set name. */
      gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(nationsets_chooser),
                                          2);
      /* The dropdown displays the marked-up description. */
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_layout_clear(GTK_CELL_LAYOUT(nationsets_chooser));
      gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(nationsets_chooser),
                                 renderer, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(nationsets_chooser),
                                     renderer, "markup", 1, NULL);
      g_signal_connect(nationsets_chooser, "destroy",
                       G_CALLBACK(gtk_widget_destroyed), &nationsets_chooser);
      g_signal_connect(nationsets_chooser, "changed",
                       G_CALLBACK(nationset_callback), NULL);
      {
        /* Populate initially from client's view of server setting */
        struct option *poption = optset_option_by_name(server_optset,
                                                       "nationset");
        if (poption) {
          nationset_sync_to_server(option_str_get(poption));
        }
      }

      label = g_object_new(GTK_TYPE_LABEL,
          "use-underline", TRUE,
          "label", _("_Nation Set:"),
          "xalign", 0.0,
          "yalign", 0.5,
          NULL);
      gtk_label_set_mnemonic_widget(GTK_LABEL(label), nationsets_chooser);

      gtk_widget_set_hexpand(nationsets_chooser, TRUE);
      gtk_grid_attach(GTK_GRID(nation_selection_list), label,
                      0, 0, 1, 1);
      gtk_grid_attach(GTK_GRID(nation_selection_list), nationsets_chooser,
                      1, 0, 1, 1);
    }

    races_notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(races_notebook), GTK_POS_LEFT);  
    gtk_grid_attach(GTK_GRID(nation_selection_list), races_notebook,
                    0, 2, 2, 1);

    /* Suppress notebook tabs if there will be only one ("All") */
    {
      bool show_groups = FALSE;

      nation_groups_iterate(pgroup) {
        if (!is_nation_group_hidden(pgroup)) {
          show_groups = TRUE;
          break;
        }
      } nation_groups_iterate_end;
      if (!show_groups) {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(races_notebook), FALSE);
      } else {
        label = g_object_new(GTK_TYPE_LABEL,
            "use-underline", TRUE,
            "label", _("Nation _Groups:"),
            "xalign", 0.0,
            "yalign", 0.5,
            NULL);
        gtk_label_set_mnemonic_widget(GTK_LABEL(label), races_notebook);
        gtk_grid_attach(GTK_GRID(nation_selection_list), label,
                        0, 1, 2, 1);
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(races_notebook), TRUE);
      }
    }

    /* Populate treeview */
    create_nation_selection_lists();

    gtk_container_add(GTK_CONTAINER(hbox), nation_selection_list);
  }

  /* Right side. */
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
  gtk_container_add(GTK_CONTAINER(hbox), notebook);

  /* Properties pane. */
  label = gtk_label_new_with_mnemonic(_("_Properties"));

  races_properties = table = gtk_grid_new();
  g_signal_connect(table, "destroy",
      G_CALLBACK(gtk_widget_destroyed), &races_properties);
  g_object_set(table, "margin", 6, NULL);
  gtk_grid_set_row_spacing(GTK_GRID(table), 2);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);

  /* Leader. */ 
  {
    GtkListStore *model = gtk_list_store_new(1, G_TYPE_STRING);

    combo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(model));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combo), 0);
    g_object_unref(G_OBJECT(model));
  }
  races_leader = combo;
  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", GTK_COMBO_BOX(combo),
      "label", _("_Leader:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  gtk_widget_set_margin_bottom(label, 6);
  gtk_widget_set_margin_end(label, 12);
  gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 2);
  gtk_grid_attach(GTK_GRID(table), combo, 1, 0, 2, 1);

  cmd = gtk_radio_button_new_with_mnemonic(NULL, _("_Female"));
  gtk_widget_set_margin_bottom(cmd, 6);
  races_sex[0] = cmd;
  gtk_grid_attach(GTK_GRID(table), cmd, 1, 1, 1, 1);

  cmd = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(cmd),
      _("_Male"));
  gtk_widget_set_margin_bottom(cmd, 6);
  races_sex[1] = cmd;
  gtk_grid_attach(GTK_GRID(table), cmd, 2, 1, 1, 1);

  /* City style. */
  store = gtk_list_store_new(3, G_TYPE_INT,
      GDK_TYPE_PIXBUF, G_TYPE_STRING);

  list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_hexpand(list, TRUE);
  gtk_widget_set_vexpand(list, TRUE);
  races_style_list = list;
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)), "changed",
      G_CALLBACK(races_style_callback), NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_margin_top(sw, 6);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), list);
  gtk_grid_attach(GTK_GRID(table), sw, 1, 2, 2, 2);

  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", list,
      "label", _("City _Styles:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  gtk_widget_set_margin_top(label, 6);
  gtk_widget_set_margin_end(label, 12);
  gtk_grid_attach(GTK_GRID(table), label, 0, 2, 1, 1);

  render = gtk_cell_renderer_pixbuf_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, render,
      "pixbuf", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
  render = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, render,
      "text", 2, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

  /* Populate style store. */
  styles_iterate(pstyle) {
    GdkPixbuf *img;
    struct sprite *s;
    GtkTreeIter it;

    i = basic_city_style_for_style(pstyle);

    if (i >= 0) {
      gtk_list_store_append(store, &it);

      s = crop_blankspace(get_sample_city_sprite(tileset, i));
      img = sprite_get_pixbuf(s);
      free_sprite(s);
      gtk_list_store_set(store, &it, 0, i, 1, img, 2,
                         city_style_name_translation(i), -1);
      g_object_unref(img);
    }
  } styles_iterate_end;

  /* Legend pane. */
  label = gtk_label_new_with_mnemonic(_("_Description"));

  text = gtk_text_view_new();
  g_object_set(text, "margin", 6, NULL);
  gtk_widget_set_hexpand(text, TRUE);
  gtk_widget_set_vexpand(text, TRUE);
  races_text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 6);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 6);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), text, label);

  /* Signals. */
  g_signal_connect(shell, "destroy",
      G_CALLBACK(gtk_widget_destroyed), &races_shell);
  g_signal_connect(shell, "response",
      G_CALLBACK(races_response), NULL);

  g_signal_connect(GTK_COMBO_BOX(races_leader), "changed",
      G_CALLBACK(races_leader_callback), NULL);

  g_signal_connect(races_sex[0], "toggled",
      G_CALLBACK(races_sex_callback), GINT_TO_POINTER(0));
  g_signal_connect(races_sex[1], "toggled",
      G_CALLBACK(races_sex_callback), GINT_TO_POINTER(1));

  /* Finish up. */
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CANCEL);

  /* You can't assign NO_NATION during a running game. */
  if (C_S_RUNNING == client_state()) {
    gtk_dialog_set_response_sensitive(GTK_DIALOG(shell), GTK_RESPONSE_NO,
                                      FALSE);
  }

  gtk_widget_show(gtk_dialog_get_content_area(GTK_DIALOG(shell)));

  /* Select player's current nation in UI, if any */
  if (races_player->nation) {
    select_nation(nation_number(races_player->nation),
                  player_name(races_player),
                  races_player->is_male,
                  style_number(races_player->style));
    /* Make sure selected nation is visible
     * (last page, "All", will certainly contain it) */
    fc_assert(gtk_notebook_get_n_pages(GTK_NOTEBOOK(races_notebook)) > 0);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(races_notebook), -1);
  } else {
    select_nation(-1, NULL, FALSE, 0);
  }
}

/**********************************************************************//**
  Popup the dialog 10% inside the main-window
**************************************************************************/
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

/**********************************************************************//**
  Close nations dialog  
**************************************************************************/
void popdown_races_dialog(void)
{
  if (races_shell) {
    gtk_widget_destroy(races_shell);
  }

  /* We're probably starting a new game, maybe with a new ruleset.
     So we warn the worklist dialog. */
  blank_max_unit_size();
}

/**********************************************************************//**
  Update which nations are allowed to be selected (due to e.g. another
  player choosing a nation).
**************************************************************************/
void races_toggles_set_sensitive(void)
{
  int i;

  if (!races_shell) {
    return;
  }

  for (i = 0; i <= nation_group_count(); i++) {
    if (races_nation_list[i]) {
      GtkTreeView *list = GTK_TREE_VIEW(races_nation_list[i]);
      GtkTreeModel *model = gtk_tree_view_get_model(list);
      GtkTreeSelection* select = gtk_tree_view_get_selection(list);
      GtkTreeIter it;
      gboolean chosen;

      /* Update 'chosen' column in model */
      if (gtk_tree_model_get_iter_first(model, &it)) {
        do {
          int nation_no;
          struct nation_type *nation;

          gtk_tree_model_get(model, &it, 0, &nation_no, -1);
          nation = nation_by_number(nation_no);

          chosen = !is_nation_pickable(nation)
            || (nation->player && nation->player != races_player);

          gtk_list_store_set(GTK_LIST_STORE(model), &it, 1, chosen, -1);

        } while (gtk_tree_model_iter_next(model, &it));
      }

      /* If our selection is now invalid, deselect it */
      if (gtk_tree_selection_get_selected(select, &model, &it)) {
        gtk_tree_model_get(model, &it, 1, &chosen, -1);

        if (chosen) {
          gtk_tree_selection_unselect_all(select);
        }
      }
    }
  }
}

/**********************************************************************//**
  Called whenever a user selects a nation in nation list
**************************************************************************/
static void races_nation_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gboolean chosen;
    int newnation;

    gtk_tree_model_get(model, &it, 0, &newnation, 1, &chosen, -1);

    /* Only allow nations not chosen by another player */
    if (!chosen) {
      if (newnation != selected_nation) {
        /* Choose a random leader */
        select_nation(newnation, NULL, FALSE,
                      style_number(style_of_nation(nation_by_number(newnation))));
      }
      return;
    }
  }

  /* Fall-through if no valid nation selected */
  select_nation(-1, NULL, FALSE, 0);
}

/**********************************************************************//**
  Leader name has been chosen
**************************************************************************/
static void races_leader_callback(void)
{
  const struct nation_leader *pleader;
  const gchar *name;

  name =
    gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(races_leader))));

  if (selected_nation != -1
      &&(pleader = nation_leader_by_name(nation_by_number(selected_nation),
                                         name))) {
    selected_sex = nation_leader_is_male(pleader);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(races_sex[selected_sex]),
                                 TRUE);
  }
}

/**********************************************************************//**
  Leader sex has been chosen
**************************************************************************/
static void races_sex_callback(GtkWidget *w, gpointer data)
{
  selected_sex = GPOINTER_TO_INT(data);
}

/**********************************************************************//**
  Determines which nations can be selected in the UI
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

/**********************************************************************//**
  City style has been chosen
**************************************************************************/
static void races_style_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_tree_model_get(model, &it, 0, &selected_style, -1);
  } else {
    selected_style = -1;
  }
}

/**********************************************************************//**
  User has selected some of the responses for whole nations dialog
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
      output_window_append(ftc_client, _("You must select your sex."));
      return;
    }

    if (selected_style == -1) {
      output_window_append(ftc_client, _("You must select your style."));
      return;
    }

    s = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(races_leader))));

    /* Perform a minimum of sanity test on the name. */
    /* This could call is_allowed_player_name if it were available. */
    if (strlen(s) == 0) {
      output_window_append(ftc_client, _("You must type a legal name."));
      return;
    }

    dsend_packet_nation_select_req(&client.conn,
                                   player_number(races_player), selected_nation,
                                   selected_sex, s,
                                   selected_style);
  } else if (response == GTK_RESPONSE_NO) {
    dsend_packet_nation_select_req(&client.conn,
				   player_number(races_player),
				   -1, FALSE, "", 0);
  }

  popdown_races_dialog();
}

/**********************************************************************//**
  Adjust tax rates from main window
**************************************************************************/
gboolean taxrates_callback(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  common_taxrates_callback((size_t) data);
  return TRUE;
}

/**********************************************************************//**
  Pops up a dialog to confirm upgrading of the unit.
**************************************************************************/
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

/**********************************************************************//**
  Pops up a dialog to confirm disband of the unit(s).
**************************************************************************/
void popup_disband_dialog(struct unit_list *punits)
{
  GtkWidget *shell;
  char buf[512];

  if (!punits || unit_list_size(punits) == 0) {
    return;
  }

  if (!get_units_disband_info(buf, sizeof(buf), punits)) {
    shell = gtk_message_dialog_new(NULL, 0,
				   GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                                   "%s", buf);
    gtk_window_set_title(GTK_WINDOW(shell), _("Disband Units"));
    setup_dialog(shell, toplevel);
    g_signal_connect(shell, "response", G_CALLBACK(gtk_widget_destroy),
                    NULL);
    gtk_window_present(GTK_WINDOW(shell));
  } else {
    shell = gtk_message_dialog_new(NULL, 0,
				   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                   "%s", buf);
    gtk_window_set_title(GTK_WINDOW(shell), _("Disband Units"));
    setup_dialog(shell, toplevel);
    gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_YES);

    if (gtk_dialog_run(GTK_DIALOG(shell)) == GTK_RESPONSE_YES) {
      unit_list_iterate(punits, punit) {
        if (unit_can_do_action(punit, ACTION_DISBAND_UNIT)) {
          request_unit_disband(punit);
        }
      } unit_list_iterate_end;
    }
    gtk_widget_destroy(shell);
  }
}

/**********************************************************************//**
  This function is called when the client disconnects or the game is
  over.  It should close all dialog windows for that game.
**************************************************************************/
void popdown_all_game_dialogs(void)
{
  gui_dialog_destroy_all();
  property_editor_popdown(editprop_get_property_editor());
  unit_select_dialog_popdown();
}

/**********************************************************************//**
  Player has gained a new tech.
**************************************************************************/
void show_tech_gained_dialog(Tech_type_id tech)
{
  const struct advance *padvance = valid_advance_by_number(tech);

  if (NULL != padvance
      && (GUI_GTK_OPTION(popup_tech_help) == GUI_POPUP_TECH_HELP_ENABLED
          || (GUI_GTK_OPTION(popup_tech_help) == GUI_POPUP_TECH_HELP_RULESET
              && game.control.popup_tech_help))) {
    popup_help_dialog_typed(advance_name_translation(padvance), HELP_TECH);
  }
}

/**********************************************************************//**
  Show tileset error dialog. It's blocking as client will
  shutdown as soon as this function returns.
**************************************************************************/
void show_tileset_error(const char *msg)
{
  if (is_gui_up()) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
                                    _("Tileset problem, it's probably incompatible with the ruleset:\n%s"),
                                    msg);
    setup_dialog(dialog, toplevel);

    gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);
  }
}

/**********************************************************************//**
  Give a warning when user is about to edit scenario with manually
  set properties.
**************************************************************************/
bool handmade_scenario_warning(void)
{
  /* Just tell the client common code to handle this. */
  return FALSE;
}

/**********************************************************************//**
  Popup detailed information about battle or save information for
  some kind of statistics
**************************************************************************/
void popup_combat_info(int attacker_unit_id, int defender_unit_id,
                       int attacker_hp, int defender_hp,
                       bool make_att_veteran, bool make_def_veteran)
{
}

/**********************************************************************//**
  Popup dialog showing given image and text,
  start playing given sound, stop playing sound when popup is closed.
  Take all space available to show image if fullsize is set.
  If there are other the same popups show them in queue.
***************************************************************************/
void show_img_play_snd(const char *img_path, const char *snd_path,
                       const char *desc, bool fullsize)
{
  /* PORTME */
}
