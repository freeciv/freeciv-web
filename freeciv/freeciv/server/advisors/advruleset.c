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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "base.h"
#include "effects.h"
#include "map.h"
#include "movement.h"
#include "unittype.h"

#include "advruleset.h"

/**********************************************************************//**
  Initialise the unit data from the ruleset for the advisors.
**************************************************************************/
void adv_units_ruleset_init(void)
{
  unit_class_iterate(pclass) {
    bool move_land_enabled  = FALSE; /* Can move at some land terrains */
    bool move_land_disabled = FALSE; /* Cannot move at some land terrains */
    bool move_sea_enabled   = FALSE; /* Can move at some ocean terrains */
    bool move_sea_disabled  = FALSE; /* Cannot move at some ocean terrains */

    terrain_type_iterate(pterrain) {
      if (is_native_to_class(pclass, pterrain, NULL)) {
        /* Can move at terrain */
        if (is_ocean(pterrain)) {
          move_sea_enabled = TRUE;
        } else {
          move_land_enabled = TRUE;
        }
      } else {
        /* Cannot move at terrain */
        if (is_ocean(pterrain)) {
          move_sea_disabled = TRUE;
        } else {
          move_land_disabled = TRUE;
        }
      }
    } terrain_type_iterate_end;

    if (move_land_enabled && !move_land_disabled) {
      pclass->adv.land_move = MOVE_FULL;
    } else if (move_land_enabled && move_land_disabled) {
      pclass->adv.land_move = MOVE_PARTIAL;
    } else {
      fc_assert(!move_land_enabled);
      pclass->adv.land_move = MOVE_NONE;
    }

    if (move_sea_enabled && !move_sea_disabled) {
      pclass->adv.sea_move = MOVE_FULL;
    } else if (move_sea_enabled && move_sea_disabled) {
      pclass->adv.sea_move = MOVE_PARTIAL;
    } else {
      fc_assert(!move_sea_enabled);
      pclass->adv.sea_move = MOVE_NONE;
    }

  } unit_class_iterate_end;

  unit_type_iterate(ptype) {
    ptype->adv.igwall = TRUE;

    effect_list_iterate(get_effects(EFT_DEFEND_BONUS), peffect) {
      if (peffect->value > 0) {
        requirement_vector_iterate(&peffect->reqs, preq) {
          if (!is_req_active(NULL, NULL, NULL, NULL, NULL, NULL, ptype,
                             NULL, NULL, NULL, preq, RPT_POSSIBLE)) {
            ptype->adv.igwall = FALSE;
            break;
          }
        } requirement_vector_iterate_end;
      }
      if (!ptype->adv.igwall) {
        break;
      }
    } effect_list_iterate_end;

    ptype->adv.worker = utype_has_flag(ptype, UTYF_SETTLERS);
  } unit_type_iterate_end;
}
