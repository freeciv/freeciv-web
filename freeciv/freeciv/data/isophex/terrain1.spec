
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tim F. Smith
    Andreas Røsdal (hex mode)
    Daniel Speyer
    GriffonSpade
"

[file]
gfx = "isophex/terrain1"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"

; terrain - layer 0

  0,    0,  "t.l0.desert1"
  0,    1,  "t.l0.plains1"
  0,    2,  "t.l0.grassland1"
  0,    3,  "t.l0.tundra1"
  0,    4,  "t.l0.arctic1"
  0,    5,  "t.l0.swamp1"
  0,    6,  "t.l0.jungle1"
  0,    2,  "t.l0.forest1"
  0,    2,  "t.l0.hills1"
  0,    2,  "t.l0.mountains1"
  0,   10,  "t.l0.inaccessible1"


; Terrain special resources:

  1,    0, "ts.oil"
  2,    0, "ts.oasis"
  1,    1, "ts.buffalo"
  2,    1, "ts.wheat"
  1,    2, "ts.grassland_resources"
  1,    2, "ts.river_resources"
  2,    2, "ts.pheasant"
  1,    3, "ts.tundra_game"
  2,    3, "ts.furs"
  1,    4, "ts.arctic_ivory"
  2,    4, "ts.arctic_oil"
  1,    5, "ts.peat"
  2,    5, "ts.spice"
  1,    6, "ts.gems"
  2,    6, "ts.fruit"
  1,    7, "ts.forest_game"
  2,    7, "ts.silk"
  1,    8, "ts.coal"
  2,    8, "ts.wine"
  1,    9, "ts.gold"
  2,    9, "ts.iron"
  1,   10, "ts.fish"
  2,   10, "ts.whales"

  3,    6, "ts.seals"

; Strategic Resources

  3,    0, "ts.saltpeter"
  3,    1, "ts.aluminum"
  3,    2, "ts.uranium"
  3,    3, "ts.horses"
  3,    4, "ts.elephant"
  3,    5, "ts.rubber"

; Roads

  6,    0, "road.road_isolated"
  6,    1, "road.road_n"
  6,    2, "road.road_ne"
  6,    3, "road.road_e"
  6,    4, "road.road_se"
  6,    5, "road.road_s"
  6,    6, "road.road_sw"
  6,    7, "road.road_w"
  6,    8, "road.road_nw"

; Rails

  7,    0, "road.rail_isolated"
  7,    1, "road.rail_n"
  7,    2, "road.rail_ne"
  7,    3, "road.rail_e"
  7,    4, "road.rail_se"
  7,    5, "road.rail_s"
  7,    6, "road.rail_sw"
  7,    7, "road.rail_w"
  7,    8, "road.rail_nw"

; Maglevs

  8,    0, "road.maglev_isolated"
  8,    1, "road.maglev_n"
  8,    2, "road.maglev_ne"
  8,    3, "road.maglev_e"
  8,    4, "road.maglev_se"
  8,    5, "road.maglev_s"
  8,    6, "road.maglev_sw"
  8,    7, "road.maglev_w"
  8,    8, "road.maglev_nw"

;add-ons

  4,    0, "tx.oil_mine"
  4,    1, "tx.oil_rig"
  4,    2, "tx.mine"
  4,    3, "tx.irrigation"
  4,    4, "tx.farmland"
  4,    5, "tx.fallout"
  4,    6, "tx.pollution"
  4,    7, "tx.village"
 
; misc

  5,    0, "t.coast_color"
  5,    0, "t.blend.lake"
  5,    0, "t.blend.coast"
  5,    0, "t.blend.floor"
  5,    0, "t.blend.ocean"
  5,    1, "t.dither_tile"
  5,    1, "tx.darkness"
  5,    3, "user.attention"
  5,    4, "tx.fog"
  5,    5, "mask.tile"

; goto-path

  5,    7, "path.step"            ; turn boundary within path
  5,    8, "path.exhausted_mp"    ; tip of path, no MP left
  5,    9, "path.normal"          ; tip of path with MP remaining
  5,   10, "path.waypoint"

}
