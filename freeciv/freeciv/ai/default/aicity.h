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
#ifndef FC__AICITY_H
#define FC__AICITY_H

/* common */
#include "effects.h" /* enum effect_type */
#include "fc_types.h"

/* server/advisors */
#include "advchoice.h"

struct adv_data;
struct tech_vector;

struct ai_activity_cache; /* defined and only used within aicity.c */

/* Who's coming to kill us, for attack co-ordination */
struct ai_invasion {
  int attack;         /* Units capable of attacking city */
  int occupy;         /* Units capable of occupying city */
};

struct ai_city {
  adv_want worth; /* Cache city worth here, sum of all weighted incomes */

  int building_turn;            /* only recalculate every Nth turn */
  int building_wait;            /* for weighting values */
#define BUILDING_WAIT_MINIMUM (1)

  struct adv_choice choice;     /* to spend gold in the right place only */

  struct ai_invasion invasion;
  int attack, bcost; /* This is also for invasion - total power and value of
                      * all units coming to kill us. */

  unsigned int danger;          /* danger to be compared to assess_defense */
  unsigned int grave_danger;    /* danger, should show positive feedback */
  unsigned int urgency;         /* how close the danger is; if zero,
                                   bodyguards can leave */
  int wallvalue;                /* how much it helps for defenders to be
                                   ground units */

  int distance_to_wonder_city;  /* wondercity will set this for us,
                                   avoiding paradox */

  bool celebrate;               /* try to celebrate in this city */
  bool diplomat_threat;         /* enemy diplomat or spy is near the city */
  bool has_diplomat;            /* this city has diplomat or spy defender */

  /* These values are for builder (UTYF_SETTLERS) and founder (UTYF_CITIES) units.
   * Negative values indicate that the city needs a boat first;
   * -value is the degree of want in that case. */
  bool founder_boat;            /* city founder will need a boat */
  int founder_turn;             /* only recalculate every Nth turn */
  int founder_want;
  int worker_want;
  struct unit_type *worker_type;
};

void dai_manage_cities(struct ai_type *ait, struct player *pplayer);

void dai_city_alloc(struct ai_type *ait, struct city *pcity);
void dai_city_free(struct ai_type *ait, struct city *pcity);

struct section_file;
void dai_city_save(struct ai_type *ait, const char *aitstr,
                   struct section_file *file,
                   const struct city *pcity, const char *citystr);
void dai_city_load(struct ai_type *ait, const char *aitstr,
                   const struct section_file *file,
                   struct city *pcity, const char *citystr);

void want_techs_for_improvement_effect(struct ai_type *ait,
                                       struct player *pplayer,
                                       const struct city *pcity,
                                       const struct impr_type *pimprove,
                                       struct tech_vector *needed_techs,
                                       adv_want building_want);

void dont_want_tech_obsoleting_impr(struct ai_type *ait,
                                    struct player *pplayer,
                                    const struct city *pcity,
                                    const struct impr_type *pimprove,
                                    adv_want building_want);

void dai_build_adv_init(struct ai_type *ait, struct player *pplayer);
void dai_build_adv_adjust(struct ai_type *ait, struct player *pplayer,
                          struct city *wonder_city);

void dai_consider_wonder_city(struct ai_type *ait, struct city *pcity, bool *result);

Impr_type_id dai_find_source_building(struct city *pcity,
                                      enum effect_type effect_type,
                                      struct unit_type *utype);

adv_want dai_city_want(struct player *pplayer, struct city *acity, 
                       struct adv_data *adv, struct impr_type *pimprove);

#endif  /* FC__AICITY_H */
