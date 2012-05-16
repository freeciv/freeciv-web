
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
    Tommy <yobbo3@hotmail.com> (hex mode)
    Daniel Markstedt <himasaram@spray.se> (added dithering)
"

[file]
gfx = "hex2t/tiles"

[grid_main]

x_top_left = 2
y_top_left = 2
dx = 40
dy = 72
pixel_border = 2

tiles = { "row", "column","tag"

; roads

 0, 0, "r.road_n"
 0, 1, "r.road_ne"
 0, 2, "r.road_e"
 0, 7, "r.road_se"
 0, 3, "r.road_s"
 0, 4, "r.road_sw"
 0, 5, "r.road_w"
 0, 6, "r.road_nw"

 0, 8, "r.road_isolated"

; rails

 1, 0, "r.rail_n"
 1, 1, "r.rail_ne"
 1, 2, "r.rail_e"
 1, 7, "r.rail_se"
 1, 3, "r.rail_s"
 1, 4, "r.rail_sw"
 1, 5, "r.rail_w"
 1, 6, "r.rail_nw"

 1, 8, "r.rail_isolated"


; terrain : if more t.whateverN are given it picks one randomly for each tile.
;  for example with t.desert1 and t.desert2.
;  ... actually in this tileset it doesn't even use these
;      as the second ocean layer is drawn over the top of them
;      but it's useful to know ...

  2, 0, "t.l0.desert1"
  6, 6, "t.l0.desert2"
  6, 7, "t.l0.desert3"

  2, 1, "t.l0.plains1"
  2, 2, "t.l0.grassland1"
  2, 3, "t.l0.forest1"
  2, 4, "t.l0.hills1"
  2, 5, "t.l0.mountains1"
  2, 6, "t.l0.tundra1"
  2, 7, "t.l0.arctic1"
  2, 8, "t.l0.swamp1"

; FIXME: The same sprite is drawn twice on top of each other here.
  2, 9, "t.l0.jungle1"
  2, 9, "t.l1.jungle1"

; more ocean in overlays
  2, 10, "t.l0.ocean1"
  6, 8, "t.l0.ocean2"
  6, 8, "t.l0.shelf1"
  2, 10, "t.l0.shelf2"

; Terrain special resources:

  3, 0, "ts.oasis"
  4, 0, "ts.oil"
  3, 1, "ts.buffalo"
  4, 1, "ts.wheat"
  4, 2, "ts.grassland_resources", "ts.river_resources"
  3, 3, "ts.pheasant"
  4, 3, "ts.silk"
  3, 4, "ts.coal"
  4, 4, "ts.wine"
  3, 5, "ts.gold"
  4, 5, "ts.iron"
  3, 6, "ts.tundra_game"
  4, 6, "ts.furs"
  3, 7, "ts.arctic_ivory"
  4, 7, "ts.arctic_oil"
  3, 8, "ts.peat"
  4, 8, "ts.spice"
  3, 9, "ts.gems"
  4, 9, "ts.fruit"
  3, 10, "ts.fish"
  4, 10, "ts.whales"

  5, 7, "ts.seals"
  5, 8, "ts.forest_game"
  5, 9, "ts.horses"


; extras

  5, 0, "tx.oil_mine" 
  5, 1, "tx.mine"
  5, 2, "tx.irrigation"
  5, 3, "tx.farmland"
  5, 4, "tx.pollution"
  5, 5, "tx.fallout"
  5, 6, "tx.village"

; random stuff

  6, 0, "t.dither_tile"
  6, 0, "tx.darkness"
  6, 2, "mask.tile"
  6, 3, "t.coast_color"

  6, 4, "user.attention"
  6, 5, "tx.fog"

; darkness

  7, 0, "tx.darkness_n"
  7, 1, "tx.darkness_ne"
  7, 2, "tx.darkness_e"
  7, 3, "tx.darkness_s"
  7, 4, "tx.darkness_sw"
  7, 5, "tx.darkness_w"

; Unit upkeep in city dialog:
; These should probably be handled differently and have
; a different size...

  8, 0, "upkeep.gold"
  8, 1, "upkeep.gold2"
  8, 2, "upkeep.shield"
  8, 3, "upkeep.food"
  8, 4, "upkeep.food2"
  8, 5, "upkeep.unhappy"
  8, 6, "upkeep.unhappy2"

}



