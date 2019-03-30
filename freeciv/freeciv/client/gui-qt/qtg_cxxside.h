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

#ifndef FC__QTG_CXXSIDE_H
#define FC__QTG_CXXSIDE_H

// common
#include "fc_types.h"

// client
#include "tilespec.h"

// gui-qt
#include "canvas.h"
#include "pages.h"


void setup_gui_funcs();

void qtg_ui_init();
void qtg_ui_main(int argc, char *argv[]);
void qtg_ui_exit();

enum gui_type qtg_get_gui_type();
void qtg_insert_client_build_info(char *outbuf, size_t outlen);
void qtg_adjust_default_options();

void qtg_version_message(const char *vertext);
void qtg_real_output_window_append(const char *astring,
                                   const struct text_tag_list *tags,
                                   int conn_id);

bool qtg_is_view_supported(enum ts_type type);
void qtg_tileset_type_set(enum ts_type type);
void qtg_free_intro_radar_sprites();
struct sprite *qtg_load_gfxfile(const char *filename);
struct sprite *qtg_create_sprite(int width, int height, struct color *pcolor);
void qtg_get_sprite_dimensions(struct sprite *sprite, int *width, int *height);
struct sprite *qtg_crop_sprite(struct sprite *source,
                               int x, int y, int width, int height,
                               struct sprite *mask,
                               int mask_offset_x, int mask_offset_y,
                               float scale, bool smooth);
void qtg_free_sprite(struct sprite *s);

struct color *qtg_color_alloc(int r, int g, int b);
void qtg_color_free(struct color *pcolor);

struct canvas *qtg_canvas_create(int width, int height);
void qtg_canvas_free(struct canvas *store);
void qtg_canvas_set_zoom(struct canvas *store, float zoom);
bool qtg_has_zoom_support();
void qtg_canvas_copy(struct canvas *dest, struct canvas *src,
		     int src_x, int src_y, int dest_x, int dest_y, int width,
		     int height);
void qtg_canvas_put_sprite(struct canvas *pcanvas,
                           int canvas_x, int canvas_y,
                           struct sprite *sprite,
                           int offset_x, int offset_y, int width, int height);
void qtg_canvas_put_sprite_full(struct canvas *pcanvas,
                                int canvas_x, int canvas_y,
                                struct sprite *sprite);
void qtg_canvas_put_sprite_fogged(struct canvas *pcanvas,
                                  int canvas_x, int canvas_y,
                                  struct sprite *psprite,
                                  bool fog, int fog_x, int fog_y);
void qtg_canvas_put_rectangle(struct canvas *pcanvas,
                              struct color *pcolor,
                              int canvas_x, int canvas_y,
                              int width, int height);
void qtg_canvas_fill_sprite_area(struct canvas *pcanvas,
                                 struct sprite *psprite, struct color *pcolor,
                                 int canvas_x, int canvas_y);
void qtg_canvas_put_line(struct canvas *pcanvas, struct color *pcolor,
                         enum line_type ltype, int start_x, int start_y,
                         int dx, int dy);
void qtg_canvas_put_curved_line(struct canvas *pcanvas, struct color *pcolor,
                                enum line_type ltype, int start_x, int start_y,
                                int dx, int dy);
void qtg_get_text_size(int *width, int *height,
                       enum client_font font, const char *text);
void qtg_canvas_put_text(struct canvas *pcanvas, int canvas_x, int canvas_y,
                         enum client_font font, struct color *pcolor,
                         const char *text);

void qtg_set_rulesets(int num_rulesets, char **rulesets);
void qtg_options_extra_init();
void qtg_server_connect();
void qtg_add_net_input(int sock);
void qtg_remove_net_input();
void qtg_real_conn_list_dialog_update(void *unused);
void qtg_close_connection_dialog();
void qtg_add_idle_callback(void (callback)(void *), void *data);
void qtg_sound_bell();

void qtg_real_set_client_page(enum client_pages page);
enum client_pages qtg_get_current_client_page();

void qtg_popup_combat_info(int attacker_unit_id, int defender_unit_id,
                           int attacker_hp, int defender_hp,
                           bool make_att_veteran, bool make_def_veteran);
void qtg_set_unit_icon(int idx, struct unit *punit);
void qtg_set_unit_icons_more_arrow(bool onoff);
void qtg_real_focus_units_changed(void);
void qtg_gui_update_font(const char *font_name, const char *font_value);

void qtg_editgui_refresh();
void qtg_editgui_notify_object_created(int tag, int id);
void qtg_editgui_notify_object_changed(int objtype, int object_id, bool removal);
void qtg_editgui_popup_properties(const struct tile_list *tiles, int objtype);
void qtg_editgui_tileset_changed();
void qtg_editgui_popdown_all();

void qtg_update_timeout_label();
void qtg_real_city_dialog_popup(struct city *pcity);
void qtg_real_city_dialog_refresh(struct city *pcity);
void qtg_popdown_city_dialog(struct city *pcity);
void qtg_popdown_all_city_dialogs();
bool qtg_handmade_scenario_warning();
void qtg_refresh_unit_city_dialogs(struct unit *punit);
bool qtg_city_dialog_is_open(struct city *pcity);

bool qtg_request_transport(struct unit *pcargo, struct tile *ptile);

void qtg_gui_load_theme(const char *directory, const char *theme_name);
void qtg_gui_clear_theme();
char **qtg_get_gui_specific_themes_directories(int *count);
char **qtg_get_useable_themes_in_directory(const char *directory, int *count);

#endif /* FC__QTG_CXXSIDE_H */
