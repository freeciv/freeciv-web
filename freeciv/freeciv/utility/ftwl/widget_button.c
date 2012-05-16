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

#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <stdio.h>

#include "widget_p.h"

#include "mem.h"

#define STRING_PADDING 4
#define ICON_PADDING 4

/*************************************************************************
  ...
*************************************************************************/
static void draw(struct sw_widget *widget)
{
  struct ct_point pos;
  enum widget_face face = get_widget_face(widget);
  struct ct_size size =
      { widget->inner_bounds.width, widget->inner_bounds.height };

  if (widget->data.button.background_faces[face]) {
    pos.x = widget->inner_bounds.x;
    pos.y = widget->inner_bounds.y;
    be_copy_osda_to_osda(sw_widget_get_osda(widget),
			 widget->data.button.background_faces[face],
			 &size, &pos, NULL);
  }

  if (widget->data.button.text[face]) {
    pos.x = widget->inner_bounds.x + widget->data.button.text_offset[face].x;
    pos.y = widget->inner_bounds.y + widget->data.button.text_offset[face].y;

    be_draw_string(sw_widget_get_osda(widget), &pos, widget->data.button.text[face]);
  }

  if (widget->data.button.foreground_faces[face]) {
    pos.x =
	widget->inner_bounds.x +
	widget->data.button.foreground_faces_offset[face].x;
    pos.y =
	widget->inner_bounds.y +
	widget->data.button.foreground_faces_offset[face].y;

    be_copy_osda_to_osda(sw_widget_get_osda(widget),
			 widget->data.button.foreground_faces[face],
			 &size, &pos, NULL);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void click(struct sw_widget *widget)
{
  if (widget->data.button.callback) {
    widget->data.button.callback(widget, widget->data.button.callback_data);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static bool key(struct sw_widget *widget, const struct be_key *key,
		void *data)
{
  if (widget->data.button.shortcut
      && ct_key_matches(widget->data.button.shortcut, key)) {
    click(widget);
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
  ...
*************************************************************************/
static void destroy(struct sw_widget *widget)
{
  int i;

  if (widget->data.button.text[WF_NORMAL]) {
    ct_string_destroy(widget->data.button.text[WF_NORMAL]);
  }

  for (i = 0; i < 4; i++) {
    if (widget->data.button.background_faces[i]) {
      be_free_osda(widget->data.button.background_faces[i]);
    }
  }
}


/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_button_create(struct sw_widget *parent,
				   struct ct_string *strings[4],
				   struct osda *foreground_faces[4],
				   struct osda *background_faces[4])
{
  struct sw_widget *result = create_widget(parent, WT_BUTTON);
  int i;
  struct ct_size size = { 0, 0 };

  result->destroy = destroy;
  result->draw = draw;
  result->click = click;
  result->key = key;
  result->key_data = NULL;

  result->can_be_pressed = TRUE;
  result->can_be_selected = TRUE;

  result->data.button.callback = NULL;
  result->data.button.shortcut = NULL;

  assert(!(strings && foreground_faces));
  assert(strings || foreground_faces || background_faces);

  /* First stage: set the fields and calculate the size */
  if (strings) {
    for (i = 0; i < 4; i++) {
      result->data.button.text[i] = strings[i];
      size.width =
	  MAX(size.width,
	      result->data.button.text[i]->size.width + 2 * STRING_PADDING);
      size.height =
	  MAX(size.height,
	      result->data.button.text[i]->size.height + 2 * STRING_PADDING);
    }
  } else {
    result->data.button.text[WF_NORMAL] = NULL;
    result->data.button.text[WF_SELECTED] = NULL;
    result->data.button.text[WF_PRESSED] = NULL;
    result->data.button.text[WF_DISABLED] = NULL;
  }

  if (foreground_faces) {
    for (i = 0; i < 4; i++) {
      struct ct_size s;

      result->data.button.foreground_faces[i] = foreground_faces[i];
      be_osda_get_size(&s, foreground_faces[i]);
      size.width = MAX(size.width, s.width + 2 * STRING_PADDING);
      size.height = MAX(size.height, s.height + 2 * STRING_PADDING);
    }
  } else {
    result->data.button.foreground_faces[WF_NORMAL] = NULL;
    result->data.button.foreground_faces[WF_SELECTED] = NULL;
    result->data.button.foreground_faces[WF_PRESSED] = NULL;
    result->data.button.foreground_faces[WF_DISABLED] = NULL;
  }


  if (background_faces) {
    int i;

    for (i = 0; i < 4; i++) {
      struct ct_size s;

      result->data.button.background_faces[i] = background_faces[i];
      be_osda_get_size(&s, background_faces[i]);
      size.width = MAX(size.width, s.width);
      size.height = MAX(size.height, s.height);
    }
  } else {
    result->data.button.background_faces[WF_NORMAL] = NULL;
    result->data.button.background_faces[WF_SELECTED] = NULL;
    result->data.button.background_faces[WF_PRESSED] = NULL;
    result->data.button.background_faces[WF_DISABLED] = NULL;
  }

  result->inner_bounds.width = size.width;
  result->inner_bounds.height = size.height;

  /* Second stage: center */

  if (strings) {
    int i;
    struct ct_rect bbox = result->inner_bounds;

    bbox.x = STRING_PADDING;
    bbox.y = STRING_PADDING;
    bbox.width -= 2 * STRING_PADDING;
    bbox.height -= 2 * STRING_PADDING;

    for (i = 0; i < 4; i++) {
      struct ct_rect t = { 0, 0,
	result->data.button.text[i]->size.width,
	result->data.button.text[i]->size.height
      };
      align(&bbox, &t, A_CENTER);
      result->data.button.text_offset[i].x = t.x;
      result->data.button.text_offset[i].y = t.y;
    }
  }

  if (foreground_faces) {
    int i;
    struct ct_rect bbox = result->inner_bounds;

    bbox.x = ICON_PADDING;
    bbox.y = ICON_PADDING;
    bbox.width -= 2 * ICON_PADDING;
    bbox.height -= 2 * ICON_PADDING;

    for (i = 0; i < 4; i++) {
      struct ct_size s;

      be_osda_get_size(&s, result->data.button.foreground_faces[i]);

      {
	struct ct_rect t = { 0, 0, s.width, s.height };
	align(&bbox, &t, A_CENTER);
	result->data.button.foreground_faces_offset[i].x = t.x;
	result->data.button.foreground_faces_offset[i].y = t.y;
      }
    }
  }

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_button_create_text_and_background(struct sw_widget
						       *parent, struct ct_string
						       *string, struct sprite
						       *background_faces)
{
  struct ct_size whole_size, size;
  struct ct_string *strings[4];
  struct osda *faces[4];
  int i;
     
  be_sprite_get_size(&whole_size, background_faces);

  assert((whole_size.height % 4) == 0);

  size = whole_size;
  size.height /= 4;

  for (i = 0; i < 4; i++) {
    struct osda *t = be_create_osda(size.width, size.height);
    struct sprite *s = be_crop_sprite(background_faces, 0, i * size.height,
				      size.width, size.height);
    struct ct_point point = { 0, 0 };

    be_draw_sprite(t, s, &size, &point, &point);
    faces[i] = t;
  }

  for (i = 0; i < 4; i++) {
    strings[i] = string;
  }
  return sw_button_create(parent, string ? strings : NULL, NULL, faces);
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_button_create_bounded(struct sw_widget *parent,
					   struct ct_string *string,
					   struct sprite *background_faces,
					   const struct ct_rect *bounds,
					   enum ws_alignment alignment)
{
  int size;

  for (size = string->font_size; size > 0; size--) {
    struct ct_string *string2 = ct_string_clone1(string, size);
    if (string2->size.width <= bounds->width &&
	string2->size.height + 2 * STRING_PADDING <= bounds->height) {
      struct sw_widget *result =
	  sw_button_create_text_and_background(parent, string2, background_faces);
      sw_widget_align_box(result, alignment, bounds);
      return result;
    } else {
      ct_string_destroy(string2);
    }
  }
  assert(0);
  return NULL;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_button_set_callback(struct sw_widget *widget,
			    void (*callback) (struct sw_widget * widget,
					      void *data), void *data)
{
  widget->data.button.callback = callback;
  widget->data.button.callback_data = data;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_button_set_shortcut(struct sw_widget *widget,
			    const struct be_key *key)
{
  if (widget->data.button.shortcut) {
    ct_key_destroy(widget->data.button.shortcut);
  }
  widget->data.button.shortcut = ct_key_clone(key);
}
