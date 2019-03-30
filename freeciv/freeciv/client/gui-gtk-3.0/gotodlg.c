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
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "game.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "unit.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"
#include "goto.h"
#include "options.h"
#include "text.h"

/* clien/gui-gtk-3.0 */
#include "plrdlg.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"

#include "gotodlg.h"


static GtkWidget *dshell = NULL;
static GtkWidget *view;
static GtkWidget *source;
static GtkWidget *all_toggle;
static GtkListStore *goto_list_store;
static GtkTreeSelection *goto_list_selection;
struct tile *original_tile;
static bool gotodlg_updating = FALSE;

static void update_goto_dialog(GtkToggleButton *button);
static void update_source_label(void);
static void refresh_airlift_column(void);
static void refresh_airlift_button(void);
static void goto_selection_callback(GtkTreeSelection *selection, gpointer data);

static struct city *get_selected_city(void);

enum {
  CMD_AIRLIFT = 1, CMD_GOTO
};

enum {
  GD_COL_CITY_ID = 0,   /* Not shown if not compiled with --enable-debug. */
  GD_COL_CITY_NAME,
  GD_COL_FLAG,
  GD_COL_NATION,
  GD_COL_AIRLIFT,

  GD_COL_NUM
};

/**********************************************************************//**
  User has responded to goto dialog
**************************************************************************/
static void goto_cmd_callback(GtkWidget *dlg, gint arg)
{
  switch (arg) {
  case GTK_RESPONSE_CANCEL:
    center_tile_mapcanvas(original_tile);
    break;

  case CMD_AIRLIFT:
    {
      struct city *pdestcity = get_selected_city();

      if (pdestcity) {
        unit_list_iterate(get_units_in_focus(), punit) {
          if (unit_can_airlift_to(punit, pdestcity)) {
            request_unit_airlift(punit, pdestcity);
          }
        } unit_list_iterate_end;
      }
    }
    break;

  case CMD_GOTO:
    {
      struct city *pdestcity = get_selected_city();

      if (pdestcity) {
	unit_list_iterate(get_units_in_focus(), punit) {
          send_goto_tile(punit, pdestcity->tile);
        } unit_list_iterate_end;
      }
    }
    break;

  default:
    break;
  }

  gtk_widget_destroy(dlg);
  dshell = NULL;
}

/**********************************************************************//**
  Create goto -dialog for gotoing or airlifting unit
**************************************************************************/
static void create_goto_dialog(void)
{
  GtkWidget *sw, *label, *frame, *vbox;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  dshell = gtk_dialog_new_with_buttons(_("Goto/Airlift Unit"),
    NULL,
    0,
    GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL,
    _("Air_lift"),
    CMD_AIRLIFT,
    _("_Goto"),
    CMD_GOTO,
    NULL);
  setup_dialog(dshell, toplevel);
  gtk_window_set_position(GTK_WINDOW(dshell), GTK_WIN_POS_MOUSE);
  gtk_dialog_set_default_response(GTK_DIALOG(dshell), CMD_GOTO);
  g_signal_connect(dshell, "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dshell);
  g_signal_connect(dshell, "response",
                   G_CALLBACK(goto_cmd_callback), NULL);

  source = gtk_label_new("" /* filled in later */);
  gtk_label_set_line_wrap(GTK_LABEL(source), TRUE);
  gtk_label_set_justify(GTK_LABEL(source), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dshell))),
        source, FALSE, FALSE, 0);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "label", _("Select destination ci_ty"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  frame = gtk_frame_new("");
  gtk_frame_set_label_widget(GTK_FRAME(frame), label);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dshell))),
        frame, TRUE, TRUE, 0);

  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing(GTK_GRID(vbox), 6);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  goto_list_store = gtk_list_store_new(GD_COL_NUM, G_TYPE_INT, G_TYPE_STRING,
                                       GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(goto_list_store),
                                       GD_COL_CITY_NAME, GTK_SORT_ASCENDING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(goto_list_store));
  gtk_widget_set_hexpand(view, TRUE);
  gtk_widget_set_vexpand(view, TRUE);
  g_object_unref(goto_list_store);
  goto_list_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), GD_COL_CITY_NAME);
  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), TRUE);

  /* Set the mnemonic in the frame label to focus the city list */
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), view);

#ifdef FREECIV_DEBUG
  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(_("Id"), rend,
    "text", GD_COL_CITY_ID, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  gtk_tree_view_column_set_sort_column_id(col, GD_COL_CITY_ID);
#endif /* FREECIV_DEBUG */

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(_("City"), rend,
    "text", GD_COL_CITY_NAME, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  gtk_tree_view_column_set_sort_column_id(col, GD_COL_CITY_NAME);

  rend = gtk_cell_renderer_pixbuf_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
    "pixbuf", GD_COL_FLAG, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(_("Nation"), rend,
    "text", GD_COL_NATION, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  gtk_tree_view_column_set_sort_column_id(col, GD_COL_NATION);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(_("Airlift"), rend,
    "text", GD_COL_AIRLIFT, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  gtk_tree_view_column_set_sort_column_id(col, GD_COL_AIRLIFT);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(sw), 200);

  gtk_container_add(GTK_CONTAINER(vbox), sw);

  all_toggle = gtk_check_button_new_with_mnemonic(_("Show _All Cities"));
  gtk_container_add(GTK_CONTAINER(vbox), all_toggle);

  g_signal_connect(all_toggle, "toggled", G_CALLBACK(update_goto_dialog), NULL);

  g_signal_connect(goto_list_selection, "changed",
    G_CALLBACK(goto_selection_callback), NULL);

  gtk_widget_show_all(dshell);

  original_tile = get_center_tile_mapcanvas();

  update_source_label();
  update_goto_dialog(GTK_TOGGLE_BUTTON(all_toggle));
  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/**********************************************************************//**
  Popup the dialog
**************************************************************************/
void popup_goto_dialog(void)
{
  if (!can_client_issue_orders() || get_num_units_in_focus() == 0) {
    return;
  }

  if (!dshell) {
    create_goto_dialog();
  }

  gtk_window_present(GTK_WINDOW(dshell));
}

/**********************************************************************//**
  Return currently selected city
**************************************************************************/
static struct city *get_selected_city(void)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  int city_id;

  if (!gtk_tree_selection_get_selected(goto_list_selection, NULL, &it)) {
    return NULL;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

  gtk_tree_model_get(model, &it, GD_COL_CITY_ID, &city_id, -1);

  return game_city_by_number(city_id);
}

/**********************************************************************//**
  Appends the list of the city owned by the player in the goto dialog.
**************************************************************************/
static bool list_store_append_player_cities(GtkListStore *store,
                                            const struct player *pplayer)
{
  GtkTreeIter it;
  struct nation_type *pnation = nation_of_player(pplayer);
  const char *nation = nation_adjective_translation(pnation);
  GdkPixbuf *pixbuf;

  if (city_list_size(pplayer->cities) == 0) {
    return FALSE;
  }

  pixbuf = get_flag(pnation);

  city_list_iterate(pplayer->cities, pcity) {
    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it,
                       GD_COL_CITY_ID, pcity->id,
                       GD_COL_CITY_NAME, city_name_get(pcity),
                       GD_COL_FLAG, pixbuf,
                       GD_COL_NATION, nation,
                       /* GD_COL_AIRLIFT is populated later */
                       -1);
  } city_list_iterate_end;
  g_object_unref(pixbuf);

  return TRUE;
}

/**********************************************************************//**
  Refresh the label that shows where the selected unit(s) currently are
  (and the relevant cities' airlift capacities, if relevant).
**************************************************************************/
static void update_source_label(void)
{
  /* Arbitrary limit to stop the label getting ridiculously long */
  static const int max_cities = 10;
  struct {
    const struct city *city;
    struct unit_list *units;
  } cities[max_cities];
  int ncities = 0;
  bool too_many = FALSE;
  bool no_city = FALSE; /* any units not in a city? */
  struct astring strs[max_cities];
  int nstrs;
  char *last_str;
  const char *descriptions[max_cities+1];
  int i;

  /* Sanity check: if no units selected, give up */
  if (unit_list_size(get_units_in_focus()) == 0) {
    gtk_label_set_text(GTK_LABEL(source), _("No units selected."));
    return;
  }

  /* Divide selected units up into a list of unique cities */
  unit_list_iterate(get_units_in_focus(), punit) {
    const struct city *pcity = tile_city(unit_tile(punit));
    if (pcity) {
      /* Inefficient, but it's not a long list */
      for (i = 0; i < ncities; i++) {
        if (cities[i].city == pcity) {
          unit_list_append(cities[i].units, punit);
          break;
        }
      }
      if (i == ncities) {
        if (ncities < max_cities) {
          cities[ncities].city = pcity;
          cities[ncities].units = unit_list_new();
          unit_list_append(cities[ncities].units, punit);
          ncities++;
        } else {
          too_many = TRUE;
          break;
        }
      }
    } else {
      no_city = TRUE;
    }
  } unit_list_iterate_end;

  /* Describe the individual cities. */
  for (i = 0; i < ncities; i++) {
    const char *air_text = get_airlift_text(cities[i].units, NULL);

    astr_init(&strs[i]);
    if (air_text != NULL) {
      astr_add(&strs[i], 
               /* TRANS: goto/airlift dialog. "Paris (airlift: 2/4)".
                * A set of these appear in an "and"-separated list. */
               _("%s (airlift: %s)"),
               city_name_get(cities[i].city), air_text);
    } else {
      astr_add(&strs[i], "%s", city_name_get(cities[i].city));
    }
    descriptions[i] = astr_str(&strs[i]);
    unit_list_destroy(cities[i].units);
  }
  if (too_many) {
    /* TRANS: goto/airlift dialog. Too many cities to list, some omitted.
     * Appears at the end of an "and"-separated list. */
    descriptions[ncities] = last_str = fc_strdup(Q_("?gotodlg:more"));
    nstrs = ncities+1;
  } else if (no_city) {
    /* TRANS: goto/airlift dialog. For units not currently in a city.
     * Appears at the end of an "and"-separated list. */
    descriptions[ncities] = last_str = fc_strdup(Q_("?gotodlg:no city"));
    nstrs = ncities+1;
  } else {
    last_str = NULL;
    nstrs = ncities;
  }

  /* Finally, update the label. */
  {
    struct astring label = ASTRING_INIT, list = ASTRING_INIT;
    astr_set(&label, 
             /* TRANS: goto/airlift dialog. Current location of units; %s is an
              * "and"-separated list of cities and associated info */
             _("Currently in: %s"),
             astr_build_and_list(&list, descriptions, nstrs));
    astr_free(&list);
    gtk_label_set_text(GTK_LABEL(source), astr_str(&label));
    astr_free(&label);
  }

  /* Clear up. */
  for (i = 0; i < ncities; i++) {
    astr_free(&strs[i]);
  }
  free(last_str); /* might have been NULL */
}

/**********************************************************************//**
  Refresh city list (in response to "all cities" checkbox changing).
**************************************************************************/
static void update_goto_dialog(GtkToggleButton *button)
{
  bool nonempty = FALSE;
  
  if (!client_has_player()) {
    /* Case global observer. */
    return;
  }

  gotodlg_updating = TRUE;

  gtk_list_store_clear(goto_list_store);

  if (gtk_toggle_button_get_active(button)) {
    players_iterate(pplayer) {
      nonempty |= list_store_append_player_cities(goto_list_store, pplayer);
    } players_iterate_end;
  } else {
    nonempty |= list_store_append_player_cities(goto_list_store, client_player());
  }

  gotodlg_updating = FALSE;

  refresh_airlift_column();

  if (!nonempty) {
    /* No selection causes callbacks to fire, causing also Airlift button
     * to update. Do it here. */
    refresh_airlift_button();
  }
}

/**********************************************************************//**
  Refresh airlift column in city list (without tearing everything down).
**************************************************************************/
static void refresh_airlift_column(void)
{
  GtkTreeIter iter;
  bool valid;

  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(goto_list_store), &iter);
  while (valid) {
    int city_id;
    const struct city *pcity;
    const char *air_text;

    gtk_tree_model_get(GTK_TREE_MODEL(goto_list_store), &iter,
                       GD_COL_CITY_ID, &city_id, -1);
    pcity = game_city_by_number(city_id);
    fc_assert_ret(pcity != NULL);
    air_text = get_airlift_text(get_units_in_focus(), pcity);
    gtk_list_store_set(GTK_LIST_STORE(goto_list_store), &iter,
                       GD_COL_AIRLIFT, air_text ? air_text : "-", -1);
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(goto_list_store), &iter);
  }
}

/**********************************************************************//**
  Refresh the state of the "Airlift" button for the currently selected
  unit(s) and city.
**************************************************************************/
static void refresh_airlift_button(void)
{
  struct city *pdestcity = get_selected_city();

  if (NULL != pdestcity) {
    bool can_airlift = FALSE;

    /* Allow action if any of the selected units can airlift. */
    unit_list_iterate(get_units_in_focus(), punit) {
      if (unit_can_airlift_to(punit, pdestcity)) {
        can_airlift = TRUE;
        break;
      }
    } unit_list_iterate_end;

    if (can_airlift) {
      gtk_dialog_set_response_sensitive(GTK_DIALOG(dshell),
                                        CMD_AIRLIFT, TRUE);
      return;
    }
  }
  gtk_dialog_set_response_sensitive(GTK_DIALOG(dshell), CMD_AIRLIFT, FALSE);
}

/**********************************************************************//**
  Update goto dialog. button tells if cities of all players or just
  client's player should be listed.
**************************************************************************/
static void goto_selection_callback(GtkTreeSelection *selection,
                                    gpointer data)
{
  struct city *pdestcity;

  if (gotodlg_updating) {
    return;
  }

  pdestcity = get_selected_city();

  if (NULL != pdestcity) {
    center_tile_mapcanvas(city_tile(pdestcity));
  }
  refresh_airlift_button();
}

/**********************************************************************//**
  Called when the set of units in focus has changed; updates airlift info
**************************************************************************/
void goto_dialog_focus_units_changed(void)
{
  /* Is the dialog currently being displayed? */
  if (dshell) {
    /* Location of current units and ability to airlift may have changed */
    update_source_label();
    refresh_airlift_column();
    refresh_airlift_button();
  }
}
