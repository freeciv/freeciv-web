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

#include <assert.h>

#include "climisc.h"		/* for write_chatline_content */
#include "colors.h"
#include "widget.h"
#include "gui_main.h"
#include "chat.h"
#include "theme_engine.h"
#include "registry.h"

#include "gui_main.h"
#include "colors_g.h"

#include "chatline.h"

static struct {
  struct ct_string *template;
  struct ct_rect bounds;
  enum ws_alignment alignment;

  struct sprite *up, *down, *left, *right;
  struct sprite *vert_top, *vert_bottom, *vert_repeat, *vert_center;
  struct sprite *hori_top, *hori_bottom, *hori_repeat, *hori_center;
} config;

static struct {
  struct sw_widget *window;
  struct sw_widget *chatline_vslider;
  struct sw_widget *chatline_hslider;
  struct sw_widget *chatline_list;
} widgets;

/**************************************************************************
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
**************************************************************************/
void real_output_window_append(const char *astring,
                               const struct text_tag_list *tags,
                               int conn_id)
{
  struct ct_string *string;
  struct sw_widget *label;
  static int index = 0;

  string = ct_string_clone3(config.template, astring);
  label = sw_label_create_text(root_window, string);
  sw_widget_set_background_color(label,
				 enum_color_to_be_color(COLOR_STD_WHITE));
  sw_list_set_item(widgets.chatline_list, 0, index, label);
  index++;
  /* scroll down */
  sw_slider_set_offset(widgets.chatline_vslider, 1.0);
  chat_add(astring, conn_id);
}

/**************************************************************************
  Get the text of the output window, and call write_chatline_content() to
  log it.
**************************************************************************/
void log_output_window(void)
{
  /* PORTME */
  write_chatline_content(NULL);
}

/**************************************************************************
  Clear all text from the output window.
**************************************************************************/
void clear_output_window(void)
{
  /* PORTME */
#if 0
  set_output_window_text(_("Cleared output window."));
#endif
}

/**************************************************************************
  ...
**************************************************************************/
static void list_changed(struct sw_widget *list, void *data)
{
  sw_update_vslider_from_list(widgets.chatline_vslider,
			      widgets.chatline_list);
    sw_update_hslider_from_list(widgets.chatline_hslider,
			      widgets.chatline_list);
}

/**************************************************************************
  ...
**************************************************************************/
static void vscroll_buttons_callback(struct sw_widget *button, void *data)
{
  int dx = (int) data;
  float old = sw_slider_get_offset(widgets.chatline_vslider);
  float width = sw_slider_get_width(widgets.chatline_vslider);

  sw_slider_set_offset(widgets.chatline_vslider, old + dx * 0.9 * width);
}

/**************************************************************************
  ...
**************************************************************************/
static void hscroll_buttons_callback(struct sw_widget *button, void *data)
{
  int dx = (int) data;
  float old = sw_slider_get_offset(widgets.chatline_hslider);
  float width = sw_slider_get_width(widgets.chatline_hslider);

  sw_slider_set_offset(widgets.chatline_hslider, old + dx * 0.9 * width);
}

/**************************************************************************
  ...
**************************************************************************/
static void vslider_changed(struct sw_widget *widget, void *data)
{
  sw_update_list_from_vslider(widgets.chatline_list,
			      widgets.chatline_vslider);
}

static void hslider_changed(struct sw_widget *widget, void *data)
{
  sw_update_list_from_hslider(widgets.chatline_list, widgets.chatline_hslider);
}

/**************************************************************************
  ...
**************************************************************************/
static void read_config(void)
{
  struct section_file *file = te_open_themed_file("chatline.prop");

  te_read_bounds_alignment(file, "xyz", &config.bounds, &config.alignment);
  config.template = te_read_string(file, "xyz", "text", TRUE, FALSE);

  config.up = te_load_gfx(secfile_lookup_str(file, "xyz.up"));
  config.down = te_load_gfx(secfile_lookup_str(file, "xyz.down"));
  config.left = te_load_gfx(secfile_lookup_str(file, "xyz.left"));
  config.right = te_load_gfx(secfile_lookup_str(file, "xyz.right"));

  config.vert_top = te_load_gfx(secfile_lookup_str(file, "xyz.vert-top"));
  config.vert_bottom =
      te_load_gfx(secfile_lookup_str(file, "xyz.vert-bottom"));
  config.vert_repeat =
      te_load_gfx(secfile_lookup_str(file, "xyz.vert-repeat"));
  if (secfile_lookup_str_default(file, NULL, "xyz.vert-center")) {
    config.vert_center =
	te_load_gfx(secfile_lookup_str(file, "xyz.vert-center"));
  } else {
    config.vert_center = NULL;
  }

  config.hori_top = te_load_gfx(secfile_lookup_str(file, "xyz.hori-top"));
  config.hori_bottom =
      te_load_gfx(secfile_lookup_str(file, "xyz.hori-bottom"));
  config.hori_repeat =
      te_load_gfx(secfile_lookup_str(file, "xyz.hori-repeat"));
  if (secfile_lookup_str_default(file, NULL, "xyz.hori-center")) {
    config.hori_center =
	te_load_gfx(secfile_lookup_str(file, "xyz.hori-center"));
  } else {
    config.hori_center = NULL;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void construct_widgets(void)
{
  struct sw_widget *button;
  struct ct_rect rect_up, rect_down, rect_left, rect_right;
  struct ct_size vert_slider;
  struct ct_size hori_slider;

  widgets.window =
      sw_window_create(root_window,
		       config.bounds.width, config.bounds.height, NULL,
		       FALSE, DEPTH_MAX);
  sw_widget_set_background_color(widgets.window, enum_color_to_be_color
				 (COLOR_STD_RED));
  sw_widget_set_position(widgets.window, config.bounds.x, config.bounds.y);

  button =
      sw_button_create_text_and_background(widgets.window, NULL, config.up);
  sw_widget_align_parent(button, A_NE);
  sw_widget_get_bounds(button, &rect_up);
  sw_button_set_callback(button, vscroll_buttons_callback, (void *) -1);

  button =
      sw_button_create_text_and_background(widgets.window, NULL,
					   config.down);
  sw_widget_align_parent(button, A_SE);
  sw_widget_get_bounds(button, &rect_down);
  sw_button_set_callback(button, vscroll_buttons_callback, (void *) +1);

  button =
      sw_button_create_text_and_background(widgets.window, NULL,
					   config.left);
  sw_widget_align_parent(button, A_SW);
  sw_widget_get_bounds(button, &rect_left);
  sw_button_set_callback(button, hscroll_buttons_callback, (void *) -1);

  button =
      sw_button_create_text_and_background(widgets.window, NULL,
					   config.right);
  sw_widget_set_position(button, 0, 0);
  sw_widget_get_bounds(button, &rect_right);
  sw_widget_set_position(button, rect_down.x - rect_right.width - 2, 0);
  sw_widget_align_parent(button, A_S);
  sw_widget_get_bounds(button, &rect_right);
  sw_button_set_callback(button, hscroll_buttons_callback, (void *) +1);

  be_sprite_get_size(&vert_slider, config.vert_top);

  widgets.chatline_vslider =
      sw_slider_create(widgets.window, vert_slider.width,
		       rect_down.y - (rect_up.y + rect_up.height),
		       config.vert_top, config.vert_bottom,
		       config.vert_repeat, config.vert_center, TRUE);
  sw_widget_set_position(widgets.chatline_vslider, rect_up.x + 2,
			 rect_up.height);
  sw_widget_set_background_color(widgets.chatline_vslider,
				 enum_color_to_be_color(COLOR_STD_WHITE));
  sw_slider_set_slided_notify(widgets.chatline_vslider, vslider_changed,
			      NULL);

  be_sprite_get_size(&hori_slider, config.hori_top);

  widgets.chatline_hslider =
      sw_slider_create(widgets.window,
		       rect_right.x - (rect_left.x + rect_left.width),
		       hori_slider.height/4,
		       config.hori_top, config.hori_bottom,
		       config.hori_repeat, config.hori_center, FALSE);
  sw_widget_set_position(widgets.chatline_hslider, rect_left.width,
			 rect_left.y + 1);
  sw_widget_set_background_color(widgets.chatline_hslider,
				 enum_color_to_be_color(COLOR_STD_WHITE));
  sw_slider_set_slided_notify(widgets.chatline_hslider, hslider_changed, NULL);

  widgets.chatline_list =
      sw_list_create(widgets.window, rect_up.x - rect_left.x - 2,
		     rect_down.y - rect_up.y - 2);
  sw_widget_set_background_color(widgets.chatline_list,
				 enum_color_to_be_color(COLOR_STD_WHITE));
  sw_widget_set_position(widgets.chatline_list, 0, 0);
  sw_list_set_content_changed_notify(widgets.chatline_list, list_changed,
				     NULL);
}

/**************************************************************************
  ...
**************************************************************************/
void chatline_create(void)
{
  read_config();
  construct_widgets();
}
