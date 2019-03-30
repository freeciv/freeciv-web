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

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "government.h"
#include "packets.h"
#include "player.h"
#include "research.h"

/* client */
#include "client_main.h"
#include "options.h"

/* client/gui-gtk-4.0 */
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"

#include "inteldlg.h"

/******************************************************************/
static const char *table_text[] = {
  N_("Ruler:"),
  N_("Government:"),
  N_("Capital:"),
  N_("Gold:"),
  NULL,
  N_("Tax:"),
  N_("Science:"),
  N_("Luxury:"),
  NULL,
  N_("Researching:")
};

enum table_label {
  LABEL_RULER,
  LABEL_GOVERNMENT,
  LABEL_CAPITAL,
  LABEL_GOLD,
  LABEL_SEP1,
  LABEL_TAX,
  LABEL_SCIENCE,
  LABEL_LUXURY,
  LABEL_SEP2,
  LABEL_RESEARCHING,
  LABEL_LAST
};

/******************************************************************/
struct intel_dialog {
  struct player *pplayer;
  GtkWidget *shell;

  GtkTreeStore *diplstates;
  GtkListStore *techs;
  GtkWidget *table_labels[LABEL_LAST];
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct intel_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct intel_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;
static struct intel_dialog *create_intel_dialog(struct player *p);

/**********************************************************************//**
  Initialize intelligenze dialogs
**************************************************************************/
void intel_dialog_init()
{
  dialog_list = dialog_list_new();
}

/**********************************************************************//**
  Free resources allocated for intelligenze dialogs
**************************************************************************/
void intel_dialog_done()
{
  dialog_list_destroy(dialog_list);
}

/**********************************************************************//**
  Get intelligence dialog between client user and other player
  passed as parameter.
**************************************************************************/
static struct intel_dialog *get_intel_dialog(struct player *pplayer)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pplayer == pplayer) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Open intelligence dialog
**************************************************************************/
void popup_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog;

  if (!(pdialog = get_intel_dialog(p))) {
    pdialog = create_intel_dialog(p);
  }

  update_intel_dialog(p);

  gtk_window_present(GTK_WINDOW(pdialog->shell));
}

/**********************************************************************//**
  Intelligence dialog destruction requested
**************************************************************************/
static void intel_destroy_callback(GtkWidget *w, gpointer data)
{
  struct intel_dialog *pdialog = (struct intel_dialog *)data;

  dialog_list_remove(dialog_list, pdialog);

  free(pdialog);
}

/**********************************************************************//**
  Close an intelligence dialog for the given player.
**************************************************************************/
void close_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog = get_intel_dialog(p);

  intel_destroy_callback(NULL, pdialog);
}

/**********************************************************************//**
  Create new intelligence dialog between client user and player
  given as parameter.
**************************************************************************/
static struct intel_dialog *create_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog;

  GtkWidget *shell, *notebook, *label, *sw, *view, *table;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;
  int i;

  pdialog = fc_malloc(sizeof(*pdialog));
  pdialog->pplayer = p;

  shell = gtk_dialog_new_with_buttons(NULL,
                                      NULL,
                                      0,
                                      _("Close"),
                                      GTK_RESPONSE_CLOSE,
                                      NULL);
  pdialog->shell = shell;
  gtk_window_set_default_size(GTK_WINDOW(shell), 350, 350);
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);

  g_signal_connect(shell, "destroy",
                   G_CALLBACK(intel_destroy_callback), pdialog);
  g_signal_connect(shell, "response",
                   G_CALLBACK(gtk_widget_destroy), NULL);

  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(shell))), notebook);
 
  /* overview tab. */
  table = gtk_grid_new();
  g_object_set(table, "margin", 6, NULL);

  gtk_grid_set_row_spacing(GTK_GRID(table), 2);
  gtk_grid_set_column_spacing(GTK_GRID(table), 12);

  /* TRANS: Overview tab of foreign intelligence report dialog */
  label = gtk_label_new_with_mnemonic(_("_Overview"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);

  for (i = 0; i < ARRAY_SIZE(table_text); i++) {
    if (table_text[i]) {
      label = gtk_label_new(_(table_text[i]));
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
      gtk_grid_attach(GTK_GRID(table), label, 0, i, 1, 1);

      label = gtk_label_new(NULL);
      pdialog->table_labels[i] = label;
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
      gtk_grid_attach(GTK_GRID(table), label, 1, i, 1, 1);
    } else {
      pdialog->table_labels[i] = NULL;
    }
  }

  /* diplomacy tab. */
  pdialog->diplstates = gtk_tree_store_new(1, G_TYPE_STRING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pdialog->diplstates));
  g_object_set(view, "margin", 6, NULL);
  gtk_widget_set_hexpand(view, TRUE);
  gtk_widget_set_vexpand(view, TRUE);
  g_object_unref(pdialog->diplstates);
  gtk_widget_set_margin_start(view, 6);
  gtk_widget_set_margin_end(view, 6);
  gtk_widget_set_margin_top(view, 6);
  gtk_widget_set_margin_bottom(view, 6);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
    "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  gtk_tree_view_expand_all(GTK_TREE_VIEW(view));

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  label = gtk_label_new_with_mnemonic(_("_Diplomacy"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);

  /* techs tab. */
  pdialog->techs = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(pdialog->techs),
      1, GTK_SORT_ASCENDING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pdialog->techs));
  g_object_set(view, "margin", 6, NULL);
  gtk_widget_set_hexpand(view, TRUE);
  gtk_widget_set_vexpand(view, TRUE);
  g_object_unref(pdialog->techs);
  gtk_widget_set_margin_start(view, 6);
  gtk_widget_set_margin_end(view, 6);
  gtk_widget_set_margin_top(view, 6);
  gtk_widget_set_margin_bottom(view, 6);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_toggle_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
    "active", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
    "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  label = gtk_label_new_with_mnemonic(_("_Techs"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);

  gtk_widget_show(gtk_dialog_get_content_area(GTK_DIALOG(shell)));

  dialog_list_prepend(dialog_list, pdialog);

  return pdialog;
}

/**********************************************************************//**
  Update the intelligence dialog for the given player.  This is called by
  the core client code when that player's information changes.
**************************************************************************/
void update_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog = get_intel_dialog(p);

  if (pdialog) {
    const struct research *mresearch, *presearch;
    GtkTreeIter diplstates[DS_LAST];
    int i;

    /* window title. */
    gchar *title = g_strdup_printf(_("Foreign Intelligence: %s Empire"),
			  nation_adjective_for_player(p));
    gtk_window_set_title(GTK_WINDOW(pdialog->shell), title);
    g_free(title);

    /* diplomacy tab. */
    gtk_tree_store_clear(pdialog->diplstates);

    for (i = 0; i < ARRAY_SIZE(diplstates); i++) {
      GtkTreeIter it;
      GValue v = { 0, };

      gtk_tree_store_append(pdialog->diplstates, &it, NULL);
      g_value_init(&v, G_TYPE_STRING);
      g_value_set_static_string(&v, diplstate_type_translated_name(i));
      gtk_tree_store_set_value(pdialog->diplstates, &it, 0, &v);
      g_value_unset(&v);
      diplstates[i] = it;
    }

    players_iterate(other) {
      const struct player_diplstate *state;
      GtkTreeIter it;
      GValue v = { 0, };

      if (other == p || !other->is_alive) {
	continue;
      }
      state = player_diplstate_get(p, other);
      gtk_tree_store_append(pdialog->diplstates, &it,
			    &diplstates[state->type]);
      g_value_init(&v, G_TYPE_STRING);
      g_value_set_static_string(&v, player_name(other));
      gtk_tree_store_set_value(pdialog->diplstates, &it, 0, &v);
      g_value_unset(&v);
    } players_iterate_end;

    /* techs tab. */
    gtk_list_store_clear(pdialog->techs);

    mresearch = research_get(client_player());
    presearch = research_get(p);
    advance_index_iterate(A_FIRST, advi) {
      if (research_invention_state(presearch, advi) == TECH_KNOWN) {
        GtkTreeIter it;

        gtk_list_store_append(pdialog->techs, &it);

        gtk_list_store_set(pdialog->techs, &it,
                           0, research_invention_state(mresearch, advi)
                           != TECH_KNOWN,
                           1, research_advance_name_translation(presearch,
                                                                advi),
                           -1);
      }
    } advance_index_iterate_end;

    /* table labels. */
    for (i = 0; i < ARRAY_SIZE(pdialog->table_labels); i++) {
      if (pdialog->table_labels[i]) {
        struct city *pcity;
        gchar *buf = NULL;
        char tbuf[256];

        switch (i) {
        case LABEL_RULER:
          ruler_title_for_player(p, tbuf, sizeof(tbuf));
          buf = g_strdup(tbuf);
          break;
        case LABEL_GOVERNMENT:
          buf = g_strdup(government_name_for_player(p));
          break;
        case LABEL_CAPITAL:
          pcity = player_capital(p);
          /* TRANS: "unknown" location */
          buf = g_strdup((!pcity) ? _("(unknown)") : city_name_get(pcity));
          break;
        case LABEL_GOLD:
          buf = g_strdup_printf("%d", p->economic.gold);
          break;
        case LABEL_TAX:
          buf = g_strdup_printf("%d%%", p->economic.tax);
          break;
        case LABEL_SCIENCE:
          buf = g_strdup_printf("%d%%", p->economic.science);
          break;
        case LABEL_LUXURY:
          buf = g_strdup_printf("%d%%", p->economic.luxury);
          break;
        case LABEL_RESEARCHING:
          {
            struct research *research = research_get(p);

            switch (research->researching) {
            case A_UNKNOWN:
              /* TRANS: "Unknown" advance/technology */
              buf = g_strdup(_("(Unknown)"));
              break;
            case A_UNSET:
              /* TRANS: missing value */
              buf = g_strdup(_("(none)"));
              break;
            default:
              buf = g_strdup_printf("%s(%d/%d)",
                                    research_advance_name_translation
                                        (research, research->researching),
                                    research->bulbs_researched,
                                    research->client.researching_cost);
              break;
            }
            break;
          }
        default:
          break;
        }

        if (buf) {
          gtk_label_set_text(GTK_LABEL(pdialog->table_labels[i]), buf);
	  g_free(buf);
        }
      }
    }
  }
}
