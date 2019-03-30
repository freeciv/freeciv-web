
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
	Pep
"

[file]
gfx = "misc/editor"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 30
dy = 30
pixel_border = 1

tiles = { "row", "column", "tag"
  1,  0, "editor.erase_2"
  1,  1, "editor.brush"
  1,  2, "editor.startpos"
  1,  3, "editor.terrain_2"
  1,  4, "editor.terrain_resource"
  1,  5, "editor.terrain_special_1"
  1,  6, "editor.terrain_special_2"
  1,  7, "editor.terrain_special"
  1,  8, "editor.terrain_special_4"
  1,  9, "editor.terrain_special_5"
  2,  0, "editor.military_base"
  2,  1, "editor.unit_1"
  2,  2, "editor.city"
  2,  3, "editor.vision"
  2,  4, "editor.territory"
  2,  4, "editor.road"
  2,  5, "editor.properties"
  2,  6, "editor.copypaste"
  2,  7, "editor.unit"
  2,  8, "editor.unit_3"
  2,  9, "editor.unit_4"
  3,  4, "editor.erase"
  3,  5, "editor.terrain"
  3,  6, "editor.fill"
  3,  7, "editor.tiles_1"
  3,  8, "editor.tiles_2"
  3,  9, "editor.terrain_resource_2"
}

[grid_mini]

x_top_left = 1
y_top_left = 125
dx = 16
dy = 16
pixel_border = 1

tiles = { "row", "column", "tag"
  0,  0, "editor.mini_p" 
  0,  1, "editor.mini_m" 
  0,  2, "editor.mini_d" 
  0,  3, "editor.mini_g" 
  0,  4, "editor.mini_o" 
  0,  5, "editor.mini_h" 
  0,  6, "editor.mini_r" 
  0,  7, "editor.mini_s" 
  0,  8, "editor.mini_f" 
  0,  9, "editor.mini_t" 
  0, 10, "editor.mini_j" 
  0, 11, "editor.mini_gl"
  0, 12, "editor.mini_erase"
  0, 13, "editor.mini_freeciv"
}

[grid_extra]

x_top_left = 1
y_top_left = 142
dx = 30
dy = 30
pixel_border = 1

tiles = { "row", "column", "tag"
  0,  0, "editor.draw_line"
  0,  1, "editor.fill_enclosed"
  0,  2, "editor.tiny_brush"
  0,  3, "editor.type_paint"
  0,  4, "editor.bitmap_to_map"
  0,  5, "editor.copy"
  0,  6, "editor.paste"
}
