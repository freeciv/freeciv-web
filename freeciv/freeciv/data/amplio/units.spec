
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
gfx = "amplio/units"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 64
dy = 48

tiles = { "row", "column", "tag"
				; Scenario League tags in brackets
  0,  0, "u.armor"		; [Nemo]
  0,  1, "u.howitzer"		; [Nemo]
  0,  2, "u.battleship"		; [Nemo]
  0,  3, "u.bomber"		; [GB]
  0,  4, "u.cannon"		; [CT]
  0,  5, "u.caravan"		; [Alex] & [CT]
  0,  6, "u.carrier"		; [Nemo]
  0,  7, "u.catapult"		; [CT]
  0,  8, "u.horsemen"		; [GB]
  0,  9, "u.chariot"		; [BB] & [GB]
  0, 10, "u.cruiser"		; [Nemo]
  0, 11, "u.diplomat"		; [Nemo]
  0, 12, "u.fighter"		; [Sor]
  0, 13, "u.frigate"		; [BB]
  0, 14, "u.ironclad"		; [Nemo]
  0, 15, "u.knights"		; [BB]
  0, 16, "u.legion"		; [GB]
  0, 17, "u.mech_inf"		; [GB]
  0, 18, "u.warriors"		; [GB]
  0, 19, "u.musketeers"		; [Alex] & [CT]
  1,  0, "u.nuclear"		; [Nemo] & [CS]
  1,  1, "u.phalanx"		; [GB] & [CT]
  1,  2, "u.riflemen"		; [Alex]
  1,  3, "u.caravel"		; [BB]
  1,  4, "u.settlers"		; [MHN]
  1,  5, "u.submarine"		; [GP]
  1,  6, "u.transport"		; [Nemo]
  1,  7, "u.trireme"		; [BB]
  1,  8, "u.archers"		; [GB]
  1,  9, "u.cavalry"		; [Alex]
  1, 10, "u.cruise_missile"	; [CS]
  1, 11, "u.destroyer"		; [Nemo]
  1, 12, "u.dragoons"		; [GB]
  1, 13, "u.explorer"		; [Alex] & [CT]
  1, 14, "u.freight"		; [CT] & qwm
  1, 15, "u.galleon"		; [BB]
  1, 16, "u.partisan"		; [BB] & [CT]
  1, 17, "u.pikemen"		; [T]
  2,  0, "u.marines"		; [GB]
  2,  1, "u.spy"		; [EW] & [CT]
  2,  2, "u.engineers"		; [Nemo] & [CT]
  2,  3, "u.artillery"		; [GB]
  2,  4, "u.helicopter"		; [T]
  2,  5, "u.alpine_troops"	; [Nemo]
  2,  6, "u.stealth_bomber"	; [GB]
  2,  7, "u.stealth_fighter"	; [Nemo] & [AHS]
  2,  8, "u.aegis_cruiser"	; [GP]
  2,  9, "u.paratroopers"	; [Alex]
  2, 10, "u.elephants"		; [Alex] & [GG] & [CT]
  2, 11, "u.crusaders"		; [BB]
  2, 12, "u.fanatics"		; [GB] & [CT]
  2, 13, "u.awacs"		; [APL]
  2, 14, "u.worker"		; [GB]
  2, 15, "u.leader"		; [GB]
  2, 16, "u.barbarian_leader"	; FiftyNine
  2, 17, "u.migrants"		; Eleazar
;  3, 15, "u.train"		; Eleazar

; Veteran Levels: up to 9 military honors for experienced units

  3, 0, "unit.vet_1"
  3, 1, "unit.vet_2"
  3, 2, "unit.vet_3"
  3, 3, "unit.vet_4"
  3, 4, "unit.vet_5"
  3, 5, "unit.vet_6"
  3, 6, "unit.vet_7"
  3, 7, "unit.vet_8"
  3, 8, "unit.vet_9"

  3, 11, "unit.lowfuel"
  3, 11, "unit.tired"
}
