
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Hogne Håskjold
    Tim F. Smith <yoohootim@hotmail.com>
    Yautja
    Daniel Speyer
    Eleazar"

[file]
gfx = "amplio/water"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 48
pixel_border = 1

tiles = { "row", "column", "tag"

; Rivers (as special type), and whether north, south, east, west 
; also has river or is ocean:

 2,  0, "tx.s_river_n0e0s0w0"
 2,  1, "tx.s_river_n0e0s0w1"
 2,  2, "tx.s_river_n0e0s1w0"
 2,  3, "tx.s_river_n0e0s1w1"
 2,  4, "tx.s_river_n0e1s0w0"
 2,  5, "tx.s_river_n0e1s0w1"
 2,  6, "tx.s_river_n0e1s1w0"
 2,  7, "tx.s_river_n0e1s1w1"
 3,  0, "tx.s_river_n1e0s0w0"
 3,  1, "tx.s_river_n1e0s0w1"
 3,  2, "tx.s_river_n1e0s1w0"
 3,  3, "tx.s_river_n1e0s1w1"
 3,  4, "tx.s_river_n1e1s0w0"
 3,  5, "tx.s_river_n1e1s0w1"
 3,  6, "tx.s_river_n1e1s1w0"
 3,  7, "tx.s_river_n1e1s1w1"

; Rivers as overlay

 2,  0, "t.t_river_n0e0s0w0"
 2,  1, "t.t_river_n0e0s0w1"
 2,  2, "t.t_river_n0e0s1w0"
 2,  3, "t.t_river_n0e0s1w1"
 2,  4, "t.t_river_n0e1s0w0"
 2,  5, "t.t_river_n0e1s0w1"
 2,  6, "t.t_river_n0e1s1w0"
 2,  7, "t.t_river_n0e1s1w1"
 3,  0, "t.t_river_n1e0s0w0"
 3,  1, "t.t_river_n1e0s0w1"
 3,  2, "t.t_river_n1e0s1w0"
 3,  3, "t.t_river_n1e0s1w1"
 3,  4, "t.t_river_n1e1s0w0"
 3,  5, "t.t_river_n1e1s0w1"
 3,  6, "t.t_river_n1e1s1w0"
 3,  7, "t.t_river_n1e1s1w1"

;river outlets

 10, 0, "tx.river_outlet_n"
 10, 1, "tx.river_outlet_e"
 10, 2, "tx.river_outlet_s"
 10, 3, "tx.river_outlet_w"}


[grid_coasts]

x_top_left = 1
y_top_left = 645
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
 11, 15, "t.l0.lake_cell_r_l_l_l"}
