/***********************************************************************
 Freeciv - Copyright (C) 1996-2007 - The Freeciv Project
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
#include "mapgen_topology.h"
#include "mapgen_utils.h"

#include "height_map.h"

int *height_map = NULL;
int hmap_shore_level = 0, hmap_mountain_level = 0;

/**********************************************************************//**
  Factor by which to lower height map near poles in normalize_hmap_poles
**************************************************************************/
static float hmap_pole_factor(struct tile *ptile)
{
  float factor = 1.0;

  if (near_singularity(ptile)) {
    /* Map edge near pole: clamp to what linear ramp would give us at pole
     * (maybe greater than 0) */
    factor = (100 - wld.map.server.flatpoles) / 100.0;
  } else if (wld.map.server.flatpoles > 0) {
    /* Linear ramp down from 100% at 2.5*ICE_BASE_LEVEL to (100-flatpoles) %
     * at the poles */
    factor = 1 - ((1 - (map_colatitude(ptile) / (2.5 * ICE_BASE_LEVEL)))
                  * wld.map.server.flatpoles / 100);
  }
  if (wld.map.server.separatepoles
      && map_colatitude(ptile) >= 2 * ICE_BASE_LEVEL) {
    /* A band of low height to try to separate the pole (this function is
     * only assumed to be called <= 2.5*ICE_BASE_LEVEL) */
    factor = MIN(factor, 0.1);
  }
  return factor;
}

/**********************************************************************//**
  Lower the land near the map edges and (optionally) the polar region to
  avoid too much land there.

  See also renormalize_hmap_poles
**************************************************************************/
void normalize_hmap_poles(void)
{
  whole_map_iterate(&(wld.map), ptile) {
    if (map_colatitude(ptile) <= 2.5 * ICE_BASE_LEVEL) {
      hmap(ptile) *= hmap_pole_factor(ptile);
    } else if (near_singularity(ptile)) {
      /* Near map edge but not near pole. */
      hmap(ptile) = 0;
    }
  } whole_map_iterate_end;
}

/**********************************************************************//**
  Invert (most of) the effects of normalize_hmap_poles so that we have
  accurate heights for texturing the poles.
**************************************************************************/
void renormalize_hmap_poles(void)
{
  whole_map_iterate(&(wld.map), ptile) {
    if (hmap(ptile) == 0) {
      /* Nothing left to restore. */
    } else if (map_colatitude(ptile) <= 2.5 * ICE_BASE_LEVEL) {
      float factor = hmap_pole_factor(ptile);

      if (factor > 0) {
        /* Invert the previously applied function */
        hmap(ptile) /= factor;
      }
    }
  } whole_map_iterate_end;
}

/**********************************************************************//**
  Create uncorrelated rand map and do some call to smoth to correlate
  it a little and create randoms shapes
**************************************************************************/
void make_random_hmap(int smooth)
{
  int i = 0;
  height_map = fc_malloc(sizeof(*height_map) * MAP_INDEX_SIZE);

  INITIALIZE_ARRAY(height_map, MAP_INDEX_SIZE, fc_rand(1000 * smooth));

  for (; i < smooth; i++) {
    smooth_int_map(height_map, TRUE);
  }

  adjust_int_map(height_map, hmap_max_level);
}

/**********************************************************************//**
  Recursive function which does the work for generator 5.

  All (x0,y0) and (x1,y1) are in native coordinates.
**************************************************************************/
static void gen5rec(int step, int xl, int yt, int xr, int yb)
{
  int val[2][2];
  int x1wrap = xr; /* to wrap correctly */ 
  int y1wrap = yb; 

  /* All x and y values are native. */

  if (((yb - yt <= 0) || (xr - xl <= 0)) 
      || ((yb - yt == 1) && (xr - xl == 1))) {
    return;
  }

  if (xr == wld.map.xsize) {
    x1wrap = 0;
  }
  if (yb == wld.map.ysize) {
    y1wrap = 0;
  }

  val[0][0] = hmap(native_pos_to_tile(&(wld.map), xl, yt));
  val[0][1] = hmap(native_pos_to_tile(&(wld.map), xl, y1wrap));
  val[1][0] = hmap(native_pos_to_tile(&(wld.map), x1wrap, yt));
  val[1][1] = hmap(native_pos_to_tile(&(wld.map), x1wrap, y1wrap));

  /* set midpoints of sides to avg of side's vertices plus a random factor */
  /* unset points are zero, don't reset if set */
#define set_midpoints(X, Y, V)						\
  {									\
    struct tile *ptile = native_pos_to_tile(&(wld.map), (X), (Y));	\
    if (map_colatitude(ptile) <= ICE_BASE_LEVEL / 2) {			\
      /* possibly flatten poles, or possibly not (even at map edge) */	\
      hmap(ptile) = (V) * (100 - wld.map.server.flatpoles) / 100;	\
    } else if (near_singularity(ptile)					\
               || hmap(ptile) != 0) {					\
      /* do nothing */							\
    } else {								\
      hmap(ptile) = (V);						\
    }									\
  }

  set_midpoints((xl + xr) / 2, yt,
                (val[0][0] + val[1][0]) / 2 + (int)fc_rand(step) - step / 2);
  set_midpoints((xl + xr) / 2,  y1wrap,
                (val[0][1] + val[1][1]) / 2 + (int)fc_rand(step) - step / 2);
  set_midpoints(xl, (yt + yb)/2,
                (val[0][0] + val[0][1]) / 2 + (int)fc_rand(step) - step / 2);
  set_midpoints(x1wrap,  (yt + yb) / 2,
                (val[1][0] + val[1][1]) / 2 + (int)fc_rand(step) - step / 2);

  /* set middle to average of midpoints plus a random factor, if not set */
  set_midpoints((xl + xr) / 2, (yt + yb) / 2,
                ((val[0][0] + val[0][1] + val[1][0] + val[1][1]) / 4
                 + (int)fc_rand(step) - step / 2));

#undef set_midpoints

  /* now call recursively on the four subrectangles */
  gen5rec(2 * step / 3, xl, yt, (xr + xl) / 2, (yb + yt) / 2);
  gen5rec(2 * step / 3, xl, (yb + yt) / 2, (xr + xl) / 2, yb);
  gen5rec(2 * step / 3, (xr + xl) / 2, yt, xr, (yb + yt) / 2);
  gen5rec(2 * step / 3, (xr + xl) / 2, (yb + yt) / 2, xr, yb);
}

/**********************************************************************//**
Generator 5 makes earthlike worlds with one or more large continents and
a scattering of smaller islands. It does so by dividing the world into
blocks and on each block raising or lowering the corners, then the 
midpoints and middle and so on recursively.  Fiddling with 'xdiv' and 
'ydiv' will change the size of the initial blocks and, if the map does not 
wrap in at least one direction, fiddling with 'avoidedge' will change the 
liklihood of continents butting up to non-wrapped edges.

  All X and Y values used in this function are in native coordinates.

  extra_div can be increased to break the world up into more, smaller
  islands.  This is used in conjunction with the startpos setting.
**************************************************************************/
void make_pseudofractal1_hmap(int extra_div)
{
  const bool xnowrap = !current_topo_has_flag(TF_WRAPX);
  const bool ynowrap = !current_topo_has_flag(TF_WRAPY);

  /* 
   * How many blocks should the x and y directions be divided into
   * initially. 
   */
  const int xdiv = 5 + extra_div;		
  const int ydiv = 5 + extra_div;

  int xdiv2 = xdiv + (xnowrap ? 1 : 0);
  int ydiv2 = ydiv + (ynowrap ? 1 : 0);

  int xmax = wld.map.xsize - (xnowrap ? 1 : 0);
  int ymax = wld.map.ysize - (ynowrap ? 1 : 0);
  int x_current, y_current;
  /* just need something > log(max(xsize, ysize)) for the recursion */
  int step = wld.map.xsize + wld.map.ysize; 
  /* edges are avoided more strongly as this increases */
  int avoidedge = (100 - wld.map.server.landpercent) * step / 100 + step / 3; 

  height_map = fc_malloc(sizeof(*height_map) * MAP_INDEX_SIZE);

  /* initialize map */
  INITIALIZE_ARRAY(height_map, MAP_INDEX_SIZE, 0);

  /* set initial points */
  for (x_current = 0; x_current < xdiv2; x_current++) {
    for (y_current = 0; y_current < ydiv2; y_current++) {
      do_in_map_pos(&(wld.map), ptile,
                    (x_current * xmax / xdiv), (y_current * ymax / ydiv)) {
        /* set initial points */
        hmap(ptile) = fc_rand(2 * step) - (2 * step) / 2;

	if (near_singularity(ptile)) {
	  /* avoid edges (topological singularities) */
	  hmap(ptile) -= avoidedge;
	}

	if (map_colatitude(ptile) <= ICE_BASE_LEVEL / 2 ) {
	  /* separate poles and avoid too much land at poles */
          hmap(ptile) -= fc_rand(avoidedge * wld.map.server.flatpoles / 100);
	}
      } do_in_map_pos_end;
    }
  }

  /* calculate recursively on each block */
  for (x_current = 0; x_current < xdiv; x_current++) {
    for (y_current = 0; y_current < ydiv; y_current++) {
      gen5rec(step, x_current * xmax / xdiv, y_current * ymax / ydiv, 
	      (x_current + 1) * xmax / xdiv, (y_current + 1) * ymax / ydiv);
    }
  }

  /* put in some random fuzz */
  whole_map_iterate(&(wld.map), ptile) {
    hmap(ptile) = 8 * hmap(ptile) + fc_rand(4) - 2;
  } whole_map_iterate_end;

  adjust_int_map(height_map, hmap_max_level);
}

/**********************************************************************//**
  We don't want huge areas of grass/plains,
  so we put in a hill here and there, where it gets too 'clean'

  Return TRUE if the terrain around the given map position is "clean".  This
  means that all the terrain for 2 squares around it is not mountain or hill.
**************************************************************************/
bool area_is_too_flat(struct tile *ptile, int thill, int my_height)
{
  int higher_than_me = 0;

  square_iterate(&(wld.map), ptile, 2, tile1) {
    if (hmap(tile1) > thill) {
      return FALSE;
    }
    if (hmap(tile1) > my_height) {
      if (map_distance(ptile, tile1) == 1) {
        return FALSE;
      }
      if (++higher_than_me > 2) {
        return FALSE;
      }
    }
  } square_iterate_end;

  if ((thill - hmap_shore_level) * higher_than_me
      > (my_height - hmap_shore_level) * 4) {
    return FALSE;
  }

  return TRUE;
}
