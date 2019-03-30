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

#ifndef FC__CANVAS_H
#define FC__CANVAS_H

extern "C" {
#include "canvas_g.h"
}

// Qt
#include <QPixmap>

struct canvas {
  QPixmap map_pixmap;
};

struct canvas *qtg_canvas_create(int width, int height);
void pixmap_copy(QPixmap *dest, QPixmap *src, int src_x, int src_y,
                 int dest_x, int dest_y, int width, int height);
void image_copy(QImage *dest, QImage *src, int src_x, int src_y,
                 int dest_x, int dest_y, int width, int height);
QRect zealous_crop_rect(QImage &p);
#endif /* FC__CANVAS_H */
