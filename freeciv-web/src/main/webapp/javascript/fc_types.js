/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

var TRUE = true;
var FALSE = false;

var MAX_NUM_ITEMS = 200;

var FC_INFINITY = (1000 * 1000 * 1000);

var ACTIVITY_IDLE = 0;
var ACTIVITY_POLLUTION = 1;
var ACTIVITY_OLD_ROAD = 2;
var ACTIVITY_MINE = 3;
var ACTIVITY_IRRIGATE = 4;
var ACTIVITY_FORTIFIED = 5;
var ACTIVITY_FORTRESS = 6;
var ACTIVITY_SENTRY = 7;
var ACTIVITY_OLD_RAILROAD = 8;
var ACTIVITY_PILLAGE = 9;
var ACTIVITY_GOTO = 10;
var ACTIVITY_EXPLORE = 11;
var ACTIVITY_TRANSFORM = 12;
var ACTIVITY_UNKNOWN = 13;		/* savegame compatability. */
var ACTIVITY_AIRBASE = 14;
var ACTIVITY_FORTIFYING = 15;
var ACTIVITY_FALLOUT = 16;
var ACTIVITY_PATROL_UNUSED = 17;		/* savegame compatability. */
var ACTIVITY_BASE = 18;			/* building base */
var ACTIVITY_GEN_ROAD = 19;
var ACTIVITY_CONVERT = 20;
var ACTIVITY_LAST = 21;   /* leave this one last */




/* The kind of universals_u (value_union_type was req_source_type).
 * Note: order must correspond to universal_names[] in requirements.c.
 */
var VUT_NONE = 0;
var VUT_ADVANCE = 1;
var VUT_GOVERNMENT = 2;
var VUT_IMPROVEMENT = 3;
var VUT_SPECIAL = 4;
var VUT_TERRAIN = 5;
var VUT_NATION = 6;
var VUT_UTYPE = 7;
var VUT_UTFLAG = 8;
var VUT_UCLASS = 9;
var VUT_UCFLAG = 10;
var VUT_OTYPE = 11;
var VUT_SPECIALIST = 12;
var VUT_MINSIZE = 13;		/* Minimum size: at city range means city size */
var VUT_AI_LEVEL = 14;		/* AI level of the player */
var VUT_TERRAINCLASS = 15;	/* More generic terrain type, currently "Land" or "Ocean" */
var VUT_BASE = 16;
var VUT_MINYEAR = 17;
var VUT_TERRAINALTER = 18;     /* Terrain alterations that are possible */
var VUT_CITYTILE = 19;         /* Target tile is used by city */
var VUT_LAST = 20;

/* Sometimes we don't know (or don't care) if some requirements for effect
 * are currently fulfilled or not. This enum tells lower level functions
 * how to handle uncertain requirements.
 */
var RPT_POSSIBLE = 0; /* We want to know if it is possible that effect is active */
var RPT_CERTAIN = 1;  /* We want to know if it is certain that effect is active  */

