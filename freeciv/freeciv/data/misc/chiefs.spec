[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Falk Hueffner <falk.hueffner@student.uni-tuebingen.de>
"

[file]
gfx = "misc/chiefs"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 45
dy = 45

tiles = { "row", "column", "tag"

; Unit hit-point bars: approx percent of hp remaining
0,  0, "unit.hp_100"
0,  1, "unit.hp_90"
0,  2, "unit.hp_80"
0,  3, "unit.hp_70"
0,  4, "unit.hp_60"
0,  5, "unit.hp_50"
0,  6, "unit.hp_40"
0,  7, "unit.hp_30"
0,  8, "unit.hp_20"
0,  9, "unit.hp_10"
0, 10, "unit.hp_0"

}
