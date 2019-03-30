
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
    Eleazar (buoy)
    Vincent Croisier <vincent.croisier@advalvas.be> (ruins)
    Michael Johnson <justaguest> (nuke explosion)
    The Square Cow (inaccessible terrain)
    GriffonSpade
"

[file]
gfx = "trident/tiles"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

; Grassland, and whether terrain to north, south, east, west 
; is more grassland:

  0,  2, "t.l0.grassland1"
  0,  3, "t.l0.inaccessible1"

  0,  1, "t.l1.grassland1"
  0,  1, "t.l1.hills1"
  0,  1, "t.l1.forest1"
  0,  1, "t.l1.mountains1"
  0,  1, "t.l1.desert1"
  0,  1, "t.l1.jungle1"
  0,  1, "t.l1.plains1"
  0,  1, "t.l1.swamp1"
  0,  1, "t.l1.tundra1"

  0,  1, "t.l2.grassland1"
  0,  1, "t.l2.hills1"
  0,  1, "t.l2.forest1"
  0,  1, "t.l2.mountains1"
  0,  1, "t.l2.desert1"
  0,  1, "t.l2.arctic1"
  0,  1, "t.l2.jungle1"
  0,  1, "t.l2.plains1"
  0,  1, "t.l2.swamp1"
  0,  1, "t.l2.tundra1"


; For hills, forest and mountains don't currently have a full set,
; re-use values but provide for future expansion; current sets
; effectively ignore N/S terrain.

; Hills, and whether terrain to north, south, east, west 
; is more hills.

  0,  4, "t.l0.hills_n0e0s0w0",  ; not-hills E and W
         "t.l0.hills_n0e0s1w0", 
         "t.l0.hills_n1e0s0w0", 
         "t.l0.hills_n1e0s1w0" 
  0,  5, "t.l0.hills_n0e1s0w0",  ; hills E
         "t.l0.hills_n0e1s1w0", 
         "t.l0.hills_n1e1s0w0", 
         "t.l0.hills_n1e1s1w0" 
  0,  6, "t.l0.hills_n0e1s0w1",  ; hills E and W
         "t.l0.hills_n0e1s1w1", 
         "t.l0.hills_n1e1s0w1", 
         "t.l0.hills_n1e1s1w1" 
  0,  7, "t.l0.hills_n0e0s0w1",  ; hills W
         "t.l0.hills_n0e0s1w1", 
         "t.l0.hills_n1e0s0w1", 
         "t.l0.hills_n1e0s1w1" 

; Forest, and whether terrain to north, south, east, west 
; is more forest.

  0,  8, "t.l0.forest_n0e0s0w0",  ; not-forest E and W
         "t.l0.forest_n0e0s1w0", 
         "t.l0.forest_n1e0s0w0", 
         "t.l0.forest_n1e0s1w0" 
  0,  9, "t.l0.forest_n0e1s0w0",  ; forest E
         "t.l0.forest_n0e1s1w0", 
         "t.l0.forest_n1e1s0w0", 
         "t.l0.forest_n1e1s1w0" 
  0, 10, "t.l0.forest_n0e1s0w1",  ; forest E and W
         "t.l0.forest_n0e1s1w1", 
         "t.l0.forest_n1e1s0w1", 
         "t.l0.forest_n1e1s1w1" 
  0, 11, "t.l0.forest_n0e0s0w1",  ; forest W
         "t.l0.forest_n0e0s1w1", 
         "t.l0.forest_n1e0s0w1", 
         "t.l0.forest_n1e0s1w1" 

; Mountains, and whether terrain to north, south, east, west 
; is more mountains.

  0, 12, "t.l0.mountains_n0e0s0w0",  ; not-mountains E and W
         "t.l0.mountains_n0e0s1w0", 
         "t.l0.mountains_n1e0s0w0", 
         "t.l0.mountains_n1e0s1w0" 
  0, 13, "t.l0.mountains_n0e1s0w0",  ; mountains E
         "t.l0.mountains_n0e1s1w0", 
         "t.l0.mountains_n1e1s0w0", 
         "t.l0.mountains_n1e1s1w0" 
  0, 14, "t.l0.mountains_n0e1s0w1",  ; mountains E and W
         "t.l0.mountains_n0e1s1w1", 
         "t.l0.mountains_n1e1s0w1", 
         "t.l0.mountains_n1e1s1w1" 
  0, 15, "t.l0.mountains_n0e0s0w1",  ; mountains W
         "t.l0.mountains_n0e0s1w1", 
         "t.l0.mountains_n1e0s0w1", 
         "t.l0.mountains_n1e0s1w1" 

; Desert, and whether terrain to north, south, east, west 
; is more desert:

  1,  0, "t.l0.desert_n1e1s1w1"
  1,  1, "t.l0.desert_n0e1s1w1"
  1,  2, "t.l0.desert_n1e0s1w1"
  1,  3, "t.l0.desert_n0e0s1w1"
  1,  4, "t.l0.desert_n1e1s0w1"
  1,  5, "t.l0.desert_n0e1s0w1"
  1,  6, "t.l0.desert_n1e0s0w1"
  1,  7, "t.l0.desert_n0e0s0w1"
  1,  8, "t.l0.desert_n1e1s1w0"
  1,  9, "t.l0.desert_n0e1s1w0"
  1, 10, "t.l0.desert_n1e0s1w0"
  1, 11, "t.l0.desert_n0e0s1w0"
  1, 12, "t.l0.desert_n1e1s0w0"
  1, 13, "t.l0.desert_n0e1s0w0"
  1, 14, "t.l0.desert_n1e0s0w0"
  1, 15, "t.l0.desert_n0e0s0w0"

; Arctic, and whether terrain to north, south, east, west 
; is more arctic:

  6,  0, "t.l0.arctic_n1e1s1w1"
  6,  1, "t.l0.arctic_n0e1s1w1"
  6,  2, "t.l0.arctic_n1e0s1w1"
  6,  3, "t.l0.arctic_n0e0s1w1"
  6,  4, "t.l0.arctic_n1e1s0w1"
  6,  5, "t.l0.arctic_n0e1s0w1"
  6,  6, "t.l0.arctic_n1e0s0w1"
  6,  7, "t.l0.arctic_n0e0s0w1"
  6,  8, "t.l0.arctic_n1e1s1w0"
  6,  9, "t.l0.arctic_n0e1s1w0"
  6, 10, "t.l0.arctic_n1e0s1w0"
  6, 11, "t.l0.arctic_n0e0s1w0"
  6, 12, "t.l0.arctic_n1e1s0w0"
  6, 13, "t.l0.arctic_n0e1s0w0"
  6, 14, "t.l0.arctic_n1e0s0w0"
  6, 15, "t.l0.arctic_n0e0s0w0"

  2,  0, "t.l1.arctic_n1e1s1w1"
  2,  1, "t.l1.arctic_n0e1s1w1"
  2,  2, "t.l1.arctic_n1e0s1w1"
  2,  3, "t.l1.arctic_n0e0s1w1"
  2,  4, "t.l1.arctic_n1e1s0w1"
  2,  5, "t.l1.arctic_n0e1s0w1"
  2,  6, "t.l1.arctic_n1e0s0w1"
  2,  7, "t.l1.arctic_n0e0s0w1"
  2,  8, "t.l1.arctic_n1e1s1w0"
  2,  9, "t.l1.arctic_n0e1s1w0"
  2, 10, "t.l1.arctic_n1e0s1w0"
  2, 11, "t.l1.arctic_n0e0s1w0"
  2, 12, "t.l1.arctic_n1e1s0w0"
  2, 13, "t.l1.arctic_n0e1s0w0"
  2, 14, "t.l1.arctic_n1e0s0w0"
  2, 15, "t.l1.arctic_n0e0s0w0"

; Jungle, and whether terrain to north, south, east, west 
; is more jungle:

  3,  0, "t.l0.jungle_n1e1s1w1"
  3,  1, "t.l0.jungle_n0e1s1w1"
  3,  2, "t.l0.jungle_n1e0s1w1"
  3,  3, "t.l0.jungle_n0e0s1w1"
  3,  4, "t.l0.jungle_n1e1s0w1"
  3,  5, "t.l0.jungle_n0e1s0w1"
  3,  6, "t.l0.jungle_n1e0s0w1"
  3,  7, "t.l0.jungle_n0e0s0w1"
  3,  8, "t.l0.jungle_n1e1s1w0"
  3,  9, "t.l0.jungle_n0e1s1w0"
  3, 10, "t.l0.jungle_n1e0s1w0"
  3, 11, "t.l0.jungle_n0e0s1w0"
  3, 12, "t.l0.jungle_n1e1s0w0"
  3, 13, "t.l0.jungle_n0e1s0w0"
  3, 14, "t.l0.jungle_n1e0s0w0"
  3, 15, "t.l0.jungle_n0e0s0w0"

; Plains, and whether terrain to north, south, east, west 
; is more plains:

  4,  0, "t.l0.plains_n1e1s1w1"
  4,  1, "t.l0.plains_n0e1s1w1"
  4,  2, "t.l0.plains_n1e0s1w1"
  4,  3, "t.l0.plains_n0e0s1w1"
  4,  4, "t.l0.plains_n1e1s0w1"
  4,  5, "t.l0.plains_n0e1s0w1"
  4,  6, "t.l0.plains_n1e0s0w1"
  4,  7, "t.l0.plains_n0e0s0w1"
  4,  8, "t.l0.plains_n1e1s1w0"
  4,  9, "t.l0.plains_n0e1s1w0"
  4, 10, "t.l0.plains_n1e0s1w0"
  4, 11, "t.l0.plains_n0e0s1w0"
  4, 12, "t.l0.plains_n1e1s0w0"
  4, 13, "t.l0.plains_n0e1s0w0"
  4, 14, "t.l0.plains_n1e0s0w0"
  4, 15, "t.l0.plains_n0e0s0w0"

; Swamp, and whether terrain to north, south, east, west 
; is more swamp:

  5,  0, "t.l0.swamp_n1e1s1w1"
  5,  1, "t.l0.swamp_n0e1s1w1"
  5,  2, "t.l0.swamp_n1e0s1w1"
  5,  3, "t.l0.swamp_n0e0s1w1"
  5,  4, "t.l0.swamp_n1e1s0w1"
  5,  5, "t.l0.swamp_n0e1s0w1"
  5,  6, "t.l0.swamp_n1e0s0w1"
  5,  7, "t.l0.swamp_n0e0s0w1"
  5,  8, "t.l0.swamp_n1e1s1w0"
  5,  9, "t.l0.swamp_n0e1s1w0"
  5, 10, "t.l0.swamp_n1e0s1w0"
  5, 11, "t.l0.swamp_n0e0s1w0"
  5, 12, "t.l0.swamp_n1e1s0w0"
  5, 13, "t.l0.swamp_n0e1s0w0"
  5, 14, "t.l0.swamp_n1e0s0w0"
  5, 15, "t.l0.swamp_n0e0s0w0"

; Tundra, and whether terrain to north, south, east, west 
; is more tundra:

  6,  0, "t.l0.tundra_n1e1s1w1"
  6,  1, "t.l0.tundra_n0e1s1w1"
  6,  2, "t.l0.tundra_n1e0s1w1"
  6,  3, "t.l0.tundra_n0e0s1w1"
  6,  4, "t.l0.tundra_n1e1s0w1"
  6,  5, "t.l0.tundra_n0e1s0w1"
  6,  6, "t.l0.tundra_n1e0s0w1"
  6,  7, "t.l0.tundra_n0e0s0w1"
  6,  8, "t.l0.tundra_n1e1s1w0"
  6,  9, "t.l0.tundra_n0e1s1w0"
  6, 10, "t.l0.tundra_n1e0s1w0"
  6, 11, "t.l0.tundra_n0e0s1w0"
  6, 12, "t.l0.tundra_n1e1s0w0"
  6, 13, "t.l0.tundra_n0e1s0w0"
  6, 14, "t.l0.tundra_n1e0s0w0"
  6, 15, "t.l0.tundra_n0e0s0w0"

; Ocean, and whether terrain to north, south, east, west 
; is more ocean (else shoreline)

  10,  0, "t.l1.coast_n1e1s1w1"
  10,  1, "t.l1.coast_n0e1s1w1"
  10,  2, "t.l1.coast_n1e0s1w1"
  10,  3, "t.l1.coast_n0e0s1w1"
  10,  4, "t.l1.coast_n1e1s0w1"
  10,  5, "t.l1.coast_n0e1s0w1"
  10,  6, "t.l1.coast_n1e0s0w1"
  10,  7, "t.l1.coast_n0e0s0w1"
  10,  8, "t.l1.coast_n1e1s1w0"
  10,  9, "t.l1.coast_n0e1s1w0"
  10, 10, "t.l1.coast_n1e0s1w0"
  10, 11, "t.l1.coast_n0e0s1w0"
  10, 12, "t.l1.coast_n1e1s0w0"
  10, 13, "t.l1.coast_n0e1s0w0"
  10, 14, "t.l1.coast_n1e0s0w0"
  10, 15, "t.l1.coast_n0e0s0w0"

  10,  0, "t.l1.floor_n1e1s1w1"
  10,  1, "t.l1.floor_n0e1s1w1"
  10,  2, "t.l1.floor_n1e0s1w1"
  10,  3, "t.l1.floor_n0e0s1w1"
  10,  4, "t.l1.floor_n1e1s0w1"
  10,  5, "t.l1.floor_n0e1s0w1"
  10,  6, "t.l1.floor_n1e0s0w1"
  10,  7, "t.l1.floor_n0e0s0w1"
  10,  8, "t.l1.floor_n1e1s1w0"
  10,  9, "t.l1.floor_n0e1s1w0"
  10, 10, "t.l1.floor_n1e0s1w0"
  10, 11, "t.l1.floor_n0e0s1w0"
  10, 12, "t.l1.floor_n1e1s0w0"
  10, 13, "t.l1.floor_n0e1s0w0"
  10, 14, "t.l1.floor_n1e0s0w0"
  10, 15, "t.l1.floor_n0e0s0w0"

  10,  0, "t.l1.lake_n1e1s1w1"
  10,  1, "t.l1.lake_n0e1s1w1"
  10,  2, "t.l1.lake_n1e0s1w1"
  10,  3, "t.l1.lake_n0e0s1w1"
  10,  4, "t.l1.lake_n1e1s0w1"
  10,  5, "t.l1.lake_n0e1s0w1"
  10,  6, "t.l1.lake_n1e0s0w1"
  10,  7, "t.l1.lake_n0e0s0w1"
  10,  8, "t.l1.lake_n1e1s1w0"
  10,  9, "t.l1.lake_n0e1s1w0"
  10, 10, "t.l1.lake_n1e0s1w0"
  10, 11, "t.l1.lake_n0e0s1w0"
  10, 12, "t.l1.lake_n1e1s0w0"
  10, 13, "t.l1.lake_n0e1s0w0"
  10, 14, "t.l1.lake_n1e0s0w0"
  10, 15, "t.l1.lake_n0e0s0w0"

  10,  0, "t.l1.inaccessible_n1e1s1w1"
  10,  1, "t.l1.inaccessible_n0e1s1w1"
  10,  2, "t.l1.inaccessible_n1e0s1w1"
  10,  3, "t.l1.inaccessible_n0e0s1w1"
  10,  4, "t.l1.inaccessible_n1e1s0w1"
  10,  5, "t.l1.inaccessible_n0e1s0w1"
  10,  6, "t.l1.inaccessible_n1e0s0w1"
  10,  7, "t.l1.inaccessible_n0e0s0w1"
  10,  8, "t.l1.inaccessible_n1e1s1w0"
  10,  9, "t.l1.inaccessible_n0e1s1w0"
  10, 10, "t.l1.inaccessible_n1e0s1w0"
  10, 11, "t.l1.inaccessible_n0e0s1w0"
  10, 12, "t.l1.inaccessible_n1e1s0w0"
  10, 13, "t.l1.inaccessible_n0e1s0w0"
  10, 14, "t.l1.inaccessible_n1e0s0w0"
  10, 15, "t.l1.inaccessible_n0e0s0w0"

; Ice Shelves

  11,  0, "t.l2.coast_n1e1s1w1"
  11,  1, "t.l2.coast_n0e1s1w1"
  11,  2, "t.l2.coast_n1e0s1w1"
  11,  3, "t.l2.coast_n0e0s1w1"
  11,  4, "t.l2.coast_n1e1s0w1"
  11,  5, "t.l2.coast_n0e1s0w1"
  11,  6, "t.l2.coast_n1e0s0w1"
  11,  7, "t.l2.coast_n0e0s0w1"
  11,  8, "t.l2.coast_n1e1s1w0"
  11,  9, "t.l2.coast_n0e1s1w0"
  11, 10, "t.l2.coast_n1e0s1w0"
  11, 11, "t.l2.coast_n0e0s1w0"
  11, 12, "t.l2.coast_n1e1s0w0"
  11, 13, "t.l2.coast_n0e1s0w0"
  11, 14, "t.l2.coast_n1e0s0w0"
  11, 15, "t.l2.coast_n0e0s0w0"


  11,  0, "t.l2.floor_n1e1s1w1"
  11,  1, "t.l2.floor_n0e1s1w1"
  11,  2, "t.l2.floor_n1e0s1w1"
  11,  3, "t.l2.floor_n0e0s1w1"
  11,  4, "t.l2.floor_n1e1s0w1"
  11,  5, "t.l2.floor_n0e1s0w1"
  11,  6, "t.l2.floor_n1e0s0w1"
  11,  7, "t.l2.floor_n0e0s0w1"
  11,  8, "t.l2.floor_n1e1s1w0"
  11,  9, "t.l2.floor_n0e1s1w0"
  11, 10, "t.l2.floor_n1e0s1w0"
  11, 11, "t.l2.floor_n0e0s1w0"
  11, 12, "t.l2.floor_n1e1s0w0"
  11, 13, "t.l2.floor_n0e1s0w0"
  11, 14, "t.l2.floor_n1e0s0w0"
  11, 15, "t.l2.floor_n0e0s0w0"


  11,  0, "t.l2.lake_n1e1s1w1"
  11,  1, "t.l2.lake_n0e1s1w1"
  11,  2, "t.l2.lake_n1e0s1w1"
  11,  3, "t.l2.lake_n0e0s1w1"
  11,  4, "t.l2.lake_n1e1s0w1"
  11,  5, "t.l2.lake_n0e1s0w1"
  11,  6, "t.l2.lake_n1e0s0w1"
  11,  7, "t.l2.lake_n0e0s0w1"
  11,  8, "t.l2.lake_n1e1s1w0"
  11,  9, "t.l2.lake_n0e1s1w0"
  11, 10, "t.l2.lake_n1e0s1w0"
  11, 11, "t.l2.lake_n0e0s1w0"
  11, 12, "t.l2.lake_n1e1s0w0"
  11, 13, "t.l2.lake_n0e1s0w0"
  11, 14, "t.l2.lake_n1e0s0w0"
  11, 15, "t.l2.lake_n0e0s0w0"


  11,  0, "t.l2.inaccessible_n1e1s1w1"
  11,  1, "t.l2.inaccessible_n0e1s1w1"
  11,  2, "t.l2.inaccessible_n1e0s1w1"
  11,  3, "t.l2.inaccessible_n0e0s1w1"
  11,  4, "t.l2.inaccessible_n1e1s0w1"
  11,  5, "t.l2.inaccessible_n0e1s0w1"
  11,  6, "t.l2.inaccessible_n1e0s0w1"
  11,  7, "t.l2.inaccessible_n0e0s0w1"
  11,  8, "t.l2.inaccessible_n1e1s1w0"
  11,  9, "t.l2.inaccessible_n0e1s1w0"
  11, 10, "t.l2.inaccessible_n1e0s1w0"
  11, 11, "t.l2.inaccessible_n0e0s1w0"
  11, 12, "t.l2.inaccessible_n1e1s0w0"
  11, 13, "t.l2.inaccessible_n0e1s0w0"
  11, 14, "t.l2.inaccessible_n1e0s0w0"
  11, 15, "t.l2.inaccessible_n0e0s0w0"

; Darkness (unexplored) to north, south, east, west 

 12,  0, "mask.tile"
 12,  1, "tx.darkness_n1e0s0w0"
 12,  2, "tx.darkness_n0e1s0w0"
 12,  3, "tx.darkness_n1e1s0w0"
 12,  4, "tx.darkness_n0e0s1w0"
 12,  5, "tx.darkness_n1e0s1w0"
 12,  6, "tx.darkness_n0e1s1w0"
 12,  7, "tx.darkness_n1e1s1w0"
 12,  8, "tx.darkness_n0e0s0w1"
 12,  9, "tx.darkness_n1e0s0w1"
 12, 10, "tx.darkness_n0e1s0w1"
 12, 11, "tx.darkness_n1e1s0w1"
 12, 12, "tx.darkness_n0e0s1w1"
 12, 13, "tx.darkness_n1e0s1w1"
 12, 14, "tx.darkness_n0e1s1w1"
 12, 15, "tx.darkness_n1e1s1w1"
 12, 16, "tx.fog"



; Rivers (as special type), and whether north, south, east, west 
; also has river or is ocean:

 13,  0, "road.river_s_n0e0s0w0"
 13,  1, "road.river_s_n1e0s0w0"
 13,  2, "road.river_s_n0e1s0w0"
 13,  3, "road.river_s_n1e1s0w0"
 13,  4, "road.river_s_n0e0s1w0"
 13,  5, "road.river_s_n1e0s1w0"
 13,  6, "road.river_s_n0e1s1w0"
 13,  7, "road.river_s_n1e1s1w0"
 13,  8, "road.river_s_n0e0s0w1"
 13,  9, "road.river_s_n1e0s0w1"
 13, 10, "road.river_s_n0e1s0w1"
 13, 11, "road.river_s_n1e1s0w1"
 13, 12, "road.river_s_n0e0s1w1"
 13, 13, "road.river_s_n1e0s1w1"
 13, 14, "road.river_s_n0e1s1w1"
 13, 15, "road.river_s_n1e1s1w1"

; River outlets, river to north, south, east, west 

  14, 12, "road.river_outlet_n"
  14, 13, "road.river_outlet_w"
  14, 14, "road.river_outlet_s"
  14, 15, "road.river_outlet_e"

; Terrain special resources:

 14,  0, "ts.spice"
 14,  1, "ts.furs"
 14,  2, "ts.peat"
 14,  3, "ts.arctic_ivory"
 14,  4, "ts.fruit"
 14,  5, "ts.iron"
 14,  6, "ts.whales"
 14,  7, "ts.wheat"
 14,  8, "ts.pheasant"
 14,  9, "ts.buffalo"
 14, 10, "ts.silk"
 14, 11, "ts.wine"

 15,  0, "ts.seals"
 15,  1, "ts.oasis"
 15,  2, "ts.forest_game"
 15,  3, "ts.grassland_resources"
 15,  4, "ts.coal"
 15,  5, "ts.gems"
 15,  6, "ts.gold"
 15,  7, "ts.fish"
 15,  8, "ts.horses"
 15,  9, "ts.river_resources"
 15, 10, "ts.oil", "ts.arctic_oil"
 15, 11, "ts.tundra_game"

; Terrain Strategic Resources

 15, 12, "ts.aluminum"
 15, 13, "ts.uranium"
 15, 14, "ts.saltpeter"
 15, 15, "ts.elephant"

; Terrain improvements and similar:

 16,  0, "tx.farmland"
 16,  1, "tx.irrigation"
 16,  2, "tx.mine"
 16,  3, "tx.oil_mine"
 16,  4, "tx.pollution"
 16,  5, "tx.fallout"
 16, 13, "tx.oil_rig"

; Bases
 16,  6, "base.buoy_mg"
 16,  7, "extra.ruins_mg"
 16,  8, "tx.village"
 16,  9, "base.airstrip_mg"
 16, 10, "base.airbase_mg"
 16, 11, "base.outpost_mg"
 16, 12, "base.fortress_bg"

; Numbers: city size: (also used for goto)

 17,  0, "city.size_0"
 17,  1, "city.size_1"
 17,  2, "city.size_2"
 17,  3, "city.size_3"
 17,  4, "city.size_4"
 17,  5, "city.size_5"
 17,  6, "city.size_6"
 17,  7, "city.size_7"
 17,  8, "city.size_8"
 17,  9, "city.size_9"
 18,  0, "city.size_00"
 18,  1, "city.size_10"
 18,  2, "city.size_20"
 18,  3, "city.size_30"
 18,  4, "city.size_40"
 18,  5, "city.size_50"
 18,  6, "city.size_60"
 18,  7, "city.size_70"
 18,  8, "city.size_80"
 18,  9, "city.size_90"
 19,  1, "city.size_100"
 19,  2, "city.size_200"
 19,  3, "city.size_300"
 19,  4, "city.size_400"
 19,  5, "city.size_500"
 19,  6, "city.size_600"
 19,  7, "city.size_700"
 19,  8, "city.size_800"
 19,  9, "city.size_900"

; Numbers: city tile food/shields/trade y/g/b

 20,  0, "city.t_food_0"
 20,  1, "city.t_food_1"
 20,  2, "city.t_food_2"
 20,  3, "city.t_food_3"
 20,  4, "city.t_food_4"
 20,  5, "city.t_food_5"
 20,  6, "city.t_food_6"
 20,  7, "city.t_food_7"
 20,  8, "city.t_food_8"
 20,  9, "city.t_food_9"

 21,  0, "city.t_shields_0"
 21,  1, "city.t_shields_1"
 21,  2, "city.t_shields_2"
 21,  3, "city.t_shields_3"
 21,  4, "city.t_shields_4"
 21,  5, "city.t_shields_5"
 21,  6, "city.t_shields_6"
 21,  7, "city.t_shields_7"
 21,  8, "city.t_shields_8"
 21,  9, "city.t_shields_9"

 22,  0, "city.t_trade_0"
 22,  1, "city.t_trade_1"
 22,  2, "city.t_trade_2"
 22,  3, "city.t_trade_3"
 22,  4, "city.t_trade_4"
 22,  5, "city.t_trade_5"
 22,  6, "city.t_trade_6"
 22,  7, "city.t_trade_7"
 22,  8, "city.t_trade_8"
 22,  9, "city.t_trade_9"

; Unit Misc:

  5, 16, "unit.tired"
  5, 16, "unit.lowfuel"
  5, 17, "unit.loaded"
  5, 18, "user.attention"	; Variously crosshair/red-square/arrows
  5, 19, "unit.stack"

; Goto path:

  1, 16, "path.step"            ; turn boundary within path
  2, 16, "path.exhausted_mp"    ; tip of path, no MP left
  3, 16, "path.normal"          ; tip of path with MP remaining
  4, 16, "path.waypoint"

; Unit activity letters:  (note unit icons have just "u.")

  6, 17, "unit.auto_attack",
         "unit.auto_settler"
  6, 18, "unit.connect"
  6, 19, "unit.auto_explore"

  7, 16, "unit.fortifying"
  7, 17, "unit.fortified"
  7, 18, "unit.sentry"
  7, 19, "unit.patrol"

  8, 16, "unit.plant"
  8, 17, "unit.irrigate"
  8, 18, "unit.transform"
  8, 19, "unit.pillage"

  9, 16, "unit.pollution"
  9, 17, "unit.fallout"
  9, 18, "unit.convert"
  9, 19, "unit.goto"


; Unit Activities

 10, 16, "unit.airstrip"
 10, 17, "unit.outpost"
 10, 18, "unit.airbase"
 10, 19, "unit.fortress"
 11, 19, "unit.buoy"

; Road Activities

 11, 16, "unit.road"
 11, 17, "unit.rail"
 11, 18, "unit.maglev"

; Unit hit-point bars: approx percent of hp remaining

 17,  10, "unit.hp_100"
 17,  11, "unit.hp_90"
 17,  12, "unit.hp_80"
 17,  13, "unit.hp_70"
 17,  14, "unit.hp_60"
 17,  15, "unit.hp_50"
 17,  16, "unit.hp_40"
 17,  17, "unit.hp_30"
 17,  18, "unit.hp_20"
 17,  19, "unit.hp_10"
 18,  19, "unit.hp_0"

; Veteran Levels: up to 9 military honors for experienced units

 18, 10, "unit.vet_1"
 18, 11, "unit.vet_2"
 18, 12, "unit.vet_3"
 18, 13, "unit.vet_4"
 18, 14, "unit.vet_5"
 18, 15, "unit.vet_6"
 18, 16, "unit.vet_7"
 18, 17, "unit.vet_8"
 18, 18, "unit.vet_9"



; Unit upkeep in city dialog:
; These should probably be handled differently and have
; a different size...

 19, 10, "upkeep.shield"
 19, 11, "upkeep.shield2"
 19, 12, "upkeep.shield3"
 19, 13, "upkeep.shield4"
 19, 14, "upkeep.shield5"
 19, 15, "upkeep.shield6"
 19, 16, "upkeep.shield7"
 19, 17, "upkeep.shield8"
 19, 18, "upkeep.shield9"
 19, 19, "upkeep.shield10"

 20, 10, "upkeep.unhappy"
 20, 11, "upkeep.unhappy2"
 20, 12, "upkeep.unhappy3"
 20, 13, "upkeep.unhappy4"
 20, 14, "upkeep.unhappy5"
 20, 15, "upkeep.unhappy6"
 20, 16, "upkeep.unhappy7"
 20, 17, "upkeep.unhappy8"
 20, 18, "upkeep.unhappy9"
 20, 19, "upkeep.unhappy10"

 21, 10, "upkeep.food"
 21, 11, "upkeep.food2"
 21, 12, "upkeep.food3"
 21, 13, "upkeep.food4"
 21, 14, "upkeep.food5"
 21, 15, "upkeep.food6"
 21, 16, "upkeep.food7"
 21, 17, "upkeep.food8"
 21, 18, "upkeep.food9"
 21, 19, "upkeep.food10"

 22, 10, "upkeep.gold"
 22, 11, "upkeep.gold2"
 22, 12, "upkeep.gold3"
 22, 13, "upkeep.gold4"
 22, 14, "upkeep.gold5"
 22, 15, "upkeep.gold6"
 22, 16, "upkeep.gold7"
 22, 17, "upkeep.gold8"
 22, 18, "upkeep.gold9"
 22, 19, "upkeep.gold10"

}

[grid_nuke]

x_top_left = 510
y_top_left = 30
dx = 90
dy = 90

tiles = { "row", "column", "tag"
  0, 0, "explode.nuke"
}

[grid_ocean]
x_top_left = 0
y_top_left = 210
dx = 15
dy = 15

tiles = {"row", "column", "tag"

; coast cell sprites.  See doc/README.graphics

  0,  0, "t.l0.coast_cell_u000"
  0,  0, "t.l0.coast_cell_u001"
  0,  0, "t.l0.coast_cell_u100"
  0,  0, "t.l0.coast_cell_u101"

  0,  1, "t.l0.coast_cell_u010"
  0,  1, "t.l0.coast_cell_u011"
  0,  1, "t.l0.coast_cell_u110"
  0,  1, "t.l0.coast_cell_u111"

  0,  2, "t.l0.coast_cell_r000"
  0,  2, "t.l0.coast_cell_r001"
  0,  2, "t.l0.coast_cell_r100"
  0,  2, "t.l0.coast_cell_r101"

  0,  3, "t.l0.coast_cell_r010"
  0,  3, "t.l0.coast_cell_r011"
  0,  3, "t.l0.coast_cell_r110"
  0,  3, "t.l0.coast_cell_r111"

  0,  4, "t.l0.coast_cell_l000"
  0,  4, "t.l0.coast_cell_l001"
  0,  4, "t.l0.coast_cell_l100"
  0,  4, "t.l0.coast_cell_l101"

  0,  5, "t.l0.coast_cell_l010"
  0,  5, "t.l0.coast_cell_l011"
  0,  5, "t.l0.coast_cell_l110"
  0,  5, "t.l0.coast_cell_l111"

  0,  6, "t.l0.coast_cell_d000"
  0,  6, "t.l0.coast_cell_d001"
  0,  6, "t.l0.coast_cell_d100"
  0,  6, "t.l0.coast_cell_d101"

  0,  7, "t.l0.coast_cell_d010"
  0,  7, "t.l0.coast_cell_d011"
  0,  7, "t.l0.coast_cell_d110"
  0,  7, "t.l0.coast_cell_d111"

 ; Deep Ocean fallback to Ocean tiles
  0,  8, "t.l0.floor_cell_u000"
  0,  8, "t.l0.floor_cell_u001"
  0,  8, "t.l0.floor_cell_u100"
  0,  8, "t.l0.floor_cell_u101"

  0,  9, "t.l0.floor_cell_u010"
  0,  9, "t.l0.floor_cell_u011"
  0,  9, "t.l0.floor_cell_u110"
  0,  9, "t.l0.floor_cell_u111"

  0, 10, "t.l0.floor_cell_r000"
  0, 10, "t.l0.floor_cell_r001"
  0, 10, "t.l0.floor_cell_r100"
  0, 10, "t.l0.floor_cell_r101"

  0, 11, "t.l0.floor_cell_r010"
  0, 11, "t.l0.floor_cell_r011"
  0, 11, "t.l0.floor_cell_r110"
  0, 11, "t.l0.floor_cell_r111"

  0, 12, "t.l0.floor_cell_l000"
  0, 12, "t.l0.floor_cell_l001"
  0, 12, "t.l0.floor_cell_l100"
  0, 12, "t.l0.floor_cell_l101"

  0, 13, "t.l0.floor_cell_l010"
  0, 13, "t.l0.floor_cell_l011"
  0, 13, "t.l0.floor_cell_l110"
  0, 13, "t.l0.floor_cell_l111"

  0, 14, "t.l0.floor_cell_d000"
  0, 14, "t.l0.floor_cell_d001"
  0, 14, "t.l0.floor_cell_d100"
  0, 14, "t.l0.floor_cell_d101"

  0, 15, "t.l0.floor_cell_d010"
  0, 15, "t.l0.floor_cell_d011"
  0, 15, "t.l0.floor_cell_d110"
  0, 15, "t.l0.floor_cell_d111"

; Lake fallback to Ocean tiles
  0, 16, "t.l0.lake_cell_u000"
  0, 16, "t.l0.lake_cell_u001"
  0, 16, "t.l0.lake_cell_u100"
  0, 16, "t.l0.lake_cell_u101"

  0, 17, "t.l0.lake_cell_u010"
  0, 17, "t.l0.lake_cell_u011"
  0, 17, "t.l0.lake_cell_u110"
  0, 17, "t.l0.lake_cell_u111"

  0, 18, "t.l0.lake_cell_r000"
  0, 18, "t.l0.lake_cell_r001"
  0, 18, "t.l0.lake_cell_r100"
  0, 18, "t.l0.lake_cell_r101"

  0, 19, "t.l0.lake_cell_r010"
  0, 19, "t.l0.lake_cell_r011"
  0, 19, "t.l0.lake_cell_r110"
  0, 19, "t.l0.lake_cell_r111"

  0, 20, "t.l0.lake_cell_l000"
  0, 20, "t.l0.lake_cell_l001"
  0, 20, "t.l0.lake_cell_l100"
  0, 20, "t.l0.lake_cell_l101"

  0, 21, "t.l0.lake_cell_l010"
  0, 21, "t.l0.lake_cell_l011"
  0, 21, "t.l0.lake_cell_l110"
  0, 21, "t.l0.lake_cell_l111"

  0, 22, "t.l0.lake_cell_d000"
  0, 22, "t.l0.lake_cell_d001"
  0, 22, "t.l0.lake_cell_d100"
  0, 22, "t.l0.lake_cell_d101"

  0, 23, "t.l0.lake_cell_d010"
  0, 23, "t.l0.lake_cell_d011"
  0, 23, "t.l0.lake_cell_d110"
  0, 23, "t.l0.lake_cell_d111"

; Inaccessible fallback to Ocean tiles
  0, 24, "t.l0.inaccessible_cell_u000"
  0, 24, "t.l0.inaccessible_cell_u001"
  0, 24, "t.l0.inaccessible_cell_u100"
  0, 24, "t.l0.inaccessible_cell_u101"

  0, 25, "t.l0.inaccessible_cell_u010"
  0, 25, "t.l0.inaccessible_cell_u011"
  0, 25, "t.l0.inaccessible_cell_u110"
  0, 25, "t.l0.inaccessible_cell_u111"

  0, 26, "t.l0.inaccessible_cell_r000"
  0, 26, "t.l0.inaccessible_cell_r001"
  0, 26, "t.l0.inaccessible_cell_r100"
  0, 26, "t.l0.inaccessible_cell_r101"

  0, 27, "t.l0.inaccessible_cell_r010"
  0, 27, "t.l0.inaccessible_cell_r011"
  0, 27, "t.l0.inaccessible_cell_r110"
  0, 27, "t.l0.inaccessible_cell_r111"

  0, 28, "t.l0.inaccessible_cell_l000"
  0, 28, "t.l0.inaccessible_cell_l001"
  0, 28, "t.l0.inaccessible_cell_l100"
  0, 28, "t.l0.inaccessible_cell_l101"

  0, 29, "t.l0.inaccessible_cell_l010"
  0, 29, "t.l0.inaccessible_cell_l011"
  0, 29, "t.l0.inaccessible_cell_l110"
  0, 29, "t.l0.inaccessible_cell_l111"

  0, 30, "t.l0.inaccessible_cell_d000"
  0, 30, "t.l0.inaccessible_cell_d001"
  0, 30, "t.l0.inaccessible_cell_d100"
  0, 30, "t.l0.inaccessible_cell_d101"

  0, 31, "t.l0.inaccessible_cell_d010"
  0, 31, "t.l0.inaccessible_cell_d011"
  0, 31, "t.l0.inaccessible_cell_d110"
  0, 31, "t.l0.inaccessible_cell_d111"

}
