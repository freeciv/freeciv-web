/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "game.h"
#include "victory.h"

#include "calendar.h"

/************************************************************************//**
  Advance the calendar in the passed game_info structure
  (may only be a copy of the real one).
  FIXME: would be nice to pass a struct containing just the
  calendar, not the whole game_info struct.
****************************************************************************/
void game_next_year(struct packet_game_info *info)
{
  int increase = get_world_bonus(EFT_TURN_YEARS);
  const int slowdown = (victory_enabled(VC_SPACERACE)
			? get_world_bonus(EFT_SLOW_DOWN_TIMELINE) : 0);
  int fragment_years;

  if (info->year_0_hack) {
    /* hacked it to get rid of year 0 */
    info->year = 0;
    info->year_0_hack = FALSE;
  }

    /* !McFred: 
       - want year += 1 for spaceship.
    */

  /* test game with 7 normal AI's, gen 4 map, foodbox 10, foodbase 0: 
   * Gunpowder about 0 AD
   * Railroad  about 500 AD
   * Electricity about 1000 AD
   * Refining about 1500 AD (212 active units)
   * about 1750 AD
   * about 1900 AD
   */

  /* Note the slowdown operates even if Enable_Space is not active.  See
   * README.effects for specifics. */
  if (slowdown >= 3) {
    if (increase > 1) {
      increase = 1;
    }
  } else if (slowdown >= 2) {
    if (increase > 2) {
      increase = 2;
    }
  } else if (slowdown >= 1) {
    if (increase > 5) {
      increase = 5;
    }
  }

  if (game.calendar.calendar_fragments) {
    info->fragment_count += get_world_bonus(EFT_TURN_FRAGMENTS);
    fragment_years = info->fragment_count / game.calendar.calendar_fragments;

    increase += fragment_years;
    info->fragment_count -= fragment_years * game.calendar.calendar_fragments;
  }

  info->year += increase;

  if (info->year == 0 && game.calendar.calendar_skip_0) {
    info->year = 1;
    info->year_0_hack = TRUE;
  }
}

/************************************************************************//**
  Advance the game year.
****************************************************************************/
void game_advance_year(void)
{
  game_next_year(&game.info);
  game.info.turn++;
}

/************************************************************************//**
  Produce a statically allocated textual representation of the given
  calendar fragment.
****************************************************************************/
const char *textcalfrag(int frag)
{
  static char buf[MAX_LEN_NAME];

  fc_assert_ret_val(game.calendar.calendar_fragments > 0, "");
  if (game.calendar.calendar_fragment_name[frag][0] != '\0') {
    fc_snprintf(buf, sizeof(buf), "%s",
                _(game.calendar.calendar_fragment_name[frag]));
  } else {
    /* Human readable fragment count starts from 1, not 0 */
    fc_snprintf(buf, sizeof(buf), "%d", frag + 1);
  }
  return buf;
}

/************************************************************************//**
  Produce a statically allocated textual representation of the given
  year.
****************************************************************************/
const char *textyear(int year)
{
  static char y[32];

  if (year < 0) {
    /* TRANS: <year> <label> -> "1000 BC" */
    fc_snprintf(y, sizeof(y), _("%d %s"), -year,
                _(game.calendar.negative_year_label));
  } else {
    /* TRANS: <year> <label> -> "1000 AD" */
    fc_snprintf(y, sizeof(y), _("%d %s"), year,
                _(game.calendar.positive_year_label));
  }

  return y;
}

/************************************************************************//**
  Produce a statically allocated textual representation of the current
  calendar time.
****************************************************************************/
const char *calendar_text(void)
{
  if (game.calendar.calendar_fragments) {
    static char buffer[128];

    fc_snprintf(buffer, sizeof(buffer), "%s/%s", textyear(game.info.year),
                textcalfrag(game.info.fragment_count));
    return buffer;
  } else {
    return textyear(game.info.year);
  }
}
