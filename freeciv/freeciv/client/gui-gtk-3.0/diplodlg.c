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
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "diptreaty.h"
#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "research.h"

/* client */
#include "chatline.h"
#include "client_main.h"
#include "climisc.h"
#include "options.h"

/* client/gui-gtk-3.0 */
#include "diplodlg.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "plrdlg.h"

#define MAX_NUM_CLAUSES 64

struct Diplomacy_dialog {
  struct Treaty treaty;
  struct gui_dialog* dialog;
  
  GtkWidget *menu0;
  GtkWidget *menu1;

  GtkWidget *image0;
  GtkWidget *image1;

  GtkListStore *store;
};

struct Diplomacy_notebook {
  struct gui_dialog* dialog;
  GtkWidget *notebook;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct Diplomacy_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct Diplomacy_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;
static struct Diplomacy_notebook *dipl_main;

static struct Diplomacy_dialog *create_diplomacy_dialog(struct player *plr0, 
						 struct player *plr1);

static struct Diplomacy_dialog *find_diplomacy_dialog(int other_player_id);
static void popup_diplomacy_dialog(int other_player_id, int initiated_from);
static void diplomacy_dialog_map_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_seamap_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_tech_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_city_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_ceasefire_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_peace_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_alliance_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_vision_callback(GtkWidget *w, gpointer data);
static void diplomacy_dialog_embassy_callback(GtkWidget *w, gpointer data);
static void close_diplomacy_dialog(struct Diplomacy_dialog *pdialog);
static void update_diplomacy_dialog(struct Diplomacy_dialog *pdialog);
static void diplo_dialog_returnkey(GtkWidget *w, gpointer data);

static struct Diplomacy_notebook *diplomacy_main_create(void);
static void diplomacy_main_destroy(void);
static void diplomacy_main_response(struct gui_dialog *dlg, int response,
                                    gpointer data);

#define RESPONSE_CANCEL_MEETING 100
#define RESPONSE_CANCEL_MEETING_ALL 101

/************************************************************************//**
  Server tells us that either party has accepted treaty
****************************************************************************/
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted,
                                    bool other_accepted)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  pdialog->treaty.accept0 = I_accepted;
  pdialog->treaty.accept1 = other_accepted;

  update_diplomacy_dialog(pdialog);
  gui_dialog_alert(pdialog->dialog);
}

/************************************************************************//**
  Someone is initiating meeting with us.
****************************************************************************/
void handle_diplomacy_init_meeting(int counterpart, int initiated_from)
{
  popup_diplomacy_dialog(counterpart, initiated_from);
}

/************************************************************************//**
  Meeting has been cancelled.
****************************************************************************/
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  close_diplomacy_dialog(pdialog);
}

/************************************************************************//**
  Added clause to the meeting
****************************************************************************/
void handle_diplomacy_create_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  add_clause(&pdialog->treaty, player_by_number(giver), type, value);
  update_diplomacy_dialog(pdialog);
  gui_dialog_alert(pdialog->dialog);  
}

/************************************************************************//**
  Removed clause from meeting.
****************************************************************************/
void handle_diplomacy_remove_clause(int counterpart, int giver,
                                    enum clause_type type, int value)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  remove_clause(&pdialog->treaty, player_by_number(giver), type, value);
  update_diplomacy_dialog(pdialog);
  gui_dialog_alert(pdialog->dialog);
}

/************************************************************************//**
  Popup the dialog 10% inside the main-window
****************************************************************************/
static void popup_diplomacy_dialog(int other_player_id, int initiated_from)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(other_player_id);

  if (!can_client_issue_orders()) {
    return;
  }

  if (!is_human(client.conn.playing)) {
    return; /* Don't show if we are not human controlled. */
  }

  if (!pdialog) {
    pdialog = create_diplomacy_dialog(client.conn.playing,
                                      player_by_number(other_player_id));
  }

  gui_dialog_present(pdialog->dialog);
  /* We initated the meeting - Make the tab active */
  if (player_by_number(initiated_from) == client.conn.playing) {
    /* we have to raise the diplomacy meeting tab as well as the selected
     * meeting. */
    fc_assert_ret(dipl_main != NULL);
    gui_dialog_raise(dipl_main->dialog);
    gui_dialog_raise(pdialog->dialog);

    if (players_dialog_shell != NULL) {
      gui_dialog_set_return_dialog(pdialog->dialog, players_dialog_shell);
    }
  }
}

/************************************************************************//**
  Utility for g_list_sort(). See below.
****************************************************************************/
static gint sort_advance_names(gconstpointer a, gconstpointer b)
{
  const struct advance *padvance1 = (const struct advance *) a;
  const struct advance *padvance2 = (const struct advance *) b;

  return fc_strcoll(advance_name_translation(padvance1),
                    advance_name_translation(padvance2));
}

/************************************************************************//**
  Popup menu about adding clauses
****************************************************************************/
static void popup_add_menu(GtkMenuShell *parent, gpointer data)
{
  struct Diplomacy_dialog *pdialog;
  struct player *pgiver, *pother;
  GtkWidget *item, *menu;


  /* init. */
  gtk_container_foreach(GTK_CONTAINER(parent),
                        (GtkCallback) gtk_widget_destroy, NULL);

  pdialog = (struct Diplomacy_dialog *) data;
  pgiver = (struct player *) g_object_get_data(G_OBJECT(parent), "plr");
  pother = (pgiver == pdialog->treaty.plr0
            ? pdialog->treaty.plr1 : pdialog->treaty.plr0);


  /* Maps. */
  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_mnemonic(_("World-map"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
  g_object_set_data(G_OBJECT(item), "plr", pgiver);
  g_signal_connect(item, "activate",
		   G_CALLBACK(diplomacy_dialog_map_callback), pdialog);

  item = gtk_menu_item_new_with_mnemonic(_("Sea-map"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_object_set_data(G_OBJECT(item), "plr", pgiver);
  g_signal_connect(item, "activate",
		   G_CALLBACK(diplomacy_dialog_seamap_callback), pdialog);

  item = gtk_menu_item_new_with_mnemonic(_("_Maps"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
  gtk_widget_show_all(item);

  /* Trading: advances */
  if (game.info.trading_tech) {
    const struct research *gresearch = research_get(pgiver);
    const struct research *oresearch = research_get(pother);
    GtkWidget *advance_item;
    GList *sorting_list = NULL;

    advance_item = gtk_menu_item_new_with_mnemonic(_("_Advances"));
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), advance_item);

    advance_iterate(A_FIRST, padvance) {
      Tech_type_id i = advance_number(padvance);

      if (research_invention_state(gresearch, i) == TECH_KNOWN
          && research_invention_gettable(oresearch, i,
                                         game.info.tech_trade_allow_holes)
          && (research_invention_state(oresearch, i) == TECH_UNKNOWN
              || research_invention_state(oresearch, i)
                 == TECH_PREREQS_KNOWN)) {
        sorting_list = g_list_prepend(sorting_list, padvance);
      }
    } advance_iterate_end;

    if (NULL == sorting_list) {
      /* No advance. */
      gtk_widget_set_sensitive(advance_item, FALSE);
    } else {
      GList *list_item;
      const struct advance *padvance;

      sorting_list = g_list_sort(sorting_list, sort_advance_names);
      menu = gtk_menu_new();

      /* TRANS: All technologies menu item in the diplomatic dialog. */
      item = gtk_menu_item_new_with_label(_("All advances"));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_object_set_data(G_OBJECT(item), "player_from",
                        GINT_TO_POINTER(player_number(pgiver)));
      g_object_set_data(G_OBJECT(item), "player_to",
                        GINT_TO_POINTER(player_number(pother)));
      g_signal_connect(item, "activate",
                       G_CALLBACK(diplomacy_dialog_tech_callback),
                       GINT_TO_POINTER(A_LAST));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                            gtk_separator_menu_item_new());

      for (list_item = sorting_list; NULL != list_item;
           list_item = g_list_next(list_item)) {
        padvance = (const struct advance *) list_item->data;
        item =
            gtk_menu_item_new_with_label(advance_name_translation(padvance));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_object_set_data(G_OBJECT(item), "player_from",
                          GINT_TO_POINTER(player_number(pgiver)));
        g_object_set_data(G_OBJECT(item), "player_to",
                          GINT_TO_POINTER(player_number(pother)));
        g_signal_connect(item, "activate",
                         G_CALLBACK(diplomacy_dialog_tech_callback),
                         GINT_TO_POINTER(advance_number(padvance)));
      }

      gtk_menu_item_set_submenu(GTK_MENU_ITEM(advance_item), menu);
      g_list_free(sorting_list);
    }

    gtk_widget_show_all(advance_item);
  }


  /* Trading: cities. */

  /****************************************************************
  Creates a sorted list of plr0's cities, excluding the capital and
  any cities not visible to plr1.  This means that you can only trade 
  cities visible to requesting player.  

			      - Kris Bubendorfer
  *****************************************************************/
  if (game.info.trading_city) {
    int i = 0, j = 0, n = city_list_size(pgiver->cities);
    struct city **city_list_ptrs;

    if (n > 0) {
      city_list_ptrs = fc_malloc(sizeof(struct city *) * n);
    } else {
      city_list_ptrs = NULL;
    }

    city_list_iterate(pgiver->cities, pcity) {
      if (!is_capital(pcity)) {
	city_list_ptrs[i] = pcity;
	i++;
      }
    } city_list_iterate_end;

    qsort(city_list_ptrs, i, sizeof(struct city *), city_name_compare);
    
    menu = gtk_menu_new();

    for (j = 0; j < i; j++) {
      item = gtk_menu_item_new_with_label(city_name_get(city_list_ptrs[j]));

      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_signal_connect(item, "activate",
		       G_CALLBACK(diplomacy_dialog_city_callback),
			 GINT_TO_POINTER((player_number(pgiver) << 24) |
					 (player_number(pother) << 16) |
					 city_list_ptrs[j]->id));
    }
    free(city_list_ptrs);

    item = gtk_menu_item_new_with_mnemonic(_("_Cities"));
    gtk_widget_set_sensitive(item, (i > 0));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
    gtk_widget_show_all(item);
  }


  /* Give shared vision. */
  item = gtk_menu_item_new_with_mnemonic(_("_Give shared vision"));
  g_object_set_data(G_OBJECT(item), "plr", pgiver);
  g_signal_connect(item, "activate",
		   G_CALLBACK(diplomacy_dialog_vision_callback), pdialog);

  if (gives_shared_vision(pgiver, pother)) {
    gtk_widget_set_sensitive(item, FALSE);
  }
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
  gtk_widget_show(item);


  /* Give embassy. */
  item = gtk_menu_item_new_with_mnemonic(_("Give _embassy"));
  g_object_set_data(G_OBJECT(item), "plr", pgiver);
  g_signal_connect(item, "activate",
		   G_CALLBACK(diplomacy_dialog_embassy_callback), pdialog);

  /* Don't take in account the embassy effects. */
  if (player_has_real_embassy(pother, pgiver)) {
    gtk_widget_set_sensitive(item, FALSE);
  }
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
  gtk_widget_show(item);


  /* Pacts. */
  if (pgiver == pdialog->treaty.plr0) {
    enum diplstate_type ds;

    ds = player_diplstate_get(pgiver, pother)->type;

    menu = gtk_menu_new();
    item = gtk_menu_item_new_with_mnemonic(Q_("?diplomatic_state:Cease-fire"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
    g_signal_connect(item, "activate",
		     G_CALLBACK(diplomacy_dialog_ceasefire_callback), pdialog);
    gtk_widget_set_sensitive(item, ds != DS_CEASEFIRE && ds != DS_TEAM);

    item = gtk_menu_item_new_with_mnemonic(Q_("?diplomatic_state:Peace"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate",
		     G_CALLBACK(diplomacy_dialog_peace_callback), pdialog);
    gtk_widget_set_sensitive(item, ds != DS_PEACE && ds != DS_TEAM);

    item = gtk_menu_item_new_with_mnemonic(Q_("?diplomatic_state:Alliance"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate",
		     G_CALLBACK(diplomacy_dialog_alliance_callback), pdialog);
    gtk_widget_set_sensitive(item, ds != DS_ALLIANCE && ds != DS_TEAM);

    item = gtk_menu_item_new_with_mnemonic(_("_Pacts"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
    gtk_widget_show_all(item);
  }
}

/************************************************************************//**
  Some clause activated
****************************************************************************/
static void row_callback(GtkTreeView *view, GtkTreePath *path,
                         GtkTreeViewColumn *col, gpointer data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *)data;
  gint i;
  gint *index;

  index = gtk_tree_path_get_indices(path);

  i = 0; 
  clause_list_iterate(pdialog->treaty.clauses, pclause) {
    if (i == index[0]) {
      dsend_packet_diplomacy_remove_clause_req(&client.conn,
                                               player_number(pdialog->treaty.plr1),
                                               player_number(pclause->from),
                                               pclause->type,
                                               pclause->value);
      return;
    }
    i++;
  } clause_list_iterate_end;
}

/************************************************************************//**
  Create the main tab for diplomatic meetings.
****************************************************************************/
static struct Diplomacy_notebook *diplomacy_main_create(void)
{
  /* Collect all meetings in one main tab. */
  if (!dipl_main) {
    GtkWidget *dipl_box, *dipl_sw;

    dipl_main = fc_malloc(sizeof(*dipl_main));
    gui_dialog_new(&(dipl_main->dialog), GTK_NOTEBOOK(top_notebook),
                  dipl_main->dialog, TRUE);
    dipl_main->notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(dipl_main->notebook),
                             GTK_POS_RIGHT);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(dipl_main->notebook), TRUE);

    dipl_sw = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(dipl_sw, "margin", 2, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dipl_sw),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(dipl_sw), dipl_main->notebook);

    /* Buttons */
    gui_dialog_add_stockbutton(dipl_main->dialog, GTK_STOCK_CANCEL,
                               _("Cancel _all meetings"),
                               RESPONSE_CANCEL_MEETING_ALL);

    /* Responces for _all_ meetings. */
    gui_dialog_response_set_callback(dipl_main->dialog,
                                     diplomacy_main_response);
    gui_dialog_set_default_response(dipl_main->dialog,
                                    RESPONSE_CANCEL_MEETING_ALL);

    dipl_box = dipl_main->dialog->vbox;
    gtk_container_add(GTK_CONTAINER(dipl_box), dipl_sw);

    gui_dialog_show_all(dipl_main->dialog);
    gui_dialog_present(dipl_main->dialog);
  }

  return dipl_main;
}

/************************************************************************//**
  Destroy main diplomacy dialog.
****************************************************************************/
static void diplomacy_main_destroy(void)
{
  if (dipl_main->dialog) {
    gui_dialog_destroy(dipl_main->dialog);
  }
  free(dipl_main);
  dipl_main = NULL;
}

/************************************************************************//**
  User has responded to whole diplomacy dialog (main tab).
****************************************************************************/
static void diplomacy_main_response(struct gui_dialog *dlg, int response,
                                    gpointer data)
{
  if (!dipl_main) {
    return;
  }

  switch (response) {
  default:
    log_error("unhandled response in %s: %d", __FUNCTION__, response);
    /* No break. */
  case GTK_RESPONSE_DELETE_EVENT:   /* GTK: delete the widget. */
  case RESPONSE_CANCEL_MEETING_ALL: /* Cancel all meetings. */
    dialog_list_iterate(dialog_list, adialog) {
      /* This will do a round trip to the server ans close the diolag in the
       * client. Closing the last dialog will also close the main tab.*/
      dsend_packet_diplomacy_cancel_meeting_req(&client.conn,
                                                player_number(
                                                  adialog->treaty.plr1));
    } dialog_list_iterate_end;
    break;
  }
}

/************************************************************************//**
  Destroy diplomacy dialog
****************************************************************************/
static void diplomacy_destroy(struct Diplomacy_dialog* pdialog)
{
  if (NULL != pdialog->dialog) {
    /* pdialog->dialog may be NULL if the tab have been destroyed
     * by an other way. */
    gui_dialog_destroy(pdialog->dialog);
  }
  dialog_list_remove(dialog_list, pdialog);
  free(pdialog);

  if (dialog_list) {
    /* Diplomatic meetings in one main tab. */
    if (dialog_list_size(dialog_list) > 0) {
      if (dipl_main && dipl_main->dialog) {
        gchar *buf;

        buf = g_strdup_printf(_("Diplomacy [%d]"), dialog_list_size(dialog_list));
        gui_dialog_set_title(dipl_main->dialog, buf);
        g_free(buf);
      }
    } else if (dipl_main) {
      /* No meeting left - destroy main tab. */
      diplomacy_main_destroy();
    }
  }
}

/************************************************************************//**
  User has responded to whole diplomacy dialog (one meeting).
****************************************************************************/
static void diplomacy_response(struct gui_dialog *dlg, int response,
                               gpointer data)
{
  struct Diplomacy_dialog *pdialog = NULL;

  fc_assert_ret(data);
  pdialog = (struct Diplomacy_dialog *)data;

  switch (response) {
  case GTK_RESPONSE_ACCEPT:         /* Accept treaty. */
    dsend_packet_diplomacy_accept_treaty_req(&client.conn,
                                             player_number(
                                               pdialog->treaty.plr1));
    break;

  default:
    log_error("unhandled response in %s: %d", __FUNCTION__, response);
    /* No break. */
  case GTK_RESPONSE_DELETE_EVENT:   /* GTK: delete the widget. */
  case GTK_RESPONSE_CANCEL:         /* GTK: cancel button. */
  case RESPONSE_CANCEL_MEETING:     /* Cancel meetings. */
    dsend_packet_diplomacy_cancel_meeting_req(&client.conn,
                                              player_number(
                                                pdialog->treaty.plr1));
    break;
  }
}

/************************************************************************//**
  Setups diplomacy dialog widgets.
****************************************************************************/
static struct Diplomacy_dialog *create_diplomacy_dialog(struct player *plr0,
                                                        struct player *plr1)
{
  struct Diplomacy_notebook *dipl_dialog;
  GtkWidget *vbox, *hbox, *table, *mainbox;
  GtkWidget *label, *sw, *view, *image, *spin;
  GtkWidget *menubar, *menuitem, *menu, *notebook;
  struct sprite *flag_spr;
  GtkListStore *store;
  GtkCellRenderer *rend;
  int i;

  struct Diplomacy_dialog *pdialog;
  char plr_buf[4 * MAX_LEN_NAME];
  gchar *buf;

  pdialog = fc_malloc(sizeof(*pdialog));

  dialog_list_prepend(dialog_list, pdialog);
  init_treaty(&pdialog->treaty, plr0, plr1);

  /* Get main diplomacy tab. */
  dipl_dialog = diplomacy_main_create();

  buf = g_strdup_printf(_("Diplomacy [%d]"), dialog_list_size(dialog_list));
  gui_dialog_set_title(dipl_dialog->dialog, buf);
  g_free(buf);

  notebook = dipl_dialog->notebook;

  gui_dialog_new(&(pdialog->dialog), GTK_NOTEBOOK(notebook), pdialog, FALSE);

  /* Buttons */
  gui_dialog_add_stockbutton(pdialog->dialog, GTK_STOCK_DND,
                             _("Accept treaty"), GTK_RESPONSE_ACCEPT);
  gui_dialog_add_stockbutton(pdialog->dialog, GTK_STOCK_CANCEL,
                             _("Cancel meeting"), RESPONSE_CANCEL_MEETING);

  /* Responces for one meeting. */
  gui_dialog_response_set_callback(pdialog->dialog, diplomacy_response);
  gui_dialog_set_default_response(pdialog->dialog, RESPONSE_CANCEL_MEETING);

  /* Label for the new meeting. */
  buf = g_strdup_printf("%s", nation_plural_for_player(plr1));
  gui_dialog_set_title(pdialog->dialog, buf);

  /* Sort meeting tabs alphabetically by the tab label. */
  for (i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)); i++) {
    GtkWidget *prev_page
      = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
    struct gui_dialog *prev_dialog
      = g_object_get_data(G_OBJECT(prev_page), "gui-dialog-data");
    const char *prev_label
      = gtk_label_get_text(GTK_LABEL(prev_dialog->v.tab.label));

    if (fc_strcasecmp(buf, prev_label) < 0) {
      gtk_notebook_reorder_child(GTK_NOTEBOOK(notebook),
                                 pdialog->dialog->vbox, i);
      break;
    }
  }
  g_free(buf);

  /* Content. */
  mainbox = pdialog->dialog->vbox;

  /* us. */
  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing(GTK_GRID(vbox), 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
  gtk_container_add(GTK_CONTAINER(mainbox), vbox);

  /* Our nation. */
  label = gtk_label_new(NULL);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  buf = g_strdup_printf("<span size=\"large\"><u>%s</u></span>",
                        nation_plural_for_player(plr0));
  gtk_label_set_markup(GTK_LABEL(label), buf);
  g_free(buf);
  gtk_container_add(GTK_CONTAINER(vbox), label);

  hbox = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(hbox), 5);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  /* Our flag */
  flag_spr = get_nation_flag_sprite(tileset, nation_of_player(plr0));

  image = gtk_image_new_from_surface(flag_spr->surface);
  gtk_container_add(GTK_CONTAINER(hbox), image);

  /* Our name. */
  label = gtk_label_new(NULL);
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  buf = g_strdup_printf("<span size=\"large\" weight=\"bold\">%s</span>",
                        ruler_title_for_player(plr0, plr_buf, sizeof(plr_buf)));
  gtk_label_set_markup(GTK_LABEL(label), buf);
  g_free(buf);
  gtk_container_add(GTK_CONTAINER(hbox), label);

  image = gtk_image_new();
  pdialog->image0 = image;
  gtk_container_add(GTK_CONTAINER(hbox), image);

  /* Menu for clauses: we. */
  menubar = gtk_aux_menu_bar_new();

  menu = gtk_menu_new();
  pdialog->menu0 = menu;

  menuitem = gtk_image_menu_item_new_with_label(_("Add Clause..."));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),
                                gtk_image_new_from_stock(GTK_STOCK_ADD,
                                                         GTK_ICON_SIZE_MENU));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuitem);
  g_object_set_data(G_OBJECT(menu), "plr", plr0);
  g_signal_connect(menu, "show", G_CALLBACK(popup_add_menu), pdialog);

  /* Main table for clauses and (if activated) gold trading: we. */
  table = gtk_grid_new();
  gtk_widget_set_halign(table, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(table, GTK_ALIGN_CENTER);
  gtk_grid_set_column_spacing(GTK_GRID(table), 16);
  gtk_container_add(GTK_CONTAINER(vbox), table);

  if (game.info.trading_gold) {
    spin = gtk_spin_button_new_with_range(0.0, plr0->economic.gold + 0.1,
                                          1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
    gtk_entry_set_width_chars(GTK_ENTRY(spin), 16);
    gtk_grid_attach(GTK_GRID(table), spin, 1, 0, 1, 1);
    g_object_set_data(G_OBJECT(spin), "plr", plr0);
    g_signal_connect_after(spin, "value-changed",
                           G_CALLBACK(diplo_dialog_returnkey), pdialog);

    label = g_object_new(GTK_TYPE_LABEL, "use-underline", TRUE,
                         "mnemonic-widget", spin, "label", _("Gold:"),
                         "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(table), menubar, 2, 0, 1, 1);
  } else {
    gtk_grid_attach(GTK_GRID(table), menubar, 0, 0, 1, 1);
  }

  /* them. */
  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing(GTK_GRID(vbox), 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
  gtk_container_add(GTK_CONTAINER(mainbox), vbox);

  /* Their nation. */
  label = gtk_label_new(NULL);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  buf = g_strdup_printf("<span size=\"large\"><u>%s</u></span>",
                        nation_plural_for_player(plr1));
  gtk_label_set_markup(GTK_LABEL(label), buf);
  g_free(buf);
  gtk_container_add(GTK_CONTAINER(vbox), label);

  hbox = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(hbox), 5);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  /* Their flag */
  flag_spr = get_nation_flag_sprite(tileset, nation_of_player(plr1));

  image = gtk_image_new_from_surface(flag_spr->surface);
  gtk_container_add(GTK_CONTAINER(hbox), image);

  /* Their name. */
  label = gtk_label_new(NULL);
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  buf = g_strdup_printf("<span size=\"large\" weight=\"bold\">%s</span>",
                        ruler_title_for_player(plr1, plr_buf, sizeof(plr_buf)));
  gtk_label_set_markup(GTK_LABEL(label), buf);
  g_free(buf);
  gtk_container_add(GTK_CONTAINER(hbox), label);

  image = gtk_image_new();
  pdialog->image1 = image;
  gtk_container_add(GTK_CONTAINER(hbox), image);

  /* Menu for clauses: they. */
  menubar = gtk_aux_menu_bar_new();

  menu = gtk_menu_new();
  pdialog->menu1 = menu;

  menuitem = gtk_image_menu_item_new_with_label(_("Add Clause..."));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),
                                gtk_image_new_from_stock(GTK_STOCK_ADD,
                                                         GTK_ICON_SIZE_MENU));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuitem);
  g_object_set_data(G_OBJECT(menu), "plr", plr1);
  g_signal_connect(menu, "show", G_CALLBACK(popup_add_menu), pdialog);

  /* Main table for clauses and (if activated) gold trading: they. */
  table = gtk_grid_new();
  gtk_widget_set_halign(table, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(table, GTK_ALIGN_CENTER);
  gtk_grid_set_column_spacing(GTK_GRID(table), 16);
  gtk_container_add(GTK_CONTAINER(vbox), table);

  if (game.info.trading_gold) {
    spin = gtk_spin_button_new_with_range(0.0, plr1->economic.gold + 0.1,
                                          1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
    gtk_entry_set_width_chars(GTK_ENTRY(spin), 16);
    gtk_grid_attach(GTK_GRID(table), spin, 1, 0, 1, 1);
    g_object_set_data(G_OBJECT(spin), "plr", plr1);
    g_signal_connect_after(spin, "value-changed",
                           G_CALLBACK(diplo_dialog_returnkey), pdialog);

    label = g_object_new(GTK_TYPE_LABEL, "use-underline", TRUE,
                         "mnemonic-widget", spin, "label", _("Gold:"),
                         "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(table), menubar, 2, 0, 1, 1);
  } else {
    gtk_grid_attach(GTK_GRID(table), menubar, 0, 0, 1, 1);
  }

  /* Clauses. */
  mainbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(mainbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(pdialog->dialog->vbox), mainbox);

  store = gtk_list_store_new(1, G_TYPE_STRING);
  pdialog->store = store;

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_hexpand(view, TRUE);
  gtk_widget_set_vexpand(view, TRUE);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
  g_object_unref(store);
  gtk_widget_set_size_request(view, 320, 100);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
    rend, "text", 0, NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  g_object_set(sw, "margin", 2, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("C_lauses:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);

  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(mainbox), vbox);
  gtk_container_add(GTK_CONTAINER(vbox), label);
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  gtk_widget_show_all(mainbox);

  g_signal_connect(view, "row_activated", G_CALLBACK(row_callback), pdialog);

  update_diplomacy_dialog(pdialog);
  gui_dialog_show_all(pdialog->dialog);

  return pdialog;
}

/************************************************************************//**
  Update diplomacy dialog
****************************************************************************/
static void update_diplomacy_dialog(struct Diplomacy_dialog *pdialog)
{
  GtkListStore *store;
  GtkTreeIter it;
  bool blank = TRUE;
  GdkPixbuf *pixbuf;

  store = pdialog->store;

  gtk_list_store_clear(store);
  clause_list_iterate(pdialog->treaty.clauses, pclause) {
    char buf[128];

    client_diplomacy_clause_string(buf, sizeof(buf), pclause);

    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it, 0, buf, -1);
    blank = FALSE;
  } clause_list_iterate_end;

  if (blank) {
    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it, 0,
		       _("--- This treaty is blank. "
		 	 "Please add some clauses. ---"), -1);
  }

  pixbuf = get_thumb_pixbuf(pdialog->treaty.accept0);
  gtk_image_set_from_pixbuf(GTK_IMAGE(pdialog->image0), pixbuf);
  g_object_unref(G_OBJECT(pixbuf));
  pixbuf = get_thumb_pixbuf(pdialog->treaty.accept1);
  gtk_image_set_from_pixbuf(GTK_IMAGE(pdialog->image1), pixbuf);
  g_object_unref(G_OBJECT(pixbuf));
}

/************************************************************************//**
  Callback for the diplomatic dialog: give tech.
****************************************************************************/
static void diplomacy_dialog_tech_callback(GtkWidget *w, gpointer data)
{
  int giver, dest, other, tech;

  giver = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "player_from"));
  dest = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "player_to"));
  tech = GPOINTER_TO_INT(data);
  if (player_by_number(giver) == client_player()) {
    other = dest;
  } else {
    other = giver;
  }

  if (A_LAST == tech) {
    /* All techs. */
    struct player *pgiver = player_by_number(giver);
    struct player *pdest = player_by_number(dest);
    const struct research *dresearch, *gresearch;

    fc_assert_ret(NULL != pgiver);
    fc_assert_ret(NULL != pdest);

    dresearch = research_get(pdest);
    gresearch = research_get(pgiver);
    advance_iterate(A_FIRST, padvance) {
      Tech_type_id i = advance_number(padvance);

      if (research_invention_state(gresearch, i) == TECH_KNOWN
          && research_invention_gettable(dresearch, i,
                                         game.info.tech_trade_allow_holes)
          && (research_invention_state(dresearch, i) == TECH_UNKNOWN
              || research_invention_state(dresearch, i)
                 == TECH_PREREQS_KNOWN)) {
        dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
                                                 CLAUSE_ADVANCE, i);
      }
    } advance_iterate_end;
  } else {
    /* Only one tech. */
    dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
                                             CLAUSE_ADVANCE, tech);
  }
}

/************************************************************************//**
  Callback for trading cities
			      - Kris Bubendorfer
****************************************************************************/
static void diplomacy_dialog_city_callback(GtkWidget *w, gpointer data)
{
  size_t choice = GPOINTER_TO_UINT(data);
  int giver = (choice >> 24) & 0xff, dest = (choice >> 16) & 0xff, other;
  int city = choice & 0xffff;

  if (player_by_number(giver) == client.conn.playing) {
    other = dest;
  } else {
    other = giver;
  }

  dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
					   CLAUSE_CITY, city);
}

/************************************************************************//**
  Map menu item activated
****************************************************************************/
static void diplomacy_dialog_map_callback(GtkWidget *w, gpointer data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *)data;
  struct player *pgiver;
  
  pgiver = (struct player *)g_object_get_data(G_OBJECT(w), "plr");

  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(pdialog->treaty.plr1),
                                           player_number(pgiver), CLAUSE_MAP, 0);
}

/************************************************************************//**
  Seamap menu item activated
****************************************************************************/
static void diplomacy_dialog_seamap_callback(GtkWidget *w, gpointer data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *)data;
  struct player *pgiver;
  
  pgiver = (struct player *)g_object_get_data(G_OBJECT(w), "plr");

  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(pdialog->treaty.plr1),
                                           player_number(pgiver), CLAUSE_SEAMAP,
                                           0);
}

/************************************************************************//**
  Adding pact clause
****************************************************************************/
static void diplomacy_dialog_add_pact_clause(GtkWidget *w, gpointer data,
                                             int type)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *)data;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pdialog->treaty.plr0),
					   type, 0);
}

/************************************************************************//**
  Ceasefire pact menu item activated
****************************************************************************/
static void diplomacy_dialog_ceasefire_callback(GtkWidget *w, gpointer data)
{
  diplomacy_dialog_add_pact_clause(w, data, CLAUSE_CEASEFIRE);
}

/************************************************************************//**
  Peace pact menu item activated
****************************************************************************/
static void diplomacy_dialog_peace_callback(GtkWidget *w, gpointer data)
{
  diplomacy_dialog_add_pact_clause(w, data, CLAUSE_PEACE);
}

/************************************************************************//**
  Alliance pact menu item activated
****************************************************************************/
static void diplomacy_dialog_alliance_callback(GtkWidget *w, gpointer data)
{
  diplomacy_dialog_add_pact_clause(w, data, CLAUSE_ALLIANCE);
}

/************************************************************************//**
  Shared vision menu item activated
****************************************************************************/
static void diplomacy_dialog_vision_callback(GtkWidget *w, gpointer data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) data;
  struct player *pgiver =
      (struct player *) g_object_get_data(G_OBJECT(w), "plr");

  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(pdialog->treaty.plr1),
                                           player_number(pgiver), CLAUSE_VISION,
                                           0);
}

/************************************************************************//**
  Embassy menu item activated
****************************************************************************/
static void diplomacy_dialog_embassy_callback(GtkWidget *w, gpointer data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) data;
  struct player *pgiver =
      (struct player *) g_object_get_data(G_OBJECT(w), "plr");

  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(pdialog->treaty.plr1),
                                           player_number(pgiver), CLAUSE_EMBASSY,
                                           0);
}

/************************************************************************//**
  Close diplomacy dialog
****************************************************************************/
void close_diplomacy_dialog(struct Diplomacy_dialog *pdialog)
{
  diplomacy_destroy(pdialog);
}

/************************************************************************//**
  Initialize diplomacy dialog
****************************************************************************/
void diplomacy_dialog_init()
{
  dialog_list = dialog_list_new();
  dipl_main = NULL;
}

/************************************************************************//**
  Free resources allocated for diplomacy dialog
****************************************************************************/
void diplomacy_dialog_done()
{
  dialog_list_destroy(dialog_list);
}

/************************************************************************//**
  Find diplomacy dialog between player and other player
****************************************************************************/
static struct Diplomacy_dialog *find_diplomacy_dialog(int other_player_id)
{
  struct player *plr0 = client.conn.playing;
  struct player *plr1 = player_by_number(other_player_id);

  dialog_list_iterate(dialog_list, pdialog) {
    if ((pdialog->treaty.plr0 == plr0 && pdialog->treaty.plr1 == plr1) ||
        (pdialog->treaty.plr0 == plr1 && pdialog->treaty.plr1 == plr0)) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/************************************************************************//**
  User hit enter after entering gold amount
****************************************************************************/
static void diplo_dialog_returnkey(GtkWidget *w, gpointer data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) data;
  struct player *pgiver =
      (struct player *) g_object_get_data(G_OBJECT(w), "plr");
  int amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
  
  if (amount >= 0 && amount <= pgiver->economic.gold) {
    dsend_packet_diplomacy_create_clause_req(&client.conn,
                                             player_number(pdialog->treaty.plr1),
                                             player_number(pgiver),
                                             CLAUSE_GOLD, amount);
  } else {
    output_window_append(ftc_client, _("Invalid amount of gold specified."));
  }
}

/************************************************************************//**
  Close all dialogs, for when client disconnects from game.
****************************************************************************/
void close_all_diplomacy_dialogs(void)
{
  while (dialog_list_size(dialog_list) > 0) {
    close_diplomacy_dialog(dialog_list_get(dialog_list, 0));
  }
}
