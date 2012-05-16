
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Jason Dorje Short <jdorje@freeciv.org>
; adapted to hex2t
    Daniel Markstedt <himasaram@spray.se>
"

[file]
gfx = "hex2t/grid"

[grid_main]

x_top_left = 2
y_top_left = 2
dx = 40
dy = 72
pixel_border = 2

tiles = { "row", "column", "tag"
  0, 1, "grid.main.lr"
  0, 2, "grid.main.we"
  0, 3, "grid.main.ns"

  1, 1, "grid.city.lr"
  1, 2, "grid.city.we"
  1, 3, "grid.city.ns"

  2, 1, "grid.worked.lr"
  2, 2, "grid.worked.we"
  2, 3, "grid.worked.ns"

  3, 3, "grid.unavailable"

  4, 1, "grid.selected.lr"
  4, 2, "grid.selected.we"
  4, 3, "grid.selected.ns"

  5, 1, "grid.coastline.lr"
  5, 2, "grid.coastline.we"
  5, 3, "grid.coastline.ns"

  0, 0, "grid.borders.e"
  1, 0, "grid.borders.w"
  2, 0, "grid.borders.n"
  3, 0, "grid.borders.s"
  4, 0, "grid.borders.l"
  5, 0, "grid.borders.r"
}
