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
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "packets.h"
#include "unit.h"

/* client/agents */
#include "cma_fec.h"

/* client */
#include "citydlg_common.h"
#include "cityrepdata.h"
#include "client_main.h"
#include "climisc.h"
#include "global_worklist.h"
#include "mapview_common.h"
#include "options.h"

/* client/gui-gtk-3.22 */
#include "chatline.h"
#include "citydlg.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "mapctrl.h"    /* is_city_hilited() */
#include "optiondlg.h"
#include "repodlgs.h"

#include "cityrep.h"

#define NEG_VAL(x)  ((x)<0 ? (x) : (-x))

/* Some versions of gcc have problems with negative values here (PR#39722). */
#define CMA_NONE	(10000)
#define CMA_CUSTOM	(10001)

struct sell_data {
  int count;                    /* Number of cities. */
  int gold;                     /* Amount of gold. */
  struct impr_type *target;     /* The target for selling. */
};

enum city_operation_type {
  CO_CHANGE, CO_LAST, CO_NEXT, CO_FIRST, CO_NEXT_TO_LAST, CO_SELL, CO_NONE
};

/******************************************************************/
static void create_city_report_dialog(bool make_modal);

static void city_activated_callback(GtkTreeView *view, GtkTreePath *path,
				    GtkTreeViewColumn *col, gpointer data);

static void city_command_callback(struct gui_dialog *dlg, int response,
                                  gpointer data);

static void city_selection_changed_callback(GtkTreeSelection *selection);
static void city_clear_worklist_callback(GtkMenuItem *item, gpointer data);
static void update_total_buy_cost(void);

static void create_select_menu(GtkWidget *item);
static void create_change_menu(GtkWidget *item);
static void create_last_menu(GtkWidget *item);
static void create_first_menu(GtkWidget *item);
static void create_next_menu(GtkWidget *item);
static void create_next_to_last_menu(GtkWidget *item);
static void create_sell_menu(GtkWidget *item);

static struct gui_dialog *city_dialog_shell = NULL;

enum {
  CITY_CENTER = 1, CITY_POPUP, CITY_BUY
};

static GtkWidget *city_view;
static GtkTreeSelection *city_selection;
static GtkListStore *city_model;
#define CRD_COL_CITY_ID (0 + NUM_CREPORT_COLS)

static void popup_select_menu(GtkMenuShell *menu, gpointer data);
static void popup_change_menu(GtkMenuShell *menu, gpointer data);
static void popup_last_menu(GtkMenuShell *menu, gpointer data);
static void popup_first_menu(GtkMenuShell *menu, gpointer data);
static void popup_next_menu(GtkMenuShell *menu, gpointer data);
static void popup_next_to_last_menu(GtkMenuShell *menu, gpointer data);

static void recreate_sell_menu(void);

static GtkWidget *city_center_command;
static GtkWidget *city_popup_command;
static GtkWidget *city_buy_command;
static GtkWidget *city_production_command;
static GtkWidget *city_governor_command;
static GtkWidget *city_sell_command;
static GtkWidget *city_total_buy_cost_label;

static GtkWidget *change_improvements_item;
static GtkWidget *change_units_item;
static GtkWidget *change_wonders_item;

static GtkWidget *last_improvements_item;
static GtkWidget *last_units_item;
static GtkWidget *last_wonders_item;

static GtkWidget *first_improvements_item;
static GtkWidget *first_units_item;
static GtkWidget *first_wonders_item;

static GtkWidget *next_improvements_item;
static GtkWidget *next_units_item;
static GtkWidget *next_wonders_item;

static GtkWidget *next_to_last_improvements_item;
static GtkWidget *next_to_last_units_item;
static GtkWidget *next_to_last_wonders_item;

static GtkWidget *select_island_item;

static GtkWidget *select_bunit_item;
static GtkWidget *select_bimprovement_item;
static GtkWidget *select_bwonder_item;

static GtkWidget *select_supported_item;
static GtkWidget *select_present_item;
static GtkWidget *select_built_improvements_item;
static GtkWidget *select_built_wonders_item;

static GtkWidget *select_improvements_item;
static GtkWidget *select_units_item;
static GtkWidget *select_wonders_item;
static GtkWidget *select_cma_item;

static int city_dialog_shell_is_modal;

bool select_menu_cached;

/************************************************************************//**
  Return text line for the column headers for the city report
****************************************************************************/
static void get_city_table_header(char **text, int n)
{
  struct city_report_spec *spec;
  int i;

  for (i = 0, spec = city_report_specs; i < NUM_CREPORT_COLS; i++, spec++) {
    fc_snprintf(text[i], n, "%*s\n%*s",
                NEG_VAL(spec->width), spec->title1 ? spec->title1 : "",
                NEG_VAL(spec->width), spec->title2 ? spec->title2 : "");
  }
}

/****************************************************************************
                        CITY REPORT DIALOG
****************************************************************************/

/************************************************************************//**
  Returns a new tree model for the city report.
****************************************************************************/
static GtkListStore *city_report_dialog_store_new(void)
{
  GType model_types[NUM_CREPORT_COLS + 1];
  gint i;

  /* City report data. */
  for (i = 0; i < NUM_CREPORT_COLS; i++) {
    model_types[i] = G_TYPE_STRING;
  }

  /* Specific gtk client data. */
  model_types[i++] = G_TYPE_INT;        /* CRD_COL_CITY_ID */

  return gtk_list_store_newv(i, model_types);
}

/************************************************************************//**
  Set the values of the iterator.
****************************************************************************/
static void city_model_set(GtkListStore *store, GtkTreeIter *iter,
                           struct city *pcity)
{
  struct city_report_spec *spec;
  char buf[64];
  gint i;

  for (i = 0; i < NUM_CREPORT_COLS; i++) {
    spec = city_report_specs + i;
    fc_snprintf(buf, sizeof(buf), "%*s", NEG_VAL(spec->width),
                spec->func(pcity, spec->data));
    gtk_list_store_set(store, iter, i, buf, -1);
  }
  gtk_list_store_set(store, iter, CRD_COL_CITY_ID, pcity->id, -1);
}

/************************************************************************//**
  Set the values of the iterator.
****************************************************************************/
static struct city *city_model_get(GtkTreeModel *model, GtkTreeIter *iter)
{
  struct city *pcity;
  int id;

  gtk_tree_model_get(model, iter, CRD_COL_CITY_ID, &id, -1);
  pcity = game_city_by_number(id);
  return ((NULL != pcity
           && client_has_player()
           && city_owner(pcity) != client_player())
          ? NULL : pcity);
}

/************************************************************************//**
  Return TRUE if 'iter' has been set to the city row.
****************************************************************************/
static gboolean city_model_find(GtkTreeModel *model, GtkTreeIter *iter,
                                const struct city *pcity)
{
  const int searched = pcity->id;
  int id;

  if (gtk_tree_model_get_iter_first(model, iter)) {
    do {
      gtk_tree_model_get(model, iter, CRD_COL_CITY_ID, &id, -1);
      if (searched == id) {
        return TRUE;
      }
    } while (gtk_tree_model_iter_next(model, iter));
  }
  return FALSE;
}

/************************************************************************//**
  Fill the model with the current configuration.
****************************************************************************/
static void city_model_fill(GtkListStore *store,
                            GtkTreeSelection *selection, GHashTable *select)
{
  GtkTreeIter iter;

  if (client_has_player()) {
    city_list_iterate(client_player()->cities, pcity) {
      gtk_list_store_append(store, &iter);
      city_model_set(store, &iter, pcity);
      if (NULL != select
          && g_hash_table_remove(select, GINT_TO_POINTER(pcity->id))) {
        gtk_tree_selection_select_iter(selection, &iter);
      }
    } city_list_iterate_end;
  } else {
    /* Global observer case. */
    cities_iterate(pcity) {
      gtk_list_store_append(store, &iter);
      city_model_set(store, &iter, pcity);
      if (NULL != select
          && g_hash_table_remove(select, GINT_TO_POINTER(pcity->id))) {
        gtk_tree_selection_select_iter(selection, &iter);
      }
    } cities_iterate_end;
  }
}

/************************************************************************//**
  Popup the city report dialog, and optionally raise it.
****************************************************************************/
void city_report_dialog_popup(bool raise)
{
  if (!city_dialog_shell) {
    city_dialog_shell_is_modal = FALSE;
    
    create_city_report_dialog(FALSE);

    select_menu_cached = FALSE;
  }

  gui_dialog_present(city_dialog_shell);
  hilite_cities_from_canvas();
  if (raise) {
    gui_dialog_raise(city_dialog_shell);
  }
}

/************************************************************************//**
  Closes the city report dialog.
****************************************************************************/
void city_report_dialog_popdown(void)
{
  if (city_dialog_shell) {
    gui_dialog_destroy(city_dialog_shell);
  }
}

/************************************************************************//**
  Make submenu listing possible build targets
****************************************************************************/
static void append_impr_or_unit_to_menu_item(GtkMenuItem *parent_item,
                                             bool append_units,
                                             bool append_wonders,
                                             enum city_operation_type 
                                             city_operation, 
                                             TestCityFunc test_func,
                                             GCallback callback,
                                             int size)
{
  GtkWidget *menu;
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  int i, item, targets_used;
  char *row[4];
  char buf[4][64];

  GtkSizeGroup *group[3];
  const char *markup[3] = {
    "weight=\"bold\"",
    "",
    ""
  };

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(parent_item, menu);

  if (city_operation != CO_NONE) {
    GPtrArray *selected;
    ITree it;
    int num_selected = 0;
    GtkTreeModel *model = GTK_TREE_MODEL(city_model);
    struct city **data;

    selected = g_ptr_array_sized_new(size);

    for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
      struct city *pcity;

      if (!itree_is_selected(city_selection, &it)
          || !(pcity = city_model_get(model, TREE_ITER_PTR(it)))) {
        continue;
      }

      g_ptr_array_add(selected, pcity);
      num_selected++;
    }

    data = (struct city **)g_ptr_array_free(selected, FALSE);
    targets_used
      = collect_production_targets(targets, data, num_selected, append_units,
				   append_wonders, TRUE, test_func);
    g_free(data);
  } else {
    targets_used = collect_production_targets(targets, NULL, 0, append_units,
					      append_wonders, FALSE,
					      test_func);
  }

  name_and_sort_items(targets, targets_used, items,
		      city_operation != CO_NONE, NULL);

  for (i = 0; i < 4; i++) {
    row[i] = buf[i];
  }

  g_object_set_data(G_OBJECT(menu), "freeciv_test_func", test_func);
  g_object_set_data(G_OBJECT(menu), "freeciv_city_operation",
                    GINT_TO_POINTER(city_operation));

  for (i = 0; i < 3; i++) {
    group[i] = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  }

  for (item = 0; item < targets_used; item++) {
    struct universal target = items[item].item;
    GtkWidget *menu_item, *hbox, *label;
    char txt[256];

    get_city_dialog_production_row(row, sizeof(buf[0]), &target, NULL);

    menu_item = gtk_menu_item_new();
    hbox = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(hbox), 18);
    gtk_container_add(GTK_CONTAINER(menu_item), hbox);

    for (i = 0; i < 3; i++) {
      if (row[i][0] == '\0') {
	continue;
      }

      if (city_operation == CO_SELL && i != 0) {
        continue;
      }

      fc_snprintf(txt, ARRAY_SIZE(txt), "<span %s>%s</span>",
                  markup[i], row[i]);

      label = gtk_label_new(NULL);
      gtk_label_set_markup(GTK_LABEL(label), txt);

      switch (i) {
        case 0:
          gtk_widget_set_halign(label, GTK_ALIGN_START);
          gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
        break;
        case 2:
          gtk_widget_set_halign(label, GTK_ALIGN_END);
          gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
          break;
        default:
          break;
      }

      gtk_container_add(GTK_CONTAINER(hbox), label);
      gtk_size_group_add_widget(group[i], label);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    g_signal_connect(menu_item, "activate", callback,
		     GINT_TO_POINTER(cid_encode(target)));
  }
  
  for (i = 0; i < 3; i++) {
    g_object_unref(group[i]);
  }
  
  gtk_widget_show_all(menu);

  gtk_widget_set_sensitive(GTK_WIDGET(parent_item), (targets_used > 0));
}

/************************************************************************//**
  Change the production of one single selected city.
****************************************************************************/
static void impr_or_unit_iterate(GtkTreeModel *model, GtkTreePath *path,
                                 GtkTreeIter *iter, gpointer data)
{
  struct universal target = cid_decode(GPOINTER_TO_INT(data));
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    city_change_production(pcity, &target);
  }
}

/************************************************************************//**
  Called by select_impr_or_unit_callback for each city that is selected in
  the city list dialog to have a object appended to the worklist. Sends a
  packet adding the item to the end of the worklist.
****************************************************************************/
static void worklist_last_impr_or_unit_iterate(GtkTreeModel *model,
                                               GtkTreePath *path,
                                               GtkTreeIter *iter,
                                               gpointer data)
{
  struct universal target = cid_decode(GPOINTER_TO_INT(data));
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    (void) city_queue_insert(pcity, -1, &target);
  }
  /* perhaps should warn the user if not successful? */
}

/************************************************************************//**
  Called by select_impr_or_unit_callback for each city that is selected in
  the city list dialog to have a object inserted first to the worklist.
  Sends a packet adding the current production to the first place after the
  current production of the worklist. Then changes the production to the
  requested item.
****************************************************************************/
static void worklist_first_impr_or_unit_iterate(GtkTreeModel *model,
                                                GtkTreePath *path,
                                                GtkTreeIter *iter,
                                                gpointer data)
{
  struct universal target = cid_decode(GPOINTER_TO_INT(data));
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    (void) city_queue_insert(pcity, 0, &target);
  }
  /* perhaps should warn the user if not successful? */
}

/************************************************************************//**
  Called by select_impr_or_unit_callback for each city that is selected in
  the city list dialog to have a object added next to the worklist. Sends a
  packet adding the item to the first place after the current production of
  the worklist.
****************************************************************************/
static void worklist_next_impr_or_unit_iterate(GtkTreeModel *model,
                                               GtkTreePath *path,
                                               GtkTreeIter *iter,
                                               gpointer data)
{
  struct universal target = cid_decode(GPOINTER_TO_INT(data));
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    (void) city_queue_insert(pcity, 1, &target);
  }
  /* perhaps should warn the user if not successful? */
}

/************************************************************************//**
  Called by select_impr_or_unit_callback for each city that is selected in
  the city list dialog to have an object added before the last position in
  the worklist.
****************************************************************************/
static void worklist_next_to_last_impr_or_unit_iterate(GtkTreeModel *model,
                                                       GtkTreePath *path,
                                                       GtkTreeIter *iter,
                                                       gpointer data)
{
  struct universal target = cid_decode(GPOINTER_TO_INT(data));
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    city_queue_insert(pcity, worklist_length(&pcity->worklist), &target);
  }
}

/************************************************************************//**
  Iterate the cities going to sell.
****************************************************************************/
static void sell_impr_iterate(GtkTreeModel *model, GtkTreePath *path,
                              GtkTreeIter *iter, gpointer data)
{
  struct sell_data *sd = (struct sell_data *) data; 
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity
      && !pcity->did_sell
      && city_has_building(pcity, sd->target)) {
    sd->count++;
    sd->gold += impr_sell_gold(sd->target);
    city_sell_improvement(pcity, improvement_number(sd->target));
  }
}

/************************************************************************//**
  Some build target, either improvement or unit, has been selected from
  some menu.
****************************************************************************/
static void select_impr_or_unit_callback(GtkWidget *wdg, gpointer data)
{
  struct universal target = cid_decode(GPOINTER_TO_INT(data));
  GObject *parent = G_OBJECT(gtk_widget_get_parent(wdg));
  TestCityFunc test_func = g_object_get_data(parent, "freeciv_test_func");
  enum city_operation_type city_operation = 
    GPOINTER_TO_INT(g_object_get_data(parent, "freeciv_city_operation"));

  /* if this is not a city operation: */
  if (city_operation == CO_NONE) {
    GtkTreeModel *model = GTK_TREE_MODEL(city_model);
    ITree it;

    gtk_tree_selection_unselect_all(city_selection);
    for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
      struct city *pcity = city_model_get(model, TREE_ITER_PTR(it));

      if (NULL != pcity && test_func(pcity, &target)) {
        itree_select(city_selection, &it);
      }
    }
  } else {
    GtkTreeSelectionForeachFunc foreach_func;

    connection_do_buffer(&client.conn);
    switch (city_operation) {
    case CO_LAST:
      gtk_tree_selection_selected_foreach(city_selection,
					  worklist_last_impr_or_unit_iterate,
					  GINT_TO_POINTER(cid_encode(target)));
      break;
    case CO_CHANGE:
      gtk_tree_selection_selected_foreach(city_selection,
					  impr_or_unit_iterate,
					  GINT_TO_POINTER(cid_encode(target)));
      break;
    case CO_FIRST:
      gtk_tree_selection_selected_foreach(city_selection,
					  worklist_first_impr_or_unit_iterate,
					  GINT_TO_POINTER(cid_encode(target)));
      break;
    case CO_NEXT:
      gtk_tree_selection_selected_foreach(city_selection,
					  worklist_next_impr_or_unit_iterate,
					  GINT_TO_POINTER(cid_encode(target)));
      break;
    case CO_NEXT_TO_LAST:
      foreach_func = worklist_next_to_last_impr_or_unit_iterate;
      gtk_tree_selection_selected_foreach(city_selection, foreach_func,
                                          GINT_TO_POINTER(cid_encode(target)));
      break;
    case CO_SELL:
      fc_assert_action(target.kind == VUT_IMPROVEMENT, break);
      {
        struct impr_type *building = target.value.building;
        struct sell_data sd = { 0, 0, building };
        GtkWidget *w;
        gint res;
        gchar *buf;
        const char *imprname = improvement_name_translation(building);

        /* Ask confirmation */
        buf = g_strdup_printf(_("Are you sure you want to sell those %s?"), imprname);
        w = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO, "%s", buf);
        g_free(buf);
        res = gtk_dialog_run(GTK_DIALOG(w));    /* Synchron. */
        gtk_widget_destroy(w);
        if (res == GTK_RESPONSE_NO) {
          break;
        }

        gtk_tree_selection_selected_foreach(city_selection,
                                            sell_impr_iterate, &sd);
        if (sd.count > 0) {
          /* FIXME: plurality of sd.count is ignored! */
          /* TRANS: "Sold 3 Harbor for 90 gold." (Pluralisation is in gold --
           * second %d -- not in buildings.) */
          w = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                     PL_("Sold %d %s for %d gold.",
                                         "Sold %d %s for %d gold.",
                                         sd.gold),
                                     sd.count, imprname, sd.gold);
        } else {
          w = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                     _("No %s could be sold."),
                                     imprname);
        }

        g_signal_connect(w, "response",
                         G_CALLBACK(gtk_widget_destroy), NULL);
        gtk_window_present(GTK_WINDOW(w));      /* Asynchron. */
      }
      break;
    case CO_NONE:
      break;
    }
    connection_do_unbuffer(&client.conn);
  }
}

/************************************************************************//**
  CMA callback.
****************************************************************************/
static void cma_iterate(GtkTreeModel *model, GtkTreePath *path,
                        GtkTreeIter *iter, gpointer data)
{
  struct city *pcity = city_model_get(model, iter);
  int idx = GPOINTER_TO_INT(data);

  if (NULL != pcity) {
    if (CMA_NONE == idx) {
      cma_release_city(pcity);
    } else {
      cma_put_city_under_agent(pcity, cmafec_preset_get_parameter(idx));
    }
    refresh_city_dialog(pcity);
  }
}

/************************************************************************//**
  Called when one clicks on an CMA item to make a selection or to
  change a selection's preset.
****************************************************************************/
static void select_cma_callback(GtkWidget *w, gpointer data)
{
  int idx = GPOINTER_TO_INT(data);
  GObject *parent = G_OBJECT(gtk_widget_get_parent(w));
  bool change_cma =
      GPOINTER_TO_INT(g_object_get_data(parent, "freeciv_change_cma"));
  struct cm_parameter parameter;

  /* If this is not the change button but the select cities button. */
  if (!change_cma) {
    ITree it;
    GtkTreeModel *model = GTK_TREE_MODEL(city_model);

    gtk_tree_selection_unselect_all(city_selection);
    for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
      struct city *pcity = city_model_get(model, TREE_ITER_PTR(it));
      int controlled;
      bool select;

      if (NULL == pcity) {
        continue;
      }
      controlled = cma_is_city_under_agent(pcity, &parameter);
      select = FALSE;

      if (idx == CMA_NONE) {
        /* CMA_NONE selects not-controlled, all others require controlled */
        if (!controlled) {
          select = TRUE;
        }
      } else if (controlled) {
        if (idx == CMA_CUSTOM) {
          if (cmafec_preset_get_index_of_parameter(&parameter) == -1) {
            select = TRUE;
          }
        } else if (cm_are_parameter_equal(&parameter,
                                          cmafec_preset_get_parameter(idx))) {
          select = TRUE;
        }
      }

      if (select) {
	itree_select(city_selection, &it);
      }
    }
  } else {
    gtk_tree_selection_selected_foreach(city_selection,
                                        cma_iterate, GINT_TO_POINTER(idx));
  }
}

/************************************************************************//**
  Create the cma entries in the change menu and the select menu. The
  indices CMA_NONE and CMA_CUSTOM are special.
  CMA_NONE signifies a preset of "none" and CMA_CUSTOM a
  "custom" preset.
****************************************************************************/
static void append_cma_to_menu_item(GtkMenuItem *parent_item, bool change_cma)
{
  GtkWidget *menu;
  int i;
  struct cm_parameter parameter;
  GtkWidget *w;

  w = gtk_menu_item_get_submenu(parent_item);
  if (w != NULL && gtk_widget_get_visible(w)) {
    return;
  }

  if (!can_client_issue_orders()) {
    gtk_menu_item_set_submenu(parent_item, NULL);
    return;
  }
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(parent_item, menu);

  if (change_cma) {
    w = gtk_menu_item_new_with_label(_("none"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), w);
    g_signal_connect(w, "activate", G_CALLBACK(select_cma_callback),
                     GINT_TO_POINTER(CMA_NONE));
    fc_assert(GPOINTER_TO_INT(GINT_TO_POINTER(CMA_NONE)) == CMA_NONE);

    for (i = 0; i < cmafec_preset_num(); i++) {
      w = gtk_menu_item_new_with_label(cmafec_preset_get_descr(i));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), w);
      g_signal_connect(w, "activate", G_CALLBACK(select_cma_callback),
                       GINT_TO_POINTER(i));
      fc_assert(GPOINTER_TO_INT(GINT_TO_POINTER(i)) == i);
    }
  } else {
    /* search for a "none" */
    int found;

    found = 0;
    city_list_iterate(client.conn.playing->cities, pcity) {
      if (!cma_is_city_under_agent(pcity, NULL)) {
	found = 1;
	break;
      }
    } city_list_iterate_end;

    if (found) {
      w = gtk_menu_item_new_with_label(_("none"));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), w);
      g_signal_connect(w, "activate", G_CALLBACK(select_cma_callback),
		       GINT_TO_POINTER(CMA_NONE));
    }

    /* 
     * Search for a city that's under custom (not preset) agent. Might
     * take a lonnggg time.
     */
    found = 0;
    city_list_iterate(client.conn.playing->cities, pcity) {
      if (cma_is_city_under_agent(pcity, &parameter) &&
	  cmafec_preset_get_index_of_parameter(&parameter) == -1) {
	found = 1;
	break;
      }
    } city_list_iterate_end;

    if (found) {
      /* we found city that's under agent but not a preset */
      w = gtk_menu_item_new_with_label(_("custom"));

      gtk_menu_shell_append(GTK_MENU_SHELL(menu), w);
      g_signal_connect(w, "activate",
	G_CALLBACK(select_cma_callback), GINT_TO_POINTER(CMA_CUSTOM));
    }

    /* only fill in presets that are being used. */
    for (i = 0; i < cmafec_preset_num(); i++) {
      found = 0;
      city_list_iterate(client.conn.playing->cities, pcity) {
	if (cma_is_city_under_agent(pcity, &parameter) &&
	    cm_are_parameter_equal(&parameter,
				   cmafec_preset_get_parameter(i))) {
	  found = 1;
	  break;
	}
      } city_list_iterate_end;
      if (found) {
	w = gtk_menu_item_new_with_label(cmafec_preset_get_descr(i));

      gtk_menu_shell_append(GTK_MENU_SHELL(menu), w);
	g_signal_connect(w, "activate",
	  G_CALLBACK(select_cma_callback), GINT_TO_POINTER(i));
      }
    }
  }

  g_object_set_data(G_OBJECT(menu), "freeciv_change_cma",
		    GINT_TO_POINTER(change_cma));
  gtk_widget_show_all(menu);
}

/************************************************************************//**
  Helper function to append a worklist to the current work list of one city
  in the city report. This function is called over all selected rows in the
  list view.
****************************************************************************/
static void append_worklist_foreach(GtkTreeModel *model, GtkTreePath *path,
                                    GtkTreeIter *iter, gpointer data)
{
  const struct worklist *pwl = data;
  struct city *pcity = city_model_get(model, iter);

  fc_assert_ret(pwl != NULL);

  if (NULL != pcity) {
    city_queue_insert_worklist(pcity, -1, pwl);
  }
}

/************************************************************************//**
  Menu item callback to append the global worklist associated with this
  item to the worklists of all selected cities. The worklist pointer is
  passed in 'data'.
****************************************************************************/
static void append_worklist_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct global_worklist *pgwl =
    global_worklist_by_id(GPOINTER_TO_INT(data));

  fc_assert_ret(city_selection != NULL);

  if (!pgwl) {
    /* Maybe removed by an other way, not an error. */
    return;
  }

  gtk_tree_selection_selected_foreach(city_selection,
                                      append_worklist_foreach,
                                      (gpointer) global_worklist_get(pgwl));
}

/************************************************************************//**
  Helper function to set a worklist for one city in the city report. This
  function is called over all selected rows in the list view.
****************************************************************************/
static void set_worklist_foreach(GtkTreeModel *model, GtkTreePath *path,
                                 GtkTreeIter *iter, gpointer data)
{
  const struct worklist *pwl = data;
  struct city *pcity = city_model_get(model, iter);

  fc_assert_ret(pwl != NULL);

  if (NULL != pcity) {
    city_set_queue(pcity, pwl);
  }
}

/************************************************************************//**
  Menu item callback to set a city's worklist to the global worklist
  associated with this menu item. The worklist pointer is passed in 'data'.
****************************************************************************/
static void set_worklist_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct global_worklist *pgwl =
    global_worklist_by_id(GPOINTER_TO_INT(data));

  fc_assert_ret(city_selection != NULL);
  gtk_tree_selection_selected_foreach(city_selection, set_worklist_foreach,
                                      (gpointer) global_worklist_get(pgwl));

  if (!pgwl) {
    /* Maybe removed by an other way, not an error. */
    return;
  }

  gtk_tree_selection_selected_foreach(city_selection,
                                      set_worklist_foreach,
                                      (gpointer) global_worklist_get(pgwl));
}

/************************************************************************//**
  Empty and refill the submenu of the menu item passed as 'data'. The menu
  will be filled with menu items corresponding to the global worklists.
****************************************************************************/
static void production_menu_shown(GtkWidget *widget, gpointer data)
{
  GtkWidget *menu, *item;
  GtkMenuItem *parent_item;
  GCallback callback;
  int count = 0;

  parent_item = data;
  fc_assert_ret(parent_item != NULL);
  fc_assert_ret(GTK_IS_MENU_ITEM(parent_item));

  callback = g_object_get_data(G_OBJECT(parent_item), "item_callback");
  fc_assert_ret(callback != NULL);

  menu = gtk_menu_item_get_submenu(parent_item);
  if (menu != NULL && gtk_widget_get_visible(menu)) {
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(menu));
  }

  if (menu == NULL) {
    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(parent_item, menu);
  }

  if (!can_client_issue_orders()) {
    return;
  }

  gtk_container_forall(GTK_CONTAINER(menu),
                       (GtkCallback) gtk_widget_destroy, NULL);

  global_worklists_iterate(pgwl) {
    item = gtk_menu_item_new_with_label(global_worklist_name(pgwl));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", callback,
                     GINT_TO_POINTER(global_worklist_id(pgwl)));
    count++;
  } global_worklists_iterate_end;

  if (count == 0) {
    item = gtk_menu_item_new_with_label(_("(no worklists defined)"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  gtk_widget_show_all(menu);
}

/************************************************************************//**
  Update city report views
****************************************************************************/
static void city_report_update_views(void)
{
  struct city_report_spec *spec;
  GtkTreeView *view;
  GtkTreeViewColumn *col;
  GList *columns, *p;

  view = GTK_TREE_VIEW(city_view);
  fc_assert_ret(view != NULL);

  columns = gtk_tree_view_get_columns(view);

  for (p = columns; p != NULL; p = p->next) {
    col = p->data;
    spec = g_object_get_data(G_OBJECT(col), "city_report_spec");
    gtk_tree_view_column_set_visible(col, spec->show);
  }

  g_list_free(columns);
}

/************************************************************************//**
  User has toggled some column viewing option
****************************************************************************/
static void toggle_view(GtkCheckMenuItem *item, gpointer data)
{
  struct city_report_spec *spec = data;

  spec->show ^= 1;
  city_report_update_views();
}

/************************************************************************//**
  Create view menu for city report menubar.
****************************************************************************/
static void update_view_menu(GtkWidget *show_item)
{
  GtkWidget *menu, *item;
  struct city_report_spec *spec;
  int i;

  menu = gtk_menu_new();
  for (i = 0, spec = city_report_specs + i; i < NUM_CREPORT_COLS; i++, spec++) {
    item = gtk_check_menu_item_new_with_label(spec->explanation);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), spec->show);
    g_signal_connect(item, "toggled", G_CALLBACK(toggle_view), (gpointer)spec);
  }
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(show_item), menu);
}

/************************************************************************//**
  Create menubar for city report
****************************************************************************/
static GtkWidget *create_city_report_menubar(void)
{
  GtkWidget *vbox, *sep, *menubar, *menu, *item;

  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  menubar = gtk_aux_menu_bar_new();
  gtk_container_add(GTK_CONTAINER(vbox), menubar);
  
  item = gtk_menu_item_new_with_mnemonic(_("_Production"));
  city_production_command = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  item = gtk_menu_item_new_with_mnemonic(_("Chan_ge"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  create_change_menu(item);

  item = gtk_menu_item_new_with_mnemonic(_("Add _First"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  create_first_menu(item);

  item = gtk_menu_item_new_with_mnemonic(_("Add _Next"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  create_next_menu(item);

  item = gtk_menu_item_new_with_mnemonic(_("Add _2nd Last"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  create_next_to_last_menu(item);

  item = gtk_menu_item_new_with_mnemonic(_("Add _Last"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  create_last_menu(item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

  item = gtk_menu_item_new_with_label(_("Set Worklist"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_object_set_data(G_OBJECT(item), "item_callback",
                    set_worklist_callback);
  g_signal_connect(menu, "show", G_CALLBACK(production_menu_shown), item);

  item = gtk_menu_item_new_with_label(_("Append Worklist"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_object_set_data(G_OBJECT(item), "item_callback",
                    append_worklist_callback);
  g_signal_connect(menu, "show", G_CALLBACK(production_menu_shown), item);

  item = gtk_menu_item_new_with_mnemonic(_("Clear _Worklist"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(city_clear_worklist_callback), NULL);

  item = gtk_menu_item_new_with_mnemonic(_("Gover_nor"));
  city_governor_command = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
  append_cma_to_menu_item(GTK_MENU_ITEM(item), TRUE);

  item = gtk_menu_item_new_with_mnemonic(_("S_ell"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
  city_sell_command = item;
  create_sell_menu(item);

  item = gtk_menu_item_new_with_mnemonic(_("_Select"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
  create_select_menu(item);

  item = gtk_menu_item_new_with_mnemonic(_("_Display"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
  update_view_menu(item);
  return vbox;
}

/************************************************************************//**
  Sort callback.
****************************************************************************/
static gint cityrep_sort_func(GtkTreeModel *model, GtkTreeIter *a,
                              GtkTreeIter *b, gpointer data)
{
  gint col = GPOINTER_TO_INT(data);
  const gchar *str1, *str2;

  gtk_tree_model_get(model, a, col, &str1, -1);
  gtk_tree_model_get(model, b, col, &str2, -1);

  return cityrepfield_compare(str1, str2);
}

/************************************************************************//**
  Create city report dialog.
****************************************************************************/
static void create_city_report_dialog(bool make_modal)
{
  static char **titles;
  static char (*buf)[128];
  struct city_report_spec *spec;
  GtkWidget *w, *sw, *menubar;
  int i;

  gui_dialog_new(&city_dialog_shell, GTK_NOTEBOOK(top_notebook), NULL, TRUE);
  gui_dialog_set_title(city_dialog_shell, _("Cities"));

  gui_dialog_set_default_size(city_dialog_shell, -1, 420);

  gui_dialog_response_set_callback(city_dialog_shell,
      city_command_callback);

  /* menubar */
  menubar = create_city_report_menubar();
  gui_dialog_add_widget(city_dialog_shell, menubar);

  /* buttons */
  city_total_buy_cost_label = gtk_label_new(NULL);
  gtk_widget_set_hexpand(city_total_buy_cost_label, TRUE);
  gtk_label_set_ellipsize(GTK_LABEL(city_total_buy_cost_label),
                          PANGO_ELLIPSIZE_START);
  gtk_container_add(GTK_CONTAINER(city_dialog_shell->action_area),
                    city_total_buy_cost_label);

  w = gui_dialog_add_button(city_dialog_shell, NULL,
                            _("Buy"), CITY_BUY);
  city_buy_command = w;

  w = gui_dialog_add_button(city_dialog_shell, NULL,
                            _("Inspect"), CITY_POPUP);
  city_popup_command = w;

  w = gui_dialog_add_button(city_dialog_shell, NULL,
                            _("Center"), CITY_CENTER);
  city_center_command = w;

  gui_dialog_set_default_response(city_dialog_shell,
                                  GTK_RESPONSE_CLOSE);

  /* tree view */
  buf = fc_realloc(buf, NUM_CREPORT_COLS * sizeof(buf[0]));
  titles = fc_realloc(titles, NUM_CREPORT_COLS * sizeof(titles[0]));
  for (i = 0; i < NUM_CREPORT_COLS; i++) {
    titles[i] = buf[i];
  }
  get_city_table_header(titles, sizeof(buf[0]));

  city_model = city_report_dialog_store_new();

  city_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(city_model));
  gtk_widget_set_hexpand(city_view, TRUE);
  gtk_widget_set_vexpand(city_view, TRUE);
  g_object_unref(city_model);
  gtk_widget_set_name(city_view, "small_font");
  g_signal_connect(city_view, "row_activated",
		   G_CALLBACK(city_activated_callback), NULL);
  city_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(city_view));
  gtk_tree_selection_set_mode(city_selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect(city_selection, "changed",
	G_CALLBACK(city_selection_changed_callback), NULL);

  for (i = 0, spec = city_report_specs; i < NUM_CREPORT_COLS; i++, spec++) {
    GtkWidget *header;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes(NULL, renderer,
                                                   "text", i, NULL);
    header = gtk_label_new(titles[i]);
    gtk_widget_set_tooltip_text(header, spec->explanation);
    gtk_widget_show(header);
    gtk_tree_view_column_set_widget(col, header);
    gtk_tree_view_column_set_visible(col, spec->show);
    gtk_tree_view_column_set_sort_column_id(col, i);
    gtk_tree_view_column_set_reorderable(col, TRUE);
    g_object_set_data(G_OBJECT(col), "city_report_spec", spec);
    gtk_tree_view_append_column(GTK_TREE_VIEW(city_view), col);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(city_model), i,
                                    cityrep_sort_func, GINT_TO_POINTER(i),
                                    NULL);
  }

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(sw), city_view);

  gtk_container_add(GTK_CONTAINER(city_dialog_shell->vbox), sw);

  city_model_fill(city_model, NULL, NULL);
  gui_dialog_show_all(city_dialog_shell);

  city_selection_changed_callback(city_selection);
}

/************************************************************************//**
  User has chosen to select all cities
****************************************************************************/
static void city_select_all_callback(GtkMenuItem *item, gpointer data)
{
  gtk_tree_selection_select_all(city_selection);
}

/************************************************************************//**
  User has chosen to unselect all cities
****************************************************************************/
static void city_unselect_all_callback(GtkMenuItem *item, gpointer data)
{
  gtk_tree_selection_unselect_all(city_selection);
}

/************************************************************************//**
  User has chosen to invert selection
****************************************************************************/
static void city_invert_selection_callback(GtkMenuItem *item, gpointer data)
{
  ITree it;
  GtkTreeModel *model = GTK_TREE_MODEL(city_model);

  for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
    if (itree_is_selected(city_selection, &it)) {
      itree_unselect(city_selection, &it);
    } else {
      itree_select(city_selection, &it);
    }
  }
}

/************************************************************************//**
  User has chosen to select coastal cities
****************************************************************************/
static void city_select_coastal_callback(GtkMenuItem *item, gpointer data)
{
  ITree it;
  GtkTreeModel *model = GTK_TREE_MODEL(city_model);

  gtk_tree_selection_unselect_all(city_selection);

  for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
    struct city *pcity = city_model_get(model, TREE_ITER_PTR(it));

    if (NULL != pcity && is_terrain_class_near_tile(pcity->tile, TC_OCEAN)) {
      itree_select(city_selection, &it);
    }
  }
}

/************************************************************************//**
  Select all cities on the same continent.
****************************************************************************/
static void same_island_iterate(GtkTreeModel *model, GtkTreePath *path,
                                GtkTreeIter *iter, gpointer data)
{
  struct city *selected_pcity = city_model_get(model, iter);
  ITree it;

  if (NULL == selected_pcity) {
    return;
  }

  for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
    struct city *pcity = city_model_get(model, TREE_ITER_PTR(it));

    if (NULL != pcity
        && (tile_continent(pcity->tile)
            == tile_continent(selected_pcity->tile))) {
      itree_select(city_selection, &it);
    }
  }
}

/************************************************************************//**
  User has chosen to select all cities on same island
****************************************************************************/
static void city_select_same_island_callback(GtkMenuItem *item, gpointer data)
{
  gtk_tree_selection_selected_foreach(city_selection,same_island_iterate,NULL);
}

/************************************************************************//**
  User has chosen to select cities with certain target in production
****************************************************************************/
static void city_select_building_callback(GtkMenuItem *item, gpointer data)
{
  enum production_class_type which = GPOINTER_TO_INT(data);
  ITree it;
  GtkTreeModel *model = GTK_TREE_MODEL(city_model);

  gtk_tree_selection_unselect_all(city_selection);

  for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
    struct city *pcity = city_model_get(model, TREE_ITER_PTR(it));

    if (NULL != pcity
        && ((which == PCT_UNIT && VUT_UTYPE == pcity->production.kind)
            || (which == PCT_NORMAL_IMPROVEMENT
                && VUT_IMPROVEMENT == pcity->production.kind
                && !is_wonder(pcity->production.value.building))
            || (which == PCT_WONDER
                && VUT_IMPROVEMENT == pcity->production.kind
                && is_wonder(pcity->production.value.building)))) {
      itree_select(city_selection, &it);
    }
  }
}

/************************************************************************//**
  Buy the production in one single city.
****************************************************************************/
static void buy_iterate(GtkTreeModel *model, GtkTreePath *path,
                        GtkTreeIter *iter, gpointer data)
{
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    cityrep_buy(pcity);
  }
}

/************************************************************************//**
  Center to one single city.
****************************************************************************/
static void center_iterate(GtkTreeModel *model, GtkTreePath *path,
                           GtkTreeIter *iter, gpointer data)
{
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    center_tile_mapcanvas(pcity->tile);
  }
}

/************************************************************************//**
  Popup the dialog of a single city.
****************************************************************************/
static void popup_iterate(GtkTreeModel *model, GtkTreePath *path,
                          GtkTreeIter *iter, gpointer data)
{
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    if (gui_options.center_when_popup_city) {
      center_tile_mapcanvas(pcity->tile);
    }
    popup_city_dialog(pcity);
  }
}

/************************************************************************//**
  gui_dialog response callback.
****************************************************************************/
static void city_command_callback(struct gui_dialog *dlg, int response,
                                  gpointer data)
{
  switch (response) {
  case CITY_CENTER:
    if (1 == gtk_tree_selection_count_selected_rows(city_selection)) {
      /* Center to city doesn't make sense if many city are selected. */
      gtk_tree_selection_selected_foreach(city_selection, center_iterate,
                                          NULL);
    }
    break;
  case CITY_POPUP:
    gtk_tree_selection_selected_foreach(city_selection, popup_iterate, NULL);
    break;
  case CITY_BUY:
    gtk_tree_selection_selected_foreach(city_selection, buy_iterate, NULL);
    break;
  default:
    gui_dialog_destroy(dlg);
    break;
  }
}

/************************************************************************//**
  User has selected city row from city report.
****************************************************************************/
static void city_activated_callback(GtkTreeView *view, GtkTreePath *path,
                                    GtkTreeViewColumn *col, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GdkWindow *win;
  GdkSeat *seat;
  GdkModifierType mask;

  model = gtk_tree_view_get_model(view);

  if (!gtk_tree_model_get_iter(model, &iter, path)) {
    return;
  }

  win = gdk_get_default_root_window();
  seat = gdk_display_get_default_seat(gdk_window_get_display(win));

  gdk_window_get_device_position(win, gdk_seat_get_pointer(seat),
                                 NULL, NULL, &mask);

  if (!(mask & GDK_CONTROL_MASK)) {
    popup_iterate(model, path, &iter, NULL);
  } else {
    center_iterate(model, path, &iter, NULL);
  }
}

/************************************************************************//**
  Update the city report dialog
****************************************************************************/
void real_city_report_dialog_update(void *unused)
{
  GHashTable *selected;
  ITree iter;
  gint city_id;

  if (NULL == city_dialog_shell) {
    return;
  }

  /* Save the selection. */
  selected = g_hash_table_new(NULL, NULL);
  for (itree_begin(GTK_TREE_MODEL(city_model), &iter);
       !itree_end(&iter); itree_next(&iter)) {
    if (itree_is_selected(city_selection, &iter)) {
      itree_get(&iter, CRD_COL_CITY_ID, &city_id, -1);
      g_hash_table_insert(selected, GINT_TO_POINTER(city_id), NULL);
    }
  }

  /* Update and restore the selection. */
  gtk_list_store_clear(city_model);
  city_model_fill(city_model, city_selection, selected);
  g_hash_table_destroy(selected);

  if (gtk_widget_get_sensitive(city_governor_command)) {
    append_cma_to_menu_item(GTK_MENU_ITEM(city_governor_command), TRUE);
  }

  select_menu_cached = FALSE;
}

/************************************************************************//**
  Update the text for a single city in the city report
****************************************************************************/
void real_city_report_update_city(struct city *pcity)
{
  GtkTreeIter iter;

  if (NULL == city_dialog_shell) {
    return;
  }

  if (!city_model_find(GTK_TREE_MODEL(city_model), &iter, pcity)) {
    gtk_list_store_prepend(city_model, &iter);
  }
  city_model_set(city_model, &iter, pcity);

  update_total_buy_cost();
}

/************************************************************************//**
  Create submenu for changing production target
****************************************************************************/
static void create_change_menu(GtkWidget *item)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  g_signal_connect(menu, "show", G_CALLBACK(popup_change_menu), NULL);

  change_units_item = gtk_menu_item_new_with_label(_("Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), change_units_item);
  change_improvements_item = gtk_menu_item_new_with_label(_("Improvements"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), change_improvements_item);
  change_wonders_item = gtk_menu_item_new_with_label(_("Wonders"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), change_wonders_item);
}

/************************************************************************//**
  Creates the last menu.
****************************************************************************/
static void create_last_menu(GtkWidget *item)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  g_signal_connect(menu, "show", G_CALLBACK(popup_last_menu), NULL);

  last_units_item = gtk_menu_item_new_with_label(_("Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), last_units_item);
  last_improvements_item = gtk_menu_item_new_with_label(_("Improvements"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), last_improvements_item);
  last_wonders_item = gtk_menu_item_new_with_label(_("Wonders"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), last_wonders_item);
}

/************************************************************************//**
  Creates the first menu.
****************************************************************************/
static void create_first_menu(GtkWidget *item)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  g_signal_connect(menu, "show", G_CALLBACK(popup_first_menu), NULL);

  first_units_item = gtk_menu_item_new_with_label(_("Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), first_units_item);
  first_improvements_item = gtk_menu_item_new_with_label(_("Improvements"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), first_improvements_item);
  first_wonders_item = gtk_menu_item_new_with_label(_("Wonders"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), first_wonders_item);
}

/************************************************************************//**
  Creates the next menu.
****************************************************************************/
static void create_next_menu(GtkWidget *item)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  g_signal_connect(menu, "show", G_CALLBACK(popup_next_menu), NULL);

  next_units_item = gtk_menu_item_new_with_label(_("Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), next_units_item);
  next_improvements_item = gtk_menu_item_new_with_label(_("Improvements"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), next_improvements_item);
  next_wonders_item = gtk_menu_item_new_with_label(_("Wonders"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), next_wonders_item);
}

/************************************************************************//**
  Append the "next to last" submenu to the given menu item.
****************************************************************************/
static void create_next_to_last_menu(GtkWidget *parent_item)
{
  GtkWidget *menu, *item;

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent_item), menu);
  g_signal_connect(menu, "show",
                   G_CALLBACK(popup_next_to_last_menu), NULL);

  item = gtk_menu_item_new_with_label(_("Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  next_to_last_units_item = item;

  item = gtk_menu_item_new_with_label(_("Improvements"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  next_to_last_improvements_item = item;

  item = gtk_menu_item_new_with_label(_("Wonders"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  next_to_last_wonders_item = item;
}

/************************************************************************//**
  Create the sell menu (empty).
****************************************************************************/
static void create_sell_menu(GtkWidget *item)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
}

/************************************************************************//**
  Pops up menu where user can select build target.
****************************************************************************/
static void popup_change_menu(GtkMenuShell *menu, gpointer data)
{
  int n;

  n = gtk_tree_selection_count_selected_rows(city_selection);

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(change_improvements_item),
                                   FALSE, FALSE, CO_CHANGE,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(change_units_item),
                                   TRUE, FALSE, CO_CHANGE,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(change_wonders_item),
                                   FALSE, TRUE, CO_CHANGE,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
}

/************************************************************************//**
  Pops up the last menu.
****************************************************************************/
static void popup_last_menu(GtkMenuShell *menu, gpointer data)
{
  int n;

  n = gtk_tree_selection_count_selected_rows(city_selection);

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(last_improvements_item),
                                   FALSE, FALSE, CO_LAST,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(last_units_item),
                                   TRUE, FALSE, CO_LAST,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(last_wonders_item),
                                   FALSE, TRUE, CO_LAST,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
}

/************************************************************************//**
  Pops up the first menu.
****************************************************************************/
static void popup_first_menu(GtkMenuShell *menu, gpointer data)
{
  int n;

  n = gtk_tree_selection_count_selected_rows(city_selection);

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(first_improvements_item),
                                   FALSE, FALSE, CO_FIRST,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(first_units_item),
                                   TRUE, FALSE, CO_FIRST,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(first_wonders_item),
                                   FALSE, TRUE, CO_FIRST,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
}

/************************************************************************//**
  Pops up the next menu.
****************************************************************************/
static void popup_next_menu(GtkMenuShell *menu, gpointer data)
{
  int n;

  n = gtk_tree_selection_count_selected_rows(city_selection);

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(next_improvements_item),
                                   FALSE, FALSE, CO_NEXT,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(next_units_item),
                                   TRUE, FALSE, CO_NEXT,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(next_wonders_item),
                                   FALSE, TRUE, CO_NEXT,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), n);
}

/************************************************************************//**
  Re-create the submenus in the next-to-last production change menu.
****************************************************************************/
static void popup_next_to_last_menu(GtkMenuShell *menu, gpointer data)
{
  GtkWidget *item;
  GCallback callback;
  int n;

  fc_assert_ret(city_selection != NULL);

  n = gtk_tree_selection_count_selected_rows(city_selection);
  callback = G_CALLBACK(select_impr_or_unit_callback);

  item = next_to_last_improvements_item;
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(item),
                                   FALSE, FALSE, CO_NEXT_TO_LAST,
                                   can_city_build_now,
                                   callback, n);
  item = next_to_last_units_item;
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(item),
                                   TRUE, FALSE, CO_NEXT_TO_LAST,
                                   can_city_build_now,
                                   callback, n);
  item = next_to_last_wonders_item;
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(item),
                                   FALSE, TRUE, CO_NEXT_TO_LAST,
                                   can_city_build_now,
                                   callback, n);
}

/************************************************************************//**
  Update the sell menu.
****************************************************************************/
static void recreate_sell_menu(void)
{
  int n;
  GList *children;
  GtkWidget *menu;

  n = gtk_tree_selection_count_selected_rows(city_selection);
  menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(city_sell_command));
  gtk_menu_popdown(GTK_MENU(menu));

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(city_sell_command),
                                   FALSE, FALSE, CO_SELL,
                                   can_city_sell_universal,
                                   G_CALLBACK(select_impr_or_unit_callback),
                                   n);

  menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(city_sell_command));
  children = gtk_container_get_children(GTK_CONTAINER(menu));

  n = g_list_length(children);
  gtk_widget_set_sensitive(city_sell_command, n > 0);
  g_list_free(children);
}

/************************************************************************//**
  Creates select menu
****************************************************************************/
static void create_select_menu(GtkWidget *item)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  g_signal_connect(menu, "show", G_CALLBACK(popup_select_menu), NULL);

  item = gtk_menu_item_new_with_label(_("All Cities"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
		   G_CALLBACK(city_select_all_callback), NULL);

  item = gtk_menu_item_new_with_label(_("No Cities"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
  		   G_CALLBACK(city_unselect_all_callback), NULL);

  item = gtk_menu_item_new_with_label(_("Invert Selection"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
		   G_CALLBACK(city_invert_selection_callback), NULL);


  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);


  item = gtk_menu_item_new_with_label(_("Building Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
  		   G_CALLBACK(city_select_building_callback),
		   GINT_TO_POINTER(PCT_UNIT));

  item = gtk_menu_item_new_with_label( _("Building Improvements"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
  		   G_CALLBACK(city_select_building_callback),
		   GINT_TO_POINTER(PCT_NORMAL_IMPROVEMENT));

  item = gtk_menu_item_new_with_label(_("Building Wonders"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
  		   G_CALLBACK(city_select_building_callback),
		   GINT_TO_POINTER(PCT_WONDER));


  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);


  select_bunit_item =
	gtk_menu_item_new_with_label(_("Building Unit"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_bunit_item);

  select_bimprovement_item =
	gtk_menu_item_new_with_label( _("Building Improvement"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_bimprovement_item);

  select_bwonder_item =
	gtk_menu_item_new_with_label(_("Building Wonder"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_bwonder_item);


  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);


  item = gtk_menu_item_new_with_label(_("Coastal Cities"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
		   G_CALLBACK(city_select_coastal_callback), NULL);

  select_island_item = gtk_menu_item_new_with_label(_("Same Island"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_island_item);
  g_signal_connect(select_island_item, "activate",
		   G_CALLBACK(city_select_same_island_callback), NULL);


  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);


  select_supported_item = gtk_menu_item_new_with_label(_("Supported Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_supported_item);

  select_present_item = gtk_menu_item_new_with_label(_("Units Present"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_present_item);

  select_built_improvements_item =
	gtk_menu_item_new_with_label(_("Improvements in City"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_built_improvements_item);

  select_built_wonders_item =
	gtk_menu_item_new_with_label(_("Wonders in City"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_built_wonders_item);


  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

  
  select_units_item =
	gtk_menu_item_new_with_label(_("Available Units"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_units_item);
  select_improvements_item =
	gtk_menu_item_new_with_label(_("Available Improvements"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_improvements_item);
  select_wonders_item =
	gtk_menu_item_new_with_label(_("Available Wonders"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_wonders_item);
  select_cma_item =
	gtk_menu_item_new_with_label(_("Citizen Governor"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_cma_item);
}

/************************************************************************//**
  Returns whether city is building given target
****************************************************************************/
static bool city_building_impr_or_unit(const struct city *pcity,
                                       const struct universal *target)
{
  return are_universals_equal(&pcity->production, target);
}

/************************************************************************//**
  Popup select menu
****************************************************************************/
static void popup_select_menu(GtkMenuShell *menu, gpointer data)
{
  int n;

  if (select_menu_cached)
    return;

  n = gtk_tree_selection_count_selected_rows(city_selection);
  gtk_widget_set_sensitive(select_island_item, (n > 0));

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_bunit_item),
                                   TRUE, FALSE, CO_NONE,
                                   city_building_impr_or_unit,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_bimprovement_item),
                                   FALSE, FALSE, CO_NONE,
                                   city_building_impr_or_unit,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_bwonder_item),
                                   FALSE, TRUE, CO_NONE,
                                   city_building_impr_or_unit,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_supported_item),
                                   TRUE, FALSE, CO_NONE,
                                   city_unit_supported,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_present_item),
                                   TRUE, FALSE, CO_NONE,
                                   city_unit_present,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_built_improvements_item),
                                   FALSE, FALSE, CO_NONE,
                                   city_building_present,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_built_wonders_item),
                                   FALSE, TRUE, CO_NONE,
                                   city_building_present,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);

  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_improvements_item),
                                   FALSE, FALSE, CO_NONE,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_units_item),
                                   TRUE, FALSE, CO_NONE,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_impr_or_unit_to_menu_item(GTK_MENU_ITEM(select_wonders_item),
                                   FALSE, TRUE, CO_NONE,
                                   can_city_build_now,
                                   G_CALLBACK(select_impr_or_unit_callback), -1);
  append_cma_to_menu_item(GTK_MENU_ITEM(select_cma_item), FALSE);

  select_menu_cached = TRUE;
}

/************************************************************************//**
  Update the value displayed by the "total buy cost" label in the city
  report, or make it blank if nothing can be bought.
****************************************************************************/
static void update_total_buy_cost(void)
{
  GtkWidget *label, *view;
  GList *rows, *p;
  GtkTreeModel *model;
  GtkTreeSelection *sel;
  GtkTreePath *path;
  GtkTreeIter iter;
  struct city *pcity;
  int total = 0;

  view = city_view;
  label = city_total_buy_cost_label;

  if (!view || !label) {
    return;
  }

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  rows = gtk_tree_selection_get_selected_rows(sel, &model);

  for (p = rows; p != NULL; p = p->next) {
    path = p->data;
    if (gtk_tree_model_get_iter(model, &iter, path)) {
      if ((pcity = city_model_get(model, &iter))) {
        total += pcity->client.buy_cost;
      }
    }
    gtk_tree_path_free(path);
  }
  g_list_free(rows);

  if (total > 0) {
    gchar *buf = g_strdup_printf(_("Total Buy Cost: %d"), total);

    gtk_label_set_text(GTK_LABEL(label), buf);
    g_free(buf);
  } else {
    gtk_label_set_text(GTK_LABEL(label), NULL);
  }
}

/************************************************************************//**
  Update city report button sensitivity and total buy cost label when the
  user makes a change in the selection of cities.
****************************************************************************/
static void city_selection_changed_callback(GtkTreeSelection *selection)
{
  int n;
  bool obs_may, plr_may;

  n = gtk_tree_selection_count_selected_rows(selection);
  obs_may = n > 0;
  plr_may = obs_may && can_client_issue_orders();

  gtk_widget_set_sensitive(city_production_command, plr_may);
  gtk_widget_set_sensitive(city_governor_command, plr_may);
  gtk_widget_set_sensitive(city_center_command, obs_may);
  gtk_widget_set_sensitive(city_popup_command, obs_may);
  gtk_widget_set_sensitive(city_buy_command, plr_may);
  if (plr_may) {
    recreate_sell_menu();
  } else {
    gtk_widget_set_sensitive(city_sell_command, FALSE);
  }

  update_total_buy_cost();
}

/************************************************************************//**
  Clear the worklist in one selected city in the city report.
****************************************************************************/
static void clear_worklist_foreach_func(GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer data)
{
  struct city *pcity = city_model_get(model, iter);

  if (NULL != pcity) {
    struct worklist empty;

    worklist_init(&empty);
    city_set_worklist(pcity, &empty);
  }
}

/************************************************************************//**
  Called when the "clear worklist" menu item is activated.
****************************************************************************/
static void city_clear_worklist_callback(GtkMenuItem *item, gpointer data)
{
  struct connection *pconn = &client.conn;

  fc_assert_ret(city_selection != NULL);

  connection_do_buffer(pconn);
  gtk_tree_selection_selected_foreach(city_selection,
                                      clear_worklist_foreach_func, NULL);
  connection_do_unbuffer(pconn);
}

/************************************************************************//**
  After a selection rectangle is defined, make the cities that
  are hilited on the canvas exclusively hilited in the
  City List window.
****************************************************************************/
void hilite_cities_from_canvas(void)
{
  ITree it;
  GtkTreeModel *model;

  if (!city_dialog_shell) return;

  model = GTK_TREE_MODEL(city_model);

  gtk_tree_selection_unselect_all(city_selection);

  for (itree_begin(model, &it); !itree_end(&it); itree_next(&it)) {
    struct city *pcity = city_model_get(model, TREE_ITER_PTR(it));

    if (NULL != pcity && is_city_hilited(pcity)) {
      itree_select(city_selection, &it);
    }
  }
}

/************************************************************************//**
  Toggle a city's hilited status.
****************************************************************************/
void toggle_city_hilite(struct city *pcity, bool on_off)
{
  GtkTreeIter iter;

  if (NULL == city_dialog_shell) {
    return;
  }

  if (city_model_find(GTK_TREE_MODEL(city_model), &iter, pcity)) {
    if (on_off) {
      gtk_tree_selection_select_iter(city_selection, &iter);
    } else {
      gtk_tree_selection_unselect_iter(city_selection, &iter);
    }
  }
}
