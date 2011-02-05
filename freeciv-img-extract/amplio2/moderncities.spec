;
; The names for city tiles are not free and must follow the following rules.
; The names consists of style name, _ , size. The style name is as specified
; in cities.ruleset file. The size indicates which size city must
; have to be drawn with a tile. E.g. european_4 means that the tile is to be
; used for cities of size 4+ in european style. Obviously the first tile
; must be style_name_0. The sizes must be in ascending order.
; There must also be a style_name_wall tile used to draw the wall and
; an occupied tile to indicate a miltary units in a city.
; The maximum size supported now is 31, but there can only be MAX_CITY_TILES
; normal tiles. The constant is defined in common/city.h and set to 8 now.
;

[spec]

; Format and options of this spec file:
options =   "+Freeciv-2.3-spec"

[info]

artists =   "
    Airfield, city walls and misc stuff Hogne Håskjold <hogne@freeciv.org>
    Modern, Post Modern and Electric Age by Smiley, www.firstcultural.com
    City walls by Hogne Håskjold
    Buoy by Eleazar
    Ruins by Vincent Croisier
  "

[file]
gfx =   "amplio2/moderncities"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 72
pixel_border = 1

tiles = {   "row", "column", "tag"

; default tiles

 1,  2,   "cd.city"
 1,  7,   "cd.city_wall"
 0,  6,   "cd.occupied"

; used by all city styles

 0,  0,   "city.disorder"
 0,  1,   "base.airbase_mg"
 0,  2,   "tx.airbase_full"
 0,  4,   "base.fortress_fg"
 0,  5,   "base.fortress_bg"
 0,  6,   "city.electricage_occupied_0"
 0,  6,   "city.modern_occupied_0"
 0,  6,   "city.postmodern_occupied_0"
 0,  8,   "base.buoy_mg"
 0,  9,   "base.ruins_mg"
;
; city tiles
;

 2,  0,   "city.electricage_city_0"
 2,  1,   "city.electricage_city_4"
 2,  2,   "city.electricage_city_8"
 2,  3,   "city.electricage_city_12"
 2,  4,   "city.electricage_city_16" 
 2,  5,   "city.electricage_wall_0"
 2,  6,   "city.electricage_wall_4"
 2,  7,   "city.electricage_wall_8"
 2,  8,   "city.electricage_wall_12"
 2,  9,   "city.electricage_wall_16" 


 3,  0,   "city.modern_city_0"
 3,  1,   "city.modern_city_4"
 3,  2,   "city.modern_city_8"
 3,  3,   "city.modern_city_12"
 3,  4,   "city.modern_city_16"
 3,  5,   "city.modern_wall_0"
 3,  6,   "city.modern_wall_4"
 3,  7,   "city.modern_wall_8"
 3,  8,   "city.modern_wall_12"
 3,  9,   "city.modern_wall_16"


 4,  0,   "city.postmodern_city_0"
 4,  1,   "city.postmodern_city_4"
 4,  2,   "city.postmodern_city_8"
 4,  3,   "city.postmodern_city_12"
 4,  4,   "city.postmodern_city_16" 
 4,  5,   "city.postmodern_wall_0"
 4,  6,   "city.postmodern_wall_4"
 4,  7,   "city.postmodern_wall_8"
 4,  8,   "city.postmodern_wall_12"
 4,  9,   "city.postmodern_wall_16" 

 }
