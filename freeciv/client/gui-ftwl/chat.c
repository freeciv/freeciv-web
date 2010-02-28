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
#include <stdio.h>

#include "connection.h"
#include "climisc.h"      /* for write_chatline_content */
#include "colors.h"
#include "log.h"
#include "widget.h"
#include "gui_main.h"
#include "mem.h"
#include "theme_engine.h"
#include "registry.h"
#include "chatline_common.h"
#include "tilespec.h"
#include "player.h"

#include "chat.h"

#define INPUT_DEPTH	DEPTH_MAX-3
#define OUTPUT_DEPTH	DEPTH_MAX-4

static struct {
  struct sw_widget *window, *edit;
  struct ct_rect bounds;
  struct ct_string *template;
  enum ws_alignment alignment;
} input;

static struct {
  int items;
  struct {
    struct sw_widget *widget;
    int timeout;
    struct ct_rect bounds;
  } *item;
  struct sw_widget *window;

  be_color background;
  struct ct_rect outer_bounds, inner_bounds;
  struct ct_string *template;
  int padding;
  int delay;
  bool has_background;
} output;

/*************************************************************************
 ...
*************************************************************************/
void chat_create(void)
{
  struct section_file *file = te_open_themed_file("oog.prop");

  /* input section */
  input.window = NULL;

  te_read_bounds_alignment(file, "input", &input.bounds, &input.alignment);
  input.template = te_read_string(file, "input", "text", TRUE, FALSE);

  /* output section */
  output.items = 0;
  output.item = NULL;
  output.window = NULL;

  te_read_bounds_alignment(file, "output", &output.outer_bounds, NULL);
  output.template = te_read_string(file, "output", "text", FALSE, FALSE);
  output.padding = secfile_lookup_int(file, "output.padding");
  output.delay = secfile_lookup_int(file, "output.delay");
  output.has_background =
      secfile_lookup_str_default(file, NULL, "output.background");
  if (output.has_background) {
    output.background = te_read_color(file, "output", "", "background");
  }
  output.inner_bounds = output.outer_bounds;
  output.inner_bounds.x += output.padding;
  output.inner_bounds.y += output.padding;
  output.inner_bounds.width -= 2 * output.padding;
  output.inner_bounds.height -= 2 * output.padding;
}

/*************************************************************************
 ...
*************************************************************************/
static void create_window(int height)
{
  assert(output.window == NULL);
  output.window =
      sw_window_create(root_window, output.outer_bounds.width, height, NULL,
		       FALSE, OUTPUT_DEPTH);
  if (output.has_background) {
    sw_widget_set_background_color(output.window, output.background);
  }
  sw_widget_set_position(output.window, output.outer_bounds.x,
			 output.outer_bounds.y);
  sw_window_set_draggable(output.window, FALSE);
}

/*************************************************************************
 ...
*************************************************************************/
static void remove_window(void)
{
  sw_widget_destroy(output.window);
  output.window = NULL;
}

/*************************************************************************
 ...
*************************************************************************/
static int get_remaining_height(void)
{
  int i, height = 0;

  for (i = 0; i < output.items; i++) {
    height += output.item[i].bounds.height;
  }
  return output.inner_bounds.height - height;
}

/*************************************************************************
 ...
*************************************************************************/
static void remove_all(void)
{
  int i;
  for (i = 0; i < output.items; i++) {
    sw_window_remove(output.item[i].widget);
  }
}

/*************************************************************************
 ...
*************************************************************************/
static void update(void)
{
  int i, height = 2 * output.padding;

  for (i = 0; i < output.items; i++) {
    height += output.item[i].bounds.height;
  }

  if (output.items == 0) {
    if (output.window) {
      remove_window();
    }
  } else {
    if (output.window) {
      sw_window_resize(output.window, output.outer_bounds.width, height);
    } else {
      create_window(height);
    }
  }

  height = output.padding;
  for (i = 0; i < output.items; i++) {
    sw_window_add(output.window, output.item[i].widget);
    sw_widget_set_position(output.item[i].widget, output.padding, height);
    height += output.item[i].bounds.height;
  }
}

/*************************************************************************
 ...
*************************************************************************/
static void remove0(void)
{
  int i;

  assert(output.items > 0);
  sw_widget_destroy(output.item[0].widget);
  for (i = 1; i < output.items; i++) {
    output.item[i - 1] = output.item[i];
  }
  output.items--;
}

/*************************************************************************
 ...
*************************************************************************/
static void remove_top(void)
{
  assert(output.items > 0);
  sw_remove_timeout(output.item[0].timeout);
  remove0();
}

/*************************************************************************
 ...
*************************************************************************/
static void callback_remove(void *data)
{
  int i, j = -1;

  remove_all();

  for (i = 0; i < output.items; i++) {
    if (data == output.item[i].widget) {
      j = i;
      break;
    }
  }
  if (j >= 0) {
    for (i = 0; i < j; i++) {
      remove_top();
    }
    remove0();
  }
  update();
}

/*************************************************************************
 ...
*************************************************************************/
void chat_add(const char *astring, int conn_id)
{
  struct ct_string *string;
  struct sw_widget *label;
  struct ct_rect rect;
  struct connection *conn = find_conn_by_id(conn_id);
  struct color *pcolor = color_alloc(0, 0, 0);
  struct player *pplayer=NULL;

  freelog(LOG_VERBOSE,"ogg_add(%d,%s)",conn_id, astring);

  if (output.window) {
    sw_window_resize(output.window, output.outer_bounds.width,
		     output.outer_bounds.height);
  } else {
    create_window(output.outer_bounds.height);
  }

  if (NULL != conn && NULL != conn->playing) {
    pplayer = conn->playing;
    color_free(pcolor);
    pcolor = get_player_color(tileset, pplayer);
  }

  freelog(LOG_VERBOSE, "id=%d conn=%p player=%s", conn_id, conn,
	  pplayer ? player_name(pplayer) : "none");
  
  string = ct_string_clone4(output.template, astring, pcolor->color);
  string = ct_string_wrap(string, output.inner_bounds.width);
  label = sw_label_create_text(output.window, string);
  sw_widget_set_position(label, 0, 0);
  sw_widget_get_bounds(label, &rect);
  sw_window_remove(label);
  
  remove_all();

  for (;;) {
    int remaining_height = get_remaining_height();

    if (remaining_height < rect.height) {
      assert(output.items > 0); /* label is too big */
      remove_top();
    } else {
      break;
    }
  }

  output.items++;
  output.item = fc_realloc(output.item, sizeof(*output.item) * output.items);
  output.item[output.items - 1].widget = label;
  output.item[output.items - 1].timeout =
      sw_add_timeout(output.delay, callback_remove,
		     output.item[output.items - 1].widget);
  output.item[output.items - 1].bounds = rect;
  update();
}

/*************************************************************************
 ...
*************************************************************************/
static void fc_send(void)
{
  send_chat(sw_edit_get_text(input.edit));
}

/*************************************************************************
 ...
*************************************************************************/
static bool my_key_handler(struct sw_widget *widget,
			   const struct be_key *key, void *data)
{
  if (key->type == BE_KEY_RETURN) {
    fc_send();
    chat_popdown_input();
    return TRUE;
  } else if (key->type == BE_KEY_ESCAPE) {
    chat_popdown_input();
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 ...
*************************************************************************/
void chat_popdown_input(void)
{
  if (input.window) {
    sw_widget_destroy(input.window);
    input.window = NULL;
  }
}

/*************************************************************************
 ...
*************************************************************************/
void chat_popup_input(void)
{
  struct ct_string *t = ct_string_clone3(input.template, "");

  assert(input.window == NULL);

  input.window =
      sw_window_create(root_window, input.bounds.width, input.bounds.height,
		       NULL, FALSE, INPUT_DEPTH);
  sw_widget_set_position(input.window, input.bounds.x, input.bounds.y);
  sw_window_set_draggable(input.window, FALSE);
  sw_window_set_key_notify(input.window, my_key_handler, NULL);

  input.edit = sw_edit_create(input.window, 40, t);
  sw_widget_set_position(input.edit, 0, 0);
  sw_widget_select(input.edit);
  ct_string_destroy(t);
}
