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

#include "fcintl.h"
#include "log.h"
#include "mem.h"

/* REMOVE ME */
#include "timing.h"

#define FLUSH_RECTS TRUE       /* flush dirty rectangles or whole screen */

#define DEBUG_UPDATES  0
#define DUMP_UPDATES   0
#define DUMP_WINDOWS   0
#define DEBUG_PAINT_ALL	FALSE

struct sw_widget *root_window;
static struct widget_list *windows_back_to_front;
static struct widget_list *windows_front_to_back;
struct widget_list *deferred_destroyed_widgets;
static bool dump_screen = FALSE;

#define TITLE_PADDING 3

/*************************************************************************
  ...
*************************************************************************/
static void draw_extra_background(struct sw_widget *widget,
				  const struct ct_rect *region)
{
  if (widget->data.window.canvas_background) {
   struct ct_size size = { region->width,
			    region->height };
    struct ct_point pos = { region->x, region->y };

    be_copy_osda_to_osda(sw_widget_get_osda(widget),
			 widget->data.window.canvas_background,
			 &size, &pos, &pos);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void draw(struct sw_widget *widget)
{
  if (widget->data.window.title) {
    struct ct_point pos;
    struct ct_rect rect = widget->inner_bounds;

    rect.height = widget->data.window.title->size.height + 2 * TITLE_PADDING;

    be_draw_region(widget->data.window.target, &rect,
		   widget->data.window.title->background);
    be_draw_rectangle(sw_widget_get_osda(widget), &rect, 1,
		      widget->data.window.title->foreground);
    pos.x = widget->inner_bounds.x + TITLE_PADDING;
    pos.y = widget->inner_bounds.y + TITLE_PADDING;
    be_draw_string(sw_widget_get_osda(widget), &pos,
		   widget->data.window.title);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void drag_start(struct sw_widget *widget,
		       const struct ct_point *mouse,
		       enum be_mouse_button button)
{
  if (button == BE_MB_LEFT) {
    widget->data.window.pos_at_drag_start.x = widget->outer_bounds.x;
    widget->data.window.pos_at_drag_start.y = widget->outer_bounds.y;

    sw_widget_set_border_color(widget, 
                               be_get_color(255, 255, 0, MAX_OPACITY));
  } else if (widget->data.window.user_drag_start) {
    widget->data.window.user_drag_start(widget, mouse, button);
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
  if (button == BE_MB_LEFT) {
    int dx = current_position->x - start_position->x;
    int dy = current_position->y - start_position->y;
    struct ct_rect new = widget->outer_bounds;

    new.x = widget->data.window.pos_at_drag_start.x + dx;
    new.y = widget->data.window.pos_at_drag_start.y + dy;

    if (ct_rect_valid(&new)
	&& ct_rect_in_rect(&new, &root_window->data.window.children_bounds)) {
      sw_widget_set_position(widget, new.x, new.y);
    }
  } else if (widget->data.window.user_drag_move) {
    widget->data.window.user_drag_move(widget, start_position,
				       current_position, button);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void drag_end(struct sw_widget *widget, enum be_mouse_button button)
{
  if (button == BE_MB_LEFT) {
    sw_widget_set_border_color(widget, 
                               be_get_color(255, 255, 255, MAX_OPACITY));
  } else if (widget->data.window.user_drag_end) {
    widget->data.window.user_drag_end(widget, button);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void destroy(struct sw_widget *widget)
{
  widget_list_iterate(widget->data.window.children, pwidget) {
    real_widget_destroy(pwidget);
  } widget_list_iterate_end;


  be_free_osda(widget->data.window.target);
  if (widget->data.window.title) {
    ct_string_destroy(widget->data.window.title);
  }

  widget_list_unlink(windows_front_to_back, widget);
  widget_list_unlink(windows_back_to_front, widget);
}

/*************************************************************************
  ...
*************************************************************************/
static int my_sort_back_to_front(const void *p1, const void *p2)
{
  const struct sw_widget *w1 =
      (const struct sw_widget *) *(const void **) p1;
  const struct sw_widget *w2 =
      (const struct sw_widget *) *(const void **) p2;

  return w1->data.window.depth - w2->data.window.depth;
}

/*************************************************************************
  ...
*************************************************************************/
static int my_sort_front_to_back(const void *p1, const void *p2)
{
  const struct sw_widget *w1 =
      (const struct sw_widget *) *(const void **) p1;
  const struct sw_widget *w2 =
      (const struct sw_widget *) *(const void **) p2;

  return w2->data.window.depth - w1->data.window.depth;
}

/*************************************************************************
  ...
*************************************************************************/
static void sort(void)
{
  widget_list_sort(windows_front_to_back, my_sort_front_to_back);
  widget_list_sort(windows_back_to_front, my_sort_back_to_front);
}

static void sw_window_set_depth(struct sw_widget *widget, int depth)
{
  widget->data.window.depth = depth;
  sort();
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_window_create(struct sw_widget *parent, int width,
				   int height, struct ct_string *title,
				   bool has_border, int depth)
{
  struct sw_widget *result = create_widget(parent, WT_WINDOW);
  int border_width = has_border ? BORDER_WIDTH : 0;

  if (width == 0 && height == 0) {
    struct ct_size size;

    be_screen_get_size(&size);
    width = size.width;
    height = size.height;
  }

  if (depth == 0) {
    depth = parent ? parent->data.window.depth + 1 : DEPTH_MIN;
  }
  assert(depth == DEPTH_HIDDEN
	 || (depth >= DEPTH_MIN && depth <= DEPTH_MAX));

  result->destroy = destroy;
  result->draw = draw;
  result->draw_extra_background = draw_extra_background;
  result->drag_start = drag_start;
  result->drag_move = drag_move;
  result->drag_end = drag_end;

  result->can_be_dragged[BE_MB_LEFT] = TRUE;

  result->data.window.children = widget_list_new();
  widget_list_append(windows_front_to_back, result);
  widget_list_append(windows_back_to_front, result);

  result->data.window.to_flush = region_list_new();

  result->inner_bounds.x = border_width;
  result->inner_bounds.y = border_width;
  result->inner_bounds.width = width;
  result->inner_bounds.height = height;

  result->outer_bounds.width = 2 * border_width + width;
  result->outer_bounds.height = 2 * border_width + height;

  result->data.window.target =
      be_create_osda(2 * border_width + width, 2 * border_width + height);
  result->data.window.title = title;
  result->data.window.shown = TRUE;
  result->data.window.list = NULL;

  result->data.window.canvas_background = NULL;

  result->data.window.inner_deco_height = 0;
  if (title) {
    result->data.window.inner_deco_height =
	result->data.window.title->size.height + 2 * TITLE_PADDING;
  }

  if (has_border) {
    sw_widget_set_border_color(result, 
                               be_get_color(255, 255, 255, MAX_OPACITY));
  }

  sw_window_set_depth(result, depth);

  return result;
}

static struct osda *whole_osda = NULL;

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_create_root_window(void)
{
  struct ct_size size;
  struct sw_widget *result;

  assert(!root_window);
  windows_front_to_back = widget_list_new();
  windows_back_to_front = widget_list_new();
  deferred_destroyed_widgets = widget_list_new();

  be_screen_get_size(&size);

  whole_osda = be_create_osda(size.width, size.height);

  result =
      sw_window_create(NULL, size.width, size.height, NULL, FALSE,
		       DEPTH_MIN + 1);
  sw_widget_set_position(result, 0, 0);
  sw_widget_set_background_color(result, 
                                 be_get_color(22, 44, 88, MAX_OPACITY));
  sw_window_set_draggable(result, FALSE);

  root_window = result;
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_add(struct sw_widget *window, struct sw_widget *widget)
{
  assert(window && widget);
  assert(!widget->parent);

  widget_list_iterate(window->data.window.children, pwidget) {
    assert(pwidget != widget);
  } widget_list_iterate_end;

  widget_list_prepend(window->data.window.children, widget);
  widget->parent = window;
}

/*************************************************************************
  ...
*************************************************************************/
void remove_all_from_window(struct sw_widget *widget)
{
  while (widget_list_size(widget->data.window.children) > 0) {
    sw_window_remove(widget_list_get(widget->data.window.children, 0));
  }
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_remove(struct sw_widget *widget)
{
  struct sw_widget *old_parent = widget->parent;
  int found = 0;

  assert(old_parent);

  widget_list_iterate(old_parent->data.window.children, pwidget) {
    if (pwidget == widget) {
      found++;
    }
  } widget_list_iterate_end;

  assert(found == 1);
  widget_list_unlink(old_parent->data.window.children, widget);
  widget->parent = NULL;
}

/*************************************************************************
  ...
*************************************************************************/
static void flush_one_window(struct sw_widget *widget,
			     const struct ct_rect *screen_rect)
{
  struct ct_size size;
  struct ct_point src_pos, dest_pos;

  if (!widget->data.window.shown) {
    return;
  }

  size.width = screen_rect->width;
  size.height = screen_rect->height;
  dest_pos.x = screen_rect->x;
  dest_pos.y = screen_rect->y;
  src_pos.x = screen_rect->x - widget->outer_bounds.x;
  src_pos.y = screen_rect->y - widget->outer_bounds.y;

  be_copy_osda_to_osda(whole_osda, widget->data.window.target, &size,
		       &dest_pos, &src_pos);
}

/*************************************************************************
  ...
*************************************************************************/
void flush_rect_to_screen(const struct ct_rect *rect)
{
  be_copy_osda_to_screen(whole_osda, rect);  
}

/*************************************************************************
  ...
*************************************************************************/
void flush_all_to_screen(void)
{
  be_copy_osda_to_screen(whole_osda, NULL);

  if (dump_screen) {
    static int counter = 0;
    char b[100];

    my_snprintf(b, sizeof(b), "fc_%05d.ppm", counter++);
    freelog(LOG_NORMAL, _("Making screenshot %s"), b);

    be_write_osda_to_file(whole_osda, b);
  }
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_resize(struct sw_widget *widget, int width, int height)
{
  be_free_osda(widget->data.window.target);
  widget->data.window.target = be_create_osda(width, height);
  widget->inner_bounds.width = width;
  widget->inner_bounds.height = height;
  inner_size_changed(widget);
  parent_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
static void draw_background_region(struct sw_widget *widget,
				   const struct ct_rect * rect)
{
  if (widget->draw_extra_background) {
    widget->draw_extra_background(widget, rect);
  }
  if (widget->background_sprite) {
    struct ct_point pos;
    struct ct_size size;

    pos.x = rect->x;
    pos.y = rect->y;

    size.width = rect->width;
    size.height = rect->height;

    be_draw_sprite(sw_widget_get_osda(widget), widget->background_sprite,
		   &size, &pos, &pos);
  } else if (widget->has_background_color) {
    be_draw_region(sw_widget_get_osda(widget), rect,
		   widget->background_color);
  } else {
    if (widget->parent && widget->type != WT_WINDOW) {
      draw_background_region(widget->parent, rect);
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void draw_background(struct sw_widget *widget)
{
  draw_background_region(widget, &widget->inner_bounds);

  if (widget->has_border_color) {
    struct ct_rect rect = widget->outer_bounds;

    if (widget->type == WT_WINDOW) {
      rect.x = 0;
      rect.y = 0;
    }

    be_draw_rectangle(sw_widget_get_osda(widget), &rect, BORDER_WIDTH,
		      widget->border_color);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void add_flush_region(struct sw_widget *widget, const struct ct_rect
			     *region)
{
  struct sw_widget *window;

  if (widget->type == WT_WINDOW) {
    window = widget;
  } else {
    window = widget->parent;
  }

  region_list_iterate(window->data.window.to_flush, pregion) {
    if (ct_rect_in_rect(region, pregion)) {
      return;
    }
  } region_list_iterate_end;
  
  region_list_prepend(window->data.window.to_flush, ct_rect_clone(region));
}

/*************************************************************************
  ...
*************************************************************************/
static void draw_tooltip(struct sw_widget *widget)
{
  struct ct_point pos;
  struct ct_rect rect;
  const int PADDING = 5;
  struct osda *osda;
  int i, extra = 2 * PADDING + widget->tooltip->shadow;

  rect.width = widget->tooltip->text->size.width + extra;
  rect.height = widget->tooltip->text->size.height + extra;

  rect.x =
      MAX(0,
	  widget->outer_bounds.x + (widget->outer_bounds.width -
				    rect.width) / 2);
  rect.y = MAX(0,widget->outer_bounds.y - rect.height - PADDING);

  pos.x = rect.x + PADDING;
  pos.y = rect.y + PADDING;

  if (widget->type == WT_WINDOW) {
    osda = sw_widget_get_osda(widget->parent);
  } else {
    osda = sw_widget_get_osda(widget);
  }

  for (i = 1; i < widget->tooltip->shadow; i++) {
    struct ct_rect rect2 = rect;

    rect2.x += i;
    rect2.y += i;
    be_draw_region(osda, &rect2, widget->tooltip->shadow_color);
  }

  be_draw_region(osda, &rect, widget->tooltip->text->background);
  be_draw_string(osda, &pos, widget->tooltip->text);
  be_draw_rectangle(osda, &rect, 1, widget->tooltip->text->foreground);
}

/*************************************************************************
  ...
*************************************************************************/
void untooltip(struct sw_widget *widget)
{
  struct ct_point pos;
  struct ct_rect rect;
  const int PADDING = 5;
  struct osda *osda;
  int i, extra;

  if (widget->tooltip && widget->tooltip_callback_id != 0) {
    sw_remove_timeout(widget->tooltip_callback_id);
    widget->tooltip_callback_id = 0;
  }

  if (!widget->tooltip || (widget->tooltip && !widget->tooltip_shown)) {
    return;
  }
  widget->tooltip_shown = FALSE;
  
  extra = 2 * PADDING + widget->tooltip->shadow;
  
  rect.width = widget->tooltip->text->size.width + extra;
  rect.height = widget->tooltip->text->size.height + extra;

  rect.x =
      MAX(0,
	  widget->outer_bounds.x + (widget->outer_bounds.width -
				    rect.width) / 2);
  rect.y = MAX(0,widget->outer_bounds.y - rect.height - PADDING);

  pos.x = rect.x + PADDING;
  pos.y = rect.y + PADDING;

  if (widget->type == WT_WINDOW) {
    osda = sw_widget_get_osda(widget->parent);
  } else {
    osda = sw_widget_get_osda(widget);
  }

  for (i = 1; i < widget->tooltip->shadow; i++) {
    struct ct_rect rect2 = rect;

    rect2.x += i;
    rect2.y += i;
    be_draw_region(osda, &rect2, be_get_color(0, 0, 0, MIN_OPACITY));
  }

  be_draw_region(osda, &rect, be_get_color(0, 0, 0, MIN_OPACITY));
  be_draw_rectangle(osda, &rect, 1, be_get_color(0, 0, 0, MIN_OPACITY));  
  
  parent_needs_paint(widget);
}

/*************************************************************************
  ...
*************************************************************************/
static void draw_one(struct sw_widget *widget)
{
  draw_background(widget);
  assert(widget->draw);
  widget->draw(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void update_window(struct sw_widget *widget)
{
  assert(widget->type == WT_WINDOW);
  if (widget->needs_repaint) {
    draw_one(widget);

    widget_list_iterate(widget->data.window.children, pwidget) {
      if (pwidget->type != WT_WINDOW) {
	draw_one(pwidget);
      }
    }
    widget_list_iterate_end;

    widget_list_iterate(widget->data.window.children, pwidget) {
      if (pwidget->tooltip && pwidget->tooltip_shown) {
	draw_tooltip(pwidget);
      }
    } widget_list_iterate_end;
    if (widget->tooltip && widget->tooltip_shown) {
      draw_tooltip(widget);
    }

    widget->needs_repaint = FALSE;

    {
      struct ct_rect tmp =
	  { 0, 0, widget->outer_bounds.width, widget->outer_bounds.height };
      add_flush_region(widget, &tmp);
    }
  } else {
    widget_list_iterate(widget->data.window.children, pwidget) {
      if (pwidget->type != WT_WINDOW && pwidget->needs_repaint) {
	draw_one(pwidget);
	pwidget->needs_repaint = FALSE;
	add_flush_region(widget, &pwidget->outer_bounds);
      }
    }
    widget_list_iterate_end;
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void merge_regions(struct region_list *list)
{
  struct region_list *tmp, *orig, *copy;

  tmp = region_list_new();

  orig = list;
  copy = tmp;

  region_list_iterate(orig, region) {
    if (ct_rect_in_rect_list(region, copy)) {
      free(region);
    } else {
      region_list_prepend(copy, region);
    }
  } region_list_iterate_end;

  orig = tmp;
  copy = list;
  region_list_clear(copy);

  region_list_iterate(orig, region) {
    if (ct_rect_in_rect_list(region, copy)) {
      free(region);
    } else {
      region_list_prepend(copy, region);
    }
  } region_list_iterate_end;

  region_list_free(tmp);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_paint_all(void)
{
  int regions=0;
  struct timer *timer1 = new_timer(TIMER_USER, TIMER_ACTIVE);
  struct timer *timer2 = new_timer(TIMER_USER, TIMER_ACTIVE);
  struct timer *timer3 = new_timer(TIMER_USER, TIMER_ACTIVE);
  struct timer *timer4 = new_timer(TIMER_USER, TIMER_ACTIVE);
  struct region_list *normalized_regions;
  static int call = -1;
#if DUMP_UPDATES
  char filename[100];
  int region_nr;
#endif

  call++;

  handle_destroyed_widgets();

  normalized_regions = region_list_new();

  start_timer(timer1);
  widget_list_iterate(windows_back_to_front, widget) {
    update_window(widget);
    regions += region_list_size(widget->data.window.to_flush);
  } widget_list_iterate_end;
  stop_timer(timer1);

  if (regions == 0) {
    return;
  }

  if (DEBUG_PAINT_ALL) {
    printf("updated windows; %d regions have to be flushed\n", regions);
  }

  if (DUMP_WINDOWS) {
    widget_list_iterate(windows_back_to_front, widget) {
      char name[100];

      if (widget->data.window.depth > -1) {
	sprintf(name, "window-c%03d-%p.ppm", call, widget);
	be_write_osda_to_file(widget->data.window.target, name);
      }
    } widget_list_iterate_end;
  }

  if (DEBUG_PAINT_ALL) {
    start_timer(timer2);
  }
  widget_list_iterate(windows_back_to_front, widget) {
#if 0
    if (DEBUG_PAINT_ALL) {
      region_list_iterate(widget->data.window.to_flush, region) {
	printf("  region=%s\n", ct_rect_to_string(region));
      } region_list_iterate_end;
    }
#endif

    region_list_iterate(widget->data.window.to_flush, region) {
      region->x += widget->outer_bounds.x;
      region->y += widget->outer_bounds.y;
      region_list_unlink(widget->data.window.to_flush, region);
      region_list_prepend(normalized_regions, region);
    } region_list_iterate_end;
  } widget_list_iterate_end;
  if (DEBUG_PAINT_ALL) {
    stop_timer(timer2);
  }

#if 0
  if (DEBUG_PAINT_ALL) {
    printf("  normalized_regions\n");
    region_list_iterate(normalized_regions, region) {
      printf("    region=%s\n", ct_rect_to_string(region));
    } region_list_iterate_end;
  }
#endif

  merge_regions(normalized_regions);

#if 0
  if (DEBUG_PAINT_ALL) {
    printf("  merged normalized_regions\n");
    region_list_iterate(normalized_regions, region) {
      printf("    region=%s\n", ct_rect_to_string(region));
    } region_list_iterate_end;
  }
#endif

  if(DEBUG_PAINT_ALL) {
    printf("starting flushing of %d regions\n",
           region_list_size(normalized_regions));
  }

#if DUMP_UPDATES
  sprintf(filename,"whole-c%03d-r000-before.ppm",call);
  be_write_osda_to_file(whole_osda,filename);
  region_nr = 1;
#endif

  if (DEBUG_PAINT_ALL) {
    start_timer(timer3);
  }
  region_list_iterate(normalized_regions, region) {
    int window_nr = 0;

    if (DEBUG_UPDATES)
      printf("region = %s\n", ct_rect_to_string(region));

    widget_list_iterate(windows_back_to_front, widget) {
      struct ct_rect rect = *region;

      ct_rect_intersect(&rect, &widget->outer_bounds);
      if (DEBUG_UPDATES)
	printf("  window=%p %s depth=%d\n", widget,
	       ct_rect_to_string(&widget->outer_bounds),
	       widget->data.window.depth);
      if (!ct_rect_empty(&rect)) {
	if (DEBUG_UPDATES)
	  printf("    window intersects with dirty region = %s\n",
		 ct_rect_to_string(&rect));
#if DUMP_UPDATES
	sprintf(filename, "whole-c%03d-r%03d-w%03d-0-before.ppm", call,
		region_nr, window_nr);
	be_write_osda_to_file(whole_osda,filename);
#endif
	flush_one_window(widget, &rect);
#if DUMP_UPDATES
	sprintf(filename, "whole-c%03d-r%03d-w%03d-1-after.ppm", call,
		region_nr, window_nr);
	be_write_osda_to_file(whole_osda,filename);
#endif
      } else {
	if (DEBUG_UPDATES) {
	  printf("    disjunkt\n");
        }
      }
      window_nr++;
    } widget_list_iterate_end;
    
#if FLUSH_RECTS
    flush_rect_to_screen(region);
#endif
    
    region_list_unlink(normalized_regions, region);
    free(region);
#if DUMP_UPDATES
    region_nr++;
#endif
  } region_list_iterate_end;
  if (DEBUG_PAINT_ALL) {
    stop_timer(timer3);
  }
#if DUMP_UPDATES
  sprintf(filename,"whole-c%03d-r999-after.ppm",call);
  be_write_osda_to_file(whole_osda,filename);
#endif

#if !FLUSH_RECTS
  if (DEBUG_PAINT_ALL) {
    start_timer(timer4);
  }
  flush_all_to_screen();
  if (DEBUG_PAINT_ALL) {
    stop_timer(timer4);
  }
#endif
  
  if (DEBUG_PAINT_ALL) {
    printf("PAINT-ALL: update=%fs normalize=%fs flushs=%fs flush-all=%fs\n",
           read_timer_seconds(timer1), read_timer_seconds(timer2),
	   read_timer_seconds(timer3), read_timer_seconds(timer4));
  }
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *search_widget(const struct ct_point *pos,
				enum event_type event_type)
{
  struct ct_point tmp;
  struct sw_widget *window = NULL;

  if (!ct_point_valid(pos)) {
    return NULL;
  }

  if (0) {
    widget_list_iterate(windows_front_to_back, pwidget) {
      printf("%p: shown=%d accepts=%d inside=%d depth=%d size=%dx%d\n",
	     pwidget, pwidget->data.window.shown,
	     pwidget->accepts_events[event_type], ct_point_in_rect(pos,
								   &pwidget->
								   outer_bounds),
	     pwidget->data.window.depth, pwidget->outer_bounds.width,
	     pwidget->outer_bounds.height);
    } widget_list_iterate_end;
  }

  widget_list_iterate(windows_front_to_back, pwidget) {
    if (pwidget->data.window.shown && pwidget->accepts_events[event_type]
	&& ct_point_in_rect(pos, &pwidget->outer_bounds)) {
      bool is_transparent;
      tmp = *pos;
      tmp.x -= pwidget->outer_bounds.x;
      tmp.y -= pwidget->outer_bounds.y;

      is_transparent = 
	  be_is_transparent_pixel(pwidget->data.window.target, &tmp);
      if (!is_transparent) {
	window = pwidget;
	break;
      }
    }
  } widget_list_iterate_end;

  if (!window) {
    return NULL;
  }
  assert(ct_point_valid(&tmp));

  widget_list_iterate(window->data.window.children, pwidget) {
    if (pwidget->type != WT_WINDOW && pwidget->accepts_events[event_type]
	&& ct_point_in_rect(&tmp, &pwidget->outer_bounds)) {
	assert(pwidget->type<=WT_LAST);
      return pwidget;
    }
  } widget_list_iterate_end;

  assert(window->type<=WT_LAST);
  return window;
}

/*************************************************************************
  ...
*************************************************************************/
struct sw_widget *sw_window_create_by_clone(struct sw_widget *widget,
					    int depth)
{
  struct sw_widget *result =
      sw_window_create(widget, widget->outer_bounds.width,
		       widget->outer_bounds.height, NULL, FALSE, depth);

  result->can_be_dragged[BE_MB_LEFT] = widget->can_be_dragged;
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_set_canvas_background(struct sw_widget *widget, bool yes)
{
  if (widget->data.window.canvas_background) {
    be_free_osda(widget->data.window.canvas_background);
    widget->data.window.canvas_background = NULL;
  }
  if (yes) {
    widget->data.window.canvas_background =
	be_create_osda(widget->outer_bounds.width,
		       widget->outer_bounds.height);
  }
}

/*************************************************************************
  ...
*************************************************************************/
struct osda *sw_window_get_canvas_background(struct sw_widget *widget)
{
    return widget->data.window.canvas_background;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_set_mouse_press_notify(struct sw_widget *widget,
				      void (*callback) (struct sw_widget *
							widget,
							const struct ct_point
							* pos, enum be_mouse_button button,
							int state,
							void *data),
				      void *data)
{
  widget->click_start = callback;
  widget->click_start_data = data;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_canvas_background_region_needs_repaint(
                  struct sw_widget *widget, const struct ct_rect *region)
{
  struct ct_rect region2;
  struct ct_size size;
  struct ct_point pos;

  if (region) {
    region2 = *region;
  } else {
    sw_widget_get_bounds(widget, &region2);
  }

  size = (struct ct_size){ region2.width, region2.height };
  pos = (struct ct_point){ region2.x, region2.y };
  
  be_copy_osda_to_osda(sw_widget_get_osda(widget),
		       widget->data.window.canvas_background,
		       &size, &pos, &pos);

  add_flush_region(widget, &region2);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_set_dump_screen(bool v)
{
  dump_screen = v;
}

/*************************************************************************
  ...
*************************************************************************/
bool sw_get_dump_screen(void)
{
  return dump_screen;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_set_draggable(struct sw_widget *widget, bool draggable)
{
  widget->can_be_dragged[BE_MB_LEFT] = draggable;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_set_key_notify(struct sw_widget *widget,
			      bool(*callback) (struct sw_widget *
					       widget,
					       const struct be_key * key,
					       void *data), void *data)
{
  widget->key = callback;
  widget->key_data = data;
}

/*************************************************************************
  ...
*************************************************************************/
bool deliver_key(const struct be_key *key)
{
  widget_list_iterate(windows_front_to_back, pwidget) {
    if (!pwidget->data.window.shown || !pwidget->accepts_events[EV_KEYBOARD]) {
      continue;
    }
    if (pwidget->key && pwidget->key(pwidget, key,pwidget->key_data)) {
      return TRUE;
    }

    widget_list_iterate(pwidget->data.window.children, pwidget2) {
      if (pwidget2->key && pwidget2->key(pwidget2, key, pwidget2->key_data)) {
	return TRUE;
      }
    } widget_list_iterate_end;
  } widget_list_iterate_end;

  return FALSE;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_window_set_user_drag(struct sw_widget * widget,void (*drag_start)
			      (struct sw_widget * widget,
			       const struct ct_point * mouse,
			       enum be_mouse_button button),
			     void (*drag_move) (struct sw_widget * widget,
						const struct ct_point *
						start_position,
						const struct ct_point *
						current_position,
						enum be_mouse_button button),
			     void (*drag_end) (struct sw_widget * widget,
					       enum be_mouse_button button))
{
  widget->data.window.user_drag_start = drag_start;
  widget->data.window.user_drag_move = drag_move;
  widget->data.window.user_drag_end = drag_end;
  widget->can_be_dragged[BE_MB_RIGHT] = TRUE;
}
