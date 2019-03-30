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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "bitvector.h"
#include "log.h"

/* common */
#include "city.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "player.h"
#include "research.h"
#include "specialist.h"
#include "terrain.h"
#include "unit.h"
#include "unitlist.h"

/* server */
#include "citytools.h"
#include "cityturn.h"           /* city_repair_size() */
#include "maphand.h"
#include "plrhand.h"
#include "srv_main.h"
#include "unittools.h"

#include "sanitycheck.h"


#ifdef SANITY_CHECKING

#define SANITY_FAIL(format, ...) \
  fc_assert_fail(file, function, line, NULL, format, ## __VA_ARGS__)

#define SANITY_CHECK(check) \
  fc_assert_full(file, function, line, check, , NOLOGMSG, NOLOGMSG)

#define SANITY_CITY(_city, check)                                           \
  fc_assert_full(file, function, line, check, ,                             \
                 "(%4d, %4d) in \"%s\"[%d]", TILE_XY((_city)->tile),        \
                 city_name_get(_city), city_size_get(_city))

#define SANITY_TERRAIN(_tile, check)                                        \
  fc_assert_full(file, function, line, check, ,                             \
                 "(%4d, %4d) at \"%s\"", TILE_XY(_tile),                    \
                 terrain_rule_name(tile_terrain(_tile)))

#define SANITY_TILE(_tile, check)                                           \
  do {                                                                      \
    struct city *_tile##_city = tile_city(_tile);                           \
    if (NULL != _tile##_city) {                                             \
      SANITY_CITY(_tile##_city, check);                                     \
    } else {                                                                \
      SANITY_TERRAIN(_tile, check);                                         \
    }                                                                       \
  } while (FALSE)

static void check_city_feelings(const struct city *pcity, const char *file,
                                const char *function, int line);

/**********************************************************************//**
  Sanity checking on map (tile) specials.
**************************************************************************/
static void check_specials(const char *file, const char *function, int line)
{
  whole_map_iterate(&(wld.map), ptile) {
    const struct terrain *pterrain = tile_terrain(ptile);

    extra_type_iterate(pextra) {
      if (tile_has_extra(ptile, pextra)) {
        extra_deps_iterate(&(pextra->reqs), pdep) {
          SANITY_TILE(ptile, tile_has_extra(ptile, pdep));
        } extra_deps_iterate_end;
      }
    } extra_type_iterate_end;

    extra_type_by_cause_iterate(EC_MINE, pextra) {
      if (tile_has_extra(ptile, pextra)) {
        SANITY_TILE(ptile, pterrain->mining_result == pterrain);
      }
    } extra_type_by_cause_iterate_end;
    extra_type_by_cause_iterate(EC_IRRIGATION, pextra) {
      if (tile_has_extra(ptile, pextra)) {
        SANITY_TILE(ptile, pterrain->irrigation_result == pterrain);
      }
    } extra_type_by_cause_iterate_end;

    SANITY_TILE(ptile, terrain_index(pterrain) >= T_FIRST 
                       && terrain_index(pterrain) < terrain_count());
  } whole_map_iterate_end;
}

/**********************************************************************//**
  Sanity checking on fog-of-war (visibility, shared vision, etc.).
**************************************************************************/
static void check_fow(const char *file, const char *function, int line)
{
  if (!game_was_started()) {
    /* The private map of the players is only allocated at game start. */
    return;
  }

  whole_map_iterate(&(wld.map), ptile) {
    players_iterate(pplayer) {
      struct player_tile *plr_tile = map_get_player_tile(ptile, pplayer);

      vision_layer_iterate(v) {
        /* underflow of unsigned int */
        SANITY_TILE(ptile, plr_tile->seen_count[v] < 30000);
        SANITY_TILE(ptile, plr_tile->own_seen[v] < 30000);
        SANITY_TILE(ptile, plr_tile->own_seen[v] <= plr_tile->seen_count[v]);
      } vision_layer_iterate_end;

      /* Lots of server bits depend on this. */
      SANITY_TILE(ptile, plr_tile->seen_count[V_INVIS]
		   <= plr_tile->seen_count[V_MAIN]);
      SANITY_TILE(ptile, plr_tile->own_seen[V_INVIS]
		   <= plr_tile->own_seen[V_MAIN]);
    } players_iterate_end;
  } whole_map_iterate_end;

  SANITY_CHECK(game.government_during_revolution != NULL);
  SANITY_CHECK(game.government_during_revolution
	       == government_by_number(game.info.government_during_revolution_id));
}

/**********************************************************************//**
  Miscellaneous sanity checks.
**************************************************************************/
static void check_misc(const char *file, const char *function, int line)
{
  int nplayers = 0, nbarbs = 0;

  /* Do not use player_slots_iterate as we want to check the index! */
  player_slots_iterate(pslot) {
    if (player_slot_is_used(pslot)) {
      if (is_barbarian(player_slot_get_player(pslot))) {
        nbarbs++;
      }
      nplayers++;
    }
  } player_slots_iterate_end;

  SANITY_CHECK(nplayers == player_count());
  SANITY_CHECK(nbarbs == server.nbarbarians);

  SANITY_CHECK(player_count() <= player_slot_count());
  SANITY_CHECK(team_count() <= MAX_NUM_TEAM_SLOTS);
  SANITY_CHECK(normal_player_count() <= game.server.max_players);
}

/**********************************************************************//**
  Sanity checks on the map itself.  See also check_specials.
**************************************************************************/
static void check_map(const char *file, const char *function, int line)
{
  whole_map_iterate(&(wld.map), ptile) {
    struct city *pcity = tile_city(ptile);
    int cont = tile_continent(ptile);

    CHECK_INDEX(tile_index(ptile));

    if (NULL != pcity) {
      SANITY_TILE(ptile, same_pos(pcity->tile, ptile));
      if (BORDERS_DISABLED != game.info.borders) {
        SANITY_TILE(ptile, tile_owner(ptile) != NULL);
      }
    }

    if (is_ocean_tile(ptile)) {
      SANITY_TILE(ptile, cont < 0);
      adjc_iterate(&(wld.map), ptile, tile1) {
	if (is_ocean_tile(tile1)) {
	  SANITY_TILE(ptile, tile_continent(tile1) == cont);
	}
      } adjc_iterate_end;
    } else {
      SANITY_TILE(ptile, cont > 0);
      adjc_iterate(&(wld.map), ptile, tile1) {
	if (!is_ocean_tile(tile1)) {
	  SANITY_TILE(ptile, tile_continent(tile1) == cont);
	}
      } adjc_iterate_end;
    }

    unit_list_iterate(ptile->units, punit) {
      SANITY_TILE(ptile, same_pos(unit_tile(punit), ptile));

      /* Check diplomatic status of stacked units. */
      unit_list_iterate(ptile->units, punit2) {
	SANITY_TILE(ptile, pplayers_allied(unit_owner(punit), 
                                           unit_owner(punit2)));
      } unit_list_iterate_end;
      if (pcity) {
	SANITY_TILE(ptile, pplayers_allied(unit_owner(punit), 
                                           city_owner(pcity)));
      }
    } unit_list_iterate_end;
  } whole_map_iterate_end;
}

/**********************************************************************//**
  Verify that the city itself has sane values.
**************************************************************************/
static bool check_city_good(struct city *pcity, const char *file,
                            const char *function, int line)
{
  struct player *pplayer = city_owner(pcity);
  struct tile *pcenter = city_tile(pcity);

  if (NULL == pcenter) {
    /* Editor! */
    SANITY_FAIL("(----,----) city has no tile (skipping remaining tests), "
                "at %s \"%s\"[%d]%s",
                nation_rule_name(nation_of_player(pplayer)),
                city_name_get(pcity), city_size_get(pcity),
                "{city center}");
    return FALSE;
  }

  SANITY_CITY(pcity, !terrain_has_flag(tile_terrain(pcenter), TER_NO_CITIES));

  if (BORDERS_DISABLED != game.info.borders) {
    SANITY_CITY(pcity, NULL != tile_owner(pcenter));
  }

  if (NULL != tile_owner(pcenter)) {
    if (tile_owner(pcenter) != pplayer) {
      SANITY_FAIL("(%4d,%4d) tile owned by %s, at %s \"%s\"[%d]%s",
                  TILE_XY(pcenter),
                  nation_rule_name(nation_of_player(tile_owner(pcenter))),
                  nation_rule_name(nation_of_player(pplayer)),
                  city_name_get(pcity), city_size_get(pcity),
                  "{city center}");
    }
  }

  unit_list_iterate(pcity->units_supported, punit) {
    SANITY_CITY(pcity, punit->homecity == pcity->id);
    SANITY_CITY(pcity, unit_owner(punit) == pplayer);
  } unit_list_iterate_end;

  city_built_iterate(pcity, pimprove) {
    if (is_small_wonder(pimprove)) {
      SANITY_CITY(pcity, city_from_small_wonder(pplayer, pimprove) == pcity);
    } else if (is_great_wonder(pimprove)) {
      SANITY_CITY(pcity, city_from_great_wonder(pimprove) == pcity);
    }
  } city_built_iterate_end;

  trade_routes_iterate(pcity, proute) {
    struct city *partner = game_city_by_number(proute->partner);

    if (partner != NULL) {
      struct trade_route *back_route = NULL;

      trade_routes_iterate(partner, pback) {
        if (pback->partner == pcity->id) {
          back_route = pback;
          break;
        }
      } trade_routes_iterate_end;

      SANITY_CITY(pcity, back_route != NULL);

      if (back_route != NULL) {
        switch (back_route->dir) {
        case RDIR_TO:
          SANITY_CITY(pcity, proute->dir == RDIR_FROM);
          break;
        case RDIR_FROM:
          SANITY_CITY(pcity, proute->dir == RDIR_TO);
          break;
        case RDIR_BIDIRECTIONAL:
          SANITY_CITY(pcity, proute->dir == RDIR_BIDIRECTIONAL);
          break;
        }

        SANITY_CITY(pcity, proute->goods == back_route->goods);
      }
    }
  } trade_routes_iterate_end;

  return TRUE;
}

/**********************************************************************//**
  Sanity check city size versus worker and specialist counts.
**************************************************************************/
static void check_city_size(struct city *pcity, const char *file,
                            const char *function, int line)
{
  int delta;
  int citizen_count = 0;
  struct tile *pcenter = city_tile(pcity);

  SANITY_CITY(pcity, city_size_get(pcity) >= 1);

  city_tile_iterate_skip_free_worked(city_map_radius_sq_get(pcity), pcenter,
                                  ptile, _index, _x, _y) {
    if (tile_worked(ptile) == pcity) {
      citizen_count++;
    }
  } city_tile_iterate_skip_free_worked_end;

  citizen_count += city_specialists(pcity);
  delta = city_size_get(pcity) - citizen_count;
  if (0 != delta) {
    SANITY_FAIL("(%4d,%4d) %d citizens not equal [size], "
                "repairing \"%s\"[%d]", TILE_XY(pcity->tile),
                citizen_count, city_name_get(pcity), city_size_get(pcity));

    citylog_map_workers(LOG_DEBUG, pcity);
    log_debug("[%s (%d)] specialists: %d", city_name_get(pcity), pcity->id,
              city_specialists(pcity));

    city_repair_size(pcity, delta);
    city_refresh_from_main_map(pcity, NULL);
  }
}

/**********************************************************************//**
  Verify that the number of people with feelings + specialists equal
  city size.
**************************************************************************/
static void check_city_feelings(const struct city *pcity, const char *file,
                                const char *function, int line)
{
  int feel;
  int spe = city_specialists(pcity);

  for (feel = FEELING_BASE; feel < FEELING_LAST; feel++) {
    int sum = 0;
    int ccategory;

    for (ccategory = CITIZEN_HAPPY; ccategory < CITIZEN_LAST; ccategory++) {
      sum += pcity->feel[ccategory][feel];
    }

    /* While loading savegame, we want to check sanity of values read from the
     * savegame despite the fact that city workers_frozen level of the city
     * is above zero -> can't limit sanitycheck callpoints by that. Instead
     * we check even more relevant needs_arrange. */
    SANITY_CITY(pcity, !pcity->server.needs_arrange);

    SANITY_CITY(pcity, city_size_get(pcity) - spe == sum);
  }
}

/**********************************************************************//**
  Verify that the city has sane values.
**************************************************************************/
void real_sanity_check_city(struct city *pcity, const char *file,
                            const char *function, int line)
{
  if (check_city_good(pcity, file, function, line)) {
    check_city_size(pcity, file, function, line);
    check_city_feelings(pcity, file, function, line);
  }
}

/**********************************************************************//**
  Sanity checks on all cities in the world.
**************************************************************************/
static void check_cities(const char *file, const char *function, int line)
{
  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      SANITY_CITY(pcity, city_owner(pcity) == pplayer);

      real_sanity_check_city(pcity, file, function, line);
    } city_list_iterate_end;
  } players_iterate_end;
}

/**********************************************************************//**
  Sanity checks on all units in the world.
**************************************************************************/
static void check_units(const char *file, const char *function, int line)
{
  players_iterate(pplayer) {
    unit_list_iterate(pplayer->units, punit) {
      struct tile *ptile = unit_tile(punit);
      struct terrain *pterr = tile_terrain(ptile);
      struct city *pcity;
      struct city *phome;
      struct unit *ptrans = unit_transport_get(punit);

      SANITY_CHECK(unit_owner(punit) == pplayer);

      if (IDENTITY_NUMBER_ZERO != punit->homecity) {
        SANITY_CHECK(phome = player_city_by_number(pplayer,
                                                   punit->homecity));
        if (phome) {
          SANITY_CHECK(city_owner(phome) == pplayer);
        }
      }

      /* Unit in the correct player list? */
      SANITY_CHECK(player_unit_by_number(unit_owner(punit),
                                         punit->id) != NULL);

      if (!can_unit_continue_current_activity(punit)) {
        SANITY_FAIL("(%4d,%4d) %s has activity %s, "
                    "but it can't continue at %s",
                    TILE_XY(ptile), unit_rule_name(punit),
                    get_activity_text(punit->activity),
                    tile_get_info_text(ptile, TRUE, 0));
      }

      if (activity_requires_target(punit->activity)
          && (punit->activity != ACTIVITY_IRRIGATE || pterr->irrigation_result == pterr)
          && (punit->activity != ACTIVITY_MINE || pterr->mining_result == pterr)) {
        SANITY_CHECK(punit->activity_target != NULL);
      }

      pcity = tile_city(ptile);
      if (pcity) {
	SANITY_CHECK(pplayers_allied(city_owner(pcity), pplayer));
      }

      SANITY_CHECK(punit->moves_left >= 0);
      SANITY_CHECK(punit->hp > 0);

      /* Check for ground units in the ocean. */
      SANITY_CHECK(can_unit_exist_at_tile(&(wld.map), punit, ptile)
                   || ptrans != NULL);

      /* Check for over-full transports. */
      SANITY_CHECK(get_transporter_occupancy(punit)
                   <= get_transporter_capacity(punit));

      /* Check transporter. This should be last as the pointer ptrans will
       * be modified. */
      if (ptrans != NULL) {
        struct unit *plevel = punit;
        int level = 0;

        /* Make sure the transporter is on the tile. */
        SANITY_CHECK(same_pos(unit_tile(punit), unit_tile(ptrans)));

        /* Can punit be cargo for its transporter? */
        SANITY_CHECK(unit_transport_check(punit, ptrans));

        /* Check that the unit is listed as transported. */
        SANITY_CHECK(unit_list_search(unit_transport_cargo(ptrans),
                                      punit) != NULL);

        /* Check the depth of the transportation. */
        while (ptrans) {
          struct unit_list *pcargos = unit_transport_cargo(ptrans);

          SANITY_CHECK(pcargos != NULL);
          SANITY_CHECK(level < GAME_TRANSPORT_MAX_RECURSIVE);

          /* Check for next level. */
          plevel = ptrans;
          ptrans = unit_transport_get(plevel);
          level++;
        }

        /* Transporter capacity will be checked when transporter itself
         * is checked */
      }

      /* Check that cargo is marked as transported with this unit */
      unit_list_iterate(unit_transport_cargo(punit), pcargo) {
        SANITY_CHECK(unit_transport_get(pcargo) == punit);
      } unit_list_iterate_end;
    } unit_list_iterate_end;
  } players_iterate_end;
}

/**********************************************************************//**
  Sanity checks on all players.
**************************************************************************/
static void check_players(const char *file, const char *function, int line)
{
  players_iterate(pplayer) {
    int found_palace = 0;

    if (!pplayer->is_alive) {
      /* Dead players' units and cities are disbanded in kill_player(). */
      SANITY_CHECK(unit_list_size(pplayer->units) == 0);
      SANITY_CHECK(city_list_size(pplayer->cities) == 0);

      continue;
    }

    SANITY_CHECK(pplayer->server.adv != NULL);
    SANITY_CHECK(!pplayer->nation || pplayer->nation->player == pplayer);
    SANITY_CHECK(player_list_search(team_members(pplayer->team), pplayer));

    SANITY_CHECK(!(city_list_size(pplayer->cities) > 0
                   && !pplayer->server.got_first_city));

    city_list_iterate(pplayer->cities, pcity) {
      if (is_capital(pcity)) {
	found_palace++;
      }
      SANITY_CITY(pcity, found_palace <= 1);
    } city_list_iterate_end;

    players_iterate(pplayer2) {
      struct player_diplstate *state1, *state2;

      if (pplayer2 == pplayer) {
        break; /* Do diplomatic sanity check only once per player couple. */
      }

      state1 = player_diplstate_get(pplayer, pplayer2);
      state2 = player_diplstate_get(pplayer2, pplayer);
      SANITY_CHECK(state1->type == state2->type);
      SANITY_CHECK(state1->max_state == state2->max_state);
      if (state1->type == DS_CEASEFIRE
          || state1->type == DS_ARMISTICE) {
        SANITY_CHECK(state1->turns_left == state2->turns_left);
      }
      if (state1->type == DS_TEAM) {
        SANITY_CHECK(players_on_same_team(pplayer, pplayer2));
        SANITY_CHECK(player_has_real_embassy(pplayer, pplayer2));
        SANITY_CHECK(player_has_real_embassy(pplayer2, pplayer));
        if (pplayer->is_alive && pplayer2->is_alive) {
          SANITY_CHECK(really_gives_vision(pplayer, pplayer2));
          SANITY_CHECK(really_gives_vision(pplayer2, pplayer));
        }
      }
      if (pplayer->is_alive
          && pplayer2->is_alive
          && pplayers_allied(pplayer, pplayer2)) {
        enum dipl_reason allied_players_can_be_allied =
          pplayer_can_make_treaty(pplayer, pplayer2, DS_ALLIANCE);
        SANITY_CHECK(allied_players_can_be_allied
                     != DIPL_ALLIANCE_PROBLEM_US);
        SANITY_CHECK(allied_players_can_be_allied
                     != DIPL_ALLIANCE_PROBLEM_THEM);
      }
    } players_iterate_end;

    if (pplayer->revolution_finishes == -1) {
      if (government_of_player(pplayer) == game.government_during_revolution) {
        SANITY_FAIL("%s government is anarchy, but does not finish!",
                    nation_rule_name(nation_of_player(pplayer)));
      }
      SANITY_CHECK(government_of_player(pplayer) != game.government_during_revolution);
    } else if (pplayer->revolution_finishes > game.info.turn) {
      SANITY_CHECK(government_of_player(pplayer) == game.government_during_revolution);
    } else {
      /* Things may vary in this case depending on when the sanity_check
       * call is made.  No better check is possible. */
    }

    /* Dying players shouldn't be left around.  But they are. */
    SANITY_CHECK(!BV_ISSET(pplayer->server.status, PSTATUS_DYING));
  } players_iterate_end;

  nations_iterate(pnation) {
    SANITY_CHECK(!pnation->player || pnation->player->nation == pnation);
  } nations_iterate_end;

  teams_iterate(pteam) {
    player_list_iterate(team_members(pteam), pplayer) {
      SANITY_CHECK(pplayer->team == pteam);
    } player_list_iterate_end;
  } teams_iterate_end;
}

/**********************************************************************//**
  Sanity checking on teams.
**************************************************************************/
static void check_teams(const char *file, const char *function, int line)
{
  int count[MAX_NUM_TEAM_SLOTS];

  memset(count, 0, sizeof(count));
  players_iterate(pplayer) {
    /* For the moment, all players have teams. */
    SANITY_CHECK(pplayer->team != NULL);
    if (pplayer->team) {
      count[team_index(pplayer->team)]++;
    }
  } players_iterate_end;

  team_slots_iterate(tslot) {
    if (team_slot_is_used(tslot)) {
      struct team *pteam = team_slot_get_team(tslot);
      fc_assert_exit(pteam);
      SANITY_CHECK(player_list_size(team_members(pteam))
                   == count[team_slot_index(tslot)]);
    }
  } team_slots_iterate_end;
}

/**********************************************************************//**
  Sanity checks on all players.
**************************************************************************/
static void
check_researches(const char *file, const char *function, int line)
{
  researches_iterate(presearch) {
    SANITY_CHECK(S_S_RUNNING != server_state()
                 || A_UNSET == presearch->researching
                 || is_future_tech(presearch->researching)
                 || (A_NONE != presearch->researching
                     && valid_advance_by_number(presearch->researching)));
    SANITY_CHECK(A_UNSET == presearch->tech_goal
                 || (A_NONE != presearch->tech_goal
                     && valid_advance_by_number(presearch->tech_goal)));
  } researches_iterate_end;
}

/**********************************************************************//**
  Sanity checking on connections.
**************************************************************************/
static void check_connections(const char *file, const char *function,
                              int line)
{
  /* est_connections is a subset of all_connections */
  SANITY_CHECK(conn_list_size(game.all_connections)
	       >= conn_list_size(game.est_connections));
}

/**********************************************************************//**
  Do sanity checks on the server state.  Call this once per turn or
  whenever you feel like it.

  But be careful, calling it too much would make the server slow down.  And
  at some times the server isn't supposed to be in a sane state so you
  can't call it in the middle of an operation that is supposed to be
  atomic.
**************************************************************************/
void real_sanity_check(const char *file, const char *function, int line)
{
  if (!map_is_empty()) {
    /* Don't sanity-check the map if it hasn't been created yet (this
     * happens when loading scenarios). */
    check_specials(file, function, line);
    check_map(file, function, line);
    check_cities(file, function, line);
    check_units(file, function, line);
    check_fow(file, function, line);
  }
  check_misc(file, function, line);
  check_players(file, function, line);
  check_teams(file, function, line);
  check_researches(file, function, line);
  check_connections(file, function, line);
}

/**********************************************************************//**
  Verify that the tile has sane values. This should be called after the
  terrain is changed.
**************************************************************************/
void real_sanity_check_tile(struct tile *ptile, const char *file,
                            const char *function, int line)
{
  SANITY_CHECK(ptile != NULL);
  SANITY_CHECK(ptile->terrain != NULL);

  unit_list_iterate(ptile->units, punit) {
    /* Check if the units can survive on the tile (terrain). Here only the
     * 'easy' test if the unit is transported is done. A complete check is
     * done by check_units() in real_sanity_check(). */
    if (!can_unit_exist_at_tile(&(wld.map), punit, ptile)
        && !unit_transported(punit)) {
      SANITY_FAIL("(%4d,%4d) %s can't survive on %s", TILE_XY(ptile),
                  unit_rule_name(punit), tile_get_info_text(ptile, TRUE, 0));
    }
  } unit_list_iterate_end;
}

#endif /* SANITY_CHECKING */
