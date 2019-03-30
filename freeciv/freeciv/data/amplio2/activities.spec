
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
    GriffonSpade [GS]
"

[file]
gfx = "amplio2/activities"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 28
dy = 24
pixel_border = 1

tiles = { "row", "column", "tag"
; Unit activity letters:  (note unit icons have just "u.")

; [GS]
  0, 0, "unit.road"
  0, 1, "unit.rail"
  0, 2, "unit.maglev"

  1, 0, "unit.outpost"
  1, 1, "unit.airstrip"
  1, 2, "unit.fortress"
  1, 3, "unit.airbase"
  1, 4, "unit.buoy"

  2, 0, "unit.fortified"
  2, 1, "unit.fortifying"
  2, 2, "unit.sentry"
  2, 3, "unit.patrol"
  2, 4, "unit.pillage"

  3, 0, "unit.irrigate"
  3, 1, "unit.plant"
  3, 2, "unit.transform"
  3, 3, "unit.pollution"
  3, 4, "unit.fallout"

  4, 0, "unit.goto"
  4, 1, "unit.convert"
  4, 2, "unit.auto_explore"

; [GS]
  5,  0, "unit.irrigation"
  5,  1, "unit.farmland"
  5,  2, "unit.mine"
  5,  3, "unit.oil_mine"
  5,  4, "unit.oil_rig"

}
