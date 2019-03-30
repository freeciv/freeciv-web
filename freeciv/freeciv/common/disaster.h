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
#ifndef FC__DISASTER_H
#define FC__DISASTER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "name_translation.h"
#include "requirements.h"

/* Used in the network protocol. */
#define SPECENUM_NAME disaster_effect_id
#define SPECENUM_VALUE0 DE_DESTROY_BUILDING
#define SPECENUM_VALUE0NAME "DestroyBuilding"
#define SPECENUM_VALUE1 DE_REDUCE_POP
#define SPECENUM_VALUE1NAME "ReducePopulation"
#define SPECENUM_VALUE2 DE_EMPTY_FOODSTOCK
#define SPECENUM_VALUE2NAME "EmptyFoodStock"
#define SPECENUM_VALUE3 DE_EMPTY_PRODSTOCK
#define SPECENUM_VALUE3NAME "EmptyProdStock"
#define SPECENUM_VALUE4 DE_POLLUTION
#define SPECENUM_VALUE4NAME "Pollution"
#define SPECENUM_VALUE5 DE_FALLOUT
#define SPECENUM_VALUE5NAME "Fallout"
#define SPECENUM_VALUE6 DE_REDUCE_DESTROY
#define SPECENUM_VALUE6NAME "ReducePopDestroy"
#define SPECENUM_COUNT DE_COUNT
#define SPECENUM_BITVECTOR bv_disaster_effects
#include "specenum_gen.h"

#define DISASTER_BASE_RARITY 1000000

struct disaster_type {
  int id;
  struct name_translation name;

  struct requirement_vector reqs;

  /* Final probability for each city each turn is
   * this frequency * game.info.disasters frequency setting / DISASTER_BASE_RARITY */
  int frequency;

  bv_disaster_effects effects;
};

/* Initialization and iteration */
void disaster_types_init(void);
void disaster_types_free(void);

/* General disaster type accessor functions. */
Disaster_type_id disaster_count(void);
Disaster_type_id disaster_index(const struct disaster_type *pdis);
Disaster_type_id disaster_number(const struct disaster_type *pdis);

struct disaster_type *disaster_by_number(Disaster_type_id id);

const char *disaster_name_translation(struct disaster_type *pdis);
const char *disaster_rule_name(struct disaster_type *pdis);

bool disaster_has_effect(const struct disaster_type *pdis,
                         enum disaster_effect_id effect);

bool can_disaster_happen(const struct disaster_type *pdis,
                         const struct city *pcity);

#define disaster_type_iterate(_p)                                \
{                                                                \
  int _i_;                                                       \
  for (_i_ = 0; _i_ < game.control.num_disaster_types ; _i_++) { \
    struct disaster_type *_p = disaster_by_number(_i_);

#define disaster_type_iterate_end                       \
  }}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__DISASTER_H */
