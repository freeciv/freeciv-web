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
#include <string.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "game.h"
#include "government.h"
#include "movement.h"
#include "player.h"
#include "tech.h"
#include "unitlist.h"

#include "unittype.h"

static struct unit_type unit_types[U_LAST];
static struct unit_class unit_classes[UCL_LAST];
/* the unit_types and unit_classes arrays are now setup in:
   server/ruleset.c (for the server)
   client/packhand.c (for the client)
*/

static const char *unit_class_flag_names[] = {
  "TerrainSpeed", "TerrainDefense", "DamageSlows", "CanOccupyCity", "Missile",
  "RoadNative", "RiverNative", "BuildAnywhere", "Unreachable",
  "CollectRansom", "ZOC", "CanFortify", "CanPillage", "DoesntOccupyTile"
};
static const char *flag_names[] = {
  "TradeRoute", "HelpWonder", "IgZOC", "NonMil", "IgTer", 
  "OneAttack", "Pikemen", "Horse", "IgWall", "FieldUnit", 
  "AEGIS", "Marines", "Partial_Invis", "Settlers", "Diplomat",
  "Trireme", "Nuclear", "Spy", "Transform", "Paratroopers",
  "Cities", "No_Land_Attack",
  "AddToCity", "Fanatic", "GameLoss", "Unique", "Unbribable", 
  "Undisbandable", "SuperSpy", "NoHome", "NoVeteran", "Bombarder",
  "CityBuster", "NoBuild", "BadWallAttacker", "BadCityDefender",
  "Helicopter", "AirUnit", "Fighter", "BarbarianOnly", "Shield2Gold"
};
static char *user_flag_names[MAX_NUM_USER_UNIT_FLAGS] = { NULL, NULL, NULL, NULL };
static const char *role_names[] = {
  "FirstBuild", "Explorer", "Hut", "HutTech", "Partisan",
  "DefendOk", "DefendGood", "AttackFast", "AttackStrong",
  "Ferryboat", "Barbarian", "BarbarianTech", "BarbarianBoat",
  "BarbarianBuild", "BarbarianBuildTech", "BarbarianLeader",
  "BarbarianSea", "BarbarianSeaTech", "Cities", "Settlers",
  "GameLoss", "Diplomat", "Hunter"
};

/**************************************************************************
  Return the first item of unit_types.
**************************************************************************/
struct unit_type *unit_type_array_first(void)
{
  if (game.control.num_unit_types > 0) {
    return unit_types;
  }
  return NULL;
}

/**************************************************************************
  Return the last item of unit_types.
**************************************************************************/
const struct unit_type *unit_type_array_last(void)
{
  if (game.control.num_unit_types > 0) {
    return &unit_types[game.control.num_unit_types - 1];
  }
  return NULL;
}

/**************************************************************************
  Return the number of unit types.
**************************************************************************/
Unit_type_id utype_count(void)
{
  return game.control.num_unit_types;
}

/**************************************************************************
  Return the unit type index.

  Currently same as utype_number(), paired with utype_count()
  indicates use as an array index.
**************************************************************************/
Unit_type_id utype_index(const struct unit_type *punittype)
{
  assert(punittype);
  return punittype - unit_types;
}

/**************************************************************************
  Return the unit type index.
**************************************************************************/
Unit_type_id utype_number(const struct unit_type *punittype)
{
  assert(punittype);
  return punittype->item_number;
}

/**************************************************************************
  Return a pointer for the unit type struct for the given unit type id.

  This function returns NULL for invalid unit pointers (some callers
  rely on this).
**************************************************************************/
struct unit_type *utype_by_number(const Unit_type_id id)
{
  if (id < 0 || id > game.control.num_unit_types) {
    return NULL;
  }
  return &unit_types[id];
}

/**************************************************************************
  Return the unit type for this unit.
**************************************************************************/
struct unit_type *unit_type(const struct unit *punit)
{
  return punit->utype;
}


/**************************************************************************
  Returns the upkeep of a unit of this type under the given government.
**************************************************************************/
int utype_upkeep_cost(const struct unit_type *ut, struct player *pplayer,
                      Output_type_id otype)
{
  int val = ut->upkeep[otype], gold_upkeep_factor;

  if (get_player_bonus(pplayer, EFT_FANATICS)
      && BV_ISSET(ut->flags, F_FANATIC)) {
    /* Special case: fanatics have no upkeep under fanaticism. */
    return 0;
  }

  /* switch shield upkeep to gold upkeep if
     - the effect 'EFT_SHIELD2GOLD_FACTOR' is non-zero (it gives the
       conversion factor in percent) and
     - the unit has the corresponding flag set (F_SHIELD2GOLD)
     FIXME: Should the ai know about this? */
  gold_upkeep_factor = get_player_bonus(pplayer, EFT_SHIELD2GOLD_FACTOR);
  gold_upkeep_factor = (gold_upkeep_factor > 0) ? gold_upkeep_factor : 0;
  if (gold_upkeep_factor > 0 && utype_has_flag(ut, F_SHIELD2GOLD)) {
    switch (otype) {
      case O_GOLD:
        val = ceil((0.01 * gold_upkeep_factor) * ut->upkeep[O_SHIELD]);
        break;
      case O_SHIELD:
        val = 0;
        break;
      default:
        /* fall through */
        break;
    }
  }

  val *= get_player_output_bonus(pplayer, get_output_type(otype), 
                                 EFT_UPKEEP_FACTOR);
  return val;
}

/**************************************************************************
  Return the "happy cost" (the number of citizens who are discontented)
  for this unit.
**************************************************************************/
int utype_happy_cost(const struct unit_type *ut, 
                     const struct player *pplayer)
{
  return ut->happy_cost * get_player_bonus(pplayer, EFT_UNHAPPY_FACTOR);
}

/**************************************************************************
  Return whether the given unit class has the flag.
**************************************************************************/
bool uclass_has_flag(const struct unit_class *punitclass,
		     enum unit_class_flag_id flag)
{
  assert(flag >= 0 && flag < UCF_LAST);
  return BV_ISSET(punitclass->flags, flag);
}

/**************************************************************************
  Return whether the given unit type has the flag.
**************************************************************************/
bool utype_has_flag(const struct unit_type *punittype, int flag)
{
  assert(flag>=0 && flag<F_LAST);
  return BV_ISSET(punittype->flags, flag);
}

/**************************************************************************
  Return whether the unit has the given flag.
**************************************************************************/
bool unit_has_type_flag(const struct unit *punit, enum unit_flag_id flag)
{
  return utype_has_flag(unit_type(punit), flag);
}

/**************************************************************************
  Return whether the given unit type has the role.  Roles are like
  flags but have no meaning except to the AI.
**************************************************************************/
bool utype_has_role(const struct unit_type *punittype, int role)
{
  assert(role>=L_FIRST && role<L_LAST);
  return BV_ISSET(punittype->roles, role - L_FIRST);
}

/**************************************************************************
  Return whether the unit has the given role.
**************************************************************************/
bool unit_has_type_role(const struct unit *punit, enum unit_role_id role)
{
  return utype_has_role(unit_type(punit), role);
}

/****************************************************************************
  Returns the number of shields it takes to build this unit type.
****************************************************************************/
int utype_build_shield_cost(const struct unit_type *punittype)
{
  return MAX(punittype->build_cost * game.info.shieldbox / 100, 1);
}

/****************************************************************************
  Returns the number of shields it takes to build this unit.
****************************************************************************/
int unit_build_shield_cost(const struct unit *punit)
{
  return utype_build_shield_cost(unit_type(punit));
}

/****************************************************************************
  Returns the amount of gold it takes to rush this unit.
****************************************************************************/
int utype_buy_gold_cost(const struct unit_type *punittype,
			int shields_in_stock)
{
  int cost = 0;
  const int missing = utype_build_shield_cost(punittype) - shields_in_stock;

  if (missing > 0) {
    cost = 2 * missing + (missing * missing) / 20;
  }
  if (shields_in_stock == 0) {
    cost *= 2;
  }
  return cost;
}

/****************************************************************************
  Returns the number of shields received when this unit type is disbanded.
****************************************************************************/
int utype_disband_shields(const struct unit_type *punittype)
{
  return utype_build_shield_cost(punittype) / 2;
}

/****************************************************************************
  Returns the number of shields received when this unit is disbanded.
****************************************************************************/
int unit_disband_shields(const struct unit *punit)
{
  return utype_disband_shields(unit_type(punit));
}

/**************************************************************************
...
**************************************************************************/
int utype_pop_value(const struct unit_type *punittype)
{
  return (punittype->pop_cost);
}

/**************************************************************************
...
**************************************************************************/
int unit_pop_value(const struct unit *punit)
{
  return utype_pop_value(unit_type(punit));
}

/**************************************************************************
  Return move type of the unit class
**************************************************************************/
enum unit_move_type uclass_move_type(const struct unit_class *pclass)
{
  return pclass->move_type;
}

/**************************************************************************
  Return move type of the unit type
**************************************************************************/
enum unit_move_type utype_move_type(const struct unit_type *punittype)
{
  return uclass_move_type(utype_class(punittype));
}

/**************************************************************************
  Return the (translated) name of the unit type.
  You don't have to free the return pointer.
**************************************************************************/
const char *utype_name_translation(struct unit_type *punittype)
{
  if (NULL == punittype->name.translated) {
    /* delayed (unified) translation */
    punittype->name.translated = ('\0' == punittype->name.vernacular[0])
				 ? punittype->name.vernacular
				 : Q_(punittype->name.vernacular);
  }
  return punittype->name.translated;
}

/**************************************************************************
  Return the (translated) name of the unit.
  You don't have to free the return pointer.
**************************************************************************/
const char *unit_name_translation(struct unit *punit)
{
  return utype_name_translation(unit_type(punit));
}

/**************************************************************************
  Return the (untranslated) rule name of the unit type.
  You don't have to free the return pointer.
**************************************************************************/
const char *utype_rule_name(const struct unit_type *punittype)
{
  return Qn_(punittype->name.vernacular);
}

/**************************************************************************
  Return the (untranslated) rule name of the unit.
  You don't have to free the return pointer.
**************************************************************************/
const char *unit_rule_name(const struct unit *punit)
{
  return utype_rule_name(unit_type(punit));
}

/**************************************************************************
...
**************************************************************************/
const char *utype_values_string(const struct unit_type *punittype)
{
  static char buffer[256];

  if (utype_fuel(punittype)) {
    my_snprintf(buffer, sizeof(buffer),
		"%d/%d/%d(%d)",
		punittype->attack_strength,
		punittype->defense_strength,
		punittype->move_rate / 3,
		punittype->move_rate / 3 * utype_fuel(punittype));
  } else {
    my_snprintf(buffer, sizeof(buffer),
		"%d/%d/%d",
		punittype->attack_strength,
		punittype->defense_strength,
		punittype->move_rate / 3);
  }
  return buffer;
}

/**************************************************************************
...
**************************************************************************/
const char *utype_values_translation(struct unit_type *punittype)
{
  static char buffer[256];

  my_snprintf(buffer, sizeof(buffer),
              "%s [%s]",
              utype_name_translation(punittype),
              utype_values_string(punittype));
  return buffer;
}

/**************************************************************************
  Return the (translated) name of the unit class.
  You don't have to free the return pointer.
**************************************************************************/
const char *uclass_name_translation(struct unit_class *pclass)
{
  if (NULL == pclass->name.translated) {
    /* delayed (unified) translation */
    pclass->name.translated = ('\0' == pclass->name.vernacular[0])
			      ? pclass->name.vernacular
			      : Q_(pclass->name.vernacular);
  }
  return pclass->name.translated;
}

/**************************************************************************
  Return the (untranslated) rule name of the unit class.
  You don't have to free the return pointer.
**************************************************************************/
const char *uclass_rule_name(const struct unit_class *pclass)
{
  return Qn_(pclass->name.vernacular);
}

/**************************************************************************
 Return a string with all the names of units with this flag
 Return NULL if no unit with this flag exists.
 The string must be free'd

 TODO: if there are more than 4 units with this flag return
       a fallback string (e.g. first unit name + "and similar units"
**************************************************************************/
const char *role_units_translations(int flag)
{
  int count = num_role_units(flag);

  if (count == 1) {
    return mystrdup(utype_name_translation(get_role_unit(flag, 0)));
  }

  if (count > 0) {
    struct astring astr;

    astr_init(&astr);
    astr_minsize(&astr, 1);
    astr.str[0] = 0;

    while ((count--) > 0) {
      struct unit_type *u = get_role_unit(flag, count);
      const char *unitname = utype_name_translation(u);

      /* there should be something like astr_append() */
      astr_minsize(&astr, astr.n + strlen(unitname));
      strcat(astr.str, unitname);

      if (count == 1) {
	const char *and_str = _(" and ");

        astr_minsize(&astr, astr.n + strlen(and_str));
        strcat(astr.str, and_str);
      } else {
        if (count != 0) {
	  const char *and_comma = Q_("?and:, ");

	  astr_minsize(&astr, astr.n + strlen(and_comma));
	  strcat(astr.str, and_comma);
        } else {
	  return astr.str;
	}
      }
    }
  }
  return NULL;
}

/**************************************************************************
  Return whether this player can upgrade this unit type (to any other
  unit type).  Returns NULL if no upgrade is possible.
**************************************************************************/
struct unit_type *can_upgrade_unittype(const struct player *pplayer,
				       struct unit_type *punittype)
{
  struct unit_type *upgrade = punittype;
  struct unit_type *best_upgrade = NULL;

  if (!can_player_build_unit_direct(pplayer, punittype)) {
    return NULL;
  }
  while ((upgrade = upgrade->obsoleted_by) != U_NOT_OBSOLETED) {
    if (can_player_build_unit_direct(pplayer, upgrade)) {
      best_upgrade = upgrade;
    }
  }

  return best_upgrade;
}

/**************************************************************************
  Return the cost (gold) of upgrading a single unit of the specified type
  to the new type.  This price could (but currently does not) depend on
  other attributes (like nation or government type) of the player the unit
  belongs to.
**************************************************************************/
int unit_upgrade_price(const struct player *pplayer,
		       const struct unit_type *from,
		       const struct unit_type *to)
{
  int base_cost = utype_buy_gold_cost(to, utype_disband_shields(from));

  return base_cost
    * (100 + get_player_bonus(pplayer, EFT_UPGRADE_PRICE_PCT))
    / 100;
}

/**************************************************************************
  Returns the unit type that has the given (translated) name.
  Returns NULL if none match.
**************************************************************************/
struct unit_type *find_unit_type_by_translated_name(const char *name)
{
  unit_type_iterate(punittype) {
    if (0 == strcmp(utype_name_translation(punittype), name)) {
      return punittype;
    }
  } unit_type_iterate_end;

  return NULL;
}

/**************************************************************************
  Returns the unit type that has the given (untranslated) rule name.
  Returns NULL if none match.
**************************************************************************/
struct unit_type *find_unit_type_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  unit_type_iterate(punittype) {
    if (0 == mystrcasecmp(utype_rule_name(punittype), qname)) {
      return punittype;
    }
  } unit_type_iterate_end;

  return NULL;
}

/**************************************************************************
  Returns the unit class that has the given (untranslated) rule name.
  Returns NULL if none match.
**************************************************************************/
struct unit_class *find_unit_class_by_rule_name(const char *s)
{
  const char *qs = Qn_(s);

  unit_class_iterate(pclass) {
    if (0 == mystrcasecmp(uclass_rule_name(pclass), qs)) {
      return pclass;
    }
  } unit_class_iterate_end;
  return NULL;
}

/**************************************************************************
  Convert unit class flag names to enum; case insensitive;
  returns UCF_LAST if can't match.
**************************************************************************/
enum unit_class_flag_id find_unit_class_flag_by_rule_name(const char *s)
{
  enum unit_class_flag_id i;

  assert(ARRAY_SIZE(unit_class_flag_names) == UCF_LAST);
  
  for(i = 0; i < UCF_LAST; i++) {
    if (mystrcasecmp(unit_class_flag_names[i], s)==0) {
      return i;
    }
  }
  return UCF_LAST;
}

/**************************************************************************
  Return the (untranslated) rule name of the unit class flag.
**************************************************************************/
const char *unit_class_flag_rule_name(enum unit_class_flag_id id)
{
  assert(ARRAY_SIZE(unit_class_flag_names) == UCF_LAST);
  assert(id >= 0 && id < UCF_LAST);
  return unit_class_flag_names[id];
}

/**************************************************************************
  Sets user defined name for unit flag.
**************************************************************************/
void set_user_unit_flag_name(enum unit_flag_id id, const char *name)
{
  int ufid = id - F_USER_FLAG_1;

  assert(id >= F_USER_FLAG_1 && id < F_LAST);

  if (user_flag_names[ufid] != NULL) {
    free(user_flag_names[ufid]);
    user_flag_names[ufid] = NULL;
  }

  if (name && name[0] != '\0') {
    user_flag_names[ufid] = mystrdup(name);
  }
}

/**************************************************************************
  Convert flag names to enum; case insensitive;
  returns F_LAST if can't match.
**************************************************************************/
enum unit_flag_id find_unit_flag_by_rule_name(const char *s)
{
  enum unit_flag_id i;

  assert(ARRAY_SIZE(flag_names) == F_USER_FLAG_1);
  
  for (i = 0; i < F_USER_FLAG_1; i++) {
    if (mystrcasecmp(flag_names[i], s) == 0) {
      return i;
    }
  }
  for (i = 0; i < MAX_NUM_USER_UNIT_FLAGS; i++) {
    if (user_flag_names[i] != NULL
        && mystrcasecmp(user_flag_names[i], s) == 0) {
      return i + F_USER_FLAG_1;
    }
  }

  return F_LAST;
}

/**************************************************************************
  Return the (untranslated) rule name of the unit flag.
**************************************************************************/
const char *unit_flag_rule_name(enum unit_flag_id id)
{
  assert(ARRAY_SIZE(flag_names) == F_USER_FLAG_1);
  assert(id >= 0 && id < F_LAST);

  if (id < F_USER_FLAG_1) {
    return flag_names[id];
  }

  return user_flag_names[id - F_USER_FLAG_1];
}

/**************************************************************************
  Convert role names to enum; case insensitive;
  returns L_LAST if can't match.
**************************************************************************/
enum unit_role_id find_unit_role_by_rule_name(const char *s)
{
  enum unit_role_id i;

  assert(ARRAY_SIZE(role_names) == (L_LAST - L_FIRST));

  for(i=L_FIRST; i<L_LAST; i++) {
    if (mystrcasecmp(role_names[i-L_FIRST], s)==0) {
      return i;
    }
  }
  return L_LAST;
}

/**************************************************************************
  Return the (untranslated) rule name of the unit role.
**************************************************************************/
const char *unit_role_rule_name(enum unit_role_id id)
{
  assert(ARRAY_SIZE(role_names) == L_LAST);
  assert(id >= 0 && id < L_LAST);
  return role_names[id];
}

/**************************************************************************
Whether player can build given unit somewhere,
ignoring whether unit is obsolete and assuming the
player has a coastal city.
**************************************************************************/
bool can_player_build_unit_direct(const struct player *p,
				  const struct unit_type *punittype)
{
  CHECK_UNIT_TYPE(punittype);

  if (is_barbarian(p)
      && !utype_has_role(punittype, L_BARBARIAN_BUILD)
      && !utype_has_role(punittype, L_BARBARIAN_BUILD_TECH)) {
    /* Barbarians can build only role units */
    return FALSE;
  }

  if (utype_has_flag(punittype, F_NUCLEAR)
      && !get_player_bonus(p, EFT_ENABLE_NUKE) > 0) {
    return FALSE;
  }
  if (utype_has_flag(punittype, F_NOBUILD)) {
    return FALSE;
  }

  if (utype_has_flag(punittype, F_BARBARIAN_ONLY)
      && !is_barbarian(p)) {
    /* Unit can be built by barbarians only and this player is
     * not barbarian */
    return FALSE;
  }

  if (punittype->need_government
      && punittype->need_government != government_of_player(p)) {
    return FALSE;
  }
  if (player_invention_state(p, advance_number(punittype->require_advance)) != TECH_KNOWN) {
    if (!is_barbarian(p)) {
      /* Normal players can never build units without knowing tech
       * requirements. */
      return FALSE;
    }
    if (!utype_has_role(punittype, L_BARBARIAN_BUILD)) {
      /* Even barbarian cannot build this unit without tech */

      /* Unit has to have L_BARBARIAN_BUILD_TECH role
       * In the beginning of this function we checked that
       * barbarian player tries to build only role
       * L_BARBARIAN_BUILD or L_BARBARIAN_BUILD_TECH units. */
      assert(utype_has_role(punittype, L_BARBARIAN_BUILD_TECH));

      /* Client does not know all the advances other players have
       * got. So following gives wrong answer in the client.
       * This is called at the client when received create_city
       * packet for a barbarian city. City initialization tries
       * to find L_FIRSTBUILD unit. */

      if (!game.info.global_advances[advance_index(punittype->require_advance)]) {
        /* Nobody knows required tech */
        return FALSE;
      }
    }
    
  }
  if (utype_has_flag(punittype, F_UNIQUE)) {
    /* FIXME: This could be slow if we have lots of units. We could
     * consider keeping an array of unittypes updated with this info 
     * instead. */
    unit_list_iterate(p->units, punit) {
      if (unit_type(punit) == punittype) { 
        return FALSE;
      }
    } unit_list_iterate_end;
  }

  /* If the unit has a building requirement, we check to see if the player
   * can build that building.  Note that individual cities may not have
   * that building, so they still may not be able to build the unit. */
  if (punittype->need_improvement) {
    if (is_great_wonder(punittype->need_improvement)
        && (great_wonder_is_built(punittype->need_improvement)
            || great_wonder_is_destroyed(punittype->need_improvement))) {
      /* It's already built great wonder */
      if (great_wonder_owner(punittype->need_improvement) != p) {
        /* Not owned by this player. Either destroyed or owned by somebody
         * else. */
        return FALSE;
      }
    } else {
      if (!can_player_build_improvement_direct(p,
                                               punittype->need_improvement)) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**************************************************************************
Whether player can build given unit somewhere;
returns FALSE if unit is obsolete.
**************************************************************************/
bool can_player_build_unit_now(const struct player *p,
			       const struct unit_type *punittype)
{
  if (!can_player_build_unit_direct(p, punittype)) {
    return FALSE;
  }
  while ((punittype = punittype->obsoleted_by) != U_NOT_OBSOLETED) {
    if (can_player_build_unit_direct(p, punittype)) {
	return FALSE;
    }
  }
  return TRUE;
}

/**************************************************************************
Whether player can _eventually_ build given unit somewhere -- ie,
returns TRUE if unit is available with current tech OR will be available
with future tech. Returns FALSE if unit is obsolete.
**************************************************************************/
bool can_player_build_unit_later(const struct player *p,
				 const struct unit_type *punittype)
{
  CHECK_UNIT_TYPE(punittype);
  if (utype_has_flag(punittype, F_NOBUILD)) {
    return FALSE;
  }
  while ((punittype = punittype->obsoleted_by) != U_NOT_OBSOLETED) {
    if (can_player_build_unit_direct(p, punittype)) {
      return FALSE;
    }
  }
  return TRUE;
}

/**************************************************************************
The following functions use static variables so we can quickly look up
which unit types have given flag or role.
For these functions flags and roles are considered to be in the same "space",
and any "role" argument can also be a "flag".
Unit order is in terms of the order in the units ruleset.
**************************************************************************/
static bool first_init = TRUE;
static int n_with_role[L_LAST];
static struct unit_type **with_role[L_LAST];

/**************************************************************************
Do the real work for role_unit_precalcs, for one role (or flag), given by i.
**************************************************************************/
static void precalc_one(int i,
			bool (*func_has)(const struct unit_type *, int))
{
  int j;

  /* Count: */
  unit_type_iterate(u) {
    if (func_has(u, i)) {
      n_with_role[i]++;
    }
  } unit_type_iterate_end;

  if(n_with_role[i] > 0) {
    with_role[i] = fc_malloc(n_with_role[i] * sizeof(*with_role[i]));
    j = 0;
    unit_type_iterate(u) {
      if (func_has(u, i)) {
	with_role[i][j++] = u;
      }
    } unit_type_iterate_end;
    assert(j==n_with_role[i]);
  }
}

/**************************************************************************
Initialize; it is safe to call this multiple times (eg, if units have
changed due to rulesets in client).
**************************************************************************/
void role_unit_precalcs(void)
{
  int i;
  
  if(!first_init) {
    for(i=0; i<L_LAST; i++) {
      free(with_role[i]);
    }
  }
  for(i=0; i<L_LAST; i++) {
    with_role[i] = NULL;
    n_with_role[i] = 0;
  }

  for(i=0; i<F_LAST; i++) {
    precalc_one(i, utype_has_flag);
  }
  for(i=L_FIRST; i<L_LAST; i++) {
    precalc_one(i, utype_has_role);
  }
  first_init = FALSE;
}

/**************************************************************************
How many unit types have specified role/flag.
**************************************************************************/
int num_role_units(int role)
{
  assert((role>=0 && role<F_LAST) || (role>=L_FIRST && role<L_LAST));
  return n_with_role[role];
}

/**************************************************************************
Return index-th unit with specified role/flag.
Index -1 means (n-1), ie last one.
**************************************************************************/
struct unit_type *get_role_unit(int role, int index)
{
  assert((role>=0 && role<F_LAST) || (role>=L_FIRST && role<L_LAST));
  if (index==-1) index = n_with_role[role]-1;
  assert(index>=0 && index<n_with_role[role]);
  return with_role[role][index];
}

/**************************************************************************
  Return "best" unit this city can build, with given role/flag.
  Returns NULL if none match. "Best" means highest unit type id.
**************************************************************************/
struct unit_type *best_role_unit(const struct city *pcity, int role)
{
  struct unit_type *u;
  int j;
  
  assert((role>=0 && role<F_LAST) || (role>=L_FIRST && role<L_LAST));

  for(j=n_with_role[role]-1; j>=0; j--) {
    u = with_role[role][j];
    if (can_city_build_unit_now(pcity, u)) {
      return u;
    }
  }
  return NULL;
}

/**************************************************************************
  Return "best" unit the player can build, with given role/flag.
  Returns NULL if none match. "Best" means highest unit type id.

  TODO: Cache the result per player?
**************************************************************************/
struct unit_type *best_role_unit_for_player(const struct player *pplayer,
				       int role)
{
  int j;

  assert((role >= 0 && role < F_LAST) || (role >= L_FIRST && role < L_LAST));

  for(j = n_with_role[role]-1; j >= 0; j--) {
    struct unit_type *utype = with_role[role][j];

    if (can_player_build_unit_now(pplayer, utype)) {
      return utype;
    }
  }

  return NULL;
}

/**************************************************************************
  Return first unit the player can build, with given role/flag.
  Returns NULL if none match.  Used eg when placing starting units.
**************************************************************************/
struct unit_type *first_role_unit_for_player(const struct player *pplayer,
					int role)
{
  int j;

  assert((role >= 0 && role < F_LAST) || (role >= L_FIRST && role < L_LAST));

  for(j = 0; j < n_with_role[role]; j++) {
    struct unit_type *utype = with_role[role][j];

    if (can_player_build_unit_now(pplayer, utype)) {
      return utype;
    }
  }

  return NULL;
}

/****************************************************************************
  Inialize unit-type structures.
****************************************************************************/
void unit_types_init(void)
{
  int i;

  /* Can't use unit_type_iterate or utype_by_number here because
   * num_unit_types isn't known yet. */
  for (i = 0; i < ARRAY_SIZE(unit_types); i++) {
    unit_types[i].item_number = i;
  }
}

/**************************************************************************
  Frees the memory associated with this unit type.
**************************************************************************/
static void unit_type_free(struct unit_type *punittype)
{
  free(punittype->helptext);
  punittype->helptext = NULL;
}

/***************************************************************
 Frees the memory associated with all unit types.
***************************************************************/
void unit_types_free(void)
{
  unit_type_iterate(punittype) {
    unit_type_free(punittype);
  } unit_type_iterate_end;
}

/***************************************************************
  Frees the memory associated with all unit type flags
***************************************************************/
void unit_flags_free(void)
{
  int i;

  for (i = 0; i < MAX_NUM_USER_UNIT_FLAGS; i++) {
    if (user_flag_names[i] != 0) {
      free(user_flag_names[i]);
      user_flag_names[i] = NULL;
    }
  }
}

/**************************************************************************
  Return the first item of unit_classes.
**************************************************************************/
struct unit_class *unit_class_array_first(void)
{
  if (game.control.num_unit_classes > 0) {
    return unit_classes;
  }
  return NULL;
}

/**************************************************************************
  Return the last item of unit_classes.
**************************************************************************/
const struct unit_class *unit_class_array_last(void)
{
  if (game.control.num_unit_classes > 0) {
    return &unit_classes[game.control.num_unit_classes - 1];
  }
  return NULL;
}

/**************************************************************************
  Return the unit_class count.
**************************************************************************/
Unit_Class_id uclass_count(void)
{
  return game.control.num_unit_classes;
}

/**************************************************************************
  Return the unit_class index.

  Currently same as uclass_number(), paired with uclass_count()
  indicates use as an array index.
**************************************************************************/
Unit_Class_id uclass_index(const struct unit_class *pclass)
{
  assert(pclass);
  return pclass - unit_classes;
}

/**************************************************************************
  Return the unit_class index.
**************************************************************************/
Unit_Class_id uclass_number(const struct unit_class *pclass)
{
  assert(pclass);
  return pclass->item_number;
}

/****************************************************************************
  Returns unit class pointer for an ID value.
****************************************************************************/
struct unit_class *uclass_by_number(const Unit_Class_id id)
{
  if (id < 0 || id >= game.control.num_unit_classes) {
    return NULL;
  }
  return &unit_classes[id];
}

/***************************************************************
 Returns unit class pointer for a unit type.
***************************************************************/
struct unit_class *utype_class(const struct unit_type *punittype)
{
  assert(punittype->uclass);
  return punittype->uclass;
}

/***************************************************************
 Returns unit class pointer for a unit.
***************************************************************/
struct unit_class *unit_class(const struct unit *punit)
{
  return utype_class(unit_type(punit));
}

/****************************************************************************
  Initialize unit_class structures.
****************************************************************************/
void unit_classes_init(void)
{
  int i;

  /* Can't use unit_class_iterate or uclass_by_number here because
   * num_unit_classes isn't known yet. */
  for (i = 0; i < ARRAY_SIZE(unit_classes); i++) {
    unit_classes[i].item_number = i;
  }
}
