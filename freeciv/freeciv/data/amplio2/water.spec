
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
gfx = "amplio2/water"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 48
pixel_border = 1

tiles = { "row", "column", "tag"

; Rivers (as special type), and whether north, south, east, west 
; also has river or is ocean:

 2,  0, "road.river_s_n0e0s0w0"
 2,  1, "road.river_s_n0e0s0w1"
 2,  2, "road.river_s_n0e0s1w0"
 2,  3, "road.river_s_n0e0s1w1"
 2,  4, "road.river_s_n0e1s0w0"
 2,  5, "road.river_s_n0e1s0w1"
 2,  6, "road.river_s_n0e1s1w0"
 2,  7, "road.river_s_n0e1s1w1"
 3,  0, "road.river_s_n1e0s0w0"
 3,  1, "road.river_s_n1e0s0w1"
 3,  2, "road.river_s_n1e0s1w0"
 3,  3, "road.river_s_n1e0s1w1"
 3,  4, "road.river_s_n1e1s0w0"
 3,  5, "road.river_s_n1e1s0w1"
 3,  6, "road.river_s_n1e1s1w0"
 3,  7, "road.river_s_n1e1s1w1"

; River outlets

 4,  0, "road.river_outlet_n"
 4,  1, "road.river_outlet_e"
 4,  2, "road.river_outlet_s"
 4,  3, "road.river_outlet_w"
}


[grid_coasts]

x_top_left = 1
y_top_left = 437
dx = 48
dy = 24
pixel_border = 1

tiles = { "row", "column", "tag"

 0, 0,  "t.l1.coast_cell_u_w_w_w" ;vacant cell
 0, 2,  "t.l1.coast_cell_u_i_w_w"
 0, 4,  "t.l1.coast_cell_u_w_i_w"
 0, 6,  "t.l1.coast_cell_u_i_i_w"
 0, 8,  "t.l1.coast_cell_u_w_w_i"
 0, 10, "t.l1.coast_cell_u_i_w_i"
 0, 12, "t.l1.coast_cell_u_w_i_i"
 0, 14, "t.l1.coast_cell_u_i_i_i"
 
 1, 0,  "t.l1.coast_cell_d_w_w_w" ;vacant cell
 1, 2,  "t.l1.coast_cell_d_i_w_w"
 1, 4,  "t.l1.coast_cell_d_w_i_w"
 1, 6,  "t.l1.coast_cell_d_i_i_w"
 1, 8,  "t.l1.coast_cell_d_w_w_i"
 1, 10, "t.l1.coast_cell_d_i_w_i"
 1, 12, "t.l1.coast_cell_d_w_i_i"
 1, 14, "t.l1.coast_cell_d_i_i_i"

 2, 0,  "t.l1.coast_cell_l_w_w_w" ;vacant cell
 2, 2,  "t.l1.coast_cell_l_i_w_w"
 2, 4,  "t.l1.coast_cell_l_w_i_w"
 2, 6,  "t.l1.coast_cell_l_i_i_w"
 2, 8,  "t.l1.coast_cell_l_w_w_i"
 2, 10, "t.l1.coast_cell_l_i_w_i"
 2, 12, "t.l1.coast_cell_l_w_i_i"
 2, 14, "t.l1.coast_cell_l_i_i_i"

 2, 1,  "t.l1.coast_cell_r_w_w_w"
 2, 3,  "t.l1.coast_cell_r_i_w_w"
 2, 5,  "t.l1.coast_cell_r_w_i_w"
 2, 7,  "t.l1.coast_cell_r_i_i_w"
 2, 9,  "t.l1.coast_cell_r_w_w_i"
 2, 11, "t.l1.coast_cell_r_i_w_i"
 2, 13, "t.l1.coast_cell_r_w_i_i"
 2, 15, "t.l1.coast_cell_r_i_i_i"

; deep ocean cell sprites:
 0, 0,  "t.l1.floor_cell_u_w_w_w" ;vacant cell
 0, 2,  "t.l1.floor_cell_u_i_w_w"
 0, 4,  "t.l1.floor_cell_u_w_i_w"
 0, 6,  "t.l1.floor_cell_u_i_i_w"
 0, 8,  "t.l1.floor_cell_u_w_w_i"
 0, 10, "t.l1.floor_cell_u_i_w_i"
 0, 12, "t.l1.floor_cell_u_w_i_i"
 0, 14, "t.l1.floor_cell_u_i_i_i"
 
 1, 0,  "t.l1.floor_cell_d_w_w_w" ;vacant cell
 1, 2,  "t.l1.floor_cell_d_i_w_w"
 1, 4,  "t.l1.floor_cell_d_w_i_w"
 1, 6,  "t.l1.floor_cell_d_i_i_w"
 1, 8,  "t.l1.floor_cell_d_w_w_i"
 1, 10, "t.l1.floor_cell_d_i_w_i"
 1, 12, "t.l1.floor_cell_d_w_i_i"
 1, 14, "t.l1.floor_cell_d_i_i_i"

 2, 0,  "t.l1.floor_cell_l_w_w_w" ;vacant cell
 2, 2,  "t.l1.floor_cell_l_i_w_w"
 2, 4,  "t.l1.floor_cell_l_w_i_w"
 2, 6,  "t.l1.floor_cell_l_i_i_w"
 2, 8,  "t.l1.floor_cell_l_w_w_i"
 2, 10, "t.l1.floor_cell_l_i_w_i"
 2, 12, "t.l1.floor_cell_l_w_i_i"
 2, 14, "t.l1.floor_cell_l_i_i_i"

 2, 1,  "t.l1.floor_cell_r_w_w_w"
 2, 3,  "t.l1.floor_cell_r_i_w_w"
 2, 5,  "t.l1.floor_cell_r_w_i_w"
 2, 7,  "t.l1.floor_cell_r_i_i_w"
 2, 9,  "t.l1.floor_cell_r_w_w_i"
 2, 11, "t.l1.floor_cell_r_i_w_i"
 2, 13, "t.l1.floor_cell_r_w_i_i"
 2, 15, "t.l1.floor_cell_r_i_i_i"

; lake tiles
 9, 0,  "t.l0.lake_cell_u_s_s_s" ;vacant cell
 9, 2,  "t.l0.lake_cell_u_l_s_s"
 9, 4,  "t.l0.lake_cell_u_s_l_s"
 9, 6,  "t.l0.lake_cell_u_l_l_s"
 9, 8,  "t.l0.lake_cell_u_s_s_l"
 9, 10, "t.l0.lake_cell_u_l_s_l"
 9, 12, "t.l0.lake_cell_u_s_l_l"
 9, 14, "t.l0.lake_cell_u_l_l_l"
 
 10, 0,  "t.l0.lake_cell_d_s_s_s" ;vacant cell
 10, 2,  "t.l0.lake_cell_d_l_s_s"
 10, 4,  "t.l0.lake_cell_d_s_l_s"
 10, 6,  "t.l0.lake_cell_d_l_l_s"
 10, 8,  "t.l0.lake_cell_d_s_s_l"
 10, 10, "t.l0.lake_cell_d_l_s_l"
 10, 12, "t.l0.lake_cell_d_s_l_l"
 10, 14, "t.l0.lake_cell_d_l_l_l"

 11, 0,  "t.l0.lake_cell_l_s_s_s" ;vacant cell
 11, 2,  "t.l0.lake_cell_l_l_s_s"
 11, 4,  "t.l0.lake_cell_l_s_l_s"
 11, 6,  "t.l0.lake_cell_l_l_l_s"
 11, 8,  "t.l0.lake_cell_l_s_s_l"
 11, 10, "t.l0.lake_cell_l_l_s_l"
 11, 12, "t.l0.lake_cell_l_s_l_l"
 11, 14, "t.l0.lake_cell_l_l_l_l"

 11, 1,  "t.l0.lake_cell_r_s_s_s"
 11, 3,  "t.l0.lake_cell_r_l_s_s"
 11, 5,  "t.l0.lake_cell_r_s_l_s"
 11, 7,  "t.l0.lake_cell_r_l_l_s"
 11, 9,  "t.l0.lake_cell_r_s_s_l"
 11, 11, "t.l0.lake_cell_r_l_s_l"
 11, 13, "t.l0.lake_cell_r_s_l_l"
 11, 15, "t.l0.lake_cell_r_l_l_l"
}
