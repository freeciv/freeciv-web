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

#include <stdio.h> /* for remove() */ 

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "string_vector.h"
#include "support.h"

/* common */
#include "ai.h"
#include "calendar.h"
#include "events.h"
#include "game.h"
#include "improvement.h"
#include "movement.h"
#include "packets.h"

/* server */
#include "citytools.h"
#include "connecthand.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "srv_main.h"
#include "unittools.h"

/* server/advisors */
#include "advdata.h"

#include "gamehand.h"

#define CHALLENGE_ROOT "challenge"

#define SPECLIST_TAG startpos
#define SPECLIST_TYPE struct startpos
#include "speclist.h"
#define startpos_list_iterate(list, plink, psp)                             \
  TYPED_LIST_BOTH_ITERATE(struct startpos_list_link, struct startpos,       \
                          list, plink, psp)
#define startpos_list_iterate_end LIST_BOTH_ITERATE_END

struct team_placement_config {
  struct tile **startpos;
  int flexible_startpos_num;
  int usable_startpos_num;
  int total_startpos_num;
};

struct team_placement_state {
  int *startpos;
  long score;
};

#define SPECPQ_TAG team_placement
#define SPECPQ_DATA_TYPE struct team_placement_state *
#define SPECPQ_PRIORITY_TYPE long
#include "specpq.h"

static struct strvec *ruleset_choices = NULL;

/************************************************************************//**
  Get role_id for given role character
****************************************************************************/
enum unit_role_id crole_to_role_id(char crole)
{
  switch (crole) {
  case 'c':
    return L_START_CITIES;
  case 'w':
    return L_START_WORKER;
  case 'x':
    return L_START_EXPLORER;
  case 'k':
    return L_START_KING;
  case 's':
    return L_START_DIPLOMAT;
  case 'f':
    return L_START_FERRY;
  case 'd':
    return L_START_DEFEND_OK;
  case 'D':
    return L_START_DEFEND_GOOD;
  case 'a':
    return L_START_ATTACK_FAST;
  case 'A':
    return L_START_ATTACK_STRONG;
  default: 
    return 0;
  }
}

/************************************************************************//**
  Get unit_type for given role character
****************************************************************************/
struct unit_type *crole_to_unit_type(char crole, struct player *pplayer)
{
  struct unit_type *utype = NULL;
  enum unit_role_id role = crole_to_role_id(crole);

  if (role == 0) {
    fc_assert_ret_val(FALSE, NULL);
    return NULL;
  }

  /* Create the unit of an appropriate type, if it exists */
  if (num_role_units(role) > 0) {
    if (pplayer != NULL) {
      utype = first_role_unit_for_player(pplayer, role);
    }
    if (utype == NULL) {
      utype = get_role_unit(role, 0);
    }
  }

  return utype;
}

/************************************************************************//**
  Place a starting unit for the player. Returns tile where unit was really
  placed.
****************************************************************************/
static struct tile *place_starting_unit(struct tile *starttile,
                                        struct player *pplayer,
                                        char crole)
{
  struct tile *ptile = NULL;
  struct unit_type *utype = crole_to_unit_type(crole, pplayer);
  bool hut_present = FALSE;

  if (utype != NULL) {
    iterate_outward(&(wld.map), starttile,
                    wld.map.xsize + wld.map.ysize, itertile) {
      if (!is_non_allied_unit_tile(itertile, pplayer)
          && is_native_tile(utype, itertile)) {
        ptile = itertile;
        break;
      }
    } iterate_outward_end;
  }

  if (ptile == NULL) {
    /* No place where unit may exist. */
    return NULL;
  }

  fc_assert_ret_val(!is_non_allied_unit_tile(ptile, pplayer), NULL);

  /* For scenarios or dispersion, huts may coincide with player starts (in 
   * other cases, huts are avoided as start positions).  Remove any such hut,
   * and make sure to tell the client, since we may have already sent this
   * tile (with the hut) earlier: */
  extra_type_by_cause_iterate(EC_HUT, pextra) {
    if (tile_has_extra(ptile, pextra)) {
      tile_extra_rm_apply(ptile, pextra);
      hut_present = TRUE;
    }
  } extra_type_by_cause_iterate_end;

  if (hut_present) {
    update_tile_knowledge(ptile);
    log_verbose("Removed hut on start position for %s",
                player_name(pplayer));
  }

  /* Expose visible area. */
  map_show_circle(pplayer, ptile, game.server.init_vis_radius_sq);

  if (utype != NULL) {
    (void) create_unit(pplayer, ptile, utype, FALSE, 0, 0);
    return ptile;
  }

  return NULL;
}

/************************************************************************//**
  Find a valid position not far from our starting position.
****************************************************************************/
static struct tile *find_dispersed_position(struct player *pplayer,
                                            struct tile *pcenter)
{
  struct tile *ptile;
  int x, y;

  do {
    index_to_map_pos(&x, &y, tile_index(pcenter));
    x += fc_rand(2 * game.server.dispersion + 1) - game.server.dispersion;
    y += fc_rand(2 * game.server.dispersion + 1) - game.server.dispersion;
  } while (!((ptile = map_pos_to_tile(&(wld.map), x, y))
             && tile_continent(pcenter) == tile_continent(ptile)
             && !is_ocean_tile(ptile)
             && real_map_distance(pcenter, ptile) < game.server.dispersion
                + 1
             && !is_non_allied_unit_tile(ptile, pplayer)));

  return ptile;
}

/* Calculate the distance between tiles, according to the 'teamplacement'
 * setting set to 'CLOSEST'. */
#define team_placement_closest sq_map_distance

/************************************************************************//**
  Calculate the distance between tiles, according to the 'teamplacement'
  setting set to 'CONTINENT'.
****************************************************************************/
static int team_placement_continent(const struct tile *ptile1,
                                    const struct tile *ptile2)
{
  return (ptile1->continent == ptile2->continent
          ? sq_map_distance(ptile1, ptile2)
          : sq_map_distance(ptile1, ptile2) + MAP_INDEX_SIZE);
}

/************************************************************************//**
  Calculate the distance between tiles, according to the 'teamplacement'
  setting set to 'HORIZONTAL'.
****************************************************************************/
static int team_placement_horizontal(const struct tile *ptile1,
                                     const struct tile *ptile2)
{
  int dx, dy;

  map_distance_vector(&dx, &dy, ptile1, ptile2);
  /* Map vector to natural vector (Y axis). */
  return abs(MAP_IS_ISOMETRIC ? dx + dy : dy);
}

/************************************************************************//**
  Calculate the distance between tiles, according to the 'teamplacement'
  setting set to 'VERTICAL'.
****************************************************************************/
static int team_placement_vertical(const struct tile *ptile1,
                                   const struct tile *ptile2)
{
  int dx, dy;

  map_distance_vector(&dx, &dy, ptile1, ptile2);
  /* Map vector to natural vector (X axis). */
  return abs(MAP_IS_ISOMETRIC ? dx - dy : dy);
}

/************************************************************************//**
  Destroys a team_placement_state structure.
****************************************************************************/
static void team_placement_state_destroy(struct team_placement_state *pstate)
{
  free(pstate->startpos);
  free(pstate);
}

/************************************************************************//**
  Find the best team placement, according to the 'team_placement' setting.
****************************************************************************/
static void do_team_placement(const struct team_placement_config *pconfig,
                              struct team_placement_state *pbest_state,
                              int iter_max)
{
  const size_t state_array_size = (sizeof(*pbest_state->startpos)
                                   * pconfig->total_startpos_num);
  struct team_placement_pq *pqueue =
      team_placement_pq_new(pconfig->total_startpos_num * 4);
  int (*distance)(const struct tile *, const struct tile *) = NULL;
  struct team_placement_state *pstate, *pnew;
  const struct tile *ptile1, *ptile2;
  long base_delta, delta;
  bool base_delta_calculated;
  int iter = 0;
  bool repeat;
  int i, j, k, t1, t2;

  switch (wld.map.server.team_placement) {
  case TEAM_PLACEMENT_CLOSEST:
    distance = team_placement_closest;
    break;
  case TEAM_PLACEMENT_CONTINENT:
    distance = team_placement_continent;
    break;
  case TEAM_PLACEMENT_HORIZONTAL:
    distance = team_placement_horizontal;
    break;
  case TEAM_PLACEMENT_VERTICAL:
    distance = team_placement_vertical;
    break;
  case TEAM_PLACEMENT_DISABLED:
    break;
  }
  fc_assert_ret_msg(distance != NULL, "Wrong team_placement variant (%d)",
                    wld.map.server.team_placement);

  /* Initialize starting state. */
  pstate = fc_malloc(sizeof(*pstate));
  pstate->startpos = fc_malloc(state_array_size);
  memcpy(pstate->startpos, pbest_state->startpos, state_array_size);
  pstate->score = pbest_state->score;

  do {
    repeat = FALSE;
    for (i = 0; i < pconfig->usable_startpos_num; i++) {
      t1 = pstate->startpos[i];
      if (t1 == -1) {
        continue; /* Not used. */
      }
      ptile1 = pconfig->startpos[i];
      base_delta_calculated = FALSE;
      for (j = i + 1; j < (i >= pconfig->flexible_startpos_num
                           ? pconfig->usable_startpos_num
                           : pconfig->flexible_startpos_num); j++) {
        t2 = pstate->startpos[j];
        if (t2 == -1) {
          /* Not assigned yet. */
          ptile2 = pconfig->startpos[j];
          if (base_delta_calculated) {
            delta = base_delta;
            for (k = 0; k < pconfig->total_startpos_num; k++) {
              if (k != i && t1 == pstate->startpos[k]) {
                delta += distance(ptile2, pconfig->startpos[k]);
              }
            }
          } else {
            delta = 0;
            base_delta = 0;
            for (k = 0; k < pconfig->total_startpos_num; k++) {
              if (k != i && t1 == pstate->startpos[k]) {
                base_delta -= distance(ptile1, pconfig->startpos[k]);
                delta += distance(ptile2, pconfig->startpos[k]);
              }
            }
            delta += base_delta;
            base_delta_calculated = TRUE;
          }
        } else if (t1 < t2) {
          ptile2 = pconfig->startpos[j];
          if (base_delta_calculated) {
            delta = base_delta;
            for (k = 0; k < pconfig->total_startpos_num; k++) {
              if (k != i && t1 == pstate->startpos[k]) {
                delta += distance(ptile2, pconfig->startpos[k]);
              } else if (k != j && t2 == pstate->startpos[k]) {
                delta -= distance(ptile2, pconfig->startpos[k]);
                delta += distance(ptile1, pconfig->startpos[k]);
              }
            }
          } else {
            delta = 0;
            base_delta = 0;
            for (k = 0; k < pconfig->total_startpos_num; k++) {
              if (k != i && t1 == pstate->startpos[k]) {
                base_delta -= distance(ptile1, pconfig->startpos[k]);
                delta += distance(ptile2, pconfig->startpos[k]);
              } else if (k != j && t2 == pstate->startpos[k]) {
                delta -= distance(ptile2, pconfig->startpos[k]);
                delta += distance(ptile1, pconfig->startpos[k]);
              }
            }
            delta += base_delta;
            base_delta_calculated = TRUE;
          }
        } else {
          continue;
        }

        if (delta <= 0) {
          repeat = TRUE;
          pnew = fc_malloc(sizeof(*pnew));
          pnew->startpos = fc_malloc(state_array_size);
          memcpy(pnew->startpos, pstate->startpos, state_array_size);
          pnew->startpos[i] = t2;
          pnew->startpos[j] = t1;
          pnew->score = pstate->score + delta;
          team_placement_pq_insert(pqueue, pnew, -pnew->score);

          if (pnew->score < pbest_state->score) {
            memcpy(pbest_state->startpos, pnew->startpos, state_array_size);
            pbest_state->score = pnew->score;
          }
        }
      }
    }

    team_placement_state_destroy(pstate);
    if (iter++ >= iter_max) {
      log_normal(_("Didn't find optimal solution for team placement "
                   "in %d iterations."), iter);
      break;
    }

  } while (repeat && team_placement_pq_remove(pqueue, &pstate));

  team_placement_pq_destroy_full(pqueue, team_placement_state_destroy);
}

/************************************************************************//**
  Initialize a new game: place the players' units onto the map, etc.
****************************************************************************/
void init_new_game(void)
{
  struct startpos_list *impossible_list, *targeted_list, *flexible_list;
  struct tile *player_startpos[player_slot_count()];
  int placed_units[player_slot_count()];
  int players_to_place = player_count();
  int sulen;

  randomize_base64url_string(server.game_identifier,
                             sizeof(server.game_identifier));

  /* Assign players to starting positions on the map.
   * (In scenarios with restrictions on which nations can use which predefined
   * start positions, this process tries to satisfy those restrictions, but
   * does not guarantee to. Even if there is a solution to the matching
   * problem, this algorithm may not find it.) */

  fc_assert(player_count() <= map_startpos_count());

  /* Convert the startposition hash table in a linked lists, as we mostly
   * need now to iterate it now. And then, we will be able to remove the
   * assigned start postions one by one. */
  impossible_list = startpos_list_new();
  targeted_list = startpos_list_new();
  flexible_list = startpos_list_new();

  map_startpos_iterate(psp) {
    if (startpos_allows_all(psp)) {
      startpos_list_append(flexible_list, psp);
    } else {
      startpos_list_append(targeted_list, psp);
    }
  } map_startpos_iterate_end;

  fc_assert(startpos_list_size(targeted_list)
            + startpos_list_size(flexible_list) == map_startpos_count());

  memset(player_startpos, 0, sizeof(player_startpos));
  log_verbose("Placing players at start positions.");

  /* First assign start positions which have restrictions on which nations
   * can use them. */
  if (0 < startpos_list_size(targeted_list)) {
    log_verbose("Assigning matching nations.");

    startpos_list_shuffle(targeted_list); /* Randomize. */
    do {
      struct nation_type *pnation;
      struct startpos_list_link *choice;
      bool removed = FALSE;

      /* Assign first players which can pick only one start position. */
      players_iterate(pplayer) {
        if (NULL != player_startpos[player_index(pplayer)]) {
          /* Already assigned. */
          continue;
        }

        pnation = nation_of_player(pplayer);
        choice = NULL;
        startpos_list_iterate(targeted_list, plink, psp) {
          if (startpos_nation_allowed(psp, pnation)) {
            if (NULL != choice) {
              choice = NULL;
              break; /* Many choices. */
            } else {
              choice = plink;
            }
          }
        } startpos_list_iterate_end;

        if (NULL != choice) {
          /* Assign this start position to this player and remove
           * both from consideration. */
          struct tile *ptile =
              startpos_tile(startpos_list_link_data(choice));

          player_startpos[player_index(pplayer)] = ptile;
          startpos_list_erase(targeted_list, choice);
          players_to_place--;
          removed = TRUE;
          log_verbose("Start position (%d, %d) exactly matches player %s (%s).",
                      TILE_XY(ptile), player_name(pplayer),
                      nation_rule_name(pnation));
        }
      } players_iterate_end;

      if (!removed) {
        /* Didn't find any 1:1 matches. For the next restricted start
         * position, assign a random matching player. (This may create
         * restrictions such that more 1:1 matches are possible.) */
        struct startpos *psp = startpos_list_back(targeted_list);
        struct tile *ptile = startpos_tile(psp);
        struct player *rand_plr = NULL;
        int i = 0;

        startpos_list_pop_back(targeted_list); /* Detach 'psp'. */
        players_iterate(pplayer) {
          if (NULL != player_startpos[player_index(pplayer)]) {
            /* Already assigned. */
            continue;
          }

          pnation = nation_of_player(pplayer);
          if (startpos_nation_allowed(psp, pnation) && 0 == fc_rand(++i)) {
            rand_plr = pplayer;
          }
        } players_iterate_end;

        if (NULL != rand_plr) {
          player_startpos[player_index(rand_plr)] = ptile;
          players_to_place--;
          log_verbose("Start position (%d, %d) matches player %s (%s).",
                      TILE_XY(ptile), player_name(rand_plr),
                      nation_rule_name(nation_of_player(rand_plr)));
        } else {
          /* This start position cannot be assigned, given the assignments
           * made so far. We may have to fall back to mismatched
           * assignments. */
          log_verbose("Start position (%d, %d) cannot be assigned for "
                      "any player, keeping for the moment...",
                      TILE_XY(ptile));
          /* Keep it for later, we may need it. */
          startpos_list_append(impossible_list, psp);
        }
      }
    } while (0 < players_to_place && 0 < startpos_list_size(targeted_list));
  }

  /* Now try to assign with regard to the 'teamplacement' setting. */
  if (players_to_place > 0
      && wld.map.server.team_placement != TEAM_PLACEMENT_DISABLED
      && player_count() > team_count()) {
    const struct player_list *members;
    int team_placement_players_to_place = 0;
    int real_team_count = 0;

    teams_iterate(pteam) {
      members = team_members(pteam);
      fc_assert(0 < player_list_size(members));
      real_team_count++;
      if (player_list_size(members) == 1) {
        /* Single player teams, doesn't count for team placement. */
        continue;
      }
      player_list_iterate(members, pplayer) {
        if (player_startpos[player_index(pplayer)] == NULL) {
          team_placement_players_to_place++;
        }
      } player_list_iterate_end;
    } teams_iterate_end;

    if (real_team_count > 1 && team_placement_players_to_place > 0) {
      /* We really can do something to improve team placement. */
      struct team_placement_config config;
      struct team_placement_state state;
      int i, j, t;

      log_verbose("Do team placement for %d players, using %s variant.",
                  team_placement_players_to_place,
                  team_placement_name(wld.map.server.team_placement));

      /* Initialize configuration. */
      config.flexible_startpos_num = startpos_list_size(flexible_list);
      config.usable_startpos_num = config.flexible_startpos_num;
      if (config.flexible_startpos_num < team_placement_players_to_place) {
        config.usable_startpos_num += startpos_list_size(impossible_list);
      }
      config.total_startpos_num = (config.usable_startpos_num
                                   + player_count() - players_to_place);
      config.startpos = fc_malloc(sizeof(*config.startpos)
                                  * config.total_startpos_num);
      i = 0;
      startpos_list_iterate(flexible_list, plink, psp) {
        config.startpos[i++] = startpos_tile(psp);
      } startpos_list_iterate_end;
      fc_assert(i == config.flexible_startpos_num);
      if (i < config.usable_startpos_num) {
        startpos_list_iterate(impossible_list, plink, psp) {
          config.startpos[i++] = startpos_tile(psp);
        } startpos_list_iterate_end;
      }
      fc_assert(i == config.usable_startpos_num);
      while (i < config.total_startpos_num) {
        config.startpos[i++] = NULL;
      }
      fc_assert(i == config.total_startpos_num);

      /* Initialize state. */
      state.startpos = fc_malloc(sizeof(*state.startpos)
                                 * config.total_startpos_num);
      state.score = 0;
      i = 0;
      j = config.usable_startpos_num;
      teams_iterate(pteam) {
        members = team_members(pteam);
        if (player_list_size(members) <= 1) {
          /* Single player teams, doesn't count for team placement. */
          continue;
        }
        t = team_number(pteam);
        player_list_iterate(members, pplayer) {
          struct tile *ptile = player_startpos[player_index(pplayer)];

          if (ptile == NULL) {
            state.startpos[i++] = t;
          } else {
            state.startpos[j] = t;
            config.startpos[j] = ptile;
            j++;
          }
        } player_list_iterate_end;
      } teams_iterate_end;
      while (i < config.usable_startpos_num) {
        state.startpos[i++] = -1;
      }
      fc_assert(i == config.usable_startpos_num);
      while (j < config.total_startpos_num) {
        state.startpos[j++] = -1;
      }
      fc_assert(j == config.total_startpos_num);

      /* Look for best team placement. */
      do_team_placement(&config, &state, team_placement_players_to_place);

      /* Apply result. */
      for (i = 0; i < config.usable_startpos_num; i++) {
        t = state.startpos[i];
        if (t != -1) {
          const struct team *pteam = team_by_number(t);
          int candidate_index = -1;
          int candidate_num = 0;

          log_verbose("Start position (%d, %d) assigned to team %d (%s)",
                      TILE_XY(config.startpos[i]),
                      t, team_rule_name(pteam));

          player_list_iterate(team_members(pteam), member) {
            if (player_startpos[player_index(member)] == NULL
                && fc_rand(++candidate_num) == 0) {
              candidate_index = player_index(member);
            }
          } player_list_iterate_end;
          fc_assert(candidate_index >= 0);
          player_startpos[candidate_index] = config.startpos[i];
          team_placement_players_to_place--;
          players_to_place--;
        }
      }
      fc_assert(team_placement_players_to_place == 0);

      /* Free data. */
      if (players_to_place > 0) {
        /* We need to remove used startpos from the lists. */
        i = 0;
        startpos_list_iterate(flexible_list, plink, psp) {
          fc_assert(config.startpos[i] == startpos_tile(psp));
          if (state.startpos[i] != -1) {
            startpos_list_erase(flexible_list, plink);
          }
          i++;
        } startpos_list_iterate_end;
        fc_assert(i == config.flexible_startpos_num);
        if (i < config.usable_startpos_num) {
          startpos_list_iterate(impossible_list, plink, psp) {
            fc_assert(config.startpos[i] == startpos_tile(psp));
            if (state.startpos[i] != -1) {
              startpos_list_erase(impossible_list, plink);
            }
            i++;
          } startpos_list_iterate_end;
        }
        fc_assert(i == config.usable_startpos_num);
      }

      free(config.startpos);
    }
  }

  /* Now assign unrestricted start positions to any remaining players. */
  if (0 < players_to_place && 0 < startpos_list_size(flexible_list)) {
    struct tile *ptile;

    log_verbose("Assigning unrestricted start positions.");

    startpos_list_shuffle(flexible_list); /* Randomize. */
    players_iterate(pplayer) {
      if (NULL != player_startpos[player_index(pplayer)]) {
        /* Already assigned. */
        continue;
      }

      ptile = startpos_tile(startpos_list_front(flexible_list));
      player_startpos[player_index(pplayer)] = ptile;
      players_to_place--;
      startpos_list_pop_front(flexible_list);
      log_verbose("Start position (%d, %d) assigned randomly "
                  "to player %s (%s).", TILE_XY(ptile), player_name(pplayer),
                  nation_rule_name(nation_of_player(pplayer)));
      if (0 == startpos_list_size(flexible_list)) {
        break;
      }
    } players_iterate_end;
  }

  if (0 < players_to_place && 0 < startpos_list_size(impossible_list)) {
    /* We still have players to place, and we have some restricted start
     * positions whose nation requirements can't be satisfied given existing
     * assignments. Fall back to making assignments ignoring the positions'
     * nation requirements. */

    struct tile *ptile;

    log_verbose("Ignoring nation restrictions on remaining start positions.");

    startpos_list_shuffle(impossible_list); /* Randomize. */
    players_iterate(pplayer) {
      if (NULL != player_startpos[player_index(pplayer)]) {
        /* Already assigned. */
        continue;
      }

      ptile = startpos_tile(startpos_list_front(impossible_list));
      player_startpos[player_index(pplayer)] = ptile;
      players_to_place--;
      startpos_list_pop_front(impossible_list);
      log_verbose("Start position (%d, %d) assigned to mismatched "
                  "player %s (%s).", TILE_XY(ptile), player_name(pplayer),
                  nation_rule_name(nation_of_player(pplayer)));
      if (0 == startpos_list_size(impossible_list)) {
        break;
      }
    } players_iterate_end;
  }

  fc_assert(0 == players_to_place);

  startpos_list_destroy(impossible_list);
  startpos_list_destroy(targeted_list);
  startpos_list_destroy(flexible_list);

  sulen = strlen(game.server.start_units);

  /* Loop over all players, creating their initial units... */
  players_iterate(pplayer) {
    struct tile *ptile;

    /* We have to initialise the advisor and ai here as we could make contact
     * to other nations at this point. */
    adv_data_phase_init(pplayer, FALSE);
    CALL_PLR_AI_FUNC(phase_begin, pplayer, pplayer, FALSE);

    ptile = player_startpos[player_index(pplayer)];

    fc_assert_action(NULL != ptile, continue);

    /* Place first city */
    if (game.server.start_city) {
      create_city(pplayer, ptile, city_name_suggestion(pplayer, ptile),
                  NULL);
    }

    if (sulen > 0) {
      /* Place the first unit. */
      if (place_starting_unit(ptile, pplayer,
                              game.server.start_units[0]) != NULL) {
        placed_units[player_index(pplayer)] = 1;
      } else {
        placed_units[player_index(pplayer)] = 0;
      }
    } else {
      placed_units[player_index(pplayer)] = 0;
    }
  } players_iterate_end;

  /* Place all other units. */
  players_iterate(pplayer) {
    int i;
    struct tile *const ptile = player_startpos[player_index(pplayer)];
    struct nation_type *nation = nation_of_player(pplayer);

    fc_assert_action(NULL != ptile, continue);

    /* Place global start units */
    for (i = 1; i < sulen; i++) {
      struct tile *rand_tile = find_dispersed_position(pplayer, ptile);

      /* Create the unit of an appropriate type. */
      if (place_starting_unit(rand_tile, pplayer,
                              game.server.start_units[i]) != NULL) {
        placed_units[player_index(pplayer)]++;
      }
    }

    /* Place nation specific start units (not role based!) */
    i = 0;
    while (NULL != nation->init_units[i] && MAX_NUM_UNIT_LIST > i) {
      struct tile *rand_tile = find_dispersed_position(pplayer, ptile);

      create_unit(pplayer, rand_tile, nation->init_units[i], FALSE, 0, 0);
      placed_units[player_index(pplayer)]++;
      i++;
    }
  } players_iterate_end;

  players_iterate(pplayer) {
    /* Close the active phase for advisor and ai for all players; it was
     * opened in the first loop above. */
    adv_data_phase_done(pplayer);
    CALL_PLR_AI_FUNC(phase_finished, pplayer, pplayer);

    fc_assert_msg(game.server.start_city || 0 < placed_units[player_index(pplayer)],
                  _("No units placed for %s!"), player_name(pplayer));
  } players_iterate_end;
}

/************************************************************************//**
  Tell clients the year, and also update turn_done and nturns_idle fields
  for all players.
****************************************************************************/
void send_year_to_clients(void)
{
  struct packet_new_year apacket;

  players_iterate(pplayer) {
    pplayer->nturns_idle++;
  } players_iterate_end;

  apacket.year = game.info.year;
  apacket.fragments = game.info.fragment_count;
  apacket.turn = game.info.turn;
  lsend_packet_new_year(game.est_connections, &apacket);

  /* Hmm, clients could add this themselves based on above packet? */
  notify_conn(game.est_connections, NULL, E_NEXT_YEAR, ftc_any,
              _("Year: %s"), calendar_text());
}

/************************************************************************//**
  Send game_info packet; some server options and various stuff...
  dest == NULL means game.est_connections

  It may be sent at any time. It MUST be sent before any player info, 
  as it contains the number of players.  To avoid inconsistency, it
  SHOULD be sent after rulesets and any other server settings.
****************************************************************************/
void send_game_info(struct conn_list *dest)
{
  struct packet_timeout_info tinfo;

  if (!dest) {
    dest = game.est_connections;
  }

  tinfo = game.tinfo;

  /* the following values are computed every
     time a packet_game_info packet is created */

  /* Sometimes this function is called before the phase_timer is
   * initialized.  In that case we want to send the dummy value. */
  if (current_turn_timeout() > 0 && game.server.phase_timer) {
    /* Whenever the client sees this packet, it starts a new timer at 0;
     * but the server's timer is only ever reset at the start of a phase
     * (and game.tinfo.seconds_to_phasedone is relative to this).
     * Account for the difference. */
    tinfo.seconds_to_phasedone = game.tinfo.seconds_to_phasedone
      - timer_read_seconds(game.server.phase_timer)
      - game.server.additional_phase_seconds;
  } else {
    /* unused but at least initialized */
    tinfo.seconds_to_phasedone = -1.0;
  }

  conn_list_iterate(dest, pconn) {
    /* Timeout info is separate from other packets since it has to
     * be sent always (it's not 'is-info') while the others are 'is-info'
     * Calendar info has been split from Game info packet to make packet
     * size more tolerable when json protocol is in use. */
    send_packet_game_info(pconn, &(game.info));
    send_packet_calendar_info(pconn, &(game.calendar));
    send_packet_timeout_info(pconn, &tinfo);
  }
  conn_list_iterate_end;
}

/************************************************************************//**
  Send current scenario info. dest NULL causes send to everyone
****************************************************************************/
void send_scenario_info(struct conn_list *dest)
{
  if (!dest) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    send_packet_scenario_info(pconn, &(game.scenario));
  } conn_list_iterate_end;
}

/************************************************************************//**
  Send description of the current scenario. dest NULL causes send to everyone
****************************************************************************/
void send_scenario_description(struct conn_list *dest)
{
  if (!dest) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    send_packet_scenario_description(pconn, &(game.scenario_desc));
  } conn_list_iterate_end;
}

/************************************************************************//**
  adjusts game.info.timeout based on various server options

  timeoutint: adjust game.info.timeout every timeoutint turns
  timeoutinc: adjust game.info.timeout by adding timeoutinc to it.
  timeoutintinc: every time we adjust game.info.timeout, we add timeoutintinc
                 to timeoutint.
  timeoutincmult: every time we adjust game.info.timeout, we multiply timeoutinc
                  by timeoutincmult
****************************************************************************/
int update_timeout(void)
{
  /* if there's no timer or we're doing autogame, do nothing */
  if (game.info.timeout < 1 || game.server.timeoutint == 0) {
    return game.info.timeout;
  }

  if (game.server.timeoutcounter >= game.server.timeoutint) {
    game.info.timeout += game.server.timeoutinc;
    game.server.timeoutinc *= game.server.timeoutincmult;

    game.server.timeoutcounter = 1;
    game.server.timeoutint += game.server.timeoutintinc;

    if (game.info.timeout > GAME_MAX_TIMEOUT) {
      notify_conn(game.est_connections, NULL, E_SETTING, ftc_server,
                  _("The turn timeout has exceeded its maximum value, "
                    "fixing at its maximum."));
      log_debug("game.info.timeout exceeded maximum value");
      game.info.timeout = GAME_MAX_TIMEOUT;
      game.server.timeoutint = 0;
      game.server.timeoutinc = 0;
    } else if (game.info.timeout < 0) {
      notify_conn(game.est_connections, NULL, E_SETTING, ftc_server,
                  _("The turn timeout is smaller than zero, "
                    "fixing at zero."));
      log_debug("game.info.timeout less than zero");
      game.info.timeout = 0;
    }
  } else {
    game.server.timeoutcounter++;
  }

  log_debug("timeout=%d, inc=%d incmult=%d\n   "
            "int=%d, intinc=%d, turns till next=%d",
            game.info.timeout, game.server.timeoutinc,
            game.server.timeoutincmult, game.server.timeoutint,
            game.server.timeoutintinc,
            game.server.timeoutint - game.server.timeoutcounter);

  return game.info.timeout;
}

/************************************************************************//**
  adjusts game.seconds_to_turn_done when enemy moves a unit, we see it and
  the remaining timeout is smaller than the timeoutaddenemymove option.

  It's possible to use a similar function to do that per-player.  In
  theory there should be a separate timeout for each player and the
  added time should only go onto the victim's timer.
****************************************************************************/
void increase_timeout_because_unit_moved(void)
{
  if (current_turn_timeout() > 0 && game.server.timeoutaddenemymove > 0) {
    double maxsec = (timer_read_seconds(game.server.phase_timer)
		     + (double) game.server.timeoutaddenemymove);

    if (maxsec > game.tinfo.seconds_to_phasedone) {
      game.tinfo.seconds_to_phasedone = maxsec;
      send_game_info(NULL);
    }	
  }
}

/************************************************************************//**
  Generate challenge filename for this connection, cannot fail.
****************************************************************************/
static void gen_challenge_filename(struct connection *pc)
{
}

/************************************************************************//**
  Get challenge filename for this connection.
****************************************************************************/
static const char *get_challenge_filename(struct connection *pc)
{
  static char filename[MAX_LEN_PATH];

  fc_snprintf(filename, sizeof(filename), "%s_%d_%d",
      CHALLENGE_ROOT, srvarg.port, pc->id);

  return filename;
}

/************************************************************************//**
  Get challenge full filename for this connection.
****************************************************************************/
static const char *get_challenge_fullname(struct connection *pc)
{
  static char fullname[MAX_LEN_PATH];
  const char *sdir = freeciv_storage_dir();
  const char *cname;

  if (sdir == NULL) {
    return NULL;
  }

  cname = get_challenge_filename(pc);

  if (cname == NULL) {
    return NULL;
  }

  fc_snprintf(fullname, sizeof(fullname), "%s" DIR_SEPARATOR "%s", sdir, cname);

  return fullname;
}

/************************************************************************//**
  Find a file that we can write too, and return it's name.
****************************************************************************/
const char *new_challenge_filename(struct connection *pc)
{
  gen_challenge_filename(pc);
  return get_challenge_filename(pc);
}

/************************************************************************//**
  Call this on a connection with HACK access to send it a set of ruleset
  choices.  Probably this should be called immediately when granting
  HACK access to a connection.
****************************************************************************/
static void send_ruleset_choices(struct connection *pc)
{
  struct packet_ruleset_choices packet;
  size_t i;

  if (ruleset_choices == NULL) {
    /* This is only read once per server invocation.  Add a new ruleset
     * and you have to restart the server. */
    ruleset_choices = fileinfolist(get_data_dirs(), RULESET_SUFFIX);
  }

  packet.ruleset_count = MIN(MAX_NUM_RULESETS, strvec_size(ruleset_choices));
  for (i = 0; i < packet.ruleset_count; i++) {
    sz_strlcpy(packet.rulesets[i], strvec_get(ruleset_choices, i));
  }

  send_packet_ruleset_choices(pc, &packet);
}

/************************************************************************//**
  Free list of ruleset choices.
****************************************************************************/
void ruleset_choices_free(void)
{
  if (ruleset_choices != NULL) {
    strvec_destroy(ruleset_choices);
    ruleset_choices = NULL;
  }
}

/************************************************************************//**
  Opens a file specified by the packet and compares the packet values with
  the file values. Sends an answer to the client once it's done.
****************************************************************************/
void handle_single_want_hack_req(struct connection *pc,
                                 const struct packet_single_want_hack_req *
                                 packet)
{
  struct section_file *secfile;
  const char *token = NULL;
  bool you_have_hack = FALSE;

  if ((secfile = secfile_load(get_challenge_fullname(pc), FALSE))) {
    token = secfile_lookup_str(secfile, "challenge.token");
    you_have_hack = (token && strcmp(token, packet->token) == 0);
    secfile_destroy(secfile);
  } else {
    log_debug("Error reading '%s':\n%s", get_challenge_fullname(pc),
              secfile_error());
  }

  if (!token) {
    log_debug("Failed to read authentication token");
  }

  if (you_have_hack) {
    conn_set_access(pc, ALLOW_HACK, TRUE);
  }

  dsend_packet_single_want_hack_reply(pc, you_have_hack);

  send_ruleset_choices(pc);
  send_conn_info(pc->self, NULL);
}
