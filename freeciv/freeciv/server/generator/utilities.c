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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* utilities */
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "shared.h"		/* bool type */

/* common */
#include "map.h"
#include "packets.h"

#include "utilities.h"

/****************************************************************************
 Map that contains, according to circumstances, information on whether
 we have already placed terrain (special, hut) here.
****************************************************************************/
static bool *placed_map;

/**************************************************************************
 return TRUE if initialized
*************************************************************************/ 
bool placed_map_is_initialized(void)
{
  return placed_map != NULL;
}

/****************************************************************************
  Create a clean pmap
****************************************************************************/
void create_placed_map(void)                               
{                                                          
  assert(!placed_map_is_initialized());                              
  placed_map = fc_malloc (sizeof(bool) * MAP_INDEX_SIZE);   
  INITIALIZE_ARRAY(placed_map, MAP_INDEX_SIZE, FALSE );     
}

/**************************************************************************** 
  Free the pmap
****************************************************************************/
void destroy_placed_map(void)   
{                              
  assert(placed_map_is_initialized()); 
  free(placed_map);            
  placed_map = NULL;           
}



#define pmap(_tile) (placed_map[tile_index(_tile)])

/**************************************************************************
  Checks if land has not yet been placed on pmap at (x, y)
**************************************************************************/
bool not_placed(const struct tile *ptile)
{
  return !pmap(ptile);
}

/**************************************************************************
  Mark tile terrain as placed.
**************************************************************************/
void map_set_placed(struct tile *ptile)
{
  pmap(ptile) = TRUE;
}

/**************************************************************************
  Mark tile terrain as not placed.
**************************************************************************/
void map_unset_placed(struct tile *ptile)
{
  pmap(ptile) = FALSE;
}

/**************************************************************************** 
  set all oceanics tiles in placed_map
****************************************************************************/
void set_all_ocean_tiles_placed(void) 
{
  whole_map_iterate(ptile) {
    if (is_ocean_tile(ptile)) {
      map_set_placed(ptile);
    }
  } whole_map_iterate_end;
}

/****************************************************************************
  Set all nearby tiles as placed on pmap. 
****************************************************************************/
void set_placed_near_pos(struct tile *ptile, int dist)
{
  square_iterate(ptile, dist, tile1) {
    map_set_placed(tile1);
  } square_iterate_end;
}

/**************************************************************************
  Change the values of the integer map, so that they contain ranking of each 
  tile scaled to [0 .. int_map_max].
  The lowest 20% of tiles will have values lower than 0.2 * int_map_max.

  If filter is non-null then it only tiles for which filter(ptile, data) is
  TRUE will be considered.
**************************************************************************/
void adjust_int_map_filtered(int *int_map, int int_map_max, void *data,
			     bool (*filter)(const struct tile *ptile,
					    const void *data))
{
  int minval = 0, maxval = 0, total = 0;
  bool first = TRUE;

  /* Determine minimum and maximum value. */
  whole_map_iterate_filtered(ptile, data, filter) {
    if (first) {
      minval = int_map[tile_index(ptile)];
      maxval = int_map[tile_index(ptile)];
    } else {
      maxval = MAX(maxval, int_map[tile_index(ptile)]);
      minval = MIN(minval, int_map[tile_index(ptile)]);
    }
    first = FALSE;
    total++;
  } whole_map_iterate_filtered_end;

  if (total == 0) {
    return;
  }

  {
    int const size = 1 + maxval - minval;
    int i, count = 0, frequencies[size];

    INITIALIZE_ARRAY(frequencies, size, 0);

    /* Translate value so the minimum value is 0
       and count the number of occurencies of all values to initialize the 
       frequencies[] */
    whole_map_iterate_filtered(ptile, data, filter) {
      int_map[tile_index(ptile)] -= minval;
      frequencies[int_map[tile_index(ptile)]]++;
    } whole_map_iterate_filtered_end;

    /* create the linearize function as "incremental" frequencies */
    for(i =  0; i < size; i++) {
      count += frequencies[i]; 
      frequencies[i] = (count * int_map_max) / total;
    }

    /* apply the linearize function */
    whole_map_iterate_filtered(ptile, data, filter) {
      int_map[tile_index(ptile)] = frequencies[int_map[tile_index(ptile)]];
    } whole_map_iterate_filtered_end;
  }
}

bool is_normal_nat_pos(int x, int y)
{
  NATIVE_TO_MAP_POS(&x, &y, x, y);
  return is_normal_map_pos(x, y);
}

/****************************************************************************
 * Apply a Gaussian difusion filtre on the map
 * the size of the map is MAP_INDEX_SIZE and the map is indexed by 
 * native_pos_to_index function
 * if zeroes_at_edges is set, any unreal position on difusion has 0 value
 * if zeroes_at_edges in unset the unreal position are not counted.
 ****************************************************************************/
 void smooth_int_map(int *int_map, bool zeroes_at_edges)
{
  float weight[5] =  {0.35,  0.5 ,1 , 0.5, 0.35};
  float total_weight = 2.70;
  bool axe = TRUE;
  int alt_int_map[MAP_INDEX_SIZE];
  int *target_map, *source_map;

  assert(int_map != NULL);

  target_map = alt_int_map;
  source_map = int_map;

  do {
    whole_map_iterate(ptile) {
      int  N = 0, D = 0;

      axis_iterate(ptile, pnear, i, 2, axe) {
	D += weight[i + 2];
	N += weight[i + 2] * source_map[tile_index(pnear)];
      } axis_iterate_end;
      if(zeroes_at_edges) {
	D = total_weight;
      }
      target_map[tile_index(ptile)] = N / D;
    } whole_map_iterate_end;

    if (MAP_IS_ISOMETRIC) {
      weight[0] = weight[4] = 0.5;
      weight[1] = weight[3] = 0.7;
      total_weight = 3.4;  
    }

    axe = !axe;

    source_map = alt_int_map;
    target_map = int_map;

  } while (!axe);
}

/* These arrays are indexed by continent number (or negative of the
 * ocean number) so the 0th element is unused and the array is 1 element
 * larger than you'd expect.
 *
 * The lake surrounders array tells how many land continents surround each
 * ocean (or -1 if the ocean touches more than one continent).
 *
 * The _sizes arrays give the sizes (in tiles) of each continent and
 * ocean.
 */
static Continent_id *lake_surrounders;
static int *continent_sizes, *ocean_sizes;


/**************************************************************************
  Calculate lake_surrounders[] array
**************************************************************************/
static void recalculate_lake_surrounders(void)
{
  const size_t size = (map.num_oceans + 1) * sizeof(*lake_surrounders);

  lake_surrounders = fc_realloc(lake_surrounders, size);
  memset(lake_surrounders, 0, size);
  
  whole_map_iterate(ptile) {
    const struct terrain *pterrain = tile_terrain(ptile);
    Continent_id cont = tile_continent(ptile);

    if (T_UNKNOWN == pterrain) {
      continue;
    }
    if (!terrain_has_flag(pterrain, TER_OCEANIC)) {
      adjc_iterate(ptile, tile2) {
        Continent_id cont2 = tile_continent(tile2);
	if (is_ocean_tile(tile2)) {
	  if (lake_surrounders[-cont2] == 0) {
	    lake_surrounders[-cont2] = cont;
	  } else if (lake_surrounders[-cont2] != cont) {
	    lake_surrounders[-cont2] = -1;
	  }
	}
      } adjc_iterate_end;
    }
  } whole_map_iterate_end;
}

/**************************************************************************
  Number this tile and nearby tiles (recursively) with the specified
  continent number nr, using a flood-fill algorithm.

  is_land tells us whether we are assigning continent numbers or ocean 
  numbers.
**************************************************************************/
static void assign_continent_flood(struct tile *ptile, bool is_land, int nr)
{
  const struct terrain *pterrain = tile_terrain(ptile);

  if (tile_continent(ptile) != 0) {
    return;
  }

  if (T_UNKNOWN == pterrain) {
    return;
  }

  if (!XOR(is_land, terrain_has_flag(pterrain, TER_OCEANIC))) {
    return;
  }

  tile_set_continent(ptile, nr);
  
  /* count the tile */
  if (nr < 0) {
    ocean_sizes[-nr]++;
  } else {
    continent_sizes[nr]++;
  }

  adjc_iterate(ptile, tile1) {
    assign_continent_flood(tile1, is_land, nr);
  } adjc_iterate_end;
}

/**************************************************************************
  Regenerate all oceanic tiles with coasts, lakes, and deeper oceans.
  Assumes assign_continent_numbers() and recalculate_lake_surrounders()
  have already been done!
  FIXME: insufficiently generalized, use terrain property.
  FIXME: Results differ from initially generated waters, but this is not
         used at all in normal map generation.
**************************************************************************/
void regenerate_lakes(tile_knowledge_cb knowledge_cb)
{
#define MAX_ALT_TER_TYPES 5
#define DEFAULT_NEAR_COAST (6)
  struct terrain *lakes[MAX_ALT_TER_TYPES];
  int num_laketypes;

  num_laketypes = terrains_by_flag(TER_FRESHWATER, lakes, sizeof(lakes));
  if (num_laketypes > MAX_ALT_TER_TYPES) {
    freelog(LOG_NORMAL, "Number of lake types in ruleset %d, considering only %d ones.",
            num_laketypes, MAX_ALT_TER_TYPES);
    num_laketypes = MAX_ALT_TER_TYPES;
  }

#undef MAX_ALT_TER_TYPES

  if (num_laketypes > 0) {
    /* Lakes */
    whole_map_iterate(ptile) {
      struct terrain *pterrain = tile_terrain(ptile);
      Continent_id here = tile_continent(ptile);

      if (T_UNKNOWN == pterrain) {
        continue;
      }
      if (!terrain_has_flag(pterrain, TER_OCEANIC)) {
        continue;
      }
      if (0 < lake_surrounders[-here]) {
        if (terrain_control.lake_max_size >= ocean_sizes[-here]) {
          tile_change_terrain(ptile, lakes[myrand(num_laketypes)]);
        }
        if (knowledge_cb) {
          knowledge_cb(ptile);
        }
        continue;
      }
    } whole_map_iterate_end;
  }
}

/**************************************************************************
  Get continent surrounding lake, or -1 if there is multiple continents.
**************************************************************************/
int get_lake_surrounders(Continent_id cont)
{
  return lake_surrounders[-cont];
}

/*************************************************************************
  Return size in tiles of the given continent(not ocean)
*************************************************************************/
int get_continent_size(Continent_id id)
{
  assert(id > 0);
  return continent_sizes[id];
}

/*************************************************************************
  Return size in tiles of the given ocean. You should use positive ocean
  number.
*************************************************************************/
int get_ocean_size(Continent_id id) 
{
  assert(id > 0);
  return ocean_sizes[id];
}

/**************************************************************************
  Assigns continent and ocean numbers to all tiles, and set
  map.num_continents and map.num_oceans.  Recalculates continent and
  ocean sizes, and lake_surrounders[] arrays.

  Continents have numbers 1 to map.num_continents _inclusive_.
  Oceans have (negative) numbers -1 to -map.num_oceans _inclusive_.
**************************************************************************/
void assign_continent_numbers(void)
{
  /* Initialize */
  map.num_continents = 0;
  map.num_oceans = 0;

  whole_map_iterate(ptile) {
    tile_set_continent(ptile, 0);
  } whole_map_iterate_end;

  /* Assign new numbers */
  whole_map_iterate(ptile) {
    const struct terrain *pterrain = tile_terrain(ptile);

    if (tile_continent(ptile) != 0) {
      /* Already assigned. */
      continue;
    }

    if (T_UNKNOWN == pterrain) {
      continue; /* Can't assign this. */
    }

    if (!terrain_has_flag(pterrain, TER_OCEANIC)) {
      map.num_continents++;
      continent_sizes = fc_realloc(continent_sizes,
		       (map.num_continents + 1) * sizeof(*continent_sizes));
      continent_sizes[map.num_continents] = 0;
      assign_continent_flood(ptile, TRUE, map.num_continents);
    } else {
      map.num_oceans++;
      ocean_sizes = fc_realloc(ocean_sizes,
		       (map.num_oceans + 1) * sizeof(*ocean_sizes));
      ocean_sizes[map.num_oceans] = 0;
      assign_continent_flood(ptile, FALSE, -map.num_oceans);
    }
  } whole_map_iterate_end;

  recalculate_lake_surrounders();

  freelog(LOG_VERBOSE, "Map has %d continents and %d oceans", 
	  map.num_continents, map.num_oceans);
}

/**************************************************************************
  Return most shallow ocean terrain type. Freshwater lakes are not
  considered, if there is any salt water terrain types.
**************************************************************************/
struct terrain *most_shallow_ocean(void)
{
  bool oceans = FALSE;
  struct terrain *shallow = NULL;

  terrain_type_iterate(pterr) {
    if (is_ocean(pterr)) {
      if (!oceans && !terrain_has_flag(pterr, TER_FRESHWATER)) {
        /* First ocean type */
        oceans = TRUE;
        shallow = pterr;
      } else if (!shallow
                 || pterr->property[MG_OCEAN_DEPTH] <
                    shallow->property[MG_OCEAN_DEPTH]) {
        shallow = pterr;
      }
    }
  } terrain_type_iterate_end;

  return shallow;
}

/**************************************************************************
  Picks an ocean terrain to match the given depth.
  Return NULL when there is no available ocean.
**************************************************************************/
struct terrain *pick_ocean(int depth)
{
  struct terrain *best_terrain = NULL;
  int best_match = TERRAIN_OCEAN_DEPTH_MAXIMUM;

  terrain_type_iterate(pterrain) {
    if (terrain_has_flag(pterrain, TER_OCEANIC)
      &&  TERRAIN_OCEAN_DEPTH_MINIMUM <= pterrain->property[MG_OCEAN_DEPTH]) {
      int match = abs(depth - pterrain->property[MG_OCEAN_DEPTH]);

      if (best_match > match) {
	best_match = match;
	best_terrain = pterrain;
      }
    }
  } terrain_type_iterate_end;

  return best_terrain;
}

/**************************************************************************
  Makes a simple depth map for all ocean tiles based on their proximity
  to any land tiles and reassignes ocean terrain types based on their
  MG_OCEAN_DEPTH property values.

  This is used by the island generator to regenerate shallow ocean areas
  near the coast.

  FIXME: Make the generated shallow areas more interesting, and take into
  account map parameters. Remove the need for this function by making
  the generator automatically create shallow ocean areas when islands
  are created.
**************************************************************************/
void smooth_water_depth(void)
{
  struct terrain *ocean_type;
  int num_ocean_types = 0, depth, i;
  int *dmap;
  const int dmap_max = 100;
  const int ocean_max = TERRAIN_OCEAN_DEPTH_MAXIMUM;
  const int ocean_min = TERRAIN_OCEAN_DEPTH_MINIMUM;
  const int ocean_span = ocean_max - ocean_min;

  /* Approximately controls how far out the shallow areas will go. */
  const int spread = 2;

  terrain_type_iterate(pterrain) {
    if (pterrain->property[MG_OCEAN_DEPTH] > 0) {
      num_ocean_types++;
    }
  } terrain_type_iterate_end;

  if (num_ocean_types < 2) {
    return;
  }

  dmap = fc_malloc(MAP_INDEX_SIZE * sizeof(int));

  whole_map_iterate(ptile) {
    /* The depth values are reversed so that the diffusion
     * filter causes land to "flow" out into the ocean. */
    dmap[tile_index(ptile)] = is_ocean_tile(ptile) ? 0 : dmap_max;
  } whole_map_iterate_end;

  for (i = 0; i < spread; i++) {
    /* Use the gaussian diffusion filter to "spread"
     * the height of the land into the ocean. */
    smooth_int_map(dmap, TRUE);
  }

  whole_map_iterate(ptile) {
    if (!is_ocean_tile(ptile)) {
      continue;
    }

    depth = dmap[tile_index(ptile)];

    /* Reverse the diffusion filter hack. */
    depth = dmap_max - depth;

    /* Scale the depth value from the interval [0, dmap_max]
     * to [ocean_min, ocean_max]. */
    depth = ocean_min + ocean_span * depth / dmap_max;

    /* Make sure that depth value is something that the
     * function pick_ocean can understand. */
    depth = CLIP(ocean_min, depth, ocean_max);

    ocean_type = pick_ocean(depth);
    if (ocean_type) {
      tile_set_terrain(ptile, ocean_type);
    }
  } whole_map_iterate_end;

  free(dmap);
}
