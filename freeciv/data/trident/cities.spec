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
gfx = "trident/cities"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

; default tiles

 1,  2, "cd.city"
 1,  3, "cd.city_wall"
 1,  4, "cd.occupied"

; used by all city styles

 0,  0, "city.disorder"

;
; city tiles
;

 1,  0, "city.european_city_0"
 1,  1, "city.european_city_5"
 1,  2, "city.european_city_10"
 1,  3, "city.european_wall_0"
 1,  4, "city.european_occupied_0"

 5,  0, "city.classical_city_0"
 5,  1, "city.classical_city_5"
 5,  2, "city.classical_city_10"
 5,  3, "city.classical_wall_0"
 5,  4, "city.classical_occupied_0"

 2,  0, "city.industrial_city_0"
 2,  1, "city.industrial_city_5"
 2,  2, "city.industrial_city_10"
 2,  3, "city.industrial_wall_0"
 2,  4, "city.industrial_occupied_0"

 3,  0, "city.modern_city_0"
 3,  1, "city.modern_city_5"
 3,  2, "city.modern_city_10"
 3,  3, "city.modern_wall_0"
 3,  4, "city.modern_occupied_0"

 4,  0, "city.postmodern_city_0"
 4,  1, "city.postmodern_city_5"
 4,  2, "city.postmodern_city_10"
 4,  3, "city.postmodern_wall_0"
 4,  4, "city.postmodern_occupied_0"

 6,  0, "city.asian_city_0"
 6,  1, "city.asian_city_5"
 6,  2, "city.asian_city_10"
 6,  3, "city.asian_wall_0"
 6,  4, "city.asian_occupied_0"

 7,  0, "city.tropical_city_0"
 7,  1, "city.tropical_city_5"
 7,  2, "city.tropical_city_10"
 7,  3, "city.tropical_wall_0"
 7,  4, "city.tropical_occupied_0"
}
