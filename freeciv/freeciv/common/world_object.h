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
#ifndef FC__WORLD_OBJECT_H
#define FC__WORLD_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "map_types.h"


/* struct city_hash. */
#define SPECHASH_TAG city
#define SPECHASH_INT_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct city *
#include "spechash.h"

/* struct unit_hash. */
#define SPECHASH_TAG unit
#define SPECHASH_INT_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct unit *
#include "spechash.h"

struct world
{
  struct civ_map map;
  struct city_hash *cities;
  struct unit_hash *units;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__WORLD_OBJECT_H */
