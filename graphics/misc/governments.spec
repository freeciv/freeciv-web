[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Alexandre Beraud <a_beraud@lemel.fr>
    Enrico Bini <e.bini@sssup.it> (fundamentalism icon)
    GriffonSpade (tribal and federation)
"

[file]
gfx = "misc/governments"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 15
dy = 20

tiles = { "row", "column", "tag"

; Government icons:

  0,  0, "gov.anarchy"
  0,  1, "gov.despotism"
  0,  2, "gov.monarchy"
  0,  3, "gov.communism"
  0,  4, "gov.fundamentalism"
  0,  5, "gov.republic"
  0,  6, "gov.democracy"
  0,  7, "gov.tribal"
  0,  8, "gov.federation"

}
