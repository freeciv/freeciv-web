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

#ifndef FC__OVERVIEW_COMMON_H
#define FC__OVERVIEW_COMMON_H

#include "fc_types.h"

#include "canvas_g.h"

#include "options.h"

enum overview_layers {
  OLAYER_BACKGROUND,
  OLAYER_RELIEF,
  OLAYER_BORDERS,
  OLAYER_BORDERS_ON_OCEAN,
  OLAYER_UNITS,
  OLAYER_CITIES,
  OLAYER_COUNT
};

/* Holds all information about the overview aka minimap. */
struct overview {
  /* The following fields are controlled by mapview_common.c. */
  double map_x0, map_y0; /* Origin of the overview, in natural coords. */
  int width, height;		/* Size in pixels. */

  /* Holds the map, unwrapped. */
  struct canvas *map;

  /* A backing store for the window itself, wrapped. */
  struct canvas *window;

  bool fog;
  bool layers[OLAYER_COUNT];
};

extern struct overview overview;

/* The overview tile width and height are defined in terms of the base
 * size.  For iso-maps the width is twice the height since "natural"
 * coordinates are used.  For classical maps the width and height are
 * equal.  The base size may be adjusted to get the correct scale. */
extern int OVERVIEW_TILE_SIZE;
#define OVERVIEW_TILE_WIDTH ((MAP_IS_ISOMETRIC ? 2 : 1) * OVERVIEW_TILE_SIZE)
#define OVERVIEW_TILE_HEIGHT OVERVIEW_TILE_SIZE

void map_to_overview_pos(int *overview_x, int *overview_y,
			 int map_x, int map_y);
void overview_to_map_pos(int *map_x, int *map_y,
			 int overview_x, int overview_y);

void refresh_overview_canvas(void);
void overview_update_tile(struct tile *ptile);
void calculate_overview_dimensions(void);

void center_tile_overviewcanvas(void);

void flush_dirty_overview(void);

void overview_redraw_callback(struct client_option *option);

#endif /* FC__OVERVIEW_COMMON_H */
