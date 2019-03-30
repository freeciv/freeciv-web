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

#ifndef FC__QTG_CSIDE_H
#define FC__QTG_CSIDE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "fc_types.h"
#include "featured_text.h"
#include "tile.h"

/* client/include */
#include "canvas_g.h"
#include "pages_g.h"

/* client */
#include "tilespec.h"

struct gui_funcs {
  void (*ui_init)(void);
  void (*ui_main)(int argc, char *argv[]);
  void (*ui_exit)(void);

  enum gui_type (*get_gui_type)(void);
  void (*insert_client_build_info)(char *outbuf, size_t outlen);
  void (*adjust_default_options)(void);

  void (*version_message)(const char *vertext);
  void (*real_output_window_append)(const char *astring,
                                    const struct text_tag_list *tags,
                                    int conn_id);

  bool (*is_view_supported)(enum ts_type type);
  void (*tileset_type_set)(enum ts_type type);
  void (*free_intro_radar_sprites)(void);
  struct sprite * (*load_gfxfile)(const char *filename);
  struct sprite * (*create_sprite)(int width, int height, struct color *pcolor);
  void (*get_sprite_dimensions)(struct sprite *sprite, int *width, int *height);
  struct sprite * (*crop_sprite)(struct sprite *source,
                                 int x, int y, int width, int height,
                                 struct sprite *mask,
                                 int mask_offset_x, int mask_offset_y,
                                 float scale, bool smooth);
  void (*free_sprite)(struct sprite *s);

  struct color *(*color_alloc)(int r, int g, int b);
  void (*color_free)(struct color *pcolor);

  struct canvas *(*canvas_create)(int width, int height);
  void (*canvas_free)(struct canvas *store);
  void (*canvas_set_zoom)(struct canvas *store, float zoom);
  bool (*has_zoom_support)(void);
  void (*canvas_copy)(struct canvas *dest, struct canvas *src,
                      int src_x, int src_y, int dest_x, int dest_y, int width,
                      int height);
  void (*canvas_put_sprite)(struct canvas *pcanvas,
                            int canvas_x, int canvas_y,
                            struct sprite *psprite,
                            int offset_x, int offset_y, int width, int height);
  void (*canvas_put_sprite_full)(struct canvas *pcanvas,
                                 int canvas_x, int canvas_y,
                                 struct sprite *psprite);
  void (*canvas_put_sprite_fogged)(struct canvas *pcanvas,
                                   int canvas_x, int canvas_y,
                                   struct sprite *psprite,
                                   bool fog, int fog_x, int fog_y);
  void (*canvas_put_rectangle)(struct canvas *pcanvas,
                               struct color *pcolor,
                               int canvas_x, int canvas_y,
                               int width, int height);
  void (*canvas_fill_sprite_area)(struct canvas *pcanvas,
                                  struct sprite *psprite, struct color *pcolor,
                                  int canvas_x, int canvas_y);
  void (*canvas_put_line)(struct canvas *pcanvas, struct color *pcolor,
                          enum line_type ltype, int start_x, int start_y,
                          int dx, int dy);
  void (*canvas_put_curved_line)(struct canvas *pcanvas, struct color *pcolor,
                                 enum line_type ltype, int start_x, int start_y,
                                 int dx, int dy);
  void (*get_text_size)(int *width, int *height,
                        enum client_font font, const char *text);
  void (*canvas_put_text)(struct canvas *pcanvas, int canvas_x, int canvas_y,
                          enum client_font font, struct color *pcolor,
                          const char *text);

  void (*set_rulesets)(int num_rulesets, char **rulesets);
  void (*options_extra_init)(void);
  void (*server_connect)(void);
  void (*add_net_input)(int sock);
  void (*remove_net_input)(void);
  void (*real_conn_list_dialog_update)(void *unused);
  void (*close_connection_dialog)(void);
  void (*add_idle_callback)(void (callback)(void *), void *data);
  void (*sound_bell)(void);

  void (*real_set_client_page)(enum client_pages page);
  enum client_pages (*get_current_client_page)(void);

  void (*set_unit_icon)(int idx, struct unit *punit);
  void (*set_unit_icons_more_arrow)(bool onoff);
  void (*real_focus_units_changed)(void);
  void (*gui_update_font)(const char *font_name, const char *font_value);

  void (*editgui_refresh)(void);
  void (*editgui_notify_object_created)(int tag, int id);
  void (*editgui_notify_object_changed)(int objtype, int object_id, bool removal);
  void (*editgui_popup_properties)(const struct tile_list *tiles, int objtype);
  void (*editgui_tileset_changed)(void);
  void (*editgui_popdown_all)(void);

  void (*popup_combat_info)(int attacker_unit_id, int defender_unit_id,
                            int attacker_hp, int defender_hp,
                            bool make_att_veteran, bool make_def_veteran);
  void (*update_timeout_label)(void);
  void (*real_city_dialog_popup)(struct city *pcity);
  void (*real_city_dialog_refresh)(struct city *pcity);
  void (*popdown_city_dialog)(struct city *pcity);
  void (*popdown_all_city_dialogs)(void);
  bool (*handmade_scenario_warning)(void);
  void (*refresh_unit_city_dialogs)(struct unit *punit);
  bool (*city_dialog_is_open)(struct city *pcity);

  bool (*request_transport)(struct unit *pcargo, struct tile *ptile);

  void (*gui_load_theme)(const char *directory, const char *theme_name);
  void (*gui_clear_theme)(void);
  char **(*get_gui_specific_themes_directories)(int *count);
  char **(*get_useable_themes_in_directory)(const char *directory, int *count);
};

struct gui_funcs *get_gui_funcs(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__QTG_CSIDE_H */
