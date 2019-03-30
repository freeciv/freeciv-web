/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
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
#include "log.h"
#include "rand.h"

/* common */
#include "citizens.h"
#include "city.h"
#include "fc_types.h"
#include "game.h"
#include "player.h"

/* server */
#include "cityturn.h"
#include "sanitycheck.h"


#include "citizenshand.h"

#define log_citizens log_debug

/*************************************************************************//**
  Update the nationality according to the city size. New citiens are added
  using the nationality of the owner. If the city size is reduced, the
  citizens are removed first from the foreign citizens.
*****************************************************************************/
#define log_citizens_add(_pcity, _delta, _pplayer)                           \
  log_citizens("%s (size %d; %s): %+d citizen(s) for %s (now: %d)",          \
               city_name_get(_pcity), city_size_get(_pcity),                 \
               player_name(city_owner(_pcity)), _delta,                      \
               player_name(_pplayer),                                        \
               citizens_nation_get(_pcity, _pplayer->slot));
void citizens_update(struct city *pcity, struct player *plr)
{
  int delta;

  fc_assert_ret(pcity);

  if (pcity->server.debug) {
    /* before */
    citizens_print(pcity);
  }

  if (game.info.citizen_nationality != TRUE) {
    return;
  }

  if (pcity->nationality == NULL) {
    /* If nationalities are not set (virtual cities) do nothing. */
    return;
  }

  delta = city_size_get(pcity) - citizens_count(pcity);

  if (delta == 0) {
    /* No change of the city size */
    return;
  }

  if (delta > 0) {
    if (plr != NULL) {
      citizens_nation_add(pcity, plr->slot, delta);
      log_citizens_add(pcity, delta, plr);
    } else {
      /* Add new citizens with the nationality of the current owner. */
      citizens_nation_add(pcity, city_owner(pcity)->slot, delta);
      log_citizens_add(pcity, delta, city_owner(pcity));
    }
  } else {
    /* Removed citizens. */
    struct player_slot *city_nations[MAX_NUM_PLAYER_SLOTS];
    int count = 0;

    /* Create a list of foreign nationalities. */
    citizens_foreign_iterate(pcity, pslot, nationality) {
      city_nations[count] = pslot;
      count++;
    } citizens_foreign_iterate_end;

    /* First remove from foreign nationalities. */
    while (count > 0 && delta < 0) {
      int selected = fc_rand(count);
      struct player_slot *pslot = city_nations[selected];
      struct player *pplayer = player_slot_get_player(pslot);
      citizens nationality = citizens_nation_get(pcity, pslot);

      fc_assert_ret(nationality != 0);
      fc_assert_ret(pplayer != NULL);

      if (nationality == 1) {
        /* Remove one citizen. */
        delta++;
        citizens_nation_set(pcity, pslot, 0);
        /* Remove this nation from the list of nationalities. */
        if (selected != count) {
          city_nations[selected] = city_nations[count - 1];
        }
        count--;

        log_citizens_add(pcity, -1, pplayer);
      } else {
        /* Get the minimal reduction = the maximum value of two negative
         * numbers. */
        int diff = MAX(delta, - nationality / 2);
        delta -= diff;
        citizens_nation_add(pcity, pslot, diff);
        log_citizens_add(pcity, diff, pplayer);
      }
    }

    if (delta < 0) {
      /* Now take the remaining citizens loss from the nation of the owner. */
      citizens_nation_add(pcity, city_owner(pcity)->slot, delta);
      log_citizens_add(pcity, delta, city_owner(pcity));
    }
  }

  fc_assert_ret(city_size_get(pcity) == citizens_count(pcity));

  if (pcity->server.debug) {
    /* after */
    citizens_print(pcity);
  }
}
#undef log_citizens_add

/*************************************************************************//**
  Print the data about the citizens.
*****************************************************************************/
void citizens_print(const struct city *pcity)
{
  fc_assert_ret(pcity);

  if (game.info.citizen_nationality != TRUE) {
    return;
  }

  log_citizens("%s (size %d; %s): %d citizen(s)",
               city_name_get(pcity), city_size_get(pcity),
               player_name(city_owner(pcity)), citizens_count(pcity));

  citizens_iterate(pcity, pslot, nationality) {
    struct player *pplayer = player_slot_get_player(pslot);

    fc_assert_ret(pplayer != NULL);

    log_citizens("%s (size %d; %s): %d citizen(s) for %s",
                 city_name_get(pcity), city_size_get(pcity),
                 player_name(city_owner(pcity)), nationality,
                 player_name(pplayer));
  } citizens_iterate_end;
}

/*************************************************************************//**
  Return whether citizen should be converted this turn.
*****************************************************************************/
static bool citizen_convert_check(struct city *pcity)
{
  if (fc_rand(1000) + 1 > game.info.citizen_convert_speed) {
    return FALSE;
  }

  return TRUE;
}

/*************************************************************************//**
  Convert one (random) foreign citizen to the nationality of the owner.
*****************************************************************************/
void citizens_convert(struct city *pcity)
{
  struct player_slot *city_nations[MAX_NUM_PLAYER_SLOTS], *pslot;
  struct player *pplayer;
  int count = 0;

  fc_assert_ret(pcity);

  if (!game.info.citizen_nationality) {
    return;
  }

  if (!citizen_convert_check(pcity)) {
    return;
  }

  if (citizens_nation_foreign(pcity) == 0) {
    /* Only our own citizens. */
    return;
  }

  /* Create a list of foreign nationalities. */
  citizens_foreign_iterate(pcity, foreign_slot, nationality) {
    if (nationality != 0) {
      city_nations[count++] = foreign_slot;
    }
  } citizens_foreign_iterate_end;

  /* Now convert one citizens to the city owners nationality. */
  pslot = city_nations[fc_rand(count)];
  pplayer = player_slot_get_player(pslot);

  fc_assert_ret(pplayer != NULL);

  log_citizens("%s (size %d; %s): convert 1 citizen from %s",
               city_name_get(pcity), city_size_get(pcity),
               player_name(city_owner(pcity)), player_name(pplayer));
  citizens_nation_move(pcity, pslot, city_owner(pcity)->slot, 1);
}

/*************************************************************************//**
  Convert citizens to the nationality of the one conquering the city.
*****************************************************************************/
void citizens_convert_conquest(struct city *pcity)
{
  struct player_slot *conqueror;

  if (!game.info.citizen_nationality || game.info.conquest_convert_pct == 0) {
    return;
  }

  conqueror = city_owner(pcity)->slot;

  citizens_foreign_iterate(pcity, pslot, nat) {
    /* Convert 'game.info.conquest_convert_pct' citizens of each foreign
     * nationality to the nation of the new owner (but at least 1). */
    citizens convert = MAX(1, nat * game.info.conquest_convert_pct
                           / 100);
    struct player *pplayer = player_slot_get_player(pslot);

    fc_assert_ret(pplayer != NULL);

    log_citizens("%s (size %d; %s): convert %d citizen from %s (conquered)",
                 city_name_get(pcity), city_size_get(pcity),
                 player_name(city_owner(pcity)), convert,
                 player_name(pplayer));
    citizens_nation_move(pcity, pslot, conqueror, convert);
  } citizens_foreign_iterate_end;
}
