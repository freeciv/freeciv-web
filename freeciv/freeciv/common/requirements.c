/********************************************************************** 
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
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

#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "support.h"

#include "government.h"
#include "improvement.h"
#include "map.h"
#include "specialist.h"

#include "requirements.h"

/* Names of source types.  These must correspond to enum universals_n in
 * fc_types.h.  Do not change these unless you know what you're doing! */
static const char *universal_names[] = {
  "None",
  "Tech",
  "Gov",
  "Building",
  "Special",
  "Terrain",
  "Nation",
  "UnitType",
  "UnitFlag",
  "UnitClass",
  "UnitClassFlag",
  "OutputType",
  "Specialist",
  "MinSize",
  "AI",
  "TerrainClass",
  "Base",
  "MinYear",
  "TerrainAlter",
  "CityTile"
};

/* Names of requirement ranges. These must correspond to enum req_range in
 * requirements.h.  Do not change these unless you know what you're doing! */
static const char *req_range_names[REQ_RANGE_LAST] = {
  "Local",
  "Adjacent",
  "City",
  "Continent",
  "Player",
  "World"
};

/**************************************************************************
  Convert a range name to an enumerated value.

  The check is case insensitive and returns REQ_RANGE_LAST if no match
  is found.
**************************************************************************/
enum req_range req_range_from_str(const char *str)
{
  enum req_range range;

  assert(ARRAY_SIZE(req_range_names) == REQ_RANGE_LAST);

  for (range = 0; range < REQ_RANGE_LAST; range++) {
    if (0 == mystrcasecmp(req_range_names[range], str)) {
      return range;
    }
  }

  return REQ_RANGE_LAST;
}

/**************************************************************************
  Parse requirement type (kind) and value strings into a universal
  structure.  Passing in a NULL type is considered VUT_NONE (not an error).

  Pass this some values like "Building", "Factory".
  FIXME: ensure that every caller checks error return!
**************************************************************************/
struct universal universal_by_rule_name(const char *kind,
					const char *value)
{
  struct universal source;

  assert(ARRAY_SIZE(universal_names) == VUT_LAST);

  if (kind) {
    for (source.kind = 0;
	 source.kind < ARRAY_SIZE(universal_names);
	 source.kind++) {
      if (0 == mystrcasecmp(universal_names[source.kind], kind)) {
	break;
      }
    }
  } else {
    source.kind = VUT_NONE;
  }

  /* Finally scan the value string based on the type of the source. */
  switch (source.kind) {
  case VUT_NONE:
    return source;
  case VUT_ADVANCE:
    source.value.advance = find_advance_by_rule_name(value);
    if (source.value.advance != NULL) {
      return source;
    }
    break;
  case VUT_GOVERNMENT:
    source.value.govern = find_government_by_rule_name(value);
    if (source.value.govern != NULL) {
      return source;
    }
    break;
  case VUT_IMPROVEMENT:
    source.value.building = find_improvement_by_rule_name(value);
    if (source.value.building != NULL) {
      return source;
    }
    break;
  case VUT_SPECIAL:
    source.value.special = find_special_by_rule_name(value);
    if (source.value.special != S_LAST) {
      return source;
    }
    break;
  case VUT_TERRAIN:
    source.value.terrain = find_terrain_by_rule_name(value);
    if (source.value.terrain != T_UNKNOWN) {
      return source;
    }
    break;
  case VUT_NATION:
    source.value.nation = find_nation_by_rule_name(value);
    if (source.value.nation != NO_NATION_SELECTED) {
      return source;
    }
    break;
  case VUT_UTYPE:
    source.value.utype = find_unit_type_by_rule_name(value);
    if (source.value.utype) {
      return source;
    }
    break;
  case VUT_UTFLAG:
    source.value.unitflag = find_unit_flag_by_rule_name(value);
    if (source.value.unitflag != F_LAST) {
      return source;
    }
    break;
  case VUT_UCLASS:
    source.value.uclass = find_unit_class_by_rule_name(value);
    if (source.value.uclass) {
      return source;
    }
    break;
  case VUT_UCFLAG:
    source.value.unitclassflag = find_unit_class_flag_by_rule_name(value);
    if (source.value.unitclassflag != UCF_LAST) {
      return source;
    }
    break;
  case VUT_OTYPE:
    source.value.outputtype = find_output_type_by_identifier(value);
    if (source.value.outputtype != O_LAST) {
      return source;
    }
    break;
  case VUT_SPECIALIST:
    source.value.specialist = find_specialist_by_rule_name(value);
    if (source.value.specialist) {
      return source;
    }
  case VUT_MINSIZE:
    source.value.minsize = atoi(value);
    if (source.value.minsize > 0) {
      return source;
    }
    break;
  case VUT_AI_LEVEL:
    source.value.ai_level = find_ai_level_by_name(value);
    if (source.value.ai_level != AI_LEVEL_LAST) {
      return source;
    }
    break;
  case VUT_TERRAINCLASS:
    source.value.terrainclass = find_terrain_class_by_rule_name(value);
    if (source.value.terrainclass != TC_LAST) {
      return source;
    }
    break;
  case VUT_BASE:
    source.value.base = find_base_type_by_rule_name(value);
    if (source.value.base != NULL) {
      return source;
    }
    break;
  case VUT_MINYEAR:
    source.value.minyear = atoi(value);
    if (source.value.minyear >= GAME_START_YEAR) {
      return source;
    }
    break;
  case VUT_TERRAINALTER:
    source.value.terrainalter = find_terrain_alteration_by_rule_name(value);
    if (source.value.terrainalter != TA_LAST) {
      return source;
    }
    break;
  case VUT_CITYTILE:
    source.value.citytile = find_citytile_by_rule_name(value);
    if (source.value.citytile != CITYT_LAST) {
      return source;
    }
    break;
  case VUT_LAST:
  default:
    break;
  }

  /* If we reach here there's been an error. */
  source.kind = VUT_LAST;
  return source;
}

/**************************************************************************
  Combine values into a universal structure.  This is for serialization
  and is the opposite of universal_extraction().
  FIXME: ensure that every caller checks error return!
**************************************************************************/
struct universal universal_by_number(const enum universals_n kind,
				     const int value)
{
  struct universal source;

  source.kind = kind;

  switch (source.kind) {
  case VUT_NONE:
    return source;
  case VUT_ADVANCE:
    source.value.advance = advance_by_number(value);
    if (source.value.advance != NULL) {
      return source;
    }
    break;
  case VUT_GOVERNMENT:
    source.value.govern = government_by_number(value);
    if (source.value.govern != NULL) {
      return source;
    }
    break;
  case VUT_IMPROVEMENT:
    source.value.building = improvement_by_number(value);
    if (source.value.building != NULL) {
      return source;
    }
    break;
  case VUT_SPECIAL:
    source.value.special = value;
    return source;
  case VUT_TERRAIN:
    source.value.terrain = terrain_by_number(value);
    if (source.value.terrain != NULL) {
      return source;
    }
    break;
  case VUT_NATION:
    source.value.nation = nation_by_number(value);
    if (source.value.nation != NULL) {
      return source;
    }
    break;
  case VUT_UTYPE:
    source.value.utype = utype_by_number(value);
    if (source.value.utype != NULL) {
      return source;
    }
    break;
  case VUT_UTFLAG:
    source.value.unitflag = value;
    return source;
  case VUT_UCLASS:
    source.value.uclass = uclass_by_number(value);
    if (source.value.uclass != NULL) {
      return source;
    }
    break;
  case VUT_UCFLAG:
    source.value.unitclassflag = value;
    return source;
  case VUT_OTYPE:
    source.value.outputtype = value;
    return source;
  case VUT_SPECIALIST:
    source.value.specialist = specialist_by_number(value);
    return source;
  case VUT_MINSIZE:
    source.value.minsize = value;
    return source;
  case VUT_AI_LEVEL:
    source.value.ai_level = value;
    return source;
  case VUT_TERRAINCLASS:
    source.value.terrainclass = value;
    return source;
  case VUT_BASE:
    source.value.base = base_by_number(value);
    return source;
  case VUT_MINYEAR:
    source.value.minyear = value;
    return source;
  case VUT_TERRAINALTER:
    source.value.terrainalter = value;
    return source;
  case VUT_CITYTILE:
    source.value.citytile = value;
    return source;
  case VUT_LAST:
    return source;
  default:
    assert(0);
    break;
  }

  /* If we reach here there's been an error. */
  source.kind = VUT_LAST;
  return source;
}

/**************************************************************************
  Extract universal structure into its components for serialization;
  the opposite of universal_by_number().
**************************************************************************/
void universal_extraction(const struct universal *source,
			  int *kind, int *value)
{
  *kind = source->kind;
  *value = universal_number(source);
}

/**************************************************************************
  Return the universal number of the constituent.
**************************************************************************/
int universal_number(const struct universal *source)
{
  switch (source->kind) {
  case VUT_NONE:
    return 0;
  case VUT_ADVANCE:
    return advance_number(source->value.advance);
  case VUT_GOVERNMENT:
    return government_number(source->value.govern);
  case VUT_IMPROVEMENT:
    return improvement_number(source->value.building);
  case VUT_SPECIAL:
    return source->value.special;
  case VUT_TERRAIN:
    return terrain_number(source->value.terrain);
  case VUT_NATION:
    return nation_number(source->value.nation);
  case VUT_UTYPE:
    return utype_number(source->value.utype);
  case VUT_UTFLAG:
    return source->value.unitflag;
  case VUT_UCLASS:
    return uclass_number(source->value.uclass);
  case VUT_UCFLAG:
    return source->value.unitclassflag;
  case VUT_OTYPE:
    return source->value.outputtype;
  case VUT_SPECIALIST:
    return specialist_number(source->value.specialist);
  case VUT_MINSIZE:
    return source->value.minsize;
  case VUT_AI_LEVEL:
    return source->value.ai_level;
  case VUT_TERRAINCLASS:
    return source->value.terrainclass;
  case VUT_BASE:
    return base_number(source->value.base);
  case VUT_MINYEAR:
    return source->value.minyear;
  case VUT_TERRAINALTER:
    return source->value.terrainalter;
  case VUT_CITYTILE:
    return source->value.citytile;
  case VUT_LAST:
  default:
    break;
  }

  /* If we reach here there's been an error. */
  assert(0);
  return 0;
}

/****************************************************************************
  Parse a requirement type and value string into a requrement structure.
  Returns VUT_LAST on error.  Passing in a NULL type is considered VUT_NONE
  (not an error).

  Pass this some values like "Building", "Factory".
****************************************************************************/
struct requirement req_from_str(const char *type, const char *range,
				bool survives, bool negated,
				const char *value)
{
  struct requirement req;
  bool invalid = TRUE;

  req.source = universal_by_rule_name(type, value);

  /* Scan the range string to find the range.  If no range is given a
   * default fallback is used rather than giving an error. */
  req.range = req_range_from_str(range);
  if (req.range == REQ_RANGE_LAST) {
    switch (req.source.kind) {
    case VUT_NONE:
    case VUT_LAST:
      break;
    case VUT_IMPROVEMENT:
    case VUT_SPECIAL:
    case VUT_TERRAIN:
    case VUT_UTYPE:
    case VUT_UTFLAG:
    case VUT_UCLASS:
    case VUT_UCFLAG:
    case VUT_OTYPE:
    case VUT_SPECIALIST:
    case VUT_TERRAINCLASS:
    case VUT_BASE:
    case VUT_TERRAINALTER:
    case VUT_CITYTILE:
      req.range = REQ_RANGE_LOCAL;
      break;
    case VUT_MINSIZE:
      req.range = REQ_RANGE_CITY;
      break;
    case VUT_GOVERNMENT:
    case VUT_ADVANCE:
    case VUT_NATION:
    case VUT_AI_LEVEL:
      req.range = REQ_RANGE_PLAYER;
      break;
    case VUT_MINYEAR:
      req.range = REQ_RANGE_WORLD;
      break;
    }
  }

  req.survives = survives;
  req.negated = negated;

  /* These checks match what combinations are supported inside
   * is_req_active(). */
  switch (req.source.kind) {
  case VUT_SPECIAL:
  case VUT_TERRAIN:
  case VUT_TERRAINCLASS:
  case VUT_BASE:
    invalid = (req.range != REQ_RANGE_LOCAL
	       && req.range != REQ_RANGE_ADJACENT);
    break;
  case VUT_ADVANCE:
    invalid = (req.range < REQ_RANGE_PLAYER);
    break;
  case VUT_GOVERNMENT:
  case VUT_AI_LEVEL:
    invalid = (req.range != REQ_RANGE_PLAYER);
    break;
  case VUT_IMPROVEMENT:
    invalid = ((req.range == REQ_RANGE_WORLD
		&& !is_great_wonder(req.source.value.building))
	       || (req.range > REQ_RANGE_CITY
		   && !is_wonder(req.source.value.building)));
    break;
  case VUT_MINSIZE:
    invalid = (req.range != REQ_RANGE_CITY);
    break;
  case VUT_NATION:
    invalid = (req.range != REQ_RANGE_PLAYER
	       && req.range != REQ_RANGE_WORLD);
    break;
  case VUT_UTYPE:
  case VUT_UTFLAG:
  case VUT_UCLASS:
  case VUT_UCFLAG:
  case VUT_OTYPE:
  case VUT_SPECIALIST:
  case VUT_TERRAINALTER: /* XXX could in principle support ADJACENT */
  case VUT_CITYTILE:
    invalid = (req.range != REQ_RANGE_LOCAL);
    break;
  case VUT_MINYEAR:
    invalid = (req.range != REQ_RANGE_WORLD);
    break;
  case VUT_NONE:
    invalid = FALSE;
    break;
  case VUT_LAST:
    break;
  }
  if (invalid) {
    freelog(LOG_ERROR, "Invalid requirement %s | %s | %s | %s | %s",
	    type, range,
	    survives ? "survives" : "",
	    negated ? "negated" : "", value);
    req.source.kind = VUT_LAST;
  }

  return req;
}

/****************************************************************************
  Set the values of a req from serializable integers.  This is the opposite
  of req_get_values.
****************************************************************************/
struct requirement req_from_values(int type, int range,
				   bool survives, bool negated,
				   int value)
{
  struct requirement req;

  req.source = universal_by_number(type, value);
  req.range = range;
  req.survives = survives;
  req.negated = negated;
  return req;
}

/****************************************************************************
  Return the value of a req as a serializable integer.  This is the opposite
  of req_set_value.
****************************************************************************/
void req_get_values(const struct requirement *req,
		    int *type, int *range,
		    bool *survives, bool *negated,
		    int *value)
{
  universal_extraction(&req->source, type, value);
  *range = req->range;
  *survives = req->survives;
  *negated = req->negated;
}

/****************************************************************************
  Returns TRUE if req1 and req2 are equal.
****************************************************************************/
bool are_requirements_equal(const struct requirement *req1,
			    const struct requirement *req2)
{
  return (are_universals_equal(&req1->source, &req2->source)
	  && req1->range == req2->range
	  && req1->survives == req2->survives
	  && req1->negated == req2->negated);
}

/****************************************************************************
  Returns the number of total world buildings (this includes buildings
  that have been destroyed).
****************************************************************************/
static int num_world_buildings_total(const struct impr_type *building)
{
  if (is_great_wonder(building)) {
    return (great_wonder_is_built(building)
            || great_wonder_is_destroyed(building) ? 1 : 0);
  } else {
    freelog(LOG_ERROR,
	    /* TRANS: Obscure ruleset error. */
	    _("World-ranged requirements are only supported for wonders."));
    return 0;
  }
}

/****************************************************************************
  Returns the number of buildings of a certain type in the world.
****************************************************************************/
static int num_world_buildings(const struct impr_type *building)
{
  if (is_great_wonder(building)) {
    return (great_wonder_is_built(building) ? 1 : 0);
  } else {
    freelog(LOG_ERROR,
	    /* TRANS: Obscure ruleset error. */
	    _("World-ranged requirements are only supported for wonders."));
    return 0;
  }
}

/****************************************************************************
  Returns the number of buildings of a certain type owned by plr.
****************************************************************************/
static int num_player_buildings(const struct player *pplayer,
				const struct impr_type *building)
{
  if (is_wonder(building)) {
    return (wonder_is_built(pplayer, building) ? 1 : 0);
  } else {
    freelog(LOG_ERROR,
	    /* TRANS: Obscure ruleset error. */
	    _("Player-ranged requirements are only supported for wonders."));
    return 0;
  }
}

/****************************************************************************
  Returns the number of buildings of a certain type on a continent.
****************************************************************************/
static int num_continent_buildings(const struct player *pplayer,
				   int continent,
				   const struct impr_type *building)
{
  if (is_wonder(building)) {
    const struct city *pcity;

    pcity = find_city_from_wonder(pplayer, building);
    if (pcity && tile_continent(pcity->tile) == continent) {
      return 1;
    }
  } else {
    freelog(LOG_ERROR,
	    /* TRANS: Obscure ruleset error. */
	    _("Island-ranged requirements are only supported for wonders."));
  }
  return 0;
}

/****************************************************************************
  Returns the number of buildings of a certain type in a city.
****************************************************************************/
static int num_city_buildings(const struct city *pcity,
			      const struct impr_type *building)
{
  return (city_has_building(pcity, building) ? 1 : 0);
}

/****************************************************************************
  How many of the source building are there within range of the target?

  The target gives the type of the target.  The exact target is a player,
  city, or building specified by the target_xxx arguments.

  The range gives the range of the requirement.

  "Survives" specifies whether the requirement allows destroyed sources.
  If set then all source buildings ever built are counted; if not then only
  living buildings are counted.

  source gives the building type of the source in question.

  Note that this function does a lookup into the source caches to find
  the number of available sources.  However not all source caches exist: if
  the cache doesn't exist then we return 0.
****************************************************************************/
static int count_buildings_in_range(const struct player *target_player,
				    const struct city *target_city,
				    const struct impr_type *target_building,
				    enum req_range range,
				    bool survives,
				    const struct impr_type *source)
{
  if (improvement_obsolete(target_player, source)) {
    return 0;
  }

  if (survives) {
    if (range == REQ_RANGE_WORLD) {
      return num_world_buildings_total(source);
    } else {
      /* There is no sources cache for this. */
      freelog(LOG_ERROR,
	      /* TRANS: Obscure ruleset error. */
	      _("Surviving requirements are only "
		"supported at world range."));
      return 0;
    }
  }

  switch (range) {
  case REQ_RANGE_WORLD:
    return num_world_buildings(source);
  case REQ_RANGE_PLAYER:
    return target_player ? num_player_buildings(target_player, source) : 0;
  case REQ_RANGE_CONTINENT:
    if (target_player && target_city) {
      int continent = tile_continent(target_city->tile);

      return num_continent_buildings(target_player, continent, source);
    } else {
      /* At present, "Continent" effects can affect only
       * cities and units in cities. */
      return 0;
    }
  case REQ_RANGE_CITY:
    return target_city ? num_city_buildings(target_city, source) : 0;
  case REQ_RANGE_LOCAL:
    if (target_building && target_building == source) {
      return num_city_buildings(target_city, source);
    } else {
      /* TODO: other local targets */
      return 0;
    }
  case REQ_RANGE_ADJACENT:
    return 0;
  case REQ_RANGE_LAST:
    break;
  }
  assert(0);
  return 0;
}

/****************************************************************************
  Is there a source tech within range of the target?
****************************************************************************/
static bool is_tech_in_range(const struct player *target_player,
			     enum req_range range,
			     Tech_type_id tech)
{
  switch (range) {
  case REQ_RANGE_PLAYER:
    return (target_player
	    && player_invention_state(target_player, tech) == TECH_KNOWN);
  case REQ_RANGE_WORLD:
    return game.info.global_advances[tech];
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_LAST:
    break;
  }

  assert(0);
  return FALSE;
}

/****************************************************************************
  Is there a source special within range of the target?
****************************************************************************/
static bool is_special_in_range(const struct tile *target_tile,
				enum req_range range, bool survives,
				enum tile_special_type special)

{
  switch (range) {
  case REQ_RANGE_LOCAL:
    return target_tile && tile_has_special(target_tile, special);
  case REQ_RANGE_ADJACENT:
    return target_tile && is_special_near_tile(target_tile, special, TRUE);
  case REQ_RANGE_CITY:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_LAST:
    break;
  }

  assert(0);
  return FALSE;
}

/****************************************************************************
  Is there a source tile within range of the target?
****************************************************************************/
static bool is_terrain_in_range(const struct tile *target_tile,
				enum req_range range, bool survives,
				const struct terrain *pterrain)
{
  if (!target_tile) {
    return FALSE;
  }

  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has the terrain. */
    return pterrain && tile_terrain(target_tile) == pterrain;
  case REQ_RANGE_ADJACENT:
    return pterrain && is_terrain_near_tile(target_tile, pterrain, TRUE);
  case REQ_RANGE_CITY:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_LAST:
    break;
  }

  assert(0);
  return FALSE;
}

/****************************************************************************
  Is there a source terrain class within range of the target?
****************************************************************************/
static bool is_terrain_class_in_range(const struct tile *target_tile,
                                      enum req_range range, bool survives,
                                      enum terrain_class class)
{
  if (!target_tile) {
    return FALSE;
  }

  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has the terrain of correct class. */
    return terrain_belongs_to_class(tile_terrain(target_tile), class);
  case REQ_RANGE_ADJACENT:
    return is_terrain_class_near_tile(target_tile, class);
  case REQ_RANGE_CITY:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_LAST:
    break;
  }

  assert(0);
  return FALSE;
}

/****************************************************************************
  Is there a source base type within range of the target?
****************************************************************************/
static bool is_base_type_in_range(const struct tile *target_tile,
                                  enum req_range range, bool survives,
                                  struct base_type *pbase)
{
  if (!target_tile) {
    return FALSE;
  }

  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has base of requested type. */
    return tile_has_base(target_tile, pbase);
  case REQ_RANGE_ADJACENT:
    return is_base_near_tile(target_tile, pbase);
  case REQ_RANGE_CITY:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_LAST:
    break;
  }

  assert(0);
  return FALSE;
}

/****************************************************************************
  Is there a terrain which can support the specified infrastructure
  within range of the target?
****************************************************************************/
static bool is_terrain_alter_possible_in_range(const struct tile *target_tile,
                                           enum req_range range, bool survives,
                                           enum terrain_alteration alteration)
{
  if (!target_tile) {
    return FALSE;
  }

  switch (range) {
  case REQ_RANGE_LOCAL:
    return terrain_can_support_alteration(tile_terrain(target_tile),
                                          alteration);
  case REQ_RANGE_ADJACENT: /* XXX Could in principle support ADJACENT. */
  case REQ_RANGE_CITY:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_LAST:
    break;
  }

  assert(0);
  return FALSE;
}

/****************************************************************************
  Is there a nation within range of the target?
****************************************************************************/
static bool is_nation_in_range(const struct player *target_player,
			       enum req_range range, bool survives,
			       const struct nation_type *nation)
{
  switch (range) {
  case REQ_RANGE_PLAYER:
    return target_player && nation_of_player(target_player) == nation;
  case REQ_RANGE_WORLD:
    /* FIXME: inefficient */
    players_iterate(pplayer) {
      if (nation_of_player(pplayer) == nation && (pplayer->is_alive || survives)) {
	return TRUE;
      }
    } players_iterate_end;
    return FALSE;
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_LAST:
    break;
  }

  assert(0);
  return FALSE;
}

/****************************************************************************
  Is there a unit of the given type within range of the target?
****************************************************************************/
static bool is_unittype_in_range(const struct unit_type *target_unittype,
				 enum req_range range, bool survives,
				 struct unit_type *punittype)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  return (range == REQ_RANGE_LOCAL
	  && (!target_unittype
	      || target_unittype == punittype));
}

/****************************************************************************
  Is there a unit with the given flag within range of the target?
****************************************************************************/
static bool is_unitflag_in_range(const struct unit_type *target_unittype,
				 enum req_range range, bool survives,
				 enum unit_flag_id unitflag,
                                 enum req_problem_type prob_type)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  if (range != REQ_RANGE_LOCAL) {
    return FALSE;
  }
  if (!target_unittype) {
    /* Unknow means TRUE  for RPT_POSSIBLE
     *              FALSE for RPT_CERTAIN
     */
    return prob_type == RPT_POSSIBLE;
  }

  return utype_has_flag(target_unittype, unitflag);
}

/****************************************************************************
  Is there a unit with the given flag within range of the target?
****************************************************************************/
static bool is_unitclass_in_range(const struct unit_type *target_unittype,
				  enum req_range range, bool survives,
				  struct unit_class *pclass)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  return (range == REQ_RANGE_LOCAL
	  && (!target_unittype
	      || utype_class(target_unittype) == pclass));
}

/****************************************************************************
  Is there a unit with the given flag within range of the target?
****************************************************************************/
static bool is_unitclassflag_in_range(const struct unit_type *target_unittype,
				      enum req_range range, bool survives,
				      enum unit_class_flag_id ucflag)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  return (range == REQ_RANGE_LOCAL
	  && (!target_unittype
	      || uclass_has_flag(utype_class(target_unittype), ucflag)));
}

/****************************************************************************
  Checks the requirement to see if it is active on the given target.

  target gives the type of the target
  (player,city,building,tile) give the exact target
  req gives the requirement itself

  Make sure you give all aspects of the target when calling this function:
  for instance if you have TARGET_CITY pass the city's owner as the target
  player as well as the city itself as the target city.
****************************************************************************/
bool is_req_active(const struct player *target_player,
		   const struct city *target_city,
		   const struct impr_type *target_building,
		   const struct tile *target_tile,
		   const struct unit_type *target_unittype,
		   const struct output_type *target_output,
		   const struct specialist *target_specialist,
		   const struct requirement *req,
                   const enum   req_problem_type prob_type)
{
  bool eval = FALSE;

  /* Note the target may actually not exist.  In particular, effects that
   * have a VUT_SPECIAL or VUT_TERRAIN may often be passed to this function
   * with a city as their target.  In this case the requirement is simply
   * not met. */
  switch (req->source.kind) {
  case VUT_NONE:
    eval = TRUE;
    break;
  case VUT_ADVANCE:
    /* The requirement is filled if the player owns the tech. */
    eval = is_tech_in_range(target_player, req->range,
			    advance_number(req->source.value.advance));
    break;
  case VUT_GOVERNMENT:
    /* The requirement is filled if the player is using the government. */
    eval = (government_of_player(target_player) == req->source.value.govern);
    break;
  case VUT_IMPROVEMENT:
    /* The requirement is filled if there's at least one of the building
     * in the city.  (This is a slightly nonstandard use of
     * count_sources_in_range.) */
    eval = (count_buildings_in_range(target_player, target_city,
				     target_building,
				     req->range, req->survives,
				     req->source.value.building) > 0);
    break;
  case VUT_SPECIAL:
    eval = is_special_in_range(target_tile,
			       req->range, req->survives,
			       req->source.value.special);
    break;
  case VUT_TERRAIN:
    eval = is_terrain_in_range(target_tile,
			       req->range, req->survives,
			       req->source.value.terrain);
    break;
  case VUT_NATION:
    eval = is_nation_in_range(target_player, req->range, req->survives,
			      req->source.value.nation);
    break;
  case VUT_UTYPE:
    eval = is_unittype_in_range(target_unittype,
				req->range, req->survives,
				req->source.value.utype);
    break;
  case VUT_UTFLAG:
    eval = is_unitflag_in_range(target_unittype,
				req->range, req->survives,
				req->source.value.unitflag,
                                prob_type);
    break;
  case VUT_UCLASS:
    eval = is_unitclass_in_range(target_unittype,
				 req->range, req->survives,
				 req->source.value.uclass);
    break;
  case VUT_UCFLAG:
    eval = is_unitclassflag_in_range(target_unittype,
				     req->range, req->survives,
				     req->source.value.unitclassflag);
    break;
  case VUT_OTYPE:
    eval = (target_output
	    && target_output->index == req->source.value.outputtype);
    break;
  case VUT_SPECIALIST:
    eval = (target_specialist
	    && target_specialist == req->source.value.specialist);
    break;
  case VUT_MINSIZE:
    eval = target_city && target_city->size >= req->source.value.minsize;
    break;
  case VUT_AI_LEVEL:
    eval = target_player
      && target_player->ai_data.control
      && target_player->ai_data.skill_level == req->source.value.ai_level;
    break;
  case VUT_TERRAINCLASS:
    eval = is_terrain_class_in_range(target_tile,
                                     req->range, req->survives,
                                     req->source.value.terrainclass);
    break;
  case VUT_BASE:
    eval = is_base_type_in_range(target_tile,
                                 req->range, req->survives,
                                 req->source.value.base);
    break;
  case VUT_MINYEAR:
    eval = game.info.year >= req->source.value.minyear;
    break;
  case VUT_TERRAINALTER:
    eval = is_terrain_alter_possible_in_range(target_tile,
                                              req->range, req->survives,
                                              req->source.value.terrainalter);
    break;
  case VUT_CITYTILE:
    if (target_tile) {
      if (req->source.value.citytile == CITYT_CENTER) {
        if (target_city) {
          eval = is_city_center(target_city, target_tile);
        } else {
          eval = tile_city(target_tile) != NULL;
        }
      } else {
        /* Not implemented */
        assert(FALSE);
      }
    } else {
      eval = FALSE;
    }
    break;
  case VUT_LAST:
    assert(0);
    return FALSE;
  }

  if (req->negated) {
    return !eval;
  } else {
    return eval;
  }
}

/****************************************************************************
  Checks the requirement(s) to see if they are active on the given target.

  target gives the type of the target
  (player,city,building,tile) give the exact target

  reqs gives the requirement vector.
  The function returns TRUE only if all requirements are active.

  Make sure you give all aspects of the target when calling this function:
  for instance if you have TARGET_CITY pass the city's owner as the target
  player as well as the city itself as the target city.
****************************************************************************/
bool are_reqs_active(const struct player *target_player,
		     const struct city *target_city,
		     const struct impr_type *target_building,
		     const struct tile *target_tile,
		     const struct unit_type *target_unittype,
		     const struct output_type *target_output,
		     const struct specialist *target_specialist,
		     const struct requirement_vector *reqs,
                     const enum   req_problem_type prob_type)
{
  requirement_vector_iterate(reqs, preq) {
    if (!is_req_active(target_player, target_city, target_building,
		       target_tile, target_unittype, target_output,
		       target_specialist,
		       preq, prob_type)) {
      return FALSE;
    }
  } requirement_vector_iterate_end;
  return TRUE;
}

/****************************************************************************
  Return TRUE if this is an "unchanging" requirement.  This means that
  if a target can't meet the requirement now, it probably won't ever be able
  to do so later.  This can be used to do requirement filtering when checking
  if a target may "eventually" become available.

  Note this isn't absolute.  Returning TRUE here just means that the
  requirement probably can't be met.  In some cases (particularly terrains)
  it may be wrong.
*****************************************************************************/
bool is_req_unchanging(const struct requirement *req)
{
  switch (req->source.kind) {
  case VUT_NATION:
  case VUT_NONE:
  case VUT_OTYPE:
  case VUT_SPECIALIST:	/* Only so long as it's at local range only */
  case VUT_AI_LEVEL:
  case VUT_CITYTILE:
    return TRUE;
  case VUT_ADVANCE:
  case VUT_GOVERNMENT:
  case VUT_IMPROVEMENT:
  case VUT_MINSIZE:
  case VUT_UTYPE:	/* Not sure about this one */
  case VUT_UTFLAG:	/* Not sure about this one */
  case VUT_UCLASS:	/* Not sure about this one */
  case VUT_UCFLAG:	/* Not sure about this one */
    return FALSE;
  case VUT_SPECIAL:
  case VUT_TERRAIN:
  case VUT_TERRAINCLASS:
  case VUT_TERRAINALTER:
  case VUT_BASE:
    /* Terrains, specials and bases aren't really unchanging; in fact they're
     * practically guaranteed to change.  We return TRUE here for historical
     * reasons and so that the AI doesn't get confused (since the AI
     * doesn't know how to meet special and terrain requirements). */
    return TRUE;
  case VUT_MINYEAR:
    /* Once year is reached, it does not change again */
    return req->source.value.minyear > game.info.year;
  case VUT_LAST:
    break;
  }
  assert(0);
  return TRUE;
}

/****************************************************************************
  Return TRUE iff the two sources are equivalent.  Note this isn't the
  same as an == or memcmp check.
*****************************************************************************/
bool are_universals_equal(const struct universal *psource1,
			  const struct universal *psource2)
{
  if (psource1->kind != psource2->kind) {
    return FALSE;
  }
  switch (psource1->kind) {
  case VUT_NONE:
    return TRUE;
  case VUT_ADVANCE:
    return psource1->value.advance == psource2->value.advance;
  case VUT_GOVERNMENT:
    return psource1->value.govern == psource2->value.govern;
  case VUT_IMPROVEMENT:
    return psource1->value.building == psource2->value.building;
  case VUT_SPECIAL:
    return psource1->value.special == psource2->value.special;
  case VUT_TERRAIN:
    return psource1->value.terrain == psource2->value.terrain;
  case VUT_NATION:
    return psource1->value.nation == psource2->value.nation;
  case VUT_UTYPE:
    return psource1->value.utype == psource2->value.utype;
  case VUT_UTFLAG:
    return psource1->value.unitflag == psource2->value.unitflag;
  case VUT_UCLASS:
    return psource1->value.uclass == psource2->value.uclass;
  case VUT_UCFLAG:
    return psource1->value.unitclassflag == psource2->value.unitclassflag;
  case VUT_OTYPE:
    return psource1->value.outputtype == psource2->value.outputtype;
  case VUT_SPECIALIST:
    return psource1->value.specialist == psource2->value.specialist;
  case VUT_MINSIZE:
    return psource1->value.minsize == psource2->value.minsize;
  case VUT_AI_LEVEL:
    return psource1->value.ai_level == psource2->value.ai_level;
  case VUT_TERRAINCLASS:
    return psource1->value.terrainclass == psource2->value.terrainclass;
  case VUT_BASE:
    return psource1->value.base == psource2->value.base;
  case VUT_MINYEAR:
    return psource1->value.minyear == psource2->value.minyear;
  case VUT_TERRAINALTER:
    return psource1->value.terrainalter == psource2->value.terrainalter;
  case VUT_CITYTILE:
    return psource1->value.citytile == psource2->value.citytile;
  case VUT_LAST:
    break;
  }
  assert(0);
  return FALSE;
}

/****************************************************************************
  Return the (untranslated) rule name of the kind of universal.
  You don't have to free the return pointer.
*****************************************************************************/
const char *universal_kind_name(const enum universals_n kind)
{
  assert(kind >= 0 && kind < ARRAY_SIZE(universal_names));
  return universal_names[kind];
}

/****************************************************************************
  Return the (untranslated) rule name of the universal.
  You don't have to free the return pointer.
*****************************************************************************/
const char *universal_rule_name(const struct universal *psource)
{
  switch (psource->kind) {
  case VUT_NONE:
  case VUT_CITYTILE:
    /* TRANS: missing value */
    return N_("(none)");
  case VUT_ADVANCE:
    return advance_rule_name(psource->value.advance);
  case VUT_GOVERNMENT:
    return government_rule_name(psource->value.govern);
  case VUT_IMPROVEMENT:
    return improvement_rule_name(psource->value.building);
  case VUT_SPECIAL:
    return special_rule_name(psource->value.special);
  case VUT_TERRAIN:
    return terrain_rule_name(psource->value.terrain);
  case VUT_NATION:
    return nation_rule_name(psource->value.nation);
  case VUT_UTYPE:
    return utype_rule_name(psource->value.utype);
  case VUT_UTFLAG:
    return unit_flag_rule_name(psource->value.unitflag);
  case VUT_UCLASS:
    return uclass_rule_name(psource->value.uclass);
  case VUT_UCFLAG:
    return unit_class_flag_rule_name(psource->value.unitclassflag);
  case VUT_OTYPE:
    return get_output_name(psource->value.outputtype);
  case VUT_SPECIALIST:
    return specialist_rule_name(psource->value.specialist);
  case VUT_MINSIZE:
    return N_("Size %d");
  case VUT_AI_LEVEL:
    return ai_level_name(psource->value.ai_level);
  case VUT_TERRAINCLASS:
    return terrain_class_rule_name(psource->value.terrainclass);
  case VUT_BASE:
    return base_rule_name(psource->value.base);
  case VUT_TERRAINALTER:
    return terrain_alteration_rule_name(psource->value.terrainalter);
  case VUT_LAST:
  default:
    assert(0);
    break;
  }

  return NULL;
}

/****************************************************************************
  Make user-friendly text for the source.  The text is put into a user
  buffer which is also returned.
*****************************************************************************/
const char *universal_name_translation(const struct universal *psource,
				       char *buf, size_t bufsz)
{
  buf[0] = '\0'; /* to be safe. */
  switch (psource->kind) {
  case VUT_NONE:
    /* TRANS: missing value */
    mystrlcat(buf, _("(none)"), bufsz);
    break;
  case VUT_ADVANCE:
    mystrlcat(buf, advance_name_translation(psource->value.advance), bufsz);
    break;
  case VUT_GOVERNMENT:
    mystrlcat(buf, government_name_translation(psource->value.govern), bufsz);
    break;
  case VUT_IMPROVEMENT:
    mystrlcat(buf, improvement_name_translation(psource->value.building), bufsz);
    break;
  case VUT_SPECIAL:
    mystrlcat(buf, special_name_translation(psource->value.special), bufsz);
    break;
  case VUT_TERRAIN:
    mystrlcat(buf, terrain_name_translation(psource->value.terrain), bufsz);
    break;
  case VUT_NATION:
    mystrlcat(buf, nation_adjective_translation(psource->value.nation), bufsz);
    break;
  case VUT_UTYPE:
    mystrlcat(buf, utype_name_translation(psource->value.utype), bufsz);
    break;
  case VUT_UTFLAG:
    cat_snprintf(buf, bufsz, _("\"%s\" units"),
		 /* flag names are never translated */
		 unit_flag_rule_name(psource->value.unitflag));
    break;
  case VUT_UCLASS:
    cat_snprintf(buf, bufsz, _("%s units"),
		 uclass_name_translation(psource->value.uclass));
    break;
  case VUT_UCFLAG:
    cat_snprintf(buf, bufsz, _("\"%s\" units"),
		 /* flag names are never translated */
		 unit_flag_rule_name(psource->value.unitflag));
    break;
  case VUT_OTYPE:
    mystrlcat(buf, get_output_name(psource->value.outputtype), bufsz); /* FIXME */
    break;
  case VUT_SPECIALIST:
    mystrlcat(buf, specialist_name_translation(psource->value.specialist), bufsz);
    break;
  case VUT_MINSIZE:
    cat_snprintf(buf, bufsz, _("Size %d"),
		 psource->value.minsize);
    break;
  case VUT_AI_LEVEL:
    /* TRANS: "Hard AI" */
    cat_snprintf(buf, bufsz, _("%s AI"),
                 ai_level_name(psource->value.ai_level)); /* FIXME */
    break;
  case VUT_TERRAINCLASS:
    /* TRANS: "Land terrain" */
    cat_snprintf(buf, bufsz, _("%s terrain"),
                 terrain_class_name_translation(psource->value.terrainclass));
    break;
  case VUT_BASE:
    /* TRANS: "Fortress base" */
    cat_snprintf(buf, bufsz, _("%s base"),
                 base_name_translation(psource->value.base));
    break;
  case VUT_MINYEAR:
    cat_snprintf(buf, bufsz, _("After %s"),
                 textyear(psource->value.minyear));
    break;
  case VUT_TERRAINALTER:
    /* TRANS: "Irrigation possible" */
    cat_snprintf(buf, bufsz, _("%s possible"),
                 terrain_alteration_name_translation(psource->value.terrainalter));
    break;
  case VUT_CITYTILE:
    mystrlcat(buf, _("City center tile"), bufsz);
    break;
  case VUT_LAST:
    assert(0);
    break;
  }

  return buf;
}

/****************************************************************************
  Return untranslated name of the universal source name.
*****************************************************************************/
const char *universal_type_rule_name(const struct universal *psource)
{
  return universal_kind_name(psource->kind);
}

/**************************************************************************
  Return the number of shields it takes to build this universal.
**************************************************************************/
int universal_build_shield_cost(const struct universal *target)
{
  switch (target->kind) {
  case VUT_IMPROVEMENT:
    return impr_build_shield_cost(target->value.building);
  case VUT_UTYPE:
    return utype_build_shield_cost(target->value.utype);
  default:
    break;
  }
  return FC_INFINITY;
}
