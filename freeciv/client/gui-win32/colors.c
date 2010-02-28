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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>

#include "log.h"
#include "assert.h"
#include "mem.h"

#include "colors.h"

/****************************************************************************
  Allocate a color and return a pointer to it.
****************************************************************************/
struct color *color_alloc(int r, int g, int b)
{
  struct color *color = fc_malloc(sizeof(*color));

  color->rgb = RGB(r, g, b);

  return color;
}

/****************************************************************************
  Free a previously allocated color.  See color_alloc.
****************************************************************************/
void color_free(struct color *color)
{
  free(color);
}

/****************************************************************************
  Allocate a pen based on this color.
****************************************************************************/
HPEN pen_alloc(struct color *color)
{
  if (!color) {
    /* Color table has not been allocated yet, return a black pen */
    return CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
  }

  return CreatePen(PS_SOLID, 0, color->rgb);
}

/****************************************************************************
  Free a previously allocated pen.  See pen_alloc.
****************************************************************************/
void pen_free(HPEN pen)
{
  DeleteObject(pen);
}

/****************************************************************************
  Allocate a brush based on this color.
****************************************************************************/
HBRUSH brush_alloc(struct color *color)
{
  if (!color) {
    /* Color table has not been allocated yet, return a black brush */
    return CreateSolidBrush(RGB(0, 0, 0));
  }

  return CreateSolidBrush(color->rgb);
}

/****************************************************************************
  Free a previously allocated brush.  See brush_alloc.
****************************************************************************/
void brush_free(HBRUSH brush)
{
  DeleteObject(brush);
}
