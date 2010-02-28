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

#ifndef FC__WIDGET_H
#define FC__WIDGET_H

struct sprite;
struct sw_widget;

#define SPECLIST_TAG widget
#define SPECLIST_TYPE struct sw_widget
#include "speclist.h"

#define widget_list_iterate(list, item) \
    TYPED_LIST_ITERATE(struct sw_widget, list, item)
#define widget_list_iterate_end  LIST_ITERATE_END

#include "common_types.h"
#include "back_end.h"

enum ws_alignment {
  A_N, A_S, A_W, A_E,
  A_NC, A_SC, A_WC, A_EC,
  A_NW, A_NE, A_SW, A_SE,
  A_NS, A_WE, A_CENTER
};

/* ===== widget ==== */
void sw_widget_set_position(struct sw_widget *widget, int x, int y);
void sw_widget_get_bounds(struct sw_widget *widget, struct ct_rect *bounds);
struct osda *sw_widget_get_osda(struct sw_widget *widget);
void sw_widget_align_parent(struct sw_widget *widget,
			    enum ws_alignment alignment);
void sw_widget_align_box(struct sw_widget *widget,
			 enum ws_alignment alignment, const struct ct_rect
			 *box);
void sw_widget_set_tooltip(struct sw_widget *widget,
			   const struct ct_tooltip *tooltip);
void sw_widget_hcenter(struct sw_widget *widget);
void sw_widget_vcenter(struct sw_widget *widget);
void sw_widget_set_background_sprite(struct sw_widget *widget,
				     struct sprite *sprite);
void sw_widget_set_background_color(struct sw_widget *widget,
				    be_color background_color);
void sw_widget_set_border_color(struct sw_widget *widget,
				be_color border_color);
void sw_widget_destroy(struct sw_widget *widget);
void sw_widget_disable_mouse_events(struct sw_widget *widget);
void sw_widget_select(struct sw_widget *widget);
void sw_widget_set_enabled(struct sw_widget *widget, bool enabled);

/* ===== window ==== */
struct sw_widget *sw_window_create(struct sw_widget *parent, int width,
				   int height, struct ct_string *title,
				   bool has_border, int depth);
struct sw_widget *sw_window_create_by_clone(struct sw_widget *widget,
					    int depth);

void sw_window_add(struct sw_widget *window, struct sw_widget *widget);
void sw_window_remove(struct sw_widget *widget);
void sw_window_resize(struct sw_widget *widget, int width, int height);
void sw_window_set_draggable(struct sw_widget *widget, bool draggable);
void sw_window_set_canvas_background(struct sw_widget *widget, bool yes);
struct osda *sw_window_get_canvas_background(struct sw_widget *widget);
void sw_window_set_mouse_press_notify(struct sw_widget *widget,
				      void (*callback) (struct sw_widget *
							widget,
							const struct ct_point
							* pos, enum be_mouse_button button, int state,
							void *data),
				      void *data);
void sw_window_set_key_notify(struct sw_widget *widget,
			      bool(*callback) (struct sw_widget *
					       widget,
					       const struct be_key * key,
					       void *data), void *data);
void sw_window_canvas_background_region_needs_repaint(struct sw_widget
						      *widget,
						      const struct ct_rect
						      *region);
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
					       enum be_mouse_button button));

/* ===== button ==== */
struct sw_widget *sw_button_create(struct sw_widget *parent,
				   struct ct_string *strings[4],
				   struct osda *foreground_faces[4],
				   struct osda *background_faces[4]);
struct sw_widget *sw_button_create_text(struct sw_widget *parent,
					struct ct_string *string);
struct sw_widget *sw_button_create_bounded(struct sw_widget *parent,
					   struct ct_string *string,
					   struct sprite *background_faces, 
					   const struct ct_rect *bounds,enum ws_alignment alignment);
struct sw_widget *sw_button_create_text_and_background(struct sw_widget
						       *parent, struct ct_string
						       *string, struct sprite
						       *background_faces);

void sw_button_set_callback(struct sw_widget *widget,
			    void (*callback) (struct sw_widget * widget,
					      void *data), void *data);
void sw_button_set_shortcut(struct sw_widget *widget,
			    const struct be_key *key);

/* ===== edit ==== */
struct sw_widget *sw_edit_create(struct sw_widget *parent, int max_size,
				 struct ct_string *temp_and_initial_text);
struct sw_widget *sw_edit_create_bounded(struct sw_widget *parent,
					 int max_size,
					 struct ct_string *temp_and_initial_text,
					 struct ct_rect *bounds,enum ws_alignment alignment);

/* static buffer */
const char *sw_edit_get_text(struct sw_widget *widget);

/* ===== label ==== */
struct sw_widget *sw_label_create_text(struct sw_widget *parent,
				       struct ct_string *string);
struct sw_widget *sw_label_create_text_bounded(struct sw_widget *parent,
					       struct ct_string *string,
					       struct ct_rect *bounds,
					       enum ws_alignment alignment);

/* ===== list ==== */
struct sw_widget *sw_list_create(struct sw_widget *parent, int pixel_width,
				 int pixel_height);
void sw_list_clear(struct sw_widget *widget);
void sw_list_set_item(struct sw_widget *widget, int column, int row,
		      struct sw_widget *item);
void sw_list_get_view_size(struct sw_widget *widget, struct ct_size *size);
void sw_list_get_window_size(struct sw_widget *widget,struct ct_size *size);
void sw_list_get_offset(struct sw_widget *widget, struct ct_point *pos);
void sw_list_set_offset(struct sw_widget *widget, const struct ct_point *pos);
void sw_list_set_selected_row(struct sw_widget *widget, int row, bool show);
int sw_list_get_selected_row(struct sw_widget *widget);
void sw_list_set_row_enabled(struct sw_widget *widget, int row, bool enabled);
bool sw_list_is_row_enabled(struct sw_widget *widget, int row);
void sw_list_add_buttons_and_vslider(struct sw_widget *widget,
				     struct sprite *up, struct sprite *down,
				     struct sprite *button_background,
				     struct sprite *scrollbar);
void sw_list_set_content_changed_notify(struct sw_widget *widget,
					void (*callback) (struct sw_widget
							  * widget,
							  void *data),
					void *data);

void sw_list_set_selection_changed_notify(struct sw_widget *widget,
					  void (*callback) (struct sw_widget
							    * widget,
							    void *data),
					  void *data);

/* ===== slider ==== */
struct sw_widget *sw_slider_create(struct sw_widget *parent, int width,
				   int height, struct sprite *top,
				   struct sprite *bottom,struct sprite *repeat,
				   struct sprite *center,
				   bool vertical);
void sw_slider_set_slided_notify(struct sw_widget *widget,
				 void (*callback) (struct sw_widget * widget,
						   void *data), void *data);
float sw_slider_get_offset(struct sw_widget *widget);
float sw_slider_get_width(struct sw_widget *widget);
void sw_slider_set_offset(struct sw_widget *widget, float offset);
void sw_slider_set_width(struct sw_widget *widget, float width);

/* ===== slider & list ==== */
void sw_update_vslider_from_list(struct sw_widget *slider,
				 struct sw_widget *list);
void sw_update_hslider_from_list(struct sw_widget *slider,
				 struct sw_widget *list);
void sw_update_list_from_vslider(struct sw_widget *list,
				 struct sw_widget *slider);
void sw_update_list_from_hslider(struct sw_widget *list,
				 struct sw_widget *slider);

/* ===== other ==== */
void sw_init(void);
void sw_paint_all(void);
struct sw_widget *sw_create_root_window(void);
void sw_mainloop(void (*input_callback)(int socket));
int sw_add_timeout(int msec, void (*callback) (void *data), void *data);
void sw_remove_timeout(int id);
void sw_set_dump_screen(bool dump_screen);
bool sw_get_dump_screen(void);

#endif				/* FC__WIDGET_H */
