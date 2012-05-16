/********************************************************************** 
 Freeciv - Copyright (C) 2005 The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__SPRITE_H
#define FC__SPRITE_H

#include "sprite_g.h"

struct sprite {
  struct SDL_Surface *psurface;
};

#define GET_SURF(m_sprite)	(m_sprite->psurface)

#endif				/* FC__SPRITE_H */
