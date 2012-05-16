/********************************************************************** 
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__COLORS_COMMON_H
#define FC__COLORS_COMMON_H

#include "registry.h"

#include "fc_types.h"

/* The color system is designed on the assumption that almost, but
 * not quite, all displays will be truecolor. */

struct color;
struct color_system;
struct tileset;

enum color_std {
  /* Mapview colors */
  COLOR_MAPVIEW_UNKNOWN, /* Black */
  COLOR_MAPVIEW_CITYTEXT, /* white */
  COLOR_MAPVIEW_CITYGROWTH_BLOCKED, /* red */
  COLOR_MAPVIEW_GOTO, /* cyan */
  COLOR_MAPVIEW_SELECTION, /* yellow */
  COLOR_MAPVIEW_TRADEROUTE_LINE,
  COLOR_MAPVIEW_TRADEROUTES_ALL_BUILT, /* green */
  COLOR_MAPVIEW_TRADEROUTES_SOME_BUILT, /* yellow */
  COLOR_MAPVIEW_TRADEROUTES_NO_BUILT, /* red */
  COLOR_MAPVIEW_CITY_LINK, /* green */
  COLOR_MAPVIEW_TILE_LINK, /* red */
  COLOR_MAPVIEW_UNIT_LINK, /* cyan */

  /* Spaceship colors */
  COLOR_SPACESHIP_BACKGROUND, /* black */

  /* Overview colors */
  COLOR_OVERVIEW_UNKNOWN, /* Black */
  COLOR_OVERVIEW_MY_CITY, /* white */
  COLOR_OVERVIEW_ALLIED_CITY,
  COLOR_OVERVIEW_ENEMY_CITY, /* cyan */
  COLOR_OVERVIEW_MY_UNIT, /* yellow */
  COLOR_OVERVIEW_ALLIED_UNIT,
  COLOR_OVERVIEW_ENEMY_UNIT, /* red */
  COLOR_OVERVIEW_OCEAN, /* ocean/blue */
  COLOR_OVERVIEW_LAND, /* ground/green */
  COLOR_OVERVIEW_VIEWRECT, /* white */

  /* Reqtree colors */
  COLOR_REQTREE_RESEARCHING, /* cyan */
  COLOR_REQTREE_KNOWN, /* ground/green */
  COLOR_REQTREE_GOAL_PREREQS_KNOWN, /* race8 */
  COLOR_REQTREE_GOAL_UNKNOWN, /* race3 */
  COLOR_REQTREE_PREREQS_KNOWN, /* yellow */
  COLOR_REQTREE_UNKNOWN, /* red */
  COLOR_REQTREE_UNREACHABLE,
  COLOR_REQTREE_BACKGROUND, /* black */
  COLOR_REQTREE_TEXT, /* black */
  COLOR_REQTREE_EDGE, /* gray */

  /* Player dialog */
  COLOR_PLAYER_COLOR_BACKGROUND, /* black */

  COLOR_LAST
};

struct color *get_color(const struct tileset *t, enum color_std color);
struct color *get_player_color(const struct tileset *t,
			       const struct player *pplayer);
struct color *get_terrain_color(const struct tileset *t,
				const struct terrain *pterrain);

/* Functions used by the tileset to allocate the color system. */
struct color_system *color_system_read(struct section_file *file);
void color_system_setup_terrain(struct color_system *colors,
				const struct terrain *pterrain,
				const char *tag);
void color_system_free(struct color_system *colors);

#endif /* FC__COLORS_COMMON_H */
