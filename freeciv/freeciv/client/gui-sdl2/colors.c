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

/***********************************************************************
                          colors.c  -  description
                             -------------------
    begin                : Mon Jul 15 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* client */
#include "tilespec.h"

/* gui-sdl2 */
#include "themespec.h"

#include "colors.h"

/**********************************************************************//**
  Get color from theme.
**************************************************************************/
SDL_Color *get_theme_color(enum theme_color themecolor)
{
  return theme_get_color(theme, themecolor)->color;
}

/**********************************************************************//**
  Get color for some game object instance.
**************************************************************************/
SDL_Color *get_game_color(enum color_std stdcolor)
{
  return get_color(tileset, stdcolor)->color;
}

/**********************************************************************//**
  Allocate a color with alpha channel and return a pointer to it. Alpha
  channel is not really used yet.
**************************************************************************/
struct color *color_alloc_rgba(int r, int g, int b, int a)
{
  struct color *result = fc_malloc(sizeof(*result));
  SDL_Color *pcolor = fc_malloc(sizeof(*pcolor));

  pcolor->r = r;
  pcolor->g = g;
  pcolor->b = b;
  pcolor->a = a;

  result->color = pcolor;

  return result;
}

/**********************************************************************//**
  Allocate a solid color and return a pointer to it.
**************************************************************************/
struct color *color_alloc(int r, int g, int b)
{
  struct color *result = fc_malloc(sizeof(*result));
  SDL_Color *pcolor = fc_malloc(sizeof(*pcolor));

  pcolor->r = r;
  pcolor->g = g;
  pcolor->b = b;
  pcolor->a = 255;

  result->color = pcolor;

  return result;
}

/**********************************************************************//**
  Free resources allocated for color.
**************************************************************************/
void color_free(struct color *pcolor)
{
  if (!pcolor) {
    return;
  }

  if (pcolor->color) {
    free(pcolor->color);
  }

  free(pcolor);
}

/**********************************************************************//**
  Return a number indicating the perceptual brightness of this color
  relative to others (larger is brighter).
**************************************************************************/
int color_brightness_score(struct color *pcolor)
{
  struct rgbcolor *prgb = rgbcolor_new(pcolor->color->r,
                                       pcolor->color->g,
                                       pcolor->color->b);
  int score = rgbcolor_brightness_score(prgb);

  rgbcolor_destroy(prgb);
  return score;
}
