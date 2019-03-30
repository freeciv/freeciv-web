
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Jason Dorje Short <jdorje@freeciv.org>
"

[file]
gfx = "misc/overlays"

[grid_main]

x_top_left = 2
y_top_left = 2
dx = 128
dy = 64
pixel_border=2

tiles = { "row", "column", "tag"

  0, 0, "colors.overlay_0"
  0, 1, "colors.overlay_1"
  1, 0, "colors.overlay_2"
  1, 1, "colors.overlay_3"

  0, 2, "mask.worked_tile"
  1, 2, "mask.unworked_tile"
}
