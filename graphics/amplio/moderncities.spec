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
    Airfield, city walls and misc stuff Hogne Håskjold <hogne@freeciv.org>
    Modern, Post Modern and Electric Age by Smiley, www.firstcultural.com
    City walls by Hogne Håskjold
    Buoy by Eleazar
    Ruins by Vincent Croisier
    Fortress and Airstrip by GriffonSpade
"

[file]
gfx = "amplio/moderncities"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 72
pixel_border = 1

tiles = { "row", "column", "tag"

; default tiles

 1,  2, "cd.city"
 1,  7, "cd.city_wall"
 0,  6, "cd.occupied"

; used by all city styles

 0,  0, "city.disorder"
 0,  1, "base.airbase_mg"
 0,  2, "tx.airbase_full"
 0,  4, "base.outpost_fg"
 0,  5, "base.outpost_bg"
 0,  6, "city.electricage_occupied_0"
 0,  6, "city.modern_occupied_0"
 0,  6, "city.postmodern_occupied_0"
 0,  8, "base.buoy_mg"
 0,  9, "extra.ruins_mg"

 1,  1, "base.airstrip_mg"
 1,  4, "base.fortress_fg"
 1,  5, "base.fortress_bg"
;
; city tiles
;

 2,  0, "city.electricage_city_0"
 2,  1, "city.electricage_city_1"
 2,  2, "city.electricage_city_2"
 2,  3, "city.electricage_city_3"
 2,  4, "city.electricage_city_4" 
 2,  5, "city.electricage_wall_0"
 2,  6, "city.electricage_wall_1"
 2,  7, "city.electricage_wall_2"
 2,  8, "city.electricage_wall_3"
 2,  9, "city.electricage_wall_4" 


 3,  0, "city.modern_city_0"
 3,  1, "city.modern_city_1"
 3,  2, "city.modern_city_2"
 3,  3, "city.modern_city_3"
 3,  4, "city.modern_city_4"
 3,  5, "city.modern_wall_0"
 3,  6, "city.modern_wall_1"
 3,  7, "city.modern_wall_2"
 3,  8, "city.modern_wall_3"
 3,  9, "city.modern_wall_4"


 4,  0, "city.postmodern_city_0"
 4,  1, "city.postmodern_city_1"
 4,  2, "city.postmodern_city_2"
 4,  3, "city.postmodern_city_3"
 4,  4, "city.postmodern_city_4" 
 4,  5, "city.postmodern_wall_0"
 4,  6, "city.postmodern_wall_1"
 4,  7, "city.postmodern_wall_2"
 4,  8, "city.postmodern_wall_3"
 4,  9, "city.postmodern_wall_4" 

}
