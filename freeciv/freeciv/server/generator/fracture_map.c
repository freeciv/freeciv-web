/***********************************************************************
 Freeciv - Copyright (C) 1996-2016 - The Freeciv Project
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
#include "rand.h"

/* common */
#include "map.h"

/* server/generator */
#include "height_map.h"
#include "mapgen_topology.h"
#include "mapgen_utils.h"

#include "fracture_map.h"

static void circle_bresenham(int xc, int yc, int r, int nn);
static void fmfill(int x, int y, int c, int r);
static int local_ave_elevation(struct tile *ptile);

extern int *height_map;
int num_landmass = 50;

typedef struct {
  int x;
  int y;
} map_point;

typedef struct {
  int minX, minY;
  int maxX, maxY;
  int elevation;
} map_landmass;

/* Landmass: a chunk of rock with common properties */
static map_landmass *landmass;
static map_point *fracture_points;

/**********************************************************************//**
  Fracture map generator
**************************************************************************/
void make_fracture_map(void)
{
  int nn, mm;
  int rad;
  int x,y;
  struct tile *ptile1;

  /* Calculate the mountain level.  map.server.mountains specifies the
   * percentage of land that is turned into hills and mountains. */
  hmap_mountain_level = (((hmap_max_level - hmap_shore_level)
                          * (100 - wld.map.server.steepness))
                         / 100 + hmap_shore_level);

  /* For larger maps, increase the number of landmasses - makes the map more interesting */
  num_landmass = 20 + 15 * get_sqsize();
  landmass = (map_landmass *)fc_malloc((wld.map.xsize / 2 + wld.map.ysize / 2 + num_landmass) * sizeof(map_landmass));
  fracture_points = (map_point *)fc_malloc((wld.map.xsize / 2 + wld.map.ysize / 2 + num_landmass) * sizeof(map_point));
  height_map = fc_malloc(sizeof(*height_map) * MAP_INDEX_SIZE);

  /* Setup a whole bunch of landmasses along the view bordere. These will be sunken
     to create ocean terrain.*/
  nn = 0;
  for (x = 3; x < wld.map.xsize; x += 5, nn++) {
    fracture_points[nn].x = x;
    fracture_points[nn].y = 3;
  }
  for (x = 3; x < wld.map.xsize; x += 5, nn++) {
    fracture_points[nn].x = x;
    fracture_points[nn].y = wld.map.ysize - 3;
  }
  for (y = 3; y < wld.map.ysize; y += 5, nn++) {
    fracture_points[nn].x = 3;
    fracture_points[nn].y = y;
  }
  for (y = 3; y < wld.map.ysize; y += 5, nn++) {
    fracture_points[nn].x = wld.map.xsize - 3;
    fracture_points[nn].y = y;
  }

  /* pick remaining points randomly */
  mm = nn;
  for (; nn < mm + num_landmass; nn++) {
    fracture_points[nn].x = fc_rand(wld.map.xsize - 6) + 3;
    fracture_points[nn].y = fc_rand(wld.map.ysize - 6) + 3;
  }
  for (nn = 0; nn < mm + num_landmass; nn++) {
    landmass[nn].minX = wld.map.xsize - 1;
    landmass[nn].minY = wld.map.ysize - 1;
    landmass[nn].maxX = 0;
    landmass[nn].maxY = 0;
    x = fracture_points[nn].x;
    y = fracture_points[nn].y;
    ptile1 = native_pos_to_tile(&(wld.map), x, y);
    ptile1->continent = nn + 1;
  }

  /* Assign a base elevation to the landmass */
  for (nn = 0; nn < mm + num_landmass; nn++) {
    if (nn < mm) { /* sink the border masses */
      landmass[nn].elevation = 0;
    } else {
      landmass[nn].elevation = fc_rand(1000);
    }
  }

  /* Assign cells to landmass. Gradually expand the radius of the 
     fracture point. */
  for (rad = 1; rad < (wld.map.xsize >> 1); rad++) {
    for (nn = 0; nn < mm + num_landmass; nn++) {
      circle_bresenham(fracture_points[nn].x, fracture_points[nn].y, rad, nn+1);
    }
  }

  /* put in some random fuzz */
  whole_map_iterate(&(wld.map), ptile) {
    if (hmap(ptile) > hmap_shore_level) {
      hmap(ptile) = hmap(ptile) + fc_rand(4) - 2;
    }
    if (hmap(ptile) <= hmap_shore_level) {
      hmap(ptile) = hmap_shore_level + 1;
    }
  } whole_map_iterate_end;

  adjust_int_map(height_map, hmap_max_level);
  free(landmass);
  free(fracture_points);
}

/**********************************************************************//**
  An expanding circle from the fracture point is used to determine the
   midpoint between fractures. The cells must be assigned to landmasses
   anyway.
**************************************************************************/
static void circle_bresenham(int xc, int yc, int r, int nn)
{
  int x = 0; 
  int y = r; 
  int p = 3 - 2 * r;

  if (!r) {
    return;
  }

  while (y >= x) { /* only formulate 1/8 of circle */
    fmfill(xc-x, yc-y, nn, r); /* upper left left */
    fmfill(xc-y, yc-x, nn, r); /* upper upper left */
    fmfill(xc+y, yc-x, nn, r); /* upper upper right */
    fmfill(xc+x, yc-y, nn, r); /* upper right right */
    fmfill(xc-x, yc+y, nn, r); /* lower left left */
    fmfill(xc-y, yc+x, nn, r); /* lower lower left */
    fmfill(xc+y, yc+x, nn, r); /* lower lower right */
    fmfill(xc+x, yc+y, nn, r); /* lower right right */
    if (p < 0) {
      p += 4 * x++ + 6;
    } else {
      p += 4 * (x++ - y--) + 10;
    }
  }
}

/**********************************************************************//**
   Assign landmass in 3x3 area increments to avoid "holes" created by the
   circle algorithm.
**************************************************************************/
static void fmfill(int x, int y, int c, int r)
{
  int x_less, x_more, y_less, y_more;
  struct tile *ptileXY;
  struct tile *ptileX2Y;
  struct tile *ptileX1Y;
  struct tile *ptileXY2;
  struct tile *ptileXY1;
  struct tile *ptileX2Y1;
  struct tile *ptileX2Y2;
  struct tile *ptileX1Y2;
  struct tile *ptileX1Y1;

  if (x < 0) {
    x = wld.map.xsize + x;
  } else if (x > wld.map.xsize) {
    x = x - wld.map.xsize;
  }
  x_less = x - 1;
  if (x_less < 0) {
    x_less = wld.map.xsize - 1;
  }
  x_more = x + 1;
  if (x_more >= wld.map.xsize) {
    x_more = 0;
  }
  y_less = y - 1;
  if (y_less < 0) {
    y_less = wld.map.ysize - 1;
  }
  y_more = y + 1;
  if (y_more >= wld.map.ysize) {
    y_more = 0;
  }

  if (y >= 0 && y < wld.map.ysize) {
    ptileXY = native_pos_to_tile(&(wld.map), x, y);
    ptileX2Y = native_pos_to_tile(&(wld.map), x_more, y);
    ptileX1Y = native_pos_to_tile(&(wld.map), x_less, y);
    ptileXY2 = native_pos_to_tile(&(wld.map), x, y_more);
    ptileXY1 = native_pos_to_tile(&(wld.map), x, y_less);
    ptileX2Y1 = native_pos_to_tile(&(wld.map), x_more, y_less);
    ptileX2Y2 = native_pos_to_tile(&(wld.map), x_more, y_more);
    ptileX1Y2 = native_pos_to_tile(&(wld.map), x_less, y_more);
    ptileX1Y1 = native_pos_to_tile(&(wld.map), x_less, y_less);

    if (ptileXY->continent == 0 ) {
      ptileXY->continent = c;
      ptileX2Y->continent = c;
      ptileX1Y->continent = c;
      ptileXY2->continent = c;
      ptileXY1->continent = c;
      ptileX2Y2->continent = c;
      ptileX2Y1->continent = c;
      ptileX1Y2->continent = c;
      ptileX1Y1->continent = c;
      hmap(ptileXY) = landmass[c-1].elevation;
      hmap(ptileX2Y) = landmass[c-1].elevation;
      hmap(ptileX1Y) = landmass[c-1].elevation;
      hmap(ptileXY2) = landmass[c-1].elevation;
      hmap(ptileXY1) = landmass[c-1].elevation;
      hmap(ptileX2Y1) = landmass[c-1].elevation;
      hmap(ptileX2Y2) = landmass[c-1].elevation;
      hmap(ptileX1Y2) = landmass[c-1].elevation;
      hmap(ptileX1Y1) = landmass[c-1].elevation;
      /* This bit of code could track the maximum and minimum extent
       * of the landmass. */
      if (x < landmass[c-1].minX) {
        landmass[c-1].minX = x;
      }
      if (x > landmass[c-1].maxX) {
        landmass[c-1].maxX = x;
      }
      if (y < landmass[c-1].minY) {
        landmass[c-1].minY = y;
      }
      if (y > landmass[c-1].maxY) {
        landmass[c-1].maxY = y;
      }
    }
  }
}

/**********************************************************************//**
    Determine the local average elevation. Used to determine where hills
    and mountains are. 
**************************************************************************/
static int local_ave_elevation(struct tile *ptile)
{
  int ele;
  int n;

  n = ele = 0;
  square_iterate(&(wld.map), ptile, 3, tile2) {
    ele = ele + hmap(tile2);
    n++;
  } square_iterate_end;
  ele /= n;

  return ele;
}

/**********************************************************************//**
  make_fracture_relief() Goes through a couple of iterations.
  The first iteration chooses mountains and hills based on how much the
  tile exceeds the elevation of the surrounding tiles. This will typically
  causes hills and mountains to be placed along the edges of landmasses.
  It can generate mountain ranges where there a differences in elevation
  between landmasses.
**************************************************************************/
void make_fracture_relief(void)
{
  bool choose_mountain;
  bool choose_hill;
  int landarea;
  int total_mtns;
  int iter;

  /* compute the land area */
  landarea = 0;
  whole_map_iterate(&(wld.map), ptile) {
    if (hmap(ptile) > hmap_shore_level) {
      landarea++;
    }
  } whole_map_iterate_end;

  /* Iteration 1
     Choose hills and mountains based on local elevation.
  */
  total_mtns = 0;
  whole_map_iterate(&(wld.map), ptile) {
    if (not_placed(ptile) && hmap(ptile) > hmap_shore_level) {  /* place on land only */
      /* mountains */
      choose_mountain = (hmap(ptile) > local_ave_elevation(ptile) * 1.20)
        || (area_is_too_flat(ptile, hmap_mountain_level, hmap(ptile)) && (fc_rand(10) < 4));

      choose_hill = (hmap(ptile) > local_ave_elevation(ptile) * 1.10)
        || (area_is_too_flat(ptile, hmap_mountain_level, hmap(ptile)) && (fc_rand(10) < 4));
      /* The following avoids hills and mountains directly along the coast. */
      if (count_terrain_class_near_tile(ptile, TRUE, TRUE, TC_OCEAN) > 0) {
        choose_mountain = FALSE;
        choose_hill = FALSE;
      }
      if (choose_mountain) {
        total_mtns++;
        tile_set_terrain(ptile,pick_terrain(MG_MOUNTAINOUS, MG_UNUSED, MG_GREEN));
        map_set_placed(ptile);
      } else if (choose_hill) {
        /* hills */
        total_mtns++;
        tile_set_terrain(ptile,pick_terrain(MG_MOUNTAINOUS, MG_GREEN, MG_UNUSED));
        map_set_placed(ptile);
      }
    }
  } whole_map_iterate_end;

  /* Iteration 2
     Make sure the map has at least the minimum number of mountains according to the
     map steepness setting.
     The iteration limit is a failsafe to prevent the iteration from taking forever.
  */
  for (iter = 0; total_mtns < (landarea * wld.map.server.steepness) / 100 && iter < 50;
       iter++) {
    whole_map_iterate(&(wld.map), ptile) {
      if (not_placed(ptile) && hmap(ptile) > hmap_shore_level) {  /* place on land only */
        choose_mountain = fc_rand(10000) < 10;
        choose_hill = fc_rand(10000) < 10;
        if (choose_mountain) {
          total_mtns++;
          tile_set_terrain(ptile,pick_terrain(MG_MOUNTAINOUS, MG_UNUSED, MG_GREEN));
          map_set_placed(ptile);
        } else if (choose_hill) {
          /* hills */
          total_mtns++;
          tile_set_terrain(ptile,pick_terrain(MG_MOUNTAINOUS, MG_GREEN, MG_UNUSED));
          map_set_placed(ptile);
        }
      }
      if (total_mtns >= landarea * wld.map.server.steepness / 100) {
        break;
      }
    } whole_map_iterate_end;
  }
}
