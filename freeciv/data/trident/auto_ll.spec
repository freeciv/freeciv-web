
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
"

[file]
gfx = "trident/tiles"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

; Unit activity letters:  (note unit icons have just "u.")

  4, 17, "unit.auto_attack",
         "unit.auto_settler"
  4, 18, "unit.connect"
  4, 19, "unit.auto_explore"

}
