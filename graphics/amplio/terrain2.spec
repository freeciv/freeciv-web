
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Hogne Håskjold
    Tim F. Smith <yoohootim@hotmail.com>
    Yautja
    Daniel Speyer
    Eleazar
"

[file]
gfx = "amplio/terrain2"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 48
pixel_border = 1

tiles = { "row", "column","tag"

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
 6,  1, "t.l1.mountains_n0e0s0w1"
 6,  2, "t.l1.mountains_n0e0s1w0"
 6,  3, "t.l1.mountains_n0e0s1w1"
 6,  4, "t.l1.mountains_n0e1s0w0"
 6,  5, "t.l1.mountains_n0e1s0w1"
 6,  6, "t.l1.mountains_n0e1s1w0"
 6,  7, "t.l1.mountains_n0e1s1w1"
 7,  0, "t.l1.mountains_n1e0s0w0"
 7,  1, "t.l1.mountains_n1e0s0w1"
 7,  2, "t.l1.mountains_n1e0s1w0"
 7,  3, "t.l1.mountains_n1e0s1w1"
 7,  4, "t.l1.mountains_n1e1s0w0"
 7,  5, "t.l1.mountains_n1e1s0w1"
 7,  6, "t.l1.mountains_n1e1s1w0"
 7,  7, "t.l1.mountains_n1e1s1w1"

;hills as overlay

 8,  0, "t.l1.hills_n0e0s0w0"
 8,  1, "t.l1.hills_n0e0s0w1"
 8,  2, "t.l1.hills_n0e0s1w0"
 8,  3, "t.l1.hills_n0e0s1w1"
 8,  4, "t.l1.hills_n0e1s0w0"
 8,  5, "t.l1.hills_n0e1s0w1"
 8,  6, "t.l1.hills_n0e1s1w0"
 8,  7, "t.l1.hills_n0e1s1w1"
 9,  0, "t.l1.hills_n1e0s0w0"
 9,  1, "t.l1.hills_n1e0s0w1"
 9,  2, "t.l1.hills_n1e0s1w0"
 9,  3, "t.l1.hills_n1e0s1w1"
 9,  4, "t.l1.hills_n1e1s0w0"
 9,  5, "t.l1.hills_n1e1s0w1"
 9,  6, "t.l1.hills_n1e1s1w0"
 9,  7, "t.l1.hills_n1e1s1w1"

}


[grid_coasts]

x_top_left = 1
y_top_left = 645
dx = 48
dy = 24
pixel_border = 1

tiles = { "row", "column","tag"

;* previous coordinates now in water.spec

}
