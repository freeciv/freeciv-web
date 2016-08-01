/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

/* Amplio.tilespec ported to Javascript. */


var tileset_tile_width = 96;
var tileset_tile_height = 48;

var tileset_options = "+tilespec4+2007.Oct.26";

// A simple name for the tileset specified by this file:
var tileset_name = "amplio2";
var priority = 20;

var tileset_image_count = 3;

var normal_tile_width  = 96;
var normal_tile_height = 48;
var small_tile_width   = 15;
var small_tile_height  = 20;

var is_hex = 0;
var is_isometric = 1;


// Do not blend hills and mountains together.
var is_mountainous = 0;

// Use roadstyle 0 (old iso style)
var roadstyle = 0;

// Fogstyle 2, darkness_style 4 : blended fog
var fogstyle = 2;
var darkness_style = 4;

// offset the flags by this amount when drawing units
var unit_flag_offset_x = 25;
var unit_flag_offset_y = 16;
var city_flag_offset_x = 2;
var city_flag_offset_y = 9;

var city_size_offset_x = 0;
var city_size_offset_y = 20;

var unit_activity_offset_x = 55;
var unit_activity_offset_y = 25;

// offset the units by this amount when drawing units
var unit_offset_x = 19;
var unit_offset_y = 14;

// Enable citybar
var is_full_citybar = 1;

// offset the citybar text by this amount (from the city tile origin)
var citybar_offset_y = 55;
var citybar_offset_x = 45;

// offset the tile label by this amount (from the city tile origin)
var tilelabel_offset_y = 15;
var tilelabel_offset_x = 0;

// Font size (points) to use to draw city names and productions:
var city_names_font_size = 10;
var city_productions_font_size = 8;

var dither_offset_x = [normal_tile_width/2, 0, normal_tile_width/2, 0];
var dither_offset_y = [0, normal_tile_height/2, normal_tile_height/2, 0];

var ts_layer = [];

//[layer0]
ts_layer[0] = {};
ts_layer[0]['match_types'] = ["shallow", "deep", "land"];

//[layer1]
ts_layer[1] = {};
ts_layer[1]['match_types'] = ["forest", "hills", "mountains", "water", "ice", "jungle"];

//[layer2]
ts_layer[2] = {};
ts_layer[2]['match_types'] = ["water", "ice"];

// Water graphics referenced by terrain.ruleset

var ts_tiles = {};


ts_tiles['lake'] = {};
ts_tiles['lake']['is_blended'] = 0;
ts_tiles['lake']['num_layers'] = 1;
ts_tiles['lake']['layer0_match_type'] = "shallow";
ts_tiles['lake']['layer0_match_with'] = ["land"];
ts_tiles['lake']['layer0_sprite_type'] = "corner";

ts_tiles['coast'] = {};
ts_tiles['coast']['is_blended'] = 1;
ts_tiles['coast']['num_layers'] = 2;
ts_tiles['coast']['layer0_match_type'] = "shallow";
ts_tiles['coast']['layer0_match_with'] = ["deep", "land"];
ts_tiles['coast']['layer0_sprite_type'] = "corner";
ts_tiles['coast']['layer1_match_type'] = "water";
ts_tiles['coast']['layer1_match_with'] = ["ice"];
ts_tiles['coast']['layer1_sprite_type'] = "corner";

ts_tiles['floor'] = {};
ts_tiles['floor']['is_blended'] = 0;
ts_tiles['floor']['num_layers'] = 2;
ts_tiles['floor']['layer0_match_type'] = "deep";
ts_tiles['floor']['layer0_match_with'] = ["shallow", "land"];
ts_tiles['floor']['layer0_sprite_type'] = "corner";
ts_tiles['floor']['layer1_match_type'] = "water";
ts_tiles['floor']['layer1_match_with'] = ["ice"];
ts_tiles['floor']['layer1_sprite_type'] = "corner";

// Land graphics referenced by terrain.ruleset

ts_tiles['arctic'] = {};
//; treated as water for ice cliffs
ts_tiles['arctic']['is_blended'] = 0;
ts_tiles['arctic']['num_layers'] = 3;
ts_tiles['arctic']['layer0_match_type'] = "shallow";
ts_tiles['arctic']['layer1_match_type'] = "ice";
ts_tiles['arctic']['layer2_match_type'] = "ice";
ts_tiles['arctic']['mine_sprite'] = "tx.oil_mine";

ts_tiles['desert'] = {};
ts_tiles['desert']['is_blended'] = 1;
ts_tiles['desert']['num_layers'] = 1;
ts_tiles['desert']['layer0_match_type'] = "land";
ts_tiles['desert']['mine_sprite'] = "tx.oil_mine";

ts_tiles['forest'] = {};
ts_tiles['forest']['is_blended'] = 1;
ts_tiles['forest']['num_layers'] = 2;
ts_tiles['forest']['layer0_match_type'] = "land";
ts_tiles['forest']['layer1_match_type'] = "forest";
ts_tiles['forest']['layer1_match_with'] = ["forest"];

ts_tiles['grassland'] = {};
ts_tiles['grassland']['is_blended'] = 1;
ts_tiles['grassland']['num_layers'] = 1;
ts_tiles['grassland']['layer0_match_type'] = "land";

ts_tiles['hills'] = {};
ts_tiles['hills']['is_blended'] = 1;
ts_tiles['hills']['num_layers'] = 2;
ts_tiles['hills']['layer0_match_type'] = "land";
ts_tiles['hills']['layer1_match_type'] = "hills";
ts_tiles['hills']['layer1_match_with'] = ["hills"];
ts_tiles['hills']['mine_sprite'] = "tx.mine";

ts_tiles['jungle'] = {};
ts_tiles['jungle']['is_blended'] = 1;
ts_tiles['jungle']['num_layers'] = 2;
ts_tiles['jungle']['layer0_match_type'] = "land";
ts_tiles['jungle']['layer1_match_type'] = "jungle";
ts_tiles['jungle']['layer1_match_with'] = ["jungle"];

ts_tiles['mountains'] = {};
ts_tiles['mountains']['is_blended'] = 1;
ts_tiles['mountains']['num_layers'] = 2;
ts_tiles['mountains']['layer0_match_type'] = "land";
ts_tiles['mountains']['layer1_match_type'] = "mountains";
ts_tiles['mountains']['layer1_match_with'] = ["mountains"];
ts_tiles['mountains']['mine_sprite'] = "tx.mine";

ts_tiles['plains'] = {};
ts_tiles['plains']['is_blended'] = 1;
ts_tiles['plains']['num_layers'] = 1;
ts_tiles['plains']['layer0_match_type'] = "land";

ts_tiles['swamp'] = {};
ts_tiles['swamp']['is_blended'] = 1;
ts_tiles['swamp']['num_layers'] = 1;
ts_tiles['swamp']['layer0_match_type'] = "land";

ts_tiles['tundra'] = {};
ts_tiles['tundra']['is_blended'] = 1;
ts_tiles['tundra']['num_layers'] = 1;
ts_tiles['tundra']['layer0_match_type'] = "land";

ts_tiles['inaccessible'] = {};
ts_tiles['inaccessible']['is_blended'] = 0;
ts_tiles['inaccessible']['num_layers'] = 1;
ts_tiles['inaccessible']['layer0_match_type'] = "land";




var tile_types_setup =
{
"l0.lake":	{"match_style":MATCH_PAIR,"sprite_type":CELL_CORNER,"mine_tag":"(null)","match_indices":2,"match_index":[0,2],"dither":false},
"l0.coast":	{"match_style":MATCH_FULL,"sprite_type":CELL_CORNER,"mine_tag":"(null)","match_indices":3,"match_index":[0,1,2],"dither":false},
"l1.coast":	{"match_style":MATCH_PAIR,"sprite_type":CELL_CORNER,"mine_tag":"(null)","match_indices":2,"match_index":[3,4],"dither":false},
"l0.floor":	{"match_style":MATCH_FULL,"sprite_type":CELL_CORNER,"mine_tag":"(null)","match_indices":3,"match_index":[1,0,2],"dither":false},
"l1.floor":	{"match_style":MATCH_PAIR,"sprite_type":CELL_CORNER,"mine_tag":"(null)","match_indices":2,"match_index":[3,4],"dither":false},
"l0.arctic":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"tx.oil_mine","match_indices":1,"match_index":[0],"dither":false},
"l0.desert":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"tx.oil_mine","match_indices":1,"match_index":[2],"dither":true},
"l0.forest":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":1,"match_index":[2],"dither":true},
"l1.forest":	{"match_style":MATCH_SAME,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":2,"match_index":[0,0],"dither":false},
"l0.grassland":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":1,"match_index":[2],"dither":true},
"l0.hills":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"tx.mine","match_indices":1,"match_index":[2],"dither":true},
"l1.hills":	{"match_style":MATCH_SAME,"sprite_type":CELL_WHOLE,"mine_tag":"tx.mine","match_indices":2,"match_index":[1,1],"dither":false},
"l0.jungle":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":1,"match_index":[5],"dither":true},
"l1.jungle":	{"match_style":MATCH_SAME,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":2,"match_index":[5,5],"dither":false},
"l0.mountains":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"tx.mine","match_indices":1,"match_index":[2],"dither":true},
"l1.mountains":	{"match_style":MATCH_SAME,"sprite_type":CELL_WHOLE,"mine_tag":"tx.mine","match_indices":2,"match_index":[2,2],"dither":false},
"l0.plains":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":1,"match_index":[2],"dither":true},
"l0.swamp":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":1,"match_index":[2],"dither":true},
"l0.tundra":	{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":1,"match_index":[2],"dither":true},
"l0.inaccessible":{"match_style":MATCH_NONE,"sprite_type":CELL_WHOLE,"mine_tag":"(null)","match_indices":1,"match_index":[2],"dither":false}

};



var cellgroup_map =
{"coast.0":"t.l0.cellgroup_s_s_s_s",
"coast.1":"t.l0.cellgroup_s_s_s_s",
"coast.2":"t.l0.cellgroup_s_s_s_s",
"coast.3":"t.l0.cellgroup_s_s_s_s",
"coast.4":"t.l0.cellgroup_s_s_s_d",
"coast.5":"t.l0.cellgroup_s_d_s_s",
"coast.6":"t.l0.cellgroup_d_s_s_s",
"coast.7":"t.l0.cellgroup_s_s_d_s",
"coast.8":"t.l0.cellgroup_s_s_s_l",
"coast.9":"t.l0.cellgroup_s_l_s_s",
"coast.10":"t.l0.cellgroup_l_s_s_s",
"coast.11":"t.l0.cellgroup_s_s_l_s",
"coast.12":"t.l0.cellgroup_d_s_s_s",
"coast.13":"t.l0.cellgroup_s_s_d_s",
"coast.14":"t.l0.cellgroup_s_d_s_s",
"coast.15":"t.l0.cellgroup_s_s_s_d",
"coast.16":"t.l0.cellgroup_d_s_s_d",
"coast.17":"t.l0.cellgroup_s_d_d_s",
"coast.18":"t.l0.cellgroup_d_d_s_s",
"coast.19":"t.l0.cellgroup_s_s_d_d",
"coast.20":"t.l0.cellgroup_d_s_s_l",
"coast.21":"t.l0.cellgroup_s_l_d_s",
"coast.22":"t.l0.cellgroup_l_d_s_s",
"coast.23":"t.l0.cellgroup_s_s_l_d",
"coast.24":"t.l0.cellgroup_l_s_s_s",
"coast.25":"t.l0.cellgroup_s_s_l_s",
"coast.26":"t.l0.cellgroup_s_l_s_s",
"coast.27":"t.l0.cellgroup_s_s_s_l",
"coast.28":"t.l0.cellgroup_l_s_s_d",
"coast.29":"t.l0.cellgroup_s_d_l_s",
"coast.30":"t.l0.cellgroup_d_l_s_s",
"coast.31":"t.l0.cellgroup_s_s_d_l",
"coast.32":"t.l0.cellgroup_l_s_s_l",
"coast.33":"t.l0.cellgroup_s_l_l_s",
"coast.34":"t.l0.cellgroup_l_l_s_s",
"coast.35":"t.l0.cellgroup_s_s_l_l",
"coast.36":"t.l0.cellgroup_s_d_s_s",
"coast.37":"t.l0.cellgroup_s_s_s_d",
"coast.38":"t.l0.cellgroup_s_s_d_s",
"coast.39":"t.l0.cellgroup_d_s_s_s",
"coast.40":"t.l0.cellgroup_s_d_s_d",
"coast.41":"t.l0.cellgroup_s_d_s_d",
"coast.42":"t.l0.cellgroup_d_s_d_s",
"coast.43":"t.l0.cellgroup_d_s_d_s",
"coast.44":"t.l0.cellgroup_s_d_s_l",
"coast.45":"t.l0.cellgroup_s_l_s_d",
"coast.46":"t.l0.cellgroup_l_s_d_s",
"coast.47":"t.l0.cellgroup_d_s_l_s",
"coast.48":"t.l0.cellgroup_d_d_s_s",
"coast.49":"t.l0.cellgroup_s_s_d_d",
"coast.50":"t.l0.cellgroup_s_d_d_s",
"coast.51":"t.l0.cellgroup_d_s_s_d",
"coast.52":"t.l0.cellgroup_d_d_s_d",
"coast.53":"t.l0.cellgroup_s_d_d_d",
"coast.54":"t.l0.cellgroup_d_d_d_s",
"coast.55":"t.l0.cellgroup_d_s_d_d",
"coast.56":"t.l0.cellgroup_d_d_s_l",
"coast.57":"t.l0.cellgroup_s_l_d_d",
"coast.58":"t.l0.cellgroup_l_d_d_s",
"coast.59":"t.l0.cellgroup_d_s_l_d",
"coast.60":"t.l0.cellgroup_l_d_s_s",
"coast.61":"t.l0.cellgroup_s_s_l_d",
"coast.62":"t.l0.cellgroup_s_l_d_s",
"coast.63":"t.l0.cellgroup_d_s_s_l",
"coast.64":"t.l0.cellgroup_l_d_s_d",
"coast.65":"t.l0.cellgroup_s_d_l_d",
"coast.66":"t.l0.cellgroup_d_l_d_s",
"coast.67":"t.l0.cellgroup_d_s_d_l",
"coast.68":"t.l0.cellgroup_l_d_s_l",
"coast.69":"t.l0.cellgroup_s_l_l_d",
"coast.70":"t.l0.cellgroup_l_l_d_s",
"coast.71":"t.l0.cellgroup_d_s_l_l",
"coast.72":"t.l0.cellgroup_s_l_s_s",
"coast.73":"t.l0.cellgroup_s_s_s_l",
"coast.74":"t.l0.cellgroup_s_s_l_s",
"coast.75":"t.l0.cellgroup_l_s_s_s",
"coast.76":"t.l0.cellgroup_s_l_s_d",
"coast.77":"t.l0.cellgroup_s_d_s_l",
"coast.78":"t.l0.cellgroup_d_s_l_s",
"coast.79":"t.l0.cellgroup_l_s_d_s",
"coast.80":"t.l0.cellgroup_s_l_s_l",
"coast.81":"t.l0.cellgroup_s_l_s_l",
"coast.82":"t.l0.cellgroup_l_s_l_s",
"coast.83":"t.l0.cellgroup_l_s_l_s",
"coast.84":"t.l0.cellgroup_d_l_s_s",
"coast.85":"t.l0.cellgroup_s_s_d_l",
"coast.86":"t.l0.cellgroup_s_d_l_s",
"coast.87":"t.l0.cellgroup_l_s_s_d",
"coast.88":"t.l0.cellgroup_d_l_s_d",
"coast.89":"t.l0.cellgroup_s_d_d_l",
"coast.90":"t.l0.cellgroup_d_d_l_s",
"coast.91":"t.l0.cellgroup_l_s_d_d",
"coast.92":"t.l0.cellgroup_d_l_s_l",
"coast.93":"t.l0.cellgroup_s_l_d_l",
"coast.94":"t.l0.cellgroup_l_d_l_s",
"coast.95":"t.l0.cellgroup_l_s_l_d",
"coast.96":"t.l0.cellgroup_l_l_s_s",
"coast.97":"t.l0.cellgroup_s_s_l_l",
"coast.98":"t.l0.cellgroup_s_l_l_s",
"coast.99":"t.l0.cellgroup_l_s_s_l",
"coast.100":"t.l0.cellgroup_l_l_s_d",
"coast.101":"t.l0.cellgroup_s_d_l_l",
"coast.102":"t.l0.cellgroup_d_l_l_s",
"coast.103":"t.l0.cellgroup_l_s_d_l",
"coast.104":"t.l0.cellgroup_l_l_s_l",
"coast.105":"t.l0.cellgroup_s_l_l_l",
"coast.106":"t.l0.cellgroup_l_l_l_s",
"coast.107":"t.l0.cellgroup_l_s_l_l",
"floor.0":"t.l0.cellgroup_d_d_d_d",
"floor.1":"t.l0.cellgroup_d_d_d_d",
"floor.2":"t.l0.cellgroup_d_d_d_d",
"floor.3":"t.l0.cellgroup_d_d_d_d",
"floor.4":"t.l0.cellgroup_d_d_d_s",
"floor.5":"t.l0.cellgroup_d_s_d_d",
"floor.6":"t.l0.cellgroup_s_d_d_d",
"floor.7":"t.l0.cellgroup_d_d_s_d",
"floor.8":"t.l0.cellgroup_d_d_d_l",
"floor.9":"t.l0.cellgroup_d_l_d_d",
"floor.10":"t.l0.cellgroup_l_d_d_d",
"floor.11":"t.l0.cellgroup_d_d_l_d",
"floor.12":"t.l0.cellgroup_s_d_d_d",
"floor.13":"t.l0.cellgroup_d_d_s_d",
"floor.14":"t.l0.cellgroup_d_s_d_d",
"floor.15":"t.l0.cellgroup_d_d_d_s",
"floor.16":"t.l0.cellgroup_s_d_d_s",
"floor.17":"t.l0.cellgroup_d_s_s_d",
"floor.18":"t.l0.cellgroup_s_s_d_d",
"floor.19":"t.l0.cellgroup_d_d_s_s",
"floor.20":"t.l0.cellgroup_s_d_d_l",
"floor.21":"t.l0.cellgroup_d_l_s_d",
"floor.22":"t.l0.cellgroup_l_s_d_d",
"floor.23":"t.l0.cellgroup_d_d_l_s",
"floor.24":"t.l0.cellgroup_l_d_d_d",
"floor.25":"t.l0.cellgroup_d_d_l_d",
"floor.26":"t.l0.cellgroup_d_l_d_d",
"floor.27":"t.l0.cellgroup_d_d_d_l",
"floor.28":"t.l0.cellgroup_l_d_d_s",
"floor.29":"t.l0.cellgroup_d_s_l_d",
"floor.30":"t.l0.cellgroup_s_l_d_d",
"floor.31":"t.l0.cellgroup_d_d_s_l",
"floor.32":"t.l0.cellgroup_l_d_d_l",
"floor.33":"t.l0.cellgroup_d_l_l_d",
"floor.34":"t.l0.cellgroup_l_l_d_d",
"floor.35":"t.l0.cellgroup_d_d_l_l",
"floor.36":"t.l0.cellgroup_d_s_d_d",
"floor.37":"t.l0.cellgroup_d_d_d_s",
"floor.38":"t.l0.cellgroup_d_d_s_d",
"floor.39":"t.l0.cellgroup_s_d_d_d",
"floor.40":"t.l0.cellgroup_d_s_d_s",
"floor.41":"t.l0.cellgroup_d_s_d_s",
"floor.42":"t.l0.cellgroup_s_d_s_d",
"floor.43":"t.l0.cellgroup_s_d_s_d",
"floor.44":"t.l0.cellgroup_d_s_d_l",
"floor.45":"t.l0.cellgroup_d_l_d_s",
"floor.46":"t.l0.cellgroup_l_d_s_d",
"floor.47":"t.l0.cellgroup_s_d_l_d",
"floor.48":"t.l0.cellgroup_s_s_d_d",
"floor.49":"t.l0.cellgroup_d_d_s_s",
"floor.50":"t.l0.cellgroup_d_s_s_d",
"floor.51":"t.l0.cellgroup_s_d_d_s",
"floor.52":"t.l0.cellgroup_s_s_d_s",
"floor.53":"t.l0.cellgroup_d_s_s_s",
"floor.54":"t.l0.cellgroup_s_s_s_d",
"floor.55":"t.l0.cellgroup_s_d_s_s",
"floor.56":"t.l0.cellgroup_s_s_d_l",
"floor.57":"t.l0.cellgroup_d_l_s_s",
"floor.58":"t.l0.cellgroup_l_s_s_d",
"floor.59":"t.l0.cellgroup_s_d_l_s",
"floor.60":"t.l0.cellgroup_l_s_d_d",
"floor.61":"t.l0.cellgroup_d_d_l_s",
"floor.62":"t.l0.cellgroup_d_l_s_d",
"floor.63":"t.l0.cellgroup_s_d_d_l",
"floor.64":"t.l0.cellgroup_l_s_d_s",
"floor.65":"t.l0.cellgroup_d_s_l_s",
"floor.66":"t.l0.cellgroup_s_l_s_d",
"floor.67":"t.l0.cellgroup_s_d_s_l",
"floor.68":"t.l0.cellgroup_l_s_d_l",
"floor.69":"t.l0.cellgroup_d_l_l_s",
"floor.70":"t.l0.cellgroup_l_l_s_d",
"floor.71":"t.l0.cellgroup_s_d_l_l",
"floor.72":"t.l0.cellgroup_d_l_d_d",
"floor.73":"t.l0.cellgroup_d_d_d_l",
"floor.74":"t.l0.cellgroup_d_d_l_d",
"floor.75":"t.l0.cellgroup_l_d_d_d",
"floor.76":"t.l0.cellgroup_d_l_d_s",
"floor.77":"t.l0.cellgroup_d_s_d_l",
"floor.78":"t.l0.cellgroup_s_d_l_d",
"floor.79":"t.l0.cellgroup_l_d_s_d",
"floor.80":"t.l0.cellgroup_d_l_d_l",
"floor.81":"t.l0.cellgroup_d_l_d_l",
"floor.82":"t.l0.cellgroup_l_d_l_d",
"floor.83":"t.l0.cellgroup_l_d_l_d",
"floor.84":"t.l0.cellgroup_s_l_d_d",
"floor.85":"t.l0.cellgroup_d_d_s_l",
"floor.86":"t.l0.cellgroup_d_s_l_d",
"floor.87":"t.l0.cellgroup_l_d_d_s",
"floor.88":"t.l0.cellgroup_s_l_d_s",
"floor.89":"t.l0.cellgroup_d_s_s_l",
"floor.90":"t.l0.cellgroup_s_s_l_d",
"floor.91":"t.l0.cellgroup_l_d_s_s",
"floor.92":"t.l0.cellgroup_s_l_d_l",
"floor.93":"t.l0.cellgroup_d_l_s_l",
"floor.94":"t.l0.cellgroup_l_s_l_d",
"floor.95":"t.l0.cellgroup_l_d_l_s",
"floor.96":"t.l0.cellgroup_l_l_d_d",
"floor.97":"t.l0.cellgroup_d_d_l_l",
"floor.98":"t.l0.cellgroup_d_l_l_d",
"floor.99":"t.l0.cellgroup_l_d_d_l",
"floor.100":"t.l0.cellgroup_l_l_d_s",
"floor.101":"t.l0.cellgroup_d_s_l_l",
"floor.102":"t.l0.cellgroup_s_l_l_d",
"floor.103":"t.l0.cellgroup_l_d_s_l",
"floor.104":"t.l0.cellgroup_l_l_d_l",
"floor.105":"t.l0.cellgroup_d_l_l_l",
"floor.106":"t.l0.cellgroup_l_l_l_d",
"floor.107":"t.l0.cellgroup_l_d_l_l"};
