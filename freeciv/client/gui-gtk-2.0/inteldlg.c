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

/* client */
#include "client_main.h"
#include "options.h"

/* gui-gtk-2.0 */
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

/****************************************************************
...
*****************************************************************/
void intel_dialog_init()
{
  dialog_list = dialog_list_new();
}

/****************************************************************
...
*****************************************************************/
void intel_dialog_done()
{
  dialog_list_free(dialog_list);
}

/****************************************************************
...
*****************************************************************/
static struct intel_dialog *get_intel_dialog(struct player *pplayer)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pplayer == pplayer) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/****************************************************************
... 
*****************************************************************/
void popup_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog;

  if (!(pdialog = get_intel_dialog(p))) {
    pdialog = create_intel_dialog(p);
  }

  update_intel_dialog(p);

  gtk_window_present(GTK_WINDOW(pdialog->shell));
}

/****************************************************************
...
*****************************************************************/
static void intel_destroy_callback(GtkWidget *w, gpointer data)
{
  struct intel_dialog *pdialog = (struct intel_dialog *)data;

  dialog_list_unlink(dialog_list, pdialog);

  free(pdialog);
}

/****************************************************************
...
*****************************************************************/
static struct intel_dialog *create_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog;

  GtkWidget *shell, *notebook, *label, *sw, *view, *table, *alignment;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  int i;
 
  pdialog = fc_malloc(sizeof(*pdialog));
  pdialog->pplayer = p;
 
  shell = gtk_dialog_new_with_buttons(NULL,
      NULL,
      0,
      GTK_STOCK_CLOSE,
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
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), notebook);
 
  /* overview tab. */
  table = gtk_table_new(ARRAY_SIZE(table_text), 2, FALSE);

  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);

  alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
  gtk_container_set_border_width(GTK_CONTAINER(alignment), 6);
  gtk_container_add(GTK_CONTAINER(alignment), table);

  label = gtk_label_new_with_mnemonic(_("_Overview"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), alignment, label);

  for (i = 0; i < ARRAY_SIZE(table_text); i++) {
    if (table_text[i]) {
      label = gtk_label_new(_(table_text[i]));
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      gtk_table_attach(GTK_TABLE(table), label,
	  0, 1, i, i+1, GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);

      label = gtk_label_new(NULL);
      pdialog->table_labels[i] = label;
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      gtk_table_attach(GTK_TABLE(table), label,
	  1, 2, i, i+1, GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
    } else {
      pdialog->table_labels[i] = NULL;
      gtk_table_set_row_spacing(GTK_TABLE(table), i, 12);
    }
  }

  /* diplomacy tab. */
  pdialog->diplstates = gtk_tree_store_new(1, G_TYPE_STRING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pdialog->diplstates));
  g_object_unref(pdialog->diplstates);
  gtk_container_set_border_width(GTK_CONTAINER(view), 6);
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

  alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  gtk_container_set_border_width(GTK_CONTAINER(alignment), 6);
  gtk_container_add(GTK_CONTAINER(alignment), sw);

  label = gtk_label_new_with_mnemonic(_("_Diplomacy"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), alignment, label);

  /* techs tab. */
  pdialog->techs = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(pdialog->techs),
      1, GTK_SORT_ASCENDING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pdialog->techs));
  g_object_unref(pdialog->techs);
  gtk_container_set_border_width(GTK_CONTAINER(view), 6);
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

  alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  gtk_container_set_border_width(GTK_CONTAINER(alignment), 6);
  gtk_container_add(GTK_CONTAINER(alignment), sw);

  label = gtk_label_new_with_mnemonic(_("_Techs"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), alignment, label);

  gtk_widget_show_all(GTK_DIALOG(shell)->vbox);

  dialog_list_prepend(dialog_list, pdialog);

  return pdialog;
}

/****************************************************************************
  Update the intelligence dialog for the given player.  This is called by
  the core client code when that player's information changes.
****************************************************************************/
void update_intel_dialog(struct player *p)
{
  struct intel_dialog *pdialog = get_intel_dialog(p);

  if (pdialog) {
    GtkTreeIter diplstates[DS_LAST];
    char buf[64];
    int i;

    /* window title. */
    my_snprintf(buf, sizeof(buf),
	_("Foreign Intelligence: %s Empire"),
	nation_adjective_for_player(p));
    gtk_window_set_title(GTK_WINDOW(pdialog->shell), buf);

    /* diplomacy tab. */
    gtk_tree_store_clear(pdialog->diplstates);

    for (i = 0; i < ARRAY_SIZE(diplstates); i++) {
      GtkTreeIter it;
      GValue v = { 0, };

      gtk_tree_store_append(pdialog->diplstates, &it, NULL);
      g_value_init(&v, G_TYPE_STRING);
      g_value_set_static_string(&v, diplstate_text(i));
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
      state = pplayer_get_diplstate(p, other);
      gtk_tree_store_append(pdialog->diplstates, &it,
			    &diplstates[state->type]);
      g_value_init(&v, G_TYPE_STRING);
      g_value_set_static_string(&v, player_name(other));
      gtk_tree_store_set_value(pdialog->diplstates, &it, 0, &v);
      g_value_unset(&v);
    } players_iterate_end;

    /* techs tab. */
    gtk_list_store_clear(pdialog->techs);

    advance_index_iterate(A_FIRST, i) {
      if(player_invention_state(p, i)==TECH_KNOWN) {
	GtkTreeIter it;

	gtk_list_store_append(pdialog->techs, &it);

	gtk_list_store_set(pdialog->techs, &it,
			   0, (TECH_KNOWN != player_invention_state(client.conn.playing, i)),
			   1, advance_name_for_player(p, i),
			   -1);
      }
    } advance_index_iterate_end;

    /* table labels. */
    for (i = 0; i < ARRAY_SIZE(pdialog->table_labels); i++) {
      if (pdialog->table_labels[i]) {
	struct city *pcity;

	switch (i) {
	  case LABEL_RULER:
	    my_snprintf(buf, sizeof(buf), "%s %s", 
		ruler_title_translation(p),
		player_name(p));
	    break;
	  case LABEL_GOVERNMENT:
	    sz_strlcpy(buf, government_name_for_player(p));
	    break;
	  case LABEL_CAPITAL:
	    pcity = find_palace(p);
	    /* TRANS: "unknown" location */
	    sz_strlcpy(buf, (!pcity) ? _("(unknown)") : city_name(pcity));
	    break;
	  case LABEL_GOLD:
	    my_snprintf(buf, sizeof(buf), "%d", p->economic.gold);
	    break;
	  case LABEL_TAX:
	    my_snprintf(buf, sizeof(buf), "%d%%", p->economic.tax);
	    break;
	  case LABEL_SCIENCE:
	    my_snprintf(buf, sizeof(buf), "%d%%", p->economic.science);
	    break;
	  case LABEL_LUXURY:
	    my_snprintf(buf, sizeof(buf), "%d%%", p->economic.luxury);
	    break;
	  case LABEL_RESEARCHING: {
	    struct player_research* research = get_player_research(p);
	    switch (research->researching) {
	    case A_UNKNOWN:
	      /* TRANS: "Unknown" advance/technology */
	      my_snprintf(buf, sizeof(buf), _("(Unknown)"));
	      break;
	    case A_UNSET:
	      /* TRANS: missing value */
	      my_snprintf(buf, sizeof(buf), _("(none)"));
	      break;
	    default:
	      my_snprintf(buf, sizeof(buf), "%s(%d/%d)",
		  advance_name_researching(p),
		  research->bulbs_researched, total_bulbs_required(p));
	      break;
	    };
	    break;
	  }
	  default:
	    buf[0] = '\0';
	    break;
	}

	if (buf[0] != '\0') {
	  gtk_label_set_text(GTK_LABEL(pdialog->table_labels[i]), buf);
	}
      }
    }
  }
}

