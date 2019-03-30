/**********************************************************************
 Freeciv - Copyright (C) 2006 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__WIDGET_COMBO_H
#define FC__WIDGET_COMBO_H

struct strvec;

#define combo_new_from_chars(background, dest, font_size,                   \
                             char_string, vector, length, flags)            \
  combo_new(background, dest, create_utf8_from_char(char_string,            \
            font_size), vector, length, flags)

struct widget *combo_new(SDL_Surface *background, struct gui_layer *dest,
                         utf8_str *pstr, const struct strvec *vector,
                         int length, Uint32 flags);
void combo_popup(struct widget *combo);
void combo_popdown(struct widget *combo);

#endif /* FC__WIDGET_COMBO_H */
