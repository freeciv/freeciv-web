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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <stdio.h>

#include "mem.h"
#include "widget_p.h"

/*************************************************************************
  ...
*************************************************************************/
static struct sw_widget **get_item(struct sw_widget *widget, int column,
				   int row)
{
  struct sw_widget **result =
      widget->data.list.items + row * widget->data.list.columns + column;

  assert(column >= 0 && column < widget->data.list.columns);
  assert(row >= 0 && row < widget->data.list.rows);

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
static void ensure_size(struct sw_widget *widget, int column, int row)
{
  assert(column >= -1 && row >= -1);

  if (column > widget->data.list.columns - 1 ||
      row > widget->data.list.rows - 1) {
    int old_columns = widget->data.list.columns;
    int old_rows = widget->data.list.rows;
    int new_columns = MAX(column + 1, old_columns);
    int new_rows = MAX(row + 1, old_rows);
    int x, y;
    struct sw_widget **new_items =
	fc_malloc(sizeof(*new_items) * new_columns * new_rows);

    if(0)
    printf("resizing from %dx%d to %dx%d\n", old_columns, old_rows,
	   new_columns, new_rows);
    for (x = 0; x < new_columns; x++) {
      for (y = 0; y < new_rows; y++) {
	new_items[y * new_columns + x] = NULL;
      }
    }

    for (x = 0; x < old_columns; x++) {
      for (y = 0; y < old_rows; y++) {
	new_items[y * new_columns + x] = *get_item(widget, x, y);
      }
    }

    free(widget->data.list.items);
    widget->data.list.items = new_items;
    widget->data.list.columns = new_columns;
    widget->data.list.rows = new_rows;

    if (new_columns != old_columns) {
      enum ws_alignment *new_column_alignments =
	  fc_malloc(sizeof(*new_column_alignments) * new_columns);

      for (x = 0; x < new_columns; x++) {
	new_column_alignments[x] = A_WC;
      }

      for (x = 0; x < old_columns; x++) {
	new_column_alignments[x] = widget->data.list.column_alignments[x];
      }
      free(widget->data.list.column_alignments);
      widget->data.list.column_alignments = new_column_alignments;
    }

    if (new_rows != old_rows) {
      bool *new_row_enabled = fc_malloc(sizeof(*new_row_enabled) * new_rows);

      for (y = 0; y < new_rows; y++) {
	new_row_enabled[y] = TRUE;
      }

      for (y = 0; y < old_rows; y++) {
	new_row_enabled[y] = widget->data.list.row_enabled[y];
      }
      free(widget->data.list.row_enabled);
      widget->data.list.row_enabled = new_row_enabled;
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void layout(struct sw_widget *widget)
{
  int x, y;

  if (widget->data.list.columns == 0 && widget->data.list.rows == 0) {
    return;
  }
  remove_all_from_window(widget->data.list.window);

  widget->data.list.widths =
      fc_realloc(widget->data.list.widths,
		 sizeof(*widget->data.list.widths) *
		 widget->data.list.columns);
  widget->data.list.heights =
      fc_realloc(widget->data.list.heights,
		 sizeof(*widget->data.list.heights) *
		 widget->data.list.rows);
  widget->data.list.start_x =
      fc_realloc(widget->data.list.start_x,
		 sizeof(*widget->data.list.start_x) *
		 widget->data.list.columns);
  widget->data.list.start_y =
      fc_realloc(widget->data.list.start_y,
		 sizeof(*widget->data.list.start_y) *
		 widget->data.list.rows);

  for (x = 0; x < widget->data.list.columns; x++) {
    int max = 0;

    for (y = 0; y < widget->data.list.rows; y++) {
      struct sw_widget *w = *get_item(widget, x, y);

      if (w) {
	max = MAX(max, w->outer_bounds.width);
      }
    }
    widget->data.list.widths[x] = max;
  }

  for (y = 0; y < widget->data.list.rows; y++) {
    int max = 0;

    for (x = 0; x < widget->data.list.columns; x++) {
      struct sw_widget *w = *get_item(widget, x, y);
      if (w) {
	max = MAX(max, w->outer_bounds.height);
      }
    }
    widget->data.list.heights[y] = max;
  }

  widget->data.list.start_x[0] = 0;
  for (x = 0; x < widget->data.list.columns - 1; x++) {
    widget->data.list.start_x[x + 1] =
	widget->data.list.start_x[x] + widget->data.list.widths[x];
  }

  widget->data.list.start_y[0] = 0;
  for (y = 0; y < widget->data.list.rows - 1; y++) {
    widget->data.list.start_y[y + 1] =
	widget->data.list.start_y[y] + widget->data.list.heights[y];
  }

  if (1) {
    for (x = 0; x < widget->data.list.columns; x++) {
      printf("column[%d] starts at %d and has width %d\n", x,
	     widget->data.list.start_x[x], widget->data.list.widths[x]);
    }

    for (y = 0; y < widget->data.list.rows; y++) {
      printf("row[%d] starts %d and has height %d\n", y,
	     widget->data.list.start_y[y], widget->data.list.heights[y]);
    }
  }

  {
    int new_width =
	widget->data.list.start_x[widget->data.list.columns - 1] +
	widget->data.list.widths[widget->data.list.columns - 1];
    int new_height =
	widget->data.list.start_y[widget->data.list.rows - 1] +
	widget->data.list.heights[widget->data.list.rows - 1];
    if (new_width < widget->inner_bounds.width) {
      new_width = widget->inner_bounds.width;
    }
    if (new_height == 0) {
      new_height = 1;
    }

    printf("resize to %dx%d\n", new_width, new_height);
    sw_window_resize(widget->data.list.window, new_width, new_height);
  }

  for (x = 0; x < widget->data.list.columns; x++) {
    for (y = 0; y < widget->data.list.rows; y++) {
      struct sw_widget *w = *get_item(widget, x, y);

      if (w) {
	struct ct_rect bb;
	struct ct_rect pos;

	bb.x = widget->data.list.start_x[x];
	bb.y = widget->data.list.start_y[y];
	bb.width = widget->data.list.widths[x];
	bb.height = widget->data.list.heights[y];

	pos = w->outer_bounds;
	pos.x = 0;
	pos.y = 0;

	align(&bb, &pos, widget->data.list.column_alignments[x]);

	sw_window_add(widget->data.list.window, w);
	sw_widget_set_position(w, pos.x, pos.y);
      }
    }
  }

  widget_needs_paint(widget);
  widget->data.list.needs_layout = FALSE;
  update_window(widget->data.list.window);
}

/*************************************************************************
  ...
*************************************************************************/
static void ensure_valid_layout(struct sw_widget *widget)
{
  if (widget->data.list.needs_layout) {
    layout(widget);
    if (widget->data.list.callback1) {
      widget->data.list.callback1(widget, widget->data.list.callback1_data);
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void draw(struct sw_widget *widget)
{
  struct ct_size size;
  struct ct_point src_pos;
  struct ct_point dest_pos;
  struct sw_widget *window = widget->data.list.window;

  ensure_valid_layout(widget);
  size.width = MIN(window->inner_bounds.width, widget->inner_bounds.width);
  size.height = MIN(window->inner_bounds.height,widget->inner_bounds.height);

  src_pos = widget->data.list.offset;

  dest_pos.x = widget->inner_bounds.x;
  dest_pos.y = widget->inner_bounds.y;

  be_copy_osda_to_osda(sw_widget_get_osda(widget), window->data.window.target, &size,
		       &dest_pos, &src_pos);
}

/*************************************************************************
  ...
*************************************************************************/
static void draw_extra_background(struct sw_widget *widget,
				  const struct ct_rect *region)
{
  struct ct_rect rect;
  struct sw_widget *list = widget->data.window.list;
  int row = list->data.list.selected_row;

  if (row == -1) {
    return;
  }

  ensure_valid_layout(list);

  rect.x = 0;
  rect.y = list->data.list.start_y[row];
  rect.width = widget->inner_bounds.width;
  rect.height = list->data.list.heights[row];
  ct_rect_intersect(&rect, region);
  be_draw_region(sw_widget_get_osda(widget), &rect,
		 be_get_color(189, 210, 238, MAX_OPACITY));
}

/*************************************************************************
  ...
*************************************************************************/
static void click_start(struct sw_widget *widget,
			const struct ct_point *mouse,
			enum be_mouse_button button, int state, void *data)
{
  int y, row = -1;
  struct ct_point pos = *mouse;

  pos.y += widget->data.list.offset.y;

  ensure_valid_layout(widget);

  for (y = 0; y < widget->data.list.rows; y++) {
    if (pos.y >= widget->data.list.start_y[y] &&
	pos.y <
	widget->data.list.start_y[y] + widget->data.list.heights[y]) {
      row = y;
      break;
    }
  }
  sw_list_set_selected_row(widget, row, FALSE);
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_list_create(struct sw_widget *parent, int pixel_width,
				 int pixel_height)
{
  struct sw_widget *result = create_widget(parent, WT_LIST);

  result->draw = draw;
  result->click_start = click_start;

  result->inner_bounds.width = pixel_width;
  result->inner_bounds.height = pixel_height;
  result->outer_bounds = result->inner_bounds;

  result->data.list.callback1 = NULL;
  result->data.list.callback2 = NULL;

  result->data.list.window =
      sw_window_create(NULL, 1, 1, NULL, FALSE, DEPTH_HIDDEN);
  sw_widget_set_background_color(result->data.list.window,
				 be_get_color(255, 255, 255, MAX_OPACITY));
  result->data.list.window->data.window.shown = FALSE;
  result->data.list.window->data.window.list = result;
  result->data.list.window->draw_extra_background = draw_extra_background;
  sw_widget_set_position(result->data.list.window, 0, 0);
  result->data.list.needs_layout = TRUE;
  result->data.list.offset.x = 0;
  result->data.list.offset.y = 0;
  result->data.list.columns = 0;
  result->data.list.rows = 0;
  result->data.list.items = NULL;
  result->data.list.column_alignments = NULL;
  result->data.list.row_enabled = NULL;

  result->data.list.widths = NULL;
  result->data.list.heights = NULL;
  result->data.list.start_x = NULL;
  result->data.list.start_y = NULL;

  result->data.list.selected_row = -1;

  sw_widget_set_background_color(result, 
                                 be_get_color(255, 255, 255, MAX_OPACITY));
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_set_item(struct sw_widget *widget, int column, int row,
		      struct sw_widget *item)
{
  ensure_size(widget, column, row);
  assert(widget && item);
  assert(column >= 0 && column < widget->data.list.columns);
  assert(row >= 0 && row < widget->data.list.rows);

  item->can_be_pressed = FALSE;
  item->can_be_selected = FALSE;
  item->can_be_dragged[BE_MB_LEFT] = FALSE;
  item->can_be_dragged[BE_MB_RIGHT] = FALSE;
  item->can_be_dragged[BE_MB_MIDDLE] = FALSE;

  sw_window_remove(item);
  sw_widget_set_position(item, 0, 0);
  widget->data.list.needs_layout = TRUE;
  widget_needs_paint(widget);

  *get_item(widget, column, row) = item;
  widget->data.list.selected_row = 0;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_get_offset(struct sw_widget *widget, struct ct_point *pos)
{
  *pos = widget->data.list.offset;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_set_offset(struct sw_widget *widget,
			const struct ct_point *position)
{
  struct sw_widget *window = widget->data.list.window;
  struct ct_point pos = *position;
  int diff;

  if (pos.x < 0) {
    pos.x = 0;
  }

  if (pos.y < 0) {
    pos.y = 0;
  }

  diff = window->outer_bounds.width - widget->inner_bounds.width;
  if (diff > 0 && pos.x > diff) {
    pos.x = diff;
  }

  diff = window->outer_bounds.height - widget->inner_bounds.height;
  if (diff > 0 && pos.y > diff) {
    pos.y = diff;
  }

  if (widget->data.list.offset.x != pos.x ||
      widget->data.list.offset.y != pos.y) {
    widget->data.list.offset = pos;
    widget_needs_paint(widget);

    if (widget->data.list.callback1) {
      widget->data.list.callback1(widget, widget->data.list.callback1_data);
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_get_view_size(struct sw_widget *widget, struct ct_size *size)
{
  size->height = widget->inner_bounds.height;
  size->width = widget->inner_bounds.width;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_get_window_size(struct sw_widget *widget,struct ct_size *size)
{
  struct sw_widget *window = widget->data.list.window;

  size->height = window->outer_bounds.height;
  size->width = window->outer_bounds.width;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_set_content_changed_notify(struct sw_widget *widget,
					void (*callback) (struct sw_widget
							  * widget,
							  void *data),
					void *data)
{
  widget->data.list.callback1 = callback;
  widget->data.list.callback1_data = data;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_set_selected_row(struct sw_widget *widget, int row, bool show)
{
  ensure_size(widget, -1, row);
  widget->data.list.selected_row = row;
  widget_needs_paint(widget);
  widget_needs_paint(widget->data.list.window);
  update_window(widget->data.list.window);
  if (show) {
    struct ct_point pos = widget->data.list.offset;

    pos.y = widget->data.list.start_y[row];
    sw_list_set_offset(widget, &pos);
  }
  if (widget->data.list.callback2) {
    widget->data.list.callback2(widget, widget->data.list.callback2_data);
  }
}

/*************************************************************************
  ...
*************************************************************************/
int sw_list_get_selected_row(struct sw_widget *widget)
{
  return widget->data.list.selected_row;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_set_row_enabled(struct sw_widget *widget, int row, bool enabled)
{
  ensure_size(widget, -1, row);
  widget->data.list.row_enabled[row] = enabled;
}

/*************************************************************************
  ...
*************************************************************************/
bool sw_list_is_row_enabled(struct sw_widget *widget, int row)
{
  return widget->data.list.row_enabled[row];
}

/*************************************************************************
  ...
*************************************************************************/
#if 0
static void vslider_changed(struct sw_widget *widget, void *data)
{
  sw_update_list_from_vslider(data, widget);
}

/*************************************************************************
  ...
*************************************************************************/
static void list_changed(struct sw_widget *widget, void *data)
{
    //printf("WIDGET_LIST\n");
  sw_update_vslider_from_list(data, widget);
}

/*************************************************************************
  ...
*************************************************************************/
static void vscroll_button_up_callback(struct sw_widget *button, void *data)
{
  struct sw_widget *vslider = data;
  float old = sw_slider_get_offset(vslider);
  float width = sw_slider_get_width(vslider);

  sw_slider_set_offset(vslider, old - 0.9 * width);
}

/*************************************************************************
  ...
*************************************************************************/
static void vscroll_button_down_callback(struct sw_widget *button, void *data)
{
  struct sw_widget *vslider = data;
  float old = sw_slider_get_offset(vslider);
  float width = sw_slider_get_width(vslider);

  sw_slider_set_offset(vslider, old + 0.9 * width);
}
#endif

/*************************************************************************
  ...
*************************************************************************/
void sw_list_add_buttons_and_vslider(struct sw_widget *widget,
				     struct sprite *up, struct sprite *down,
				     struct sprite *button_background,
				     struct sprite *scrollbar)
{
    assert(0);
    /*
  struct sw_widget *window = widget->parent;
  struct sw_widget *button_up, *button_down, *vslider;
  struct ct_rect rect_list, rect_up, rect_down;

  sw_widget_get_bounds(widget, &rect_list);

  button_up = sw_button_create_text_and_background(window, NULL, up);
  sw_widget_set_position(button_up, rect_list.x + rect_list.width,
			 rect_list.y);
  sw_widget_get_bounds(button_up, &rect_up);

  button_down = sw_button_create_text_and_background(window, NULL, down);

  sw_widget_set_position(button_down, 0, 0);
  sw_widget_get_bounds(button_down, &rect_down);
  sw_widget_set_position(button_down, rect_list.x + rect_list.width,
			 rect_list.y + rect_list.height - rect_down.height);
  sw_widget_get_bounds(button_down, &rect_down);

  vslider =
      sw_slider_create(window, rect_up.width - 2,
		       rect_down.y - (rect_up.y + rect_up.height) - 2,
		       scrollbar, TRUE);
  sw_widget_set_position(vslider, rect_up.x - 1, rect_up.y + rect_up.height);
  sw_widget_set_background_color(vslider, COLOR_STD_RACE2);
  sw_slider_set_slided_notify(vslider, vslider_changed, widget);

  sw_button_set_callback(button_up, vscroll_button_up_callback, vslider);
  sw_button_set_callback(button_down, vscroll_button_down_callback, vslider);
  sw_list_set_content_changed_notify(widget, list_changed, vslider);
    */
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_set_selection_changed_notify(struct sw_widget *widget,
					  void (*callback) (struct sw_widget
							    * widget,
							    void *data),
					  void *data)
{
  widget->data.list.callback2 = callback;
  widget->data.list.callback2_data = data;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_list_clear(struct sw_widget *widget)
{
  int x, y;

  if (widget->data.list.columns == 0 && widget->data.list.rows == 0) {
    return;
  }
  
  for (x = 0; x < widget->data.list.columns; x++) {
    for (y = 0; y < widget->data.list.rows; y++) {
      struct sw_widget *w = *get_item(widget, x, y);

      if (w) {
	real_widget_destroy(w);
      }
    }
  }
  
  widget->data.list.columns = 0;
  widget->data.list.rows = 0;
  free(widget->data.list.items);
  widget->data.list.items = NULL;
  free(widget->data.list.column_alignments);
  widget->data.list.column_alignments = NULL;
  free(widget->data.list.row_enabled);
  widget->data.list.row_enabled = NULL;

  widget->data.list.needs_layout = TRUE;  
}
