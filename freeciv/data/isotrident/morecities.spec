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
options = "+spec3"

[info]

artists = "
    Jerzy Klek <jekl@altavista.net>

    european style based on trident tileset by
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Marco Saupe <msaupe@saale-net.de> (reworked classic, industrial and modern)
"

[file]
gfx = "isotrident/morecities"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 64
dy = 48

tiles = { "row", "column", "tag"
;
; city tiles
;

 0,  0, "city.asian_city_0"
 0,  1, "city.asian_city_5"
 0,  2, "city.asian_city_10"
 ;1,  3, "city.asian_wall"
 0,  4, "city.asian_occupied_0"
 0,  5, "city.asian_wall_0"
 0,  6, "city.asian_wall_5"
 0,  7, "city.asian_wall_10"
   

 1,  0, "city.tropical_city_0"
 1,  1, "city.tropical_city_5"
 1,  2, "city.tropical_city_10"
 ;5,  3, "city.tropical_wall"
 1,  4, "city.tropical_occupied_0"
 1,  5, "city.tropical_wall_0"
 1,  6, "city.tropical_wall_5"
 1,  7, "city.tropical_wall_10"
   
}
