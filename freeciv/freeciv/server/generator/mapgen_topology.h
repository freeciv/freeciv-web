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
#ifndef FC__MAPGEN_TOPOLOGY_H
#define FC__MAPGEN_TOPOLOGY_H

/* utility */
#include "support.h"            /* bool type */

/* this is the maximal colatitude at equators returned by 
   map_colatitude */

#define MAX_COLATITUDE 1000

int get_sqsize(void);

/* Size safe Unit of colatitude. 0 is not allowed due to possibility of
 * division by 0 in mapgen.c */ 
#define L_UNIT MAX(1, wld.map.server.size * MAX_COLATITUDE / (30 * get_sqsize()))

/* define the 3 regions of a Earth like map:
     0           - COLD_LV:        cold region
     COLD_LV     - TREOPICAL_LV:   temperate wet region
     TROPICAL_LV - MAX_COLATITUDE: tropical wet region
   and a dry region, this last one can ovelap others
   DRY_MIN_LEVEL- DRY_MAX_LEVEL */
#define COLD_LEVEL                                                           \
   (MAX(0, MAX_COLATITUDE * (60*7 - wld.map.server.temperature * 6 ) / 700))
#define TROPICAL_LEVEL                                                       \
   (MIN(MAX_COLATITUDE * 9 /10,                                              \
    MAX_COLATITUDE * (143*7 - wld.map.server.temperature * 10) / 700))
#define DRY_MIN_LEVEL                                                        \
   (MAX_COLATITUDE * (7300 - wld.map.server.temperature * 18 ) / 10000)
#define DRY_MAX_LEVEL                                                        \
   (MAX_COLATITUDE * (7300 + wld.map.server.temperature * 17 ) / 10000)

/* used to create the poles and for separating them.  In a
 * mercator projection map we don't want the poles to be too big. */
extern int ice_base_colatitude;
#define ICE_BASE_LEVEL ice_base_colatitude

int map_colatitude(const struct tile *ptile);
bool near_singularity(const struct tile *ptile);
void generator_init_topology(bool autosize);

#endif  /* FC__MAPGEN_TOPOLOGY_H */
