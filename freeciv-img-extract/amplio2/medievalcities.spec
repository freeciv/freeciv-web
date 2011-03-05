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
    Medieval by CapTVK
    Steam Age by Smiley, www.firstcultural.com
    City walls by Hogne Håskjold
 "

[file]
gfx =  "amplio/medievalcities"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 72
pixel_border = 1

tiles = {  "row", "column", "tag"

; used by all city styles

 2,  11,  "city.european_occupied_0"
 2,  11,  "city.industrial_occupied_0"

;
; city tiles
;

 0,  0,  "city.european_city_0"
 0,  1,  "city.european_city_4"
 0,  2,  "city.european_city_8"
 0,  3,  "city.european_city_12"
 0,  4,  "city.european_city_16" 
 0,  5,  "city.european_wall_0"
 0,  6,  "city.european_wall_4"
 0,  7,  "city.european_wall_8"
 0,  8,  "city.european_wall_12"
 0,  9,  "city.european_wall_16"
 
 1,  0,  "city.industrial_city_0"
 1,  1,  "city.industrial_city_4"
 1,  2,  "city.industrial_city_8"
 1,  3,  "city.industrial_city_12"
 1,  4,  "city.industrial_city_16" 
 1,  5,  "city.industrial_wall_0"
 1,  6,  "city.industrial_wall_4"
 1,  7,  "city.industrial_wall_8"
 1,  8,  "city.industrial_wall_12"
 1,  9,  "city.industrial_wall_16" 

 }
