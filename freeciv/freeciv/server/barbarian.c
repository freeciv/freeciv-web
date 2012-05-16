/********************************************************************** 
 Freeciv - Copyright (C) 2003 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/**********************************************************************
  Functions for creating barbarians in huts, land and sea
  Started by Jerzy Klek <jekl@altavista.net>
  with more ideas from Falk Hueffner 
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "support.h"

/* common */
#include "effects.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "tech.h"
#include "terrain.h"
#include "unitlist.h"

/* ai */
#include "aidata.h"
#include "aitools.h"

/* server */
#include "gamehand.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "techtools.h"
#include "unithand.h"
#include "unittools.h"

#include "barbarian.h"

#define BARBARIAN_INITIAL_VISION_RADIUS 3
#define BARBARIAN_INITIAL_VISION_RADIUS_SQ 9

/**************************************************************************
  Is player a land barbarian?
**************************************************************************/
bool is_land_barbarian(struct player *pplayer)
{
  return (pplayer->ai_data.barbarian_type == LAND_BARBARIAN);
}

/**************************************************************************
  Is player a sea barbarian?
**************************************************************************/
bool is_sea_barbarian(struct player *pplayer)
{
  return (pplayer->ai_data.barbarian_type == SEA_BARBARIAN);
}

/**************************************************************************
  Creates the land/sea barbarian player and inits some stuff. If 
  barbarian player already exists, return player pointer. If barbarians 
  are dead, revive them with a new leader :-)

  Dead barbarians forget the map and lose the money.
**************************************************************************/
static struct player *create_barbarian_player(enum barbarian_type type)
{
  struct player *barbarians;
  struct nation_type *nation;

  players_iterate(barbarians) {
    if ((type == LAND_BARBARIAN && is_land_barbarian(barbarians))
        || (type == SEA_BARBARIAN && is_sea_barbarian(barbarians))) {
      if (!barbarians->is_alive) {
        barbarians->economic.gold = 0;
        barbarians->is_alive = TRUE;
        barbarians->is_dying = FALSE;
        pick_random_player_name(nation_of_player(barbarians), barbarians->name);
	sz_strlcpy(barbarians->username, ANON_USER_NAME);
        /* I need to make them to forget the map, I think */
	whole_map_iterate(ptile) {
	  map_clear_known(ptile, barbarians);
	} whole_map_iterate_end;
      }
      barbarians->economic.gold += 100;  /* New leader, new money */
      return barbarians;
    }
  } players_iterate_end;

  /* make a new player, or not */
  barbarians = server_create_player();
  if (!barbarians) {
    return NULL;
  }

  server_player_init(barbarians, TRUE, TRUE);

  nation = pick_a_nation(NULL, FALSE, TRUE, type);
  player_set_nation(barbarians, nation);
  pick_random_player_name(nation, barbarians->name);

  server.nbarbarians++;
  game.info.max_players = MAX(player_count(), game.info.max_players);

  sz_strlcpy(barbarians->username, ANON_USER_NAME);
  barbarians->is_connected = FALSE;
  barbarians->government = nation->init_government;
  assert(barbarians->revolution_finishes < 0);
  barbarians->capital = FALSE;
  barbarians->economic.gold = 100;

  barbarians->phase_done = TRUE;

  /* Do the ai */
  barbarians->ai_data.control = TRUE;
  barbarians->ai_data.barbarian_type = type;
  set_ai_level_directer(barbarians, game.info.skill_level);
  init_tech(barbarians, TRUE);
  give_global_initial_techs(barbarians);
  give_nation_initial_techs(barbarians);

  /* Ensure that we are at war with everyone else */
  players_iterate(pplayer) {
    if (pplayer != barbarians) {
      pplayer->diplstates[player_index(barbarians)].type = DS_WAR;
      barbarians->diplstates[player_index(pplayer)].type = DS_WAR;
    }
  } players_iterate_end;

  freelog(LOG_VERBOSE, "Created barbarian %s, player %d",
          player_name(barbarians),
          player_number(barbarians));
  notify_player(NULL, NULL, E_UPRISING, FTC_SERVER_INFO, NULL,
                _("%s gain a leader by the name %s. Dangerous "
                  "times may lie ahead."),
                nation_plural_for_player(barbarians),
                player_name(barbarians));

  send_player_info(barbarians, NULL);

  return barbarians;
}

/**************************************************************************
  Check if a tile is land and free of enemy units
**************************************************************************/
static bool is_free_land(struct tile *ptile, struct player *who)
{
  return (!is_ocean_tile(ptile)
	  && !is_non_allied_unit_tile((ptile), who));
}

/**************************************************************************
  Check if a tile is sea and free of enemy units
**************************************************************************/
static bool is_free_sea(struct tile *ptile, struct player *who)
{
  return (is_ocean_tile(ptile)
	  && !is_non_allied_unit_tile((ptile), who));
}

/**************************************************************************
  (Re)initialize direction checked status array based on terrain class.
**************************************************************************/
static void init_dir_checked_status(bool *checked,
                                    enum terrain_class *terrainc,
                                    enum terrain_class tclass)
{
  int dir;

  for (dir = 0; dir < 8; dir++) {
    if (terrainc[dir] == tclass) {
      checked[dir] = FALSE;
    } else {
      checked[dir] = TRUE;
    }
  }
}

/**************************************************************************
  Return random directory from not yet checked ones.
**************************************************************************/
static int random_unchecked_direction(int possibilities, const bool *checked)
{
  int j = -1;
  int i;

  int num = myrand(possibilities);
  for (i = 0; i <= num; i++) {
    j++;
    while (checked[j]) {
      j++;
      assert(j < 8);
    }
  }

  return j;
}

/**************************************************************************
  Unleash barbarians means give barbarian player some units and move them 
  out of the hut, unless there's no place to go.

  Barbarian unit deployment algorithm: If enough free land around, deploy
  on land, if not enough land but some sea free, load some of them on 
  boats, otherwise (not much land and no sea) kill enemy unit and stay in 
  a village. The return value indicates if the explorer survived entering 
  the vilage.
**************************************************************************/
bool unleash_barbarians(struct tile *ptile)
{
  struct player *barbarians;
  int unit_cnt;
  int i;
  bool alive = TRUE;     /* explorer survived */
  enum terrain_class terrainc[8];
  struct tile *dir_tiles[8];
  int land_tiles = 0;
  int ocean_tiles = 0;
  bool checked[8];
  int checked_count;
  int dir;
  bool barbarian_stays = FALSE;

  /* FIXME: When there is no L_BARBARIAN unit,
   *        but L_BARBARIAN_TECH is already available,
   *        we should unleash those.
   *        Doesn't affect any ruleset I'm aware of. */
  if (game.info.barbarianrate == 0
      || game.info.turn < game.info.onsetbarbarian
      || num_role_units(L_BARBARIAN) == 0) {
    unit_list_iterate_safe((ptile)->units, punit) {
      wipe_unit(punit);
    } unit_list_iterate_safe_end;
    return FALSE;
  }

  barbarians = create_barbarian_player(LAND_BARBARIAN);
  if (!barbarians) {
    return FALSE;
  }

  unit_cnt = 3 + myrand(4);
  for (i = 0; i < unit_cnt; i++) {
    struct unit_type *punittype
      = find_a_unit_type(L_BARBARIAN, L_BARBARIAN_TECH);

    /* If unit cannot live on this tile, we just don't create one.
     * Maybe find_a_unit_type() should take tile parameter, so
     * we could get suitable unit if one exist. */
    if (is_native_tile(punittype, ptile)) {
      struct unit *barb_unit;

      barb_unit = create_unit(barbarians, ptile, punittype, 0, 0, -1);
      freelog(LOG_DEBUG, "Created barbarian unit %s",
              utype_rule_name(punittype));
      send_unit_info(NULL, barb_unit);
    }
  }

  /* Get information about surrounding terrains in terrain class level.
   * Only needed if we consider moving units away to random directions. */
  for (dir = 0; dir < 8; dir++) {
    dir_tiles[dir] = mapstep(ptile, dir);
    if (dir_tiles[dir] == NULL) {
      terrainc[dir] = TC_LAST;
    } else if (is_free_land(dir_tiles[dir], barbarians)) {
      terrainc[dir] = TC_LAND;
      land_tiles++;
    } else if (is_free_sea(dir_tiles[dir], barbarians)) {
      terrainc[dir] = TC_OCEAN;
      ocean_tiles++;
    } else {
      terrainc[dir] = TC_LAST;
    }
  }

  if (land_tiles >= 3) {
    /* Enough land, scatter guys around */
    unit_list_iterate_safe((ptile)->units, punit2) {
      if (unit_owner(punit2) == barbarians) {
        bool dest_found = FALSE;

        /* Initialize checked status for checking free land tiles */
        init_dir_checked_status(checked, terrainc, TC_LAND);

        /* Search tile to move to */
        for (checked_count = 0; !dest_found && checked_count < land_tiles;
             checked_count++) {
          int rdir = random_unchecked_direction(land_tiles - checked_count, checked);

          if (can_unit_move_to_tile(punit2, dir_tiles[rdir], TRUE)) {
            (void) unit_move_handling(punit2, dir_tiles[rdir], TRUE, FALSE);
            freelog(LOG_DEBUG, "Moved barbarian unit from %d %d to %d, %d", 
                    ptile->x, ptile->y, dir_tiles[rdir]->x, dir_tiles[rdir]->y);
            dest_found = TRUE;
          }

          checked[rdir] = TRUE;
        }
        if (!dest_found) {
          /* This barbarian failed to move out of hut tile. */
          barbarian_stays = TRUE;
        }
      }
    } unit_list_iterate_safe_end;

  } else {
    if (ocean_tiles > 0) {
      /* maybe it's an island, try to get on boats */
      struct tile *btile = NULL; /* Boat tile */

      /* Initialize checked status for checking Ocean tiles */
      init_dir_checked_status(checked, terrainc, TC_OCEAN);

      /* Search tile for boat. We always create just one boat. */
      for (checked_count = 0; btile == NULL && checked_count < ocean_tiles;
           checked_count++) {
        struct unit_type *boat;
        int rdir = random_unchecked_direction(ocean_tiles - checked_count, checked);

        boat = find_a_unit_type(L_BARBARIAN_BOAT, -1);
        if (is_native_tile(boat, dir_tiles[rdir])) {
          (void) create_unit(barbarians, dir_tiles[rdir], boat, 0, 0, -1);
          btile = dir_tiles[rdir];
        }

        checked[rdir] = TRUE;
      }

      if (btile) {
        /* We do have a boat. Try to get everybody in */
        unit_list_iterate_safe((ptile)->units, punit2) {
          if (unit_owner(punit2) == barbarians) {
            if (can_unit_move_to_tile(punit2, btile, TRUE)) {
              (void) unit_move_handling(punit2, btile, TRUE, FALSE);
            }
          }
        } unit_list_iterate_safe_end;
      }

      /* Move rest of the barbarians to random land tiles */
      unit_list_iterate_safe((ptile)->units, punit2) {
        if (unit_owner(punit2) == barbarians) {
          bool dest_found = FALSE;

          /* Initialize checked status for checking Land tiles */
          init_dir_checked_status(checked, terrainc, TC_LAND);

          /* Search tile to move to */
          for (checked_count = 0; !dest_found && checked_count < land_tiles;
               checked_count++) {
            int rdir;
            rdir = random_unchecked_direction(land_tiles - checked_count, checked);

            if (can_unit_move_to_tile(punit2, dir_tiles[rdir], TRUE)) {
              (void) unit_move_handling(punit2, dir_tiles[rdir], TRUE, FALSE);
              dest_found = TRUE;
            }

            checked[rdir] = TRUE;
          }
          if (!dest_found) {
            /* This barbarian failed to move out of hut tile. */
            barbarian_stays = TRUE;
          }
        }
      } unit_list_iterate_safe_end;
    } else {
      /* The village is surrounded! Barbarians cannot leave. */
      barbarian_stays = TRUE;
    }
  }

  if (barbarian_stays) {
    /* There's barbarian in this village! Kill the explorer. */
    unit_list_iterate_safe((ptile)->units, punit2) {
      if (unit_owner(punit2) != barbarians) {
        wipe_unit(punit2);
        alive = FALSE;
      } else {
        send_unit_info(NULL, punit2);
      }
    } unit_list_iterate_safe_end;
  }

  /* FIXME: I don't know if this is needed */
  if (ptile) {
    map_show_circle(barbarians, ptile, BARBARIAN_INITIAL_VISION_RADIUS_SQ);
  }

  return alive;
}

/**************************************************************************
  Is sea not further than a couple of tiles away from land?
**************************************************************************/
static bool is_near_land(struct tile *tile0)
{
  square_iterate(tile0, 4, ptile) {
    if (!is_ocean_tile(ptile)) {
      return TRUE;
    }
  } square_iterate_end;

  return FALSE;
}

/**************************************************************************
  Return this or a neighbouring tile that is free of any units
**************************************************************************/
static struct tile *find_empty_tile_nearby(struct tile *ptile)
{
  square_iterate(ptile, 1, tile1) {
    if (unit_list_size(tile1->units) == 0) {
      return tile1;
    }
  } square_iterate_end;

  return NULL;
}

/**************************************************************************
  The barbarians are summoned at a randomly chosen place if:
  1. It's not closer than MIN_UNREST_DIST and not further than 
     MAX_UNREST_DIST from the nearest city. City owner is called 'victim' 
     here.
  2. The place or a neighbouring tile must be empty to deploy the units.
  3. If it's the sea it shouldn't be far from the land. (questionable)
  4. Place must be known to the victim
  5. The uprising chance depends also on the victim empire size, its
     government (civil_war_chance) and barbarian difficulty level.
  6. The number of land units also depends slightly on victim's empire 
     size and barbarian difficulty level.
  Q: The empire size is used so there are no uprisings in the beginning 
     of the game (year is not good as it can be customized), but it seems 
     a bit unjust if someone is always small. So maybe it should rather 
     be an average number of cities (all cities/player num)? Depending 
     on the victim government type is also questionable.
**************************************************************************/
static void try_summon_barbarians(void)
{
  struct tile *ptile, *utile;
  int i, dist;
  int uprise = 1;
  struct city *pc;
  struct player *barbarians, *victim;
  struct unit_type *leader_type;
  int barb_count, really_created = 0;

  /* We attempt the summons on a particular, random position.  If this is
   * an invalid position then the summons simply fails this time.  This means
   * that a particular tile's chance of being summoned on is independent of
   * all the other tiles on the map - which is essential for balanced
   * gameplay. */
  ptile = rand_map_pos();

  if (terrain_has_flag(tile_terrain(ptile), TER_NO_BARBS)) {
    return;
  }


  if (!(pc = dist_nearest_city(NULL, ptile, TRUE, FALSE))) {
    /* any city */
    return;
  }

  victim = city_owner(pc);

  dist = real_map_distance(ptile, pc->tile);
  freelog(LOG_DEBUG,"Closest city (to %d,%d) is %s (at %d,%d) distance %d.", 
          TILE_XY(ptile), city_name(pc), TILE_XY(pc->tile), dist);
  if (dist > MAX_UNREST_DIST || dist < MIN_UNREST_DIST) {
    return;
  }

  /* I think Sea Raiders can come out of unknown sea territory */
  if (!(utile = find_empty_tile_nearby(ptile))
      || (!map_is_known(utile, victim)
	  && !is_ocean_tile(utile))
      || !is_near_land(utile)) {
    return;
  }

  /* do not harass small civs - in practice: do not uprise at the beginning */
  if ((int)myrand(UPRISE_CIV_MORE) >
           (int)city_list_size(victim->cities) -
                UPRISE_CIV_SIZE/(game.info.barbarianrate-1)
      || myrand(100) > get_player_bonus(victim, EFT_CIVIL_WAR_CHANCE)) {
    return;
  }
  freelog(LOG_DEBUG, "Barbarians are willing to fight");

  if (tile_has_special(utile, S_HUT)) {
    /* remove the hut in place of uprising */
    tile_clear_special(utile, S_HUT);
    update_tile_knowledge(utile);
  }

  barb_count = myrand(3) + uprise * game.info.barbarianrate;
  leader_type = get_role_unit(L_BARBARIAN_LEADER, 0);

  if (!is_ocean_tile(utile)) {
    /* land (disembark) barbarians */
    barbarians = create_barbarian_player(LAND_BARBARIAN);
    if (!barbarians) {
      return;
    }
    if (city_list_size(victim->cities) > UPRISE_CIV_MOST) {
      uprise = 3;
    }
    for (i = 0; i < barb_count; i++) {
      struct unit_type *punittype
	= find_a_unit_type(L_BARBARIAN, L_BARBARIAN_TECH);

      /* If unit cannot live on this tile, we just don't create one.
       * Maybe find_a_unit_type() should take tile parameter, so
       * we could get suitable unit if one exist. */
      if (is_native_tile(punittype, utile)) {
        (void) create_unit(barbarians, utile, punittype, 0, 0, -1);
        really_created++;
        freelog(LOG_DEBUG, "Created barbarian unit %s",
                utype_rule_name(punittype));
      }
    }
 
    if (is_native_tile(leader_type, utile)) { 
      (void) create_unit(barbarians, utile,
                         leader_type, 0, 0, -1);
      really_created++;
    }
  } else {                   /* sea raiders - their units will be veteran */
    struct unit *ptrans;
    struct unit_type *boat;

    barbarians = create_barbarian_player(SEA_BARBARIAN);
    if (!barbarians) {
      return;
    }
    ai_data_phase_init(barbarians, TRUE); /* Created ferry may need ai data */

    boat = find_a_unit_type(L_BARBARIAN_BOAT,-1);

    if (is_native_tile(boat, utile)
        && (!utype_has_flag(boat, F_TRIREME) || is_safe_ocean(utile))) {
      int cap;

      ptrans = create_unit(barbarians, utile, boat, 0, 0, -1);
      really_created++;
      cap = get_transporter_capacity(ptrans);

      /* Fill boat with barb_count barbarians at max, leave space for leader */
      for (i = 0; i < cap - 1 && i < barb_count; i++) {
        struct unit_type *barb
          = find_a_unit_type(L_BARBARIAN_SEA, L_BARBARIAN_SEA_TECH);

        if (can_unit_type_transport(boat, utype_class(barb))) {
          (void) create_unit_full(barbarians, utile, barb, 0, 0, -1, -1,
                                  ptrans);
          really_created++;
          freelog(LOG_DEBUG, "Created barbarian unit %s",
                  utype_rule_name(barb));
        }
      }

      if (can_unit_type_transport(boat, utype_class(leader_type))) {
        (void) create_unit_full(barbarians, utile, leader_type, 0, 0,
                                -1, -1, ptrans);
        really_created++;
      }
    }

    ai_data_phase_done(barbarians);
  }

  if (really_created == 0) {
    /* No barbarians found suitable spot */
    return;
  }

  /* Is this necessary?  create_unit_full already sends unit info. */
  unit_list_iterate(utile->units, punit2) {
    send_unit_info(NULL, punit2);
  } unit_list_iterate_end;

  /* to let them know where to get you */
  map_show_circle(barbarians, utile, BARBARIAN_INITIAL_VISION_RADIUS_SQ);
  map_show_circle(barbarians, pc->tile, BARBARIAN_INITIAL_VISION_RADIUS_SQ);

  /* There should probably be a different message about Sea Raiders */
  if (is_land_barbarian(barbarians)) {
    notify_player(victim, utile, E_UPRISING, FTC_SERVER_INFO, NULL,
                  _("Native unrest near %s led by %s."),
                  city_link(pc),
                  player_name(barbarians));
  } else if (map_is_known_and_seen(utile, victim, V_MAIN)) {
    notify_player(victim, utile, E_UPRISING, FTC_SERVER_INFO, NULL,
                  _("Sea raiders seen near %s!"),
                  city_link(pc));
  }
}

/**************************************************************************
  Summon barbarians out of the blue. Try more times for more difficult 
  levels - which means there can be more than one uprising in one year!
**************************************************************************/
void summon_barbarians(void)
{
  int i, n;

  if (game.info.barbarianrate == 0) {
    return;
  }

  if (game.info.turn < game.info.onsetbarbarian) {
    return;
  }

  n = map_num_tiles() / MAP_FACTOR;
  if (n == 0) {
    /* Allow barbarians on maps smaller than MAP_FACTOR */
    n = 1;
  }

  for (i = 0; i < n * (game.info.barbarianrate - 1); i++) {
    try_summon_barbarians();
  }
}
