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
#ifndef FC__AIDATA_H
#define FC__AIDATA_H

/* utility */
#include "support.h"

/* common */
#include "fc_types.h"
#include "tech.h"

/* server/advisors */
#include "advtools.h"

struct player;

enum winning_strategy {
  WIN_OPEN,     /* still undetermined */
  WIN_WAR,      /* we have no other choice than to crush all opposition */
  WIN_SPACE,    /* we will race for space, peace very important */
  WIN_CAPITAL   /* we cannot win unless we take war_target's capital */
};

#define SPECENUM_NAME war_reason
#define SPECENUM_VALUE0 DAI_WR_BEHAVIOUR
#define SPECENUM_VALUE0NAME "Behaviour"
#define SPECENUM_VALUE1 DAI_WR_SPACE
#define SPECENUM_VALUE1NAME "Space"
#define SPECENUM_VALUE2 DAI_WR_EXCUSE
#define SPECENUM_VALUE2NAME "Excuse"
#define SPECENUM_VALUE3 DAI_WR_HATRED
#define SPECENUM_VALUE3NAME "Hatred"
#define SPECENUM_VALUE4 DAI_WR_ALLIANCE
#define SPECENUM_VALUE4NAME "Alliance"
#define SPECENUM_VALUE5 DAI_WR_NONE
#define SPECENUM_VALUE5NAME "None"
#include "specenum_gen.h"

struct ai_dip_intel {
  /* Remember one example of each for text spam purposes. */
  struct player *is_allied_with_enemy;
  struct player *at_war_with_ally;
  struct player *is_allied_with_ally;

  signed char spam;      /* timer to avoid spamming a player with chat */
  int distance;   /* average distance to that player's cities */
  int countdown;  /* we're on a countdown to war declaration */
  enum war_reason war_reason; /* why we declare war */
  signed char ally_patience; /* we EXPECT our allies to help us! */
  signed char asked_about_peace;     /* don't ask again */
  signed char asked_about_alliance;  /* don't nag! */
  signed char asked_about_ceasefire; /* don't ... you get the point */
  signed char warned_about_space;
};

/* max size of a short */
#define MAX_NUM_ID (1+MAX_UINT16)

BV_DEFINE(bv_id, MAX_NUM_ID);

struct ai_plr
{
  bool phase_initialized;

  int last_num_continents;
  int last_num_oceans;

  struct {
    int passengers;   /* number of passengers waiting for boats */
    int boats;
    int available_boats;

    int *workers;     /* cities to workers on continent */
    int *ocean_workers;

    bv_id diplomat_reservations;
  } stats;

  /* AI diplomacy and opinions on other players */
  struct {
    const struct ai_dip_intel **player_intel_slots;
    enum winning_strategy strategy;
    int timer; /* pursue our goals with some stubbornness, in turns */
    char love_coeff;          /* Reduce love with this % each turn */
    char love_incr;           /* Modify love with this fixed amount */
    int req_love_for_peace;
    int req_love_for_alliance;
  } diplomacy;

  /* Cache map for AI settlers; defined in aisettler.c. */
  struct ai_settler *settler;

  /* The units of tech_want seem to be shields */
  adv_want tech_want[A_LAST+1];
};

void dai_data_init(struct ai_type *ait, struct player *pplayer);
void dai_data_close(struct ai_type *ait, struct player *pplayer);

void dai_data_phase_begin(struct ai_type *ait, struct player *pplayer,
                          bool is_new_phase);
void dai_data_phase_finished(struct ai_type *ait, struct player *pplayer);
bool is_ai_data_phase_open(struct ai_type *ait, struct player *pplayer);

struct ai_plr *dai_plr_data_get(struct ai_type *ait, struct player *pplayer,
                                bool *caller_closes);

struct ai_dip_intel *dai_diplomacy_get(struct ai_type *ait,
                                       const struct player *plr1,
                                       const struct player *plr2);

void dai_gov_value(struct ai_type *ait, struct player *pplayer, struct government *gov,
                   adv_want *val, bool *override);

void dai_adjust_policies(struct ai_type *ait, struct player *pplayer);

#endif /* FC__AIDATA_H */
