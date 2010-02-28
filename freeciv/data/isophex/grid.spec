
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Jason Dorje Short <jdorje@freeciv.org>
"

[file]
gfx = "isophex/grid"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 32
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

  0, 3, "grid.unavailable"

  0, 4, "grid.selected.we"
  1, 4, "grid.selected.ns"
  2, 4, "grid.selected.ud"

  0, 5, "grid.coastline.we"
  1, 5, "grid.coastline.ns"
  2, 5, "grid.coastline.ud"

  3, 0, "grid.borders.n"
  3, 1, "grid.borders.s"
  3, 2, "grid.borders.w"
  3, 3, "grid.borders.e"
  3, 4, "grid.borders.u"
  3, 5, "grid.borders.d"
}
