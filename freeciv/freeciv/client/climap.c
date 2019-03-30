/***********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Project
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

/* common */
#include "map.h"
#include "shared.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "tilespec.h"           /* tileset_is_isometric(tileset) */

/**********************************************************************//**
  A tile's "known" field is used by the server to store whether _each_
  player knows the tile.  Client-side, it's used as an enum known_type
  to track whether the tile is known/fogged/unknown.

  Judicious use of this function also makes things very convenient for
  civworld, since it uses both client and server-style storage; since it
  uses the stock tilespec.c file, this function serves as a wrapper.
**************************************************************************/
enum known_type client_tile_get_known(const struct tile *ptile)
{
  if (NULL == client.conn.playing) {
    if (client_is_observer()) {
      return TILE_KNOWN_SEEN;
    } else {
      return TILE_UNKNOWN;
    }
  }
  return tile_get_known(ptile, client.conn.playing);
}

/**********************************************************************//**
  Convert the given GUI direction into a map direction.

  GUI directions correspond to the current viewing interface, so that
  DIR8_NORTH is up on the mapview.  map directions correspond to the
  underlying map tiles, so that DIR8_NORTH means moving with a vector of
  (0,-1).  Neither necessarily corresponds to "north" on the underlying
  world (once iso-maps are possible).

  See also map_to_gui_dir().
**************************************************************************/
enum direction8 gui_to_map_dir(enum direction8 gui_dir)
{
  if (tileset_is_isometric(tileset)) {
    return dir_ccw(gui_dir);
  } else {
    return gui_dir;
  }
}

/**********************************************************************//**
  Convert the given GUI direction into a map direction.

  See also gui_to_map_dir().
**************************************************************************/
enum direction8 map_to_gui_dir(enum direction8 map_dir)
{
  if (tileset_is_isometric(tileset)) {
    return dir_cw(map_dir);
  } else {
    return map_dir;
  }
}

/**********************************************************************//**
  Client variant of city_tile().  This include the case of this could a
  ghost city (see client/packhand.c).  In a such case, the returned tile
  is an approximative position of the city on the map.
**************************************************************************/
struct tile *client_city_tile(const struct city *pcity)
{
  int dx, dy;
  double x = 0, y = 0;
  size_t num = 0;

  if (NULL == pcity) {
    return NULL;
  }

  if (NULL != city_tile(pcity)) {
    /* Normal city case. */
    return city_tile(pcity);
  }

  whole_map_iterate(&(wld.map), ptile) {
    int tile_x, tile_y;

    index_to_map_pos(&tile_x, &tile_y, tile_index(ptile));
    if (pcity == tile_worked(ptile)) {
      if (0 == num) {
        x = tile_x;
        y = tile_y;
        num = 1;
      } else {
        num++;
        base_map_distance_vector(&dx, &dy, (int)x, (int)y, tile_x, tile_y);
        x += (double) dx / num;
        y += (double) dy / num;
      }
    }
  } whole_map_iterate_end;

  if (0 < num) {
    return map_pos_to_tile(&(wld.map), (int) x, (int) y);
  } else {
    return NULL;
  }
}

/**********************************************************************//**
  Returns TRUE when a tile is available to be worked, or the city itself
  is currently working the tile (and can continue).

  See also city_can_work_tile() (common/city.[ch]).
**************************************************************************/
bool client_city_can_work_tile(const struct city *pcity,
                               const struct tile *ptile)
{
  return base_city_can_work_tile(client_player(), pcity, ptile);
}
