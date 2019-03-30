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

// gui-qt
#include "colors.h"
#include "qtg_cxxside.h"

/************************************************************************//**
  Allocate a color (adjusting it for our colormap if necessary on paletted
  systems) and return a pointer to it.
****************************************************************************/
struct color *qtg_color_alloc(int r, int g, int b)
{
  struct color *pcolor = new color;

  pcolor->qcolor.setRgb(r, g, b);

  return pcolor;
}

/************************************************************************//**
  Free a previously allocated color.  See qtg_color_alloc.
****************************************************************************/
void qtg_color_free(struct color *pcolor)
{
  delete pcolor;
}

/************************************************************************//**
  Return a number indicating the perceptual brightness of this color
  relative to others (larger is brighter).
****************************************************************************/
int color_brightness_score(struct color *pcolor)
{
  /* QColor has color space conversions, but nothing giving a perceptually
   * even color space */
  struct rgbcolor *prgb = rgbcolor_new(pcolor->qcolor.red(),
                                       pcolor->qcolor.green(),
                                       pcolor->qcolor.blue());
  int score = rgbcolor_brightness_score(prgb);

  rgbcolor_destroy(prgb);
  return score;
}
