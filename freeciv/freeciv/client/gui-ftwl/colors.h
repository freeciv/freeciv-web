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

#include "colors_g.h"

#include "common_types.h"

struct color {
  be_color color;
};

enum colors { 
  COLOR_STD_BLACK, COLOR_STD_WHITE, COLOR_STD_RED, 
  COLOR_STD_YELLOW, COLOR_STD_CYAN, 
  COLOR_STD_GROUND, COLOR_STD_OCEAN, 
  COLOR_STD_BACKGROUND, 
  COLOR_STD_RACE0, COLOR_STD_RACE1, COLOR_STD_RACE2, 
  COLOR_STD_RACE3, COLOR_STD_RACE4, COLOR_STD_RACE5, 
  COLOR_STD_RACE6, COLOR_STD_RACE7, COLOR_STD_RACE8, 
  COLOR_STD_RACE9, COLOR_STD_RACE10, COLOR_STD_RACE11, 
  COLOR_STD_RACE12, COLOR_STD_RACE13,
  COLOR_STD_LAST,
    
  COLOR_EXT_BLUE = COLOR_STD_LAST, 
  COLOR_EXT_TOOLTIP_FOREGROUND, 
  COLOR_EXT_TOOLTIP_BACKGROUND, 
  COLOR_EXT_GRAY, 
  COLOR_EXT_LIGHT_GRAY, 
  COLOR_EXT_GREEN, 
  COLOR_EXT_DEFAULT_WINDOW_BACKGROUND, 
  COLOR_EXT_SELECTED_ROW, 
  COLOR_EXT_DARK_BLUE, 
  COLOR_EXT_OUTERSPACE_FG, 
  COLOR_EXT_OUTERSPACE_BG, 
  COLOR_EXT_LAST 
}; 

#endif				/* FC__COLORS_H */
