;
; The names for city tiles are not free and must follow the following rules.
; The names consists of 'style name' + '_' + 'index'. The style name is as
; specified in cities.ruleset file and the index only defines the read order
; of the images. The definitions are read starting with index 0 till the first
; missing value The index is checked against the city bonus of effect
; EFT_CITY_IMG and the resulting image is used to draw the city on the tile.
;
; Obviously the first tile must be 'style_name'_city_0 and the sizes must be
; in ascending order. There must also be a 'style_name'_wall_0 tile used to
; draw the wall and an occupied tile to indicate a military units in a city.
; The maximum number of images is only limited by the maximum size of a city
; (currently MAX_CITY_SIZE = 255).
;

[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

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
 1,  1, "city.european_city_1"
 1,  2, "city.european_city_2"
 1,  3, "city.european_wall_0"
 1,  4, "city.european_occupied_0"

 5,  0, "city.classical_city_0"
 5,  1, "city.classical_city_1"
 5,  2, "city.classical_city_2"
 5,  3, "city.classical_wall_0"
 5,  4, "city.classical_occupied_0"

 2,  0, "city.industrial_city_0"
 2,  1, "city.industrial_city_1"
 2,  2, "city.industrial_city_2"
 2,  3, "city.industrial_wall_0"
 2,  4, "city.industrial_occupied_0"

 3,  0, "city.modern_city_0"
 3,  1, "city.modern_city_1"
 3,  2, "city.modern_city_2"
 3,  3, "city.modern_wall_0"
 3,  4, "city.modern_occupied_0"

 4,  0, "city.postmodern_city_0"
 4,  1, "city.postmodern_city_1"
 4,  2, "city.postmodern_city_2"
 4,  3, "city.postmodern_wall_0"
 4,  4, "city.postmodern_occupied_0"

 6,  0, "city.asian_city_0"
 6,  1, "city.asian_city_1"
 6,  2, "city.asian_city_2"
 6,  3, "city.asian_wall_0"
 6,  4, "city.asian_occupied_0"

 7,  0, "city.tropical_city_0"
 7,  1, "city.tropical_city_1"
 7,  2, "city.tropical_city_2"
 7,  3, "city.tropical_wall_0"
 7,  4, "city.tropical_occupied_0"

 8,  0, "city.electricage_city_0"
 8,  1, "city.electricage_city_1"
 8,  2, "city.electricage_city_2"
 8,  3, "city.electricage_wall_0"
 8,  4, "city.electricage_occupied_0"
}
