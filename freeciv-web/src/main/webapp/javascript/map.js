/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


var map = {};
var tiles = {};

var DIR8_NORTHWEST = 0;
var DIR8_NORTH = 1;
var DIR8_NORTHEAST = 2;
var DIR8_WEST = 3;
var DIR8_EAST = 4;
var DIR8_SOUTHWEST = 5;
var DIR8_SOUTH = 6;
var DIR8_SOUTHEAST = 7;
var DIR8_LAST = 8;
var DIR8_COUNT = DIR8_LAST;

var TF_WRAPX = 1;
var TF_WRAPY = 2;
var TF_ISO = 4;
var TF_HEX = 8;

var T_NONE = 0; /* A special flag meaning no terrain type. */
var T_UNKNOWN = 0; /* An unknown terrain. */

/* The first terrain value. */
var T_FIRST = 0;

/* used to compute neighboring tiles.
 *
 * using
 *   x1 = x + DIR_DX[dir];
 *   y1 = y + DIR_DY[dir];
 * will give you the tile as shown below.
 *   -------
 *   |0|1|2|
 *   |-+-+-|
 *   |3| |4|
 *   |-+-+-|
 *   |5|6|7|
 *   -------
 * Note that you must normalize x1 and y1 yourself.
 */
var DIR_DX = [ -1, 0, 1, -1, 1, -1, 0, 1 ];
var DIR_DY = [ -1, -1, -1, 0, 0, 1, 1, 1 ];


/****************************************************************************
  ...
****************************************************************************/
function topo_has_flag(flag) 
{
  return ((map['topology_id'] & (flag)) != 0);
}

/**************************************************************************
  Allocate space for map, and initialise the tiles.
  Uses current map.xsize and map.ysize.
**************************************************************************/
function map_allocate()
{
  /*freelog(LOG_DEBUG, "map_allocate (was %p) (%d,%d)",
	  (void *)map.tiles, map.xsize, map.ysize);*/

  /*assert(map.tiles == NULL);*/
  tiles = {};

  /* Note this use of whole_map_iterate may be a bit sketchy, since the
   * tile values (ptile->index, etc.) haven't been set yet.  It might be
   * better to do a manual loop here. */
  for (var x = 0; x < map['xsize']; x++) {
    for (var y = 0; y < map['ysize']; y++) {
      tile = {};
      tile['index'] = x + y * map['xsize'];
      tile['x'] = x;
      tile['y'] = y;
      //tile['nat_x'] = tile['index'] % map['xsize'];
      //tile['nat_y'] = Math.floor(tile['index'] / map['xsize']);
      tile = tile_init(tile);

      tiles[tile['index']] = tile;
    }
  }


  /* TODO: generate_city_map_indices(); */
  /* TODO: generate_map_indices(); */

  map['startpos_table'] = {};
  
  init_overview();
}

/****************************************************************************
  ...
****************************************************************************/
function tile_init(tile)
{
  tile['continent'] = 0;
  tile['known'] = null;  /* tile_known in C side */ 
  tile['seen'] = {};     /* tile_seen in C side */
  tile['specials'] = [];
  tile['resource'] = null;
  tile['terrain'] = T_UNKNOWN;
  tile['units'] = [];
  tile['owner'] = null;
  tile['claimer'] = null;
  tile['worked'] = null;
  tile['spec_sprite'] = null;
  return tile;
}

/****************************************************************************
  ...
****************************************************************************/
function map_init_topology(set_sizes)
{
  var dir;

  if (!set_sizes && is_server()) {
    /* skip this, since webclient is a client. */
  }
  
  /* sanity check for iso topologies*/
  /*assert(!MAP_IS_ISOMETRIC || (map.ysize % 2) == 0);*/

  /* The size and ratio must satisfy the minimum and maximum *linear*
   * restrictions on width */
  /*assert(MAP_WIDTH >= MAP_MIN_LINEAR_SIZE);
  assert(MAP_HEIGHT >= MAP_MIN_LINEAR_SIZE);
  assert(MAP_WIDTH <= MAP_MAX_LINEAR_SIZE);
  assert(MAP_HEIGHT <= MAP_MAX_LINEAR_SIZE);*/

  map.valid_dirs = {};
  map.cardinal_dirs = {};

  map.num_valid_dirs = 0;
  map.num_cardinal_dirs = 0;
  for (var dir = 0; dir < 8; dir++) {
    if (is_valid_dir(dir)) {
      map.valid_dirs[map.num_valid_dirs] = dir;
      map.num_valid_dirs++;
    }
    if (is_cardinal_dir(dir)) {
      map.cardinal_dirs[map.num_cardinal_dirs] = dir;
      map.num_cardinal_dirs++;
    }
  }
  /*assert(map.num_valid_dirs > 0 && map.num_valid_dirs <= 8);
  assert(map.num_cardinal_dirs > 0
	 && map.num_cardinal_dirs <= map.num_valid_dirs);*/

}

/****************************************************************************
  ...
****************************************************************************/
function is_valid_dir(dir)
{
  switch (dir) {
  case DIR8_SOUTHEAST:
  case DIR8_NORTHWEST:
    /* These directions are invalid in hex topologies. */
    return !(topo_has_flag(TF_HEX) && !topo_has_flag(TF_ISO));
  case DIR8_NORTHEAST:
  case DIR8_SOUTHWEST:
    /* These directions are invalid in iso-hex topologies. */
    return !(topo_has_flag(TF_HEX) && topo_has_flag(TF_ISO));
  case DIR8_NORTH:
  case DIR8_EAST:
  case DIR8_SOUTH:
  case DIR8_WEST:
    return true;
  default:
    return false;
  }
}

/****************************************************************************
  ...
****************************************************************************/
function is_cardinal_dir(dir)
{
  switch (dir) {
  case DIR8_NORTH:
  case DIR8_SOUTH:
  case DIR8_EAST:
  case DIR8_WEST:
    return true;
  case DIR8_SOUTHEAST:
  case DIR8_NORTHWEST:
    /* These directions are cardinal in iso-hex topologies. */
    return topo_has_flag(TF_HEX) && topo_has_flag(TF_ISO);
  case DIR8_NORTHEAST:
  case DIR8_SOUTHWEST:
    /* These directions are cardinal in hexagonal topologies. */
    return topo_has_flag(TF_HEX) && !topo_has_flag(TF_ISO);
  }
  return false;
}

/****************************************************************************
  Return the tile for the given cartesian (map) position.
****************************************************************************/
function map_pos_to_tile(x, y)
{
  return tiles[x + y * map['xsize']];
}

/****************************************************************************
  Return the tile for the given index.
****************************************************************************/
function index_to_tile(index)
{
  return tiles[index];
}

/****************************************************************************
  Obscure math.  See explanation in doc/HACKING. 
****************************************************************************/
function NATIVE_TO_MAP_POS(nat_x, nat_y)                     
{  							    
    var pmap_x = Math.floor(((nat_y) + ((nat_y) & 1)) / 2 + (nat_x));
    var pmap_y = Math.floor(nat_y - pmap_x + map['xsize']);
    return {'map_x' : pmap_x, 'map_y' : pmap_y};                           
}

/****************************************************************************
  ...
****************************************************************************/
function MAP_TO_NATIVE_POS(map_x, map_y)                     
{   
   var pnat_y = Math.floor(map_x + map_y - map['xsize']);
   var pnat_x = Math.floor((2 * map_x - pnat_y - (pnat_y & 1)) / 2);
   return {'nat_y': pnat_y, 'nat_x': pnat_x};          
}  

/****************************************************************************
  ...
****************************************************************************/
function NATURAL_TO_MAP_POS(nat_x, nat_y)                    
{   
   var pmap_x = Math.floor(((nat_y) + (nat_x)) / 2);                                  
   var pmap_y = Math.floor(nat_y - pmap_x + map['xsize']);
   return {'map_x' : pmap_x, 'map_y' : pmap_y};                          
}  

/****************************************************************************
  ...
****************************************************************************/
function MAP_TO_NATURAL_POS(map_x, map_y)                    
{							    
    var pnat_y = Math.floor((map_x) + (map_y) - map['xsize']);                            
    var pnat_x = Math.floor(2 * (map_x) - pnat_y);
    
    return {'nat_y' : pnat_y, 'nat_x' : pnat_x};                                  
}

/****************************************************************************
  Step from the given tile in the given direction.  The new tile is returned,
  or NULL if the direction is invalid or leads off the map.
****************************************************************************/
function mapstep(ptile, dir)
{
  var x, y;
  
  if (!is_valid_dir(dir)) {
    return null;
  }

  var s = DIRSTEP(x, y, dir);
  x = s['dest_x'] + ptile['x'];
  y = s['dest_y'] + ptile['y'];

  return map_pos_to_tile(x, y);
}

/****************************************************************************
  ...
****************************************************************************/
function DIRSTEP(dest_x, dest_y, dir)	
{
    return { "dest_x" : DIR_DX[(dir)], "dest_y" : DIR_DY[(dir)]};
}



/**************************************************************************
Return the debugging name of the direction.
**************************************************************************/
function dir_get_name(dir)
{
  /* a switch statement is used so the ordering can be changed easily */
  switch (dir) {
  case DIR8_NORTH:
    return "N";
  case DIR8_NORTHEAST:
    return "NE";
  case DIR8_EAST:
    return "E";
  case DIR8_SOUTHEAST:
    return "SE";
  case DIR8_SOUTH:
    return "S";
  case DIR8_SOUTHWEST:
    return "SW";
  case DIR8_WEST:
    return "W";
  case DIR8_NORTHWEST:
    return "NW";
  };
  
  return "[Undef]";
}

/**************************************************************************
  Returns the next direction clock-wise.
**************************************************************************/
function dir_cw(dir)
{
  /* a switch statement is used so the ordering can be changed easily */
  switch (dir) {
  case DIR8_NORTH:
    return DIR8_NORTHEAST;
  case DIR8_NORTHEAST:
    return DIR8_EAST;
  case DIR8_EAST:
    return DIR8_SOUTHEAST;
  case DIR8_SOUTHEAST:
    return DIR8_SOUTH;
  case DIR8_SOUTH:
    return DIR8_SOUTHWEST;
  case DIR8_SOUTHWEST:
    return DIR8_WEST;
  case DIR8_WEST:
    return DIR8_NORTHWEST;
  case DIR8_NORTHWEST:
    return DIR8_NORTH;
  };
  
  return -1;
}

/**************************************************************************
  Returns the next direction counter-clock-wise.
**************************************************************************/
function dir_ccw(dir)
{
  /* a switch statement is used so the ordering can be changed easily */
  switch (dir) {
  case DIR8_NORTH:
    return DIR8_NORTHWEST;
  case DIR8_NORTHEAST:
    return DIR8_NORTH;
  case DIR8_EAST:
    return DIR8_NORTHEAST;
  case DIR8_SOUTHEAST:
    return DIR8_EAST;
  case DIR8_SOUTH:
    return DIR8_SOUTHEAST;
  case DIR8_SOUTHWEST:
    return DIR8_SOUTH;
  case DIR8_WEST:
    return DIR8_SOUTHWEST;
  case DIR8_NORTHWEST:
    return DIR8_WEST;
  }
  return -1;
}
