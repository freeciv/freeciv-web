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
#ifndef FC__BORDERS_H
#define FC__BORDERS_H

#include "fc_types.h"

bool is_border_source(struct tile *ptile);
int tile_border_source_radius_sq(struct tile *ptile);
int tile_border_source_strength(struct tile *ptile);
int tile_border_strength(struct tile *ptile, struct tile *source);

#endif  /* FC__BORDERS_H */
