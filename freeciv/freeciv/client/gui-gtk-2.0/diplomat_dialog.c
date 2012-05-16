/********************************************************************** 
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
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

#include <gtk/gtk.h>

/* utility */
#include "support.h"

/* common */
#include "game.h"
#include "unit.h"
#include "unitlist.h"

/* client */
#include "dialogs_g.h"
#include "chatline.h"
#include "choice_dialog.h"
#include "client_main.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "control.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "packhand.h"

#include "dialogs.h"
#include "wldlg.h"

static GtkWidget *diplomat_dialog;
static int diplomat_id;
static int diplomat_target_id;

static GtkWidget  *spy_tech_shell;
static int         steal_advance;

static GtkWidget  *spy_sabotage_shell;
static int         sabotage_improvement;

/****************************************************************
...
*****************************************************************/
static void bribe_response(GtkWidget *w, gint response)
{
  if (response == GTK_RESPONSE_YES) {
    request_diplomat_action(DIPLOMAT_BRIBE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(w);
}

/****************************************************************
...  Ask the server how much the bribe is
*****************************************************************/
static void diplomat_bribe_callback(GtkWidget *w, gpointer data)
{
  if (game_find_unit_by_number(diplomat_id)
   && game_find_unit_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_BRIBE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
void popup_bribe_dialog(struct unit *punit, int cost)
{
  GtkWidget *shell;

  if (unit_has_type_flag(punit, F_UNBRIBABLE)) {
    shell = popup_choice_dialog(GTK_WINDOW(toplevel), _("Ooops..."),
                                 _("This unit cannot be bribed!"),
                                 GTK_STOCK_OK, NULL, NULL, NULL);
    gtk_window_present(GTK_WINDOW(shell));
    return;
  } else if (cost <= client.conn.playing->economic.gold) {
    shell = gtk_message_dialog_new(NULL, 0,
      GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
      _("Bribe unit for %d gold?\nTreasury contains %d gold."),
      cost, client.conn.playing->economic.gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Bribe Enemy Unit"));
    setup_dialog(shell, toplevel);
  } else {
    shell = gtk_message_dialog_new(NULL,
      0,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
      _("Bribing the unit costs %d gold.\nTreasury contains %d gold."),
      cost, client.conn.playing->economic.gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Traitors Demand Too Much!"));
    setup_dialog(shell, toplevel);
  }
  gtk_window_present(GTK_WINDOW(shell));
  
  g_signal_connect(shell, "response", G_CALLBACK(bribe_response), NULL);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_sabotage_callback(GtkWidget *w, gpointer data)
{
  if(game_find_unit_by_number(diplomat_id) && 
     game_find_city_by_number(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, -1);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_investigate_callback(GtkWidget *w, gpointer data)
{
  if(game_find_unit_by_number(diplomat_id) && 
     (game_find_city_by_number(diplomat_target_id))) { 
    request_diplomat_action(DIPLOMAT_INVESTIGATE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void spy_sabotage_unit_callback(GtkWidget *w, gpointer data)
{
  request_diplomat_action(SPY_SABOTAGE_UNIT, diplomat_id,
			  diplomat_target_id, 0);
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_embassy_callback(GtkWidget *w, gpointer data)
{
  if(game_find_unit_by_number(diplomat_id) && 
     (game_find_city_by_number(diplomat_target_id))) { 
    request_diplomat_action(DIPLOMAT_EMBASSY, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void spy_poison_callback(GtkWidget *w, gpointer data)
{
  if(game_find_unit_by_number(diplomat_id) &&
     (game_find_city_by_number(diplomat_target_id))) {
    request_diplomat_action(SPY_POISON, diplomat_id, diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_steal_callback(GtkWidget *w, gpointer data)
{
  if(game_find_unit_by_number(diplomat_id) && 
     game_find_city_by_number(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
			    diplomat_target_id, A_UNSET);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void spy_advances_response(GtkWidget *w, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT && steal_advance > 0) {
    if (game_find_unit_by_number(diplomat_id) && 
        game_find_city_by_number(diplomat_target_id)) { 
      request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
			      diplomat_target_id, steal_advance);
    }
  }
  gtk_widget_destroy(spy_tech_shell);
  spy_tech_shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void spy_advances_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_tree_model_get(model, &it, 1, &steal_advance, -1);
    
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_tech_shell),
      GTK_RESPONSE_ACCEPT, TRUE);
  } else {
    steal_advance = 0;
	  
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_tech_shell),
      GTK_RESPONSE_ACCEPT, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void create_advances_list(struct player *pplayer,
				 struct player *pvictim)
{  
  GtkWidget *sw, *label, *vbox, *view;
  GtkListStore *store;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  spy_tech_shell = gtk_dialog_new_with_buttons(_("Steal Technology"),
    NULL,
    0,
    GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL,
    _("_Steal"),
    GTK_RESPONSE_ACCEPT,
    NULL);
  setup_dialog(spy_tech_shell, toplevel);
  gtk_window_set_position(GTK_WINDOW(spy_tech_shell), GTK_WIN_POS_MOUSE);

  gtk_dialog_set_default_response(GTK_DIALOG(spy_tech_shell),
				  GTK_RESPONSE_ACCEPT);

  label = gtk_frame_new(_("Select Advance to Steal"));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(spy_tech_shell)->vbox), label);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(label), vbox);
      
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
						 "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("_Advances:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_container_add(GTK_CONTAINER(vbox), label);
  
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_set_size_request(sw, -1, 200);
  
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  /* Now populate the list */
  if (pvictim) { /* you don't want to know what lag can do -- Syela */
    GtkTreeIter it;
    GValue value = { 0, };

    advance_index_iterate(A_FIRST, i) {
      if(player_invention_state(pvictim, i)==TECH_KNOWN && 
	 (player_invention_state(pplayer, i)==TECH_UNKNOWN || 
	  player_invention_state(pplayer, i)==TECH_PREREQS_KNOWN)) {
	gtk_list_store_append(store, &it);

	g_value_init(&value, G_TYPE_STRING);
	g_value_set_static_string(&value,
				  advance_name_for_player(client.conn.playing, i));
	gtk_list_store_set_value(store, &it, 0, &value);
	g_value_unset(&value);
	gtk_list_store_set(store, &it, 1, i, -1);
      }
    } advance_index_iterate_end;

    gtk_list_store_append(store, &it);

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, _("At Spy's Discretion"));
    gtk_list_store_set_value(store, &it, 0, &value);
    g_value_unset(&value);
    gtk_list_store_set(store, &it, 1, A_UNSET, -1);
  }

  gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_tech_shell),
    GTK_RESPONSE_ACCEPT, FALSE);
  
  gtk_widget_show_all(GTK_DIALOG(spy_tech_shell)->vbox);

  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), "changed",
		   G_CALLBACK(spy_advances_callback), NULL);
  g_signal_connect(spy_tech_shell, "response",
		   G_CALLBACK(spy_advances_response), NULL);
  
  steal_advance = 0;

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/****************************************************************
...
*****************************************************************/
static void spy_improvements_response(GtkWidget *w, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT && sabotage_improvement > -2) {
    if (game_find_unit_by_number(diplomat_id) && 
        game_find_city_by_number(diplomat_target_id)) { 
      request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
			      diplomat_target_id, sabotage_improvement + 1);
    }
  }
  gtk_widget_destroy(spy_sabotage_shell);
  spy_sabotage_shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void spy_improvements_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_tree_model_get(model, &it, 1, &sabotage_improvement, -1);
    
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_sabotage_shell),
      GTK_RESPONSE_ACCEPT, TRUE);
  } else {
    sabotage_improvement = -2;
	  
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_sabotage_shell),
      GTK_RESPONSE_ACCEPT, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void create_improvements_list(struct player *pplayer,
				     struct city *pcity)
{  
  GtkWidget *sw, *label, *vbox, *view;
  GtkListStore *store;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;
  GtkTreeIter it;
  
  spy_sabotage_shell = gtk_dialog_new_with_buttons(_("Sabotage Improvements"),
    NULL,
    0,
    GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL,
    _("_Sabotage"), 
    GTK_RESPONSE_ACCEPT,
    NULL);
  setup_dialog(spy_sabotage_shell, toplevel);
  gtk_window_set_position(GTK_WINDOW(spy_sabotage_shell), GTK_WIN_POS_MOUSE);

  gtk_dialog_set_default_response(GTK_DIALOG(spy_sabotage_shell),
				  GTK_RESPONSE_ACCEPT);

  label = gtk_frame_new(_("Select Improvement to Sabotage"));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(spy_sabotage_shell)->vbox), label);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(label), vbox);
      
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
						 "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("_Improvements:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_container_add(GTK_CONTAINER(vbox), label);
  
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_set_size_request(sw, -1, 200);
  
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  /* Now populate the list */
  gtk_list_store_append(store, &it);
  gtk_list_store_set(store, &it, 0, _("City Production"), 1, -1, -1);

  city_built_iterate(pcity, pimprove) {
    if (pimprove->sabotage > 0) {
      gtk_list_store_append(store, &it);
      gtk_list_store_set(store, &it,
                         0, city_improvement_name_translation(pcity, pimprove),
                         1, improvement_number(pimprove),
                         -1);
    }  
  } city_built_iterate_end;

  gtk_list_store_append(store, &it);
  gtk_list_store_set(store, &it, 0, _("At Spy's Discretion"), 1, B_LAST, -1);

  gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_sabotage_shell),
    GTK_RESPONSE_ACCEPT, FALSE);
  
  gtk_widget_show_all(GTK_DIALOG(spy_sabotage_shell)->vbox);

  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), "changed",
		   G_CALLBACK(spy_improvements_callback), NULL);
  g_signal_connect(spy_sabotage_shell, "response",
		   G_CALLBACK(spy_improvements_response), NULL);

  sabotage_improvement = -2;
	  
  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/****************************************************************
...
*****************************************************************/
static void spy_steal_popup(GtkWidget *w, gpointer data)
{
  struct city *pvcity = game_find_city_by_number(diplomat_target_id);
  struct player *pvictim = NULL;

  if(pvcity)
    pvictim = city_owner(pvcity);

/* it is concievable that pvcity will not be found, because something
has happened to the city during latency.  Therefore we must initialize
pvictim to NULL and account for !pvictim in create_advances_list. -- Syela */
  
  if(!spy_tech_shell){
    create_advances_list(client.conn.playing, pvictim);
    gtk_window_present(GTK_WINDOW(spy_tech_shell));
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
 Requests up-to-date list of improvements, the return of
 which will trigger the popup_sabotage_dialog() function.
*****************************************************************/
static void spy_request_sabotage_list(GtkWidget *w, gpointer data)
{
  if (game_find_unit_by_number(diplomat_id)
   && game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
 Pops-up the Spy sabotage dialog, upon return of list of
 available improvements requested by the above function.
*****************************************************************/
void popup_sabotage_dialog(struct city *pcity)
{
  if(!spy_sabotage_shell){
    create_improvements_list(client.conn.playing, pcity);
    gtk_window_present(GTK_WINDOW(spy_sabotage_shell));
  }
}

/****************************************************************
...  Ask the server how much the revolt is going to cost us
*****************************************************************/
static void diplomat_incite_callback(GtkWidget *w, gpointer data)
{
  if (game_find_unit_by_number(diplomat_id)
   && game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_INCITE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void incite_response(GtkWidget *w, gint response)
{
  if (response == GTK_RESPONSE_YES) {
    request_diplomat_action(DIPLOMAT_INCITE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(w);
}

/****************************************************************
Popup the yes/no dialog for inciting, since we know the cost now
*****************************************************************/
void popup_incite_dialog(struct city *pcity, int cost)
{
  GtkWidget *shell;
  
  if (INCITE_IMPOSSIBLE_COST == cost) {
    shell = gtk_message_dialog_new(NULL,
      0,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
      _("You can't incite a revolt in %s."),
      city_name(pcity));
    gtk_window_set_title(GTK_WINDOW(shell), _("City can't be incited!"));
  setup_dialog(shell, toplevel);
  } else if (cost <= client.conn.playing->economic.gold) {
    shell = gtk_message_dialog_new(NULL,
      0,
      GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
      _("Incite a revolt for %d gold?\nTreasury contains %d gold."),
      cost, client.conn.playing->economic.gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Incite a Revolt!"));
    setup_dialog(shell, toplevel);
  } else {
    shell = gtk_message_dialog_new(NULL,
      0,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
      _("Inciting a revolt costs %d gold.\nTreasury contains %d gold."),
      cost, client.conn.playing->economic.gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Traitors Demand Too Much!"));
    setup_dialog(shell, toplevel);
  }
  gtk_window_present(GTK_WINDOW(shell));
  
  g_signal_connect(shell, "response", G_CALLBACK(incite_response), NULL);
}


/****************************************************************
  Callback from diplomat/spy dialog for "keep moving".
  (This should only occur when entering allied city.)
*****************************************************************/
static void diplomat_keep_moving_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit;
  struct city *pcity;
  
  if( (punit=game_find_unit_by_number(diplomat_id))
      && (pcity=game_find_city_by_number(diplomat_target_id))
      && !same_pos(punit->tile, pcity->tile)) {
    request_diplomat_action(DIPLOMAT_MOVE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_destroy_callback(GtkWidget *w, gpointer data)
{
  diplomat_dialog = NULL;
  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_cancel_callback(GtkWidget *w, gpointer data)
{
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
void popup_diplomat_dialog(struct unit *punit, struct tile *dest_tile)
{
  struct city *pcity;
  struct unit *ptunit;
  GtkWidget *shl;
  char buf[128];

  diplomat_id = punit->id;

  if ((pcity = tile_city(dest_tile))) {
    /* Spy/Diplomat acting against a city */

    diplomat_target_id = pcity->id;
    my_snprintf(buf, sizeof(buf),
		_("Your %s has arrived at %s.\nWhat is your command?"),
		unit_name_translation(punit),
		city_name(pcity));

    if (!unit_has_type_flag(punit, F_SPY)){
      shl = popup_choice_dialog(GTK_WINDOW(toplevel),
	_("Choose Your Diplomat's Strategy"), buf,
	_("Establish _Embassy"), diplomat_embassy_callback, NULL,
	_("_Investigate City"), diplomat_investigate_callback, NULL,
	_("_Sabotage City"), diplomat_sabotage_callback, NULL,
	_("Steal _Technology"), diplomat_steal_callback, NULL,
	_("Incite a _Revolt"), diplomat_incite_callback, NULL,
	_("_Keep moving"), diplomat_keep_moving_callback, NULL,
	GTK_STOCK_CANCEL, diplomat_cancel_callback, NULL,
	NULL);

      if (!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, dest_tile))
	choice_dialog_button_set_sensitive(shl, 0, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 1, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 2, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_STEAL, dest_tile))
	choice_dialog_button_set_sensitive(shl, 3, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INCITE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 4, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_MOVE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 5, FALSE);
    } else {
       shl = popup_choice_dialog(GTK_WINDOW(toplevel),
	_("Choose Your Spy's Strategy"), buf,
	_("Establish _Embassy"), diplomat_embassy_callback, NULL,
	_("_Investigate City"), diplomat_investigate_callback, NULL,
	_("_Poison City"), spy_poison_callback, NULL,
	_("Industrial _Sabotage"), spy_request_sabotage_list, NULL,
	_("Steal _Technology"), spy_steal_popup, NULL,
	_("Incite a _Revolt"), diplomat_incite_callback, NULL,
	_("_Keep moving"), diplomat_keep_moving_callback, NULL,
	GTK_STOCK_CANCEL, diplomat_cancel_callback, NULL,
	NULL);

      if (!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, dest_tile))
	choice_dialog_button_set_sensitive(shl, 0, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 1, FALSE);
      if (!diplomat_can_do_action(punit, SPY_POISON, dest_tile))
	choice_dialog_button_set_sensitive(shl, 2, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 3, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_STEAL, dest_tile))
	choice_dialog_button_set_sensitive(shl, 4, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INCITE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 5, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_MOVE, dest_tile))
	choice_dialog_button_set_sensitive(shl, 6, FALSE);
     }

    diplomat_dialog = shl;

    choice_dialog_set_hide(shl, TRUE);
    g_signal_connect(shl, "destroy",
		     G_CALLBACK(diplomat_destroy_callback), NULL);
    g_signal_connect(shl, "delete_event",
		     G_CALLBACK(diplomat_cancel_callback), NULL);
  } else { 
    if ((ptunit = unit_list_get(dest_tile->units, 0))){
      /* Spy/Diplomat acting against a unit */ 
       
      diplomat_target_id = ptunit->id;
 
      shl = popup_choice_dialog(GTK_WINDOW(toplevel),
	_("Subvert Enemy Unit"),
	(!unit_has_type_flag(punit, F_SPY))?
	_("The diplomat is waiting for your command"):
	_("The spy is waiting for your command"),
	_("_Bribe Enemy Unit"), diplomat_bribe_callback, NULL,
	_("_Sabotage Enemy Unit"), spy_sabotage_unit_callback, NULL,
	GTK_STOCK_CANCEL, diplomat_cancel_callback, NULL,
	NULL);

      if (!diplomat_can_do_action(punit, DIPLOMAT_BRIBE, dest_tile)) {
	choice_dialog_button_set_sensitive(shl, 0, FALSE);
      }
      if (!diplomat_can_do_action(punit, SPY_SABOTAGE_UNIT, dest_tile)) {
	choice_dialog_button_set_sensitive(shl, 1, FALSE);
      }

      diplomat_dialog = shl;

      choice_dialog_set_hide(shl, TRUE);
      g_signal_connect(shl, "destroy",
		       G_CALLBACK(diplomat_destroy_callback), NULL);
      g_signal_connect(shl, "delete_event",
		       G_CALLBACK(diplomat_cancel_callback), NULL);
    }
  }
}

/****************************************************************
  Returns id of a diplomat currently handled in diplomat dialog
*****************************************************************/
int diplomat_handled_in_diplomat_dialog(void)
{
  if (diplomat_dialog == NULL) {
    return -1;
  }
  return diplomat_id;
}

/****************************************************************
  Closes the diplomat dialog
****************************************************************/
void close_diplomat_dialog(void)
{
  if (diplomat_dialog != NULL) {
    gtk_widget_destroy(diplomat_dialog);
  }
}
