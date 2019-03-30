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
#ifndef FC__HANDICAPS_H
#define FC__HANDICAPS_H

/* See handicap_desc() for what these do. */
enum handicap_type {
  H_DIPLOMAT = 0,
  H_AWAY,
  H_LIMITEDHUTS,
  H_DEFENSIVE,
  H_EXPERIMENTAL,
  H_RATES,
  H_TARGETS,
  H_HUTS,
  H_FOG,
  H_NOPLANES,
  H_MAP,
  H_DIPLOMACY,
  H_REVOLUTION,
  H_EXPANSION,
  H_DANGER,
  H_CEASEFIRE,
  H_NOBRIBE_WF,
  H_PRODCHGPEN,
#ifdef FREECIV_WEB
  H_ASSESS_DANGER_LIMITED,
#endif
  H_LAST
};

BV_DEFINE(bv_handicap, H_LAST);

void handicaps_init(struct player *pplayer);
void handicaps_close(struct player *pplayer);

void handicaps_set(struct player *pplayer, bv_handicap handicaps);
bool has_handicap(const struct player *pplayer, enum handicap_type htype);

const char *handicap_desc(enum handicap_type htype, bool *inverted);

#endif /* FC__HANDICAPS_H */
