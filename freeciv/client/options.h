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
#ifndef FC__OPTIONS_H
#define FC__OPTIONS_H

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "events.h"
#include "fc_types.h"           /* enum gui_type */

extern char default_user_name[512];
extern char default_server_host[512];
extern int default_server_port; 
extern char default_metaserver[512];
extern char default_theme_name[512];
extern char default_tileset_name[512];
extern char default_sound_set_name[512];
extern char default_sound_plugin_name[512];

extern bool save_options_on_exit;
extern bool fullscreen_mode;

/** Local Options: **/

extern bool solid_color_behind_units;
extern bool sound_bell_at_new_turn;
extern int smooth_move_unit_msec;
extern int smooth_center_slide_msec;
extern bool do_combat_animation;
extern bool ai_manual_turn_done;
extern bool auto_center_on_unit;
extern bool auto_center_on_combat;
extern bool auto_center_each_turn;
extern bool wakeup_focus;
extern bool goto_into_unknown;
extern bool center_when_popup_city;
extern bool concise_city_production;
extern bool auto_turn_done;
extern bool meta_accelerators;
extern bool ask_city_name;
extern bool popup_new_cities;
extern bool popup_caravan_arrival;
extern bool update_city_text_in_refresh_tile;
extern bool keyboardless_goto;
extern bool enable_cursor_changes;
extern bool separate_unit_selection;
extern bool unit_selection_clears_orders;
extern char highlight_our_names[128];

extern bool draw_city_outlines;
extern bool draw_city_output;
extern bool draw_map_grid;
extern bool draw_city_names;
extern bool draw_city_growth;
extern bool draw_city_productions;
extern bool draw_city_buycost;
extern bool draw_city_traderoutes;
extern bool draw_terrain;
extern bool draw_coastline;
extern bool draw_roads_rails;
extern bool draw_irrigation;
extern bool draw_mines;
extern bool draw_fortress_airbase;
extern bool draw_specials;
extern bool draw_pollution;
extern bool draw_cities;
extern bool draw_units;
extern bool draw_focus_unit;
extern bool draw_fog_of_war;
extern bool draw_borders;
extern bool draw_full_citybar;
extern bool draw_unit_shields;

extern bool player_dlg_show_dead_players;
extern bool reqtree_show_icons;
extern bool reqtree_curved_lines;

/* gui-gtk-2.0 client specific options. */
extern bool gui_gtk2_map_scrollbars;
extern bool gui_gtk2_dialogs_on_top;
extern bool gui_gtk2_show_task_icons;
extern bool gui_gtk2_enable_tabs;
extern bool gui_gtk2_better_fog;
extern bool gui_gtk2_show_chat_message_time;
extern bool gui_gtk2_split_bottom_notebook;
extern bool gui_gtk2_new_messages_go_to_top;
extern bool gui_gtk2_show_message_window_buttons;
extern bool gui_gtk2_metaserver_tab_first;
extern bool gui_gtk2_allied_chat_only;
extern bool gui_gtk2_small_display_layout;
extern char gui_gtk2_font_city_label[512];
extern char gui_gtk2_font_notify_label[512];
extern char gui_gtk2_font_spaceship_label[512];
extern char gui_gtk2_font_help_label[512];
extern char gui_gtk2_font_help_link[512];
extern char gui_gtk2_font_help_text[512];
extern char gui_gtk2_font_chatline[512];
extern char gui_gtk2_font_beta_label[512];
extern char gui_gtk2_font_small[512];
extern char gui_gtk2_font_comment_label[512];
extern char gui_gtk2_font_city_names[512];
extern char gui_gtk2_font_city_productions[512];

/* gui-sdl client specific options. */
extern bool gui_sdl_fullscreen;
extern int gui_sdl_screen_width;
extern int gui_sdl_screen_height;

/* gui-win32 client specific options. */
extern bool gui_win32_better_fog;
extern bool gui_win32_enable_alpha;

enum client_option_type {
  COT_BOOLEAN,
  COT_INTEGER,
  COT_STRING,
  COT_FONT
};

enum client_option_class {
  COC_GRAPHICS,
  COC_OVERVIEW,
  COC_SOUND,
  COC_INTERFACE,
  COC_NETWORK,
  COC_FONT,
  COC_MAX
};

extern const char *client_option_class_names[];

struct client_option {
  const char *name;             /* Short name - used as an identifier */
  const char *description;      /* One-line description */
  const char *help_text;        /* Paragraph-length help text */
  enum client_option_class category;
  enum gui_type specific;       /* GUI_LAST for common options. */
  enum client_option_type type;
  union {
    /* COT_BOOLEAN type option. */
    struct {
      bool *const pvalue;
      const bool def;
    } boolean;
    /* COT_INTEGER type option. */
    struct {
      int *const pvalue;
      const int def, min, max;
    } integer;
    /* COT_STRING or COT_FONT types option. */
    struct {
      char *const pvalue;
      const size_t size;
      const char *def;
      /* 
       * A function to return a static NULL-terminated list of possible
       * string values, or NULL for none. 
       */
      const char **(*const val_accessor)(void);
    } string;
  };
  void (*change_callback)(struct client_option *option);

  /* volatile */
  void *gui_data;
};

/*
 * Generate a client option of type COT_BOOLEAN.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for.  Sets to GUI_LAST for common options.
 * odef:  The default value of this client option (FALSE or TRUE).
 * ocb:   A callback function of type void (*)(struct client_option *)
 *        called when the option changed.
 */
#define GEN_BOOL_OPTION(oname, odesc, ohelp, ocat, ospec, odef, ocb)        \
{                                                                           \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  .type = COT_BOOLEAN,                                                      \
  {                                                                         \
    .boolean = {                                                            \
      .pvalue = &oname,                                                     \
      .def = odef,                                                          \
    }                                                                       \
  },                                                                        \
  .change_callback = ocb,                                                   \
}

/*
 * Generate a client option of type COT_INTEGER.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for.  Sets to GUI_LAST for common options.
 * odef:  The default value of this client option.
 * omin:  The minimal value of this client option.
 * omax:  The maximal value of this client option.
 * ocb:   A callback function of type void (*)(struct client_option *)
 *        called when the option changed.
 */
#define GEN_INT_OPTION(oname, odesc, ohelp, ocat, ospec, odef, omin, omax, ocb) \
{                                                                           \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  .type = COT_INTEGER,                                                      \
  {                                                                         \
    .integer = {                                                            \
      .pvalue = &oname,                                                     \
      .def = odef,                                                          \
      .min = omin,                                                          \
      .max = omax                                                           \
    }                                                                       \
  },                                                                        \
  .change_callback = ocb,                                                   \
}

/*
 * Generate a client option of type COT_STRING.
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
 *        gui this option is for.  Sets to GUI_LAST for common options.
 * odef:  The default string for this client option.
 * ocb:   A callback function of type void (*)(struct client_option *)
 *        called when the option changed.
 */
#define GEN_STR_OPTION(oname, odesc, ohelp, ocat, ospec, odef, ocb)         \
{                                                                           \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  .type = COT_STRING,                                                       \
  {                                                                         \
    .string = {                                                             \
      .pvalue = oname,                                                      \
      .size = sizeof(oname),                                                \
      .def = odef,                                                          \
      .val_accessor = NULL                                                  \
    }                                                                       \
  },                                                                        \
  .change_callback = ocb,                                                   \
}

/*
 * Generate a client option of type COT_STRING with a string accessor
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
 *        gui this option is for.  Sets to GUI_LAST for common options.
 * odef:  The default string for this client option.
 * oacc:  The string accessor where to find the allowed values of type
 *        const char **(*)(void) (returns a NULL-termined list of strings).
 * ocb:   A callback function of type void (*)(struct client_option *)
 *        called when the option changed.
 */
#define GEN_STR_LIST_OPTION(oname, odesc, ohelp, ocat, ospec, odef, oacc, ocb) \
{                                                                           \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  .type = COT_STRING,                                                       \
  {                                                                         \
    .string = {                                                             \
      .pvalue = oname,                                                      \
      .size = sizeof(oname),                                                \
      .def = odef,                                                          \
      .val_accessor = oacc                                                  \
    }                                                                       \
  },                                                                        \
  .change_callback = ocb,                                                   \
}

/*
 * Generate a client option of type COT_FONT.
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
 *        gui this option is for.  Sets to GUI_LAST for common options.
 * odef:  The default string for this client option.
 * ocb:   A callback function of type void (*)(struct client_option *)
 *        called when the option changed.
 */
#define GEN_FONT_OPTION(oname, odesc, ohelp, ocat, ospec, odef, ocb)        \
{                                                                           \
  .name = #oname,                                                           \
  .description = odesc,                                                     \
  .help_text = ohelp,                                                       \
  .category = ocat,                                                         \
  .specific = ospec,                                                        \
  .type = COT_FONT,                                                         \
  {                                                                         \
    .string = {                                                             \
      .pvalue = oname,                                                      \
      .size = sizeof(oname),                                                \
      .def = odef,                                                          \
      .val_accessor = NULL                                                  \
    }                                                                       \
  },                                                                        \
  .change_callback = ocb,                                                   \
}

/* Initialization and iteration */
struct client_option *client_option_array_first(void);
const struct client_option *client_option_array_last(void);

#define client_options_iterate(_p)                                          \
{                                                                           \
  const enum gui_type our_gui = get_gui_type();                             \
  struct client_option *_p = client_option_array_first();                   \
  for (; _p <= client_option_array_last(); _p++) {                          \
    if (_p->specific == GUI_LAST || _p->specific == our_gui) {

#define client_options_iterate_end                                          \
    }                                                                       \
  }                                                                         \
}

/** Message Options: **/

/* for specifying which event messages go where: */
#define NUM_MW 3
#define MW_OUTPUT    1		/* add to the output window */
#define MW_MESSAGES  2		/* add to the messages window */
#define MW_POPUP     4		/* popup an individual window */

extern unsigned int messages_where[];	/* OR-ed MW_ values [E_LAST] */

void message_options_init(void);
void message_options_free(void);

void load_general_options(void);
void load_ruleset_specific_options(void);
void load_settable_options(bool send_it);
void save_options(void);

/* Callback functions for changing options. */
void mapview_redraw_callback(struct client_option *option);

#endif  /* FC__OPTIONS_H */
