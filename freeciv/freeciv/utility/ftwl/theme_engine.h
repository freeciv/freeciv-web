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

#ifndef FC__THEME_ENGINE_H
#define FC__THEME_ENGINE_H

#include "common_types.h"
#include "widget.h"

struct section_file;
struct keybinding_list;

struct te_screen_env {
  const char *(*info_get_value) (const char *id);
  void (*button_callback) (const char *id);
  const char *(*edit_get_initial_value) (const char *id);
  int (*edit_get_width) (const char *id);
  void (*action_callback) (const char *action);
};

struct te_screen {
  struct sw_widget *window;

    /* The following fields are private. */
  struct hash_table *widgets;
  struct te_screen_env env;
  struct keybinding_list *keybindings;
};

void te_init(const char *theme, char *example_file);
struct section_file *te_open_themed_file(const char *name);
struct te_screen *te_get_screen(struct sw_widget *parent_window,
				const char *screen_name,
				const struct te_screen_env *env, int depth);
void te_destroy_screen(struct te_screen *screen);

void te_info_update(struct te_screen *screen, const char *id);
const char *te_edit_get_current_value(struct te_screen *screen,
				      const char *id);
struct sprite *te_load_gfx(const char *filename);

/* Read various data from the files */
struct ct_point te_read_point(struct section_file *file, const char *section,
                              const char *prefix);
struct ct_string *te_read_string(struct section_file *file,
				 const char *section, const char *prefix,
				 bool need_background, bool need_text);
struct ct_tooltip *te_read_tooltip(struct section_file *file,
				   const char *section, bool need_text);
void te_read_bounds_alignment(struct section_file *file,
			      const char *section, struct ct_rect *bounds,
			      enum ws_alignment *alignment);
be_color te_read_color(struct section_file *file, const char *section,
		       const char *prefix, const char *suffix);
struct ct_placement *te_read_placement(struct section_file *file,
				       const char *section);
struct be_key *te_read_key(struct section_file *file, const char *section,
			   const char *name);

#endif
