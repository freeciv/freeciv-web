
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    GriffonSpade
"

[file]
gfx = "hexemplio/grid"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 126
dy = 64
pixel_border = 1

tiles = { "row", "column", "tag"
  0, 0, "grid.main.we"
  1, 0, "grid.main.ns"
  2, 0, "grid.main.ud"

  0, 1, "grid.city.we"
  1, 1, "grid.city.ns"
  2, 1, "grid.city.ud"

  0, 2, "grid.worked.we"
  1, 2, "grid.worked.ns"
  2, 2, "grid.worked.ud"

  0, 3, "grid.selected.we"
  1, 3, "grid.selected.ns"
  2, 3, "grid.selected.ud"

  0, 4, "grid.coastline.we"
  1, 4, "grid.coastline.ns"
  2, 4, "grid.coastline.ud"

  0, 5, "grid.borders.w"
  0, 6, "grid.borders.e"
  1, 5, "grid.borders.n"
  1, 6, "grid.borders.s"
  2, 5, "grid.borders.u"
  2, 6, "grid.borders.d"

  3, 0, "tx.darkness_n"
  3, 1, "tx.darkness_e"
  3, 2, "tx.darkness_se"
  3, 3, "tx.darkness_s"
  3, 4, "tx.darkness_w"
  3, 5, "tx.darkness_nw"

  3, 6, "grid.unavailable"
  3, 7, "grid.nonnative"

}
