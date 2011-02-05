
[spec]

; Format and options of this spec file:
options =   "+Freeciv-2.3-spec"

[info]

artists =   "
    Hogne Håskjold
    Tim F. Smith <yoohootim@hotmail.com>
    Yautja
    Daniel Speyer
    Eleazar
  "

[file]
gfx =   "amplio2/water"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 48
pixel_border = 1

tiles = {   "row", "column", "tag"

; Rivers (as special type), and whether north, south, east, west 
; also has river or is ocean:

 }


[grid_coasts]

x_top_left = 1
y_top_left = 437
dx = 48
dy = 24
pixel_border = 1

tiles = {   "row", "column", "tag"

 0, 0,    "t.l1.coast_cell_u_w_w_w" ;vacant cell
 0, 2,    "t.l1.coast_cell_u_i_w_w"
 0, 4,    "t.l1.coast_cell_u_w_i_w"
 0, 6,    "t.l1.coast_cell_u_i_i_w"
 0, 8,    "t.l1.coast_cell_u_w_w_i"
 0, 10,   "t.l1.coast_cell_u_i_w_i"
 0, 12,   "t.l1.coast_cell_u_w_i_i"
 0, 14,   "t.l1.coast_cell_u_i_i_i"
 
 1, 0,    "t.l1.coast_cell_d_w_w_w" ;vacant cell
 1, 2,    "t.l1.coast_cell_d_i_w_w"
 1, 4,    "t.l1.coast_cell_d_w_i_w"
 1, 6,    "t.l1.coast_cell_d_i_i_w"
 1, 8,    "t.l1.coast_cell_d_w_w_i"
 1, 10,   "t.l1.coast_cell_d_i_w_i"
 1, 12,   "t.l1.coast_cell_d_w_i_i"
 1, 14,   "t.l1.coast_cell_d_i_i_i"

 2, 0,    "t.l1.coast_cell_l_w_w_w" ;vacant cell
 2, 2,    "t.l1.coast_cell_l_i_w_w"
 2, 4,    "t.l1.coast_cell_l_w_i_w"
 2, 6,    "t.l1.coast_cell_l_i_i_w"
 2, 8,    "t.l1.coast_cell_l_w_w_i"
 2, 10,   "t.l1.coast_cell_l_i_w_i"
 2, 12,   "t.l1.coast_cell_l_w_i_i"
 2, 14,   "t.l1.coast_cell_l_i_i_i"

 2, 1,    "t.l1.coast_cell_r_w_w_w"
 2, 3,    "t.l1.coast_cell_r_i_w_w"
 2, 5,    "t.l1.coast_cell_r_w_i_w"
 2, 7,    "t.l1.coast_cell_r_i_i_w"
 2, 9,    "t.l1.coast_cell_r_w_w_i"
 2, 11,   "t.l1.coast_cell_r_i_w_i"
 2, 13,   "t.l1.coast_cell_r_w_i_i"
 2, 15,   "t.l1.coast_cell_r_i_i_i"

 }
