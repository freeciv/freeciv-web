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
#ifndef FC__IDEX_H
#define FC__IDEX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**************************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.
***************************************************************************/

/* common */
#include "fc_types.h"
#include "world_object.h"

void idex_init(struct world *iworld);
void idex_free(struct world *iworld);

void idex_register_city(struct world *iworld, struct city *pcity);
void idex_register_unit(struct world *iworld, struct unit *punit);

void idex_unregister_city(struct world *iworld, struct city *pcity);
void idex_unregister_unit(struct world *iworld, struct unit *punit);

struct city *idex_lookup_city(struct world *iworld, int id);
struct unit *idex_lookup_unit(struct world *iworld, int id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__IDEX_H */
