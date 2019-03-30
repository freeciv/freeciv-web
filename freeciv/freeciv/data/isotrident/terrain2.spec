
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tim F. Smith <yoohootim@hotmail.com>
"

[file]
gfx = "isotrident/terrain2"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"

; Rivers (as special type), and whether north, south, east, west 
; also has river or is ocean:

 2,  0, "road.river_s_n0e0s0w0"
 2,  1, "road.river_s_n1e0s0w0"
 2,  2, "road.river_s_n0e1s0w0"
 2,  3, "road.river_s_n1e1s0w0"
 2,  4, "road.river_s_n0e0s1w0"
 2,  5, "road.river_s_n1e0s1w0"
 2,  6, "road.river_s_n0e1s1w0"
 2,  7, "road.river_s_n1e1s1w0"
 3,  0, "road.river_s_n0e0s0w1"
 3,  1, "road.river_s_n1e0s0w1"
 3,  2, "road.river_s_n0e1s0w1"
 3,  3, "road.river_s_n1e1s0w1"
 3,  4, "road.river_s_n0e0s1w1"
 3,  5, "road.river_s_n1e0s1w1"
 3,  6, "road.river_s_n0e1s1w1"
 3,  7, "road.river_s_n1e1s1w1"

;forests as overlay

 4,  0, "t.l1.forest_n0e0s0w0"
 4,  1, "t.l1.forest_n1e0s0w0"
 4,  2, "t.l1.forest_n0e1s0w0"
 4,  3, "t.l1.forest_n1e1s0w0"
 4,  4, "t.l1.forest_n0e0s1w0"
 4,  5, "t.l1.forest_n1e0s1w0"
 4,  6, "t.l1.forest_n0e1s1w0"
 4,  7, "t.l1.forest_n1e1s1w0"
 5,  0, "t.l1.forest_n0e0s0w1"
 5,  1, "t.l1.forest_n1e0s0w1"
 5,  2, "t.l1.forest_n0e1s0w1"
 5,  3, "t.l1.forest_n1e1s0w1"
 5,  4, "t.l1.forest_n0e0s1w1"
 5,  5, "t.l1.forest_n1e0s1w1"
 5,  6, "t.l1.forest_n0e1s1w1"
 5,  7, "t.l1.forest_n1e1s1w1"

;mountains as overlay

 6,  0, "t.l1.mountains_n0e0s0w0"
 6,  1, "t.l1.mountains_n1e0s0w0"
 6,  2, "t.l1.mountains_n0e1s0w0"
 6,  3, "t.l1.mountains_n1e1s0w0"
 6,  4, "t.l1.mountains_n0e0s1w0"
 6,  5, "t.l1.mountains_n1e0s1w0"
 6,  6, "t.l1.mountains_n0e1s1w0"
 6,  7, "t.l1.mountains_n1e1s1w0"
 7,  0, "t.l1.mountains_n0e0s0w1"
 7,  1, "t.l1.mountains_n1e0s0w1"
 7,  2, "t.l1.mountains_n0e1s0w1"
 7,  3, "t.l1.mountains_n1e1s0w1"
 7,  4, "t.l1.mountains_n0e0s1w1"
 7,  5, "t.l1.mountains_n1e0s1w1"
 7,  6, "t.l1.mountains_n0e1s1w1"
 7,  7, "t.l1.mountains_n1e1s1w1"

;hills as overlay

 8,  0, "t.l1.hills_n0e0s0w0"
 8,  1, "t.l1.hills_n1e0s0w0"
 8,  2, "t.l1.hills_n0e1s0w0"
 8,  3, "t.l1.hills_n1e1s0w0"
 8,  4, "t.l1.hills_n0e0s1w0"
 8,  5, "t.l1.hills_n1e0s1w0"
 8,  6, "t.l1.hills_n0e1s1w0"
 8,  7, "t.l1.hills_n1e1s1w0"
 9,  0, "t.l1.hills_n0e0s0w1"
 9,  1, "t.l1.hills_n1e0s0w1"
 9,  2, "t.l1.hills_n0e1s0w1"
 9,  3, "t.l1.hills_n1e1s0w1"
 9,  4, "t.l1.hills_n0e0s1w1"
 9,  5, "t.l1.hills_n1e0s1w1"
 9,  6, "t.l1.hills_n0e1s1w1"
 9,  7, "t.l1.hills_n1e1s1w1"

;river outlets

 10, 0, "road.river_outlet_n"
 10, 1, "road.river_outlet_e"
 10, 2, "road.river_outlet_s"
 10, 3, "road.river_outlet_w"

}


[grid_coasts]

x_top_left = 1
y_top_left = 429
dx = 32
dy = 16
pixel_border = 1

tiles = { "row", "column","tag"

; coast cell sprites.  See doc/README.graphics
 0, 0,  "t.l0.coast_cell_u000"
 0, 2,  "t.l0.coast_cell_u100"
 0, 4,  "t.l0.coast_cell_u010"
 0, 6,  "t.l0.coast_cell_u110"
 0, 8,  "t.l0.coast_cell_u001"
 0, 10, "t.l0.coast_cell_u101"
 0, 12, "t.l0.coast_cell_u011"
 0, 14, "t.l0.coast_cell_u111"
 
 1, 0,  "t.l0.coast_cell_d000"
 1, 2,  "t.l0.coast_cell_d100"
 1, 4,  "t.l0.coast_cell_d010"
 1, 6,  "t.l0.coast_cell_d110"
 1, 8,  "t.l0.coast_cell_d001"
 1, 10, "t.l0.coast_cell_d101"
 1, 12, "t.l0.coast_cell_d011"
 1, 14, "t.l0.coast_cell_d111"

 2, 0,  "t.l0.coast_cell_l000"
 2, 2,  "t.l0.coast_cell_l100"
 2, 4,  "t.l0.coast_cell_l010"
 2, 6,  "t.l0.coast_cell_l110"
 2, 8,  "t.l0.coast_cell_l001"
 2, 10, "t.l0.coast_cell_l101"
 2, 12, "t.l0.coast_cell_l011"
 2, 14, "t.l0.coast_cell_l111"

 2, 1,  "t.l0.coast_cell_r000"
 2, 3,  "t.l0.coast_cell_r100"
 2, 5,  "t.l0.coast_cell_r010"
 2, 7,  "t.l0.coast_cell_r110"
 2, 9,  "t.l0.coast_cell_r001"
 2, 11, "t.l0.coast_cell_r101"
 2, 13, "t.l0.coast_cell_r011"
 2, 15, "t.l0.coast_cell_r111"

; Deep Ocean cell sprites
 0, 16, "t.l0.floor_cell_u000"
 0, 2,  "t.l0.floor_cell_u100"
 0, 4,  "t.l0.floor_cell_u010"
 0, 6,  "t.l0.floor_cell_u110"
 0, 8,  "t.l0.floor_cell_u001"
 0, 10, "t.l0.floor_cell_u101"
 0, 12, "t.l0.floor_cell_u011"
 0, 14, "t.l0.floor_cell_u111"
 
 1, 16, "t.l0.floor_cell_d000"
 1, 2,  "t.l0.floor_cell_d100"
 1, 4,  "t.l0.floor_cell_d010"
 1, 6,  "t.l0.floor_cell_d110"
 1, 8,  "t.l0.floor_cell_d001"
 1, 10, "t.l0.floor_cell_d101"
 1, 12, "t.l0.floor_cell_d011"
 1, 14, "t.l0.floor_cell_d111"

 2, 16, "t.l0.floor_cell_l000"
 2, 2,  "t.l0.floor_cell_l100"
 2, 4,  "t.l0.floor_cell_l010"
 2, 6,  "t.l0.floor_cell_l110"
 2, 8,  "t.l0.floor_cell_l001"
 2, 10, "t.l0.floor_cell_l101"
 2, 12, "t.l0.floor_cell_l011"
 2, 14, "t.l0.floor_cell_l111"

 2, 17, "t.l0.floor_cell_r000"
 2, 3,  "t.l0.floor_cell_r100"
 2, 5,  "t.l0.floor_cell_r010"
 2, 7,  "t.l0.floor_cell_r110"
 2, 9,  "t.l0.floor_cell_r001"
 2, 11, "t.l0.floor_cell_r101"
 2, 13, "t.l0.floor_cell_r011"
 2, 15, "t.l0.floor_cell_r111"

}
