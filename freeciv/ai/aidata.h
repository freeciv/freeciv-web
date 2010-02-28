/********************************************************************** 
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

/* max size of a short */
#define MAX_NUM_ID (1+MAX_UINT16)

#include "shared.h"		/* bool type */

#include "fc_types.h"
#include "improvement.h"

#include "advdiplomacy.h"

/* 
 * This file and aidata.c contains global data structures for the AI
 * and some of the functions that fill them with useful values at the 
 * start of every turn. 
 */

enum ai_improvement_status {
  AI_IMPR_CALCULATE, /* Calculate exactly its effect */
  AI_IMPR_CALCULATE_FULL, /* Calculate including tile changes */
  AI_IMPR_ESTIMATE,  /* Estimate its effect using wild guesses */
  AI_IMPR_LAST
};

enum winning_strategy {
  WIN_OPEN,     /* still undetermined */
  WIN_WAR,      /* we have no other choice than to crush all opposition */
  WIN_SPACE,    /* we will race for space, peace very important */
  WIN_CAPITAL   /* we cannot win unless we take war_target's capital */
};

struct ai_dip_intel {
  /* Remember one example of each for text spam purposes. */
  struct player *is_allied_with_enemy;
  struct player *at_war_with_ally;
  struct player *is_allied_with_ally;

  char spam;      /* timer to avoid spamming a player with chat */
  int distance;   /* average distance to that player's cities */
  int countdown;  /* we're on a countdown to war declaration */
  enum war_reason war_reason; /* why we declare war */
  char ally_patience; /* we EXPECT our allies to help us! */
  char asked_about_peace;     /* don't ask again */
  char asked_about_alliance;  /* don't nag! */
  char asked_about_ceasefire; /* don't ... you get the point */
  char warned_about_space;
};

BV_DEFINE(bv_id, MAX_NUM_ID);
struct ai_data {
  /* The Wonder City */
  int wonder_city;

  /* Precalculated info about city improvements */
  enum ai_improvement_status impr_calc[MAX_NUM_ITEMS];
  enum req_range impr_range[MAX_NUM_ITEMS];

  /* AI diplomacy and opinions on other players */
  struct {
    struct ai_dip_intel player_intel[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
    enum winning_strategy strategy;
    int timer; /* pursue our goals with some stubbornness, in turns */
    char love_coeff;          /* Reduce love with this % each turn */
    char love_incr;           /* Modify love with this fixed amount */
    int req_love_for_peace;
    int req_love_for_alliance;
    struct player *spacerace_leader; /* who is leading the space pack */
    struct player *production_leader;
  } diplomacy;

  /* Long-term threats, not to be confused with short-term danger */
  struct {
    bool invasions;   /* check if we need to consider invasions */
    bool *continent;  /* non-allied cities on continent? */
    bool *ocean;      /* non-allied offensive ships in ocean? */
    bool missile;     /* check for non-allied missiles */
    int nuclear;      /* nuke check: 0=no, 1=capability, 2=built */
    bool igwall;      /* enemies have igwall units */
  } threats;

  /* Keeps track of which continents are fully explored already */
  struct {
    bool *ocean;      /* are we done exploring this ocean? */
    bool *continent;  /* are we done exploring this continent? */
    bool land_done;   /* nothing more on land to explore anywhere */
    bool sea_done;    /* nothing more to explore at sea */
  } explore;

  /* Keep track of available ocean channels */
  bool *channels;

  /* This struct is used for statistical unit building, eg to ensure
   * that we don't build too few or too many units of a given type. */
  struct {
    /* Counts of specific types of units. */
    struct {
      /* Unit-flag counts. */
      int triremes, missiles;

      /* Move-type counts */
      int land, sea, amphibious;

      /* Upgradeable units */
      int upgradeable;
      
      int paratroopers;
    } units;
    int *workers;     /* cities to workers on continent*/
    int *cities;      /* number of cities we have on continent */
    int passengers;   /* number of passengers waiting for boats */
    int boats;
    int available_boats;
    int average_production;
    bv_id diplomat_reservations;
  } stats;

  int num_continents; /* last time we updated our continent data */
  int num_oceans; /* last time we updated our continent data */

  /* Dynamic weights used in addition to Syela's hardcoded weights */
  int shield_priority;
  int food_priority;
  int luxury_priority;
  int gold_priority;
  int science_priority;
  int happy_priority;
  int unhappy_priority;
  int angry_priority;
  int pollution_priority;

  /* Government data */
  int *government_want;
  short govt_reeval;

  /* Goals */
  struct {
    struct {
      struct government *gov;        /* The ideal government */
      int val;        /* Its value (relative to the current gov) */
      int req;        /* The tech requirement for the ideal gov */
    } govt;
    struct government *revolution;   /* The best gov of the now available */
  } goal;
  
  /* If the ai doesn't want/need any research */
  bool wants_no_science;
  
  /* AI doesn't like having more than this number of cities */
  int max_num_cities;
};

void ai_data_init(struct player *pplayer);
void ai_data_phase_init(struct player *pplayer, bool is_new_phase);
void ai_data_phase_done(struct player *pplayer);

void ai_data_analyze_rulesets(struct player *pplayer);

struct ai_data *ai_data_get(struct player *pplayer);
const struct ai_dip_intel *ai_diplomacy_get(const struct player *pplayer,
					    const struct player *aplayer);

bool ai_channel(struct player *pplayer, Continent_id c1, Continent_id c2);

#endif
