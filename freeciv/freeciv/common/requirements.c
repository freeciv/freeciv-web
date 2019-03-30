/***********************************************************************
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
#include <fc_config.h>
#endif

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "achievements.h"
#include "calendar.h"
#include "citizens.h"
#include "culture.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "movement.h"
#include "player.h"
#include "map.h"
#include "research.h"
#include "road.h"
#include "server_settings.h"
#include "specialist.h"
#include "style.h"

#include "requirements.h"

/************************************************************************
  Container for req_item_found functions
************************************************************************/
typedef enum req_item_found (*universal_found)(const struct requirement *,
                                               const struct universal *);
static universal_found universal_found_function[VUT_COUNT] = {NULL};

/**********************************************************************//**
  Parse requirement type (kind) and value strings into a universal
  structure.  Passing in a NULL type is considered VUT_NONE (not an error).

  Pass this some values like "Building", "Factory".
  FIXME: ensure that every caller checks error return!
**************************************************************************/
struct universal universal_by_rule_name(const char *kind,
                                        const char *value)
{
  struct universal source;

  source.kind = universals_n_by_name(kind, fc_strcasecmp);
  if (!universals_n_is_valid(source.kind)) {
    return source;
  }

  universal_value_from_str(&source, value);

  return source;
}

/**********************************************************************//**
  Parse requirement value strings into a universal
  structure.
**************************************************************************/
void universal_value_from_str(struct universal *source, const char *value)
{
  /* Finally scan the value string based on the type of the source. */
  switch (source->kind) {
  case VUT_NONE:
    return;
  case VUT_ADVANCE:
    source->value.advance = advance_by_rule_name(value);
    if (source->value.advance != NULL) {
      return;
    }
    break;
  case VUT_TECHFLAG:
    source->value.techflag
      = tech_flag_id_by_name(value, fc_strcasecmp);
    if (tech_flag_id_is_valid(source->value.techflag)) {
      return;
    }
    break;
  case VUT_GOVERNMENT:
    source->value.govern = government_by_rule_name(value);
    if (source->value.govern != NULL) {
      return;
    }
    break;
  case VUT_ACHIEVEMENT:
    source->value.achievement = achievement_by_rule_name(value);
    if (source->value.achievement != NULL) {
      return;
    }
    break;
  case VUT_STYLE:
    source->value.style = style_by_rule_name(value);
    if (source->value.style != NULL) {
      return;
    }
    break;
  case VUT_IMPROVEMENT:
    source->value.building = improvement_by_rule_name(value);
    if (source->value.building != NULL) {
      return;
    }
    break;
  case VUT_IMPR_GENUS:
    source->value.impr_genus = impr_genus_id_by_name(value, fc_strcasecmp);
    if (impr_genus_id_is_valid(source->value.impr_genus)) {
      return;
    }
    break;
  case VUT_EXTRA:
    source->value.extra = extra_type_by_rule_name(value);
    if (source->value.extra != NULL) {
      return;
    }
    break;
  case VUT_GOOD:
    source->value.good = goods_by_rule_name(value);
    if (source->value.good != NULL) {
      return;
    }
    break;
  case VUT_TERRAIN:
    source->value.terrain = terrain_by_rule_name(value);
    if (source->value.terrain != T_UNKNOWN) {
      return;
    }
    break;
  case VUT_TERRFLAG:
    source->value.terrainflag
      = terrain_flag_id_by_name(value, fc_strcasecmp);
    if (terrain_flag_id_is_valid(source->value.terrainflag)) {
      return;
    }
    break;
  case VUT_NATION:
    source->value.nation = nation_by_rule_name(value);
    if (source->value.nation != NO_NATION_SELECTED) {
      return;
    }
    break;
  case VUT_NATIONGROUP:
    source->value.nationgroup = nation_group_by_rule_name(value);
    if (source->value.nationgroup != NULL) {
      return;
    }
    break;
  case VUT_NATIONALITY:
    source->value.nationality = nation_by_rule_name(value);
    if (source->value.nationality != NO_NATION_SELECTED) {
      return;
    }
    break;
  case VUT_DIPLREL:
    source->value.diplrel = diplrel_by_rule_name(value);
    if (source->value.diplrel != diplrel_other_invalid()) {
      return;
    }
    break;
  case VUT_UTYPE:
    source->value.utype = unit_type_by_rule_name(value);
    if (source->value.utype) {
      return;
    }
    break;
  case VUT_UTFLAG:
    source->value.unitflag = unit_type_flag_id_by_name(value, fc_strcasecmp);
    if (unit_type_flag_id_is_valid(source->value.unitflag)) {
      return;
    }
    break;
  case VUT_UCLASS:
    source->value.uclass = unit_class_by_rule_name(value);
    if (source->value.uclass) {
      return;
    }
    break;
  case VUT_UCFLAG:
    source->value.unitclassflag
      = unit_class_flag_id_by_name(value, fc_strcasecmp);
    if (unit_class_flag_id_is_valid(source->value.unitclassflag)) {
      return;
    }
    break;
  case VUT_MINVETERAN:
    source->value.minveteran = atoi(value);
    if (source->value.minveteran > 0) {
      return;
    }
    break;
  case VUT_UNITSTATE:
    source->value.unit_state = ustate_prop_by_name(value, fc_strcasecmp);
    if (ustate_prop_is_valid(source->value.unit_state)) {
      return;
    }
    break;
  case VUT_MINMOVES:
    source->value.minmoves = atoi(value);
    if (source->value.minmoves > 0) {
      return;
    }
    break;
  case VUT_MINHP:
    source->value.min_hit_points = atoi(value);
    if (source->value.min_hit_points > 0) {
      return;
    }
    break;
  case VUT_AGE:
    source->value.age = atoi(value);
    if (source->value.age > 0) {
      return;
    }
    break;
  case VUT_MINTECHS:
    source->value.min_techs = atoi(value);
    if (source->value.min_techs > 0) {
      return;
    }
  case VUT_ACTION:
    source->value.action = action_by_rule_name(value);
    if (source->value.action != NULL) {
      return;
    }
    break;
  case VUT_OTYPE:
    source->value.outputtype = output_type_by_identifier(value);
    if (source->value.outputtype != O_LAST) {
      return;
    }
    break;
  case VUT_SPECIALIST:
    source->value.specialist = specialist_by_rule_name(value);
    if (source->value.specialist) {
      return;
    }
  case VUT_MINSIZE:
    source->value.minsize = atoi(value);
    if (source->value.minsize > 0) {
      return;
    }
    break;
  case VUT_MINCULTURE:
    source->value.minculture = atoi(value);
    if (source->value.minculture > 0) {
      return;
    }
    break;
  case VUT_AI_LEVEL:
    source->value.ai_level = ai_level_by_name(value, fc_strcasecmp);
    if (ai_level_is_valid(source->value.ai_level)) {
      return;
    }
    break;
  case VUT_MAXTILEUNITS:
    source->value.max_tile_units = atoi(value);
    if (0 <= source->value.max_tile_units) {
      return;
    }
    break;
  case VUT_TERRAINCLASS:
    source->value.terrainclass
      = terrain_class_by_name(value, fc_strcasecmp);
    if (terrain_class_is_valid(source->value.terrainclass)) {
      return;
    }
    break;
  case VUT_BASEFLAG:
    source->value.baseflag = base_flag_id_by_name(value, fc_strcasecmp);
    if (base_flag_id_is_valid(source->value.baseflag)) {
      return;
    }
    break;
  case VUT_ROADFLAG:
    source->value.roadflag = road_flag_id_by_name(value, fc_strcasecmp);
    if (road_flag_id_is_valid(source->value.roadflag)) {
      return;
    }
    break;
  case VUT_EXTRAFLAG:
    source->value.extraflag = extra_flag_id_by_name(value, fc_strcasecmp);
    if (extra_flag_id_is_valid(source->value.extraflag)) {
      return;
    }
    break;
  case VUT_MINYEAR:
    source->value.minyear = atoi(value);
    return;
  case VUT_MINCALFRAG:
    /* Rule names are 0-based numbers, not pretty names from ruleset */
    source->value.mincalfrag = atoi(value);
    if (source->value.mincalfrag >= 0) {
      /* More range checking done later, in sanity_check_req_individual() */
      return;
    }
  case VUT_TOPO:
    source->value.topo_property = topo_flag_by_name(value, fc_strcasecmp);
    if (topo_flag_is_valid(source->value.topo_property)) {
      return;
    }
    break;
  case VUT_SERVERSETTING:
    source->value.ssetval = ssetv_by_rule_name(value);
    if (source->value.ssetval != SSETV_NONE) {
      return;
    }
    break;
  case VUT_TERRAINALTER:
    source->value.terrainalter
      = terrain_alteration_by_name(value, fc_strcasecmp);
    if (terrain_alteration_is_valid(source->value.terrainalter)) {
      return;
    }
    break;
  case VUT_CITYTILE:
    source->value.citytile = citytile_type_by_name(value, fc_strcasecmp);
    if (source->value.citytile != CITYT_LAST) {
      return;
    }
    break;
  case VUT_COUNT:
    break;
  }

  /* If we reach here there's been an error. */
  source->kind = universals_n_invalid();
}

/**********************************************************************//**
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
    /* Avoid compiler warning about unitialized source.value */
    source.value.advance = NULL;

    return source;
  case VUT_ADVANCE:
    source.value.advance = advance_by_number(value);
    if (source.value.advance != NULL) {
      return source;
    }
    break;
 case VUT_TECHFLAG:
    source.value.techflag = value;
    return source;
  case VUT_GOVERNMENT:
    source.value.govern = government_by_number(value);
    if (source.value.govern != NULL) {
      return source;
    }
    break;
  case VUT_ACHIEVEMENT:
    source.value.achievement = achievement_by_number(value);
    if (source.value.achievement != NULL) {
      return source;
    }
    break;
  case VUT_STYLE:
    source.value.style = style_by_number(value);
    if (source.value.style != NULL) {
      return source;
    }
    break;
  case VUT_IMPROVEMENT:
    source.value.building = improvement_by_number(value);
    if (source.value.building != NULL) {
      return source;
    }
    break;
  case VUT_IMPR_GENUS:
    source.value.impr_genus = value;
    return source;
  case VUT_EXTRA:
    source.value.extra = extra_by_number(value);
    return source;
  case VUT_GOOD:
    source.value.good = goods_by_number(value);
    return source;
  case VUT_TERRAIN:
    source.value.terrain = terrain_by_number(value);
    if (source.value.terrain != NULL) {
      return source;
    }
    break;
  case VUT_TERRFLAG:
    source.value.terrainflag = value;
    return source;
  case VUT_NATION:
    source.value.nation = nation_by_number(value);
    if (source.value.nation != NULL) {
      return source;
    }
    break;
  case VUT_NATIONGROUP:
    source.value.nationgroup = nation_group_by_number(value);
    if (source.value.nationgroup != NULL) {
      return source;
    }
    break;
  case VUT_DIPLREL:
    source.value.diplrel = value;
    if (source.value.diplrel != diplrel_other_invalid()) {
      return source;
    }
    break;
  case VUT_NATIONALITY:
    source.value.nationality = nation_by_number(value);
    if (source.value.nationality != NULL) {
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
  case VUT_MINVETERAN:
    source.value.minveteran = value;
    return source;
  case VUT_UNITSTATE:
    source.value.unit_state = value;
    return source;
  case VUT_MINMOVES:
    source.value.minmoves = value;
    return source;
  case VUT_MINHP:
    source.value.min_hit_points = value;
    return source;
  case VUT_AGE:
    source.value.age = value;
    return source;
  case VUT_MINTECHS:
    source.value.min_techs = value;
    return source;
  case VUT_ACTION:
    source.value.action = action_by_number(value);
    if (source.value.action != NULL) {
      return source;
    }
    break;
  case VUT_OTYPE:
    source.value.outputtype = value;
    return source;
  case VUT_SPECIALIST:
    source.value.specialist = specialist_by_number(value);
    return source;
  case VUT_MINSIZE:
    source.value.minsize = value;
    return source;
  case VUT_MINCULTURE:
    source.value.minculture = value;
    return source;
  case VUT_AI_LEVEL:
    source.value.ai_level = value;
    return source;
  case VUT_MAXTILEUNITS:
    source.value.max_tile_units = value;
    return source;
  case VUT_TERRAINCLASS:
    source.value.terrainclass = value;
    return source;
  case VUT_BASEFLAG:
    source.value.baseflag = value;
    return source;
  case VUT_ROADFLAG:
    source.value.roadflag = value;
    return source;
  case VUT_EXTRAFLAG:
    source.value.extraflag = value;
    return source;
  case VUT_MINYEAR:
    source.value.minyear = value;
    return source;
  case VUT_MINCALFRAG:
    source.value.mincalfrag = value;
    return source;
  case VUT_TOPO:
    source.value.topo_property = value;
    return source;
  case VUT_SERVERSETTING:
    source.value.ssetval = value;
    return source;
  case VUT_TERRAINALTER:
    source.value.terrainalter = value;
    return source;
  case VUT_CITYTILE:
    source.value.citytile = value;
    return source;
  case VUT_COUNT:
    break;
  }

  /* If we reach here there's been an error. */
  source.kind = universals_n_invalid();
  /* Avoid compiler warning about unitialized source.value */
  source.value.advance = NULL;

  return source;
}

/**********************************************************************//**
  Extract universal structure into its components for serialization;
  the opposite of universal_by_number().
**************************************************************************/
void universal_extraction(const struct universal *source,
			  int *kind, int *value)
{
  *kind = source->kind;
  *value = universal_number(source);
}

/**********************************************************************//**
  Return the universal number of the constituent.
**************************************************************************/
int universal_number(const struct universal *source)
{
  switch (source->kind) {
  case VUT_NONE:
    return 0;
  case VUT_ADVANCE:
    return advance_number(source->value.advance);
  case VUT_TECHFLAG:
    return source->value.techflag;
  case VUT_GOVERNMENT:
    return government_number(source->value.govern);
  case VUT_ACHIEVEMENT:
    return achievement_number(source->value.achievement);
  case VUT_STYLE:
    return style_number(source->value.style);
  case VUT_IMPROVEMENT:
    return improvement_number(source->value.building);
  case VUT_IMPR_GENUS:
    return source->value.impr_genus;
  case VUT_EXTRA:
    return extra_number(source->value.extra);
  case VUT_GOOD:
    return goods_number(source->value.good);
  case VUT_TERRAIN:
    return terrain_number(source->value.terrain);
  case VUT_TERRFLAG:
    return source->value.terrainflag;
  case VUT_NATION:
    return nation_number(source->value.nation);
  case VUT_NATIONGROUP:
    return nation_group_number(source->value.nationgroup);
  case VUT_NATIONALITY:
    return nation_number(source->value.nationality);
  case VUT_DIPLREL:
    return source->value.diplrel;
  case VUT_UTYPE:
    return utype_number(source->value.utype);
  case VUT_UTFLAG:
    return source->value.unitflag;
  case VUT_UCLASS:
    return uclass_number(source->value.uclass);
  case VUT_UCFLAG:
    return source->value.unitclassflag;
  case VUT_MINVETERAN:
    return source->value.minveteran;
  case VUT_UNITSTATE:
    return source->value.unit_state;
  case VUT_MINMOVES:
    return source->value.minmoves;
  case VUT_MINHP:
    return source->value.min_hit_points;
  case VUT_AGE:
    return source->value.age;
  case VUT_MINTECHS:
    return source->value.min_techs;
  case VUT_ACTION:
    return action_number(source->value.action);
  case VUT_OTYPE:
    return source->value.outputtype;
  case VUT_SPECIALIST:
    return specialist_number(source->value.specialist);
  case VUT_MINSIZE:
    return source->value.minsize;
  case VUT_MINCULTURE:
    return source->value.minculture;
  case VUT_AI_LEVEL:
    return source->value.ai_level;
  case VUT_MAXTILEUNITS:
    return source->value.max_tile_units;
  case VUT_TERRAINCLASS:
    return source->value.terrainclass;
  case VUT_BASEFLAG:
    return source->value.baseflag;
  case VUT_ROADFLAG:
    return source->value.roadflag;
  case VUT_EXTRAFLAG:
    return source->value.extraflag;
  case VUT_MINYEAR:
    return source->value.minyear;
  case VUT_MINCALFRAG:
    return source->value.mincalfrag;
  case VUT_TOPO:
    return source->value.topo_property;
  case VUT_SERVERSETTING:
    return source->value.ssetval;
  case VUT_TERRAINALTER:
    return source->value.terrainalter;
  case VUT_CITYTILE:
    return source->value.citytile;
  case VUT_COUNT:
    break;
  }

  /* If we reach here there's been an error. */
  fc_assert_msg(FALSE, "universal_number(): invalid source kind %d.",
                source->kind);
  return 0;
}


/**********************************************************************//**
  Returns the given requirement as a formatted string ready for printing.
  Does not care about the 'quiet' property.
**************************************************************************/
const char *req_to_fstring(const struct requirement *req)
{
  struct astring printable_req = ASTRING_INIT;

  astr_set(&printable_req, "%s%s %s %s%s",
           req->survives ? "surviving " : "",
           req_range_name(req->range),
           universal_type_rule_name(&req->source),
           req->present ? "" : "!",
           universal_rule_name(&req->source));

  return astr_str(&printable_req);
}

/**********************************************************************//**
  Parse a requirement type and value string into a requirement structure.
  Returns the invalid element for enum universal_n on error. Passing in a
  NULL type is considered VUT_NONE (not an error).

  Pass this some values like "Building", "Factory".
**************************************************************************/
struct requirement req_from_str(const char *type, const char *range,
				bool survives, bool present, bool quiet,
				const char *value)
{
  struct requirement req;
  bool invalid;
  const char *error = NULL;

  req.source = universal_by_rule_name(type, value);

  invalid = !universals_n_is_valid(req.source.kind);
  if (invalid) {
    error = "bad type or name";
  } else {
    /* Scan the range string to find the range.  If no range is given a
     * default fallback is used rather than giving an error. */
    req.range = req_range_by_name(range, fc_strcasecmp);
    if (!req_range_is_valid(req.range)) {
      switch (req.source.kind) {
      case VUT_NONE:
      case VUT_COUNT:
        break;
      case VUT_IMPROVEMENT:
      case VUT_IMPR_GENUS:
      case VUT_EXTRA:
      case VUT_GOOD:
      case VUT_TERRAIN:
      case VUT_TERRFLAG:
      case VUT_UTYPE:
      case VUT_UTFLAG:
      case VUT_UCLASS:
      case VUT_UCFLAG:
      case VUT_MINVETERAN:
      case VUT_UNITSTATE:
      case VUT_MINMOVES:
      case VUT_MINHP:
      case VUT_AGE:
      case VUT_ACTION:
      case VUT_OTYPE:
      case VUT_SPECIALIST:
      case VUT_TERRAINCLASS:
      case VUT_BASEFLAG:
      case VUT_ROADFLAG:
      case VUT_EXTRAFLAG:
      case VUT_TERRAINALTER:
      case VUT_CITYTILE:
      case VUT_MAXTILEUNITS:
        req.range = REQ_RANGE_LOCAL;
        break;
      case VUT_MINSIZE:
      case VUT_MINCULTURE:
      case VUT_NATIONALITY:
        req.range = REQ_RANGE_CITY;
        break;
      case VUT_GOVERNMENT:
      case VUT_ACHIEVEMENT:
      case VUT_STYLE:
      case VUT_ADVANCE:
      case VUT_TECHFLAG:
      case VUT_NATION:
      case VUT_NATIONGROUP:
      case VUT_DIPLREL:
      case VUT_AI_LEVEL:
        req.range = REQ_RANGE_PLAYER;
        break;
      case VUT_MINYEAR:
      case VUT_MINCALFRAG:
      case VUT_TOPO:
      case VUT_MINTECHS:
      case VUT_SERVERSETTING:
        req.range = REQ_RANGE_WORLD;
        break;
      }
    }

    req.survives = survives;
    req.present = present;
    req.quiet = quiet;

    /* These checks match what combinations are supported inside
     * is_req_active(). However, it's only possible to do basic checks,
     * not anything that might depend on the rest of the ruleset which
     * might not have been loaded yet. */
    switch (req.source.kind) {
    case VUT_TERRAIN:
    case VUT_EXTRA:
    case VUT_TERRAINCLASS:
    case VUT_TERRFLAG:
    case VUT_BASEFLAG:
    case VUT_ROADFLAG:
    case VUT_EXTRAFLAG:
      invalid = (req.range != REQ_RANGE_LOCAL
                 && req.range != REQ_RANGE_CADJACENT
                 && req.range != REQ_RANGE_ADJACENT
                 && req.range != REQ_RANGE_CITY
                 && req.range != REQ_RANGE_TRADEROUTE);
      break;
    case VUT_ADVANCE:
    case VUT_TECHFLAG:
    case VUT_ACHIEVEMENT:
    case VUT_MINTECHS:
      invalid = (req.range < REQ_RANGE_PLAYER);
      break;
    case VUT_GOVERNMENT:
    case VUT_AI_LEVEL:
    case VUT_STYLE:
      invalid = (req.range != REQ_RANGE_PLAYER);
      break;
    case VUT_MINSIZE:
    case VUT_NATIONALITY:
    case VUT_GOOD:
      invalid = (req.range != REQ_RANGE_CITY
                 && req.range != REQ_RANGE_TRADEROUTE);
      break;
    case VUT_MINCULTURE:
      invalid = (req.range != REQ_RANGE_CITY
                 && req.range != REQ_RANGE_TRADEROUTE
                 && req.range != REQ_RANGE_PLAYER
                 && req.range != REQ_RANGE_TEAM
                 && req.range != REQ_RANGE_ALLIANCE
                 && req.range != REQ_RANGE_WORLD);
      break;
    case VUT_DIPLREL:
      invalid = (req.range != REQ_RANGE_LOCAL
                 && req.range != REQ_RANGE_PLAYER
                 && req.range != REQ_RANGE_TEAM
                 && req.range != REQ_RANGE_ALLIANCE
                 && req.range != REQ_RANGE_WORLD)
                /* Non local foreign makes no sense. */
                || (req.source.value.diplrel == DRO_FOREIGN
                    && req.range != REQ_RANGE_LOCAL);
      break;
    case VUT_NATION:
    case VUT_NATIONGROUP:
      invalid = (req.range != REQ_RANGE_PLAYER
                 && req.range != REQ_RANGE_TEAM
                 && req.range != REQ_RANGE_ALLIANCE
                 && req.range != REQ_RANGE_WORLD);
      break;
    case VUT_UTYPE:
    case VUT_UTFLAG:
    case VUT_UCLASS:
    case VUT_UCFLAG:
    case VUT_MINVETERAN:
    case VUT_UNITSTATE:
    case VUT_MINMOVES:
    case VUT_MINHP:
    case VUT_ACTION:
    case VUT_OTYPE:
    case VUT_SPECIALIST:
    case VUT_TERRAINALTER: /* XXX could in principle support C/ADJACENT */
      invalid = (req.range != REQ_RANGE_LOCAL);
      break;
    case VUT_CITYTILE:
    case VUT_MAXTILEUNITS:
      invalid = (req.range != REQ_RANGE_LOCAL
                 && req.range != REQ_RANGE_CADJACENT
                 && req.range != REQ_RANGE_ADJACENT);
      break;
    case VUT_MINYEAR:
    case VUT_MINCALFRAG:
    case VUT_TOPO:
    case VUT_SERVERSETTING:
      invalid = (req.range != REQ_RANGE_WORLD);
      break;
    case VUT_AGE:
      /* FIXME: could support TRADEROUTE, TEAM, etc */
      invalid = (req.range != REQ_RANGE_LOCAL
                 && req.range != REQ_RANGE_CITY
                 && req.range != REQ_RANGE_PLAYER);
      break;
    case VUT_IMPR_GENUS:
      /* TODO: Support other ranges too. */
      invalid = req.range != REQ_RANGE_LOCAL;
      break;
    case VUT_IMPROVEMENT:
      /* Valid ranges depend on the building genus (wonder/improvement),
       * which might not have been loaded from the ruleset yet.
       * So we allow anything here, and do a proper check once ruleset
       * loading is complete, in sanity_check_req_individual(). */
    case VUT_NONE:
      invalid = FALSE;
      break;
    case VUT_COUNT:
      break;
    }
    if (invalid) {
      error = "bad range";
    }
  }

  if (!invalid) {
    /* Check 'survives'. */
    switch (req.source.kind) {
    case VUT_IMPROVEMENT:
      /* See buildings_in_range(). */
      invalid = survives && req.range <= REQ_RANGE_CONTINENT;
      break;
    case VUT_NATION:
    case VUT_ADVANCE:
      invalid = survives && req.range != REQ_RANGE_WORLD;
      break;
    case VUT_IMPR_GENUS:
    case VUT_GOVERNMENT:
    case VUT_TERRAIN:
    case VUT_UTYPE:
    case VUT_UTFLAG:
    case VUT_UCLASS:
    case VUT_UCFLAG:
    case VUT_MINVETERAN:
    case VUT_UNITSTATE:
    case VUT_MINMOVES:
    case VUT_MINHP:
    case VUT_AGE:
    case VUT_ACTION:
    case VUT_OTYPE:
    case VUT_SPECIALIST:
    case VUT_MINSIZE:
    case VUT_MINCULTURE:
    case VUT_AI_LEVEL:
    case VUT_TERRAINCLASS:
    case VUT_MINYEAR:
    case VUT_MINCALFRAG:
    case VUT_TOPO:
    case VUT_SERVERSETTING:
    case VUT_TERRAINALTER:
    case VUT_CITYTILE:
    case VUT_TERRFLAG:
    case VUT_NATIONALITY:
    case VUT_BASEFLAG:
    case VUT_ROADFLAG:
    case VUT_EXTRAFLAG:
    case VUT_EXTRA:
    case VUT_GOOD:
    case VUT_TECHFLAG:
    case VUT_ACHIEVEMENT:
    case VUT_NATIONGROUP:
    case VUT_STYLE:
    case VUT_DIPLREL:
    case VUT_MAXTILEUNITS:
    case VUT_MINTECHS:
      /* Most requirements don't support 'survives'. */
      invalid = survives;
      break;
    case VUT_NONE:
    case VUT_COUNT:
      break;
    }
    if (invalid) {
      error = "bad 'survives'";
    }
  }

  if (invalid) {
    log_error("Invalid requirement %s | %s | %s | %s | %s: %s",
              type, range, survives ? "survives" : "",
              present ? "present" : "", value, error);
    req.source.kind = universals_n_invalid();
  }

  return req;
}

/**********************************************************************//**
  Set the values of a req from serializable integers.  This is the opposite
  of req_get_values.
**************************************************************************/
struct requirement req_from_values(int type, int range,
				   bool survives, bool present, bool quiet,
				   int value)
{
  struct requirement req;

  req.source = universal_by_number(type, value);
  req.range = range;
  req.survives = survives;
  req.present = present;
  req.quiet = quiet;

  return req;
}

/**********************************************************************//**
  Return the value of a req as a serializable integer.  This is the opposite
  of req_set_value.
**************************************************************************/
void req_get_values(const struct requirement *req,
		    int *type, int *range,
		    bool *survives, bool *present, bool *quiet,
		    int *value)
{
  universal_extraction(&req->source, type, value);
  *range = req->range;
  *survives = req->survives;
  *present = req->present;
  *quiet = req->quiet;
}

/**********************************************************************//**
  Returns TRUE if req1 and req2 are equal.
  Does not care if one is quiet and the other not.
**************************************************************************/
bool are_requirements_equal(const struct requirement *req1,
			    const struct requirement *req2)
{
  return (are_universals_equal(&req1->source, &req2->source)
	  && req1->range == req2->range
	  && req1->survives == req2->survives
	  && req1->present == req2->present);
}

/**********************************************************************//**
  Returns TRUE if req1 and req2 directly negate each other.
**************************************************************************/
static bool are_requirements_opposites(const struct requirement *req1,
                                       const struct requirement *req2)
{
  return (are_universals_equal(&req1->source, &req2->source)
          && req1->range == req2->range
          && req1->survives == req2->survives
          && req1->present != req2->present);
}

/**********************************************************************//**
  Returns TRUE if the specified building requirement contradicts the
  specified building genus requirement.
**************************************************************************/
static bool impr_contra_genus(const struct requirement *impr_req,
                              const struct requirement *genus_req)
{
  /* The input is sane. */
  fc_assert_ret_val(impr_req->source.kind == VUT_IMPROVEMENT, FALSE);
  fc_assert_ret_val(genus_req->source.kind == VUT_IMPR_GENUS, FALSE);

  if (impr_req->range == REQ_RANGE_LOCAL
      && genus_req->range == REQ_RANGE_LOCAL) {
    /* Applies to the same target building. */

    if (impr_req->present && !genus_req->present) {
      /* The target building can't not have the genus it has. */
      return (impr_req->source.value.building->genus
              == genus_req->source.value.impr_genus);
    }

    if (impr_req->present && genus_req->present) {
      /* The target building can't have another genus than it has. */
      return (impr_req->source.value.building->genus
              != genus_req->source.value.impr_genus);
    }
  }

  /* No special knowledge. */
  return FALSE;
}

/**********************************************************************//**
  Returns TRUE if the specified nation requirement contradicts the
  specified nation group requirement.
**************************************************************************/
static bool nation_contra_group(const struct requirement *nation_req,
                                const struct requirement *group_req)
{
  /* The input is sane. */
  fc_assert_ret_val(nation_req->source.kind == VUT_NATION, FALSE);
  fc_assert_ret_val(group_req->source.kind == VUT_NATIONGROUP, FALSE);

  if (nation_req->range == REQ_RANGE_PLAYER
      && group_req->range == REQ_RANGE_PLAYER) {
    /* Applies to the same target building. */

    if (nation_req->present && !group_req->present) {
      /* The target nation can't be in the group. */
      return nation_is_in_group(nation_req->source.value.nation,
                                group_req->source.value.nationgroup);
    }
  }

  /* No special knowledge. */
  return FALSE;
}

/**********************************************************************//**
  Returns TRUE if req1 and req2 contradicts each other.

  TODO: If information about what entity each requirement type will be
  evaluated against is passed it will become possible to detect stuff like
  that an unclaimed tile contradicts all DiplRel requirements against it.
**************************************************************************/
bool are_requirements_contradictions(const struct requirement *req1,
                                     const struct requirement *req2)
{
  if (are_requirements_opposites(req1, req2)) {
    /* The exact opposite. */
    return TRUE;
  }

  switch (req1->source.kind) {
  case VUT_IMPROVEMENT:
    if (req2->source.kind == VUT_IMPR_GENUS) {
      return impr_contra_genus(req1, req2);
    }

    /* No special knowledge. */
    return FALSE;
    break;
  case VUT_IMPR_GENUS:
    if (req2->source.kind == VUT_IMPROVEMENT) {
      return impr_contra_genus(req2, req1);
    }

    /* No special knowledge. */
    return FALSE;
    break;
  case VUT_DIPLREL:
    if (req2->source.kind != VUT_DIPLREL) {
      /* Finding contradictions across requirement kinds aren't supported
       * for DiplRel requirements. */
      return FALSE;
    } else {
      /* Use the special knowledge about DiplRel requirements to find
       * contradictions. */

      bv_diplrel_all_reqs req1_contra;
      int req2_pos;

      req1_contra = diplrel_req_contradicts(req1);
      req2_pos = requirement_diplrel_ereq(req2->source.value.diplrel,
                                          req2->range,
                                          req2->present);

      return BV_ISSET(req1_contra, req2_pos);
    }
    break;
  case VUT_MINMOVES:
    if (req2->source.kind != VUT_MINMOVES) {
      /* Finding contradictions across requirement kinds aren't supported
       * for MinMoveFrags requirements. */
      return FALSE;
    } else if (req1->present == req2->present) {
      /* No contradiction possible. */
      return FALSE;
    } else {
      /* Number of move fragments left can't be larger than the number
       * required to be present and smaller than the number required to not
       * be present when the number required to be present is smaller than
       * the number required to not be present. */
      if (req1->present) {
        return req1->source.value.minmoves >= req2->source.value.minmoves;
      } else {
        return req1->source.value.minmoves <= req2->source.value.minmoves;
      }
    }
    break;
  case VUT_NATION:
    if (req2->source.kind == VUT_NATIONGROUP) {
      return nation_contra_group(req1, req2);
    }

    /* No special knowledge. */
    return FALSE;
    break;
  case VUT_NATIONGROUP:
    if (req2->source.kind == VUT_NATION) {
      return nation_contra_group(req2, req1);
    }

    /* No special knowledge. */
    return FALSE;
    break;
  default:
    /* No special knowledge exists. The requirements aren't the exact
     * opposite of each other per the initial check. */
    return FALSE;
    break;
  }
}

/**********************************************************************//**
  Returns TRUE if the given requirement contradicts the given requirement
  vector.
**************************************************************************/
bool does_req_contradicts_reqs(const struct requirement *req,
                               const struct requirement_vector *vec)
{
  /* If the requirement is contradicted by any requirement in the vector it
   * contradicts the entire requirement vector. */
  requirement_vector_iterate(vec, preq) {
    if (are_requirements_contradictions(req, preq)) {
      return TRUE;
    }
  } requirement_vector_iterate_end;

  /* Not a singe requirement in the requirement vector is contradicted be
   * the specified requirement. */
  return FALSE;
}

/* No self contradictions in the requirement vector. */
#define NO_CONTRADICTIONS (-1)

/**********************************************************************//**
  Returns the first requirement in a requirement vector that contradicts
  with other requirements in the same requirement vector.
**************************************************************************/
static int first_contradiction(const struct requirement_vector *vec)
{
  int i;

  for (i = 0; i < requirement_vector_size(vec); i++) {
    struct requirement *preq = requirement_vector_get(vec, i);

    if (does_req_contradicts_reqs(preq, vec)) {
      return i;
    }
  }

  return NO_CONTRADICTIONS;
}

/**********************************************************************//**
  Clean up self contradictions from a requirement vector.

  When two requirements conflicts the earliest requirement is removed.
  This allows requirement adjustment code to append the new requirement(s)
  and leave the contradiction clean up to this function.
**************************************************************************/
void requirement_vector_contradiction_clean(struct requirement_vector *vec)
{
  int conflict;

  while (NO_CONTRADICTIONS != (conflict = first_contradiction(vec))) {
    requirement_vector_remove(vec, conflict);
  }
}

/**********************************************************************//**
  Returns TRUE if players are in the same requirements range.
**************************************************************************/
static inline bool players_in_same_range(const struct player *pplayer1,
                                         const struct player *pplayer2,
                                         enum req_range range)
{
  switch (range) {
  case REQ_RANGE_WORLD:
    return TRUE;
  case REQ_RANGE_ALLIANCE:
    return pplayers_allied(pplayer1, pplayer2);
  case REQ_RANGE_TEAM:
    return players_on_same_team(pplayer1, pplayer2);
  case REQ_RANGE_PLAYER:
    return pplayer1 == pplayer2;
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CITY:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);
  return FALSE;
}

/**********************************************************************//**
  Returns the number of total world buildings (this includes buildings
  that have been destroyed).
**************************************************************************/
static int num_world_buildings_total(const struct impr_type *building)
{
  if (is_great_wonder(building)) {
    return (great_wonder_is_built(building)
            || great_wonder_is_destroyed(building) ? 1 : 0);
  } else {
    log_error("World-ranged requirements are only supported for wonders.");
    return 0;
  }
}

/**********************************************************************//**
  Returns the number of buildings of a certain type in the world.
**************************************************************************/
static int num_world_buildings(const struct impr_type *building)
{
  if (is_great_wonder(building)) {
    return (great_wonder_is_built(building) ? 1 : 0);
  } else {
    log_error("World-ranged requirements are only supported for wonders.");
    return 0;
  }
}

/**********************************************************************//**
  Returns whether a building of a certain type has ever been built by
  pplayer, even if it has subsequently been destroyed.

  Note: the implementation of this is no different in principle from
  num_world_buildings_total(), but the semantics are different because
  unlike a great wonder, a small wonder could be destroyed and rebuilt
  many times, requiring return of values >1, but there's no record kept
  to support that. Fortunately, the only current caller doesn't need the
  exact number.
**************************************************************************/
static bool player_has_ever_built(const struct player *pplayer,
                                  const struct impr_type *building)
{
  if (is_wonder(building)) {
    return (wonder_is_built(pplayer, building)
            || wonder_is_lost(pplayer, building) ? TRUE : FALSE);
  } else {
    log_error("Player-ranged requirements are only supported for wonders.");
    return FALSE;
  }
}

/**********************************************************************//**
  Returns the number of buildings of a certain type owned by plr.
**************************************************************************/
static int num_player_buildings(const struct player *pplayer,
				const struct impr_type *building)
{
  if (is_wonder(building)) {
    return (wonder_is_built(pplayer, building) ? 1 : 0);
  } else {
    log_error("Player-ranged requirements are only supported for wonders.");
    return 0;
  }
}

/**********************************************************************//**
  Returns the number of buildings of a certain type on a continent.
**************************************************************************/
static int num_continent_buildings(const struct player *pplayer,
				   int continent,
				   const struct impr_type *building)
{
  if (is_wonder(building)) {
    const struct city *pcity;

    pcity = city_from_wonder(pplayer, building);
    if (pcity && pcity->tile && tile_continent(pcity->tile) == continent) {
      return 1;
    }
  } else {
    log_error("Island-ranged requirements are only supported for wonders.");
  }
  return 0;
}

/**********************************************************************//**
  Returns the number of buildings of a certain type in a city.
**************************************************************************/
static int num_city_buildings(const struct city *pcity,
			      const struct impr_type *building)
{
  return (city_has_building(pcity, building) ? 1 : 0);
}

/**********************************************************************//**
  Are there any source buildings within range of the target that are not
  obsolete?

  The target gives the type of the target.  The exact target is a player,
  city, or building specified by the target_xxx arguments.

  The range gives the range of the requirement.

  "Survives" specifies whether the requirement allows destroyed sources.
  If set then all source buildings ever built are counted; if not then only
  living buildings are counted.

  source gives the building type of the source in question.
**************************************************************************/
static enum fc_tristate
is_building_in_range(const struct player *target_player,
                     const struct city *target_city,
                     const struct impr_type *target_building,
                     enum req_range range,
                     bool survives,
                     const struct impr_type *source)
{
  /* Check if it's certain that the building is obsolete given the
   * specification we have */
  if (improvement_obsolete(target_player, source, target_city)) {
    return TRI_NO;
  }

  if (survives) {

    /* Check whether condition has ever held, using cached information. */
    switch (range) {
    case REQ_RANGE_WORLD:
      return BOOL_TO_TRISTATE(num_world_buildings_total(source) > 0);
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_TEAM:
      if (target_player == NULL) {
        return TRI_MAYBE;
      }
      players_iterate_alive(plr2) {
        if (players_in_same_range(target_player, plr2, range)
            && player_has_ever_built(plr2, source)) {
          return TRI_YES;
        }
      } players_iterate_alive_end;
      return TRI_NO;
    case REQ_RANGE_PLAYER:
      if (target_player == NULL) {
        return TRI_MAYBE;
      }
      return BOOL_TO_TRISTATE(player_has_ever_built(target_player, source));
    case REQ_RANGE_CONTINENT:
    case REQ_RANGE_TRADEROUTE:
    case REQ_RANGE_CITY:
    case REQ_RANGE_LOCAL:
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
      /* There is no sources cache for this. */
      log_error("Surviving requirements are only supported at "
                "World/Alliance/Team/Player ranges.");
      return TRI_NO;
    case REQ_RANGE_COUNT:
      break;
    }

  } else {

    /* Non-surviving requirement. */
    switch (range) {
    case REQ_RANGE_WORLD:
      return BOOL_TO_TRISTATE(num_world_buildings(source) > 0);
    case REQ_RANGE_ALLIANCE:
    case REQ_RANGE_TEAM:
      if (target_player == NULL) {
        return TRI_MAYBE;
      }
      players_iterate_alive(plr2) {
        if (players_in_same_range(target_player, plr2, range)
            && num_player_buildings(plr2, source) > 0) {
          return TRI_YES;
        }
      } players_iterate_alive_end;
      return TRI_NO;
    case REQ_RANGE_PLAYER:
      if (target_player == NULL) {
        return TRI_MAYBE;
      }
      return BOOL_TO_TRISTATE(num_player_buildings(target_player, source) > 0);
    case REQ_RANGE_CONTINENT:
      /* At present, "Continent" effects can affect only
       * cities and units in cities. */
      if (target_player && target_city) {
        int continent = tile_continent(target_city->tile);
        return BOOL_TO_TRISTATE(num_continent_buildings(target_player,
                                                        continent, source) > 0);
      } else {
        return TRI_MAYBE;
      }
    case REQ_RANGE_TRADEROUTE:
      if (target_city) {
        if (num_city_buildings(target_city, source) > 0) {
          return TRI_YES;
        } else {
          trade_partners_iterate(target_city, trade_partner) {
            if (num_city_buildings(trade_partner, source) > 0) {
              return TRI_YES;
            }
          } trade_partners_iterate_end;
        }
        return TRI_NO;
      } else {
        return TRI_MAYBE;
      }
    case REQ_RANGE_CITY:
      if (target_city) {
        return BOOL_TO_TRISTATE(num_city_buildings(target_city, source) > 0);
      } else {
        return TRI_MAYBE;
      }
    case REQ_RANGE_LOCAL:
      if (target_building) {
        if (target_building == source) {
          return TRI_YES;
        } else {
          return TRI_NO;
        }
      } else {
        /* TODO: other local targets */
        return TRI_MAYBE;
      }
    case REQ_RANGE_CADJACENT:
    case REQ_RANGE_ADJACENT:
      return TRI_NO;
    case REQ_RANGE_COUNT:
      break;
    }

  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);
  return TRI_NO;
}

/**********************************************************************//**
  Is there a source tech within range of the target?
**************************************************************************/
static enum fc_tristate is_tech_in_range(const struct player *target_player,
                                         enum req_range range, bool survives,
                                         Tech_type_id tech)
{
  if (survives) {
    fc_assert(range == REQ_RANGE_WORLD);
    return BOOL_TO_TRISTATE(game.info.global_advances[tech]);
  }

  /* Not a 'surviving' requirement. */
  switch (range) {
  case REQ_RANGE_PLAYER:
    if (NULL != target_player) {
      return BOOL_TO_TRISTATE(TECH_KNOWN == research_invention_state
                                (research_get(target_player), tech));
    } else {
      return TRI_MAYBE;
    }
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
   if (NULL == target_player) {
     return TRI_MAYBE;
   }
   players_iterate_alive(plr2) {
     if (players_in_same_range(target_player, plr2, range)) {
       if (research_invention_state(research_get(plr2), tech)
           == TECH_KNOWN) {
         return TRI_YES;
       }
     }
   } players_iterate_alive_end;

   return TRI_NO;
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a tech with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_techflag_in_range(const struct player *target_player,
                                             enum req_range range,
                                             enum tech_flag_id techflag)
{
  switch (range) {
  case REQ_RANGE_PLAYER:
    if (NULL != target_player) {
      return BOOL_TO_TRISTATE(player_knows_techs_with_flag(target_player, techflag));
    } else {
      return TRI_MAYBE;
    }
    break;
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
    if (NULL == target_player) {
      return TRI_MAYBE;
    }
    players_iterate_alive(plr2) {
      if (players_in_same_range(target_player, plr2, range)
          && player_knows_techs_with_flag(plr2, techflag)) {
        return TRI_YES;
      }
    } players_iterate_alive_end;
    return TRI_NO;
  case REQ_RANGE_WORLD:
    players_iterate(pplayer) {
      if (player_knows_techs_with_flag(pplayer, techflag)) {
        return TRI_YES;
      }
    } players_iterate_end;

    return TRI_NO;
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is city or player with at least minculture culture in range?
**************************************************************************/
static enum fc_tristate is_minculture_in_range(const struct city *target_city,
                                               const struct player *target_player,
                                               enum req_range range,
                                               int minculture)
{
  switch (range) {
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(city_culture(target_city) >= minculture);
  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    if (city_culture(target_city) >= minculture) {
      return TRI_YES;
    } else {
      trade_partners_iterate(target_city, trade_partner) {
        if (city_culture(trade_partner) >= minculture) {
          return TRI_YES;
        }
      } trade_partners_iterate_end;
      return TRI_MAYBE;
    }
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
    if (NULL == target_player) {
      return TRI_MAYBE;
    }
    players_iterate_alive(plr2) {
      if (players_in_same_range(target_player, plr2, range)) {
        if (player_culture(plr2) >= minculture) {
          return TRI_YES;
        }
      }
    } players_iterate_alive_end;
    return TRI_NO;
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a tile with max X units within range of the target?
**************************************************************************/
static enum fc_tristate
is_tile_units_in_range(const struct tile *target_tile, enum req_range range,
                       int maxUnits)
{
  /* TODO: if can't see V_INVIS -> TRI_MAYBE */
  switch (range) {
  case REQ_RANGE_LOCAL:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(unit_list_size(target_tile->units) <= maxUnits);
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    if (unit_list_size(target_tile->units) <= maxUnits) {
      return TRI_YES;
    }
    cardinal_adjc_iterate(&(wld.map), target_tile, adjc_tile) {
      if (unit_list_size(adjc_tile->units) <= maxUnits) {
        return TRI_YES;
      }
    } cardinal_adjc_iterate_end;
    return TRI_NO;
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    if (unit_list_size(target_tile->units) <= maxUnits) {
      return TRI_YES;
    }
    adjc_iterate(&(wld.map), target_tile, adjc_tile) {
      if (unit_list_size(adjc_tile->units) <= maxUnits) {
        return TRI_YES;
      }
    } adjc_iterate_end;
    return TRI_NO;
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a source extra type within range of the target?
**************************************************************************/
static enum fc_tristate is_extra_type_in_range(const struct tile *target_tile,
                                               const struct city *target_city,
                                               enum req_range range, bool survives,
                                               struct extra_type *pextra)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has extra of requested type. */
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_extra(target_tile, pextra));
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_extra(target_tile, pextra)
                            || is_extra_card_near(target_tile, pextra));
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_extra(target_tile, pextra)
                            || is_extra_near_tile(target_tile, pextra));
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_extra(ptile, pextra)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    return TRI_NO;

  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_extra(ptile, pextra)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;
    trade_partners_iterate(target_city, trade_partner) {
      city_tile_iterate(city_map_radius_sq_get(trade_partner),
                        city_tile(trade_partner), ptile) {
        if (tile_has_extra(ptile, pextra)) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
    } trade_partners_iterate_end;

    return TRI_NO;

  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a source goods type within range of the target?
**************************************************************************/
static enum fc_tristate is_goods_type_in_range(const struct tile *target_tile,
                                               const struct city *target_city,
                                               enum req_range range, bool survives,
                                               struct goods_type *pgood)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CITY:
    /* The requirement is filled if the tile has extra of requested type. */
    if (!target_city) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(city_receives_goods(target_city, pgood));
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a source tile within range of the target?
**************************************************************************/
static enum fc_tristate is_terrain_in_range(const struct tile *target_tile,
                                            const struct city *target_city,
                                            enum req_range range, bool survives,
                                            const struct terrain *pterrain)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has the terrain. */
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return pterrain && tile_terrain(target_tile) == pterrain;
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(pterrain && is_terrain_card_near(target_tile, pterrain, TRUE));
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(pterrain && is_terrain_near_tile(target_tile, pterrain, TRUE));
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    if (pterrain != NULL) {
      city_tile_iterate(city_map_radius_sq_get(target_city),
                        city_tile(target_city), ptile) {
        if (tile_terrain(ptile) == pterrain) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
    }
    return TRI_NO;
  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    if (pterrain != NULL) {
      city_tile_iterate(city_map_radius_sq_get(target_city),
                        city_tile(target_city), ptile) {
        if (tile_terrain(ptile) == pterrain) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
      trade_partners_iterate(target_city, trade_partner) {
        city_tile_iterate(city_map_radius_sq_get(trade_partner),
                          city_tile(trade_partner), ptile) {
          if (tile_terrain(ptile) == pterrain) {
            return TRI_YES;
          }
        } city_tile_iterate_end;
      } trade_partners_iterate_end;
    }
    return TRI_NO;
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a source terrain class within range of the target?
**************************************************************************/
static enum fc_tristate is_terrain_class_in_range(const struct tile *target_tile,
                                                  const struct city *target_city,
                                                  enum req_range range, bool survives,
                                                  enum terrain_class pclass)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has the terrain of correct class. */
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(terrain_type_terrain_class(tile_terrain(target_tile)) == pclass);
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(terrain_type_terrain_class(tile_terrain(target_tile)) == pclass
                            || is_terrain_class_card_near(target_tile, pclass));
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(terrain_type_terrain_class(tile_terrain(target_tile)) == pclass
                            || is_terrain_class_near_tile(target_tile, pclass));
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      const struct terrain *pterrain = tile_terrain(ptile);
      if (pterrain != T_UNKNOWN
          && terrain_type_terrain_class(pterrain) == pclass) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    return TRI_NO;
  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      const struct terrain *pterrain = tile_terrain(ptile);
      if (pterrain != T_UNKNOWN
          && terrain_type_terrain_class(pterrain) == pclass) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    trade_partners_iterate(target_city, trade_partner) {
      city_tile_iterate(city_map_radius_sq_get(trade_partner),
                        city_tile(trade_partner), ptile) {
        const struct terrain *pterrain = tile_terrain(ptile);
        if (pterrain != T_UNKNOWN
            && terrain_type_terrain_class(pterrain) == pclass) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
    } trade_partners_iterate_end;

    return TRI_NO;
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a terrain with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_terrainflag_in_range(const struct tile *target_tile,
                                                const struct city *target_city,
                                                enum req_range range, bool survives,
                                                enum terrain_flag_id terrflag)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is fulfilled if the tile has a terrain with
     * correct flag. */
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(terrain_has_flag(tile_terrain(target_tile),
                                             terrflag));
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(terrain_has_flag(tile_terrain(target_tile),
                                             terrflag)
                            || is_terrain_flag_card_near(target_tile,
                                                         terrflag));
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(terrain_has_flag(tile_terrain(target_tile),
                                             terrflag)
                            || is_terrain_flag_near_tile(target_tile,
                                                         terrflag));
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      const struct terrain *pterrain = tile_terrain(ptile);
      if (pterrain != T_UNKNOWN
          && terrain_has_flag(pterrain, terrflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    return TRI_NO;
  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      const struct terrain *pterrain = tile_terrain(ptile);
      if (pterrain != T_UNKNOWN
          && terrain_has_flag(pterrain, terrflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    trade_partners_iterate(target_city, trade_partner) {
      city_tile_iterate(city_map_radius_sq_get(trade_partner),
                        city_tile(trade_partner), ptile) {
        const struct terrain *pterrain = tile_terrain(ptile);
        if (pterrain != T_UNKNOWN
            && terrain_has_flag(pterrain, terrflag)) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
    } trade_partners_iterate_end;

    return TRI_NO;
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a base with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_baseflag_in_range(const struct tile *target_tile,
                                             const struct city *target_city,
                                             enum req_range range, bool survives,
                                             enum base_flag_id baseflag)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has a base with correct flag. */
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_base_flag(target_tile, baseflag));
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_base_flag(target_tile, baseflag)
                            || is_base_flag_card_near(target_tile, baseflag));
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_base_flag(target_tile, baseflag)
                            || is_base_flag_near_tile(target_tile, baseflag));
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_base_flag(ptile, baseflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    return TRI_NO;
  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_base_flag(ptile, baseflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    trade_partners_iterate(target_city, trade_partner) {
      city_tile_iterate(city_map_radius_sq_get(trade_partner),
                        city_tile(trade_partner), ptile) {
        if (tile_has_base_flag(ptile, baseflag)) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
    } trade_partners_iterate_end;

    return TRI_NO;
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a road with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_roadflag_in_range(const struct tile *target_tile,
                                             const struct city *target_city,
                                             enum req_range range, bool survives,
                                             enum road_flag_id roadflag)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has a road with correct flag. */
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_road_flag(target_tile, roadflag));
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_road_flag(target_tile, roadflag)
                            || is_road_flag_card_near(target_tile, roadflag));
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_road_flag(target_tile, roadflag)
                            || is_road_flag_near_tile(target_tile, roadflag));
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_road_flag(ptile, roadflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    return TRI_NO;
  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_road_flag(ptile, roadflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    trade_partners_iterate(target_city, trade_partner) {
      city_tile_iterate(city_map_radius_sq_get(trade_partner),
                        city_tile(trade_partner), ptile) {
        if (tile_has_road_flag(ptile, roadflag)) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
    } trade_partners_iterate_end;

    return TRI_NO;
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there an extra with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_extraflag_in_range(const struct tile *target_tile,
                                              const struct city *target_city,
                                              enum req_range range, bool survives,
                                              enum extra_flag_id extraflag)
{
  switch (range) {
  case REQ_RANGE_LOCAL:
    /* The requirement is filled if the tile has an extra with correct flag. */
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_extra_flag(target_tile, extraflag));
  case REQ_RANGE_CADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_extra_flag(target_tile, extraflag)
                            || is_extra_flag_card_near(target_tile, extraflag));
  case REQ_RANGE_ADJACENT:
    if (!target_tile) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(tile_has_extra_flag(target_tile, extraflag)
                            || is_extra_flag_near_tile(target_tile, extraflag));
  case REQ_RANGE_CITY:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_extra_flag(ptile, extraflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    return TRI_NO;
  case REQ_RANGE_TRADEROUTE:
    if (!target_city) {
      return TRI_MAYBE;
    }
    city_tile_iterate(city_map_radius_sq_get(target_city),
                      city_tile(target_city), ptile) {
      if (tile_has_extra_flag(ptile, extraflag)) {
        return TRI_YES;
      }
    } city_tile_iterate_end;

    trade_partners_iterate(target_city, trade_partner) {
      city_tile_iterate(city_map_radius_sq_get(trade_partner),
                        city_tile(trade_partner), ptile) {
        if (tile_has_extra_flag(ptile, extraflag)) {
          return TRI_YES;
        }
      } city_tile_iterate_end;
    } trade_partners_iterate_end;

    return TRI_NO;
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a terrain which can support the specified infrastructure
  within range of the target?
**************************************************************************/
static enum fc_tristate is_terrain_alter_possible_in_range(const struct tile *target_tile,
                                                           enum req_range range,
                                                           bool survives,
                                                           enum terrain_alteration alteration)
{
  if (!target_tile) {
    return TRI_MAYBE;
  }

  switch (range) {
  case REQ_RANGE_LOCAL:
    return BOOL_TO_TRISTATE(terrain_can_support_alteration(tile_terrain(target_tile),
                                                           alteration));
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT: /* XXX Could in principle support ADJACENT. */
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a nation within range of the target?
**************************************************************************/
static enum fc_tristate is_nation_in_range(const struct player *target_player,
                                           enum req_range range, bool survives,
                                           const struct nation_type *nation)
{
  switch (range) {
  case REQ_RANGE_PLAYER:
    if (target_player == NULL) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(nation_of_player(target_player) == nation);
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
    if (target_player == NULL) {
      return TRI_MAYBE;
    }
    players_iterate_alive(plr2) {
      if (players_in_same_range(target_player, plr2, range)) {
        if (nation_of_player(plr2) == nation) {
          return TRI_YES;
        }
      }
    } players_iterate_alive_end;
    return TRI_NO;
  case REQ_RANGE_WORLD:
    /* NB: if a player is ever removed outright from the game
     * (e.g. via /remove), rather than just dying, this 'survives'
     * requirement will stop being true for their nation.
     * create_command_newcomer() can also cause this to happen. */
    return BOOL_TO_TRISTATE(NULL != nation->player
                            && (survives || nation->player->is_alive));
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a nation group within range of the target?
**************************************************************************/
static enum fc_tristate
is_nation_group_in_range(const struct player *target_player,
                         enum req_range range, bool survives,
                         const struct nation_group *ngroup)
{
  switch (range) {
  case REQ_RANGE_PLAYER:
    if (target_player == NULL) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(nation_is_in_group(nation_of_player(target_player),
                                               ngroup));
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
    if (target_player == NULL) {
      return TRI_MAYBE;
    }
    players_iterate_alive(plr2) {
      if (players_in_same_range(target_player, plr2, range)) {
        if (nation_is_in_group(nation_of_player(plr2), ngroup)) {
          return TRI_YES;
        }
      }
    } players_iterate_alive_end;
    return TRI_NO;
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a nationality within range of the target?
**************************************************************************/
static enum fc_tristate is_nationality_in_range(const struct city *target_city,
                                                enum req_range range,
                                                const struct nation_type *nationality)
{
  switch (range) {
  case REQ_RANGE_CITY:
    if (target_city == NULL) {
     return TRI_MAYBE;
    }
    citizens_iterate(target_city, slot, count) {
      if (player_slot_get_player(slot)->nation == nationality) {
        return TRI_YES;
      }
    } citizens_iterate_end;

    return TRI_NO;
  case REQ_RANGE_TRADEROUTE:
    if (target_city == NULL) {
      return TRI_MAYBE;
    }
    citizens_iterate(target_city, slot, count) {
      if (player_slot_get_player(slot)->nation == nationality) {
        return TRI_YES;
      }
    } citizens_iterate_end;

    trade_partners_iterate(target_city, trade_partner) {
      citizens_iterate(trade_partner, slot, count) {
        if (player_slot_get_player(slot)->nation == nationality) {
          return TRI_YES;
        }
      } citizens_iterate_end;
    } trade_partners_iterate_end;

    return TRI_NO;
  case REQ_RANGE_PLAYER:
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is the diplomatic state within range of the target?
**************************************************************************/
static enum fc_tristate is_diplrel_in_range(const struct player *target_player,
                                            const struct player *other_player,
                                            enum req_range range,
                                            int diplrel)
{
  switch (range) {
  case REQ_RANGE_PLAYER:
    if (target_player == NULL) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(is_diplrel_to_other(target_player, diplrel));
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
    if (target_player == NULL) {
      return TRI_MAYBE;
    }
    players_iterate_alive(plr2) {
      if (players_in_same_range(target_player, plr2, range)) {
        if (is_diplrel_to_other(plr2, diplrel)) {
          return TRI_YES;
        }
      }
    } players_iterate_alive_end;
    return TRI_NO;
  case REQ_RANGE_LOCAL:
    if (target_player == NULL || other_player == NULL) {
      return TRI_MAYBE;
    }
    return BOOL_TO_TRISTATE(is_diplrel_between(target_player, other_player, diplrel));
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid range %d.", range);

  return TRI_MAYBE;
}

/**********************************************************************//**
  Is there a unit of the given type within range of the target?
**************************************************************************/
static enum fc_tristate is_unittype_in_range(const struct unit_type *target_unittype,
                                             enum req_range range, bool survives,
                                             struct unit_type *punittype)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  return BOOL_TO_TRISTATE(range == REQ_RANGE_LOCAL
                          && (!target_unittype
                              || target_unittype == punittype));
}

/**********************************************************************//**
  Is there a unit with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_unitflag_in_range(const struct unit_type *target_unittype,
                                             enum req_range range, bool survives,
                                             enum unit_type_flag_id unitflag)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  if (range != REQ_RANGE_LOCAL) {
    return TRI_NO;
  }
  if (!target_unittype) {
    return TRI_MAYBE;
  }

  return BOOL_TO_TRISTATE(utype_has_flag(target_unittype, unitflag));
}

/**********************************************************************//**
  Is there a unit with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_unitclass_in_range(const struct unit_type *target_unittype,
                                              enum req_range range, bool survives,
                                              struct unit_class *pclass)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  return BOOL_TO_TRISTATE(range == REQ_RANGE_LOCAL
                          && (!target_unittype
                              || utype_class(target_unittype) == pclass));
}

/**********************************************************************//**
  Is there a unit with the given flag within range of the target?
**************************************************************************/
static enum fc_tristate is_unitclassflag_in_range(const struct unit_type *target_unittype,
                                                  enum req_range range, bool survives,
                                                  enum unit_class_flag_id ucflag)
{
  /* If no target_unittype is given, we allow the req to be met.  This is
   * to allow querying of certain effect types (like the presence of city
   * walls) without actually knowing the target unit. */
  return BOOL_TO_TRISTATE(range == REQ_RANGE_LOCAL
                          && (!target_unittype
                              || uclass_has_flag(utype_class(target_unittype), ucflag)));
}

/**********************************************************************//**
  Is the given property of the unit state true?
**************************************************************************/
static enum fc_tristate is_unit_state(const struct unit *target_unit,
                                      enum req_range range,
                                      bool survives,
                                      enum ustate_prop uprop)
{
  fc_assert_ret_val_msg(range == REQ_RANGE_LOCAL, TRI_NO,
                        "Unsupported range \"%s\"",
                        req_range_name(range));

  /* Could be asked with incomplete data.
   * is_req_active() will handle it based on prob_type. */
  if (target_unit == NULL) {
    return TRI_MAYBE;
  }

  switch (uprop) {
  case USP_TRANSPORTED:
    return BOOL_TO_TRISTATE(target_unit->transporter != NULL);
  case USP_LIVABLE_TILE:
    return BOOL_TO_TRISTATE(
          can_unit_exist_at_tile(&(wld.map), target_unit,
                                 unit_tile(target_unit)));
    break;
  case USP_DOMESTIC_TILE:
    return BOOL_TO_TRISTATE(
          tile_owner(unit_tile(target_unit)) == unit_owner(target_unit));
    break;
  case USP_TRANSPORTING:
    return BOOL_TO_TRISTATE(0 < get_transporter_occupancy(target_unit));
  case USP_HAS_HOME_CITY:
    return BOOL_TO_TRISTATE(target_unit->homecity > 0);
  case USP_NATIVE_TILE:
    return BOOL_TO_TRISTATE(
        is_native_tile(unit_type_get(target_unit), unit_tile(target_unit)));
    break;
  case USP_COUNT:
    fc_assert_msg(uprop != USP_COUNT, "Invalid unit state property.");
    /* Invalid property is unknowable. */
    return TRI_NO;
  }

  /* Should never be reached */
  fc_assert_ret_val_msg(FALSE, TRI_NO,
                        "Unsupported unit property \"%s\"",
                        ustate_prop_name(uprop));
}

/**********************************************************************//**
  Is center of given city in tile. If city is NULL, any city will do.
**************************************************************************/
static bool is_city_in_tile(const struct tile *ptile,
			    const struct city *pcity)
{
  if (pcity == NULL) {
    return tile_city(ptile) != NULL;
  } else {
    return is_city_center(pcity, ptile);
  }
}

/**********************************************************************//**
  Is center of given city in range. If city is NULL, any city will do.
**************************************************************************/
static enum fc_tristate is_citytile_in_range(const struct tile *target_tile,
                                             const struct city *target_city,
                                             enum req_range range,
                                             enum citytile_type citytile)
{
  if (target_tile) {
    if (citytile == CITYT_CENTER) {
      switch (range) {
      case REQ_RANGE_LOCAL:
        return BOOL_TO_TRISTATE(is_city_in_tile(target_tile, target_city));
      case REQ_RANGE_CADJACENT:
        if (is_city_in_tile(target_tile, target_city)) {
          return TRI_YES;
        }
        cardinal_adjc_iterate(&(wld.map), target_tile, adjc_tile) {
          if (is_city_in_tile(adjc_tile, target_city)) {
            return TRI_YES;
          }
        } cardinal_adjc_iterate_end;

        return TRI_NO;
      case REQ_RANGE_ADJACENT:
        if (is_city_in_tile(target_tile, target_city)) {
          return TRI_YES;
        }
        adjc_iterate(&(wld.map), target_tile, adjc_tile) {
          if (is_city_in_tile(adjc_tile, target_city)) {
            return TRI_YES;
          }
        } adjc_iterate_end;

        return TRI_NO;
      case REQ_RANGE_CITY:
      case REQ_RANGE_TRADEROUTE:
      case REQ_RANGE_CONTINENT:
      case REQ_RANGE_PLAYER:
      case REQ_RANGE_TEAM:
      case REQ_RANGE_ALLIANCE:
      case REQ_RANGE_WORLD:
      case REQ_RANGE_COUNT:
	break;
      }

      fc_assert_msg(FALSE, "Invalid range %d for citytile.", range);

      return TRI_MAYBE;
    } else if (citytile == CITYT_CLAIMED) {
      switch (range) {
      case REQ_RANGE_LOCAL:
        return BOOL_TO_TRISTATE(target_tile->owner != NULL);
      case REQ_RANGE_CADJACENT:
        if (target_tile->owner != NULL) {
          return TRI_YES;
        }
        cardinal_adjc_iterate(&(wld.map), target_tile, adjc_tile) {
          if (adjc_tile->owner != NULL) {
            return TRI_YES;
          }
        } cardinal_adjc_iterate_end;

        return TRI_NO;
      case REQ_RANGE_ADJACENT:
        if (target_tile->owner != NULL) {
          return TRI_YES;
        }
        adjc_iterate(&(wld.map), target_tile, adjc_tile) {
          if (adjc_tile->owner != NULL) {
            return TRI_YES;
          }
        } adjc_iterate_end;

        return TRI_NO;
      case REQ_RANGE_CITY:
      case REQ_RANGE_TRADEROUTE:
      case REQ_RANGE_CONTINENT:
      case REQ_RANGE_PLAYER:
      case REQ_RANGE_TEAM:
      case REQ_RANGE_ALLIANCE:
      case REQ_RANGE_WORLD:
      case REQ_RANGE_COUNT:
        break;
      }

      fc_assert_msg(FALSE, "Invalid range %d for citytile.", range);

      return TRI_MAYBE;
    } else {
      /* Not implemented */
      log_error("is_req_active(): citytile %d not supported.",
		citytile);
      return TRI_MAYBE;
    }
  } else {
    return TRI_MAYBE;
  }
}

/**********************************************************************//**
  Has achievement been claimed by someone in range.
**************************************************************************/
static enum fc_tristate
is_achievement_in_range(const struct player *target_player,
                        enum req_range range,
                        const struct achievement *achievement)
{
  if (range == REQ_RANGE_WORLD) {
    return BOOL_TO_TRISTATE(achievement_claimed(achievement));
  } else if (target_player == NULL) {
    return TRI_MAYBE;
  } else if (range == REQ_RANGE_ALLIANCE || range == REQ_RANGE_TEAM) {
    players_iterate_alive(plr2) {
      if (players_in_same_range(target_player, plr2, range)
          && achievement_player_has(achievement, plr2)) {
        return TRI_YES;
      }
    } players_iterate_alive_end;
    return TRI_NO;
  } else if (range == REQ_RANGE_PLAYER) {
    if (achievement_player_has(achievement, target_player)) {
      return TRI_YES;
    } else {
      return TRI_NO;
    }
  } else {
    fc_assert_ret_val_msg(FALSE, TRI_MAYBE,
                          "Illegal range %d for achievement requirement.",
                          range);
  }
}

/**********************************************************************//**
  Checks the requirement to see if it is active on the given target.

  target gives the type of the target
  (player,city,building,tile) give the exact target
  req gives the requirement itself

  Make sure you give all aspects of the target when calling this function:
  for instance if you have TARGET_CITY pass the city's owner as the target
  player as well as the city itself as the target city.
**************************************************************************/
bool is_req_active(const struct player *target_player,
                   const struct player *other_player,
                   const struct city *target_city,
                   const struct impr_type *target_building,
                   const struct tile *target_tile,
                   const struct unit *target_unit,
                   const struct unit_type *target_unittype,
                   const struct output_type *target_output,
                   const struct specialist *target_specialist,
                   const struct action *target_action,
                   const struct requirement *req,
                   const enum   req_problem_type prob_type)
{
  enum fc_tristate eval = TRI_NO;

  /* The supplied unit has a type. Use it if the unit type is missing. */
  if (target_unittype == NULL && target_unit != NULL) {
    target_unittype = unit_type_get(target_unit);
  }

  /* Note the target may actually not exist.  In particular, effects that
   * have a VUT_TERRAIN may often be passed
   * to this function with a city as their target.  In this case the
   * requirement is simply not met. */
  switch (req->source.kind) {
  case VUT_NONE:
    eval = TRI_YES;
    break;
  case VUT_ADVANCE:
    /* The requirement is filled if the player owns the tech. */
    eval = is_tech_in_range(target_player, req->range, req->survives,
                            advance_number(req->source.value.advance));
    break;
 case VUT_TECHFLAG:
    eval = is_techflag_in_range(target_player, req->range,
                                req->source.value.techflag);
    break;
  case VUT_GOVERNMENT:
    /* The requirement is filled if the player is using the government. */
    if (target_player == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = BOOL_TO_TRISTATE(government_of_player(target_player) == req->source.value.govern);
    }
    break;
  case VUT_ACHIEVEMENT:
    eval = is_achievement_in_range(target_player, req->range,
                                   req->source.value.achievement);
    break;
  case VUT_STYLE:
    if (target_player == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = BOOL_TO_TRISTATE(target_player->style == req->source.value.style);
    }
    break;
  case VUT_IMPROVEMENT:
    eval = is_building_in_range(target_player, target_city,
                                target_building,
                                req->range, req->survives,
                                req->source.value.building);
    break;
  case VUT_IMPR_GENUS:
    eval = (target_building ? BOOL_TO_TRISTATE(
                                target_building->genus
                                == req->source.value.impr_genus)
                            : TRI_MAYBE);
    break;
  case VUT_EXTRA:
    eval = is_extra_type_in_range(target_tile, target_city,
                                  req->range, req->survives,
                                  req->source.value.extra);
    break;
  case VUT_GOOD:
    eval = is_goods_type_in_range(target_tile, target_city,
                                  req->range, req->survives,
                                  req->source.value.good);
    break;
  case VUT_TERRAIN:
    eval = is_terrain_in_range(target_tile, target_city,
                               req->range, req->survives,
                               req->source.value.terrain);
    break;
  case VUT_TERRFLAG:
    eval = is_terrainflag_in_range(target_tile, target_city,
                                   req->range, req->survives,
                                   req->source.value.terrainflag);
    break;
  case VUT_NATION:
    eval = is_nation_in_range(target_player, req->range, req->survives,
                              req->source.value.nation);
    break;
  case VUT_NATIONGROUP:
    eval = is_nation_group_in_range(target_player, req->range, req->survives,
                                    req->source.value.nationgroup);
    break;
  case VUT_NATIONALITY:
    eval = is_nationality_in_range(target_city, req->range,
                                   req->source.value.nationality);
    break;
  case VUT_DIPLREL:
    eval = is_diplrel_in_range(target_player, other_player, req->range,
                               req->source.value.diplrel);
    break;
  case VUT_UTYPE:
    if (target_unittype == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = is_unittype_in_range(target_unittype,
                                  req->range, req->survives,
                                  req->source.value.utype);
    }
    break;
  case VUT_UTFLAG:
    eval = is_unitflag_in_range(target_unittype,
				req->range, req->survives,
				req->source.value.unitflag);
    break;
  case VUT_UCLASS:
    if (target_unittype == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = is_unitclass_in_range(target_unittype,
                                   req->range, req->survives,
                                   req->source.value.uclass);
    }
    break;
  case VUT_UCFLAG:
    if (target_unittype == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = is_unitclassflag_in_range(target_unittype,
                                       req->range, req->survives,
                                       req->source.value.unitclassflag);
    }
    break;
  case VUT_MINVETERAN:
    if (target_unit == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval =
        BOOL_TO_TRISTATE(target_unit->veteran >= req->source.value.minveteran);
    }
    break;
  case VUT_UNITSTATE:
    if (target_unit == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = is_unit_state(target_unit,
                           req->range, req->survives,
                           req->source.value.unit_state);
    }
    break;
  case VUT_MINMOVES:
    if (target_unit == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = BOOL_TO_TRISTATE(
            req->source.value.minmoves <= target_unit->moves_left);
    }
    break;
  case VUT_MINHP:
    if (target_unit == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = BOOL_TO_TRISTATE(
            req->source.value.min_hit_points <= target_unit->hp);
    }
    break;
  case VUT_AGE:
    switch (req->range) {
    case REQ_RANGE_LOCAL:
      if (target_unit == NULL || !is_server()) {
        eval = TRI_MAYBE;
      } else {
        eval = BOOL_TO_TRISTATE(
                 req->source.value.age <=
                 game.info.turn - target_unit->server.birth_turn);
      }
      break;
    case REQ_RANGE_CITY:
      if (target_city == NULL) {
        eval = TRI_MAYBE;
      } else {
        eval = BOOL_TO_TRISTATE(
                 req->source.value.age <=
                 game.info.turn - target_city->turn_founded);
      }
      break;
    case REQ_RANGE_PLAYER:
      if (target_player == NULL) {
        eval = TRI_MAYBE;
      } else {
        eval =
          BOOL_TO_TRISTATE(req->source.value.age <= player_age(target_player));
      }
      break;
    default:
      eval = TRI_MAYBE;
      break;
    }
    break;
  case VUT_MINTECHS:
    switch (req->range) {
    case REQ_RANGE_WORLD:
      /* "None" does not count */
      eval = ((game.info.global_advance_count - 1) >= req->source.value.min_techs);
      break;
    case REQ_RANGE_PLAYER:
      if (target_player == NULL) {
        eval = TRI_MAYBE;
      } else {
        /* "None" does not count */
        eval = ((research_get(target_player)->techs_researched - 1) >= req->source.value.min_techs);
      }
      break;
    default:
      eval = TRI_MAYBE;
    }
    break;
  case VUT_ACTION:
    eval = BOOL_TO_TRISTATE(target_action
                            && action_number(target_action)
                               == action_number(req->source.value.action));
    break;
  case VUT_OTYPE:
    eval = BOOL_TO_TRISTATE(target_output
                            && target_output->index == req->source.value.outputtype);
    break;
  case VUT_SPECIALIST:
    eval = BOOL_TO_TRISTATE(target_specialist
                            && target_specialist == req->source.value.specialist);
    break;
  case VUT_MINSIZE:
    if (target_city == NULL) {
      eval = TRI_MAYBE;
    } else {
      if (req->range == REQ_RANGE_TRADEROUTE) {
        bool found = FALSE;

        if (city_size_get(target_city) >= req->source.value.minsize) {
          eval = TRI_YES;
          break;
        }
        trade_partners_iterate(target_city, trade_partner) {
          if (city_size_get(trade_partner) >= req->source.value.minsize) {
            found = TRUE;
            break;
          }
        } trade_partners_iterate_end;
        eval = BOOL_TO_TRISTATE(found);
      } else {
        eval = BOOL_TO_TRISTATE(city_size_get(target_city) >= req->source.value.minsize);
      }
    }
    break;
  case VUT_MINCULTURE:
    eval = is_minculture_in_range(target_city, target_player, req->range,
                                  req->source.value.minculture);
    break;
  case VUT_AI_LEVEL:
    if (target_player == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = BOOL_TO_TRISTATE(is_ai(target_player)
                              && target_player->ai_common.skill_level == req->source.value.ai_level);
    }
    break;
  case VUT_MAXTILEUNITS:
    eval = is_tile_units_in_range(target_tile, req->range,
                                  req->source.value.max_tile_units);
    break;
  case VUT_TERRAINCLASS:
    eval = is_terrain_class_in_range(target_tile, target_city,
                                     req->range, req->survives,
                                     req->source.value.terrainclass);
    break;
  case VUT_BASEFLAG:
    eval = is_baseflag_in_range(target_tile, target_city,
                                 req->range, req->survives,
                                 req->source.value.baseflag);
    break;
  case VUT_ROADFLAG:
    eval = is_roadflag_in_range(target_tile, target_city,
                                 req->range, req->survives,
                                 req->source.value.roadflag);
    break;
  case VUT_EXTRAFLAG:
    eval = is_extraflag_in_range(target_tile, target_city,
                                 req->range, req->survives,
                                 req->source.value.extraflag);
    break;
  case VUT_MINYEAR:
    eval = BOOL_TO_TRISTATE(game.info.year >= req->source.value.minyear);
    break;
  case VUT_MINCALFRAG:
    eval = BOOL_TO_TRISTATE(game.info.fragment_count >= req->source.value.mincalfrag);
    break;
  case VUT_TOPO:
    eval = BOOL_TO_TRISTATE(current_topo_has_flag(req->source.value.topo_property));
    break;
  case VUT_SERVERSETTING:
    eval = BOOL_TO_TRISTATE(ssetv_setting_has_value(
                                req->source.value.ssetval));
    break;
  case VUT_TERRAINALTER:
    if (target_tile == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = is_terrain_alter_possible_in_range(target_tile,
                                                req->range, req->survives,
                                                req->source.value.terrainalter);
    }
    break;
  case VUT_CITYTILE:
    if (target_tile == NULL) {
      eval = TRI_MAYBE;
    } else {
      eval = is_citytile_in_range(target_tile, target_city,
                                  req->range,
                                  req->source.value.citytile);
    }
    break;
  case VUT_COUNT:
    log_error("is_req_active(): invalid source kind %d.", req->source.kind);
    return FALSE;
  }

  if (eval == TRI_MAYBE) {
    if (prob_type == RPT_POSSIBLE) {
      return TRUE;
    } else {
      return FALSE;
    }
  }
  if (req->present) {
    return (eval == TRI_YES);
  } else {
    return (eval == TRI_NO);
  }
}

/**********************************************************************//**
  Checks the requirement(s) to see if they are active on the given target.

  target gives the type of the target
  (player,city,building,tile) give the exact target

  reqs gives the requirement vector.
  The function returns TRUE only if all requirements are active.

  Make sure you give all aspects of the target when calling this function:
  for instance if you have TARGET_CITY pass the city's owner as the target
  player as well as the city itself as the target city.
**************************************************************************/
bool are_reqs_active(const struct player *target_player,
                     const struct player *other_player,
                     const struct city *target_city,
                     const struct impr_type *target_building,
                     const struct tile *target_tile,
                     const struct unit *target_unit,
                     const struct unit_type *target_unittype,
                     const struct output_type *target_output,
                     const struct specialist *target_specialist,
                     const struct action *target_action,
                     const struct requirement_vector *reqs,
                     const enum   req_problem_type prob_type)
{
  requirement_vector_iterate(reqs, preq) {
    if (!is_req_active(target_player, other_player, target_city,
                       target_building, target_tile,
                       target_unit, target_unittype,
                       target_output, target_specialist, target_action,
		       preq, prob_type)) {
      return FALSE;
    }
  } requirement_vector_iterate_end;
  return TRUE;
}

/**********************************************************************//**
  Return TRUE if this is an "unchanging" requirement.  This means that
  if a target can't meet the requirement now, it probably won't ever be able
  to do so later.  This can be used to do requirement filtering when checking
  if a target may "eventually" become available.

  Note this isn't absolute.  Returning TRUE here just means that the
  requirement probably can't be met.  In some cases (particularly terrains)
  it may be wrong.
***************************************************************************/
bool is_req_unchanging(const struct requirement *req)
{
  switch (req->source.kind) {
  case VUT_NONE:
  case VUT_ACTION:
  case VUT_OTYPE:
  case VUT_SPECIALIST:	/* Only so long as it's at local range only */
  case VUT_AI_LEVEL:
  case VUT_CITYTILE:
  case VUT_STYLE:
  case VUT_TOPO:
  case VUT_SERVERSETTING:
    return TRUE;
  case VUT_NATION:
  case VUT_NATIONGROUP:
    return (req->range != REQ_RANGE_ALLIANCE);
  case VUT_ADVANCE:
  case VUT_TECHFLAG:
  case VUT_GOVERNMENT:
  case VUT_ACHIEVEMENT:
  case VUT_IMPROVEMENT:
  case VUT_IMPR_GENUS:
  case VUT_MINSIZE:
  case VUT_MINCULTURE:
  case VUT_MINTECHS:
  case VUT_NATIONALITY:
  case VUT_DIPLREL:
  case VUT_MAXTILEUNITS:
  case VUT_UTYPE:	/* Not sure about this one */
  case VUT_UTFLAG:	/* Not sure about this one */
  case VUT_UCLASS:	/* Not sure about this one */
  case VUT_UCFLAG:	/* Not sure about this one */
  case VUT_MINVETERAN:
  case VUT_UNITSTATE:
  case VUT_MINMOVES:
  case VUT_MINHP:
  case VUT_AGE:
  case VUT_ROADFLAG:
  case VUT_EXTRAFLAG:
  case VUT_MINCALFRAG:  /* cyclically available */
    return FALSE;
  case VUT_TERRAIN:
  case VUT_EXTRA:
  case VUT_GOOD:
  case VUT_TERRAINCLASS:
  case VUT_TERRFLAG:
  case VUT_TERRAINALTER:
  case VUT_BASEFLAG:
    /* Terrains, specials and bases aren't really unchanging; in fact they're
     * practically guaranteed to change.  We return TRUE here for historical
     * reasons and so that the AI doesn't get confused (since the AI
     * doesn't know how to meet special and terrain requirements). */
    return TRUE;
  case VUT_MINYEAR:
    /* Once year is reached, it does not change again */
    return req->source.value.minyear > game.info.year;
  case VUT_COUNT:
    break;
  }
  fc_assert_msg(FALSE, "Invalid source kind %d.", req->source.kind);
  return TRUE;
}

/**********************************************************************//**
  Returns TRUE iff the requirement vector vec contains the requirement
  req.
**************************************************************************/
bool is_req_in_vec(const struct requirement *req,
                   const struct requirement_vector *vec)
{
  requirement_vector_iterate(vec, preq) {
    if (are_requirements_equal(req, preq)) {
      return TRUE;
    }
  } requirement_vector_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  Return TRUE iff the two sources are equivalent.  Note this isn't the
  same as an == or memcmp check.
***************************************************************************/
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
  case VUT_TECHFLAG:
    return psource1->value.techflag == psource2->value.techflag;
  case VUT_GOVERNMENT:
    return psource1->value.govern == psource2->value.govern;
  case VUT_ACHIEVEMENT:
    return psource1->value.achievement == psource2->value.achievement;
  case VUT_STYLE:
    return psource1->value.style == psource2->value.style;
  case VUT_IMPROVEMENT:
    return psource1->value.building == psource2->value.building;
  case VUT_IMPR_GENUS:
    return psource1->value.impr_genus == psource2->value.impr_genus;
  case VUT_EXTRA:
    return psource1->value.extra == psource2->value.extra;
  case VUT_GOOD:
    return psource1->value.good == psource2->value.good;
  case VUT_TERRAIN:
    return psource1->value.terrain == psource2->value.terrain;
  case VUT_TERRFLAG:
    return psource1->value.terrainflag == psource2->value.terrainflag;
  case VUT_NATION:
    return psource1->value.nation == psource2->value.nation;
  case VUT_NATIONGROUP:
    return psource1->value.nationgroup == psource2->value.nationgroup;
  case VUT_NATIONALITY:
    return psource1->value.nationality == psource2->value.nationality;
  case VUT_DIPLREL:
    return psource1->value.diplrel == psource2->value.diplrel;
  case VUT_UTYPE:
    return psource1->value.utype == psource2->value.utype;
  case VUT_UTFLAG:
    return psource1->value.unitflag == psource2->value.unitflag;
  case VUT_UCLASS:
    return psource1->value.uclass == psource2->value.uclass;
  case VUT_UCFLAG:
    return psource1->value.unitclassflag == psource2->value.unitclassflag;
  case VUT_MINVETERAN:
    return psource1->value.minveteran == psource2->value.minveteran;
  case VUT_UNITSTATE:
    return psource1->value.unit_state == psource2->value.unit_state;
  case VUT_MINMOVES:
    return psource1->value.minmoves == psource2->value.minmoves;
  case VUT_MINHP:
    return psource1->value.min_hit_points == psource2->value.min_hit_points;
  case VUT_AGE:
    return psource1->value.age == psource2->value.age;
  case VUT_MINTECHS:
    return psource1->value.min_techs == psource2->value.min_techs;
  case VUT_ACTION:
    return (action_number(psource1->value.action)
            == action_number(psource2->value.action));
  case VUT_OTYPE:
    return psource1->value.outputtype == psource2->value.outputtype;
  case VUT_SPECIALIST:
    return psource1->value.specialist == psource2->value.specialist;
  case VUT_MINSIZE:
    return psource1->value.minsize == psource2->value.minsize;
  case VUT_MINCULTURE:
    return psource1->value.minculture == psource2->value.minculture;
  case VUT_AI_LEVEL:
    return psource1->value.ai_level == psource2->value.ai_level;
  case VUT_MAXTILEUNITS:
    return psource1->value.max_tile_units == psource2->value.max_tile_units;
  case VUT_TERRAINCLASS:
    return psource1->value.terrainclass == psource2->value.terrainclass;
  case VUT_BASEFLAG:
    return psource1->value.baseflag == psource2->value.baseflag;
  case VUT_ROADFLAG:
    return psource1->value.roadflag == psource2->value.roadflag;
  case VUT_EXTRAFLAG:
    return psource1->value.extraflag == psource2->value.extraflag;
  case VUT_MINYEAR:
    return psource1->value.minyear == psource2->value.minyear;
  case VUT_MINCALFRAG:
    return psource1->value.mincalfrag == psource2->value.mincalfrag;
  case VUT_TOPO:
    return psource1->value.topo_property == psource2->value.topo_property;
  case VUT_SERVERSETTING:
    return psource1->value.ssetval == psource2->value.ssetval;
  case VUT_TERRAINALTER:
    return psource1->value.terrainalter == psource2->value.terrainalter;
  case VUT_CITYTILE:
    return psource1->value.citytile == psource2->value.citytile;
  case VUT_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid source kind %d.", psource1->kind);
  return FALSE;
}

/**********************************************************************//**
  Return the (untranslated) rule name of the universal.
  You don't have to free the return pointer.
***************************************************************************/
const char *universal_rule_name(const struct universal *psource)
{
  static char buffer[10];

  switch (psource->kind) {
  case VUT_NONE:
    return "(none)";
  case VUT_CITYTILE:
    return citytile_type_name(psource->value.citytile);
  case VUT_MINYEAR:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.minyear);

    return buffer;
  case VUT_MINCALFRAG:
    /* Rule name is 0-based number, not pretty name from ruleset */
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.mincalfrag);

    return buffer;
  case VUT_TOPO:
    return topo_flag_name(psource->value.topo_property);
  case VUT_SERVERSETTING:
    return ssetv_rule_name(psource->value.ssetval);
  case VUT_ADVANCE:
    return advance_rule_name(psource->value.advance);
  case VUT_TECHFLAG:
    return tech_flag_id_name(psource->value.techflag);
  case VUT_GOVERNMENT:
    return government_rule_name(psource->value.govern);
  case VUT_ACHIEVEMENT:
    return achievement_rule_name(psource->value.achievement);
  case VUT_STYLE:
    return style_rule_name(psource->value.style);
  case VUT_IMPROVEMENT:
    return improvement_rule_name(psource->value.building);
  case VUT_IMPR_GENUS:
    return impr_genus_id_name(psource->value.impr_genus);
  case VUT_EXTRA:
    return extra_rule_name(psource->value.extra);
  case VUT_GOOD:
    return goods_rule_name(psource->value.good);
  case VUT_TERRAIN:
    return terrain_rule_name(psource->value.terrain);
  case VUT_TERRFLAG:
    return terrain_flag_id_name(psource->value.terrainflag);
  case VUT_NATION:
    return nation_rule_name(psource->value.nation);
  case VUT_NATIONGROUP:
    return nation_group_rule_name(psource->value.nationgroup);
  case VUT_DIPLREL:
    return diplrel_rule_name(psource->value.diplrel);
  case VUT_NATIONALITY:
    return nation_rule_name(psource->value.nationality);
  case VUT_UTYPE:
    return utype_rule_name(psource->value.utype);
  case VUT_UTFLAG:
    return unit_type_flag_id_name(psource->value.unitflag);
  case VUT_UCLASS:
    return uclass_rule_name(psource->value.uclass);
  case VUT_UCFLAG:
    return unit_class_flag_id_name(psource->value.unitclassflag);
  case VUT_MINVETERAN:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.minveteran);

    return buffer;
  case VUT_UNITSTATE:
    return ustate_prop_name(psource->value.unit_state);
  case VUT_MINMOVES:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.minmoves);

    return buffer;
  case VUT_MINHP:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.min_hit_points);

    return buffer;
  case VUT_AGE:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.age);

    return buffer;
  case VUT_MINTECHS:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.min_techs);

    return buffer;
  case VUT_ACTION:
    return action_rule_name(psource->value.action);
  case VUT_OTYPE:
    return get_output_name(psource->value.outputtype);
  case VUT_SPECIALIST:
    return specialist_rule_name(psource->value.specialist);
  case VUT_MINSIZE:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.minsize);

    return buffer;
  case VUT_MINCULTURE:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.minculture);

    return buffer;
  case VUT_AI_LEVEL:
    return ai_level_name(psource->value.ai_level);
  case VUT_MAXTILEUNITS:
    fc_snprintf(buffer, sizeof(buffer), "%d", psource->value.max_tile_units);
    return buffer;
  case VUT_TERRAINCLASS:
    return terrain_class_name(psource->value.terrainclass);
  case VUT_BASEFLAG:
    return base_flag_id_name(psource->value.baseflag);
  case VUT_ROADFLAG:
    return road_flag_id_name(psource->value.roadflag);
  case VUT_EXTRAFLAG:
    return extra_flag_id_name(psource->value.extraflag);
  case VUT_TERRAINALTER:
    return terrain_alteration_name(psource->value.terrainalter);
  case VUT_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid source kind %d.", psource->kind);
  return NULL;
}

/**********************************************************************//**
  Make user-friendly text for the source.  The text is put into a user
  buffer which is also returned.
  This should be short, as it's used in lists like "Aqueduct+Size 8" when
  explaining a calculated value. It just needs to be enough to remind the
  player of rules they already know, not a complete explanation (use
  insert_requirement() for that).
**************************************************************************/
const char *universal_name_translation(const struct universal *psource,
                                       char *buf, size_t bufsz)
{
  buf[0] = '\0'; /* to be safe. */
  switch (psource->kind) {
  case VUT_NONE:
    /* TRANS: missing value */
    fc_strlcat(buf, _("(none)"), bufsz);
    return buf;
  case VUT_ADVANCE:
    fc_strlcat(buf, advance_name_translation(psource->value.advance), bufsz);
    return buf;
  case VUT_TECHFLAG:
    cat_snprintf(buf, bufsz, _("\"%s\" tech"),
                 tech_flag_id_translated_name(psource->value.techflag));
    return buf;
  case VUT_GOVERNMENT:
    fc_strlcat(buf, government_name_translation(psource->value.govern),
               bufsz);
    return buf;
  case VUT_ACHIEVEMENT:
    fc_strlcat(buf, achievement_name_translation(psource->value.achievement),
               bufsz);
    return buf;
  case VUT_STYLE:
    fc_strlcat(buf, style_name_translation(psource->value.style),
               bufsz);
    return buf;
  case VUT_IMPROVEMENT:
    fc_strlcat(buf, improvement_name_translation(psource->value.building),
               bufsz);
    return buf;
  case VUT_IMPR_GENUS:
    fc_strlcat(buf,
               impr_genus_id_translated_name(psource->value.impr_genus),
               bufsz);
    return buf;
  case VUT_EXTRA:
    fc_strlcat(buf, extra_name_translation(psource->value.extra), bufsz);
    return buf;
  case VUT_GOOD:
    fc_strlcat(buf, goods_name_translation(psource->value.good), bufsz);
    return buf;
  case VUT_TERRAIN:
    fc_strlcat(buf, terrain_name_translation(psource->value.terrain), bufsz);
    return buf;
  case VUT_NATION:
    fc_strlcat(buf, nation_adjective_translation(psource->value.nation),
               bufsz);
    return buf;
  case VUT_NATIONGROUP:
    fc_strlcat(buf, nation_group_name_translation(psource->value.nationgroup),
               bufsz);
    return buf;
  case VUT_NATIONALITY:
    cat_snprintf(buf, bufsz, _("%s citizens"),
                 nation_adjective_translation(psource->value.nationality));
    return buf;
  case VUT_DIPLREL:
    fc_strlcat(buf, diplrel_name_translation(psource->value.diplrel),
               bufsz);
    return buf;
  case VUT_UTYPE:
    fc_strlcat(buf, utype_name_translation(psource->value.utype), bufsz);
    return buf;
  case VUT_UTFLAG:
    cat_snprintf(buf, bufsz,
                 /* TRANS: Unit type flag */
                 Q_("?utflag:\"%s\" units"),
                 unit_type_flag_id_translated_name(
                   psource->value.unitflag));
    return buf;
  case VUT_UCLASS:
    cat_snprintf(buf, bufsz,
                 /* TRANS: Unit class */
                 _("%s units"),
		 uclass_name_translation(psource->value.uclass));
    return buf;
  case VUT_UCFLAG:
    cat_snprintf(buf, bufsz,
                 /* TRANS: Unit class flag */
                 Q_("?ucflag:\"%s\" units"),
                 unit_class_flag_id_translated_name(
                   psource->value.unitclassflag));
    return buf;
  case VUT_MINVETERAN:
    /* FIXME */
    cat_snprintf(buf, bufsz, _("Veteran level >=%d"),
		 psource->value.minveteran);
    return buf;
  case VUT_UNITSTATE:
    switch (psource->value.unit_state) {
    case USP_TRANSPORTED:
      /* TRANS: unit state. (appears in strings like "Missile+Transported") */
      cat_snprintf(buf, bufsz, _("Transported"));
      break;
    case USP_LIVABLE_TILE:
      cat_snprintf(buf, bufsz,
                   /* TRANS: unit state. (appears in strings like
                    * "Missile+On livable tile") */
                   _("On livable tile"));
      break;
    case USP_DOMESTIC_TILE:
      cat_snprintf(buf, bufsz,
                   /* TRANS: unit state. (appears in strings like
                    * "Missile+On domestic tile") */
                   _("On domestic tile"));
      break;
    case USP_TRANSPORTING:
      /* TRANS: unit state. (appears in strings like "Missile+Transported") */
      cat_snprintf(buf, bufsz, _("Transporting"));
      break;
    case USP_HAS_HOME_CITY:
      /* TRANS: unit state. (appears in strings like "Missile+Has a home city") */
      cat_snprintf(buf, bufsz, _("Has a home city"));
      break;
    case USP_NATIVE_TILE:
      cat_snprintf(buf, bufsz,
                   /* TRANS: unit state. (appears in strings like
                    * "Missile+On native tile") */
                   _("On native tile"));
      break;
    case USP_COUNT:
      fc_assert_msg(psource->value.unit_state != USP_COUNT,
                    "Invalid unit state property.");
      break;
    }
    return buf;
  case VUT_MINMOVES:
    /* TRANS: Minimum unit movement points left for requirement to be met
     * (%s is a string like "1" or "2 1/3") */
    cat_snprintf(buf, bufsz, _("%s MP"),
                 move_points_text(psource->value.minmoves, TRUE));
    return buf;
  case VUT_MINHP:
    /* TRANS: HP = hit points */
    cat_snprintf(buf, bufsz, _("%d HP"),
                 psource->value.min_hit_points);
    return buf;
  case VUT_AGE:
    cat_snprintf(buf, bufsz, _("Age %d"),
                 psource->value.age);
    return buf;
  case VUT_MINTECHS:
    cat_snprintf(buf, bufsz, _("%d Techs"),
                 psource->value.min_techs);
    return buf;
  case VUT_ACTION:
    fc_strlcat(buf, action_name_translation(psource->value.action),
               bufsz);
    return buf;
  case VUT_OTYPE:
    /* FIXME */
    fc_strlcat(buf, get_output_name(psource->value.outputtype), bufsz);
    return buf;
  case VUT_SPECIALIST:
    fc_strlcat(buf, specialist_plural_translation(psource->value.specialist),
               bufsz);
    return buf;
  case VUT_MINSIZE:
    cat_snprintf(buf, bufsz, _("Size %d"),
		 psource->value.minsize);
    return buf;
  case VUT_MINCULTURE:
    cat_snprintf(buf, bufsz, _("Culture %d"),
		 psource->value.minculture);
    return buf;
  case VUT_AI_LEVEL:
    /* TRANS: "Hard AI" */
    cat_snprintf(buf, bufsz, _("%s AI"),
                 ai_level_translated_name(psource->value.ai_level)); /* FIXME */
    return buf;
  case VUT_MAXTILEUNITS:
    /* TRANS: here <= means 'less than or equal' */
    cat_snprintf(buf, bufsz, PL_("<=%d unit",
                                 "<=%d units", psource->value.max_tile_units),
                 psource->value.max_tile_units);
    return buf;
  case VUT_TERRAINCLASS:
    /* TRANS: Terrain class: "Land terrain" */
    cat_snprintf(buf, bufsz, _("%s terrain"),
                 terrain_class_name_translation(psource->value.terrainclass));
    return buf;
  case VUT_TERRFLAG:
    cat_snprintf(buf, bufsz,
                 /* TRANS: Terrain flag */
                 Q_("?terrflag:\"%s\" terrain"),
                 terrain_flag_id_translated_name(
                   psource->value.terrainflag));
    return buf;
  case VUT_BASEFLAG:
    cat_snprintf(buf, bufsz,
                 /* TRANS: Base flag */
                 Q_("?baseflag:\"%s\" base"),
                 base_flag_id_translated_name(psource->value.baseflag));
    return buf;
  case VUT_ROADFLAG:
    cat_snprintf(buf, bufsz,
                 /* TRANS: Road flag */
                 Q_("?roadflag:\"%s\" road"),
                 road_flag_id_translated_name(psource->value.roadflag));
    return buf;
  case VUT_EXTRAFLAG:
    cat_snprintf(buf, bufsz,
                 /* TRANS: Extra flag */
                 Q_("?extraflag:\"%s\" extra"),
                 extra_flag_id_translated_name(psource->value.extraflag));
    return buf;
  case VUT_MINYEAR:
    cat_snprintf(buf, bufsz, _("After %s"),
                 textyear(psource->value.minyear));
    return buf;
  case VUT_MINCALFRAG:
    /* TRANS: here >= means 'greater than or equal'.
     * %s identifies a calendar fragment (may be bare number). */
    cat_snprintf(buf, bufsz, _(">=%s"),
                 textcalfrag(psource->value.mincalfrag));
    return buf;
  case VUT_TOPO:
    cat_snprintf(buf, bufsz, _("%s map"),
                 _(topo_flag_name(psource->value.topo_property)));
    return buf;
  case VUT_SERVERSETTING:
    fc_strlcat(buf, ssetv_human_readable(psource->value.ssetval, TRUE),
               bufsz);
    return buf;
  case VUT_TERRAINALTER:
    /* TRANS: "Irrigation possible" */
    cat_snprintf(buf, bufsz, _("%s possible"),
                 Q_(terrain_alteration_name(psource->value.terrainalter)));
    return buf;
  case VUT_CITYTILE:
    fc_strlcat(buf, _("City center"), bufsz);
    return buf;
  case VUT_COUNT:
    break;
  }

  fc_assert_msg(FALSE, "Invalid source kind %d.", psource->kind);
  return buf;
}

/**********************************************************************//**
  Return untranslated name of the universal source name.
**************************************************************************/
const char *universal_type_rule_name(const struct universal *psource)
{
  return universals_n_name(psource->kind);
}

/**********************************************************************//**
  Return the number of shields it takes to build this universal.
**************************************************************************/
int universal_build_shield_cost(const struct city *pcity,
                                const struct universal *target)
{
  switch (target->kind) {
  case VUT_IMPROVEMENT:
    return impr_build_shield_cost(pcity, target->value.building);
  case VUT_UTYPE:
    return utype_build_shield_cost(pcity, target->value.utype);
  default:
    break;
  }
  return FC_INFINITY;
}

/**********************************************************************//**
  Replaces all instances of the universal to_replace with replacement in
  the requirement vector reqs and returns TRUE iff any requirements were
  replaced.
**************************************************************************/
bool universal_replace_in_req_vec(struct requirement_vector *reqs,
                                  const struct universal *to_replace,
                                  const struct universal *replacement)
{
  bool changed = FALSE;

  requirement_vector_iterate(reqs, preq) {
    if (universal_is_mentioned_by_requirement(preq, to_replace)) {
      preq->source = *replacement;
      changed = TRUE;
    }
  } requirement_vector_iterate_end;

  return changed;
}

/**********************************************************************//**
  Returns TRUE iff the universal 'psource' is directly mentioned by any of
  the requirements in 'reqs'.
**************************************************************************/
bool universal_is_mentioned_by_requirements(
    const struct requirement_vector *reqs,
    const struct universal *psource)
{
  requirement_vector_iterate(reqs, preq) {
    if (universal_is_mentioned_by_requirement(preq, psource)) {
      return TRUE;
    }
  } requirement_vector_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  Will the universal 'source' fulfill this requirement?
**************************************************************************/
enum req_item_found
universal_fulfills_requirement(const struct requirement *preq,
                               const struct universal *source)
{
  fc_assert(universal_found_function[source->kind]);

  return (*universal_found_function[source->kind])(preq, source);
}

/**********************************************************************//**
  Will the universal 'source' fulfill the requirements in the list?
  If 'check_necessary' is FALSE: are there no requirements that 'source'
    would actively prevent the fulfilment of?
  If 'check_necessary' is TRUE: does 'source' help the requirements to be
    fulfilled? (NB 'source' might not be the only source of its type that
    would be sufficient; for instance, if 'source' is a specific terrain
    type, we can return TRUE even if the requirement is only for something
    vague like a TerrainClass.)
**************************************************************************/
bool universal_fulfills_requirements(bool check_necessary,
                                     const struct requirement_vector *reqs,
                                     const struct universal *source)
{
  bool necessary = FALSE;

  fc_assert(universal_found_function[source->kind]);

  requirement_vector_iterate(reqs, preq) {
    switch ((*universal_found_function[source->kind])(preq, source)) {
    case ITF_NOT_APPLICABLE:
      continue;
    case ITF_NO:
      if (preq->present) {
        return FALSE;
      }
      break;
    case ITF_YES:
      if (preq->present) {
        necessary = TRUE;
      } else {
        return FALSE;
      }
      break;
    }
  } requirement_vector_iterate_end;

  return (!check_necessary || necessary);
}

/**********************************************************************//**
  Returns TRUE iff the specified universal is relevant to fulfilling the
  specified requirement.
**************************************************************************/
bool universal_is_relevant_to_requirement(const struct requirement *req,
                                          const struct universal *source)
{
  switch (universal_fulfills_requirement(req, source)) {
  case ITF_NOT_APPLICABLE:
    return FALSE;
  case ITF_NO:
  case ITF_YES:
    return TRUE;
  }

  log_error("Unhandled item_found value");
  return FALSE;
}

/**********************************************************************//**
  Find if a nation fulfills a requirement
**************************************************************************/
static enum req_item_found nation_found(const struct requirement *preq,
                                        const struct universal *source)
{
  fc_assert(source->value.nation);

  switch (preq->source.kind) {
  case VUT_NATION:
    return preq->source.value.nation == source->value.nation ? ITF_YES
                                                             : ITF_NO;
  case VUT_NATIONGROUP:
    return nation_is_in_group(source->value.nation,
                              preq->source.value.nationgroup) ? ITF_YES
                                                              : ITF_NO;
  default:
    break;
  }

  return ITF_NOT_APPLICABLE;
}

/**********************************************************************//**
  Find if a government fulfills a requirement
**************************************************************************/
static enum req_item_found government_found(const struct requirement *preq,
                                            const struct universal *source)
{
  fc_assert(source->value.govern);

  if (preq->source.kind == VUT_GOVERNMENT) {
    return preq->source.value.govern == source->value.govern ? ITF_YES
                                                             : ITF_NO;
  }

  return ITF_NOT_APPLICABLE;
}

/**********************************************************************//**
  Find if an improvement fulfills a requirement
**************************************************************************/
static enum req_item_found improvement_found(const struct requirement *preq,
                                             const struct universal *source)
{
  fc_assert(source->value.building);

  /* We only ever return ITF_YES, because requiring a different
   * improvement does not mean that the improvement under consideration
   * cannot fulfill the requirements. This is necessary to allow
   * requirement vectors to specify multiple required improvements. */

  switch (preq->source.kind) {
  case VUT_IMPROVEMENT:
    if (source->value.building == preq->source.value.building) {
      return ITF_YES;
    }
    break;
  case VUT_IMPR_GENUS:
    if (source->value.building->genus == preq->source.value.impr_genus) {
      return ITF_YES;
    }
    break;
  default:
    break;
  }

  return ITF_NOT_APPLICABLE;
}

/**********************************************************************//**
  Find if a unit class fulfills a requirement
**************************************************************************/
static enum req_item_found unit_class_found(const struct requirement *preq,
                                            const struct universal *source)
{
  fc_assert(source->value.uclass);

  switch (preq->source.kind) {
  case VUT_UCLASS:
    return source->value.uclass == preq->source.value.uclass ? ITF_YES
                                                             : ITF_NO;
  case VUT_UCFLAG:
    return uclass_has_flag(source->value.uclass,
                           preq->source.value.unitclassflag) ? ITF_YES
                                                             : ITF_NO;

  default:
    /* Not found and not relevant. */
    return ITF_NOT_APPLICABLE;
  };
}

/**********************************************************************//**
  Find if a unit type fulfills a requirement
**************************************************************************/
static enum req_item_found unit_type_found(const struct requirement *preq,
                                           const struct universal *source)
{
  fc_assert(source->value.utype);

  switch (preq->source.kind) {
  case VUT_UTYPE:
    return source->value.utype == preq->source.value.utype ? ITF_YES : ITF_NO;
  case VUT_UCLASS:
    return utype_class(source->value.utype) == preq->source.value.uclass
                                            ? ITF_YES : ITF_NO;
  case VUT_UTFLAG:
    return utype_has_flag(source->value.utype,
                          preq->source.value.unitflag) ? ITF_YES : ITF_NO;
  case VUT_UCFLAG:
    return uclass_has_flag(utype_class(source->value.utype),
                           preq->source.value.unitclassflag) ? ITF_YES
                                                             : ITF_NO;
  default:
    /* Not found and not relevant. */
    return ITF_NOT_APPLICABLE;
  };
}

/**********************************************************************//**
  Find if a terrain type fulfills a requirement
**************************************************************************/
static enum req_item_found terrain_type_found(const struct requirement *preq,
                                              const struct universal *source)
{
  fc_assert(source->value.terrain);

  switch (preq->source.kind) {
  case VUT_TERRAIN:
    return source->value.terrain == preq->source.value.terrain ? ITF_YES : ITF_NO;
  case VUT_TERRAINCLASS:
    return terrain_type_terrain_class(source->value.terrain) == preq->source.value.terrainclass
      ? ITF_YES : ITF_NO;
  case VUT_TERRFLAG:
    return terrain_has_flag(source->value.terrain,
                            preq->source.value.terrainflag) ? ITF_YES : ITF_NO;
  default:
    /* Not found and not relevant. */
    return ITF_NOT_APPLICABLE;
  };
}

/**********************************************************************//**
  Find if an extra type fulfills a requirement
**************************************************************************/
static enum req_item_found extra_type_found(const struct requirement *preq,
                                            const struct universal *source)
{
  fc_assert(source->value.extra);

  switch (preq->source.kind) {
  case VUT_EXTRA:
    return source->value.extra == preq->source.value.extra ? ITF_YES : ITF_NO;
  case VUT_EXTRAFLAG:
    return extra_has_flag(source->value.extra,
                          preq->source.value.extraflag) ? ITF_YES : ITF_NO;
  case VUT_BASEFLAG:
    {
      struct base_type *b = extra_base_get(source->value.extra);
      return b && base_has_flag(b, preq->source.value.baseflag)
        ? ITF_YES : ITF_NO;
    }
  case VUT_ROADFLAG:
    {
      struct road_type *r = extra_road_get(source->value.extra);
      return r && road_has_flag(r, preq->source.value.roadflag)
        ? ITF_YES : ITF_NO;
    }
  default:
    /* Not found and not relevant. */
    return ITF_NOT_APPLICABLE;
  }
}

/**********************************************************************//**
  Find if an output type fulfills a requirement
**************************************************************************/
static enum req_item_found output_type_found(const struct requirement *preq,
                                             const struct universal *source)
{
  switch (preq->source.kind) {
  case VUT_OTYPE:
    return source->value.outputtype == preq->source.value.outputtype ? ITF_YES
                                                                     : ITF_NO;
  default:
    /* Not found and not relevant. */
    return ITF_NOT_APPLICABLE;
  }
}

/**********************************************************************//**
  Initialise universal_found_function array.
**************************************************************************/
void universal_found_functions_init(void)
{
  universal_found_function[VUT_GOVERNMENT] = &government_found;
  universal_found_function[VUT_NATION] = &nation_found;
  universal_found_function[VUT_IMPROVEMENT] = &improvement_found;
  universal_found_function[VUT_UCLASS] = &unit_class_found;
  universal_found_function[VUT_UTYPE] = &unit_type_found;
  universal_found_function[VUT_TERRAIN] = &terrain_type_found;
  universal_found_function[VUT_EXTRA] = &extra_type_found;
  universal_found_function[VUT_OTYPE] = &output_type_found;
}

/**********************************************************************//**
  Returns (the position of) the given requirement's enumerator in the
  enumeration of all possible requirements of its requirement kind.

  Note: Since this isn't used for any requirement type that supports
  surviving requirements those aren't supported. Add support if a user
  appears.
**************************************************************************/
int requirement_kind_ereq(const int value,
                          const enum req_range range,
                          const bool present,
                          const int max_value)
{
  /* The enumerators in each range starts with present for every possible
   * value followed by !present for every possible value. */
  const int pres_start = (present ? 0 : max_value);

  /* The enumerators for every range follows all the positions of the
   * previous range(s). */
  const int range_start = ((max_value - 1) * 2) * range;

  return range_start + pres_start + value;
}
