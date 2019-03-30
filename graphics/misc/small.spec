
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Alexandre Beraud <a_beraud@lemel.fr>
    Jeff Mallatt <jjm@codewell.com> (cooling flakes)
    Davide Pagnin <nightmare@freeciv.it> (angry citizens)
"

[file]
gfx = "misc/small"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 15
dy = 20

tiles = { "row", "column", "tag"

; Science progress indicators:

  0,  0, "s.science_bulb_0"
  0,  1, "s.science_bulb_1"
  0,  2, "s.science_bulb_2"
  0,  3, "s.science_bulb_3"
  0,  4, "s.science_bulb_4"
  0,  5, "s.science_bulb_5"
  0,  6, "s.science_bulb_6"
  0,  7, "s.science_bulb_7"

; Global warming progress indicators:

  0,  8, "s.warming_sun_0"
  0,  9, "s.warming_sun_1"
  0, 10, "s.warming_sun_2"
  0, 11, "s.warming_sun_3"
  0, 12, "s.warming_sun_4"
  0, 13, "s.warming_sun_5"
  0, 14, "s.warming_sun_6"
  0, 15, "s.warming_sun_7"

; Nuclear winter progress indicators:

  0, 27, "s.cooling_flake_0"
  0, 28, "s.cooling_flake_1"
  0, 29, "s.cooling_flake_2"
  0, 30, "s.cooling_flake_3"
  0, 31, "s.cooling_flake_4"
  0, 32, "s.cooling_flake_5"
  0, 33, "s.cooling_flake_6"
  0, 34, "s.cooling_flake_7"

; Panel tax icons

  0, 16, "s.tax_luxury"
  0, 17, "s.tax_science"
  0, 18, "s.tax_gold"

; Citizen icons:

  0, 19, "citizen.content_0"
  0, 20, "citizen.content_1"
  0, 21, "citizen.happy_0"
  0, 22, "citizen.happy_1"
  0, 23, "citizen.unhappy_0" 
  0, 24, "citizen.unhappy_1" 
  0, 25, "citizen.angry_0" 
  0, 26, "citizen.angry_1" 

; Right arrow icon:

  0, 35, "s.right_arrow"

  1,  0, "s.plus"
  1,  1, "s.minus"
}
