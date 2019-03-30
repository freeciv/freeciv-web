/********************************************************************** 
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
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

#include <gtk/gtk.h>

#include "canvas_g.h"

struct canvas
{
  cairo_surface_t *surface;
  cairo_t *drawable;
  float zoom;
};

#define FC_STATIC_CANVAS_INIT { NULL, NULL, 1.0 }

#endif  /* FC__CANVAS_H */
