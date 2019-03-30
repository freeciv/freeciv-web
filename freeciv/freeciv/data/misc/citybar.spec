
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
     Freim <...>
     Madeline Book <madeline.book@gmail.com> (citybar.trade)
"

[file]
gfx = "misc/citybar"

[grid_big]

x_top_left = 1
y_top_left = 1
pixel_border = 1
dx = 18
dy = 18

tiles = { "row", "column", "tag"

  0,  0, "citybar.shields"
  0,  1, "citybar.food"
  0,  2, "citybar.trade"

}

[grid_star]

x_top_left = 1
y_top_left = 20
pixel_border = 1
dx = 11
dy = 18

tiles = { "row", "column", "tag"

  0,  0, "citybar.occupied"
  0,  1, "citybar.occupancy_0"
  0,  2, "citybar.occupancy_1"
  0,  3, "citybar.occupancy_2"
  0,  4, "citybar.occupancy_3"

}

[grid_bg]
x_top_left = 1
y_top_left = 47
pixel_border = 1
dx = 398
dy = 152

tiles = { "row", "column", "tag"

  0,  0, "citybar.background"

}
