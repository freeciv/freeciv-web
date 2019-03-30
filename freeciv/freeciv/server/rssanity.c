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

/* common */
#include "achievements.h"
#include "actions.h"
#include "effects.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "player.h"
#include "road.h"
#include "specialist.h"
#include "tech.h"

/* server */
#include "ruleset.h"
#include "settings.h"

#include "rssanity.h"

/**********************************************************************//**
  Is non-rule data in ruleset sane?
**************************************************************************/
static bool sanity_check_metadata(void)
{
  if (game.ruleset_summary != NULL &&
      strlen(game.ruleset_summary) > MAX_LEN_CONTENT) {
    log_error("Too long ruleset summary. It can be only %d bytes long. "
              "Put longer explanations to ruleset description.",
              MAX_LEN_CONTENT);
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Does nation have tech initially?
**************************************************************************/
static bool nation_has_initial_tech(struct nation_type *pnation,
                                    struct advance *tech)
{
  int i;

  /* See if it's given as global init tech */
  for (i = 0; i < MAX_NUM_TECH_LIST
       && game.rgame.global_init_techs[i] != A_LAST; i++) {
    if (game.rgame.global_init_techs[i] == advance_number(tech)) {
      return TRUE;
    }
  }

  /* See if it's given as national init tech */
  for (i = 0;
       i < MAX_NUM_TECH_LIST && pnation->init_techs[i] != A_LAST;
       i++) {
    if (pnation->init_techs[i] == advance_number(tech)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**********************************************************************//**
  Returns TRUE iff the given server setting is visible enough to be
  allowed to appear in ServerSetting requirements.
**************************************************************************/
static bool sanity_check_setting_is_seen(struct setting *pset)
{
  return setting_is_visible_at_level(pset, ALLOW_INFO);
}

/**********************************************************************//**
  Returns TRUE iff the specified server setting is a game rule and
 therefore may appear in a requirement.
**************************************************************************/
static bool sanity_check_setting_is_game_rule(struct setting *pset)
{
  if ((setting_category(pset) == SSET_INTERNAL
            || setting_category(pset) == SSET_NETWORK)
           /* White list for SSET_INTERNAL and SSET_NETWORK settings. */
           && !(pset == setting_by_name("phasemode")
                || pset == setting_by_name("timeout")
                || pset == setting_by_name("timeaddenemymove")
                || pset == setting_by_name("unitwaittime")
                || pset == setting_by_name("victories"))) {
    /* The given server setting is a server operator related setting (like
     * the compression type of savegames), not a game rule. */
    return FALSE;
  }

  if (pset == setting_by_name("naturalcitynames")) {
    /* This setting is about "look", not rules. */
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Returns TRUE iff the given server setting and value combination is
  allowed to appear in ServerSetting requirements.
**************************************************************************/
bool sanity_check_server_setting_value_in_req(ssetv ssetval)
{
  server_setting_id id;
  struct setting *pset;

  /* TODO: use ssetv_setting_get() if setting value becomes multiplexed with
   * the server setting id. */
  id = (server_setting_id)ssetval;
  fc_assert_ret_val(server_setting_exists(id), FALSE);

  if (server_setting_type_get(id) != SST_BOOL) {
    /* Not supported yet. */
    return FALSE;
  }

  pset = setting_by_number(id);

  return (sanity_check_setting_is_seen(pset)
          && sanity_check_setting_is_game_rule(pset));
}

/**********************************************************************//**
  Sanity checks on a requirement in isolation.
  This will generally be things that could only not be checked at
  ruleset load time because they would have referenced things not yet
  loaded from the ruleset.
**************************************************************************/
static bool sanity_check_req_individual(struct requirement *preq,
                                        const char *list_for)
{
  switch (preq->source.kind) {
  case VUT_IMPROVEMENT:
    /* This check corresponds to what is_req_active() will support.
     * It can't be done in req_from_str(), as we may not have
     * loaded all building information at that time. */
    {
      struct impr_type *pimprove = preq->source.value.building;
      if (preq->range == REQ_RANGE_WORLD && !is_great_wonder(pimprove)) {
        log_error("%s: World-ranged requirement not supported for "
                  "%s (only great wonders supported)", list_for,
                  improvement_name_translation(pimprove));
        return FALSE;
      } else if (preq->range > REQ_RANGE_TRADEROUTE && !is_wonder(pimprove)) {
        log_error("%s: %s-ranged requirement not supported for "
                  "%s (only wonders supported)", list_for,
                  req_range_name(preq->range),
                  improvement_name_translation(pimprove));
        return FALSE;
      }
    }
    break;
  case VUT_MINCALFRAG:
    /* Currently [calendar] is loaded after some requirements are
     * parsed, so we can't do this in universal_value_from_str(). */
    if (game.calendar.calendar_fragments < 1) {
      log_error("%s: MinCalFrag requirement used in ruleset without "
                "calendar fragments", list_for);
      return FALSE;
    } else if (preq->source.value.mincalfrag >= game.calendar.calendar_fragments) {
      log_error("%s: MinCalFrag requirement %d out of range (max %d in "
                "this ruleset)", list_for, preq->source.value.mincalfrag,
                game.calendar.calendar_fragments-1);
      return FALSE;
    }
    break;
  case VUT_SERVERSETTING:
    /* There is currently no way to check a server setting's category and
     * access level that works in both the client and the server. */
    {
      server_setting_id id;
      struct setting *pset;

      id = ssetv_setting_get(preq->source.value.ssetval);
      fc_assert_ret_val(server_setting_exists(id), FALSE);
      pset = setting_by_number(id);

      if (!sanity_check_setting_is_seen(pset)) {
        log_error("%s: ServerSetting requirement %s isn't visible enough "
                  "to appear in a requirement. Everyone should be able to "
                  "see the value of a server setting that appears in a "
                  "requirement.", list_for, server_setting_name_get(id));
        return FALSE;
      }

      if (!sanity_check_setting_is_game_rule(pset)) {
        /* This is a server operator related setting (like the compression
         * type of savegames), not a game rule. */
        log_error("%s: ServerSetting requirement setting %s isn't about a "
                  "game rule.",
                  list_for, server_setting_name_get(id));
        return FALSE;
      }
    }
    break;
  default:
    /* No other universals have checks that can't be done at ruleset
     * load time. See req_from_str(). */
    break;
  }
  return TRUE;
}

/**********************************************************************//**
  Helper function for sanity_check_req_list() and sanity_check_req_vec()
**************************************************************************/
static bool sanity_check_req_set(int reqs_of_type[], int local_reqs_of_type[],
                                 struct requirement *preq, bool conjunctive,
                                 int max_tiles, const char *list_for)
{
  int rc;

  fc_assert_ret_val(universals_n_is_valid(preq->source.kind), FALSE);

  if (!sanity_check_req_individual(preq, list_for)) {
    return FALSE;
  }

  if (!conjunctive) {
    /* All the checks below are only meaningful for conjunctive lists. */
    /* FIXME: we could add checks suitable for disjunctive lists. */
    return TRUE;
  }

  /* Add to counter for positive requirements. */
  if (preq->present) {
    reqs_of_type[preq->source.kind]++;
  }
  rc = reqs_of_type[preq->source.kind];

  if (preq->range == REQ_RANGE_LOCAL && preq->present) {
    local_reqs_of_type[preq->source.kind]++;

    switch (preq->source.kind) {
     case VUT_TERRAINCLASS:
       if (local_reqs_of_type[VUT_TERRAIN] > 0) {
         log_error("%s: Requirement list has both local terrain and terrainclass requirement",
                   list_for);
         return FALSE;
       }
       break;
     case VUT_TERRAIN:
       if (local_reqs_of_type[VUT_TERRAINCLASS] > 0) {
         log_error("%s: Requirement list has both local terrain and terrainclass requirement",
                   list_for);
         return FALSE;
       }
       break;
     default:
       break;
    }
  }

  if (rc > 1 && preq->present) {
    /* Multiple requirements of the same type */
    switch (preq->source.kind) {
     case VUT_GOVERNMENT:
     case VUT_UTYPE:
     case VUT_UCLASS:
     case VUT_ACTION:
     case VUT_OTYPE:
     case VUT_SPECIALIST:
     case VUT_MINSIZE: /* Breaks nothing, but has no sense either */
     case VUT_MINMOVES: /* Breaks nothing, but has no sense either */
     case VUT_MINVETERAN: /* Breaks nothing, but has no sense either */
     case VUT_MINHP: /* Breaks nothing, but has no sense either */
     case VUT_MINYEAR:
     case VUT_MINCALFRAG:
     case VUT_AI_LEVEL:
     case VUT_TERRAINALTER: /* Local range only */
     case VUT_CITYTILE:
     case VUT_STYLE:
     case VUT_IMPR_GENUS:
       /* There can be only one requirement of these types (with current
        * range limitations)
        * Requirements might be identical, but we consider multiple
        * declarations error anyway. */

       log_error("%s: Requirement list has multiple %s requirements",
                 list_for, universal_type_rule_name(&preq->source));
       return FALSE;
       break;

     case VUT_TERRAIN:
       /* There can be only up to max_tiles requirements of these types */
       if (max_tiles != -1 && rc > max_tiles) {
         log_error("%s: Requirement list has more %s requirements than "
                   "can ever be fulfilled.", list_for,
                   universal_type_rule_name(&preq->source));
         return FALSE;
       }
       break;

     case VUT_TERRAINCLASS:
       if (rc > 2 || (max_tiles != -1 && rc > max_tiles)) {
         log_error("%s: Requirement list has more %s requirements than "
                   "can ever be fulfilled.", list_for,
                   universal_type_rule_name(&preq->source));
         return FALSE;
       }
       break;

    case VUT_AGE:
      /* There can be age of the city, unit, and player */
      if (rc > 3) {
        log_error("%s: Requirement list has more %s requirements than "
                  "can ever be fulfilled.", list_for,
                  universal_type_rule_name(&preq->source));
        return FALSE;
      }
      break;

    case VUT_MINTECHS:
      /* At ranges 'Player' and 'World' */
      if (rc > 2) {
        log_error("%s: Requirement list has more %s requirements than "
                  "can ever be fulfilled.", list_for,
                  universal_type_rule_name(&preq->source));
        return FALSE;
      }
      break;

     case VUT_SERVERSETTING:
       /* Can have multiple, since there are many settings. */
     case VUT_TOPO:
       /* Can have multiple, since it's flag based (iso & wrapx & wrapy & hex) */
     case VUT_EXTRA:
       /* Note that there can be more than 1 extra / tile. */
     case VUT_MAXTILEUNITS:
       /* Can require different numbers on e.g. local/adjacent tiles. */
     case VUT_NATION:
       /* Can require multiple nations at Team/Alliance/World range. */
     case VUT_NATIONGROUP:
       /* Nations can be in multiple groups. */
     case VUT_NONE:
     case VUT_ADVANCE:
     case VUT_TECHFLAG:
     case VUT_IMPROVEMENT:
     case VUT_UNITSTATE:
     case VUT_GOOD:
       /* Can check different properties. */
     case VUT_UTFLAG:
     case VUT_UCFLAG:
     case VUT_TERRFLAG:
     case VUT_BASEFLAG:
     case VUT_ROADFLAG:
     case VUT_EXTRAFLAG:
     case VUT_NATIONALITY:
     case VUT_MINCULTURE:
     case VUT_ACHIEVEMENT:
     case VUT_DIPLREL:
       /* Can have multiple requirements of these types */
       break;
     case VUT_COUNT:
       /* Should never be in requirement vector */
       fc_assert(FALSE);
       return FALSE;
       break;
       /* No default handling here, as we want compiler warning
        * if new requirement type is added to enum and it's not handled
        * here. */
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Sanity check requirement vector, including whether it's free of
  conflicting requirements.
  'conjunctive' should be TRUE if the vector is an AND vector (all requirements
  must be active), FALSE if it's a disjunctive (OR) vector.
  max_tiles is number of tiles that can provide requirement. Value -1
  disables checking based on number of tiles.

  Returns TRUE iff everything ok.

  TODO: This is based on current hardcoded range limitations.
        - There should be method of automatically determining these
          limitations for each requirement type
        - This function should check also problems caused by defining
          range to less than hardcoded max for requirement type
**************************************************************************/
static bool sanity_check_req_vec(const struct requirement_vector *preqs,
                                 bool conjunctive, int max_tiles,
                                 const char *list_for)
{
  int reqs_of_type[VUT_COUNT];
  int local_reqs_of_type[VUT_COUNT];

  /* Initialize requirement counters */
  memset(reqs_of_type, 0, sizeof(reqs_of_type));
  memset(local_reqs_of_type, 0, sizeof(local_reqs_of_type));

  requirement_vector_iterate(preqs, preq) {
    if (!sanity_check_req_set(reqs_of_type, local_reqs_of_type, preq,
                              conjunctive, max_tiles, list_for)) {
      return FALSE;
    }
    requirement_vector_iterate(preqs, nreq) {
      if (are_requirements_contradictions(preq, nreq)) {
        log_error("%s: Requirements {%s} and {%s} contradict each other.",
                  list_for,
                  req_to_fstring(preq),
                  req_to_fstring(nreq));
        return FALSE;
      }
    } requirement_vector_iterate_end;
  } requirement_vector_iterate_end;

  return TRUE;
}

/**********************************************************************//**
  Sanity check callback for iterating effects cache.
**************************************************************************/
static bool effect_list_sanity_cb(struct effect *peffect, void *data)
{
  int one_tile = -1; /* TODO: Determine correct value from effect.
                      *       -1 disables checking */

  return sanity_check_req_vec(&peffect->reqs, TRUE, one_tile,
                              effect_type_name(peffect->type));
}

/**********************************************************************//**
  Sanity check barbarian unit types
**************************************************************************/
static bool rs_barbarian_units(void)
{
  if (num_role_units(L_BARBARIAN) > 0) {
    if (num_role_units(L_BARBARIAN_LEADER) == 0) {
      ruleset_error(LOG_ERROR, "No role barbarian leader units");
      return FALSE;
    }
    if (num_role_units(L_BARBARIAN_BUILD) == 0) {
      ruleset_error(LOG_ERROR, "No role barbarian build units");
      return FALSE;
    }
    if (num_role_units(L_BARBARIAN_BOAT) == 0) {
      ruleset_error(LOG_ERROR, "No role barbarian ship units");
      return FALSE;
    } else if (num_role_units(L_BARBARIAN_BOAT) > 0) {
      bool sea_capable = FALSE;
      struct unit_type *u = get_role_unit(L_BARBARIAN_BOAT, 0);

      terrain_type_iterate(pterr) {
        if (is_ocean(pterr)
            && BV_ISSET(pterr->native_to, uclass_index(utype_class(u)))) {
          sea_capable = TRUE;
          break;
        }
      } terrain_type_iterate_end;

      if (!sea_capable) {
        ruleset_error(LOG_ERROR,
                      "Barbarian boat (%s) needs to be able to move at sea.",
                      utype_rule_name(u));
        return FALSE;
      }
    }
    if (num_role_units(L_BARBARIAN_SEA) == 0) {
      ruleset_error(LOG_ERROR, "No role sea raider barbarian units");
      return FALSE;
    }

    unit_type_iterate(ptype) {
      if (utype_has_role(ptype, L_BARBARIAN_BOAT)) {
        if (ptype->transport_capacity <= 1) {
          ruleset_error(LOG_ERROR,
                        "Barbarian boat %s has no capacity for both "
                        "leader and at least one man.",
                        utype_rule_name(ptype));
          return FALSE;
        }

        unit_type_iterate(pbarb) {
          if (utype_has_role(pbarb, L_BARBARIAN_SEA)
              || utype_has_role(pbarb, L_BARBARIAN_SEA_TECH)
              || utype_has_role(pbarb, L_BARBARIAN_LEADER)) {
            if (!can_unit_type_transport(ptype, utype_class(pbarb))) {
              ruleset_error(LOG_ERROR,
                            "Barbarian boat %s cannot transport "
                            "barbarian cargo %s.",
                            utype_rule_name(ptype),
                            utype_rule_name(pbarb));
              return FALSE;
            }
          }
        } unit_type_iterate_end;
      }
    } unit_type_iterate_end;
  }

  return TRUE;
}

/**********************************************************************//**
  Sanity check common unit types
**************************************************************************/
static bool rs_common_units(void)
{
  /* Check some required flags and roles etc: */
  if (num_role_units(UTYF_SETTLERS) == 0) {
    ruleset_error(LOG_ERROR, "No flag Settler units");
    return FALSE;
  }
  if (num_role_units(L_START_EXPLORER) == 0) {
    ruleset_error(LOG_ERROR, "No role Start Explorer units");
  }
  if (num_role_units(L_FERRYBOAT) == 0) {
    ruleset_error(LOG_ERROR, "No role Ferryboat units");
  }
  if (num_role_units(L_FIRSTBUILD) == 0) {
    ruleset_error(LOG_ERROR, "No role Firstbuild units");
  }

  if (num_role_units(L_FERRYBOAT) > 0) {
    bool sea_capable = FALSE;
    struct unit_type *u = get_role_unit(L_FERRYBOAT, 0);

    terrain_type_iterate(pterr) {
      if (is_ocean(pterr)
          && BV_ISSET(pterr->native_to, uclass_index(utype_class(u)))) {
        sea_capable = TRUE;
        break;
      }
    } terrain_type_iterate_end;

    if (!sea_capable) {
      ruleset_error(LOG_ERROR,
                    "Ferryboat (%s) needs to be able to move at sea.",
                    utype_rule_name(u));
      return FALSE;
    }
  }

  if (num_role_units(L_PARTISAN) == 0
      && effect_cumulative_max(EFT_INSPIRE_PARTISANS, NULL) > 0) {
    ruleset_error(LOG_ERROR,
                  "Inspire_Partisans effect present, but no units with partisan role.");
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Sanity check buildings
**************************************************************************/
static bool rs_buildings(void)
{
  /* Special Genus */
  improvement_iterate(pimprove) {
    if (improvement_has_flag(pimprove, IF_GOLD)
        && pimprove->genus != IG_SPECIAL) {
      ruleset_error(LOG_ERROR,
                    "Gold producing improvement with genus other than \"Special\"");

      return FALSE;
    }
    if (improvement_has_flag(pimprove, IF_DISASTER_PROOF)
        && pimprove->genus != IG_IMPROVEMENT) {
      ruleset_error(LOG_ERROR,
                    "Disasterproof improvement with genus other than \"Improvement\"");

      return FALSE;
    }
  } improvement_iterate_end;

  return TRUE;
}

/**********************************************************************//**
  Check that boolean effect types have sensible effects.
**************************************************************************/
static bool sanity_check_boolean_effects(void)
{
  enum effect_type boolean_effects[] =
    {
      EFT_ANY_GOVERNMENT,
      EFT_CAPITAL_CITY,
      EFT_ENABLE_NUKE,
      EFT_ENABLE_SPACE,
      EFT_HAVE_EMBASSIES,
      EFT_NO_ANARCHY,
      EFT_NUKE_PROOF,
      EFT_REVEAL_CITIES,
      EFT_REVEAL_MAP,
      EFT_SIZE_UNLIMIT,
      EFT_SS_STRUCTURAL,
      EFT_SS_COMPONENT,
      EFT_NO_UNHAPPY,
      EFT_RAPTURE_GROW,
      EFT_HAS_SENATE,
      EFT_INSPIRE_PARTISANS,
      EFT_HAPPINESS_TO_GOLD,
      EFT_FANATICS,
      EFT_NO_DIPLOMACY,
      EFT_GOV_CENTER,
      EFT_NOT_TECH_SOURCE,
      EFT_VICTORY,
      EFT_HAVE_CONTACTS,
      EFT_COUNT
    };
  int i;
  bool ret = TRUE;

  for (i = 0; boolean_effects[i] != EFT_COUNT; i++) {
    if (effect_cumulative_min(boolean_effects[i], NULL) < 0
        && effect_cumulative_max(boolean_effects[i], NULL) == 0) {
      ruleset_error(LOG_ERROR, "Boolean effect %s can get disabled, but it can't get "
                    "enabled before that.", effect_type_name(boolean_effects[i]));
      ret = FALSE;
    }
  }

  return ret;
}

/**********************************************************************//**
  Some more sanity checking once all rulesets are loaded. These check
  for some cross-referencing which was impossible to do while only one
  party was loaded in load_ruleset_xxx()

  Returns TRUE iff everything ok.
**************************************************************************/
bool sanity_check_ruleset_data(bool ignore_retired)
{
  int num_utypes;
  int i;
  bool ok = TRUE; /* Store failures to variable instead of returning
                   * immediately so all errors get printed, not just first
                   * one. */
  bool default_gov_failed = FALSE;

  if (!sanity_check_metadata()) {
    ok = FALSE;
  }

  if (game.info.tech_cost_style == TECH_COST_CIV1CIV2
      && game.info.free_tech_method == FTM_CHEAPEST) {
    ruleset_error(LOG_ERROR, "Cost based free tech method, but tech cost style "
                  "1 so all techs cost the same.");
    ok = FALSE;
  }

  /* Advances. */
  advance_iterate(A_FIRST, padvance) {
    for (i = AR_ONE; i < AR_SIZE; i++) {
      const struct advance *preq;

      if (i == AR_ROOT) {
        /* Self rootreq is a feature. */
        continue;
      }

      preq = advance_requires(padvance, i);

      if (A_NEVER == preq) {
        continue;
      } else if (preq == padvance) {
        ruleset_error(LOG_ERROR, "Tech \"%s\" requires itself.",
                      advance_rule_name(padvance));
        ok = FALSE;
        continue;
      }

      advance_req_iterate(preq, preqreq) {
        if (preqreq == padvance) {
          ruleset_error(LOG_ERROR,
                        "Tech \"%s\" requires itself indirectly via \"%s\".",
                        advance_rule_name(padvance),
                        advance_rule_name(preq));
          ok = FALSE;
        }
      } advance_req_iterate_end;
    }

    requirement_vector_iterate(&(padvance->research_reqs), preq) {
      if (preq->source.kind == VUT_ADVANCE) {
        /* Don't allow this even if allowing changing reqs. Players will
         * expect all tech reqs to appear in the client tech tree. That
         * should be taken care of first. */
        ruleset_error(LOG_ERROR,
                      "Tech \"%s\" requires a tech in its research_reqs."
                      " This isn't supported yet. Please keep using req1"
                      " and req2 like before.",
                      advance_rule_name(padvance));
        ok = FALSE;
      } else if (!is_req_unchanging(preq)) {
        /* Only support unchanging requirements until the reachability code
         * can handle it and the tech tree can display changing
         * requirements. */
        ruleset_error(LOG_ERROR,
                      "Tech \"%s\" has the requirement %s in its"
                      " research_reqs. This requirement may change during"
                      " the game. Changing requirements aren't supported"
                      " yet.",
                      advance_rule_name(padvance),
                      req_to_fstring(preq));
        ok = FALSE;
      }
    } requirement_vector_iterate_end;

    if (padvance->bonus_message != NULL) {
      if (!formats_match(padvance->bonus_message, "%s")) {
        ruleset_error(LOG_ERROR,
                      "Tech \"%s\" bonus message is not format with %%s for a bonus tech name.",
                      advance_rule_name(padvance));
        ok = FALSE;
      }
    }
  } advance_iterate_end;

  if (game.default_government == game.government_during_revolution) {
    ruleset_error(LOG_ERROR,
                  "The government form %s reserved for revolution handling has been set as "
                  "default_government.",
                  government_rule_name(game.government_during_revolution));
    ok = FALSE;
    default_gov_failed = TRUE;
  }

  /* Check that all players can have their initial techs */
  nations_iterate(pnation) {
    int techi;

    /* Check global initial techs */
    for (techi = 0; techi < MAX_NUM_TECH_LIST
         && game.rgame.global_init_techs[techi] != A_LAST; techi++) {
      Tech_type_id tech = game.rgame.global_init_techs[techi];
      struct advance *a = valid_advance_by_number(tech);

      if (a == NULL) {
        ruleset_error(LOG_ERROR,
                      "Tech %s does not exist, but is initial "
                      "tech for everyone.",
                      advance_rule_name(advance_by_number(tech)));
        ok = FALSE;
      } else if (advance_by_number(A_NONE) != a->require[AR_ROOT]
          && !nation_has_initial_tech(pnation, a->require[AR_ROOT])) {
        /* Nation has no root_req for tech */
        ruleset_error(LOG_ERROR,
                      "Tech %s is initial for everyone, but %s has "
                      "no root_req for it.",
                      advance_rule_name(a),
                      nation_rule_name(pnation));
        ok = FALSE;
      }
    }

    /* Check national initial techs */
    for (techi = 0;
         techi < MAX_NUM_TECH_LIST && pnation->init_techs[techi] != A_LAST;
         techi++) {
      Tech_type_id tech = pnation->init_techs[techi];
      struct advance *a = valid_advance_by_number(tech);

      if (a == NULL) {
        ruleset_error(LOG_ERROR,
                      "Tech %s does not exist, but is tech for %s.",
                      advance_rule_name(advance_by_number(tech)),
                      nation_rule_name(pnation));
        ok = FALSE;
      } else if (advance_by_number(A_NONE) != a->require[AR_ROOT]
          && !nation_has_initial_tech(pnation, a->require[AR_ROOT])) {
        /* Nation has no root_req for tech */
        ruleset_error(LOG_ERROR,
                      "Tech %s is initial for %s, but they have "
                      "no root_req for it.",
                      advance_rule_name(a),
                      nation_rule_name(pnation));
        ok = FALSE;
      }
    }

    /* Check national initial buildings */
    if (nation_barbarian_type(pnation) != NOT_A_BARBARIAN
        && pnation->init_buildings[0] != B_LAST) {
      ruleset_error(LOG_ERROR,
                    "Barbarian nation %s has init_buildings set but will "
                    "never see them", nation_rule_name(pnation));
    }

    if (!default_gov_failed && pnation->init_government == game.government_during_revolution) {
      ruleset_error(LOG_ERROR,
                    "The government form %s reserved for revolution handling has been set as "
                    "initial government for %s.",
                    government_rule_name(game.government_during_revolution),
                    nation_rule_name(pnation));
      ok = FALSE;
    }
  } nations_iterate_end;

  /* Check against unit upgrade loops */
  num_utypes = game.control.num_unit_types;
  unit_type_iterate(putype) {
    int chain_length = 0;
    struct unit_type *upgraded = putype;

    while (upgraded != NULL) {
      upgraded = upgraded->obsoleted_by;
      chain_length++;
      if (chain_length > num_utypes) {
        ruleset_error(LOG_ERROR,
                      "There seems to be obsoleted_by loop in update "
                      "chain that starts from %s", utype_rule_name(putype));
        ok = FALSE;
      }
    }
  } unit_type_iterate_end;

  /* Some unit type properties depend on other unit type properties to work
   * properly. */
  unit_type_iterate(putype) {
    /* "Spy" is a better "Diplomat". Until all the places that assume that
     * "Diplomat" is set if "Spy" is set is changed this limitation must be
     * kept. */
    if (utype_has_flag(putype, UTYF_SPY)
        && !utype_has_flag(putype, UTYF_DIPLOMAT)) {
      ruleset_error(LOG_ERROR,
                    "The unit type '%s' has the 'Spy' unit type flag but "
                    "not the 'Diplomat' unit type flag.",
                    utype_rule_name(putype));
      ok = FALSE;
    }
  } unit_type_iterate_end;

  /* Check that unit type fields are in range. */
  unit_type_iterate(putype) {
    if (putype->paratroopers_range < 0
        || putype->paratroopers_range > UNIT_MAX_PARADROP_RANGE) {
      /* Paradrop range is limited by the network protocol. */
      ruleset_error(LOG_ERROR,
                    "The paratroopers_range of the unit type '%s' is %d. "
                    "That is out of range. Max range is %d.",
                    utype_rule_name(putype),
                    putype->paratroopers_range, UNIT_MAX_PARADROP_RANGE);
      ok = FALSE;
    }
  } unit_type_iterate_end;

  /* Check requirement sets against conflicting requirements.
   * Effects use requirement lists */
  if (!iterate_effect_cache(effect_list_sanity_cb, NULL)) {
    ruleset_error(LOG_ERROR,
                  "Effects have conflicting or invalid requirements!");
    ok = FALSE;
  }

  if (!sanity_check_boolean_effects()) {
    ok = FALSE;
  }

  /* Others use requirement vectors */

  /* Disasters */
  disaster_type_iterate(pdis) {
    if (!sanity_check_req_vec(&pdis->reqs, TRUE, -1,
                              disaster_rule_name(pdis))) {
      ruleset_error(LOG_ERROR,
                    "Disasters have conflicting or invalid requirements!");
      ok = FALSE;
    }
  } disaster_type_iterate_end;

  /* Goods */
  goods_type_iterate(pgood) {
    if (!sanity_check_req_vec(&pgood->reqs, TRUE, -1,
                              goods_rule_name(pgood))) {
      ruleset_error(LOG_ERROR,
                    "Goods have conflicting or invalid requirements!");
      ok = FALSE;
    }
  } goods_type_iterate_end;

  /* Buildings */
  improvement_iterate(pimprove) {
    if (!sanity_check_req_vec(&pimprove->reqs, TRUE, -1,
                              improvement_rule_name(pimprove))) {
      ruleset_error(LOG_ERROR,
                    "Buildings have conflicting or invalid requirements!");
      ok = FALSE;
    }
    if (!sanity_check_req_vec(&pimprove->obsolete_by, FALSE, -1,
                              improvement_rule_name(pimprove))) {
      ruleset_error(LOG_ERROR,
                    "Buildings have conflicting or invalid obsolescence req!");
      ok = FALSE;
    }
  } improvement_iterate_end;

  /* Governments */
  governments_iterate(pgov) {
    if (!sanity_check_req_vec(&pgov->reqs, TRUE, -1,
                              government_rule_name(pgov))) {
      ruleset_error(LOG_ERROR,
                    "Governments have conflicting or invalid requirements!");
      ok = FALSE;
    }
  } governments_iterate_end;

  /* Specialists */
  specialist_type_iterate(sp) {
    struct specialist *psp = specialist_by_number(sp);

    if (!sanity_check_req_vec(&psp->reqs, TRUE, -1,
                              specialist_rule_name(psp))) {
      ruleset_error(LOG_ERROR,
                    "Specialists have conflicting or invalid requirements!");
      ok = FALSE;
    }
  } specialist_type_iterate_end;

  /* Extras */
  extra_type_iterate(pextra) {
    if (!sanity_check_req_vec(&pextra->reqs, TRUE, -1,
                              extra_rule_name(pextra))) {
      ruleset_error(LOG_ERROR,
                    "Extras have conflicting or invalid requirements!");
      ok = FALSE;
    }
    if (!sanity_check_req_vec(&pextra->rmreqs, TRUE, -1,
                              extra_rule_name(pextra))) {
      ruleset_error(LOG_ERROR,
                    "Extras have conflicting or invalid removal requirements!");
      ok = FALSE;
    }
  } extra_type_iterate_end;

  /* Roads */
  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    struct road_type *proad = extra_road_get(pextra);

    extra_type_list_iterate(proad->integrators, iextra) {
      struct road_type *iroad = extra_road_get(iextra);
      int pnbr = road_number(proad);

      if (pnbr != road_number(iroad)
          && !BV_ISSET(iroad->integrates, pnbr)) {
        /* We don't support non-symmetric integrator relationships yet. */
        ruleset_error(LOG_ERROR,
                      "Road '%s' integrates with '%s' but not vice versa!",
                      extra_rule_name(pextra),
                      extra_rule_name(iextra));
        ok = FALSE;
      }
    } extra_type_list_iterate_end;
  } extra_type_by_cause_iterate_end;

  /* Bases */
  extra_type_by_cause_iterate(EC_BASE, pextra) {
    int bfi;
    struct base_type *pbase = extra_base_get(pextra);

    if (ignore_retired) {
      /* Base flags haven't been updated yet. */
      break;
    }

    for (bfi = 0; bfi < BF_COUNT; bfi++) {
      if (!base_flag_is_retired(bfi)) {
        /* Still valid. */
        continue;
      }

      if (BV_ISSET(pbase->flags, bfi)) {
        ruleset_error(LOG_ERROR,
                      "Base %s uses the retired base flag %s!",
                      extra_name_translation(pextra),
                      base_flag_id_name(bfi));
      }
    }
  } extra_type_by_cause_iterate_end;

  /* City styles */
  for (i = 0; i < game.control.styles_count; i++) {
    if (!sanity_check_req_vec(&city_styles[i].reqs, TRUE, -1,
                              city_style_rule_name(i))) {
      ruleset_error(LOG_ERROR,
                    "City styles have conflicting or invalid requirements!");
      ok = FALSE;
    }
  }

  /* Actions */
  action_iterate(act) {
    struct action *paction = action_by_number(act);

    if (paction->min_distance < 0) {
      ruleset_error(LOG_ERROR, "Action %s: negative min distance (%d).",
                    action_id_rule_name(act), paction->min_distance);
      ok = FALSE;
    }

    if (paction->min_distance > ACTION_DISTANCE_LAST_NON_SIGNAL) {
      ruleset_error(LOG_ERROR, "Action %s: min distance (%d) larger than "
                               "any distance on a map can be (%d).",
                    action_id_rule_name(act), paction->min_distance,
                    ACTION_DISTANCE_LAST_NON_SIGNAL);
      ok = FALSE;
    }

    if (paction->max_distance > ACTION_DISTANCE_MAX) {
      ruleset_error(LOG_ERROR, "Action %s: max distance is %d. "
                    "A map can't be that big.",
                    action_id_rule_name(act), paction->max_distance);
      ok = FALSE;
    }

    if (!action_distance_inside_max(paction, paction->min_distance)) {
      ruleset_error(LOG_ERROR,
                    "Action %s: min distance is %d but max distance is %d.",
                    action_id_rule_name(act),
                    paction->min_distance, paction->max_distance);
      ok = FALSE;
    }

    action_iterate(blocker) {
      if (BV_ISSET(paction->blocked_by, blocker)
          && action_id_get_target_kind(blocker) == ATK_UNIT
          && action_id_get_target_kind(act) != ATK_UNIT) {
        /* Can't find an individual unit target to evaluate the blocking
         * action against. (A tile may have more than one individual
         * unit) */
        ruleset_error(LOG_ERROR,
                      "The action %s can't block %s.",
                      action_id_rule_name(blocker),
                      action_id_rule_name(act));
        ok = FALSE;
      }
    } action_iterate_end;

    action_enabler_list_iterate(action_enablers_for_action(act), enabler) {
      if (!sanity_check_req_vec(&(enabler->actor_reqs), TRUE, -1,
                                "Action Enabler Actor Reqs")
          || !sanity_check_req_vec(&(enabler->target_reqs), TRUE, -1,
                                   "Action Enabler Target Reqs")) {
        ruleset_error(LOG_ERROR,
                      "Action enabler for %s has conflicting or invalid "
                      "requirements!", action_id_rule_name(act));
        ok = FALSE;
      }

      if (action_id_get_target_kind(enabler->action) == ATK_SELF) {
        /* Special test for self targeted actions. */

        if (requirement_vector_size(&(enabler->target_reqs)) > 0) {
          /* Shouldn't have target requirements since the action doesn't
           * have a target. */
          ruleset_error(LOG_ERROR,
                        "An action enabler for %s has a target "
                        "requirement vector. %s doesn't have a target.",
                        action_id_rule_name(act),
                        action_id_rule_name(act));
          ok = FALSE;
        }
      }

      requirement_vector_iterate(&(enabler->target_reqs), preq) {
        if (preq->source.kind == VUT_DIPLREL
            && preq->range == REQ_RANGE_LOCAL) {
          /* A Local DiplRel requirement can be expressed as a requirement
           * in actor_reqs. Demand that it is there. This avoids breaking
           * code that reasons about actions. */
          ruleset_error(LOG_ERROR,
                        "Action enabler for %s has a local DiplRel "
                        "requirement %s in target_reqs! Please read the "
                        "section \"Requirement vector rules\" in "
                        "doc/README.actions",
                        action_id_rule_name(act),
                        req_to_fstring(preq));
          ok = FALSE;
        }
      } requirement_vector_iterate_end;

      if (!ignore_retired) {
        /* Support for letting the following hard requirements be implicit
         * were retired in Freeciv 3.0. Make sure that the opposite of each
         * hard action requirement blocks all its action enablers. */

        const char *error_message;

        if ((error_message
             = action_enabler_obligatory_reqs_missing(enabler))) {
            ruleset_error(LOG_ERROR, error_message,
                          action_id_rule_name(act));
            ok = FALSE;
          }
      }
    } action_enabler_list_iterate_end;
  } action_iterate_end;

  /* There must be basic city style for each nation style to start with */
  styles_iterate(pstyle) {
    if (basic_city_style_for_style(pstyle) < 0) {
      ruleset_error(LOG_ERROR,
                    "There's no basic city style for nation style %s",
                    style_rule_name(pstyle));
      ok = FALSE;
    }
  } styles_iterate_end;

  /* Music styles */
  music_styles_iterate(pmus) {
    if (!sanity_check_req_vec(&pmus->reqs, TRUE, -1, "Music Style")) {
      ruleset_error(LOG_ERROR,
                    "Music Styles have conflicting or invalid requirements!");
      ok = FALSE;
    }
  } music_styles_iterate_end;

  terrain_type_iterate(pterr) {
    if (pterr->animal != NULL) {
      if (!is_native_to_class(utype_class(pterr->animal), pterr, NULL)) {
        ruleset_error(LOG_ERROR,
                      "%s has %s as animal to appear, but it's not native to the terrain.",
                      terrain_rule_name(pterr), utype_rule_name(pterr->animal));
        ok = FALSE;
      }
    }
  } terrain_type_iterate_end;

  /* Check that all unit classes can exist somewhere */
  unit_class_iterate(pclass) {
    if (!uclass_has_flag(pclass, UCF_BUILD_ANYWHERE)) {
      bool can_exist = FALSE;

      terrain_type_iterate(pterr) {
        if (BV_ISSET(pterr->native_to, uclass_index(pclass))) {
          can_exist = TRUE;
          break;
        }
      } terrain_type_iterate_end;

      if (!can_exist) {
        extra_type_iterate(pextra) {
          if (BV_ISSET(pextra->native_to, uclass_index(pclass))
              && extra_has_flag(pextra, EF_NATIVE_TILE)) {
            can_exist = TRUE;
            break;
          }
        } extra_type_iterate_end;
      }

      if (!can_exist) {
        ruleset_error(LOG_ERROR,
                      "Unit class %s cannot exist anywhere.",
                      uclass_rule_name(pclass));
        ok = FALSE;
      }
    }
  } unit_class_iterate_end;

  achievements_iterate(pach) {
    if (!pach->unique && pach->cons_msg == NULL) {
      ruleset_error(LOG_ERROR,
                    "Achievement %s has no message for consecutive gainers though "
                    "it's possible to be gained by multiple players",
                    achievement_rule_name(pach));
      ok = FALSE;
    }
  } achievements_iterate_end;

  if (game.server.ruledit.embedded_nations != NULL) {
    int nati;

    for (nati = 0; nati < game.server.ruledit.embedded_nations_count; nati++) {
      struct nation_type *pnat
        = nation_by_rule_name(game.server.ruledit.embedded_nations[nati]);

      if (pnat == NULL) {
        ruleset_error(LOG_ERROR,
                      "There's nation %s listed in embedded nations, but there's "
                      "no such nation.",
                      game.server.ruledit.embedded_nations[nati]);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    ok = rs_common_units();
  }
  if (ok) {
    ok = rs_barbarian_units();
  }
  if (ok) {
    ok = rs_buildings();
  }

  return ok;
}

/**********************************************************************//**
  Apply some automatic defaults to already loaded rulesets.

  Returns TRUE iff everything ok.
**************************************************************************/
bool autoadjust_ruleset_data(void)
{
  bool ok = TRUE;
  
  extra_type_by_cause_iterate(EC_RESOURCE, pextra) {
    extra_type_by_cause_iterate(EC_RESOURCE, pextra2) {
      if (pextra != pextra2) {
        int idx = extra_index(pextra2);

        if (!BV_ISSET(pextra->conflicts, idx)) {
          log_debug("Autoconflicting resource %s with %s",
                    extra_rule_name(pextra), extra_rule_name(pextra2));
          BV_SET(pextra->conflicts, extra_index(pextra2));
        }
      }
    } extra_type_by_cause_iterate_end;
  } extra_type_by_cause_iterate_end;

  /* Hard coded action blocking. */
  {
    const struct {
      const enum gen_action blocked;
      const enum gen_action blocker;
    } must_block[] = {
      /* Hard code that Help Wonder blocks Recycle Unit. This must be done
       * because caravan_shields makes it possible to avoid the
       * consequences of choosing to do Recycle Unit rather than having it
       * do Help Wonder.
       *
       * Explanation: Recycle Unit adds 50% of the shields used to produce
       * the unit to the production of the city where it is located. Help
       * Wonder adds 100%. If a unit that can do Help Wonder is recycled in
       * a city and the production later is changed to something that can
       * receive help from Help Wonder the remaining 50% of the shields are
       * added. This can be done because the city remembers them in
       * caravan_shields.
       *
       * If a unit that can do Help Wonder intentionally is recycled rather
       * than making it do Help Wonder its shields will still be
       * remembered. The target city that got 50% of the shields can
       * therefore get 100% of them by changing its production. This trick
       * makes the ability to select Recycle Unit when Help Wonder is legal
       * pointless. */
      { ACTION_RECYCLE_UNIT, ACTION_HELP_WONDER },

      /* Allowing regular disband when ACTION_HELP_WONDER or
       * ACTION_RECYCLE_UNIT is legal while ACTION_HELP_WONDER always
       * blocks ACTION_RECYCLE_UNIT doesn't work well with the force_*
       * semantics. Should move to the ruleset once it has blocked_by
       * semantics. */
      { ACTION_DISBAND_UNIT, ACTION_HELP_WONDER },
      { ACTION_DISBAND_UNIT, ACTION_RECYCLE_UNIT },

      /* Hard code that the ability to perform a regular attack blocks city
       * conquest. Is redundant as long as the requirement that the target
       * tile has no units remains hard coded. Kept "just in case" that
       * changes. */
      { ACTION_CONQUER_CITY, ACTION_ATTACK },
      { ACTION_CONQUER_CITY, ACTION_SUICIDE_ATTACK },
    };

    int i;

    for (i = 0; i < ARRAY_SIZE(must_block); i++) {
      enum gen_action blocked = must_block[i].blocked;
      enum gen_action blocker = must_block[i].blocker;

      if (!action_id_would_be_blocked_by(blocked, blocker)) {
        log_verbose("Autoblocking %s with %s",
                    action_id_rule_name(blocked),
                    action_id_rule_name(blocker));
        BV_SET(action_by_number(blocked)->blocked_by, blocker);
      }
    }
  }

  return ok;
}

/**********************************************************************//**
  Set and lock settings that must have certain value.
**************************************************************************/
bool autolock_settings(void)
{
  bool ok = TRUE;

  if (num_role_units(L_BARBARIAN) == 0) {
    struct setting *pset = setting_by_name("barbarians");

    log_normal(_("Disabling 'barbarians' setting for lack of suitable "
                 "unit types."));
    setting_lock_set(pset, FALSE);
    if (!setting_enum_set(pset, "DISABLED", NULL, NULL, 0)) {
      ok = FALSE;
    }
    setting_lock_set(pset, TRUE);
  }

  return ok;
}
