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
#ifndef FC__CITYHAND_H
#define FC__CITYHAND_H

#include "fc_types.h"

#include "hand_gen.h"

struct connection;
struct conn_list;

void really_handle_city_sell(struct player *pplayer, struct city *pcity,
			     struct impr_type *pimprove);
void really_handle_city_buy(struct player *pplayer, struct city *pcity);

#endif  /* FC__CITYHAND_H */
