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

/* Any client that contains this file is using callback interface */
#define GUI_CB_MODE 1

/* client */
#include "gui_interface.h"

/* client/include */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "connectdlg_g.h"
#include "dialogs_g.h"
#include "editgui_g.h"
#include "graphics_g.h"
#include "gui_main_g.h"
#include "mapview_g.h"
#include "sprite_g.h"
#include "themes_g.h"

#include "gui_cbsetter.h"

/**********************************************************************//**
  Setup the gui callback table.
**************************************************************************/
void setup_gui_funcs()
{
  struct gui_funcs *funcs = get_gui_funcs();

  funcs->ui_init = gui_ui_init;
  funcs->ui_main = gui_ui_main;
  funcs->ui_exit = gui_ui_exit;

  funcs->get_gui_type = gui_get_gui_type;
  funcs->insert_client_build_info = gui_insert_client_build_info;
  funcs->adjust_default_options = gui_adjust_default_options;

  funcs->version_message = gui_version_message;
  funcs->real_output_window_append = gui_real_output_window_append;

  funcs->is_view_supported = gui_is_view_supported;
  funcs->free_intro_radar_sprites = gui_free_intro_radar_sprites;
  funcs->load_gfxfile = gui_load_gfxfile;
  funcs->create_sprite = gui_create_sprite;
  funcs->get_sprite_dimensions = gui_get_sprite_dimensions;
  funcs->crop_sprite = gui_crop_sprite;
  funcs->free_sprite = gui_free_sprite;

  funcs->color_alloc = gui_color_alloc;
  funcs->color_free = gui_color_free;

  funcs->canvas_create = gui_canvas_create;
  funcs->canvas_free = gui_canvas_free;
  funcs->canvas_set_zoom = gui_canvas_set_zoom;
  funcs->has_zoom_support = gui_has_zoom_support;
  funcs->canvas_copy = gui_canvas_copy;
  funcs->canvas_put_sprite = gui_canvas_put_sprite;
  funcs->canvas_put_sprite_full = gui_canvas_put_sprite_full;
  funcs->canvas_put_sprite_fogged = gui_canvas_put_sprite_fogged;
  funcs->canvas_put_rectangle = gui_canvas_put_rectangle;
  funcs->canvas_fill_sprite_area = gui_canvas_fill_sprite_area;
  funcs->canvas_put_line = gui_canvas_put_line;
  funcs->canvas_put_curved_line = gui_canvas_put_curved_line;
  funcs->get_text_size = gui_get_text_size;
  funcs->canvas_put_text = gui_canvas_put_text;

  funcs->set_rulesets = gui_set_rulesets;
  funcs->options_extra_init = gui_options_extra_init;
  funcs->server_connect = gui_server_connect;
  funcs->add_net_input = gui_add_net_input;
  funcs->remove_net_input = gui_remove_net_input;
  funcs->real_conn_list_dialog_update = gui_real_conn_list_dialog_update;
  funcs->close_connection_dialog = gui_close_connection_dialog;
  funcs->add_idle_callback = gui_add_idle_callback;
  funcs->sound_bell = gui_sound_bell;

  funcs->real_set_client_page = gui_real_set_client_page;
  funcs->get_current_client_page = gui_get_current_client_page;

  funcs->set_unit_icon = gui_set_unit_icon;
  funcs->set_unit_icons_more_arrow = gui_set_unit_icons_more_arrow;
  funcs->real_focus_units_changed = gui_real_focus_units_changed;
  funcs->gui_update_font = gui_gui_update_font;

  funcs->editgui_refresh = gui_editgui_refresh;
  funcs->editgui_notify_object_created = gui_editgui_notify_object_created;
  funcs->editgui_notify_object_changed = gui_editgui_notify_object_changed;
  funcs->editgui_popup_properties = gui_editgui_popup_properties;
  funcs->editgui_tileset_changed = gui_editgui_tileset_changed;
  funcs->editgui_popdown_all = gui_editgui_popdown_all;

  funcs->popup_combat_info = gui_popup_combat_info;
  funcs->update_timeout_label = gui_update_timeout_label;
  funcs->real_city_dialog_popup = gui_real_city_dialog_popup;
  funcs->real_city_dialog_refresh = gui_real_city_dialog_refresh;
  funcs->popdown_city_dialog = gui_popdown_city_dialog;
  funcs->popdown_all_city_dialogs = gui_popdown_all_city_dialogs;
  funcs->handmade_scenario_warning = gui_handmade_scenario_warning;
  funcs->refresh_unit_city_dialogs = gui_refresh_unit_city_dialogs;
  funcs->city_dialog_is_open = gui_city_dialog_is_open;

  funcs->request_transport = gui_request_transport;

  funcs->gui_load_theme = gui_gui_load_theme;
  funcs->gui_clear_theme = gui_gui_clear_theme;
  funcs->get_gui_specific_themes_directories = gui_get_gui_specific_themes_directories;
  funcs->get_useable_themes_in_directory = gui_get_useable_themes_in_directory;
}
