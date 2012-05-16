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

#ifndef FC_WIDGET_P_H
#define FC_WIDGET_P_H

#include "shared.h"
#include "widget.h"

#define BORDER_WIDTH	1

/* Widget Types */
enum widget_type {
  WT_WINDOW,
  WT_BUTTON,
  WT_EDIT,
  WT_LABEL,
  WT_LIST,
  WT_CLIST,
  WT_SLIDER,
  WT_LAST = WT_SLIDER
};

enum widget_face {
  WF_NORMAL,
  WF_SELECTED,
  WF_PRESSED,
  WF_DISABLED,
  WF_HIDDEN,
  NUM_WIDGET_FACES
};

enum event_type {
  EV_KEYBOARD,
  EV_MOUSE,
  EV_LAST
};

enum slider_part {
  SP_TOP,
  SP_BOTTOM,
  SP_REPEAT,
  SP_CENTER,
  NUM_SP
};

struct sw_widget {
  struct sw_widget *parent;
  enum widget_type type;
  bool pressed;
  bool selected;
  bool disabled;
  bool dragged;
  bool accepts_events[EV_LAST];

  bool can_be_pressed;
  bool can_be_selected;
  bool can_be_dragged[BE_MB_LAST];

  bool needs_repaint;

  struct ct_point pos;
  struct ct_rect inner_bounds, outer_bounds;

  struct ct_tooltip *tooltip;
  bool tooltip_shown;
  int tooltip_callback_id;

  bool has_background_color;
  be_color background_color;
  struct sprite *background_sprite;	/* default NULL */
  bool has_border_color;
  be_color border_color;

  void (*destroy) (struct sw_widget *widget);
  void (*entered) (struct sw_widget *widget);
  void (*left) (struct sw_widget *widget);
  void (*click) (struct sw_widget *widget);
  bool(*key) (struct sw_widget *widget, const struct be_key *key,
	      void *data);
  void *key_data;
  void (*draw) (struct sw_widget *widget);
  void (*draw_extra_background) (struct sw_widget *widget,
				 const struct ct_rect* rect);
  void (*drag_start) (struct sw_widget *widget,
		      const struct ct_point *mouse,
		      enum be_mouse_button button);
  void (*drag_move) (struct sw_widget *widget,
		     const struct ct_point *start_position,
		     const struct ct_point *current_position,
		     enum be_mouse_button button);
  void (*drag_end) (struct sw_widget *widget, enum be_mouse_button button);

  void (*click_start) (struct sw_widget *widget,
		       const struct ct_point *mouse,
		       enum be_mouse_button button, int state, void *data);
  void *click_start_data;

  union {
    struct {
      struct widget_list *children;
      struct osda *target;
      struct ct_string *title;
      struct ct_point pos_at_drag_start;
      int transparency;
      bool shown;
      struct ct_rect children_bounds;
      int inner_deco_height;
      int depth;
      struct sw_widget *list;
      struct osda *canvas_background;
      struct region_list *to_flush;
      void (*user_drag_start) (struct sw_widget * widget,
			       const struct ct_point * mouse,
			       enum be_mouse_button button);
      void (*user_drag_move) (struct sw_widget * widget,
			      const struct ct_point * start_position,
			      const struct ct_point * current_position,
			      enum be_mouse_button button);
      void (*user_drag_end) (struct sw_widget * widget,
			     enum be_mouse_button button);
    } window;
    struct {
      struct osda *gfx;
    } icon;
    struct {
      struct ct_string *text[NUM_WIDGET_FACES];
      struct ct_point text_offset[NUM_WIDGET_FACES];

      struct osda *foreground_faces[NUM_WIDGET_FACES];
      struct ct_point foreground_faces_offset[NUM_WIDGET_FACES];

      struct osda *background_faces[NUM_WIDGET_FACES];

      void (*callback) (struct sw_widget * widget, void *data);
      void *callback_data;
      struct be_key *shortcut;
    } button;
    struct {
      struct ct_string *template;
      char *buffer;
      int cursor;
      int max_size;
      be_color color1, color2, color_selected, color_noselected;
    } edit;
    struct {
      struct ct_string *text;
    } label;
    struct {
      struct ct_point offset;
      struct sw_widget *window;
      void (*callback1) (struct sw_widget *widget, void *data);
      void *callback1_data;
      void (*callback2) (struct sw_widget *widget, void *data);
      void *callback2_data;
      int columns, rows;
      struct sw_widget **items;
      bool needs_layout;
      int *widths, *heights;
      int *start_x, *start_y;
      int selected_row;
      enum ws_alignment *column_alignments;
      bool *row_enabled;
    } list;
    struct {
      float width, offset, pos_at_drag_start;
      bool vertical;
      void (*callback) (struct sw_widget * widget, void *data);
      void *callback_data;
      struct sprite *faces[NUM_WIDGET_FACES][NUM_SP];
      int cache_top_height, cache_center_height, cache_min_length;
      be_color color_active, color_drag, color_nodrag;
    } slider;
  } data;
};

extern struct sw_widget *root_window;
extern struct widget_list *deferred_destroyed_widgets;

void handle_callbacks(void);
void handle_idle_callbacks(void);
void get_select_timeout(struct timeval *timeout);

struct sw_widget *create_widget(struct sw_widget *parent,
				enum widget_type type);
enum widget_face get_widget_face(struct sw_widget *widget);
void translate_point(struct sw_widget *widget, struct ct_point *dest,
		     const struct ct_point *src);
void inner_size_changed(struct sw_widget *widget);
void flush_rect_to_screen(const struct ct_rect *rect);
void flush_all_to_screen(void);

void parent_needs_paint(struct sw_widget *widget);
void widget_needs_paint(struct sw_widget *widget);

void update_window(struct sw_widget *widget);
struct sw_widget *search_widget(const struct ct_point *pos,
				enum event_type event_type);
void real_widget_destroy(struct sw_widget *widget);
void remove_all_from_window(struct sw_widget *widget);

void align(const struct ct_rect *bb, struct ct_rect *item,
	   enum ws_alignment alignment);
void untooltip(struct sw_widget *widget);
bool deliver_key(const struct be_key *key);
void handle_destroyed_widgets(void);

#endif
