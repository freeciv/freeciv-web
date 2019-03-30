
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tim F. Smith
    Daniel Speyer (mix)
    Frederic Rodrigo (mix)
    Andreas Røsdal (hex mode)
    GriffonSpade
"

[file]
gfx = "isophex/terrain2"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"

;forests as overlay

 0,  0, "t.l1.forest_n0e0se0s0w0nw0"
 0,  1, "t.l1.forest_n1e0se0s0w0nw0"
 0,  2, "t.l1.forest_n0e1se0s0w0nw0"
 0,  3, "t.l1.forest_n1e1se0s0w0nw0"
 0,  4, "t.l1.forest_n0e0se0s1w0nw0"
 0,  5, "t.l1.forest_n1e0se0s1w0nw0"
 0,  6, "t.l1.forest_n0e1se0s1w0nw0"
 0,  7, "t.l1.forest_n1e1se0s1w0nw0"
 1,  0, "t.l1.forest_n0e0se0s0w1nw0"
 1,  1, "t.l1.forest_n1e0se0s0w1nw0"
 1,  2, "t.l1.forest_n0e1se0s0w1nw0"
 1,  3, "t.l1.forest_n1e1se0s0w1nw0"
 1,  4, "t.l1.forest_n0e0se0s1w1nw0"
 1,  5, "t.l1.forest_n1e0se0s1w1nw0"
 1,  6, "t.l1.forest_n0e1se0s1w1nw0"
 1,  7, "t.l1.forest_n1e1se0s1w1nw0"

 ; The below sprites are duplicates of the previous sprites,
 ; since there aren't yet graphics for the extra hex directions.
 
 0,  0, "t.l1.forest_n0e0se0s0w0nw1"
 0,  1, "t.l1.forest_n1e0se0s0w0nw1"
 0,  2, "t.l1.forest_n0e1se0s0w0nw1"
 0,  3, "t.l1.forest_n1e1se0s0w0nw1"
 0,  4, "t.l1.forest_n0e0se0s1w0nw1"
 0,  5, "t.l1.forest_n1e0se0s1w0nw1"
 0,  6, "t.l1.forest_n0e1se0s1w0nw1"
 0,  7, "t.l1.forest_n1e1se0s1w0nw1"
 1,  0, "t.l1.forest_n0e0se0s0w1nw1"
 1,  1, "t.l1.forest_n1e0se0s0w1nw1"
 1,  2, "t.l1.forest_n0e1se0s0w1nw1"
 1,  3, "t.l1.forest_n1e1se0s0w1nw1"
 1,  4, "t.l1.forest_n0e0se0s1w1nw1"
 1,  5, "t.l1.forest_n1e0se0s1w1nw1"
 1,  6, "t.l1.forest_n0e1se0s1w1nw1"
 1,  7, "t.l1.forest_n1e1se0s1w1nw1"
 0,  0, "t.l1.forest_n0e0se1s0w0nw0"
 0,  1, "t.l1.forest_n1e0se1s0w0nw0"
 0,  2, "t.l1.forest_n0e1se1s0w0nw0"
 0,  3, "t.l1.forest_n1e1se1s0w0nw0"
 0,  4, "t.l1.forest_n0e0se1s1w0nw0"
 0,  5, "t.l1.forest_n1e0se1s1w0nw0"
 0,  6, "t.l1.forest_n0e1se1s1w0nw0"
 0,  7, "t.l1.forest_n1e1se1s1w0nw0"
 1,  0, "t.l1.forest_n0e0se1s0w1nw0"
 1,  1, "t.l1.forest_n1e0se1s0w1nw0"
 1,  2, "t.l1.forest_n0e1se1s0w1nw0"
 1,  3, "t.l1.forest_n1e1se1s0w1nw0"
 1,  4, "t.l1.forest_n0e0se1s1w1nw0"
 1,  5, "t.l1.forest_n1e0se1s1w1nw0"
 1,  6, "t.l1.forest_n0e1se1s1w1nw0"
 1,  7, "t.l1.forest_n1e1se1s1w1nw0"
 0,  0, "t.l1.forest_n0e0se1s0w0nw1"
 0,  1, "t.l1.forest_n1e0se1s0w0nw1"
 0,  2, "t.l1.forest_n0e1se1s0w0nw1"
 0,  3, "t.l1.forest_n1e1se1s0w0nw1"
 0,  4, "t.l1.forest_n0e0se1s1w0nw1"
 0,  5, "t.l1.forest_n1e0se1s1w0nw1"
 0,  6, "t.l1.forest_n0e1se1s1w0nw1"
 0,  7, "t.l1.forest_n1e1se1s1w0nw1"
 1,  0, "t.l1.forest_n0e0se1s0w1nw1"
 1,  1, "t.l1.forest_n1e0se1s0w1nw1"
 1,  2, "t.l1.forest_n0e1se1s0w1nw1"
 1,  3, "t.l1.forest_n1e1se1s0w1nw1"
 1,  4, "t.l1.forest_n0e0se1s1w1nw1"
 1,  5, "t.l1.forest_n1e0se1s1w1nw1"
 1,  6, "t.l1.forest_n0e1se1s1w1nw1"
 1,  7, "t.l1.forest_n1e1se1s1w1nw1"

;mountains as overlay

 2,  0, "t.l1.mountains_n0e0se0s0w0nw0"
 2,  1, "t.l1.mountains_n1e0se0s0w0nw0"
 2,  2, "t.l1.mountains_n0e1se0s0w0nw0"
 2,  3, "t.l1.mountains_n1e1se0s0w0nw0"
 2,  4, "t.l1.mountains_n0e0se0s1w0nw0"
 2,  5, "t.l1.mountains_n1e0se0s1w0nw0"
 2,  6, "t.l1.mountains_n0e1se0s1w0nw0"
 2,  7, "t.l1.mountains_n1e1se0s1w0nw0"
 3,  0, "t.l1.mountains_n0e0se0s0w1nw0"
 3,  1, "t.l1.mountains_n1e0se0s0w1nw0"
 3,  2, "t.l1.mountains_n0e1se0s0w1nw0"
 3,  3, "t.l1.mountains_n1e1se0s0w1nw0"
 3,  4, "t.l1.mountains_n0e0se0s1w1nw0"
 3,  5, "t.l1.mountains_n1e0se0s1w1nw0"
 3,  6, "t.l1.mountains_n0e1se0s1w1nw0"
 3,  7, "t.l1.mountains_n1e1se0s1w1nw0"

 ; The below sprites are duplicates of the previous sprites,
 ; since there aren't yet graphics for the extra hex directions.
 
 2,  0, "t.l1.mountains_n0e0se0s0w0nw1"
 2,  1, "t.l1.mountains_n1e0se0s0w0nw1"
 2,  2, "t.l1.mountains_n0e1se0s0w0nw1"
 2,  3, "t.l1.mountains_n1e1se0s0w0nw1"
 2,  4, "t.l1.mountains_n0e0se0s1w0nw1"
 2,  5, "t.l1.mountains_n1e0se0s1w0nw1"
 2,  6, "t.l1.mountains_n0e1se0s1w0nw1"
 2,  7, "t.l1.mountains_n1e1se0s1w0nw1"
 3,  0, "t.l1.mountains_n0e0se0s0w1nw1"
 3,  1, "t.l1.mountains_n1e0se0s0w1nw1"
 3,  2, "t.l1.mountains_n0e1se0s0w1nw1"
 3,  3, "t.l1.mountains_n1e1se0s0w1nw1"
 3,  4, "t.l1.mountains_n0e0se0s1w1nw1"
 3,  5, "t.l1.mountains_n1e0se0s1w1nw1"
 3,  6, "t.l1.mountains_n0e1se0s1w1nw1"
 3,  7, "t.l1.mountains_n1e1se0s1w1nw1"
 2,  0, "t.l1.mountains_n0e0se1s0w0nw0"
 2,  1, "t.l1.mountains_n1e0se1s0w0nw0"
 2,  2, "t.l1.mountains_n0e1se1s0w0nw0"
 2,  3, "t.l1.mountains_n1e1se1s0w0nw0"
 2,  4, "t.l1.mountains_n0e0se1s1w0nw0"
 2,  5, "t.l1.mountains_n1e0se1s1w0nw0"
 2,  6, "t.l1.mountains_n0e1se1s1w0nw0"
 2,  7, "t.l1.mountains_n1e1se1s1w0nw0"
 3,  0, "t.l1.mountains_n0e0se1s0w1nw0"
 3,  1, "t.l1.mountains_n1e0se1s0w1nw0"
 3,  2, "t.l1.mountains_n0e1se1s0w1nw0"
 3,  3, "t.l1.mountains_n1e1se1s0w1nw0"
 3,  4, "t.l1.mountains_n0e0se1s1w1nw0"
 3,  5, "t.l1.mountains_n1e0se1s1w1nw0"
 3,  6, "t.l1.mountains_n0e1se1s1w1nw0"
 3,  7, "t.l1.mountains_n1e1se1s1w1nw0"
 2,  0, "t.l1.mountains_n0e0se1s0w0nw1"
 2,  1, "t.l1.mountains_n1e0se1s0w0nw1"
 2,  2, "t.l1.mountains_n0e1se1s0w0nw1"
 2,  3, "t.l1.mountains_n1e1se1s0w0nw1"
 2,  4, "t.l1.mountains_n0e0se1s1w0nw1"
 2,  5, "t.l1.mountains_n1e0se1s1w0nw1"
 2,  6, "t.l1.mountains_n0e1se1s1w0nw1"
 2,  7, "t.l1.mountains_n1e1se1s1w0nw1"
 3,  0, "t.l1.mountains_n0e0se1s0w1nw1"
 3,  1, "t.l1.mountains_n1e0se1s0w1nw1"
 3,  2, "t.l1.mountains_n0e1se1s0w1nw1"
 3,  3, "t.l1.mountains_n1e1se1s0w1nw1"
 3,  4, "t.l1.mountains_n0e0se1s1w1nw1"
 3,  5, "t.l1.mountains_n1e0se1s1w1nw1"
 3,  6, "t.l1.mountains_n0e1se1s1w1nw1"
 3,  7, "t.l1.mountains_n1e1se1s1w1nw1"

;hills as overlay

 4,  0, "t.l1.hills_n0e0se0s0w0nw0"
 4,  1, "t.l1.hills_n1e0se0s0w0nw0"
 4,  2, "t.l1.hills_n0e1se0s0w0nw0"
 4,  3, "t.l1.hills_n1e1se0s0w0nw0"
 4,  4, "t.l1.hills_n0e0se0s1w0nw0"
 4,  5, "t.l1.hills_n1e0se0s1w0nw0"
 4,  6, "t.l1.hills_n0e1se0s1w0nw0"
 4,  7, "t.l1.hills_n1e1se0s1w0nw0"
 5,  0, "t.l1.hills_n0e0se0s0w1nw0"
 5,  1, "t.l1.hills_n1e0se0s0w1nw0"
 5,  2, "t.l1.hills_n0e1se0s0w1nw0"
 5,  3, "t.l1.hills_n1e1se0s0w1nw0"
 5,  4, "t.l1.hills_n0e0se0s1w1nw0"
 5,  5, "t.l1.hills_n1e0se0s1w1nw0"
 5,  6, "t.l1.hills_n0e1se0s1w1nw0"
 5,  7, "t.l1.hills_n1e1se0s1w1nw0"

 ; The below sprites are duplicates of the previous sprites,
 ; since there aren't yet graphics for the extra hex directions.
 
 4,  0, "t.l1.hills_n0e0se0s0w0nw1"
 4,  1, "t.l1.hills_n1e0se0s0w0nw1"
 4,  2, "t.l1.hills_n0e1se0s0w0nw1"
 4,  3, "t.l1.hills_n1e1se0s0w0nw1"
 4,  4, "t.l1.hills_n0e0se0s1w0nw1"
 4,  5, "t.l1.hills_n1e0se0s1w0nw1"
 4,  6, "t.l1.hills_n0e1se0s1w0nw1"
 4,  7, "t.l1.hills_n1e1se0s1w0nw1"
 5,  0, "t.l1.hills_n0e0se0s0w1nw1"
 5,  1, "t.l1.hills_n1e0se0s0w1nw1"
 5,  2, "t.l1.hills_n0e1se0s0w1nw1"
 5,  3, "t.l1.hills_n1e1se0s0w1nw1"
 5,  4, "t.l1.hills_n0e0se0s1w1nw1"
 5,  5, "t.l1.hills_n1e0se0s1w1nw1"
 5,  6, "t.l1.hills_n0e1se0s1w1nw1"
 5,  7, "t.l1.hills_n1e1se0s1w1nw1"
 4,  0, "t.l1.hills_n0e0se1s0w0nw0"
 4,  1, "t.l1.hills_n1e0se1s0w0nw0"
 4,  2, "t.l1.hills_n0e1se1s0w0nw0"
 4,  3, "t.l1.hills_n1e1se1s0w0nw0"
 4,  4, "t.l1.hills_n0e0se1s1w0nw0"
 4,  5, "t.l1.hills_n1e0se1s1w0nw0"
 4,  6, "t.l1.hills_n0e1se1s1w0nw0"
 4,  7, "t.l1.hills_n1e1se1s1w0nw0"
 5,  0, "t.l1.hills_n0e0se1s0w1nw0"
 5,  1, "t.l1.hills_n1e0se1s0w1nw0"
 5,  2, "t.l1.hills_n0e1se1s0w1nw0"
 5,  3, "t.l1.hills_n1e1se1s0w1nw0"
 5,  4, "t.l1.hills_n0e0se1s1w1nw0"
 5,  5, "t.l1.hills_n1e0se1s1w1nw0"
 5,  6, "t.l1.hills_n0e1se1s1w1nw0"
 5,  7, "t.l1.hills_n1e1se1s1w1nw0"
 4,  0, "t.l1.hills_n0e0se1s0w0nw1"
 4,  1, "t.l1.hills_n1e0se1s0w0nw1"
 4,  2, "t.l1.hills_n0e1se1s0w0nw1"
 4,  3, "t.l1.hills_n1e1se1s0w0nw1"
 4,  4, "t.l1.hills_n0e0se1s1w0nw1"
 4,  5, "t.l1.hills_n1e0se1s1w0nw1"
 4,  6, "t.l1.hills_n0e1se1s1w0nw1"
 4,  7, "t.l1.hills_n1e1se1s1w0nw1"
 5,  0, "t.l1.hills_n0e0se1s0w1nw1"
 5,  1, "t.l1.hills_n1e0se1s0w1nw1"
 5,  2, "t.l1.hills_n0e1se1s0w1nw1"
 5,  3, "t.l1.hills_n1e1se1s0w1nw1"
 5,  4, "t.l1.hills_n0e0se1s1w1nw1"
 5,  5, "t.l1.hills_n1e0se1s1w1nw1"
 5,  6, "t.l1.hills_n0e1se1s1w1nw1"
 5,  7, "t.l1.hills_n1e1se1s1w1nw1"

; River outlets

 6,  0, "road.river_outlet_n"
 6,  1, "road.river_outlet_e"
 6,  2, "road.river_outlet_s"
 6,  3, "road.river_outlet_w"
 6,  4, "road.river_outlet_se"
 6,  5, "road.river_outlet_nw"

}


[grid_coasts]

x_top_left = 1
y_top_left = 283
dx = 32
dy = 16
pixel_border = 1

tiles = { "row", "column","tag"

; lake cell sprites.  See doc/README.graphics

 0, 0,  "t.l0.lake_cell_u000"
 0, 2,  "t.l0.lake_cell_u100"
 0, 4,  "t.l0.lake_cell_u010"
 0, 6,  "t.l0.lake_cell_u110"
 0, 8,  "t.l0.lake_cell_u001"
 0, 10, "t.l0.lake_cell_u101"
 0, 12, "t.l0.lake_cell_u011"
 0, 14, "t.l0.lake_cell_u111"
 
 1, 0,  "t.l0.lake_cell_d000"
 1, 2,  "t.l0.lake_cell_d100"
 1, 4,  "t.l0.lake_cell_d010"
 1, 6,  "t.l0.lake_cell_d110"
 1, 8,  "t.l0.lake_cell_d001"
 1, 10, "t.l0.lake_cell_d101"
 1, 12, "t.l0.lake_cell_d011"
 1, 14, "t.l0.lake_cell_d111"

 2, 0,  "t.l0.lake_cell_l000"
 2, 2,  "t.l0.lake_cell_l100"
 2, 4,  "t.l0.lake_cell_l010"
 2, 6,  "t.l0.lake_cell_l110"
 2, 8,  "t.l0.lake_cell_l001"
 2, 10, "t.l0.lake_cell_l101"
 2, 12, "t.l0.lake_cell_l011"
 2, 14, "t.l0.lake_cell_l111"

 2, 1,  "t.l0.lake_cell_r000"
 2, 3,  "t.l0.lake_cell_r100"
 2, 5,  "t.l0.lake_cell_r010"
 2, 7,  "t.l0.lake_cell_r110"
 2, 9,  "t.l0.lake_cell_r001"
 2, 11, "t.l0.lake_cell_r101"
 2, 13, "t.l0.lake_cell_r011"
 2, 15, "t.l0.lake_cell_r111"

; coast cell sprites.  See doc/README.graphics

 3, 0,  "t.l0.coast_cell_u000"
 3, 2,  "t.l0.coast_cell_u100"
 3, 4,  "t.l0.coast_cell_u010"
 3, 6,  "t.l0.coast_cell_u110"
 3, 8,  "t.l0.coast_cell_u001"
 3, 10, "t.l0.coast_cell_u101"
 3, 12, "t.l0.coast_cell_u011"
 3, 14, "t.l0.coast_cell_u111"
 
 4, 0,  "t.l0.coast_cell_d000"
 4, 2,  "t.l0.coast_cell_d100"
 4, 4,  "t.l0.coast_cell_d010"
 4, 6,  "t.l0.coast_cell_d110"
 4, 8,  "t.l0.coast_cell_d001"
 4, 10, "t.l0.coast_cell_d101"
 4, 12, "t.l0.coast_cell_d011"
 4, 14, "t.l0.coast_cell_d111"

 5, 0,  "t.l0.coast_cell_l000"
 5, 2,  "t.l0.coast_cell_l100"
 5, 4,  "t.l0.coast_cell_l010"
 5, 6,  "t.l0.coast_cell_l110"
 5, 8,  "t.l0.coast_cell_l001"
 5, 10, "t.l0.coast_cell_l101"
 5, 12, "t.l0.coast_cell_l011"
 5, 14, "t.l0.coast_cell_l111"

 5, 1,  "t.l0.coast_cell_r000"
 5, 3,  "t.l0.coast_cell_r100"
 5, 5,  "t.l0.coast_cell_r010"
 5, 7,  "t.l0.coast_cell_r110"
 5, 9,  "t.l0.coast_cell_r001"
 5, 11, "t.l0.coast_cell_r101"
 5, 13, "t.l0.coast_cell_r011"
 5, 15, "t.l0.coast_cell_r111"

; floor cell sprites.  See doc/README.graphics

 6, 0,  "t.l0.floor_cell_u000"
 6, 2,  "t.l0.floor_cell_u100"
 6, 4,  "t.l0.floor_cell_u010"
 6, 6,  "t.l0.floor_cell_u110"
 6, 8,  "t.l0.floor_cell_u001"
 6, 10, "t.l0.floor_cell_u101"
 6, 12, "t.l0.floor_cell_u011"
 6, 14, "t.l0.floor_cell_u111"
 
 7, 0,  "t.l0.floor_cell_d000"
 7, 2,  "t.l0.floor_cell_d100"
 7, 4,  "t.l0.floor_cell_d010"
 7, 6,  "t.l0.floor_cell_d110"
 7, 8,  "t.l0.floor_cell_d001"
 7, 10, "t.l0.floor_cell_d101"
 7, 12, "t.l0.floor_cell_d011"
 7, 14, "t.l0.floor_cell_d111"

 8, 0,  "t.l0.floor_cell_l000"
 8, 2,  "t.l0.floor_cell_l100"
 8, 4,  "t.l0.floor_cell_l010"
 8, 6,  "t.l0.floor_cell_l110"
 8, 8,  "t.l0.floor_cell_l001"
 8, 10, "t.l0.floor_cell_l101"
 8, 12, "t.l0.floor_cell_l011"
 8, 14, "t.l0.floor_cell_l111"

 8, 1,  "t.l0.floor_cell_r000"
 8, 3,  "t.l0.floor_cell_r100"
 8, 5,  "t.l0.floor_cell_r010"
 8, 7,  "t.l0.floor_cell_r110"
 8, 9,  "t.l0.floor_cell_r001"
 8, 11, "t.l0.floor_cell_r101"
 8, 13, "t.l0.floor_cell_r011"
 8, 15, "t.l0.floor_cell_r111"

}
