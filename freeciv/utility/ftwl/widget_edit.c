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

/*************************************************************************
  ...
*************************************************************************/
static bool key(struct sw_widget *widget, const struct be_key *key,
		void *data)
{
  int cursor = widget->data.edit.cursor;
  int chars = strlen(widget->data.edit.buffer);

  if (key->control && key->type == BE_KEY_NORMAL && key->key == 'a') {
    widget->data.edit.cursor = 0;
  } else if (key->control && key->type == BE_KEY_NORMAL && key->key == 'e') {
    widget->data.edit.cursor = strlen(widget->data.edit.buffer);
  } else if (key->type == BE_KEY_LEFT) {
    widget->data.edit.cursor--;
  } else if (key->type == BE_KEY_RIGHT) {
    widget->data.edit.cursor++;
  } else if (key->type == BE_KEY_BACKSPACE) {
    if (cursor > 0) {
      memmove(widget->data.edit.buffer + cursor - 1,
	      widget->data.edit.buffer + cursor, chars - cursor);
      widget->data.edit.buffer[chars - 1] = '\0';
      widget->data.edit.cursor--;
    }
  } else if (key->type == BE_KEY_DELETE) {
    if (chars > 1 && cursor != chars - 1) {
      memmove(widget->data.edit.buffer + cursor,
	      widget->data.edit.buffer + cursor + 1, chars - cursor);
      widget->data.edit.buffer[chars - 1] = '\0';
    }
  } else if (key->type == BE_KEY_NORMAL) {
    if (chars <= widget->data.edit.max_size) {
      //printf("prev='%s' ", widget->data.edit.buffer);
      memmove(widget->data.edit.buffer + cursor + 1,
	      widget->data.edit.buffer + cursor, chars - cursor);
      widget->data.edit.buffer[chars + 1] = '\0';
      widget->data.edit.buffer[cursor] = key->key;
      widget->data.edit.cursor++;
      //printf(" -> '%s'\n", widget->data.edit.buffer);
    }
  } else {
    return FALSE;
  }
      
  if (widget->data.edit.cursor < 0) {
    widget->data.edit.cursor = 0;
  }

  if (widget->data.edit.cursor >= strlen(widget->data.edit.buffer)) {
    widget->data.edit.cursor = strlen(widget->data.edit.buffer) - 1;
  }
  widget_needs_paint(widget);
  return TRUE;
}

/*************************************************************************
  ...
*************************************************************************/
static void draw(struct sw_widget *widget)
{
  int chars = strlen(widget->data.edit.buffer);
  struct ct_point pos;
  struct ct_rect rect;
  //const int PADDING = 5;
  int i;
  struct ct_string *all = ct_string_clone3(widget->data.edit.template,
					   widget->data.edit.buffer);

  rect = widget->inner_bounds;

  be_draw_region(sw_widget_get_osda(widget), &rect,
		 widget->data.edit.template->background);

  pos.x =
      widget->inner_bounds.x + (widget->inner_bounds.width -
				all->size.width) / 2;
  pos.y =
      widget->inner_bounds.y + (widget->inner_bounds.height -
				all->size.height) / 2;

  for (i = 0; i < chars; i++) {
    struct ct_string *t;
    char one[2];

    one[0] = widget->data.edit.buffer[i];
    one[1] = '\0';

    t = ct_string_clone3(widget->data.edit.template, one);

    if (widget->data.edit.cursor == i && widget->selected) {
      rect.x = pos.x;
      rect.y = pos.y;
      rect.width = t->size.width;
      rect.height = t->size.height;

      be_draw_region(sw_widget_get_osda(widget), &rect,
		     widget->data.edit.color1);
    }

    if (i != chars - 1) {
      be_draw_string(sw_widget_get_osda(widget), &pos, t);
    }

    if (widget->data.edit.cursor == i && !widget->selected) {
      rect.x = pos.x;
      rect.y = pos.y;
      rect.width = t->size.width;
      rect.height = t->size.height;

      be_draw_rectangle(sw_widget_get_osda(widget), &rect, 1,
			widget->data.edit.color2);
    }

    pos.x += t->size.width;
    ct_string_destroy(t);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void entered(struct sw_widget *widget)
{
  sw_widget_set_border_color(widget, widget->data.edit.color_selected);
}

/*************************************************************************
  ...
*************************************************************************/
static void left(struct sw_widget *widget)
{
  sw_widget_set_border_color(widget, widget->data.edit.color_noselected);
}

/*************************************************************************
  ...
*************************************************************************/
static void destroy(struct sw_widget *widget)
{
  ct_string_destroy(widget->data.edit.template);
  free(widget->data.edit.buffer);
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_edit_create(struct sw_widget *parent,int max_size,
				 struct ct_string *temp_and_initial_text)
{
  struct sw_widget *result = create_widget(parent, WT_EDIT);
  char *tmp = fc_malloc(max_size + 2);
  int i;
  struct ct_string *all;

  result->destroy = destroy;
  result->entered=entered;
  result->left=left;
  result->draw = draw;
  result->key = key;
  result->key_data = NULL;
  result->can_be_selected = TRUE;

  // FIXME make configurable
  result->data.edit.color1 = be_get_color(255, 0, 0, MAX_OPACITY);
  result->data.edit.color2 = be_get_color(255, 0, 0, MAX_OPACITY);
  result->data.edit.color_selected = be_get_color(255, 255, 255, MAX_OPACITY);
  result->data.edit.color_noselected = be_get_color(0, 0, 0, MAX_OPACITY);

  for (i = 0; i < max_size + 1; i++) {
    tmp[i] = 'M';
  }
  tmp[i] = '\0';

  result->data.edit.max_size = max_size;
  result->data.edit.template = ct_string_clone(temp_and_initial_text);
  assert(strlen(temp_and_initial_text->text) <= max_size);
  result->data.edit.buffer = fc_malloc(max_size + 2);
  strcpy(result->data.edit.buffer, temp_and_initial_text->text);
  strcat(result->data.edit.buffer, "x");
  result->data.edit.cursor = 0;

  all = ct_string_clone3(result->data.edit.template, tmp);
  free(tmp);
  result->inner_bounds.width = all->size.width;
  result->inner_bounds.height = 2 * all->size.height;
  result->outer_bounds=result->inner_bounds;

  sw_widget_set_background_color(result,
				 result->data.edit.template->background);
  sw_widget_set_border_color(result, result->data.edit.color_noselected);

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_edit_create_bounded(struct sw_widget *parent,
					 int max_size,
					 struct ct_string *template,
					 struct ct_rect *bounds,
					 enum ws_alignment alignment)
{
  int i, size;
  char *tmp = fc_malloc(max_size + 2);

  for (i = 0; i < max_size + 1; i++) {
    tmp[i] = 'M';
  }
  tmp[i] = '\0';

  for (size = template->font_size; size > 0; size--) {
    struct ct_string *string2 = ct_string_clone2(template, size, tmp);
    if (string2->size.width <= bounds->width &&
	2 * string2->size.height+2 <= bounds->height) {
      struct ct_string *real_template = ct_string_clone1(template, size);
      struct sw_widget *result =
	  sw_edit_create(parent, max_size, real_template);

      sw_widget_align_box(result, alignment,bounds);
      ct_string_destroy(string2);
      free(tmp);
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
const char *sw_edit_get_text(struct sw_widget *widget)
{
  static char *tmp;

  tmp = fc_realloc(tmp, strlen(widget->data.edit.buffer) + 1);
  strcpy(tmp, widget->data.edit.buffer);
  tmp[strlen(tmp) - 1] = '\0';
  return tmp;
}
