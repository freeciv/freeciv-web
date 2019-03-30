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
#ifndef FC__AIPLAYER_H
#define FC__AIPLAYER_H

/* common */
#include "player.h"

struct player;
struct ai_plr;

void dai_player_alloc(struct ai_type *ait, struct player *pplayer);
void dai_player_free(struct ai_type *ait, struct player *pplayer);
void dai_player_save(struct ai_type *ait, const char *aitstr,
                     struct player *pplayer, struct section_file *file,
                     int plrno);
void dai_player_save_relations(struct ai_type *ait, const char *aitstr,
                               struct player *pplayer, struct player *other,
                               struct section_file *file, int plrno);
void dai_player_load(struct ai_type *ait, const char *aitstr,
                     struct player *pplayer, const struct section_file *file,
                     int plrno);
void dai_player_load_relations(struct ai_type *ait, const char *aitstr,
                               struct player *pplayer, struct player *other,
                               const struct section_file *file, int plrno);

void dai_player_copy(struct ai_type *ait,
                     struct player *original, struct player *created);
void dai_gained_control(struct ai_type *ait, struct player *pplayer);

static inline struct ai_city *def_ai_city_data(const struct city *pcity,
                                               struct ai_type *deftype)
{
  return (struct ai_city *)city_ai_data(pcity, deftype);
}

static inline struct unit_ai *def_ai_unit_data(const struct unit *punit,
                                               struct ai_type *deftype)
{
  return (struct unit_ai *)unit_ai_data(punit, deftype);
}

static inline struct ai_plr *def_ai_player_data(const struct player *pplayer,
                                                struct ai_type *deftype)
{
  return (struct ai_plr *)player_ai_data(pplayer, deftype);
}

struct dai_private_data
{
  bool contemplace_workers;
};

#endif /* FC__AIPLAYER_H */
