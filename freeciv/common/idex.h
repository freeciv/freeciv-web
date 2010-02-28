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
#ifndef FC__IDEX_H
#define FC__IDEX_H

/**************************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.
***************************************************************************/

#include "fc_types.h"

void idex_init(void);
void idex_free(void);

void idex_register_city(struct city *pcity);
void idex_register_unit(struct unit *punit);

void idex_unregister_city(struct city *pcity);
void idex_unregister_unit(struct unit *punit);

struct city *idex_lookup_city(int id);
struct unit *idex_lookup_unit(int id);

#endif  /* FC__IDEX_H */
