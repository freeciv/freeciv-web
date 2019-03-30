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
#include "graphics.h"

struct sprite {
  struct SDL_Surface *psurface;
};

/* Use this when sure that m_sprite is not NULL, to avoid risk of freeing
 * Main.dummy */
#define GET_SURF_REAL(m_sprite)  ((m_sprite)->psurface)

/* If m_sprite is NULL, return a dummy (small, empty) surface instead */
#define GET_SURF(m_sprite)  ((m_sprite) ? GET_SURF_REAL(m_sprite) : Main.dummy)

#endif /* FC__SPRITE_H */
