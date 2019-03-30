
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
	Hogne Håskjold <hogne@freeciv.org>[HH]
	Eleazar[El]
	GriffonSpade[GS]
"

[file]
gfx = "hexemplio/activities"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 31
dy = 25
pixel_border = 1

tiles = { "row", "column", "tag"

; Unit activity letters: unit commands

  0, 0, "unit.fortified"                ;[HH]
  0, 1, "unit.fortifying"               ;[GS]
  0, 2, "unit.auto_explore"	        ;[?]
  0, 3, "unit.sentry"                   ;[?]
  0, 4, "unit.goto"                     ;[?]
;  0, 5, "unit.patrol"                   ;[?]
  0, 6, "unit.convert"                  ;[?][GS]

; Unit activity letters: tile commands

  1, 0, "unit.rest"                     ;[?]
  1, 1, "unit.irrigate"                 ;[?]
  1, 2, "unit.plant"                    ;[?]
  1, 3, "unit.transform"                ;[?]
  1, 4, "unit.pillage"                  ;[?]
  1, 5, "unit.pollution"                ;[?]
  1, 6, "unit.fallout"                  ;[?]
  1, 7, "unit.farm"                     ;[?]

; Unit Activities: roads

  2, 0, "unit.road"                     ;[GS]
  2, 1, "unit.rail"                     ;[GS]
;  2, 2, "unit.highway"                  ;[GS]
  2, 3, "unit.maglev"                   ;[GS]

; Unit activities: bases

  2,  4, "unit.outpost"                 ;[GS]
  2,  5, "unit.fortress"                ;[GS]
  2,  6, "unit.airstrip"                ;[GS]
  2,  7, "unit.airbase"                 ;[GS]
  3,  5, "unit.buoy"                    ;[GS][El]

; Unit activities: other extras

  3,  0, "unit.irrigation"              ;[GS]
  3,  1, "unit.farmland"                ;[GS]
  3,  2, "unit.mine"                    ;[GS]
  3,  3, "unit.oil_mine"                ;[GS]
  3,  4, "unit.oil_rig"                 ;[GS]

}
