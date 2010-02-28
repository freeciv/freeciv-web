/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Poject
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__TOOLS_H
#define FC__TOOLS_H

#include "fc_types.h"

/* See client/gui-gtk-2.0/editprop.c for instructions
 * on how to add more object types. */
enum editor_object_type {
  OBJTYPE_TILE = 0,
  OBJTYPE_UNIT,
  OBJTYPE_CITY,
  OBJTYPE_PLAYER,
  OBJTYPE_GAME,
  
  NUM_OBJTYPES
};

enum editor_tool_type {
  ETT_TERRAIN = 0,
  ETT_TERRAIN_RESOURCE,
  ETT_TERRAIN_SPECIAL,
  ETT_MILITARY_BASE,
  ETT_UNIT,
  ETT_CITY,
  ETT_VISION,
  ETT_TERRITORY,
  ETT_STARTPOS,
  ETT_COPYPASTE,

  NUM_EDITOR_TOOL_TYPES
};

enum editor_tool_mode {
  ETM_PAINT = 0,
  ETM_ERASE,
  ETM_COPY,
  ETM_PASTE,

  NUM_EDITOR_TOOL_MODES
};

void editor_init(void);
bool editor_is_active(void);
enum editor_tool_type editor_get_tool(void);
void editor_set_tool(enum editor_tool_type emt);
const struct tile *editor_get_current_tile(void);
void editor_set_current_tile(const struct tile *ptile);

bool editor_tool_has_mode(enum editor_tool_type ett,
                          enum editor_tool_mode etm);
enum editor_tool_mode editor_tool_get_mode(enum editor_tool_type ett);
void editor_tool_set_mode(enum editor_tool_type ett,
                          enum editor_tool_mode etm);
void editor_tool_toggle_mode(enum editor_tool_type ett,
                             enum editor_tool_mode etm);
void editor_tool_cycle_mode(enum editor_tool_type ett);
const char *editor_tool_get_mode_name(enum editor_tool_type ett,
                                      enum editor_tool_mode etm);

bool editor_tool_has_size(enum editor_tool_type ett);
int editor_tool_get_size(enum editor_tool_type ett);
void editor_tool_set_size(enum editor_tool_type ett, int size);

bool editor_tool_has_count(enum editor_tool_type ett);
int editor_tool_get_count(enum editor_tool_type ett);
void editor_tool_set_count(enum editor_tool_type ett, int count);

bool editor_tool_has_applied_player(enum editor_tool_type ett);
int editor_tool_get_applied_player(enum editor_tool_type ett);
void editor_tool_set_applied_player(enum editor_tool_type,
                                    int player_no);

bool editor_tool_is_usable(enum editor_tool_type ett);
bool editor_tool_has_value(enum editor_tool_type ett);
bool editor_tool_has_value_erase(enum editor_tool_type ett);
int editor_tool_get_value(enum editor_tool_type ett);
void editor_tool_set_value(enum editor_tool_type ett, int value);
const char *editor_tool_get_value_name(enum editor_tool_type ett,
                                       int value);

const char *editor_tool_get_name(enum editor_tool_type ett);
struct sprite *editor_tool_get_sprite(enum editor_tool_type ett);
const char *editor_tool_get_tooltip(enum editor_tool_type ett);

const char *editor_get_mode_tooltip(enum editor_tool_mode etm);
struct sprite *editor_get_mode_sprite(enum editor_tool_mode etm);

struct edit_buffer;
struct edit_buffer *editor_get_copy_buffer(void);


enum editor_keyboard_modifiers {
  EKM_NONE  = 0,
  EKM_SHIFT = 1<<0,
  EKM_ALT   = 1<<1,
  EKM_CTRL  = 1<<2
};

enum mouse_button_values {
  MOUSE_BUTTON_OTHER = 0,

  MOUSE_BUTTON_LEFT = 1,
  MOUSE_BUTTON_MIDDLE = 2,
  MOUSE_BUTTON_RIGHT = 3
};

void editor_mouse_button_press(int canvas_x, int canvas_y,
                               int button, int modifiers);
void editor_mouse_button_release(int canvas_x, int canvas_y,
                                 int button, int modifiers);
void editor_mouse_move(int canvas_x, int canvas_y, int modifiers);

void editor_apply_tool(const struct tile *ptile,
                       bool part_of_selection);
void editor_notify_edit_finished(void);

void editor_selection_clear(void);
void editor_selection_add(const struct tile *ptile);
void editor_selection_remove(const struct tile *ptile);
bool editor_tile_is_selected(const struct tile *ptile);
void editor_apply_tool_to_selection(void);
int editor_selection_count(void);
const struct tile *editor_get_selection_center(void);

struct unit *editor_create_unit_virtual(void);


/* These type flags determine what an edit buffer
 * will copy from its source tiles. Multiple flags
 * may be set via bitwise OR. */
enum edit_buffer_types {
  EBT_TERRAIN  = 1<<0,
  EBT_RESOURCE = 1<<1,
  EBT_SPECIAL  = 1<<2,
  EBT_BASE     = 1<<3,
  EBT_UNIT     = 1<<4,
  EBT_CITY     = 1<<5,

  /* Equal to the bitwise OR of all preceding flags. */
  EBT_ALL      = (1<<6) - 1
};

struct edit_buffer *edit_buffer_new(int type_flags);
void edit_buffer_free(struct edit_buffer *ebuf);
void edit_buffer_clear(struct edit_buffer *ebuf);
void edit_buffer_set_origin(struct edit_buffer *ebuf,
                            const struct tile *ptile);
const struct tile *edit_buffer_get_origin(const struct edit_buffer *ebuf);
bool edit_buffer_has_type(const struct edit_buffer *ebuf, int type);
void edit_buffer_copy(struct edit_buffer *ebuf, const struct tile *ptile);
void edit_buffer_copy_square(struct edit_buffer *ebuf,
                             const struct tile *center,
                             int radius);
void edit_buffer_paste(struct edit_buffer *ebuf, const struct tile *dest);
int edit_buffer_get_status_string(const struct edit_buffer *ebuf,
                                  char *buf, int buflen);

/* Iterates over all type flags set for the given buffer. */
#define edit_buffer_type_iterate(ARG_ebuf, NAME_type) \
do {\
  int NAME_type;\
  if (!(ARG_ebuf)) {\
    break;\
  }\
  for (NAME_type = 1; NAME_type < EBT_ALL; NAME_type <<= 1) {\
    if (!(edit_buffer_has_type((ARG_ebuf), NAME_type))) {\
      continue;\
    }

#define edit_buffer_type_iterate_end \
  }\
} while (0)

#endif /* FC__TOOLS_H */
