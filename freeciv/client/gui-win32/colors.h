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

struct color {
  LONG rgb;
};

HPEN pen_alloc(struct color *color);
void pen_free(HPEN pen);
HBRUSH brush_alloc(struct color *color);
void brush_free(HBRUSH brush);

#endif  /* FC__COLORS_H */
