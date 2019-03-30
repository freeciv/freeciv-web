/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"

/* common */
#include "fc_types.h"
#include "game.h"
#include "player.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"

/* client */
#include "client_main.h"
#include "control.h"
#include "goto.h"
#include "tilespec.h"
#include "unitselect_common.h"

/* client/gui-gtk-4.0 */
#include "graphics.h"
#include "gui_stuff.h"
#include "gui_main.h"

#include "unitselect.h"

/* Activate this to get more columns (see below) */
#undef DEBUG_USDLG

enum usdlg_column_types {
  COL_PIXBUF,
  COL_TEXT,
  COL_INT
};

enum usdlg_row_types {
  ROW_UNITTYPE,
  ROW_ACTIVITY,
  ROW_UNIT,
  ROW_UNIT_TRANSPORTED
};

/* Basic data (Unit, description, count) */
#define USDLG_COLUMNS_DEFAULT       3
/* Additional data; shown if DEBUG_USDLG */
#define USDLG_COL_UTID          USDLG_COLUMNS_DEFAULT + 0 /* Unit type ID */
#define USDLG_COL_UID           USDLG_COLUMNS_DEFAULT + 1 /* Unit ID */
#define USDLG_COL_LOCATION      USDLG_COLUMNS_DEFAULT + 2 /* Unit location */
#define USDLG_COL_ACTIVITY      USDLG_COLUMNS_DEFAULT + 3 /* Unit activity */
#define USDLG_COL_ROW_TYPE      USDLG_COLUMNS_DEFAULT + 4 /* Row type */
#define USDLG_COLUMNS_DEBUG     USDLG_COLUMNS_DEFAULT + 5
/* Layout options; never shown */
#define USDLG_COL_STYLE         USDLG_COLUMNS_DEBUG + 0
#define USDLG_COL_WEIGHT        USDLG_COLUMNS_DEBUG + 1
#define USDLG_COLUMNS_ALL       USDLG_COLUMNS_DEBUG + 2

#ifdef DEBUG_USDLG
  #define USDLG_COLUMNS_SHOW    USDLG_COLUMNS_DEBUG
#else
  #define USDLG_COLUMNS_SHOW    USDLG_COLUMNS_DEFAULT
#endif /* DEBUG_USDLG */

enum usdlg_column_types usdlg_col_types[USDLG_COLUMNS_ALL] = {
  COL_PIXBUF, /* Unit */
  COL_TEXT,   /* Description */
  COL_INT,    /* Count */
  COL_INT,    /* Debug: unit type */
  COL_INT,    /* Debug: unit ID */
  COL_INT,    /* Debug: location */
  COL_INT,    /* Debug: activity */
  COL_INT,    /* Debug: row type */
  COL_INT,    /* Layout: style */
  COL_INT     /* Layout: width */
};

static const char *usdlg_col_titles[USDLG_COLUMNS_ALL] = {
  N_("Unit"),
  N_("Description"),
  N_("Count"),
  "[Unittype]", /* Only for debug, no translation! */
  "[Unit ID]",
  "[Location]",
  "[Activity]",
  "[Row type]",
  "[Style]",
  "[Width]"
};

enum usdlg_cmd {
  USDLG_CMD_SELECT,
  USDLG_CMD_DESELECT,
  USDLG_CMD_READY,
  USDLG_CMD_SENTRY,
  USDLG_CMD_CENTER,
  USDLG_CMD_FOCUS,
  USDLG_CMD_LAST
};

struct unit_select_dialog {
  struct tile *ptile;
  int unit_id_focus;

  GtkWidget *shell;
  GtkWidget *notebook;

  struct {
    GtkTreeStore *store;
    GtkWidget *view;
    GtkTreePath *path;
  } units;

  struct {
    GtkTreeStore *store;
    GtkWidget *page;
    GtkWidget *view;
    GtkTreePath *path;

    GtkWidget *cmd[USDLG_CMD_LAST];
  } tabs[SELLOC_COUNT];
};

/* The unit selection dialog; should only be used in usdlg_get(). */
static struct unit_select_dialog *unit_select_dlg = NULL;

static struct unit_select_dialog *usdlg_get(bool create);
static struct unit_select_dialog *usdlg_create(void);
static void usdlg_destroy(void);
static void usdlg_destroy_callback(GObject *object, gpointer data);
static void usdlg_tile(struct unit_select_dialog *pdialog,
                       struct tile *ptile);
static void usdlg_refresh(struct unit_select_dialog *pdialog);

static void usdlg_tab_select(struct unit_select_dialog *pdialog,
                             const char *title,
                             enum unit_select_location_mode loc);
static GtkTreeStore *usdlg_tab_store_new(void);
static bool usdlg_tab_update(struct unit_select_dialog *pdialog, 
                             struct usdata_hash *ushash,
                             enum unit_select_location_mode loc);
static void usdlg_tab_append_utype(GtkTreeStore *store,
                                   enum unit_select_location_mode loc,
                                   struct unit_type *putype,
                                   GtkTreeIter *it);
static void usdlg_tab_append_activity(GtkTreeStore *store,
                                      enum unit_select_location_mode loc,
                                      const struct unit_type *putype,
                                      enum unit_activity act,
                                      int count, GtkTreeIter *it,
                                      GtkTreeIter *parent);
static void usdlg_tab_append_units(struct unit_select_dialog *pdialog,
                                   enum unit_select_location_mode loc,
                                   enum unit_activity act,
                                   const struct unit *punit,
                                   bool transported, GtkTreeIter *it,
                                   GtkTreeIter *parent);

static void usdlg_cmd_ready(GObject *object, gpointer data);
static void usdlg_cmd_sentry(GObject *object, gpointer data);
static void usdlg_cmd_select(GObject *object, gpointer data);
static void usdlg_cmd_deselect(GObject *object, gpointer data);
static void usdlg_cmd_exec(GObject *object, gpointer mode_data,
                           enum usdlg_cmd cmd);
static void usdlg_cmd_exec_unit(struct unit *punit, enum usdlg_cmd cmd);
static void usdlg_cmd_center(GObject *object, gpointer data);
static void usdlg_cmd_focus(GObject *object, gpointer data);
static void usdlg_cmd_focus_real(GtkTreeView *view);
static void usdlg_cmd_row_activated(GtkTreeView *view, GtkTreePath *path,
                                    GtkTreeViewColumn *col, gpointer data);
static void usdlg_cmd_cursor_changed(GtkTreeView *view, gpointer data);


/*************************************************************************//**
  Popup the unit selection dialog.
*****************************************************************************/
void unit_select_dialog_popup_main(struct tile *ptile, bool create)
{
  struct unit_select_dialog *pdialog;

  /* Create the dialog if it is requested. */
  pdialog = usdlg_get(create);

  /* Present the unit selection dialog if it exists. */
  if (pdialog) {
    /* Show all. */
    gtk_widget_show(GTK_WIDGET(pdialog->shell));
    /* Update tile. */
    usdlg_tile(pdialog, ptile);
    /* Refresh data and hide unused tabs. */
    usdlg_refresh(pdialog);
  }
}

/*************************************************************************//**
  Popdown the unit selection dialog.
*****************************************************************************/
void unit_select_dialog_popdown(void)
{
  usdlg_destroy();
}

/*************************************************************************//**
  Get the current unit selection dialog. Create it if needed and 'create' is
  TRUE.
*****************************************************************************/
static struct unit_select_dialog *usdlg_get(bool create)
{
  if (unit_select_dlg) {
    /* Return existing dialog. */
    return unit_select_dlg;
  } else if (create) {
    /* Create new dialog. */
    unit_select_dlg = usdlg_create();
    return unit_select_dlg;
  } else {
    /* Nothing. */
    return NULL;
  }
}

/*************************************************************************//**
  Create a new unit selection dialog.
*****************************************************************************/
static struct unit_select_dialog *usdlg_create(void)
{
  GtkWidget *vbox;
  GtkWidget *close_cmd;
  struct unit_select_dialog *pdialog;

  /* Create a container for the dialog. */
  pdialog = fc_calloc(1, sizeof(*pdialog));

  /* No tile defined. */
  pdialog->ptile = NULL;

  /* Create the dialog. */
  pdialog->shell = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(pdialog->shell), _("Unit selection"));
  setup_dialog(pdialog->shell, toplevel);
  g_signal_connect(pdialog->shell, "destroy",
                   G_CALLBACK(usdlg_destroy_callback), pdialog);
  gtk_window_set_position(GTK_WINDOW(pdialog->shell), GTK_WIN_POS_MOUSE);
  gtk_widget_realize(pdialog->shell);

  vbox = gtk_dialog_get_content_area(GTK_DIALOG(pdialog->shell));

  /* Notebook. */
  pdialog->notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pdialog->notebook),
                           GTK_POS_BOTTOM);
  gtk_box_pack_start(GTK_BOX(vbox), pdialog->notebook);

  /* Append pages. */
  usdlg_tab_select(pdialog, _("_Units"), SELLOC_UNITS);
  usdlg_tab_select(pdialog, _("_Tile"), SELLOC_TILE);
  usdlg_tab_select(pdialog, _("C_ontinent"), SELLOC_CONT);
  usdlg_tab_select(pdialog, _("_Land"), SELLOC_LAND);
  usdlg_tab_select(pdialog, _("_Sea"), SELLOC_SEA);
  usdlg_tab_select(pdialog, _("_Both"), SELLOC_BOTH);
  usdlg_tab_select(pdialog, _("_World"), SELLOC_WORLD);

  /* Buttons. */
  close_cmd = gtk_dialog_add_button(GTK_DIALOG(pdialog->shell),
                                    _("Close"), GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response(GTK_DIALOG(pdialog->shell),
                                  GTK_RESPONSE_CLOSE);
  g_signal_connect(close_cmd, "clicked",
                   G_CALLBACK(usdlg_destroy_callback), pdialog);

  return pdialog;
}

/*************************************************************************//**
  Destroy a unit selection dialog.
*****************************************************************************/
static void usdlg_destroy(void)
{
  if (unit_select_dlg) {
    gtk_widget_destroy(GTK_WIDGET(unit_select_dlg->shell));
    free(unit_select_dlg);
  }
  unit_select_dlg = NULL;
}

/*************************************************************************//**
  Callback for the destruction of the dialog.
*****************************************************************************/
static void usdlg_destroy_callback(GObject *object, gpointer data)
{
  usdlg_destroy();
}

/*************************************************************************//**
  Set the reference tile.
*****************************************************************************/
static void usdlg_tile(struct unit_select_dialog *pdialog,
                       struct tile *ptile)
{
  if (!pdialog) {
    return;
  }

  /* Check for a valid tile. */
  if (ptile != NULL) {
    pdialog->ptile = ptile;
  } else if (pdialog->ptile == NULL) {
    struct unit *punit = head_of_units_in_focus();

    if (punit) {
      pdialog->ptile = unit_tile(punit);
      center_tile_mapcanvas(pdialog->ptile);
    } else {
      pdialog->ptile = get_center_tile_mapcanvas();
    }
  }
}

/*************************************************************************//**
  Refresh the dialog.
*****************************************************************************/
static void usdlg_refresh(struct unit_select_dialog *pdialog)
{
  struct usdata_hash *ushash = NULL;
  enum unit_select_location_mode loc;

  if (!pdialog) {
    return;
  }

  /* Sort units into the hash. */
  ushash = usdlg_data_new(pdialog->ptile);
  /* Update all tabs. */
  for (loc = unit_select_location_mode_begin();
       loc != unit_select_location_mode_end();
       loc = unit_select_location_mode_next(loc)) {
    bool show = usdlg_tab_update(pdialog, ushash, loc);

    if (!show) {
      gtk_widget_hide(pdialog->tabs[loc].page);
    } else {
      gtk_widget_show(pdialog->tabs[loc].page);

      if (pdialog->tabs[loc].path) {
        gtk_tree_view_expand_row(GTK_TREE_VIEW(pdialog->tabs[loc].view),
                                 pdialog->tabs[loc].path,FALSE);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(pdialog->tabs[loc].view),
                                 pdialog->tabs[loc].path, NULL, FALSE);
        gtk_tree_path_free(pdialog->tabs[loc].path);
        pdialog->tabs[loc].path = NULL;
      }
    }
  }
  /* Destroy the hash. */
  usdlg_data_destroy(ushash);
}

/*************************************************************************//**
  +--------------------------------+
  | +-----------------+----------+ |
  | | (unit list)     | select   | |
  | |                 | deselect | |
  | |                 |          | |
  | |                 | center   | |
  | |                 | focus    | |
  | +-----------------+----------+ |
  | | tabs | ... |                 |
  |                          close |
  +--------------------------------+
*****************************************************************************/
static void usdlg_tab_select(struct unit_select_dialog *pdialog,
                             const char *title,
                             enum unit_select_location_mode loc)
{
  GtkWidget *page, *label, *hbox, *vbox, *view, *sw;
  GtkTreeStore *store;
  static bool titles_done;
  int i;

  page = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(page),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_margin_start(page, 8);
  gtk_widget_set_margin_end(page, 8);
  gtk_widget_set_margin_top(page, 8);
  gtk_widget_set_margin_bottom(page, 8);
  pdialog->tabs[loc].page = page;

  label = gtk_label_new_with_mnemonic(title);
  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  hbox = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(page), hbox);

  store = usdlg_tab_store_new();
  pdialog->tabs[loc].store = store;

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_hexpand(view, TRUE);
  gtk_widget_set_vexpand(view, TRUE);
  pdialog->tabs[loc].view = view;
  g_object_unref(store);

  g_signal_connect(view, "row-activated", G_CALLBACK(usdlg_cmd_row_activated),
                   (gpointer *)loc);
  g_signal_connect(view, "cursor-changed",
                   G_CALLBACK(usdlg_cmd_cursor_changed), (gpointer *)loc);

  /* Translate titles. */
  intl_slist(ARRAY_SIZE(usdlg_col_titles), usdlg_col_titles, &titles_done);

  for (i = 0; i < USDLG_COLUMNS_SHOW; i++) {
    GtkTreeViewColumn *column = NULL;
    GtkCellRenderer *renderer = NULL;

    switch (usdlg_col_types[i]) {
    case COL_PIXBUF:
      renderer = gtk_cell_renderer_pixbuf_new();
      column = gtk_tree_view_column_new_with_attributes(
                 usdlg_col_titles[i], renderer, "pixbuf", i, NULL);
      gtk_tree_view_column_set_expand(column, FALSE);
      break;
    case COL_TEXT:
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes(
                 usdlg_col_titles[i], renderer, "text", i,
                 "style", USDLG_COL_STYLE, "weight", USDLG_COL_WEIGHT, NULL);
      gtk_tree_view_column_set_expand(column, TRUE);
      break;
    case COL_INT:
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes(
                 usdlg_col_titles[i], renderer, "text", i,
                 "style", USDLG_COL_STYLE, "weight", USDLG_COL_WEIGHT, NULL);
      g_object_set(renderer, "xalign", 1.0, NULL);
      gtk_tree_view_column_set_alignment(column, 1.0);
      gtk_tree_view_column_set_expand(column, FALSE);
      break;
    }

    fc_assert_ret(column != NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
  }

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(sw), 300);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_container_add(GTK_CONTAINER(hbox), sw);

  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(hbox), vbox);

  /* button box 1: ready, sentry */

  pdialog->tabs[loc].cmd[USDLG_CMD_READY]
    = gtk_button_new_with_mnemonic(_("Ready"));
  gtk_container_add(GTK_CONTAINER(vbox),
                    pdialog->tabs[loc].cmd[USDLG_CMD_READY]);
  g_signal_connect(pdialog->tabs[loc].cmd[USDLG_CMD_READY], "clicked",
                   G_CALLBACK(usdlg_cmd_ready), (gpointer *)loc);
  gtk_widget_set_sensitive(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_READY]), FALSE);

  pdialog->tabs[loc].cmd[USDLG_CMD_SENTRY]
    = gtk_button_new_with_mnemonic(_("Sentry"));
  gtk_widget_set_margin_bottom(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_SENTRY]), 10);
  gtk_container_add(GTK_CONTAINER(vbox),
                    pdialog->tabs[loc].cmd[USDLG_CMD_SENTRY]);
  g_signal_connect(pdialog->tabs[loc].cmd[USDLG_CMD_SENTRY], "clicked",
                   G_CALLBACK(usdlg_cmd_sentry), (gpointer *)loc);
  gtk_widget_set_sensitive(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_SENTRY]), FALSE);

  /* button box 2: select, deselect */

  pdialog->tabs[loc].cmd[USDLG_CMD_SELECT]
    = gtk_button_new_with_mnemonic(_("_Select"));
  gtk_container_add(GTK_CONTAINER(vbox),
                    pdialog->tabs[loc].cmd[USDLG_CMD_SELECT]);
  g_signal_connect(pdialog->tabs[loc].cmd[USDLG_CMD_SELECT], "clicked",
                   G_CALLBACK(usdlg_cmd_select), (gpointer *)loc);
  gtk_widget_set_sensitive(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_SELECT]), FALSE);

  pdialog->tabs[loc].cmd[USDLG_CMD_DESELECT]
    = gtk_button_new_with_mnemonic(_("_Deselect"));
  gtk_widget_set_margin_bottom(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_DESELECT]), 10);
  gtk_container_add(GTK_CONTAINER(vbox),
                    pdialog->tabs[loc].cmd[USDLG_CMD_DESELECT]);
  g_signal_connect(pdialog->tabs[loc].cmd[USDLG_CMD_DESELECT], "clicked",
                   G_CALLBACK(usdlg_cmd_deselect), (gpointer *)loc);
  gtk_widget_set_sensitive(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_DESELECT]), FALSE);

  /* button box 3: center, focus */

  pdialog->tabs[loc].cmd[USDLG_CMD_CENTER]
    = gtk_button_new_with_mnemonic(_("C_enter"));
  gtk_container_add(GTK_CONTAINER(vbox),
                    pdialog->tabs[loc].cmd[USDLG_CMD_CENTER]);
  g_signal_connect(pdialog->tabs[loc].cmd[USDLG_CMD_CENTER], "clicked",
                   G_CALLBACK(usdlg_cmd_center), (gpointer *)loc);
  gtk_widget_set_sensitive(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_CENTER]), FALSE);

  pdialog->tabs[loc].cmd[USDLG_CMD_FOCUS]
    = gtk_button_new_with_mnemonic(_("_Focus"));
  gtk_container_add(GTK_CONTAINER(vbox),
                    pdialog->tabs[loc].cmd[USDLG_CMD_FOCUS]);
  g_signal_connect(pdialog->tabs[loc].cmd[USDLG_CMD_FOCUS], "clicked",
                   G_CALLBACK(usdlg_cmd_focus), (gpointer *)loc);
  gtk_widget_set_sensitive(
    GTK_WIDGET(pdialog->tabs[loc].cmd[USDLG_CMD_FOCUS]), FALSE);
}

/*************************************************************************//**
  Create a player dialog store.
*****************************************************************************/
static GtkTreeStore *usdlg_tab_store_new(void)
{
  GtkTreeStore *store;
  GType model_types[USDLG_COLUMNS_ALL];
  int i;

  for (i = 0; i < USDLG_COLUMNS_ALL; i++) {
    switch (usdlg_col_types[i]) {
    case COL_PIXBUF:
      model_types[i] = GDK_TYPE_PIXBUF;
      break;
    case COL_TEXT:
      model_types[i] = G_TYPE_STRING;
      break;
    case COL_INT:
      model_types[i] = G_TYPE_INT;
      break;
    }
  }

  store = gtk_tree_store_newv(i, model_types);

  return store;
}

/*************************************************************************//**
  Update on tab of the dialog.
*****************************************************************************/
static bool usdlg_tab_update(struct unit_select_dialog *pdialog,
                             struct usdata_hash *ushash,
                             enum unit_select_location_mode loc)
{
  bool show = FALSE;
  GtkTreeStore *store;

  fc_assert_ret_val(ushash, FALSE);
  fc_assert_ret_val(pdialog != NULL, FALSE);

  store = pdialog->tabs[loc].store;

  /* clear current store. */
  gtk_tree_store_clear(GTK_TREE_STORE(store));

  /* Iterate over all unit types. */
  if (loc == SELLOC_UNITS) {
    /* Special case - show all units on this tile in their transports. */
    unit_type_iterate(utype) {
      struct usdata *data;

      usdata_hash_lookup(ushash, utype_index(utype), &data);

      if (!data) {
        continue;
      }

      activity_type_iterate(act) {
        if (unit_list_size(data->units[loc][act]) == 0) {
          continue;
        }

        unit_list_iterate(data->units[loc][act], punit) {
          GtkTreeIter it_unit;

          usdlg_tab_append_units(pdialog, loc, act, punit, FALSE,
                                 &it_unit, NULL);
        } unit_list_iterate_end;

        /* Show this tab. */
        show = TRUE;
      } activity_type_iterate_end;
    } unit_type_iterate_end;
  } else {
    unit_type_iterate(utype) {
      struct usdata *data;
      bool first = TRUE;
      GtkTreeIter it_utype;
      GtkTreePath *path;
      int count = 0;

      usdata_hash_lookup(ushash, utype_index(utype), &data);

      if (!data) {
        continue;
      }

      activity_type_iterate(act) {
        GtkTreeIter it_act;

        if (unit_list_size(data->units[loc][act]) == 0) {
          continue;
        }

        /* Level 1: Display unit type. */
        if (first) {
          usdlg_tab_append_utype(GTK_TREE_STORE(store), loc, data->utype,
                                 &it_utype);
          first = FALSE;
        }

        /* Level 2: Display unit activities. */
        usdlg_tab_append_activity(GTK_TREE_STORE(store), loc, data->utype,
                                  act, unit_list_size(data->units[loc][act]),
                                  &it_act, &it_utype);

        /* Level 3: Display all units with this activitiy
         *          (and transported units in further level(s)). */
        unit_list_iterate(data->units[loc][act], punit) {
          GtkTreeIter it_unit;

          usdlg_tab_append_units(pdialog, loc, act, punit, FALSE,
                                 &it_unit, &it_act);
        } unit_list_iterate_end;

        count += unit_list_size(data->units[loc][act]);

        /* Update sum of units with this type. */
        gtk_tree_store_set(GTK_TREE_STORE(store), &it_utype, 2, count, -1);

        /* Expand to the activities. */
        path
          = gtk_tree_model_get_path(GTK_TREE_MODEL(pdialog->tabs[loc].store),
                                    &it_utype);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(pdialog->tabs[loc].view), path,
                                 FALSE);
        gtk_tree_path_free(path);

        /* Show this tab. */
        show = TRUE;
      } activity_type_iterate_end;
    } unit_type_iterate_end;
  }

  return show;
}

/*************************************************************************//**
  Append the data for one unit type.
*****************************************************************************/
static void usdlg_tab_append_utype(GtkTreeStore *store,
                                   enum unit_select_location_mode loc,
                                   struct unit_type *putype,
                                   GtkTreeIter *it)
{
  GdkPixbuf *pix;
  char buf[128];

  fc_assert_ret(store != NULL);
  fc_assert_ret(putype != NULL);

  /* Add this item. */
  gtk_tree_store_append(GTK_TREE_STORE(store), it, NULL);

  /* Create a icon */
  {
    struct canvas canvas_store = FC_STATIC_CANVAS_INIT;

    canvas_store.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));

    put_unittype(putype, &canvas_store, 1.0, 0, 0);
    pix = surface_get_pixbuf(canvas_store.surface, tileset_full_tile_width(tileset),
        tileset_full_tile_height(tileset));
    cairo_surface_destroy(canvas_store.surface);
  }

  /* The name of the unit. */
  fc_snprintf(buf, sizeof(buf), "%s", utype_name_translation(putype));

  /* Add it to the tree. */
  gtk_tree_store_set(GTK_TREE_STORE(store), it,
                     0, pix,                            /* Unit pixmap */
                     1, buf,                            /* Text */
                     2, -1, /* will be set later */     /* Number of units */
                     3, utype_index(putype),            /* Unit type ID */
                     /* 4: not set */                   /* Unit ID */
                     5, loc,                            /* Unit location */
                     /* 6: not set */                   /* Unit activity */
                     7, ROW_UNITTYPE,                   /* Row type */
                     8, PANGO_STYLE_NORMAL,             /* Style */
                     9, PANGO_WEIGHT_BOLD,              /* Weight */
                     -1);
  g_object_unref(pix);
}

/*************************************************************************//**
  Append the unit activity.
*****************************************************************************/
static void usdlg_tab_append_activity(GtkTreeStore *store,
                                      enum unit_select_location_mode loc,
                                      const struct unit_type *putype,
                                      enum unit_activity act,
                                      int count, GtkTreeIter *it,
                                      GtkTreeIter *parent)
{
  char buf[128] = "";

  fc_assert_ret(store != NULL);
  fc_assert_ret(putype != NULL);

  /* Add this item. */
  gtk_tree_store_append(GTK_TREE_STORE(store), it, parent);

  /* The activity. */
  fc_snprintf(buf, sizeof(buf), "%s", get_activity_text(act));

  /* Add it to the tree. */
  gtk_tree_store_set(GTK_TREE_STORE(store), it,
                     /* 0: not set */                   /* Unit pixmap */
                     1, buf,                            /* Text */
                     2, count,                          /* Number of units */
                     3, utype_index(putype),            /* Unit type ID */
                     /* 4: not set */                   /* Unit ID */
                     5, loc,                            /* Unit location */
                     6, act,                            /* Unit activity */
                     7, ROW_ACTIVITY,                   /* Row type */
                     8, PANGO_STYLE_NORMAL,             /* Style */
                     9, PANGO_WEIGHT_NORMAL,            /* Weight */
                     -1);
}

/*************************************************************************//**
  Get an unit selection list item suitable image of the specified unit.

  Caller is responsible for getting rid of the returned image after use.
*****************************************************************************/
GdkPixbuf *usdlg_get_unit_image(const struct unit *punit)
{
  GdkPixbuf *out;
  struct canvas canvas_store = FC_STATIC_CANVAS_INIT;

  canvas_store.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
      tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));

  put_unit(punit, &canvas_store, 1.0, 0, 0);
  out = surface_get_pixbuf(canvas_store.surface,
                           tileset_full_tile_width(tileset),
                           tileset_full_tile_height(tileset));
  cairo_surface_destroy(canvas_store.surface);

  return out;
}

/*************************************************************************//**
  Get an unit selection list item suitable description of the specified
  unit.
*****************************************************************************/
const char *usdlg_get_unit_descr(const struct unit *punit)
{
  static char buf[248] = "";
  char buf2[248] = "";
  struct city *phome;

  phome = game_city_by_number(punit->homecity);
  if (phome) {
    fc_snprintf(buf2, sizeof(buf2), "%s", city_name_get(phome));
  } else if (unit_owner(punit) == client_player()
             || client_is_global_observer()) {
    /* TRANS: used in place of unit home city name */
    sz_strlcpy(buf2, _("no home city"));
  } else {
    /* TRANS: used in place of unit home city name */
    sz_strlcpy(buf2, _("unknown"));
  }
#ifdef FREECIV_DEBUG
  /* Strings only used in debug builds, don't bother with i18n */
  fc_snprintf(buf, sizeof(buf), "%s [Unit ID %d]\n(%s)\nCoordinates: (%d,%d)",
              unit_name_translation(punit), punit->id, buf2,
              TILE_XY(unit_tile(punit)));
  {
    struct unit *ptrans = unit_transport_get(punit);

    if (ptrans) {
      cat_snprintf(buf, sizeof(buf), "\nTransported by unit ID %d",
                   ptrans->id);
    }
  }
#else /* FREECIV_DEBUG */
  /* TRANS: unit type and home city, e.g. "Transport\n(New Orleans)" */
  fc_snprintf(buf, sizeof(buf), _("%s\n(%s)"), unit_name_translation(punit),
              buf2);
#endif /* FREECIV_DEBUG */

  return buf;
}

/*************************************************************************//**
  Append units (recursively).
*****************************************************************************/
static void usdlg_tab_append_units(struct unit_select_dialog *pdialog,
                                   enum unit_select_location_mode loc,
                                   enum unit_activity act,
                                   const struct unit *punit,
                                   bool transported, GtkTreeIter *it,
                                   GtkTreeIter *parent)
{
  const char *text;
  GdkPixbuf *pix;
  enum usdlg_row_types row = ROW_UNIT;
  int style = PANGO_STYLE_NORMAL;
  int weight = PANGO_WEIGHT_NORMAL;
  GtkTreeStore *store;

  fc_assert_ret(pdialog != NULL);
  fc_assert_ret(punit != NULL);

  store = pdialog->tabs[loc].store;


  /* Add this item. */
  gtk_tree_store_append(GTK_TREE_STORE(store), it, parent);

  /* Unit gfx */
  pix = usdlg_get_unit_image(punit);

  text = usdlg_get_unit_descr(punit);

  if (transported) {
    weight = PANGO_WEIGHT_NORMAL;
    style = PANGO_STYLE_ITALIC;
    row = ROW_UNIT_TRANSPORTED;
  }

  /* Add it to the tree. */
  gtk_tree_store_set(GTK_TREE_STORE(store), it,
                     0, pix,                            /* Unit pixmap */
                     1, text,                           /* Text */
                     2, 1,                              /* Number of units */
                     3, utype_index(unit_type_get(punit)), /* Unit type ID */
                     4, punit->id,                      /* Unit ID */
                     5, loc,                            /* Unit location */
                     6, act,                            /* Unit activity */
                     7, row,                            /* Row type */
                     8, style,                          /* Style */
                     9, weight,                         /* Weight */
                     -1);
  g_object_unref(pix);

  if (get_transporter_occupancy(punit) > 0) {
    unit_list_iterate(unit_transport_cargo(punit), pcargo) {
      GtkTreeIter it_cargo;

      usdlg_tab_append_units(pdialog, loc, act, pcargo, TRUE, &it_cargo, it);
    } unit_list_iterate_end;
  }

  if (!transported && unit_is_in_focus(punit)
      && get_num_units_in_focus() == 1) {
    /* Put the keyboard focus on the selected unit. It isn't transported.
     * Selection maps to keyboard focus since it alone is selected. */
    fc_assert_action(pdialog->tabs[loc].path == NULL,
                     /* Don't leak memory. */
                     gtk_tree_path_free(pdialog->tabs[loc].path));

    pdialog->tabs[loc].path
      = gtk_tree_model_get_path(GTK_TREE_MODEL(store), it);
  }
}

/*************************************************************************//**
  Callback for the ready button.
*****************************************************************************/
static void usdlg_cmd_ready(GObject *object, gpointer data)
{
  usdlg_cmd_exec(object, data, USDLG_CMD_READY);
}

/*************************************************************************//**
  Callback for the sentry button.
*****************************************************************************/
static void usdlg_cmd_sentry(GObject *object, gpointer data)
{
  usdlg_cmd_exec(object, data, USDLG_CMD_SENTRY);
}

/*************************************************************************//**
  Callback for the select button.
*****************************************************************************/
static void usdlg_cmd_select(GObject *object, gpointer data)
{
  usdlg_cmd_exec(object, data, USDLG_CMD_SELECT);
}

/*************************************************************************//**
  Callback for the deselect button.
*****************************************************************************/
static void usdlg_cmd_deselect(GObject *object, gpointer data)
{
  usdlg_cmd_exec(object, data, USDLG_CMD_DESELECT);
}

/*************************************************************************//**
  Main function for the callbacks.
*****************************************************************************/
static void usdlg_cmd_exec(GObject *object, gpointer mode_data,
                           enum usdlg_cmd cmd)
{
  enum unit_select_location_mode loc_mode = (enum unit_select_location_mode) mode_data;
  GtkTreeView *view;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;
  gint row;
  struct unit_select_dialog *pdialog = usdlg_get(FALSE);

  fc_assert_ret(pdialog != NULL);
  fc_assert_ret(unit_select_location_mode_is_valid(loc_mode));

  if (!can_client_change_view() || !can_client_control()) {
    return;
  }

  view = GTK_TREE_VIEW(pdialog->tabs[loc_mode].view);
  selection = gtk_tree_view_get_selection(view);

  if (!gtk_tree_selection_get_selected(selection, &model, &it)) {
    log_debug("No selection");
    return;
  }
  gtk_tree_model_get(model, &it, USDLG_COL_ROW_TYPE, &row, -1);

  switch (row) {
  case ROW_UNITTYPE:
    {
      gint loc, utid;
      struct usdata_hash *ushash;
      struct usdata *data;

      gtk_tree_model_get(model, &it, USDLG_COL_LOCATION, &loc,
                         USDLG_COL_UTID, &utid, -1);

      /* We can't be sure that all units still exists - recalc the data. */
      ushash = usdlg_data_new(pdialog->ptile);

      usdata_hash_lookup(ushash, utid, &data);
      if (data != NULL) {
        activity_type_iterate(act) {
          if (unit_list_size(data->units[loc][act]) == 0) {
            continue;
          }

          unit_list_iterate(data->units[loc][act], punit) {
            usdlg_cmd_exec_unit(punit, cmd);
          } unit_list_iterate_end;
        } activity_type_iterate_end;
      }

      /* Destroy the hash. */
      usdlg_data_destroy(ushash);
    }
    break;
  case ROW_ACTIVITY:
    {
      gint loc, act, utid;
      struct usdata_hash *ushash;
      struct usdata *data;

      gtk_tree_model_get(model, &it, USDLG_COL_ACTIVITY, &act,
                         USDLG_COL_LOCATION, &loc, USDLG_COL_UTID, &utid, -1);

      /* We can't be sure that all units still exists - recalc the data. */
      ushash = usdlg_data_new(pdialog->ptile);

      usdata_hash_lookup(ushash, utid, &data);
      if (data != NULL 
          && unit_list_size(data->units[loc][act]) != 0) {
        unit_list_iterate(data->units[loc][act], punit) {
          usdlg_cmd_exec_unit(punit, cmd);
        } unit_list_iterate_end;
      }

      /* Destroy the hash. */
      usdlg_data_destroy(ushash);
    }
    break;
  case ROW_UNIT:
  case ROW_UNIT_TRANSPORTED:
    {
      gint uid;
      struct unit *punit;

      gtk_tree_model_get(model, &it, USDLG_COL_UID, &uid, -1);

      punit = game_unit_by_number(uid);

      if (!punit) {
        log_debug("Unit vanished (Unit ID %d)!", uid);
        return;
      }

      usdlg_cmd_exec_unit(punit, cmd);
    }
    break;
  }

  /* Update focus. */
  unit_focus_update();
  /* Refresh dialog. */
  usdlg_refresh(pdialog);
}

/*************************************************************************//**
  Update one unit (select/deselect/ready/sentry).
*****************************************************************************/
static void usdlg_cmd_exec_unit(struct unit *punit, enum usdlg_cmd cmd)
{
  fc_assert_ret(punit);

  switch (cmd) {
  case USDLG_CMD_SELECT:
    if (!unit_is_in_focus(punit)) {
      unit_focus_add(punit);
    }
  break;
  case USDLG_CMD_DESELECT:
    if (unit_is_in_focus(punit)) {
      unit_focus_remove(punit);
    }
    break;
  case USDLG_CMD_READY:
    if (punit->activity != ACTIVITY_IDLE) {
      request_new_unit_activity(punit, ACTIVITY_IDLE);
    }
    break;
  case USDLG_CMD_SENTRY:
    if (punit->activity != ACTIVITY_SENTRY) {
      request_new_unit_activity(punit, ACTIVITY_SENTRY);
    }
    break;
  case USDLG_CMD_CENTER:
  case USDLG_CMD_FOCUS:
    /* Nothing here. It is done in its own functions. */
    break;
  case USDLG_CMD_LAST:
    /* Should never happen. */
    fc_assert_ret(cmd != USDLG_CMD_LAST);
    break;
  }
}

/*************************************************************************//**
  Callback for the center button.
*****************************************************************************/
static void usdlg_cmd_center(GObject *object, gpointer data)
{
  enum unit_select_location_mode loc = (enum unit_select_location_mode) data;
  GtkTreeView *view;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;
  gint row;
  struct unit_select_dialog *pdialog = usdlg_get(FALSE);

  fc_assert_ret(pdialog != NULL);
  fc_assert_ret(unit_select_location_mode_is_valid(loc));

  view = GTK_TREE_VIEW(pdialog->tabs[loc].view);
  selection = gtk_tree_view_get_selection(view);

  if (!gtk_tree_selection_get_selected(selection, &model, &it)) {
    log_debug("No selection");
    return;
  }
  gtk_tree_model_get(model, &it, USDLG_COL_ROW_TYPE, &row, -1);

  if (row == ROW_UNIT || row == ROW_UNIT_TRANSPORTED) {
    gint uid;
    struct unit *punit;

    gtk_tree_model_get(model, &it, USDLG_COL_UID, &uid, -1);

    punit = player_unit_by_number(client_player(), uid);
    if (punit) {
      center_tile_mapcanvas(unit_tile(punit));
    }
  }
}

/*************************************************************************//**
  Callback for the focus button.
*****************************************************************************/
static void usdlg_cmd_focus(GObject *object, gpointer data)
{
  enum unit_select_location_mode loc = (enum unit_select_location_mode) data;
  struct unit_select_dialog *pdialog = usdlg_get(FALSE);

  fc_assert_ret(pdialog != NULL);
  fc_assert_ret(unit_select_location_mode_is_valid(loc));

  usdlg_cmd_focus_real(GTK_TREE_VIEW(pdialog->tabs[loc].view));
}

/*************************************************************************//**
  Callback if a row is activated.
*****************************************************************************/
static void usdlg_cmd_row_activated(GtkTreeView *view, GtkTreePath *path,
                                    GtkTreeViewColumn *col, gpointer data)
{
  usdlg_cmd_focus_real(view);
}

/*************************************************************************//**
  Focus to the currently selected unit.
*****************************************************************************/
static void usdlg_cmd_focus_real(GtkTreeView *view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
  GtkTreeModel *model;
  GtkTreeIter it;
  gint row;

  if (!can_client_change_view() || !can_client_control()) {
    return;
  }

  if (!gtk_tree_selection_get_selected(selection, &model, &it)) {
    log_debug("No selection");
    return;
  }
  gtk_tree_model_get(model, &it, USDLG_COL_ROW_TYPE, &row, -1);

  if (row == ROW_UNIT || row == ROW_UNIT_TRANSPORTED) {
    gint uid;
    struct unit *punit;

    gtk_tree_model_get(model, &it, USDLG_COL_UID, &uid, -1);

    punit = player_unit_by_number(client_player(), uid);
    if (punit && unit_owner(punit) == client_player()) {
      unit_focus_set(punit);
      usdlg_destroy();
    }
  }
}

/*************************************************************************//**
  Callback if the row is changed.
*****************************************************************************/
static void usdlg_cmd_cursor_changed(GtkTreeView *view, gpointer data)
{
  enum unit_select_location_mode loc = (enum unit_select_location_mode) data;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;
  gint row, uid;
  struct unit_select_dialog *pdialog = usdlg_get(FALSE);
  struct unit *punit;
  bool cmd_status[USDLG_CMD_LAST];
  int cmd_id;

  fc_assert_ret(unit_select_location_mode_is_valid(loc));

  if (pdialog == NULL) {
    /* Dialog closed, nothing we can do */
    return;
  }

  selection = gtk_tree_view_get_selection(view);
  if (!gtk_tree_selection_get_selected(selection, &model, &it)) {
    log_debug("No selection");
    return;
  }
  gtk_tree_model_get(model, &it, USDLG_COL_ROW_TYPE, &row, USDLG_COL_UID,
                     &uid, -1);

  switch (row) {
  case ROW_UNITTYPE:
  case ROW_ACTIVITY:
    /* Button status for rows unittype and activity:
     *             player    observer
     * ready        TRUE      FALSE
     * sentry       TRUE      FALSE
     * select       TRUE      FALSE
     * deselect     TRUE      FALSE
     * center       FALSE     FALSE
     * focus        FALSE     FALSE */
    if (can_client_change_view() && can_client_control()) {
      cmd_status[USDLG_CMD_READY] = TRUE;
      cmd_status[USDLG_CMD_SENTRY] = TRUE;
      cmd_status[USDLG_CMD_SELECT] = TRUE;
      cmd_status[USDLG_CMD_DESELECT] = TRUE;
    } else {
      cmd_status[USDLG_CMD_READY] = FALSE;
      cmd_status[USDLG_CMD_SENTRY] = FALSE;
      cmd_status[USDLG_CMD_SELECT] = FALSE;
      cmd_status[USDLG_CMD_DESELECT] = FALSE;
    }

    cmd_status[USDLG_CMD_CENTER] = FALSE;
    cmd_status[USDLG_CMD_FOCUS] = FALSE;
    break;
  case ROW_UNIT:
  case ROW_UNIT_TRANSPORTED:
    /* Button status for rows unit and unit (transported):
     *             player    observer
     * ready        !IDLE     FALSE
     * sentry       !SENTRY   FALSE
     * select       !FOCUS    FALSE
     * deselect     FOCUS     FALSE
     * center       TRUE      TRUE
     * focus        !FOCUS    FALSE */
    punit = player_unit_by_number(client_player(), uid);

    if (punit && can_client_change_view() && can_client_control()) {
      if (punit->activity == ACTIVITY_IDLE) {
        cmd_status[USDLG_CMD_READY] = FALSE;
      } else {
        cmd_status[USDLG_CMD_READY] = TRUE;
      }

      if (punit->activity == ACTIVITY_SENTRY) {
        cmd_status[USDLG_CMD_SENTRY] = FALSE;
      } else {
        cmd_status[USDLG_CMD_SENTRY] = TRUE;
      }

      if (!unit_is_in_focus(punit)) {
        cmd_status[USDLG_CMD_SELECT] = TRUE;
        cmd_status[USDLG_CMD_DESELECT] = FALSE;
        cmd_status[USDLG_CMD_FOCUS] = TRUE;
      } else {
        cmd_status[USDLG_CMD_SELECT] = FALSE;
        cmd_status[USDLG_CMD_DESELECT] = TRUE;
        cmd_status[USDLG_CMD_FOCUS] = FALSE;
      }
    } else {
      cmd_status[USDLG_CMD_READY] = FALSE;
      cmd_status[USDLG_CMD_SENTRY] = FALSE;

      cmd_status[USDLG_CMD_SELECT] = FALSE;
      cmd_status[USDLG_CMD_DESELECT] = FALSE;

      cmd_status[USDLG_CMD_FOCUS] = FALSE;
    }

    cmd_status[USDLG_CMD_CENTER] = TRUE;
    break;
  }

  /* Set widget status. */
  for (cmd_id = 0; cmd_id < USDLG_CMD_LAST; cmd_id++) {
    gtk_widget_set_sensitive(GTK_WIDGET(pdialog->tabs[loc].cmd[cmd_id]),
                             cmd_status[cmd_id]);
  }
}
