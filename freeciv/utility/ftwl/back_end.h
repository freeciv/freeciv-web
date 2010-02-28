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
#ifndef FC__BACK_END_H
#define FC__BACK_END_H

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif

#include "common_types.h"

#define MAX_OPACITY    255
#define MIN_OPACITY    0

#define DEPTH_MIN		100
#define DEPTH_MAX		300
#define DEPTH_HIDDEN		(DEPTH_MIN-1)

struct be_key {
  bool alt;
  bool shift;
  bool control;
  enum {
    BE_KEY_NORMAL,
    BE_KEY_LEFT,
    BE_KEY_RIGHT,
    BE_KEY_UP,
    BE_KEY_DOWN,
    BE_KEY_RETURN,
    BE_KEY_ENTER,
    BE_KEY_BACKSPACE,
    BE_KEY_DELETE,
    BE_KEY_ESCAPE,
    BE_KEY_PRINT,
    BE_KEY_SPACE,
    BE_KEY_KP_0,
    BE_KEY_KP_1,
    BE_KEY_KP_2,
    BE_KEY_KP_3,
    BE_KEY_KP_4,
    BE_KEY_KP_5,
    BE_KEY_KP_6,
    BE_KEY_KP_7,
    BE_KEY_KP_8,
    BE_KEY_KP_9,
    NUM_BE_KEYS
  } type;
  char key;
};

enum be_mouse_button {
  BE_MB_LEFT,
  BE_MB_RIGHT,
  BE_MB_MIDDLE,
  BE_MB_LAST
};

enum be_event_type {
  BE_NO_EVENT,
  BE_DATA_OTHER_FD,
  BE_TIMEOUT,
  BE_EXPOSE,
  BE_MOUSE_MOTION,
  BE_MOUSE_PRESSED,
  BE_MOUSE_RELEASED,
  BE_KEY_PRESSED
};

struct be_event {
  enum be_event_type type;
  int socket;			/* BE_DATA_OTHER_FD */
  struct ct_point position;	// for BE_MOUSE_*, BE_KEY_PRESSED
  enum be_mouse_button button;	// for BE_MOUSE_*
  struct be_key key;		// for BE_KEY_PRESSED
  int state;			// unused;
};

struct osda;  /* Off-Screen Drawing Area */
struct sprite;
struct FT_Bitmap_;

#include "text_renderer.h"

/* ===== general osda ===== */
struct osda *be_create_osda(int width, int height);
void be_free_osda(struct osda *osda);

/* ===== drawing to osda ===== */
#define be_draw_string tr_draw_string

void be_draw_bitmap(struct osda *target, be_color color,
		    const struct ct_point *position,
		    struct  FT_Bitmap_ *bitmap);

void be_draw_region(struct osda *target, const struct ct_rect *region, 
		    be_color color);
void be_draw_line(struct osda *target, const struct ct_point *start,
		  const struct ct_point *end, int line_width, bool dashed,
		  be_color color);
void be_draw_rectangle(struct osda *target, const struct ct_rect *spec,
		       int line_width, be_color color);
void be_draw_sprite(struct osda *target, 
		    const struct sprite *sprite,
		    const struct ct_size *size,
		    const struct ct_point *dest_pos,
		    const struct ct_point *src_pos);
void be_multiply_alphas(struct sprite *dest_sprite,
			const struct sprite *src_sprite,
			const struct ct_point *src_pos);
void be_copy_osda_to_osda(struct osda *dest,
			  struct osda *src,
			  const struct ct_size *size,
			  const struct ct_point *dest_pos,
			  const struct ct_point *src_pos);

/* ===== query info ===== */
void be_screen_get_size(struct ct_size *size);
#define be_string_get_size tr_string_get_size
void be_sprite_get_size(struct ct_size *size,
			const struct sprite *sprite);
void be_osda_get_size(struct ct_size *size,
		      const struct osda *osda);
bool be_is_transparent_pixel(struct osda *osda, const struct ct_point *pos);

/* ===== graphics.c implementation ===== */
struct sprite *be_load_gfxfile(const char *filename);
struct sprite *be_crop_sprite(struct sprite *source,
			      int x, int y, int width, int height);
void be_free_sprite(struct sprite *sprite);

/* ===== other ===== */
void be_init(const struct ct_size *screen_size, bool fullscreen);
bool be_supports_fullscreen(void);
void be_next_non_blocking_event(struct be_event *event);
void be_next_blocking_event(struct be_event *event, struct timeval *timeout);
void be_add_net_input(int sock);
void be_remove_net_input(void);
void be_copy_osda_to_screen(struct osda *src, const struct ct_rect *rect);
void be_write_osda_to_file(struct osda *osda, const char *filename);
be_color be_get_color(int red, int green, int blue, int alpha);

#endif				/* FC__BACK_END_H */
