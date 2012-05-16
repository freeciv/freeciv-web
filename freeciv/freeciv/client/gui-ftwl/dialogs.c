/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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

/* common & utility */
#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "packets.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* client */
#include "chatline_common.h"
#include "client_main.h"
#include "gui_main.h"
#include "widget.h"

#include "dialogs.h"
#include "mapview.h"

static struct sw_widget *nations_window;
#if 0
static struct sw_widget *nations_list;
static struct sw_widget *leaders_list;
static struct sw_widget *leaders_sex_list;
static Nation_type_id selected_nation;
#endif

/**************************************************************************
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
**************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  /* PORTME */
}

/**************************************************************************
  Popup a dialog to display connection message from server.
**************************************************************************/
void popup_connect_msg(const char *headline, const char *message)
{
  /* FIXME: Needs proper implementation.
   *        Now just puts to chat window so message is not completely lost. */

  output_window_append(FTC_CLIENT_INFO, NULL, message);
}

/**************************************************************************
  Popup a generic dialog to display some generic information.
**************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
			 const char *lines)
{
  /* PORTME */
}

#if 0
/****************************************************************
 ...
*****************************************************************/
static void connect_callback(struct sw_widget *list, void *data)
{
  int leader_count, leader=sw_list_get_selected_row(leaders_list);
  struct nation_leader *leaders = get_nation_leaders(selected_nation, &leader_count);

  if (strlen(leaders[leader].name) == 0) {
    append_output_window(_("You must type a legal name."));
    return;
  }

  dsend_packet_nation_select_req(&client.conn, selected_nation,
				 sw_list_get_selected_row(leaders_sex_list)==0,
				 leaders[leader].name,1);
}

/****************************************************************
 ...
*****************************************************************/
static void select_random_nation(void)
{
  /* try to find a free nation */
  while (1) {
    int index = myrand(game.control.playable_nation_count);

    if (sw_list_is_row_enabled(nations_list, index)) {
      sw_list_set_selected_row(nations_list, index, TRUE);
      break;
    }
  }
}

/****************************************************************
  Selects a leader.
*****************************************************************/
static void select_random_leader(void)
{
  int leader_count, i;
  
  get_nation_leaders(selected_nation, &leader_count);

  i = myrand(leader_count);
  sw_list_set_selected_row(leaders_list, i, TRUE);
}

/****************************************************************
 ...
*****************************************************************/
static void nations_list_selection_changed(struct sw_widget *widget,
					   void *data)
{
  int row = sw_list_get_selected_row(nations_list);
  struct nation_type *nation = nation_by_number(row);
  int leader_count, i;
  struct nation_leader *leaders = get_nation_leaders(row, &leader_count);

  selected_nation = row;

  freelog(LOG_DEBUG, "selected %s\n", nation_rule_name(nation));
  sw_list_clear(leaders_list);

  for (i = 0; i < leader_count; i++) {
    struct sw_widget *label;
    struct ct_string *string;

    string = ct_string_create(STYLE_NORMAL, 14,
			      ct_extend_std_color(COLOR_STD_BLACK),
			      COLOR_EXT_GRAY, leaders[i].name);
    label = sw_label_create_text(root_window, string);
    sw_list_set_item(leaders_list, 0, i, label);
  }
  select_random_leader();
}

/****************************************************************
 ...
*****************************************************************/
static void leaders_list_selection_changed(struct sw_widget *widget,
					   void *data)
{
  int row = sw_list_get_selected_row(leaders_list);
  int leader_count;
  struct nation_leader *leaders = get_nation_leaders(selected_nation, &leader_count);

  sw_list_set_selected_row(leaders_sex_list, leaders[row].is_male ? 0 : 1,
			   FALSE);
}
#endif

/**************************************************************************
  Popup the nation selection dialog.
**************************************************************************/
void popup_races_dialog(struct player *pplayer)
{
#if 0
  struct ct_string *string;
  struct sw_widget *label;
  struct sw_widget *button;
  int i;

  string = ct_string_create(STYLE_NORMAL, 14,
			    ct_extend_std_color(COLOR_STD_BLACK),
			    COLOR_EXT_GRAY, "Choose your nation");

  nations_window =
      sw_window_create(root_window, 600, 350, string, 0, TRUE);
  sw_widget_set_position(nations_window, 20, 120);
  sw_widget_set_background_color(nations_window, COLOR_STD_RACE2);

  string = ct_string_create(STYLE_NORMAL, 34,
			    ct_extend_std_color(COLOR_STD_BLACK),
			    COLOR_EXT_BLUE, _("Ok"));
  button =
      sw_button_create(nations_window, string, NULL, NULL,
		       theme.button_background);
  sw_widget_set_position(button, 30, 250);
  sw_button_set_callback(button, connect_callback, NULL);

  nations_list = sw_list_create(nations_window, 200, 200);
  sw_widget_set_position(nations_list, 20, 20);
  sw_list_add_buttons_and_vslider(nations_list, theme.button.up,
				  theme.button.down,
				  theme.button_background,
				  theme.scrollbar.vertic);
  sw_list_set_selection_changed_notify(nations_list,
				       nations_list_selection_changed, NULL);

  leaders_list = sw_list_create(nations_window, 200, 100);
  sw_widget_set_position(leaders_list, 300, 20);
  sw_list_add_buttons_and_vslider(leaders_list, theme.button.up,
				  theme.button.down,
				  theme.button_background,
				  theme.scrollbar.vertic);
  sw_list_set_selection_changed_notify(leaders_list,
				       leaders_list_selection_changed, NULL);

  leaders_sex_list = sw_list_create(nations_window, 200, 50);
  sw_widget_set_position(leaders_sex_list, 300, 150);

  string = ct_string_create(STYLE_NORMAL, 14,
			    ct_extend_std_color(COLOR_STD_BLACK),
			    COLOR_EXT_GRAY, _("Male"));
  label = sw_label_create_text(root_window, string);
  sw_list_set_item(leaders_sex_list, 0, 0, label);

  string = ct_string_create(STYLE_NORMAL, 14,
			    ct_extend_std_color(COLOR_STD_BLACK),
			    COLOR_EXT_GRAY, _("Female"));
  label = sw_label_create_text(root_window, string);
  sw_list_set_item(leaders_sex_list, 0, 1, label);

  for (i = 0; i < game.control.playable_nation_count; i++) {
    struct nation_type *nation = nation_by_number(i);

    button = sw_button_create(nations_window, NULL,
			      NULL, nation->flag_sprite, NULL);
    sw_list_set_item(nations_list, 0, i, button);

    string = ct_string_create(STYLE_NORMAL, 14,
			      ct_extend_std_color(COLOR_STD_BLACK),
			      COLOR_EXT_GRAY, nation_adjective_translation(nation));
    label = sw_label_create_text(root_window, string);
    sw_list_set_item(nations_list, 1, i, label);

    string = ct_string_create(STYLE_ITALIC, 14,
			      ct_extend_std_color(COLOR_STD_BLACK),
			      COLOR_EXT_GRAY, Q_(nation->category));
    label = sw_label_create_text(root_window, string);
    sw_list_set_item(nations_list, 2, i, label);    
  }
  select_random_nation();
#endif
}

/**************************************************************************
  Close the nation selection dialog.  This should allow the user to
  (at least) select a unit to activate.
**************************************************************************/
void popdown_races_dialog(void)
{
  if (nations_window) {
    sw_widget_destroy(nations_window);
    nations_window = NULL;
  }
}

/**************************************************************************
  Popup a dialog window to select units on a particular tile.
**************************************************************************/
void popup_unit_select_dialog(struct tile *ptile)
{
  /* PORTME */
}

/**************************************************************************
  In the nation selection dialog, make already-taken nations unavailable.
  This information is contained in the packet_nations_used packet.
**************************************************************************/
void races_toggles_set_sensitive(void)
{
  /* PORTME */
}

/**************************************************************************
  Popup a dialog giving a player choices when their caravan arrives at
  a city (other than its home city).  Example:
    - Establish traderoute.
    - Help build wonder.
    - Keep moving.
**************************************************************************/
void popup_caravan_dialog(struct unit *punit,
			  struct city *phomecity, struct city *pdestcity)
{
  /* PORTME */
}

/**************************************************************************
  Is there currently a caravan dialog open?  This is important if there
  can be only one such dialog at a time; otherwise return FALSE.
**************************************************************************/
bool caravan_dialog_is_open(int* unit_id, int* city_id)
{
  /* PORTME */
  return FALSE;
}

/****************************************************************
  Updates caravan dialog
****************************************************************/
void caravan_dialog_update(void)
{
  /* PORTME */
}

/**************************************************************************
  Popup a dialog giving a diplomatic unit some options when moving into
  the target tile.
**************************************************************************/
void popup_diplomat_dialog(struct unit *punit, struct tile *dest_tile)
{
  /* PORTME */
}

/****************************************************************
  Returns id of a diplomat currently handled in diplomat dialog
*****************************************************************/
int diplomat_handled_in_diplomat_dialog(void)
{
  /* PORTME */    
  return -1;  
}

/****************************************************************
  Closes the diplomat dialog
****************************************************************/
void close_diplomat_dialog(void)
{
  /* PORTME */
}

/**************************************************************************
  Popup a window asking a diplomatic unit if it wishes to incite the
  given enemy city.
**************************************************************************/
void popup_incite_dialog(struct city *pcity, int cost)
{
  /* PORTME */
}

/**************************************************************************
  Popup a dialog asking a diplomatic unit if it wishes to bribe the
  given enemy unit.
**************************************************************************/
void popup_bribe_dialog(struct unit *punit, int cost)
{
  /* PORTME */
}

/**************************************************************************
  Popup a dialog asking a diplomatic unit if it wishes to sabotage the
  given enemy city.
**************************************************************************/
void popup_sabotage_dialog(struct city *pcity)
{
  /* PORTME */
}

/**************************************************************************
  Popup a dialog asking the unit which improvement they would like to
  pillage.
**************************************************************************/
void popup_pillage_dialog(struct unit *punit, bv_special may_pillage,
                          bv_bases bases)
{
  /* PORTME */
}

/**************************************************************************
  This function is called when the client disconnects or the game is
  over.  It should close all dialog windows for that game.
**************************************************************************/
void popdown_all_game_dialogs(void)
{
  /* PORTME */
  clear_focus_tile();
}

/**************************************************************************
  Ruleset (modpack) has suggested loading certain tileset. Confirm from
  user and load.
**************************************************************************/
void popup_tileset_suggestion_dialog(void)
{
}

/**************************************************************************
  Tileset (modpack) has suggested loading certain theme. Confirm from
  user and load.
**************************************************************************/
bool popup_theme_suggestion_dialog(const char *theme_name)
{
  /* Don't load */
  return FALSE;
}
