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

#include "log.h"
#include "mem.h"
#include "text_renderer.h"

/*************************************************************************
  ...
*************************************************************************/
enum widget_face get_widget_face(struct sw_widget *widget)
{
  if (widget->disabled) {
    return WF_DISABLED;
  }
  if (widget->pressed) {
    return WF_PRESSED;
  }
  if (widget->selected) {
    return WF_SELECTED;
  }
  return WF_NORMAL;
}

/*************************************************************************
  ...
*************************************************************************/
static void popup_tooltip(void *data)
{
  struct sw_widget *widget = data;

  widget->tooltip_shown = TRUE;

  widget->tooltip_callback_id = 0;

  parent_needs_paint(widget);
}

static struct sw_widget *dragged_widget = NULL;
static enum be_mouse_button drag_button;

/*************************************************************************
  ...
*************************************************************************/
static void mouse_released(struct sw_widget *widget,
			   const struct ct_point *position,
			   bool released_inside)
{
  if (widget->can_be_pressed && widget->pressed) {
    widget->pressed = FALSE;

    if (released_inside) {
      if (widget->click) {
	widget->click(widget);
      } else {
	printf("WARNING: no callback set %p\n", widget);
      }
    }
  }

  if (widget->dragged) {
    widget->dragged = FALSE;
    widget->drag_end(widget, drag_button);
    assert(dragged_widget == widget);
    dragged_widget = NULL;
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void mouse_leaves(struct sw_widget *widget,
			 const struct ct_point *position)
{
  untooltip(widget);

  mouse_released(widget, position, FALSE);

  if (widget->selected || widget->left) {
    widget_needs_paint(widget);
  }
  widget->selected = FALSE;
  if (widget->left) {
    widget->left(widget);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void mouse_enters(struct sw_widget *widget,
			 const struct ct_point *position)
{
  if (widget->can_be_selected) {
    widget->selected = TRUE;
    widget_needs_paint(widget);
  }
  if (widget->entered) {
    widget->entered(widget);
    widget_needs_paint(widget);
  }

  if (widget->tooltip) {
    assert(widget->tooltip_callback_id == 0);
    widget->tooltip_callback_id =
	sw_add_timeout(widget->tooltip->delay, popup_tooltip, widget);
  }
}

static struct ct_point drag_start_pos;

/*************************************************************************
  ...
*************************************************************************/
static void mouse_pressed(struct sw_widget *widget,
			  const struct ct_point *position,
			  enum be_mouse_button button, int state)
{
  if (widget->click_start) {
    struct ct_point pos = *position;

    pos.x -= widget->outer_bounds.x;
    pos.y -= widget->outer_bounds.y;

    pos.x -= widget->parent->outer_bounds.x;
    pos.y -= widget->parent->outer_bounds.y;

    widget->click_start(widget, &pos, button, state,
			widget->click_start_data);
  }

  if (widget->can_be_pressed) {
    widget->pressed = TRUE;
    widget_needs_paint(widget);
  }

  if (widget->can_be_dragged[button]) {
    drag_start_pos = *position;
    widget->dragged = TRUE;
    assert(widget->drag_start);
    widget->drag_start(widget, &drag_start_pos, button);
    dragged_widget = widget;
    drag_button = button;
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void mouse_moved(struct sw_widget *widget,
			const struct ct_point *position)
{
  if (widget && widget->dragged) {
    assert(widget == dragged_widget);
    assert(widget->drag_move);
    widget->drag_move(widget, &drag_start_pos, position, drag_button);
  }
}

static struct sw_widget *selected_widget = NULL;
static bool selected_widget_gets_keyboard = FALSE;

/*************************************************************************
  ...
*************************************************************************/
static void handle_mouse_motion(struct sw_widget *widget,
				const struct ct_point *position)
{
  selected_widget_gets_keyboard = FALSE;

  if (selected_widget == widget) {
    mouse_moved(widget, position);
    return;
  }

  if (selected_widget) {
    mouse_leaves(selected_widget, position);
  }
  if (widget) {
    mouse_enters(widget, position);
  }
  selected_widget = widget;
}

/*************************************************************************
  ...
*************************************************************************/
void sw_widget_select(struct sw_widget *widget)
{
  struct ct_point pos = { widget->outer_bounds.x, widget->outer_bounds.y };
  if (widget->type != WT_WINDOW) {
    assert(widget->parent);
    pos.x += widget->parent->outer_bounds.x;
    pos.y += widget->parent->outer_bounds.y;
  }
  handle_mouse_motion(widget, &pos);
  selected_widget_gets_keyboard = TRUE;
}

static struct sw_widget *pressed_widget = NULL;
static enum be_mouse_button pressed_button = 0;

/*************************************************************************
  ...
*************************************************************************/
static void handle_mouse_press(struct sw_widget *widget,
			       const struct ct_point *position,
			       enum be_mouse_button button, int state)
{
  if (pressed_widget == widget) {
    /* another mouse button pressed */
    return;
  }

  if (pressed_widget) {
    mouse_released(pressed_widget, position, FALSE);
  }
  if (widget) {
    mouse_pressed(widget, position, button, state);
  }
  pressed_widget = widget;
  pressed_button = button;
}

/*************************************************************************
  ...
*************************************************************************/
static void handle_mouse_release(struct sw_widget *widget,
				 const struct ct_point *position,
				 enum be_mouse_button button)
{
  if (pressed_widget == widget && pressed_button == button) {
    mouse_released(pressed_widget, position, TRUE);
  } else {
    mouse_released(pressed_widget, position, FALSE);
  }

  pressed_widget = NULL;
  pressed_button = 0;
}

/*************************************************************************
  ...
*************************************************************************/
void handle_destroyed_widgets(void)
{
  if (widget_list_size(deferred_destroyed_widgets) == 0) {
    return;
  }

  widget_list_iterate(deferred_destroyed_widgets, pwidget) {
    if ((pwidget->tooltip && pwidget->tooltip_shown)
	|| dragged_widget == pwidget || selected_widget == pwidget
	|| pressed_widget == pwidget) {
      struct ct_point pos = { 32000, 32000 };

      printf("  move mouse away tooltip=%d drag=%d select=%d press=%d\n",
	     (pwidget->tooltip
	      && pwidget->tooltip_shown), dragged_widget == pwidget,
	     selected_widget == pwidget, pressed_widget == pwidget);
      //be_write_osda_to_file(sw_widget_get_osda(pwidget), "destroyed_widget.pnm");

      handle_mouse_motion(NULL, &pos);
    }

    real_widget_destroy(pwidget);
    widget_list_unlink(deferred_destroyed_widgets, pwidget);
  } widget_list_iterate_end;
}

/*************************************************************************
  ...
*************************************************************************/
static void handle_event(const struct be_event *event,
			 void (*input_callback) (int socket))
{
  switch (event->type) {
  case BE_DATA_OTHER_FD:
    input_callback(event->socket);
    break;
  case BE_EXPOSE:
    flush_all_to_screen();
    break;
  case BE_TIMEOUT:
    handle_callbacks();
    break;

  case BE_MOUSE_MOTION:
    {
      struct sw_widget *widget = search_widget(&event->position, EV_MOUSE);

      handle_mouse_motion(widget, &event->position);
    }
    break;

  case BE_MOUSE_PRESSED:
    {
      struct sw_widget *widget = search_widget(&event->position, EV_MOUSE);

      handle_mouse_press(widget, &event->position, event->button,
			 event->state);
    }
    break;

  case BE_MOUSE_RELEASED:
    {
      struct sw_widget *widget = search_widget(&event->position, EV_MOUSE);

      handle_mouse_release(widget, &event->position, event->button);
    }
    break;

  case BE_KEY_PRESSED:
    {
      bool handled = FALSE;
      struct sw_widget *widget;

      assert(ct_key_is_valid(&event->key));

      if (selected_widget_gets_keyboard && selected_widget) {
	widget = selected_widget;
      } else {
	widget = search_widget(&event->position, EV_KEYBOARD);
      }
      if (widget && widget->key) {
	handled = widget->key(widget, &event->key, widget->key_data);
      }
      if (!handled) {
	handled = deliver_key(&event->key);
      }
      if (!handled) {
	printf("WARNING: unhandled key stroke\n");
      }
    }
    break;
  case BE_NO_EVENT:
    break;
  }
}

/*************************************************************************
  ...
*************************************************************************/
void sw_mainloop(void (*input_callback)(int socket))
{
  sw_paint_all();

  while (TRUE) {
    struct be_event event;
    struct timeval timeout;

    while (TRUE) {
      handle_callbacks();

      be_next_non_blocking_event(&event);
      if (event.type == BE_NO_EVENT) {
	break;
      }
      handle_event(&event, input_callback);
    }

    /* No events queued. We are idle. */
    handle_idle_callbacks();

    /* 
     * Since the next step may take some while, we update the screen
     * to make the user happy.
     */
    sw_paint_all();

    /* Wait for the server, the network or the timeout */
    get_select_timeout(&timeout);    
    be_next_blocking_event(&event, &timeout);

    if (event.type != BE_NO_EVENT) {
      handle_event(&event, input_callback);
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
void parent_needs_paint(struct sw_widget *widget)
{
  if (widget->parent) {
    widget->parent->needs_repaint = TRUE;
  } else {
    widget->needs_repaint = TRUE;
  }
}

/*************************************************************************
  ...
*************************************************************************/
void widget_needs_paint(struct sw_widget *widget)
{
  widget->needs_repaint = TRUE;
}

/*************************************************************************
  ...
*************************************************************************/
void real_widget_destroy(struct sw_widget *widget)
{
  assert(widget->type <= WT_LAST);
  if (widget->destroy) {
    widget->destroy(widget);
  } else {
    printf("WARNING: no destroy for type %d\n", widget->type);
  }
  if (widget->parent) {
    sw_window_remove(widget);
  }
  memset(widget, 0x54, sizeof(*widget));

  /* Ensure no dangling globals */
  if (selected_widget == widget) {
    selected_widget = NULL;
  } else if (pressed_widget == widget) {
    pressed_widget = NULL;
  } else if (dragged_widget == widget) {
    dragged_widget = NULL;
  }

  free(widget);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_update_vslider_from_list(struct sw_widget *slider,
				 struct sw_widget *list)
{
  struct ct_size view_size, window_size;
  struct ct_point position;

  sw_list_get_view_size(list, &view_size);
  sw_list_get_window_size(list, &window_size);
  sw_list_get_offset(list, &position);

  if(0)
  printf("LIST width=%d offset=%d size=%d\n", view_size.height, position.y,
	 window_size.height);
  sw_slider_set_width(slider, (float) view_size.height / window_size.height);
  sw_slider_set_offset(slider, (float) position.y / window_size.height);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_update_hslider_from_list(struct sw_widget *slider,
				 struct sw_widget *list)
{
  struct ct_size view_size, window_size;
  struct ct_point position;

  sw_list_get_view_size(list, &view_size);
  sw_list_get_window_size(list, &window_size);
  sw_list_get_offset(list, &position);

  sw_slider_set_width(slider, (float) view_size.width / window_size.width);
  sw_slider_set_offset(slider, (float) position.x / window_size.width);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_update_list_from_vslider(struct sw_widget *list,
				 struct sw_widget *slider)
{
  float offset = sw_slider_get_offset(slider);
  struct ct_size view_size, window_size;
  struct ct_point position;

  sw_list_get_view_size(list, &view_size);
  sw_list_get_window_size(list, &window_size);
  sw_list_get_offset(list, &position);
  position.y = offset * window_size.height;
  sw_list_set_offset(list, &position);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_update_list_from_hslider(struct sw_widget *list,
				 struct sw_widget *slider)
{
  float offset = sw_slider_get_offset(slider);
  struct ct_size view_size, window_size;
  struct ct_point position;

  sw_list_get_view_size(list, &view_size);
  sw_list_get_window_size(list, &window_size);
  sw_list_get_offset(list, &position);
  position.x = offset * window_size.width;
  sw_list_set_offset(list, &position);
}

/*************************************************************************
  ...
*************************************************************************/
void sw_init(void)
{
  tr_init();
}
