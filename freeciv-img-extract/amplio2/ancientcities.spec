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
options =  "+Freeciv-2.3-spec"

[info]

artists =  "
    Asian style by CapTVK
    Polynesian style by CapTVK
    Celtic style by Erwan, adapted to 96x48 by CapTVK
    Roman style by CapTVK
    City walls by Hogne Håskjold
 "

[file]
gfx =  "amplio2/ancientcities"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 72
pixel_border = 1

tiles = {  "row", "column", "tag"

; used by all city styles

 2,  11,  "city.asian_occupied_0"
 2,  11,  "city.tropical_occupied_0"
 2,  11,  "city.celtic_occupied_0"
 2,  11,  "city.classical_occupied_0"
 2,  11,  "city.babylonian_occupied_0"


;
; city tiles
;

 0,  0,  "city.asian_city_0"
 0,  1,  "city.asian_city_4"
 0,  2,  "city.asian_city_8"
 0,  3,  "city.asian_city_12"
 0,  4,  "city.asian_city_16" 
 0,  5,  "city.asian_wall_0"
 0,  6,  "city.asian_wall_4"
 0,  7,  "city.asian_wall_8"
 0,  8,  "city.asian_wall_12"
 0,  9,  "city.asian_wall_16" 
   

 1,  0,  "city.tropical_city_0"
 1,  1,  "city.tropical_city_4"
 1,  2,  "city.tropical_city_8"
 1,  3,  "city.tropical_city_12"
 1,  4,  "city.tropical_city_16" 
 1,  5,  "city.tropical_wall_0"
 1,  6,  "city.tropical_wall_4"
 1,  7,  "city.tropical_wall_8"
 1,  8,  "city.tropical_wall_12"
 1,  9,  "city.tropical_wall_16" 


 2,  0,  "city.celtic_city_0"
 2,  1,  "city.celtic_city_4"
 2,  2,  "city.celtic_city_8"
 2,  3,  "city.celtic_city_12"
 2,  4,  "city.celtic_city_16" 
 2,  5,  "city.celtic_wall_0"
 2,  6,  "city.celtic_wall_4"
 2,  7,  "city.celtic_wall_8"
 2,  8,  "city.celtic_wall_12"
 2,  9,  "city.celtic_wall_16" 


 3,  0,  "city.classical_city_0"
 3,  1,  "city.classical_city_4"
 3,  2,  "city.classical_city_8"
 3,  3,  "city.classical_city_12"
 3,  4,  "city.classical_city_16"
 3,  5,  "city.classical_wall_0"
 3,  6,  "city.classical_wall_4"
 3,  7,  "city.classical_wall_8"
 3,  8,  "city.classical_wall_12"
 3,  9,  "city.classical_wall_16"

 4,  0,  "city.babylonian_city_0"
 4,  1,  "city.babylonian_city_4"
 4,  2,  "city.babylonian_city_8"
 4,  3,  "city.babylonian_city_12"
 4,  4,  "city.babylonian_city_16"
 4,  5,  "city.babylonian_wall_0"
 4,  6,  "city.babylonian_wall_4"
 4,  7,  "city.babylonian_wall_8"
 4,  8,  "city.babylonian_wall_12"
 4,  9,  "city.babylonian_wall_16"

 }
