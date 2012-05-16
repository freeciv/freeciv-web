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
static void draw(struct sw_widget *widget)
{
  int max_length, length, m, offset;
  enum widget_face face = get_widget_face(widget);

  if (widget->dragged) {
    face = WF_PRESSED;
  }

  if (widget->data.slider.vertical) {
    m = widget->inner_bounds.width;
    max_length = widget->inner_bounds.height;
  } else {
    m = widget->inner_bounds.height;
    max_length = widget->inner_bounds.width;
  }

  length = widget->data.slider.width * max_length;
  offset = widget->data.slider.offset * max_length;
  if (length < widget->data.slider.cache_min_length) {
    length = widget->data.slider.cache_min_length;
  }

  if (length + offset > max_length) {
    length = max_length - offset;
  }

  if(0)
  printf("width=%f offset=%f\n", widget->data.slider.width,
	 widget->data.slider.offset);
  if(0)
  printf("  %d + %d = %d\n",length,offset,max_length);

  if (!widget->data.slider.faces[face]) {
    struct ct_rect rect;

    if (widget->data.slider.vertical) {
      rect.x = widget->inner_bounds.x;
      rect.y = widget->inner_bounds.y + offset;
      rect.width = m;
      rect.height = length;
    } else {
      rect.x = widget->inner_bounds.x + offset;
      rect.y = widget->inner_bounds.y;
      rect.width = length;
      rect.height = m;
    }

    be_draw_region(sw_widget_get_osda(widget), &rect,
		   widget->data.slider.color_active);
  } else {
    struct ct_point dest_pos;
    int center_height = widget->data.slider.cache_center_height;
    int top_height = widget->data.slider.cache_top_height;
    int repeat_height = length - 2 * top_height;
    int i;

    if (widget->data.slider.vertical) {
      dest_pos.x = widget->inner_bounds.x;
      dest_pos.y = widget->inner_bounds.y + offset;

      be_draw_sprite(sw_widget_get_osda(widget),
		     widget->data.slider.faces[SP_TOP][face], NULL,
		     &dest_pos, NULL);

      dest_pos.y += length - top_height;
      be_draw_sprite(sw_widget_get_osda(widget),
		     widget->data.slider.faces[SP_BOTTOM][face], NULL,
		     &dest_pos, NULL);

      dest_pos.y = widget->inner_bounds.y + offset + top_height;
      for (i = 0; i < repeat_height; i++) {
	be_draw_sprite(sw_widget_get_osda(widget),
		       widget->data.slider.faces[SP_REPEAT][face],NULL,
		       &dest_pos, NULL);
	dest_pos.y++;
      }

      if (widget->data.slider.faces[SP_CENTER][face]) {
	dest_pos.y =
	    widget->inner_bounds.y + offset + top_height +
	    (repeat_height - center_height) / 2;
	be_draw_sprite(sw_widget_get_osda(widget),
		       widget->data.slider.faces[SP_CENTER][face], NULL,
		       &dest_pos, NULL);
      }
    } else {
      dest_pos.x = widget->inner_bounds.x+offset;
      dest_pos.y = widget->inner_bounds.y;

      be_draw_sprite(sw_widget_get_osda(widget),
		     widget->data.slider.faces[SP_TOP][face], NULL,
		     &dest_pos, NULL);

      dest_pos.x += length - top_height;
      be_draw_sprite(sw_widget_get_osda(widget),
		     widget->data.slider.faces[SP_BOTTOM][face], NULL,
		     &dest_pos, NULL);

      dest_pos.x = widget->inner_bounds.x + offset + top_height;
      for (i = 0; i < repeat_height; i++) {
	be_draw_sprite(sw_widget_get_osda(widget),
		       widget->data.slider.faces[SP_REPEAT][face],NULL,
		       &dest_pos, NULL);
	dest_pos.x++;
      }

      if (widget->data.slider.faces[SP_CENTER][face]) {
	dest_pos.x =
	    widget->inner_bounds.x + offset + top_height +
	    (repeat_height - center_height) / 2;
	be_draw_sprite(sw_widget_get_osda(widget),
		       widget->data.slider.faces[SP_CENTER][face], NULL,
		       &dest_pos, NULL);
      }
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void drag_start(struct sw_widget *widget,
		       const struct ct_point *mouse,
		       enum be_mouse_button button)
{
  widget->data.slider.pos_at_drag_start = widget->data.slider.offset;

  if (!widget->data.slider.faces[0]) {
      sw_widget_set_border_color(widget, widget->data.slider.color_drag);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void drag_move(struct sw_widget *widget,
		      const struct ct_point *start_position,
		      const struct ct_point *current_position,
		      enum be_mouse_button button)
{
  int diff =
      widget->data.slider.vertical ? (current_position->y -
				      start_position->
				      y) : (current_position->x -
					    start_position->x);
  float diff_f =
      (float) diff /
      (widget->data.slider.vertical ? widget->inner_bounds.height : widget->
       inner_bounds.width);
  float new;

  //printf("diff = %d = %f\n", diff, diff_f);
  new = widget->data.slider.pos_at_drag_start + diff_f;

  sw_slider_set_offset(widget, new);
}

/*************************************************************************
  ...
*************************************************************************/
static void drag_end(struct sw_widget *widget, enum be_mouse_button button)
{
  if (!widget->data.slider.faces[0]) {
    sw_widget_set_border_color(widget, widget->data.slider.color_nodrag);
  }
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_slider_create(struct sw_widget *parent, int width,
				   int height, struct sprite *top,
				   struct sprite *bottom,struct sprite *repeat,
				   struct sprite *center,
				   bool vertical)
{
  struct sw_widget *result = create_widget(parent, WT_SLIDER);
  int i;

  result->draw = draw;
  result->drag_start = drag_start;
  result->drag_move = drag_move;
  result->drag_end = drag_end;

  result->can_be_selected = TRUE;
  result->can_be_dragged[BE_MB_LEFT] = TRUE;

  result->inner_bounds.width = width;
  result->inner_bounds.height = height;

  result->data.slider.vertical = vertical;
  result->data.slider.width = 1.0;
  result->data.slider.offset = 0.0;
  result->data.slider.callback = NULL;

  result->data.slider.color_active = be_get_color(255, 0, 0, MAX_OPACITY);
  result->data.slider.color_drag = be_get_color(255, 255, 255, MAX_OPACITY);
  result->data.slider.color_nodrag = be_get_color(0, 0, 0, MAX_OPACITY);

  if (!top && !bottom && !repeat && !center) {
    int j;

    sw_widget_set_border_color(result, result->data.slider.color_nodrag);
    for (i = 0; i < 4; i++) {
      for (j = 0; j < NUM_SP; j++) {
	result->data.slider.faces[j][i] = NULL;
      }
    }
    result->data.slider.cache_min_length = 10;
  } else {
    struct ct_size top_size;
    struct ct_size bottom_size;
    struct ct_size repeat_size;
    struct ct_size center_size;

    struct ct_size top_size2;
    struct ct_size bottom_size2;
    struct ct_size repeat_size2;
    struct ct_size center_size2;

    assert(top && bottom && repeat);

    be_sprite_get_size(&top_size, top);
    be_sprite_get_size(&bottom_size, bottom);
    be_sprite_get_size(&repeat_size, repeat);
    if (center) {
      be_sprite_get_size(&center_size, center);
    }

    assert((top_size.height % 4) == 0);
    assert((bottom_size.height % 4) == 0);
    assert((repeat_size.height % 4) == 0);
    if (center) {
      assert((center_size.height % 4) == 0);
    }

    top_size2 = top_size;
    top_size2.height /= 4;

    bottom_size2 = bottom_size;
    bottom_size2.height /= 4;

    repeat_size2 = repeat_size;
    repeat_size2.height /= 4;

    /* This is outside 'if (center)' -block to avoid
     * '...might be used uninitialized...' warning from
     * compiler that does not realize that it's never
     * used if center == NULL. */
    center_size2 = center_size;
    if (center) {
      center_size2.height /= 4;
    }

    for (i = 0; i < 4; i++) {
      result->data.slider.faces[SP_TOP][i] =
	  be_crop_sprite(top, 0, i * top_size2.height,
		      top_size2.width, top_size2.height);
      result->data.slider.faces[SP_BOTTOM][i] =
	  be_crop_sprite(bottom, 0, i * bottom_size2.height,
		      bottom_size2.width, bottom_size2.height);
      result->data.slider.faces[SP_REPEAT][i] =
	  be_crop_sprite(repeat, 0, i * repeat_size2.height,
		      repeat_size2.width, repeat_size2.height);
      if (center) {
	result->data.slider.faces[SP_CENTER][i] =
	    be_crop_sprite(center, 0, i * center_size2.height,
			center_size2.width, center_size2.height);
      } else {
	result->data.slider.faces[SP_CENTER][i] = NULL;
      }
    }

    if (vertical) {
      assert(top_size2.width == bottom_size2.width
	     && top_size2.width == repeat_size2.width
	     && top_size2.width == width);
      assert(top_size2.height == bottom_size2.height);
      assert(repeat_size2.height == 1);

      if (center) {
	assert(top_size2.width == center_size2.width);
      }
      result->data.slider.cache_top_height = top_size2.height;
      if (center) {
	result->data.slider.cache_center_height = center_size2.height;
      } else {
	result->data.slider.cache_center_height = 0;
      }
    } else {
      assert(top_size2.height == bottom_size2.height);
      assert(top_size2.height == repeat_size2.height);
      printf("%d %d\n",top_size2.height, height);
      assert(top_size2.height == height);
      assert(top_size2.width == bottom_size2.width);
      assert(repeat_size2.width == 1);

      if (center) {
	assert(top_size2.height == center_size2.height);
      }
      result->data.slider.cache_top_height = top_size2.width;
      if (center) {
	result->data.slider.cache_center_height = center_size2.width;
      } else {
	result->data.slider.cache_center_height = 0;
      }
    }

    result->data.slider.cache_min_length =
	MAX(2 * result->data.slider.cache_top_height,
	    result->data.slider.cache_center_height + 10);
  }

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_slider_set_offset(struct sw_widget *widget, float offset)
{
  if (offset < 0.0) {
    offset = 0.0;
  }
  if(0)
  printf("set_offset(%f) width=%f\n", offset, widget->data.slider.width);
  if (offset + widget->data.slider.width > 1.0) {
    offset = 1.0 - widget->data.slider.width;
  }
  if(0)
  printf("  %f -> %f\n",widget->data.slider.offset,offset);
  widget->data.slider.offset = offset;
  if (widget->data.slider.callback) {
    widget->data.slider.callback(widget, widget->data.slider.callback_data);
  }
  widget_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_slider_set_width(struct sw_widget *widget, float width)
{
  if (width < 0.0) {
    width = 0.0;
  }
  if (width > 1.0) {
    width = 1.0;
  }
  widget->data.slider.width = width;
  if (widget->data.slider.offset + width > 1.0) {
    widget->data.slider.offset = 1.0 - width;
  }
  widget_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
float sw_slider_get_offset(struct sw_widget *widget)
{
  return widget->data.slider.offset;
}

/*************************************************************************
  ...
*************************************************************************/
float sw_slider_get_width(struct sw_widget *widget)
{
  return widget->data.slider.width;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_slider_set_slided_notify(struct sw_widget *widget,
				 void (*callback) (struct sw_widget * widget,
						   void *data), void *data)
{
  widget->data.slider.callback = callback;
  widget->data.slider.callback_data = data;
}
