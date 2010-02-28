/********************************************************************** 
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
#include <config.h>
#endif

#include "game.h"
#include "log.h"
#include "unit.h"

#include "ailog.h"
#include "aitools.h"

#include "aiguard.h"

enum bodyguard_enum {
  BODYGUARD_WANTED = -1,
  BODYGUARD_NONE
};

/**************************************************************************
  Do sanity checks on a guard, reporting error messages to the log
  if necessary.

  Inconsistent references do not always indicate an error, because units
  can change owners (for example, because of civil war) outside the control
  the the AI code.
**************************************************************************/
void aiguard_check_guard(const struct unit *guard)
{
  const struct unit *charge_unit = game_find_unit_by_number(guard->ai.charge);
  const struct city *charge_city = game_find_city_by_number(guard->ai.charge);
  const struct player *guard_owner = unit_owner(guard);
  const struct player *charge_owner = NULL;

  assert(BODYGUARD_NONE <= guard->ai.charge);
  assert(charge_unit == NULL || charge_city == NULL); /* IDs always distinct */

  if (charge_unit) {
    charge_owner = unit_owner(charge_unit);
  } else if (charge_city) {
    charge_owner = city_owner(charge_city);
  }

  if (charge_unit && charge_unit->ai.bodyguard != guard->id) {
    BODYGUARD_LOG(LOG_DEBUG, guard, "inconsistent guard references");
  } else if (!charge_unit && !charge_city && 0 < guard->ai.charge) {
    BODYGUARD_LOG(LOG_DEBUG, guard, "dangling guard reference");
  }
  if (charge_owner && pplayers_at_war(charge_owner, guard_owner)) {
    /* Probably due to civil war */
    BODYGUARD_LOG(LOG_DEBUG, guard, "enemy charge");
  } else if (charge_owner && charge_owner != guard_owner) {
    /* Probably sold a city with its supported units. */
    BODYGUARD_LOG(LOG_DEBUG, guard, "foreign charge");
  }
}

/**************************************************************************
  Do sanity checks on a charge, reporting error messages to the log
  if necessary.

  Inconsistent references do not always indicate an error, because units
  can change owners (for example, because of civil war) outside the control
  the the AI code.
**************************************************************************/
void aiguard_check_charge_unit(const struct unit *charge)
{
  const struct player *charge_owner = unit_owner(charge);
  struct unit *guard = game_find_unit_by_number(charge->ai.bodyguard);
  assert(guard == NULL || BODYGUARD_WANTED <= guard->ai.bodyguard);
 
 if (guard && guard->ai.charge != charge->id) {
    /* FIXME: UNIT_LOG should take a const struct * */
    UNIT_LOG(LOG_DEBUG, charge,
             "inconsistent guard references");
  } else if (guard && unit_owner(guard) != charge_owner) {
    /* FIXME: UNIT_LOG should take a const struct * */
    UNIT_LOG(LOG_DEBUG, charge, "foreign guard");
  }
}

/**************************************************************************
  Remove the assignment of a charge to a guard.

  Assumes that a unit can have at most one guard.
**************************************************************************/
void aiguard_clear_charge(struct unit *guard)
{
  struct unit *charge_unit = game_find_unit_by_number(guard->ai.charge);
  struct city *charge_city = game_find_city_by_number(guard->ai.charge);

  assert(charge_unit == NULL || charge_city == NULL); /* IDs always distinct */

  if (charge_unit) {
    BODYGUARD_LOG(LOGLEVEL_BODYGUARD, guard, "unassigned (unit)");
    charge_unit->ai.bodyguard = BODYGUARD_NONE;
  } else if (charge_city) {
    BODYGUARD_LOG(LOGLEVEL_BODYGUARD, guard, "unassigned (city)");
  }
  /* else not assigned or charge was destroyed */
  guard->ai.charge = BODYGUARD_NONE;

  CHECK_GUARD(guard);
}

/**************************************************************************
  Remove assignment of bodyguard for a unit.

  Assumes that a unit can have at most one guard.

  There is no analogous function for cities, because cities can have many
  guards: instead use aiguard_clear_charge for each city guard.
**************************************************************************/
void aiguard_clear_guard(struct unit *charge)
{
  if (0 < charge->ai.bodyguard) {
    struct unit *guard = game_find_unit_by_number(charge->ai.bodyguard);

    if (guard && guard->ai.charge == charge->id) {
      /* charge doesn't want us anymore */
      guard->ai.charge = BODYGUARD_NONE;
    }
  }

  charge->ai.bodyguard = BODYGUARD_NONE;

  CHECK_CHARGE_UNIT(charge);
}

/**************************************************************************
  Assign a bodyguard to a unit.

  Assumes that a unit can have at most one guard.
**************************************************************************/
void aiguard_assign_guard_unit(struct unit *charge, struct unit *guard)
{
  assert(charge != NULL);
  assert(guard != NULL);
  assert(guard != charge);
  assert(unit_owner(charge) == unit_owner(guard));

  /* Remove previous assignment: */
  aiguard_clear_charge(guard);
  aiguard_clear_guard(charge);

  guard->ai.charge = charge->id;
  charge->ai.bodyguard = guard->id;

  BODYGUARD_LOG(LOGLEVEL_BODYGUARD, guard, "assigned charge");
  CHECK_GUARD(guard);
  CHECK_CHARGE_UNIT(charge);
}

/**************************************************************************
  Assign a guard to a city.
**************************************************************************/
void aiguard_assign_guard_city(struct city *charge, struct unit *guard)
{
  assert(charge != NULL);
  assert(guard != NULL);
  /*
   * Usually, but not always, city_owner(charge) == unit_owner(guard).
   */

  if (0 < guard->ai.charge && guard->ai.charge != charge->id) {
    /* Remove previous assignment: */
    aiguard_clear_charge(guard);
  }

  guard->ai.charge = charge->id;
  if (city_owner(charge) != unit_owner(guard)) {
    /* Peculiar, but not always an error */
    BODYGUARD_LOG(LOGLEVEL_BODYGUARD, guard, "assigned foreign charge");
  } else {
    BODYGUARD_LOG(LOGLEVEL_BODYGUARD, guard, "assigned charge");
  }

  CHECK_GUARD(guard);
}

/**************************************************************************
  Request a (new) bodyguard for the unit.
**************************************************************************/
void aiguard_request_guard(struct unit *punit)
{
  /* Remove previous assignment */
  aiguard_clear_guard(punit);

  UNIT_LOG(LOGLEVEL_BODYGUARD, punit, "requests a guard");
  punit->ai.bodyguard = BODYGUARD_WANTED;

  CHECK_CHARGE_UNIT(punit);
}

/**************************************************************************
  Has a unit requested a guard and not (yet) been provided with one?
**************************************************************************/
bool aiguard_wanted(struct unit *charge)
{
  CHECK_CHARGE_UNIT(charge);
  return (charge->ai.bodyguard == BODYGUARD_WANTED);
}

/**************************************************************************
  Has a charge unit been assigned to a guard?
**************************************************************************/
bool aiguard_has_charge(struct unit *guard)
{
  CHECK_GUARD(guard);
  return (guard->ai.charge != BODYGUARD_NONE);
}

/**************************************************************************
  Has a guard been assigned to a charge?
**************************************************************************/
bool aiguard_has_guard(struct unit *charge)
{
  CHECK_CHARGE_UNIT(charge);
  return (0 < charge->ai.bodyguard);
}

/**************************************************************************
  Which unit, if any, is the body guard of a unit?
  Returns NULL if the unit has not been assigned a guard.
**************************************************************************/
struct unit *aiguard_guard_of(struct unit *charge)
{
  CHECK_CHARGE_UNIT(charge);
  return game_find_unit_by_number(charge->ai.bodyguard);
}

/**************************************************************************
  Which unit (if any) has a guard been assigned to?
  Returns NULL if the unit is not the guard for a unit.
**************************************************************************/
struct unit *aiguard_charge_unit(struct unit *guard)
{
  CHECK_GUARD(guard);
  return game_find_unit_by_number(guard->ai.charge);
}

/**************************************************************************
  Which city (if any) has a guard been assigned to?
  Returns NULL if the unit is not a guard for a city.
**************************************************************************/
struct city *aiguard_charge_city(struct unit *guard)
{
  CHECK_GUARD(guard);
  return game_find_city_by_number(guard->ai.charge);
}

/**************************************************************************
  Check whether the assignment of a guard is still sane, and fix and problems.
  It was once sane, but might have been destroyed or become an enemy since.
**************************************************************************/
void aiguard_update_charge(struct unit *guard)
{
  const struct unit *charge_unit = game_find_unit_by_number(guard->ai.charge);
  const struct city *charge_city = game_find_city_by_number(guard->ai.charge);
  const struct player *guard_owner = unit_owner(guard);
  const struct player *charge_owner = NULL;

  assert(BODYGUARD_NONE <= guard->ai.charge);
  assert(charge_unit == NULL || charge_city == NULL); /* IDs always distinct */

  if (charge_unit) {
    charge_owner = unit_owner(charge_unit);
  } else if (charge_city) {
    charge_owner = city_owner(charge_city);
  }

  if (!charge_unit && !charge_city && 0 < guard->ai.charge) {
    guard->ai.charge = BODYGUARD_NONE;
    BODYGUARD_LOG(LOGLEVEL_BODYGUARD, guard, "charge was destroyed");
  }
  if (charge_owner && charge_owner != guard_owner) {
    BODYGUARD_LOG(LOGLEVEL_BODYGUARD, guard, "charge transferred, dismiss");
    aiguard_clear_charge(guard);
  }

  CHECK_GUARD(guard);
}
