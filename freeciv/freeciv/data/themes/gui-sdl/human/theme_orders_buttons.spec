
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
   Michael Speck <kulkanie@gmx.net>
   Rafal Bursig <bursig@poczta.fm>
   Adam Szwajnoch <pamash@poczta.onet.pl>
   Jukka Liukkonen <pilkku@iki.fi>
   Kohsuke Kawaguchi <kk@kohsuke.org> (http://sourceforge.jp/projects/freetrain/)
   Daniel Markstedt <markstedt@gmail.com>
"

[file]
gfx = "themes/gui-sdl/human/theme_orders_buttons"

[grid_main]

x_top_left = 0
y_top_left = 1
dx = 120
dy = 30
pixel_border = 1

tiles = { "row", "column","tag"

; terrain
  0,    0,  "theme.order_auto_attack"
  1,    0,  "theme.order_auto_connect"
  2,    0,  "theme.order_auto_explorer"
  3,    0,  "theme.order_auto_settler"
  4,    0,  "theme.order_build_city"
  5,    0,  "theme.order_cutdown_forest"
  6,    0,  "theme.order_plant_forest"
  7,    0,  "theme.order_build_mining"
  8,    0,  "theme.order_irrigation"    
  9,    0,  "theme.order_done"
  10,   0,  "theme.order_disband"  
  11,   0,  "theme.order_fortify"
  12,   0,  "theme.order_goto"
  13,   0,  "theme.order_goto_city"
  14,   0,  "theme.order_home"
  15,   0,  "theme.order_nuke"
  16,   0,  "theme.order_paradrop"
  17,   0,  "theme.order_patrol"
  18,   0,  "theme.order_pillage"
  19,   0,  "theme.order_build_railroad"
  20,   0,  "theme.order_build_road"
  21,   0,  "theme.order_sentry"
  22,   0,  "theme.order_unload"
  23,   0,  "theme.order_wait"
  24,   0,  "theme.order_build_fortress"
  25,   0,  "theme.order_clean_fallout"
  26,   0,  "theme.order_clean_pollution"
  27,   0,  "theme.order_build_airbase"
  28,   0,  "theme.order_transform"
  29,   0,  "theme.order_add_to_city"
  30,   0,  "theme.order_carravan_wonder"
  31,   0,  "theme.order_carravan_traderoute"
  32,   0,  "theme.order_spying"
  33,   0,  "theme.order_wakeup"
  34,   0,  "theme.order_return"
  35,   0,  "theme.order_airlift"
  36,   0,  "theme.order_load"
; 37,   0,  "theme.order_"
; 38,   0,  "theme.order_"
  39,   0,  "theme.order_empty"
 
}
