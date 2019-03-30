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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "ai.h"
#include "base.h"
#include "citizens.h"
#include "city.h"
#include "culture.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "idex.h"
#include "map.h"
#include "movement.h"
#include "player.h"
#include "requirements.h"
#include "research.h"
#include "road.h"
#include "specialist.h"
#include "tech.h"
#include "traderoutes.h"
#include "unit.h"
#include "unitlist.h"
#include "vision.h"

/* common/scriptcore */
#include "luascript_types.h"

/* server */
#include "barbarian.h"
#include "citizenshand.h"
#include "cityturn.h"
#include "gamehand.h"           /* send_game_info() */
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "sanitycheck.h"
#include "sernet.h"
#include "spacerace.h"
#include "srv_main.h"
#include "techtools.h"
#include "unithand.h"
#include "unittools.h"

/* server/advisors */
#include "advbuilding.h"
#include "advgoto.h"
#include "autosettlers.h"
#include "infracache.h"

/* server/scripting */
#include "script_server.h"

/* ai */
#include "handicaps.h"

/* ai */
#include "handicaps.h"

#include "citytools.h"


/* Queue for pending auto_arrange_workers() */
static struct city_list *arrange_workers_queue = NULL;

/* Suppress sending cities during game_load() and end_phase() */
static bool send_city_suppressed = FALSE;

static bool city_workers_queue_remove(struct city *pcity);

static void announce_trade_route_removal(struct city *pc1, struct city *pc2,
                                         bool source_gone);

/************************************************************************//**
  Freeze the workers (citizens on tiles) for the city.  They will not be
  auto-arranged until unfreeze_workers is called.

  Long explanation:

  Historically auto_arrange_workers was called every time a city changed.
  If the city grew or shrunk, a new tile became available or was removed,
  the function would be called.  However in at least one place this breaks.
  In some operations (like transfer_city) multiple things may change and
  the city is not left in a sane state in between.  Calling
  auto_arrange_workers after each change means it's called with an "insane"
  city.  This can lead at best to a failed sanity check with a wasted call,
  or at worse to a more major bug.  The solution is freeze_workers and
  thaw_workers.

  Call freeze_workers to freeze the auto-arranging of citizens.  So long as
  the freeze is in place no arrangement will be done for this city.  Any
  call to auto_arrange_workers will just queue up an arrangement for later.
  Later when thaw_workers is called, the freeze is removed and the
  auto-arrange will be done if there is any arrangement pending.

  Freezing may safely be done more than once.

  It is thus always safe to call freeze and thaw around any set of city
  actions.  However this is unlikely to be needed in very many places.
****************************************************************************/
void city_freeze_workers(struct city *pcity)
{
  pcity->server.workers_frozen++;
}

/************************************************************************//**
  Thaw (unfreeze) the workers (citizens on tiles) for the city.  The workers
  will be auto-arranged if there is an arrangement pending.  See explanation
  in freeze_workers().
****************************************************************************/
void city_thaw_workers(struct city *pcity)
{
  pcity->server.workers_frozen--;
  fc_assert(pcity->server.workers_frozen >= 0);
  if (pcity->server.workers_frozen == 0 && pcity->server.needs_arrange) {
    city_refresh(pcity); /* Citizen count sanity */
    auto_arrange_workers(pcity);
  }
}

/************************************************************************//**
  Queue pending auto_arrange_workers() for later.
****************************************************************************/
void city_freeze_workers_queue(struct city *pcity)
{
  if (NULL == arrange_workers_queue) {
    arrange_workers_queue = city_list_new();
  } else if (city_list_find_number(arrange_workers_queue, pcity->id)) {
    return;
  }

  city_list_prepend(arrange_workers_queue, pcity);
  city_freeze_workers(pcity);
  pcity->server.needs_arrange = TRUE;
}

/************************************************************************//**
  Remove a city from the queue for later calls to auto_arrange_workers().
  Reterns TRUE if the city was found in the queue.
****************************************************************************/
static bool city_workers_queue_remove(struct city *pcity)
{
  if (arrange_workers_queue == NULL) {
    return FALSE;
  }

  return city_list_remove(arrange_workers_queue, pcity);
}

/************************************************************************//**
  Process the frozen workers.
  Call sync_cities() to send the affected cities to the clients.
****************************************************************************/
void city_thaw_workers_queue(void)
{
  if (NULL == arrange_workers_queue) {
    return;
  }

  city_list_iterate(arrange_workers_queue, pcity) {
    city_thaw_workers(pcity);
  } city_list_iterate_end;

  city_list_destroy(arrange_workers_queue);
  arrange_workers_queue = NULL;
}

/************************************************************************//**
  Returns the priority of the city name at the given position, using its
  own internal algorithm.  Lower priority values are more desired, and all
  priorities are non-negative.

  This function takes into account game.natural_city_names, and should be
  able to deal with any future options we want to add.
****************************************************************************/
static int evaluate_city_name_priority(struct tile *ptile,
                                       const struct nation_city *pncity,
                                       int default_priority)
{
  /* Lower values mean higher priority. */
  float priority = (float)default_priority;
  enum nation_city_preference goodness;

  /* Increasing this value will increase the difference caused by
     (non-)matching terrain.  A matching terrain is mult_factor
     "better" than an unlisted terrain, which is mult_factor
     "better" than a non-matching terrain. */
  const float mult_factor = 1.4;
  bool river = FALSE;

  /*
   * If natural city names aren't being used, we just return the
   * base value.  This will have the effect of the first-listed
   * city being used.  We do this here rather than special-casing
   * it elewhere because this localizes everything to this
   * function, even though it's a bit inefficient.
   */
  if (!game.server.natural_city_names) {
    return default_priority;
  }

  /*
   * Assuming we're using the natural city naming system, we use
   * an internal alorithm to calculate the priority of each name.
   * It's a pretty fuzzy algorithm; we basically do the following:
   *   - Change the priority scale from 0..n to 10..n+10.  This means
   *     each successive city is 10% lower priority than the first.
   *   - Multiply by a semi-random number.  This has the effect of
   *     decreasing rounding errors in the successive calculations,
   *     as well as introducing a smallish random effect.
   *   - Check over all the applicable terrains, and
   *     multiply or divide the priority based on whether or not
   *     the terrain matches.  See comment below.
   */

  priority += 10.0;
  priority *= 10.0 + fc_rand(5);

  /*
   * The terrain priority in the struct nation_city will be either
   * -1, 0, or 1.  We therefore take this as-is if the terrain is
   * present, or negate it if not.
   *
   * The reason we multiply as well as divide the value is so
   * that cities that don't care what terrain they are on (which
   * is the default) will be left in the middle of the pack.  If
   * we _only_ multiplied (or divided), then cities that had more
   * terrain labels would have their priorities hurt (or helped).
   */
  goodness = nation_city_river_preference(pncity);
  extra_type_by_cause_iterate(EC_ROAD, priver) {
    if (tile_has_extra(ptile, priver)
        && road_has_flag(extra_road_get(priver), RF_RIVER)) {
      river = TRUE;
      break;
    }
  } extra_type_by_cause_iterate_end;
  if (!river) {
    goodness = nation_city_preference_revert(goodness);
  }

  switch (goodness) {
  case NCP_DISLIKE:
    priority *= mult_factor;
    break;
  case NCP_NONE:
    break;
  case NCP_LIKE:
    priority /= mult_factor;
    break;
  }

  terrain_type_iterate(pterrain) {
    /* Now we do the same for every available terrain. */
    goodness = nation_city_terrain_preference(pncity, pterrain);
    if (!is_terrain_near_tile(ptile, pterrain, TRUE)) {
      goodness = nation_city_preference_revert(goodness);
    }
    switch (goodness) {
    case NCP_DISLIKE:
      priority *= mult_factor;
      break;
    case NCP_NONE:
      break;
    case NCP_LIKE:
      priority /= mult_factor;
    }
  } terrain_type_iterate_end;

  return (int) priority;
}

/************************************************************************//**
  Checks if a city name belongs to default city names of a particular
  player.
****************************************************************************/
static bool is_default_city_name(const char *name, struct player *pplayer)
{
  nation_city_list_iterate(nation_cities(nation_of_player(pplayer)),
                           pncity) {
    if (0 == fc_strcasecmp(name, nation_city_name(pncity))) {
      return TRUE;
    }
  } nation_city_list_iterate_end;
  return FALSE;
}

/************************************************************************//**
  Searches through a city name list (a struct nation_city array) to pick
  the best available city name, and returns a pointer to it. The function
  checks if the city name is available and calls
  evaluate_city_name_priority() to determine the priority of the city name.
  If the list has no valid entries in it, NULL will be returned.
****************************************************************************/
static const char *search_for_city_name(struct tile *ptile,
                                        const struct nation_city_list *
                                        default_cities,
                                        struct player *pplayer)
{
  int choice = 0, priority, best_priority = -1;
  const char *name, *best_name = NULL;

  nation_city_list_iterate(default_cities, pncity) {
    name = nation_city_name(pncity);
    if (NULL == game_city_by_name(name)
        && is_allowed_city_name(pplayer, name, NULL, 0)) {
      priority = evaluate_city_name_priority(ptile, pncity, choice++);
      if (-1 == best_priority || priority < best_priority) {
        best_priority = priority;
        best_name = name;
      }
    }
  } nation_city_list_iterate_end;

  return best_name;
}

/************************************************************************//**
  Checks, if a city name is allowed for a player. If not, reports a
  reason for rejection. There's 4 different modes:
  0: no restrictions,
  1: a city name has to be unique to player
  2: a city name has to be globally unique
  3: a city name has to be globally unique, and players can't use names
     that are in another player's default city names. (E.g., Swedish may not
     call new cities or rename old cities as Helsinki, because it's in
     Finns' default city names.  Duplicated names may be used by
     either nation.)
****************************************************************************/
bool is_allowed_city_name(struct player *pplayer, const char *cityname,
			  char *error_buf, size_t bufsz)
{
  struct connection *pconn = conn_by_user(pplayer->username);

  /* Mode 1: A city name has to be unique for each player. */
  if (CNM_PLAYER_UNIQUE == game.server.allowed_city_names
      && city_list_find_name(pplayer->cities, cityname)) {
    if (error_buf) {
      fc_snprintf(error_buf, bufsz, _("You already have a city called %s."),
                  cityname);
    }
    return FALSE;
  }

  /* Modes 2,3: A city name has to be globally unique. */
  if ((CNM_GLOBAL_UNIQUE == game.server.allowed_city_names
       || CNM_NO_STEALING == game.server.allowed_city_names)
      && game_city_by_name(cityname)) {
    if (error_buf) {
      fc_snprintf(error_buf, bufsz,
                  _("A city called %s already exists."), cityname);
    }
    return FALSE;
  }

  /* General rule: any name in our ruleset is allowed. */
  if (is_default_city_name(cityname, pplayer)) {
    return TRUE;
  }

  /* 
   * Mode 3: Check that the proposed city name is not in another
   * player's default city names.  Note the name will already have been
   * allowed if it is in this player's default city names list.
   */
  if (CNM_NO_STEALING == game.server.allowed_city_names) {
    struct player *pother = NULL;

    players_iterate(player2) {
      if (player2 != pplayer && is_default_city_name(cityname, player2)) {
	pother = player2;
	break;
      }
    } players_iterate_end;

    if (pother != NULL) {
      if (error_buf) {
        fc_snprintf(error_buf, bufsz,
                    _("Can't use %s as a city name. It is reserved for %s."),
                    cityname, nation_plural_for_player(pother));
      }
      return FALSE;
    }
  }

  /* To prevent abuse, only players with HACK access (usually local
   * connections) can use non-ascii names.  Otherwise players could use
   * confusing garbage names in multi-player games.
   *
   * We can even reach here for an AI player, if all the cities of the
   * original nation are exhausted and the backup nations have non-ascii
   * names in them. */
  if (!is_ascii_name(cityname)
      && (!pconn || pconn->access_level != ALLOW_HACK)) {
    if (error_buf) {
      fc_snprintf(error_buf, bufsz,
                  _("%s is not a valid name. Only ASCII or "
                    "ruleset names are allowed for cities."),
                  cityname);
    }
    return FALSE;
  }


  return TRUE;
}

/************************************************************************//**
  Come up with a default name when a new city is about to be built. Handle
  running out of names etc. gracefully. Maybe we should keeptrack of which
  names have been rejected by the player, so that we do not suggest them
  again?
****************************************************************************/
const char *city_name_suggestion(struct player *pplayer, struct tile *ptile)
{
  struct nation_type *pnation = nation_of_player(pplayer);
  const char *name;

  log_verbose("Suggesting city name for %s at (%d,%d)",
              player_name(pplayer), TILE_XY(ptile));

  /* First try default city names. */
  name = search_for_city_name(ptile, nation_cities(pnation), pplayer);
  if (NULL != name) {
    log_debug("Default city name found: %s.", name);
    return name;
  }

  /* Not found... Let's try a straightforward algorithm to look through
   * nations to find a city name.
   *
   * We start by adding the player's nation to the queue. Then we proceed:
   * - Pick a random nation from the queue.
   * - If it has a valid city name, use that.
   * - Otherwise, add all parent and child nations to the queue.
   * - If the queue is empty, add all remaining nations to it and continue.
   *
   * Variables used:
   * - nation_list is a queue of nations to look through.
   * - nations_selected tells whether each nation is in the queue already
   * - queue_size gives the size of the queue (number of nations in it).
   * - i is the current position in the queue.
   * Note that nations aren't removed from the queue after they're processed.
   * New nations are just added onto the end. */
  {
    struct nation_type *nation_list[nation_count()];
    bool nations_selected[nation_count()];
    int queue_size = 1, i = 0, idx;

    memset(nations_selected, 0, sizeof(nations_selected));
    nation_list[0] = pnation;
    nations_selected[nation_index(pnation)] = TRUE;

    while (i < nation_count()) {
      for (; i < queue_size; i++) {

        if (0 < i) {
          /* Pick a random nation from the queue. */
          const int which = i + fc_rand(queue_size - i);
          struct nation_type *tmp = nation_list[i];

          nation_list[i] = nation_list[which];
          nation_list[which] = tmp;

          pnation = nation_list[i];
          log_debug("Looking through %s.", nation_rule_name(pnation));
          name = search_for_city_name(ptile, nation_cities(pnation), pplayer);

          if (NULL != name) {
            return name;
          }
        }

        /* Append the nation's civil war nations into the search tree. */
        nation_list_iterate(pnation->server.civilwar_nations, n) {
          idx = nation_index(n);
          if (!nations_selected[idx]) {
            nation_list[queue_size] = n;
            nations_selected[idx] = TRUE;
            queue_size++;
            log_debug("Child %s.", nation_rule_name(n));
          }
        } nation_list_iterate_end;

        /* Append the nation's parent nations into the search tree. */
        nation_list_iterate(pnation->server.parent_nations, n) {
          idx = nation_index(n);
          if (!nations_selected[idx]) {
            nation_list[queue_size] = n;
            nations_selected[idx] = TRUE;
            queue_size++;
            log_debug("Parent %s.", nation_rule_name(n));
          }
        } nation_list_iterate_end;
      }

      /* Still not found; append all remaining nations. */
      allowed_nations_iterate(n) {
        idx = nation_index(n);
        if (!nations_selected[idx]) {
          nation_list[queue_size] = n;
          nations_selected[nation_index(n)] = TRUE;
          queue_size++;
          log_debug("Misc nation %s.", nation_rule_name(n));
        }
      } allowed_nations_iterate_end;
    }
  }

  /* Not found in rulesets, make a default name. */
  {
    static char tempname[MAX_LEN_CITYNAME];
    int i;

    log_debug("City name not found in rulesets.");
    for (i = 1; i <= map_num_tiles(); i++ ) {
      fc_snprintf(tempname, MAX_LEN_CITYNAME, _("City no. %d"), i);
      if (NULL == game_city_by_name(tempname)) {
        return tempname;
      }
    }

    fc_assert_msg(FALSE, "Failed to generate a city name.");
    sz_strlcpy(tempname, _("A poorly-named city"));
    return tempname;
  }
}

/************************************************************************//**
  Calculate the remaining build points
****************************************************************************/
int build_points_left(struct city *pcity)
{
  int cost = impr_build_shield_cost(pcity, pcity->production.value.building);

  return cost - pcity->shield_stock;
}

/************************************************************************//**
  How many veteran levels will created unit of this type get?
****************************************************************************/
int do_make_unit_veteran(struct city *pcity,
                         const struct unit_type *punittype)
{
  int levels = get_unittype_bonus(city_owner(pcity), pcity->tile, punittype,
                                  EFT_VETERAN_BUILD);
  int max_levels = utype_veteran_levels(punittype) - 1;

  levels = CLIP(0, levels, max_levels);

  return levels;
}

/************************************************************************//**
  Change player that owns a unit and, if appropriate, its home city,
  with verbose output.
  If 'rehome' is not set, only change the player which owns the unit
  (the new owner is new_pcity's owner). Otherwise the new unit will be
  given a homecity, even if it was homeless before.
****************************************************************************/
static void transfer_unit(struct unit *punit, struct city *tocity,
                          bool rehome, bool verbose)
{
  struct player *from_player = unit_owner(punit);
  struct player *to_player = city_owner(tocity);

  /* Transferring a dying GameLoss unit as part of the loot for
   * killing it caused gna bug #23676. */
  fc_assert_ret_msg(!punit->server.dying,
                    "Tried to transfer the dying unit %d.",
                    punit->id);

  if (from_player == to_player) {
    fc_assert_ret(rehome);
    log_verbose("Changed homecity of %s %s to %s",
                nation_rule_name(nation_of_player(from_player)),
                unit_rule_name(punit),
                city_name_get(tocity));
    if (verbose) {
      notify_player(from_player, unit_tile(punit),
                    E_UNIT_RELOCATED, ftc_server,
		    _("Changed homecity of %s to %s."),
		    unit_link(punit),
		    city_link(tocity));
    }
  } else {
    struct tile *utile = unit_tile(punit);
    struct city *in_city = tile_city(utile);

    if (utype_player_already_has_this_unique(to_player,
                                             unit_type_get(punit))) {
      /* This is a unique unit that to_player already has. A transfer would
       * break the rule that a player only may have one unit of each unique
       * unit type. */

      log_debug("%s already have a %s. Can't transfer from %s",
                nation_rule_name(nation_of_player(to_player)),
                unit_rule_name(punit),
                nation_rule_name(nation_of_player(from_player)));

      if (utype_has_flag(unit_type_get(punit), UTYF_GAMELOSS)) {
        /* Try to save game loss units. */
        bounce_unit(punit, verbose);
      } else {
        /* Kill the unique unit. */

        if (verbose) {
          notify_player(from_player, unit_tile(punit),
                        E_UNIT_LOST_MISC, ftc_server,
                        /* TRANS: Americans ... Leader */
                        _("The %s already have a %s. Can't transfer yours."),
                        nation_plural_for_player(to_player),
                        unit_tile_link(punit));
        }

        wipe_unit(punit, ULR_CITY_LOST, NULL);
      }

      return;
    }

    if (in_city) {
      log_verbose("Transferred %s in %s from %s to %s",
                  unit_rule_name(punit), city_name_get(in_city),
                  nation_rule_name(nation_of_player(from_player)),
                  nation_rule_name(nation_of_player(to_player)));
      if (verbose) {
        notify_player(from_player, unit_tile(punit),
                      E_UNIT_RELOCATED, ftc_server,
		      _("Transferred %s in %s from %s to %s."),
		      unit_link(punit),
		      city_link(in_city),
		      nation_plural_for_player(from_player),
		      nation_plural_for_player(to_player));
      }
    } else if (can_unit_exist_at_tile(&(wld.map), punit, tocity->tile)) {
      log_verbose("Transferred %s from %s to %s",
                  unit_rule_name(punit),
                  nation_rule_name(nation_of_player(from_player)),
                  nation_rule_name(nation_of_player(to_player)));
      if (verbose) {
        notify_player(from_player, unit_tile(punit),
                      E_UNIT_RELOCATED, ftc_server,
                      _("Transferred %s from %s to %s."),
		      unit_link(punit),
		      nation_plural_for_player(from_player),
		      nation_plural_for_player(to_player));
      }
    } else {
      log_verbose("Could not transfer %s from %s to %s",
                  unit_rule_name(punit),
                  nation_rule_name(nation_of_player(from_player)),
                  nation_rule_name(nation_of_player(to_player)));
      if (verbose) {
        notify_player(from_player, unit_tile(punit),
                      E_UNIT_LOST_MISC, ftc_server,
                      /* TRANS: Polish Destroyer ... German <city> */
                      _("%s %s lost in transfer to %s %s"),
                      nation_adjective_for_player(from_player),
                      unit_tile_link(punit),
                      nation_adjective_for_player(to_player),
                      city_link(tocity));
      }
      wipe_unit(punit, ULR_CITY_LOST, NULL);
      return;
    }

    maybe_make_contact(utile, to_player);
  }
  unit_change_homecity_handling(punit, tocity, rehome);
}

/************************************************************************//**
  When a city is transferred (bought, incited, disbanded, civil war):
  Units in a transferred city are transferred to the new owner; units
  supported by the city, but held in other cities are updated to
  reflect those cities as their new homecity.  Units supported
  by the transferred city that are not in a city tile may be deleted.

  - Kris Bubendorfer <Kris.Bubendorfer@MCS.VUW.AC.NZ>

  pplayer: The player receiving the units if they are not disbanded and
           are not in a city
  pvictim: The owner of the city the units are transferred from.
  units:   A list of units to be transferred, typically a city's unit list.
  pcity:   Default city the units are transferred to.
  exclude_city: The units cannot be transferred to this city.
  kill_outside: Units outside this range are deleted. -1 means no units
                are deleted.
  verbose: Messages are sent to the involved parties.
****************************************************************************/
void transfer_city_units(struct player *pplayer, struct player *pvictim,
                         struct unit_list *units, struct city *pcity,
                         struct city *exclude_city,
                         int kill_outside, bool verbose)
{
  struct tile *ptile = pcity->tile;
  int saved_id = pcity->id;
  const char *name = city_name_get(pcity);

  /* Transfer enemy units in the city to the new owner.
   * Only relevant if we are transferring to another player. */
  if (pplayer != pvictim) {
    unit_list_iterate_safe((ptile)->units, vunit)  {
      if (vunit->server.dying) {
        /* Don't transfer or bounce a dying unit. It will soon be gone
         * anyway.
         *
         * Bouncing a dying unit isn't a good idea.
         * Remaining death handling may do things meant for its current
         * location to the new location. (Example: Stack death)
         * bounce_unit() will wipe the unit if it can't be bounced. Wiping
         * the dying unit isn't a good idea. The remaining death handling
         * code will try to read from it.
         *
         * Transferring a dying GameLoss unit as part of the loot for
         * killing it caused gna bug #23676. */
        continue;
      }

      /* Don't transfer units already owned by new city-owner --wegge */
      if (unit_owner(vunit) == pvictim) {
        /* Determine whether unit was homeless. If it was, we don't give
         * it a homecity, only change ownership.
         * We have to search the transferred city's former units because
         * the unit may have been made only temporarily homeless during
         * city transfer. */
        bool homeless = (vunit->homecity == 0)
          && !unit_list_search(units, vunit);

        /* vunit may die during transfer_unit().
         * unit_list_remove() is still safe using vunit pointer, as
         * pointer is not used for dereferencing, only as value.
         * Not sure if it would be safe to unlink first and transfer only
         * after that. Not sure if it is correct to unlink at all in
         * some cases, depending which list 'units' points to. */
	transfer_unit(vunit, pcity, !homeless, verbose);
	unit_list_remove(units, vunit);
      } else if (!pplayers_allied(pplayer, unit_owner(vunit))) {
        /* the owner of vunit is allied to pvictim but not to pplayer */
        bounce_unit(vunit, verbose);
      }
    } unit_list_iterate_safe_end;
  }

  if (!city_exist(saved_id)) {
    saved_id = 0;
  }

  /* Any remaining units supported by the city are either given new home
     cities or maybe destroyed */
  unit_list_iterate_safe(units, vunit) {
    struct city *new_home_city = tile_city(unit_tile(vunit));

    if (vunit->server.dying) {
      /* Don't transfer or destroy a dying unit. It will soon be gone
       * anyway.
       *
       * Transfering a dying GameLoss unit as part of the loot for
       * killing it caused gna bug #23676. */
      continue;
    }

    if (new_home_city && new_home_city != exclude_city
	&& city_owner(new_home_city) == unit_owner(vunit)) {
      /* unit is in another city: make that the new homecity,
       * unless that city is actually the same city (happens if disbanding)
       * Unit had a homecity since it was supported, so rehome. */
      transfer_unit(vunit, new_home_city, TRUE, verbose);
    } else if ((kill_outside == -1
                || real_map_distance(unit_tile(vunit), ptile) <= kill_outside)
               && saved_id) {
      /* else transfer to specified city. */
      transfer_unit(vunit, pcity, TRUE, verbose);
      if (unit_tile(vunit) == ptile && !pplayers_allied(pplayer, pvictim)) {
        /* Unit is inside city being transferred, bounce it */
        bounce_unit(vunit, TRUE);
      }
    } else {
      /* The unit is lost.  Call notify_player (in all other cases it is
       * called automatically). */
      log_verbose("Lost %s %s at (%d,%d) when %s was lost.",
                  nation_rule_name(nation_of_unit(vunit)),
                  unit_rule_name(vunit), TILE_XY(unit_tile(vunit)), name);
      if (verbose) {
        notify_player(unit_owner(vunit), unit_tile(vunit),
                      E_UNIT_LOST_MISC, ftc_server,
                      _("%s lost along with control of %s."),
                      unit_tile_link(vunit), name);
      }
      wipe_unit(vunit, ULR_CITY_LOST, NULL);
    }
  } unit_list_iterate_safe_end;

#ifdef FREECIV_DEBUG
  unit_list_iterate(pcity->units_supported, punit) {
    if (punit->server.dying) {
      /* Leave the dying alone. */
      continue;
    }

    fc_assert(punit->homecity == pcity->id);
    fc_assert(unit_owner(punit) == pplayer);
  } unit_list_iterate_end;
#endif /* FREECIV_DEBUG */
}

/************************************************************************//**
  Find the city closest to 'ptile'. Some restrictions can be applied:

  'pexclcity'       not this city
  'pplayer'         player to be used by 'only_known', 'only_player' and
                    'only_enemy'.
  'only_ocean'      if set the city must be adjacent to ocean.
  'only_continent'  if set only cities on the same continent as 'ptile' are
                    valid.
  'only_known'      if set only cities known to 'pplayer' are considered.
  'only_player'     if set and 'pplayer' is not NULL only cities of this
                    player are returned.
  'only_enemy'      if set and 'pplayer' is not NULL only cities of players
                    which are at war with 'pplayer' are returned.
  'pclass'          if set, and 'pclass' is not NULL only cities that have
                    adjacent native terrain for that unit class are returned.

  If no city is found NULL is returned.
****************************************************************************/
struct city *find_closest_city(const struct tile *ptile,
                               const struct city *pexclcity,
                               const struct player *pplayer,
                               bool only_ocean, bool only_continent,
                               bool only_known, bool only_player,
                               bool only_enemy, const struct unit_class *pclass)
{
  Continent_id con;
  struct city *best_city = NULL;
  int best_dist = -1;

  fc_assert_ret_val(ptile != NULL, NULL);

  if (pplayer != NULL && only_player && only_enemy) {
    log_error("Non of my own cities will be at war with me!");
    return NULL;
  }

  con = tile_continent(ptile);

  players_iterate(aplayer) {
    if (pplayer != NULL && only_player && pplayer != aplayer) {
      /* only cities of player 'pplayer' */
      continue;
    }

    if (pplayer != NULL && only_enemy
        && !pplayers_at_war(pplayer, aplayer)) {
      /* only cities of players at war with player 'pplayer' */
      continue;
    }

    city_list_iterate(aplayer->cities, pcity) {
      int city_dist;

      if (pexclcity && pexclcity == pcity) {
        /* not this city */
        continue;
      }

      city_dist = real_map_distance(ptile, city_tile(pcity));

      /* Find the closest city matching the requirements.
       * - closer than the current best city
       * - (if required) on the same continent
       * - (if required) adjacent to ocean
       * - (if required) only cities known by the player
       * - (if required) only cities native to the class */
      if ((best_dist == -1 || city_dist < best_dist)
          && (!only_continent || con == tile_continent(pcity->tile))
          && (!only_ocean || is_terrain_class_near_tile(city_tile(pcity), TC_OCEAN))
          && (!only_known
              || (map_is_known(city_tile(pcity), pplayer)
                  && map_get_player_site(city_tile(pcity), pplayer)->identity
                     > IDENTITY_NUMBER_ZERO))
          && (pclass == NULL
              || is_native_near_tile(&(wld.map), pclass, city_tile(pcity)))) {
        best_dist = city_dist;
        best_city = pcity;
      }
    } city_list_iterate_end;
  } players_iterate_end;

  return best_city;
}

/************************************************************************//**
  Called when a player conquers a city, remove buildings (not wonders and
  always palace) with game.server.razechance% chance, barbarians destroy more
  set the city's shield stock to 0
****************************************************************************/
static void raze_city(struct city *pcity)
{
  int razechance = game.server.razechance;
  bool city_remains = TRUE;

  /* land barbarians are more likely to destroy city improvements */
  if (is_land_barbarian(city_owner(pcity))) {
    razechance += 30;
  }

  city_built_iterate(pcity, pimprove) {
    /* Small wonders should have already been removed by
     * transfer_city() (with 100% probability). */
    fc_assert(!is_small_wonder(pimprove));
    if (is_improvement(pimprove) && (fc_rand(100) < razechance)) {
      /* FIXME: Should maybe have conquering unit instead of NULL as destoryer */
      city_remains = building_removed(pcity, pimprove, "razed", NULL);
      if (!city_remains) {
        break;
      }
    }
  } city_built_iterate_end;

  if (city_remains) {
    nullify_prechange_production(pcity);
    pcity->shield_stock = 0;
  }
}

/************************************************************************//**
  The following has to be called every time AFTER a city (pcity) has changed
  owner to update the city's trade routes.
****************************************************************************/
static void reestablish_city_trade_routes(struct city *pcity) 
{
  trade_routes_iterate_safe(pcity, proute) {
    bool keep_route;
    struct trade_route *back;
    struct city *partner = game_city_by_number(proute->partner);

    /* Remove the city's trade routes (old owner).
     * Do not announce removal as we might restore the route immediately below */
    back = remove_trade_route(pcity, proute, FALSE, FALSE);

    keep_route = can_cities_trade(pcity, partner)
      && can_establish_trade_route(pcity, partner);

    if (!keep_route) {
      enum trade_route_type type = cities_trade_route_type(pcity, partner);
      struct trade_route_settings *settings = trade_route_settings_by_type(type);

      if (settings->cancelling != TRI_CANCEL) {
        keep_route = TRUE;
      }
    }

    /* Readd the city's trade route (new owner) */
    if (keep_route) {
      trade_route_list_append(pcity->routes, proute);
      trade_route_list_append(partner->routes, back);
    } else {
      free(proute);
      free(back);

      /* Now announce the traderoute removal */
      announce_trade_route_removal(pcity, partner, FALSE);
    }

    /* refresh regardless; either it lost a trade route or the trade
     * route revenue changed. */
    city_refresh(partner);
    send_city_info(city_owner(partner), partner);

    /* Give the new owner infos about the city which has a trade route
     * with the transferred city. */
    reality_check_city(city_owner(pcity), partner->tile);
    update_dumb_city(city_owner(pcity), partner);
    send_city_info(city_owner(pcity), partner);
  } trade_routes_iterate_safe_end;
}

/************************************************************************//**
  Create saved small wonders in random cities. Usually used to save the
  palace when the capital was conquered. Respects the 'savepalace'
  server setting.
****************************************************************************/
static void build_free_small_wonders(struct player *pplayer,
                                     bv_imprs *had_small_wonders)
{
  int size = city_list_size(pplayer->cities);

  if (!game.server.savepalace) {
    /* Nothing to do */
    return;
  }

  if (size == 0) {
    /* The last city was removed or transferred to the enemy.
     * If the victim survives to found or acquire another city, they'll
     * get any savepalace initial buildings then. */
    return;
  }

  improvement_iterate(pimprove) {
    if (improvement_has_flag(pimprove, IF_SAVE_SMALL_WONDER)
        && BV_ISSET(*had_small_wonders, improvement_index(pimprove))) {
      /* FIXME: instead, find central city */
      struct city *pnew_city = city_list_get(pplayer->cities, fc_rand(size));

      fc_assert_action(NULL == city_from_small_wonder(pplayer, pimprove),
                       continue);

      city_add_improvement(pnew_city, pimprove);

      /*
       * send_player_cities will recalculate all cities and send them to
       * the client.
       */
      send_player_cities(pplayer);

      notify_player(pplayer, city_tile(pnew_city), E_IMP_BUILD, ftc_server,
		    /* FIXME: should already be notified about city loss? */
		    /* TRANS: <building> ... <city> */
		    _("A replacement %s was built in %s."),
		    improvement_name_translation(pimprove),
		    city_link(pnew_city));
      /* 
       * The enemy want to see the new capital in his intelligence
       * report. 
       */
      send_city_info(NULL, pnew_city);
    }
  } improvement_iterate_end;
}

/************************************************************************//**
  Handles all transactions in relation to transferring a city.

  The kill_outside and transfer_unit_verbose arguments are passed to
  transfer_city_units(), which is called in the middle of the function.

  Return TRUE iff the city remains after transfering (the city may be
  destroyed by a script, notably with bouncing or wiping units).
****************************************************************************/
bool transfer_city(struct player *ptaker, struct city *pcity,
                   int kill_outside, bool transfer_unit_verbose,
                   bool resolve_stack, bool raze, bool build_free)
{
  char old_city_name[MAX_LEN_CITYNAME];
  bv_imprs had_small_wonders;
  struct vision *old_vision, *new_vision;
  struct unit_list *old_city_units = unit_list_new();
  struct player *pgiver = city_owner(pcity);
  struct tile *pcenter = city_tile(pcity);
  int saved_id = pcity->id;
  bool city_remains = TRUE;
  bool had_great_wonders = FALSE;
  const citizens old_taker_content_citizens = player_content_citizens(ptaker);
  const citizens old_giver_content_citizens = player_content_citizens(pgiver);
  const citizens old_taker_angry_citizens = player_angry_citizens(ptaker);
  const citizens old_giver_angry_citizens = player_angry_citizens(pgiver);
  bool taker_had_no_cities = (city_list_size(ptaker->cities) == 0);
  bool new_extras;
  const int units_num = unit_list_size(pcenter->units);
  bv_player *could_see_unit = (units_num > 0
                               ? fc_malloc(sizeof(*could_see_unit)
                                           * units_num)
                               : NULL);
  int i;

  fc_assert_ret_val(pgiver != ptaker, TRUE);

  /* Remember what player see what unit. */
  i = 0;
  unit_list_iterate(pcenter->units, aunit) {
    BV_CLR_ALL(could_see_unit[i]);
    players_iterate(aplayer) {
      if (can_player_see_unit(aplayer, aunit)) {
        BV_SET(could_see_unit[i], player_index(aplayer));
      }
    } players_iterate_end;
    i++;
  } unit_list_iterate_end;
  fc_assert(i == units_num);

  /* Remove AI control of the old owner. */
  CALL_PLR_AI_FUNC(city_lost, pcity->owner, pcity->owner, pcity);

  /* Forget old tasks */
  clear_worker_tasks(pcity);

  /* Activate AI control of the new owner. */
  CALL_PLR_AI_FUNC(city_got, ptaker, ptaker, pcity);

  city_freeze_workers(pcity);

  unit_list_iterate(pcity->units_supported, punit) {
    unit_list_prepend(old_city_units, punit);
    /* Mark unit to have no homecity at all.
     * 1. We remove unit from units_supported list here,
     *    real_change_unit_homecity() should not attempt it.
     * 2. Otherwise we might delete the homecity from under the unit
     *    in the client */
    punit->homecity = 0;
    send_unit_info(NULL, punit);
  } unit_list_iterate_end;
  unit_list_clear(pcity->units_supported);

  /* Remove all global improvement effects that this city confers (but
     then restore the local improvement list - we need this to restore the
     global effects for the new city owner) */
  BV_CLR_ALL(had_small_wonders);
  city_built_iterate(pcity, pimprove) {
    if (is_small_wonder(pimprove)) {
      /* Small wonders are really removed (not restored later). */
      building_removed(pcity, pimprove, "conquered", NULL);
      BV_SET(had_small_wonders, improvement_index(pimprove));
    } else {
      city_remove_improvement(pcity, pimprove);
      if (is_great_wonder(pimprove)) {
        had_great_wonders = TRUE;
      }
      /* note: internal turn here, next city_built_iterate(). */
      pcity->built[improvement_index(pimprove)].turn = game.info.turn; /*I_ACTIVE*/
    }
  } city_built_iterate_end;

  give_citymap_from_player_to_player(pcity, pgiver, ptaker);
  old_vision = pcity->server.vision;
  new_vision = vision_new(ptaker, pcenter);
  pcity->server.vision = new_vision;
  vision_reveal_tiles(new_vision, game.server.vision_reveal_tiles);
  vision_change_sight(new_vision, old_vision->radius_sq);

  ASSERT_VISION(new_vision);

  sz_strlcpy(old_city_name, city_name_get(pcity));
  if (CNM_PLAYER_UNIQUE == game.server.allowed_city_names
      && city_list_find_name(ptaker->cities, city_name_get(pcity))) {
    sz_strlcpy(pcity->name,
	       city_name_suggestion(ptaker, pcenter));
    notify_player(ptaker, pcenter, E_BAD_COMMAND, ftc_server,
		  _("You already had a city called %s."
		    " The city was renamed to %s."),
		  old_city_name,
		  city_link(pcity));
  }

  /* Has to follow the unfog call above. */
  city_list_remove(pgiver->cities, pcity);
  map_clear_border(pcenter);
  /* city_thaw_workers_queue() later */

  pcity->owner = ptaker;
  map_claim_ownership(pcenter, ptaker, pcenter, TRUE);
  city_list_prepend(ptaker->cities, pcity);

  /* Hide/reveal units. Do it after vision have been given to taker, city
   * owner has been changed, and before any script could be spawned. */
  i = 0;
  unit_list_iterate(pcenter->units, aunit) {
    players_iterate(aplayer) {
      if (can_player_see_unit(aplayer, aunit)) {
        if (!BV_ISSET(could_see_unit[i], player_index(aplayer))
            && !aunit->server.dying) {
          /* Reveal 'aunit'. */
          send_unit_info(aplayer->connections, aunit);
        }
      } else {
        if (BV_ISSET(could_see_unit[i], player_index(aplayer))) {
          /* Hide 'aunit'. */
          unit_goes_out_of_sight(aplayer, aunit);
        }
      }
    } players_iterate_end;
    i++;
  } unit_list_iterate_end;
  fc_assert(i == units_num);
  free(could_see_unit);
  could_see_unit = NULL;

  transfer_city_units(ptaker, pgiver, old_city_units,
		      pcity, NULL,
		      kill_outside, transfer_unit_verbose);
  /* The units themselves are allready freed by transfer_city_units. */
  unit_list_destroy(old_city_units);

  if (resolve_stack) {
    resolve_unit_stacks(pgiver, ptaker, transfer_unit_verbose);
  }

  if (! city_exist(saved_id)) {
    city_remains = FALSE;
  }

  if (city_remains) {
    /* Update the city's trade routes. */
    reestablish_city_trade_routes(pcity);

    city_refresh(pcity);
  }

  /* 
   * maybe_make_contact() MUST be called before city_map_update_all(),
   * since the diplomacy status can influence whether a tile is available.
   */
  maybe_make_contact(pcenter, ptaker);

  if (city_remains) {
    struct extra_type *upgradet;

    if (raze) {
      raze_city(pcity);
    }

    if (taker_had_no_cities) {
      /* If conqueror previously had no cities, we might need to give
       * them a palace etc */
      if (build_free) {
        city_build_free_buildings(pcity);
      } /* else caller should probably ensure palace is built */
      ptaker->server.got_first_city = TRUE;
    }

    citizens_convert_conquest(pcity);

    /* Restore any global improvement effects that this city confers */
    city_built_iterate(pcity, pimprove) {
      city_add_improvement(pcity, pimprove);
    } city_built_iterate_end;

    /* Set production to something valid for pplayer, if not.
     * (previously allowed building obsolete units.) */
    if (!can_city_build_now(pcity, &pcity->production)) {
      advisor_choose_build(ptaker, pcity);
    }

    /* What wasn't obsolete for the old owner may be so now. */
    remove_obsolete_buildings_city(pcity, TRUE);

    new_extras = upgrade_city_extras(pcity, &upgradet);

    if (new_extras) {
      const char *clink = city_link(pcity);

      notify_player(ptaker, pcenter, E_CITY_TRANSFER, ftc_server,
                    _("The people in %s are stunned by your "
                      "technological insight!"),
                    clink);

      if (upgradet != NULL) {
        notify_player(ptaker, pcenter, E_CITY_TRANSFER, ftc_server,
                      _("Workers spontaneously gather and upgrade "
                        "%s with %s."),
                      clink, extra_name_translation(upgradet));
      } else {
        notify_player(ptaker, pcenter, E_CITY_TRANSFER, ftc_server,
                      _("Workers spontaneously gather and upgrade "
                        "%s infrastructure."),
                      clink);
      }
      update_tile_knowledge(pcenter);
    }

    /* Build a new palace for free if the player lost her capital and
       savepalace is on. */
    build_free_small_wonders(pgiver, &had_small_wonders);

    /* Refresh the city's vision range, since it might be different 
     * under the new owner. */
    city_refresh_vision(pcity);

    /* Update the national borders, within the current vision and culture.
     * This could leave a border ring around the city, updated later by
     * map_calculate_borders() at the next turn.
     */
    map_claim_border(pcenter, ptaker, -1);
    /* city_thaw_workers_queue() later */

    auto_arrange_workers(pcity); /* does city_map_update_all() */
    city_thaw_workers(pcity);
    city_thaw_workers_queue();  /* after old city has a chance to work! */
    city_refresh_queue_add(pcity);
    /* no sanity check here as the city is not refreshed! */
  }

  if (city_remains) {
    /* Send city with updated owner information to giver and to everyone
     * having shared vision pact with him/her before (s)he may
     * lose vision to it. When we later send info to everybody seeing the city,
     * (s)he may not be included. */
    send_city_info(NULL, pcity);
  }

  /* Remove the sight points from the giver. */
  vision_clear_sight(old_vision);
  vision_free(old_vision);

  /* Send wonder infos. */
  if (had_great_wonders) {
    send_game_info(NULL);
    if (city_remains) {
      send_player_info_c(ptaker, NULL);

      /* Refresh all cities of the taker to account for possible changes due
       * to player wide effects. */
      city_list_iterate(ptaker->cities, acity) {
        city_refresh_queue_add(acity);
      } city_list_iterate_end;
    } else {
      /* Refresh all cities to account for possible global effects. */
      cities_iterate(acity) {
        city_refresh_queue_add(acity);
      } cities_iterate_end;
    }
  }
  if (BV_ISSET_ANY(had_small_wonders) || had_great_wonders) {
    /* No need to send to detached connections. */
    send_player_info_c(pgiver, NULL);

    /* Refresh all cities of the giver to account for possible changes due
     * to player wide effects. */
    city_list_iterate(pgiver->cities, acity) {
      city_refresh_queue_add(acity);
    } city_list_iterate_end;
  }

  /* Refresh all cities in the queue. */
  city_refresh_queue_processing();
  /* After the refresh the sanity check can be done. */
  sanity_check_city(pcity);

  if (city_remains) {
    /* Send information about conquered city to all players. */
    send_city_info(NULL, pcity);
  }

  /* We may cross the EFT_EMPIRE_SIZE_* effects, then we will have to
   * refresh all cities for the player. */
  if (old_taker_content_citizens != player_content_citizens(ptaker)
      || old_taker_angry_citizens != player_angry_citizens(ptaker)) {
    city_refresh_for_player(ptaker);
  }
  if (old_giver_content_citizens != player_content_citizens(pgiver)
      || old_giver_angry_citizens != player_angry_citizens(pgiver)) {
    city_refresh_for_player(pgiver);
  }

  sync_cities();

  return city_remains;
}

/************************************************************************//**
  Give to a new city the free (initial) buildings.
  Call this when a player has just acquired a city (or batch of cities,
  e.g. civil war) after having no cities.
  Doesn't check for building uniqueness! -- assumes player has no other
  cities which might contain unique buildings.
****************************************************************************/
void city_build_free_buildings(struct city *pcity)
{
  struct player *pplayer;
  struct nation_type *nation;
  int i;
  bool has_small_wonders, has_great_wonders;
  bool first_city;

  fc_assert_ret(NULL != pcity);
  pplayer = city_owner(pcity);
  fc_assert_ret(NULL != pplayer);
  nation = nation_of_player(pplayer);
  fc_assert_ret(NULL != nation);

  /* If this isn't the first city a player has ever had, they only get
   * any initial buildings with the SaveSmallWonder flag, and then only
   * if savepalace is enabled. */
  first_city = !pplayer->server.got_first_city;

  has_small_wonders = FALSE;
  has_great_wonders = FALSE;

  /* Global free buildings. */
  for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
    Impr_type_id n = game.rgame.global_init_buildings[i];
    struct impr_type *pimprove;

    if (n == B_LAST) {
      break;
    }

    pimprove = improvement_by_number(n);
    fc_assert_action(!is_great_wonder(pimprove), continue);
    if (first_city ||
        (game.server.savepalace
         && improvement_has_flag(pimprove, IF_SAVE_SMALL_WONDER))) {
      city_add_improvement(pcity, pimprove);
      if (is_small_wonder(pimprove)) {
        has_small_wonders = TRUE;
      }
    }
  }

  /* Nation specific free buildings. */
  for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
    Impr_type_id n = nation->init_buildings[i];
    struct impr_type *pimprove;

    if (n == B_LAST) {
      break;
    }

    pimprove = improvement_by_number(n);
    if (first_city ||
        (game.server.savepalace
         && improvement_has_flag(pimprove, IF_SAVE_SMALL_WONDER))) {
      city_add_improvement(pcity, pimprove);
      if (is_small_wonder(pimprove)) {
        has_small_wonders = TRUE;
      } else if (is_great_wonder(pimprove)) {
        has_great_wonders = TRUE;
      }
    }
  }

  /* Update wonder infos. */
  if (has_great_wonders) {
    send_game_info(NULL);
    /* No need to send to detached connections. */
    send_player_info_c(pplayer, NULL);
  } else if (has_small_wonders) {
    /* No need to send to detached connections. */
    send_player_info_c(pplayer, NULL);
  }
}

/************************************************************************//**
  Creates real city.
****************************************************************************/
void create_city(struct player *pplayer, struct tile *ptile,
                 const char *name, struct player *nationality)
{
  struct player *saved_owner = tile_owner(ptile);
  struct tile *saved_claimer = tile_claimer(ptile);
  struct city *pwork = tile_worked(ptile);
  struct city *pcity;
  const citizens old_content_citizens = player_content_citizens(pplayer);
  const citizens old_angry_citizens = player_angry_citizens(pplayer);

  log_debug("create_city() %s", name);

  pcity = create_city_virtual(pplayer, ptile, name);

  /* Remove units no more seen. Do it before city is really put into the
   * game. */
  players_iterate(other_player) {
    if (can_player_see_units_in_city(other_player, pcity)
        || !map_is_known_and_seen(ptile, other_player, V_MAIN)) {
      continue;
    }
    unit_list_iterate(ptile->units, punit) {
      if (can_player_see_unit(other_player, punit)) {
        unit_goes_out_of_sight(other_player, punit);
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  adv_city_alloc(pcity);

  tile_set_owner(ptile, pplayer, ptile); /* temporarily */
  city_choose_build_default(pcity);
  pcity->id = identity_number();

  fc_allocate_mutex(&game.server.mutexes.city_list);
  idex_register_city(&wld, pcity);
  fc_release_mutex(&game.server.mutexes.city_list);

  if (city_list_size(pplayer->cities) == 0) {
    /* Free initial buildings, or at least a palace if they were
     * previously careless enough to lose all their cities */
    city_build_free_buildings(pcity);
    pplayer->server.got_first_city = TRUE;
  }

  /* Set up citizens nationality. */
  citizens_init(pcity);

  /* Place a worker at the is_city_center() is_free_worked().
   * It is possible to build a city on a tile that is already worked;
   * this will displace the worker on the newly-built city's tile -- Syela */
  tile_set_worked(ptile, pcity); /* instead of city_map_update_worker() */

  if (NULL != pwork) {
    /* was previously worked by another city */
    /* Turn citizen into specialist. */
    pwork->specialists[DEFAULT_SPECIALIST]++;
    /* One less citizen. Citizen sanity will be handled later in
     * city_thaw_workers_queue() */
    pwork->server.synced = FALSE;
    city_freeze_workers_queue(pwork);
  }

  /* Update citizens. */
  citizens_update(pcity, nationality);

  /* Restore the old-owner information so removal
   * of territory claiming bases can work relative to it. */
  tile_set_owner(ptile, saved_owner, saved_claimer);

  /* Destroy any extras that don't belong in the city. */
  extra_type_iterate(pextra) {
    if (tile_has_extra(ptile, pextra)
        && !is_native_tile_to_extra(pextra, ptile)) {
      destroy_extra(ptile, pextra);
    }
  } extra_type_iterate_end;

  /* Build any extras that the city should have. */
  upgrade_city_extras(pcity, NULL);

  /* Claim the ground we stand on */
  map_claim_ownership(ptile, pplayer, ptile, TRUE);

  /* Before arranging workers to show unknown land */
  pcity->server.vision = vision_new(pplayer, ptile);
  vision_reveal_tiles(pcity->server.vision, game.server.vision_reveal_tiles);
  city_refresh_vision(pcity);
  city_list_prepend(pplayer->cities, pcity);

  /* This is dependent on the current vision, so must be done after
   * vision is prepared and before arranging workers. */
  map_claim_border(ptile, pplayer, -1);
  /* city_thaw_workers_queue() later */

  /* Refresh the city.  First a city refresh is done (this shouldn't
   * send any packets to the client because the city has no supported units)
   * then rearrange the workers.  Note that auto_arrange_workers does its
   * own refresh call; it is safest to do our own controlled city_refresh
   * first. */
  city_refresh(pcity);
  auto_arrange_workers(pcity);
  city_thaw_workers_queue(); /* after new city has a chance to work! */
  city_refresh_queue_processing();

  /* Bases destroyed earlier may have had watchtower effect. Refresh
   * unit vision. */
  unit_list_refresh_vision(ptile->units);

  update_tile_knowledge(ptile);

  if (old_content_citizens != player_content_citizens(pplayer)
      || old_angry_citizens != player_angry_citizens(pplayer)) {
    /* We crossed the EFT_EMPIRE_SIZE_* effects, we have to refresh all
     * cities for the player. */
    city_refresh_for_player(pplayer);
  }

  pcity->server.synced = FALSE;
  send_city_info(NULL, pcity);
  sync_cities(); /* Will also send pwork. */

  notify_player(pplayer, ptile, E_CITY_BUILD, ftc_server,
		_("You have founded %s."),
		city_link(pcity));
  maybe_make_contact(ptile, city_owner(pcity));

  unit_list_iterate((ptile)->units, punit) {
    struct city *home = game_city_by_number(punit->homecity);

    /* Catch fortress building, transforming into ocean, etc. */
    if (!can_unit_continue_current_activity(punit)) {
      unit_activity_handling(punit, ACTIVITY_IDLE);
    }

    /* Update happiness (the unit may no longer cause unrest). */
    if (home) {
      if (city_refresh(home)) {
        /* Shouldn't happen, but better be safe than sorry. */
        auto_arrange_workers(home);
      }
      sanity_check_city(home);
      send_city_info(city_owner(home), home);
    }
  } unit_list_iterate_end;

  sanity_check_city(pcity);

  script_server_signal_emit("city_built", pcity);

  CALL_FUNC_EACH_AI(city_created, pcity);
  CALL_PLR_AI_FUNC(city_got, pplayer, pplayer, pcity);
}

/************************************************************************//**
  Remove a city from the game.
****************************************************************************/
void remove_city(struct city *pcity)
{
  struct player *powner = city_owner(pcity);
  struct tile *pcenter = city_tile(pcity);
  bv_imprs had_small_wonders;
  struct vision *old_vision;
  int id = pcity->id; /* We need this even after memory has been freed */
  bool had_great_wonders = FALSE;
  const citizens old_content_citizens = player_content_citizens(powner);
  const citizens old_angry_citizens = player_angry_citizens(powner);
  struct dbv tile_processed;
  struct tile_list *process_queue;
  const char *ctl = city_tile_link(pcity);

  CALL_PLR_AI_FUNC(city_lost, powner, powner, pcity);
  CALL_FUNC_EACH_AI(city_destroyed, pcity);

  BV_CLR_ALL(had_small_wonders);
  city_built_iterate(pcity, pimprove) {
    building_removed(pcity, pimprove, "city_destroyed", NULL);

    if (is_small_wonder(pimprove)) {
      BV_SET(had_small_wonders, improvement_index(pimprove));
    } else if (is_great_wonder(pimprove)) {
      had_great_wonders = TRUE;
    }
  } city_built_iterate_end;

  /* Rehome units in other cities */
  unit_list_iterate_safe(pcity->units_supported, punit) {
    struct city *new_home_city = tile_city(unit_tile(punit));

    if (new_home_city
	&& new_home_city != pcity
        && city_owner(new_home_city) == powner
        && !punit->server.dying) {
      transfer_unit(punit, new_home_city, TRUE, TRUE);
    }
  } unit_list_iterate_safe_end;

  /* make sure ships are not left on land when city is removed. */
  unit_list_iterate_safe(pcenter->units, punit) {
    bool moved;
    struct unit_type *punittype = unit_type_get(punit);

    if (is_native_tile(punittype, pcenter)) {
      continue;
    }

    unit_activity_handling(punit, ACTIVITY_IDLE);
    moved = FALSE;
    adjc_iterate(&(wld.map), pcenter, tile1) {
      if (!moved && is_native_tile(punittype, tile1)) {
        if (adv_could_unit_move_to_tile(punit, tile1) == 1) {
          /* Move */
          moved = unit_move_handling(punit, tile1, FALSE, TRUE, NULL);
          if (moved) {
            notify_player(unit_owner(punit), tile1,
                          E_UNIT_RELOCATED, ftc_server,
                          _("Moved %s out of disbanded city %s "
                            "since it cannot stay on %s."),
                          unit_link(punit), ctl,
                          terrain_name_translation(tile_terrain(pcenter)));
            break;
          }
        }
      }
    } adjc_iterate_end;
    if (!moved) {
      notify_player(unit_owner(punit), unit_tile(punit),
                    E_UNIT_LOST_MISC, ftc_server,
                    _("When %s was disbanded your %s could not "
                      "get out, and it was therefore lost."),
                    ctl,
                    unit_tile_link(punit));
      wipe_unit(punit, ULR_CITY_LOST, NULL);
    }
  } unit_list_iterate_safe_end;

  process_queue = tile_list_new();
  dbv_init(&tile_processed, map_num_tiles());
  for (tile_list_append(process_queue, pcenter); tile_list_size(process_queue) > 0;) {
    struct tile *ptile = tile_list_front(process_queue);
    tile_list_pop_front(process_queue);
    dbv_set(&tile_processed, tile_index(ptile));
    adjc_iterate(&(wld.map), ptile, piter) {
      struct city *other_city;

      if (dbv_isset(&tile_processed, tile_index(piter))) {
        continue;
      }
      other_city = tile_city(piter);
      if (other_city != NULL) {
        /* Adjacent tile has a city that may have been part of same channel */
        dbv_set(&tile_processed, tile_index(piter));
        tile_list_append(process_queue, piter);
        unit_list_iterate_safe(piter->units, punit) {
          struct unit_class *pclass = utype_class(punit->utype);

          if (!uclass_has_flag(pclass, UCF_BUILD_ANYWHERE)
              && !is_native_tile(punit->utype, piter)
              && !is_city_channel_tile(pclass, piter, pcenter)) {
            notify_player(unit_owner(punit), unit_tile(punit),
                          E_UNIT_LOST_MISC, ftc_server,
                          _("When %s was disbanded your %s in %s was trapped, "
                            "and it was therefore lost."),
                          ctl,
                          unit_tile_link(punit),
                          city_link(other_city));
            wipe_unit(punit, ULR_CITY_LOST, NULL);
          }
        } unit_list_iterate_safe_end;
      } else {
        dbv_set(&tile_processed, tile_index(piter));
      }
    } adjc_iterate_end;
  }

  dbv_free(&tile_processed);
  tile_list_destroy(process_queue);

  if (!city_exist(id)) {
    /* Wiping trapped units caused city to disappear. */
    return;
  }

  /* Any remaining supported units are destroyed */
  unit_list_iterate_safe(pcity->units_supported, punit) {
    wipe_unit(punit, ULR_CITY_LOST, NULL);
  } unit_list_iterate_safe_end;

  if (!city_exist(id)) {
    /* Wiping supported units caused city to disappear. */
    return;
  }

  trade_routes_iterate_safe(pcity, proute) {
    struct  trade_route *pback = remove_trade_route(pcity, proute,
                                                    TRUE, TRUE);

    FC_FREE(proute);
    FC_FREE(pback);
  } trade_routes_iterate_safe_end;

  map_clear_border(pcenter);
  city_workers_queue_remove(pcity);
  city_thaw_workers_queue();
  city_refresh_queue_processing();

  /* idex_unregister_city() is called in game_remove_city() below */

  /* identity_number_release(pcity->id) is *NOT* done!  The cities may
     still be alive in the client, or in the player map.  The number of
     removed cities is small, so the loss is acceptable.
   */

  old_vision = pcity->server.vision;
  pcity->server.vision = NULL;
  script_server_remove_exported_object(pcity);
  adv_city_free(pcity);

  /* Remove city from the map. */
  tile_set_worked(pcenter, NULL);

  /* Reveal units. */
  players_iterate(other_player) {
    if (can_player_see_units_in_city(other_player, pcity)
        || !map_is_known_and_seen(pcenter, other_player, V_MAIN)) {
      continue;
    }
    unit_list_iterate(pcenter->units, punit) {
      if (can_player_see_unit(other_player, punit)) {
        send_unit_info(other_player->connections, punit);
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  fc_allocate_mutex(&game.server.mutexes.city_list);
  game_remove_city(&wld, pcity);
  fc_release_mutex(&game.server.mutexes.city_list);

  /* Remove any extras that were only there because the city was there. */
  extra_type_iterate(pextra) {
    if (tile_has_extra(pcenter, pextra)
        && !is_native_tile_to_extra(pextra, pcenter)) {
      tile_extra_rm_apply(pcenter, pextra);
    }
  } extra_type_iterate_end;

  players_iterate(other_player) {
    if (map_is_known_and_seen(pcenter, other_player, V_MAIN)) {
      reality_check_city(other_player, pcenter);
    }
  } players_iterate_end;

  conn_list_iterate(game.est_connections, pconn) {
    if (NULL == pconn->playing && pconn->observer) {
      /* For detached observers we have to send a specific packet.  This is
       * a hack necessitated by the private map that exists for players but
       * not observers. */
      dsend_packet_city_remove(pconn, id);
    }
  } conn_list_iterate_end;

  vision_clear_sight(old_vision);
  vision_free(old_vision);

  /* Infrastructures may have changed. */
  send_tile_info(NULL, pcenter, FALSE);

  /* Build a new palace for free if the player lost her capital and
     savepalace is on. */
  build_free_small_wonders(powner, &had_small_wonders);

  /* Update wonder infos. */
  if (had_great_wonders) {
    send_game_info(NULL);
    /* No need to send to detached connections. */
    send_player_info_c(powner, NULL);
  } else if (BV_ISSET_ANY(had_small_wonders)) {
    /* No need to send to detached connections. */
    send_player_info_c(powner, NULL);
  }

  if (old_content_citizens != player_content_citizens(powner)
      || old_angry_citizens != player_angry_citizens(powner)) {
    /* We crossed the EFT_EMPIRE_SIZE_* effects, we have to refresh all
     * cities for the player. */
    city_refresh_for_player(powner);
  }

  sync_cities();
}

/************************************************************************//**
  Handle unit conquering a city.
    - Can't conquer a city when not at war. (Enters cities peacefully
      during peace. At the moment this can happen to domestic, allied and
      team mate cities)
    - A unit can't conquer a city if it is owned by the animal barbarian.
    - A unit can't conquer a city if its unit class is missing the
      "CanOccupyCity" unit class flag.
    - A unit can't conquer a city if its unit type has the "NonMil" unit
      type flag.
    - Transported units trying to conquer a city should be unloaded before
      this function is called.

  Returns TRUE iff action could be done, FALSE if it couldn't. Even if
  this returns TRUE, unit may have died during the action.
****************************************************************************/
bool unit_conquer_city(struct unit *punit, struct city *pcity)
{
  bool try_civil_war = FALSE;
  bool city_remains;
  int coins;
  struct player *pplayer = unit_owner(punit);
  struct player *cplayer = city_owner(pcity);

  /* If not at war, may peacefully enter city. */
  fc_assert_ret_val_msg(pplayers_at_war(pplayer, cplayer), FALSE,
                        "Can't conquer city during peace.");

  /* If we cannot occupy the city, this unit entering will not trigger the
   * effects below. */
  fc_assert_ret_val_msg(unit_can_take_over(punit)
                        || utype_can_do_action(unit_type_get(punit),
                                               ACTION_PARADROP),
                        FALSE, "Bad unit for city occupation.");

  /* A transported unit trying to conquer a city should already have been
   * unloaded. */
  fc_assert_ret_val_msg(punit->transporter == NULL, FALSE,
                        "Can't conquer city while transported.");

  /* Okay, we're at war - invader captures/destroys city... */
  
  /* If a capital is captured, then spark off a civil war 
     - Kris Bubendorfer
     Also check spaceships --dwp
  */

  if (is_capital(pcity)
      && (cplayer->spaceship.state == SSHIP_STARTED
          || cplayer->spaceship.state == SSHIP_LAUNCHED)) {
    spaceship_lost(cplayer);
  }
  
  if (is_capital(pcity)
      && civil_war_possible(cplayer, TRUE, TRUE)
      && normal_player_count() < MAX_NUM_PLAYERS
      && civil_war_triggered(cplayer)) {
    try_civil_war = TRUE;
  }

  /* 
   * We later remove a citizen. Lets check if we can save this since
   * the city will be destroyed.
   */
  if (city_size_get(pcity) <= 1) {
    int saved_id = pcity->id;

    notify_player(pplayer, city_tile(pcity), E_UNIT_WIN_ATT, ftc_server,
                  _("You destroy %s completely."),
                  city_tile_link(pcity));
    notify_player(cplayer, city_tile(pcity), E_CITY_LOST, ftc_server,
                  _("%s has been destroyed by %s."), 
                  city_tile_link(pcity), player_name(pplayer));
    script_server_signal_emit("city_destroyed", pcity, cplayer, pplayer);

    /* We cant't be sure of city existence after running some script */
    if (city_exist(saved_id)) {
      remove_city(pcity);
    }

    if (try_civil_war) {
      (void) civil_war(cplayer);
    }
    return TRUE;
  }

  coins = cplayer->economic.gold;
  coins = MIN(coins,
              fc_rand((coins / 20) + 1)
              + (coins * (city_size_get(pcity))) / 200);
  pplayer->economic.gold += coins;
  cplayer->economic.gold -= coins;
  send_player_info_c(pplayer, pplayer->connections);
  send_player_info_c(cplayer, cplayer->connections);
  if (pcity->original != pplayer) {
    if (coins > 0) {
      notify_player(pplayer, city_tile(pcity), E_UNIT_WIN_ATT, ftc_server,
		    PL_("You conquer %s; your lootings accumulate"
			" to %d gold!",
			"You conquer %s; your lootings accumulate"
			" to %d gold!", coins), 
		    city_link(pcity),
		    coins);
      notify_player(cplayer, city_tile(pcity), E_CITY_LOST, ftc_server,
		    PL_("%s conquered %s and looted %d gold"
			" from the city.",
			"%s conquered %s and looted %d gold"
			" from the city.", coins),
		    player_name(pplayer),
		    city_link(pcity),
		    coins);
    } else {
      notify_player(pplayer, city_tile(pcity), E_UNIT_WIN_ATT, ftc_server,
		    _("You conquer %s."),
		    city_link(pcity));
      notify_player(cplayer, city_tile(pcity), E_CITY_LOST, ftc_server,
		    _("%s conquered %s."),
		    player_name(pplayer),
		    city_link(pcity));
    }
  } else {
    if (coins > 0) {
      notify_player(pplayer, city_tile(pcity), E_UNIT_WIN_ATT, ftc_server,
		    PL_("You have liberated %s!"
			" Lootings accumulate to %d gold.",
			"You have liberated %s!"
			" Lootings accumulate to %d gold.", coins),
		    city_link(pcity),
		    coins);
      notify_player(cplayer, city_tile(pcity), E_CITY_LOST, ftc_server,
		    PL_("%s liberated %s and looted %d gold"
			" from the city.",
			"%s liberated %s and looted %d gold"
			" from the city.", coins),
		    player_name(pplayer),
		    city_link(pcity),
		    coins);
    } else {
      notify_player(pplayer, city_tile(pcity), E_UNIT_WIN_ATT, ftc_server,
		    _("You have liberated %s!"),
		    city_link(pcity));
      notify_player(cplayer, city_tile(pcity), E_CITY_LOST, ftc_server,
		    _("%s liberated %s."),
		    player_name(pplayer),
		    city_link(pcity));
    }
  }

  if (fc_rand(100) <  get_unit_bonus(punit, EFT_CONQUEST_TECH_PCT)) {
    /* Just try to steal. Ignore failures to get tech */
    steal_a_tech(pplayer, cplayer, A_UNSET);
  }

  /* We transfer the city first so that it is in a consistent state when
   * the size is reduced. */
  /* FIXME: maybe it should be a ruleset option whether barbarians get
   * free buildings such as palaces? */
  city_remains = transfer_city(pplayer, pcity, 0, TRUE, TRUE, TRUE,
                               !is_barbarian(pplayer));

  if (city_remains) {
    /* reduce size should not destroy this city */
    fc_assert(city_size_get(pcity) > 1);
    city_reduce_size(pcity, 1, pplayer, "conquest");
  }

  if (try_civil_war) {
    (void) civil_war(cplayer);
  }

  if (city_remains) {
    script_server_signal_emit("city_transferred", pcity, cplayer, pplayer,
                              "conquest");
    script_server_signal_emit("city_lost", pcity, cplayer, pplayer);
  }

  return TRUE;
}

/************************************************************************//**
  Which wall gfx city should display?
****************************************************************************/
static int city_got_citywalls(const struct city *pcity)
{
  int walls = get_city_bonus(pcity, EFT_VISIBLE_WALLS);

  return walls > 0 ? walls : 0;
}

/************************************************************************//**
  Suppress sending cities during game_load() and end_phase()
****************************************************************************/
bool send_city_suppression(bool now)
{
  bool formerly = send_city_suppressed;

  send_city_suppressed = now;
  return formerly;
}

/************************************************************************//**
  This fills out a package from a player's vision_site.
****************************************************************************/
static void package_dumb_city(struct player* pplayer, struct tile *ptile,
                              struct packet_city_short_info *packet)
{
  struct vision_site *pdcity = map_get_player_city(ptile, pplayer);

  packet->id = pdcity->identity;
  packet->owner = player_number(vision_site_owner(pdcity));

  packet->tile = tile_index(ptile);
  sz_strlcpy(packet->name, pdcity->name);

  packet->size = vision_site_size_get(pdcity);

  packet->occupied = pdcity->occupied;
  packet->walls = pdcity->walls;
  packet->style = pdcity->style;
  packet->city_image = pdcity->city_image;

  packet->happy = pdcity->happy;
  packet->unhappy = pdcity->unhappy;

  packet->improvements = pdcity->improvements;
}

/************************************************************************//**
  Update plrtile information about the city, and send out information to
  the clients if it has changed.
****************************************************************************/
void refresh_dumb_city(struct city *pcity)
{
  players_iterate(pplayer) {
    if (player_can_see_city_externals(pplayer, pcity)) {
      if (update_dumb_city(pplayer, pcity)) {
	struct packet_city_short_info packet;

	if (city_owner(pcity) != pplayer) {
	  /* Don't send the short_city information to someone who can see the
	   * city's internals.  Doing so would really confuse the client. */
	  package_dumb_city(pplayer, pcity->tile, &packet);
	  lsend_packet_city_short_info(pplayer->connections, &packet);
	}
      }
    }
  } players_iterate_end;

  /* Don't send to non-player observers since they don't have 'dumb city'
   * information. */
}

/************************************************************************//**
  Broadcast info about a city to all players who observe the tile. 
  If the player can see the city we update the city info first.
  If not we just use the info from the players private map.
  See also comments to send_city_info_at_tile().
  (Split off from send_city_info_at_tile() because that was getting
  too difficult for me to understand... --dwp)
****************************************************************************/
static void broadcast_city_info(struct city *pcity)
{
  struct packet_city_info packet;
  struct packet_web_city_info_addition web_packet;
  struct packet_city_short_info sc_pack;
  struct player *powner = city_owner(pcity);
  struct traderoute_packet_list *routes = traderoute_packet_list_new();

  /* Send to everyone who can see the city. */
  package_city(pcity, &packet, &web_packet, routes, FALSE);
  players_iterate(pplayer) {
    if (can_player_see_city_internals(pplayer, pcity)) {
      if (!send_city_suppressed || pplayer != powner) {
        update_dumb_city(powner, pcity);
        lsend_packet_city_info(powner->connections, &packet, FALSE);
        web_lsend_packet(city_info_addition, powner->connections, &web_packet, FALSE);
        traderoute_packet_list_iterate(routes, route_packet) {
          lsend_packet_traderoute_info(powner->connections, route_packet);
        } traderoute_packet_list_iterate_end;
      }
    } else {
      if (player_can_see_city_externals(pplayer, pcity)) {
        reality_check_city(pplayer, pcity->tile);
	update_dumb_city(pplayer, pcity);
	package_dumb_city(pplayer, pcity->tile, &sc_pack);
	lsend_packet_city_short_info(pplayer->connections, &sc_pack);
      }
    }
  } players_iterate_end;

  /* Send to global observers. */
  conn_list_iterate(game.est_connections, pconn) {
    if (conn_is_global_observer(pconn)) {
      send_packet_city_info(pconn, &packet, FALSE);
      web_send_packet(city_info_addition, pconn, &web_packet, FALSE);
    }
  } conn_list_iterate_end;

  traderoute_packet_list_iterate(routes, route_packet) {
    FC_FREE(route_packet);
  } traderoute_packet_list_iterate_end;
  traderoute_packet_list_destroy(routes);
}

/************************************************************************//**
  Send to each client information about the cities it knows about.
  dest may not be NULL
****************************************************************************/
void send_all_known_cities(struct conn_list *dest)
{
  conn_list_do_buffer(dest);
  conn_list_iterate(dest, pconn) {
    struct player *pplayer = pconn->playing;

    if (!pplayer && !pconn->observer) {
      continue;
    }
    whole_map_iterate(&(wld.map), ptile) {
      if (!pplayer || NULL != map_get_player_site(ptile, pplayer)) {
	send_city_info_at_tile(pplayer, pconn->self, NULL, ptile);
      }
    } whole_map_iterate_end;
  }
  conn_list_iterate_end;
  conn_list_do_unbuffer(dest);
  flush_packets();
}

/************************************************************************//**
  Send information about all his/her cities to player
****************************************************************************/
void send_player_cities(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    if (city_refresh(pcity)) {
      log_error("%s radius changed while sending to player.",
                city_name_get(pcity));

      /* Make sure that no workers in illegal position outside radius. */
      auto_arrange_workers(pcity);
    }
    send_city_info(pplayer, pcity);
  }
  city_list_iterate_end;
}

/************************************************************************//**
  A wrapper, accessing either broadcast_city_info() (dest == NULL),
  or a convenience case of send_city_info_at_tile().
  Must specify non-NULL pcity.
****************************************************************************/
void send_city_info(struct player *dest, struct city *pcity)
{
  struct player *powner = city_owner(pcity);

  if (S_S_RUNNING != server_state() && S_S_OVER != server_state()) {
    return;
  }

  if (dest == powner && send_city_suppressed) {
    return;
  }

  if (!dest || dest == powner) {
    pcity->server.synced = TRUE;
  }

  if (!dest) {
    broadcast_city_info(pcity);
  } else {
    send_city_info_at_tile(dest, dest->connections, pcity, pcity->tile);
  }

  if (game.info.team_pooled_research
      && player_list_size(team_members(powner->team)) > 1) {
    /* We want to send the new total bulbs production of the team. */
    send_research_info(research_get(powner), NULL);
  }
}

/************************************************************************//**
  Send info about a city, as seen by pviewer, to dest (usually dest will
  be pviewer->connections). If pplayer can see the city we update the city
  info first. If not we just use the info from the players private map.

  If (pviewer == NULL) this is for observers, who see everything (?)
  For this function dest may not be NULL.  See send_city_info() and
  broadcast_city_info().

  If pcity is non-NULL it should be same as tile_city(x,y); if pcity
  is NULL, this function calls tile_city(x,y) (it is ok if this
  returns NULL).

  Sometimes a player's map contain a city that doesn't actually exist. Use
  reality_check_city(pplayer, ptile) to update that. Remember to NOT send info
  about a city to a player who thinks the tile contains another city. If you
  want to update the clients info of the tile you must use
  reality_check_city(pplayer, ptile) first. This is generally taken care of
  automatically when a tile becomes visible.
****************************************************************************/
void send_city_info_at_tile(struct player *pviewer, struct conn_list *dest,
                            struct city *pcity, struct tile *ptile)
{
  struct packet_city_info packet;
  struct packet_web_city_info_addition web_packet;
  struct packet_city_short_info sc_pack;
  struct player *powner = NULL;
  struct traderoute_packet_list *routes = NULL;

  if (!pcity) {
    pcity = tile_city(ptile);
  }
  if (pcity) {
    powner = city_owner(pcity);
  }
  if (powner && powner == pviewer) {
    /* send info to owner */
    /* This case implies powner non-NULL which means pcity non-NULL */
    if (!send_city_suppressed) {
      routes = traderoute_packet_list_new();

      /* send all info to the owner */
      update_dumb_city(powner, pcity);
      package_city(pcity, &packet, &web_packet, routes, FALSE);
      lsend_packet_city_info(dest, &packet, FALSE);
      web_lsend_packet(city_info_addition, dest, &web_packet, FALSE);
      traderoute_packet_list_iterate(routes, route_packet) {
        lsend_packet_traderoute_info(dest, route_packet);
      } traderoute_packet_list_iterate_end;
      if (dest == powner->connections) {
        /* HACK: send also a copy to global observers. */
        conn_list_iterate(game.est_connections, pconn) {
          if (conn_is_global_observer(pconn)) {
            send_packet_city_info(pconn, &packet, FALSE);
            traderoute_packet_list_iterate(routes, route_packet) {
              send_packet_traderoute_info(pconn, route_packet);
            } traderoute_packet_list_iterate_end;
          }
        } conn_list_iterate_end;
      }
    }
  } else {
    /* send info to non-owner */
    if (!pviewer) {	/* observer */
      if (pcity) {
        routes = traderoute_packet_list_new();

	package_city(pcity, &packet, &web_packet, routes, FALSE);   /* should be dumb_city info? */
        lsend_packet_city_info(dest, &packet, FALSE);
        web_lsend_packet(city_info_addition, dest, &web_packet, FALSE);
        traderoute_packet_list_iterate(routes, route_packet) {
          lsend_packet_traderoute_info(dest, route_packet);
        } traderoute_packet_list_iterate_end;
      }
    } else {
      if (!map_is_known(ptile, pviewer)) {
	/* Without the conditional we'd have an infinite loop here. */
	map_show_tile(pviewer, ptile);
      }
      if (map_is_known_and_seen(ptile, pviewer, V_MAIN)) {
	if (pcity) {		/* it's there and we see it; update and send */
	  update_dumb_city(pviewer, pcity);
	  package_dumb_city(pviewer, ptile, &sc_pack);
	  lsend_packet_city_short_info(dest, &sc_pack);
	}
      } else {			/* not seen; send old info */
	if (NULL != map_get_player_site(ptile, pviewer)) {
	  package_dumb_city(pviewer, ptile, &sc_pack);
	  lsend_packet_city_short_info(dest, &sc_pack);
	}
      }
    }
  }

  if (routes != NULL) {
    traderoute_packet_list_iterate(routes, route_packet) {
      FC_FREE(route_packet);
    } traderoute_packet_list_iterate_end;
    traderoute_packet_list_destroy(routes);
  }
}

/************************************************************************//**
  Fill city info packet with information about given city.
****************************************************************************/
void package_city(struct city *pcity, struct packet_city_info *packet,
                  struct packet_web_city_info_addition *web_packet,
                  struct traderoute_packet_list *routes,
                  bool dipl_invest)
{
  int i;
  int ppl = 0;
  char can_build_impr_buf[MAX_NUM_ITEMS + 1];
  char can_build_unit_buf[MAX_NUM_ITEMS + 1];
  char food_output_buf[MAX_NUM_ITEMS + 1];
  char shield_output_buf[MAX_NUM_ITEMS + 1];
  char trade_output_buf[MAX_NUM_ITEMS + 1];
  struct tile *pcenter = city_tile(pcity);
  int c = 0;

  packet->id = pcity->id;
  packet->owner = player_number(city_owner(pcity));
  packet->tile = tile_index(city_tile(pcity));
  sz_strlcpy(packet->name, city_name_get(pcity));

  packet->size = city_size_get(pcity);
  for (i = 0; i < FEELING_LAST; i++) {
    packet->ppl_happy[i] = pcity->feel[CITIZEN_HAPPY][i];
    packet->ppl_content[i] = pcity->feel[CITIZEN_CONTENT][i];
    packet->ppl_unhappy[i] = pcity->feel[CITIZEN_UNHAPPY][i];
    packet->ppl_angry[i] = pcity->feel[CITIZEN_ANGRY][i];
    if (i == 0) {
      ppl += packet->ppl_happy[i];
      ppl += packet->ppl_content[i];
      ppl += packet->ppl_unhappy[i];
      ppl += packet->ppl_angry[i];
    }
  }
  /* The number of data in specialists[] array */
  packet->specialists_size = specialist_count();
  specialist_type_iterate(sp) {
    packet->specialists[sp] = pcity->specialists[sp];
    ppl += packet->specialists[sp];
  } specialist_type_iterate_end;

  /* The nationality of the citizens. */
  packet->nationalities_count = 0;
  if (game.info.citizen_nationality) {
    int cit = 0;

    player_slots_iterate(pslot) {
      citizens nationality = citizens_nation_get(pcity, pslot);
      if (nationality != 0) {
        /* This player should exist! */
        fc_assert(player_slot_get_player(pslot) != NULL);

        packet->nation_id[packet->nationalities_count]
          = player_slot_index(pslot);
        packet->nation_citizens[packet->nationalities_count]
          = nationality;
        packet->nationalities_count++;

        cit += nationality;
      }
    } player_slots_iterate_end;

    fc_assert(cit == packet->size);
  }

  packet->history = pcity->history;
  packet->culture = city_culture(pcity);
  packet->buy_cost = city_production_buy_gold_cost(pcity);

  if (packet->size != ppl) {
    static bool recursion = FALSE;

    if (recursion) {
      /* Recursion didn't help. Do not enter infinite recursive loop.
       * Package city as it is. */
      log_error("Failed to fix inconsistent city size.");
      recursion = FALSE;
    } else {
      /* Note: If you get this error and try to debug the cause, you may find
       *       using sanity_check_feelings() in some key points useful. */
      /* Have this as an fc_assert() first, so one can use '-F' to caught these in
       * debugger. */
      fc_assert(packet->size == ppl);

      /* In all builds have an error message shown. */
      log_error("City size %d, citizen count %d for %s",
                packet->size, ppl, city_name_get(pcity));

      /* Try to fix */
      city_refresh(pcity);
      auto_arrange_workers(pcity);

      /* And repackage */
      recursion = TRUE;
      package_city(pcity, packet, web_packet, routes, dipl_invest);
      recursion = FALSE;

      return;
    }
  }

  packet->city_radius_sq = pcity->city_radius_sq;

  i = 0;
  trade_routes_iterate(pcity, proute) {
    struct packet_traderoute_info *tri_packet = fc_malloc(sizeof(struct packet_traderoute_info));

    tri_packet->city = pcity->id;
    tri_packet->index = i;
    tri_packet->partner = proute->partner;
    tri_packet->value = proute->value;
    tri_packet->direction = proute->dir;
    tri_packet->goods = goods_number(proute->goods);

    traderoute_packet_list_append(routes, tri_packet);

    i++;
  } trade_routes_iterate_end;

  packet->traderoute_count = i;

  output_type_iterate(o) {
    packet->surplus[o] = pcity->surplus[o];
    packet->waste[o] = pcity->waste[o];
    packet->unhappy_penalty[o] = pcity->unhappy_penalty[o];
    packet->prod[o] = pcity->prod[o];
    packet->citizen_base[o] = pcity->citizen_base[o];
    packet->usage[o] = pcity->usage[o];
  } output_type_iterate_end;

  packet->food_stock = pcity->food_stock;
  packet->shield_stock = pcity->shield_stock;
  packet->pollution = pcity->pollution;
  packet->illness_trade = pcity->illness_trade;
  packet->city_options = pcity->city_options;

  packet->production_kind = pcity->production.kind;
  packet->production_value = universal_number(&pcity->production);

  packet->turn_last_built=pcity->turn_last_built;
  packet->turn_founded = pcity->turn_founded;

  packet->changed_from_kind = pcity->changed_from.kind;
  packet->changed_from_value = universal_number(&pcity->changed_from);

  packet->before_change_shields=pcity->before_change_shields;
  packet->disbanded_shields=pcity->disbanded_shields;
  packet->caravan_shields=pcity->caravan_shields;
  packet->last_turns_shield_surplus = pcity->last_turns_shield_surplus;

  worklist_copy(&packet->worklist, &pcity->worklist);
  packet->diplomat_investigate=dipl_invest;

  packet->airlift = pcity->airlift;
  packet->did_buy = pcity->did_buy;
  packet->did_sell = pcity->did_sell;
  packet->was_happy = pcity->was_happy;

  packet->walls = city_got_citywalls(pcity);
  packet->style = pcity->style;
  packet->city_image = get_city_bonus(pcity, EFT_CITY_IMAGE);

  improvement_iterate(pimprove) {
    can_build_impr_buf[improvement_index(pimprove)] = 
	    can_city_build_improvement_now(pcity, pimprove)
      ? '1' : '0';
  } improvement_iterate_end;
  can_build_impr_buf[improvement_count()] = '\0';
  sz_strlcpy(packet->can_build_improvement, can_build_impr_buf);

  unit_type_iterate(punittype) {
    can_build_unit_buf[utype_index(punittype)] = 
	    can_city_build_unit_now(pcity, punittype)
      ? '1' : '0';
  } unit_type_iterate_end;
  can_build_unit_buf[utype_count()] = '\0';
  sz_strlcpy(packet->can_build_unit, can_build_unit_buf);


  BV_CLR_ALL(packet->improvements);
  improvement_iterate(pimprove) {
    if (city_has_building(pcity, pimprove)) {
      BV_SET(packet->improvements, improvement_index(pimprove));
    }
  } improvement_iterate_end;

#ifdef FREECIV_WEB
  web_packet->id = pcity->id;

  web_packet->granary_size = city_granary_size(city_size_get(pcity));
  web_packet->granary_turns = city_turns_to_grow(pcity);
#endif /* FREECIV_WEB */

  city_tile_iterate(city_map_radius_sq_get(pcity), pcenter, ptile) {
    char f[2];
    char s[2];
    char t[2];

    fc_snprintf(f, sizeof(f), "%d", city_tile_output_now(pcity, ptile, O_FOOD));
    fc_snprintf(s, sizeof(s), "%d", city_tile_output_now(pcity, ptile, O_SHIELD));
    fc_snprintf(t, sizeof(t), "%d", city_tile_output_now(pcity, ptile, O_TRADE));
    food_output_buf[c] = f[0];
    shield_output_buf[c] = s[0];
    trade_output_buf[c] = t[0];

    c += 1;

  } city_tile_iterate_end;
  food_output_buf[c] = '\0';
  shield_output_buf[c] = '\0';
  trade_output_buf[c] = '\0';
  sz_strlcpy(packet->food_output, food_output_buf);
  sz_strlcpy(packet->shield_output, shield_output_buf);
  sz_strlcpy(packet->trade_output, trade_output_buf);
}

/************************************************************************//**
  Updates a players knowledge about a city. If the player_tile already
  contains a city it must be the same city (avoid problems by always calling
  reality_check_city() first)

  Returns TRUE iff anything has changed for the player city (i.e., if the
  client needs to be updated with a *short* city packet).  This information
  is only used in refresh_dumb_cities; elsewhere the data is (of necessity)
  broadcast regardless.
****************************************************************************/
bool update_dumb_city(struct player *pplayer, struct city *pcity)
{
  bv_imprs improvements;
  struct tile *pcenter = city_tile(pcity);
  struct vision_site *pdcity = map_get_player_city(pcenter, pplayer);
  /* pcity->client.occupied isn't used at the server, so we go straight to the
   * unit list to check the occupied status. */
  bool occupied = (unit_list_size(pcenter->units) > 0);
  bool walls = city_got_citywalls(pcity);
  bool happy = city_happy(pcity);
  bool unhappy = city_unhappy(pcity);
  int style = pcity->style;
  int city_image = get_city_bonus(pcity, EFT_CITY_IMAGE);

  BV_CLR_ALL(improvements);
  improvement_iterate(pimprove) {
    if (is_improvement_visible(pimprove)
     && city_has_building(pcity, pimprove)) {
      BV_SET(improvements, improvement_index(pimprove));
    }
  } improvement_iterate_end;

  if (NULL == pdcity) {
    pdcity = vision_site_new_from_city(pcity);
    change_playertile_site(map_get_player_tile(pcenter, pplayer), pdcity);
  } else if (pdcity->location != pcenter) {
    log_error("Trying to update bad city (wrong location) "
              "at %i,%i for player %s",
              TILE_XY(pcity->tile), player_name(pplayer));
    pdcity->location = pcenter;   /* ?? */
  } else if (pdcity->identity != pcity->id) {
    log_error("Trying to update old city (wrong identity) "
              "at %i,%i for player %s",
              TILE_XY(city_tile(pcity)), player_name(pplayer));
    pdcity->identity = pcity->id;   /* ?? */
  } else if (pdcity->occupied == occupied
             && pdcity->walls == walls
             && pdcity->happy == happy
             && pdcity->unhappy == unhappy
             && pdcity->style == style
             && pdcity->city_image == city_image
             && BV_ARE_EQUAL(pdcity->improvements, improvements)
             && vision_site_size_get(pdcity) == city_size_get(pcity)
             && vision_site_owner(pdcity) == city_owner(pcity)
             && 0 == strcmp(pdcity->name, city_name_get(pcity))) {
    return FALSE;
  }

  vision_site_update_from_city(pdcity, pcity);
  pdcity->occupied = occupied;
  pdcity->walls = walls;
  pdcity->style = style;
  pdcity->city_image = city_image;
  pdcity->happy = happy;
  pdcity->unhappy = unhappy;
  pdcity->improvements = improvements;

  return TRUE;
}

/************************************************************************//**
  Removes outdated (nonexistant) cities from a player
****************************************************************************/
void reality_check_city(struct player *pplayer, struct tile *ptile)
{
  struct vision_site *pdcity = map_get_player_city(ptile, pplayer);

  if (pdcity) {
    struct city *pcity = tile_city(ptile);

    if (!pcity || pcity->id != pdcity->identity) {
      struct player_tile *playtile = map_get_player_tile(ptile, pplayer);

      dlsend_packet_city_remove(pplayer->connections, pdcity->identity);
      fc_assert_ret(playtile->site == pdcity);
      playtile->site = NULL;
      vision_site_destroy(pdcity);
    }
  }
}

/************************************************************************//**
  Removes a dumb city.  Called when the vision changed to unknown.
****************************************************************************/
void remove_dumb_city(struct player *pplayer, struct tile *ptile)
{
  struct vision_site *pdcity = map_get_player_city(ptile, pplayer);

  if (pdcity) {
    struct player_tile *playtile = map_get_player_tile(ptile, pplayer);

    dlsend_packet_city_remove(pplayer->connections, pdcity->identity);
    fc_assert_ret(playtile->site == pdcity);
    playtile->site = NULL;
    vision_site_destroy(pdcity);
  }
}

/************************************************************************//**
  Announce to the owners of the cities that trade route has been canceled
  between them.
****************************************************************************/
static void announce_trade_route_removal(struct city *pc1, struct city *pc2,
                                         bool source_gone)
{
  struct player *plr1 = city_owner(pc1);
  struct player *plr2 = city_owner(pc2);
  char city1_link[MAX_LEN_LINK];
  char city2_link[MAX_LEN_LINK];

  sz_strlcpy(city1_link, city_link(pc1));
  sz_strlcpy(city2_link, city_link(pc2));

  if (plr1 == plr2) {
    if (source_gone) {
      notify_player(plr2, city_tile(pc2),
                    E_CARAVAN_ACTION, ftc_server,
                    _("Trade between %s and %s lost along with city."),
                    city1_link, city2_link);
    } else {
      notify_player(plr1, city_tile(pc1),
                    E_CARAVAN_ACTION, ftc_server,
                    _("Trade route between %s and %s canceled."),
                    city1_link, city2_link);
    }
  } else {
    if (source_gone) {
      notify_player(plr2, city_tile(pc2),
                    E_CARAVAN_ACTION, ftc_server,
                    /* TRANS: "...between Spanish city Madrid and Paris..." */
                    _("Trade between %s city %s and %s lost along with "
                      "their city."),
                    nation_adjective_for_player(plr1), city1_link, city2_link);
      /* It's implicit to removed city's owner that that city no longer
       * has trade routes, so say nothing in that case */
    } else {
      notify_player(plr2, city_tile(pc2),
                    E_CARAVAN_ACTION, ftc_server,
                    _("Sorry, the %s canceled the trade route "
                      "from %s to your city %s."),
                    nation_plural_for_player(plr1), city1_link, city2_link);
      notify_player(plr1, city_tile(pc1),
                    E_CARAVAN_ACTION, ftc_server,
                    /* TRANS: "...from Paris to Spanish city Madrid." */
                    _("We canceled the trade route "
                      "from %s to %s city %s."),
                    city1_link, nation_adjective_for_player(plr2), city2_link);
    }
  }
}

/************************************************************************//**
  Remove the trade route between pc1 and pc2 (if one exists).
  source_gone should be TRUE if the reason for removal is the imminent
  removal of the source city (pc1) from the game.

  Does not free the trade route structures, only removes them from the
  cities.
****************************************************************************/
struct trade_route *remove_trade_route(struct city *pc1, struct trade_route *proute,
                                       bool announce, bool source_gone)
{
  struct city *pc2 = game_city_by_number(proute->partner);
  struct trade_route *back_route = NULL;

  fc_assert_ret_val(pc1 && proute, NULL);

  trade_route_list_remove(pc1->routes, proute);

  if (pc2 != NULL) {
    trade_routes_iterate(pc2, pback) {
      if (pc1->id == pback->partner) {
        back_route = pback;
      }
    } trade_routes_iterate_end;

    if (back_route != NULL) {
      trade_route_list_remove(pc2->routes, back_route);
    }
  }

  if (announce) {
    announce_trade_route_removal(pc1, pc2, source_gone);

    city_refresh(pc2);
    send_city_info(city_owner(pc2), pc2);
  }

  return back_route;
}

/************************************************************************//**
  Remove/cancel the city's least valuable trade routes.
****************************************************************************/
static void remove_smallest_trade_routes(struct city *pcity)
{
  struct trade_route_list *smallest = trade_route_list_new();

  (void) city_trade_removable(pcity, smallest);
  trade_route_list_iterate(smallest, proute) {
    struct trade_route *back;

    back = remove_trade_route(pcity, proute, TRUE, FALSE);
    free(proute);
    free(back);
  } trade_route_list_iterate_end;
  trade_route_list_destroy(smallest);
}

/************************************************************************//**
  Establish a trade route.
****************************************************************************/
void establish_trade_route(struct city *pc1, struct city *pc2)
{
  struct trade_route *proute;

  if (city_num_trade_routes(pc1) >= max_trade_routes(pc1)) {
    remove_smallest_trade_routes(pc1);
  }

  if (city_num_trade_routes(pc2) >= max_trade_routes(pc2)) {
    remove_smallest_trade_routes(pc2);
  }

  proute = fc_malloc(sizeof(struct trade_route));
  proute->partner = pc2->id;
  proute->dir = RDIR_FROM;
  trade_route_list_append(pc1->routes, proute);

  proute = fc_malloc(sizeof(struct trade_route));
  proute->partner = pc1->id;
  proute->dir = RDIR_TO;
  trade_route_list_append(pc2->routes, proute);

  /* recalculate illness due to trade */
  if (game.info.illness_on) {
    pc1->server.illness = city_illness_calc(pc1, NULL, NULL,
                                            &(pc1->illness_trade), NULL);
    pc2->server.illness = city_illness_calc(pc2, NULL, NULL,
                                            &(pc2->illness_trade), NULL);
  }
}

/************************************************************************//**
  Sell the improvement from the city, and give the player the owner.  Not
  all buildings can be sold.

  I guess the player should always be the city owner?
****************************************************************************/
void do_sell_building(struct player *pplayer, struct city *pcity,
                      struct impr_type *pimprove, const char *reason)
{
  if (can_city_sell_building(pcity, pimprove)) {
    pplayer->economic.gold += impr_sell_gold(pimprove);
    building_lost(pcity, pimprove, reason, NULL);
  }
}

/************************************************************************//**
  Remove building from the city. Emits lua signal.
****************************************************************************/
bool building_removed(struct city *pcity, const struct impr_type *pimprove,
                      const char *reason, struct unit *destroyer)
{
  int backup = pcity->id;

  city_remove_improvement(pcity, pimprove);

  script_server_signal_emit("building_lost", pcity, pimprove, reason,
                            destroyer);

  return city_exist(backup);
}

/************************************************************************//**
  Destroy the improvement in the city straight-out.  Note that this is
  different from selling a building.
****************************************************************************/
void building_lost(struct city *pcity, const struct impr_type *pimprove,
                   const char *reason, struct unit *destroyer)
{
  struct player *owner = city_owner(pcity);
  bool was_capital = is_capital(pcity);
  bool city_remains;
 
  city_remains = building_removed(pcity, pimprove, reason, destroyer);

  if ((was_capital && (!city_remains || !is_capital(pcity)))
      && (owner->spaceship.state == SSHIP_STARTED
	  || owner->spaceship.state == SSHIP_LAUNCHED)) {
    /* If the capital was lost (by destruction of the palace) production on
     * the spaceship is lost. */
    spaceship_lost(owner);
  }

  if (city_remains) {
    /* update city; influence of effects (buildings, ...) on unit upkeep */
    if (city_refresh(pcity)) {
      auto_arrange_workers(pcity);
    }

    /* Re-update the city's visible area.  This updates fog if the vision
     * range increases or decreases. */
    city_refresh_vision(pcity);
  }
}

/************************************************************************//**
  Recalculate the upkeep needed for all units supported by the city. It has
  to be called

  - if a save game is loaded (via city_refresh() in game_load_internal())

  - if the number of units supported by a city changes
    * create a unit (in create_unit_full())
    * bride a unit (in diplomat_bribe())
    * change homecity (in unit_change_homecity_handling())
    * destroy a unit (in wipe_unit())

  - if the rules for the upkeep calculation change. This is due to effects
    which influence the upkeep calculation.
    * tech researched, government change (via city_refresh())
    * building destroyed (in building_lost())
    * building created (via city_refresh() in in city_build_building())

  If the upkeep for a unit changes, an update is send to the player.
****************************************************************************/
void city_units_upkeep(const struct city *pcity)
{
  int free_uk[O_LAST];
  int cost;
  struct unit_type *ut;
  struct player *plr;
  bool update;

  if (!pcity || !pcity->units_supported
      || unit_list_size(pcity->units_supported) < 1) {
    return;
  }

  memset(free_uk, 0, O_LAST * sizeof(*free_uk));
  output_type_iterate(o) {
    free_uk[o] = get_city_output_bonus(pcity, get_output_type(o),
                                       EFT_UNIT_UPKEEP_FREE_PER_CITY);
  } output_type_iterate_end;

  /* save the upkeep for all units in the corresponding punit struct */
  unit_list_iterate(pcity->units_supported, punit) {
    ut = unit_type_get(punit);
    plr = unit_owner(punit);
    update = FALSE;

    output_type_iterate(o) {
      cost = utype_upkeep_cost(ut, plr, o);
      if (cost > 0) {
        if (free_uk[o] > cost) {
          free_uk[o] -= cost;
          cost = 0;
        } else {
          cost -= free_uk[o];
          free_uk[o] = 0;
        }
      }

      if (cost != punit->upkeep[o]) {
        update = TRUE;
        punit->upkeep[o] = cost;
      }
    } output_type_iterate_end;

    if (update) {
      /* Update unit information to the player and global observers. */
      send_unit_info(NULL, punit);
    }
  } unit_list_iterate_end;
}

/************************************************************************//**
  Change the build target.
****************************************************************************/
void change_build_target(struct player *pplayer, struct city *pcity,
                         struct universal *target,
                         enum event_type event)
{
  const char *name;
  const char *source;

  /* If the city is already building this thing, don't do anything */
  if (are_universals_equal(&pcity->production, target)) {
    return;
  }

  /* Is the city no longer building a wonder? */
  if (VUT_IMPROVEMENT == pcity->production.kind
      && is_great_wonder(pcity->production.value.building)
      && event != E_IMP_AUTO
      && event != E_WORKLIST) {
    /* If the build target is changed because of an advisor's suggestion or
       because the worklist advances, then the wonder was completed -- 
       don't announce that the player has *stopped* building that wonder. 
       */
    notify_player(NULL, city_tile(pcity), E_WONDER_STOPPED, ftc_server,
                  _("The %s have stopped building The %s in %s."),
                  nation_plural_for_player(pplayer),
                  city_production_name_translation(pcity),
                  city_link(pcity));
  }

  /* Manage the city change-production penalty.
     (May penalize, restore or do nothing to the shield_stock.) */
  if (!is_ai(pplayer) || has_handicap(pplayer, H_PRODCHGPEN)) {
    pcity->shield_stock = city_change_production_penalty(pcity, target);
  }

  /* Change build target. */
  pcity->production = *target;

  /* What's the name of the target? */
  name = city_production_name_translation(pcity);

  switch (event) {
    case E_WORKLIST:
      /* TRANS: Possible 'source' of the production change
       * (in "<city> is building ..." sentence). */
      source = _(" from the worklist");
      break;
    case E_IMP_AUTO:
      /* TRANS: Possible 'source' of the production change
       * (in "<city> is building ..." sentence). */
      source = _(" as suggested by the advisor");
      break;
    default:
      source = "";
      break;
  }

  log_base(LOG_BUILD_TARGET, "%s started building %s%s.",
           city_name_get(pcity), name, source);

  /* Tell the player what's up. */
  /* FIXME: this may give bad grammar when translated if the 'source'
   * string can have multiple values. */
  notify_player(pplayer, city_tile(pcity), event, ftc_server,
                /* TRANS: "<city> is building <production><source>."
                 * 'source' might be an empty string, or a clause like
                 * " from the worklist". */
                _("%s is building %s%s."),
                city_link(pcity),
                name, source);

  /* If the city is building a wonder, tell the rest of the world
     about it. */
  if (VUT_IMPROVEMENT == pcity->production.kind
      && is_great_wonder(pcity->production.value.building)) {
    notify_player(NULL, city_tile(pcity), E_WONDER_STARTED, ftc_server,
                  _("The %s have started building The %s in %s."),
                  nation_plural_for_player(pplayer),
                  name,
                  city_link(pcity));
  }
}

/************************************************************************//**
  Change from worked to empty.
  city_x, city_y are city map coordinates.
  Call sync_cities() to send the affected cities to the clients.
****************************************************************************/
void city_map_update_empty(struct city *pcity, struct tile *ptile)
{
  tile_set_worked(ptile, NULL);
  send_tile_info(NULL, ptile, FALSE);
  pcity->server.synced = FALSE;
}

/************************************************************************//**
  Change from empty to worked.
  city_x, city_y are city map coordinates.
  Call sync_cities() to send the affected cities to the clients.
****************************************************************************/
void city_map_update_worker(struct city *pcity, struct tile *ptile)
{
  tile_set_worked(ptile, pcity);
  send_tile_info(NULL, ptile, FALSE);
  pcity->server.synced = FALSE;
}

/************************************************************************//**
  Updates the worked status of a tile.

  If the status changes, auto_arrange_workers() may be called.
****************************************************************************/
static bool city_map_update_tile_direct(struct tile *ptile, bool queued)
{
  struct city *pwork = tile_worked(ptile);

  if (NULL != pwork
   && !is_free_worked(pwork, ptile)
   && !city_can_work_tile(pwork, ptile)) {
    tile_set_worked(ptile, NULL);
    send_tile_info(NULL, ptile, FALSE);

    pwork->specialists[DEFAULT_SPECIALIST]++; /* keep city sanity */
    pwork->server.synced = FALSE;

    if (queued) {
      city_freeze_workers_queue(pwork); /* place the displaced later */
    } else {
      city_refresh(pwork); /* Specialist added, keep citizen count sanity */
      auto_arrange_workers(pwork);
      send_city_info(NULL, pwork);
    }
    return TRUE;
  }

  return FALSE;
}

/************************************************************************//**
  Updates the worked status of a tile.
  Call city_thaw_workers_queue() followed by sync_cities() to send the
  affected cities to the clients.
****************************************************************************/
bool city_map_update_tile_frozen(struct tile *ptile)
{
  return city_map_update_tile_direct(ptile, TRUE);
}

/************************************************************************//**
  Updates the worked status of a tile immediately.
****************************************************************************/
bool city_map_update_tile_now(struct tile *ptile)
{
  return city_map_update_tile_direct(ptile, FALSE);
}

/************************************************************************//**
  Make sure all players (clients) have up-to-date information about all
  their cities.
****************************************************************************/
void sync_cities(void)
{
  if (send_city_suppressed) {
    return;
  }

  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      if (!pcity->server.synced) {
	/* sending will set to TRUE. */
	send_city_info(pplayer, pcity);
      }
    } city_list_iterate_end;
  } players_iterate_end;
}

/************************************************************************//**
  Called by auto_arrange_workers() and below.
****************************************************************************/
void city_map_update_all(struct city *pcity)
{
  struct tile *pcenter = city_tile(pcity);

  city_tile_iterate_skip_free_worked(city_map_radius_sq_get(pcity), pcenter,
                                     ptile, _index, _x, _y) {
    /* bypass city_map_update_tile_now() for efficiency */
    city_map_update_tile_direct(ptile, FALSE);
  } city_tile_iterate_skip_free_worked_end;
}

/************************************************************************//**
  Update worked map of all cities of given player
****************************************************************************/
void city_map_update_all_cities_for_player(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    city_freeze_workers(pcity);
    city_map_update_all(pcity);
    city_thaw_workers(pcity);
  } city_list_iterate_end;
}

/************************************************************************//**
  For each city adjacent to ptile, check all the buildings in the city.
  Any which have unmet terrain requirements will be sold.  This is called
  after any terrain changes (but this may be tied to the default ruleset).

  FIXME: This function isn't very general.  It would be better to check
  each turn to make sure all requirements of all buildings of all cities
  are met, and sell any buildings that can't be supported.  Terrains aren't
  the only requirement that may disappear.
****************************************************************************/
void city_landlocked_sell_coastal_improvements(struct tile *ptile)
{
  adjc_iterate(&(wld.map), ptile, tile1) {
    struct city *pcity = tile_city(tile1);

    if (pcity && !is_terrain_class_near_tile(tile1, TC_OCEAN)) {
      struct player *pplayer = city_owner(pcity);

      /* Sell all buildings (but not Wonders) that must be next to the ocean */
      city_built_iterate(pcity, pimprove) {
        if (!can_city_sell_building(pcity, pimprove)) {
          continue;
        }

	requirement_vector_iterate(&pimprove->reqs, preq) {
	  if ((VUT_TERRAIN == preq->source.kind
               || VUT_TERRAINCLASS == preq->source.kind)
              && !is_req_active(city_owner(pcity), NULL, pcity, NULL,
                                NULL, NULL, NULL, NULL, NULL, NULL,
				preq, TRUE)) {
            int price = impr_sell_gold(pimprove);

            do_sell_building(pplayer, pcity, pimprove, "landlocked");
            notify_player(pplayer, tile1, E_IMP_SOLD, ftc_server,
                          PL_("You sell %s in %s (now landlocked)"
                              " for %d gold.",
                              "You sell %s in %s (now landlocked)"
                              " for %d gold.", price),
                          improvement_name_translation(pimprove),
                          city_link(pcity), price);
	  }
	} requirement_vector_iterate_end;
      } city_built_iterate_end;
    }
  } adjc_iterate_end;
}

/************************************************************************//**
  Refresh the city's vision.

  This function has very small overhead and can be called any time effects
  may have changed the vision range of the city.
****************************************************************************/
void city_refresh_vision(struct city *pcity)
{
  v_radius_t vision_radius_sq =
      V_RADIUS(get_city_bonus(pcity, EFT_CITY_VISION_RADIUS_SQ), 2, 2);

  vision_change_sight(pcity->server.vision, vision_radius_sq);
  ASSERT_VISION(pcity->server.vision);
}

/************************************************************************//**
  Refresh the vision of all cities owned by a player, for empire-wide
  effects.
****************************************************************************/
void refresh_player_cities_vision(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    city_refresh_vision(pcity);
  } city_list_iterate_end;
}

/************************************************************************//**
  Updates the squared city radius. Returns if the radius is changed.
****************************************************************************/
bool city_map_update_radius_sq(struct city *pcity)
{

  fc_assert_ret_val(pcity != NULL, FALSE);

  int city_tiles_old, city_tiles_new;
  int city_radius_sq_old = city_map_radius_sq_get(pcity);
  int city_radius_sq_new = game.info.init_city_radius_sq
                           + get_city_bonus(pcity, EFT_CITY_RADIUS_SQ);

  /* check minimum / maximum allowed city radii */
  city_radius_sq_new = CLIP(CITY_MAP_MIN_RADIUS_SQ, city_radius_sq_new,
                            CITY_MAP_MAX_RADIUS_SQ);

  if (city_radius_sq_new == city_radius_sq_old) {
    /* no change */
    return FALSE;
  }

  /* get number of city tiles for each radii */
  city_tiles_old = city_map_tiles(city_radius_sq_old);
  city_tiles_new = city_map_tiles(city_radius_sq_new);

  if (city_tiles_old == city_tiles_new) {
    /* a change of the squared city radius but no change of the number of
     * city tiles */
    return FALSE;;
  }

  log_debug("[%s (%d)] city_map_radius_sq: %d => %d", city_name_get(pcity),
            pcity->id, city_radius_sq_old, city_radius_sq_new);

  /* workers map before */
  log_debug("[%s (%d)] city size: %d; specialists: %d (before change)",
            city_name_get(pcity), pcity->id, city_size_get(pcity),
            city_specialists(pcity));
  citylog_map_workers(LOG_DEBUG, pcity);

  city_map_radius_sq_set(pcity, city_radius_sq_new);

  if (city_tiles_old < city_tiles_new) {
    /* increased number of city tiles */
    city_refresh_vision(pcity);
  } else {
    /* reduced number of city tiles */
    int workers = 0;

    /* remove workers from the tiles removed rom the city map */
    city_map_iterate_radius_sq(city_radius_sq_new, city_radius_sq_old,
                               city_x, city_y) {
      struct tile *ptile = city_map_to_tile(city_tile(pcity),
                                            city_radius_sq_old, city_x,
                                            city_y);

      if (ptile && pcity == tile_worked(ptile)) {
        city_map_update_empty(pcity, ptile);
        workers++;
      }
    } city_map_iterate_radius_sq_end;

    /* add workers to free city tiles */
    if (workers > 0) {
      int radius_sq = city_map_radius_sq_get(pcity);
      city_map_iterate_without_index(radius_sq, city_x, city_y) {
        struct tile *ptile = city_map_to_tile(city_tile(pcity), radius_sq,
                                              city_x, city_y);

        if (ptile && !is_free_worked(pcity, ptile)
            && tile_worked(ptile) != pcity
            && city_can_work_tile(pcity, ptile)) {
          city_map_update_worker(pcity, ptile);
          workers--;
        }

        if (workers <= 0) {
          break;
        }
      } city_map_iterate_without_index_end;
    }

    /* if there are still workers they will be updated to specialists */
    if (workers > 0) {
      pcity->specialists[DEFAULT_SPECIALIST] += workers;
    }

    city_refresh_vision(pcity);
  }

  /* if city is under AI control update it */
  adv_city_update(pcity);

  notify_player(city_owner(pcity), city_tile(pcity), E_CITY_RADIUS_SQ,
                ftc_server, _("The size of the city map of %s is %s."),
                city_name_get(pcity),
                city_tiles_old < city_tiles_new ? _("increased")
                                                : _("reduced"));

  /* workers map after */
  log_debug("[%s (%d)] city size: %d; specialists: %d (after change)",
            city_name_get(pcity), pcity->id, city_size_get(pcity),
            city_specialists(pcity));
  citylog_map_workers(LOG_DEBUG, pcity);

  return TRUE;
}

/************************************************************************//**
  Clear worker task from the city and inform owner
****************************************************************************/
void clear_worker_task(struct city *pcity, struct worker_task *ptask)
{
  struct packet_worker_task packet;

  if (ptask == NULL) {
    return;
  }

  worker_task_list_remove(pcity->task_reqs, ptask);

  packet.city_id = pcity->id;
  packet.tile_id = tile_index(ptask->ptile);
  packet.activity = ACTIVITY_LAST;
  packet.tgt = 0;
  packet.want = 0;

  free(ptask);

  lsend_packet_worker_task(city_owner(pcity)->connections, &packet);
  lsend_packet_worker_task(game.glob_observers, &packet);
}

/************************************************************************//**
  Clear all worker tasks from the city and inform owner
****************************************************************************/
void clear_worker_tasks(struct city *pcity)
{
  while (worker_task_list_size(pcity->task_reqs) > 0) {
    clear_worker_task(pcity, worker_task_list_get(pcity->task_reqs, 0));
  }
}

/************************************************************************//**
  Send city worker task to owner
****************************************************************************/
void package_and_send_worker_tasks(struct city *pcity)
{
  struct packet_worker_task packet;

  packet.city_id = pcity->id;

  worker_task_list_iterate(pcity->task_reqs, ptask) {
    packet.tile_id = tile_index(ptask->ptile);
    packet.activity = ptask->act;
    if (ptask->tgt == NULL) {
      packet.tgt = -1;
    } else {
      packet.tgt = extra_number(ptask->tgt);
    }
    packet.want = ptask->want;

    lsend_packet_worker_task(city_owner(pcity)->connections, &packet);
    lsend_packet_worker_task(game.glob_observers, &packet);
  } worker_task_list_iterate_end;
}

/************************************************************************//**
  Return the cost (gold) to buy the current city production.
****************************************************************************/
int city_production_buy_gold_cost(const struct city *pcity)
{
  int build = pcity->shield_stock;

  switch (pcity->production.kind) {
  case VUT_IMPROVEMENT:
    return impr_buy_gold_cost(pcity, pcity->production.value.building,
                              build);
  case VUT_UTYPE:
    return utype_buy_gold_cost(pcity, pcity->production.value.utype,
                               build);
  default:
    break;
  };

  return FC_INFINITY;
}
