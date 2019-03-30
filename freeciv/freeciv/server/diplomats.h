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
#ifndef FC__DIPLOMATS_H
#define FC__DIPLOMATS_H

#include "fc_types.h"

bool diplomat_embassy(struct player *pplayer, struct unit *pdiplomat,
                      struct city *pcity, const struct action *paction);
bool diplomat_investigate(struct player *pplayer, struct unit *pdiplomat,
                          struct city *pcity,
                          const struct action *paction);
void spy_send_sabotage_list(struct connection *pc, struct unit *pdiplomat,
                            struct city *pcity,
                            const struct action *paction,
                            bool disturb_player);
bool spy_poison(struct player *pplayer, struct unit *pdiplomat,
                struct city *pcity, const struct action *paction);
bool spy_sabotage_unit(struct player *pplayer, struct unit *pdiplomat,
                       struct unit *pvictim,
                       const struct action *paction);
bool diplomat_bribe(struct player *pplayer, struct unit *pdiplomat,
                    struct unit *pvictim, const struct action *paction);
int diplomats_unignored_tech_stealings(struct unit *pdiplomat,
                                       struct city *pcity);
bool diplomat_get_tech(struct player *pplayer, struct unit *pdiplomat,
                       struct city  *pcity, int technology,
                       const struct action *paction);
bool diplomat_incite(struct player *pplayer, struct unit *pdiplomat,
                     struct city *pcity, const struct action *paction);
bool diplomat_sabotage(struct player *pplayer, struct unit *pdiplomat,
                       struct city *pcity, Impr_type_id improvement,
                       const struct action *paction);
bool spy_steal_gold(struct player *act_player, struct unit *act_unit,
                    struct city *tgt_city,
                    const struct action *paction);
bool spy_steal_some_maps(struct player *act_player, struct unit *act_unit,
                         struct city *tgt_city,
                         const struct action *paction);
bool spy_nuke_city(struct player *act_player, struct unit *act_unit,
                   struct city *tgt_city, const struct action *paction);

int count_diplomats_on_tile(struct tile *ptile);

#endif  /* FC__DIPLOMATS_H */
