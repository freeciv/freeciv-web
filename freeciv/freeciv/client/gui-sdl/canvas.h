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

#ifndef FC__CANVAS_H
#define FC__CANVAS_H

#include "SDL.h"

#include "canvas_g.h"

struct canvas {
  SDL_Surface *surf;
};

struct canvas *canvas_create_with_alpha(int width, int height);
    
#endif				/* FC__CANVAS_H */
