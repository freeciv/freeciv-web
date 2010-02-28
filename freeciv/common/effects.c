/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Team
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
#include <ctype.h>
#include <string.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"
#include "shared.h" /* ARRAY_SIZE */

#include "city.h"
#include "effects.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "tech.h"
 
/* Names of effect types.
 * (These must correspond to enum effect_type_id in effects.h.) */
static const char *effect_type_names[EFT_LAST] = {
  "Tech_Parasite",
  "Airlift",
  "Any_Government",
  "Capital_City",
  "Enable_Nuke",
  "Enable_Space",
  "Specialist_Output",
  "Output_Bonus",
  "Output_Bonus_2",
  "Output_Add_Tile",
  "Output_Inc_Tile",
  "Output_Per_Tile",
  "Output_Waste_Pct",
  "Force_Content",
  /* TODO: "Force_Content_Pct", */
  "Give_Imm_Tech",
  "Growth_Food",
  "Health_Pct",
  "Have_Embassies",
  "Make_Content",
  "Make_Content_Mil",
  "Make_Content_Mil_Per",
  /* TODO: "Make_Content_Pct", */
  "Make_Happy",
  "No_Anarchy",
  "Nuke_Proof",
  /* TODO: "Pollu_Adj", */
  /* TODO: "Pollu_Pct", */
  /* TODO: "Pollu_Pop_Adj", */
  "Pollu_Pop_Pct",
  /* TODO: "Pollu_Prod_Adj", */
  "Pollu_Prod_Pct",
  "Reveal_Cities",
  "Reveal_Map",
  /* TODO: "Incite_Dist_Adj", */
  "Incite_Cost_Pct",
  "Size_Adj",
  "Size_Unlimit",
  "SS_Structural",
  "SS_Component",
  "SS_Module",
  "Spy_Resistant",
  "Move_Bonus",
  "Unit_No_Lose_Pop",
  "Unit_Recover",
  "Upgrade_Unit",
  "Upkeep_Free",
  "No_Unhappy",
  "Veteran_Build",
  "Veteran_Combat",
  "HP_Regen",
  "City_Vision_Radius_Sq",
  "Unit_Vision_Radius_Sq",
  "Defend_Bonus",
  "No_Incite",
  "Gain_AI_Love",
  "Turn_Years",
  "Slow_Down_Timeline",
  "Civil_War_Chance",
  "Migration_Pct",
  "Empire_Size_Base",
  "Empire_Size_Step",
  "Max_Rates",
  "Martial_Law_Each",
  "Martial_Law_Max",
  "Rapture_Grow",
  "Unbribable_Units",
  "Revolution_When_Unhappy",
  "Has_Senate",
  "Inspire_Partisans",
  "Happiness_To_Gold",
  "Fanatics",
  "No_Diplomacy",
  "Trade_Revenue_Bonus",
  "Unhappy_Factor",
  "Upkeep_Factor",
  "Unit_Upkeep_Free_Per_City",
  "Output_Waste",
  "Output_Waste_By_Distance",
  "Output_Penalty_Tile",
  "Output_Inc_Tile_Celebrate",
  "City_Unhappy_Size",
  "Upgrade_Price_Pct",
  "Visible_Walls",
  "Tech_Cost_Factor",
  "Shield2Gold_Factor",
  "Tile_Workable"
};

static bool initialized = FALSE;

/**************************************************************************
  Convert effect type names to enum; case insensitive;
  returns EFT_LAST if can't match.
**************************************************************************/
enum effect_type effect_type_from_str(const char *str)
{
  enum effect_type effect_type;

  assert(ARRAY_SIZE(effect_type_names) == EFT_LAST);

  for (effect_type = 0; effect_type < EFT_LAST; effect_type++) {
    if (0 == mystrcasecmp(effect_type_names[effect_type], str)) {
      return effect_type;
    }
  }

  return EFT_LAST;
}

/**************************************************************************
  Return the name for an effect type enumeration.  Returns NULL if the
  effect_type is invalid.  This is the inverse of effect_type_from_str.
**************************************************************************/
const char *effect_type_name(enum effect_type effect_type)
{
  assert(ARRAY_SIZE(effect_type_names) == EFT_LAST);
  if (effect_type >= 0 && effect_type < EFT_LAST) {
    return effect_type_names[effect_type];
  } else {
    assert(0);
    return NULL;
  }
}

/**************************************************************************
  The code creates a ruleset cache on ruleset load. This constant cache
  is used to speed up effects queries.  After the cache is created it is
  not modified again (though it may later be freed).

  Since the cache is constant, the server only needs to send effects data to
  the client upon connect. It also means that an AI can do fast searches in
  the effects space by trying the possible combinations of addition or
  removal of buildings with the effects it cares about.


  To know how much a target is being affected, simply use the convenience
  functions:

  * get_player_bonus
  * get_city_bonus
  * get_city_tile_bonus
  * get_building_bonus

  These functions require as arguments the target and the effect type to be
  queried.

  Effect sources are unique and at a well known place in the
  data structures.  This allows the above queries to be fast:
    - Look up the possible sources for the effect (O(1) lookup)
    - For each source, find out if it is present (O(1) lookup per source).
  The first is commonly called the "ruleset cache" and is stored statically
  in this file.  The second is the "sources cache" and is stored all over.

  Any type of effect range and "survives" is possible if we have a sources
  cache for that combination.  For instance
    - There is a sources cache of all existing buildings in a city; thus any
      building effect in a city can have city range.
    - There is a sources cache of all wonders in the world; thus any wonder
      effect can have world range.
    - There is a sources cache of all wonders for each player; thus any
      wonder effect can have player range.
    - There is a sources cache of all wonders ever built; thus any wonder
      effect that survives can have world range.
  However there is no sources cache for many of the possible sources.  For
  instance non-unique buildings do not have a world-range sources cahce, so
  you can't have a non-wonder building have a world-ranged effect.

  The sources caches could easily be extended by generalizing it to a set
  of arrays
    game.buildings[], pplayer->buildings[],
    pisland->builidngs[], pcity->buildings[]
  which would store the number of buildings of that type present by game,
  player, island (continent) or city.  This would allow non-surviving effects
  to come from any building at any range.  However to allow surviving effects
  a second set of arrays would be needed.  This should enable basic support
  for small wonders and satellites.

  No matter which sources caches are present, we should always know where
  to look for a source and so the lookups will always be fast even as the
  number of possible sources increases.
**************************************************************************/

/**************************************************************************
  Ruleset cache. The cache is created during ruleset loading and the data
  is organized to enable fast queries.
**************************************************************************/
static struct {
  /* A single list containing every effect. */
  struct effect_list *tracker;

  /* This array provides a full list of the effects of this type
   * (It's not really a cache, it's the real data.) */
  struct effect_list *effects[EFT_LAST];

  struct {
    /* This cache shows for each building, which effects it provides. */
    struct effect_list *buildings[B_LAST];
    struct effect_list *govs[G_MAGIC];
  } reqs;
} ruleset_cache;


/**************************************************************************
  Get a list of effects of this type.
**************************************************************************/
static struct effect_list *get_effects(enum effect_type effect_type)
{
  return ruleset_cache.effects[effect_type];
}

/**************************************************************************
  Get a list of effects with this requirement source.

  Note: currently only buildings and governments are supported.
**************************************************************************/
struct effect_list *get_req_source_effects(struct universal *psource)
{
  int type, value;

  universal_extraction(psource, &type, &value);

  switch (type) {
  case VUT_GOVERNMENT:
    if (value >= 0 && value < government_count()) {
      return ruleset_cache.reqs.govs[value];
    } else {
      return NULL;
    }
  case VUT_IMPROVEMENT:
    if (value >= 0 && value < improvement_count()) {
      return ruleset_cache.reqs.buildings[value];
    } else {
      return NULL;
    }
  default:
    return NULL;
  }
}

/**************************************************************************
  Add effect to ruleset cache.
**************************************************************************/
struct effect *effect_new(enum effect_type type, int value)
{
  struct effect *peffect;

  /* Create the effect. */
  peffect = fc_malloc(sizeof(*peffect));
  peffect->type = type;
  peffect->value = value;

  peffect->reqs = requirement_list_new();
  peffect->nreqs = requirement_list_new();

  /* Now add the effect to the ruleset cache. */
  effect_list_append(ruleset_cache.tracker, peffect);
  effect_list_append(get_effects(type), peffect);
  return peffect;
}

/**************************************************************************
  Free effect.
**************************************************************************/
static void effect_free(struct effect *peffect)
{
  requirement_list_iterate(peffect->reqs, preq) {
    free(preq);
  } requirement_list_iterate_end;
  requirement_list_free(peffect->reqs);

  requirement_list_iterate(peffect->nreqs, preq) {
    free(preq);
  } requirement_list_iterate_end;
  requirement_list_free(peffect->nreqs);

  free(peffect);
}

/**************************************************************************
  Append requirement to effect.
**************************************************************************/
void effect_req_append(struct effect *peffect, bool neg,
		       struct requirement *preq)
{
  struct requirement_list *req_list;

  if (neg) {
    req_list = peffect->nreqs;
  } else {
    req_list = peffect->reqs;
  }

  /* Append requirement to the effect. */
  requirement_list_append(req_list, preq);

  /* Add effect to the source's effect list. */
  if (!neg) {
    struct effect_list *eff_list = get_req_source_effects(&preq->source);

    if (eff_list) {
      effect_list_append(eff_list, peffect);
    }
  }
}

/**************************************************************************
  Initialize the ruleset cache.  The ruleset cache should be empty
  before this is done (so if it's previously been initialized, it needs
  to be freed (see ruleset_cache_free) before it can be reused).
**************************************************************************/
void ruleset_cache_init(void)
{
  int i;

  initialized = TRUE;

  ruleset_cache.tracker = effect_list_new();

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.effects); i++) {
    ruleset_cache.effects[i] = effect_list_new();
  }
  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.buildings); i++) {
    ruleset_cache.reqs.buildings[i] = effect_list_new();
  }
  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.govs); i++) {
    ruleset_cache.reqs.govs[i] = effect_list_new();
  }
}

/**************************************************************************
  Free the ruleset cache.  This should be called at the end of the game or
  when the client disconnects from the server.  See ruleset_cache_init.
**************************************************************************/
void ruleset_cache_free(void)
{
  int i;
  struct effect_list *plist = ruleset_cache.tracker;

  if (plist) {
    effect_list_iterate(plist, peffect) {
      effect_free(peffect);
    } effect_list_iterate_end;
    effect_list_free(plist);
    ruleset_cache.tracker = NULL;
  }

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.effects); i++) {
    struct effect_list *plist = ruleset_cache.effects[i];

    if (plist) {
      effect_list_free(plist);
      ruleset_cache.effects[i] = NULL;
    }
  }

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.buildings); i++) {
    struct effect_list *plist = ruleset_cache.reqs.buildings[i];

    if (plist) {
      effect_list_free(plist);
      ruleset_cache.reqs.buildings[i] = NULL;
    }
  }

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.govs); i++) {
    struct effect_list *plist = ruleset_cache.reqs.govs[i];

    if (plist) {
      effect_list_free(plist);
      ruleset_cache.reqs.govs[i] = NULL;
    }
  }

  initialized = FALSE;
}

/**************************************************************************
  Receives a new effect.  This is called by the client when the packet
  arrives.
**************************************************************************/
void recv_ruleset_effect(struct packet_ruleset_effect *packet)
{
  effect_new(packet->effect_type, packet->effect_value);
}

/**************************************************************************
  Receives a new effect *requirement*.  This is called by the client when
  the packet arrives.
**************************************************************************/
void recv_ruleset_effect_req(struct packet_ruleset_effect_req *packet)
{
  if (packet->effect_id != effect_list_size(ruleset_cache.tracker) - 1) {
    freelog(LOG_ERROR, "Bug in recv_ruleset_effect_req.");
  } else {
    struct effect *peffect = effect_list_get(ruleset_cache.tracker, -1);
    struct requirement req, *preq;

    req = req_from_values(packet->source_type, packet->range, packet->survives,
	packet->negated, packet->source_value);

    preq = fc_malloc(sizeof(*preq));
    *preq = req;

    effect_req_append(peffect, packet->neg, preq);
  }
}

/**************************************************************************
  Send the ruleset cache data over the network.
**************************************************************************/
void send_ruleset_cache(struct conn_list *dest)
{
  unsigned id = 0;

  effect_list_iterate(ruleset_cache.tracker, peffect) {
    struct packet_ruleset_effect effect_packet;

    effect_packet.effect_type = peffect->type;
    effect_packet.effect_value = peffect->value;

    lsend_packet_ruleset_effect(dest, &effect_packet);

    requirement_list_iterate(peffect->reqs, preq) {
      struct packet_ruleset_effect_req packet;
      int type, range, value;
      bool survives, negated;

      req_get_values(preq, &type, &range, &survives, &negated, &value);
      packet.effect_id = id;
      packet.neg = FALSE;
      packet.source_type = type;
      packet.source_value = value;
      packet.range = range;
      packet.survives = survives;
      packet.negated = FALSE;

      lsend_packet_ruleset_effect_req(dest, &packet);
    } requirement_list_iterate_end;

    requirement_list_iterate(peffect->nreqs, preq) {
      struct packet_ruleset_effect_req packet;
      int type, range, value;
      bool survives, negated;

      req_get_values(preq, &type, &range, &survives, &negated, &value);
      packet.effect_id = id;
      packet.neg = TRUE;
      packet.source_type = type;
      packet.source_value = value;
      packet.range = range;
      packet.survives = survives;
      packet.negated = FALSE;

      lsend_packet_ruleset_effect_req(dest, &packet);
    } requirement_list_iterate_end;

    id++;
  } effect_list_iterate_end;
}

/**************************************************************************
  Returns a buildable, non-obsolete building that can provide the effect.

  Note: this function is an inefficient hack to be used by the old AI.  It
  will never find wonders, since that's not what the AI wants.
**************************************************************************/
Impr_type_id ai_find_source_building(struct city *pcity,
				     enum effect_type effect_type,
                                     struct unit_class *uclass,
                                     enum unit_move_type move)
{
  int greatest_value = 0;
  struct impr_type *best_building = NULL;

  /* There's no point in defining both of these as uclass is more restrictive
   * than move_type */
  assert(uclass == NULL || move == MOVETYPE_LAST);

  effect_list_iterate(get_effects(effect_type), peffect) {
    if (peffect->value > greatest_value) {
      struct impr_type *building = NULL;
      bool wrong_unit = FALSE;

      requirement_list_iterate(peffect->reqs, preq) {
        if (VUT_IMPROVEMENT == preq->source.kind) {
          building = preq->source.value.building;

          if (!can_city_build_improvement_now(pcity, building)
              || !is_improvement(building)) {          
            building = NULL;
            break;
          }
        }
        if (VUT_UCLASS == preq->source.kind) {
          if ((uclass != NULL && preq->source.value.uclass != uclass)
              || (move != MOVETYPE_LAST
                  && uclass_move_type(preq->source.value.uclass) != move)) {
            /* Effect requires other kind of unit than what we are interested about */
            wrong_unit = TRUE;
            break;
          }
        }
      } requirement_list_iterate_end;
      if (!wrong_unit && building != NULL) {
        best_building = building;
	greatest_value = peffect->value;
      }
    }
  } effect_list_iterate_end;

  if (best_building) {
    return improvement_number(best_building);
  }
  return B_LAST;
}

/**************************************************************************
  Returns TRUE if the building has any effect bonuses of the given type.

  Note that this function returns a boolean rather than an integer value
  giving the exact bonus.  Finding the exact bonus requires knowing the
  effect range and may take longer.  This function should only be used
  in situations where the range doesn't matter.
**************************************************************************/
bool building_has_effect(const struct impr_type *pimprove,
			 enum effect_type effect_type)
{
  struct universal source = {
    .kind = VUT_IMPROVEMENT,
    /* just to bamboozle the annoying compiler warning */
    .value = {.building = improvement_by_number(improvement_number(pimprove))}
  };
  struct effect_list *plist = get_req_source_effects(&source);

  if (!plist) {
    return FALSE;
  }

  effect_list_iterate(plist, peffect) {
    if (peffect->type == effect_type) {
      return TRUE;
    }
  } effect_list_iterate_end;
  return FALSE;
}

/**************************************************************************
  Return TRUE iff any of the disabling requirements for this effect are
  active (an effect is active if all of its enabling requirements and
  none of its disabling ones are active).
**************************************************************************/
bool is_effect_disabled(const struct player *target_player,
		        const struct city *target_city,
		        const struct impr_type *target_building,
		        const struct tile *target_tile,
			const struct unit_type *target_unittype,
			const struct output_type *target_output,
			const struct specialist *target_specialist,
		        const struct effect *peffect,
                        const enum   req_problem_type prob_type)
{
  requirement_list_iterate(peffect->nreqs, preq) {
    if (is_req_active(target_player, target_city, target_building,
		      target_tile, target_unittype, target_output,
		      target_specialist,
		      preq, prob_type)) {
      return TRUE;
    }
  } requirement_list_iterate_end;
  return FALSE;
}

/**************************************************************************
  Return TRUE iff all of the enabling requirements for this effect are
  active (an effect is active if all of its enabling requirements and
  none of its disabling ones are active).
**************************************************************************/
static bool is_effect_enabled(const struct player *target_player,
			      const struct city *target_city,
			      const struct impr_type *target_building,
			      const struct tile *target_tile,
			      const struct unit_type *target_unittype,
			      const struct output_type *target_output,
			      const struct specialist *target_specialist,
			      const struct effect *peffect,
                              const enum   req_problem_type prob_type)
{
  requirement_list_iterate(peffect->reqs, preq) {
    if (!is_req_active(target_player, target_city, target_building,
		       target_tile, target_unittype, target_output,
		       target_specialist,
		       preq, prob_type)) {
      return FALSE;
    }
  } requirement_list_iterate_end;
  return TRUE;
}

/**************************************************************************
  Is the effect active at a certain target (player, city or building)?

  This checks whether an effect's requirements are met.

  target gives the type of the target
  (player,city,building,tile) give the exact target
  peffect gives the exact effect value
**************************************************************************/
static bool is_effect_active(const struct player *target_player,
			     const struct city *target_city,
			     const struct impr_type *target_building,
			     const struct tile *target_tile,
			     const struct unit_type *target_unittype,
			     const struct output_type *target_output,
			     const struct specialist *target_specialist,
			     const struct effect *peffect,
                             const enum   req_problem_type prob_type)
{
  /* Reversed prob_type when checking disabling effects */
  return is_effect_enabled(target_player, target_city, target_building,
			   target_tile, target_unittype, target_output,
			   target_specialist,
			   peffect, prob_type)
    && !is_effect_disabled(target_player, target_city, target_building,
			   target_tile, target_unittype, target_output,
			   target_specialist,
			   peffect, REVERSED_RPT(prob_type));
}

/**************************************************************************
  Can the effect from the source building be active at a certain target
  (player, city or building)?  This doesn't check if the source exists,
  but tells whether the effect from it would be active if the source did
  exist.  It is thus useful to the AI in figuring out what sources to
  try to obtain.

  target gives the type of the target
  (player,city,building,tile) give the exact target
  source gives the source type of the effect
  peffect gives the exact effect value
**************************************************************************/
bool is_effect_useful(const struct player *target_player,
		      const struct city *target_city,
		      const struct impr_type *target_building,
		      const struct tile *target_tile,
		      const struct unit_type *target_unittype,
		      const struct output_type *target_output,
		      const struct specialist *target_specialist,
		      const struct impr_type *source,
		      const struct effect *peffect,
		      const enum   req_problem_type prob_type)
{
  /* Reversed prob_type when checking disabling effects */
  if (is_effect_disabled(target_player, target_city, target_building,
			 target_tile, target_unittype, target_output,
			 target_specialist,
			 peffect, REVERSED_RPT(prob_type))) {
    return FALSE;
  }
  requirement_list_iterate(peffect->reqs, preq) {
    if (VUT_IMPROVEMENT == preq->source.kind
	&& preq->source.value.building == source) {
      continue;
    }
    if (!is_req_active(target_player, target_city, target_building,
		       target_tile, target_unittype, target_output,
		       target_specialist,
		       preq, prob_type)) {
      return FALSE;
    }
  } requirement_list_iterate_end;
  return TRUE;
}

/**************************************************************************
  Returns TRUE if a building is replaced.  To be replaced, all its effects
  must be made redundant by groups that it is in.
  prob_type CERTAIN or POSSIBLE is answer to function name.
**************************************************************************/
bool is_building_replaced(const struct city *pcity,
                          struct impr_type *pimprove,
                          const enum req_problem_type prob_type)
{
  struct effect_list *plist;
  struct universal source = {
    .kind = VUT_IMPROVEMENT,
    .value = {.building = pimprove}
  };

  /* A capitalization production is never redundant. */
  if (improvement_has_flag(pimprove, IF_GOLD)) {
    return FALSE;
  }

  plist = get_req_source_effects(&source);

  /* A building with no effects and no flags is always redundant! */
  if (!plist) {
    return TRUE;
  }

  effect_list_iterate(plist, peffect) {
    /* We use TARGET_BUILDING as the lowest common denominator.  Note that
     * the building is its own target - but whether this is actually
     * checked depends on the range of the effect. */
    /* Prob_type is not reversed here. disabled is equal to replaced, not
     * reverse */
    if (!is_effect_disabled(city_owner(pcity), pcity,
			    pimprove,
			    NULL, NULL, NULL, NULL,
			    peffect, prob_type)) {
      return FALSE;
    }
  } effect_list_iterate_end;
  return TRUE;
}

/**************************************************************************
  Returns the effect bonus of a given type for any target.

  target gives the type of the target
  (player,city,building,tile) give the exact target
  effect_type gives the effect type to be considered

  Returns the effect sources of this type _currently active_.

  The returned vector must be freed (building_vector_free) when the caller
  is done with it.
**************************************************************************/
static int get_target_bonus_effects(struct effect_list *plist,
                                    const struct player *target_player,
                                    const struct city *target_city,
                                    const struct impr_type *target_building,
                                    const struct tile *target_tile,
                                    const struct unit_type *target_unittype,
                                    const struct output_type *target_output,
                                    const struct specialist *target_specialist,
                                    enum effect_type effect_type)
{
  int bonus = 0;

  /* Loop over all effects of this type. */
  effect_list_iterate(get_effects(effect_type), peffect) {
    /* For each effect, see if it is active. */
    if (is_effect_active(target_player, target_city, target_building,
			 target_tile, target_unittype, target_output,
			 target_specialist,
			 peffect, RPT_CERTAIN)) {
      /* And if so add on the value. */
      bonus += peffect->value;

      if (plist) {
	effect_list_append(plist, peffect);
      }
    }
  } effect_list_iterate_end;

  return bonus;
}

/**************************************************************************
  Returns the effect bonus for the whole world.
**************************************************************************/
int get_world_bonus(enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  return get_target_bonus_effects(NULL,
				  NULL, NULL, NULL, NULL, NULL, NULL, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the effect bonus for a player.
**************************************************************************/
int get_player_bonus(const struct player *pplayer,
		     enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  return get_target_bonus_effects(NULL,
				  pplayer, NULL, NULL,
				  NULL, NULL, NULL, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the effect bonus at a city.
**************************************************************************/
int get_city_bonus(const struct city *pcity, enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  return get_target_bonus_effects(NULL,
				  city_owner(pcity), pcity, NULL,
				  NULL, NULL, NULL, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the effect bonus of a specialist in a city.
**************************************************************************/
int get_city_specialist_output_bonus(const struct city *pcity,
				     const struct specialist *pspecialist,
				     const struct output_type *poutput,
				     enum effect_type effect_type)
{
  assert(pcity != NULL);
  assert(pspecialist != NULL);
  assert(poutput != NULL);
  return get_target_bonus_effects(NULL,
				  city_owner(pcity), pcity, NULL,
				  NULL, NULL, poutput, pspecialist,
				  effect_type);
}

/**************************************************************************
  Returns the effect bonus at a city tile.

  FIXME: this is now used both for tile bonuses, tile-output bonuses,
  and city-output bonuses.  Thus ptile or poutput may be NULL for
  certain callers.  This could be changed by adding 2 new functions to
  the interface but they'd be almost identical and their likely names
  would conflict with functions already in city.c.
**************************************************************************/
int get_city_tile_output_bonus(const struct city *pcity,
			       const struct tile *ptile,
			       const struct output_type *poutput,
			       enum effect_type effect_type)
{
  assert(pcity != NULL);
  return get_target_bonus_effects(NULL,
			 	  city_owner(pcity), pcity, NULL,
				  ptile, NULL, poutput, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the player effect bonus of an output.
**************************************************************************/
int get_player_output_bonus(const struct player *pplayer,
                            const struct output_type *poutput,
                            enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  assert(pplayer != NULL);
  assert(poutput != NULL);
  assert(effect_type != EFT_LAST);
  return get_target_bonus_effects(NULL, pplayer, NULL, NULL, NULL,
                                  NULL, poutput, NULL, effect_type);
}

/**************************************************************************
  Returns the player effect bonus of an output.
**************************************************************************/
int get_city_output_bonus(const struct city *pcity,
                          const struct output_type *poutput,
                          enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  assert(pcity != NULL);
  assert(poutput != NULL);
  assert(effect_type != EFT_LAST);
  return get_target_bonus_effects(NULL, city_owner(pcity), pcity, NULL,
                                  NULL, NULL, poutput, NULL, effect_type);
}

/**************************************************************************
  Returns the effect bonus at a building.
**************************************************************************/
int get_building_bonus(const struct city *pcity,
		       const struct impr_type *building,
		       enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  assert(NULL != pcity && NULL != building);
  return get_target_bonus_effects(NULL,
			 	  city_owner(pcity), pcity,
				  building,
				  NULL, NULL, NULL, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the effect bonus that applies at a tile for a given unittype.

  For instance with EFT_DEFEND_BONUS the attacker's unittype and the
  defending tile should be passed in.  Slightly counter-intuitive!
  See doc/README.effects to see how the unittype applies for each effect
  here.
**************************************************************************/
int get_unittype_bonus(const struct player *pplayer,
		       const struct tile *ptile,
		       const struct unit_type *punittype,
		       enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  assert(pplayer != NULL && ptile != NULL && punittype != NULL);
  return get_target_bonus_effects(NULL,
                                 pplayer, tile_city(ptile), NULL, ptile,
                                 punittype, NULL, NULL, effect_type);
}

/**************************************************************************
  Returns the effect bonus at a building.
**************************************************************************/
int get_unit_bonus(const struct unit *punit, enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  assert(punit != NULL);
  return get_target_bonus_effects(NULL,
				  unit_owner(punit),
				  punit->tile ? tile_city(punit->tile) : NULL,
				  NULL, punit->tile,
				  unit_type(punit), NULL, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the effect sources of this type _currently active_ at the player.

  The returned vector must be freed (building_vector_free) when the caller
  is done with it.
**************************************************************************/
int get_player_bonus_effects(struct effect_list *plist,
			     const struct player *pplayer,
			     enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  assert(pplayer != NULL);
  return get_target_bonus_effects(plist,
			  	  pplayer, NULL, NULL,
				  NULL, NULL, NULL, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the effect sources of this type _currently active_ at the city.

  The returned vector must be freed (building_vector_free) when the caller
  is done with it.
**************************************************************************/
int get_city_bonus_effects(struct effect_list *plist,
			   const struct city *pcity,
			   const struct output_type *poutput,
			   enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  assert(pcity != NULL);
  return get_target_bonus_effects(plist,
			 	  city_owner(pcity), pcity, NULL,
				  NULL, NULL, poutput, NULL,
				  effect_type);
}

/**************************************************************************
  Returns the effect bonus the currently-in-construction-item will provide.

  Note this is not called get_current_production_bonus because that would
  be confused with EFT_PROD_BONUS.

  Problem type tells if we need to be CERTAIN about bonus before counting
  it or is POSSIBLE bonus enough.
**************************************************************************/
int get_current_construction_bonus(const struct city *pcity,
				   enum effect_type effect_type,
                                   const enum req_problem_type prob_type)
{
  if (!initialized) {
    return 0;
  }

  if (VUT_IMPROVEMENT == pcity->production.kind) {
    struct impr_type *building = pcity->production.value.building;
    struct universal source = {
      .kind = VUT_IMPROVEMENT,
      .value = {.building = building}
    };
    struct effect_list *plist = get_req_source_effects(&source);
    int power = 0;

    if (plist) {

      effect_list_iterate(plist, peffect) {
	if (peffect->type != effect_type) {
	  continue;
	}
	if (is_effect_useful(city_owner(pcity), pcity, building,
			     NULL, NULL, NULL, NULL,
			     building, peffect, prob_type)) {
	  power += peffect->value;
	}
      } effect_list_iterate_end;

      return power;
    }
  }
  return 0;
}

/**************************************************************************
  Make user-friendly text for the source.  The text is put into a user
  buffer.
**************************************************************************/
void get_effect_req_text(struct effect *peffect, char *buf, size_t buf_len)
{
  buf[0] = '\0';

  requirement_list_iterate(peffect->reqs, preq) {
    if (buf[0] != '\0') {
      mystrlcat(buf, "+", buf_len);
    }

    universal_name_translation(&preq->source,
			buf + strlen(buf), buf_len - strlen(buf));
  } requirement_list_iterate_end;
}

/**************************************************************************
  Iterate through all the effects in cache, and call callback for each.
  This is currently not very generic implementation, as we have only one user;
  ruleset sanity checking. If any callback returns FALSE, there is no
  further checking and this will return FALSE.
**************************************************************************/
bool iterate_effect_cache(iec_cb cb)
{
  assert(cb != NULL);

  effect_list_iterate(ruleset_cache.tracker, peffect) {
    if (!cb(peffect)) {
      return FALSE;
    }
  } effect_list_iterate_end;

  return TRUE;
}
