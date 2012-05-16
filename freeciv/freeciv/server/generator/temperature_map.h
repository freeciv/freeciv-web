/********************************************************************** 
 Freeciv - Copyright (C) 2004 - Marcelo J. Burda
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__TEMPERATURE_MAP_H
#define FC__TEMPERATURE_MAP_H

#include "fcintl.h"
#include "shared.h"		/* bool type */

/*
 *  temperature_map[] stores the temperature of each tile
 *  values on tmap can get one of these 4 values
 *  there is 4 extra values as macros combining the 4 basics ones
 */
typedef int temperature_type;

#define  TT_FROZEN    1
#define  TT_COLD      2
#define  TT_TEMPERATE 4
#define  TT_TROPICAL  8

#define TT_NFROZEN (TT_COLD | TT_TEMPERATE | TT_TROPICAL)
#define TT_ALL (TT_FROZEN | TT_NFROZEN)
#define TT_NHOT (TT_FROZEN | TT_COLD)
#define TT_HOT (TT_TEMPERATE | TT_TROPICAL)

bool temperature_is_initialized(void);
bool tmap_is(const struct tile *ptile, temperature_type tt);
bool is_temperature_type_near(const struct tile *ptile, temperature_type tt);
void destroy_tmap(void);
void create_tmap(bool real);

#endif  /* FC__TEMPERATURE_MAP_H */
