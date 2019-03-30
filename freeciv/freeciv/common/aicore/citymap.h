/***********************************************************************
 Freeciv - Copyright (C) 2003 - Per I. Mathisen
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__CITYMAP_H
#define FC__CITYMAP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* utility */
#include "support.h" /* bool type */

/* common */
#include "fc_types.h"

void citymap_turn_init(struct player *pplayer);
void citymap_reserve_city_spot(struct tile *ptile, int id);
void citymap_free_city_spot(struct tile *ptile, int id);
void citymap_reserve_tile(struct tile *ptile, int id);
int citymap_read(struct tile *ptile);
bool citymap_is_reserved(struct tile *ptile);

void citymap_free(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__CITYMAP_H */
