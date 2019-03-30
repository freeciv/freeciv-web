
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    GriffonSpade
"

[file]
gfx = "alio/tunnels"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 126
dy = 64
pixel_border = 1

tiles = { "row", "column","tag"
; Burrow Tubes (as special type), and whether north, south, east, west 

 2,  0, "road.burrow_n0e0se0s0w0nw0"
 2,  1, "road.burrow_n1e0se0s0w0nw0"
 2,  2, "road.burrow_n0e1se0s0w0nw0"
 2,  3, "road.burrow_n1e1se0s0w0nw0"
 2,  4, "road.burrow_n0e0se0s1w0nw0"
 2,  5, "road.burrow_n1e0se0s1w0nw0"
 2,  6, "road.burrow_n0e1se0s1w0nw0"
 2,  7, "road.burrow_n1e1se0s1w0nw0"
 
 3,  0, "road.burrow_n0e0se0s0w1nw0"
 3,  1, "road.burrow_n1e0se0s0w1nw0"
 3,  2, "road.burrow_n0e1se0s0w1nw0"
 3,  3, "road.burrow_n1e1se0s0w1nw0"
 3,  4, "road.burrow_n0e0se0s1w1nw0"
 3,  5, "road.burrow_n1e0se0s1w1nw0"
 3,  6, "road.burrow_n0e1se0s1w1nw0"
 3,  7, "road.burrow_n1e1se0s1w1nw0"

 4,  0, "road.burrow_n0e0se1s0w0nw0"
 4,  1, "road.burrow_n1e0se1s0w0nw0"
 4,  2, "road.burrow_n0e1se1s0w0nw0"
 4,  3, "road.burrow_n1e1se1s0w0nw0"
 4,  4, "road.burrow_n0e0se1s1w0nw0"
 4,  5, "road.burrow_n1e0se1s1w0nw0"
 4,  6, "road.burrow_n0e1se1s1w0nw0"
 4,  7, "road.burrow_n1e1se1s1w0nw0"

 5,  0, "road.burrow_n0e0se1s0w1nw0"
 5,  1, "road.burrow_n1e0se1s0w1nw0"
 5,  2, "road.burrow_n0e1se1s0w1nw0"
 5,  3, "road.burrow_n1e1se1s0w1nw0"
 5,  4, "road.burrow_n0e0se1s1w1nw0"
 5,  5, "road.burrow_n1e0se1s1w1nw0"
 5,  6, "road.burrow_n0e1se1s1w1nw0"
 5,  7, "road.burrow_n1e1se1s1w1nw0"

 6,  0, "road.burrow_n0e0se0s0w0nw1"
 6,  1, "road.burrow_n1e0se0s0w0nw1"
 6,  2, "road.burrow_n0e1se0s0w0nw1"
 6,  3, "road.burrow_n1e1se0s0w0nw1"
 6,  4, "road.burrow_n0e0se0s1w0nw1"
 6,  5, "road.burrow_n1e0se0s1w0nw1"
 6,  6, "road.burrow_n0e1se0s1w0nw1"
 6,  7, "road.burrow_n1e1se0s1w0nw1"

 7,  0, "road.burrow_n0e0se0s0w1nw1"
 7,  1, "road.burrow_n1e0se0s0w1nw1"
 7,  2, "road.burrow_n0e1se0s0w1nw1"
 7,  3, "road.burrow_n1e1se0s0w1nw1"
 7,  4, "road.burrow_n0e0se0s1w1nw1"
 7,  5, "road.burrow_n1e0se0s1w1nw1"
 7,  6, "road.burrow_n0e1se0s1w1nw1"
 7,  7, "road.burrow_n1e1se0s1w1nw1"

 8,  0, "road.burrow_n0e0se1s0w0nw1"
 8,  1, "road.burrow_n1e0se1s0w0nw1"
 8,  2, "road.burrow_n0e1se1s0w0nw1"
 8,  3, "road.burrow_n1e1se1s0w0nw1"
 8,  4, "road.burrow_n0e0se1s1w0nw1"
 8,  5, "road.burrow_n1e0se1s1w0nw1"
 8,  6, "road.burrow_n0e1se1s1w0nw1"
 8,  7, "road.burrow_n1e1se1s1w0nw1"

 9,  0, "road.burrow_n0e0se1s0w1nw1"
 9,  1, "road.burrow_n1e0se1s0w1nw1"
 9,  2, "road.burrow_n0e1se1s0w1nw1"
 9,  3, "road.burrow_n1e1se1s0w1nw1"
 9,  4, "road.burrow_n0e0se1s1w1nw1"
 9,  5, "road.burrow_n1e0se1s1w1nw1"
 9,  6, "road.burrow_n0e1se1s1w1nw1"
 9,  7, "road.burrow_n1e1se1s1w1nw1"

; Tunnels (as special type), and whether north, south, east, west 

 10,  0, "road.tunnel_n0e0se0s0w0nw0"
 10,  1, "road.tunnel_n1e0se0s0w0nw0"
 10,  2, "road.tunnel_n0e1se0s0w0nw0"
 10,  3, "road.tunnel_n1e1se0s0w0nw0"
 10,  4, "road.tunnel_n0e0se0s1w0nw0"
 10,  5, "road.tunnel_n1e0se0s1w0nw0"
 10,  6, "road.tunnel_n0e1se0s1w0nw0"
 10,  7, "road.tunnel_n1e1se0s1w0nw0"

 11,  0, "road.tunnel_n0e0se0s0w1nw0"
 11,  1, "road.tunnel_n1e0se0s0w1nw0"
 11,  2, "road.tunnel_n0e1se0s0w1nw0"
 11,  3, "road.tunnel_n1e1se0s0w1nw0"
 11,  4, "road.tunnel_n0e0se0s1w1nw0"
 11,  5, "road.tunnel_n1e0se0s1w1nw0"
 11,  6, "road.tunnel_n0e1se0s1w1nw0"
 11,  7, "road.tunnel_n1e1se0s1w1nw0"

12,  0, "road.tunnel_n0e0se1s0w0nw0"
12,  1, "road.tunnel_n1e0se1s0w0nw0"
12,  2, "road.tunnel_n0e1se1s0w0nw0"
12,  3, "road.tunnel_n1e1se1s0w0nw0"
12,  4, "road.tunnel_n0e0se1s1w0nw0"
12,  5, "road.tunnel_n1e0se1s1w0nw0"
12,  6, "road.tunnel_n0e1se1s1w0nw0"
12,  7, "road.tunnel_n1e1se1s1w0nw0"

13,  0, "road.tunnel_n0e0se1s0w1nw0"
13,  1, "road.tunnel_n1e0se1s0w1nw0"
13,  2, "road.tunnel_n0e1se1s0w1nw0"
13,  3, "road.tunnel_n1e1se1s0w1nw0"
13,  4, "road.tunnel_n0e0se1s1w1nw0"
13,  5, "road.tunnel_n1e0se1s1w1nw0"
13,  6, "road.tunnel_n0e1se1s1w1nw0"
13,  7, "road.tunnel_n1e1se1s1w1nw0"

14,  0, "road.tunnel_n0e0se0s0w0nw1"
14,  1, "road.tunnel_n1e0se0s0w0nw1"
14,  2, "road.tunnel_n0e1se0s0w0nw1"
14,  3, "road.tunnel_n1e1se0s0w0nw1"
14,  4, "road.tunnel_n0e0se0s1w0nw1"
14,  5, "road.tunnel_n1e0se0s1w0nw1"
14,  6, "road.tunnel_n0e1se0s1w0nw1"
14,  7, "road.tunnel_n1e1se0s1w0nw1"

15,  0, "road.tunnel_n0e0se0s0w1nw1"
15,  1, "road.tunnel_n1e0se0s0w1nw1"
15,  2, "road.tunnel_n0e1se0s0w1nw1"
15,  3, "road.tunnel_n1e1se0s0w1nw1"
15,  4, "road.tunnel_n0e0se0s1w1nw1"
15,  5, "road.tunnel_n1e0se0s1w1nw1"
15,  6, "road.tunnel_n0e1se0s1w1nw1"
15,  7, "road.tunnel_n1e1se0s1w1nw1"

16,  0, "road.tunnel_n0e0se1s0w0nw1"
16,  1, "road.tunnel_n1e0se1s0w0nw1"
16,  2, "road.tunnel_n0e1se1s0w0nw1"
16,  3, "road.tunnel_n1e1se1s0w0nw1"
16,  4, "road.tunnel_n0e0se1s1w0nw1"
16,  5, "road.tunnel_n1e0se1s1w0nw1"
16,  6, "road.tunnel_n0e1se1s1w0nw1"
16,  7, "road.tunnel_n1e1se1s1w0nw1"

17,  0, "road.tunnel_n0e0se1s0w1nw1"
17,  1, "road.tunnel_n1e0se1s0w1nw1"
17,  2, "road.tunnel_n0e1se1s0w1nw1"
17,  3, "road.tunnel_n1e1se1s0w1nw1"
17,  4, "road.tunnel_n0e0se1s1w1nw1"
17,  5, "road.tunnel_n1e0se1s1w1nw1"
17,  6, "road.tunnel_n0e1se1s1w1nw1"
17,  7, "road.tunnel_n1e1se1s1w1nw1"

}

