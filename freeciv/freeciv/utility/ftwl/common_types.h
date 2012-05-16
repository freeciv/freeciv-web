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

#ifndef FC__TYPES_H
#define FC__TYPES_H

#include "shared.h"

/* int is sometimes less than 32 bits */
typedef unsigned long be_color;

struct be_key;

struct ct_rect;
#define SPECLIST_TAG region
#define SPECLIST_TYPE struct ct_rect
#include "speclist.h"

#define region_list_iterate(list, item) \
    TYPED_LIST_ITERATE(struct ct_rect, list, item)
#define region_list_iterate_end  LIST_ITERATE_END

struct ct_rect {
  int x, y, width, height;
};

struct ct_size {
  int width, height;
};

struct ct_point {
  int x, y;
};

struct tr_string_data;

struct ct_string {
  char *font;
  int font_size;
  be_color foreground;
  be_color background;
  char *text;
  bool anti_alias;
  int outline_width;
  be_color outline_color;
  enum cts_transform {
    CTS_TRANSFORM_NONE,
    CTS_TRANSFORM_UPPER
  } transform;	

  /* The following fields are cached. */
  int rows;
  struct {
    char *text;
    struct tr_string_data *data;
  } *row;
  struct ct_size size;
};

struct ct_tooltip {
  struct ct_string *text;
  int delay;
  int shadow;
  be_color shadow_color;
};

struct ct_placement {
  enum ct_placement_type { PL_LINE,PL_GRID } type;
  union {
    struct {
      int start_x, start_y, dx, dy;
    } line;
    struct {
      int x, y, dx, dy,width,height;
      enum ct_placement_align { PL_N,PL_S } last;
    } grid;
  } data;
};

struct ct_string *ct_string_create(const char *font, int size,
				   be_color foreground,
				   be_color background,
				   const char *text, bool anti_alias,
				   int outline_width,
				   be_color outline_color,
				   enum cts_transform transform);
void ct_string_destroy(struct ct_string *string);
struct ct_string *ct_string_clone(const struct ct_string *orig);
struct ct_string *ct_string_clone1(const struct ct_string *orig, int new_size);
struct ct_string *ct_string_clone2(const struct ct_string *orig,
				   int new_size, const char *new_text);
struct ct_string *ct_string_clone3(const struct ct_string *orig,
				   const char *new_text);
struct ct_string *ct_string_clone4(const struct ct_string *orig,
				   const char *new_text, be_color new_color);
struct ct_string *ct_string_wrap(const struct ct_string *orig, int max_width);

struct ct_tooltip *ct_tooltip_create(const struct ct_string *text,
				     int delay, int shadow,
				     be_color shadow_color);
void ct_tooltip_destroy(struct ct_tooltip *tooltip);
struct ct_tooltip *ct_tooltip_clone(const struct ct_tooltip *orig);
struct ct_tooltip *ct_tooltip_clone1(const struct ct_tooltip *orig,
				     const char *new_text);

bool ct_rect_valid(const struct ct_rect *container);
bool ct_point_valid(const struct ct_point *point);
void ct_clip_point(struct ct_point *to_draw, const struct ct_rect *available);

bool ct_rect_in_rect_list(const struct ct_rect *item,
		     const struct region_list *list);
bool ct_rect_in_rect(const struct ct_rect *item,
		     const struct ct_rect *container);
bool ct_point_in_rect(const struct ct_point *item,
		      const struct ct_rect *container);
const char *ct_rect_to_string(const struct ct_rect *rect);
bool ct_rect_equal(const struct ct_rect *rect1,
		   const struct ct_rect *rect2);
void ct_rect_intersect(struct ct_rect *dest, const struct ct_rect *src);
struct ct_rect *ct_rect_clone(const struct ct_rect *src);
bool ct_rect_empty(const struct ct_rect *rect);
bool ct_size_empty(const struct ct_size *size);
void ct_rect_fill_on_2_points(struct ct_rect *rect,
			      const struct ct_point *point1,
			      const struct ct_point *point2);
void ct_clip_rect(struct ct_rect *to_draw, const struct ct_rect *available);

void ct_get_placement(const struct ct_placement *placement,
		      struct ct_point *dest, int i, int num);

struct be_key *ct_key_clone(const struct be_key *key);
void ct_key_destroy(struct be_key *key);
const char *ct_key_format(const struct be_key *key);
struct be_key *ct_key_parse(const char *string);
bool ct_key_matches(const struct be_key *template,
		    const struct be_key *actual_key);
bool ct_key_is_valid(const struct be_key *key);

#endif
