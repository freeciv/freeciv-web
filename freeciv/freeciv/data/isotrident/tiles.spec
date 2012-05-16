
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
"

[file]
gfx = "isotrident/tiles"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 64
dy = 32

tiles = { "row", "column", "tag"
; Unit activity letters:  (note unit icons have just "u.")

  0, 18, "unit.auto_attack",
         "unit.auto_settler"
  0, 19, "unit.stack"
  1, 17, "unit.loaded"
  1, 18, "unit.connect"
  1, 19, "unit.auto_explore"
  2, 12, "unit.transform"
  2, 13, "unit.sentry"
  2, 14, "unit.goto"
  2, 15, "unit.mine"
  2, 16, "unit.pollution"
  2, 17, "unit.road"
  2, 18, "unit.irrigate"
  2, 19, "unit.fortifying",
         "unit.fortress"
  3, 16, "unit.airbase"
  3, 17, "unit.pillage"
  3, 18, "unit.fortified"
  3, 19, "unit.fallout"
  4, 19, "unit.patrol"

; Unit hit-point bars: approx percent of hp remaining

  7,  0, "unit.hp_100"
  7,  1, "unit.hp_90"
  7,  2, "unit.hp_80"
  7,  3, "unit.hp_70"
  7,  4, "unit.hp_60"
  7,  5, "unit.hp_50"
  7,  6, "unit.hp_40"
  7,  7, "unit.hp_30"
  7,  8, "unit.hp_20"
  7,  9, "unit.hp_10"
  7, 10, "unit.hp_0"

; Numbers: city size:

  5,  0, "city.size_0"
  5,  1, "city.size_1"
  5,  2, "city.size_2"
  5,  3, "city.size_3"
  5,  4, "city.size_4"
  5,  5, "city.size_5"
  5,  6, "city.size_6"
  5,  7, "city.size_7"
  5,  8, "city.size_8"
  5,  9, "city.size_9"
  5, 10, "city.size_10"
  5, 11, "city.size_20"
  5, 12, "city.size_30"
  5, 13, "city.size_40"
  5, 14, "city.size_50"
  5, 15, "city.size_60"
  5, 16, "city.size_70"
  5, 17, "city.size_80"
  5, 18, "city.size_90"

; Numbers: city tile food/shields/trade y/g/b

  8,  0, "city.t_food_0"
  8,  1, "city.t_food_1"
  8,  2, "city.t_food_2"
  8,  3, "city.t_food_3"
  8,  4, "city.t_food_4"
  8,  5, "city.t_food_5"
  8,  6, "city.t_food_6"
  8,  7, "city.t_food_7"
  8,  8, "city.t_food_8"
  8,  9, "city.t_food_9"

  6,  0, "city.t_shields_0"
  6,  1, "city.t_shields_1"
  6,  2, "city.t_shields_2"
  6,  3, "city.t_shields_3"
  6,  4, "city.t_shields_4"
  6,  5, "city.t_shields_5"
  6,  6, "city.t_shields_6"
  6,  7, "city.t_shields_7"
  6,  8, "city.t_shields_8"
  6,  9, "city.t_shields_9"

  6, 10, "city.t_trade_0"
  6, 11, "city.t_trade_1"
  6, 12, "city.t_trade_2"
  6, 13, "city.t_trade_3"
  6, 14, "city.t_trade_4"
  6, 15, "city.t_trade_5"
  6, 16, "city.t_trade_6"
  6, 17, "city.t_trade_7"
  6, 18, "city.t_trade_8"
  6, 19, "city.t_trade_9"

; Unit upkeep in city dialog:
; These should probably be handled differently and have
; a different size...

  7, 12, "upkeep.gold"
  7, 13, "upkeep.gold2"
  7, 15, "upkeep.food"
  7, 16, "upkeep.food2"
  7, 17, "upkeep.unhappy"
  7, 18, "upkeep.unhappy2"
  7, 19, "upkeep.shield"

; Nuclear explosion: this could maybe now be handled as one 
; big graphic (?), but for now is done old way as 3 by 3:

  0,  0, "explode.nuke_00"
  0,  1, "explode.nuke_01"
  0,  2, "explode.nuke_02"
  1,  0, "explode.nuke_10"
  1,  1, "explode.nuke_11"
  1,  2, "explode.nuke_12"
  2,  0, "explode.nuke_20"
  2,  1, "explode.nuke_21"
  2,  2, "explode.nuke_22"

}
