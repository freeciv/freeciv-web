
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
    Tommy <yobbo3@hotmail.com> (hex mode)
    Daniel Markstedt <himasaram@spray.se> (added dithering)
    GriffinSpade (UnitCosts)
"

[file]
gfx = "hex2t/unitcost"

[grid_upkeep]

x_top_left = 1
y_top_left = 1
dx = 40
dy = 28
pixel_border = 1

tiles = { "row", "column","tag"

;  0, 0, "upkeep.shield"
  0, 1, "upkeep.shield2"
  0, 2, "upkeep.shield3"
  0, 3, "upkeep.shield4"
  0, 4, "upkeep.shield5"
  0, 5, "upkeep.shield6"
  0, 6, "upkeep.shield7"
  0, 7, "upkeep.shield8"
  0, 8, "upkeep.shield9"
  0, 9, "upkeep.shield10"
;  1, 0, "upkeep.food"
;  1, 1, "upkeep.food2"
  1, 2, "upkeep.food3"
  1, 3, "upkeep.food4"
  1, 4, "upkeep.food5"
  1, 5, "upkeep.food6"
  1, 6, "upkeep.food7"
  1, 7, "upkeep.food8"
  1, 8, "upkeep.food9"
  1, 9, "upkeep.food10"
;  2, 0, "upkeep.unhappy"
;  2, 1, "upkeep.unhappy2"
  2, 2, "upkeep.unhappy3"
  2, 3, "upkeep.unhappy4"
  2, 4, "upkeep.unhappy5"
  2, 5, "upkeep.unhappy6"
  2, 6, "upkeep.unhappy7"
  2, 7, "upkeep.unhappy8"
  2, 8, "upkeep.unhappy9"
  2, 9, "upkeep.unhappy10"
;  3, 0, "upkeep.gold"
;  3, 1, "upkeep.gold2"
  3, 2, "upkeep.gold3"
  3, 3, "upkeep.gold4"
  3, 4, "upkeep.gold5"
  3, 5, "upkeep.gold6"
  3, 6, "upkeep.gold7"
  3, 7, "upkeep.gold8"
  3, 8, "upkeep.gold9"
  3, 9, "upkeep.gold10"
}
