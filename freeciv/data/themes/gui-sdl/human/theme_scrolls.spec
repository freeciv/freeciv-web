
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
  Hogne Håskjold <haskjold@gmail.com>
"

[file]
gfx = "themes/gui-sdl/human/theme_scrolls"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 16
pixel_border = 1

tiles = { "row", "column","tag"
;scroll buttons

  0,    0,  "theme.UP_scroll"
  1,    0,  "theme.DOWN_scroll"
; 2,    0,  "theme.RIGHT_scroll"
; 3,    0,  "theme.LEFT_scroll"
}
