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

/* client */
#include "dialogs_g.h"
#include "chatline.h"
#include "choice_dialog.h"
#include "client_main.h"
#include "climisc.h"
#include "control.h"
#include "gui_main.h"
#include "gui_stuff.h"

#include "dialogs.h"
#include "wldlg.h"

static int caravan_city_id;
static int caravan_unit_id;

static GtkWidget *caravan_dialog;

/****************************************************************
...
*****************************************************************/
static void caravan_establish_trade_callback(GtkWidget *w, gpointer data)
{
  dsend_packet_unit_establish_trade(&client.conn, caravan_unit_id);
}

/****************************************************************
...
*****************************************************************/
static void caravan_help_build_wonder_callback(GtkWidget *w, gpointer data)
{
  dsend_packet_unit_help_build_wonder(&client.conn, caravan_unit_id);
}

/****************************************************************
...
*****************************************************************/
static void caravan_destroy_callback(GtkWidget *w, gpointer data)
{
  caravan_dialog = NULL;
  process_caravan_arrival(NULL);
}

/****************************************************************
  Fills the buf with proper text which should be displayed on 
  the helpbuild wonder button.
*****************************************************************/
static void get_help_build_wonder_button_label(char* buf, int bufsize,
                                               bool* help_build_possible)
{
  struct city* destcity = game_find_city_by_number(caravan_city_id);
  struct unit* caravan = game_find_unit_by_number(caravan_unit_id);
  
  if (destcity && caravan
      && unit_can_help_build_wonder(caravan, destcity)) {
    my_snprintf(buf, bufsize, _("Help build _Wonder (%d remaining)"),
	impr_build_shield_cost(destcity->production.value.building)
	- destcity->shield_stock);
    *help_build_possible = TRUE;
  } else {
    my_snprintf(buf, bufsize, _("Help build _Wonder"));
    *help_build_possible = FALSE;
  }
}

/****************************************************************
...
*****************************************************************/
void popup_caravan_dialog(struct unit *punit,
			  struct city *phomecity, struct city *pdestcity)
{
  char buf[128], wonder[128];
  bool can_establish, can_trade, can_wonder;
  
  my_snprintf(buf, sizeof(buf),
	      _("Your caravan from %s reaches the city of %s.\nWhat now?"),
	      city_name(phomecity), city_name(pdestcity));
  
  caravan_city_id=pdestcity->id; /* callbacks need these */
  caravan_unit_id=punit->id;
  
  get_help_build_wonder_button_label(wonder, sizeof(wonder), &can_wonder);
  
  can_trade = can_cities_trade(phomecity, pdestcity);
  can_establish = can_trade
  		  && can_establish_trade_route(phomecity, pdestcity);


  caravan_dialog = popup_choice_dialog(GTK_WINDOW(toplevel),
    _("Your Caravan Has Arrived"), 
    buf,
    (can_establish ? _("Establish _Traderoute") :
    _("Enter Marketplace")),caravan_establish_trade_callback, NULL,
    wonder,caravan_help_build_wonder_callback, NULL,
    _("_Keep moving"), NULL, NULL,
    NULL);

  g_signal_connect(caravan_dialog, "destroy",
		   G_CALLBACK(caravan_destroy_callback), NULL);
  
  if (!can_trade) {
    choice_dialog_button_set_sensitive(caravan_dialog, 0, FALSE);
  }
  
  if (!can_wonder) {
    choice_dialog_button_set_sensitive(caravan_dialog, 1, FALSE);
  }
}

/****************************************************************
  Returns whether the caravan dialog is open, and sets 
  caravan id and destination city id, if they are not NULL.
*****************************************************************/
bool caravan_dialog_is_open(int* unit_id, int* city_id)
{
  if (unit_id) {
    *unit_id = caravan_unit_id;
  }
  if (city_id) {
    *city_id = caravan_city_id;
  }
  return caravan_dialog != NULL;
}

/****************************************************************
  Updates caravan dialog
****************************************************************/
void caravan_dialog_update(void)
{
  char buf[128];
  bool can_help;
  get_help_build_wonder_button_label(buf, sizeof(buf), &can_help);
  choice_dialog_button_set_label(caravan_dialog, 1, buf);
  choice_dialog_button_set_sensitive(caravan_dialog, 1, can_help);
}
