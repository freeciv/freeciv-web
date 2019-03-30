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
#ifndef FC__COLORS_G_H
#define FC__COLORS_G_H

#include "colors_common.h"

#include "gui_proto_constructor.h"

struct color;

GUI_FUNC_PROTO(struct color *, color_alloc, int r, int g, int b)
GUI_FUNC_PROTO(void, color_free, struct color *color)

GUI_FUNC_PROTO(int, color_brightness_score, struct color *color)

#endif  /* FC__COLORS_G_H */
