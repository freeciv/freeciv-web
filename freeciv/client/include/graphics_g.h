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
#ifndef FC__GRAPHICS_G_H
#define FC__GRAPHICS_G_H

#include "shared.h"		/* bool type */

#include "sprite_g.h"

bool isometric_view_supported(void);
bool overhead_view_supported(void);

void load_intro_gfx(void);
void load_cursors(void);

void free_intro_radar_sprites(void);

#endif  /* FC__GRAPHICS_G_H */
