
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tim F. Smith <yoohootim@hotmail.com> [TS]
    Andreas Røsdal <andrearo@pvv.ntnu.no> (hex mode)
    Daniel Speyer <dspeyer@users.sf.net> [DS]
    Architetto Francesco [http://www.public-domain-photos.com/] [AF]
	Peter Arbor <peter.arbor@gmail.com> [PA]
	GriffonSpade [GS]
	Unknown Battle For Wesnoth artists [BFW]
	Unknown Compass Artist [CA]
	Unknown Opengameart.org Artist(s)
	Unknown FreeCol Artist(s)
	Vegard Stolpnessaeter [VS]
"

[file]
gfx = "hexemplio/terrain"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 126
dy = 64
pixel_border = 1

tiles = { "row", "column","tag"

; terrain - layer 0
  0,    0,  "t.l0.desert1"			;[?]
  0,    1,  "t.l0.plains1"			;[PA]
  0,    1,  "t.l0.hills1"			;[PA]
  0,    1,  "t.l0.mountains1"		;[PA]
  0,    2,  "t.l0.grassland1"		;[PA]
  0,    2,  "t.l0.jungle1"			;[PA]
  0,    2,  "t.l0.forest1"			;[PA]
  0,    3,  "t.l0.tundra1"			;[PA]
  0,    4,  "t.l0.arctic1"			;[?]
  0,    5,  "t.l0.swamp1"			;{PA}

  0,    7,  "t.l0.coast1"			;[PA][GS]
  5,    6,  "t.l0.inaccessible1"	;[?][GS]

; terrain - layer 1
  5,    0,  "t.l1.hills1"
  5,    0,  "t.l1.mountains1"
  5,    0,  "t.l1.tundra1"
  5,    0,  "t.l1.arctic1"
  5,    0,  "t.l1.swamp1"
  5,    0,  "t.l1.jungle1"
  5,    0,  "t.l1.forest1"
  5,    0,  "t.l1.inaccessible1"

; Terrain special resources:
 1,   0, "ts.oil"											;[?]
 1,   0, "ts.arctic_oil"									;[?]
 1,   1, "ts.buffalo"										;[?]
 1,   2, "ts.grassland_resources"							;[?]
 1,   2, "ts.river_resources"								;[?]
 1,   3, "ts.tundra_game"									;[?]
 1,   4, "ts.arctic_ivory"									;[?]
 1,   5, "ts.peat"											;[?]
 1,   6, "ts.coal"											;[?]
 1,   7, "ts.forest_game"									;[?]
 1,   8, "ts.gold"											;[?]

 2,   0, "ts.oasis"											;[BFW]
 2,   1, "ts.wheat"											;[?]
 2,   2, "ts.pheasant"										;[?]
 2,   3, "ts.furs"											;[?]
 2,   4, "ts.seals"											;[?]
 2,   5, "ts.spice"											;[?]
 2,   6, "ts.wine"											;[?]
 2,   7, "ts.silk"											;[?]
 2,   8, "ts.iron"											;[?]

 3,   6, "ts.fruit"											;[?]
 3,   7, "ts.gems"											;[?]
 3,   8, "ts.fish"											;[?]

 4,   8, "ts.whales"										;[?]

; Strategic Resources
 3,   0, "ts.saltpeter"										;[?]
 3,   1, "ts.aluminum"										;[?]
 3,   2, "ts.uranium"										;[?]
 3,   3, "ts.horses"										;[?]
 3,   4, "ts.elephant"										;[AF]
 3,   5, "ts.rubber"										;[GS]

;add-ons
 4, 0, "tx.oil_mine" 										;[?]
 4, 1, "tx.mine"											;[?]
 4, 2, "tx.oil_rig"											;[?][GS]
 4, 3, "tx.irrigation"										;[GS]
 4, 4, "tx.farmland"										;[GS]
 4, 5, "tx.fallout"											;[?][GS]
 4, 6, "tx.pollution"										;[?]
 4, 7, "tx.village"											;[BFW][GS]

;misc
 5, 5, "mask.tile"
 5, 1, "t.dither_tile"										
 5, 1, "tx.darkness"										;[DS][?][GS]
 0, 0, "t.coast_color"										;[?]
; 0, 0, "t.floor_color"										;[?]
 0, 0, "t.lake_color"										;[?]
; 0, 0, "t.inaccessible_water_color"						;[?]
 5, 0, "t.blend.coast"
 5, 0, "t.blend.ocean"
 5, 0, "t.blend.floor"
 5, 0, "t.blend.lake"
 0, 4, "t.blend.arctic"										;[?]
 5, 3, "user.attention"										;[GS]
 5, 4, "tx.fog"
; 5, 6, "t.l0.charcoal1"									;[?]
; 5, 7, "t.l0.compass1"										;[CA][PA]

;goto path
 6, 0, "path.step"            ; turn boundary within path
 6, 1, "path.exhausted_mp"    ; tip of path, no MP left
 6, 2, "path.normal"          ; tip of path with MP remaining
 6, 3, "path.waypoint"

; Irrigation (as special type), and whether north, south, east, west 

 4,  3, "tx.irrigation_s_n0e0se0s0w0nw0"
 4,  3, "tx.irrigation_s_n1e0se0s0w0nw0"
 4,  3, "tx.irrigation_s_n0e1se0s0w0nw0"
 4,  3, "tx.irrigation_s_n1e1se0s0w0nw0"
 4,  3, "tx.irrigation_s_n0e0se0s1w0nw0"
 4,  3, "tx.irrigation_s_n1e0se0s1w0nw0"
 4,  3, "tx.irrigation_s_n0e1se0s1w0nw0"
 4,  3, "tx.irrigation_s_n1e1se0s1w0nw0"
 4,  3, "tx.irrigation_s_n0e0se0s0w1nw0"
 4,  3, "tx.irrigation_s_n1e0se0s0w1nw0"
 4,  3, "tx.irrigation_s_n0e1se0s0w1nw0"
 4,  3, "tx.irrigation_s_n1e1se0s0w1nw0"
 4,  3, "tx.irrigation_s_n0e0se0s1w1nw0"
 4,  3, "tx.irrigation_s_n1e0se0s1w1nw0"
 4,  3, "tx.irrigation_s_n0e1se0s1w1nw0"
 4,  3, "tx.irrigation_s_n1e1se0s1w1nw0"
 4,  3, "tx.irrigation_s_n0e0se1s0w0nw0"
 4,  3, "tx.irrigation_s_n1e0se1s0w0nw0"
 4,  3, "tx.irrigation_s_n0e1se1s0w0nw0"
 4,  3, "tx.irrigation_s_n1e1se1s0w0nw0"
 4,  3, "tx.irrigation_s_n0e0se1s1w0nw0"
 4,  3, "tx.irrigation_s_n1e0se1s1w0nw0"
 4,  3, "tx.irrigation_s_n0e1se1s1w0nw0"
 4,  3, "tx.irrigation_s_n1e1se1s1w0nw0"
 4,  3, "tx.irrigation_s_n0e0se1s0w1nw0"
 4,  3, "tx.irrigation_s_n1e0se1s0w1nw0"
 4,  3, "tx.irrigation_s_n0e1se1s0w1nw0"
 4,  3, "tx.irrigation_s_n1e1se1s0w1nw0"
 4,  3, "tx.irrigation_s_n0e0se1s1w1nw0"
 4,  3, "tx.irrigation_s_n1e0se1s1w1nw0"
 4,  3, "tx.irrigation_s_n0e1se1s1w1nw0"
 4,  3, "tx.irrigation_s_n1e1se1s1w1nw0"
 4,  3, "tx.irrigation_s_n0e0se0s0w0nw1"
 4,  3, "tx.irrigation_s_n1e0se0s0w0nw1"
 4,  3, "tx.irrigation_s_n0e1se0s0w0nw1"
 4,  3, "tx.irrigation_s_n1e1se0s0w0nw1"
 4,  3, "tx.irrigation_s_n0e0se0s1w0nw1"
 4,  3, "tx.irrigation_s_n1e0se0s1w0nw1"
 4,  3, "tx.irrigation_s_n0e1se0s1w0nw1"
 4,  3, "tx.irrigation_s_n1e1se0s1w0nw1"
 4,  3, "tx.irrigation_s_n0e0se0s0w1nw1"
 4,  3, "tx.irrigation_s_n1e0se0s0w1nw1"
 4,  3, "tx.irrigation_s_n0e1se0s0w1nw1"
 4,  3, "tx.irrigation_s_n1e1se0s0w1nw1"
 4,  3, "tx.irrigation_s_n0e0se0s1w1nw1"
 4,  3, "tx.irrigation_s_n1e0se0s1w1nw1"
 4,  3, "tx.irrigation_s_n0e1se0s1w1nw1"
 4,  3, "tx.irrigation_s_n1e1se0s1w1nw1"
 4,  3, "tx.irrigation_s_n0e0se1s0w0nw1"
 4,  3, "tx.irrigation_s_n1e0se1s0w0nw1"
 4,  3, "tx.irrigation_s_n0e1se1s0w0nw1"
 4,  3, "tx.irrigation_s_n1e1se1s0w0nw1"
 4,  3, "tx.irrigation_s_n0e0se1s1w0nw1"
 4,  3, "tx.irrigation_s_n1e0se1s1w0nw1"
 4,  3, "tx.irrigation_s_n0e1se1s1w0nw1"
 4,  3, "tx.irrigation_s_n1e1se1s1w0nw1"
 4,  3, "tx.irrigation_s_n0e0se1s0w1nw1"
 4,  3, "tx.irrigation_s_n1e0se1s0w1nw1"
 4,  3, "tx.irrigation_s_n0e1se1s0w1nw1"
 4,  3, "tx.irrigation_s_n1e1se1s0w1nw1"
 4,  3, "tx.irrigation_s_n0e0se1s1w1nw1"
 4,  3, "tx.irrigation_s_n1e0se1s1w1nw1"
 4,  3, "tx.irrigation_s_n0e1se1s1w1nw1"
 4,  3, "tx.irrigation_s_n1e1se1s1w1nw1"

; Farmland (as special type), and whether north, south, east, west 

 4,  4, "tx.farmland_s_n0e0se0s0w0nw0"
 4,  4, "tx.farmland_s_n1e0se0s0w0nw0"
 4,  4, "tx.farmland_s_n0e1se0s0w0nw0"
 4,  4, "tx.farmland_s_n1e1se0s0w0nw0"
 4,  4, "tx.farmland_s_n0e0se0s1w0nw0"
 4,  4, "tx.farmland_s_n1e0se0s1w0nw0"
 4,  4, "tx.farmland_s_n0e1se0s1w0nw0"
 4,  4, "tx.farmland_s_n1e1se0s1w0nw0"
 4,  4, "tx.farmland_s_n0e0se0s0w1nw0"
 4,  4, "tx.farmland_s_n1e0se0s0w1nw0"
 4,  4, "tx.farmland_s_n0e1se0s0w1nw0"
 4,  4, "tx.farmland_s_n1e1se0s0w1nw0"
 4,  4, "tx.farmland_s_n0e0se0s1w1nw0"
 4,  4, "tx.farmland_s_n1e0se0s1w1nw0"
 4,  4, "tx.farmland_s_n0e1se0s1w1nw0"
 4,  4, "tx.farmland_s_n1e1se0s1w1nw0"
 4,  4, "tx.farmland_s_n0e0se1s0w0nw0"
 4,  4, "tx.farmland_s_n1e0se1s0w0nw0"
 4,  4, "tx.farmland_s_n0e1se1s0w0nw0"
 4,  4, "tx.farmland_s_n1e1se1s0w0nw0"
 4,  4, "tx.farmland_s_n0e0se1s1w0nw0"
 4,  4, "tx.farmland_s_n1e0se1s1w0nw0"
 4,  4, "tx.farmland_s_n0e1se1s1w0nw0"
 4,  4, "tx.farmland_s_n1e1se1s1w0nw0"
 4,  4, "tx.farmland_s_n0e0se1s0w1nw0"
 4,  4, "tx.farmland_s_n1e0se1s0w1nw0"
 4,  4, "tx.farmland_s_n0e1se1s0w1nw0"
 4,  4, "tx.farmland_s_n1e1se1s0w1nw0"
 4,  4, "tx.farmland_s_n0e0se1s1w1nw0"
 4,  4, "tx.farmland_s_n1e0se1s1w1nw0"
 4,  4, "tx.farmland_s_n0e1se1s1w1nw0"
 4,  4, "tx.farmland_s_n1e1se1s1w1nw0"
 4,  4, "tx.farmland_s_n0e0se0s0w0nw1"
 4,  4, "tx.farmland_s_n1e0se0s0w0nw1"
 4,  4, "tx.farmland_s_n0e1se0s0w0nw1"
 4,  4, "tx.farmland_s_n1e1se0s0w0nw1"
 4,  4, "tx.farmland_s_n0e0se0s1w0nw1"
 4,  4, "tx.farmland_s_n1e0se0s1w0nw1"
 4,  4, "tx.farmland_s_n0e1se0s1w0nw1"
 4,  4, "tx.farmland_s_n1e1se0s1w0nw1"
 4,  4, "tx.farmland_s_n0e0se0s0w1nw1"
 4,  4, "tx.farmland_s_n1e0se0s0w1nw1"
 4,  4, "tx.farmland_s_n0e1se0s0w1nw1"
 4,  4, "tx.farmland_s_n1e1se0s0w1nw1"
 4,  4, "tx.farmland_s_n0e0se0s1w1nw1"
 4,  4, "tx.farmland_s_n1e0se0s1w1nw1"
 4,  4, "tx.farmland_s_n0e1se0s1w1nw1"
 4,  4, "tx.farmland_s_n1e1se0s1w1nw1"
 4,  4, "tx.farmland_s_n0e0se1s0w0nw1"
 4,  4, "tx.farmland_s_n1e0se1s0w0nw1"
 4,  4, "tx.farmland_s_n0e1se1s0w0nw1"
 4,  4, "tx.farmland_s_n1e1se1s0w0nw1"
 4,  4, "tx.farmland_s_n0e0se1s1w0nw1"
 4,  4, "tx.farmland_s_n1e0se1s1w0nw1"
 4,  4, "tx.farmland_s_n0e1se1s1w0nw1"
 4,  4, "tx.farmland_s_n1e1se1s1w0nw1"
 4,  4, "tx.farmland_s_n0e0se1s0w1nw1"
 4,  4, "tx.farmland_s_n1e0se1s0w1nw1"
 4,  4, "tx.farmland_s_n0e1se1s0w1nw1"
 4,  4, "tx.farmland_s_n1e1se1s0w1nw1"
 4,  4, "tx.farmland_s_n0e0se1s1w1nw1"
 4,  4, "tx.farmland_s_n1e0se1s1w1nw1"
 4,  4, "tx.farmland_s_n0e1se1s1w1nw1"
 4,  4, "tx.farmland_s_n1e1se1s1w1nw1"

}
