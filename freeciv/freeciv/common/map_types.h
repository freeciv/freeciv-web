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
#ifndef FC__MAP_TYPES_H
#define FC__MAP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "fc_types.h"

/****************************************************************
  Miscellaneous terrain information
*****************************************************************/
#define terrain_misc packet_ruleset_terrain_control

/* Some types used below. */
struct nation_hash;
struct nation_type;
struct packet_edit_startpos_full;
struct startpos;
struct startpos_hash;

enum mapsize_type {
  MAPSIZE_FULLSIZE = 0, /* Using the number of tiles / 1000. */
  MAPSIZE_PLAYER,       /* Define the number of (land) tiles per player;
                         * the setting 'landmass' and the number of players
                         * are used to calculate the map size. */
  MAPSIZE_XYSIZE        /* 'xsize' and 'ysize' are defined. */
};

enum map_generator {
  MAPGEN_SCENARIO = 0,
  MAPGEN_RANDOM,
  MAPGEN_FRACTAL,
  MAPGEN_ISLAND,
  MAPGEN_FAIR,
  MAPGEN_FRACTURE
};

enum map_startpos {
  MAPSTARTPOS_DEFAULT = 0,      /* Generator's choice. */
  MAPSTARTPOS_SINGLE,           /* One player per continent. */
  MAPSTARTPOS_2or3,             /* Two on three players per continent. */
  MAPSTARTPOS_ALL,              /* All players on a single continent. */
  MAPSTARTPOS_VARIABLE,         /* Depending on size of continents. */
};

#define SPECENUM_NAME team_placement
#define SPECENUM_VALUE0 TEAM_PLACEMENT_DISABLED
#define SPECENUM_VALUE1 TEAM_PLACEMENT_CLOSEST
#define SPECENUM_VALUE2 TEAM_PLACEMENT_CONTINENT
#define SPECENUM_VALUE3 TEAM_PLACEMENT_HORIZONTAL
#define SPECENUM_VALUE4 TEAM_PLACEMENT_VERTICAL
#include "specenum_gen.h"

struct civ_map {
  int topology_id;
  enum direction8 valid_dirs[8], cardinal_dirs[8];
  int num_valid_dirs, num_cardinal_dirs;
  struct iter_index *iterate_outwards_indices;
  int num_iterate_outwards_indices;
  int xsize, ysize; /* native dimensions */
  int num_continents;
  int num_oceans;               /* not updated at the client */
  struct tile *tiles;
  struct startpos_hash *startpos_table;

  union {
    struct {
      enum mapsize_type mapsize; /* how the map size is defined */
      int size; /* used to calculate [xy]size */
      int tilesperplayer; /* tiles per player; used to calculate size */
      int seed_setting;
      int seed;
      int riches;
      int huts;
      int huts_absolute; /* For compatibility conversion from pre-2.6 savegames */
      int animals;
      int landpercent;
      enum map_generator generator;
      enum map_startpos startpos;
      bool tinyisles;
      bool separatepoles;
      int flatpoles;
      bool single_pole;
      bool alltemperate;
      int temperature;
      int wetness;
      int steepness;
      bool ocean_resources;         /* Resources in the middle of the ocean */
      bool have_huts;
      bool have_resources;
      enum team_placement team_placement;
    } server;

    /* Add client side when needed */
  };
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__MAP_H */
