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
#ifndef FC__COLORS_H
#define FC__COLORS_H

#include <gtk/gtk.h>

#include "colors_g.h"

struct color {
  GdkRGBA color;
};

enum Display_color_type {
  BW_DISPLAY, GRAYSCALE_DISPLAY, COLOR_DISPLAY
};

enum Display_color_type get_visual(void);

#endif  /* FC__COLORS_H */
