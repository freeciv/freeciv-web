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

#include <stdarg.h>

/* utility */
#include "astring.h"
#include "log.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

/* common */
#include "city.h"
#include "game.h"
#include "unit.h"

/* server */
#include "gotohand.h"
#include "notify.h"
#include "srv_main.h"

/* ai */
#include "aidata.h"

#include "ailog.h"

static struct timer *aitimer[AIT_LAST][2];
static int recursion[AIT_LAST];

/* General AI logging functions */

/**************************************************************************
  Log player tech messages.
**************************************************************************/
void TECH_LOG(int level, const struct player *pplayer,
              struct advance *padvance, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  int minlevel = MIN(LOGLEVEL_TECH, level);

  if (!valid_advance(padvance) || advance_by_number(A_NONE) == padvance) {
    return;
  }

  if (BV_ISSET(pplayer->debug, PLAYER_DEBUG_TECH)) {
    minlevel = LOG_TEST;
  } else if (minlevel > fc_log_level) {
    return;
  }

  my_snprintf(buffer, sizeof(buffer), "%s::%s (want %d, dist %d) ", 
              player_name(pplayer),
              advance_name_by_player(pplayer, advance_number(padvance)), 
              pplayer->ai_data.tech_want[advance_index(padvance)], 
              num_unknown_techs_for_goal(pplayer, advance_number(padvance)));

  va_start(ap, msg);
  my_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (BV_ISSET(pplayer->debug, PLAYER_DEBUG_TECH)) {
    notify_conn(NULL, NULL, E_AI_DEBUG, FTC_LOG, NULL, "%s", buffer);
  }
  freelog(minlevel, "%s", buffer);
}

/**************************************************************************
  Log player messages, they will appear like this
    
  where ti is timer, co countdown and lo love for target, who is e.
**************************************************************************/
void DIPLO_LOG(int level, const struct player *pplayer,
	       const struct player *aplayer, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  int minlevel = MIN(LOGLEVEL_PLAYER, level);
  const struct ai_dip_intel *adip;

  if (BV_ISSET(pplayer->debug, PLAYER_DEBUG_DIPLOMACY)) {
    minlevel = LOG_TEST;
  } else if (minlevel > fc_log_level) {
    return;
  }

  /* Don't use ai_data_get since it can have side effects. */
  adip = ai_diplomacy_get(pplayer, aplayer);

  my_snprintf(buffer, sizeof(buffer), "%s->%s(l%d,c%d,d%d%s): ", 
              player_name(pplayer),
              player_name(aplayer), 
              pplayer->ai_data.love[player_index(aplayer)],
              adip->countdown, 
              adip->distance,
              adip->is_allied_with_enemy ? "?" :
              (adip->at_war_with_ally ? "!" : ""));

  va_start(ap, msg);
  my_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (BV_ISSET(pplayer->debug, PLAYER_DEBUG_DIPLOMACY)) {
    notify_conn(NULL, NULL, E_AI_DEBUG, FTC_LOG, NULL, "%s", buffer);
  }
  freelog(minlevel, "%s", buffer);
}

/**************************************************************************
  Log city messages, they will appear like this
    2: Polish Romenna(5,35) [s1 d106 u11 g1] must have Archers ...
**************************************************************************/
void CITY_LOG(int level, const struct city *pcity, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  int minlevel = MIN(LOGLEVEL_CITY, level);

  if (pcity->debug) {
    minlevel = LOG_TEST;
  } else if (minlevel > fc_log_level) {
    return;
  }

  my_snprintf(buffer, sizeof(buffer), "%s %s(%d,%d) [s%d d%d u%d g%d] ",
              nation_rule_name(nation_of_city(pcity)),
              city_name(pcity),
              TILE_XY(pcity->tile), pcity->size,
              pcity->ai->danger, pcity->ai->urgency,
              pcity->ai->grave_danger);

  va_start(ap, msg);
  my_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (pcity->debug) {
    notify_conn(NULL, NULL, E_AI_DEBUG, FTC_LOG, NULL, "%s", buffer);
  }
  freelog(minlevel, "%s", buffer);
}

/**************************************************************************
  Log unit messages, they will appear like this
    2: Polish Archers[139] (5,35)->(0,0){0,0} stays to defend city
  where [] is unit id, ()->() are coordinates present and goto, and
  {,} contains bodyguard and ferryboat ids.
**************************************************************************/
void UNIT_LOG(int level, const struct unit *punit, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  int minlevel = MIN(LOGLEVEL_UNIT, level);
  int gx, gy;
  bool messwin = FALSE; /* output to message window */

  if (punit->debug) {
    minlevel = LOG_TEST;
  } else {
    /* Are we a virtual unit evaluated in a debug city?. */
    if (punit->id == 0) {
      struct city *pcity = tile_city(punit->tile);

      if (pcity && pcity->debug) {
        minlevel = LOG_TEST;
        messwin = TRUE;
      }
    }
    if (minlevel > fc_log_level) {
      return;
    }
  }

  if (punit->goto_tile) {
    gx = punit->goto_tile->x;
    gy = punit->goto_tile->y;
  } else {
    gx = gy = -1;
  }
  
  my_snprintf(buffer, sizeof(buffer),
	      "%s %s[%d] %s (%d,%d)->(%d,%d){%d,%d} ",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit),
              punit->id,
	      get_activity_text(punit->activity),
	      TILE_XY(punit->tile),
	      gx, gy,
              punit->ai.bodyguard, punit->ai.ferryboat);

  va_start(ap, msg);
  my_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (punit->debug || messwin) {
    notify_conn(NULL, NULL, E_AI_DEBUG, FTC_LOG, NULL, "%s", buffer);
  }
  freelog(minlevel, "%s", buffer);
}

/**************************************************************************
  Log message for bodyguards. They will appear like this
    2: Polish Mech. Inf.[485] bodyguard (38,22){Riflemen:574@37,23} was ...
  note that these messages are likely to wrap if long.
**************************************************************************/
void BODYGUARD_LOG(int level, const struct unit *punit, const char *msg)
{
  char buffer[500];
  int minlevel = MIN(LOGLEVEL_BODYGUARD, level);
  const struct unit *pcharge;
  const struct city *pcity;
  int id = -1;
  int charge_x = -1;
  int charge_y = -1;
  const char *type = "guard";
  const char *s = "none";

  if (punit->debug) {
    minlevel = LOG_TEST;
  } else if (minlevel > fc_log_level) {
    return;
  }

  pcity = game_find_city_by_number(punit->ai.charge);
  pcharge = game_find_unit_by_number(punit->ai.charge);
  if (pcharge) {
    charge_x = pcharge->tile->x;
    charge_y = pcharge->tile->y;
    id = pcharge->id;
    type = "bodyguard";
    s = unit_rule_name(pcharge);
  } else if (pcity) {
    charge_x = pcity->tile->x;
    charge_y = pcity->tile->y;
    id = pcity->id;
    type = "cityguard";
    s = city_name(pcity);
  }
  /* else perhaps the charge died */

  my_snprintf(buffer, sizeof(buffer),
              "%s %s[%d] %s (%d,%d){%s:%d@%d,%d} ",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit),
              punit->id,
              type,
              TILE_XY(punit->tile),
	      s, id, charge_x, charge_y);
  cat_snprintf(buffer, sizeof(buffer), "%s", msg);
  if (punit->debug) {
    notify_conn(NULL, NULL, E_AI_DEBUG, FTC_LOG, NULL, "%s", buffer);
  }
  freelog(minlevel, "%s", buffer);
}

/**************************************************************************
  Measure the time between the calls.  Used to see where in the AI too
  much CPU is being used.
**************************************************************************/
void TIMING_LOG(enum ai_timer timer, enum ai_timer_activity activity)
{
  static int turn = -1;
  int i;

  if (turn == -1) {
    for (i = 0; i < AIT_LAST; i++) {
      aitimer[i][0] = new_timer(TIMER_CPU, TIMER_ACTIVE);
      aitimer[i][1] = new_timer(TIMER_CPU, TIMER_ACTIVE);
      recursion[i] = 0;
    }
  }

  if (game.info.turn != turn) {
    turn = game.info.turn;
    for (i = 0; i < AIT_LAST; i++) {
      clear_timer(aitimer[i][0]);
    }
    assert(activity == TIMER_START);
  }

  if (activity == TIMER_START && recursion[timer] == 0) {
    start_timer(aitimer[timer][0]);
    start_timer(aitimer[timer][1]);
    recursion[timer]++;
  } else if (activity == TIMER_STOP && recursion[timer] == 1) {
    stop_timer(aitimer[timer][0]);
    stop_timer(aitimer[timer][1]);
    recursion[timer]--;
  }
}

/**************************************************************************
  Print results
**************************************************************************/
void TIMING_RESULTS(void)
{
  char buf[200];

#define AILOG_OUT(text, which)                                                 \
  my_snprintf(buf, sizeof(buf), "  %s: %g sec turn, %g sec game", text,  \
           read_timer_seconds(aitimer[which][0]),                        \
           read_timer_seconds(aitimer[which][1]));                       \
  freelog(LOG_TEST, "%s", buf);                                          \
  notify_conn(NULL, NULL, E_AI_DEBUG, FTC_LOG, NULL, "%s", buf);

  freelog(LOG_TEST, "  --- AI timing results ---");
  notify_conn(NULL, NULL, E_AI_DEBUG, FTC_LOG, NULL,
              "  --- AI timing results ---");
  AILOG_OUT("Total AI time", AIT_ALL);
  AILOG_OUT("Movemap", AIT_MOVEMAP);
  AILOG_OUT("Units", AIT_UNITS);
  AILOG_OUT(" - Military", AIT_MILITARY);
  AILOG_OUT(" - Attack", AIT_ATTACK);
  AILOG_OUT(" - Defense", AIT_DEFENDERS);
  AILOG_OUT(" - Ferry", AIT_FERRY);
  AILOG_OUT(" - Rampage", AIT_RAMPAGE);
  AILOG_OUT(" - Bodyguard", AIT_BODYGUARD);
  AILOG_OUT(" - Recover", AIT_RECOVER);
  AILOG_OUT(" - Caravan", AIT_CARAVAN);
  AILOG_OUT(" - Hunter", AIT_HUNTER);
  AILOG_OUT(" - Airlift", AIT_AIRLIFT);
  AILOG_OUT(" - Diplomat", AIT_DIPLOMAT);
  AILOG_OUT(" - Air", AIT_AIRUNIT);
  AILOG_OUT(" - Explore", AIT_EXPLORER);
  AILOG_OUT("fstk", AIT_FSTK);
  AILOG_OUT("Settlers", AIT_SETTLERS);
  AILOG_OUT("Workers", AIT_WORKERS);
  AILOG_OUT("Government", AIT_GOVERNMENT);
  AILOG_OUT("Taxes", AIT_TAXES);
  AILOG_OUT("Cities", AIT_CITIES);
  AILOG_OUT(" - Buildings", AIT_BUILDINGS);
  AILOG_OUT(" - Danger", AIT_DANGER);
  AILOG_OUT(" - Worker want", AIT_CITY_TERRAIN);
  AILOG_OUT(" - Military want", AIT_CITY_MILITARY);
  AILOG_OUT(" - Settler want", AIT_CITY_SETTLERS);
  AILOG_OUT("Citizen arrange", AIT_CITIZEN_ARRANGE);
  AILOG_OUT("Tech", AIT_TECH);
}
