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
static enum widget_type sw_widget_get_type(const struct sw_widget *widget)
{
  return widget->type;
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *create_widget(struct sw_widget *parent,
				enum widget_type type)
{
  struct sw_widget *result = fc_malloc(sizeof(*result));

  result->parent = NULL;
  if (parent) {
    assert(sw_widget_get_type(parent) == WT_WINDOW);
    sw_window_add(parent, result);
  }

  result->type = type;
  result->pressed = FALSE;
  result->selected = FALSE;
  result->disabled = FALSE;
  result->dragged=FALSE;
  result->accepts_events[EV_MOUSE] = TRUE;
  result->accepts_events[EV_KEYBOARD] = TRUE;

  result->can_be_pressed = FALSE;
  result->can_be_selected = FALSE;
  result->can_be_dragged[BE_MB_LEFT] = FALSE;
  result->can_be_dragged[BE_MB_RIGHT] = FALSE;
  result->can_be_dragged[BE_MB_MIDDLE] = FALSE;

  result->needs_repaint = TRUE;

  result->pos.x = -1;
  result->pos.y = -1;
  result->inner_bounds.width = 0;
  result->inner_bounds.height = 0;

  result->destroy = NULL;
  result->entered = NULL;
  result->left = NULL;
  result->click = NULL;
  result->click_start = NULL;
  result->key = NULL;
  result->draw = NULL;
  result->draw_extra_background = NULL;
  result->drag_start=NULL;
  result->drag_move=NULL;
  result->drag_end=NULL;

  result->tooltip = NULL;

  result->has_background_color = FALSE;
  result->background_sprite = NULL;
  result->has_border_color = FALSE;

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_set_tooltip(struct sw_widget *widget,
			   const struct ct_tooltip *tooltip)
{
  if (widget->tooltip) {
    untooltip(widget);
    ct_tooltip_destroy(widget->tooltip);
  }

  widget->tooltip = ct_tooltip_clone(tooltip);
  widget->tooltip_shown = FALSE;
  widget->tooltip_callback_id = 0;

  widget_needs_paint(widget);
}

/*************************************************************************
  input is pos and inner_bounds.{width,height}
*************************************************************************/
static void update_bounds(struct sw_widget *widget)
{
  int border = 0;

  if (widget->pos.x == -1 && widget->pos.y == -1) {
    return;
  }

  assert(ct_point_valid(&widget->pos));

  if (widget->has_border_color) {
    border = BORDER_WIDTH;
  }

  widget->outer_bounds.x = widget->pos.x;
  widget->outer_bounds.y = widget->pos.y;
  widget->outer_bounds.width = widget->inner_bounds.width + 2 * border;
  widget->outer_bounds.height = widget->inner_bounds.height + 2 * border;

  if (widget->type != WT_WINDOW) {
    widget->inner_bounds.x = widget->pos.x + border;
    widget->inner_bounds.y = widget->pos.y + border;
  } else {
    widget->inner_bounds.x = border;
    widget->inner_bounds.y = border;

    widget->data.window.children_bounds.x = border;
    widget->data.window.children_bounds.y =
	border + widget->data.window.inner_deco_height;
    widget->data.window.children_bounds.width = widget->inner_bounds.width;
    widget->data.window.children_bounds.height =
	widget->inner_bounds.height - widget->data.window.inner_deco_height;
    if (!ct_rect_valid(&widget->data.window.children_bounds)) {
      printf("children bounds of %p are %s and so invalid\n", widget,
	     ct_rect_to_string(&widget->data.window.children_bounds));
      assert(0);
    }
  }
 
  if (!ct_rect_valid(&widget->outer_bounds)) {
    printf("outer_bounds of %p are %s and so invalid\n", widget,
	   ct_rect_to_string(&widget->outer_bounds));
    assert(0);
  }
  if (!ct_rect_valid(&widget->inner_bounds)) {
    printf("inner_bounds of %p are %s and so invalid\n", widget,
	   ct_rect_to_string(&widget->inner_bounds));
    assert(0);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void border_changed(struct sw_widget *widget)
{
  update_bounds(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void inner_size_changed(struct sw_widget *widget)
{
  update_bounds(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_set_position(struct sw_widget *widget, int x, int y)
{
  widget->pos.x = x;
  widget->pos.y = y;

  assert(widget->pos.x >= 0 && widget->pos.y >= 0);

  if (widget->parent) {
    widget->pos.x += widget->parent->data.window.children_bounds.x;
    widget->pos.y += widget->parent->data.window.children_bounds.y;
  }
  assert(widget->pos.x >= 0 && widget->pos.y >= 0);

  update_bounds(widget);

  assert(ct_rect_valid(&widget->outer_bounds));
  assert(ct_rect_valid(&widget->inner_bounds));

  if (widget->parent && !ct_rect_in_rect
      (&widget->outer_bounds,
       &widget->parent->data.window.children_bounds)) {
    printf("child %p (%s) ", widget,
	   ct_rect_to_string(&widget->outer_bounds));
    printf("is outside of parent %p (%s)\n", widget->parent,
	   ct_rect_to_string(&widget->parent->data.window.children_bounds));
  }
  parent_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_hcenter(struct sw_widget *widget)
{
  sw_widget_set_position(widget,
			 (widget->parent->data.window.children_bounds.width -
			  widget->outer_bounds.width) / 2,
			 widget->outer_bounds.y);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_vcenter(struct sw_widget *widget)
{
  sw_widget_set_position(widget, widget->outer_bounds.x,
			 (widget->parent->data.window.children_bounds.
			  height - widget->outer_bounds.height) / 2);
}

/*************************************************************************
  ...
*************************************************************************/
void align(const struct ct_rect *bb, struct ct_rect *item,
	   enum ws_alignment alignment)
{
  switch (alignment) {
  case A_W:
  case A_NW:
  case A_SW:
  case A_WC:
    item->x = bb->x;
    break;
  case A_E:
  case A_NE:
  case A_SE:
      case A_EC:
    item->x = bb->x+bb->width - item->width;
    break;
  case A_WE:
  case A_CENTER:
  case A_NC:
  case A_SC:
    item->x = bb->x+(bb->width - item->width) / 2;
    break;
  case A_N:
  case A_S:
    break;
  default:
    assert(0);
  }

  switch (alignment) {
  case A_N:
  case A_NW:
  case A_NE:
      case A_NC:
    item->y = bb->y;
    break;
  case A_S:
  case A_SW:
  case A_SE:
      case A_SC:
    item->y = bb->y+bb->height - item->height;
    break;
  case A_NS:
  case A_CENTER:
      case A_WC:
      case A_EC:
    item->y = bb->y+(bb->height - item->height) / 2;
    break;
  case A_W:
  case A_E:
    break;
  default:
    assert(0);
  }
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_align_parent(struct sw_widget *widget, enum ws_alignment alignment)
{
  struct sw_widget *window = widget->parent;
  struct ct_rect rect, bb;

  if (widget->pos.x == -1 && widget->pos.y == -1) {
    sw_widget_set_position(widget, 0, 0);
  }

  update_bounds(widget);
  rect = widget->outer_bounds;
  bb = window->data.window.children_bounds;
  bb.x = 0;
  bb.y = 0;

  align(&bb, &rect, alignment);
  sw_widget_set_position(widget, rect.x, rect.y);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_align_box(struct sw_widget *widget,
			 enum ws_alignment alignment, const struct ct_rect
			 *box)
{
  struct ct_rect rect, bb;

  if (widget->pos.x == -1 && widget->pos.y == -1) {
    sw_widget_set_position(widget, 0, 0);
  }

  update_bounds(widget);
  rect = widget->outer_bounds;
  bb = *box;

  align(&bb, &rect, alignment);
  sw_widget_set_position(widget, rect.x, rect.y);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_set_background_sprite(struct sw_widget *widget,
				     struct sprite *sprite)
{
  widget->background_sprite = sprite;
  widget_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_set_background_color(struct sw_widget *widget,
				    be_color background_color)
{
  widget->has_background_color = TRUE;
  widget->background_color = background_color;
  widget_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_set_border_color(struct sw_widget *widget,
				be_color border_color)
{
  widget->has_border_color = TRUE;
  widget->border_color = border_color;
  border_changed(widget);
  widget_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_get_bounds(struct sw_widget *widget, struct ct_rect *bounds)
{
  *bounds = widget->outer_bounds;
  bounds->y -= widget->parent->data.window.inner_deco_height;
}

/*************************************************************************
  ...
*************************************************************************/
struct osda *sw_widget_get_osda(struct sw_widget *widget)
{
  if (widget->type == WT_WINDOW) {
    return widget->data.window.target;
  }
  assert(widget->parent && widget->parent->type == WT_WINDOW);
  return widget->parent->data.window.target;
}


/*************************************************************************
  ...
*************************************************************************/
void sw_widget_destroy(struct sw_widget *widget)
{
  assert(widget);
  widget_list_prepend(deferred_destroyed_widgets, widget);
  parent_needs_paint(widget);
  // FIXME this is unsafe if
  // widget_list_size(deferred_destroyed_widgets) > 1
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_disable_mouse_events(struct sw_widget *widget)
{
  widget->accepts_events[EV_MOUSE] = FALSE;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_set_enabled(struct sw_widget *widget, bool enabled)
{
  widget->disabled = !enabled;
}
