
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
    Tommy <yobbo3@hotmail.com> (hex mode)
    Daniel Markstedt <himasaram@spray.se> (added dithering)
    GriffonSpade (oil_rig)
"

[file]
gfx = "hex2t/tiles"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 40
dy = 72
pixel_border = 1

tiles = { "row", "column","tag"

; Roads

 0, 0, "road.road_n"
 0, 1, "road.road_ne"
 0, 2, "road.road_e"
 0, 7, "road.road_se"
 0, 3, "road.road_s"
 0, 4, "road.road_sw"
 0, 5, "road.road_w"
 0, 6, "road.road_nw"

 0, 8, "road.road_isolated"

; Rails

 1, 0, "road.rail_n"
 1, 1, "road.rail_ne"
 1, 2, "road.rail_e"
 1, 7, "road.rail_se"
 1, 3, "road.rail_s"
 1, 4, "road.rail_sw"
 1, 5, "road.rail_w"
 1, 6, "road.rail_nw"

 1, 8, "road.rail_isolated"


; Maglevs

 2, 0, "road.maglev_n"
 2, 1, "road.maglev_ne"
 2, 2, "road.maglev_e"
 2, 7, "road.maglev_se"
 2, 3, "road.maglev_s"
 2, 4, "road.maglev_sw"
 2, 5, "road.maglev_w"
 2, 6, "road.maglev_nw"

 2, 8, "road.maglev_isolated"

; terrain : if more t.whateverN are given it picks one randomly for each tile.
;  for example with t.desert1 and t.desert2.
;  ... actually in this tileset it doesn't even use these
;      as the second ocean layer is drawn over the top of them
;      but it's useful to know ...

  3, 0, "t.l0.desert1"
  7, 6, "t.l0.desert2"
  7, 7, "t.l0.desert3"

  3, 1, "t.l0.plains1"
  3, 2, "t.l0.grassland1"
  3, 3, "t.l0.forest1"
  3, 4, "t.l0.hills1"
  3, 5, "t.l0.mountains1"
  3, 6, "t.l0.tundra1"
  3, 7, "t.l0.arctic1"
  3, 8, "t.l0.swamp1"

  7, 9, "t.l0.inaccessible1"

; FIXME: The same sprite is drawn twice on top of each other here.
  3, 3, "t.l0.jungle1"
  3, 9, "t.l1.jungle1"

; more ocean in overlays
;  3, 10, "t.l0.coast1"
;  7, 8,  "t.l0.coast2"
;  3, 11, "t.l0.floor1"

; Terrain special resources:

  4, 0, "ts.oasis"
  5, 0, "ts.oil"
  4, 1, "ts.buffalo"
  5, 1, "ts.wheat"
  5, 2, "ts.grassland_resources", "ts.river_resources"
  4, 3, "ts.pheasant"
  5, 3, "ts.silk"
  4, 4, "ts.coal"
  5, 4, "ts.wine"
  4, 5, "ts.gold"
  5, 5, "ts.iron"
  4, 6, "ts.tundra_game"
  5, 6, "ts.furs"
  4, 7, "ts.arctic_ivory"
  5, 7, "ts.arctic_oil"
  4, 8, "ts.peat"
  5, 8, "ts.spice"
  4, 9, "ts.gems"
  5, 9, "ts.fruit"
  4, 10, "ts.fish"
  5, 10, "ts.whales"

  6, 7, "ts.seals"
  6, 8, "ts.forest_game"
  6, 9, "ts.horses"


; extras

  6, 0, "tx.oil_mine"
  6, 10, "tx.oil_rig"
  6, 1, "tx.mine"
  6, 2, "tx.irrigation"
  6, 3, "tx.farmland"
  6, 4, "tx.pollution"
  6, 5, "tx.fallout"
  6, 6, "tx.village"

; random stuff

  7, 0, "t.dither_tile"
  7, 0, "tx.darkness"
  7, 2, "mask.tile"
  7, 3, "t.coast_color"

  7, 4, "user.attention"
  7, 5, "tx.fog"

; darkness

  8, 0, "tx.darkness_n"
  8, 1, "tx.darkness_ne"
  8, 2, "tx.darkness_e"
  8, 3, "tx.darkness_s"
  8, 4, "tx.darkness_sw"
  8, 5, "tx.darkness_w"

; goto path

  8, 6, "path.step"            ; turn boundary within path
  8, 7, "path.exhausted_mp"    ; tip of path, no MP left
  8, 8, "path.normal"          ; tip of path with MP remaining
  8, 9, "path.waypoint"
}

[grid_upkeep]

x_top_left = 452
y_top_left = 455
dx = 40
dy = 28
pixel_border = 1

tiles = { "row", "column","tag"

; Unit upkeep in city dialog:
; These should probably be handled differently and have
; a different size...

  0, 0, "upkeep.shield"
  1, 0, "upkeep.gold"
  2, 0, "upkeep.gold2"
  3, 0, "upkeep.food"
  4, 0, "upkeep.food2"
  5, 0, "upkeep.unhappy"
  6, 0, "upkeep.unhappy2"

}



