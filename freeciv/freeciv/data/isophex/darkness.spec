
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Tim F. Smith <yoohootim@hotmail.com>
    Andreas Røsdal <andrearo@pvv.ntnu.no> (hex mode)
    Daniel Speyer <dspeyer@users.sf.net> 
"

[file]
gfx = "isophex/darkness"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"

  0, 0, "tx.darkness_n"
  0, 1, "tx.darkness_e"
  0, 2, "tx.darkness_se"
  1, 0, "tx.darkness_s"
  1, 1, "tx.darkness_w"
  1, 2, "tx.darkness_nw"
}
