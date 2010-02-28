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

#include <assert.h>
#include <stdio.h>

/* uitlity */
#include "log.h"
#include "mem.h"
#include "support.h"

#include "back_end.h"
#include "text_renderer.h"

#include "common_types.h"

static struct {
  int type;
  char *name;
} keymap[] = {
  {
  BE_KEY_LEFT, "Left"}, {
  BE_KEY_RIGHT, "Right"}, {
  BE_KEY_UP, "Up"}, {
  BE_KEY_DOWN, "Down"}, {
  BE_KEY_RETURN, "Return"}, {
  BE_KEY_ENTER, "Enter"}, {
  BE_KEY_BACKSPACE, "Backspace"}, {
  BE_KEY_DELETE, "Del"}, {
  BE_KEY_ESCAPE, "Esc"}, {
  BE_KEY_SPACE, "Space"}, {
  BE_KEY_KP_0, "Kp0"}, {
  BE_KEY_KP_1, "Kp1"}, {
  BE_KEY_KP_2, "Kp2"}, {
  BE_KEY_KP_3, "Kp3"}, {
  BE_KEY_KP_4, "Kp4"}, {
  BE_KEY_KP_5, "Kp5"}, {
  BE_KEY_KP_6, "Kp6"}, {
  BE_KEY_KP_7, "Kp7"}, {
  BE_KEY_KP_8, "Kp8"}, {
  BE_KEY_KP_9, "Kp9"}, {
  BE_KEY_PRINT, "Print"}
};

/*************************************************************************
  ...
*************************************************************************/
bool ct_rect_valid(const struct ct_rect *rect)
{
  return rect->x >= 0 && rect->y >= 0 && rect->width >= 0 && rect->height >= 0;
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_point_valid(const struct ct_point *point)
{
  return point->x >= 0 && point->y >= 0;
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_rect_in_rect_list(const struct ct_rect *item,
			  const struct region_list *region_list)
{
  region_list_iterate(region_list, container) {
    if (ct_rect_in_rect(item, container)) {
      return TRUE;
    }
  } region_list_iterate_end;
  return FALSE;
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_rect_in_rect(const struct ct_rect *item,
		     const struct ct_rect *container)
		    
{
    assert(ct_rect_valid(container) && ct_rect_valid(item));

    return (item->x >= container->x && item->y >= container->y &&
	    item->x + item->width <= container->x + container->width &&
	    item->y + item->height <= container->y + container->height);
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_point_in_rect(const struct ct_point *item,
		      const struct ct_rect *container)
{
    return (item->x >= container->x && item->y >= container->y &&
	    item->x < container->x + container->width &&
	    item->y < container->y + container->height);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_string *ct_string_create(const char *font, int size,
				   be_color foreground,
				   be_color background,
				   const char *text, bool anti_alias,
				   int outline_width,
				   be_color outline_color,
				   enum cts_transform transform)
{
  struct ct_string *result = fc_malloc(sizeof(*result));

  result->font = mystrdup(font);
  result->font_size = size;
  result->foreground = foreground;
  result->background = background;
  result->text = mystrdup(text);
  result->anti_alias = anti_alias;
  result->outline_width = outline_width;
  result->outline_color = outline_color;
  result->transform = transform;

  if (transform == CTS_TRANSFORM_NONE) {
    /* nothing */
  } else if (transform == CTS_TRANSFORM_UPPER) {
    size_t len = strlen(result->text);
    int i;

    for (i = 0; i < len; i++) {
      result->text[i] = my_toupper(result->text[i]);
    }
  } else {
    assert(0);
  }
      
  /* split the string */
  {
      int row;
      char *s, *tmp = mystrdup(result->text);

      result->rows = 1;
      for (s = tmp; *s != '\0'; s++) {
	if (*s == '\n') {
	  result->rows++;
	}
      }
      result->row = fc_malloc(sizeof(*result->row) * result->rows);
      s = tmp;

      for (row = 0; row < result->rows; row++) {
	char *end = strchr(s, '\n');

	if (end) {
	  *end = '\0';
	  result->row[row].text = mystrdup(s);
	  s = end + 1;
	} else {
	  result->row[row].text = mystrdup(s);
	}
	//printf("[%d]='%s'\n", row, result->row[row].text);
      }
      free(tmp);
  }
  
  tr_prepare_string(result);
  be_string_get_size(&result->size, result);

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void ct_string_destroy(struct ct_string *string)
{
  int row;

  free(string->font);
  free(string->text);
  tr_free_string(string);
  for (row = 0; row < string->rows; row++) {
    free(string->row[row].text);
  }
  free(string->row);
  free(string);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_string *ct_string_clone(const struct ct_string *orig)
{
  return ct_string_create(orig->font, orig->font_size, orig->foreground,
			  orig->background, orig->text, orig->anti_alias,
			  orig->outline_width, orig->outline_color,
			  orig->transform);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_string *ct_string_clone1(const struct ct_string *orig,
				   int new_size)
{
  return ct_string_create(orig->font, new_size, orig->foreground,
			  orig->background, orig->text, orig->anti_alias,
			  orig->outline_width, orig->outline_color,
			  orig->transform);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_string *ct_string_clone2(const struct ct_string *orig,
				   int new_size, const char *new_text)
{
  return ct_string_create(orig->font, new_size, orig->foreground,
			  orig->background, new_text, orig->anti_alias,
			  orig->outline_width, orig->outline_color,
			  orig->transform);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_string *ct_string_clone3(const struct ct_string *orig,
				   const char *new_text)
{
  return ct_string_create(orig->font, orig->font_size, orig->foreground,
			  orig->background, new_text, orig->anti_alias,
			  orig->outline_width, orig->outline_color,
			  orig->transform);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_string *ct_string_clone4(const struct ct_string *orig,
				   const char *new_text, be_color new_color)
{
  return ct_string_create(orig->font, orig->font_size, new_color,
			  orig->background, new_text, orig->anti_alias,
			  orig->outline_width, orig->outline_color,
			  orig->transform);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_string *ct_string_wrap(const struct ct_string *orig, int max_width)
{
  struct ct_string *result;
  int columns;

  for (columns = 100; columns > 1; columns--) {
    char *copy = mystrdup(orig->text);

    wordwrap_string(copy, columns);
    result = ct_string_clone3(orig, copy);
    free(copy);
    if (result->size.width <= max_width) {
      return result;
    }
  }
  assert(0);
  return NULL;
}

/*************************************************************************
  ...
*************************************************************************/
const char *ct_rect_to_string(const struct ct_rect *rect)
{
  static char buffer[100];

  my_snprintf(buffer, 100, "{x=%d, y=%d, width=%d, height=%d}", rect->x,
	      rect->y, rect->width, rect->height);
  return buffer;
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_rect_equal(const struct ct_rect *rect1, const struct ct_rect *rect2)
{
  return ct_rect_in_rect(rect1, rect2) && ct_rect_in_rect(rect2, rect1);
}

/*************************************************************************
  ...
*************************************************************************/
void ct_rect_intersect(struct ct_rect *dest, const struct ct_rect *src)
{
  int last_x = MIN(dest->x + dest->width, src->x + src->width);
  int last_y = MIN(dest->y + dest->height, src->y + src->height);

  assert(ct_rect_valid(dest) && ct_rect_valid(src));
  dest->x = MAX(dest->x, src->x);
  dest->y = MAX(dest->y, src->y);

  dest->width = MAX(0, last_x - dest->x);
  dest->height = MAX(0, last_y - dest->y);
  assert(ct_rect_valid(dest));
}

/*************************************************************************
  ...
*************************************************************************/
void ct_clip_point(struct ct_point *to_draw, const struct ct_rect *available)
{
  to_draw->x =
      CLIP(available->x, to_draw->x, available->x + available->width-1);
  to_draw->y =
      CLIP(available->y, to_draw->y, available->y + available->height-1);
  assert(ct_point_in_rect(to_draw, available));
}

/*************************************************************************
  ...
*************************************************************************/
void ct_clip_rect(struct ct_rect *to_draw, const struct ct_rect *available)
{
  struct ct_point p1 = { to_draw->x, to_draw->y };
  struct ct_point p2 =
      { to_draw->x + to_draw->width-1, to_draw->y + to_draw->height-1 };

  ct_clip_point(&p1, available);
  ct_clip_point(&p2, available);

  /* If after clipping the points are outside we have an empty area */
  if (!ct_point_in_rect(&p1, to_draw) || !ct_point_in_rect(&p2, to_draw)) {
    to_draw->x = 0;
    to_draw->y = 0;
    to_draw->width = 0;
    to_draw->height = 0;
  } else {
    ct_rect_fill_on_2_points(to_draw, &p1, &p2);
  }

  assert(ct_rect_in_rect(to_draw, available));
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_rect *ct_rect_clone(const struct ct_rect *src)
{
  struct ct_rect *result = fc_malloc(sizeof(*result));

  *result = *src;
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_rect_empty(const struct ct_rect *rect)
{
  return rect->width == 0 || rect->height == 0;
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_size_empty(const struct ct_size *size)
{
  return size->width == 0 || size->height == 0;
}

/*************************************************************************
  ...
*************************************************************************/
void ct_rect_fill_on_2_points(struct ct_rect *rect,
			      const struct ct_point *point1,
			      const struct ct_point *point2)
{
  rect->x = MIN(point1->x, point2->x);
  rect->y = MIN(point1->y, point2->y);
  rect->width = MAX(point1->x, point2->x) - rect->x+1;
  rect->height = MAX(point1->y, point2->y) - rect->y+1;
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_tooltip *ct_tooltip_create(const struct ct_string *text,
				     int delay, int shadow,
				     be_color shadow_color)
{
  struct ct_tooltip *result = fc_malloc(sizeof(*result));

  result->text = ct_string_clone(text);
  result->delay = delay;
  result->shadow = shadow;
  result->shadow_color = shadow_color;

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void ct_tooltip_destroy(struct ct_tooltip *tooltip)
{
    ct_string_destroy(tooltip->text);
    free(tooltip);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_tooltip *ct_tooltip_clone(const struct ct_tooltip *orig)
{
  return ct_tooltip_create(orig->text, orig->delay, orig->shadow,
			   orig->shadow_color);
}

/*************************************************************************
  ...
*************************************************************************/
struct ct_tooltip *ct_tooltip_clone1(const struct ct_tooltip *orig,
				     const char *new_text)
{
  struct ct_tooltip *result;
  struct ct_string *s = ct_string_clone3(orig->text, new_text);

  result = ct_tooltip_create(s, orig->delay, orig->shadow,
			     orig->shadow_color);
  ct_string_destroy(s);
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void ct_get_placement(const struct ct_placement *placement,
		      struct ct_point *dest, int i, int num)
{
  if (placement->type == PL_LINE) {
    dest->x = placement->data.line.start_x + i * placement->data.line.dx;
    dest->y = placement->data.line.start_y + i * placement->data.line.dy;
  } else if (placement->type == PL_GRID) {
    int x = i / placement->data.grid.height;
    int y = i % placement->data.grid.height;

    if (placement->data.grid.last == PL_S) {
      y = placement->data.grid.height - y-1;
    }
    dest->x = placement->data.grid.x + x * placement->data.grid.dx;
    dest->y = placement->data.grid.y + y * placement->data.grid.dy;
  } else {
    assert(0);
  }
}

/*************************************************************************
  ...
*************************************************************************/
const char *ct_key_format(const struct be_key *key)
{
  static char out[100];
  char buffer[100];

  if (key->type == BE_KEY_NORMAL) {
    my_snprintf(buffer, sizeof(buffer), "%c", key->key);
  } else {
    int i;
    bool found = FALSE;

    for (i = 0; i < ARRAY_SIZE(keymap); i++) {
      if (keymap[i].type == key->type) {
	my_snprintf(buffer, sizeof(buffer), "%s", keymap[i].name);
	found = TRUE;
	break;
      }
    }
    assert(found);
  }

  my_snprintf(out, sizeof(out), "%s%s%s%s",
	   (key->alt ? "Alt-" : ""),
	   (key->control ? "Ctrl-" : ""),
	   (key->shift ? "Shift-" : ""), buffer);
  return out;
}

/*************************************************************************
  ...
*************************************************************************/
struct be_key *ct_key_parse(const char *string)
{
  struct be_key *result = fc_malloc(sizeof(*result));
  const char alt[] = "Alt-";
  const char ctrl[] = "Ctrl-";
  const char shift[] = "Shift-";
  const char *orig=string;
  int i;
  bool found = FALSE;


  result->alt = FALSE;
  result->control = FALSE;
  result->shift = FALSE;

  if (strncmp(string, alt, strlen(alt)) == 0) {
    result->alt = TRUE;
    string += strlen(alt);
  }
  if (strncmp(string, ctrl, strlen(ctrl)) == 0) {
    result->control = TRUE;
    string += strlen(ctrl);
  }
  if (strncmp(string, shift, strlen(shift)) == 0) {
    result->shift = TRUE;
    string += strlen(shift);
  }

  for (i = 0; i < ARRAY_SIZE(keymap); i++) {
    if (strcmp(string, keymap[i].name) == 0) {
      result->type = keymap[i].type;
      found = TRUE;
      break;
    }
  }
  if (!found) {
    if (strlen(string) != 1) {
      die("key description '%s' can't be parsed", orig);
    }
    result->type = BE_KEY_NORMAL;
    result->key = string[0];
  }

  if (!ct_key_is_valid(result)) {
    die("key description '%s' isn't valid", orig);
  }
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_key_matches(const struct be_key *template,
		    const struct be_key *actual_key)
{
  if (template->alt && !actual_key->alt)
    return FALSE;
  if (template->control && !actual_key->control)
    return FALSE;
  if (template->shift && !actual_key->shift)
    return FALSE;
  if (template->type != actual_key->type)
    return FALSE;
  if (template->type == BE_KEY_NORMAL && template->key != actual_key->key)
    return FALSE;
  return TRUE;
}

/*************************************************************************
  ...
*************************************************************************/
struct be_key *ct_key_clone(const struct be_key *key)
{
  struct be_key *result = fc_malloc(sizeof(*result));
  *result = *key;
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void ct_key_destroy(struct be_key *key)
{
  free(key);
}

/*************************************************************************
  ...
*************************************************************************/
bool ct_key_is_valid(const struct be_key *key)
{
  if (key->shift && key->type == BE_KEY_NORMAL && key->key != ' ') {
    return FALSE;
  }

  return TRUE;
}
