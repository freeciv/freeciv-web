[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Hogne Håskjold <haskjold@gmail.com>
    Light bulb and sign from Ubuntu Human theme
    Skull from Battle for Wesnoth
"

[file]
gfx = "themes/gui-sdl/human/icons"

[grid_polluttion]

x_top_left = 48
y_top_left = 34
dx = 32
dy = 32
pixel_border = 1

tiles = { "row", "column", "tag"

; used by all city styles

  0,  0, "city.pollution"

}

[grid_city_resource]

x_top_left = 1
y_top_left = 1
dx = 14
dy = 14
pixel_border = 1

tiles = { "row", "column", "tag"
  0,  0, "city.food_waste"
  0,  1, "city.shield_waste"
  0,  2, "city.trade_waste"
  1,  0, "city.food"
  1,  1, "city.shield"
  1,  2, "city.trade"
  0,  3, "city.lux"
  0,  4, "city.coin"
  0,  5, "city.colb"
  1,  3, "city.red_face"
  1,  4, "city.dark_coin"
  1,  5, "city.unkeep_coin"

}

[grid_city_small_resource]

x_top_left = 1
y_top_left = 34
dx = 10
dy = 10
pixel_border = 1

tiles = { "row", "column", "tag"
  0,  0, "city.small_food"
  0,  1, "city.small_shield"
  0,  2, "city.small_trade"
  0,  3, "city.small_red_face"
  1,  0, "city.small_lux"
  1,  1, "city.small_coin"
  1,  2, "city.small_colb"

}
