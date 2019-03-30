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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>

/* common */
#include "map.h"
#include "player.h"
#include "research.h"

/* server */
#include "notify.h"

/* ai */
#include "aicity.h"
#include "aidata.h"
#include "aiplayer.h"
#include "aiunit.h"

#include "ailog.h"


/**********************************************************************//**
  Produce logline fragment for srv_log.
**************************************************************************/
void dai_city_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct city *pcity)
{
  struct ai_city *city_data = def_ai_city_data(pcity, ait);

  fc_snprintf(buffer, buflength, "d%d u%d g%d",
              city_data->danger, city_data->urgency,
              city_data->grave_danger);
}

/**********************************************************************//**
  Produce logline fragment for srv_log.
**************************************************************************/
void dai_unit_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct unit *punit)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  fc_snprintf(buffer, buflength, "%d %d",
              unit_data->bodyguard, unit_data->ferryboat);
}

/**********************************************************************//**
  Log player tech messages.
**************************************************************************/
void real_tech_log(struct ai_type *ait, const char *file, const char *function,
                   int line, enum log_level level, bool send_notify,
                   const struct player *pplayer, struct advance *padvance,
                   const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  struct ai_plr *plr_data;

  if (!valid_advance(padvance) || advance_by_number(A_NONE) == padvance) {
    return;
  }

  plr_data = def_ai_player_data(pplayer, ait);
  fc_snprintf(buffer, sizeof(buffer), "%s::%s (want " ADV_WANT_PRINTF ", dist %d) ",
              player_name(pplayer),
              advance_rule_name(padvance),
              plr_data->tech_want[advance_index(padvance)],
              research_goal_unknown_techs(research_get(pplayer),
                                          advance_number(padvance)));

  va_start(ap, msg);
  fc_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (send_notify) {
    notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s", buffer);
  }
  do_log(file, function, line, FALSE, level, "%s", buffer);
}

/**********************************************************************//**
  Log player messages, they will appear like this

  where ti is timer, co countdown and lo love for target, who is e.
**************************************************************************/
void real_diplo_log(struct ai_type *ait, const char *file,
                    const char *function, int line,
                    enum log_level level, bool send_notify,
                    const struct player *pplayer,
                    const struct player *aplayer, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  const struct ai_dip_intel *adip;

  /* Don't use ai_data_get since it can have side effects. */
  adip = dai_diplomacy_get(ait, pplayer, aplayer);

  fc_snprintf(buffer, sizeof(buffer), "%s->%s(l%d,c%d,d%d%s): ",
              player_name(pplayer),
              player_name(aplayer),
              pplayer->ai_common.love[player_index(aplayer)],
              adip->countdown,
              adip->distance,
              adip->is_allied_with_enemy ? "?" :
              (adip->at_war_with_ally ? "!" : ""));

  va_start(ap, msg);
  fc_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (send_notify) {
    notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s", buffer);
  }
  do_log(file, function, line, FALSE, level, "%s", buffer);
}

/**********************************************************************//**
  Log message for bodyguards. They will appear like this
    2: Polish Mech. Inf.[485] bodyguard (38,22){Riflemen:574@37,23} was ...
  note that these messages are likely to wrap if long.
**************************************************************************/
void real_bodyguard_log(struct ai_type *ait, const char *file,
                        const char *function, int line,
                        enum log_level level,  bool send_notify,
                        const struct unit *punit, const char *msg, ...)
{
  char buffer[500];
  char buffer2[500];
  va_list ap;
  const struct unit *pcharge;
  const struct city *pcity;
  int id = -1;
  int charge_x = -1;
  int charge_y = -1;
  const char *type = "guard";
  const char *s = "none";
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  pcity = game_city_by_number(unit_data->charge);
  pcharge = game_unit_by_number(unit_data->charge);
  if (pcharge) {
    index_to_map_pos(&charge_x, &charge_y, tile_index(unit_tile(pcharge)));
    id = pcharge->id;
    type = "bodyguard";
    s = unit_rule_name(pcharge);
  } else if (pcity) {
    index_to_map_pos(&charge_x, &charge_y, tile_index(city_tile(pcity)));
    id = pcity->id;
    type = "cityguard";
    s = city_name_get(pcity);
  }
  /* else perhaps the charge died */

  fc_snprintf(buffer, sizeof(buffer),
              "%s %s[%d] %s (%d,%d){%s:%d@%d,%d} ",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit),
              punit->id,
              type,
              TILE_XY(unit_tile(punit)),
              s, id, charge_x, charge_y);

  va_start(ap, msg);
  fc_vsnprintf(buffer2, sizeof(buffer2), msg, ap);
  va_end(ap);

  cat_snprintf(buffer, sizeof(buffer), "%s", buffer2);
  if (send_notify) {
    notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s", buffer);
  }
  do_log(file, function, line, FALSE, level, "%s", buffer);
}
