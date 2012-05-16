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

#include "gtkpixcomm.h"

enum canvas_type {
  CANVAS_PIXMAP,
  CANVAS_PIXCOMM,
  CANVAS_PIXBUF
};

struct canvas
{
  enum canvas_type type;

  union {
    GdkPixmap *pixmap;
    GtkPixcomm *pixcomm;
    GdkPixbuf *pixbuf;
  } v;
};

#endif  /* FC__CANVAS_H */
