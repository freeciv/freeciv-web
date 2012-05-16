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

#include <assert.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "ioz.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* common */
#include "events.h"
#include "version.h"

/* agents */
#include "cma_fec.h"

/* include */
#include "chatline_g.h"
#include "dialogs_g.h"
#include "gui_main_g.h"
#include "menu_g.h"

/* client */
#include "audio.h"
#include "cityrepdata.h"
#include "client_main.h"
#include "mapview_common.h"
#include "overview_common.h"
#include "plrdlg_common.h"
#include "repodlgs_common.h"
#include "servers.h"
#include "themes_common.h"
#include "tilespec.h"

#include "options.h"

/* Iteration loop, including invalid options for the current gui type. */
#define client_options_iterate_all(_p)                                      \
{                                                                           \
  struct client_option *_p = client_option_array_first();                   \
  for (; _p <= client_option_array_last(); _p++) {                          \

#define client_options_iterate_all_end                                      \
  }                                                                         \
}


/****************************************************************
 The "options" file handles actual "options", and also view options,
 message options, dialog/report settings, cma settings, server settings,
 and global worklists.
*****************************************************************/

/** Defaults for options normally on command line **/

char default_user_name[512] = "\0";
char default_server_host[512] = "localhost";
int  default_server_port = DEFAULT_SOCK_PORT;
char default_metaserver[512] = META_URL;
char default_theme_name[512] = "human";
char default_tileset_name[512] = "\0";
char default_sound_set_name[512] = "stdsounds";
char default_sound_plugin_name[512] = "\0";

bool save_options_on_exit = TRUE;
bool fullscreen_mode = FALSE;

/** Local Options: **/

bool solid_color_behind_units = FALSE;
bool sound_bell_at_new_turn = FALSE;
int  smooth_move_unit_msec = 30;
int smooth_center_slide_msec = 200;
bool do_combat_animation = TRUE;
bool ai_manual_turn_done = TRUE;
bool auto_center_on_unit = TRUE;
bool auto_center_on_combat = FALSE;
bool auto_center_each_turn = TRUE;
bool wakeup_focus = TRUE;
bool goto_into_unknown = TRUE;
bool center_when_popup_city = TRUE;
bool concise_city_production = FALSE;
bool auto_turn_done = FALSE;
bool meta_accelerators = TRUE;
bool ask_city_name = TRUE;
bool popup_new_cities = TRUE;
bool popup_caravan_arrival = TRUE;
bool keyboardless_goto = TRUE;
bool enable_cursor_changes = TRUE;
bool separate_unit_selection = FALSE;
bool unit_selection_clears_orders = TRUE;
char highlight_our_names[128] = "yellow";

/* This option is currently set by the client - not by the user. */
bool update_city_text_in_refresh_tile = TRUE;

bool draw_city_outlines = TRUE;
bool draw_city_output = FALSE;
bool draw_map_grid = FALSE;
bool draw_city_names = TRUE;
bool draw_city_growth = TRUE;
bool draw_city_productions = FALSE;
bool draw_city_buycost = FALSE;
bool draw_city_traderoutes = FALSE;
bool draw_terrain = TRUE;
bool draw_coastline = FALSE;
bool draw_roads_rails = TRUE;
bool draw_irrigation = TRUE;
bool draw_mines = TRUE;
bool draw_fortress_airbase = TRUE;
bool draw_specials = TRUE;
bool draw_pollution = TRUE;
bool draw_cities = TRUE;
bool draw_units = TRUE;
bool draw_focus_unit = FALSE;
bool draw_fog_of_war = TRUE;
bool draw_borders = TRUE;
bool draw_full_citybar = TRUE;
bool draw_unit_shields = TRUE;
bool player_dlg_show_dead_players = TRUE;
bool reqtree_show_icons = TRUE;
bool reqtree_curved_lines = FALSE;

/* gui-gtk-2.0 client specific options. */
bool gui_gtk2_map_scrollbars = FALSE;
bool gui_gtk2_dialogs_on_top = TRUE;
bool gui_gtk2_show_task_icons = TRUE;
bool gui_gtk2_enable_tabs = TRUE;
bool gui_gtk2_better_fog = TRUE;
bool gui_gtk2_show_chat_message_time = FALSE;
bool gui_gtk2_split_bottom_notebook = FALSE;
bool gui_gtk2_new_messages_go_to_top = FALSE;
bool gui_gtk2_show_message_window_buttons = TRUE;
bool gui_gtk2_metaserver_tab_first = FALSE;
bool gui_gtk2_allied_chat_only = FALSE;
bool gui_gtk2_small_display_layout = FALSE;
char gui_gtk2_font_city_label[512] = "Monospace 8";
char gui_gtk2_font_notify_label[512] = "Monospace Bold 9";
char gui_gtk2_font_spaceship_label[512] = "Monospace 8";
char gui_gtk2_font_help_label[512] = "Sans Bold 10";
char gui_gtk2_font_help_link[512] = "Sans 9";
char gui_gtk2_font_help_text[512] = "Monospace 8";
char gui_gtk2_font_chatline[512] = "Monospace 8";
char gui_gtk2_font_beta_label[512] = "Sans Italic 10";
char gui_gtk2_font_small[512] = "Sans 9";
char gui_gtk2_font_comment_label[512] = "Sans Italic 9";
char gui_gtk2_font_city_names[512] = "Sans Bold 10";
char gui_gtk2_font_city_productions[512] = "Serif 10";

/* gui-sdl client specific options. */
bool gui_sdl_fullscreen = FALSE;
int gui_sdl_screen_width = 640;
int gui_sdl_screen_height = 480;

/* gui-win32 client specific options. */
bool gui_win32_better_fog = TRUE;
bool gui_win32_enable_alpha = TRUE;

static void reqtree_show_icons_callback(struct client_option *option);
static void view_option_changed_callback(struct client_option *option);

const char *client_option_class_names[COC_MAX] = {
  N_("Graphics"),
  N_("Overview"),
  N_("Sound"),
  N_("Interface"),
  N_("Network"),
  N_("Font")
};

static struct client_option options[] = {
  GEN_STR_OPTION(default_user_name,
		 N_("Login name"),
		 N_("This is the default login username that will be used "
		    "in the connection dialogs or with the -a command-line "
		    "parameter."),
		 COC_NETWORK, GUI_LAST, NULL, NULL),
  GEN_STR_OPTION(default_server_host,
		 N_("Server"),
		 N_("This is the default server hostname that will be used "
		    "in the connection dialogs or with the -a command-line "
		    "parameter."),
		 COC_NETWORK, GUI_LAST, "localhost", NULL),
  GEN_INT_OPTION(default_server_port,
		 N_("Server port"),
		 N_("This is the default server port that will be used "
		    "in the connection dialogs or with the -a command-line "
		    "parameter."),
		 COC_NETWORK, GUI_LAST, DEFAULT_SOCK_PORT, 0, 65535, NULL),
  GEN_STR_OPTION(default_metaserver,
		 N_("Metaserver"),
		 N_("The metaserver is a host that the client contacts to "
		    "find out about games on the internet.  Don't change "
		    "this from its default value unless you know what "
		    "you're doing."),
		 COC_NETWORK, GUI_LAST, META_URL, NULL),
  GEN_STR_LIST_OPTION(default_sound_set_name,
                      N_("Soundset"),
                      N_("This is the soundset that will be used.  Changing "
                         "this is the same as using the -S command-line "
                         "parameter."),
                      COC_SOUND, GUI_LAST, "stdsounds", get_soundset_list, NULL),
  GEN_STR_LIST_OPTION(default_sound_plugin_name,
                      N_("Sound plugin"),
                      N_("If you have a problem with sound, try changing "
                         "the sound plugin.  The new plugin won't take "
                         "effect until you restart Freeciv.  Changing this "
                         "is the same as using the -P command-line option."),
                      COC_SOUND, GUI_LAST, NULL, get_soundplugin_list, NULL),
  GEN_STR_LIST_OPTION(default_theme_name, N_("Theme"),
                      N_("By changing this option you change the "
                         "active theme."),
                      COC_GRAPHICS, GUI_LAST, NULL,
                      get_themes_list, theme_reread_callback),
  GEN_STR_LIST_OPTION(default_tileset_name, N_("Tileset"),
                      N_("By changing this option you change the active "
                         "tileset.  This is the same as using the -t "
                         "command-line parameter."),
                      COC_GRAPHICS, GUI_LAST, NULL,
                      get_tileset_list, tilespec_reread_callback),

  GEN_BOOL_OPTION(solid_color_behind_units,
                  N_("Solid unit background color"),
                  N_("Setting this option will cause units on the map "
                     "view to be drawn with a solid background color "
                     "instead of the flag backdrop."),
                  COC_GRAPHICS, GUI_LAST, FALSE, mapview_redraw_callback),
  GEN_BOOL_OPTION(draw_city_outlines, N_("Draw city outlines"),
                  N_("Setting this option will draw a line at the city "
                     "workable limit."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_output, N_("Draw city output"),
                  N_("Setting this option will draw city output for every "
                     "citizen."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_map_grid, N_("Draw the map grid"),
                  N_("Setting this option will draw a grid over the map."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_names, N_("Draw the city names"),
                  N_("Setting this option will draw the names of the cities"
                     "on the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_growth, N_("Draw the city growthes"),
                  N_("Setting this option will draw in how any turns the "
                     "cities will grow or shrink."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_productions, N_("Draw the city productions"),
                  N_("Setting this option will draw what the cities are "
                     "currently building on the map."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_buycost, N_("Draw the city buy costs"),
                  N_("Setting this option will draw how many golds are "
                     "needed to buy the production of the cities."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_traderoutes, N_("Draw the city traderoutes"),
                  N_("Setting this option will draw traderoutes lines "
                     "between cities which have traderoutes."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_terrain, N_("Draw the terrain"),
                  N_("Setting this option will draw the terrain."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_coastline, N_("Draw the coast line"),
                  N_("Setting this option will draw a line to separate the "
                     "land of the ocean."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_roads_rails, N_("Draw the roads and the railroads"),
                  N_("Setting this option will draw the roads and the "
                     "railroads on  the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_irrigation, N_("Draw the irrigations"),
                  N_("Setting this option will draw the irrigations "
                     "on the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_mines, N_("Draw the mines"),
                  N_("Setting this option will draw the mines on the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_fortress_airbase, N_("Draw the bases"),
                  N_("Setting this option will draw the bases on the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_specials, N_("Draw the specials"),
                  N_("Setting this option will draw the specials on the "
                     "map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_pollution, N_("Draw the pollution/nuclear fallouts"),
                  N_("Setting this option will draw the pollution and the "
                     "nuclear fallouts on the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_cities, N_("Draw the cities"),
                  N_("Setting this option will draw the cities on the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_units, N_("Draw the units"),
                  N_("Setting this option will draw the units on the map."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_focus_unit, N_("Draw the units in focus"),
                  N_("Setting this option will draw the units in focus, "
                     "including the case the other units wouldn't be "
                     "drawn."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_fog_of_war, N_("Draw the fog of war"),
                  N_("Setting this option will draw the fog of war."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_borders, N_("Draw the borders"),
                  N_("Setting this option will draw the nationnal borders."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(player_dlg_show_dead_players,
                  N_("Show dead players in nation report."),
                  N_("Setting this option will draw the players already "
                     "dead in the nation report page."),
                  COC_GRAPHICS, GUI_LAST, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(sound_bell_at_new_turn, N_("Sound bell at new turn"),
		  N_("Set this option to have a \"bell\" event be generated "
		     "at the start of a new turn.  You can control the "
		     "behavior of the \"bell\" event by editing the message "
		     "options."),
		  COC_SOUND, GUI_LAST, FALSE, NULL),
  GEN_INT_OPTION(smooth_move_unit_msec,
		 N_("Unit movement animation time (milliseconds)"),
		 N_("This option controls how long unit \"animation\" takes "
		    "when a unit moves on the map view.  Set it to 0 to "
		    "disable animation entirely."),
		 COC_GRAPHICS, GUI_LAST, 30, 0, 2000, NULL),
  GEN_INT_OPTION(smooth_center_slide_msec,
		 N_("Mapview recentering time (milliseconds)"),
		 N_("When the map view is recentered, it will slide "
		    "smoothly over the map to its new position.  This "
		    "option controls how long this slide lasts.  Set it to "
		    "0 to disable mapview sliding entirely."),
		 COC_GRAPHICS, GUI_LAST, 200, 0, 5000, NULL),
  GEN_BOOL_OPTION(do_combat_animation, N_("Show combat animation"),
		  N_("Disabling this option will turn off combat animation "
		     "between units on the mapview."),
		  COC_GRAPHICS, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(draw_full_citybar, N_("Draw the citybar"),
                  N_("Setting this option will display a 'citybar' "
                     "containing useful information beneath each city. "
                     "Disabling this option will display only the city's "
                     "name and optionally, production."),
                  COC_GRAPHICS, GUI_LAST, TRUE, view_option_changed_callback),
  GEN_BOOL_OPTION(reqtree_show_icons,
                  N_("Show icons in the technology tree"),
                  N_("Setting this option will display icons "
                     "on the technology tree diagram. Turning "
                     "this option off makes the technology tree "
                     "more compact."),
                  COC_GRAPHICS, GUI_LAST, TRUE, reqtree_show_icons_callback),
  GEN_BOOL_OPTION(reqtree_curved_lines,
                  N_("Use curved lines in the technology tree"),
                  N_("Setting this option make the technology tree "
                     "diagram use curved lines to show technology "
                     "relations. Turning this option off causes "
                     "the lines to be drawn straight."),
                  COC_GRAPHICS, GUI_LAST, FALSE,
                  reqtree_show_icons_callback),
  GEN_BOOL_OPTION(draw_unit_shields, N_("Draw shield graphics for units"),
                  N_("Setting this option will draw a shield icon "
                     "as the flags on units.  If unset, the full flag will "
                     "be drawn."),
                  COC_GRAPHICS, GUI_LAST, TRUE, mapview_redraw_callback),
   GEN_STR_OPTION(highlight_our_names,
                  N_("Color to highlight your player/user name"),
                  N_("If set, your player and user name in the new chat "
                     "messages will be highlighted using this color as "
                     "background.  If not set, it will just not highlight "
                     "anything."),
                  COC_GRAPHICS, GUI_LAST, "yellow", NULL),
  GEN_BOOL_OPTION(ai_manual_turn_done, N_("Manual Turn Done in AI Mode"),
		  N_("Disable this option if you do not want to "
		     "press the Turn Done button manually when watching "
		     "an AI player."),
		  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(auto_center_on_unit, N_("Auto Center on Units"),
		  N_("Set this option to have the active unit centered "
		     "automatically when the unit focus changes."),
		  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(auto_center_on_combat, N_("Auto Center on Combat"),
		  N_("Set this option to have any combat be centered "
		     "automatically.  Disabled this will speed up the time "
		     "between turns but may cause you to miss combat "
		     "entirely."),
		  COC_INTERFACE, GUI_LAST, FALSE, NULL),
  GEN_BOOL_OPTION(auto_center_each_turn, N_("Auto Center on New Turn"),
                  N_("Set this option to have the client automatically "
                     "recenter the map on a suitable location at the "
                     "start of each turn."),
                  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(wakeup_focus, N_("Focus on Awakened Units"),
		  N_("Set this option to have newly awoken units be "
		     "focused automatically."),
		  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(keyboardless_goto, N_("Keyboardless goto"),
                  N_("If this option is set then a goto may be initiated "
                     "by left-clicking and then holding down the mouse "
                     "button while dragging the mouse onto a different "
                     "tile."),
                  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(goto_into_unknown, N_("Allow goto into the unknown"),
		  N_("Setting this option will make the game consider "
		     "moving into unknown tiles.  If not, then goto routes "
		     "will detour around or be blocked by unknown tiles."),
		  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(center_when_popup_city, N_("Center map when Popup city"),
		  N_("Setting this option makes the mapview center on a "
		     "city when its city dialog is popped up."),
		  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(concise_city_production, N_("Concise City Production"),
		  N_("Set this option to make the city production (as shown "
		     "in the city dialog) to be more compact."),
		  COC_INTERFACE, GUI_LAST, FALSE, NULL),
  GEN_BOOL_OPTION(auto_turn_done, N_("End Turn when done moving"),
		  N_("Setting this option makes your turn end automatically "
		     "when all your units are done moving."),
		  COC_INTERFACE, GUI_LAST, FALSE, NULL),
  GEN_BOOL_OPTION(ask_city_name, N_("Prompt for city names"),
		  N_("Disabling this option will make the names of newly "
		     "founded cities chosen automatically by the server."),
		  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(popup_new_cities, N_("Pop up city dialog for new cities"),
		  N_("Setting this option will pop up a newly-founded "
		     "city's city dialog automatically."),
		  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(popup_caravan_arrival, N_("Pop up caravan actions"),
                  N_("If this option is enabled, when caravans arrive "
                     "at a city where they can establish a traderoute "
                     "or help build a wonder, a window will popup asking "
                     "which action should be performed. Disabling this "
                     "option means you will have to do the action "
                     "manually by pressing either 'r' (for a traderoute) "
                     "or 'b' (for building a wonder) when the caravan "
                     "is in the city."),
                  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(enable_cursor_changes, N_("Enable cursor changing"),
                  N_("This option controls whether the client should "
                     "try to change the mouse cursor depending on what "
                     "is being pointed at, as well as to indicate "
                     "changes in the client or server state."),
                  COC_INTERFACE, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(separate_unit_selection, N_("Select cities before units"),
                  N_("If this option is enabled, when both cities and "
                     "units are present in the selection rectangle, only "
                     "cities will be selected."),
                  COC_INTERFACE, GUI_LAST, FALSE, NULL),
  GEN_BOOL_OPTION(unit_selection_clears_orders,
                  N_("Clear unit orders on selection"),
                  N_("Enabling this option will cause unit orders to be "
                     "cleared whenever one or more units are selected. If "
                     "this option is disabled, selecting units will not "
                     "cause them to stop their current activity. Instead, "
                     "their orders will be cleared only when new orders "
                     "are given or if you press <space>."),
                  COC_INTERFACE, GUI_LAST, TRUE, NULL),

  GEN_BOOL_OPTION(overview.layers[OLAYER_BACKGROUND],
		  N_("Background layer"),
		  N_("The background layer of the overview shows just "
		     "ocean and land."),
                  COC_OVERVIEW, GUI_LAST, TRUE, NULL),
  GEN_BOOL_OPTION(overview.layers[OLAYER_RELIEF],
                  N_("Terrain relief map layer"),
                  N_("The relief layer shows all terrains on the map."),
                  COC_OVERVIEW, GUI_LAST, FALSE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_BORDERS],
                  N_("Borders layer"),
                  N_("The borders layer of the overview shows which tiles "
                     "are owned by each player."),
                  COC_OVERVIEW, GUI_LAST, FALSE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_BORDERS_ON_OCEAN],
                  N_("Borders layer on ocean tiles"),
                  N_("The borders layer of the overview are drawn on "
                     "ocean tiles as well (this may look ugly with many "
                     "islands). This option is only of interest if you "
                     "have set the option \"Borders layer\" already."),
                  COC_OVERVIEW, GUI_LAST, TRUE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_UNITS],
                  N_("Units layer"),
                  N_("Enabling this will draw units on the overview."),
                  COC_OVERVIEW, GUI_LAST, TRUE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_CITIES],
                  N_("Cities layer"),
                  N_("Enabling this will draw cities on the overview."),
                  COC_OVERVIEW, GUI_LAST, TRUE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.fog,
                  N_("Overview fog of war"),
                  N_("Enabling this will show fog of war on the "
                     "overview."),
                  COC_OVERVIEW, GUI_LAST, TRUE, overview_redraw_callback),

  /* gui-gtk-2.0 client specific options. */
  GEN_BOOL_OPTION(gui_gtk2_map_scrollbars, N_("Show Map Scrollbars"),
                  N_("Disable this option to hide the scrollbars on the "
                     "map view."),
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_dialogs_on_top, N_("Keep dialogs on top"),
                  N_("If this option is set then dialog windows will always "
                     "remain in front of the main Freeciv window. "
                     "Disabling this has no effect in fullscreen mode."),
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_show_task_icons, N_("Show worklist task icons"),
                  N_("Disabling this will turn off the unit and building "
                     "icons in the worklist dialog and the production "
                     "tab of the city dialog."),
                  COC_GRAPHICS, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_enable_tabs, N_("Enable status report tabs"),
                  N_("If this option is enabled then report dialogs will "
                     "be shown as separate tabs rather than in popup "
                     "dialogs."),
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_better_fog,
                  N_("Better fog-of-war drawing"),
                  N_("If this is enabled then a better method is used "
                     "for drawing fog-of-war.  It is not any slower but "
                     "will consume about twice as much memory."),
                  COC_GRAPHICS, GUI_GTK2, TRUE, mapview_redraw_callback),
  GEN_BOOL_OPTION(gui_gtk2_show_chat_message_time,
                  N_("Show time for each chat message"),
                  N_("If this option is enabled then all chat messages "
                     "will be prefixed by a time string of the form "
                     "[hour:minute:second]."),
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_split_bottom_notebook,
                  N_("Split bottom notebook area"),
                  N_("Enabling this option will split the bottom "
                     "notebook into a left and right notebook so that "
                     "two tabs may be viewed at once."),
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_new_messages_go_to_top,
                  N_("New message events go to top of list"),
                  N_("If this option is enabled, new events in the "
                     "message window will appear at the top of the list, "
                     "rather than being appended at the bottom."),
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_show_message_window_buttons,
                  N_("Show extra message window buttons"),
                  N_("If this option is enabled, there will be two "
                     "buttons displayed in the message window for "
                     "inspecting a city and going to a location. If this "
                     "option is disabled, these buttons will not appear "
                     "(you can still double-click with the left mouse "
                     "button or right-click on a row to inspect or goto "
                     "respectively). This option will only take effect "
                     "once the message window is closed and reopened."),
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_metaserver_tab_first,
                  N_("Metaserver tab first in network page"),
                  N_("If this option is enabled, the metaserver tab will "
                     "be the first notebook tab in the network page. This "
                     "option requires a restart in order to take effect."),
                  COC_NETWORK, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_allied_chat_only,
                  N_("Plain chat messages are sent to allies only"),
                  N_("If this option is enabled, then plain messages "
                     "typed into the chat entry while the game is "
                     "running will only be sent to your allies. "
                     "Otherwise plain messages will be sent as "
                     "public chat messages. To send a public chat "
                     "message with this option enabled, prefix the "
                     "message with a single colon ':'. This option "
                     "can also be set using a toggle button beside "
                     "the chat entry (only visible in multiplayer "
                     "games)."),
                  COC_NETWORK, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_small_display_layout,
                  N_("Arrange widgets for small displays"),
                  N_("If this option is enabled, widgets in the main "
                     "window will be arrange so that they take up the "
                     "least amount of total screen space. Specifically, "
                     "the left panel containing the overview, player "
                     "status, and the unit information box will be "
                     "extended over the entire left side of the window. "
                     "This option requires a restart in order to take "
                     "effect."), COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_FONT_OPTION(gui_gtk2_font_city_label,
                  N_("City Label"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Monospace 8", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_notify_label,
                  N_("Notify Label"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Monospace Bold 9", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_spaceship_label,
                  N_("Spaceship Label"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Monospace 8", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_help_label,
                  N_("Help Label"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Sans Bold 10", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_help_link,
                  N_("Help Link"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Sans 9", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_help_text,
                  N_("Help Text"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Monospace 8", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_chatline,
                  N_("Chatline Area"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Monospace 8", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_beta_label,
                  N_("Beta Label"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Sans Italic 10", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_small,
                  N_("Small Font"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Sans 9", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_comment_label,
                  N_("Comment Label"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Sans Italic 9", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_city_names,
                  N_("City Names"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Sans Bold 10", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_city_productions,
                  N_("City Productions"),
                  N_("FIXME"),
                  COC_FONT, GUI_GTK2, "Serif 10", NULL),

  /* gui-sdl client specific options. */
  GEN_BOOL_OPTION(gui_sdl_fullscreen, N_("Full Screen"), 
                  N_("If this option is set the client will use the "
                     "whole screen area for drawing"),
                  COC_INTERFACE, GUI_SDL, FALSE, NULL),
  GEN_INT_OPTION(gui_sdl_screen_width, N_("Screen width"),
                 N_("This option saves the width of the selected screen "
                    "resolution"),
                 COC_INTERFACE, GUI_SDL, 640, 320, 1280, NULL),
  GEN_INT_OPTION(gui_sdl_screen_height, N_("Screen height"),
                 N_("This option saves the height of the selected screen "
                    "resolution"),
                 COC_INTERFACE, GUI_SDL, 480, 240, 960, NULL),

  /* gui-win32 client specific options. */
  GEN_BOOL_OPTION(gui_win32_better_fog,
                  N_("Better fog-of-war drawing"),
                  N_("If this is enabled then a better method is used for "
                     "drawing fog-of-war.  It is not any slower but will "
                     "consume about twice as much memory."),
                  COC_GRAPHICS, GUI_WIN32, TRUE, mapview_redraw_callback),
  GEN_BOOL_OPTION(gui_win32_enable_alpha,
                  N_("Enable alpha blending"),
                  N_("If this is enabled, then alpha blending will be "
                     "used in rendering, instead of an ordered dither.  "
                     "If there is no hardware support for alpha "
                     "blending, this is much slower."),
                  COC_GRAPHICS, GUI_WIN32, TRUE, mapview_redraw_callback)
};
static const int num_options = ARRAY_SIZE(options);

#undef GEN_BOOL_OPTION
#undef GEN_INT_OPTION
#undef GEN_STR_OPTION
#undef GEN_FONT_OPTION
#undef GEN_STR_LIST_OPTION


/** Message Options: **/

unsigned int messages_where[E_LAST];


/**************************************************************************
  Return the first item of options.
**************************************************************************/
struct client_option *client_option_array_first(void)
{
  return options;
}

/**************************************************************************
  Return the last item of options.
**************************************************************************/
const struct client_option *client_option_array_last(void)
{
  return options + num_options - 1;
}

/****************************************************************
  These could be a static table initialisation, except
  its easier to do it this way.
*****************************************************************/
void message_options_init(void)
{
  int none[] = {
    E_IMP_BUY, E_IMP_SOLD, E_UNIT_BUY,
    E_UNIT_LOST_ATT, E_UNIT_WIN_ATT, E_GAME_START,
    E_NATION_SELECTED, E_CITY_BUILD, E_NEXT_YEAR,
    E_CITY_PRODUCTION_CHANGED,
    E_CITY_MAY_SOON_GROW, E_WORKLIST, E_AI_DEBUG
  };
  int out_only[] = {
    E_CHAT_MSG, E_CHAT_ERROR, E_CONNECTION, E_LOG_ERROR, E_SETTING,
    E_VOTE_NEW, E_VOTE_RESOLVED, E_VOTE_ABORTED
  };
  int all[] = {
    E_LOG_FATAL, E_SCRIPT
  };
  int i;

  for (i = 0; i < E_LAST; i++) {
    messages_where[i] = MW_MESSAGES;
  }
  for (i = 0; i < ARRAY_SIZE(none); i++) {
    messages_where[none[i]] = 0;
  }
  for (i = 0; i < ARRAY_SIZE(out_only); i++) {
    messages_where[out_only[i]] = MW_OUTPUT;
  }
  for (i = 0; i < ARRAY_SIZE(all); i++) {
    messages_where[all[i]] = MW_MESSAGES | MW_POPUP;
  }

  events_init();
}

/****************************************************************
... 
*****************************************************************/
void message_options_free(void)
{
  events_free();
}

/****************************************************************
... 
*****************************************************************/
static void message_options_load(struct section_file *file, const char *prefix)
{
  int i;

  for (i = 0; i < E_LAST; i++) {
    messages_where[i] =
      secfile_lookup_int_default(file, messages_where[i],
				 "%s.message_where_%02d", prefix, i);
  }
}

/****************************************************************
... 
*****************************************************************/
static void message_options_save(struct section_file *file, const char *prefix)
{
  int i;

  for (i = 0; i < E_LAST; i++) {
    secfile_insert_int_comment(file, messages_where[i],
			       get_event_message_text(i),
			       "%s.message_where_%02d", prefix, i);
  }
}


/****************************************************************
 Does heavy lifting for looking up a preset.
*****************************************************************/
static void load_cma_preset(struct section_file *file, int i)
{
  struct cm_parameter parameter;
  const char *name =
    secfile_lookup_str_default(file, "preset",
                               "cma.preset%d.name", i);

  output_type_iterate(o) {
    parameter.minimal_surplus[o] =
        secfile_lookup_int_default(file, 0, "cma.preset%d.minsurp%d", i, o);
    parameter.factor[o] =
        secfile_lookup_int_default(file, 0, "cma.preset%d.factor%d", i, o);
  } output_type_iterate_end;
  parameter.require_happy =
      secfile_lookup_bool_default(file, FALSE, "cma.preset%d.reqhappy", i);
  parameter.happy_factor =
      secfile_lookup_int_default(file, 0, "cma.preset%d.happyfactor", i);
  parameter.allow_disorder = FALSE;
  parameter.allow_specialists = TRUE;

  cmafec_preset_add(name, &parameter);
}

/****************************************************************
 Does heavy lifting for inserting a preset.
*****************************************************************/
static void save_cma_preset(struct section_file *file, int i)
{
  const struct cm_parameter *const pparam = cmafec_preset_get_parameter(i);
  char *name = cmafec_preset_get_descr(i);

  secfile_insert_str(file, name, "cma.preset%d.name", i);

  output_type_iterate(o) {
    secfile_insert_int(file, pparam->minimal_surplus[o],
                       "cma.preset%d.minsurp%d", i, o);
    secfile_insert_int(file, pparam->factor[o],
                       "cma.preset%d.factor%d", i, o);
  } output_type_iterate_end;
  secfile_insert_bool(file, pparam->require_happy,
                      "cma.preset%d.reqhappy", i);
  secfile_insert_int(file, pparam->happy_factor,
                     "cma.preset%d.happyfactor", i);
}

/****************************************************************
 Insert all cma presets.
*****************************************************************/
static void save_cma_presets(struct section_file *file)
{
  int i;

  secfile_insert_int_comment(file, cmafec_preset_num(),
                             _("If you add a preset by hand,"
                               " also update \"number_of_presets\""),
                             "cma.number_of_presets");
  for (i = 0; i < cmafec_preset_num(); i++) {
    save_cma_preset(file, i);
  }
}


/****************************************************************
 Returns pointer to static memory containing name of option file.
 Ie, based on FREECIV_OPT env var, and home dir. (or a
 OPTION_FILE_NAME define defined in config.h)
 Or NULL if problem.
*****************************************************************/
static char *option_file_name(void)
{
  static char name_buffer[256];
  char *name;

  name = getenv("FREECIV_OPT");

  if (name) {
    sz_strlcpy(name_buffer, name);
  } else {
#ifndef OPTION_FILE_NAME
    name = user_home_dir();
    if (!name) {
      freelog(LOG_ERROR, _("Cannot find your home directory"));
      return NULL;
    }
    mystrlcpy(name_buffer, name, 231);
    sz_strlcat(name_buffer, "/.civclientrc");
#else
    mystrlcpy(name_buffer,OPTION_FILE_NAME,sizeof(name_buffer));
#endif
  }
  freelog(LOG_VERBOSE, "settings file is %s", name_buffer);
  return name_buffer;
}


/****************************************************************
 Load settable per connection/server options.
*****************************************************************/
void load_settable_options(bool send_it)
{
  char buffer[MAX_LEN_MSG];
  struct section_file sf;
  char *name;
  char *desired_string;
  int i = 0;

  name = option_file_name();
  if (!name) {
    /* fail silently */
    return;
  }
  if (!section_file_load(&sf, name))
    return;

  for (; i < num_settable_options; i++) {
    struct options_settable *o = &settable_options[i];
    bool changed = FALSE;

    my_snprintf(buffer, sizeof(buffer), "/set %s ", o->name);

    switch (o->stype) {
    case SSET_BOOL:
      o->desired_val = secfile_lookup_bool_default(&sf, o->default_val,
                                                   "server.%s", o->name);
      changed = (o->desired_val != o->default_val
              && o->desired_val != o->val);
      if (changed) {
        sz_strlcat(buffer, o->desired_val ? "1" : "0");
      }
      break;
    case SSET_INT:
      o->desired_val = secfile_lookup_int_default(&sf, o->default_val,
                                                  "server.%s", o->name);
      changed = (o->desired_val != o->default_val
              && o->desired_val != o->val);
      if (changed) {
        cat_snprintf(buffer, sizeof(buffer), "%d", o->desired_val);
      }
      break;
    case SSET_STRING:
      desired_string = secfile_lookup_str_default(&sf, o->default_strval,
                                                  "server.%s", o->name);
      if (NULL != desired_string) {
        if (NULL != o->desired_strval) {
          free(o->desired_strval);
        }
        o->desired_strval = mystrdup(desired_string);
        changed = (0 != strcmp(desired_string, o->default_strval)
                && 0 != strcmp(desired_string, o->strval));
        if (changed) {
          sz_strlcat(buffer, desired_string);
        }
      }
      break;
    default:
      freelog(LOG_ERROR,
              "load_settable_options() bad type %d.",
              o->stype);
      break;
    };

    if (changed && send_it) {
      send_chat(buffer);
    }
  }

  section_file_free(&sf);
}

/****************************************************************
 Save settable per connection/server options.
*****************************************************************/
static void save_settable_options(struct section_file *sf)
{
  int i = 0;

  for (; i < num_settable_options; i++) {
    struct options_settable *o = &settable_options[i];

    switch (o->stype) {
    case SSET_BOOL:
      if (o->desired_val != o->default_val) {
        secfile_insert_bool(sf, o->desired_val, "server.%s", o->name);
      }
      break;
    case SSET_INT:
      if (o->desired_val != o->default_val) {
        secfile_insert_int(sf, o->desired_val, "server.%s",  o->name);
      }
      break;
    case SSET_STRING:
      if (NULL != o->desired_strval
       && 0 != strcmp(o->desired_strval, o->default_strval)) {
        secfile_insert_str(sf, o->desired_strval, "server.%s", o->name);
      }
      break;
    default:
      freelog(LOG_ERROR,
              "save_settable_options() bad type %d.",
              o->stype);
      break;
    };
  }
}


/****************************************************************
 Load from the rc file any options that are not ruleset specific.
 It is called after ui_init(), yet before ui_main().
 Unfortunately, this means that some clients cannot display.
 Instead, use freelog().
*****************************************************************/
void load_general_options(void)
{
  struct section_file sf;
  int i, num;
  char *name;
  const char * const prefix = "client";

  name = option_file_name();
  if (!name) {
    /* FIXME: need better messages */
    freelog(LOG_ERROR, _("Save failed, cannot find a filename."));
    return;
  }
  if (!section_file_load(&sf, name)) {
    /* try to create the rc file */
    section_file_init(&sf);
    secfile_insert_str(&sf, VERSION_STRING, "client.version");

    create_default_cma_presets();
    save_cma_presets(&sf);

    /* FIXME: need better messages */
    if (!section_file_save(&sf, name, 0, FZ_PLAIN)) {
      freelog(LOG_ERROR, _("Save failed, cannot write to file %s"), name);
    } else {
      freelog(LOG_NORMAL, _("Saved settings to file %s"), name);
    }
    section_file_free(&sf);
    return;
  }

  /* a "secret" option for the lazy. TODO: make this saveable */
  sz_strlcpy(password, 
             secfile_lookup_str_default(&sf, "", "%s.password", prefix));

  save_options_on_exit =
    secfile_lookup_bool_default(&sf, save_options_on_exit,
				"%s.save_options_on_exit", prefix);
  fullscreen_mode =
    secfile_lookup_bool_default(&sf, fullscreen_mode,
				"%s.fullscreen_mode", prefix);

  client_options_iterate_all(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      *(o->boolean.pvalue) =
	  secfile_lookup_bool_default(&sf, *(o->boolean.pvalue), "%s.%s",
				      prefix, o->name);
      break;
    case COT_INTEGER:
      *(o->integer.pvalue) =
	  secfile_lookup_int_default(&sf, *(o->integer.pvalue), "%s.%s",
				      prefix, o->name);
      break;
    case COT_STRING:
    case COT_FONT:
      mystrlcpy(o->string.pvalue,
                     secfile_lookup_str_default(&sf, o->string.pvalue, "%s.%s",
                     prefix, o->name), o->string.size);
      break;
    }
  } client_options_iterate_all_end;

  message_options_load(&sf, prefix);
  
  /* Players dialog */
  for(i = 1; i < num_player_dlg_columns; i++) {
    bool *show = &(player_dlg_columns[i].show);
    *show = secfile_lookup_bool_default(&sf, *show, "%s.player_dlg_%s", prefix,
                                        player_dlg_columns[i].tagname);
  }
  
  /* Load cma presets. If cma.number_of_presets doesn't exist, don't load 
   * any, the order here should be reversed to keep the order the same */
  num = secfile_lookup_int_default(&sf, -1, "cma.number_of_presets");
  if (num == -1) {
    create_default_cma_presets();
  } else {
    for (i = num - 1; i >= 0; i--) {
      load_cma_preset(&sf, i);
    }
  }
 
  section_file_free(&sf);
}

/****************************************************************
 this loads from the rc file any options which need to know what the 
 current ruleset is. It's called the first time client goes into
 C_S_RUNNING
*****************************************************************/
void load_ruleset_specific_options(void)
{
  struct section_file sf;
  int i;
  char *name = option_file_name();

  if (!name) {
    /* fail silently */
    return;
  }
  if (!section_file_load(&sf, name))
    return;

  if (NULL != client.conn.playing) {
    /* load global worklists */
    for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
      worklist_load(&sf, &client.worklists[i],
		    "worklists.worklist%d", i);
    }
  }

  /* Load city report columns (which include some ruleset data). */
  for (i = 0; i < num_city_report_spec(); i++) {
    bool *ip = city_report_spec_show_ptr(i);

    *ip = secfile_lookup_bool_default(&sf, *ip, "client.city_report_%s",
				     city_report_spec_tagname(i));
  }

  section_file_free(&sf);
}

/****************************************************************
... 
*****************************************************************/
void save_options(void)
{
  struct section_file sf;
  int i;
  char *name = option_file_name();

  if (!name) {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("Save failed, cannot find a filename."));
    return;
  }

  section_file_init(&sf);
  secfile_insert_str(&sf, VERSION_STRING, "client.version");

  secfile_insert_bool(&sf, save_options_on_exit, "client.save_options_on_exit");
  secfile_insert_bool(&sf, fullscreen_mode, "client.fullscreen_mode");

  client_options_iterate_all(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      secfile_insert_bool(&sf, *(o->boolean.pvalue), "client.%s", o->name);
      break;
    case COT_INTEGER:
      secfile_insert_int(&sf, *(o->integer.pvalue), "client.%s", o->name);
      break;
    case COT_STRING:
    case COT_FONT:
      secfile_insert_str(&sf, o->string.pvalue, "client.%s", o->name);
      break;
    }
  } client_options_iterate_all_end;

  message_options_save(&sf, "client");

  for (i = 0; i < num_city_report_spec(); i++) {
    secfile_insert_bool(&sf, *(city_report_spec_show_ptr(i)),
		       "client.city_report_%s",
		       city_report_spec_tagname(i));
  }
  
  /* Players dialog */
  for (i = 1; i < num_player_dlg_columns; i++) {
    secfile_insert_bool(&sf, player_dlg_columns[i].show,
                        "client.player_dlg_%s",
                        player_dlg_columns[i].tagname);
  }

  /* server settings */
  save_cma_presets(&sf);
  save_settable_options(&sf);

  /* insert global worklists */
  if (NULL != client.conn.playing) {
    for(i = 0; i < MAX_NUM_WORKLISTS; i++){
      if (client.worklists[i].is_valid) {
	worklist_save(&sf, &client.worklists[i], client.worklists[i].length,
		      "worklists.worklist%d", i);
      }
    }
  }

  /* save to disk */
  if (!section_file_save(&sf, name, 0, FZ_PLAIN)) {
    output_window_printf(FTC_CLIENT_INFO, NULL,
                         _("Save failed, cannot write to file %s"), name);
  } else {
    output_window_printf(FTC_CLIENT_INFO, NULL,
                         _("Saved settings to file %s"), name);
  }
  section_file_free(&sf);
}

/****************************************************************************
  Callback when a mapview graphics option is changed (redraws the canvas).
****************************************************************************/
void mapview_redraw_callback(struct client_option *option)
{
  update_map_canvas_visible();
}

/****************************************************************************
   Callback when the reqtree  show icons option is changed.
   The tree is recalculated.
****************************************************************************/
static void reqtree_show_icons_callback(struct client_option *option)
{
  /* This will close research dialog, when it's open again the techtree will
   * be recalculated */
  popdown_all_game_dialogs();
}

/****************************************************************************
  Callback for when any view option is changed.
****************************************************************************/
static void view_option_changed_callback(struct client_option *option)
{
  update_menus();
  update_map_canvas_visible();
}
