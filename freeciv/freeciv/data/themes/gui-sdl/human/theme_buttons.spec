
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Delete icon by Michael Speck <kulkanie@gmx.net>, modified by Hogne Håskjold
    Rafal Bursig <bursig@poczta.fm>
    Christian Prochaska <cp.ml.freeciv.dev@googlemail.com>
    Hogne Håskjold <haskjold@gmail.com>
    Icons from Ubuntu Human Theme: next arrow, back arrow, left arrow, right arrow,
    up arrow, down arrow, new turn gear, CMA monitor
    lock icon and floppy disk (modified by Hogne Håskjold) from gnome-icon-theme 
    (http://ftp.gnome.org/pub/GNOME/sources/gnome-icon-theme/)
"

[file]
gfx = "themes/gui-sdl/human/theme_buttons"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 96
dy = 24
pixel_border = 1

tiles = { "row", "column","tag"

  0,    0,  "theme.OK_button"
  1,    0,  "theme.FAIL_button"
  2,    0,  "theme.NEXT_button"
  3,    0,  "theme.BACK_button"
  4,    0,  "theme.LEFT_ARROW_button"
  5,    0,  "theme.RIGHT_ARROW_button"
  6,    0,  "theme.META_button"
  7,    0,  "theme.MAP_button"
  8,    0,  "theme.FIND_CITY_button"
  9,    0,  "theme.NEW_TURN_button"
  10,   0,  "theme.LOG_button"
  11,   0,  "theme.UNITS_INFO_button"
  12,   0,  "theme.OPTIONS_button"
  13,   0,  "theme.INFO_button"
  14,   0,  "theme.ARMY_button"
  15,   0,  "theme.HAPPY_button"
  16,   0,  "theme.HOME_button"
  17,   0,  "theme.BUY_button"
  18,   0,  "theme.PROD_button"
  19,   0,  "theme.WORK_LIST_button"
  
  0,    1,  "theme.CMA_button"
  1,    1,  "theme.UNLOCK_button"
  2,    1,  "theme.LOCK_button"
  3,    1,  "theme.PLAYERS_button"
  4,    1,  "theme.UNITS_button"
  5,    1,  "theme.SAVE_button"
  6,    1,  "theme.LOAD_button"
  7,    1,  "theme.DELETE_button"
 ; 8,    1,  "theme.xxx_button"
 ; 9,    1,  "theme.xxx_button"
 ; 10,    1,  "theme.xxx_button"
  11,    1,  "theme.BORDERS_button"
  
}
