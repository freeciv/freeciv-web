/***********************************************************************
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
#include <fc_config.h>
#endif

/* utility */
#include "mem.h"

/* common */
#include "rgbcolor.h"

/* gui main header */
#include "gui_stub.h"

#include "colors.h"

/************************************************************************//**
  Allocate a color (adjusting it for our colormap if necessary on paletted
  systems) and return a pointer to it.
****************************************************************************/
struct color *gui_color_alloc(int r, int g, int b)
{
  struct color *color = fc_malloc(sizeof(*color));

  /* PORTME */
  color->r = r;
  color->g = g;
  color->b = b;

  return color;
}

/************************************************************************//**
  Free a previously allocated color.  See color_alloc.
****************************************************************************/
void gui_color_free(struct color *color)
{
  /* PORTME */
  free(color);
}

/************************************************************************//**
  Return a number indicating the perceptual brightness of this color
  relative to others (larger is brighter).
****************************************************************************/
int color_brightness_score(struct color *pcolor)
{
  /* PORTME */
  /* Can use GUI-specific colorspace functions here. This is a fallback
   * using platform-independent code */
  struct rgbcolor *prgb = rgbcolor_new(pcolor->r, pcolor->g, pcolor->b);
  int score = rgbcolor_brightness_score(prgb);

  rgbcolor_destroy(prgb);
  return score;
}
