/*****************************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "rand.h"

/* common */
#include "map.h"
#include "movement.h"
#include "research.h"
#include "unittype.h"

/* common/scriptcore */
#include "api_game_find.h"
#include "luascript.h"

/* server */
#include "aiiface.h"
#include "barbarian.h"
#include "citytools.h"
#include "console.h" /* enum rfc_status */
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "srv_main.h" /* game_was_started() */
#include "stdinhand.h"
#include "techtools.h"
#include "unittools.h"

/* server/scripting */
#include "script_server.h"

/* server/generator */
#include "mapgen_utils.h"

#include "api_server_edit.h"


/*************************************************************************//**
  Unleash barbarians on a tile, for example from a hut
*****************************************************************************/
bool api_edit_unleash_barbarians(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 2, Tile, FALSE);

  return unleash_barbarians(ptile);
}

/*************************************************************************//**
  Place partisans for a player around a tile (normally around a city).
*****************************************************************************/
void api_edit_place_partisans(lua_State *L, Tile *ptile, Player *pplayer,
                              int count, int sq_radius)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 2, Tile);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 3, Player);
  LUASCRIPT_CHECK_ARG(L, 0 <= sq_radius, 5, "radius must be positive");
  LUASCRIPT_CHECK(L, 0 < num_role_units(L_PARTISAN),
                  "no partisans in ruleset");

  return place_partisans(ptile, pplayer, count, sq_radius);
}

/*************************************************************************//**
  Create a new unit.
*****************************************************************************/
Unit *api_edit_create_unit(lua_State *L, Player *pplayer, Tile *ptile,
                           Unit_Type *ptype, int veteran_level,
                           City *homecity, int moves_left)
{
  return api_edit_create_unit_full(L, pplayer, ptile, ptype, veteran_level,
                                      homecity, moves_left, -1, NULL);
}

/*************************************************************************//**
  Create a new unit.
*****************************************************************************/
Unit *api_edit_create_unit_full(lua_State *L, Player *pplayer, Tile *ptile,
                                Unit_Type *ptype, int veteran_level,
                                City *homecity, int moves_left, int hp_left,
                                Unit *ptransport)
{
  struct fc_lua *fcl;
  struct city *pcity;

  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 2, Player, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 3, Tile, NULL);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != NULL, "Undefined Freeciv lua state!", NULL);

  if (ptype == NULL
      || ptype < unit_type_array_first() || ptype > unit_type_array_last()) {
    return NULL;
  }

  if (ptransport) {
    /* Extensive check to see if transport and unit are compatible */
    int ret;
    struct unit *pvirt = unit_virtual_create(pplayer, NULL, ptype,
                                             veteran_level);
    unit_tile_set(pvirt, ptile);
    pvirt->homecity = homecity ? homecity->id : 0;
    ret = can_unit_load(pvirt, ptransport);
    unit_virtual_destroy(pvirt);
    if (!ret) {
      luascript_log(fcl, LOG_ERROR, "create_unit_full: '%s' cannot transport "
                                    "'%s' here",
                    utype_rule_name(unit_type_get(ptransport)),
                    utype_rule_name(ptype));
      return NULL;
    }
  } else if (!can_exist_at_tile(&(wld.map), ptype, ptile)) {
    luascript_log(fcl, LOG_ERROR, "create_unit_full: '%s' cannot exist at "
                                  "tile", utype_rule_name(ptype));
    return NULL;
  }

  if (is_non_allied_unit_tile(ptile, pplayer)) {
    luascript_log(fcl, LOG_ERROR, "create_unit_full: tile is occupied by "
                                  "enemy unit");
    return NULL;
  }

  pcity = tile_city(ptile);
  if (pcity != NULL && !pplayers_allied(pplayer, city_owner(pcity))) {
    luascript_log(fcl, LOG_ERROR, "create_unit_full: tile is occupied by "
                                  "enemy city");
    return NULL;
  }

  return create_unit_full(pplayer, ptile, ptype, veteran_level,
                          homecity ? homecity->id : 0, moves_left,
                          hp_left, ptransport);
}

/*************************************************************************//**
  Teleport unit to destination tile
*****************************************************************************/
bool api_edit_unit_teleport(lua_State *L, Unit *punit, Tile *dest)
{
  bool alive;
  struct city *pcity;

  LUASCRIPT_CHECK_STATE(L, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, punit, 2, Unit, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, dest, 3, Tile, FALSE);

  /* Teleport first so destination is revealed even if unit dies */
  alive = unit_move(punit, dest, 0, NULL,
                    /* The old call would result in occupation before the
                     * checks below. */
                    ((pcity = tile_city(dest))
                     && (unit_owner(punit)->ai_common.barbarian_type
                         != ANIMAL_BARBARIAN)
                     && uclass_has_flag(unit_class_get(punit),
                                        UCF_CAN_OCCUPY_CITY)
                     && !unit_has_type_flag(punit, UTYF_CIVILIAN)
                     && pplayers_at_war(unit_owner(punit),
                                        city_owner(pcity))));
  if (alive) {
    struct player *owner = unit_owner(punit);

    if (!can_unit_exist_at_tile(&(wld.map), punit, dest)) {
      wipe_unit(punit, ULR_NONNATIVE_TERR, NULL);
      return FALSE;
    }
    if (is_non_allied_unit_tile(dest, owner)
        || (pcity && !pplayers_allied(city_owner(pcity), owner))) {
      wipe_unit(punit, ULR_STACK_CONFLICT, NULL);
      return FALSE;
    }
  }

  return alive;
}

/*************************************************************************//**
  Change unit orientation
*****************************************************************************/
void api_edit_unit_turn(lua_State *L, Unit *punit, Direction dir)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, punit, 2, Unit);
 
  if (direction8_is_valid(dir)) {
    punit->facing = dir;

    send_unit_info(NULL, punit);
  } else {
    log_error("Illegal direction %d for unit from lua script", dir);
  }
}

/*************************************************************************//**
  Kill the unit.
*****************************************************************************/
void api_edit_unit_kill(lua_State *L, Unit *punit, const char *reason,
                        Player *killer)
{
  enum unit_loss_reason loss_reason;

  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, punit, 2, Unit);
  LUASCRIPT_CHECK_ARG_NIL(L, reason, 3, string);

  loss_reason = unit_loss_reason_by_name(reason, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, unit_loss_reason_is_valid(loss_reason), 3,
                      "Invalid unit loss reason");

  wipe_unit(punit, loss_reason, killer);
}

/*************************************************************************//**
  Change terrain on tile
*****************************************************************************/
bool api_edit_change_terrain(lua_State *L, Tile *ptile, Terrain *pterr)
{
  struct terrain *old_terrain;

  LUASCRIPT_CHECK_STATE(L, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 2, Tile, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, pterr, 3, Terrain, FALSE);

  old_terrain = tile_terrain(ptile);

  if (old_terrain == pterr
      || (terrain_has_flag(pterr, TER_NO_CITIES) && tile_city(ptile) != NULL)) {
    return FALSE;
  }
  
  tile_change_terrain(ptile, pterr);
  fix_tile_on_terrain_change(ptile, old_terrain, FALSE);
  if (need_to_reassign_continents(old_terrain, pterr)) {
    assign_continent_numbers();
    send_all_known_tiles(NULL);
  }

  update_tile_knowledge(ptile);

  return TRUE;
}

/*************************************************************************//**
  Create a new city.
*****************************************************************************/
void api_edit_create_city(lua_State *L, Player *pplayer, Tile *ptile,
                          const char *name)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 2, Player);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 3, Tile);

  if (!name || name[0] == '\0') {
    name = city_name_suggestion(pplayer, ptile);
  }

  /* TODO: Allow initial citizen to be of nationality other than owner */
  create_city(pplayer, ptile, name, pplayer);
}

/*************************************************************************//**
  Create a new player.
*****************************************************************************/
Player *api_edit_create_player(lua_State *L, const char *username,
                               Nation_Type *pnation, const char *ai)
{
  struct player *pplayer = NULL;
  char buf[128] = "";
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, username, 2, string, NULL);
  if (!ai) {
    ai = default_ai_type_name();
  }

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != NULL, "Undefined Freeciv lua state!", NULL);

  if (game_was_started()) {
    create_command_newcomer(username, ai, FALSE, pnation, &pplayer,
                            buf, sizeof(buf));
  } else {
    create_command_pregame(username, ai, FALSE, &pplayer,
                           buf, sizeof(buf));
  }

  if (strlen(buf) > 0) {
    luascript_log(fcl, LOG_NORMAL, "%s", buf);
  }

  return pplayer;
}

/*************************************************************************//**
  Change pplayer's gold by amount.
*****************************************************************************/
void api_edit_change_gold(lua_State *L, Player *pplayer, int amount)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 2, Player);

  pplayer->economic.gold = MAX(0, pplayer->economic.gold + amount);
}

/*************************************************************************//**
  Give pplayer technology ptech. Quietly returns NULL if
  player already has this tech; otherwise returns the tech granted.
  Use NULL for ptech to grant a random tech.
  sends script signal "tech_researched" with the given reason
*****************************************************************************/
Tech_Type *api_edit_give_technology(lua_State *L, Player *pplayer,
                                    Tech_Type *ptech, int cost, bool notify,
                                    const char *reason)
{
  struct research *presearch;
  Tech_type_id id;
  Tech_Type *result;

  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 2, Player, NULL);
  LUASCRIPT_CHECK_ARG(L, cost >= -3, 4, "Unknown give_tech() cost value", NULL);

  presearch = research_get(pplayer);
  if (ptech) {
    id = advance_number(ptech);
  } else {
    id = pick_free_tech(presearch);
  }

  if (is_future_tech(id)
      || research_invention_state(presearch, id) != TECH_KNOWN) {
    if (cost < 0) {
      if (cost == -1) {
        cost = game.server.freecost;
      } else if (cost == -2) {
        cost = game.server.conquercost;
      } else if (cost == -3) {
        cost = game.server.diplbulbcost;
      } else {
        
        cost = 0;
      }
    }
    research_apply_penalty(presearch, id, cost);
    found_new_tech(presearch, id, FALSE, TRUE);
    result = advance_by_number(id);
    script_tech_learned(presearch, pplayer, result, reason);

    if (notify && result != NULL) {
      const char *adv_name = research_advance_name_translation(presearch, id);
      char research_name[MAX_LEN_NAME * 2];

      research_pretty_name(presearch, research_name, sizeof(research_name));

      notify_player(pplayer, NULL, E_TECH_GAIN, ftc_server,
                    Q_("?fromscript:You acquire %s."), adv_name);
      notify_research(presearch, pplayer, E_TECH_GAIN, ftc_server,
                      /* TRANS: "The Greeks ..." or "The members of
                       * team Red ..." */
                      Q_("?fromscript:The %s acquire %s and share this "
                         "advance with you."),
                      nation_plural_for_player(pplayer), adv_name);
      notify_research_embassies(presearch, NULL, E_TECH_EMBASSY, ftc_server,
                                /* TRANS: "The Greeks ..." or "The members of
                                 * team Red ..." */
                                Q_("?fromscript:The %s acquire %s."),
                                research_name, adv_name);
    }

    return result;
  } else {
    return NULL;
  }
}

/*************************************************************************//**
  Modify player's trait value.
*****************************************************************************/
bool api_edit_trait_mod_set(lua_State *L, Player *pplayer,
                            const char *tname, const int mod)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 2, Player, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, FALSE);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  pplayer->ai_common.traits[tr].mod += mod;

  return TRUE;
}

/*************************************************************************//**
  Create a new owned extra.
*****************************************************************************/
void api_edit_create_owned_extra(lua_State *L, Tile *ptile, const char *name,
                                 Player *pplayer)
{
  struct extra_type *pextra;

  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 2, Tile);

  if (!name) {
    return;
  }

  pextra = extra_type_by_rule_name(name);

  if (pextra) {
    create_extra(ptile, pextra, pplayer);
    update_tile_knowledge(ptile);
  }
}

/*************************************************************************//**
  Create a new extra.
*****************************************************************************/
void api_edit_create_extra(lua_State *L, Tile *ptile, const char *name)
{
  api_edit_create_owned_extra(L, ptile, name, NULL);
}

/*************************************************************************//**
  Create a new base.
*****************************************************************************/
void api_edit_create_base(lua_State *L, Tile *ptile, const char *name,
                          Player *pplayer)
{
  api_edit_create_owned_extra(L, ptile, name, pplayer);
}

/*************************************************************************//**
  Add a new road.
*****************************************************************************/
void api_edit_create_road(lua_State *L, Tile *ptile, const char *name)
{
  api_edit_create_owned_extra(L, ptile, name, NULL);
}

/*************************************************************************//**
  Remove extra from tile, if present
*****************************************************************************/
void api_edit_remove_extra(lua_State *L, Tile *ptile, const char *name)
{
  struct extra_type *pextra;

  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 2, Tile);

  if (!name) {
    return;
  }

  pextra = extra_type_by_rule_name(name);

  if (pextra != NULL && tile_has_extra(ptile, pextra)) {
    tile_extra_rm_apply(ptile, pextra);
    update_tile_knowledge(ptile);
  }
}

/*************************************************************************//**
  Set tile label text.
*****************************************************************************/
void api_edit_tile_set_label(lua_State *L, Tile *ptile, const char *label)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_SELF(L, ptile);
  LUASCRIPT_CHECK_ARG_NIL(L, label, 3, string);

  tile_set_label(ptile, label);
  if (server_state() >= S_S_RUNNING) {
    send_tile_info(NULL, ptile, FALSE);
  }
}

/*************************************************************************//**
  Global climate change.
*****************************************************************************/
void api_edit_climate_change(lua_State *L, enum climate_change_type type,
                             int effect)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG(L, type == CLIMATE_CHANGE_GLOBAL_WARMING
                      || type == CLIMATE_CHANGE_NUCLEAR_WINTER,
                      2, "invalid climate change type");
  LUASCRIPT_CHECK_ARG(L, effect > 0, 3, "effect must be greater than zero");

  climate_change(type == CLIMATE_CHANGE_GLOBAL_WARMING, effect);
}

/*************************************************************************//**
  Provoke a civil war.
*****************************************************************************/
Player *api_edit_civil_war(lua_State *L, Player *pplayer, int probability)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, pplayer, 2, Player, NULL);
  LUASCRIPT_CHECK_ARG(L, probability >= 0 && probability <= 100,
                      3, "must be a percentage", NULL);

  if (!civil_war_possible(pplayer, FALSE, FALSE)) {
    return NULL;
  }

  if (probability == 0) {
    /* Calculate chance with normal rules */
    if (!civil_war_triggered(pplayer)) {
      return NULL;
    }
  } else {
    /* Fixed chance specified by script */
    if (fc_rand(100) >= probability) {
      return NULL;
    }
  }

  return civil_war(pplayer);
}

/*************************************************************************//**
  Make player winner of the scenario
*****************************************************************************/
void api_edit_player_victory(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_SELF(L, pplayer);

  player_status_add(pplayer, PSTATUS_WINNER);
}

/*************************************************************************//**
  Move a unit.
*****************************************************************************/
bool api_edit_unit_move(lua_State *L, Unit *punit, Tile *ptile,
                        int movecost)
{
  struct city *pcity;

  LUASCRIPT_CHECK_STATE(L, FALSE);
  LUASCRIPT_CHECK_SELF(L, punit, FALSE);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 3, Tile, FALSE);
  LUASCRIPT_CHECK_ARG(L, movecost >= 0, 4, "Negative move cost!", FALSE);

  return unit_move(punit, ptile, movecost, NULL,
                   /* Backwards compatibility for old scripts in rulesets
                    * and (scenario) savegames. */
                   ((pcity = tile_city(ptile))
                    && (unit_owner(punit)->ai_common.barbarian_type
                        != ANIMAL_BARBARIAN)
                    && uclass_has_flag(unit_class_get(punit),
                                       UCF_CAN_OCCUPY_CITY)
                    && !unit_has_type_flag(punit, UTYF_CIVILIAN)
                    && pplayers_at_war(unit_owner(punit),
                                       city_owner(pcity))));
}

/*************************************************************************//**
  Prohibit unit from moving
*****************************************************************************/
void api_edit_unit_moving_disallow(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_SELF(L, punit);

  if (punit != NULL) {
    punit->stay = TRUE;
  }
}

/*************************************************************************//**
  Allow unit to move
*****************************************************************************/
void api_edit_unit_moving_allow(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_SELF(L, punit);

  if (punit != NULL) {
    punit->stay = FALSE;
  }
}

/*************************************************************************//**
  Add history to a city
*****************************************************************************/
void api_edit_city_add_history(lua_State *L, City *pcity, int amount)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_SELF(L, pcity);

  pcity->history += amount;
}

/*************************************************************************//**
  Add history to a player
*****************************************************************************/
void api_edit_player_add_history(lua_State *L, Player *pplayer, int amount)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_SELF(L, pplayer);

  pplayer->culture += amount;
}
