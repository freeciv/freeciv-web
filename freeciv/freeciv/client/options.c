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

#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/* utility */
#include "deprecations.h"
#include "fcintl.h"
#include "ioz.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "shared.h"
#include "string_vector.h"
#include "support.h"

/* common */
#include "events.h"
#include "version.h"

/* client/agents */
#include "cma_fec.h"

/* client/include */
#include "chatline_g.h"
#include "dialogs_g.h"
#include "gui_main_g.h"
#include "menu_g.h"
#include "optiondlg_g.h"
#include "repodlgs_g.h"
#include "voteinfo_bar_g.h"

/* client */
#include "audio.h"
#include "cityrepdata.h"
#include "client_main.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "global_worklist.h"
#include "mapctrl_common.h"
#include "mapview_common.h"
#include "music.h"
#include "overview_common.h"
#include "packhand_gen.h"
#include "plrdlg_common.h"
#include "repodlgs_common.h"
#include "servers.h"
#include "themes_common.h"
#include "tilespec.h"

#include "options.h"


struct client_options gui_options = {
/** Defaults for options normally on command line **/

  .default_user_name = "\0",
  .default_server_host = "localhost",
  .default_server_port = DEFAULT_SOCK_PORT,
  .default_metaserver = DEFAULT_METASERVER_OPTION,
  .default_tileset_square_name = "\0",
  .default_tileset_hex_name = "\0",
  .default_tileset_isohex_name = "\0",
  .default_sound_set_name = "stdsounds",
  .default_music_set_name = "stdmusic",
  .default_sound_plugin_name = "\0",
  .default_chat_logfile = GUI_DEFAULT_CHAT_LOGFILE,

  .save_options_on_exit = TRUE,

  .use_prev_server = FALSE,
  .heartbeat_enabled = FALSE,

/** Migrations **/
  .first_boot = FALSE,
  .default_tileset_name = "\0",
  .default_tileset_overhead_name = "\0",
  .default_tileset_iso_name = "\0",
  .gui_gtk3_migrated_from_gtk2 = FALSE,
  .gui_gtk3_22_migrated_from_gtk3 = FALSE,
  .gui_gtk4_migrated_from_gtk3_22 = FALSE,
  .gui_sdl2_migrated_from_sdl = FALSE,
  .gui_gtk2_migrated_from_2_5 = FALSE,
  .gui_gtk3_migrated_from_2_5 = FALSE,
  .gui_qt_migrated_from_2_5 = FALSE,

  .migrate_fullscreen = FALSE,

/** Local Options: **/

  .solid_color_behind_units = FALSE,
  .sound_bell_at_new_turn = FALSE,
  .smooth_move_unit_msec = 30,
  .smooth_center_slide_msec = 200,
  .smooth_combat_step_msec = 10,
  .ai_manual_turn_done = TRUE,
  .auto_center_on_unit = TRUE,
  .auto_center_on_automated = TRUE,
  .auto_center_on_combat = FALSE,
  .auto_center_each_turn = TRUE,
  .wakeup_focus = TRUE,
  .goto_into_unknown = TRUE,
  .center_when_popup_city = TRUE,
  .show_previous_turn_messages = TRUE,
  .concise_city_production = FALSE,
  .auto_turn_done = FALSE,
  .meta_accelerators = TRUE,
  .ask_city_name = TRUE,
  .popup_new_cities = TRUE,
  .popup_actor_arrival = TRUE,
  .popup_attack_actions = TRUE,
  .keyboardless_goto = TRUE,
  .enable_cursor_changes = TRUE,
  .separate_unit_selection = FALSE,
  .unit_selection_clears_orders = TRUE,
  .highlight_our_names = FT_COLOR("#000000", "#FFFF00"),

  .voteinfo_bar_use = TRUE,
  .voteinfo_bar_always_show = FALSE,
  .voteinfo_bar_hide_when_not_player = FALSE,
  .voteinfo_bar_new_at_front = FALSE,

  .autoaccept_tileset_suggestion = FALSE,
  .autoaccept_soundset_suggestion = FALSE,
  .autoaccept_musicset_suggestion = FALSE,

  .sound_enable_effects = TRUE,
  .sound_enable_menu_music = TRUE,
  .sound_enable_game_music = TRUE,

/* This option is currently set by the client - not by the user. */
  .update_city_text_in_refresh_tile = TRUE,

  .draw_city_outlines = TRUE,
  .draw_city_output = FALSE,
  .draw_map_grid = FALSE,
  .draw_city_names = TRUE,
  .draw_city_growth = TRUE,
  .draw_city_productions = TRUE,
  .draw_city_buycost = FALSE,
  .draw_city_trade_routes = FALSE,
  .draw_terrain = TRUE,
  .draw_coastline = FALSE,
  .draw_roads_rails = TRUE,
  .draw_irrigation = TRUE,
  .draw_mines = TRUE,
  .draw_fortress_airbase = TRUE,
  .draw_specials = TRUE,
  .draw_huts = TRUE,
  .draw_pollution = TRUE,
  .draw_cities = TRUE,
  .draw_units = TRUE,
  .draw_focus_unit = FALSE,
  .draw_fog_of_war = TRUE,
  .draw_borders = TRUE,
  .draw_native = FALSE,
  .draw_full_citybar = TRUE,
  .draw_unit_shields = TRUE,
  .player_dlg_show_dead_players = TRUE,
  .reqtree_show_icons = TRUE,
  .reqtree_curved_lines = FALSE,

/* options for map images */
/*  .mapimg_format, */
  .mapimg_zoom = 2,
/* See the definition of MAPIMG_LAYER in mapimg.h. */
  .mapimg_layer = {
    FALSE, /* a - MAPIMG_LAYER_AREA */
    TRUE,  /* b - MAPIMG_LAYER_BORDERS */
    TRUE,  /* c - MAPIMG_LAYER_CITIES */
    TRUE,  /* f - MAPIMG_LAYER_FOGOFWAR */
    TRUE,  /* k - MAPIMG_LAYER_KNOWLEDGE */
    TRUE,  /* t - MAPIMG_LAYER_TERRAIN */
    TRUE   /* u - MAPIMG_LAYER_UNITS */
  },
/*  .mapimg_filename, */

  .zoom_set = FALSE,
  .zoom_default_level = 1.0,

/* gui-gtk-2.0 client specific options. */
  .gui_gtk2_default_theme_name = FC_GTK2_DEFAULT_THEME_NAME,
  .gui_gtk2_fullscreen = FALSE,
  .gui_gtk2_map_scrollbars = FALSE,
  .gui_gtk2_dialogs_on_top = TRUE,
  .gui_gtk2_show_task_icons = TRUE,
  .gui_gtk2_enable_tabs = TRUE,
  .gui_gtk2_better_fog = TRUE,
  .gui_gtk2_show_chat_message_time = FALSE,
  .gui_gtk2_new_messages_go_to_top = FALSE,
  .gui_gtk2_show_message_window_buttons = TRUE,
  .gui_gtk2_metaserver_tab_first = FALSE,
  .gui_gtk2_allied_chat_only = FALSE,
  .gui_gtk2_message_chat_location = GUI_GTK_MSGCHAT_MERGED,
  .gui_gtk2_small_display_layout = TRUE,
  .gui_gtk2_mouse_over_map_focus = FALSE,
  .gui_gtk2_chatline_autocompletion = TRUE,
  .gui_gtk2_citydlg_xsize = GUI_GTK2_CITYDLG_DEFAULT_XSIZE,
  .gui_gtk2_citydlg_ysize = GUI_GTK2_CITYDLG_DEFAULT_YSIZE,
  .gui_gtk2_popup_tech_help = GUI_POPUP_TECH_HELP_RULESET,
  .gui_gtk2_font_city_label = "Monospace 8",
  .gui_gtk2_font_notify_label = "Monospace Bold 9",
  .gui_gtk2_font_spaceship_label = "Monospace 8",
  .gui_gtk2_font_help_label = "Sans Bold 10",
  .gui_gtk2_font_help_link = "Sans 9",
  .gui_gtk2_font_help_text = "Monospace 8",
  .gui_gtk2_font_chatline = "Monospace 8",
  .gui_gtk2_font_beta_label = "Sans Italic 10",
  .gui_gtk2_font_small = "Sans 9",
  .gui_gtk2_font_comment_label = "Sans Italic 9",
  .gui_gtk2_font_city_names = "Sans Bold 10",
  .gui_gtk2_font_city_productions = "Serif 10",
  .gui_gtk2_font_reqtree_text = "Serif 10",

/* gui-gtk-3.0 client specific options. */
  .gui_gtk3_default_theme_name = FC_GTK3_DEFAULT_THEME_NAME,
  .gui_gtk3_fullscreen = FALSE,
  .gui_gtk3_map_scrollbars = FALSE,
  .gui_gtk3_dialogs_on_top = TRUE,
  .gui_gtk3_show_task_icons = TRUE,
  .gui_gtk3_enable_tabs = TRUE,
  .gui_gtk3_show_chat_message_time = FALSE,
  .gui_gtk3_new_messages_go_to_top = FALSE,
  .gui_gtk3_show_message_window_buttons = TRUE,
  .gui_gtk3_metaserver_tab_first = FALSE,
  .gui_gtk3_allied_chat_only = FALSE,
  .gui_gtk3_message_chat_location = GUI_GTK_MSGCHAT_MERGED,
  .gui_gtk3_small_display_layout = TRUE,
  .gui_gtk3_mouse_over_map_focus = FALSE,
  .gui_gtk3_chatline_autocompletion = TRUE,
  .gui_gtk3_citydlg_xsize = GUI_GTK3_CITYDLG_DEFAULT_XSIZE,
  .gui_gtk3_citydlg_ysize = GUI_GTK3_CITYDLG_DEFAULT_YSIZE,
  .gui_gtk3_popup_tech_help = GUI_POPUP_TECH_HELP_RULESET,
  .gui_gtk3_governor_range_min = -20,
  .gui_gtk3_governor_range_max = 20,
  .gui_gtk3_font_city_label = "Monospace 8",
  .gui_gtk3_font_notify_label = "Monospace Bold 9",
  .gui_gtk3_font_spaceship_label = "Monospace 8",
  .gui_gtk3_font_help_label = "Sans Bold 10",
  .gui_gtk3_font_help_link = "Sans 9",
  .gui_gtk3_font_help_text = "Monospace 8",
  .gui_gtk3_font_chatline = "Monospace 8",
  .gui_gtk3_font_beta_label = "Sans Italic 10",
  .gui_gtk3_font_small = "Sans 9",
  .gui_gtk3_font_comment_label = "Sans Italic 9",
  .gui_gtk3_font_city_names = "Sans Bold 10",
  .gui_gtk3_font_city_productions = "Serif 10",
  .gui_gtk3_font_reqtree_text = "Serif 10",

/* gui-gtk-3.22 client specific options. */
  .gui_gtk3_22_default_theme_name = FC_GTK3_22_DEFAULT_THEME_NAME,
  .gui_gtk3_22_fullscreen = FALSE,
  .gui_gtk3_22_map_scrollbars = FALSE,
  .gui_gtk3_22_dialogs_on_top = TRUE,
  .gui_gtk3_22_show_task_icons = TRUE,
  .gui_gtk3_22_enable_tabs = TRUE,
  .gui_gtk3_22_show_chat_message_time = FALSE,
  .gui_gtk3_22_new_messages_go_to_top = FALSE,
  .gui_gtk3_22_show_message_window_buttons = TRUE,
  .gui_gtk3_22_metaserver_tab_first = FALSE,
  .gui_gtk3_22_allied_chat_only = FALSE,
  .gui_gtk3_22_message_chat_location = GUI_GTK_MSGCHAT_MERGED,
  .gui_gtk3_22_small_display_layout = TRUE,
  .gui_gtk3_22_mouse_over_map_focus = FALSE,
  .gui_gtk3_22_chatline_autocompletion = TRUE,
  .gui_gtk3_22_citydlg_xsize = GUI_GTK3_22_CITYDLG_DEFAULT_XSIZE,
  .gui_gtk3_22_citydlg_ysize = GUI_GTK3_22_CITYDLG_DEFAULT_YSIZE,
  .gui_gtk3_22_popup_tech_help = GUI_POPUP_TECH_HELP_RULESET,
  .gui_gtk3_22_governor_range_min = -20,
  .gui_gtk3_22_governor_range_max = 20,
  .gui_gtk3_22_font_city_label = "Monospace 8",
  .gui_gtk3_22_font_notify_label = "Monospace Bold 9",
  .gui_gtk3_22_font_spaceship_label = "Monospace 8",
  .gui_gtk3_22_font_help_label = "Sans Bold 10",
  .gui_gtk3_22_font_help_link = "Sans 9",
  .gui_gtk3_22_font_help_text = "Monospace 8",
  .gui_gtk3_22_font_chatline = "Monospace 8",
  .gui_gtk3_22_font_beta_label = "Sans Italic 10",
  .gui_gtk3_22_font_small = "Sans 9",
  .gui_gtk3_22_font_comment_label = "Sans Italic 9",
  .gui_gtk3_22_font_city_names = "Sans Bold 10",
  .gui_gtk3_22_font_city_productions = "Serif 10",
  .gui_gtk3_22_font_reqtree_text = "Serif 10",
  
/* gui-gtk-3.x client specific options. */
  .gui_gtk4_default_theme_name = FC_GTK4_DEFAULT_THEME_NAME,
  .gui_gtk4_fullscreen = FALSE,
  .gui_gtk4_map_scrollbars = FALSE,
  .gui_gtk4_dialogs_on_top = TRUE,
  .gui_gtk4_show_task_icons = TRUE,
  .gui_gtk4_enable_tabs = TRUE,
  .gui_gtk4_show_chat_message_time = FALSE,
  .gui_gtk4_new_messages_go_to_top = FALSE,
  .gui_gtk4_show_message_window_buttons = TRUE,
  .gui_gtk4_metaserver_tab_first = FALSE,
  .gui_gtk4_allied_chat_only = FALSE,
  .gui_gtk4_message_chat_location = GUI_GTK_MSGCHAT_MERGED,
  .gui_gtk4_small_display_layout = TRUE,
  .gui_gtk4_mouse_over_map_focus = FALSE,
  .gui_gtk4_chatline_autocompletion = TRUE,
  .gui_gtk4_citydlg_xsize = GUI_GTK4_CITYDLG_DEFAULT_XSIZE,
  .gui_gtk4_citydlg_ysize = GUI_GTK4_CITYDLG_DEFAULT_YSIZE,
  .gui_gtk4_popup_tech_help = GUI_POPUP_TECH_HELP_RULESET,
  .gui_gtk4_governor_range_min = -20,
  .gui_gtk4_governor_range_max = 20,
  .gui_gtk4_font_city_label = "Monospace 8",
  .gui_gtk4_font_notify_label = "Monospace Bold 9",
  .gui_gtk4_font_spaceship_label = "Monospace 8",
  .gui_gtk4_font_help_label = "Sans Bold 10",
  .gui_gtk4_font_help_link = "Sans 9",
  .gui_gtk4_font_help_text = "Monospace 8",
  .gui_gtk4_font_chatline = "Monospace 8",
  .gui_gtk4_font_beta_label = "Sans Italic 10",
  .gui_gtk4_font_small = "Sans 9",
  .gui_gtk4_font_comment_label = "Sans Italic 9",
  .gui_gtk4_font_city_names = "Sans Bold 10",
  .gui_gtk4_font_city_productions = "Serif 10",
  .gui_gtk4_font_reqtree_text = "Serif 10",

/* gui-sdl client specific options. */
  .gui_sdl_fullscreen = FALSE,
  .gui_sdl_screen = VIDEO_MODE(640, 480),
  .gui_sdl_do_cursor_animation = TRUE,
  .gui_sdl_use_color_cursors = TRUE,

/* gui-sdl2 client specific options. */
  .gui_sdl2_default_theme_name = FC_SDL2_DEFAULT_THEME_NAME,
  .gui_sdl2_fullscreen = FALSE,
  .gui_sdl2_screen = VIDEO_MODE(640, 480),
  .gui_sdl2_swrenderer = FALSE,
  .gui_sdl2_do_cursor_animation = TRUE,
  .gui_sdl2_use_color_cursors = TRUE,
  .gui_sdl2_font_city_names = "10",
  .gui_sdl2_font_city_productions = "10",

/* gui-qt client specific options. */
  .gui_qt_fullscreen = FALSE,
  .gui_qt_show_preview = TRUE,
  .gui_qt_sidebar_left = TRUE,
  .gui_qt_default_theme_name = FC_QT_DEFAULT_THEME_NAME,
  .gui_qt_font_city_label = "Monospace,8,-1,5,50,0,0,0,0,0",
  .gui_qt_font_default = "Sans Serif,10,-1,5,75,0,0,0,0,0",
  .gui_qt_font_notify_label = "Monospace,8,-1,5,75,0,0,0,0,0",
  .gui_qt_font_spaceship_label = "Monospace,8,-1,5,50,0,0,0,0,0",
  .gui_qt_font_help_label = "Sans Serif,9,-1,5,50,0,0,0,0,0",
  .gui_qt_font_help_link = "Sans Serif,9,-1,5,50,0,0,0,0,0",
  .gui_qt_font_help_text = "Monospace,8,-1,5,50,0,0,0,0,0",
  .gui_qt_font_help_title = "Sans Serif,10,-1,5,75,0,0,0,0,0",
  .gui_qt_font_chatline = "Monospace,8,-1,5,50,0,0,0,0,0",
  .gui_qt_font_beta_label = "Sans Serif,10,-1,5,50,1,0,0,0,0",
  .gui_qt_font_small = "Sans Serif,9,-1,5,50,0,0,0,0,0",
  .gui_qt_font_comment_label = "Sans Serif,9,-1,5,50,1,0,0,0,0",
  .gui_qt_font_city_names = "Sans Serif,10,-1,5,75,0,0,0,0,0",
  .gui_qt_font_city_productions = "Sans Serif,10,-1,5,50,1,0,0,0,0",
  .gui_qt_font_reqtree_text = "Sans Serif,10,-1,5,50,1,0,0,0,0",
  .gui_qt_show_titlebar = TRUE,
  .gui_qt_wakeup_text = "Wake up %1"
};

/* Set to TRUE after the first call to options_init(), to avoid the usage
 * of non-initialized datas when calling the changed callback. */
static bool options_fully_initialized = FALSE;

static const struct strvec *get_mapimg_format_list(const struct option *poption);

/****************************************************************************
  Option set structure.
****************************************************************************/
struct option_set {
  struct option * (*option_by_number) (int);
  struct option * (*option_first) (void);

  int (*category_number) (void);
  const char * (*category_name) (int);
};

/************************************************************************//**
  Returns the option corresponding of the number in this option set.
****************************************************************************/
struct option *optset_option_by_number(const struct option_set *poptset,
                                       int id)
{
  fc_assert_ret_val(NULL != poptset, NULL);

  return poptset->option_by_number(id);
}

/************************************************************************//**
  Returns the option corresponding of the name in this option set.
****************************************************************************/
struct option *optset_option_by_name(const struct option_set *poptset,
                                     const char *name)
{
  fc_assert_ret_val(NULL != poptset, NULL);

  options_iterate(poptset, poption) {
    if (0 == strcmp(option_name(poption), name)) {
      return poption;
    }
  } options_iterate_end;
  return NULL;
}

/************************************************************************//**
  Returns the first option of this option set.
****************************************************************************/
struct option *optset_option_first(const struct option_set *poptset)
{
  fc_assert_ret_val(NULL != poptset, NULL);

  return poptset->option_first();
}

/************************************************************************//**
  Returns the number of categories of this option set.
****************************************************************************/
int optset_category_number(const struct option_set *poptset)
{
  fc_assert_ret_val(NULL != poptset, 0);

  return poptset->category_number();
}

/************************************************************************//**
  Returns the name (translated) of the category of this option set.
****************************************************************************/
const char *optset_category_name(const struct option_set *poptset,
                                 int category)
{
  fc_assert_ret_val(NULL != poptset, NULL);

  return poptset->category_name(category);
}


/****************************************************************************
  The base class for options.
****************************************************************************/
struct option {
  /* A link to the option set. */
  const struct option_set *poptset;
  /* Type of the option. */
  enum option_type type;

  /* Common accessors. */
  const struct option_common_vtable {
    int (*number) (const struct option *);
    const char * (*name) (const struct option *);
    const char * (*description) (const struct option *);
    const char * (*help_text) (const struct option *);
    int (*category) (const struct option *);
    bool (*is_changeable) (const struct option *);
    struct option * (*next) (const struct option *);
  } *common_vtable;
  /* Specific typed accessors. */
  union {
    /* Specific boolean accessors (OT_BOOLEAN == type). */
    const struct option_bool_vtable {
      bool (*get) (const struct option *);
      bool (*def) (const struct option *);
      bool (*set) (struct option *, bool);
    } *bool_vtable;
    /* Specific integer accessors (OT_INTEGER == type). */
    const struct option_int_vtable {
      int (*get) (const struct option *);
      int (*def) (const struct option *);
      int (*minimum) (const struct option *);
      int (*maximum) (const struct option *);
      bool (*set) (struct option *, int);
    } *int_vtable;
    /* Specific string accessors (OT_STRING == type). */
    const struct option_str_vtable {
      const char * (*get) (const struct option *);
      const char * (*def) (const struct option *);
      const struct strvec * (*values) (const struct option *);
      bool (*set) (struct option *, const char *);
    } *str_vtable;
    /* Specific enum accessors (OT_ENUM == type). */
    const struct option_enum_vtable {
      int (*get) (const struct option *);
      int (*def) (const struct option *);
      const struct strvec * (*values) (const struct option *);
      bool (*set) (struct option *, int);
      int (*cmp) (const char *, const char *);
    } *enum_vtable;
    /* Specific bitwise accessors (OT_BITWISE == type). */
    const struct option_bitwise_vtable {
      unsigned (*get) (const struct option *);
      unsigned (*def) (const struct option *);
      const struct strvec * (*values) (const struct option *);
      bool (*set) (struct option *, unsigned);
    } *bitwise_vtable;
    /* Specific font accessors (OT_FONT == type). */
    const struct option_font_vtable {
      const char * (*get) (const struct option *);
      const char * (*def) (const struct option *);
      const char * (*target) (const struct option *);
      bool (*set) (struct option *, const char *);
    } *font_vtable;
    /* Specific color accessors (OT_COLOR == type). */
    const struct option_color_vtable {
      struct ft_color (*get) (const struct option *);
      struct ft_color (*def) (const struct option *);
      bool (*set) (struct option *, struct ft_color);
    } *color_vtable;
    /* Specific video mode accessors (OT_VIDEO_MODE == type). */
    const struct option_video_mode_vtable {
      struct video_mode (*get) (const struct option *);
      struct video_mode (*def) (const struct option *);
      bool (*set) (struct option *, struct video_mode);
    } *video_mode_vtable;
  };

  /* Called after the value changed. */
  void (*changed_callback) (struct option *option);

  int callback_data;

  /* Volatile. */
  void *gui_data;
};

#define OPTION(poption) ((struct option *) (poption))

#define OPTION_INIT(optset, spec_type, spec_table_var, common_table,        \
                    spec_table, changed_cb, cb_data) {                      \
  .poptset = optset,                                                        \
  .type = spec_type,                                                        \
  .common_vtable = &common_table,                                           \
  INIT_BRACE_BEGIN                                                          \
    .spec_table_var = &spec_table                                           \
  INIT_BRACE_END,                                                           \
  .changed_callback = changed_cb,                                           \
  .callback_data = cb_data,                                                 \
  .gui_data = NULL                                                          \
}
#define OPTION_BOOL_INIT(optset, common_table, bool_table, changed_cb)      \
  OPTION_INIT(optset, OT_BOOLEAN, bool_vtable, common_table, bool_table,    \
              changed_cb, 0)
#define OPTION_INT_INIT(optset, common_table, int_table, changed_cb)        \
  OPTION_INIT(optset, OT_INTEGER, int_vtable, common_table, int_table,      \
              changed_cb, 0)
#define OPTION_STR_INIT(optset, common_table, str_table, changed_cb, cb_data) \
  OPTION_INIT(optset, OT_STRING, str_vtable, common_table, str_table,       \
              changed_cb, cb_data)
#define OPTION_ENUM_INIT(optset, common_table, enum_table, changed_cb)      \
  OPTION_INIT(optset, OT_ENUM, enum_vtable, common_table, enum_table,       \
              changed_cb, 0)
#define OPTION_BITWISE_INIT(optset, common_table, bitwise_table,            \
                            changed_cb)                                     \
  OPTION_INIT(optset, OT_BITWISE, bitwise_vtable, common_table,             \
              bitwise_table, changed_cb, 0)
#define OPTION_FONT_INIT(optset, common_table, font_table, changed_cb)      \
  OPTION_INIT(optset, OT_FONT, font_vtable, common_table, font_table,       \
              changed_cb, 0)
#define OPTION_COLOR_INIT(optset, common_table, color_table, changed_cb)    \
  OPTION_INIT(optset, OT_COLOR, color_vtable, common_table, color_table,    \
              changed_cb, 0)
#define OPTION_VIDEO_MODE_INIT(optset, common_table, video_mode_table,      \
                               changed_cb)                                  \
  OPTION_INIT(optset, OT_VIDEO_MODE, video_mode_vtable, common_table,       \
              video_mode_table, changed_cb, 0)


/************************************************************************//**
  Returns the option set owner of this option.
****************************************************************************/
const struct option_set *option_optset(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);

  return poption->poptset;
}

/************************************************************************//**
  Returns the number of the option.
****************************************************************************/
int option_number(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, -1);

  return poption->common_vtable->number(poption);
}

/************************************************************************//**
  Returns the name of the option.
****************************************************************************/
const char *option_name(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);

  return poption->common_vtable->name(poption);
}

/************************************************************************//**
  Returns the description (translated) of the option.
****************************************************************************/
const char *option_description(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);

  return poption->common_vtable->description(poption);
}

/************************************************************************//**
  Returns the help text (translated) of the option.
****************************************************************************/
const char *option_help_text(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);

  return poption->common_vtable->help_text(poption);
}

/************************************************************************//**
  Returns the type of the option.
****************************************************************************/
enum option_type option_type(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, -1);

  return poption->type;
}

/************************************************************************//**
  Returns the category of the option.
****************************************************************************/
int option_category(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, -1);

  return poption->common_vtable->category(poption);
}

/************************************************************************//**
  Returns the name (translated) of the category of the option.
****************************************************************************/
const char *option_category_name(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);

  return optset_category_name(poption->poptset,
                              poption->common_vtable->category(poption));
}

/************************************************************************//**
  Returns TRUE if this option can be modified.
****************************************************************************/
bool option_is_changeable(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, FALSE);

  return poption->common_vtable->is_changeable(poption);
}

/************************************************************************//**
  Returns the next option or NULL if this is the last.
****************************************************************************/
struct option *option_next(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);

  return poption->common_vtable->next(poption);
}

/************************************************************************//**
  Set the option to its default value.  Returns TRUE if the option changed.
****************************************************************************/
bool option_reset(struct option *poption)
{
  fc_assert_ret_val(NULL != poption, FALSE);

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    return option_bool_set(poption, option_bool_def(poption));
  case OT_INTEGER:
    return option_int_set(poption, option_int_def(poption));
  case OT_STRING:
    return option_str_set(poption, option_str_def(poption));
  case OT_ENUM:
    return option_enum_set_int(poption, option_enum_def_int(poption));
  case OT_BITWISE:
    return option_bitwise_set(poption, option_bitwise_def(poption));
  case OT_FONT:
    return option_font_set(poption, option_font_def(poption));
  case OT_COLOR:
    return option_color_set(poption, option_color_def(poption));
  case OT_VIDEO_MODE:
    return option_video_mode_set(poption, option_video_mode_def(poption));
  }
  return FALSE;
}

/************************************************************************//**
  Set the function to call every time this option changes.  Can be NULL.
****************************************************************************/
void option_set_changed_callback(struct option *poption,
                                 void (*callback) (struct option *))
{
  fc_assert_ret(NULL != poption);

  poption->changed_callback = callback;
}

/************************************************************************//**
  Force to use the option changed callback.
****************************************************************************/
void option_changed(struct option *poption)
{
  fc_assert_ret(NULL != poption);

  if (!options_fully_initialized) {
    /* Prevent to use non-initialized datas. */
    return;
  }

  if (poption->changed_callback) {
    poption->changed_callback(poption);
  }

  option_gui_update(poption);
}

/************************************************************************//**
  Set the gui data for this option.
****************************************************************************/
void option_set_gui_data(struct option *poption, void *data)
{
  fc_assert_ret(NULL != poption);

  poption->gui_data = data;
}

/************************************************************************//**
  Returns the gui data of this option.
****************************************************************************/
void *option_get_gui_data(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);

  return poption->gui_data;
}

/************************************************************************//**
  Returns the callback data of this option.
****************************************************************************/
int option_get_cb_data(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, 0);

  return poption->callback_data;
}

/************************************************************************//**
  Returns the current value of this boolean option.
****************************************************************************/
bool option_bool_get(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_BOOLEAN == poption->type, FALSE);

  return poption->bool_vtable->get(poption);
}

/************************************************************************//**
  Returns the default value of this boolean option.
****************************************************************************/
bool option_bool_def(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_BOOLEAN == poption->type, FALSE);

  return poption->bool_vtable->def(poption);
}

/************************************************************************//**
  Sets the value of this boolean option. Returns TRUE if the value changed.
****************************************************************************/
bool option_bool_set(struct option *poption, bool val)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_BOOLEAN == poption->type, FALSE);

  if (poption->bool_vtable->set(poption, val)) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Returns the current value of this integer option.
****************************************************************************/
int option_int_get(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->get(poption);
}

/************************************************************************//**
  Returns the default value of this integer option.
****************************************************************************/
int option_int_def(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->def(poption);
}

/************************************************************************//**
  Returns the minimal value of this integer option.
****************************************************************************/
int option_int_min(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->minimum(poption);
}

/************************************************************************//**
  Returns the maximal value of this integer option.
****************************************************************************/
int option_int_max(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->maximum(poption);
}

/************************************************************************//**
  Sets the value of this integer option. Returns TRUE if the value changed.
****************************************************************************/
bool option_int_set(struct option *poption, int val)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_INTEGER == poption->type, FALSE);

  if (poption->int_vtable->set(poption, val)) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Returns the current value of this string option.
****************************************************************************/
const char *option_str_get(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_STRING == poption->type, NULL);

  return poption->str_vtable->get(poption);
}

/************************************************************************//**
  Returns the default value of this string option.
****************************************************************************/
const char *option_str_def(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_STRING == poption->type, NULL);

  return poption->str_vtable->def(poption);
}

/************************************************************************//**
  Returns the possible string values of this string option.
****************************************************************************/
const struct strvec *option_str_values(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_STRING == poption->type, NULL);

  return poption->str_vtable->values(poption);
}

/************************************************************************//**
  Sets the value of this string option. Returns TRUE if the value changed.
****************************************************************************/
bool option_str_set(struct option *poption, const char *str)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_STRING == poption->type, FALSE);
  fc_assert_ret_val(NULL != str, FALSE);

  if (poption->str_vtable->set(poption, str)) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Returns the value corresponding to the user-visible (translatable but not
  translated) string. Returns -1 if not matched.
****************************************************************************/
int option_enum_str_to_int(const struct option *poption, const char *str)
{
  const struct strvec *values;
  int val;

  fc_assert_ret_val(NULL != poption, -1);
  fc_assert_ret_val(OT_ENUM == poption->type, -1);
  values = poption->enum_vtable->values(poption);
  fc_assert_ret_val(NULL != values, -1);

  for (val = 0; val < strvec_size(values); val++) {
    if (0 == poption->enum_vtable->cmp(strvec_get(values, val), str)) {
      return val;
    }
  }
  return -1;
}

/************************************************************************//**
  Returns the user-visible (translatable but not translated) string
  corresponding to the value. Returns NULL on error.
****************************************************************************/
const char *option_enum_int_to_str(const struct option *poption, int val)
{
  const struct strvec *values;

  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_ENUM == poption->type, NULL);
  values = poption->enum_vtable->values(poption);
  fc_assert_ret_val(NULL != values, NULL);

  return strvec_get(values, val);
}

/************************************************************************//**
  Returns the current value of this enum option (as an integer).
****************************************************************************/
int option_enum_get_int(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, -1);
  fc_assert_ret_val(OT_ENUM == poption->type, -1);

  return poption->enum_vtable->get(poption);
}

/************************************************************************//**
  Returns the current value of this enum option as a user-visible
  (translatable but not translated) string.
****************************************************************************/
const char *option_enum_get_str(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_ENUM == poption->type, NULL);

  return strvec_get(poption->enum_vtable->values(poption),
                    poption->enum_vtable->get(poption));
}

/************************************************************************//**
  Returns the default value of this enum option (as an integer).
****************************************************************************/
int option_enum_def_int(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, -1);
  fc_assert_ret_val(OT_ENUM == poption->type, -1);

  return poption->enum_vtable->def(poption);
}

/************************************************************************//**
  Returns the default value of this enum option as a user-visible
  (translatable but not translated) string.
****************************************************************************/
const char *option_enum_def_str(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_ENUM == poption->type, NULL);

  return strvec_get(poption->enum_vtable->values(poption),
                    poption->enum_vtable->def(poption));
}

/************************************************************************//**
  Returns the possible string values of this enum option, as user-visible
  (translatable but not translated) strings.
****************************************************************************/
const struct strvec *option_enum_values(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_ENUM == poption->type, NULL);

  return poption->enum_vtable->values(poption);
}

/************************************************************************//**
  Sets the value of this enum option. Returns TRUE if the value changed.
****************************************************************************/
bool option_enum_set_int(struct option *poption, int val)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_ENUM == poption->type, FALSE);

  if (poption->enum_vtable->set(poption, val)) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Sets the value of this enum option from a string, which is matched as a
  user-visible (translatable but not translated) string. Returns TRUE if the
  value changed.
****************************************************************************/
bool option_enum_set_str(struct option *poption, const char *str)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_ENUM == poption->type, FALSE);
  fc_assert_ret_val(NULL != str, FALSE);

  if (poption->enum_vtable->set(poption,
                                option_enum_str_to_int(poption, str))) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Returns the current value of this bitwise option.
****************************************************************************/
unsigned option_bitwise_get(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, 0);
  fc_assert_ret_val(OT_BITWISE == poption->type, 0);

  return poption->bitwise_vtable->get(poption);
}

/************************************************************************//**
  Returns the default value of this bitwise option.
****************************************************************************/
unsigned option_bitwise_def(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, 0);
  fc_assert_ret_val(OT_BITWISE == poption->type, 0);

  return poption->bitwise_vtable->def(poption);
}

/************************************************************************//**
  Returns the mask of this bitwise option.
****************************************************************************/
unsigned option_bitwise_mask(const struct option *poption)
{
  const struct strvec *values;

  fc_assert_ret_val(NULL != poption, 0);
  fc_assert_ret_val(OT_BITWISE == poption->type, 0);

  values = poption->bitwise_vtable->values(poption);
  fc_assert_ret_val(NULL != values, 0);

  return (1 << strvec_size(values)) - 1;
}

/************************************************************************//**
  Returns a vector of strings describing every bit of this option, as
  user-visible (translatable but not translated) strings.
****************************************************************************/
const struct strvec *option_bitwise_values(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_BITWISE == poption->type, NULL);

  return poption->bitwise_vtable->values(poption);
}

/************************************************************************//**
  Sets the value of this bitwise option. Returns TRUE if the value changed.
****************************************************************************/
bool option_bitwise_set(struct option *poption, unsigned val)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_BITWISE == poption->type, FALSE);

  if (0 != (val & ~option_bitwise_mask(poption))
      || !poption->bitwise_vtable->set(poption, val)) {
    return FALSE;
  }

  option_changed(poption);
  return TRUE;
}

/************************************************************************//**
  Returns the current value of this font option.
****************************************************************************/
const char *option_font_get(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_FONT == poption->type, NULL);

  return poption->font_vtable->get(poption);
}

/************************************************************************//**
  Returns the default value of this font option.
****************************************************************************/
const char *option_font_def(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_FONT == poption->type, NULL);

  return poption->font_vtable->def(poption);
}

/************************************************************************//**
  Returns the target style name of this font option.
****************************************************************************/
const char *option_font_target(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, NULL);
  fc_assert_ret_val(OT_FONT == poption->type, NULL);

  return poption->font_vtable->target(poption);
}

/************************************************************************//**
  Sets the value of this font option. Returns TRUE if the value changed.
****************************************************************************/
bool option_font_set(struct option *poption, const char *font)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_FONT == poption->type, FALSE);
  fc_assert_ret_val(NULL != font, FALSE);

  if (poption->font_vtable->set(poption, font)) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Returns the current value of this color option.
****************************************************************************/
struct ft_color option_color_get(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, ft_color_construct(NULL, NULL));
  fc_assert_ret_val(OT_COLOR == poption->type, ft_color_construct(NULL, NULL));

  return poption->color_vtable->get(poption);
}

/************************************************************************//**
  Returns the default value of this color option.
****************************************************************************/
struct ft_color option_color_def(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, ft_color_construct(NULL, NULL));
  fc_assert_ret_val(OT_COLOR == poption->type, ft_color_construct(NULL, NULL));

  return poption->color_vtable->def(poption);
}

/************************************************************************//**
  Sets the value of this color option. Returns TRUE if the value
  changed.
****************************************************************************/
bool option_color_set(struct option *poption, struct ft_color color)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_COLOR == poption->type, FALSE);

  if (poption->color_vtable->set(poption, color)) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Returns the current value of this video mode option.
****************************************************************************/
struct video_mode option_video_mode_get(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, video_mode_construct(-1, -1));
  fc_assert_ret_val(OT_VIDEO_MODE == poption->type,
                    video_mode_construct(-1, -1));

  return poption->video_mode_vtable->get(poption);
}

/************************************************************************//**
  Returns the default value of this video mode option.
****************************************************************************/
struct video_mode option_video_mode_def(const struct option *poption)
{
  fc_assert_ret_val(NULL != poption, video_mode_construct(-1, -1));
  fc_assert_ret_val(OT_VIDEO_MODE == poption->type,
                    video_mode_construct(-1, -1));

  return poption->video_mode_vtable->def(poption);
}

/************************************************************************//**
  Sets the value of this video mode option. Returns TRUE if the value
  changed.
****************************************************************************/
bool option_video_mode_set(struct option *poption, struct video_mode mode)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(OT_VIDEO_MODE == poption->type, FALSE);

  if (poption->video_mode_vtable->set(poption, mode)) {
    option_changed(poption);
    return TRUE;
  }
  return FALSE;
}


/****************************************************************************
  Client option set.
****************************************************************************/
static struct option *client_optset_option_by_number(int id);
static struct option *client_optset_option_first(void);
static int client_optset_category_number(void);
static const char *client_optset_category_name(int category);

static struct option_set client_optset_static = {
  .option_by_number = client_optset_option_by_number,
  .option_first = client_optset_option_first,
  .category_number = client_optset_category_number,
  .category_name = client_optset_category_name
};
const struct option_set *client_optset = &client_optset_static;

struct copt_val_name {
  const char *support;          /* Untranslated long support name, used
                                 * for saving. */
  const char *pretty;           /* Translated, used to display to the
                                 * users. */
};

/****************************************************************************
  Virtuals tables for the client options.
****************************************************************************/
static int client_option_number(const struct option *poption);
static const char *client_option_name(const struct option *poption);
static const char *client_option_description(const struct option *poption);
static const char *client_option_help_text(const struct option *poption);
static int client_option_category(const struct option *poption);
static bool client_option_is_changeable(const struct option *poption);
static struct option *client_option_next(const struct option *poption);
static void client_option_adjust_defaults(void);

static const struct option_common_vtable client_option_common_vtable = {
  .number = client_option_number,
  .name = client_option_name,
  .description = client_option_description,
  .help_text = client_option_help_text,
  .category = client_option_category,
  .is_changeable = client_option_is_changeable,
  .next = client_option_next
};

static bool client_option_bool_get(const struct option *poption);
static bool client_option_bool_def(const struct option *poption);
static bool client_option_bool_set(struct option *poption, bool val);

static const struct option_bool_vtable client_option_bool_vtable = {
  .get = client_option_bool_get,
  .def = client_option_bool_def,
  .set = client_option_bool_set
};

static int client_option_int_get(const struct option *poption);
static int client_option_int_def(const struct option *poption);
static int client_option_int_min(const struct option *poption);
static int client_option_int_max(const struct option *poption);
static bool client_option_int_set(struct option *poption, int val);

static const struct option_int_vtable client_option_int_vtable = {
  .get = client_option_int_get,
  .def = client_option_int_def,
  .minimum = client_option_int_min,
  .maximum = client_option_int_max,
  .set = client_option_int_set
};

static const char *client_option_str_get(const struct option *poption);
static const char *client_option_str_def(const struct option *poption);
static const struct strvec *
    client_option_str_values(const struct option *poption);
static bool client_option_str_set(struct option *poption, const char *str);

static const struct option_str_vtable client_option_str_vtable = {
  .get = client_option_str_get,
  .def = client_option_str_def,
  .values = client_option_str_values,
  .set = client_option_str_set
};

static int client_option_enum_get(const struct option *poption);
static int client_option_enum_def(const struct option *poption);
static const struct strvec *
    client_option_enum_pretty_names(const struct option *poption);
static bool client_option_enum_set(struct option *poption, int val);

static const struct option_enum_vtable client_option_enum_vtable = {
  .get = client_option_enum_get,
  .def = client_option_enum_def,
  .values = client_option_enum_pretty_names,
  .set = client_option_enum_set,
  .cmp = fc_strcasecmp
};

#if 0 /* There's no bitwise options currently */
static unsigned client_option_bitwise_get(const struct option *poption);
static unsigned client_option_bitwise_def(const struct option *poption);
static const struct strvec *
    client_option_bitwise_pretty_names(const struct option *poption);
static bool client_option_bitwise_set(struct option *poption, unsigned val);

static const struct option_bitwise_vtable client_option_bitwise_vtable = {
  .get = client_option_bitwise_get,
  .def = client_option_bitwise_def,
  .values = client_option_bitwise_pretty_names,
  .set = client_option_bitwise_set
};
#endif /* 0 */

static const char *client_option_font_get(const struct option *poption);
static const char *client_option_font_def(const struct option *poption);
static const char *client_option_font_target(const struct option *poption);
static bool client_option_font_set(struct option *poption, const char *font);

static const struct option_font_vtable client_option_font_vtable = {
  .get = client_option_font_get,
  .def = client_option_font_def,
  .target = client_option_font_target,
  .set = client_option_font_set
};

static struct ft_color client_option_color_get(const struct option *poption);
static struct ft_color client_option_color_def(const struct option *poption);
static bool client_option_color_set(struct option *poption,
                                    struct ft_color color);

static const struct option_color_vtable client_option_color_vtable = {
  .get = client_option_color_get,
  .def = client_option_color_def,
  .set = client_option_color_set
};

static struct video_mode
client_option_video_mode_get(const struct option *poption);
static struct video_mode
client_option_video_mode_def(const struct option *poption);
static bool client_option_video_mode_set(struct option *poption,
                                         struct video_mode mode);

static const struct option_video_mode_vtable client_option_video_mode_vtable = {
  .get = client_option_video_mode_get,
  .def = client_option_video_mode_def,
  .set = client_option_video_mode_set
};

enum client_option_category {
  COC_GRAPHICS,
  COC_OVERVIEW,
  COC_SOUND,
  COC_INTERFACE,
  COC_MAPIMG,
  COC_NETWORK,
  COC_FONT,
  COC_MAX
};

/****************************************************************************
  Derived class client option, inherinting of base class option.
****************************************************************************/
struct client_option {
  struct option base_option;    /* Base structure, must be the first! */

  const char *name;             /* Short name - used as an identifier */
  const char *description;      /* One-line description */
  const char *help_text;        /* Paragraph-length help text */
  enum client_option_category category;
  enum gui_type specific;       /* GUI_STUB for common options. */

  union {
    /* OT_BOOLEAN type option. */
    struct {
      bool *const pvalue;
      const bool def;
    } boolean;
    /* OT_INTEGER type option. */
    struct {
      int *const pvalue;
      const int def, min, max;
    } integer;
    /* OT_STRING type option. */
    struct {
      char *const pvalue;
      const size_t size;
      const char *const def;
      /* 
       * A function to return a string vector of possible string values,
       * or NULL for none. 
       */
      const struct strvec *(*const val_accessor) (const struct option *);
    } string;
    /* OT_ENUM type option. */
    struct {
      int *const pvalue;
      const int def;
      struct strvec *support_names, *pretty_names; /* untranslated */
      const struct copt_val_name * (*const name_accessor) (int value);
    } enumerator;
    /* OT_BITWISE type option. */
    struct {
      unsigned *const pvalue;
      const unsigned def;
      struct strvec *support_names, *pretty_names; /* untranslated */
      const struct copt_val_name * (*const name_accessor) (int value);
    } bitwise;
    /* OT_FONT type option. */
    struct {
      char *const pvalue;
      const size_t size;
      const char *const def;
      const char *const target;
    } font;
    /* OT_COLOR type option. */
    struct {
      struct ft_color *const pvalue;
      const struct ft_color def;
    } color;
    /* OT_VIDEO_MODE type option. */
    struct {
      struct video_mode *const pvalue;
      const struct video_mode def;
    } video_mode;
  };
};

#define CLIENT_OPTION(poption) ((struct client_option *) (poption))

/*
 * Generate a client option of type OT_BOOLEAN.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef:  The default value of this client option (FALSE or TRUE).
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_BOOL_OPTION(oname, odesc, ohelp, ocat, ospec, odef, ocb)        \
{                                                                           \
  .base_option = OPTION_BOOL_INIT(&client_optset_static,                    \
                                  client_option_common_vtable,              \
                                  client_option_bool_vtable, ocb),          \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .boolean = {                                                            \
      .pvalue = &gui_options.oname,                                         \
      .def = odef,                                                          \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_INTEGER.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef:  The default value of this client option.
 * omin:  The minimal value of this client option.
 * omax:  The maximal value of this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_INT_OPTION(oname, odesc, ohelp, ocat, ospec, odef, omin, omax, ocb) \
{                                                                           \
  .base_option = OPTION_INT_INIT(&client_optset_static,                     \
                                 client_option_common_vtable,               \
                                 client_option_int_vtable, ocb),            \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .integer = {                                                            \
      .pvalue = &gui_options.oname,                                         \
      .def = odef,                                                          \
      .min = omin,                                                          \
      .max = omax                                                           \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_STRING.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 *        Be sure to pass the array variable and not a pointer to it because
 *        the size is calculated with sizeof().
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determines for what particular client
 *        gui this option is for. Set to GUI_STUB for common options.
 * odef:  The default string for this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_STR_OPTION(oname, odesc, ohelp, ocat, ospec, odef, ocb, cbd)    \
{                                                                           \
  .base_option = OPTION_STR_INIT(&client_optset_static,                     \
                                 client_option_common_vtable,               \
                                 client_option_str_vtable, ocb, cbd),       \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .string = {                                                             \
      .pvalue = gui_options.oname,                                          \
      .size = sizeof(gui_options.oname),                                    \
      .def = odef,                                                          \
      .val_accessor = NULL                                                  \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_STRING with a string accessor
 * function.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 *        Be sure to pass the array variable and not a pointer to it because
 *        the size is calculated with sizeof().
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef:  The default string for this client option.
 * oacc:  The string accessor where to find the allowed values of type
 *        'const struct strvec * (*) (void)'.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_STR_LIST_OPTION(oname, odesc, ohelp, ocat, ospec, odef, oacc, ocb, cbd) \
{                                                                           \
  .base_option = OPTION_STR_INIT(&client_optset_static,                     \
                                 client_option_common_vtable,               \
                                 client_option_str_vtable, ocb, cbd),       \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .string = {                                                             \
      .pvalue = gui_options.oname,                                          \
      .size = sizeof(gui_options.oname),                                    \
      .def = odef,                                                          \
      .val_accessor = oacc                                                  \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_ENUM.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef:  The default value for this client option.
 * oacc:  The name accessor of type 'const struct copt_val_name * (*) (int)'.
 * ocb:   A callback function of type void (*) (struct option *) called when
 *        the option changed.
 */
#define GEN_ENUM_OPTION(oname, odesc, ohelp, ocat, ospec, odef, oacc, ocb)  \
{                                                                           \
  .base_option = OPTION_ENUM_INIT(&client_optset_static,                    \
                                  client_option_common_vtable,              \
                                  client_option_enum_vtable, ocb),          \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .enumerator = {                                                         \
      .pvalue = (int *) &gui_options.oname,                                 \
      .def = odef,                                                          \
      .support_names = NULL, /* Set in options_init(). */                   \
      .pretty_names  = NULL,                                                \
      .name_accessor = oacc                                                 \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_BITWISE.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef:  The default value for this client option.
 * oacc:  The name accessor of type 'const struct copt_val_name * (*) (int)'.
 * ocb:   A callback function of type void (*) (struct option *) called when
 *        the option changed.
 */
#define GEN_BITWISE_OPTION(oname, odesc, ohelp, ocat, ospec, odef, oacc,    \
                           ocb)                                             \
{                                                                           \
  .base_option = OPTION_BITWISE_INIT(&client_optset_static,                 \
                                     client_option_common_vtable,           \
                                     client_option_bitwise_vtable, ocb),    \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .bitwise = {                                                            \
      .pvalue = &gui_options.oname,                                         \
      .def = odef,                                                          \
      .support_names = NULL, /* Set in options_init(). */                   \
      .pretty_names  = NULL,                                                \
      .name_accessor = oacc                                                 \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_FONT.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 *        Be sure to pass the array variable and not a pointer to it because
 *        the size is calculated with sizeof().
 * otgt:  The target widget style.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef:  The default string for this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_FONT_OPTION(oname, otgt, odesc, ohelp, ocat, ospec, odef, ocb)  \
{                                                                           \
  .base_option = OPTION_FONT_INIT(&client_optset_static,                    \
                                  client_option_common_vtable,              \
                                  client_option_font_vtable, ocb),          \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .font = {                                                               \
      .pvalue = gui_options.oname,                                          \
      .size = sizeof(gui_options.oname),                                    \
      .def = odef,                                                          \
      .target = otgt,                                                       \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_COLOR.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef_fg, odef_bg:  The default values for this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_COLOR_OPTION(oname, odesc, ohelp, ocat, ospec, odef_fg,         \
                         odef_bg, ocb)                                      \
{                                                                           \
  .base_option = OPTION_COLOR_INIT(&client_optset_static,                   \
                                   client_option_common_vtable,             \
                                   client_option_color_vtable, ocb),        \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .color = {                                                              \
      .pvalue = &gui_options.oname,                                         \
      .def = FT_COLOR(odef_fg, odef_bg)                                     \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/*
 * Generate a client option of type OT_VIDEO_MODE.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef_width, odef_height:  The default values for this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_VIDEO_OPTION(oname, odesc, ohelp, ocat, ospec, odef_width,      \
                         odef_height, ocb)                                  \
{                                                                           \
  .base_option = OPTION_VIDEO_MODE_INIT(&client_optset_static,              \
                                        client_option_common_vtable,        \
                                        client_option_video_mode_vtable,    \
                                        ocb),                               \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  INIT_BRACE_BEGIN                                                          \
    .video_mode = {                                                         \
      .pvalue = &gui_options.oname,                                         \
      .def = VIDEO_MODE(odef_width, odef_height)                            \
    }                                                                       \
  INIT_BRACE_END                                                            \
}

/****************************************************************************
  Enumerator name accessors.
****************************************************************************/

/************************************************************************//**
  GTK message/chat layout setting names accessor.
****************************************************************************/
static const struct copt_val_name
  *gui_gtk_message_chat_location_name(int value)
{
  /* Order must match enum GUI_GTK_MSGCHAT_* */
  static const struct copt_val_name names[] = {
    /* TRANS: enum value for 'gui_gtk2/gtk3/gtk3x_message_chat_location' */
    { "SPLIT",    N_("Split") },
    /* TRANS: enum value for 'gui_gtk2/gtk3/gtk3x_message_chat_location' */
    { "SEPARATE", N_("Separate") },
    /* TRANS: enum value for 'gui_gtk2/gtk3/gtk3x_message_chat_location' */
    { "MERGED",   N_("Merged") }
  };

  return (0 <= value && value < ARRAY_SIZE(names)
          ? names + value : NULL);
}

/************************************************************************//**
  Popup tech help setting names accessor.
****************************************************************************/
static const struct copt_val_name
  *gui_popup_tech_help_name(int value)
{
  /* Order must match enum GUI_POPUP_TECH_HELP_* */
  static const struct copt_val_name names[] = {
    /* TRANS: enum value for 'gui_popup_tech_help' */
    { "ENABLED",   N_("Enabled") },
    /* TRANS: enum value for 'gui_popup_tech_help' */
    { "DISABLED",  N_("Disabled") },
    /* TRANS: enum value for 'gui_popup_tech_help' */
    { "RULESET",   N_("Ruleset") }
  };

  return (0 <= value && value < ARRAY_SIZE(names)
          ? names + value : NULL);
}

/* Some changed callbacks. */
static void reqtree_show_icons_callback(struct option *poption);
static void view_option_changed_callback(struct option *poption);
static void manual_turn_done_callback(struct option *poption);
static void voteinfo_bar_callback(struct option *poption);
static void font_changed_callback(struct option *poption);
static void mapimg_changed_callback(struct option *poption);
static void game_music_enable_callback(struct option *poption);
static void menu_music_enable_callback(struct option *poption);

static struct client_option client_options[] = {
  GEN_STR_OPTION(default_user_name,
                 N_("Login name"),
                 N_("This is the default login username that will be used "
                    "in the connection dialogs or with the -a command-line "
                    "parameter."),
                 COC_NETWORK, GUI_STUB, NULL, NULL, 0),
  GEN_BOOL_OPTION(use_prev_server, N_("Default to previously used server"),
                  N_("Automatically update \"Server\" and \"Server port\" "
                     "options to match your latest connection, so by "
                     "default you connect to the same server you used "
                     "on the previous run. You should enable "
                     "saving options on exit too, so that the automatic "
                     "updates to the options get saved too."),
                  COC_NETWORK, GUI_STUB, NULL, NULL),
  GEN_STR_OPTION(default_server_host,
                 N_("Server"),
                 N_("This is the default server hostname that will be used "
                    "in the connection dialogs or with the -a command-line "
                    "parameter."),
                 COC_NETWORK, GUI_STUB, "localhost", NULL, 0),
  GEN_INT_OPTION(default_server_port,
                 N_("Server port"),
                 N_("This is the default server port that will be used "
                    "in the connection dialogs or with the -a command-line "
                    "parameter."),
                 COC_NETWORK, GUI_STUB, DEFAULT_SOCK_PORT, 0, 65535, NULL),
  GEN_STR_OPTION(default_metaserver,
                 N_("Metaserver"),
                 N_("The metaserver is a host that the client contacts to "
                    "find out about games on the internet.  Don't change "
                    "this from its default value unless you know what "
                    "you're doing."),
                 COC_NETWORK, GUI_STUB, DEFAULT_METASERVER_OPTION, NULL, 0),
  GEN_BOOL_OPTION(heartbeat_enabled, N_("Send heartbeat messages to server"),
                  N_("Periodically send an empty heartbeat message to the "
                     "server to probe whether the connection is still up. "
                     "This can help to make it obvious when the server has "
                     "cut the connection due to a connectivity outage, if "
                     "the client would otherwise sit idle for a long period."),
                  COC_NETWORK, GUI_STUB, TRUE, NULL),
  GEN_STR_LIST_OPTION(default_sound_set_name,
                      N_("Soundset"),
                      N_("This is the soundset that will be used.  Changing "
                         "this is the same as using the -S command-line "
                         "parameter."),
                      COC_SOUND, GUI_STUB, "stdsounds", get_soundset_list, NULL, 0),
  GEN_STR_LIST_OPTION(default_music_set_name,
                      N_("Musicset"),
                      N_("This is the musicset that will be used.  Changing "
                         "this is the same as using the -m command-line "
                         "parameter."),
                      COC_SOUND, GUI_STUB, "stdmusic", get_musicset_list, musicspec_reread_callback, 0),
  GEN_STR_LIST_OPTION(default_sound_plugin_name,
                      N_("Sound plugin"),
                      N_("If you have a problem with sound, try changing "
                         "the sound plugin.  The new plugin won't take "
                         "effect until you restart Freeciv.  Changing this "
                         "is the same as using the -P command-line option."),
                      COC_SOUND, GUI_STUB, NULL, get_soundplugin_list, NULL, 0),
  GEN_STR_OPTION(default_chat_logfile,
                 N_("The chat log file"),
                 N_("The name of the chat log file."),
                 COC_INTERFACE, GUI_STUB, GUI_DEFAULT_CHAT_LOGFILE, NULL, 0),
  /* gui_gtk3/4_default_theme_name and gui_sdl2_default_theme_name are
   * different settings to avoid client crash after loading the
   * style for the other gui.  Keeps 5 different options! */
  GEN_STR_LIST_OPTION(gui_gtk3_default_theme_name, N_("Theme"),
                      N_("By changing this option you change the "
                         "active theme."),
                      COC_GRAPHICS, GUI_GTK3, FC_GTK3_DEFAULT_THEME_NAME,
                      get_themes_list, theme_reread_callback, 0),
  GEN_STR_LIST_OPTION(gui_gtk3_22_default_theme_name, N_("Theme"),
                      N_("By changing this option you change the "
                         "active theme."),
                      COC_GRAPHICS, GUI_GTK3_22, FC_GTK3_22_DEFAULT_THEME_NAME,
                      get_themes_list, theme_reread_callback, 0),
  GEN_STR_LIST_OPTION(gui_gtk4_default_theme_name, N_("Theme"),
                      N_("By changing this option you change the "
                         "active theme."),
                      COC_GRAPHICS, GUI_GTK3x, FC_GTK4_DEFAULT_THEME_NAME,
                      get_themes_list, theme_reread_callback, 0),
  GEN_STR_LIST_OPTION(gui_sdl2_default_theme_name, N_("Theme"),
                      N_("By changing this option you change the "
                         "active theme."),
                      COC_GRAPHICS, GUI_SDL2, FC_SDL2_DEFAULT_THEME_NAME,
                      get_themes_list, theme_reread_callback, 0),
  GEN_STR_LIST_OPTION(gui_qt_default_theme_name, N_("Theme"),
                      N_("By changing this option you change the "
                         "active theme."),
                      COC_GRAPHICS, GUI_QT, FC_QT_DEFAULT_THEME_NAME,
                      get_themes_list, theme_reread_callback, 0),

  /* It's important to give empty string instead of NULL as as default
   * value. For NULL value it would default to assigning first value
   * from the tileset list returned by get_tileset_list() as default
   * tileset. We don't want default tileset assigned at all here, but
   * leave it to tilespec code that can handle tileset priority. */
  GEN_STR_LIST_OPTION(default_tileset_square_name, N_("Tileset (Square)"),
                      N_("Select the tileset used with Square based maps. "
                         "This may change currently active tileset, if "
                         "you are playing on such a map, in which "
                         "case this is the same as using the -t "
                         "command-line parameter."),
                      COC_GRAPHICS, GUI_STUB, "",
                      get_tileset_list, tilespec_reread_callback, 0),
  GEN_STR_LIST_OPTION(default_tileset_hex_name, N_("Tileset (Hex)"),
                      N_("Select the tileset used with Hex maps. "
                         "This may change currently active tileset, if "
                         "you are playing on such a map, in which "
                         "case this is the same as using the -t "
                         "command-line parameter."),
                      COC_GRAPHICS, GUI_STUB, "",
                      get_tileset_list, tilespec_reread_callback, TF_HEX),
  GEN_STR_LIST_OPTION(default_tileset_isohex_name, N_("Tileset (Isometric Hex)"),
                      N_("Select the tileset used with Isometric Hex maps. "
                         "This may change currently active tileset, if "
                         "you are playing on such a map, in which "
                         "case this is the same as using the -t "
                         "command-line parameter."),
                      COC_GRAPHICS, GUI_STUB, "",
                      get_tileset_list, tilespec_reread_callback, TF_ISO | TF_HEX),

  GEN_BOOL_OPTION(draw_city_outlines, N_("Draw city outlines"),
                  N_("Setting this option will draw a line at the city "
                     "workable limit."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_output, N_("Draw city output"),
                  N_("Setting this option will draw city output for every "
                     "citizen."),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_map_grid, N_("Draw the map grid"),
                  N_("Setting this option will draw a grid over the map."),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_full_citybar, N_("Draw the city bar"),
                  N_("Setting this option will display a 'city bar' "
                     "containing useful information beneath each city. "
                     "Disabling this option will display only the city's "
                     "name and, optionally, production."),
                  COC_GRAPHICS, GUI_STUB,
                  TRUE, view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_names, N_("Draw the city names"),
                  N_("Setting this option will draw the names of the cities "
                     "on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_growth, N_("Draw the city growth"),
                  N_("Setting this option will draw in how many turns the "
                     "cities will grow or shrink."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_productions, N_("Draw the city productions"),
                  N_("Setting this option will draw what the cities are "
                     "currently building on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_buycost, N_("Draw the city buy costs"),
                  N_("Setting this option will draw how much gold is "
                     "needed to buy the production of the cities."),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_city_trade_routes, N_("Draw the city trade routes"),
                  N_("Setting this option will draw trade route lines "
                     "between cities which have trade routes."),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_terrain, N_("Draw the terrain"),
                  N_("Setting this option will draw the terrain."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_coastline, N_("Draw the coast line"),
                  N_("Setting this option will draw a line to separate the "
                     "land from the ocean."),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_roads_rails, N_("Draw the roads and the railroads"),
                  N_("Setting this option will draw the roads and the "
                     "railroads on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_irrigation, N_("Draw the irrigation"),
                  N_("Setting this option will draw the irrigation systems "
                     "on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_mines, N_("Draw the mines"),
                  N_("Setting this option will draw the mines on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_fortress_airbase, N_("Draw the bases"),
                  N_("Setting this option will draw the bases on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_specials, N_("Draw the resources"),
                  N_("Setting this option will draw the resources on the "
                     "map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_huts, N_("Draw the huts"),
                  N_("Setting this option will draw the huts on the "
                     "map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_pollution, N_("Draw the pollution/nuclear fallout"),
                  N_("Setting this option will draw pollution and "
                     "nuclear fallout on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_cities, N_("Draw the cities"),
                  N_("Setting this option will draw the cities on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_units, N_("Draw the units"),
                  N_("Setting this option will draw the units on the map."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(solid_color_behind_units,
                  N_("Solid unit background color"),
                  N_("Setting this option will cause units on the map "
                     "view to be drawn with a solid background color "
                     "instead of the flag backdrop."),
                  COC_GRAPHICS, GUI_STUB,
                  FALSE, view_option_changed_callback),
  GEN_BOOL_OPTION(draw_unit_shields, N_("Draw shield graphics for units"),
                  N_("Setting this option will draw a shield icon "
                     "as the flags on units.  If unset, the full flag will "
                     "be drawn."),
                  COC_GRAPHICS, GUI_STUB, TRUE, view_option_changed_callback),
  GEN_BOOL_OPTION(draw_focus_unit, N_("Draw the units in focus"),
                  N_("Setting this option will cause the currently focused "
                     "unit(s) to always be drawn, even if units are not "
                     "otherwise being drawn (for instance if 'Draw the units' "
                     "is unset)."),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_fog_of_war, N_("Draw the fog of war"),
                  N_("Setting this option will draw the fog of war."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_borders, N_("Draw the borders"),
                  N_("Setting this option will draw the national borders."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(draw_native, N_("Draw whether tiles are native to "
                                  "selected unit"),
                  N_("Setting this option will highlight tiles that the "
                     "currently selected unit cannot enter unaided due to "
                     "non-native terrain. (If multiple units are selected, "
                     "only tiles that all of them can enter are indicated.)"),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(player_dlg_show_dead_players,
                  N_("Show dead players in Nations report"),
                  N_("This option controls whether defeated nations are "
                     "shown on the Nations report page."),
                  COC_GRAPHICS, GUI_STUB, TRUE,
                  view_option_changed_callback),
  GEN_BOOL_OPTION(sound_bell_at_new_turn, N_("Sound bell at new turn"),
                  N_("Set this option to have a \"bell\" event be generated "
                     "at the start of a new turn.  You can control the "
                     "behavior of the \"bell\" event by editing the message "
                     "options."),
                  COC_SOUND, GUI_STUB, FALSE, NULL),
  GEN_INT_OPTION(smooth_move_unit_msec,
                 N_("Unit movement animation time (milliseconds)"),
                 N_("This option controls how long unit \"animation\" takes "
                    "when a unit moves on the map view.  Set it to 0 to "
                    "disable animation entirely."),
                 COC_GRAPHICS, GUI_STUB, 30, 0, 2000, NULL),
  GEN_INT_OPTION(smooth_center_slide_msec,
                 N_("Mapview recentering time (milliseconds)"),
                 N_("When the map view is recentered, it will slide "
                    "smoothly over the map to its new position.  This "
                    "option controls how long this slide lasts.  Set it to "
                    "0 to disable mapview sliding entirely."),
                 COC_GRAPHICS, GUI_STUB, 200, 0, 5000, NULL),
  GEN_INT_OPTION(smooth_combat_step_msec,
                 N_("Combat animation step time (milliseconds)"),
                 N_("This option controls the speed of combat animation "
                    "between units on the mapview.  Set it to 0 to disable "
                    "animation entirely."),
                 COC_GRAPHICS, GUI_STUB, 10, 0, 100, NULL),
  GEN_BOOL_OPTION(reqtree_show_icons,
                  N_("Show icons in the technology tree"),
                  N_("Setting this option will display icons "
                     "on the technology tree diagram. Turning "
                     "this option off makes the technology tree "
                     "more compact."),
                  COC_GRAPHICS, GUI_STUB, TRUE, reqtree_show_icons_callback),
  GEN_BOOL_OPTION(reqtree_curved_lines,
                  N_("Use curved lines in the technology tree"),
                  N_("Setting this option make the technology tree "
                     "diagram use curved lines to show technology "
                     "relations. Turning this option off causes "
                     "the lines to be drawn straight."),
                  COC_GRAPHICS, GUI_STUB, FALSE,
                  reqtree_show_icons_callback),
   GEN_COLOR_OPTION(highlight_our_names,
                    N_("Color to highlight your player/user name"),
                    N_("If set, your player and user name in the new chat "
                       "messages will be highlighted using this color as "
                       "background.  If not set, it will just not highlight "
                       "anything."),
                    COC_GRAPHICS, GUI_STUB, "#000000", "#FFFF00", NULL),
  GEN_BOOL_OPTION(ai_manual_turn_done, N_("Manual Turn Done in AI mode"),
                  N_("Disable this option if you do not want to "
                     "press the Turn Done button manually when watching "
                     "an AI player."),
                  COC_INTERFACE, GUI_STUB, TRUE, manual_turn_done_callback),
  GEN_BOOL_OPTION(auto_center_on_unit, N_("Auto center on units"),
                  N_("Set this option to have the active unit centered "
                     "automatically when the unit focus changes."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(auto_center_on_automated, N_("Show automated units"),
                  N_("Disable this option if you do not want to see "
                     "automated units autocentered and animated."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(auto_center_on_combat, N_("Auto center on combat"),
                  N_("Set this option to have any combat be centered "
                     "automatically.  Disabling this will speed up the time "
                     "between turns but may cause you to miss combat "
                     "entirely."),
                  COC_INTERFACE, GUI_STUB, FALSE, NULL),
  GEN_BOOL_OPTION(auto_center_each_turn, N_("Auto center on new turn"),
                  N_("Set this option to have the client automatically "
                     "recenter the map on a suitable location at the "
                     "start of each turn."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(wakeup_focus, N_("Focus on awakened units"),
                  N_("Set this option to have newly awoken units be "
                     "focused automatically."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(keyboardless_goto, N_("Keyboardless goto"),
                  N_("If this option is set then a goto may be initiated "
                     "by left-clicking and then holding down the mouse "
                     "button while dragging the mouse onto a different "
                     "tile."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(goto_into_unknown, N_("Allow goto into the unknown"),
                  N_("Setting this option will make the game consider "
                     "moving into unknown tiles.  If not, then goto routes "
                     "will detour around or be blocked by unknown tiles."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(center_when_popup_city, N_("Center map when popup city"),
                  N_("Setting this option makes the mapview center on a "
                     "city when its city dialog is popped up."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(show_previous_turn_messages, N_("Show messages from previous turn"),
                  N_("Message Window shows messages also from previous turn. "
                     "This makes sure you don't miss messages received in the end of "
                     "the turn, just before the window gets cleared."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(concise_city_production, N_("Concise city production"),
                  N_("Set this option to make the city production (as shown "
                     "in the city dialog) to be more compact."),
                  COC_INTERFACE, GUI_STUB, FALSE, NULL),
  GEN_BOOL_OPTION(auto_turn_done, N_("End turn when done moving"),
                  N_("Setting this option makes your turn end automatically "
                     "when all your units are done moving."),
                  COC_INTERFACE, GUI_STUB, FALSE, NULL),
  GEN_BOOL_OPTION(ask_city_name, N_("Prompt for city names"),
                  N_("Disabling this option will make the names of newly "
                     "founded cities be chosen automatically by the server."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(popup_new_cities, N_("Pop up city dialog for new cities"),
                  N_("Setting this option will pop up a newly-founded "
                     "city's city dialog automatically."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(popup_actor_arrival, N_("Pop up caravan and spy actions"),
                  N_("If this option is enabled, when a unit arrives at "
                     "a city where it can perform an action like "
                     "establishing a trade route, helping build a wonder, or "
                     "establishing an embassy, a window will pop up asking "
                     "which action should be performed. "
                     "Disabling this option means you will have to do the "
                     "action manually by pressing either 'r' (for a trade "
                     "route), 'b' (for building a wonder) or 'd' (for a "
                     "spy action) when the unit is in the city."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(popup_attack_actions, N_("Pop up attack questions"),
                  N_("If this option is enabled, when a unit arrives at a "
                     "target it can attack, a window will pop up asking "
                     "which action should be performed even if an attack "
                     "action is legal and no other interesting action are. "
                     "This allows you to change your mind or to select an "
                     "uninteresting action."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(enable_cursor_changes, N_("Enable cursor changing"),
                  N_("This option controls whether the client should "
                     "try to change the mouse cursor depending on what "
                     "is being pointed at, as well as to indicate "
                     "changes in the client or server state."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(separate_unit_selection, N_("Select cities before units"),
                  N_("If this option is enabled, when both cities and "
                     "units are present in the selection rectangle, only "
                     "cities will be selected. See the help on Controls."),
                  COC_INTERFACE, GUI_STUB, FALSE, NULL),
  GEN_BOOL_OPTION(unit_selection_clears_orders,
                  N_("Clear unit orders on selection"),
                  N_("Enabling this option will cause unit orders to be "
                     "cleared as soon as one or more units are selected. If "
                     "this option is disabled, busy units will not stop "
                     "their current activity when selected. Giving them "
                     "new orders will clear their current ones; pressing "
                     "<space> once will clear their orders and leave them "
                     "selected, and pressing <space> a second time will "
                     "dismiss them."),
                  COC_INTERFACE, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(voteinfo_bar_use, N_("Enable vote bar"),
                  N_("If this option is turned on, the vote bar will be "
                     "displayed to show vote information."),
                  COC_GRAPHICS, GUI_STUB, TRUE, voteinfo_bar_callback),
  GEN_BOOL_OPTION(voteinfo_bar_always_show,
                  N_("Always display the vote bar"),
                  N_("If this option is turned on, the vote bar will never "
                     "be hidden, even if there is no running vote."),
                  COC_GRAPHICS, GUI_STUB, FALSE, voteinfo_bar_callback),
  GEN_BOOL_OPTION(voteinfo_bar_hide_when_not_player,
                  N_("Do not show vote bar if not a player"),
                  N_("If this option is enabled, the client won't show the "
                     "vote bar if you are not a player."),
                  COC_GRAPHICS, GUI_STUB, FALSE, voteinfo_bar_callback),
  GEN_BOOL_OPTION(voteinfo_bar_new_at_front, N_("Set new votes at front"),
                  N_("If this option is enabled, then new votes will go "
                     "to the front of the vote list."),
                  COC_GRAPHICS, GUI_STUB, FALSE, voteinfo_bar_callback),
  GEN_BOOL_OPTION(autoaccept_tileset_suggestion,
                  N_("Autoaccept tileset suggestions"),
                  N_("If this option is enabled, any tileset suggested by "
                     "the ruleset is automatically used; otherwise you "
                     "are prompted to change tileset."),
                  COC_GRAPHICS, GUI_STUB, FALSE, NULL),

  GEN_BOOL_OPTION(sound_enable_effects,
                  N_("Enable sound effects"),
                  N_("Play sound effects, assuming there's suitable "
                     "sound plugin and soundset with the sounds."),
                  COC_SOUND, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(sound_enable_game_music,
                  N_("Enable in-game music"),
                  N_("Play music during the game, assuming there's suitable "
                     "sound plugin and musicset with in-game tracks."),
                  COC_SOUND, GUI_STUB, TRUE, game_music_enable_callback),
 GEN_BOOL_OPTION(sound_enable_menu_music,
                  N_("Enable menu music"),
                  N_("Play music while not in actual game, "
                     "assuming there's suitable "
                     "sound plugin and musicset with menu music tracks."),
                  COC_SOUND, GUI_STUB, TRUE, menu_music_enable_callback),
  GEN_BOOL_OPTION(autoaccept_soundset_suggestion,
                  N_("Autoaccept soundset suggestions"),
                  N_("If this option is enabled, any soundset suggested by "
                     "the ruleset is automatically used."),
                  COC_SOUND, GUI_STUB, FALSE, NULL),
  GEN_BOOL_OPTION(autoaccept_musicset_suggestion,
                  N_("Autoaccept musicset suggestions"),
                  N_("If this option is enabled, any musicset suggested by "
                     "the ruleset is automatically used."),
                  COC_SOUND, GUI_STUB, FALSE, NULL),

  GEN_BOOL_OPTION(overview.layers[OLAYER_BACKGROUND],
                  N_("Background layer"),
                  N_("The background layer of the overview shows just "
                     "ocean and land."),
                  COC_OVERVIEW, GUI_STUB, TRUE, NULL),
  GEN_BOOL_OPTION(overview.layers[OLAYER_RELIEF],
                  N_("Terrain relief map layer"),
                  N_("The relief layer shows all terrains on the map."),
                  COC_OVERVIEW, GUI_STUB, FALSE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_BORDERS],
                  N_("Borders layer"),
                  N_("The borders layer of the overview shows which tiles "
                     "are owned by each player."),
                  COC_OVERVIEW, GUI_STUB, FALSE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_BORDERS_ON_OCEAN],
                  N_("Borders layer on ocean tiles"),
                  N_("The borders layer of the overview are drawn on "
                     "ocean tiles as well (this may look ugly with many "
                     "islands). This option is only of interest if you "
                     "have set the option \"Borders layer\" already."),
                  COC_OVERVIEW, GUI_STUB, TRUE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_UNITS],
                  N_("Units layer"),
                  N_("Enabling this will draw units on the overview."),
                  COC_OVERVIEW, GUI_STUB, TRUE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.layers[OLAYER_CITIES],
                  N_("Cities layer"),
                  N_("Enabling this will draw cities on the overview."),
                  COC_OVERVIEW, GUI_STUB, TRUE, overview_redraw_callback),
  GEN_BOOL_OPTION(overview.fog,
                  N_("Overview fog of war"),
                  N_("Enabling this will show fog of war on the "
                     "overview."),
                  COC_OVERVIEW, GUI_STUB, TRUE, overview_redraw_callback),

  /* options for map images */
  GEN_STR_LIST_OPTION(mapimg_format,
                      N_("Image format"),
                      N_("The image toolkit and file format used for "
                         "map images."),
                      COC_MAPIMG, GUI_STUB, NULL, get_mapimg_format_list,
                      NULL, 0),
  GEN_INT_OPTION(mapimg_zoom,
                 N_("Zoom factor for map images"),
                 N_("The magnification used for map images."),
                 COC_MAPIMG, GUI_STUB, 2, 1, 5, mapimg_changed_callback),
  GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_AREA],
                  N_("Show area within borders"),
                  N_("If set, the territory of each nation is shown "
                     "on the saved image."),
                  COC_MAPIMG, GUI_STUB, FALSE, mapimg_changed_callback),
  GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_BORDERS],
                  N_("Show borders"),
                  N_("If set, the border of each nation is shown on the "
                     "saved image."),
                  COC_MAPIMG, GUI_STUB, TRUE, mapimg_changed_callback),
  GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_CITIES],
                  N_("Show cities"),
                  N_("If set, cities are shown on the saved image."),
                  COC_MAPIMG, GUI_STUB, TRUE, mapimg_changed_callback),
  GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_FOGOFWAR],
                  N_("Show fog of war"),
                  N_("If set, the extent of fog of war is shown on the "
                     "saved image."),
                  COC_MAPIMG, GUI_STUB, TRUE, mapimg_changed_callback),
  GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_TERRAIN],
                  N_("Show full terrain"),
                  N_("If set, terrain relief is shown with different colors "
                     "in the saved image; otherwise, only land and water are "
                     "distinguished."),
                  COC_MAPIMG, GUI_STUB, TRUE, mapimg_changed_callback),
  GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_UNITS],
                  N_("Show units"),
                  N_("If set, units are shown in the saved image."),
                  COC_MAPIMG, GUI_STUB, TRUE, mapimg_changed_callback),
  GEN_STR_OPTION(mapimg_filename,
                 N_("Map image file name"),
                 N_("The base part of the filename for saved map images. "
                    "A string identifying the game turn and map options will "
                    "be appended."),
                 COC_MAPIMG, GUI_STUB, GUI_DEFAULT_MAPIMG_FILENAME, NULL, 0),

  /* gui-gtk-2.0 client specific options.
   * These are still kept just so users can migrate them to gtk3-client */
  GEN_BOOL_OPTION(gui_gtk2_fullscreen, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_map_scrollbars, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_dialogs_on_top, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_show_task_icons, NULL, NULL,
                  COC_GRAPHICS, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_enable_tabs, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_better_fog, NULL, NULL,
                  COC_GRAPHICS, GUI_GTK2,
                  TRUE, view_option_changed_callback),
  GEN_BOOL_OPTION(gui_gtk2_show_chat_message_time, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_new_messages_go_to_top, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_show_message_window_buttons, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_metaserver_tab_first, NULL, NULL,
                  COC_NETWORK, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_allied_chat_only, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_ENUM_OPTION(gui_gtk2_message_chat_location, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2,
                  GUI_GTK_MSGCHAT_MERGED /* Ignored! See options_load(). */,
                  gui_gtk_message_chat_location_name, NULL),
  GEN_BOOL_OPTION(gui_gtk2_small_display_layout, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_mouse_over_map_focus, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk2_chatline_autocompletion, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2, TRUE, NULL),
  GEN_INT_OPTION(gui_gtk2_citydlg_xsize, NULL, NULL,
                 COC_INTERFACE, GUI_GTK2, GUI_GTK2_CITYDLG_DEFAULT_XSIZE,
                 GUI_GTK2_CITYDLG_MIN_XSIZE, GUI_GTK2_CITYDLG_MAX_XSIZE,
                 NULL),
  GEN_INT_OPTION(gui_gtk2_citydlg_ysize, NULL, NULL,
                 COC_INTERFACE, GUI_GTK2, GUI_GTK2_CITYDLG_DEFAULT_YSIZE,
                 GUI_GTK2_CITYDLG_MIN_YSIZE, GUI_GTK2_CITYDLG_MAX_YSIZE,
                 NULL),
  GEN_ENUM_OPTION(gui_gtk2_popup_tech_help, NULL, NULL,
                  COC_INTERFACE, GUI_GTK2,
                  GUI_POPUP_TECH_HELP_RULESET,
                  gui_popup_tech_help_name, NULL),
  GEN_FONT_OPTION(gui_gtk2_font_city_label, "city_label",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_notify_label, "notify_label",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Monospace Bold 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_spaceship_label, "spaceship_label",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_help_label, "help_label",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Sans Bold 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_help_link, "help_link",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Sans 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_help_text, "help_text",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_chatline, "chatline",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_beta_label, "beta_label",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Sans Italic 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_small, "small_font",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Sans 9", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_comment_label, "comment_label",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Sans Italic 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk2_font_city_names, "city_names",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Sans Bold 10", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_city_productions, "city_productions",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Serif 10", NULL),
  GEN_FONT_OPTION(gui_gtk2_font_reqtree_text, "reqtree_text",
                  NULL, NULL,
                  COC_FONT, GUI_GTK2,
                  "Serif 10", NULL),

  /* gui-gtk-3.0 client specific options. */
  GEN_BOOL_OPTION(gui_gtk3_fullscreen, N_("Fullscreen"),
                  N_("If this option is set the client will use the "
                     "whole screen area for drawing."),
                  COC_INTERFACE, GUI_GTK3, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_map_scrollbars, N_("Show map scrollbars"),
                  N_("Disable this option to hide the scrollbars on the "
                     "map view."),
                  COC_INTERFACE, GUI_GTK3, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_dialogs_on_top, N_("Keep dialogs on top"),
                  N_("If this option is set then dialog windows will always "
                     "remain in front of the main Freeciv window. "
                     "Disabling this has no effect in fullscreen mode."),
                  COC_INTERFACE, GUI_GTK3, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_show_task_icons, N_("Show worklist task icons"),
                  N_("Disabling this will turn off the unit and building "
                     "icons in the worklist dialog and the production "
                     "tab of the city dialog."),
                  COC_GRAPHICS, GUI_GTK3, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_enable_tabs, N_("Enable status report tabs"),
                  N_("If this option is enabled then report dialogs will "
                     "be shown as separate tabs rather than in popup "
                     "dialogs."),
                  COC_INTERFACE, GUI_GTK3, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_show_chat_message_time,
                  N_("Show time for each chat message"),
                  N_("If this option is enabled then all chat messages "
                     "will be prefixed by a time string of the form "
                     "[hour:minute:second]."),
                  COC_INTERFACE, GUI_GTK3, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_new_messages_go_to_top,
                  N_("New message events go to top of list"),
                  N_("If this option is enabled, new events in the "
                     "message window will appear at the top of the list, "
                     "rather than being appended at the bottom."),
                  COC_INTERFACE, GUI_GTK3, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_show_message_window_buttons,
                  N_("Show extra message window buttons"),
                  N_("If this option is enabled, there will be two "
                     "buttons displayed in the message window for "
                     "inspecting a city and going to a location. If this "
                     "option is disabled, these buttons will not appear "
                     "(you can still double-click with the left mouse "
                     "button or right-click on a row to inspect or goto "
                     "respectively). This option will only take effect "
                     "once the message window is closed and reopened."),
                  COC_INTERFACE, GUI_GTK3, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_metaserver_tab_first,
                  N_("Metaserver tab first in network page"),
                  N_("If this option is enabled, the metaserver tab will "
                     "be the first notebook tab in the network page. This "
                     "option requires a restart in order to take effect."),
                  COC_NETWORK, GUI_GTK3, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_allied_chat_only,
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
                  COC_INTERFACE, GUI_GTK3, FALSE, NULL),
  GEN_ENUM_OPTION(gui_gtk3_message_chat_location,
                  N_("Messages and Chat reports location"),
                  /* TRANS: The strings used in the UI for 'Split' etc are
                   * tagged 'gui_gtk2/gtk3/gtk3x_message_chat_location' */
                  N_("Controls where the Messages and Chat reports "
                     "appear relative to the main view containing the map.\n"
                     "'Split' allows all three to be seen simultaneously, "
                     "which is best for multiplayer, but requires a large "
                     "window to be usable.\n"
                     "'Separate' puts Messages and Chat in a notebook "
                     "separate from the main view, so that one of them "
                     "can always be seen alongside the main view.\n"
                     "'Merged' makes the Messages and Chat reports into "
                     "tabs alongside the map and other reports; this "
                     "allows a larger map view on small screens.\n"
                     "This option requires a restart in order to take "
                     "effect."), COC_INTERFACE, GUI_GTK3,
                  GUI_GTK_MSGCHAT_MERGED /* Ignored! See options_load(). */,
                  gui_gtk_message_chat_location_name, NULL),
  GEN_BOOL_OPTION(gui_gtk3_small_display_layout,
                  N_("Arrange widgets for small displays"),
                  N_("If this option is enabled, widgets in the main "
                     "window will be arranged so that they take up the "
                     "least amount of total screen space. Specifically, "
                     "the left panel containing the overview, player "
                     "status, and the unit information box will be "
                     "extended over the entire left side of the window. "
                     "This option requires a restart in order to take "
                     "effect."), COC_INTERFACE, GUI_GTK3, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_mouse_over_map_focus,
                  N_("Mouse over the map widget selects it automatically"),
                  N_("If this option is enabled, then the map will be "
                     "focused when the mouse hovers over it."),
                  COC_INTERFACE, GUI_GTK3, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_chatline_autocompletion,
                  N_("Player or user name autocompletion"),
                  N_("If this option is turned on, the tabulation key "
                     "will be used in the chatline to complete the word you "
                     "are typing with the name of a player or a user."),
                  COC_INTERFACE, GUI_GTK3, TRUE, NULL),
  GEN_INT_OPTION(gui_gtk3_citydlg_xsize,
                 N_("Width of the city dialog"),
                 N_("This value is only used if the width of the city "
                    "dialog is saved."),
                 COC_INTERFACE, GUI_GTK3, GUI_GTK3_CITYDLG_DEFAULT_XSIZE,
                 GUI_GTK3_CITYDLG_MIN_XSIZE, GUI_GTK3_CITYDLG_MAX_XSIZE,
                 NULL),
  GEN_INT_OPTION(gui_gtk3_citydlg_ysize,
                 N_("Height of the city dialog"),
                 N_("This value is only used if the height of the city "
                    "dialog is saved."),
                 COC_INTERFACE, GUI_GTK3, GUI_GTK3_CITYDLG_DEFAULT_YSIZE,
                 GUI_GTK3_CITYDLG_MIN_YSIZE, GUI_GTK3_CITYDLG_MAX_YSIZE,
                 NULL),
  GEN_ENUM_OPTION(gui_gtk3_popup_tech_help,
                  N_("Popup tech help when gained"),
                  N_("Controls if tech help should be opened when "
                     "new tech has been gained.\n"
                     "'Ruleset' means that behavior suggested by "
                     "current ruleset is used."), COC_INTERFACE, GUI_GTK3,
                  GUI_POPUP_TECH_HELP_RULESET,
                  gui_popup_tech_help_name, NULL),
  GEN_INT_OPTION(gui_gtk3_governor_range_min,
                 N_("Minimum surplus for a governor"),
                 N_("The lower limit of the range for requesting surpluses "
                    "from the governor."),
                 COC_INTERFACE, GUI_GTK3, GUI_GTK3_GOV_RANGE_MIN_DEFAULT,
                 GUI_GTK3_GOV_RANGE_MIN_MIN, GUI_GTK3_GOV_RANGE_MIN_MAX,
                 NULL),
  GEN_INT_OPTION(gui_gtk3_governor_range_max,
                 N_("Maximum surplus for a governor"),
                 N_("The higher limit of the range for requesting surpluses "
                    "from the governor."),
                 COC_INTERFACE, GUI_GTK3, GUI_GTK3_GOV_RANGE_MAX_DEFAULT,
                 GUI_GTK3_GOV_RANGE_MAX_MIN, GUI_GTK3_GOV_RANGE_MAX_MAX,
                 NULL),
  GEN_FONT_OPTION(gui_gtk3_font_city_label, "city_label",
                  N_("City Label"),
                  N_("This font is used to display the city labels on city "
                     "dialogs."),
                  COC_FONT, GUI_GTK3,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_notify_label, "notify_label",
                  N_("Notify Label"),
                  N_("This font is used to display server reports such "
                     "as the demographic report or historian publications."),
                  COC_FONT, GUI_GTK3,
                  "Monospace Bold 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_spaceship_label, "spaceship_label",
                  N_("Spaceship Label"),
                  N_("This font is used to display the spaceship widgets."),
                  COC_FONT, GUI_GTK3,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_help_label, "help_label",
                  N_("Help Label"),
                  N_("This font is used to display the help headers in the "
                     "help window."),
                  COC_FONT, GUI_GTK3,
                  "Sans Bold 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_help_link, "help_link",
                  N_("Help Link"),
                  N_("This font is used to display the help links in the "
                     "help window."),
                  COC_FONT, GUI_GTK3,
                  "Sans 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_help_text, "help_text",
                  N_("Help Text"),
                  N_("This font is used to display the help body text in "
                     "the help window."),
                  COC_FONT, GUI_GTK3,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_chatline, "chatline",
                  N_("Chatline Area"),
                  N_("This font is used to display the text in the "
                     "chatline area."),
                  COC_FONT, GUI_GTK3,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_beta_label, "beta_label",
                  N_("Beta Label"),
                  N_("This font is used to display the beta label."),
                  COC_FONT, GUI_GTK3,
                  "Sans Italic 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_small, "small_font",
                  N_("Small Font"),
                  N_("This font is used for any small font request.  For "
                     "example, it is used for display the building lists "
                     "in the city dialog, the Economy report or the Units "
                     "report."),
                  COC_FONT, GUI_GTK3,
                  "Sans 9", NULL),
  GEN_FONT_OPTION(gui_gtk3_font_comment_label, "comment_label",
                  N_("Comment Label"),
                  N_("This font is used to display comment labels, such as "
                     "in the governor page of the city dialogs."),
                  COC_FONT, GUI_GTK3,
                  "Sans Italic 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_font_city_names, "city_names",
                  N_("City Names"),
                  N_("This font is used to the display the city names "
                     "on the map."),
                  COC_FONT, GUI_GTK3,
                  "Sans Bold 10", NULL),
  GEN_FONT_OPTION(gui_gtk3_font_city_productions, "city_productions",
                  N_("City Productions"),
                  N_("This font is used to display the city production "
                     "on the map."),
                  COC_FONT, GUI_GTK3,
                  "Serif 10", NULL),
  GEN_FONT_OPTION(gui_gtk3_font_reqtree_text, "reqtree_text",
                  N_("Requirement Tree"),
                  N_("This font is used to the display the requirement tree "
                     "in the Research report."),
                  COC_FONT, GUI_GTK3,
                  "Serif 10", NULL),

  /* gui-gtk-3.22 client specific options. */
  GEN_BOOL_OPTION(gui_gtk3_22_fullscreen, N_("Fullscreen"),
                  N_("If this option is set the client will use the "
                     "whole screen area for drawing."),
                  COC_INTERFACE, GUI_GTK3_22, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_map_scrollbars, N_("Show map scrollbars"),
                  N_("Disable this option to hide the scrollbars on the "
                     "map view."),
                  COC_INTERFACE, GUI_GTK3_22, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_dialogs_on_top, N_("Keep dialogs on top"),
                  N_("If this option is set then dialog windows will always "
                     "remain in front of the main Freeciv window. "
                     "Disabling this has no effect in fullscreen mode."),
                  COC_INTERFACE, GUI_GTK3_22, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_show_task_icons, N_("Show worklist task icons"),
                  N_("Disabling this will turn off the unit and building "
                     "icons in the worklist dialog and the production "
                     "tab of the city dialog."),
                  COC_GRAPHICS, GUI_GTK3_22, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_enable_tabs, N_("Enable status report tabs"),
                  N_("If this option is enabled then report dialogs will "
                     "be shown as separate tabs rather than in popup "
                     "dialogs."),
                  COC_INTERFACE, GUI_GTK3_22, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_show_chat_message_time,
                  N_("Show time for each chat message"),
                  N_("If this option is enabled then all chat messages "
                     "will be prefixed by a time string of the form "
                     "[hour:minute:second]."),
                  COC_INTERFACE, GUI_GTK3_22, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_new_messages_go_to_top,
                  N_("New message events go to top of list"),
                  N_("If this option is enabled, new events in the "
                     "message window will appear at the top of the list, "
                     "rather than being appended at the bottom."),
                  COC_INTERFACE, GUI_GTK3_22, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_show_message_window_buttons,
                  N_("Show extra message window buttons"),
                  N_("If this option is enabled, there will be two "
                     "buttons displayed in the message window for "
                     "inspecting a city and going to a location. If this "
                     "option is disabled, these buttons will not appear "
                     "(you can still double-click with the left mouse "
                     "button or right-click on a row to inspect or goto "
                     "respectively). This option will only take effect "
                     "once the message window is closed and reopened."),
                  COC_INTERFACE, GUI_GTK3_22, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_metaserver_tab_first,
                  N_("Metaserver tab first in network page"),
                  N_("If this option is enabled, the metaserver tab will "
                     "be the first notebook tab in the network page. This "
                     "option requires a restart in order to take effect."),
                  COC_NETWORK, GUI_GTK3_22, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_allied_chat_only,
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
                  COC_INTERFACE, GUI_GTK3_22, FALSE, NULL),
  GEN_ENUM_OPTION(gui_gtk3_22_message_chat_location,
                  N_("Messages and Chat reports location"),
                  /* TRANS: The strings used in the UI for 'Split' etc are
                   * tagged 'gui_gtk2/gtk3/gtk3x_message_chat_location' */
                  N_("Controls where the Messages and Chat reports "
                     "appear relative to the main view containing the map.\n"
                     "'Split' allows all three to be seen simultaneously, "
                     "which is best for multiplayer, but requires a large "
                     "window to be usable.\n"
                     "'Separate' puts Messages and Chat in a notebook "
                     "separate from the main view, so that one of them "
                     "can always be seen alongside the main view.\n"
                     "'Merged' makes the Messages and Chat reports into "
                     "tabs alongside the map and other reports; this "
                     "allows a larger map view on small screens.\n"
                     "This option requires a restart in order to take "
                     "effect."), COC_INTERFACE, GUI_GTK3_22,
                  GUI_GTK_MSGCHAT_MERGED /* Ignored! See options_load(). */,
                  gui_gtk_message_chat_location_name, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_small_display_layout,
                  N_("Arrange widgets for small displays"),
                  N_("If this option is enabled, widgets in the main "
                     "window will be arranged so that they take up the "
                     "least amount of total screen space. Specifically, "
                     "the left panel containing the overview, player "
                     "status, and the unit information box will be "
                     "extended over the entire left side of the window. "
                     "This option requires a restart in order to take "
                     "effect."), COC_INTERFACE, GUI_GTK3_22, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_mouse_over_map_focus,
                  N_("Mouse over the map widget selects it automatically"),
                  N_("If this option is enabled, then the map will be "
                     "focused when the mouse hovers over it."),
                  COC_INTERFACE, GUI_GTK3_22, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk3_22_chatline_autocompletion,
                  N_("Player or user name autocompletion"),
                  N_("If this option is turned on, the tabulation key "
                     "will be used in the chatline to complete the word you "
                     "are typing with the name of a player or a user."),
                  COC_INTERFACE, GUI_GTK3_22, TRUE, NULL),
  GEN_INT_OPTION(gui_gtk3_22_citydlg_xsize,
                 N_("Width of the city dialog"),
                 N_("This value is only used if the width of the city "
                    "dialog is saved."),
                 COC_INTERFACE, GUI_GTK3_22, GUI_GTK3_22_CITYDLG_DEFAULT_XSIZE,
                 GUI_GTK3_22_CITYDLG_MIN_XSIZE, GUI_GTK3_22_CITYDLG_MAX_XSIZE,
                 NULL),
  GEN_INT_OPTION(gui_gtk3_22_citydlg_ysize,
                 N_("Height of the city dialog"),
                 N_("This value is only used if the height of the city "
                    "dialog is saved."),
                 COC_INTERFACE, GUI_GTK3_22, GUI_GTK3_22_CITYDLG_DEFAULT_YSIZE,
                 GUI_GTK3_22_CITYDLG_MIN_YSIZE, GUI_GTK3_22_CITYDLG_MAX_YSIZE,
                 NULL),
  GEN_ENUM_OPTION(gui_gtk3_22_popup_tech_help,
                  N_("Popup tech help when gained"),
                  N_("Controls if tech help should be opened when "
                     "new tech has been gained.\n"
                     "'Ruleset' means that behavior suggested by "
                     "current ruleset is used."), COC_INTERFACE, GUI_GTK3_22,
                  GUI_POPUP_TECH_HELP_RULESET,
                  gui_popup_tech_help_name, NULL),
  GEN_INT_OPTION(gui_gtk3_22_governor_range_min,
                 N_("Minimum surplus for a governor"),
                 N_("The lower limit of the range for requesting surpluses "
                    "from the governor."),
                 COC_INTERFACE, GUI_GTK3_22, GUI_GTK3_22_GOV_RANGE_MIN_DEFAULT,
                 GUI_GTK3_22_GOV_RANGE_MIN_MIN, GUI_GTK3_22_GOV_RANGE_MIN_MAX,
                 NULL),
  GEN_INT_OPTION(gui_gtk3_22_governor_range_max,
                 N_("Maximum surplus for a governor"),
                 N_("The higher limit of the range for requesting surpluses "
                    "from the governor."),
                 COC_INTERFACE, GUI_GTK3_22, GUI_GTK3_22_GOV_RANGE_MAX_DEFAULT,
                 GUI_GTK3_22_GOV_RANGE_MAX_MIN, GUI_GTK3_22_GOV_RANGE_MAX_MAX,
                 NULL),
  GEN_FONT_OPTION(gui_gtk3_22_font_city_label, "city_label",
                  N_("City Label"),
                  N_("This font is used to display the city labels on city "
                     "dialogs."),
                  COC_FONT, GUI_GTK3_22,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_notify_label, "notify_label",
                  N_("Notify Label"),
                  N_("This font is used to display server reports such "
                     "as the demographic report or historian publications."),
                  COC_FONT, GUI_GTK3_22,
                  "Monospace Bold 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_spaceship_label, "spaceship_label",
                  N_("Spaceship Label"),
                  N_("This font is used to display the spaceship widgets."),
                  COC_FONT, GUI_GTK3_22,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_help_label, "help_label",
                  N_("Help Label"),
                  N_("This font is used to display the help headers in the "
                     "help window."),
                  COC_FONT, GUI_GTK3_22,
                  "Sans Bold 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_help_link, "help_link",
                  N_("Help Link"),
                  N_("This font is used to display the help links in the "
                     "help window."),
                  COC_FONT, GUI_GTK3_22,
                  "Sans 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_help_text, "help_text",
                  N_("Help Text"),
                  N_("This font is used to display the help body text in "
                     "the help window."),
                  COC_FONT, GUI_GTK3_22,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_chatline, "chatline",
                  N_("Chatline Area"),
                  N_("This font is used to display the text in the "
                     "chatline area."),
                  COC_FONT, GUI_GTK3_22,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_beta_label, "beta_label",
                  N_("Beta Label"),
                  N_("This font is used to display the beta label."),
                  COC_FONT, GUI_GTK3_22,
                  "Sans Italic 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_small, "small_font",
                  N_("Small Font"),
                  N_("This font is used for any small font request.  For "
                     "example, it is used for display the building lists "
                     "in the city dialog, the Economy report or the Units "
                     "report."),
                  COC_FONT, GUI_GTK3_22,
                  "Sans 9", NULL),
  GEN_FONT_OPTION(gui_gtk3_22_font_comment_label, "comment_label",
                  N_("Comment Label"),
                  N_("This font is used to display comment labels, such as "
                     "in the governor page of the city dialogs."),
                  COC_FONT, GUI_GTK3_22,
                  "Sans Italic 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk3_22_font_city_names, "city_names",
                  N_("City Names"),
                  N_("This font is used to the display the city names "
                     "on the map."),
                  COC_FONT, GUI_GTK3_22,
                  "Sans Bold 10", NULL),
  GEN_FONT_OPTION(gui_gtk3_22_font_city_productions, "city_productions",
                  N_("City Productions"),
                  N_("This font is used to display the city production "
                     "on the map."),
                  COC_FONT, GUI_GTK3_22,
                  "Serif 10", NULL),
  GEN_FONT_OPTION(gui_gtk3_22_font_reqtree_text, "reqtree_text",
                  N_("Requirement Tree"),
                  N_("This font is used to the display the requirement tree "
                     "in the Research report."),
                  COC_FONT, GUI_GTK3_22,
                  "Serif 10", NULL),

  /* gui-gtk-3.x client specific options. */
  GEN_BOOL_OPTION(gui_gtk4_fullscreen, N_("Fullscreen"),
                  N_("If this option is set the client will use the "
                     "whole screen area for drawing."),
                  COC_INTERFACE, GUI_GTK3x, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_map_scrollbars, N_("Show map scrollbars"),
                  N_("Disable this option to hide the scrollbars on the "
                     "map view."),
                  COC_INTERFACE, GUI_GTK3x, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_dialogs_on_top, N_("Keep dialogs on top"),
                  N_("If this option is set then dialog windows will always "
                     "remain in front of the main Freeciv window. "
                     "Disabling this has no effect in fullscreen mode."),
                  COC_INTERFACE, GUI_GTK3x, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_show_task_icons, N_("Show worklist task icons"),
                  N_("Disabling this will turn off the unit and building "
                     "icons in the worklist dialog and the production "
                     "tab of the city dialog."),
                  COC_GRAPHICS, GUI_GTK3x, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_enable_tabs, N_("Enable status report tabs"),
                  N_("If this option is enabled then report dialogs will "
                     "be shown as separate tabs rather than in popup "
                     "dialogs."),
                  COC_INTERFACE, GUI_GTK3x, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_show_chat_message_time,
                  N_("Show time for each chat message"),
                  N_("If this option is enabled then all chat messages "
                     "will be prefixed by a time string of the form "
                     "[hour:minute:second]."),
                  COC_INTERFACE, GUI_GTK3x, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_new_messages_go_to_top,
                  N_("New message events go to top of list"),
                  N_("If this option is enabled, new events in the "
                     "message window will appear at the top of the list, "
                     "rather than being appended at the bottom."),
                  COC_INTERFACE, GUI_GTK3x, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_show_message_window_buttons,
                  N_("Show extra message window buttons"),
                  N_("If this option is enabled, there will be two "
                     "buttons displayed in the message window for "
                     "inspecting a city and going to a location. If this "
                     "option is disabled, these buttons will not appear "
                     "(you can still double-click with the left mouse "
                     "button or right-click on a row to inspect or goto "
                     "respectively). This option will only take effect "
                     "once the message window is closed and reopened."),
                  COC_INTERFACE, GUI_GTK3x, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_metaserver_tab_first,
                  N_("Metaserver tab first in network page"),
                  N_("If this option is enabled, the metaserver tab will "
                     "be the first notebook tab in the network page. This "
                     "option requires a restart in order to take effect."),
                  COC_NETWORK, GUI_GTK3x, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_allied_chat_only,
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
                  COC_INTERFACE, GUI_GTK3x, FALSE, NULL),
  GEN_ENUM_OPTION(gui_gtk4_message_chat_location,
                  N_("Messages and Chat reports location"),
                  /* TRANS: The strings used in the UI for 'Split' etc are
                   * tagged 'gui_gtk2/gtk3/gtk3x_message_chat_location' */
                  N_("Controls where the Messages and Chat reports "
                     "appear relative to the main view containing the map.\n"
                     "'Split' allows all three to be seen simultaneously, "
                     "which is best for multiplayer, but requires a large "
                     "window to be usable.\n"
                     "'Separate' puts Messages and Chat in a notebook "
                     "separate from the main view, so that one of them "
                     "can always be seen alongside the main view.\n"
                     "'Merged' makes the Messages and Chat reports into "
                     "tabs alongside the map and other reports; this "
                     "allows a larger map view on small screens.\n"
                     "This option requires a restart in order to take "
                     "effect."), COC_INTERFACE, GUI_GTK3x,
                  GUI_GTK_MSGCHAT_MERGED /* Ignored! See options_load(). */,
                  gui_gtk_message_chat_location_name, NULL),
  GEN_BOOL_OPTION(gui_gtk4_small_display_layout,
                  N_("Arrange widgets for small displays"),
                  N_("If this option is enabled, widgets in the main "
                     "window will be arranged so that they take up the "
                     "least amount of total screen space. Specifically, "
                     "the left panel containing the overview, player "
                     "status, and the unit information box will be "
                     "extended over the entire left side of the window. "
                     "This option requires a restart in order to take "
                     "effect."), COC_INTERFACE, GUI_GTK3x, TRUE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_mouse_over_map_focus,
                  N_("Mouse over the map widget selects it automatically"),
                  N_("If this option is enabled, then the map will be "
                     "focused when the mouse hovers over it."),
                  COC_INTERFACE, GUI_GTK3x, FALSE, NULL),
  GEN_BOOL_OPTION(gui_gtk4_chatline_autocompletion,
                  N_("Player or user name autocompletion"),
                  N_("If this option is turned on, the tabulation key "
                     "will be used in the chatline to complete the word you "
                     "are typing with the name of a player or a user."),
                  COC_INTERFACE, GUI_GTK3x, TRUE, NULL),
  GEN_INT_OPTION(gui_gtk4_citydlg_xsize,
                 N_("Width of the city dialog"),
                 N_("This value is only used if the width of the city "
                    "dialog is saved."),
                 COC_INTERFACE, GUI_GTK3x, GUI_GTK4_CITYDLG_DEFAULT_XSIZE,
                 GUI_GTK4_CITYDLG_MIN_XSIZE, GUI_GTK4_CITYDLG_MAX_XSIZE,
                 NULL),
  GEN_INT_OPTION(gui_gtk4_citydlg_ysize,
                 N_("Height of the city dialog"),
                 N_("This value is only used if the height of the city "
                    "dialog is saved."),
                 COC_INTERFACE, GUI_GTK3x, GUI_GTK4_CITYDLG_DEFAULT_YSIZE,
                 GUI_GTK4_CITYDLG_MIN_YSIZE, GUI_GTK4_CITYDLG_MAX_YSIZE,
                 NULL),
  GEN_ENUM_OPTION(gui_gtk4_popup_tech_help,
                  N_("Popup tech help when gained"),
                  N_("Controls if tech help should be opened when "
                     "new tech has been gained.\n"
                     "'Ruleset' means that behavior suggested by "
                     "current ruleset is used."), COC_INTERFACE, GUI_GTK3x,
                  GUI_POPUP_TECH_HELP_RULESET,
                  gui_popup_tech_help_name, NULL),
  GEN_INT_OPTION(gui_gtk4_governor_range_min,
                 N_("Minimum surplus for a governor"),
                 N_("The lower limit of the range for requesting surpluses "
                    "from the governor."),
                 COC_INTERFACE, GUI_GTK3x, GUI_GTK4_GOV_RANGE_MIN_DEFAULT,
                 GUI_GTK4_GOV_RANGE_MIN_MIN, GUI_GTK4_GOV_RANGE_MIN_MAX,
                 NULL),
  GEN_INT_OPTION(gui_gtk4_governor_range_max,
                 N_("Maximum surplus for a governor"),
                 N_("The higher limit of the range for requesting surpluses "
                    "from the governor."),
                 COC_INTERFACE, GUI_GTK3x, GUI_GTK4_GOV_RANGE_MAX_DEFAULT,
                 GUI_GTK4_GOV_RANGE_MAX_MIN, GUI_GTK4_GOV_RANGE_MAX_MAX,
                 NULL),
  GEN_FONT_OPTION(gui_gtk4_font_city_label, "city_label",
                  N_("City Label"),
                  N_("This font is used to display the city labels on city "
                     "dialogs."),
                  COC_FONT, GUI_GTK3x,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_notify_label, "notify_label",
                  N_("Notify Label"),
                  N_("This font is used to display server reports such "
                     "as the demographic report or historian publications."),
                  COC_FONT, GUI_GTK3x,
                  "Monospace Bold 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_spaceship_label, "spaceship_label",
                  N_("Spaceship Label"),
                  N_("This font is used to display the spaceship widgets."),
                  COC_FONT, GUI_GTK3x,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_help_label, "help_label",
                  N_("Help Label"),
                  N_("This font is used to display the help headers in the "
                     "help window."),
                  COC_FONT, GUI_GTK3x,
                  "Sans Bold 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_help_link, "help_link",
                  N_("Help Link"),
                  N_("This font is used to display the help links in the "
                     "help window."),
                  COC_FONT, GUI_GTK3x,
                  "Sans 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_help_text, "help_text",
                  N_("Help Text"),
                  N_("This font is used to display the help body text in "
                     "the help window."),
                  COC_FONT, GUI_GTK3x,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_chatline, "chatline",
                  N_("Chatline Area"),
                  N_("This font is used to display the text in the "
                     "chatline area."),
                  COC_FONT, GUI_GTK3x,
                  "Monospace 8", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_beta_label, "beta_label",
                  N_("Beta Label"),
                  N_("This font is used to display the beta label."),
                  COC_FONT, GUI_GTK3x,
                  "Sans Italic 10", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_small, "small_font",
                  N_("Small Font"),
                  N_("This font is used for any small font request.  For "
                     "example, it is used for display the building lists "
                     "in the city dialog, the Economy report or the Units "
                     "report."),
                  COC_FONT, GUI_GTK3x,
                  "Sans 9", NULL),
  GEN_FONT_OPTION(gui_gtk4_font_comment_label, "comment_label",
                  N_("Comment Label"),
                  N_("This font is used to display comment labels, such as "
                     "in the governor page of the city dialogs."),
                  COC_FONT, GUI_GTK3x,
                  "Sans Italic 9", font_changed_callback),
  GEN_FONT_OPTION(gui_gtk4_font_city_names, "city_names",
                  N_("City Names"),
                  N_("This font is used to the display the city names "
                     "on the map."),
                  COC_FONT, GUI_GTK3x,
                  "Sans Bold 10", NULL),
  GEN_FONT_OPTION(gui_gtk4_font_city_productions, "city_productions",
                  N_("City Productions"),
                  N_("This font is used to display the city production "
                     "on the map."),
                  COC_FONT, GUI_GTK3x,
                  "Serif 10", NULL),
  GEN_FONT_OPTION(gui_gtk4_font_reqtree_text, "reqtree_text",
                  N_("Requirement Tree"),
                  N_("This font is used to the display the requirement tree "
                     "in the Research report."),
                  COC_FONT, GUI_GTK3x,
                  "Serif 10", NULL),

  /* gui-sdl client specific options.
   * These are still kept just so users can migrate them to sdl2-client */
  GEN_BOOL_OPTION(gui_sdl_fullscreen, NULL, NULL,
                  COC_INTERFACE, GUI_SDL, FALSE, NULL),
  GEN_VIDEO_OPTION(gui_sdl_screen, NULL, NULL,
                   COC_INTERFACE, GUI_SDL, 640, 480, NULL),
  GEN_BOOL_OPTION(gui_sdl_do_cursor_animation, NULL, NULL,
                  COC_INTERFACE, GUI_SDL, TRUE, NULL),
  GEN_BOOL_OPTION(gui_sdl_use_color_cursors, NULL, NULL,
                  COC_INTERFACE, GUI_SDL, TRUE, NULL),

  /* gui-sdl2 client specific options. */
  GEN_BOOL_OPTION(gui_sdl2_fullscreen, N_("Fullscreen"),
                  N_("If this option is set the client will use the "
                     "whole screen area for drawing."),
                  COC_INTERFACE, GUI_SDL2, FALSE, NULL),
  GEN_VIDEO_OPTION(gui_sdl2_screen, N_("Screen resolution"),
                   N_("This option controls the resolution of the "
                      "selected screen."),
                   COC_INTERFACE, GUI_SDL2, 640, 480, NULL),
  GEN_BOOL_OPTION(gui_sdl2_swrenderer, N_("Use software rendering"),
                  N_("Usually hardware rendering is used when possible. "
                     "With this option set, software rendering is always used."),
                  COC_GRAPHICS, GUI_SDL2, FALSE, NULL),
  GEN_BOOL_OPTION(gui_sdl2_do_cursor_animation, N_("Do cursor animation"),
                  N_("If this option is disabled, the cursor will "
                     "always be displayed as static."),
                  COC_INTERFACE, GUI_SDL2, TRUE, NULL),
  GEN_BOOL_OPTION(gui_sdl2_use_color_cursors, N_("Use color cursors"),
                  N_("If this option is disabled, the cursor will "
                     "always be displayed in black and white."),
                  COC_INTERFACE, GUI_SDL2, TRUE, NULL),
  GEN_FONT_OPTION(gui_sdl2_font_city_names, "FONT_CITY_NAME",
                  N_("City Names"),
                  N_("The size of font used to the display the city names "
                     "on the map."),
                  COC_FONT, GUI_SDL2,
                  "10", font_changed_callback),
  GEN_FONT_OPTION(gui_sdl2_font_city_productions, "FONT_CITY_PROD",
                  N_("City Productions"),
                  N_("The size of font used to the display the city "
                     "production names on the map."),
                  COC_FONT, GUI_SDL2,
                  "10", font_changed_callback),

  /* gui-qt client specific options. */
  GEN_BOOL_OPTION(gui_qt_fullscreen, N_("Fullscreen"),
                  N_("If this option is set the client will use the "
                     "whole screen area for drawing."),
                  COC_INTERFACE, GUI_QT, FALSE, NULL),
  GEN_BOOL_OPTION(gui_qt_show_titlebar, N_("Show titlebar"),
                  N_("If this option is set the client will show a titlebar. "
                     "If disabled, then no titlebar will be shown, and "
                     "minimize/maximize/etc buttons will be placed on the "
                     "menu bar."),
                  COC_INTERFACE, GUI_QT, TRUE, NULL),
  GEN_FONT_OPTION(gui_qt_font_city_label, "city_label",
                  N_("City Label"),
                  N_("This font is used to display the city labels on city "
                     "dialogs."),
                  COC_FONT, GUI_QT,
                  "Monospace,8,-1,5,50,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_default, "default_font",
                  N_("Default font"),
                  N_("This is default font"),
                  COC_FONT, GUI_QT,
                  "Sans Serif,10,-1,5,75,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_notify_label, "notify_label",
                  N_("Notify Label"),
                  N_("This font is used to display server reports such "
                     "as the demographic report or historian publications."),
                  COC_FONT, GUI_QT,
                  "Monospace,9,-1,5,75,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_spaceship_label, "spaceship_label",
                  N_("Spaceship Label"),
                  N_("This font is used to display the spaceship widgets."),
                  COC_FONT, GUI_QT,
                  "Monospace,8,-1,5,50,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_help_label, "help_label",
                  N_("Help Label"),
                  N_("This font is used to display the help labels in the "
                     "help window."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,9,-1,5,50,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_help_link, "help_link",
                  N_("Help Link"),
                  N_("This font is used to display the help links in the "
                     "help window."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,9,-1,5,50,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_help_text, "help_text",
                  N_("Help Text"),
                  N_("This font is used to display the help body text in "
                     "the help window."),
                  COC_FONT, GUI_QT,
                  "Monospace,8,-1,5,50,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_help_title, "help_title",
                  N_("Help Title"),
                  N_("This font is used to display the help title in "
                     "the help window."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,10,-1,5,75,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_chatline, "chatline",
                  N_("Chatline Area"),
                  N_("This font is used to display the text in the "
                     "chatline area."),
                  COC_FONT, GUI_QT,
                  "Monospace,8,-1,5,50,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_beta_label, "beta_label",
                  N_("Beta Label"),
                  N_("This font is used to display the beta label."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,10,-1,5,50,1,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_small, "small_font",
                  N_("Small Font"),
                  N_("This font is used for any small font request.  For "
                     "example, it is used for display the building lists "
                     "in the city dialog, the Economy report or the Units "
                     "report."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,9,-1,5,50,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_comment_label, "comment_label",
                  N_("Comment Label"),
                  N_("This font is used to display comment labels, such as "
                     "in the governor page of the city dialogs."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,9,-1,5,50,1,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_city_names, "city_names",
                  N_("City Names"),
                  N_("This font is used to the display the city names "
                     "on the map."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,10,-1,5,75,0,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_city_productions, "city_productions",
                  N_("City Productions"),
                  N_("This font is used to display the city production "
                     "on the map."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,10,-1,5,50,1,0,0,0,0", font_changed_callback),
  GEN_FONT_OPTION(gui_qt_font_reqtree_text, "reqtree_text",
                  N_("Requirement Tree"),
                  N_("This font is used to the display the requirement tree "
                     "in the Research report."),
                  COC_FONT, GUI_QT,
                  "Sans Serif,10,-1,5,50,1,0,0,0,0", font_changed_callback),
  GEN_BOOL_OPTION(gui_qt_show_preview, N_("Show savegame information"),
                  N_("If this option is set the client will show "
                     "information and map preview of current savegame."),
                  COC_GRAPHICS, GUI_QT, TRUE, NULL),
  GEN_BOOL_OPTION(gui_qt_sidebar_left, N_("Sidebar position"),
                  N_("If this option is set, the sidebar will be to the left "
                     "of the map, otherwise to the right."),
                  COC_INTERFACE, GUI_QT, TRUE, NULL),
  GEN_STR_OPTION(gui_qt_wakeup_text,
                 N_("Wake up sequence"),
                 N_("String which will trigger sound in pregame page; "
                    "%1 stands for username."),
                 COC_INTERFACE, GUI_QT, "Wake up %1", NULL, 0)

};
static const int client_options_num = ARRAY_SIZE(client_options);

/* Iteration loop, including invalid options for the current gui type. */
#define client_options_iterate_all(poption)                                 \
{                                                                           \
  const struct client_option *const poption##_max =                         \
      client_options + client_options_num;                                  \
  struct client_option *client_##poption = client_options;                  \
  struct option *poption;                                                   \
  for (; client_##poption < poption##_max; client_##poption++) {            \
    poption = OPTION(client_##poption);

#define client_options_iterate_all_end                                      \
  }                                                                         \
}


/************************************************************************//**
  Returns the next valid option pointer for the current gui type.
****************************************************************************/
static struct client_option *
    client_option_next_valid(struct client_option *poption)
{
  const struct client_option *const max = 
    client_options + client_options_num;
  const enum gui_type our_type = get_gui_type();

  while (poption < max
         && poption->specific != GUI_STUB
         && poption->specific != our_type) {
    poption++;
  }

  return (poption < max ? poption : NULL);
}

/************************************************************************//**
  Returns the option corresponding to this id.
****************************************************************************/
static struct option *client_optset_option_by_number(int id)
{
  if (0 > id || id > client_options_num)  {
    return NULL;
  }
  return OPTION(client_options + id);
}

/************************************************************************//**
  Returns the first valid option pointer for the current gui type.
****************************************************************************/
static struct option *client_optset_option_first(void)
{
  return OPTION(client_option_next_valid(client_options));
}

/************************************************************************//**
  Returns the number of client option categories.
****************************************************************************/
static int client_optset_category_number(void)
{
  return COC_MAX;
}

/************************************************************************//**
  Returns the name (translated) of the option class.
****************************************************************************/
static const char *client_optset_category_name(int category)
{
  switch (category) {
  case COC_GRAPHICS:
    return _("Graphics");
  case COC_OVERVIEW:
    /* TRANS: Options section for overview map (mini-map) */
    return Q_("?map:Overview");
  case COC_SOUND:
    return _("Sound");
  case COC_INTERFACE:
    return _("Interface");
  case COC_MAPIMG:
    return _("Map Image");
  case COC_NETWORK:
    return _("Network");
  case COC_FONT:
    return _("Font");
  case COC_MAX:
    break;
  }

  log_error("%s: invalid option category number %d.",
            __FUNCTION__, category);
  return NULL;
}

/************************************************************************//**
  Returns the number of this client option.
****************************************************************************/
static int client_option_number(const struct option *poption)
{
  return CLIENT_OPTION(poption) - client_options;
}

/************************************************************************//**
  Returns the name of this client option.
****************************************************************************/
static const char *client_option_name(const struct option *poption)
{
  return CLIENT_OPTION(poption)->name;
}

/************************************************************************//**
  Returns the description of this client option.
****************************************************************************/
static const char *client_option_description(const struct option *poption)
{
  return _(CLIENT_OPTION(poption)->description);
}

/************************************************************************//**
  Returns the help text for this client option.
****************************************************************************/
static const char *client_option_help_text(const struct option *poption)
{
  return _(CLIENT_OPTION(poption)->help_text);
}

/************************************************************************//**
  Returns the category of this client option.
****************************************************************************/
static int client_option_category(const struct option *poption)
{
  return CLIENT_OPTION(poption)->category;
}

/************************************************************************//**
  Returns TRUE if this client option can be modified.
****************************************************************************/
static bool client_option_is_changeable(const struct option *poption)
{
  return TRUE;
}

/************************************************************************//**
  Returns the next valid option pointer for the current gui type.
****************************************************************************/
static struct option *client_option_next(const struct option *poption)
{
  return OPTION(client_option_next_valid(CLIENT_OPTION(poption) + 1));
}

/************************************************************************//**
  Returns the value of this client option of type OT_BOOLEAN.
****************************************************************************/
static bool client_option_bool_get(const struct option *poption)
{
  return *(CLIENT_OPTION(poption)->boolean.pvalue);
}

/************************************************************************//**
  Returns the default value of this client option of type OT_BOOLEAN.
****************************************************************************/
static bool client_option_bool_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->boolean.def;
}

/************************************************************************//**
  Set the value of this client option of type OT_BOOLEAN.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool client_option_bool_set(struct option *poption, bool val)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (*pcoption->boolean.pvalue == val) {
    return FALSE;
  }

  *pcoption->boolean.pvalue = val;
  return TRUE;
}

/************************************************************************//**
  Returns the value of this client option of type OT_INTEGER.
****************************************************************************/
static int client_option_int_get(const struct option *poption)
{
  return *(CLIENT_OPTION(poption)->integer.pvalue);
}

/************************************************************************//**
  Returns the default value of this client option of type OT_INTEGER.
****************************************************************************/
static int client_option_int_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->integer.def;
}

/************************************************************************//**
  Returns the minimal value for this client option of type OT_INTEGER.
****************************************************************************/
static int client_option_int_min(const struct option *poption)
{
  return CLIENT_OPTION(poption)->integer.min;
}

/************************************************************************//**
  Returns the maximal value for this client option of type OT_INTEGER.
****************************************************************************/
static int client_option_int_max(const struct option *poption)
{
  return CLIENT_OPTION(poption)->integer.max;
}

/************************************************************************//**
  Set the value of this client option of type OT_INTEGER.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool client_option_int_set(struct option *poption, int val)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (val < pcoption->integer.min
      || val > pcoption->integer.max
      || *pcoption->integer.pvalue == val) {
    return FALSE;
  }

  *pcoption->integer.pvalue = val;
  return TRUE;
}

/************************************************************************//**
  Returns the value of this client option of type OT_STRING.
****************************************************************************/
static const char *client_option_str_get(const struct option *poption)
{
  return CLIENT_OPTION(poption)->string.pvalue;
}

/************************************************************************//**
  Returns the default value of this client option of type OT_STRING.
****************************************************************************/
static const char *client_option_str_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->string.def;
}

/************************************************************************//**
  Returns the possible string values of this client option of type
  OT_STRING.
****************************************************************************/
static const struct strvec *
    client_option_str_values(const struct option *poption)
{
  return (CLIENT_OPTION(poption)->string.val_accessor
          ? CLIENT_OPTION(poption)->string.val_accessor(poption) : NULL);
}

/************************************************************************//**
  Set the value of this client option of type OT_STRING.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool client_option_str_set(struct option *poption, const char *str)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (strlen(str) >= pcoption->string.size
      || 0 == strcmp(pcoption->string.pvalue, str)) {
    return FALSE;
  }

  fc_strlcpy(pcoption->string.pvalue, str, pcoption->string.size);
  return TRUE;
}

/************************************************************************//**
  Returns the current value of this client option of type OT_ENUM.
****************************************************************************/
static int client_option_enum_get(const struct option *poption)
{
  return *(CLIENT_OPTION(poption)->enumerator.pvalue);
}

/************************************************************************//**
  Returns the default value of this client option of type OT_ENUM.
****************************************************************************/
static int client_option_enum_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->enumerator.def;
}

/************************************************************************//**
  Returns the possible values of this client option of type OT_ENUM, as
  user-visible (translatable but not translated) strings.
****************************************************************************/
static const struct strvec *
    client_option_enum_pretty_names(const struct option *poption)
{
  return CLIENT_OPTION(poption)->enumerator.pretty_names;
}

/************************************************************************//**
  Set the value of this client option of type OT_ENUM.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool client_option_enum_set(struct option *poption, int val)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (*pcoption->enumerator.pvalue == val
      || 0 > val
      || val >= strvec_size(pcoption->enumerator.support_names)) {
    return FALSE;
  }

  *pcoption->enumerator.pvalue = val;
  return TRUE;
}

/************************************************************************//**
  Returns the "support" name of the value for this client option of type
  OT_ENUM (a string suitable for saving in a file).
  The prototype must match the 'secfile_enum_name_data_fn_t' type.
****************************************************************************/
static const char *client_option_enum_secfile_str(secfile_data_t data,
                                                  int val)
{
  const struct strvec *names = CLIENT_OPTION(data)->enumerator.support_names;

  return (0 <= val && val < strvec_size(names)
          ? strvec_get(names, val) : NULL);
}

#if 0 /* There's no bitwise options currently */
/************************************************************************//**
  Returns the current value of this client option of type OT_BITWISE.
****************************************************************************/
static unsigned client_option_bitwise_get(const struct option *poption)
{
  return *(CLIENT_OPTION(poption)->bitwise.pvalue);
}

/************************************************************************//**
  Returns the default value of this client option of type OT_BITWISE.
****************************************************************************/
static unsigned client_option_bitwise_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->bitwise.def;
}

/************************************************************************//**
  Returns the possible values of this client option of type OT_BITWISE, as
  user-visible (translatable but not translated) strings.
****************************************************************************/
static const struct strvec *
    client_option_bitwise_pretty_names(const struct option *poption)
{
  return CLIENT_OPTION(poption)->bitwise.pretty_names;
}

/************************************************************************//**
  Set the value of this client option of type OT_BITWISE.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool client_option_bitwise_set(struct option *poption, unsigned val)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (*pcoption->bitwise.pvalue == val) {
    return FALSE;
  }

  *pcoption->bitwise.pvalue = val;
  return TRUE;
}
#endif /* 0 */

/************************************************************************//**
  Returns the "support" name of a single value for this client option of type
  OT_BITWISE (a string suitable for saving in a file).
  The prototype must match the 'secfile_enum_name_data_fn_t' type.
****************************************************************************/
static const char *client_option_bitwise_secfile_str(secfile_data_t data,
                                                     int val)
{
  const struct strvec *names = CLIENT_OPTION(data)->bitwise.support_names;

  return (0 <= val && val < strvec_size(names)
          ? strvec_get(names, val) : NULL);
}

/************************************************************************//**
  Returns the value of this client option of type OT_FONT.
****************************************************************************/
static const char *client_option_font_get(const struct option *poption)
{
  return CLIENT_OPTION(poption)->font.pvalue;
}

/************************************************************************//**
  Returns the default value of this client option of type OT_FONT.
****************************************************************************/
static const char *client_option_font_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->font.def;
}

/************************************************************************//**
  Returns the target style name of this client option of type OT_FONT.
****************************************************************************/
static const char *client_option_font_target(const struct option *poption)
{
  return CLIENT_OPTION(poption)->font.target;
}

/************************************************************************//**
  Set the value of this client option of type OT_FONT.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool client_option_font_set(struct option *poption, const char *font)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (strlen(font) >= pcoption->font.size
      || 0 == strcmp(pcoption->font.pvalue, font)) {
    return FALSE;
  }

  fc_strlcpy(pcoption->font.pvalue, font, pcoption->font.size);
  return TRUE;
}

/************************************************************************//**
  Returns the value of this client option of type OT_COLOR.
****************************************************************************/
static struct ft_color client_option_color_get(const struct option *poption)
{
  return *CLIENT_OPTION(poption)->color.pvalue;
}

/************************************************************************//**
  Returns the default value of this client option of type OT_COLOR.
****************************************************************************/
static struct ft_color client_option_color_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->color.def;
}

/************************************************************************//**
  Set the value of this client option of type OT_COLOR.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool client_option_color_set(struct option *poption,
                                    struct ft_color color)
{
  struct ft_color *pcolor = CLIENT_OPTION(poption)->color.pvalue;
  bool changed = FALSE;

#define color_set(color_tgt, color)                                         \
  if (NULL == color_tgt) {                                                  \
    if (NULL != color) {                                                    \
      color_tgt = fc_strdup(color);                                         \
      changed = TRUE;                                                       \
    }                                                                       \
  } else {                                                                  \
    if (NULL == color) {                                                    \
      free((void *) color_tgt);                                             \
      color_tgt = NULL;                                                     \
      changed = TRUE;                                                       \
    } else if (0 != strcmp(color_tgt, color)) {                             \
      free((void *) color_tgt);                                             \
      color_tgt = fc_strdup(color);                                         \
      changed = TRUE;                                                       \
    }                                                                       \
  }

  color_set(pcolor->foreground, color.foreground);
  color_set(pcolor->background, color.background);

#undef color_set

  return changed;
}

/************************************************************************//**
  Returns the value of this client option of type OT_VIDEO_MODE.
****************************************************************************/
static struct video_mode
client_option_video_mode_get(const struct option *poption)
{
  return *CLIENT_OPTION(poption)->video_mode.pvalue;
}

/************************************************************************//**
  Returns the default value of this client option of type OT_VIDEO_MODE.
****************************************************************************/
static struct video_mode
client_option_video_mode_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->video_mode.def;
}

/************************************************************************//**
  Set the value of this client option of type OT_VIDEO_MODE.  Returns TRUE
  if the value changed.
****************************************************************************/
static bool client_option_video_mode_set(struct option *poption,
                                         struct video_mode mode)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (0 == memcmp(&mode, pcoption->video_mode.pvalue,
                  sizeof(struct video_mode))) {
    return FALSE;
  }

  *pcoption->video_mode.pvalue = mode;
  return TRUE;
}

/************************************************************************//**
  Load the option from a file.  Returns TRUE if the option changed.
****************************************************************************/
static bool client_option_load(struct option *poption,
                               struct section_file *sf)
{
  fc_assert_ret_val(NULL != poption, FALSE);
  fc_assert_ret_val(NULL != sf, FALSE);

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    {
      bool value;

      return (secfile_lookup_bool(sf, &value, "client.%s",
                                  option_name(poption))
              && option_bool_set(poption, value));
    }
  case OT_INTEGER:
    {
      int value;

      return (secfile_lookup_int(sf, &value, "client.%s",
                                 option_name(poption))
              && option_int_set(poption, value));
    }
  case OT_STRING:
    {
      const char *string;

      return ((string = secfile_lookup_str(sf, "client.%s",
                                           option_name(poption)))
              && option_str_set(poption, string));
    }
  case OT_ENUM:
    {
      int value;

      return (secfile_lookup_enum_data(sf, &value, FALSE,
                                       client_option_enum_secfile_str,
                                       poption, "client.%s",
                                       option_name(poption))
              && option_enum_set_int(poption, value));
    }
  case OT_BITWISE:
    {
      int value;

      return (secfile_lookup_enum_data(sf, &value, TRUE,
                                       client_option_bitwise_secfile_str,
                                       poption, "client.%s",
                                       option_name(poption))
              && option_bitwise_set(poption, value));
    }
  case OT_FONT:
    {
      const char *string;

      return ((string = secfile_lookup_str(sf, "client.%s",
                                           option_name(poption)))
              && option_font_set(poption, string));
    }
  case OT_COLOR:
    {
      struct ft_color color;

      return ((color.foreground =
                   secfile_lookup_str(sf, "client.%s.foreground",
                                      option_name(poption)))
              && (color.background =
                      secfile_lookup_str(sf, "client.%s.background",
                                         option_name(poption)))
              && option_color_set(poption, color));
    }
  case OT_VIDEO_MODE:
    {
      struct video_mode mode;

      return (secfile_lookup_int(sf, &mode.width, "client.%s.width",
                                 option_name(poption))
              && secfile_lookup_int(sf, &mode.height, "client.%s.height",
                                    option_name(poption))
              && option_video_mode_set(poption, mode));
    }
  }
  return FALSE;
}

/************************************************************************//**
  Save the option to a file.
****************************************************************************/
static void client_option_save(struct option *poption,
                               struct section_file *sf)
{
  fc_assert_ret(NULL != poption);
  fc_assert_ret(NULL != sf);

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    secfile_insert_bool(sf, option_bool_get(poption),
                        "client.%s", option_name(poption));
    break;
  case OT_INTEGER:
    secfile_insert_int(sf, option_int_get(poption),
                       "client.%s", option_name(poption));
    break;
  case OT_STRING:
    secfile_insert_str(sf, option_str_get(poption),
                       "client.%s", option_name(poption));
    break;
  case OT_ENUM:
    secfile_insert_enum_data(sf, option_enum_get_int(poption), FALSE,
                             client_option_enum_secfile_str, poption,
                             "client.%s", option_name(poption));
    break;
  case OT_BITWISE:
    secfile_insert_enum_data(sf, option_bitwise_get(poption), TRUE,
                             client_option_bitwise_secfile_str, poption,
                             "client.%s", option_name(poption));
    break;
  case OT_FONT:
    secfile_insert_str(sf, option_font_get(poption),
                       "client.%s", option_name(poption));
    break;
  case OT_COLOR:
    {
      struct ft_color color = option_color_get(poption);

      secfile_insert_str(sf, color.foreground, "client.%s.foreground",
                         option_name(poption));
      secfile_insert_str(sf, color.background, "client.%s.background",
                         option_name(poption));
    }
    break;
  case OT_VIDEO_MODE:
    {
      struct video_mode mode = option_video_mode_get(poption);

      secfile_insert_int(sf, mode.width, "client.%s.width",
                         option_name(poption));
      secfile_insert_int(sf, mode.height, "client.%s.height",
                         option_name(poption));
    }
    break;
  }
}


/****************************************************************************
  Server options variables.
****************************************************************************/
static char **server_options_categories = NULL;
static struct server_option *server_options = NULL;

static int server_options_categories_num = 0;
static int server_options_num = 0;


/****************************************************************************
  Server option set.
****************************************************************************/
static struct option *server_optset_option_by_number(int id);
static struct option *server_optset_option_first(void);
static int server_optset_category_number(void);
static const char *server_optset_category_name(int category);

static struct option_set server_optset_static = {
  .option_by_number = server_optset_option_by_number,
  .option_first = server_optset_option_first,
  .category_number = server_optset_category_number,
  .category_name = server_optset_category_name
};
const struct option_set *server_optset = &server_optset_static;


/****************************************************************************
  Virtuals tables for the client options.
****************************************************************************/
static int server_option_number(const struct option *poption);
static const char *server_option_name(const struct option *poption);
static const char *server_option_description(const struct option *poption);
static const char *server_option_help_text(const struct option *poption);
static int server_option_category(const struct option *poption);
static bool server_option_is_changeable(const struct option *poption);
static struct option *server_option_next(const struct option *poption);

static const struct option_common_vtable server_option_common_vtable = {
  .number = server_option_number,
  .name = server_option_name,
  .description = server_option_description,
  .help_text = server_option_help_text,
  .category = server_option_category,
  .is_changeable = server_option_is_changeable,
  .next = server_option_next
};

static bool server_option_bool_get(const struct option *poption);
static bool server_option_bool_def(const struct option *poption);
static bool server_option_bool_set(struct option *poption, bool val);

static const struct option_bool_vtable server_option_bool_vtable = {
  .get = server_option_bool_get,
  .def = server_option_bool_def,
  .set = server_option_bool_set
};

static int server_option_int_get(const struct option *poption);
static int server_option_int_def(const struct option *poption);
static int server_option_int_min(const struct option *poption);
static int server_option_int_max(const struct option *poption);
static bool server_option_int_set(struct option *poption, int val);

static const struct option_int_vtable server_option_int_vtable = {
  .get = server_option_int_get,
  .def = server_option_int_def,
  .minimum = server_option_int_min,
  .maximum = server_option_int_max,
  .set = server_option_int_set
};

static const char *server_option_str_get(const struct option *poption);
static const char *server_option_str_def(const struct option *poption);
static const struct strvec *
    server_option_str_values(const struct option *poption);
static bool server_option_str_set(struct option *poption, const char *str);

static const struct option_str_vtable server_option_str_vtable = {
  .get = server_option_str_get,
  .def = server_option_str_def,
  .values = server_option_str_values,
  .set = server_option_str_set
};

static int server_option_enum_get(const struct option *poption);
static int server_option_enum_def(const struct option *poption);
static const struct strvec *
    server_option_enum_pretty(const struct option *poption);
static bool server_option_enum_set(struct option *poption, int val);

static const struct option_enum_vtable server_option_enum_vtable = {
  .get = server_option_enum_get,
  .def = server_option_enum_def,
  .values = server_option_enum_pretty,
  .set = server_option_enum_set,
  .cmp = strcmp
};

static unsigned server_option_bitwise_get(const struct option *poption);
static unsigned server_option_bitwise_def(const struct option *poption);
static const struct strvec *
    server_option_bitwise_pretty(const struct option *poption);
static bool server_option_bitwise_set(struct option *poption, unsigned val);

static const struct option_bitwise_vtable server_option_bitwise_vtable = {
  .get = server_option_bitwise_get,
  .def = server_option_bitwise_def,
  .values = server_option_bitwise_pretty,
  .set = server_option_bitwise_set
};

/****************************************************************************
  Derived class server option, inheriting from base class option.
****************************************************************************/
struct server_option {
  struct option base_option;    /* Base structure, must be the first! */

  char *name;                   /* Short name - used as an identifier */
  char *description;            /* One-line description */
  char *help_text;              /* Paragraph-length help text */
  unsigned char category;
  bool desired_sent;
  bool is_changeable;
  bool is_visible;

  union {
    /* OT_BOOLEAN type option. */
    struct {
      bool value;
      bool def;
    } boolean;
    /* OT_INTEGER type option. */
    struct {
      int value;
      int def, min, max;
    } integer;
    /* OT_STRING type option. */
    struct {
      char *value;
      char *def;
    } string;
    /* OT_ENUM type option. */
    struct {
      int value;
      int def;
      struct strvec *support_names;
      struct strvec *pretty_names; /* untranslated */
    } enumerator;
    /* OT_BITWISE type option. */
    struct {
      unsigned value;
      unsigned def;
      struct strvec *support_names;
      struct strvec *pretty_names; /* untranslated */
    } bitwise;
  };
};

#define SERVER_OPTION(poption) ((struct server_option *) (poption))

static void desired_settable_option_send(struct option *poption);


/************************************************************************//**
  Initialize the server options (not received yet).
****************************************************************************/
void server_options_init(void)
{
  fc_assert(NULL == server_options_categories);
  fc_assert(NULL == server_options);
  fc_assert(0 == server_options_categories_num);
  fc_assert(0 == server_options_num);
}

/************************************************************************//**
  Free one server option.
****************************************************************************/
static void server_option_free(struct server_option *poption)
{
  switch (poption->base_option.type) {
  case OT_STRING:
    if (NULL != poption->string.value) {
      FC_FREE(poption->string.value);
    }
    if (NULL != poption->string.def) {
      FC_FREE(poption->string.def);
    }
    break;

  case OT_ENUM:
    if (NULL != poption->enumerator.support_names) {
      strvec_destroy(poption->enumerator.support_names);
      poption->enumerator.support_names = NULL;
    }
    if (NULL != poption->enumerator.pretty_names) {
      strvec_destroy(poption->enumerator.pretty_names);
      poption->enumerator.pretty_names = NULL;
    }
    break;

  case OT_BITWISE:
    if (NULL != poption->bitwise.support_names) {
      strvec_destroy(poption->bitwise.support_names);
      poption->bitwise.support_names = NULL;
    }
    if (NULL != poption->bitwise.pretty_names) {
      strvec_destroy(poption->bitwise.pretty_names);
      poption->bitwise.pretty_names = NULL;
    }
    break;

  case OT_BOOLEAN:
  case OT_INTEGER:
  case OT_FONT:
  case OT_COLOR:
  case OT_VIDEO_MODE:
    break;
  }

  if (NULL != poption->name) {
    FC_FREE(poption->name);
  }
  if (NULL != poption->description) {
    FC_FREE(poption->description);
  }
  if (NULL != poption->help_text) {
    FC_FREE(poption->help_text);
  }
}

/************************************************************************//**
  Free the server options, if already received.
****************************************************************************/
void server_options_free(void)
{
  int i;

  /* Don't keep this dialog open. */
  option_dialog_popdown(server_optset);

  /* Free the options themselves. */
  if (NULL != server_options) {
    for (i = 0; i < server_options_num; i++) {
      server_option_free(server_options + i);
    }
    FC_FREE(server_options);
    server_options_num = 0;
  }

  /* Free the categories. */
  if (NULL != server_options_categories) {
    for (i = 0; i < server_options_categories_num; i++) {
      if (NULL != server_options_categories[i]) {
        FC_FREE(server_options_categories[i]);
      }
    }
    FC_FREE(server_options_categories);
    server_options_categories_num = 0;
  }
}

/************************************************************************//**
  Allocate the server options and categories.
****************************************************************************/
void handle_server_setting_control
    (const struct packet_server_setting_control *packet)
{
  int i;

  /* This packet should be received only once. */
  fc_assert_ret(NULL == server_options_categories);
  fc_assert_ret(NULL == server_options);
  fc_assert_ret(0 == server_options_categories_num);
  fc_assert_ret(0 == server_options_num);

  /* Allocate server option categories. */
  if (0 < packet->categories_num) {
    server_options_categories_num = packet->categories_num;
    server_options_categories =
        fc_calloc(server_options_categories_num,
                  sizeof(*server_options_categories));

    for (i = 0; i < server_options_categories_num; i++) {
      /* NB: Translate now. */
      server_options_categories[i] = fc_strdup(_(packet->category_names[i]));
    }
  }

  /* Allocate server options. */
  if (0 < packet->settings_num) {
    server_options_num = packet->settings_num;
    server_options = fc_calloc(server_options_num, sizeof(*server_options));
  }
}

/************************************************************************//**
  Receive a server setting info packet.
****************************************************************************/
void handle_server_setting_const
    (const struct packet_server_setting_const *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);

  fc_assert_ret(NULL != poption);

  fc_assert(NULL == psoption->name);
  psoption->name = fc_strdup(packet->name);
  fc_assert(NULL == psoption->description);
  /* NB: Translate now. */
  psoption->description = fc_strdup(_(packet->short_help));
  fc_assert(NULL == psoption->help_text);
  /* NB: Translate now. */
  psoption->help_text = fc_strdup(_(packet->extra_help));
  psoption->category = packet->category;
}

/****************************************************************************
  Common part of handle_server_setting_*() functions. See below.
****************************************************************************/
#define handle_server_setting_common(psoption, packet)                      \
  psoption->is_changeable = packet->is_changeable;                          \
  if (psoption->is_visible != packet->is_visible) {                         \
    if (psoption->is_visible) {                                             \
      need_gui_remove = TRUE;                                               \
    } else if (packet->is_visible) {                                        \
      need_gui_add = TRUE;                                                  \
    }                                                                       \
    psoption->is_visible = packet->is_visible;                              \
  }                                                                         \
                                                                            \
  if (!psoption->desired_sent                                               \
      && psoption->is_visible                                               \
      && psoption->is_changeable                                            \
      && is_server_running()                                                \
      && packet->initial_setting) {                                         \
    /* Only send our private settings if we are running                     \
     * on a forked local server, i.e. started by the                        \
     * client with the "Start New Game" button.                             \
     * Do now override settings that are already saved to savegame          \
     * and now loaded. */                                                   \
    desired_settable_option_send(OPTION(poption));                          \
    psoption->desired_sent = TRUE;                                          \
  }                                                                         \
                                                                            \
  /* Update the GUI. */                                                     \
  if (need_gui_remove) {                                                    \
    option_gui_remove(poption);                                             \
  } else if (need_gui_add) {                                                \
    option_gui_add(poption);                                                \
  } else {                                                                  \
    option_gui_update(poption);                                             \
  }

/************************************************************************//**
  Receive a boolean server setting info packet.
****************************************************************************/
void handle_server_setting_bool
    (const struct packet_server_setting_bool *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);
  bool need_gui_remove = FALSE;
  bool need_gui_add = FALSE;

  fc_assert_ret(NULL != poption);

  if (NULL == poption->common_vtable) {
    /* Not initialized yet. */
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_BOOLEAN;
    poption->bool_vtable = &server_option_bool_vtable;
  }
  fc_assert_ret_msg(OT_BOOLEAN == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_BOOLEAN), OT_BOOLEAN);

  if (packet->is_visible) {
    psoption->boolean.value = packet->val;
    psoption->boolean.def = packet->default_val;
  }

  handle_server_setting_common(psoption, packet);
}

/************************************************************************//**
  Receive a integer server setting info packet.
****************************************************************************/
void handle_server_setting_int
    (const struct packet_server_setting_int *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);
  bool need_gui_remove = FALSE;
  bool need_gui_add = FALSE;

  fc_assert_ret(NULL != poption);

  if (NULL == poption->common_vtable) {
    /* Not initialized yet. */
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_INTEGER;
    poption->int_vtable = &server_option_int_vtable;
  }
  fc_assert_ret_msg(OT_INTEGER == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_INTEGER), OT_INTEGER);

  if (packet->is_visible) {
    psoption->integer.value = packet->val;
    psoption->integer.def = packet->default_val;
    psoption->integer.min = packet->min_val;
    psoption->integer.max = packet->max_val;
  }

  handle_server_setting_common(psoption, packet);
}

/************************************************************************//**
  Receive a string server setting info packet.
****************************************************************************/
void handle_server_setting_str
    (const struct packet_server_setting_str *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);
  bool need_gui_remove = FALSE;
  bool need_gui_add = FALSE;

  fc_assert_ret(NULL != poption);

  if (NULL == poption->common_vtable) {
    /* Not initialized yet. */
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_STRING;
    poption->str_vtable = &server_option_str_vtable;
  }
  fc_assert_ret_msg(OT_STRING == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_STRING), OT_STRING);

  if (packet->is_visible) {
    if (NULL == psoption->string.value) {
      psoption->string.value = fc_strdup(packet->val);
    } else if (0 != strcmp(packet->val, psoption->string.value)) {
      free(psoption->string.value);
      psoption->string.value = fc_strdup(packet->val);
    }
    if (NULL == psoption->string.def) {
      psoption->string.def = fc_strdup(packet->default_val);
    } else if (0 != strcmp(packet->default_val, psoption->string.def)) {
      free(psoption->string.def);
      psoption->string.def = fc_strdup(packet->default_val);
    }
  }

  handle_server_setting_common(psoption, packet);
}

/************************************************************************//**
  Receive an enumerator server setting info packet.
****************************************************************************/
void handle_server_setting_enum
    (const struct packet_server_setting_enum *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);
  bool need_gui_remove = FALSE;
  bool need_gui_add = FALSE;

  fc_assert_ret(NULL != poption);

  if (NULL == poption->common_vtable) {
    /* Not initialized yet. */
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_ENUM;
    poption->enum_vtable = &server_option_enum_vtable;
  }
  fc_assert_ret_msg(OT_ENUM == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_ENUM), OT_ENUM);

  if (packet->is_visible) {
    int i;

    psoption->enumerator.value = packet->val;
    psoption->enumerator.def = packet->default_val;

    if (NULL == psoption->enumerator.support_names) {
      /* First time we get this packet. */
      fc_assert(NULL == psoption->enumerator.pretty_names);
      psoption->enumerator.support_names = strvec_new();
      strvec_reserve(psoption->enumerator.support_names, packet->values_num);
      psoption->enumerator.pretty_names = strvec_new();
      strvec_reserve(psoption->enumerator.pretty_names, packet->values_num);
      for (i = 0; i < packet->values_num; i++) {
        strvec_set(psoption->enumerator.support_names, i,
                   packet->support_names[i]);
        /* Store untranslated string from server. */
        strvec_set(psoption->enumerator.pretty_names, i,
                   packet->pretty_names[i]);
      }
    } else if (strvec_size(psoption->enumerator.support_names)
               != packet->values_num) {
      fc_assert(strvec_size(psoption->enumerator.support_names)
                == strvec_size(psoption->enumerator.pretty_names));
      /* The number of values have changed, we need to reset the list
       * of possible values. */
      strvec_reserve(psoption->enumerator.support_names, packet->values_num);
      strvec_reserve(psoption->enumerator.pretty_names, packet->values_num);
      for (i = 0; i < packet->values_num; i++) {
        strvec_set(psoption->enumerator.support_names, i,
                   packet->support_names[i]);
        /* Store untranslated string from server. */
        strvec_set(psoption->enumerator.pretty_names, i,
                   packet->pretty_names[i]);
      }
      need_gui_remove = TRUE;
      need_gui_add = TRUE;
    } else {
      /* Check if a value changed, then we need to reset the list
       * of possible values. */
      const char *str;

      for (i = 0; i < packet->values_num; i++) {
        str = strvec_get(psoption->enumerator.pretty_names, i);
        if (NULL == str || 0 != strcmp(str, packet->pretty_names[i])) {
          /* Store untranslated string from server. */
          strvec_set(psoption->enumerator.pretty_names, i,
                     packet->pretty_names[i]);
          need_gui_remove = TRUE;
          need_gui_add = TRUE;
        }
        /* Support names are not visible, we don't need to check if it
         * has changed. */
        strvec_set(psoption->enumerator.support_names, i,
                   packet->support_names[i]);
      }
    }
  }

  handle_server_setting_common(psoption, packet);
}

/************************************************************************//**
  Receive a bitwise server setting info packet.
****************************************************************************/
void handle_server_setting_bitwise
    (const struct packet_server_setting_bitwise *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);
  bool need_gui_remove = FALSE;
  bool need_gui_add = FALSE;

  fc_assert_ret(NULL != poption);

  if (NULL == poption->common_vtable) {
    /* Not initialized yet. */
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_BITWISE;
    poption->bitwise_vtable = &server_option_bitwise_vtable;
  }
  fc_assert_ret_msg(OT_BITWISE == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_BITWISE), OT_BITWISE);

  if (packet->is_visible) {
    int i;

    psoption->bitwise.value = packet->val;
    psoption->bitwise.def = packet->default_val;

    if (NULL == psoption->bitwise.support_names) {
      /* First time we get this packet. */
      fc_assert(NULL == psoption->bitwise.pretty_names);
      psoption->bitwise.support_names = strvec_new();
      strvec_reserve(psoption->bitwise.support_names, packet->bits_num);
      psoption->bitwise.pretty_names = strvec_new();
      strvec_reserve(psoption->bitwise.pretty_names, packet->bits_num);
      for (i = 0; i < packet->bits_num; i++) {
        strvec_set(psoption->bitwise.support_names, i,
                   packet->support_names[i]);
        /* Store untranslated string from server. */
        strvec_set(psoption->bitwise.pretty_names, i,
                   packet->pretty_names[i]);
      }
    } else if (strvec_size(psoption->bitwise.support_names)
               != packet->bits_num) {
      fc_assert(strvec_size(psoption->bitwise.support_names)
                == strvec_size(psoption->bitwise.pretty_names));
      /* The number of values have changed, we need to reset the list
       * of possible values. */
      strvec_reserve(psoption->bitwise.support_names, packet->bits_num);
      strvec_reserve(psoption->bitwise.pretty_names, packet->bits_num);
      for (i = 0; i < packet->bits_num; i++) {
        strvec_set(psoption->bitwise.support_names, i,
                   packet->support_names[i]);
        /* Store untranslated string from server. */
        strvec_set(psoption->bitwise.pretty_names, i,
                   packet->pretty_names[i]);
      }
      need_gui_remove = TRUE;
      need_gui_add = TRUE;
    } else {
      /* Check if a value changed, then we need to reset the list
       * of possible values. */
      const char *str;

      for (i = 0; i < packet->bits_num; i++) {
        str = strvec_get(psoption->bitwise.pretty_names, i);
        if (NULL == str || 0 != strcmp(str, packet->pretty_names[i])) {
          /* Store untranslated string from server. */
          strvec_set(psoption->bitwise.pretty_names, i,
                     packet->pretty_names[i]);
          need_gui_remove = TRUE;
          need_gui_add = TRUE;
        }
        /* Support names are not visible, we don't need to check if it
         * has changed. */
        strvec_set(psoption->bitwise.support_names, i,
                   packet->support_names[i]);
      }
    }
  }

  handle_server_setting_common(psoption, packet);
}

/************************************************************************//**
  Returns the next valid option pointer for the current gui type.
****************************************************************************/
static struct server_option *
    server_option_next_valid(struct server_option *poption)
{
  const struct server_option *const max = 
    server_options + server_options_num;

  while (NULL != poption && poption < max && !poption->is_visible) {
    poption++;
  }

  return (poption < max ? poption : NULL);
}

/************************************************************************//**
  Returns the server option associated to the number
****************************************************************************/
struct option *server_optset_option_by_number(int id)
{
  if (0 > id || id > server_options_num)  {
    return NULL;
  }
  return OPTION(server_options + id);
}

/************************************************************************//**
  Returns the first valid (visible) option pointer.
****************************************************************************/
struct option *server_optset_option_first(void)
{
  return OPTION(server_option_next_valid(server_options));
}

/************************************************************************//**
  Returns the number of server option categories.
****************************************************************************/
int server_optset_category_number(void)
{
  return server_options_categories_num;
}

/************************************************************************//**
  Returns the name (translated) of the server option category.
****************************************************************************/
const char *server_optset_category_name(int category)
{
  if (0 > category || category >= server_options_categories_num) {
    return NULL;
  }

  return server_options_categories[category];
}

/************************************************************************//**
  Returns the number of this server option.
****************************************************************************/
static int server_option_number(const struct option *poption)
{
  return SERVER_OPTION(poption) - server_options;
}

/************************************************************************//**
  Returns the name of this server option.
****************************************************************************/
static const char *server_option_name(const struct option *poption)
{
  return SERVER_OPTION(poption)->name;
}

/************************************************************************//**
  Returns the (translated) description of this server option.
****************************************************************************/
static const char *server_option_description(const struct option *poption)
{
  return SERVER_OPTION(poption)->description;
}

/************************************************************************//**
  Returns the (translated) help text for this server option.
****************************************************************************/
static const char *server_option_help_text(const struct option *poption)
{
  return SERVER_OPTION(poption)->help_text;
}

/************************************************************************//**
  Returns the category of this server option.
****************************************************************************/
static int server_option_category(const struct option *poption)
{
  return SERVER_OPTION(poption)->category;
}

/************************************************************************//**
  Returns TRUE if this client option can be modified.
****************************************************************************/
static bool server_option_is_changeable(const struct option *poption)
{
  return SERVER_OPTION(poption)->is_changeable;
}

/************************************************************************//**
  Returns the next valid (visible) option pointer.
****************************************************************************/
static struct option *server_option_next(const struct option *poption)
{
  return OPTION(server_option_next_valid(SERVER_OPTION(poption) + 1));
}

/************************************************************************//**
  Returns the value of this server option of type OT_BOOLEAN.
****************************************************************************/
static bool server_option_bool_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->boolean.value;
}

/************************************************************************//**
  Returns the default value of this server option of type OT_BOOLEAN.
****************************************************************************/
static bool server_option_bool_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->boolean.def;
}

/************************************************************************//**
  Set the value of this server option of type OT_BOOLEAN.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool server_option_bool_set(struct option *poption, bool val)
{
  struct server_option *psoption = SERVER_OPTION(poption);

  if (psoption->boolean.value == val) {
    return FALSE;
  }

  send_chat_printf("/set %s %s", psoption->name,
                   val ? "enabled" : "disabled");
  return TRUE;
}

/************************************************************************//**
  Returns the value of this server option of type OT_INTEGER.
****************************************************************************/
static int server_option_int_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.value;
}

/************************************************************************//**
  Returns the default value of this server option of type OT_INTEGER.
****************************************************************************/
static int server_option_int_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.def;
}

/************************************************************************//**
  Returns the minimal value for this server option of type OT_INTEGER.
****************************************************************************/
static int server_option_int_min(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.min;
}

/************************************************************************//**
  Returns the maximal value for this server option of type OT_INTEGER.
****************************************************************************/
static int server_option_int_max(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.max;
}

/************************************************************************//**
  Set the value of this server option of type OT_INTEGER.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool server_option_int_set(struct option *poption, int val)
{
  struct server_option *psoption = SERVER_OPTION(poption);

  if (val < psoption->integer.min
      || val > psoption->integer.max
      || psoption->integer.value == val) {
    return FALSE;
  }

  send_chat_printf("/set %s %d", psoption->name, val);
  return TRUE;
}

/************************************************************************//**
  Returns the value of this server option of type OT_STRING.
****************************************************************************/
static const char *server_option_str_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->string.value;
}

/************************************************************************//**
  Returns the default value of this server option of type OT_STRING.
****************************************************************************/
static const char *server_option_str_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->string.def;
}

/************************************************************************//**
  Returns the possible string values of this server option of type
  OT_STRING.
****************************************************************************/
static const struct strvec *
    server_option_str_values(const struct option *poption)
{
  return NULL;
}

/************************************************************************//**
  Set the value of this server option of type OT_STRING.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool server_option_str_set(struct option *poption, const char *str)
{
  struct server_option *psoption = SERVER_OPTION(poption);

  if (0 == strcmp(psoption->string.value, str)) {
    return FALSE;
  }

  send_chat_printf("/set %s \"%s\"", psoption->name, str);
  return TRUE;
}

/************************************************************************//**
  Returns the current value of this server option of type OT_ENUM.
****************************************************************************/
static int server_option_enum_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->enumerator.value;
}

/************************************************************************//**
  Returns the default value of this server option of type OT_ENUM.
****************************************************************************/
static int server_option_enum_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->enumerator.def;
}

/************************************************************************//**
  Returns the user-visible, translatable (but untranslated) "pretty" names
  of this server option of type OT_ENUM.
****************************************************************************/
static const struct strvec *
    server_option_enum_pretty(const struct option *poption)
{
  return SERVER_OPTION(poption)->enumerator.pretty_names;
}

/************************************************************************//**
  Set the value of this server option of type OT_ENUM.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool server_option_enum_set(struct option *poption, int val)
{
  struct server_option *psoption = SERVER_OPTION(poption);
  const char *name;

  if (val == psoption->enumerator.value
      || !(name = strvec_get(psoption->enumerator.support_names, val))) {
    return FALSE;
  }

  send_chat_printf("/set %s \"%s\"", psoption->name, name);
  return TRUE;
}

/************************************************************************//**
  Returns the long support names of the values of the server option of type
  OT_ENUM.
****************************************************************************/
static void server_option_enum_support_name(const struct option *poption,
                                            const char **pvalue,
                                            const char **pdefault)
{
  const struct server_option *psoption = SERVER_OPTION(poption);
  const struct strvec *values = psoption->enumerator.support_names;

  if (NULL != pvalue) {
    *pvalue = strvec_get(values, psoption->enumerator.value);
  }
  if (NULL != pdefault) {
    *pdefault = strvec_get(values, psoption->enumerator.def);
  }
}

/************************************************************************//**
  Returns the current value of this server option of type OT_BITWISE.
****************************************************************************/
static unsigned server_option_bitwise_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->bitwise.value;
}

/************************************************************************//**
  Returns the default value of this server option of type OT_BITWISE.
****************************************************************************/
static unsigned server_option_bitwise_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->bitwise.def;
}

/************************************************************************//**
  Returns the user-visible, translatable (but untranslated) "pretty" names
  of this server option of type OT_BITWISE.
****************************************************************************/
static const struct strvec *
    server_option_bitwise_pretty(const struct option *poption)
{
  return SERVER_OPTION(poption)->bitwise.pretty_names;
}

/************************************************************************//**
  Compute the long support names of a value.
****************************************************************************/
static void server_option_bitwise_support_base(const struct strvec *values,
                                               unsigned val,
                                               char *buf, size_t buf_len)
{
  int bit;

  buf[0] = '\0';
  for (bit = 0; bit < strvec_size(values); bit++) {
    if ((1 << bit) & val) {
      if ('\0' != buf[0]) {
        fc_strlcat(buf, "|", buf_len);
      }
      fc_strlcat(buf, strvec_get(values, bit), buf_len);
    }
  }
}

/************************************************************************//**
  Set the value of this server option of type OT_BITWISE.  Returns TRUE if
  the value changed.
****************************************************************************/
static bool server_option_bitwise_set(struct option *poption, unsigned val)
{
  struct server_option *psoption = SERVER_OPTION(poption);
  char name[MAX_LEN_MSG];

  if (val == psoption->bitwise.value) {
    return FALSE;
  }

  server_option_bitwise_support_base(psoption->bitwise.support_names, val,
                                     name, sizeof(name));
  send_chat_printf("/set %s \"%s\"", psoption->name, name);
  return TRUE;
}

/************************************************************************//**
  Compute the long support names of the values of the server option of type
  OT_BITWISE.
****************************************************************************/
static void server_option_bitwise_support_name(const struct option *poption,
                                               char *val_buf, size_t val_len,
                                               char *def_buf, size_t def_len)
{
  const struct server_option *psoption = SERVER_OPTION(poption);
  const struct strvec *values = psoption->bitwise.support_names;

  if (NULL != val_buf && 0 < val_len) {
    server_option_bitwise_support_base(values, psoption->bitwise.value,
                                       val_buf, val_len);
  }
  if (NULL != def_buf && 0 < def_len) {
    server_option_bitwise_support_base(values, psoption->bitwise.def,
                                       def_buf, def_len);
  }
}


/** Message Options: **/

int messages_where[E_COUNT];


/************************************************************************//**
  These could be a static table initialisation, except
  its easier to do it this way.
****************************************************************************/
static void message_options_init(void)
{
  int none[] = {
    E_IMP_BUY, E_IMP_SOLD, E_UNIT_BUY,
    E_UNIT_LOST_ATT, E_UNIT_WIN_ATT, E_GAME_START,
    E_CITY_BUILD, E_NEXT_YEAR,
    E_CITY_PRODUCTION_CHANGED,
    E_CITY_MAY_SOON_GROW, E_WORKLIST, E_AI_DEBUG
  };
  int out_only[] = {
    E_NATION_SELECTED, E_CHAT_MSG, E_CHAT_ERROR, E_CONNECTION,
    E_LOG_ERROR, E_SETTING, E_VOTE_NEW, E_VOTE_RESOLVED, E_VOTE_ABORTED
  };
  int all[] = {
    E_LOG_FATAL, E_SCRIPT, E_DEPRECATION_WARNING, E_MESSAGE_WALL
  };
  int i;

  for (i = 0; i <= event_type_max(); i++) {
    /* Include possible undefined values. */
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

/************************************************************************//**
  Free resources allocated for message options system
****************************************************************************/
static void message_options_free(void)
{
  events_free();
}

/************************************************************************//**
  Load the message options; use the function defined by
  specnum.h (see also events.h).
****************************************************************************/
static void message_options_load(struct section_file *file,
                                 const char *prefix)
{
  enum event_type event;
  int i, num_events;
  const char *p;

  if (!secfile_lookup_int(file, &num_events, "messages.count")) {
    /* version < 2.2 */
    /* Order of the events in 2.1. */
    const enum event_type old_events[] = {
      E_CITY_CANTBUILD, E_CITY_LOST, E_CITY_LOVE, E_CITY_DISORDER,
      E_CITY_FAMINE, E_CITY_FAMINE_FEARED, E_CITY_GROWTH,
      E_CITY_MAY_SOON_GROW, E_CITY_AQUEDUCT, E_CITY_AQ_BUILDING,
      E_CITY_NORMAL, E_CITY_NUKED, E_CITY_CMA_RELEASE, E_CITY_GRAN_THROTTLE,
      E_CITY_TRANSFER, E_CITY_BUILD, E_CITY_PRODUCTION_CHANGED,
      E_WORKLIST, E_UPRISING, E_CIVIL_WAR, E_ANARCHY, E_FIRST_CONTACT,
      E_NEW_GOVERNMENT, E_LOW_ON_FUNDS, E_POLLUTION, E_REVOLT_DONE,
      E_REVOLT_START, E_SPACESHIP, E_MY_DIPLOMAT_BRIBE,
      E_DIPLOMATIC_INCIDENT, E_MY_DIPLOMAT_ESCAPE, E_MY_DIPLOMAT_EMBASSY,
      E_MY_DIPLOMAT_FAILED, E_MY_DIPLOMAT_INCITE, E_MY_DIPLOMAT_POISON,
      E_MY_DIPLOMAT_SABOTAGE, E_MY_DIPLOMAT_THEFT, E_ENEMY_DIPLOMAT_BRIBE,
      E_ENEMY_DIPLOMAT_EMBASSY, E_ENEMY_DIPLOMAT_FAILED,
      E_ENEMY_DIPLOMAT_INCITE, E_ENEMY_DIPLOMAT_POISON,
      E_ENEMY_DIPLOMAT_SABOTAGE, E_ENEMY_DIPLOMAT_THEFT,
      E_CARAVAN_ACTION, E_SCRIPT, E_BROADCAST_REPORT, E_GAME_END,
      E_GAME_START, E_NATION_SELECTED, E_DESTROYED, E_REPORT, E_TURN_BELL,
      E_NEXT_YEAR, E_GLOBAL_ECO, E_NUKE, E_HUT_BARB, E_HUT_CITY, E_HUT_GOLD,
      E_HUT_BARB_KILLED, E_HUT_MERC, E_HUT_SETTLER, E_HUT_TECH,
      E_HUT_BARB_CITY_NEAR, E_IMP_BUY, E_IMP_BUILD, E_IMP_AUCTIONED,
      E_IMP_AUTO, E_IMP_SOLD, E_TECH_GAIN, E_TECH_LEARNED, E_TREATY_ALLIANCE,
      E_TREATY_BROKEN, E_TREATY_CEASEFIRE, E_TREATY_PEACE,
      E_TREATY_SHARED_VISION, E_UNIT_LOST_ATT, E_UNIT_WIN_ATT, E_UNIT_BUY,
      E_UNIT_BUILT, E_UNIT_LOST_DEF, E_UNIT_WIN_DEF, E_UNIT_BECAME_VET,
      E_UNIT_UPGRADED, E_UNIT_RELOCATED, E_UNIT_ORDERS, E_WONDER_BUILD,
      E_WONDER_OBSOLETE, E_WONDER_STARTED, E_WONDER_STOPPED,
      E_WONDER_WILL_BE_BUILT, E_DIPLOMACY, E_TREATY_EMBASSY,
      E_BAD_COMMAND, E_SETTING, E_CHAT_MSG, E_MESSAGE_WALL, E_CHAT_ERROR,
      E_CONNECTION, E_AI_DEBUG
    };
    const size_t old_events_num = ARRAY_SIZE(old_events);

    for (i = 0; i < old_events_num; i++) {
      messages_where[old_events[i]] =
        secfile_lookup_int_default(file, messages_where[old_events[i]],
                                   "%s.message_where_%02d", prefix, i);
    }
    return;
  }

  for (i = 0; i < num_events; i++) {
    p = secfile_lookup_str(file, "messages.event%d.name", i);
    if (NULL == p) {
      log_error("Corruption in file %s: %s",
                secfile_name(file), secfile_error());
      continue;
    }
    /* Compatibility: Before 3.0 E_UNIT_WIN_DEF was called E_UNIT_WIN. */
    if (!fc_strcasecmp("E_UNIT_WIN", p)) {
      log_deprecation(_("Deprecated event type E_UNIT_WIN in client options."));
      p = "E_UNIT_WIN_DEF";
    }
    event = event_type_by_name(p, strcmp);
    if (!event_type_is_valid(event)) {
      log_error("Event not supported: %s", p);
      continue;
    }

    if (!secfile_lookup_int(file, &messages_where[event],
                            "messages.event%d.where", i)) {
      log_error("Corruption in file %s: %s",
                secfile_name(file), secfile_error());
    }
  }
}

/************************************************************************//**
  Save the message options; use the function defined by
  specnum.h (see also events.h).
****************************************************************************/
static void message_options_save(struct section_file *file,
                                 const char *prefix)
{
  enum event_type event;
  int i = 0;

  for (event = event_type_begin(); event != event_type_end();
       event = event_type_next(event)) {
    secfile_insert_str(file, event_type_name(event),
                       "messages.event%d.name", i);
    secfile_insert_int(file, messages_where[i],
                       "messages.event%d.where", i);
    i++;
  }

  secfile_insert_int(file, i, "messages.count");
}

/************************************************************************//**
  Does heavy lifting for looking up a preset.
****************************************************************************/
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

/************************************************************************//**
  Does heavy lifting for inserting a preset.
****************************************************************************/
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

/************************************************************************//**
  Insert all cma presets.
****************************************************************************/
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

/* Old rc file name. */
#define OLD_OPTION_FILE_NAME ".civclientrc"
/* New rc file name. */
#define MID_OPTION_FILE_NAME ".freeciv-client-rc-%d.%d"
#define NEW_OPTION_FILE_NAME "freeciv-client-rc-%d.%d"
#if MINOR_VERSION >= 90
#define MAJOR_NEW_OPTION_FILE_NAME (MAJOR_VERSION + 1)
#define MINOR_NEW_OPTION_FILE_NAME 0
#else /* MINOR_VERSION < 90 */
#define MAJOR_NEW_OPTION_FILE_NAME MAJOR_VERSION
#if IS_DEVEL_VERSION && ! IS_FREEZE_VERSION
#define MINOR_NEW_OPTION_FILE_NAME (MINOR_VERSION + 1)
#else
#define MINOR_NEW_OPTION_FILE_NAME MINOR_VERSION
#endif /* IS_DEVEL_VERSION */
#endif /* MINOR_VERSION >= 90 */
/* The first version the new option name appeared (2.6). */
#define FIRST_MAJOR_NEW_OPTION_FILE_NAME 2
#define FIRST_MINOR_NEW_OPTION_FILE_NAME 6
/* The first version the mid option name appeared (2.2). */
#define FIRST_MAJOR_MID_OPTION_FILE_NAME 2
#define FIRST_MINOR_MID_OPTION_FILE_NAME 2
/* The first version the new boolean values appeared (2.3). */
#define FIRST_MAJOR_NEW_BOOLEAN 2
#define FIRST_MINOR_NEW_BOOLEAN 3

/************************************************************************//**
  Returns pointer to static memory containing name of the current
  option file.  Usually used for saving.
  Ie, based on FREECIV_OPT env var, and freeciv storage root dir.
  (or a OPTION_FILE_NAME define defined in fc_config.h)
  Or NULL if problem.
****************************************************************************/
static const char *get_current_option_file_name(void)
{
  static char name_buffer[256];
  const char *name;

  name = getenv("FREECIV_OPT");

  if (name) {
    sz_strlcpy(name_buffer, name);
  } else {
#ifdef OPTION_FILE_NAME
    fc_strlcpy(name_buffer, OPTION_FILE_NAME, sizeof(name_buffer));
#else
    name = freeciv_storage_dir();
    if (!name) {
      log_error(_("Cannot find freeciv storage directory"));
      return NULL;
    }
    fc_snprintf(name_buffer, sizeof(name_buffer),
                "%s" DIR_SEPARATOR NEW_OPTION_FILE_NAME, name,
                MAJOR_NEW_OPTION_FILE_NAME, MINOR_NEW_OPTION_FILE_NAME);
#endif /* OPTION_FILE_NAME */
  }
  log_verbose("settings file is %s", name_buffer);
  return name_buffer;
}

/************************************************************************//**
  Check the last option file we saved. Usually used to load. Ie, based on
  FREECIV_OPT env var, and home dir. (or a OPTION_FILE_NAME define defined
  in fc_config.h), or NULL if not found.

  Set in allow_digital_boolean if we should look for old boolean values
  (saved as 0 and 1), so if the rc file version is older than 2.3.0.
****************************************************************************/
static const char *get_last_option_file_name(bool *allow_digital_boolean)
{
  static char name_buffer[256];
  const char *name;
  static int last_minors[] = {
    0,  /* There was no 0.x releases */
    14, /* 1.14 */
    7   /* 2.7 */
  };

#if MINOR_VERSION >= 90
  FC_STATIC_ASSERT(MAJOR_VERSION < sizeof(last_minors) / sizeof(int), missing_last_minor);
#else
  FC_STATIC_ASSERT(MAJOR_VERSION <= sizeof(last_minors) / sizeof(int), missing_last_minor);
#endif

  *allow_digital_boolean = FALSE;
  name = getenv("FREECIV_OPT");
  if (name) {
    sz_strlcpy(name_buffer, name);
  } else {
#ifdef OPTION_FILE_NAME
    fc_strlcpy(name_buffer, OPTION_FILE_NAME, sizeof(name_buffer));
#else
    int major, minor;
    struct stat buf;

    name = freeciv_storage_dir();
    if (name == NULL) {
      log_error(_("Cannot find freeciv storage directory"));

      return NULL;
    }

    for (major = MAJOR_NEW_OPTION_FILE_NAME,
         minor = MINOR_NEW_OPTION_FILE_NAME;
         major >= FIRST_MAJOR_NEW_OPTION_FILE_NAME; major--) {
      for (; (major == FIRST_MAJOR_NEW_OPTION_FILE_NAME
              ? minor >= FIRST_MINOR_NEW_OPTION_FILE_NAME 
              : minor >= 0); minor--) {
        fc_snprintf(name_buffer, sizeof(name_buffer),
                    "%s" DIR_SEPARATOR NEW_OPTION_FILE_NAME, name, major, minor);
        if (0 == fc_stat(name_buffer, &buf)) {
          if (MAJOR_NEW_OPTION_FILE_NAME != major
              || MINOR_NEW_OPTION_FILE_NAME != minor) {
            log_normal(_("Didn't find '%s' option file, "
                         "loading from '%s' instead."),
                       get_current_option_file_name() + strlen(name) + 1,
                       name_buffer + strlen(name) + 1);
          }

          return name_buffer;
        }
      }
      minor = last_minors[major - 1];
    }

    /* Older versions had options file in user home directory */
    name = user_home_dir();
    if (name == NULL) {
      log_error(_("Cannot find your home directory"));

      return NULL;
    }

    /* minor having max value of FIRST_MINOR_NEW_OPTION_FILE_NAME
     * works since MID versioning scheme was used within major version 2
     * only (2.2 - 2.6) so the last minor is bigger than any earlier minor. */
    for (major = FIRST_MAJOR_MID_OPTION_FILE_NAME,
         minor = FIRST_MINOR_NEW_OPTION_FILE_NAME ;
         minor >= FIRST_MINOR_MID_OPTION_FILE_NAME ;
         minor--) {
      fc_snprintf(name_buffer, sizeof(name_buffer),
                  "%s" DIR_SEPARATOR MID_OPTION_FILE_NAME, name, major, minor);
      if (0 == fc_stat(name_buffer, &buf)) {
        log_normal(_("Didn't find '%s' option file, "
                     "loading from '%s' instead."),
                   get_current_option_file_name() + strlen(name) + 1,
                   name_buffer + strlen(name) + 1);

        if (FIRST_MINOR_NEW_BOOLEAN > minor) {
          *allow_digital_boolean = TRUE;
        }
        return name_buffer;
      }
    }

    /* Try with the old one. */
    fc_snprintf(name_buffer, sizeof(name_buffer),
                "%s" DIR_SEPARATOR OLD_OPTION_FILE_NAME, name);
    if (0 == fc_stat(name_buffer, &buf)) {
      log_normal(_("Didn't find '%s' option file, "
                   "loading from '%s' instead."),
                 get_current_option_file_name() + strlen(name) + 1,
                 OLD_OPTION_FILE_NAME);
      *allow_digital_boolean = TRUE;
      return name_buffer;
    } else {
      return NULL;
    }
#endif /* OPTION_FILE_NAME */
  }
  log_verbose("settings file is %s", name_buffer);
  return name_buffer;
}
#undef OLD_OPTION_FILE_NAME
#undef MID_OPTION_FILE_NAME
#undef NEW_OPTION_FILE_NAME
#undef FIRST_MAJOR_NEW_OPTION_FILE_NAME
#undef FIRST_MINOR_NEW_OPTION_FILE_NAME
#undef FIRST_MAJOR_MID_OPTION_FILE_NAME
#undef FIRST_MINOR_MID_OPTION_FILE_NAME
#undef FIRST_MINOR_NEW_BOOLEAN


/****************************************************************************
  Desired settable options.
****************************************************************************/
#define SPECHASH_TAG settable_options
#define SPECHASH_ASTR_KEY_TYPE
#define SPECHASH_ASTR_DATA_TYPE
#include "spechash.h"
#define settable_options_hash_iterate(hash, name, value)                    \
  TYPED_HASH_ITERATE(const char *, const char *, hash, name, value)
#define settable_options_hash_iterate_end HASH_ITERATE_END

static struct settable_options_hash *settable_options_hash = NULL;

/************************************************************************//**
  Load the server options.
****************************************************************************/
static void settable_options_load(struct section_file *sf)
{
  char buf[64];
  const struct section *psection;
  const struct entry_list *entries;
  const char *string;
  bool bval;
  int ival;

  fc_assert_ret(NULL != settable_options_hash);

  settable_options_hash_clear(settable_options_hash);

  psection = secfile_section_by_name(sf, "server");
  if (NULL == psection) {
    /* Does not exist! */
    return;
  }

  entries = section_entries(psection);
  entry_list_iterate(entries, pentry) {
    string = NULL;
    switch (entry_type(pentry)) {
    case ENTRY_BOOL:
      if (entry_bool_get(pentry, &bval)) {
        fc_strlcpy(buf, bval ? "enabled" : "disabled", sizeof(buf));
        string = buf;
      }
      break;

    case ENTRY_INT:
      if (entry_int_get(pentry, &ival)) {
        fc_snprintf(buf, sizeof(buf), "%d", ival);
        string = buf;
      }
      break;

    case ENTRY_STR:
      (void) entry_str_get(pentry, &string);
      break;

    case ENTRY_FLOAT:
    case ENTRY_FILEREFERENCE:
      /* Not supported yet */
      break;
    }

    if (NULL == string) {
      log_error("Entry type variant of \"%s.%s\" is not supported.",
                section_name(psection), entry_name(pentry));
      continue;
    }

    settable_options_hash_insert(settable_options_hash, entry_name(pentry),
                                 string);
  } entry_list_iterate_end;
}

/************************************************************************//**
  Save the desired server options.
****************************************************************************/
static void settable_options_save(struct section_file *sf)
{
  fc_assert_ret(NULL != settable_options_hash);

  settable_options_hash_iterate(settable_options_hash, name, value) {
    if (!fc_strcasecmp(name, "gameseed") || !fc_strcasecmp(name, "mapseed")) {
      /* Do not save mapseed or gameseed. */
      continue;
    }
    if (!fc_strcasecmp(name, "topology")) {
      /* client_start_server() sets topology based on tileset. Don't store
       * its choice. The tileset is already stored. Storing topology leads
       * to all sort of breakage:
       * - it breaks ruleset default topology.
       * - it interacts badly with tileset ruleset change, ruleset tileset
       *   change and topology tileset change.
       * - its value is probably based on what tileset was loaded when
       *   client_start_server() decided to set topology, not on player
       *   choice.
       */
      continue;
    }
    secfile_insert_str(sf, value, "server.%s", name);
  } settable_options_hash_iterate_end;
}

/************************************************************************//**
  Update the desired settable options hash table from the current
  setting configuration.
****************************************************************************/
void desired_settable_options_update(void)
{
  char val_buf[1024], def_buf[1024];
  const char *value, *def_val;

  fc_assert_ret(NULL != settable_options_hash);

  options_iterate(server_optset, poption) {
    value = NULL;
    def_val = NULL;
    switch (option_type(poption)) {
    case OT_BOOLEAN:
      fc_strlcpy(val_buf, option_bool_get(poption) ? "enabled" : "disabled",
                 sizeof(val_buf));
      value = val_buf;
      fc_strlcpy(def_buf, option_bool_def(poption) ? "enabled" : "disabled",
                 sizeof(def_buf));
      def_val = def_buf;
      break;
    case OT_INTEGER:
      fc_snprintf(val_buf, sizeof(val_buf), "%d", option_int_get(poption));
      value = val_buf;
      fc_snprintf(def_buf, sizeof(def_buf), "%d", option_int_def(poption));
      def_val = def_buf;
      break;
    case OT_STRING:
      value = option_str_get(poption);
      def_val = option_str_def(poption);
      break;
    case OT_ENUM:
      server_option_enum_support_name(poption, &value, &def_val);
      break;
    case OT_BITWISE:
      server_option_bitwise_support_name(poption, val_buf, sizeof(val_buf),
                                         def_buf, sizeof(def_buf));
      value = val_buf;
      def_val = def_buf;
      break;
    case OT_FONT:
    case OT_COLOR:
    case OT_VIDEO_MODE:
      break;
    }

    if (NULL == value || NULL == def_val) {
      log_error("Option type %s (%d) not supported for '%s'.",
                option_type_name(option_type(poption)), option_type(poption),
                option_name(poption));
      continue;
    }

    if (0 == strcmp(value, def_val)) {
      /* Not set, using default... */
      settable_options_hash_remove(settable_options_hash,
                                   option_name(poption));
    } else {
      /* Really desired. */
      settable_options_hash_replace(settable_options_hash,
                                    option_name(poption), value);
    }
  } options_iterate_end;
}

/************************************************************************//**
  Update a desired settable option in the hash table from a value
  which can be different of the current configuration.
****************************************************************************/
void desired_settable_option_update(const char *op_name,
                                    const char *op_value,
                                    bool allow_replace)
{
  fc_assert_ret(NULL != settable_options_hash);

  if (allow_replace) {
    settable_options_hash_replace(settable_options_hash, op_name, op_value);
  } else {
    settable_options_hash_insert(settable_options_hash, op_name, op_value);
  }
}

/************************************************************************//**
  Convert old integer to new values (Freeciv 2.2.x to Freeciv 2.3.x).
  Very ugly hack. TODO: Remove this later.
****************************************************************************/
static bool settable_option_upgrade_value(const struct option *poption,
                                          int old_value,
                                          char *buf, size_t buf_len)
{
  const char *name = option_name(poption);

#define SETTING_CASE(ARG_name, ...)                                         \
  if (0 == strcmp(ARG_name, name)) {                                        \
    static const char *values[] = { __VA_ARGS__ };                          \
    if (0 <= old_value && old_value < ARRAY_SIZE(values)                    \
        && NULL != values[old_value]) {                                     \
      fc_strlcpy(buf, values[old_value], buf_len);                          \
      return TRUE;                                                          \
    } else {                                                                \
      return FALSE;                                                         \
    }                                                                       \
  }

  SETTING_CASE("topology", "", "WRAPX", "WRAPY", "WRAPX|WRAPY", "ISO",
               "WRAPX|ISO", "WRAPY|ISO", "WRAPX|WRAPY|ISO", "HEX",
               "WRAPX|HEX", "WRAPY|HEX", "WRAPX|WRAPY|HEX", "ISO|HEX",
               "WRAPX|ISO|HEX", "WRAPY|ISO|HEX", "WRAPX|WRAPY|ISO|HEX");
  SETTING_CASE("generator", NULL, "RANDOM", "FRACTAL", "ISLAND");
  SETTING_CASE("startpos", "DEFAULT", "SINGLE", "2or3", "ALL", "VARIABLE");
  SETTING_CASE("borders", "DISABLED", "ENABLED", "SEE_INSIDE", "EXPAND");
  SETTING_CASE("diplomacy", "ALL", "HUMAN", "AI", "TEAM", "DISABLED");
  SETTING_CASE("citynames", "NO_RESTRICTIONS", "PLAYER_UNIQUE",
               "GLOBAL_UNIQUE", "NO_STEALING");
  SETTING_CASE("barbarians", "DISABLED", "HUTS_ONLY", "NORMAL", "FREQUENT",
               "HORDES");
  SETTING_CASE("phasemode", "ALL", "PLAYER", "TEAM");
  SETTING_CASE("compresstype", "PLAIN", "LIBZ", "BZIP2");

#undef SETTING_CASE
  return FALSE;
}

/************************************************************************//**
  Send the desired server options to the server.
****************************************************************************/
static void desired_settable_option_send(struct option *poption)
{
  char *desired;
  int value;

  fc_assert_ret(NULL != settable_options_hash);

  if (!settable_options_hash_lookup(settable_options_hash,
                                    option_name(poption), &desired)) {
    /* No change explicitly  desired. */
    return;
  }

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    if ((0 == fc_strcasecmp("enabled", desired)
         || (str_to_int(desired, &value) && 1 == value))
        && !option_bool_get(poption)) {
      send_chat_printf("/set %s enabled", option_name(poption));
    } else if ((0 == fc_strcasecmp("disabled", desired)
                || (str_to_int(desired, &value) && 0 == value))
               && option_bool_get(poption)) {
      send_chat_printf("/set %s disabled", option_name(poption));
    }
    return;
  case OT_INTEGER:
    if (str_to_int(desired, &value) && value != option_int_get(poption)) {
      send_chat_printf("/set %s %d", option_name(poption), value);
    }
    return;
  case OT_STRING:
    if (0 != strcmp(desired, option_str_get(poption))) {
      send_chat_printf("/set %s \"%s\"", option_name(poption), desired);
    }
    return;
  case OT_ENUM:
    {
      char desired_buf[256];
      const char *value_str;

      /* Handle old values. */
      if (str_to_int(desired, &value)
          && settable_option_upgrade_value(poption, value, desired_buf,
                                           sizeof(desired_buf))) {
        desired = desired_buf;
      }

      server_option_enum_support_name(poption, &value_str, NULL);
      if (0 != strcmp(desired, value_str)) {
        send_chat_printf("/set %s \"%s\"", option_name(poption), desired);
      }
    }
    return;
  case OT_BITWISE:
    {
      char desired_buf[256], value_buf[256];

      /* Handle old values. */
      if (str_to_int(desired, &value)
          && settable_option_upgrade_value(poption, value, desired_buf,
                                           sizeof(desired_buf))) {
        desired = desired_buf;
      }

      server_option_bitwise_support_name(poption, value_buf,
                                         sizeof(value_buf), NULL, 0);
      if (0 != strcmp(desired, value_buf)) {
        send_chat_printf("/set %s \"%s\"", option_name(poption), desired);
      }
    }
    return;
  case OT_FONT:
  case OT_COLOR:
  case OT_VIDEO_MODE:
    break;
  }

  log_error("Option type %s (%d) not supported for '%s'.",
            option_type_name(option_type(poption)), option_type(poption),
            option_name(poption));
}


/****************************************************************************
  City and player report dialog options.
****************************************************************************/
#define SPECHASH_TAG dialog_options
#define SPECHASH_ASTR_KEY_TYPE
#define SPECHASH_IDATA_TYPE bool
#define SPECHASH_UDATA_TO_IDATA FC_INT_TO_PTR
#define SPECHASH_IDATA_TO_UDATA FC_PTR_TO_INT
#include "spechash.h"
#define dialog_options_hash_iterate(hash, column, visible)                  \
  TYPED_HASH_ITERATE(const char *, intptr_t, hash, column, visible)
#define dialog_options_hash_iterate_end HASH_ITERATE_END

static struct dialog_options_hash *dialog_options_hash = NULL;

/************************************************************************//**
  Load the city and player report dialog options.
****************************************************************************/
static void options_dialogs_load(struct section_file *sf)
{
  const struct entry_list *entries;
  const char *prefixes[] = { "player_dlg_", "city_report_", NULL };
  const char **prefix;
  bool visible;

  fc_assert_ret(NULL != dialog_options_hash);

  entries = section_entries(secfile_section_by_name(sf, "client"));

  if (NULL != entries) {
    entry_list_iterate(entries, pentry) {
      for (prefix = prefixes; NULL != *prefix; prefix++) {
        if (0 == strncmp(*prefix, entry_name(pentry), strlen(*prefix))
            && secfile_lookup_bool(sf, &visible, "client.%s",
                                   entry_name(pentry))) {
          dialog_options_hash_replace(dialog_options_hash,
                                      entry_name(pentry), visible);
          break;
        }
      }
    } entry_list_iterate_end;
  }
}

/************************************************************************//**
  Save the city and player report dialog options.
****************************************************************************/
static void options_dialogs_save(struct section_file *sf)
{
  fc_assert_ret(NULL != dialog_options_hash);

  options_dialogs_update();
  dialog_options_hash_iterate(dialog_options_hash, column, visible) {
    secfile_insert_bool(sf, visible, "client.%s", column);
  } dialog_options_hash_iterate_end;
}

/************************************************************************//**
  This set the city and player report dialog options to the
  current ones.  It's called when the client goes to
  C_S_DISCONNECTED state.
****************************************************************************/
void options_dialogs_update(void)
{
  char buf[64];
  int i;

  fc_assert_ret(NULL != dialog_options_hash);

  /* Player report dialog options. */
  for (i = 1; i < num_player_dlg_columns; i++) {
    fc_snprintf(buf, sizeof(buf), "player_dlg_%s",
                player_dlg_columns[i].tagname);
    dialog_options_hash_replace(dialog_options_hash, buf,
                                player_dlg_columns[i].show);
  }

  /* City report dialog options. */
  for (i = 0; i < num_city_report_spec(); i++) {
    fc_snprintf(buf, sizeof(buf), "city_report_%s",
                city_report_spec_tagname(i));
    dialog_options_hash_replace(dialog_options_hash, buf,
                                *city_report_spec_show_ptr(i));
  }
}

/************************************************************************//**
  This set the city and player report dialog options.  It's called
  when the client goes to C_S_RUNNING state.
****************************************************************************/
void options_dialogs_set(void)
{
  char buf[64];
  bool visible;
  int i;

  fc_assert_ret(NULL != dialog_options_hash);

  /* Player report dialog options. */
  for (i = 1; i < num_player_dlg_columns; i++) {
    fc_snprintf(buf, sizeof(buf), "player_dlg_%s",
                player_dlg_columns[i].tagname);
    if (dialog_options_hash_lookup(dialog_options_hash, buf, &visible)) {
      player_dlg_columns[i].show = visible;
    }
  }

  /* City report dialog options. */
  for (i = 0; i < num_city_report_spec(); i++) {
    fc_snprintf(buf, sizeof(buf), "city_report_%s",
                city_report_spec_tagname(i));
    if (dialog_options_hash_lookup(dialog_options_hash, buf, &visible)) {
      *city_report_spec_show_ptr(i) = visible;
    }
  }
}

/************************************************************************//**
  Load from the rc file any options that are not ruleset specific.
  It is called after ui_init(), yet before ui_main().
  Unfortunately, this means that some clients cannot display.
  Instead, use log_*().
****************************************************************************/
void options_load(void)
{
  struct section_file *sf;
  bool allow_digital_boolean;
  int i, num;
  const char *name;
  const char *const prefix = "client";
  const char *str;

  name = get_last_option_file_name(&allow_digital_boolean);
  if (!name) {
    log_normal(_("Didn't find the option file. Creating a new one."));
    client_option_adjust_defaults();
    options_fully_initialized = TRUE;
    create_default_cma_presets();
    gui_options.first_boot = TRUE;
    return;
  }
  if (!(sf = secfile_load(name, TRUE))) {
    log_debug("Error loading option file '%s':\n%s", name, secfile_error());
    /* try to create the rc file */
    sf = secfile_new(TRUE);
    secfile_insert_str(sf, VERSION_STRING, "client.version");

    create_default_cma_presets();
    save_cma_presets(sf);

    /* FIXME: need better messages */
    if (!secfile_save(sf, name, 0, FZ_PLAIN)) {
      log_error(_("Save failed, cannot write to file %s"), name);
    } else {
      log_normal(_("Saved settings to file %s"), name);
    }
    secfile_destroy(sf);
    options_fully_initialized = TRUE;
    return;
  }
  secfile_allow_digital_boolean(sf, allow_digital_boolean);

  /* a "secret" option for the lazy. TODO: make this saveable */
  sz_strlcpy(password,
             secfile_lookup_str_default(sf, "", "%s.password", prefix));

  gui_options.save_options_on_exit =
    secfile_lookup_bool_default(sf, gui_options.save_options_on_exit,
                                "%s.save_options_on_exit", prefix);
  gui_options.migrate_fullscreen =
    secfile_lookup_bool_default(sf, gui_options.migrate_fullscreen,
                                "%s.fullscreen_mode", prefix);

  /* Settings migrations */
  gui_options.gui_gtk3_migrated_from_gtk2 =
    secfile_lookup_bool_default(sf, gui_options.gui_gtk3_migrated_from_gtk2,
                                "%s.migration_gtk3_from_gtk2", prefix);
  gui_options.gui_gtk3_22_migrated_from_gtk3 =
    secfile_lookup_bool_default(sf, gui_options.gui_gtk3_22_migrated_from_gtk3,
                                "%s.migration_gtk3_22_from_gtk3", prefix);
  gui_options.gui_gtk4_migrated_from_gtk3_22 =
    secfile_lookup_bool_default(sf, gui_options.gui_gtk4_migrated_from_gtk3_22,
                                "%s.migration_gtk4_from_gtk3_22", prefix);
  gui_options.gui_sdl2_migrated_from_sdl =
    secfile_lookup_bool_default(sf, gui_options.gui_sdl2_migrated_from_sdl,
                                "%s.migration_sdl2_from_sdl", prefix);
  gui_options.gui_gtk2_migrated_from_2_5 =
    secfile_lookup_bool_default(sf, gui_options.gui_gtk3_migrated_from_2_5,
                                "%s.migration_gtk2_from_2_5", prefix);
  gui_options.gui_gtk3_migrated_from_2_5 =
    secfile_lookup_bool_default(sf, gui_options.gui_gtk2_migrated_from_2_5,
                                "%s.migration_gtk3_from_2_5", prefix);
  gui_options.gui_qt_migrated_from_2_5 =
    secfile_lookup_bool_default(sf, gui_options.gui_qt_migrated_from_2_5,
                                "%s.migration_qt_from_2_5", prefix);

  /* These are not gui-enabled yet */
  gui_options.zoom_set =
    secfile_lookup_bool_default(sf, FALSE, "%s.zoom_set", prefix);
  gui_options.zoom_default_level =
    secfile_lookup_float_default(sf, 1.0,
                                 "%s.zoom_default_level", prefix);

  str = secfile_lookup_str_default(sf, NULL, "client.default_tileset_name");
  if (str != NULL) {
    sz_strlcpy(gui_options.default_tileset_name, str);
  }
  str = secfile_lookup_str_default(sf, NULL, "client.default_tileset_overhead_name");
  if (str != NULL) {
    sz_strlcpy(gui_options.default_tileset_overhead_name, str);
  }
  str = secfile_lookup_str_default(sf, NULL, "client.default_tileset_iso_name");
  if (str != NULL) {
    sz_strlcpy(gui_options.default_tileset_iso_name, str);
  }

  /* Backwards compatibility for removed options replaced by entirely "new"
   * options. The equivalent "new" option will override these, if set. */

  /* Removed in 2.3 */
  /* Note: this overrides the previously specified default for
   * gui_gtk2_message_chat_location */
  /* gtk3 client never had the old form of this option. The overridden
   * gui_gtk2_ value will be propagated to gui_gtk3_ later by
   * migrate_options_from_gtk2() if necessary. */
  if (secfile_lookup_bool_default(sf, FALSE,
                                  "%s.gui_gtk2_merge_notebooks", prefix)) {
    gui_options.gui_gtk2_message_chat_location = GUI_GTK_MSGCHAT_MERGED;
  } else if (secfile_lookup_bool_default(sf, FALSE,
                                         "%s.gui_gtk2_split_bottom_notebook",
                                         prefix)) {
    gui_options.gui_gtk2_message_chat_location = GUI_GTK_MSGCHAT_SPLIT;
  } else {
    gui_options.gui_gtk2_message_chat_location = GUI_GTK_MSGCHAT_SEPARATE;
  }

  /* Renamed in 2.6 */
  gui_options.popup_actor_arrival = secfile_lookup_bool_default(sf, TRUE,
      "%s.popup_caravan_arrival", prefix);

  /* Load all the regular options */
  client_options_iterate_all(poption) {
    client_option_load(poption, sf);
  } client_options_iterate_all_end;

  /* More backwards compatibility, for removed options that had been
   * folded into then-existing options. Here, the backwards-compatibility
   * behaviour overrides the "destination" option. */

  /* Removed in 2.4 */
  if (!secfile_lookup_bool_default(sf, TRUE,
                                   "%s.do_combat_animation", prefix)) {
    gui_options.smooth_combat_step_msec = 0;
  }

  message_options_load(sf, prefix);
  options_dialogs_load(sf);

  /* Load cma presets. If cma.number_of_presets doesn't exist, don't load 
   * any, the order here should be reversed to keep the order the same */
  if (secfile_lookup_int(sf, &num, "cma.number_of_presets")) {
    for (i = num - 1; i >= 0; i--) {
      load_cma_preset(sf, i);
    }
  } else {
    create_default_cma_presets();
  }

  settable_options_load(sf);
  global_worklists_load(sf);

  secfile_destroy(sf);
  options_fully_initialized = TRUE;
}

/************************************************************************//**
  Write messages from option saving to the output window.
****************************************************************************/
static void option_save_output_window_callback(enum log_level lvl,
                                               const char *msg, ...)
{
  va_list args;

  va_start(args, msg);
  output_window_vprintf(ftc_client, msg, args);
  va_end(args);
}

/************************************************************************//**
  Save all options.
****************************************************************************/
void options_save(option_save_log_callback log_cb)
{
  struct section_file *sf;
  const char *name = get_current_option_file_name();
  char dir_name[2048];
  int i;

  if (log_cb == NULL) {
    /* Default callback */
    log_cb = option_save_output_window_callback;
  }

  if (!name) {
    log_cb(LOG_ERROR, _("Save failed, cannot find a filename."));
    return;
  }

  sf = secfile_new(TRUE);
  secfile_insert_str(sf, VERSION_STRING, "client.version");

  secfile_insert_bool(sf, gui_options.save_options_on_exit,
                      "client.save_options_on_exit");
  secfile_insert_bool_comment(sf, gui_options.migrate_fullscreen,
                              "deprecated", "client.fullscreen_mode");

  /* Migrations */
  secfile_insert_bool(sf, gui_options.gui_gtk3_migrated_from_gtk2,
                      "client.migration_gtk3_from_gtk2");
  secfile_insert_bool(sf, gui_options.gui_gtk3_22_migrated_from_gtk3,
                      "client.migration_gtk3_22_from_gtk3");
  secfile_insert_bool(sf, gui_options.gui_gtk4_migrated_from_gtk3_22,
                      "client.migration_gtk4_from_gtk3");
  secfile_insert_bool(sf, gui_options.gui_sdl2_migrated_from_sdl,
                      "client.migration_sdl2_from_sdl");
  secfile_insert_bool(sf, gui_options.gui_gtk2_migrated_from_2_5,
                      "client.migration_gtk2_from_2_5");
  secfile_insert_bool(sf, gui_options.gui_gtk3_migrated_from_2_5,
                      "client.migration_gtk3_from_2_5");
  secfile_insert_bool(sf, gui_options.gui_qt_migrated_from_2_5,
                      "client.migration_qt_from_2_5");

  /* gui-enabled options */
  client_options_iterate_all(poption) {
    if ((client_poption->specific != GUI_SDL || !gui_options.gui_sdl2_migrated_from_sdl)
        && (client_poption->specific != GUI_GTK2 || !gui_options.gui_gtk3_migrated_from_gtk2)) {
      /* Once sdl-client options have been migrated to sdl2-client, or gtk2-client options
       * to gtk3-client, there's no use for them any more, so no point in saving them. */
      client_option_save(poption, sf);
    }
  } client_options_iterate_all_end;

  /* These are not gui-enabled yet. */
  secfile_insert_bool(sf, gui_options.zoom_set, "client.zoom_set");
  secfile_insert_float(sf, gui_options.zoom_default_level,
                       "client.zoom_default_level");

  if (gui_options.default_tileset_name[0] != '\0') {
    secfile_insert_str(sf, gui_options.default_tileset_name,
                       "client.default_tileset_name");
  }

  message_options_save(sf, "client");
  options_dialogs_save(sf);

  /* server settings */
  save_cma_presets(sf);
  settable_options_save(sf);

  /* insert global worklists */
  global_worklists_save(sf);

  /* Directory name */
  sz_strlcpy(dir_name, name);
  for (i = strlen(dir_name) - 1 ; dir_name[i] != DIR_SEPARATOR_CHAR && i >= 0; i--) {
    /* Nothing */
  }
  if (i > 0) {
    dir_name[i] = '\0';
    make_dir(dir_name);
  }

  /* save to disk */
  if (!secfile_save(sf, name, 0, FZ_PLAIN)) {
    log_cb(LOG_ERROR, _("Save failed, cannot write to file %s"), name);
  } else {
    log_cb(LOG_VERBOSE, _("Saved settings to file %s"), name);
  }
  secfile_destroy(sf);
}

/************************************************************************//**
  Initialize lists of names for a client option.
****************************************************************************/
static void options_init_names(const struct copt_val_name *(*acc)(int),
                               struct strvec **support, struct strvec **pretty)
{
  int val;
  const struct copt_val_name *name;
  fc_assert_ret(NULL != acc);
  *support = strvec_new();
  *pretty = strvec_new();
  for (val=0; (name = acc(val)); val++) {
    strvec_append(*support, name->support);
    strvec_append(*pretty, name->pretty);
  }
}

/************************************************************************//**
  Initialize the option module.
****************************************************************************/
void options_init(void)
{
  message_options_init();
  options_extra_init();
  global_worklists_init();

  settable_options_hash = settable_options_hash_new();
  dialog_options_hash = dialog_options_hash_new();

  client_options_iterate_all(poption) {
    struct client_option *pcoption = CLIENT_OPTION(poption);

    switch (option_type(poption)) {
    case OT_INTEGER:
      if (option_int_def(poption) < option_int_min(poption)
          || option_int_def(poption) > option_int_max(poption)) {
        int new_default = MAX(MIN(option_int_def(poption),
                                  option_int_max(poption)),
                              option_int_min(poption));

        log_error("option %s has default value of %d, which is "
                  "out of its range [%d; %d], changing to %d.",
                  option_name(poption), option_int_def(poption),
                  option_int_min(poption), option_int_max(poption),
                  new_default);
        *((int *) &(pcoption->integer.def)) = new_default;
      }
      break;

    case OT_STRING:
      if (gui_options.default_user_name == option_str_get(poption)) {
        /* Hack to get a default value. */
        *((const char **) &(pcoption->string.def)) =
            fc_strdup(gui_options.default_user_name);
      }

      if (NULL == option_str_def(poption)) {
        const struct strvec *values = option_str_values(poption);

        if (NULL == values || strvec_size(values) == 0) {
          log_error("Invalid NULL default string for option %s.",
                    option_name(poption));
        } else {
          *((const char **) &(pcoption->string.def)) =
              strvec_get(values, 0);
        }
      }
      break;

    case OT_ENUM:
      fc_assert(NULL == pcoption->enumerator.support_names);
      fc_assert(NULL == pcoption->enumerator.pretty_names);
      options_init_names(pcoption->enumerator.name_accessor,
                         &pcoption->enumerator.support_names,
                         &pcoption->enumerator.pretty_names);
      fc_assert(NULL != pcoption->enumerator.support_names);
      fc_assert(NULL != pcoption->enumerator.pretty_names);
      break;

    case OT_BITWISE:
      fc_assert(NULL == pcoption->bitwise.support_names);
      fc_assert(NULL == pcoption->bitwise.pretty_names);
      options_init_names(pcoption->bitwise.name_accessor,
                         &pcoption->bitwise.support_names,
                         &pcoption->bitwise.pretty_names);
      fc_assert(NULL != pcoption->bitwise.support_names);
      fc_assert(NULL != pcoption->bitwise.pretty_names);
      break;

    case OT_COLOR:
      {
        /* Duplicate the string pointers. */
        struct ft_color *pcolor = pcoption->color.pvalue;

        if (NULL != pcolor->foreground) {
          pcolor->foreground = fc_strdup(pcolor->foreground);
        }
        if (NULL != pcolor->background) {
          pcolor->background = fc_strdup(pcolor->background);
        }
      }

    case OT_BOOLEAN:
    case OT_FONT:
    case OT_VIDEO_MODE:
      break;
    }

    /* Set to default. */
    option_reset(poption);
  } client_options_iterate_all_end;
}

/************************************************************************//**
  Free the option module.
****************************************************************************/
void options_free(void)
{
  client_options_iterate_all(poption) {
    struct client_option *pcoption = CLIENT_OPTION(poption);

    switch (option_type(poption)) {
    case OT_ENUM:
      fc_assert_action(NULL != pcoption->enumerator.support_names, break);
      strvec_destroy(pcoption->enumerator.support_names);
      pcoption->enumerator.support_names = NULL;
      fc_assert_action(NULL != pcoption->enumerator.pretty_names, break);
      strvec_destroy(pcoption->enumerator.pretty_names);
      pcoption->enumerator.pretty_names = NULL;
      break;

    case OT_BITWISE:
      fc_assert_action(NULL != pcoption->bitwise.support_names, break);
      strvec_destroy(pcoption->bitwise.support_names);
      pcoption->bitwise.support_names = NULL;
      fc_assert_action(NULL != pcoption->bitwise.pretty_names, break);
      strvec_destroy(pcoption->bitwise.pretty_names);
      pcoption->bitwise.pretty_names = NULL;
      break;

    case OT_BOOLEAN:
    case OT_INTEGER:
    case OT_STRING:
    case OT_FONT:
    case OT_COLOR:
    case OT_VIDEO_MODE:
      break;
    }
  } client_options_iterate_all_end;

  if (NULL != settable_options_hash) {
    settable_options_hash_destroy(settable_options_hash);
    settable_options_hash = NULL;
  }

  if (NULL != dialog_options_hash) {
    dialog_options_hash_destroy(dialog_options_hash);
    dialog_options_hash = NULL;
  }

  message_options_free();
  global_worklists_free();
}

/************************************************************************//**
  Callback when the reqtree show icons option is changed. The tree is
  recalculated.
****************************************************************************/
static void reqtree_show_icons_callback(struct option *poption)
{
  science_report_dialog_redraw();
}

/************************************************************************//**
  Callback for when any view option is changed.
****************************************************************************/
static void view_option_changed_callback(struct option *poption)
{
  menus_init();
  update_map_canvas_visible();
}

/************************************************************************//**
  Callback for when ai_manual_turn_done is changed.
****************************************************************************/
static void manual_turn_done_callback(struct option *poption)
{
  update_turn_done_button_state();
  if (!gui_options.ai_manual_turn_done && is_ai(client.conn.playing)) {
    if (can_end_turn()) {
      user_ended_turn();
    }
  }
}

/************************************************************************//**
  Callback for when any voteinfo bar option is changed.
****************************************************************************/
static void voteinfo_bar_callback(struct option *poption)
{
  voteinfo_gui_update();
}

/************************************************************************//**
  Callback for font options.
****************************************************************************/
static void font_changed_callback(struct option *poption)
{
  fc_assert_ret(OT_FONT == option_type(OPTION(poption)));
  gui_update_font(option_font_target(poption), option_font_get(poption));
}

/************************************************************************//**
  Callback for mapimg options.
****************************************************************************/
static void mapimg_changed_callback(struct option *poption)
{
  if (!mapimg_client_define()) {
    bool success;

    log_normal("Error setting the value for %s (%s). Restoring the default "
               "value.", option_name(poption), mapimg_error());

    /* Reset the value to the default value. */
    success = option_reset(poption);
    fc_assert_msg(success == TRUE,
                  "Failed to reset the option \"%s\".",
                  option_name(poption));
    success = mapimg_client_define();
    fc_assert_msg(success == TRUE,
                  "Failed to restore mapimg definition for option \"%s\".",
                  option_name(poption));
  }
}

/************************************************************************//**
  Callback for music enabling option.
****************************************************************************/
static void game_music_enable_callback(struct option *poption)
{
  if (client_state() == C_S_RUNNING) {
    if (gui_options.sound_enable_game_music) {
      start_style_music();
    } else {
      stop_style_music();
    }
  }
}

/************************************************************************//**
  Callback for music enabling option.
****************************************************************************/
static void menu_music_enable_callback(struct option *poption)
{
  if (client_state() != C_S_RUNNING) {
    if (gui_options.sound_enable_menu_music) {
      start_menu_music("music_menu", NULL);
    } else {
      stop_menu_music();
    }
  }
}

/************************************************************************//**
  Make dynamic adjustments to first-launch default options.
****************************************************************************/
static void client_option_adjust_defaults(void)
{
  adjust_default_options();
}

/************************************************************************//**
  Convert a video mode to string. Returns TRUE on success.
****************************************************************************/
bool video_mode_to_string(char *buf, size_t buf_len, struct video_mode *mode)
{
  return (2 < fc_snprintf(buf, buf_len, "%dx%d", mode->width, mode->height));
}

/************************************************************************//**
  Convert a string to video mode. Returns TRUE on success.
****************************************************************************/
bool string_to_video_mode(const char *buf, struct video_mode *mode)
{
  return (2 == sscanf(buf, "%dx%d", &mode->width, &mode->height));
}

/************************************************************************//**
  Option framework wrapper for mapimg_get_format_list()
****************************************************************************/
static const struct strvec *get_mapimg_format_list(const struct option *poption)
{
  return mapimg_get_format_list();
}

/************************************************************************//**
  What is the user defined tileset for the given topology
****************************************************************************/
const char *tileset_name_for_topology(int topology_id)
{
  const char *tsn = NULL;
  
  switch (topology_id & (TF_ISO | TF_HEX)) {
  case 0:
  case TF_ISO:
    tsn = gui_options.default_tileset_square_name;
    break;
  case TF_HEX:
    tsn = gui_options.default_tileset_hex_name;
    break;
  case TF_ISO | TF_HEX:
    tsn = gui_options.default_tileset_isohex_name;
    break;
  }

  if (tsn == NULL) {
    tsn = gui_options.default_tileset_name;
  }

  return tsn;
}

/************************************************************************//**
  Set given tileset as the default for suitable topology
****************************************************************************/
void option_set_default_ts(struct tileset *t)
{
  const char *optname = "<not set>";
  struct option *opt;

  switch (tileset_topo_index(t)) {
  case TS_TOPO_SQUARE:
    /* Overhead */
    optname = "default_tileset_square_name";
    break;
  case TS_TOPO_HEX:
    /* Hex */
    optname = "default_tileset_hex_name";
    break;
  case TS_TOPO_ISOHEX:
    /* Isohex */
    optname = "default_tileset_isohex_name";
    break;
  }

  opt = optset_option_by_name(client_optset, optname);

  if (opt == NULL) {
    log_error("Unknown option name \"%s\" in option_set_default_ts()", optname);
    return;
  }

  /* Do not call option_str_set() since we don't want option changed callback
   * to reload this tileset. */
  opt->str_vtable->set(opt, tileset_basename(t));
  option_gui_update(opt);
}

/************************************************************************//**
  Does topology-specific tileset option lack value?
****************************************************************************/
static bool is_ts_option_unset(const char *optname)
{
  struct option *opt;
  const char *val;

  opt = optset_option_by_name(client_optset, optname);

  if (opt == NULL) {
    return TRUE;
  }

  val = opt->str_vtable->get(opt);

  if (val == NULL || val[0] == '\0') {
    return TRUE;
  }

  return FALSE;
}

/************************************************************************//**
  Fill default tilesets for topology-specific settings.
****************************************************************************/
void fill_topo_ts_default(void)
{
  if (is_ts_option_unset("default_tileset_square_name")) {
    if (gui_options.default_tileset_iso_name[0] != '\0') {
      strncpy(gui_options.default_tileset_square_name,
              gui_options.default_tileset_iso_name,
              sizeof(gui_options.default_tileset_square_name));
    } else if (gui_options.default_tileset_overhead_name[0] != '\0') {
      strncpy(gui_options.default_tileset_square_name,
              gui_options.default_tileset_overhead_name,
              sizeof(gui_options.default_tileset_square_name));
    } else {
      log_debug("Setting tileset for square topologies.");
      tilespec_try_read(NULL, FALSE, 0, FALSE);
    }
  }
  if (is_ts_option_unset("default_tileset_hex_name")) {
    log_debug("Setting tileset for hex topology.");
    tilespec_try_read(NULL, FALSE, TF_HEX, FALSE);
  }
  if (is_ts_option_unset("default_tileset_isohex_name")) {
    log_debug("Setting tileset for isohex topology.");
    tilespec_try_read(NULL, FALSE, TF_ISO | TF_HEX, FALSE);
  }
}
