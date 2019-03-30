
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Hogne Håskjold
    Daniel Speyer
    Yautja
    CapTVK
    GriffonSpade
    Gyubal Wahazar
"

[file]
gfx = "amplio2/terrain1"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 48
pixel_border = 1

tiles = { "row", "column", "tag"

; terrain
 0,  0, "t.l0.desert1"

 1,  0, "t.l0.plains1"

 2,  0, "t.l0.grassland1"

 3,  0, "t.l0.forest1"

 4,  0, "t.l0.hills1"

 5,  0, "t.l0.mountains1"

 6,  0, "t.l0.tundra1"

 7,  0, "t.l0.arctic1"
;7,  0, "t.l1.arctic1" not redrawn
;7,  0, "t.l2.arctic1" not redrawn

 8,  0, "t.l0.swamp1"

 9,  0, "t.l0.jungle1"
10,  0, "t.l0.inaccessible1"

; Terrain special resources:

 0,  2, "ts.oasis"
 0,  4, "ts.oil"

 1,  2, "ts.buffalo"
 1,  4, "ts.wheat"

 2,  2, "ts.pheasant"
 2,  4, "ts.silk"

 3,  2, "ts.coal"
 3,  4, "ts.wine"

 4,  2, "ts.gold"
 4,  4, "ts.iron"

 5,  2, "ts.tundra_game"
 5,  4, "ts.furs"

 6,  2, "ts.arctic_ivory"
 6,  4, "ts.arctic_oil"

 7,  2, "ts.peat"
 7,  4, "ts.spice"

 8,  2, "ts.gems"
 8,  4, "ts.fruit"

 9,  2, "ts.fish"
 9,  4, "ts.whales"

 10, 2, "ts.seals"
 10, 4, "ts.forest_game"

 11, 2, "ts.horses"
 11, 4, "ts.grassland_resources", "ts.river_resources"

;roads
 12, 0, "road.road_isolated"
 12, 1, "road.road_n"
 12, 2, "road.road_ne"
 12, 3, "road.road_e"
 12, 4, "road.road_se"
 12, 5, "road.road_s"
 12, 6, "road.road_sw"
 12, 7, "road.road_w"
 12, 8, "road.road_nw"

;rails
 13, 0, "road.rail_isolated"
 13, 1, "road.rail_n"
 13, 2, "road.rail_ne"
 13, 3, "road.rail_e"
 13, 4, "road.rail_se"
 13, 5, "road.rail_s"
 13, 6, "road.rail_sw"
 13, 7, "road.rail_w"
 13, 8, "road.rail_nw"

;add-ons
 0,  6, "tx.oil_mine"
 1,  6, "tx.irrigation"
 2,  6, "tx.farmland"
 3,  6, "tx.mine"
 4,  6, "tx.pollution"
 5,  6, "tx.village"
 6,  6, "tx.fallout"
 7,  6, "tx.oil_rig"

 15,  0, "t.dither_tile"
 15,  0, "tx.darkness"
 15,  2, "mask.tile"
 15,  2, "t.unknown1"
  7,  0, "t.blend.arctic" ;ice over neighbors
 15,  3, "t.blend.coast"
 15,  3, "t.blend.lake"
 15,  4, "user.attention"
 15,  5, "tx.fog"

;goto path sprites
 14,  7, "path.step"            ; turn boundary within path
 14,  8, "path.exhausted_mp"    ; tip of path, no MP left
 15,  7, "path.normal"          ; tip of path with MP remaining
 15,  8, "path.waypoint"
}
