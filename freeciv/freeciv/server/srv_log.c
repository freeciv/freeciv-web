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

#include <stdarg.h>

/* utility */
#include "astring.h"
#include "log.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

/* common */
#include "ai.h"
#include "city.h"
#include "game.h"
#include "map.h"
#include "unit.h"

/* server */
#include "notify.h"
#include "srv_main.h"

/* server/advisors */
#include "advdata.h"

#include "srv_log.h"

static struct timer *aitimer[AIT_LAST][2];
static int recursion[AIT_LAST];

/* General AI logging functions */

/**********************************************************************//**
  Log city messages, they will appear like this
    2: Polish Romenna(5,35) [s1 d106 u11 g1] must have Archers ...
**************************************************************************/
void real_city_log(const char *file, const char *function, int line,
                   enum log_level level, bool notify,
                   const struct city *pcity, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  char aibuf[500] = "\0";

  CALL_PLR_AI_FUNC(log_fragment_city, city_owner(pcity), aibuf, sizeof(aibuf), pcity);

  fc_snprintf(buffer, sizeof(buffer), "%s %s(%d,%d) (s%d) {%s} ",
              nation_rule_name(nation_of_city(pcity)),
              city_name_get(pcity),
              TILE_XY(pcity->tile), city_size_get(pcity),
              aibuf);

  va_start(ap, msg);
  fc_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (notify) {
    notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s", buffer);
  }
  do_log(file, function, line, FALSE, level, "%s", buffer);
}

/**********************************************************************//**
  Log unit messages, they will appear like this
    2: Polish Archers[139] (5,35)->(0,0){0,0} stays to defend city
  where [] is unit id, ()->() are coordinates present and goto, and
  {,} contains bodyguard and ferryboat ids.
**************************************************************************/
void real_unit_log(const char *file, const char *function, int line,
                   enum log_level level,  bool notify,
                   const struct unit *punit, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  int gx, gy;
  char aibuf[500] = "\0";

  CALL_PLR_AI_FUNC(log_fragment_unit, unit_owner(punit), aibuf, sizeof(aibuf), punit);

  if (punit->goto_tile) {
    index_to_map_pos(&gx, &gy, tile_index(punit->goto_tile));
  } else {
    gx = gy = -1;
  }

  fc_snprintf(buffer, sizeof(buffer),
              "%s %s(%d) %s (%d,%d)->(%d,%d){%s} ",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit),
              punit->id,
              get_activity_text(punit->activity),
              TILE_XY(unit_tile(punit)),
              gx, gy, aibuf);

  va_start(ap, msg);
  fc_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (notify) {
    notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s", buffer);
  }
  do_log(file, function, line, FALSE, level, "%s", buffer);
}

/**********************************************************************//**
  Measure the time between the calls.  Used to see where in the AI too
  much CPU is being used.
**************************************************************************/
void timing_log_real(enum ai_timer timer, enum ai_timer_activity activity)
{
  static int turn = -1;

  if (game.info.turn != turn) {
    int i;

    turn = game.info.turn;
    for (i = 0; i < AIT_LAST; i++) {
      timer_clear(aitimer[i][0]);
    }
    fc_assert(activity == TIMER_START);
  }

  if (activity == TIMER_START && recursion[timer] == 0) {
    timer_start(aitimer[timer][0]);
    timer_start(aitimer[timer][1]);
    recursion[timer]++;
  } else if (activity == TIMER_STOP && recursion[timer] == 1) {
    timer_stop(aitimer[timer][0]);
    timer_stop(aitimer[timer][1]);
    recursion[timer]--;
  }
}

/**********************************************************************//**
  Print results
**************************************************************************/
void timing_results_real(void)
{
  char buf[200];

#ifdef LOG_TIMERS

#define AILOG_OUT(text, which)                                              \
  fc_snprintf(buf, sizeof(buf), "  %s: %g sec turn, %g sec game", text,     \
              timer_read_seconds(aitimer[which][0]),                        \
              timer_read_seconds(aitimer[which][1]));                       \
  log_test("%s", buf);                                                      \
  notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s", buf);

  log_test("  --- AI timing results ---");

#else  /* LOG_TIMERS */

#define AILOG_OUT(text, which)                                          \
  fc_snprintf(buf, sizeof(buf), "  %s: %g sec turn, %g sec game", text, \
              timer_read_seconds(aitimer[which][0]),                    \
              timer_read_seconds(aitimer[which][1]));                   \
  notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s", buf);

#endif /* LOG_TIMERS */

  notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log,
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

/**********************************************************************//**
  Initialize AI timing system
**************************************************************************/
void timing_log_init(void)
{
  int i;

  for (i = 0; i < AIT_LAST; i++) {
    aitimer[i][0] = timer_new(TIMER_CPU, TIMER_ACTIVE);
    aitimer[i][1] = timer_new(TIMER_CPU, TIMER_ACTIVE);
    recursion[i] = 0;
  }
}

/**********************************************************************//**
  Free AI timing system resources
**************************************************************************/
void timing_log_free(void)
{
  int i;

  for (i = 0; i < AIT_LAST; i++) {
    timer_destroy(aitimer[i][0]);
    timer_destroy(aitimer[i][1]);
  }
}
