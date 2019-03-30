/***********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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
#include "log.h"

/* common */
#include "game.h"
#include "unit.h"

/* server */
#include "srv_log.h"

/* ai */
#include "ailog.h"
#include "aiplayer.h"
#include "aitools.h"
#include "aiunit.h"

#include "aiguard.h"

enum bodyguard_enum {
  BODYGUARD_WANTED = -1,
  BODYGUARD_NONE
};

/**********************************************************************//**
  Do sanity checks on a guard, reporting error messages to the log
  if necessary.

  Inconsistent references do not always indicate an error, because units
  can change owners (for example, because of civil war) outside the control
  of the AI code.
**************************************************************************/
void aiguard_check_guard(struct ai_type *ait, const struct unit *guard)
{
  struct unit_ai *guard_data = def_ai_unit_data(guard, ait);
  const struct unit *charge_unit = game_unit_by_number(guard_data->charge);
  const struct city *charge_city = game_city_by_number(guard_data->charge);
  const struct player *guard_owner = unit_owner(guard);
  const struct player *charge_owner = NULL;
  struct unit_ai *charge_data = NULL;

  fc_assert_ret(BODYGUARD_NONE <= guard_data->charge);
  /* IDs always distinct */
  fc_assert_ret(charge_unit == NULL || charge_city == NULL);

  if (charge_unit) {
    charge_owner = unit_owner(charge_unit);
    charge_data = def_ai_unit_data(charge_unit, ait);
  } else if (charge_city) {
    charge_owner = city_owner(charge_city);
  }

  if (charge_unit && charge_data->bodyguard != guard->id) {
    BODYGUARD_LOG(ait, LOG_DEBUG, guard, "inconsistent guard references");
  } else if (!charge_unit && !charge_city && 0 < guard_data->charge) {
    BODYGUARD_LOG(ait, LOG_DEBUG, guard, "dangling guard reference");
  }
  if (charge_owner && pplayers_at_war(charge_owner, guard_owner)) {
    /* Probably due to civil war */
    BODYGUARD_LOG(ait, LOG_DEBUG, guard, "enemy charge");
  } else if (charge_owner && charge_owner != guard_owner) {
    /* Probably sold a city with its supported units. */
    BODYGUARD_LOG(ait, LOG_DEBUG, guard, "foreign charge");
  }
}

/**********************************************************************//**
  Do sanity checks on a charge, reporting error messages to the log
  if necessary.

  Inconsistent references do not always indicate an error, because units
  can change owners (for example, because of civil war) outside the control
  of the AI code.
**************************************************************************/
void aiguard_check_charge_unit(struct ai_type *ait, const struct unit *charge)
{
  struct unit_ai *charge_data = def_ai_unit_data(charge, ait);
  const struct player *charge_owner = unit_owner(charge);
  const struct unit *guard = game_unit_by_number(charge_data->bodyguard);
  struct unit_ai *guard_data = NULL;

  if (guard) {
    guard_data = def_ai_unit_data(guard, ait);
  }

  fc_assert_ret(guard == NULL
                || (guard_data && BODYGUARD_WANTED <= guard_data->bodyguard));

  if (guard && guard_data->charge != charge->id) {
    UNIT_LOG(LOG_DEBUG, charge,
             "inconsistent guard references");
  } else if (guard && unit_owner(guard) != charge_owner) {
    UNIT_LOG(LOG_DEBUG, charge, "foreign guard");
  }
}

/**********************************************************************//**
  Remove the assignment of a charge to a guard.

  Assumes that a unit can have at most one guard.
**************************************************************************/
void aiguard_clear_charge(struct ai_type *ait, struct unit *guard)
{
  struct unit_ai *guard_data = def_ai_unit_data(guard, ait);
  struct unit *charge_unit = game_unit_by_number(guard_data->charge);
  struct city *charge_city = game_city_by_number(guard_data->charge);

  /* IDs always distinct */
  fc_assert_ret(charge_unit == NULL || charge_city == NULL);

  if (charge_unit) {
    BODYGUARD_LOG(ait, LOGLEVEL_BODYGUARD, guard, "unassigned (unit)");
    def_ai_unit_data(charge_unit, ait)->bodyguard = BODYGUARD_NONE;
  } else if (charge_city) {
    BODYGUARD_LOG(ait, LOGLEVEL_BODYGUARD, guard, "unassigned (city)");
  }
  /* else not assigned or charge was destroyed */
  guard_data->charge = BODYGUARD_NONE;

  CHECK_GUARD(ait, guard);
}

/**********************************************************************//**
  Remove assignment of bodyguard for a unit.

  Assumes that a unit can have at most one guard.

  There is no analogous function for cities, because cities can have many
  guards: instead use aiguard_clear_charge for each city guard.
**************************************************************************/
void aiguard_clear_guard(struct ai_type *ait, struct unit *charge)
{
  struct unit_ai *charge_data = def_ai_unit_data(charge, ait);

  if (0 < charge_data->bodyguard) {
    struct unit *guard = game_unit_by_number(charge_data->bodyguard);

    if (guard) {
      struct unit_ai *guard_data = def_ai_unit_data(guard, ait);

      if (guard_data->charge == charge->id) {
        /* charge doesn't want us anymore */
        guard_data->charge = BODYGUARD_NONE;
      }
    }
  }

  charge_data->bodyguard = BODYGUARD_NONE;

  CHECK_CHARGE_UNIT(ait, charge);
}

/**********************************************************************//**
  Assign a bodyguard to a unit.

  Assumes that a unit can have at most one guard.
**************************************************************************/
void aiguard_assign_guard_unit(struct ai_type *ait, struct unit *charge,
                               struct unit *guard)
{
  fc_assert_ret(NULL != charge);
  fc_assert_ret(NULL != guard);
  fc_assert_ret(charge != guard);
  fc_assert_ret(unit_owner(charge) == unit_owner(guard));

  /* Remove previous assignment: */
  aiguard_clear_charge(ait, guard);
  aiguard_clear_guard(ait, charge);

  def_ai_unit_data(guard, ait)->charge = charge->id;
  def_ai_unit_data(charge, ait)->bodyguard = guard->id;

  BODYGUARD_LOG(ait, LOGLEVEL_BODYGUARD, guard, "assigned charge");
  CHECK_GUARD(ait, guard);
  CHECK_CHARGE_UNIT(ait, charge);
}

/**********************************************************************//**
  Assign a guard to a city.
**************************************************************************/
void aiguard_assign_guard_city(struct ai_type *ait, struct city *charge,
                               struct unit *guard)
{
  struct unit_ai *guard_data = def_ai_unit_data(guard, ait);

  fc_assert_ret(charge != NULL);
  fc_assert_ret(guard != NULL);
  /*
   * Usually, but not always, city_owner(charge) == unit_owner(guard).
   */

  if (0 < guard_data->charge
      && guard_data->charge != charge->id) {
    /* Remove previous assignment: */
    aiguard_clear_charge(ait, guard);
  }

  guard_data->charge = charge->id;
  if (city_owner(charge) != unit_owner(guard)) {
    /* Peculiar, but not always an error */
    BODYGUARD_LOG(ait, LOGLEVEL_BODYGUARD, guard, "assigned foreign charge");
  } else {
    BODYGUARD_LOG(ait, LOGLEVEL_BODYGUARD, guard, "assigned charge");
  }

  CHECK_GUARD(ait, guard);
}

/**********************************************************************//**
  Request a (new) bodyguard for the unit.
**************************************************************************/
void aiguard_request_guard(struct ai_type *ait, struct unit *punit)
{
  /* Remove previous assignment */
  aiguard_clear_guard(ait, punit);

  UNIT_LOG(LOGLEVEL_BODYGUARD, punit, "requests a guard");
  def_ai_unit_data(punit, ait)->bodyguard = BODYGUARD_WANTED;

  CHECK_CHARGE_UNIT(ait, punit);
}

/**********************************************************************//**
  Has a unit requested a guard and not (yet) been provided with one?
**************************************************************************/
bool aiguard_wanted(struct ai_type *ait, struct unit *charge)
{
  CHECK_CHARGE_UNIT(ait, charge);
  return (def_ai_unit_data(charge, ait)->bodyguard == BODYGUARD_WANTED);
}

/**********************************************************************//**
  Has a charge unit been assigned to a guard?
**************************************************************************/
bool aiguard_has_charge(struct ai_type *ait, struct unit *guard)
{
  CHECK_GUARD(ait, guard);
  return (def_ai_unit_data(guard, ait)->charge != BODYGUARD_NONE);
}

/**********************************************************************//**
  Has a guard been assigned to a charge?
**************************************************************************/
bool aiguard_has_guard(struct ai_type *ait, struct unit *charge)
{
  CHECK_CHARGE_UNIT(ait, charge);
  return (0 < def_ai_unit_data(charge, ait)->bodyguard);
}

/**********************************************************************//**
  Which unit, if any, is the body guard of a unit?
  Returns NULL if the unit has not been assigned a guard.
**************************************************************************/
struct unit *aiguard_guard_of(struct ai_type *ait, struct unit *charge)
{
  CHECK_CHARGE_UNIT(ait, charge);
  return game_unit_by_number(def_ai_unit_data(charge, ait)->bodyguard);
}

/**********************************************************************//**
  Which unit (if any) has a guard been assigned to?
  Returns NULL if the unit is not the guard for a unit.
**************************************************************************/
struct unit *aiguard_charge_unit(struct ai_type *ait, struct unit *guard)
{
  CHECK_GUARD(ait, guard);
  return game_unit_by_number(def_ai_unit_data(guard, ait)->charge);
}

/**********************************************************************//**
  Which city (if any) has a guard been assigned to?
  Returns NULL if the unit is not a guard for a city.
**************************************************************************/
struct city *aiguard_charge_city(struct ai_type *ait, struct unit *guard)
{
  CHECK_GUARD(ait, guard);
  return game_city_by_number(def_ai_unit_data(guard, ait)->charge);
}

/**********************************************************************//**
  Check whether the assignment of a guard is still sane, and fix and problems.
  It was once sane, but might have been destroyed or become an enemy since.
**************************************************************************/
void aiguard_update_charge(struct ai_type *ait, struct unit *guard)
{
  struct unit_ai *guard_data = def_ai_unit_data(guard, ait);
  const struct unit *charge_unit = game_unit_by_number(guard_data->charge);
  const struct city *charge_city = game_city_by_number(guard_data->charge);
  const struct player *guard_owner = unit_owner(guard);
  const struct player *charge_owner = NULL;

  fc_assert_ret(BODYGUARD_NONE <= guard_data->charge);
  /* IDs always distinct */
  fc_assert_ret(charge_unit == NULL || charge_city == NULL);

  if (charge_unit) {
    charge_owner = unit_owner(charge_unit);
  } else if (charge_city) {
    charge_owner = city_owner(charge_city);
  }

  if (!charge_unit && !charge_city && 0 < guard_data->charge) {
    guard_data->charge = BODYGUARD_NONE;
    BODYGUARD_LOG(ait, LOGLEVEL_BODYGUARD, guard, "charge was destroyed");
  }
  if (charge_owner && charge_owner != guard_owner) {
    BODYGUARD_LOG(ait, LOGLEVEL_BODYGUARD, guard, "charge transferred, dismiss");
    aiguard_clear_charge(ait, guard);
  }

  CHECK_GUARD(ait, guard);
}
