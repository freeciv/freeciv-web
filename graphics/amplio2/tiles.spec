
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
"

[file]
gfx = "amplio2/tiles"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 48
pixel_border = 1

tiles = { "row", "column", "tag"
; Unit hit-point bars: approx percent of hp remaining

  0,  0, "unit.hp_100"
  0,  1, "unit.hp_90"
  0,  2, "unit.hp_80"
  0,  3, "unit.hp_70"
  0,  4, "unit.hp_60"
  0,  5, "unit.hp_50"
  0,  6, "unit.hp_40"
  0,  7, "unit.hp_30"
  0,  8, "unit.hp_20"
  0,  9, "unit.hp_10"
  0, 10, "unit.hp_0"

; Numbers: city size: (also used for goto)


  1,  0, "city.size_000"
  1,  1, "city.size_100"
  1,  2, "city.size_200"
  1,  3, "city.size_300"
  1,  4, "city.size_400"
  1,  5, "city.size_500"
  1,  6, "city.size_600"
  1,  7, "city.size_700"
  1,  8, "city.size_800"
  1,  9, "city.size_900"

  2, 0, "city.size_00"
  2, 1, "city.size_10"
  2, 2, "city.size_20"
  2, 3, "city.size_30"
  2, 4, "city.size_40"
  2, 5, "city.size_50"
  2, 6, "city.size_60"
  2, 7, "city.size_70"
  2, 8, "city.size_80"
  2, 9, "city.size_90"

  3,  0, "city.size_0"
  3,  1, "city.size_1"
  3,  2, "city.size_2"
  3,  3, "city.size_3"
  3,  4, "city.size_4"
  3,  5, "city.size_5"
  3,  6, "city.size_6"
  3,  7, "city.size_7"
  3,  8, "city.size_8"
  3,  9, "city.size_9"

; Numbers: city tile food/shields/trade y/g/b

  4,  0, "city.t_food_0"
  4,  1, "city.t_food_1"
  4,  2, "city.t_food_2"
  4,  3, "city.t_food_3"
  4,  4, "city.t_food_4"
  4,  5, "city.t_food_5"
  4,  6, "city.t_food_6"
  4,  7, "city.t_food_7"
  4,  8, "city.t_food_8"
  4,  9, "city.t_food_9"

  5,  0, "city.t_shields_0"
  5,  1, "city.t_shields_1"
  5,  2, "city.t_shields_2"
  5,  3, "city.t_shields_3"
  5,  4, "city.t_shields_4"
  5,  5, "city.t_shields_5"
  5,  6, "city.t_shields_6"
  5,  7, "city.t_shields_7"
  5,  8, "city.t_shields_8"
  5,  9, "city.t_shields_9"

  6, 0, "city.t_trade_0"
  6, 1, "city.t_trade_1"
  6, 2, "city.t_trade_2"
  6, 3, "city.t_trade_3"
  6, 4, "city.t_trade_4"
  6, 5, "city.t_trade_5"
  6, 6, "city.t_trade_6"
  6, 7, "city.t_trade_7"
  6, 8, "city.t_trade_8"
  6, 9, "city.t_trade_9"

; Unit Extras(not activities)

  3, 10, "unit.connect"
  4, 10, "unit.auto_attack",
         "unit.auto_settler"
  5, 10, "unit.stack"
  6, 10, "unit.loaded"

}
