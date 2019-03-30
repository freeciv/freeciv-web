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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "registry.h"

/* common */
#include "fc_types.h"

/* The color system is designed on the assumption that almost, but
 * not quite, all displays will be truecolor. */

struct color;
struct color_system;
struct tileset;
struct rgbcolor;

#define SPECENUM_NAME color_std
/* Mapview colors */
#define SPECENUM_VALUE0   COLOR_MAPVIEW_UNKNOWN                 /* Black */
#define SPECENUM_VALUE0NAME   "mapview_unknown"
#define SPECENUM_VALUE1   COLOR_MAPVIEW_CITYTEXT                /* white */
#define SPECENUM_VALUE1NAME   "mapview_citytext"
#define SPECENUM_VALUE2   COLOR_MAPVIEW_CITYTEXT_DARK           /* black */
#define SPECENUM_VALUE2NAME   "mapview_citytext_dark"
#define SPECENUM_VALUE3   COLOR_MAPVIEW_CITYPROD_NEGATIVE        /* red */
#define SPECENUM_VALUE3NAME   "mapview_cityprod_negative"
#define SPECENUM_VALUE4   COLOR_MAPVIEW_CITYGROWTH_BLOCKED      /* red */
#define SPECENUM_VALUE4NAME   "mapview_cityblocked"
#define SPECENUM_VALUE5   COLOR_MAPVIEW_GOTO                    /* cyan */
#define SPECENUM_VALUE5NAME   "mapview_goto"
#define SPECENUM_VALUE6   COLOR_MAPVIEW_SELECTION               /* yellow */
#define SPECENUM_VALUE6NAME   "mapview_selection"
#define SPECENUM_VALUE7   COLOR_MAPVIEW_TRADE_ROUTE_LINE
#define SPECENUM_VALUE7NAME   "mapview_trade_route_line"
#define SPECENUM_VALUE8   COLOR_MAPVIEW_TRADE_ROUTES_ALL_BUILT  /* green */
#define SPECENUM_VALUE8NAME   "mapview_trade_routes_all_built"
#define SPECENUM_VALUE9   COLOR_MAPVIEW_TRADE_ROUTES_SOME_BUILT /* yellow */
#define SPECENUM_VALUE9NAME   "mapview_trade_routes_some_built"
#define SPECENUM_VALUE10  COLOR_MAPVIEW_TRADE_ROUTES_NO_BUILT   /* red */
#define SPECENUM_VALUE10NAME   "mapview_trade_routes_no_built"
#define SPECENUM_VALUE11  COLOR_MAPVIEW_CITY_LINK               /* green */
#define SPECENUM_VALUE11NAME  "mapview_city_link"
#define SPECENUM_VALUE12  COLOR_MAPVIEW_TILE_LINK               /* red */
#define SPECENUM_VALUE12NAME  "mapview_tile_link"
#define SPECENUM_VALUE13  COLOR_MAPVIEW_UNIT_LINK               /* cyan */
#define SPECENUM_VALUE13NAME  "mapview_unit_link"
/* Spaceship colors */
#define SPECENUM_VALUE14   COLOR_SPACESHIP_BACKGROUND            /* black */
#define SPECENUM_VALUE14NAME  "spaceship_background"
/* Overview colors */
#define SPECENUM_VALUE15  COLOR_OVERVIEW_UNKNOWN      /* Black */
#define SPECENUM_VALUE15NAME  "overview_unknown"
#define SPECENUM_VALUE16  COLOR_OVERVIEW_MY_CITY      /* white */
#define SPECENUM_VALUE16NAME  "overview_mycity"
#define SPECENUM_VALUE17  COLOR_OVERVIEW_ALLIED_CITY
#define SPECENUM_VALUE17NAME  "overview_alliedcity"
#define SPECENUM_VALUE18  COLOR_OVERVIEW_ENEMY_CITY   /* cyan */
#define SPECENUM_VALUE18NAME  "overview_enemycity"
#define SPECENUM_VALUE19  COLOR_OVERVIEW_MY_UNIT      /* yellow */
#define SPECENUM_VALUE19NAME  "overview_myunit"
#define SPECENUM_VALUE20  COLOR_OVERVIEW_ALLIED_UNIT
#define SPECENUM_VALUE20NAME  "overview_alliedunit"
#define SPECENUM_VALUE21  COLOR_OVERVIEW_ENEMY_UNIT   /* red */
#define SPECENUM_VALUE21NAME  "overview_enemyunit"
#define SPECENUM_VALUE22  COLOR_OVERVIEW_OCEAN        /* ocean/blue */
#define SPECENUM_VALUE22NAME  "overview_ocean"
#define SPECENUM_VALUE23  COLOR_OVERVIEW_LAND         /* ground/green */
#define SPECENUM_VALUE23NAME  "overview_ground"
#define SPECENUM_VALUE24  COLOR_OVERVIEW_FROZEN       /* frozen/grey */
#define SPECENUM_VALUE24NAME  "overview_frozen"
#define SPECENUM_VALUE25  COLOR_OVERVIEW_VIEWRECT     /* white */
#define SPECENUM_VALUE25NAME  "overview_viewrect"
/* Reqtree colors */
#define SPECENUM_VALUE26  COLOR_REQTREE_RESEARCHING         /* cyan */
#define SPECENUM_VALUE26NAME  "reqtree_researching"
#define SPECENUM_VALUE27  COLOR_REQTREE_KNOWN               /* green */
#define SPECENUM_VALUE27NAME  "reqtree_known"
#define SPECENUM_VALUE28  COLOR_REQTREE_GOAL_PREREQS_KNOWN
#define SPECENUM_VALUE28NAME  "reqtree_goal_prereqs_known"
#define SPECENUM_VALUE29  COLOR_REQTREE_GOAL_UNKNOWN
#define SPECENUM_VALUE29NAME  "reqtree_goal_unknown"
#define SPECENUM_VALUE30  COLOR_REQTREE_PREREQS_KNOWN       /* yellow */
#define SPECENUM_VALUE30NAME  "reqtree_prereqs_known"
#define SPECENUM_VALUE31  COLOR_REQTREE_UNKNOWN             /* red */
#define SPECENUM_VALUE31NAME  "reqtree_unknown"
#define SPECENUM_VALUE32  COLOR_REQTREE_UNREACHABLE
#define SPECENUM_VALUE32NAME  "reqtree_unreachable"
#define SPECENUM_VALUE33  COLOR_REQTREE_NOT_GETTABLE
#define SPECENUM_VALUE33NAME  "reqtree_not_gettable"
#define SPECENUM_VALUE34  COLOR_REQTREE_GOAL_NOT_GETTABLE
#define SPECENUM_VALUE34NAME  "reqtree_goal_not_gettable"
#define SPECENUM_VALUE35  COLOR_REQTREE_BACKGROUND          /* black */
#define SPECENUM_VALUE35NAME  "reqtree_background"
#define SPECENUM_VALUE36  COLOR_REQTREE_TEXT                /* black */
#define SPECENUM_VALUE36NAME  "reqtree_text"
#define SPECENUM_VALUE37  COLOR_REQTREE_EDGE                /* gray */
#define SPECENUM_VALUE37NAME  "reqtree_edge"
/* Player dialog */
#define SPECENUM_VALUE38  COLOR_PLAYER_COLOR_BACKGROUND     /* black */
#define SPECENUM_VALUE38NAME  "playerdlg_background"

#define SPECENUM_COUNT    COLOR_LAST
#include "specenum_gen.h"

struct color *get_color(const struct tileset *t, enum color_std stdcolor);
bool player_has_color(const struct tileset *t,
                      const struct player *pplayer);
struct color *get_player_color(const struct tileset *t,
                               const struct player *pplayer);
struct color *get_terrain_color(const struct tileset *t,
                                const struct terrain *pterrain);
struct color *ensure_color(struct rgbcolor *rgb);

/* Functions used by the tileset to allocate the color system. */
struct color_system *color_system_read(struct section_file *file);
void color_system_free(struct color_system *colors);

/* Utilities for color values */
struct color *color_best_contrast(struct color *subject,
                                  struct color **candidates, int ncandidates);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__COLORS_COMMON_H */
