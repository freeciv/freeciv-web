/********************************************************************** 
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
#ifndef FC__HEIGHT_MAP_H
#define FC__HEIGHT_MAP_H

/* Wrappers for easy access.  They are a macros so they can be a lvalues.*/
#define hmap(_tile) (height_map[tile_index(_tile)])

/* shore_level safe unit of height */
#define H_UNIT MIN(1, (hmap_max_level - hmap_shore_level) / 100)

/*
 * Height map information
 *
 *   height_map[] stores the height of each tile
 *   hmap_max_level is the maximum height (heights will range from
 *     [0,hmap_max_level).
 *   hmap_shore_level is the level of ocean.  Any tile at this height or
 *     above is land; anything below is ocean.
 *   hmap_mount_level is the level of mountains and hills.  Any tile above
 *     this height will usually be a mountain or hill.
 */
#define hmap_max_level 1000
extern int *height_map;
extern int hmap_shore_level, hmap_mountain_level;

void normalize_hmap_poles(void);
void renormalize_hmap_poles(void);
void make_random_hmap(int smooth);
void make_pseudofractal1_hmap(int extra_div);

#endif  /* FC__HEIGHT__MAP_H */
