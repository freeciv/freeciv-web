/********************************************************************** 
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/********************************************************************** 
  Reading and using the themespec files, which describe
  the files and contents of themes.
***********************************************************************/
#ifndef FC__THEMESPEC_H
#define FC__THEMESPEC_H

#include "options.h"

#include "themebackgrounds.h"
#include "themecolors.h"

struct sprite;			/* opaque; gui-dep */

struct theme;

extern struct theme *theme;

const char **get_theme_list(void);

struct theme *theme_read_toplevel(const char *theme_name);
void theme_free(struct theme *theme);
void theme_load_sprites(struct theme *t);
void theme_free_sprites(struct theme *t);

void themespec_try_read(const char *theme_name);
void themespec_reread(const char *theme_name);
void themespec_reread_callback(struct client_option *option);

struct sprite* theme_lookup_sprite_tag_alt(struct theme *t, int loglevel,
					   const char *tag, const char *alt,
					   const char *what, const char *name);

struct theme_color_system;
struct theme_color_system *theme_get_color_system(const struct theme *t);

struct theme_background_system;
struct theme_background_system *theme_get_background_system(const struct theme *t);  

char *themespec_gfx_filename(const char *gfx_filename);

/* theme accessor functions. */
const char *theme_get_name(const struct theme *t);
const char *theme_font_filename(const struct theme *t);
int theme_default_font_size(const struct theme *t);

#endif  /* FC__THEMESPEC_H */
