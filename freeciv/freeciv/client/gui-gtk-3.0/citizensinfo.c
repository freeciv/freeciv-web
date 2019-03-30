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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* common */
#include "citizens.h"
#include "city.h"
#include "player.h"

/* client/gui-gtk-3.0 */
#include "gui_stuff.h"
#include "plrdlg.h"

#include "citizensinfo.h"


static struct citizens_dialog *citizens_dialog_get(const struct city *pcity);
static struct citizens_dialog
  *citizens_dialog_create(const struct city *pcity);
static GtkTreeStore *citizens_dialog_store_new(void);
static int citizens_dialog_default_sort_column(void);
static void citizens_dialog_row(GtkTreeStore *store, GtkTreeIter *it,
                                const struct city *pcity,
                                const struct player_slot *pslot);

static const char *col_nation(const struct city *pcity,
                              const struct player_slot *pslot);
static const char *col_citizens(const struct city *pcity,
                                const struct player_slot *pslot);

/*****************************************************************************
  The layout of the citizens display.
*****************************************************************************/
static struct citizens_column {
  bool show;
  enum player_dlg_column_type type;
  const char *title;
  const char *(*func)(const struct city *, const struct player_slot *);
                                       /* if type = COL_*TEXT */
  const char *tagname;                 /* for save_options */
} citizens_cols[] = {
  {TRUE, COL_RIGHT_TEXT, N_("#"), col_citizens, "citizens"},
  {TRUE, COL_FLAG, N_("Flag"), NULL, "flag"},
  {TRUE, COL_TEXT, N_("Nation"), col_nation, "nation"}
};
static const int num_citizens_cols = ARRAY_SIZE(citizens_cols);
#define CITIZENS_DLG_COL_STYLE       (0 + num_citizens_cols)
#define CITIZENS_DLG_COL_WEIGHT      (1 + num_citizens_cols)
#define CITIZENS_DLG_COL_ID          (2 + num_citizens_cols)

/*****************************************************************************
  The citizens dialog.
*****************************************************************************/
struct citizens_dialog {
  const struct city *pcity;
  GtkWidget *shell;
  GtkTreeStore *store;
  GtkWidget *list;
  GtkTreeModel *sort;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct citizens_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct citizens_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;

/*************************************************************************//**
  The name of the player's nation for the plrdlg.
*****************************************************************************/
static const char *col_nation(const struct city *pcity,
                              const struct player_slot *pslot)
{
  return nation_adjective_for_player(player_slot_get_player(pslot));
}

/*************************************************************************//**
  The number of citizens for the player in the city.
*****************************************************************************/
static const char *col_citizens(const struct city *pcity,
                                const struct player_slot *pslot)
{
  citizens nationality = citizens_nation_get(pcity, pslot);

  if (nationality == 0) {
    return "-";
  } else {
    static char buf[8];

    fc_snprintf(buf, sizeof(buf), "%d", nationality);

    return buf;
  }
}

/*************************************************************************//**
  Create a citizens dialog store.

  FIXME: copy of players_dialog_store_new();
*****************************************************************************/
static GtkTreeStore *citizens_dialog_store_new(void)
{
  GtkTreeStore *store;
  GType model_types[num_citizens_cols + 3];
  int i;

  for (i = 0; i < num_citizens_cols; i++) {
    switch (citizens_cols[i].type) {
    case COL_FLAG:
      model_types[i] = GDK_TYPE_PIXBUF;
      break;
    case COL_COLOR:
      model_types[i] = GDK_TYPE_PIXBUF;
      break;
    case COL_BOOLEAN:
      model_types[i] = G_TYPE_BOOLEAN;
      break;
    case COL_TEXT:
    case COL_RIGHT_TEXT:
      model_types[i] = G_TYPE_STRING;
      break;
    }
  }
  /* special (invisible rows) - Text style, weight and player id */
  model_types[i++] = G_TYPE_INT;        /* CITIZENS_DLG_COL_STYLE. */
  model_types[i++] = G_TYPE_INT;        /* CITIZENS_DLG_COL_WEIGHT. */
  model_types[i++] = G_TYPE_INT;        /* CITIZENS_DLG_COL_ID. */

  store = gtk_tree_store_newv(i, model_types);

  return store;
}

/*************************************************************************//**
  Returns column to sort by by default
*****************************************************************************/
static int citizens_dialog_default_sort_column(void) {
  return 0;
}

/*************************************************************************//**
  Create citizens dialog
*****************************************************************************/
static struct citizens_dialog
  *citizens_dialog_create(const struct city *pcity)
{
  GtkWidget *frame, *sw;
  struct citizens_dialog *pdialog = fc_malloc(sizeof(struct citizens_dialog));
  int i;

  pdialog->pcity = pcity;
  pdialog->store = citizens_dialog_store_new();
  pdialog->sort
    = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(pdialog->store));
  g_object_unref(pdialog->store);

  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(pdialog->sort),
                                       citizens_dialog_default_sort_column(),
                                       GTK_SORT_DESCENDING);

  pdialog->list
    = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pdialog->sort));
  gtk_widget_set_halign(pdialog->list, GTK_ALIGN_CENTER);
  g_object_unref(pdialog->sort);

  for (i = 0; i < num_citizens_cols; i++) {
    struct citizens_column *pcol;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    pcol = &citizens_cols[i];
    col = NULL;

    switch (pcol->type) {
    case COL_FLAG:
      renderer = gtk_cell_renderer_pixbuf_new();
      col = gtk_tree_view_column_new_with_attributes(_(pcol->title), renderer,
              "pixbuf", i, NULL);
      break;
    case COL_TEXT:
      renderer = gtk_cell_renderer_text_new();
      g_object_set(renderer, "style-set", TRUE, "weight-set", TRUE, NULL);

      col = gtk_tree_view_column_new_with_attributes(_(pcol->title), renderer,
              "text", i,
              "style", CITIZENS_DLG_COL_STYLE,
              "weight", CITIZENS_DLG_COL_WEIGHT,
              NULL);
      gtk_tree_view_column_set_sort_column_id(col, i);
      break;
    case COL_RIGHT_TEXT:
      renderer = gtk_cell_renderer_text_new();
      g_object_set(renderer, "style-set", TRUE, "weight-set", TRUE, NULL);

      col = gtk_tree_view_column_new_with_attributes(_(pcol->title), renderer,
              "text", i,
              "style", CITIZENS_DLG_COL_STYLE,
              "weight", CITIZENS_DLG_COL_WEIGHT,
              NULL);
      gtk_tree_view_column_set_sort_column_id(col, i);
      g_object_set(renderer, "xalign", 1.0, NULL);
      gtk_tree_view_column_set_alignment(col, 1.0);
      break;
    case COL_COLOR:
    case COL_BOOLEAN:
      /* These are not used. */
      fc_assert(pcol->type != COL_COLOR && pcol->type != COL_BOOLEAN);
      continue;
    }

    if (col) {
      gtk_tree_view_append_column(GTK_TREE_VIEW(pdialog->list), col);
    }
  }

  gtk_widget_set_hexpand(GTK_WIDGET(pdialog->list), TRUE);
  gtk_widget_set_vexpand(GTK_WIDGET(pdialog->list), TRUE);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(sw), pdialog->list);

  frame = gtk_frame_new(_("Citizens"));
  gtk_container_add(GTK_CONTAINER(frame), sw);

  pdialog->shell = frame;

  dialog_list_prepend(dialog_list, pdialog);

  citizens_dialog_refresh(pcity);

  return pdialog;
}

/*************************************************************************//**
  Initialize citizens dialog
*****************************************************************************/
void citizens_dialog_init()
{
  dialog_list = dialog_list_new();
}

/*************************************************************************//**
  Free resources allocated for citizens dialog
*****************************************************************************/
void citizens_dialog_done()
{
  dialog_list_destroy(dialog_list);
}

/*************************************************************************//**
  Get citizen dialog of the given city
*****************************************************************************/
static struct citizens_dialog *citizens_dialog_get(const struct city *pcity)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pcity == pcity) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/*************************************************************************//**
  Refresh citizen dialog of the given city
*****************************************************************************/
void citizens_dialog_refresh(const struct city *pcity)
{
  struct citizens_dialog *pdialog = citizens_dialog_get(pcity);

  if (pdialog == NULL) {
    return;
  }

  gtk_tree_store_clear(pdialog->store);
  citizens_iterate(pcity, pslot, nationality) {
    GtkTreeIter iter;

    gtk_tree_store_append(pdialog->store, &iter, NULL);
    citizens_dialog_row(pdialog->store, &iter, pcity, pslot);
  } citizens_iterate_end;
}

/*************************************************************************//**
  Fills the citizens list with the data for 'pslot' at the row given by 'it'.
*****************************************************************************/
static void citizens_dialog_row(GtkTreeStore *store, GtkTreeIter *it,
                                const struct city *pcity,
                                const struct player_slot *pslot)
{
  GdkPixbuf *pixbuf;
  int style = PANGO_STYLE_NORMAL, weight = PANGO_WEIGHT_NORMAL;
  int k;

  for (k = 0; k < num_citizens_cols; k++) {
    struct citizens_column *pcol = &citizens_cols[k];
    switch (pcol->type) {
    case COL_TEXT:
    case COL_RIGHT_TEXT:
      gtk_tree_store_set(store, it, k, pcol->func(pcity, pslot), -1);
      break;
    case COL_FLAG:
      pixbuf = get_flag(nation_of_player(player_slot_get_player(pslot)));
      if (pixbuf != NULL) {
        gtk_tree_store_set(store, it, k, pixbuf, -1);
        g_object_unref(pixbuf);
      }
      break;
    case COL_COLOR:
    case COL_BOOLEAN:
      /* These are not used. */
      fc_assert(pcol->type != COL_COLOR && pcol->type != COL_BOOLEAN);
      continue;
      break;
    }
  }

  if (city_owner(pcity)->slot == pslot) {
    weight = PANGO_WEIGHT_BOLD;
    style = PANGO_STYLE_NORMAL;
  }

  gtk_tree_store_set(store, it,
                     CITIZENS_DLG_COL_STYLE, style,
                     CITIZENS_DLG_COL_WEIGHT, weight,
                     CITIZENS_DLG_COL_ID, player_slot_index(pslot),
                     -1);
}

/*************************************************************************//**
  Close citizens dialog of one city
*****************************************************************************/
void citizens_dialog_close(const struct city *pcity)
{
  struct citizens_dialog *pdialog = citizens_dialog_get(pcity);
  if (pdialog == NULL) {
    return;
  }

  gtk_widget_hide(pdialog->shell);

  dialog_list_remove(dialog_list, pdialog);

  gtk_widget_destroy(pdialog->shell);
  free(pdialog);
}

/*************************************************************************//**
  Make citizen dialog of the city visible.
*****************************************************************************/
GtkWidget *citizens_dialog_display(const struct city *pcity)
{
  struct citizens_dialog *pdialog = citizens_dialog_get(pcity);

  if (!pdialog) {
    pdialog = citizens_dialog_create(pcity);
  }

  gtk_widget_show_all(pdialog->shell);
  citizens_dialog_refresh(pcity);

  return pdialog->shell;
}
