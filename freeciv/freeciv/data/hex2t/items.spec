
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jerzy Klek <jekl@altavista.net>
    Marco Saupe <msaupe@saale-net.de> (reworked classic, industrial and modern)
    Jacob Nevins (outpost)
; converted to hex:
    Tommy <yobbo3@hotmail.com>
    Eleazar (buoy)
    Vincent Croisier <vincent.croisier@advalvas.be> (ruins)
"

[file]
gfx = "hex2t/items"

[grid_main]

x_top_left = 2
y_top_left = 2
dx = 40
dy = 72
pixel_border = 2

tiles = { "row", "column", "tag"

; cities:

; used by all city styles

 1,  0, "city.disorder"
 1,  1, "base.airbase_mg"
 1,  2, "tx.airbase_full"
 1,  3, "base.airstrip_mg"
 1,  5, "base.buoy_mg"
 1,  6, "extra.ruins_mg"
 3,  8, "base.outpost_fg"
 3,  9, "base.outpost_bg"
 4,  8, "base.fortress_fg"
 4,  9, "base.fortress_bg"

; default city tiles
 2,  2, "cd.city"
 2,  3, "cd.city_wall"
 2,  4, "cd.occupied"

; city tiles

 2,  0, "city.european_city_0"
 2,  1, "city.european_city_1"
 2,  2, "city.european_city_2"
 ;2,  3, "city.european_wall"
 2,  4, "city.european_occupied_0"
 2,  5, "city.european_wall_0"
 2,  6, "city.european_wall_1"
 2,  7, "city.european_wall_2"
   
 3,  0, "city.industrial_city_0"
 3,  1, "city.industrial_city_1"
 3,  2, "city.industrial_city_2"
 ;3,  3, "city.industrial_wall"
 3,  4, "city.industrial_occupied_0"
 3,  5, "city.industrial_wall_0"
 3,  6, "city.industrial_wall_1"
 3,  7, "city.industrial_wall_2"
   
 9,  0, "city.electricage_city_0"
 9,  1, "city.electricage_city_1"
 9,  2, "city.electricage_city_2"
 ;9,  3, "city.electricage_wall"
 9,  4, "city.electricage_occupied_0"
 9,  5, "city.electricage_wall_0"
 9,  6, "city.electricage_wall_1"
 9,  7, "city.electricage_wall_2"
   
 4,  0, "city.modern_city_0"
 4,  1, "city.modern_city_1"
 4,  2, "city.modern_city_2"
 ;4,  3, "city.modern_wall"
 4,  4, "city.modern_occupied_0"
 4,  5, "city.modern_wall_0"
 4,  6, "city.modern_wall_1"
 4,  7, "city.modern_wall_2"
   
 5,  0, "city.postmodern_city_0"
 5,  1, "city.postmodern_city_1"
 5,  2, "city.postmodern_city_2"
 ;5,  3, "city.postmodern_wall"
 5,  4, "city.postmodern_occupied_0"
 5,  5, "city.postmodern_wall_0"
 5,  6, "city.postmodern_wall_1"
 5,  7, "city.postmodern_wall_2"

 6,  0, "city.classical_city_0"
 6,  1, "city.classical_city_1"
 6,  2, "city.classical_city_2"
 ;6,  3, "city.classical_wall"
 6,  4, "city.classical_occupied_0"
 6,  5, "city.classical_wall_0"
 6,  6, "city.classical_wall_1"
 6,  7, "city.classical_wall_2"
   
 7,  0, "city.asian_city_0"
 7,  1, "city.asian_city_1"
 7,  2, "city.asian_city_2"
 ;7,  3, "city.asian_wall"
 7,  4, "city.asian_occupied_0"
 7,  5, "city.asian_wall_0"
 7,  6, "city.asian_wall_1"
 7,  7, "city.asian_wall_2"
   

 8,  0, "city.tropical_city_0"
 8,  1, "city.tropical_city_1"
 8,  2, "city.tropical_city_2"
 ;8,  3, "city.tropical_wall"
 8,  4, "city.tropical_occupied_0"
 8,  5, "city.tropical_wall_0"
 8,  6, "city.tropical_wall_1"
 8,  7, "city.tropical_wall_2"

; Numbers: city size: (also used for goto)

  3,  10, "city.size_0"
  3,  11, "city.size_1"
  3,  12, "city.size_2"
  3,  13, "city.size_3"
  3,  14, "city.size_4"
  3,  15, "city.size_5"
  3,  16, "city.size_6"
  3,  17, "city.size_7"
  3,  18, "city.size_8"
  3,  19, "city.size_9"

  4,  10, "city.size_00"
  4,  11, "city.size_10"
  4,  12, "city.size_20"
  4,  13, "city.size_30"
  4,  14, "city.size_40"
  4,  15, "city.size_50"
  4,  16, "city.size_60"
  4,  17, "city.size_70"
  4,  18, "city.size_80"
  4,  19, "city.size_90"

  5,  11, "city.size_100"
  5,  12, "city.size_200"
  5,  13, "city.size_300"
  5,  14, "city.size_400"
  5,  15, "city.size_500"
  5,  16, "city.size_600"
  5,  17, "city.size_700"
  5,  18, "city.size_800"
  5,  19, "city.size_900"

; Numbers: city tile food/shields/trade y/g/b

  6,  10, "city.t_food_0"
  6,  11, "city.t_food_1"
  6,  12, "city.t_food_2"
  6,  13, "city.t_food_3"
  6,  14, "city.t_food_4"
  6,  15, "city.t_food_5"
  6,  16, "city.t_food_6"
  6,  17, "city.t_food_7"
  6,  18, "city.t_food_8"
  6,  19, "city.t_food_9"

  7,  10, "city.t_shields_0"
  7,  11, "city.t_shields_1"
  7,  12, "city.t_shields_2"
  7,  13, "city.t_shields_3"
  7,  14, "city.t_shields_4"
  7,  15, "city.t_shields_5"
  7,  16, "city.t_shields_6"
  7,  17, "city.t_shields_7"
  7,  18, "city.t_shields_8"
  7,  19, "city.t_shields_9"

  8, 10, "city.t_trade_0"
  8, 11, "city.t_trade_1"
  8, 12, "city.t_trade_2"
  8, 13, "city.t_trade_3"
  8, 14, "city.t_trade_4"
  8, 15, "city.t_trade_5"
  8, 16, "city.t_trade_6"
  8, 17, "city.t_trade_7"
  8, 18, "city.t_trade_8"
  8, 19, "city.t_trade_9"

; Veteran Levels: up to 9 military honors for experienced units

  0, 0, "unit.vet_1"
  0, 1, "unit.vet_2"
  0, 2, "unit.vet_3"
  0, 3, "unit.vet_4"
  0, 4, "unit.vet_5"
  0, 5, "unit.vet_6"
  0, 6, "unit.vet_7"
  0, 7, "unit.vet_8"
  0, 8, "unit.vet_9"

  0, 9, "unit.lowfuel"
  0, 9, "unit.tired"

; Unit activity letters:  (note unit icons have just "u.")

  0, 13, "unit.convert"
  0, 14, "unit.auto_attack",
         "unit.auto_settler"
  0, 15, "unit.stack"
  0, 16, "unit.loaded"
  0, 17, "unit.connect"
  0, 18, "unit.auto_explore"
  0, 19, "unit.patrol"

  1, 8, "unit.transform"
  1, 9, "unit.sentry"
  1, 10, "unit.goto"
  1, 11, "unit.plant"
  1, 12, "unit.pollution"
  1, 13, "unit.road"
  1, 14, "unit.irrigate"
  1, 15, "unit.fortifying",
         "unit.fortress",
         "unit.outpost"
  1, 16, "unit.airbase",
         "unit.airstrip"
  1, 17, "unit.pillage"
  1, 18, "unit.fortified"
  1, 19, "unit.fallout"

; Unit hit-point bars: approx percent of hp remaining

  2,  9,  "unit.hp_100"
  2,  10, "unit.hp_90"
  2,  11, "unit.hp_80"
  2,  12, "unit.hp_70"
  2,  13, "unit.hp_60"
  2,  14, "unit.hp_50"
  2,  15, "unit.hp_40"
  2,  16, "unit.hp_30"
  2,  17, "unit.hp_20"
  2,  18, "unit.hp_10"
  2,  19, "unit.hp_0"

}
