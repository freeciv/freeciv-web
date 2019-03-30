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
    Medieval by CapTVK
    Steam Age by Smiley, www.firstcultural.com
    City walls by Hogne Håskjold
"

[file]
gfx = "amplio/medievalcities"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 72
pixel_border = 1

tiles = { "row", "column", "tag"

; used by all city styles

 2,  11, "city.european_occupied_0"
 2,  11, "city.industrial_occupied_0"

;
; city tiles
;

 0,  0, "city.european_city_0"
 0,  1, "city.european_city_1"
 0,  2, "city.european_city_2"
 0,  3, "city.european_city_3"
 0,  4, "city.european_city_4" 
 0,  5, "city.european_wall_0"
 0,  6, "city.european_wall_1"
 0,  7, "city.european_wall_2"
 0,  8, "city.european_wall_3"
 0,  9, "city.european_wall_4"
 
 1,  0, "city.industrial_city_0"
 1,  1, "city.industrial_city_1"
 1,  2, "city.industrial_city_2"
 1,  3, "city.industrial_city_3"
 1,  4, "city.industrial_city_4" 
 1,  5, "city.industrial_wall_0"
 1,  6, "city.industrial_wall_1"
 1,  7, "city.industrial_wall_2"
 1,  8, "city.industrial_wall_3"
 1,  9, "city.industrial_wall_4" 

}
