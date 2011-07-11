
[spec]

; Format and options of this spec file:
options =  "+Freeciv-2.3-spec"

[info]

artists =  "
    Hogne Håskjold
    Tim F. Smith <yoohootim@hotmail.com>
    Yautja
    Daniel Speyer
    Eleazar
 "

[file]
gfx =  "amplio2/water"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 48
pixel_border = 1

tiles = {  "row", "column", "tag"

; Rivers (as special type), and whether north, south, east, west 
; also has river or is ocean:

 2,  0,  "tx.s_river_n0e0s0w0"
 2,  1,  "tx.s_river_n0e0s0w1"
 2,  2,  "tx.s_river_n0e0s1w0"
 2,  3,  "tx.s_river_n0e0s1w1"
 2,  4,  "tx.s_river_n0e1s0w0"
 2,  5,  "tx.s_river_n0e1s0w1"
 2,  6,  "tx.s_river_n0e1s1w0"
 2,  7,  "tx.s_river_n0e1s1w1"
 3,  0,  "tx.s_river_n1e0s0w0"
 3,  1,  "tx.s_river_n1e0s0w1"
 3,  2,  "tx.s_river_n1e0s1w0"
 3,  3,  "tx.s_river_n1e0s1w1"
 3,  4,  "tx.s_river_n1e1s0w0"
 3,  5,  "tx.s_river_n1e1s0w1"
 3,  6,  "tx.s_river_n1e1s1w0"
 3,  7,  "tx.s_river_n1e1s1w1"

; Rivers as overlay

; 2,  0,  "t.t_river_n0e0s0w0"
; 2,  1,  "t.t_river_n0e0s0w1"
; 2,  2,  "t.t_river_n0e0s1w0"
; 2,  3,  "t.t_river_n0e0s1w1"
; 2,  4,  "t.t_river_n0e1s0w0"
; 2,  5,  "t.t_river_n0e1s0w1"
; 2,  6,  "t.t_river_n0e1s1w0"
; 2,  7,  "t.t_river_n0e1s1w1"
; 3,  0,  "t.t_river_n1e0s0w0"
; 3,  1,  "t.t_river_n1e0s0w1"
; 3,  2,  "t.t_river_n1e0s1w0"
; 3,  3,  "t.t_river_n1e0s1w1"
; 3,  4,  "t.t_river_n1e1s0w0"
; 3,  5,  "t.t_river_n1e1s0w1"
; 3,  6,  "t.t_river_n1e1s1w0"
; 3,  7,  "t.t_river_n1e1s1w1"

;river outlets

 4,  0,  "tx.river_outlet_n"
 4,  1,  "tx.river_outlet_e"
 4,  2,  "tx.river_outlet_s"
 4,  3,  "tx.river_outlet_w"
 }

; removed tiles which are not used by webclient here:

