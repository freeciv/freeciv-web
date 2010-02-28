/********************************************************************** 
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
#include <config.h>
#endif

#include <assert.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"

/* common */
#include "base.h"
#include "borders.h"
#include "events.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "packets.h"
#include "unit.h"
#include "unitlist.h"
#include "vision.h"

/* generator */
#include "utilities.h"

/* server */
#include "citytools.h"
#include "cityturn.h"
#include "notify.h"
#include "plrhand.h"
#include "sernet.h"
#include "srv_main.h"
#include "unithand.h"
#include "unittools.h"

#include "maphand.h"

#define MAXIMUM_CLAIMED_OCEAN_SIZE (20)

/* Suppress send_tile_info() during game_load() */
static bool send_tile_suppressed = FALSE;

static void player_tile_init(struct tile *ptile, struct player *pplayer);
static void give_tile_info_from_player_to_player(struct player *pfrom,
						 struct player *pdest,
						 struct tile *ptile);
static void shared_vision_change_seen(struct tile *ptile,
				      struct player *pplayer, int change,
				      enum vision_layer vlayer);
static int map_get_seen(const struct tile *ptile,
			const struct player *pplayer,
			enum vision_layer vlayer);
static void map_change_own_seen(struct tile *ptile, struct player *pplayer,
				int change, enum vision_layer vlayer);

/**************************************************************************
Used only in global_warming() and nuclear_winter() below.
**************************************************************************/
static bool is_terrain_ecologically_wet(struct tile *ptile)
{
  return (is_ocean_near_tile(ptile)
	  || is_special_near_tile(ptile, S_RIVER, TRUE));
}

/**************************************************************************
...
**************************************************************************/
void global_warming(int effect)
{
  int k;

  freelog(LOG_VERBOSE, "Global warming: %d", game.info.heating);

  k = map_num_tiles();
  while(effect > 0 && (k--) > 0) {
    struct terrain *old, *new;
    struct tile *ptile;

    ptile = rand_map_pos();
    old = tile_terrain(ptile);
    if (is_terrain_ecologically_wet(ptile)) {
      new = old->warmer_wetter_result;
    } else {
      new = old->warmer_drier_result;
    }
    if (new != T_NONE && old != new) {
      effect--;
      tile_change_terrain(ptile, new);
      check_terrain_change(ptile, old);
      update_tile_knowledge(ptile);
      unit_list_iterate(ptile->units, punit) {
	if (!can_unit_continue_current_activity(punit)) {
	  unit_activity_handling(punit, ACTIVITY_IDLE);
	}
      } unit_list_iterate_end;
    } else if (old == new) {
      /* This counts toward warming although nothing is changed. */
      effect--;
    }
  }

  notify_player(NULL, NULL, E_GLOBAL_ECO, FTC_SERVER_INFO, NULL,
                _("Global warming has occurred!"));
  notify_player(NULL, NULL, E_GLOBAL_ECO, FTC_SERVER_INFO, NULL,
		_("Coastlines have been flooded and vast "
		  "ranges of grassland have become deserts."));
}

/**************************************************************************
...
**************************************************************************/
void nuclear_winter(int effect)
{
  int k;

  freelog(LOG_VERBOSE, "Nuclear winter: %d", game.info.cooling);

  k = map_num_tiles();
  while(effect > 0 && (k--) > 0) {
    struct terrain *old, *new;
    struct tile *ptile;

    ptile = rand_map_pos();
    old = tile_terrain(ptile);
    if (is_terrain_ecologically_wet(ptile)) {
      new = old->cooler_wetter_result;
    } else {
      new = old->cooler_drier_result;
    }
    if (new != T_NONE && old != new) {
      effect--;
      tile_change_terrain(ptile, new);
      check_terrain_change(ptile, old);
      update_tile_knowledge(ptile);
      unit_list_iterate(ptile->units, punit) {
	if (!can_unit_continue_current_activity(punit)) {
	  unit_activity_handling(punit, ACTIVITY_IDLE);
	}
      } unit_list_iterate_end;
    } else if (old == new) {
      /* This counts toward winter although nothing is changed. */
      effect--;
    }
  }

  notify_player(NULL, NULL, E_GLOBAL_ECO, FTC_SERVER_INFO, NULL,
                _("Nuclear winter has occurred!"));
  notify_player(NULL, NULL, E_GLOBAL_ECO, FTC_SERVER_INFO, NULL,
		_("Wetlands have dried up and vast "
		  "ranges of grassland have become tundra."));
}

/***************************************************************
To be called when a player gains the Railroad tech for the first
time.  Sends a message, and then upgrade all city squares to
railroads.  "discovery" just affects the message: set to
   1 if the tech is a "discovery",
   0 if otherwise acquired (conquer/trade/GLib).        --dwp
***************************************************************/
void upgrade_city_rails(struct player *pplayer, bool discovery)
{
  if (!(terrain_control.may_road)) {
    return;
  }

  conn_list_do_buffer(pplayer->connections);

  if (discovery) {
    notify_player(pplayer, NULL, E_TECH_GAIN, FTC_SERVER_INFO, NULL,
		  _("New hope sweeps like fire through the country as "
		    "the discovery of railroad is announced.\n"
		    "      Workers spontaneously gather and upgrade all "
		    "cities with railroads."));
  } else {
    notify_player(pplayer, NULL, E_TECH_GAIN, FTC_SERVER_INFO, NULL,
		  _("The people are pleased to hear that your "
		    "scientists finally know about railroads.\n"
		    "      Workers spontaneously gather and upgrade all "
		    "cities with railroads."));
  }
  
  city_list_iterate(pplayer->cities, pcity) {
    tile_set_special(pcity->tile, S_RAILROAD);
    update_tile_knowledge(pcity->tile);
  }
  city_list_iterate_end;

  conn_list_do_unbuffer(pplayer->connections);
}

/**************************************************************************
Return TRUE iff the player me really gives shared vision to player them.
**************************************************************************/
bool really_gives_vision(struct player *me, struct player *them)
{
  return TEST_BIT(me->really_gives_vision, player_index(them));
}

/**************************************************************************
...
**************************************************************************/
static void buffer_shared_vision(struct player *pplayer)
{
  players_iterate(pplayer2) {
    if (really_gives_vision(pplayer, pplayer2))
      conn_list_do_buffer(pplayer2->connections);
  } players_iterate_end;
  conn_list_do_buffer(pplayer->connections);
}

/**************************************************************************
...
**************************************************************************/
static void unbuffer_shared_vision(struct player *pplayer)
{
  players_iterate(pplayer2) {
    if (really_gives_vision(pplayer, pplayer2))
      conn_list_do_unbuffer(pplayer2->connections);
  } players_iterate_end;
  conn_list_do_unbuffer(pplayer->connections);
}

/**************************************************************************
...
**************************************************************************/
void give_map_from_player_to_player(struct player *pfrom, struct player *pdest)
{
  buffer_shared_vision(pdest);

  whole_map_iterate(ptile) {
    give_tile_info_from_player_to_player(pfrom, pdest, ptile);
  } whole_map_iterate_end;

  unbuffer_shared_vision(pdest);
  city_thaw_workers_queue();
  sync_cities();
}

/**************************************************************************
...
**************************************************************************/
void give_seamap_from_player_to_player(struct player *pfrom, struct player *pdest)
{
  buffer_shared_vision(pdest);

  whole_map_iterate(ptile) {
    if (is_ocean_tile(ptile)) {
      give_tile_info_from_player_to_player(pfrom, pdest, ptile);
    }
  } whole_map_iterate_end;

  unbuffer_shared_vision(pdest);
  city_thaw_workers_queue();
  sync_cities();
}

/**************************************************************************
...
**************************************************************************/
void give_citymap_from_player_to_player(struct city *pcity,
					struct player *pfrom, struct player *pdest)
{
  struct tile *pcenter = city_tile(pcity);

  buffer_shared_vision(pdest);

  city_tile_iterate(pcenter, ptile) {
    give_tile_info_from_player_to_player(pfrom, pdest, ptile);
  } city_tile_iterate_end;

  unbuffer_shared_vision(pdest);
  city_thaw_workers_queue();
  sync_cities();
}

/**************************************************************************
  Send all tiles known to specified clients.
  If dest is NULL means game.est_connections.
  
  Note for multiple connections this may change "sent" multiple times
  for single player.  This is ok, because "sent" data is just optimised
  calculations, so it will be correct before this, for each connection
  during this, and at end.
**************************************************************************/
void send_all_known_tiles(struct conn_list *dest, bool force)
{
  int tiles_sent;

  if (!dest) {
    dest = game.est_connections;
  }

  /* send whole map piece by piece to each player to balance the load
     of the send buffers better */
  tiles_sent = 0;
  conn_list_do_buffer(dest);

  whole_map_iterate(ptile) {
    tiles_sent++;
    if ((tiles_sent % map.xsize) == 0) {
      conn_list_do_unbuffer(dest);
      flush_packets();
      conn_list_do_buffer(dest);
    }

    send_tile_info(dest, ptile, FALSE, force);
  } whole_map_iterate_end;

  conn_list_do_unbuffer(dest);
  flush_packets();
}

/**************************************************************************
  Suppress send_tile_info() during game_load()
**************************************************************************/
bool send_tile_suppression(bool now)
{
  bool formerly = send_tile_suppressed;

  send_tile_suppressed = now;
  return formerly;
}

/**************************************************************************
  Send tile information to all the clients in dest which know and see
  the tile. If dest is NULL, sends to all clients (game.est_connections)
  which know and see tile.

  Note that this function does not update the playermap.  For that call
  update_tile_knowledge().
**************************************************************************/
void send_tile_info(struct conn_list *dest, struct tile *ptile,
                    bool send_unknown, bool force)
{
  struct packet_tile_info info;
  const struct nation_type *pnation;
  const struct player *owner;

  if (send_tile_suppressed) {
    return;
  }

  if (!dest) {
    dest = game.est_connections;
  }

  info.x = ptile->x;
  info.y = ptile->y;

  if (ptile->spec_sprite) {
    sz_strlcpy(info.spec_sprite, ptile->spec_sprite);
  } else {
    info.spec_sprite[0] = '\0';
  }

  pnation = map_get_startpos(ptile);
  info.nation_start = pnation ? nation_number(pnation) : -1;

  info.special[S_OLD_FORTRESS] = FALSE;
  info.special[S_OLD_AIRBASE] = FALSE;

  conn_list_iterate(dest, pconn) {
    struct player *pplayer = pconn->playing;

    if (NULL == pplayer && !pconn->observer) {
      continue;
    }

    if (!pplayer || map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
      info.known = TILE_KNOWN_SEEN;
      info.continent = tile_continent(ptile);
      owner = tile_owner(ptile);
      info.owner = (owner ? player_number(owner) : MAP_TILE_OWNER_NULL);
      info.worked = (NULL != tile_worked(ptile))
                     ? tile_worked(ptile)->id
                     : IDENTITY_NUMBER_ZERO;

      info.terrain = (NULL != tile_terrain(ptile))
                      ? terrain_number(tile_terrain(ptile))
                      : terrain_count();
      info.resource = (NULL != tile_resource(ptile))
                       ? resource_number(tile_resource(ptile))
                       : resource_count();

      tile_special_type_iterate(spe) {
	info.special[spe] = BV_ISSET(ptile->special, spe);
      } tile_special_type_iterate_end;
      info.bases = ptile->bases;

      send_packet_tile_info(pconn, force, &info);
    } else if (pplayer && map_is_known(ptile, pplayer)
	       && map_get_seen(ptile, pplayer, V_MAIN) == 0) {
      struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);
      struct vision_site *psite = map_get_player_site(ptile, pplayer);

      info.known = TILE_KNOWN_UNSEEN;
      info.continent = tile_continent(ptile);
      owner = (game.server.foggedborders
               ? plrtile->owner
               : tile_owner(ptile));
      info.owner = (owner ? player_number(owner) : MAP_TILE_OWNER_NULL);
      info.worked = (NULL != psite)
                    ? psite->identity
                    : IDENTITY_NUMBER_ZERO;

      info.terrain = (NULL != plrtile->terrain)
                      ? terrain_number(plrtile->terrain)
                      : terrain_count();
      info.resource = (NULL != plrtile->resource)
                       ? resource_number(plrtile->resource)
                       : resource_count();

      tile_special_type_iterate(spe) {
	info.special[spe] = BV_ISSET(plrtile->special, spe);
      } tile_special_type_iterate_end;
      info.bases = plrtile->bases;

      send_packet_tile_info(pconn, force, &info);
    } else if (send_unknown) {
      info.known = TILE_UNKNOWN;
      info.continent = 0;
      info.owner = MAP_TILE_OWNER_NULL;
      info.worked = IDENTITY_NUMBER_ZERO;

      info.terrain = terrain_count();
      info.resource = resource_count();

      tile_special_type_iterate(spe) {
        info.special[spe] = FALSE;
      } tile_special_type_iterate_end;
      BV_CLR_ALL(info.bases);

      send_packet_tile_info(pconn, force, &info);
    }
  }
  conn_list_iterate_end;
}

/****************************************************************************
  Assumption: Each unit type is visible on only one layer.
****************************************************************************/
static bool unit_is_visible_on_layer(const struct unit *punit,
				     enum vision_layer vlayer)
{
  return XOR(vlayer == V_MAIN, is_hiding_unit(punit));
}

/****************************************************************************
  This is a backend function that marks a tile as unfogged.  Call this when
  map_unfog_tile adds the first point of visibility to the tile, or when
  shared vision changes cause a tile to become unfogged.
****************************************************************************/
static void really_unfog_tile(struct player *pplayer, struct tile *ptile,
			      enum vision_layer vlayer)
{
  struct city *pcity;

  freelog(LOG_DEBUG, "really unfogging %d,%d\n", TILE_XY(ptile));

  map_set_known(ptile, pplayer);

  if (vlayer == V_MAIN) {
    /* send info about the tile itself 
     * It has to be sent first because the client needs correct
     * continent number before it can handle following packets
     */
    update_player_tile_knowledge(pplayer, ptile);
    send_tile_info(pplayer->connections, ptile, FALSE, FALSE);
    /* NOTE: because the V_INVIS case doesn't fall into this if statement,
     * changes to V_INVIS fogging won't send a new info packet to the client
     * and the client's tile_seen[V_INVIS] bitfield may end up being out
     * of date. */
  }

  /* discover units */
  unit_list_iterate(ptile->units, punit) {
    if (unit_is_visible_on_layer(punit, vlayer)) {
      send_unit_info(pplayer, punit);
    }
  } unit_list_iterate_end;

  if (vlayer == V_MAIN) {
    /* discover cities */ 
    reality_check_city(pplayer, ptile);

    if (NULL != (pcity = tile_city(ptile))) {
      send_city_info(pplayer, pcity);
    }
  }
}

/****************************************************************************
  Add an extra point of visibility to the given tile.  pplayer may not be
  NULL.  The caller may wish to buffer_shared_vision if calling this
  function multiple times.
****************************************************************************/
static void map_unfog_tile(struct player *pplayer, struct tile *ptile,
			   bool can_reveal_tiles,
			   enum vision_layer vlayer)
{
  /* Increase seen count. */
  shared_vision_change_seen(ptile, pplayer, +1, vlayer);

  /* And then give the vision.  Did the tile just become visible?
   * Then send info about units and cities and the tile itself. */
  players_iterate(pplayer2) {
    if (pplayer2 == pplayer || really_gives_vision(pplayer, pplayer2)) {
      bool known = map_is_known(ptile, pplayer2);

      /* When fog of war is disabled, the seen count is always at least 1. */
      if ((!known && can_reveal_tiles)
          || (known && (map_get_seen(ptile, pplayer2, vlayer)
                        == 1 + !game.info.fogofwar))) {
	really_unfog_tile(pplayer2, ptile, vlayer);
      }
    }
  } players_iterate_end;
}

/****************************************************************************
  This is a backend function that marks a tile as fogged.  Call this when
  map_fog_tile removes the last point of visibility from the tile, or when
  shared vision changes cause a tile to become fogged.
****************************************************************************/
static void really_fog_tile(struct player *pplayer, struct tile *ptile,
			    enum vision_layer vlayer)
{
  freelog(LOG_DEBUG, "Fogging %i,%i. Previous fog: %i.",
	  TILE_XY(ptile), map_get_seen(ptile, pplayer, vlayer));
 
  assert(map_get_seen(ptile, pplayer, vlayer) == 0);

  unit_list_iterate(ptile->units, punit)
    if (unit_is_visible_on_layer(punit, vlayer)) {
      unit_goes_out_of_sight(pplayer,punit);
    }
  unit_list_iterate_end;  

  if (vlayer == V_MAIN) {
    update_player_tile_last_seen(pplayer, ptile);
    send_tile_info(pplayer->connections, ptile, FALSE, FALSE);
  }
}

/**************************************************************************
  Remove a point of visibility from the given tile.  pplayer may not be
  NULL.  The caller may wish to buffer_shared_vision if calling this
  function multiple times.
**************************************************************************/
static void map_fog_tile(struct player *pplayer, struct tile *ptile,
			 enum vision_layer vlayer)
{
  shared_vision_change_seen(ptile, pplayer, -1, vlayer);

  if (map_is_known(ptile, pplayer)) {
    players_iterate(pplayer2) {
      if (pplayer2 == pplayer || really_gives_vision(pplayer, pplayer2)) {
        if (map_get_seen(ptile, pplayer2, vlayer) == 0) {
          if (game.server.foggedborders) {
            struct player_tile *plrtile = map_get_player_tile(ptile,
                                                              pplayer2);
            plrtile->owner = tile_owner(ptile);
          }
          really_fog_tile(pplayer2, ptile, vlayer);
        }
      }
    } players_iterate_end;
  }
}

/**************************************************************************
  Send basic map information: map size, topology, and is_earth.
**************************************************************************/
void send_map_info(struct conn_list *dest)
{
  struct packet_map_info minfo;

  minfo.xsize=map.xsize;
  minfo.ysize=map.ysize;
  minfo.topology_id = map.topology_id;
 
  lsend_packet_map_info(dest, &minfo);
}

/**************************************************************************
...
**************************************************************************/
static void shared_vision_change_seen(struct tile *ptile,
				      struct player *pplayer, int change,
				      enum vision_layer vlayer)
{
  map_change_seen(ptile, pplayer, change, vlayer);
  map_change_own_seen(ptile, pplayer, change, vlayer);

  players_iterate(pplayer2) {
    if (really_gives_vision(pplayer, pplayer2))
      map_change_seen(ptile, pplayer2, change, vlayer);
  } players_iterate_end;
}

/**************************************************************************
  There doesn't have to be a city.
**************************************************************************/
void map_refog_circle(struct player *pplayer, struct tile *ptile,
                      int old_radius_sq, int new_radius_sq,
                      bool can_reveal_tiles,
                      enum vision_layer vlayer)
{
  if (old_radius_sq != new_radius_sq) {
    int max_radius = MAX(old_radius_sq, new_radius_sq);

    freelog(LOG_DEBUG, "Refogging circle at %d,%d from %d to %d",
	    TILE_XY(ptile), old_radius_sq, new_radius_sq);

    buffer_shared_vision(pplayer);
    circle_dxyr_iterate(ptile, max_radius, tile1, dx, dy, dr) {
      if (dr > old_radius_sq && dr <= new_radius_sq) {
	map_unfog_tile(pplayer, tile1, can_reveal_tiles, vlayer);
      } else if (dr > new_radius_sq && dr <= old_radius_sq) {
	map_fog_tile(pplayer, tile1, vlayer);
      }
    } circle_dxyr_iterate_end;
    unbuffer_shared_vision(pplayer);
  }
}

/****************************************************************************
  Shows the area to the player.  Unless the tile is "seen", it will remain
  fogged and units will be hidden.

  Callers may wish to buffer_shared_vision before calling this function.
****************************************************************************/
void map_show_tile(struct player *src_player, struct tile *ptile)
{
  static int recurse = 0;
  freelog(LOG_DEBUG, "Showing %i,%i to %s",
	  TILE_XY(ptile), player_name(src_player));

  assert(recurse == 0);
  recurse++;

  players_iterate(pplayer) {
    if (pplayer == src_player || really_gives_vision(pplayer, src_player)) {
      struct city *pcity;

      if (!map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
	map_set_known(ptile, pplayer);

	/* as the tile may be fogged send_tile_info won't always do this for us */
	update_player_tile_knowledge(pplayer, ptile);
	update_player_tile_last_seen(pplayer, ptile);

	send_tile_info(pplayer->connections, ptile, FALSE, FALSE);

	/* remove old cities that exist no more */
	reality_check_city(pplayer, ptile);

	if ((pcity = tile_city(ptile))) {
	  /* as the tile may be fogged send_city_info won't do this for us */
	  update_dumb_city(pplayer, pcity);
	  send_city_info(pplayer, pcity);
	}

	vision_layer_iterate(v) {
	  if (map_get_seen(ptile, pplayer, v) != 0) {
	    unit_list_iterate(ptile->units, punit)
	      if (unit_is_visible_on_layer(punit, v)) {
		send_unit_info(pplayer, punit);
	      }
	    unit_list_iterate_end;
	  }
	} vision_layer_iterate_end;
      }
    }
  } players_iterate_end;

  recurse--;
}

/****************************************************************************
  Shows the area to the player.  Unless the tile is "seen", it will remain
  fogged and units will be hidden.
****************************************************************************/
void map_show_circle(struct player *pplayer, struct tile *ptile, int radius_sq)
{
  buffer_shared_vision(pplayer);

  circle_iterate(ptile, radius_sq, tile1) {
    map_show_tile(pplayer, tile1);
  } circle_iterate_end;

  unbuffer_shared_vision(pplayer);
}

/****************************************************************************
  Shows the area to the player.  Unless the tile is "seen", it will remain
  fogged and units will be hidden.
****************************************************************************/
void map_show_all(struct player *pplayer)
{
  buffer_shared_vision(pplayer);

  whole_map_iterate(ptile) {
    map_show_tile(pplayer, ptile);
  } whole_map_iterate_end;

  unbuffer_shared_vision(pplayer);
}

/****************************************************************************
  Return whether the player knows the tile.  Knowing a tile means you've
  seen it once (as opposed to seeing a tile which means you can see it now).
****************************************************************************/
bool map_is_known(const struct tile *ptile, const struct player *pplayer)
{
  return BV_ISSET(ptile->tile_known, player_index(pplayer));
}

/***************************************************************
...
***************************************************************/
bool map_is_known_and_seen(const struct tile *ptile, struct player *pplayer,
			   enum vision_layer vlayer)
{
  assert(!game.info.fogofwar
	 || (BV_ISSET(ptile->tile_seen[vlayer], player_index(pplayer))
	     == (map_get_player_tile(ptile, pplayer)->seen_count[vlayer]
		 > 0)));
  return (BV_ISSET(ptile->tile_known, player_index(pplayer))
	  && BV_ISSET(ptile->tile_seen[vlayer], player_index(pplayer)));
}

/****************************************************************************
  Return whether the player can see the tile.  Seeing a tile means you have
  vision of it now (as opposed to knowing a tile which means you've seen it
  before).  Note that a tile can be seen but not known (currently this only
  happens when a city is founded with some unknown tiles in its radius); in
  this case the tile is unknown (but map_get_seen will still return TRUE).
****************************************************************************/
static int map_get_seen(const struct tile *ptile,
			const struct player *pplayer,
			enum vision_layer vlayer)
{
  assert(!game.info.fogofwar
	 || (BV_ISSET(ptile->tile_seen[vlayer], player_index(pplayer))
	     == (map_get_player_tile(ptile, pplayer)->seen_count[vlayer]
		 > 0)));
  return map_get_player_tile(ptile, pplayer)->seen_count[vlayer];
}

/***************************************************************
...
***************************************************************/
void map_change_seen(struct tile *ptile, struct player *pplayer, int change,
		     enum vision_layer vlayer)
{
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

  /* assert to avoid underflow */
  assert(0 <= change || -change <= plrtile->seen_count[vlayer]);

  plrtile->seen_count[vlayer] += change;
  if (plrtile->seen_count[vlayer] != 0) {
    BV_SET(ptile->tile_seen[vlayer], player_index(pplayer));
  } else {
    BV_CLR(ptile->tile_seen[vlayer], player_index(pplayer));
  }
  freelog(LOG_DEBUG, "%d,%d, p: %d, change %d, result %d\n", TILE_XY(ptile),
	  player_number(pplayer), change, plrtile->seen_count[vlayer]);
}

/***************************************************************
...
***************************************************************/
static int map_get_own_seen(struct tile *ptile, struct player *pplayer,
			    enum vision_layer vlayer)
{
  return map_get_player_tile(ptile, pplayer)->own_seen[vlayer];
}

/***************************************************************
...
***************************************************************/
static void map_change_own_seen(struct tile *ptile, struct player *pplayer,
				int change,
				enum vision_layer vlayer)
{
  map_get_player_tile(ptile, pplayer)->own_seen[vlayer] += change;
}

/***************************************************************
 Changes site information for player tile.
***************************************************************/
void change_playertile_site(struct player_tile *ptile,
                            struct vision_site *new_site)
{
  if (ptile->site == new_site) {
    /* Do nothing. */
    return;
  }

  if (ptile->site != NULL) {
    /* Releasing old site from tile */
    free_vision_site(ptile->site);
  }

  ptile->site = new_site;
}

/***************************************************************
...
***************************************************************/
void map_set_known(struct tile *ptile, struct player *pplayer)
{
  BV_SET(ptile->tile_known, player_index(pplayer));
  vision_layer_iterate(v) {
    if (map_get_player_tile(ptile, pplayer)->seen_count[v] > 0) {
      BV_SET(ptile->tile_seen[v], player_index(pplayer));
    }
  } vision_layer_iterate_end;
}

/***************************************************************
...
***************************************************************/
void map_clear_known(struct tile *ptile, struct player *pplayer)
{
  BV_CLR(ptile->tile_known, player_index(pplayer));
}

/****************************************************************************
  Call this function to unfog all tiles.  This should only be called when
  a player dies or at the end of the game as it will result in permanent
  vision of the whole map.
****************************************************************************/
void map_know_and_see_all(struct player *pplayer)
{
  buffer_shared_vision(pplayer);

  whole_map_iterate(ptile) {
    vision_layer_iterate(v) {
      map_unfog_tile(pplayer, ptile, TRUE, v);
    } vision_layer_iterate_end;
  } whole_map_iterate_end;

  unbuffer_shared_vision(pplayer);
}

/**************************************************************************
  Unfogs all tiles for all players.  See map_know_and_see_all.
**************************************************************************/
void show_map_to_all(void)
{
  players_iterate(pplayer) {
    map_know_and_see_all(pplayer);
  } players_iterate_end;
}

/***************************************************************
  Allocate space for map, and initialise the tiles.
  Uses current map.xsize and map.ysize.
****************************************************************/
void player_map_allocate(struct player *pplayer)
{
  pplayer->private_map
    = fc_malloc(MAP_INDEX_SIZE * sizeof(*pplayer->private_map));

  whole_map_iterate(ptile) {
    player_tile_init(ptile, pplayer);
  } whole_map_iterate_end;
}

/***************************************************************
 frees a player's private map.
***************************************************************/
void player_map_free(struct player *pplayer)
{
  if (!pplayer->private_map) {
    return;
  }

  /* only after removing borders! */
  whole_map_iterate(ptile) {
    struct vision_site *psite = map_get_player_site(ptile, pplayer);

    if (NULL != psite) {
      free_vision_site(psite);
    }
  } whole_map_iterate_end;

  free(pplayer->private_map);
  pplayer->private_map = NULL;
}

/***************************************************************
  We need to use fogofwar_old here, so the player's tiles get
  in the same state as the other players' tiles.
***************************************************************/
static void player_tile_init(struct tile *ptile, struct player *pplayer)
{
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

  plrtile->terrain = T_UNKNOWN;
  clear_all_specials(&plrtile->special);
  plrtile->resource = NULL;
  plrtile->owner = NULL;
  plrtile->site = NULL;
  BV_CLR_ALL(plrtile->bases);

  vision_layer_iterate(v) {
    plrtile->seen_count[v] = 0;
    BV_CLR(ptile->tile_seen[v], player_index(pplayer));
  } vision_layer_iterate_end;

  if (!game.server.fogofwar_old) {
    plrtile->seen_count[V_MAIN] = 1;
    if (map_is_known(ptile, pplayer)) {
      BV_SET(ptile->tile_seen[V_MAIN], player_index(pplayer));
    }
  }

  plrtile->last_updated = game.info.year;
  vision_layer_iterate(v) {
    plrtile->own_seen[v] = plrtile->seen_count[v];
  } vision_layer_iterate_end;
}

/****************************************************************************
  Returns city located at given tile from player map.
****************************************************************************/
struct vision_site *map_get_player_city(const struct tile *ptile,
					const struct player *pplayer)
{
  struct vision_site *psite = map_get_player_site(ptile, pplayer);

  assert(psite == NULL || psite->location == ptile);
 
  return psite;
}

/****************************************************************************
  Returns site located at given tile from player map.
****************************************************************************/
struct vision_site *map_get_player_site(const struct tile *ptile,
					const struct player *pplayer)
{
  return map_get_player_tile(ptile, pplayer)->site;
}

/****************************************************************************
  Players' information of tiles is tracked so that fogged area can be kept
  consistent even when the client disconnects.  This function returns the
  player tile information for the given tile and player.
****************************************************************************/
struct player_tile *map_get_player_tile(const struct tile *ptile,
					const struct player *pplayer)
{
  return pplayer->private_map + tile_index(ptile);
}

/****************************************************************************
  Give pplayer the correct knowledge about tile; return TRUE iff
  knowledge changed.

  Note that unlike update_tile_knowledge, this function will not send any
  packets to the client.  Callers may want to call send_tile_info() if this
  function returns TRUE.
****************************************************************************/
bool update_player_tile_knowledge(struct player *pplayer, struct tile *ptile)
{
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);
  struct player *owner = (game.server.foggedborders
                          && !map_is_known_and_seen(ptile, pplayer, V_MAIN)
                          ? plrtile->owner
                          : tile_owner(ptile));

  if (plrtile->terrain != ptile->terrain
      || !BV_ARE_EQUAL(plrtile->special, ptile->special)
      || plrtile->resource != ptile->resource
      || plrtile->owner != owner
      || !BV_ARE_EQUAL(plrtile->bases, ptile->bases)) {
    plrtile->terrain = ptile->terrain;
    plrtile->special = ptile->special;
    plrtile->resource = ptile->resource;
    plrtile->owner = owner;
    plrtile->bases = ptile->bases;
    return TRUE;
  }
  return FALSE;
}

/****************************************************************************
  Update playermap knowledge for everybody who sees the tile, and send a
  packet to everyone whose info is changed.

  Note this only checks for changing of the terrain, special, or resource
  for the tile, since these are the only values held in the playermap.

  A tile's owner always can see terrain changes in his or her territory.
****************************************************************************/
void update_tile_knowledge(struct tile *ptile)
{
  /* Players */
  players_iterate(pplayer) {
    if (map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
      if (update_player_tile_knowledge(pplayer, ptile)) {
        send_tile_info(pplayer->connections, ptile, FALSE, FALSE);
      }
    }
  } players_iterate_end;

  /* Global observers */
  conn_list_iterate(game.est_connections, pconn) {
    struct player *pplayer = pconn->playing;

    if (NULL == pplayer && pconn->observer) {
      send_tile_info(pconn->self, ptile, FALSE, FALSE);
    }
  } conn_list_iterate_end;
}

/***************************************************************
...
***************************************************************/
void update_player_tile_last_seen(struct player *pplayer, struct tile *ptile)
{
  map_get_player_tile(ptile, pplayer)->last_updated = game.info.year;
}

/***************************************************************
...
***************************************************************/
static void really_give_tile_info_from_player_to_player(struct player *pfrom,
							struct player *pdest,
							struct tile *ptile)
{
  struct player_tile *from_tile, *dest_tile;
  if (!map_is_known_and_seen(ptile, pdest, V_MAIN)) {
    /* I can just hear people scream as they try to comprehend this if :).
     * Let me try in words:
     * 1) if the tile is seen by pfrom the info is sent to pdest
     *  OR
     * 2) if the tile is known by pfrom AND (he has more recent info
     *     OR it is not known by pdest)
     */
    if (map_is_known_and_seen(ptile, pfrom, V_MAIN)
	|| (map_is_known(ptile, pfrom)
	    && (((map_get_player_tile(ptile, pfrom)->last_updated
		 > map_get_player_tile(ptile, pdest)->last_updated))
	        || !map_is_known(ptile, pdest)))) {
      from_tile = map_get_player_tile(ptile, pfrom);
      dest_tile = map_get_player_tile(ptile, pdest);
      /* Update and send tile knowledge */
      map_set_known(ptile, pdest);
      dest_tile->terrain = from_tile->terrain;
      dest_tile->special = from_tile->special;
      dest_tile->resource = from_tile->resource;
      dest_tile->bases    = from_tile->bases;
      dest_tile->last_updated = from_tile->last_updated;
      send_tile_info(pdest->connections, ptile, FALSE, FALSE);

      /* update and send city knowledge */
      /* remove outdated cities */
      if (dest_tile->site) {
	if (!from_tile->site) {
	  /* As the city was gone on the newer from_tile
	     it will be removed by this function */
	  reality_check_city(pdest, ptile);
	} else /* We have a dest_city. update */
	  if (from_tile->site->identity
              != dest_tile->site->identity) {
	    /* As the city was gone on the newer from_tile
	       it will be removed by this function */
	    reality_check_city(pdest, ptile);
          }
      }

      /* Set and send new city info */
      if (from_tile->site) {
	if (!dest_tile->site) {
          /* We cannot assign new vision site with change_playertile_site(),
           * since location is not yet set up for new site */
          dest_tile->site = create_vision_site(0, ptile, NULL);
          *dest_tile->site = *from_tile->site;
	}
        /* Note that we don't care if receiver knows vision source city
         * or not. */
	send_city_info_at_tile(pdest, pdest->connections, NULL, ptile);
      }

      city_map_update_tile_frozen(ptile);
    }
  }
}

/***************************************************************
...
***************************************************************/
static void give_tile_info_from_player_to_player(struct player *pfrom,
						 struct player *pdest,
						 struct tile *ptile)
{
  really_give_tile_info_from_player_to_player(pfrom, pdest, ptile);

  players_iterate(pplayer2) {
    if (really_gives_vision(pdest, pplayer2)) {
      really_give_tile_info_from_player_to_player(pfrom, pplayer2, ptile);
    }
  } players_iterate_end;
}

/***************************************************************
This updates all players' really_gives_vision field.
If p1 gives p2 shared vision and p2 gives p3 shared vision p1
should also give p3 shared vision.
***************************************************************/
static void create_vision_dependencies(void)
{
  int added;

  players_iterate(pplayer) {
    pplayer->really_gives_vision = pplayer->gives_shared_vision;
  } players_iterate_end;

  /* In words: This terminates when it has run a round without adding
     a dependency. One loop only propagates dependencies one level deep,
     which is why we keep doing it as long as changes occur. */
  do {
    added = 0;
    players_iterate(pplayer) {
      players_iterate(pplayer2) {
	if (really_gives_vision(pplayer, pplayer2)
	    && pplayer != pplayer2) {
	  players_iterate(pplayer3) {
	    if (really_gives_vision(pplayer2, pplayer3)
		&& !really_gives_vision(pplayer, pplayer3)
		&& pplayer != pplayer3) {
	      pplayer->really_gives_vision |= (1<<player_index(pplayer3));
	      added++;
	    }
	  } players_iterate_end;
	}
      } players_iterate_end;
    } players_iterate_end;
  } while (added > 0);
}

/***************************************************************
...
***************************************************************/
void give_shared_vision(struct player *pfrom, struct player *pto)
{
  int save_vision[MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS];
  if (pfrom == pto) return;
  if (gives_shared_vision(pfrom, pto)) {
    freelog(LOG_ERROR, "Trying to give shared vision from %s to %s, "
	    "but that vision is already given!",
	    player_name(pfrom),
	    player_name(pto));
    return;
  }

  players_iterate(pplayer) {
    save_vision[player_index(pplayer)] = pplayer->really_gives_vision;
  } players_iterate_end;

  pfrom->gives_shared_vision |= 1<<player_index(pto);
  create_vision_dependencies();
  freelog(LOG_DEBUG, "giving shared vision from %s to %s\n",
	  player_name(pfrom),
	  player_name(pto));

  players_iterate(pplayer) {
    buffer_shared_vision(pplayer);
    players_iterate(pplayer2) {
      if (really_gives_vision(pplayer, pplayer2)
	  && !TEST_BIT(save_vision[player_index(pplayer)], player_index(pplayer2))) {
	freelog(LOG_DEBUG, "really giving shared vision from %s to %s\n",
	       player_name(pplayer),
	       player_name(pplayer2));
	whole_map_iterate(ptile) {
	  vision_layer_iterate(v) {
	    int change = map_get_own_seen(ptile, pplayer, v);

	    if (change != 0) {
	      map_change_seen(ptile, pplayer2, change, v);
              /* When fog of war is disabled, the seen count is always
               * at least 1.  Also when it's on the city radius, it has
               * the same behaviour. */
              if ((map_get_seen(ptile, pplayer2, v) == change
                   || !map_is_known(ptile, pplayer2))
		  && map_is_known(ptile, pplayer)) {
		really_unfog_tile(pplayer2, ptile, v);
	      }
	    }
	  } vision_layer_iterate_end;
	} whole_map_iterate_end;

	/* squares that are not seen, but which pfrom may have more recent
	   knowledge of */
	give_map_from_player_to_player(pplayer, pplayer2);
      }
    } players_iterate_end;
    unbuffer_shared_vision(pplayer);
  } players_iterate_end;

  if (S_S_RUNNING == server_state()) {
    send_player_info(pfrom, NULL);
  }
}

/***************************************************************
...
***************************************************************/
void remove_shared_vision(struct player *pfrom, struct player *pto)
{
  int save_vision[MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS];
  assert(pfrom != pto);
  if (!gives_shared_vision(pfrom, pto)) {
    freelog(LOG_ERROR, "Tried removing the shared vision from %s to %s, "
	    "but it did not exist in the first place!",
	    player_name(pfrom),
	    player_name(pto));
    return;
  }

  players_iterate(pplayer) {
    save_vision[player_index(pplayer)] = pplayer->really_gives_vision;
  } players_iterate_end;

  freelog(LOG_DEBUG, "removing shared vision from %s to %s\n",
	  player_name(pfrom),
	  player_name(pto));

  pfrom->gives_shared_vision &= ~(1<<player_index(pto));
  create_vision_dependencies();

  players_iterate(pplayer) {
    buffer_shared_vision(pplayer);
    players_iterate(pplayer2) {
      if (!really_gives_vision(pplayer, pplayer2)
	  && TEST_BIT(save_vision[player_index(pplayer)], player_index(pplayer2))) {
	freelog(LOG_DEBUG, "really removing shared vision from %s to %s\n",
	       player_name(pplayer),
	       player_name(pplayer2));
	whole_map_iterate(ptile) {
	  vision_layer_iterate(v) {
	    int change = map_get_own_seen(ptile, pplayer, v);

	    if (change > 0) {
	      map_change_seen(ptile, pplayer2, -change, v);
	      if (map_get_seen(ptile, pplayer2, v) == 0)
		really_fog_tile(pplayer2, ptile, v);
	    }
	  } vision_layer_iterate_end;
	} whole_map_iterate_end;
      }
    } players_iterate_end;
    unbuffer_shared_vision(pplayer);
  } players_iterate_end;

  if (S_S_RUNNING == server_state()) {
    send_player_info(pfrom, NULL);
  }
}

/*************************************************************************
...
*************************************************************************/
void enable_fog_of_war_player(struct player *pplayer)
{
  buffer_shared_vision(pplayer);
  whole_map_iterate(ptile) {
    map_fog_tile(pplayer, ptile, V_MAIN);
  } whole_map_iterate_end;
  unbuffer_shared_vision(pplayer);
}

/*************************************************************************
...
*************************************************************************/
void enable_fog_of_war(void)
{
  players_iterate(pplayer) {
    enable_fog_of_war_player(pplayer);
  } players_iterate_end;
}

/*************************************************************************
...
*************************************************************************/
void disable_fog_of_war_player(struct player *pplayer)
{
  buffer_shared_vision(pplayer);
  whole_map_iterate(ptile) {
    map_unfog_tile(pplayer, ptile, FALSE, V_MAIN);
  } whole_map_iterate_end;
  unbuffer_shared_vision(pplayer);
}

/*************************************************************************
...
*************************************************************************/
void disable_fog_of_war(void)
{
  players_iterate(pplayer) {
    disable_fog_of_war_player(pplayer);
  } players_iterate_end;
}

/**************************************************************************
  Set the tile to be a river if required.
  It's required if one of the tiles nearby would otherwise be part of a
  river to nowhere.
  For simplicity, I'm assuming that this is the only exit of the river,
  so I don't need to trace it across the continent.  --CJM
  Also, note that this only works for R_AS_SPECIAL type rivers.  --jjm
**************************************************************************/
static void ocean_to_land_fix_rivers(struct tile *ptile)
{
  /* clear the river if it exists */
  tile_clear_special(ptile, S_RIVER);

  cardinal_adjc_iterate(ptile, tile1) {
    if (tile_has_special(tile1, S_RIVER)) {
      bool ocean_near = FALSE;
      cardinal_adjc_iterate(tile1, tile2) {
        if (is_ocean_tile(tile2))
          ocean_near = TRUE;
      } cardinal_adjc_iterate_end;
      if (!ocean_near) {
        tile_set_special(ptile, S_RIVER);
        return;
      }
    }
  } cardinal_adjc_iterate_end;
}

/****************************************************************************
  A helper function for check_terrain_change that moves units off of invalid
  terrain after it's been changed.
****************************************************************************/
static void bounce_units_on_terrain_change(struct tile *ptile)
{
  unit_list_iterate_safe(ptile->units, punit) {
    bool unit_alive = TRUE;

    if (punit->tile == ptile
	&& punit->transported_by == -1
	&& !can_unit_exist_at_tile(punit, ptile)) {
      /* look for a nearby safe tile */
      adjc_iterate(ptile, ptile2) {
	if (can_unit_exist_at_tile(punit, ptile2)
            && !is_non_allied_unit_tile(ptile2, unit_owner(punit))
            && !is_non_allied_city_tile(ptile2, unit_owner(punit))) {
	  freelog(LOG_VERBOSE,
		  "Moved %s %s due to changing terrain at (%d,%d).",
		  nation_rule_name(nation_of_unit(punit)),
		  unit_rule_name(punit),
		  TILE_XY(punit->tile));
	  notify_player(unit_owner(punit), punit->tile, E_UNIT_RELOCATED,
                        FTC_SERVER_INFO, NULL,
                        _("Moved your %s due to changing terrain."),
                        unit_link(punit));
	  unit_alive = move_unit(punit, ptile2, 0);
	  if (unit_alive && punit->activity == ACTIVITY_SENTRY) {
	    unit_activity_handling(punit, ACTIVITY_IDLE);
	  }
	  break;
	}
      } adjc_iterate_end;
      if (unit_alive && punit->tile == ptile) {
	/* if we get here we could not move punit */
	freelog(LOG_VERBOSE,
		"Disbanded %s %s due to changing land to sea at (%d,%d).",
		nation_rule_name(nation_of_unit(punit)),
		unit_rule_name(punit),
		TILE_XY(punit->tile));
	notify_player(unit_owner(punit), punit->tile, E_UNIT_LOST_MISC,
                      FTC_SERVER_INFO, NULL,
                      _("Disbanded your %s due to changing terrain."),
                      unit_link(punit));
	wipe_unit(punit);
      }
    }
  } unit_list_iterate_safe_end;
}

/****************************************************************************
  Returns TRUE if the terrain change from 'oldter' to 'newter' requires
  extra (potentially expensive) fixing (e.g. of the surroundings).
****************************************************************************/
bool need_to_fix_terrain_change(const struct terrain *oldter,
                                const struct terrain *newter)
{
  bool old_is_ocean, new_is_ocean;
  
  if (!oldter || !newter) {
    return FALSE;
  }

  old_is_ocean = is_ocean(oldter);
  new_is_ocean = is_ocean(newter);

  return (old_is_ocean && !new_is_ocean)
    || (!old_is_ocean && new_is_ocean);
}

/****************************************************************************
  Assumes that need_to_fix_terrain_change == TRUE.
  For in-game terrain changes 'extend_rivers' should
  be TRUE, for edits it should be FALSE.
****************************************************************************/
void fix_tile_on_terrain_change(struct tile *ptile,
                                bool extend_rivers)
{
  if (!is_ocean_tile(ptile)) {
    if (extend_rivers) {
      ocean_to_land_fix_rivers(ptile);
    }
    city_landlocked_sell_coastal_improvements(ptile);
  }
  bounce_units_on_terrain_change(ptile);
}

/****************************************************************************
  Handles global side effects for a terrain change.  Call this in the
  server immediately after calling tile_change_terrain.
****************************************************************************/
void check_terrain_change(struct tile *ptile, struct terrain *oldter)
{
  struct terrain *newter = tile_terrain(ptile);

  if (!need_to_fix_terrain_change(oldter, newter)) {
    return;
  }

  fix_tile_on_terrain_change(ptile, TRUE);
  assign_continent_numbers();
  send_all_known_tiles(NULL, FALSE);
}

/*************************************************************************
  Ocean tile can be claimed iff one of the following conditions stands:
  a) it is an inland lake not larger than MAXIMUM_OCEAN_SIZE
  b) it is adjacent to only one continent and not more than two ocean tiles
  c) It is one tile away from a city
  The source which claims the ocean has to be placed on the correct continent.
  in case a) The continent which surrounds the inland lake
  in case b) The only continent which is adjacent to the tile
*************************************************************************/
static bool is_claimable_ocean(struct tile *ptile, struct tile *source)
{
  Continent_id cont = tile_continent(ptile);
  Continent_id source_cont = tile_continent(source);
  Continent_id cont2;
  int ocean_tiles;

  if (get_ocean_size(-cont) <= MAXIMUM_CLAIMED_OCEAN_SIZE
      && get_lake_surrounders(cont) == source_cont) {
    return TRUE;
  }
  
  ocean_tiles = 0;
  adjc_iterate(ptile, tile2) {
    cont2 = tile_continent(tile2);
    if (tile2 == source) {
      return TRUE;
    }
    if (cont2 == cont) {
      ocean_tiles++;
    } else if (cont2 != source_cont) {
      return FALSE; /* two land continents adjacent, punt! */
    }
  } adjc_iterate_end;
  if (ocean_tiles <= 2) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/*************************************************************************
  For each unit at the tile, queue any unique home city.
*************************************************************************/
static void map_unit_homecity_enqueue(struct tile *ptile)
{
  unit_list_iterate(ptile->units, punit) {
    struct city *phome = game_find_city_by_number(punit->homecity);

    if (NULL == phome) {
      continue;
    }

    city_refresh_queue_add(phome);
  } unit_list_iterate_end;
}

/*************************************************************************
  Claim ownership of a single tile.
*************************************************************************/
void map_claim_ownership(struct tile *ptile, struct player *powner,
                         struct tile *psource)
{
  struct player *ploser = tile_owner(ptile);

  if (game.info.borders >= 2) {
    if (ploser != powner) {
      if (ploser) {
        map_fog_tile(ploser, ptile, V_MAIN);
      }
      if (powner) {
        map_unfog_tile(powner, ptile, TRUE, V_MAIN);
      }
    }
  }

  if (ploser != powner) {
    base_type_iterate(pbase) {
      if (tile_has_base(ptile, pbase)) {
        if (pbase->vision_main_sq >= 0) {
          /* Transfer base provided vision to new owner */
          if (powner) {
            map_refog_circle(powner, ptile, -1, pbase->vision_main_sq,
                             game.info.vision_reveal_tiles, V_MAIN);
          }
          if (ploser) {
            map_refog_circle(ploser, ptile, pbase->vision_main_sq, -1,
                             game.info.vision_reveal_tiles, V_MAIN);
          }
        }
        if (pbase->vision_invis_sq >= 0) {
          /* Transfer base provided vision to new owner */
          if (powner) {
            map_refog_circle(powner, ptile, -1, pbase->vision_invis_sq,
                             game.info.vision_reveal_tiles, V_INVIS);
          }
          if (ploser) {
            map_refog_circle(ploser, ptile, pbase->vision_invis_sq, -1,
                             game.info.vision_reveal_tiles, V_INVIS);
          }
        }
      }
    } base_type_iterate_end;
  }

  tile_set_owner(ptile, powner, psource);

  if (ploser != powner) {
    if (S_S_RUNNING == server_state() && game.info.happyborders > 0) {
      map_unit_homecity_enqueue(ptile);
    }

    if (!city_map_update_tile_frozen(ptile)) {
      send_tile_info(NULL, ptile, FALSE, FALSE);
    }
  }
}

/*************************************************************************
  Remove border for this source.
*************************************************************************/
void map_clear_border(struct tile *ptile)
{
  int radius_sq = tile_border_source_radius_sq(ptile);

  circle_dxyr_iterate(ptile, radius_sq, dtile, dx, dy, dr) {
    struct tile *claimer = tile_claimer(dtile);

    if (claimer == ptile) {
      map_claim_ownership(dtile, NULL, NULL);
    }
  } circle_dxyr_iterate_end;
}

/*************************************************************************
  Update borders for this source. Call this for each new source.
*************************************************************************/
void map_claim_border(struct tile *ptile, struct player *owner)
{
  int radius_sq = tile_border_source_radius_sq(ptile);

  circle_dxyr_iterate(ptile, radius_sq, dtile, dx, dy, dr) {
    struct tile *dclaimer = tile_claimer(dtile);

    if (dr != 0 && is_border_source(dtile)) {
      /* Do not claim border sources other than self */
      continue;
    }

    if (!map_is_known(dtile, owner) && game.info.borders < 3) {
      continue;
    }

    if (NULL != dclaimer && dclaimer != ptile) {
      struct city *ccity = tile_city(dclaimer);
      int strength_old, strength_new;

      if (ccity != NULL) {
        /* Previously claimed by city */
        int city_x, city_y;

        map_distance_vector(&city_x, &city_y, ccity->tile, dtile);
        city_x += CITY_MAP_RADIUS;
        city_y += CITY_MAP_RADIUS;

        if (is_valid_city_coords(city_x, city_y)) {
          /* Tile is within city radius */
          continue;
        }
      }

      strength_old = tile_border_strength(dtile, dclaimer);
      strength_new = tile_border_strength(dtile, ptile);

      if (strength_new <= strength_old) {
        /* Stronger shall prevail,cd 
         * in case of equel strength older shall prevail */
        continue;
      }
    }

    if (is_ocean_tile(dtile)) {
      if (is_claimable_ocean(dtile, ptile)) {
        map_claim_ownership(dtile, owner, ptile);
      }
    } else {
      if (tile_continent(dtile) == tile_continent(ptile)) {
        map_claim_ownership(dtile, owner, ptile);
      }
    }
  } circle_dxyr_iterate_end;
}

/*************************************************************************
  Update borders for all sources. Call this on turn end.
*************************************************************************/
void map_calculate_borders(void)
{
  if (game.info.borders == 0) {
    return;
  }

  freelog(LOG_VERBOSE,"map_calculate_borders()");

  whole_map_iterate(ptile) {
    if (is_border_source(ptile)) {
      map_claim_border(ptile, ptile->owner);
    }
  } whole_map_iterate_end;

  freelog(LOG_VERBOSE,"map_calculate_borders() workers");
  city_thaw_workers_queue();
  city_refresh_queue_processing();
}

/****************************************************************************
  Change the sight points for the vision source, fogging or unfogging tiles
  as needed.

  See documentation in vision.h.
****************************************************************************/
void vision_change_sight(struct vision *vision, enum vision_layer vlayer,
			 int radius_sq)
{
  map_refog_circle(vision->player, vision->tile,
		   vision->radius_sq[vlayer], radius_sq,
		   vision->can_reveal_tiles, vlayer);
  vision->radius_sq[vlayer] = radius_sq;
}

/****************************************************************************
  Clear all sight points from this vision source.

  See documentation in vision.h.
****************************************************************************/
void vision_clear_sight(struct vision *vision)
{
  /* We don't use vision_layer_iterate because we have to go in reverse
   * order. */
  vision_change_sight(vision, V_INVIS, -1);
  vision_change_sight(vision, V_MAIN, -1);
}

/****************************************************************************
  Create base to tile.
****************************************************************************/
void create_base(struct tile *ptile, struct base_type *pbase,
                 struct player *pplayer)
{
  base_type_iterate(old_base) {
    if (tile_has_base(ptile, old_base)
        && !can_bases_coexist(old_base, pbase)) {
      if (territory_claiming_base(old_base)) {
        map_clear_border(ptile);
        map_claim_ownership(ptile, NULL, NULL);
      } else {
        struct player *owner = tile_owner(ptile);

        if (old_base->vision_main_sq >= 0 && owner) {
          /* Base provides vision, but no borders. */
          map_refog_circle(owner, ptile, old_base->vision_main_sq, -1,
                           game.info.vision_reveal_tiles, V_MAIN);
        }
        if (old_base->vision_invis_sq >= 0 && owner) {
          map_refog_circle(owner, ptile, old_base->vision_invis_sq, -1,
                           game.info.vision_reveal_tiles, V_INVIS);
        }
      }
      tile_remove_base(ptile, old_base);
    }
  } base_type_iterate_end;

  tile_add_base(ptile, pbase);

  /* Watchtower might become effective
   * FIXME: Reqs on other specials will not be updated immediately. */
  unit_list_refresh_vision(ptile->units);

  /* Claim base if it has "ClaimTerritory" flag */
  if (territory_claiming_base(pbase) && pplayer) {
    map_claim_ownership(ptile, pplayer, ptile);
    map_claim_border(ptile, pplayer);
    city_thaw_workers_queue();
    city_refresh_queue_processing();
  } else {
    struct player *owner = tile_owner(ptile);

    if (pbase->vision_main_sq > 0 && owner) {
      map_refog_circle(owner, ptile, -1, pbase->vision_main_sq,
                       game.info.vision_reveal_tiles, V_MAIN);
    }
    if (pbase->vision_invis_sq > 0 && owner) {
      map_refog_circle(owner, ptile, -1, pbase->vision_invis_sq,
                       game.info.vision_reveal_tiles, V_INVIS);
    }
  }
}
