/***********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Project
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

/* utility */
#include "log.h"
#include "support.h"
 
/* Change these and remake to watch logs from a specific 
   part of the AI code. */
#define LOGLEVEL_TECH LOG_DEBUG

struct player;

void dai_city_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct city *pcity);
void dai_unit_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct unit *punit);

void real_tech_log(struct ai_type *ait, const char *file, const char *function,
                   int line, enum log_level level, bool send_notify,
                   const struct player *pplayer, struct advance *padvance,
                   const char *msg, ...)
                   fc__attribute((__format__ (__printf__, 9, 10)));
#define TECH_LOG(ait, loglevel, pplayer, padvance, msg, ...)                \
{                                                                           \
  bool send_notify = BV_ISSET(pplayer->server.debug, PLAYER_DEBUG_TECH);    \
  enum log_level level = (send_notify ? LOG_AI_TEST                         \
                          : MIN(loglevel, LOGLEVEL_TECH));                  \
  if (log_do_output_for_level(level)) {                                     \
    real_tech_log(ait, __FILE__, __FUNCTION__, __FC_LINE__, level,          \
                  send_notify, pplayer, padvance, msg, ## __VA_ARGS__);     \
  }                                                                         \
}

void real_diplo_log(struct ai_type *ait,
                    const char *file, const char *function, int line,
                    enum log_level level, bool send_notify,
                    const struct player *pplayer,
                    const struct player *aplayer, const char *msg, ...)
                   fc__attribute((__format__ (__printf__, 9, 10)));
#define DIPLO_LOG(ait, loglevel, pplayer, aplayer, msg, ...)                  \
{                                                                             \
  bool send_notify = BV_ISSET(pplayer->server.debug, PLAYER_DEBUG_DIPLOMACY); \
  enum log_level level = (send_notify ? LOG_AI_TEST                           \
                          : MIN(loglevel, LOGLEVEL_PLAYER));                  \
  if (log_do_output_for_level(level)) {                                       \
    real_diplo_log(ait, __FILE__, __FUNCTION__, __FC_LINE__, level,           \
                   send_notify, pplayer, aplayer, msg, ## __VA_ARGS__);       \
  }                                                                           \
}

void real_bodyguard_log(struct ai_type *ait, const char *file,
                        const char *function, int line,
                        enum log_level level,  bool send_notify,
                        const struct unit *punit, const char *msg, ...)
                        fc__attribute((__format__ (__printf__, 8, 9)));
#define BODYGUARD_LOG(ait, loglevel, punit, msg, ...)                       \
{                                                                           \
  bool send_notify = punit->server.debug;                                   \
  enum log_level level = (send_notify ? LOG_AI_TEST                         \
                          : MIN(loglevel, LOGLEVEL_BODYGUARD));             \
  if (log_do_output_for_level(level)) {                                     \
    real_bodyguard_log(ait, __FILE__, __FUNCTION__, __FC_LINE__, level,     \
                       send_notify, punit,                                  \
                       msg, ## __VA_ARGS__);                                \
  }                                                                         \
}

#endif /* FC__AILOG_H */
