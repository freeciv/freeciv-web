
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    David Pfitzner <dwp@mso.anu.edu.au> (original design)
"

[file]
gfx = "trident/roads"

[grid_roads]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

  0,  0, "r.road_isolated"

; Cardinal roads, connections north, south, east, west:

  0,  1, "r.c_road_n1e0s0w0"
  0,  2, "r.c_road_n0e1s0w0"
  0,  3, "r.c_road_n1e1s0w0"
  0,  4, "r.c_road_n0e0s1w0"
  0,  5, "r.c_road_n1e0s1w0"
  0,  6, "r.c_road_n0e1s1w0"
  0,  7, "r.c_road_n1e1s1w0"
  0,  8, "r.c_road_n0e0s0w1"
  0,  9, "r.c_road_n1e0s0w1"
  0, 10, "r.c_road_n0e1s0w1"
  0, 11, "r.c_road_n1e1s0w1"
  0, 12, "r.c_road_n0e0s1w1"
  0, 13, "r.c_road_n1e0s1w1"
  0, 14, "r.c_road_n0e1s1w1"
  0, 15, "r.c_road_n1e1s1w1"

; Diagonal roads, connections same, rotated 45 degrees clockwise:

  1,  1, "r.d_road_ne1se0sw0nw0"
  1,  2, "r.d_road_ne0se1sw0nw0"
  1,  3, "r.d_road_ne1se1sw0nw0"
  1,  4, "r.d_road_ne0se0sw1nw0"
  1,  5, "r.d_road_ne1se0sw1nw0"
  1,  6, "r.d_road_ne0se1sw1nw0"
  1,  7, "r.d_road_ne1se1sw1nw0"
  1,  8, "r.d_road_ne0se0sw0nw1"
  1,  9, "r.d_road_ne1se0sw0nw1"
  1, 10, "r.d_road_ne0se1sw0nw1"
  1, 11, "r.d_road_ne1se1sw0nw1"
  1, 12, "r.d_road_ne0se0sw1nw1"
  1, 13, "r.d_road_ne1se0sw1nw1"
  1, 14, "r.d_road_ne0se1sw1nw1"
  1, 15, "r.d_road_ne1se1sw1nw1"
}

[grid_rails]

x_top_left = 0
y_top_left = 60	  ; Change to 0 for original trident rails, 60 for dwp-style
dx = 30
dy = 30

tiles = { "row", "column", "tag"

  2,  0, "r.rail_isolated"

; Cardinal rails, connections north, south, east, west:

  2,  1, "r.c_rail_n1e0s0w0"
  2,  2, "r.c_rail_n0e1s0w0"
  2,  3, "r.c_rail_n1e1s0w0"
  2,  4, "r.c_rail_n0e0s1w0"
  2,  5, "r.c_rail_n1e0s1w0"
  2,  6, "r.c_rail_n0e1s1w0"
  2,  7, "r.c_rail_n1e1s1w0"
  2,  8, "r.c_rail_n0e0s0w1"
  2,  9, "r.c_rail_n1e0s0w1"
  2, 10, "r.c_rail_n0e1s0w1"
  2, 11, "r.c_rail_n1e1s0w1"
  2, 12, "r.c_rail_n0e0s1w1"
  2, 13, "r.c_rail_n1e0s1w1"
  2, 14, "r.c_rail_n0e1s1w1"
  2, 15, "r.c_rail_n1e1s1w1"

; Diagonal rails, connections same, rotated 45 degrees clockwise:

  3,  1, "r.d_rail_ne1se0sw0nw0"
  3,  2, "r.d_rail_ne0se1sw0nw0"
  3,  3, "r.d_rail_ne1se1sw0nw0"
  3,  4, "r.d_rail_ne0se0sw1nw0"
  3,  5, "r.d_rail_ne1se0sw1nw0"
  3,  6, "r.d_rail_ne0se1sw1nw0"
  3,  7, "r.d_rail_ne1se1sw1nw0"
  3,  8, "r.d_rail_ne0se0sw0nw1"
  3,  9, "r.d_rail_ne1se0sw0nw1"
  3, 10, "r.d_rail_ne0se1sw0nw1"
  3, 11, "r.d_rail_ne1se1sw0nw1"
  3, 12, "r.d_rail_ne0se0sw1nw1"
  3, 13, "r.d_rail_ne1se0sw1nw1"
  3, 14, "r.d_rail_ne0se1sw1nw1"
  3, 15, "r.d_rail_ne1se1sw1nw1"

; Rail corners

  2, 16, "r.c_rail_nw"
  2, 17, "r.c_rail_ne"
  3, 16, "r.c_rail_sw"
  3, 17, "r.c_rail_se"

}
