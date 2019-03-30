
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

; Apolyton Tileset created by CapTVK with thanks to the Apolyton Civ2
; Scenario League.

; Special thanks go to:
; Alex Mor and Captain Nemo for their excellent graphics work
; in the scenarios 2194 days war, Red Front, 2nd front and other misc graphics. 
; Fairline for his huge collection of original Civ2 unit spanning centuries
; Bebro for his collection of mediveal units and ships

artists = "
    Alex Mor [Alex]
    Allard H.S. Höfelt [AHS]
    Bebro [BB]
    Captain Nemo [Nemo][MHN]
    CapTVK [CT] <thomas@worldonline.nl>
    Curt Sibling [CS]
    Erwan [EW]
    Fairline [GB]
    GoPostal [GP]
    Oprisan Sorin [Sor]
    Tanelorn [T]
    Paul Klein Lankhorst / GukGuk [GG]
    Andrew ''Panda´´ Livings [APL]
    Vodvakov
    J. W. Bjerk / Eleazar <www.jwbjerk.com>
    qwm
    FiftyNine
"

[file]
gfx = "amplio2/veterancy"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 64
dy = 48
pixel_border = 1

tiles = { "row", "column", "tag"

; Veteran Levels: up to 9 military honors for experienced units

  0, 0, "unit.vet_1"
  0, 1, "unit.vet_2"
  0, 2, "unit.vet_3"
  0, 3, "unit.vet_4"
  0, 4, "unit.vet_5"
  0, 5, "unit.vet_6"
  0, 6, "unit.vet_7"
  0, 7, "unit.vet_8"
  0, 8, "unit.vet_9"

  0,  9, "unit.tired"
  0, 10, "unit.lowfuel"
;  0, 11, "unit.lowfuel" unused low fuel variant
}
