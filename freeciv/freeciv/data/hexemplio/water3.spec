
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
		Peter Arbor <peter.arbor@gmail.com> (Original terrain)
		GriffonSpade
"

[file]
gfx = "hexemplio/water3"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 47
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"

; Floor cell sprites
 0, 0,  "t.l0.floor_cell_l000"
 0, 0,  "t.l0.floor_cell_l010"
 0, 1,  "t.l0.floor_cell_l100"
 0, 1,  "t.l0.floor_cell_l110"
 0, 2,  "t.l0.floor_cell_l001"
 0, 2,  "t.l0.floor_cell_l011"
 0, 3,  "t.l0.floor_cell_l101"
 0, 3,  "t.l0.floor_cell_l111"

 0, 4,  "t.l0.floor_cell_r000"
 0, 4,  "t.l0.floor_cell_r010"
 0, 5,  "t.l0.floor_cell_r100"
 0, 5,  "t.l0.floor_cell_r110"
 0, 6,  "t.l0.floor_cell_r001"
 0, 6,  "t.l0.floor_cell_r011"
 0, 7,  "t.l0.floor_cell_r101"
 0, 7,  "t.l0.floor_cell_r111"

; Lake cell sprites
 1, 0,  "t.l0.lake_cell_l000"
 1, 0,  "t.l0.lake_cell_l010"
 1, 1,  "t.l0.lake_cell_l100"
 1, 1,  "t.l0.lake_cell_l110"
 1, 2,  "t.l0.lake_cell_l001"
 1, 2,  "t.l0.lake_cell_l011"
 1, 3,  "t.l0.lake_cell_l101"
 1, 3,  "t.l0.lake_cell_l111"

 1, 4,  "t.l0.lake_cell_r000"
 1, 4,  "t.l0.lake_cell_r010"
 1, 5,  "t.l0.lake_cell_r100"
 1, 5,  "t.l0.lake_cell_r110"
 1, 6,  "t.l0.lake_cell_r001"
 1, 6,  "t.l0.lake_cell_r011"
 1, 7,  "t.l0.lake_cell_r101"
 1, 7,  "t.l0.lake_cell_r111"

; coast cell sprites.  See doc/README.graphics
 2, 0,  "t.l1.coast_cell_l000"
 2, 0,  "t.l1.coast_cell_l010"
 2, 1,  "t.l1.coast_cell_l100"
 2, 1,  "t.l1.coast_cell_l110"
 2, 2,  "t.l1.coast_cell_l001"
 2, 2,  "t.l1.coast_cell_l011"
 2, 3,  "t.l1.coast_cell_l101"
 2, 3,  "t.l1.coast_cell_l111"

 2, 4,  "t.l1.coast_cell_r000"
 2, 4,  "t.l1.coast_cell_r010"
 2, 5,  "t.l1.coast_cell_r100"
 2, 5,  "t.l1.coast_cell_r110"
 2, 6,  "t.l1.coast_cell_r001"
 2, 6,  "t.l1.coast_cell_r011"
 2, 7,  "t.l1.coast_cell_r101"
 2, 7,  "t.l1.coast_cell_r111"

; Deep Ocean cell sprites
 2, 0,  "t.l1.floor_cell_l000"
 2, 0,  "t.l1.floor_cell_l010"
 2, 1,  "t.l1.floor_cell_l100"
 2, 1,  "t.l1.floor_cell_l110"
 2, 2,  "t.l1.floor_cell_l001"
 2, 2,  "t.l1.floor_cell_l011"
 2, 3,  "t.l1.floor_cell_l101"
 2, 3,  "t.l1.floor_cell_l111"

 2, 4,  "t.l1.floor_cell_r000"
 2, 4,  "t.l1.floor_cell_r010"
 2, 5,  "t.l1.floor_cell_r100"
 2, 5,  "t.l1.floor_cell_r110"
 2, 6,  "t.l1.floor_cell_r001"
 2, 6,  "t.l1.floor_cell_r011"
 2, 7,  "t.l1.floor_cell_r101"
 2, 7,  "t.l1.floor_cell_r111"

; Lake cell sprites
 2, 0,  "t.l1.lake_cell_l000"
 2, 0,  "t.l1.lake_cell_l010"
 2, 1,  "t.l1.lake_cell_l100"
 2, 1,  "t.l1.lake_cell_l110"
 2, 2,  "t.l1.lake_cell_l001"
 2, 2,  "t.l1.lake_cell_l011"
 2, 3,  "t.l1.lake_cell_l101"
 2, 3,  "t.l1.lake_cell_l111"

 2, 4,  "t.l1.lake_cell_r000"
 2, 4,  "t.l1.lake_cell_r010"
 2, 5,  "t.l1.lake_cell_r100"
 2, 5,  "t.l1.lake_cell_r110"
 2, 6,  "t.l1.lake_cell_r001"
 2, 6,  "t.l1.lake_cell_r011"
 2, 7,  "t.l1.lake_cell_r101"
 2, 7,  "t.l1.lake_cell_r111"

}
