
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
		Peter Arbor <peter.arbor@gmail.com> (Original terrain)
		GriffonSpade
"

[file]
gfx = "hexemplio/water2"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"

; Floor cell sprites.  See doc/README.graphics
 0, 0,  "t.l0.floor_cell_d000"
 0, 1,  "t.l0.floor_cell_d100"
 0, 2,  "t.l0.floor_cell_d010"
 0, 3,  "t.l0.floor_cell_d110"
 0, 4,  "t.l0.floor_cell_d001"
 0, 5,  "t.l0.floor_cell_d101"
 0, 6,  "t.l0.floor_cell_d011"
 0, 7,  "t.l0.floor_cell_d111"

; Lake cell sprites
 1, 0,  "t.l0.lake_cell_d000"
 1, 1,  "t.l0.lake_cell_d100"
 1, 2,  "t.l0.lake_cell_d010"
 1, 3,  "t.l0.lake_cell_d110"
 1, 4,  "t.l0.lake_cell_d001"
 1, 5,  "t.l0.lake_cell_d101"
 1, 6,  "t.l0.lake_cell_d011"
 1, 7,  "t.l0.lake_cell_d111"

; coast cell sprites.  See doc/README.graphics
 2, 0,  "t.l1.coast_cell_d000"
 2, 1,  "t.l1.coast_cell_d100"
 2, 2,  "t.l1.coast_cell_d010"
 2, 3,  "t.l1.coast_cell_d110"
 2, 4,  "t.l1.coast_cell_d001"
 2, 5,  "t.l1.coast_cell_d101"
 2, 6,  "t.l1.coast_cell_d011"
 2, 7,  "t.l1.coast_cell_d111"

; Deep Ocean cell sprites
 2, 0,  "t.l1.floor_cell_d000"
 2, 1,  "t.l1.floor_cell_d100"
 2, 2,  "t.l1.floor_cell_d010"
 2, 3,  "t.l1.floor_cell_d110"
 2, 4,  "t.l1.floor_cell_d001"
 2, 5,  "t.l1.floor_cell_d101"
 2, 6,  "t.l1.floor_cell_d011"
 2, 7,  "t.l1.floor_cell_d111"

; Lake cell sprites
 2, 0,  "t.l1.lake_cell_d000"
 2, 1,  "t.l1.lake_cell_d100"
 2, 2,  "t.l1.lake_cell_d010"
 2, 3,  "t.l1.lake_cell_d110"
 2, 4,  "t.l1.lake_cell_d001"
 2, 5,  "t.l1.lake_cell_d101"
 2, 6,  "t.l1.lake_cell_d011"
 2, 7,  "t.l1.lake_cell_d111"

}
