
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
gfx = "hexemplio/unitcost"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 116
dy = 26
pixel_border = 1

tiles = { "row", "column", "tag"

; Unit upkeep in city dialog:

  0, 0, "upkeep.food"
  0, 1, "upkeep.food2"
  0, 2, "upkeep.food3"
  0, 3, "upkeep.food4"
  0, 4, "upkeep.food5"
  0, 5, "upkeep.food6"
  0, 6, "upkeep.food7"
  0, 7, "upkeep.food8"
  0, 8, "upkeep.food9"
  0, 9, "upkeep.food10"
  1, 0, "upkeep.shield"
  1, 1, "upkeep.shield2"
  1, 2, "upkeep.shield3"
  1, 3, "upkeep.shield4"
  1, 4, "upkeep.shield5"
  1, 5, "upkeep.shield6"
  1, 6, "upkeep.shield7"
  1, 7, "upkeep.shield8"
  1, 8, "upkeep.shield9"
  1, 9, "upkeep.shield10"
  2, 0, "upkeep.gold"
  2, 1, "upkeep.gold2"
  2, 2, "upkeep.gold3"
  2, 3, "upkeep.gold4"
  2, 4, "upkeep.gold5"
  2, 5, "upkeep.gold6"
  2, 6, "upkeep.gold7"
  2, 7, "upkeep.gold8"
  2, 8, "upkeep.gold9"
  2, 9, "upkeep.gold10"
  3, 0, "upkeep.unhappy"
  3, 1, "upkeep.unhappy2"
  3, 2, "upkeep.unhappy3"
  3, 3, "upkeep.unhappy4"
  3, 4, "upkeep.unhappy5"
  3, 5, "upkeep.unhappy6"
  3, 6, "upkeep.unhappy7"
  3, 7, "upkeep.unhappy8"
  3, 8, "upkeep.unhappy9"
  3, 9, "upkeep.unhappy10"

; 2 digit costs, not loaded by code. '00' is the 'emblem' (wheat/shield/coin/angryface) for when cost is less than 10. 0X are the ones digit(0 is 0 in the ones digit). X0 are the tens digit values.

 4, 0, "upkeep.food01"
 4, 1, "upkeep.food02"
 4, 2, "upkeep.food03"
 4, 3, "upkeep.food04"
 4, 4, "upkeep.food05"
 4, 5, "upkeep.food06"
 4, 6, "upkeep.food07"
 4, 7, "upkeep.food08"
 4, 8, "upkeep.food09"
 4, 9, "upkeep.food0"
; 5, 0, "upkeep.food10"
 5, 2, "upkeep.food20"
 5, 2, "upkeep.food30"
 5, 3, "upkeep.food40"
 5, 4, "upkeep.food50"
 5, 5, "upkeep.food60"
 5, 6, "upkeep.food70"
 5, 7, "upkeep.food80"
 5, 8, "upkeep.food90"
 5, 9, "upkeep.food00"

 6, 0, "upkeep.shield01"
 6, 1, "upkeep.shield02"
 6, 2, "upkeep.shield03"
 6, 3, "upkeep.shield04"
 6, 4, "upkeep.shield05"
 6, 5, "upkeep.shield06"
 6, 6, "upkeep.shield07"
 6, 7, "upkeep.shield08"
 6, 8, "upkeep.shield09"
 6, 9, "upkeep.shield0"
; 7, 9, "upkeep.shield10"
 7, 0, "upkeep.shield20"
 7, 1, "upkeep.shield30"
 7, 2, "upkeep.shield40"
 7, 3, "upkeep.shield50"
 7, 4, "upkeep.shield60"
 7, 5, "upkeep.shield70"
 7, 6, "upkeep.shield80"
 7, 7, "upkeep.shield90"
 7, 9, "upkeep.shield00"

 8, 0, "upkeep.gold01"
 8, 1, "upkeep.gold02"
 8, 2, "upkeep.gold03"
 8, 3, "upkeep.gold04"
 8, 4, "upkeep.gold05"
 8, 5, "upkeep.gold06"
 8, 6, "upkeep.gold07"
 8, 7, "upkeep.gold08"
 8, 8, "upkeep.gold09"
 8, 9, "upkeep.gold0"
; 9, 9, "upkeep.gold10"
 9, 0, "upkeep.gold20"
 9, 1, "upkeep.gold30"
 9, 2, "upkeep.gold40"
 9, 3, "upkeep.gold50"
 9, 4, "upkeep.gold60"
 9, 5, "upkeep.gold70"
 9, 6, "upkeep.gold80"
 9, 7, "upkeep.gold90"
 9, 9, "upkeep.gold00"

 8, 0, "upkeep.unhappy01"
 8, 1, "upkeep.unhappy02"
 8, 2, "upkeep.unhappy03"
 8, 3, "upkeep.unhappy04"
 8, 4, "upkeep.unhappy05"
 8, 5, "upkeep.unhappy06"
 8, 6, "upkeep.unhappy07"
 8, 7, "upkeep.unhappy08"
 8, 8, "upkeep.unhappy09"
 8, 9, "upkeep.unhappy0"
; 9, 9, "upkeep.unhappy10"
 9, 0, "upkeep.unhappy20"
 9, 1, "upkeep.unhappy30"
 9, 2, "upkeep.unhappy40"
 9, 3, "upkeep.unhappy50"
 9, 4, "upkeep.unhappy60"
 9, 5, "upkeep.unhappy70"
 9, 6, "upkeep.unhappy80"
 9, 7, "upkeep.unhappy90"
 9, 9, "upkeep.unhappy00"

}
