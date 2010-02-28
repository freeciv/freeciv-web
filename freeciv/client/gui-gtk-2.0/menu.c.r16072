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

#include <stdlib.h>

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "game.h"
#include "government.h"
#include "unit.h"

/* client */
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "control.h"
#include "options.h"
#include "tilespec.h"

/* gui-gtk-2.0 */
#include "chatline.h"
#include "cityrep.h"
#include "dialogs.h"
#include "editgui.h"
#include "editprop.h"
#include "finddlg.h"
#include "gotodlg.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "messagedlg.h"
#include "messagewin.h"
#include "optiondlg.h"
#include "pages.h"
#include "plrdlg.h"
#include "ratesdlg.h"
#include "repodlgs.h"
#include "spaceshipdlg.h"
#include "wldlg.h"

#include "menu.h"

static GtkUIManager *ui_manager = NULL;

static GtkActionGroup *get_safe_group(void);
static GtkActionGroup *get_edit_group(void);
static GtkActionGroup *get_unit_group(void);
static GtkActionGroup *get_playing_group(void);
static GtkActionGroup *get_player_group(void);

static void menus_set_active(GtkActionGroup *group,
                             const gchar *action_name,
                             gboolean is_active);
static void menus_set_sensitive(GtkActionGroup *group,
                                const gchar *action_name,
                                gboolean is_sensitive);

/****************************************************************
  Action "CLEAR_CHAT_LOGS" callback.
*****************************************************************/
static void clear_chat_logs_callback(GtkAction *action, gpointer data)
{
  clear_output_window();
}

/****************************************************************
  Action "SAVE_CHAT_LOGS" callback.
*****************************************************************/
static void save_chat_logs_callback(GtkAction *action, gpointer data)
{
  log_output_window();
}

/****************************************************************
  Action "LOCAL_OPTIONS" callback.
*****************************************************************/
static void local_options_callback(GtkAction *action, gpointer data)
{
  popup_option_dialog();
}

/****************************************************************
  Action "MESSAGE_OPTIONS" callback.
*****************************************************************/
static void message_options_callback(GtkAction *action, gpointer data)
{
  popup_messageopt_dialog();
}

/****************************************************************
  Action "SERVER_OPTIONS" callback.
*****************************************************************/
static void server_options_callback(GtkAction *action, gpointer data)
{
  popup_settable_options_dialog();
}

/****************************************************************
  Action "SAVE_OPTIONS" callback.
*****************************************************************/
static void save_options_callback(GtkAction *action, gpointer data)
{
  save_options();
}

/****************************************************************
  Action "RELOAD_TILESET" callback.
*****************************************************************/
static void reload_tileset_callback(GtkAction *action, gpointer data)
{
  tilespec_reread(NULL);
}

/****************************************************************
  Action "SAVE_GAME" callback.
*****************************************************************/
static void save_game_callback(GtkAction *action, gpointer data)
{
  send_save_game(NULL);
}

/****************************************************************
  Action "SAVE_GAME_AS" callback.
*****************************************************************/
static void save_game_as_callback(GtkAction *action, gpointer data)
{
  popup_save_dialog(FALSE);
}

/****************************************************************
  This is the response callback for the dialog with the message:
  Leaving a local game will end it!
****************************************************************/
static void leave_local_game_response(GtkWidget *dialog, gint response)
{
  gtk_widget_destroy(dialog);
  if (response == GTK_RESPONSE_OK) {
    /* It might be killed already */
    if (client.conn.used) {
      /* It will also kill the server */
      disconnect_from_server();
    }
  }
}

/****************************************************************
  Action "LEAVE" callback.
*****************************************************************/
static void leave_callback(GtkAction *action, gpointer data)
{
  if (is_server_running()) {
    GtkWidget* dialog =
        gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_WARNING,
                               GTK_BUTTONS_OK_CANCEL,
                               _("Leaving a local game will end it!"));
    setup_dialog(dialog, toplevel);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    g_signal_connect(dialog, "response", 
                     G_CALLBACK(leave_local_game_response), NULL);
    gtk_window_present(GTK_WINDOW(dialog));
  } else {
    disconnect_from_server();
  }
}

/****************************************************************
  Action "QUIT" callback.
*****************************************************************/
static void quit_callback(GtkAction *action, gpointer data)
{
  popup_quit_dialog();
}

/****************************************************************
  Action "FIND_CITY" callback.
*****************************************************************/
static void find_city_callback(GtkAction *action, gpointer data)
{
  popup_find_dialog();
}

/****************************************************************
  Action "WORKLISTS" callback.
*****************************************************************/
static void worklists_callback(GtkAction *action, gpointer data)
{
  popup_worklists_report();
}

/****************************************************************
  Action "MAP_VIEW" callback.
*****************************************************************/
static void map_view_callback(GtkAction *action, gpointer data)
{
  map_canvas_focus();
}

/****************************************************************
  Action "REPORT_NATIONS" callback.
*****************************************************************/
static void report_nations_callback(GtkAction *action, gpointer data)
{
  popup_players_dialog(TRUE);
}

/****************************************************************
  Action "REPORT_WOW" callback.
*****************************************************************/
static void report_wow_callback(GtkAction *action, gpointer data)
{
  send_report_request(REPORT_WONDERS_OF_THE_WORLD);
}

/****************************************************************
  Action "REPORT_TOP_CITIES" callback.
*****************************************************************/
static void report_top_cities_callback(GtkAction *action, gpointer data)
{
  send_report_request(REPORT_TOP_5_CITIES);
}

/****************************************************************
  Action "REPORT_MESSAGES" callback.
*****************************************************************/
static void report_messages_callback(GtkAction *action, gpointer data)
{
  popup_meswin_dialog(TRUE);
}

/****************************************************************
  Action "REPORT_DEMOGRAPHIC" callback.
*****************************************************************/
static void report_demographic_callback(GtkAction *action, gpointer data)
{
  send_report_request(REPORT_DEMOGRAPHIC);
}

/****************************************************************
  Action "HELP_LANGUAGE" callback.
*****************************************************************/
static void help_language_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_LANGUAGES_ITEM);
}

/****************************************************************
  Action "HELP_CONNECTING" callback.
*****************************************************************/
static void help_connecting_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_CONNECTING_ITEM);
}

/****************************************************************
  Action "HELP_CONTROLS" callback.
*****************************************************************/
static void help_controls_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_CONTROLS_ITEM);
}

/****************************************************************
  Action "HELP_CHATLINE" callback.
*****************************************************************/
static void help_chatline_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_CHATLINE_ITEM);
}

/****************************************************************
  Action "HELP_WORKLIST_EDITOR" callback.
*****************************************************************/
static void help_worklist_editor_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_WORKLIST_EDITOR_ITEM);
}

/****************************************************************
  Action "HELP_CMA" callback.
*****************************************************************/
static void help_cma_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_CMA_ITEM);
}

/****************************************************************
  Action "HELP_PLAYING" callback.
*****************************************************************/
static void help_playing_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_PLAYING_ITEM);
}

/****************************************************************
  Action "HELP_RULESET" callback.
*****************************************************************/
static void help_ruleset_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_RULESET_ITEM);
}

/****************************************************************
  Action "HELP_IMPROVEMENTS" callback.
*****************************************************************/
static void help_improvements_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_IMPROVEMENTS_ITEM);
}

/****************************************************************
  Action "HELP_UNITS" callback.
*****************************************************************/
static void help_units_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_UNITS_ITEM);
}

/****************************************************************
  Action "HELP_COMBAT" callback.
*****************************************************************/
static void help_combat_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_COMBAT_ITEM);
}

/****************************************************************
  Action "HELP_ZOC" callback.
*****************************************************************/
static void help_zoc_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_ZOC_ITEM);
}

/****************************************************************
  Action "HELP_TECH" callback.
*****************************************************************/
static void help_tech_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_TECHS_ITEM);
}

/****************************************************************
  Action "HELP_TERRAIN" callback.
*****************************************************************/
static void help_terrain_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_TERRAIN_ITEM);
}

/****************************************************************
  Action "HELP_WONDERS" callback.
*****************************************************************/
static void help_wonders_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_WONDERS_ITEM);
}

/****************************************************************
  Action "HELP_GOVERNMENT" callback.
*****************************************************************/
static void help_government_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_GOVERNMENT_ITEM);
}

/****************************************************************
  Action "HELP_HAPPINESS" callback.
*****************************************************************/
static void help_happiness_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_HAPPINESS_ITEM);
}

/****************************************************************
  Action "HELP_SPACE_RATE" callback.
*****************************************************************/
static void help_space_rate_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_SPACE_RACE_ITEM);
}

/****************************************************************
  Action "HELP_COPYING" callback.
*****************************************************************/
static void help_copying_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_COPYING_ITEM);
}

/****************************************************************
  Action "HELP_ABOUT" callback.
*****************************************************************/
static void help_about_callback(GtkAction *action, gpointer data)
{
  popup_help_dialog_string(HELP_ABOUT_ITEM);
}

/****************************************************************
  Action "SAVE_OPTIONS_ON_EXIT" callback.
*****************************************************************/
static void save_options_on_exit_callback(GtkToggleAction *action,
                                          gpointer data)
{
  save_options_on_exit = gtk_toggle_action_get_active(action);
}

/****************************************************************
  Action "EDIT_MODE" callback.
*****************************************************************/
static void edit_mode_callback(GtkToggleAction *action, gpointer data)
{
  if (game.info.is_edit_mode ^ gtk_toggle_action_get_active(action)) {
    key_editor_toggle();
    popdown_science_dialog(); /* Unreachbale techs in reqtree on/off */
  }
}

/****************************************************************
  Action "SHOW_CITY_OUTLINES" callback.
*****************************************************************/
static void show_city_outlines_callback(GtkToggleAction *action,
                                        gpointer data)
{
  if (draw_city_outlines ^ gtk_toggle_action_get_active(action)) {
    key_city_outlines_toggle();
  }
}

/****************************************************************
  Action "SHOW_CITY_OUTPUT" callback.
*****************************************************************/
static void show_city_output_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_city_output ^ gtk_toggle_action_get_active(action)) {
    key_city_output_toggle();
  }
}

/****************************************************************
  Action "SHOW_MAP_GRID" callback.
*****************************************************************/
static void show_map_grid_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_map_grid ^ gtk_toggle_action_get_active(action)) {
    key_map_grid_toggle();
  }
}

/****************************************************************
  Action "SHOW_NATIONAL_BORDERS" callback.
*****************************************************************/
static void show_national_borders_callback(GtkToggleAction *action,
                                           gpointer data)
{
  if (draw_borders ^ gtk_toggle_action_get_active(action)) {
    key_map_borders_toggle();
  }
}

/****************************************************************
  Action "SHOW_CITY_NAMES" callback.
*****************************************************************/
static void show_city_names_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_city_names ^ gtk_toggle_action_get_active(action)) {
    key_city_names_toggle();

    if (!draw_full_citybar) {
      /* The "full" city bar (i.e. the new way of drawing the
       * city name), can draw the city growth even without drawing
       * the city name. But the old method cannot. */
      menus_set_sensitive(get_safe_group(), "SHOW_CITY_GROWTH",
                          draw_city_names);
      menus_set_sensitive(get_safe_group(), "SHOW_CITY_TRADEROUTES",
                          draw_city_names);
    }
  }
}

/****************************************************************
  Action "SHOW_CITY_GROWTH" callback.
*****************************************************************/
static void show_city_growth_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_city_growth ^ gtk_toggle_action_get_active(action)) {
    key_city_growth_toggle();
  }
}

/****************************************************************
  Action "SHOW_CITY_PRODUCTIONS" callback.
*****************************************************************/
static void show_city_productions_callback(GtkToggleAction *action,
                                           gpointer data)
{
  if (draw_city_productions ^ gtk_toggle_action_get_active(action)) {
    key_city_productions_toggle();
    menus_set_sensitive(get_safe_group(), "SHOW_CITY_BUY_COST",
                        draw_city_productions);
  }
}

/****************************************************************
  Action "SHOW_CITY_BUY_COST" callback.
*****************************************************************/
static void show_city_buy_cost_callback(GtkToggleAction *action,
                                        gpointer data)
{
  if (draw_city_buycost ^ gtk_toggle_action_get_active(action)) {
    key_city_buycost_toggle();
  }
}

/****************************************************************
  Action "SHOW_CITY_TRADEROUTES" callback.
*****************************************************************/
static void show_city_traderoutes_callback(GtkToggleAction *action,
                                           gpointer data)
{
  if (draw_city_traderoutes ^ gtk_toggle_action_get_active(action)) {
    key_city_traderoutes_toggle();
  }
}

/****************************************************************
  Action "SHOW_TERRAIN" callback.
*****************************************************************/
static void show_terrain_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_terrain ^ gtk_toggle_action_get_active(action)) {
    key_terrain_toggle();
    menus_set_sensitive(get_safe_group(), "SHOW_COASTLINE", !draw_terrain);
  }
}

/****************************************************************
  Action "SHOW_COASTLINE" callback.
*****************************************************************/
static void show_coastline_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_coastline ^ gtk_toggle_action_get_active(action)) {
    key_coastline_toggle();
  }
}

/****************************************************************
  Action "SHOW_ROAD_RAILS" callback.
*****************************************************************/
static void show_road_rails_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_roads_rails ^ gtk_toggle_action_get_active(action)) {
    key_roads_rails_toggle();
  }
}

/****************************************************************
  Action "SHOW_IRRIGATION" callback.
*****************************************************************/
static void show_irrigation_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_irrigation ^ gtk_toggle_action_get_active(action)) {
    key_irrigation_toggle();
  }
}

/****************************************************************
  Action "SHOW_MINE" callback.
*****************************************************************/
static void show_mine_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_mines ^ gtk_toggle_action_get_active(action)) {
    key_mines_toggle();
  }
}

/****************************************************************
  Action "SHOW_FORTRESS_AIRBASE" callback.
*****************************************************************/
static void show_fortress_airbase_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_fortress_airbase ^ gtk_toggle_action_get_active(action)) {
    key_fortress_airbase_toggle();
  }
}

/****************************************************************
  Action "SHOW_SPECIALS" callback.
*****************************************************************/
static void show_specials_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_specials ^ gtk_toggle_action_get_active(action)) {
    key_specials_toggle();
  }
}

/****************************************************************
  Action "SHOW_POLLUTION" callback.
*****************************************************************/
static void show_pollution_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_pollution ^ gtk_toggle_action_get_active(action)) {
    key_pollution_toggle();
  }
}

/****************************************************************
  Action "SHOW_CITIES" callback.
*****************************************************************/
static void show_cities_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_cities ^ gtk_toggle_action_get_active(action)) {
    key_cities_toggle();
  }
}

/****************************************************************
  Action "SHOW_UNITS" callback.
*****************************************************************/
static void show_units_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_units ^ gtk_toggle_action_get_active(action)) {
    key_units_toggle();
    menus_set_sensitive(get_safe_group(), "SHOW_FOCUS_UNIT", !draw_units);
  }
}

/****************************************************************
  Action "SHOW_FOCUS_UNIT" callback.
*****************************************************************/
static void show_focus_unit_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_focus_unit ^ gtk_toggle_action_get_active(action)) {
    key_focus_unit_toggle();
  }
}

/****************************************************************
  Action "SHOW_FOG_OF_WAR" callback.
*****************************************************************/
static void show_fog_of_war_callback(GtkToggleAction *action, gpointer data)
{
  if (draw_fog_of_war ^ gtk_toggle_action_get_active(action)) {
    key_fog_of_war_toggle();
  }
}

/****************************************************************
  Action "SHOW_NATIONAL_BORDERS" callback.
*****************************************************************/
static void full_screen_callback(GtkToggleAction *action, gpointer data)
{
  if (fullscreen_mode ^ gtk_toggle_action_get_active(action)) {
    fullscreen_mode ^= 1;

    if (fullscreen_mode) {
      gtk_window_fullscreen(GTK_WINDOW(toplevel));
    } else {
      gtk_window_unfullscreen(GTK_WINDOW(toplevel));
    }
  }
}

/****************************************************************
  Action "RECALC_BORDERS" callback.
*****************************************************************/
static void recalc_borders_callback(GtkAction *action, gpointer data)
{
  key_editor_recalculate_borders();
}

/****************************************************************
  Action "TOGGLE_FOG" callback.
*****************************************************************/
static void toggle_fog_callback(GtkAction *action, gpointer data)
{
  key_editor_toggle_fogofwar();
}

/****************************************************************
  Action "SCENARIO_PROPERTIES" callback.
*****************************************************************/
static void scenario_properties_callback(GtkAction *action, gpointer data)
{
  popup_save_dialog(TRUE);
}

/****************************************************************
  Action "SAVE_SCENARIO" callback.
*****************************************************************/
static void save_scenario_callback(GtkAction *action, gpointer data)
{
  struct property_editor *pe;

  pe = editprop_get_property_editor();
  property_editor_reload(pe, OBJTYPE_GAME);
  property_editor_popup(pe, OBJTYPE_GAME);
}

/****************************************************************
  Action "SELECT_SINGLE" callback.
*****************************************************************/
static void select_single_callback(GtkAction *action, gpointer data)
{
  request_unit_select(get_units_in_focus(), SELTYPE_SINGLE, SELLOC_TILE);
}

/****************************************************************
  Action "SELECT_ALL_ON_TILE" callback.
*****************************************************************/
static void select_all_on_tile_callback(GtkAction *action, gpointer data)
{
  request_unit_select(get_units_in_focus(), SELTYPE_ALL, SELLOC_TILE);
}

/****************************************************************
  Action "SELECT_SAME_TYPE_TILE" callback.
*****************************************************************/
static void select_same_type_tile_callback(GtkAction *action, gpointer data)
{
  request_unit_select(get_units_in_focus(), SELTYPE_SAME, SELLOC_TILE);
}

/****************************************************************
  Action "SELECT_SAME_TYPE_CONT" callback.
*****************************************************************/
static void select_same_type_cont_callback(GtkAction *action, gpointer data)
{
  request_unit_select(get_units_in_focus(), SELTYPE_SAME, SELLOC_CONT);
}

/****************************************************************
  Action "SELECT_SAME_TYPE" callback.
*****************************************************************/
static void select_same_type_callback(GtkAction *action, gpointer data)
{
  request_unit_select(get_units_in_focus(), SELTYPE_SAME, SELLOC_ALL);
}

/****************************************************************
  Action "UNIT_WAIT" callback.
*****************************************************************/
static void unit_wait_callback(GtkAction *action, gpointer data)
{
  key_unit_wait();
}

/****************************************************************
  Action "UNIT_DONE" callback.
*****************************************************************/
static void unit_done_callback(GtkAction *action, gpointer data)
{
  key_unit_done();
}

/****************************************************************
  Action "UNIT_GOTO" callback.
*****************************************************************/
static void unit_goto_callback(GtkAction *action, gpointer data)
{
  key_unit_goto();
}

/****************************************************************
  Action "UNIT_GOTO_CITY" callback.
*****************************************************************/
static void unit_goto_city_callback(GtkAction *action, gpointer data)
{
  if (get_num_units_in_focus() > 0) {
    popup_goto_dialog();
  }
}

/****************************************************************
  Action "UNIT_RETURN" callback.
*****************************************************************/
static void unit_return_callback(GtkAction *action, gpointer data)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_return(punit);
  } unit_list_iterate_end;
}

/****************************************************************
  Action "UNIT_EXPLORE" callback.
*****************************************************************/
static void unit_explore_callback(GtkAction *action, gpointer data)
{
  key_unit_auto_explore();
}

/****************************************************************
  Action "UNIT_PATROL" callback.
*****************************************************************/
static void unit_patrol_callback(GtkAction *action, gpointer data)
{
  key_unit_patrol();
}

/****************************************************************
  Action "UNIT_SENTRY" callback.
*****************************************************************/
static void unit_sentry_callback(GtkAction *action, gpointer data)
{
  key_unit_sentry();
}

/****************************************************************
  Action "UNIT_UNSENTRY" callback.
*****************************************************************/
static void unit_unsentry_callback(GtkAction *action, gpointer data)
{
  key_unit_wakeup_others();
}

/****************************************************************
  Action "UNIT_LOAD" callback.
*****************************************************************/
static void unit_load_callback(GtkAction *action, gpointer data)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_load(punit, NULL);
  } unit_list_iterate_end;
}

/****************************************************************
  Action "UNIT_UNLOAD" callback.
*****************************************************************/
static void unit_unload_callback(GtkAction *action, gpointer data)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_unload(punit);
  } unit_list_iterate_end;
}

/****************************************************************
  Action "UNIT_UNLOAD_TRANSPORTER" callback.
*****************************************************************/
static void unit_unload_transporter_callback(GtkAction *action,
                                             gpointer data)
{
  key_unit_unload_all();
}

/****************************************************************
  Action "UNIT_HOMECITY" callback.
*****************************************************************/
static void unit_homecity_callback(GtkAction *action, gpointer data)
{
  key_unit_homecity();
}

/****************************************************************
  Action "UNIT_UPGRADE" callback.
*****************************************************************/
static void unit_upgrade_callback(GtkAction *action, gpointer data)
{
  popup_upgrade_dialog(get_units_in_focus());
}

/****************************************************************
  Action "UNIT_TRANSFORM" callback.
*****************************************************************/
static void unit_transform_callback(GtkAction *action, gpointer data)
{
  key_unit_transform_unit();
}

/****************************************************************
  Action "UNIT_DISBAND" callback.
*****************************************************************/
static void unit_disband_callback(GtkAction *action, gpointer data)
{
  key_unit_disband();
}

/****************************************************************
  Action "BUILD_CITY" callback.
*****************************************************************/
static void build_city_callback(GtkAction *action, gpointer data)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    /* FIXME: this can provide different actions for different units...
     * not good! */
    /* Enable the button for adding to a city in all cases, so we
       get an eventual error message from the server if we try. */
    if (can_unit_add_or_build_city(punit)) {
      request_unit_build_city(punit);
    } else if (unit_has_type_flag(punit, F_HELP_WONDER)) {
      request_unit_caravan_action(punit, PACKET_UNIT_HELP_BUILD_WONDER);
    }
  } unit_list_iterate_end;
}

/****************************************************************
  Action "GO_BUILD_CITY" callback.
*****************************************************************/
static void go_build_city_callback(GtkAction *action, gpointer data)
{
  request_unit_goto(ORDER_BUILD_CITY);
}

/****************************************************************
  Action "AUTO_SETTLE" callback.
*****************************************************************/
static void auto_settle_callback(GtkAction *action, gpointer data)
{
  key_unit_auto_settle();
}

/****************************************************************
  Action "BUILD_ROAD" callback.
*****************************************************************/
static void build_road_callback(GtkAction *action, gpointer data)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    /* FIXME: this can provide different actions for different units...
     * not good! */
    if (can_unit_do_activity(punit, ACTIVITY_ROAD)) {
      request_new_unit_activity(punit, ACTIVITY_ROAD);
    } else if (unit_can_est_traderoute_here(punit)) {
      request_unit_caravan_action(punit, PACKET_UNIT_ESTABLISH_TRADE);
    }
  } unit_list_iterate_end;
}

/****************************************************************
  Action "BUILD_IRRIGATION" callback.
*****************************************************************/
static void build_irrigation_callback(GtkAction *action, gpointer data)
{
  key_unit_irrigate();
}

/****************************************************************
  Action "BUILD_MINE" callback.
*****************************************************************/
static void build_mine_callack(GtkAction *action, gpointer data)
{
  key_unit_mine();
}

/****************************************************************
  Action "CONNECT_ROAD" callback.
*****************************************************************/
static void connect_road_callback(GtkAction *action, gpointer data)
{
  key_unit_connect(ACTIVITY_ROAD);
}

/****************************************************************
  Action "CONNECT_RAIL" callback.
*****************************************************************/
static void connect_rail_callback(GtkAction *action, gpointer data)
{
  key_unit_connect(ACTIVITY_RAILROAD);
}

/****************************************************************
  Action "CONNECT_IRRIGATION" callback.
*****************************************************************/
static void connect_irrigation_callack(GtkAction *action, gpointer data)
{
  key_unit_connect(ACTIVITY_IRRIGATE);
}

/****************************************************************
  Action "TRANSFORM_TERRAIN" callback.
*****************************************************************/
static void transform_terrain_callack(GtkAction *action, gpointer data)
{
  key_unit_transform();
}

/****************************************************************
  Action "CLEAN_POLLUTION" callback.
*****************************************************************/
static void clean_pollution_callback(GtkAction *action, gpointer data)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    /* FIXME: this can provide different actions for different units...
     * not good! */
    if (can_unit_do_activity(punit, ACTIVITY_POLLUTION)) {
      request_unit_pillage(punit);
    } else if (can_unit_paradrop(punit)) {
      /* FIXME: This is getting worse, we use a key_unit_*() function
       * which assign the order for all units!  Very bad! */
      key_unit_paradrop();
    }
  } unit_list_iterate_end;
}

/****************************************************************
  Action "CLEAN_FALLOUT" callback.
*****************************************************************/
static void clean_fallout_callback(GtkAction *action, gpointer data)
{
  key_unit_fallout();
}

/****************************************************************
  Action "BUILD_FORTRESS" callback.
*****************************************************************/
static void build_fortress_callback(GtkAction *action, gpointer data)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    /* FIXME: this can provide different actions for different units...
     * not good! */
    struct base_type *pbase = get_base_by_gui_type(BASE_GUI_FORTRESS,
                                                   punit, punit->tile);

    if (pbase && can_unit_do_activity_base(punit, pbase->item_number)) {
      request_new_unit_activity_base(punit, pbase);
    } else {
      request_unit_fortify(punit);
    }
  } unit_list_iterate_end;
}

/****************************************************************
  Action "BUILD_AIRBASE" callback.
*****************************************************************/
static void build_airbase_callback(GtkAction *action, gpointer data)
{
  key_unit_airbase();
}

/****************************************************************
  Action "DO_PILLAGE" callback.
*****************************************************************/
static void do_pillage_callback(GtkAction *action, gpointer data)
{
  key_unit_pillage();
}

/****************************************************************
  Action "DIPLOMAT_ACTION" callback.
*****************************************************************/
static void diplomat_action_callback(GtkAction *action, gpointer data)
{
  key_unit_diplomat_actions();
}

/****************************************************************
  Action "EXPLODE_NUKE" callback.
*****************************************************************/
static void explode_nuke_callback(GtkAction *action, gpointer data)
{
  key_unit_nuke();
}

/****************************************************************
  Action "TAX_RATE" callback.
*****************************************************************/
static void tax_rate_callback(GtkAction *action, gpointer data)
{
  popup_rates_dialog();
}

/****************************************************************
  Action "REPORT_UNITS" callback.
*****************************************************************/
static void report_units_callback(GtkAction *action, gpointer data)
{
  popup_activeunits_report_dialog(TRUE);
}

/****************************************************************
  Action "REPORT_CITIES" callback.
*****************************************************************/
static void report_cities_callback(GtkAction *action, gpointer data)
{
  popup_city_report_dialog(TRUE);
}

/****************************************************************
  Action "REPORT_ECONOMY" callback.
*****************************************************************/
static void report_economy_callback(GtkAction *action, gpointer data)
{
  popup_economy_report_dialog(TRUE);
}

/****************************************************************
  Action "REPORT_RESEARCH" callback.
*****************************************************************/
static void report_research_callback(GtkAction *action, gpointer data)
{
  popup_science_dialog(TRUE);
}

/****************************************************************
  Action "REPORT_SPACESHIP" callback.
*****************************************************************/
static void report_spaceship_callback(GtkAction *action, gpointer data)
{
  if (NULL != client.conn.playing) {
    popup_spaceship_dialog(client.conn.playing);
  }
}

/****************************************************************
  Returns the group of the actions which are always available for
  the user in anycase.  Create it if not existent.
*****************************************************************/
static GtkActionGroup *get_safe_group(void)
{
  static GtkActionGroup *group = NULL;

  if (!group) {
    const GtkActionEntry menu_entries[] = {
      {"MENU_GAME", NULL, _("_Game"), NULL, NULL, NULL},
      {"MENU_OPTIONS", NULL, _("_Options"), NULL, NULL, NULL},
      {"MENU_EDIT", NULL, _("_Edit"), NULL, NULL, NULL},
      {"MENU_VIEW", NULL, _("_View"), NULL, NULL, NULL},
      {"MENU_IMPROVEMENTS", NULL, _("_Improvements"), NULL, NULL, NULL},
      {"MENU_CIVILIZATION", NULL, _("C_ivilization"), NULL, NULL, NULL},
      {"MENU_HELP", NULL, _("_Help"), NULL, NULL, NULL},

      /* A special case to make empty menu. */
      {"NULL", NULL, NULL, NULL, NULL, NULL}
    };

    const GtkActionEntry action_entries[] = {
      /* Game menu. */
      {"CLEAR_CHAT_LOGS", GTK_STOCK_CLEAR, _("_Clear Chat Log"),
       NULL, NULL, G_CALLBACK(clear_chat_logs_callback)},
      {"SAVE_CHAT_LOGS", GTK_STOCK_SAVE_AS, _("Save Chat _Log"),
       NULL, NULL, G_CALLBACK(save_chat_logs_callback)},

      {"LOCAL_OPTIONS", GTK_STOCK_PREFERENCES, _("_Local Client"),
       NULL, NULL, G_CALLBACK(local_options_callback)},
      {"MESSAGE_OPTIONS", GTK_STOCK_PREFERENCES, _("_Message"),
       NULL, NULL, G_CALLBACK(message_options_callback)},
      {"SERVER_OPTIONS", GTK_STOCK_PREFERENCES, _("_Remote Server"),
       NULL, NULL, G_CALLBACK(server_options_callback)},
      {"SAVE_OPTIONS", GTK_STOCK_SAVE_AS, _("Save Options _Now"),
       NULL, NULL, G_CALLBACK(save_options_callback)},

      {"RELOAD_TILESET", GTK_STOCK_REVERT_TO_SAVED, _("_Reload Tileset"),
       "<Control><Alt>r", NULL, G_CALLBACK(reload_tileset_callback)},
      {"GAME_SAVE", GTK_STOCK_SAVE, _("_Save Game"),
       NULL, NULL, G_CALLBACK(save_game_callback)},
      {"GAME_SAVE_AS", GTK_STOCK_SAVE_AS, _("Save Game _As..."),
       NULL, NULL, G_CALLBACK(save_game_as_callback)},
      {"LEAVE", NULL, _("_Leave"),
       NULL, NULL, G_CALLBACK(leave_callback)},
      {"QUIT", GTK_STOCK_QUIT, _("_Quit"),
       NULL, NULL, G_CALLBACK(quit_callback)},

      /* Edit menu. */
      {"FIND_CITY", GTK_STOCK_FIND, _("_Find City"),
       "<Control>f", NULL, G_CALLBACK(find_city_callback)},
      {"WORKLISTS", NULL, _("Work_lists"),
       "<Control>l", NULL, G_CALLBACK(worklists_callback)},

      /* Civilization menu. */
      {"MAP_VIEW", NULL, _("_View"),
       "F1", NULL, G_CALLBACK(map_view_callback)},
      {"REPORT_NATIONS", NULL, _("_Nations"),
       "F3", NULL, G_CALLBACK(report_nations_callback)},

      {"REPORT_WOW", NULL, _("_Wonders of the World"),
       "F7", NULL, G_CALLBACK(report_wow_callback)},
      {"REPORT_TOP_CITIES", NULL, _("Top _Five Cities"),
       "F8", NULL, G_CALLBACK(report_top_cities_callback)},
      {"REPORT_MESSAGES", NULL, _("_Messages"),
       "F9", NULL, G_CALLBACK(report_messages_callback)},
      {"REPORT_DEMOGRAPHIC", NULL, _("_Demographics"),
       "F11", NULL, G_CALLBACK(report_demographic_callback)},

      /* Help menu. */
      {"HELP_LANGUAGES", NULL, _("Language_s"),
       NULL, NULL, G_CALLBACK(help_language_callback)},
      {"HELP_CONNECTING", NULL, _("Co_nnecting"),
       NULL, NULL, G_CALLBACK(help_connecting_callback)},
      {"HELP_CONTROLS", NULL, _("C_ontrols"),
       NULL, NULL, G_CALLBACK(help_controls_callback)},
      {"HELP_CHATLINE", NULL, _("C_hatline"),
       NULL, NULL, G_CALLBACK(help_chatline_callback)},
      {"HELP_WORKLIST_EDITOR", NULL, _("_Worklist Editor"),
       NULL, NULL, G_CALLBACK(help_worklist_editor_callback)},
      {"HELP_CMA", NULL, _("Citizen _Management"),
       NULL, NULL, G_CALLBACK(help_cma_callback)},
      {"HELP_PLAYING", NULL, _("_Playing"),
       NULL, NULL, G_CALLBACK(help_playing_callback)},

      {"HELP_RULESET", NULL, _("About Ruleset"),
       NULL, NULL, G_CALLBACK(help_ruleset_callback)},
      {"HELP_IMPROVEMENTS", NULL, _("City _Improvements"),
       NULL, NULL, G_CALLBACK(help_improvements_callback)},
      {"HELP_UNITS", NULL, _("_Units"),
       NULL, NULL, G_CALLBACK(help_units_callback)},
      {"HELP_COMBAT", NULL, _("Com_bat"),
       NULL, NULL, G_CALLBACK(help_combat_callback)},
      {"HELP_ZOC", NULL, _("_ZOC"),
       NULL, NULL, G_CALLBACK(help_zoc_callback)},
      {"HELP_TECH", NULL, _("Techno_logy"),
       NULL, NULL, G_CALLBACK(help_tech_callback)},
      {"HELP_TERRAIN", NULL, _("_Terrain"),
       NULL, NULL, G_CALLBACK(help_terrain_callback)},
      {"HELP_WONDERS", NULL, _("Won_ders"),
       NULL, NULL, G_CALLBACK(help_wonders_callback)},
      {"HELP_GOVERNMENT", NULL, _("_Government"),
       NULL, NULL, G_CALLBACK(help_government_callback)},
      {"HELP_HAPPINESS", NULL, _("Happin_ess"),
       NULL, NULL, G_CALLBACK(help_happiness_callback)},
      {"HELP_SPACE_RACE", NULL, _("Space _Race"),
       NULL, NULL, G_CALLBACK(help_space_rate_callback)},

      {"HELP_COPYING", NULL, _("_Copying"),
       NULL, NULL, G_CALLBACK(help_copying_callback)},
      {"HELP_ABOUT", NULL, _("_About"),
       NULL, NULL, G_CALLBACK(help_about_callback)}
    };

    const GtkToggleActionEntry toggle_entries[] = {
      /* Game menu. */
      {"SAVE_OPTIONS_ON_EXIT", NULL, _("Save Options on _Exit"),
       NULL, NULL, G_CALLBACK(save_options_on_exit_callback), TRUE},

      /* Edit menu. */
      {"EDIT_MODE", GTK_STOCK_EDIT, _("_Editing Mode"),
       "<Control>e", NULL, G_CALLBACK(edit_mode_callback), FALSE},

      /* View menu. */
      {"SHOW_CITY_OUTLINES", NULL, _("Cit_y Outlines"),
       "<Control>y", NULL, G_CALLBACK(show_city_outlines_callback), FALSE},
      {"SHOW_CITY_OUTPUT", NULL, _("City Output"),
       "<Control>w", NULL, G_CALLBACK(show_city_output_callback), FALSE},
      {"SHOW_MAP_GRID", NULL, _("Map _Grid"),
       "<Control>g", NULL, G_CALLBACK(show_map_grid_callback), FALSE},
      {"SHOW_NATIONAL_BORDERS", NULL, _("National _Borders"),
       "<Control>b", NULL,
       G_CALLBACK(show_national_borders_callback), FALSE},
      {"SHOW_CITY_NAMES", NULL, _("City _Names"),
       "<Control>n", NULL, G_CALLBACK(show_city_names_callback), FALSE},
      {"SHOW_CITY_GROWTH", NULL, _("City G_rowth"),
       "<Control>r", NULL, G_CALLBACK(show_city_growth_callback), FALSE},
      {"SHOW_CITY_PRODUCTIONS", NULL, _("City _Production Levels"),
       "<Control>p", NULL,
       G_CALLBACK(show_city_productions_callback), FALSE},
      {"SHOW_CITY_BUY_COST", NULL, _("City Buy Cost"),
       NULL, NULL, G_CALLBACK(show_city_buy_cost_callback), FALSE},
      {"SHOW_CITY_TRADEROUTES", NULL, _("City Tra_deroutes"),
       "<Control>d", NULL,
       G_CALLBACK(show_city_traderoutes_callback), FALSE},

      {"SHOW_TERRAIN", NULL, _("_Terrain"),
       NULL, NULL, G_CALLBACK(show_terrain_callback), FALSE},
      {"SHOW_COASTLINE", NULL, _("C_oastline"),
       NULL, NULL, G_CALLBACK(show_coastline_callback), FALSE},

      {"SHOW_ROADS_RAILS", NULL, _("_Roads & Rails"),
       NULL, NULL, G_CALLBACK(show_road_rails_callback), FALSE},
      {"SHOW_IRRIGATION", NULL, _("_Irrigation"),
       NULL, NULL, G_CALLBACK(show_irrigation_callback), FALSE},
      {"SHOW_MINES", NULL, _("_Mines"),
       NULL, NULL, G_CALLBACK(show_mine_callback), FALSE},
      {"SHOW_FORTRESS_AIRBASE", NULL, _("_Fortress & Airbase"),
       NULL, NULL, G_CALLBACK(show_fortress_airbase_callback), FALSE},

      {"SHOW_SPECIALS", NULL, _("_Specials"),
       NULL, NULL, G_CALLBACK(show_specials_callback), FALSE},
      {"SHOW_POLLUTION", NULL, _("Po_llution & Fallout"),
       NULL, NULL, G_CALLBACK(show_pollution_callback), FALSE},
      {"SHOW_CITIES", NULL, _("Citi_es"),
       NULL, NULL, G_CALLBACK(show_cities_callback), FALSE},
      {"SHOW_UNITS", NULL, _("_Units"),
       NULL, NULL, G_CALLBACK(show_units_callback), FALSE},
      {"SHOW_FOCUS_UNIT", NULL, _("Focu_s Unit"),
       NULL, NULL, G_CALLBACK(show_focus_unit_callback), FALSE},
      {"SHOW_FOG_OF_WAR", NULL, _("Fog of _War"),
       NULL, NULL, G_CALLBACK(show_fog_of_war_callback), FALSE},

      {"FULL_SCREEN", NULL, _("_Full Screen"),
       "<Alt>Return", NULL, G_CALLBACK(full_screen_callback), FALSE}
    };

    group = gtk_action_group_new("SafeGroup");
    gtk_action_group_add_actions(group, menu_entries,
                                 G_N_ELEMENTS(menu_entries), NULL);
    gtk_action_group_add_actions(group, action_entries,
                                 G_N_ELEMENTS(action_entries), NULL);
    gtk_action_group_add_toggle_actions(group, toggle_entries,
                                        G_N_ELEMENTS(toggle_entries), NULL);
  }

  return group;
}

/****************************************************************
  Returns the group of the actions which are available only
  when the edit mode is enabled.  Create it if not existent.
*****************************************************************/
static GtkActionGroup *get_edit_group(void)
{
  static GtkActionGroup *group = NULL;

  if (!group) {
    const GtkActionEntry action_entries[] = {
      /* Edit menu. */
      {"RECALC_BORDERS", NULL, _("Recalculate _Borders"),
       NULL, NULL, G_CALLBACK(recalc_borders_callback)},
      {"TOGGLE_FOG", NULL, _("Toggle Fog of _War"),
       "<Control>m", NULL, G_CALLBACK(toggle_fog_callback)},
      {"SCENARIO_PROPERTIES", NULL, _("Game/Scenario Properties"),
       NULL, NULL, G_CALLBACK(scenario_properties_callback)},
      {"SAVE_SCENARIO", GTK_STOCK_SAVE_AS, _("Save Scenario"),
       NULL, NULL, G_CALLBACK(save_scenario_callback)}
    };

    group = gtk_action_group_new("EditGroup");
    gtk_action_group_add_actions(group, action_entries,
                                 G_N_ELEMENTS(action_entries), NULL);
  }

  return group;
}

/****************************************************************
  Returns the group of the actions which are available only
  when units are selected.  Create it if not existent.
*****************************************************************/
static GtkActionGroup *get_unit_group(void)
{
  static GtkActionGroup *group = NULL;

  if (!group) {
    const GtkActionEntry menu_entries[] = {
      {"MENU_SELECT", NULL, _("_Select"), NULL, NULL, NULL},
      {"MENU_UNIT", NULL, _("_Unit"), NULL, NULL, NULL},
      {"MENU_WORK", NULL, _("_Work"), NULL, NULL, NULL},
      {"MENU_COMBAT", NULL, _("_Combat"), NULL, NULL, NULL},
      {"MENU_BUILD_BASE", NULL, _("Build _Base"), NULL, NULL, NULL},
    };

    const GtkActionEntry action_entries[] = {
      /* Select menu. */
      {"SELECT_SINGLE", NULL, _("_Single Unit (Unselect Others)"),
       "z", NULL, G_CALLBACK(select_single_callback)},
      {"SELECT_ALL_ON_TILE", NULL, _("_All On Tile"),
       "v", NULL, G_CALLBACK(select_all_on_tile_callback)},

      {"SELECT_SAME_TYPE_TILE", NULL, _("Same Type on _Tile"),
       "<shift>v", NULL, G_CALLBACK(select_same_type_tile_callback)},
      {"SELECT_SAME_TYPE_CONT", NULL, _("Same Type on _Continent"),
       "<shift>c", NULL, G_CALLBACK(select_same_type_cont_callback)},
      {"SELECT_SAME_TYPE", NULL, _("Same Type _Everywhere"),
       "<shift>x", NULL, G_CALLBACK(select_same_type_callback)},

      {"UNIT_WAIT", NULL, _("_Wait"),
       "w", NULL, G_CALLBACK(unit_wait_callback)},
      {"UNIT_DONE", NULL, _("_Done"),
       "space", NULL, G_CALLBACK(unit_done_callback)},

      /* Unit menu. */
      {"UNIT_GOTO", NULL, _("_Go to"),
       "g", NULL, G_CALLBACK(unit_goto_callback)},
      {"UNIT_GOTO_CITY", NULL, _("Go _to/Airlift to City..."),
       "t", NULL, G_CALLBACK(unit_goto_city_callback)},
      {"UNIT_RETURN", NULL, _("_Return to Nearest City"),
       "<Shift>g", NULL, G_CALLBACK(unit_return_callback)},
      {"UNIT_EXPLORE", NULL, _("Auto E_xplore"),
       "x", NULL, G_CALLBACK(unit_explore_callback)},
      {"UNIT_PATROL", NULL, _("_Patrol"),
       "q", NULL, G_CALLBACK(unit_patrol_callback)},

      {"UNIT_SENTRY", NULL, _("_Sentry"),
       "s", NULL, G_CALLBACK(unit_sentry_callback)},
      {"UNIT_UNSENTRY", NULL, _("Uns_entry All On Tile"),
       "<Shift>s", NULL, G_CALLBACK(unit_unsentry_callback)},

      {"UNIT_LOAD", NULL, _("_Load"),
       "l", NULL, G_CALLBACK(unit_load_callback)},
      {"UNIT_UNLOAD", NULL, _("_Unload"),
       "u", NULL, G_CALLBACK(unit_unload_callback)},
      {"UNIT_UNLOAD_TRANSPORTER", NULL, _("U_nload All From Transporter"),
       "<Shift>t", NULL, G_CALLBACK(unit_unload_transporter_callback)},

      {"UNIT_HOMECITY", NULL, _("Set _Home City"),
       "h", NULL, G_CALLBACK(unit_homecity_callback)},
      {"UNIT_UPGRADE", NULL, _("Upgr_ade"),
       "<Shift>u", NULL, G_CALLBACK(unit_upgrade_callback)},
      {"UNIT_TRANSFORM", NULL, _("Trans_form"),
       "<Shift>f", NULL, G_CALLBACK(unit_transform_callback)},
      {"UNIT_DISBAND", NULL, _("_Disband"),
       "<Shift>d", NULL, G_CALLBACK(unit_disband_callback)},

      /* Work menu. */
      {"BUILD_CITY", NULL, _("_Build City"),
       "b", NULL, G_CALLBACK(build_city_callback)},
      {"GO_BUILD_CITY", NULL, _("Go _to and Build city"),
       "<Shift>b", NULL, G_CALLBACK(go_build_city_callback)},
      {"AUTO_SETTLER", NULL, _("_Auto Settler"),
       "a", NULL, G_CALLBACK(auto_settle_callback)},

      {"BUILD_ROAD", NULL, _("Build _Road"),
       "r", NULL, G_CALLBACK(build_road_callback)},
      {"BUILD_IRRIGATION", NULL, _("Build _Irrigation"),
       "i", NULL, G_CALLBACK(build_irrigation_callback)},
      {"BUILD_MINE", NULL, _("Build _Mine"),
       "m", NULL, G_CALLBACK(build_mine_callack)},

      {"CONNECT_ROAD", NULL, _("Connect With Roa_d"),
       "<Shift>r", NULL, G_CALLBACK(connect_road_callback)},
      {"CONNECT_RAIL", NULL, _("Connect With Rai_l"),
       "<Shift>l", NULL, G_CALLBACK(connect_rail_callback)},
      {"CONNECT_IRRIGATION", NULL, _("Connect With Irri_gation"),
       "<Shift>i", NULL, G_CALLBACK(connect_irrigation_callack)},

      {"TRANSFORM_TERRAIN", NULL, _("Transf_orm Terrain"),
       "o", NULL, G_CALLBACK(transform_terrain_callack)},
      {"CLEAN_POLLUTION", NULL, _("Clean _Pollution"),
       "p", NULL, G_CALLBACK(clean_pollution_callback)},
      {"CLEAN_FALLOUT", NULL, _("Clean _Nuclear Fallout"),
       "n", NULL, G_CALLBACK(clean_fallout_callback)},

      /* Combat menu. */
      {"BUILD_FORTRESS", NULL, _("Build _Fortress"),
       "f", NULL, G_CALLBACK(build_fortress_callback)},
      {"BUILD_AIRBASE", NULL, _("Build Airbas_e"),
       "e", NULL, G_CALLBACK(build_airbase_callback)},

      {"DO_PILLAGE", NULL, _("_Pillage"),
       "p", NULL, G_CALLBACK(do_pillage_callback)},
      {"DIPLOMAT_ACTION", NULL, _("_Diplomat/Spy Actions"),
       "d", NULL, G_CALLBACK(diplomat_action_callback)},
      {"EXPLODE_NUKE", NULL, _("Explode _Nuclear"),
       "<Shift>n", NULL, G_CALLBACK(explode_nuke_callback)},
    };

    group = gtk_action_group_new("UnitGroup");
    gtk_action_group_add_actions(group, menu_entries,
                                 G_N_ELEMENTS(menu_entries), NULL);
    gtk_action_group_add_actions(group, action_entries,
                                 G_N_ELEMENTS(action_entries), NULL);
  }

  return group;
}

/****************************************************************
  Returns the group of the actions which are available only
  when the user is really playing, not observing.  Create it
  if not existent.
*****************************************************************/
static GtkActionGroup *get_playing_group(void)
{
  static GtkActionGroup *group = NULL;

  if (!group) {
    const GtkActionEntry menu_entries[] = {
      {"MENU_GOVERNMENT", NULL, _("_Government"), NULL, NULL, NULL},
    };

    const GtkActionEntry action_entries[] = {
      /* Civilization menu. */
      {"TAX_RATE", NULL, _("_Tax Rates..."),
       "<Control>t", NULL, G_CALLBACK(tax_rate_callback)},
    };

    group = gtk_action_group_new("PlayingGroup");
    gtk_action_group_add_actions(group, menu_entries,
                                 G_N_ELEMENTS(menu_entries), NULL);
    gtk_action_group_add_actions(group, action_entries,
                                 G_N_ELEMENTS(action_entries), NULL);
  }

  return group;
}

/****************************************************************
  Returns the group of the actions which are available only
  when the user is attached to a particular player, playing or
  observing (but not global observing).  Create it if not existent.
*****************************************************************/
static GtkActionGroup *get_player_group(void)
{
  static GtkActionGroup *group = NULL;

  if (!group) {
    const GtkActionEntry action_entries[] = {
      /* View menu. */
      {"CENTER_VIEW", NULL, _("_Center View"), "c", NULL, NULL},

      /* Civilization menu. */
      {"REPORT_UNITS", NULL, _("_Units"),
       "F2", NULL, G_CALLBACK(report_units_callback)},
      {"REPORT_CITIES", NULL, _("_Cities"),
       "F4", NULL, G_CALLBACK(report_cities_callback)},
      {"REPORT_ECONOMY", NULL, _("_Economy"),
       "F5", NULL, G_CALLBACK(report_economy_callback)},
      {"REPORT_RESEARCH", NULL, _("_Research"),
       "F6", NULL, G_CALLBACK(report_research_callback)},

      {"REPORT_SPACESHIP", NULL, _("_Spaceship"),
       "F12", NULL, G_CALLBACK(report_spaceship_callback)}
    };

    group = gtk_action_group_new("PlayerGroup");
    gtk_action_group_add_actions(group, action_entries,
                                 G_N_ELEMENTS(action_entries), NULL);
  }

  return group;
}

/****************************************************************
  Returns the name of the file readable by the GtkUIManager.
*****************************************************************/
static const gchar *get_ui_filename(void)
{
  static char filename[256];
  char *name;

  if ((name = getenv("FREECIV_MENUS"))
      || (name = datafilename("gtk_menus.xml"))) {
    sz_strlcpy(filename, name);
  } else {
    freelog(LOG_ERROR, "Gtk menus: file definition not found");
    filename[0] = '\0';
  }

  freelog(LOG_VERBOSE, "ui menu file is \"%s\".", filename);
  return filename;
}

/****************************************************************
  Called when a main widget is added my the GtkUIManager.
*****************************************************************/
static void add_widget_callback(GtkUIManager *ui_manager, GtkWidget *widget,
                                gpointer data)
{
  gtk_box_pack_start(GTK_BOX(data), widget, TRUE, TRUE, 0);
  gtk_widget_show(widget);
}

/****************************************************************
  Creates the menu bar.
*****************************************************************/
GtkWidget *setup_menus(GtkWidget *window)
{
  GtkWidget *menubar = gtk_hbox_new(FALSE, 0);
  GError *error = NULL;

  /* Creates the UI manager. */
  ui_manager = gtk_ui_manager_new();
  gtk_ui_manager_set_add_tearoffs(ui_manager, TRUE);
  g_signal_connect(ui_manager, "add_widget",
                   G_CALLBACK(add_widget_callback), menubar);

  /* Creates the actions. */
  gtk_ui_manager_insert_action_group(ui_manager, get_safe_group(), -1);
  gtk_ui_manager_insert_action_group(ui_manager, get_edit_group(), -1);
  gtk_ui_manager_insert_action_group(ui_manager, get_unit_group(), -1);
  gtk_ui_manager_insert_action_group(ui_manager, get_playing_group(), -1);
  gtk_ui_manager_insert_action_group(ui_manager, get_player_group(), -1);

  /* Enable shortcuts. */
  gtk_window_add_accel_group(GTK_WINDOW(window),
                             gtk_ui_manager_get_accel_group(ui_manager));

  /* Load the menus. */
  if (0 == gtk_ui_manager_add_ui_from_file(ui_manager,
                                           get_ui_filename(), &error)) {
    freelog(LOG_ERROR, "Gtk menus: %s", error->message);
    g_error_free(error);
  }

#ifndef DEBUG
  /* Some actions are disabled when the client was not configured with
   * --enable-debug. */
  GtkAction *action = gtk_action_group_get_action(get_safe_group(),
                                                  "RELOAD_TILESET");

  if (action) {
    gtk_action_set_sensitive(action, FALSE);
    gtk_action_set_visible(action, FALSE);
  }
#endif /* DEBUG */

  return menubar;
}

/****************************************************************
  Sets an action active.
*****************************************************************/
static void menus_set_active(GtkActionGroup *group,
                             const gchar *action_name,
                             gboolean is_active)
{
  GtkAction *action = gtk_action_group_get_action(group, action_name);

  if (!action) {
    freelog(LOG_ERROR, "Can't set active for non-existent "
            "action \"%s\" in group \"%s\".",
            action_name, gtk_action_group_get_name(group));
    return;
  }

  if (!GTK_IS_TOGGLE_ACTION(action)) {
    freelog(LOG_ERROR, "Can't set active for non-togglable "
            "action \"%s\" in group \"%s\".",
            action_name, gtk_action_group_get_name(group));
    return;
  }

  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), is_active);
}

/****************************************************************
  Sets an action sensitive.
*****************************************************************/
static void menus_set_sensitive(GtkActionGroup *group,
                                const gchar *action_name,
                                gboolean is_sensitive)
{
  GtkAction *action = gtk_action_group_get_action(group, action_name);

  if (!action) {
    freelog(LOG_ERROR, "Can't set active for non-existent "
            "action \"%s\" in group \"%s\".",
            action_name, gtk_action_group_get_name(group));
    return;
  }

  gtk_action_set_sensitive(action, is_sensitive);
}

/****************************************************************
  Renames an action.
*****************************************************************/
static void menus_rename(GtkActionGroup *group,
                         const gchar *action_name,
                         const gchar *new_label)
{
  GtkAction *action = gtk_action_group_get_action(group, action_name);

  if (!action) {
    freelog(LOG_ERROR, "Can't rename non-existent "
            "action \"%s\" in group \"%s\".",
            action_name, gtk_action_group_get_name(group));
    return;
  }

  gtk_action_set_label(action, new_label);
}

/****************************************************************
  Find the child menu of an action.
*****************************************************************/
static GtkMenu *find_action_menu(GtkActionGroup *group,
                                 const gchar *action_name)
{
  GtkAction *action = gtk_action_group_get_action(group, action_name);
  GSList *iter;

  if (!action) {
    return NULL;
  }

  for (iter = gtk_action_get_proxies(action); iter;
       iter = g_slist_next(iter)) {
    if (GTK_IS_MENU_ITEM(iter->data)) {
      return GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(iter->data)));
    }
  }

  return NULL;
}

/****************************************************************
  The player has chosen a government from the menu.
*****************************************************************/
static void government_callback(GtkMenuItem *item, gpointer data)
{
  popup_revolution_dialog((struct government *) data);
}

/****************************************************************************
  The player has chosen a base to build from the menu.
****************************************************************************/
static void base_callback(GtkMenuItem *item, gpointer data)
{
  struct base_type *pbase = data;

  unit_list_iterate(get_units_in_focus(), punit) {
    request_new_unit_activity_base(punit, pbase);
  } unit_list_iterate_end;
}

/****************************************************************************
  Return the text for the tile, changed by the activity.

  Should only be called for irrigation, mining, or transformation, and
  only when the activity changes the base terrain type.
****************************************************************************/
static const char *get_tile_change_menu_text(struct tile *ptile,
                                             enum unit_activity activity)
{
  struct tile newtile = *ptile;

  tile_apply_activity(&newtile, activity);
  return tile_get_info_text(&newtile, 0);
}

/****************************************************************
  Updates the menus.
*****************************************************************/
void update_menus(void)
{
  GtkActionGroup *safe_group;
  GtkActionGroup *edit_group;
  GtkActionGroup *unit_group;
  GtkActionGroup *playing_group;
  GtkActionGroup *player_group;
  struct unit_list *punits = NULL;
  GtkMenu *menu;
  char irrtext[128], mintext[128], transtext[128];
  struct terrain *pterrain;

  if (!ui_manager) {
    return;
  }

  safe_group = get_safe_group();
  edit_group = get_edit_group();
  unit_group = get_unit_group();
  playing_group = get_playing_group();
  player_group = get_player_group();

  menus_set_active(safe_group, "SAVE_OPTIONS_ON_EXIT", save_options_on_exit);
  menus_set_sensitive(safe_group, "SERVER_OPTIONS", client.conn.established);

  menus_set_sensitive(safe_group, "GAME_SAVE_AS",
                      can_client_access_hack()
                      && C_S_RUNNING <= client_state());
  menus_set_sensitive(safe_group, "GAME_SAVE",
                      can_client_access_hack()
                      && C_S_RUNNING <= client_state());
  menus_set_sensitive(safe_group, "LEAVE",
                      client.conn.established);

  if (!can_client_change_view()) {
    gtk_action_group_set_sensitive(safe_group, FALSE);
    gtk_action_group_set_sensitive(edit_group, FALSE);
    gtk_action_group_set_sensitive(unit_group, FALSE);
    gtk_action_group_set_sensitive(player_group, FALSE);
    gtk_action_group_set_sensitive(playing_group, FALSE);
    return;
  }

  if (get_num_units_in_focus() > 0) {
    punits = get_units_in_focus();
  }

  if ((menu = find_action_menu(playing_group, "MENU_GOVERNMENT"))) {
    GList *list, *iter;
    GtkWidget *item, *image;
    struct sprite *gsprite;
    char buf[256];

    /* Remove previous government entries. */
    list = gtk_container_get_children(GTK_CONTAINER(menu));
    for (iter = list; iter; iter = g_list_next(iter)) {
      gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(list);

    /* Add new government entries. */
    item = gtk_menu_item_new_with_mnemonic(_("_Revolution..."));
    g_signal_connect(item, "activate",
                     G_CALLBACK(government_callback), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);

    government_iterate(g) {
      if (g != game.government_during_revolution) {
        my_snprintf(buf, sizeof(buf), _("%s..."),
                    government_name_translation(g));
        item = gtk_image_menu_item_new_with_label(buf);

        if ((gsprite = get_government_sprite(tileset, g))) {
          image = gtk_image_new_from_pixbuf(sprite_get_pixbuf(gsprite));
          gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
          gtk_widget_show(image);
        }

        g_signal_connect(item, "activate",
                         G_CALLBACK(government_callback), g);

        if (!can_change_to_government(client.conn.playing, g)) {
          gtk_widget_set_sensitive(item, FALSE);
        }

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        gtk_widget_show(item);
      }
    } government_iterate_end;
  }

  if ((menu = find_action_menu(unit_group, "MENU_BUILD_BASE"))) {
    GList *list, *iter;
    GtkWidget *item;

    /* Remove previous base entries. */
    list = gtk_container_get_children(GTK_CONTAINER(menu));
    for (iter = list; iter; iter = g_list_next(iter)) {
      gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(list);

    /* Add new base entries. */
    base_type_iterate(p) {
      if (p->buildable) {
        item = gtk_menu_item_new_with_label(base_name_translation(p));

        g_signal_connect(item, "activate", G_CALLBACK(base_callback), p);

         if (punits) {
           gtk_widget_set_sensitive(item, can_units_do_base(punits,
                                                            base_number(p)));
         }

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        gtk_widget_show(item);
      }
    } base_type_iterate_end;
  }

  gtk_action_group_set_sensitive(safe_group, TRUE);
  gtk_action_group_set_sensitive(edit_group,
                                 can_conn_enable_editing(&client.conn));
  gtk_action_group_set_sensitive(player_group, client_has_player());
  gtk_action_group_set_sensitive(playing_group, can_client_issue_orders());
  gtk_action_group_set_sensitive(unit_group,
                                 can_client_issue_orders() && punits);

  menus_set_sensitive(playing_group, "TAX_RATE",
                      game.info.changable_tax
                      && can_client_issue_orders());

  menus_set_active(safe_group, "EDIT_MODE", game.info.is_edit_mode);
  editgui_refresh();

  menus_set_active(safe_group, "SHOW_CITY_OUTLINES", draw_city_outlines);
  menus_set_active(safe_group, "SHOW_CITY_OUTPUT", draw_city_output);
  menus_set_active(safe_group, "SHOW_MAP_GRID", draw_map_grid);
  menus_set_sensitive(safe_group, "SHOW_NATIONAL_BORDERS",
                      game.info.borders > 0);
  menus_set_active(safe_group, "SHOW_NATIONAL_BORDERS", draw_borders);
  menus_set_active(safe_group, "SHOW_CITY_NAMES", draw_city_names);

  /* The "full" city bar (i.e. the new way of drawing the
   * city name), can draw the city growth even without drawing
   * the city name. But the old method cannot. */
  if (draw_full_citybar) {
    menus_set_sensitive(safe_group, "SHOW_CITY_GROWTH", TRUE);
    menus_set_sensitive(safe_group, "SHOW_CITY_TRADEROUTES", TRUE);
  } else {
    menus_set_sensitive(safe_group, "SHOW_CITY_GROWTH", draw_city_names);
    menus_set_sensitive(safe_group, "SHOW_CITY_TRADEROUTES",
                        draw_city_names);
  }

  menus_set_active(safe_group, "SHOW_CITY_GROWTH", draw_city_growth);
  menus_set_active(safe_group, "SHOW_CITY_PRODUCTIONS",
                   draw_city_productions);
  menus_set_sensitive(safe_group, "SHOW_CITY_BUY_COST",
                      draw_city_productions);
  menus_set_active(safe_group, "SHOW_CITY_BUY_COST", draw_city_buycost);
  menus_set_active(safe_group, "SHOW_CITY_TRADEROUTES",
                   draw_city_traderoutes);
  menus_set_active(safe_group, "SHOW_TERRAIN", draw_terrain);
  menus_set_sensitive(safe_group, "SHOW_COASTLINE", !draw_terrain);
  menus_set_active(safe_group, "SHOW_COASTLINE", draw_coastline);
  menus_set_active(safe_group, "SHOW_ROADS_RAILS", draw_roads_rails);
  menus_set_active(safe_group, "SHOW_IRRIGATION", draw_irrigation);
  menus_set_active(safe_group, "SHOW_MINES", draw_mines);
  menus_set_active(safe_group, "SHOW_FORTRESS_AIRBASE",
                   draw_fortress_airbase);
  menus_set_active(safe_group, "SHOW_SPECIALS", draw_specials);
  menus_set_active(safe_group, "SHOW_POLLUTION", draw_pollution);
  menus_set_active(safe_group, "SHOW_CITIES", draw_cities);
  menus_set_active(safe_group, "SHOW_UNITS", draw_units);
  menus_set_sensitive(safe_group, "SHOW_FOCUS_UNIT", !draw_units);
  menus_set_active(safe_group, "SHOW_FOCUS_UNIT", draw_focus_unit);
  menus_set_active(safe_group, "SHOW_FOG_OF_WAR", draw_fog_of_war);

  menus_set_active(safe_group, "FULL_SCREEN", fullscreen_mode);

  /* Remaining part of this function: Update Unit, Work, and Combat menus */

  if (!can_client_issue_orders() || !punits) {
    return;
  }

  /* Enable the button for adding to a city in all cases, so we
   * get an eventual error message from the server if we try. */
  menus_set_sensitive(unit_group, "BUILD_CITY",
            (can_units_do(punits, can_unit_add_or_build_city)
             || can_units_do(punits, unit_can_help_build_wonder_here)));
  menus_set_sensitive(unit_group, "GO_BUILD_CITY",
            (can_units_do(punits, can_unit_add_or_build_city)
             || can_units_do(punits, unit_can_help_build_wonder_here)));
  menus_set_sensitive(unit_group, "BUILD_ROAD",
                      (can_units_do_activity(punits, ACTIVITY_ROAD)
                       || can_units_do_activity(punits, ACTIVITY_RAILROAD)
                       || can_units_do(punits,
                                       unit_can_est_traderoute_here)));
  menus_set_sensitive(unit_group, "BUILD_IRRIGATION",
                      can_units_do_activity(punits, ACTIVITY_IRRIGATE));
  menus_set_sensitive(unit_group, "BUILD_MINE",
                      can_units_do_activity(punits, ACTIVITY_MINE));
  menus_set_sensitive(unit_group, "TRANSFORM_TERRAIN",
                      can_units_do_activity(punits, ACTIVITY_TRANSFORM));
  menus_set_sensitive(unit_group, "BUILD_FORTRESS",
                      (can_units_do_base_gui(punits, BASE_GUI_FORTRESS)
                       || can_units_do_activity(punits,
                                                ACTIVITY_FORTIFYING)));
  menus_set_sensitive(unit_group, "BUILD_AIRBASE",
                      can_units_do_base_gui(punits, BASE_GUI_AIRBASE));
  menus_set_sensitive(unit_group, "CLEAN_POLLUTION",
                      (can_units_do_activity(punits, ACTIVITY_POLLUTION)
                       || can_units_do(punits, can_unit_paradrop)));
  menus_set_sensitive(unit_group, "CLEAN_FALLOUT",
                      can_units_do_activity(punits, ACTIVITY_FALLOUT));
  menus_set_sensitive(unit_group, "UNIT_SENTRY",
                      can_units_do_activity(punits, ACTIVITY_SENTRY));
  /* FIXME: should conditionally rename "Pillage" to "Pillage..." in cases where
   * selecting the command results in a dialog box listing options of what to pillage */
  menus_set_sensitive(unit_group, "DO_PILLAGE",
                      can_units_do_activity(punits, ACTIVITY_PILLAGE));
  menus_set_sensitive(unit_group, "UNIT_DISBAND",
                      units_have_flag(punits, F_UNDISBANDABLE, FALSE));
  menus_set_sensitive(unit_group, "UNIT_UPGRADE",
                      TRUE /* FIXME: what check should we do? */);
  menus_set_sensitive(unit_group, "UNIT_HOMECITY",
                      can_units_do(punits, can_unit_change_homecity));
  menus_set_sensitive(unit_group, "UNIT_UNLOAD_TRANSPORTER",
                      units_are_occupied(punits));
  menus_set_sensitive(unit_group, "UNIT_LOAD",
                      units_can_load(punits));
  menus_set_sensitive(unit_group, "UNIT_UNLOAD",
                      units_can_unload(punits));
  menus_set_sensitive(unit_group, "UNIT_UNSENTRY", 
                      units_have_activity_on_tile(punits,
                                                  ACTIVITY_SENTRY));
  menus_set_sensitive(unit_group, "AUTO_SETTLER",
                      can_units_do(punits, can_unit_do_autosettlers));
  menus_set_sensitive(unit_group, "UNIT_EXPLORE",
                      can_units_do_activity(punits, ACTIVITY_EXPLORE));
  menus_set_sensitive(unit_group, "CONNECT_ROAD",
                      can_units_do_connect(punits, ACTIVITY_ROAD));
  menus_set_sensitive(unit_group, "CONNECT_RAIL",
                      can_units_do_connect(punits, ACTIVITY_RAILROAD));
  menus_set_sensitive(unit_group, "CONNECT_IRRIGATION",
                      can_units_do_connect(punits, ACTIVITY_IRRIGATE));
  menus_set_sensitive(unit_group, "DIPLOMAT_ACTION",
                      can_units_do_diplomat_action(punits,
                                                   DIPLOMAT_ANY_ACTION));
  menus_set_sensitive(unit_group, "EXPLODE_NUKE",
                      units_have_flag(punits, F_NUCLEAR, TRUE));

  if (units_have_flag(punits, F_HELP_WONDER, TRUE)) {
    menus_rename(unit_group, "BUILD_CITY", _("Help _Build Wonder"));
  } else {
    bool city_on_tile = FALSE;

    /* FIXME: this overloading doesn't work well with multiple focus
     * units. */
    unit_list_iterate(punits, punit) {
      if (tile_city(punit->tile)) {
        city_on_tile = TRUE;
        break;
      }
    } unit_list_iterate_end;
    
    if (city_on_tile && units_have_flag(punits, F_ADD_TO_CITY, TRUE)) {
      menus_rename(unit_group, "BUILD_CITY", _("Add to City (_B)"));
    } else {
      /* refresh default order */
      menus_rename(unit_group, "BUILD_CITY", _("_Build City"));
    }
  }

  if (units_have_flag(punits, F_TRADE_ROUTE, TRUE)) {
    menus_rename(unit_group, "BUILD_ROAD", _("Establish Trade _Route"));
  } else if (units_have_flag(punits, F_SETTLERS, TRUE)) {
    bool has_road = FALSE;

    /* FIXME: this overloading doesn't work well with multiple focus
     * units. */
    unit_list_iterate(punits, punit) {
      if (tile_has_special(punit->tile, S_ROAD)) {
        has_road = TRUE;
        break;
      }
    } unit_list_iterate_end;

    if (has_road) {
      menus_rename(unit_group, "BUILD_ROAD", _("Build _Railroad"));
    } else {
      menus_rename(unit_group, "BUILD_ROAD", _("Build _Road"));
    }
  } else {
    menus_rename(unit_group, "BUILD_ROAD", _("Build _Road"));
  }

  if (unit_list_size(punits) == 1) {
    struct unit *punit = unit_list_get(punits, 0);

    pterrain = tile_terrain(punit->tile);
    if (pterrain->irrigation_result != T_NONE
        && pterrain->irrigation_result != pterrain) {
      my_snprintf(irrtext, sizeof(irrtext), _("Change to %s (_I)"),
                  get_tile_change_menu_text(punit->tile,
                                            ACTIVITY_IRRIGATE));
    } else if (tile_has_special(punit->tile, S_IRRIGATION)
               && player_knows_techs_with_flag(unit_owner(punit),
                                               TF_FARMLAND)) {
      sz_strlcpy(irrtext, _("Bu_ild Farmland"));
    } else {
      sz_strlcpy(irrtext, _("Build _Irrigation"));
    }

    if (pterrain->mining_result != T_NONE
        && pterrain->mining_result != pterrain) {
      my_snprintf(mintext, sizeof(mintext), _("Change to %s (_M)"),
                  get_tile_change_menu_text(punit->tile, ACTIVITY_MINE));
    } else {
      sz_strlcpy(mintext, _("Build _Mine"));
    }

    if (pterrain->transform_result != T_NONE
        && pterrain->transform_result != pterrain) {
      my_snprintf(transtext, sizeof(transtext), _("Transf_orm to %s"),
                  get_tile_change_menu_text(punit->tile,
                                            ACTIVITY_TRANSFORM));
    } else {
      sz_strlcpy(transtext, _("Transf_orm Terrain"));
    }
  } else {
    sz_strlcpy(irrtext, _("Build _Irrigation"));
    sz_strlcpy(mintext, _("Build _Mine"));
    sz_strlcpy(transtext, _("Transf_orm Terrain"));
  }

  menus_rename(unit_group, "BUILD_IRRIGATION", irrtext);
  menus_rename(unit_group, "BUILD_MINE", mintext);
  menus_rename(unit_group, "TRANSFORM_TERRAIN", transtext);

  if (can_units_do_activity(punits, ACTIVITY_FORTIFYING)) {
    menus_rename(unit_group, "BUILD_FORTRESS", _("_Fortify Unit"));
  } else {
    menus_rename(unit_group, "BUILD_FORTRESS", _("Build _Fortress"));
  }

  if (units_have_flag(punits, F_PARATROOPERS, TRUE)) {
    menus_rename(unit_group, "CLEAN_POLLUTION", _("Drop _Paratrooper"));
  } else {
    menus_rename(unit_group, "CLEAN_POLLUTION", _("Clean _Pollution"));
  }
}
