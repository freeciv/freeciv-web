/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__API_TYPES_H
#define FC__API_TYPES_H

#include "game.h"
#include "player.h"
#include "city.h"
#include "unit.h"
#include "tile.h"
#include "government.h"
#include "nation.h"
#include "improvement.h"
#include "tech.h"
#include "unittype.h"
#include "terrain.h"

#include "events.h"

/* Classes. */
typedef struct player Player;
typedef struct player_ai Player_ai;
typedef struct city City;
typedef struct unit Unit;
typedef struct tile Tile;
typedef struct government Government;
typedef struct nation_type Nation_Type;
typedef struct impr_type Building_Type;
typedef struct unit_type Unit_Type;
typedef struct advance Tech_Type;
typedef struct terrain Terrain;

#endif

