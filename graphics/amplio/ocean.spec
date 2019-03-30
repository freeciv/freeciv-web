[spec]

options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]
; TODO: FIX ARTISTS
artists = "
    Hogne Håskjold
    Tim F. Smith <yoohootim@hotmail.com>
    Daniel Speyer
    Yautja
    CapTVK
    Eleazar
    William Allen Simpson
"

[file]
gfx = "amplio/ocean"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 96
dy = 48
pixel_border = 0

tiles = { "row", "column", "tag"

;land, shallow(coast/shelf/ridge/vent), deep(floor/trench)
  8, 8, "t.l0.cellgroup_d_d_d_d"
  8, 7, "t.l0.cellgroup_s_d_d_d"
  8, 6, "t.l0.cellgroup_l_d_d_d"
  8, 5, "t.l0.cellgroup_d_d_d_s"
  8, 4, "t.l0.cellgroup_s_d_d_s"
  8, 3, "t.l0.cellgroup_l_d_d_s"
  8, 2, "t.l0.cellgroup_d_d_d_l"
  8, 1, "t.l0.cellgroup_s_d_d_l"
  8, 0, "t.l0.cellgroup_l_d_d_l"
  7, 8, "t.l0.cellgroup_d_s_d_d"
  7, 7, "t.l0.cellgroup_s_s_d_d"
  7, 6, "t.l0.cellgroup_l_s_d_d"
  7, 5, "t.l0.cellgroup_d_s_d_s"
  7, 4, "t.l0.cellgroup_s_s_d_s"
  7, 3, "t.l0.cellgroup_l_s_d_s"
  7, 2, "t.l0.cellgroup_d_s_d_l"
  7, 1, "t.l0.cellgroup_s_s_d_l"
  7, 0, "t.l0.cellgroup_l_s_d_l"
  6, 8, "t.l0.cellgroup_d_l_d_d"
  6, 7, "t.l0.cellgroup_s_l_d_d"
  6, 6, "t.l0.cellgroup_l_l_d_d"
  6, 5, "t.l0.cellgroup_d_l_d_s"
  6, 4, "t.l0.cellgroup_s_l_d_s"
  6, 3, "t.l0.cellgroup_l_l_d_s"
  6, 2, "t.l0.cellgroup_d_l_d_l"
  6, 1, "t.l0.cellgroup_s_l_d_l"
  6, 0, "t.l0.cellgroup_l_l_d_l"
  5, 8, "t.l0.cellgroup_d_d_s_d"
  5, 7, "t.l0.cellgroup_s_d_s_d"
  5, 6, "t.l0.cellgroup_l_d_s_d"
  5, 5, "t.l0.cellgroup_d_d_s_s"
  5, 4, "t.l0.cellgroup_s_d_s_s"
  5, 3, "t.l0.cellgroup_l_d_s_s"
  5, 2, "t.l0.cellgroup_d_d_s_l"
  5, 1, "t.l0.cellgroup_s_d_s_l"
  5, 0, "t.l0.cellgroup_l_d_s_l"
  4, 8, "t.l0.cellgroup_d_s_s_d"
  4, 7, "t.l0.cellgroup_s_s_s_d"
  4, 6, "t.l0.cellgroup_l_s_s_d"
  4, 5, "t.l0.cellgroup_d_s_s_s"
  4, 4, "t.l0.cellgroup_s_s_s_s"
  4, 3, "t.l0.cellgroup_l_s_s_s"
  4, 2, "t.l0.cellgroup_d_s_s_l"
  4, 1, "t.l0.cellgroup_s_s_s_l"
  4, 0, "t.l0.cellgroup_l_s_s_l"
  3, 8, "t.l0.cellgroup_d_l_s_d"
  3, 7, "t.l0.cellgroup_s_l_s_d"
  3, 6, "t.l0.cellgroup_l_l_s_d"
  3, 5, "t.l0.cellgroup_d_l_s_s"
  3, 4, "t.l0.cellgroup_s_l_s_s"
  3, 3, "t.l0.cellgroup_l_l_s_s"
  3, 2, "t.l0.cellgroup_d_l_s_l"
  3, 1, "t.l0.cellgroup_s_l_s_l"
  3, 0, "t.l0.cellgroup_l_l_s_l"
  2, 8, "t.l0.cellgroup_d_d_l_d"
  2, 7, "t.l0.cellgroup_s_d_l_d"
  2, 6, "t.l0.cellgroup_l_d_l_d"
  2, 5, "t.l0.cellgroup_d_d_l_s"
  2, 4, "t.l0.cellgroup_s_d_l_s"
  2, 3, "t.l0.cellgroup_l_d_l_s"
  2, 2, "t.l0.cellgroup_d_d_l_l"
  2, 1, "t.l0.cellgroup_s_d_l_l"
  2, 0, "t.l0.cellgroup_l_d_l_l"
  1, 8, "t.l0.cellgroup_d_s_l_d"
  1, 7, "t.l0.cellgroup_s_s_l_d"
  1, 6, "t.l0.cellgroup_l_s_l_d"
  1, 5, "t.l0.cellgroup_d_s_l_s"
  1, 4, "t.l0.cellgroup_s_s_l_s"
  1, 3, "t.l0.cellgroup_l_s_l_s"
  1, 2, "t.l0.cellgroup_d_s_l_l"
  1, 1, "t.l0.cellgroup_s_s_l_l"
  1, 0, "t.l0.cellgroup_l_s_l_l"
  0, 8, "t.l0.cellgroup_d_l_l_d"
  0, 7, "t.l0.cellgroup_s_l_l_d"
  0, 6, "t.l0.cellgroup_l_l_l_d"
  0, 5, "t.l0.cellgroup_d_l_l_s"
  0, 4, "t.l0.cellgroup_s_l_l_s"
  0, 3, "t.l0.cellgroup_l_l_l_s"
  0, 2, "t.l0.cellgroup_d_l_l_l"
  0, 1, "t.l0.cellgroup_s_l_l_l"
  0, 0, "t.l0.cellgroup_l_l_l_l"
}
