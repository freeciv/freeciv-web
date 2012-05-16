
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Hogne Håskjold <haskjold@gmail.com>
"

[file]
gfx = "themes/gui-sdl/human/theme_boxs"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 80
dy = 80
pixel_border = 1

tiles = { "row", "column","tag"

  0,    0,  "theme.edit"
  0,    1,  "theme.button"
}

[grid_horiz]

x_top_left = 1
y_top_left = 82
dx = 80
dy = 20
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "theme.horiz_scrollbar"
  1, 0, "theme.sbox"
  2, 0, "theme.ubox"
}

[grid_vertic]

x_top_left = 82
y_top_left = 82
dx = 20
dy = 80
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "theme.vertic_scrollbar"
  0, 1, "theme.block"
}

[grid_left_frame]

x_top_left = 166
y_top_left = 1
dx = 3
dy = 164
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "theme.left_frame"
}

[grid_right_frame]

x_top_left = 170
y_top_left = 1
dx = 3
dy = 164
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "theme.right_frame"
}

[grid_top_frame]

x_top_left = 1
y_top_left = 166
dx = 164
dy = 3
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "theme.top_frame"
}

[grid_bottom_frame]

x_top_left = 1
y_top_left = 170
dx = 164
dy = 3
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "theme.bottom_frame"
}
