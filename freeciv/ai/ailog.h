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
#ifndef FC__AILOG_H
#define FC__AILOG_H

#include "fc_types.h"

#include "gotohand.h"		/* enum goto_result */

struct ai_data;

/* 
 * Change these and remake to watch logs from a specific 
 * part of the AI code.
 */
#define LOGLEVEL_BODYGUARD LOG_DEBUG
#define LOGLEVEL_UNIT LOG_DEBUG
#define LOGLEVEL_GOTO LOG_DEBUG
#define LOGLEVEL_CITY LOG_DEBUG
#define LOGLEVEL_BUILD LOG_DEBUG
#define LOGLEVEL_HUNT LOG_DEBUG
#define LOGLEVEL_PLAYER LOG_DEBUG
#define LOGLEVEL_TECH LOG_DEBUG

enum ai_timer {
  AIT_ALL,
  AIT_MOVEMAP,
  AIT_UNITS,
  AIT_SETTLERS,
  AIT_WORKERS,
  AIT_AIDATA,
  AIT_GOVERNMENT,
  AIT_TAXES,
  AIT_CITIES,
  AIT_CITIZEN_ARRANGE,
  AIT_BUILDINGS,
  AIT_DANGER,
  AIT_TECH,
  AIT_FSTK,
  AIT_DEFENDERS,
  AIT_CARAVAN,
  AIT_HUNTER,
  AIT_AIRLIFT,
  AIT_DIPLOMAT,
  AIT_AIRUNIT,
  AIT_EXPLORER,
  AIT_EMERGENCY,
  AIT_CITY_MILITARY,
  AIT_CITY_TERRAIN,
  AIT_CITY_SETTLERS,
  AIT_ATTACK,
  AIT_MILITARY,
  AIT_RECOVER,
  AIT_BODYGUARD,
  AIT_FERRY,
  AIT_RAMPAGE,
  AIT_LAST
};

enum ai_timer_activity  {
  TIMER_START, TIMER_STOP
};

void TECH_LOG(int level, const struct player *pplayer,
              struct advance *padvance, const char *msg, ...)
     fc__attribute((__format__ (__printf__, 4, 5)));
void DIPLO_LOG(int level, const struct player *pplayer,
	       const struct player *aplayer, const char *msg, ...)
     fc__attribute((__format__ (__printf__, 4, 5)));
void CITY_LOG(int level, const struct city *pcity, const char *msg, ...)
     fc__attribute((__format__ (__printf__, 3, 4)));
void UNIT_LOG(int level, const struct unit *punit, const char *msg, ...)
     fc__attribute((__format__ (__printf__, 3, 4)));
void BODYGUARD_LOG(int level, const struct unit *punit, const char *msg);
void TIMING_LOG(enum ai_timer timer, enum ai_timer_activity activity);
void TIMING_RESULTS(void);

#endif  /* FC__AILOG_H */
