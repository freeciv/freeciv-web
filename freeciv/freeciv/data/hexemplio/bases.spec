;
; The names for city tiles are not free and must follow the following rules.
; The names consists of 'style name' + '_' + 'index'. The style name is as
; specified in cities.ruleset file and the index only defines the read order
; of the images. The definitions are read starting with index 0 till the first
; missing value The index is checked against the city bonus of effect
; 'City_Image' and the resulting image is used to draw the city on the tile.
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
    Hogne Håskjold <hogne@freeciv.org>[HH]
    Eleazar [El](buoy)
    Anton Ecker (Kaldred) (ruins)
    GriffonSpade [GS]
"

[file]
gfx = "hexemplio/bases"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 126
dy = 96
pixel_border = 1

tiles = { "row", "column", "tag"

;[HH][GS]
 0,  0, "base.airstrip_mg"			
 1,  0, "tx.airstrip_full"
;[HH][GS]
 0,  1, "base.airbase_mg"
 1,  1, "tx.airbase_full"
;[HH][GS]
 1,  2, "base.outpost_fg"
 0,  2, "base.outpost_bg"
;[HH]
 1,  3, "base.fortress_fg"
 0,  3, "base.fortress_bg"
;[HH]
 0,  4, "city.disorder"
;[El]
 1,  4, "base.buoy_mg"
;[VC]
 0,  5, "extra.ruins_mg"
;[HH]
 1,  5, "city.european_occupied_0"
 1,  5, "city.classical_occupied_0"
 1,  5, "city.asian_occupied_0"
 1,  5, "city.tropical_occupied_0"
 1,  5, "city.celtic_occupied_0"
 1,  5, "city.babylonian_occupied_0"
 1,  5, "city.industrial_occupied_0"
 1,  5, "city.electricage_occupied_0"
 1,  5, "city.modern_occupied_0"
 1,  5, "city.postmodern_occupied_0"
}
