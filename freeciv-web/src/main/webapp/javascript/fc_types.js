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
var MAX_LEN_NAME = 48;

var FC_INFINITY = (1000 * 1000 * 1000);

var ACTIVITY_IDLE = 0;
var ACTIVITY_POLLUTION = 1;
var ACTIVITY_MINE = 3;
var ACTIVITY_IRRIGATE = 4;
var ACTIVITY_FORTIFIED = 5;
var ACTIVITY_SENTRY = 7;
var ACTIVITY_PILLAGE = 9;
var ACTIVITY_GOTO = 10;
var ACTIVITY_EXPLORE = 11;
var ACTIVITY_TRANSFORM = 12;
var ACTIVITY_FORTIFYING = 15;
var ACTIVITY_FALLOUT = 16;
var ACTIVITY_BASE = 18;			/* building base */
var ACTIVITY_GEN_ROAD = 19;
var ACTIVITY_CONVERT = 20;
var ACTIVITY_LAST = 21;   /* leave this one last */

var IDENTITY_NUMBER_ZERO = 0;

/* Actions */
var ACTION_ESTABLISH_EMBASSY = 0;
var ACTION_SPY_INVESTIGATE_CITY = 1;
var ACTION_SPY_POISON = 2;
var ACTION_SPY_STEAL_GOLD = 3;
var ACTION_SPY_SABOTAGE_CITY = 4;
var ACTION_SPY_TARGETED_SABOTAGE_CITY = 5;
var ACTION_SPY_STEAL_TECH = 6;
var ACTION_SPY_TARGETED_STEAL_TECH = 7;
var ACTION_SPY_INCITE_CITY = 8;
var ACTION_TRADE_ROUTE = 9;
var ACTION_MARKETPLACE = 10;
var ACTION_HELP_WONDER = 11;
var ACTION_SPY_BRIBE_UNIT = 12;
var ACTION_SPY_SABOTAGE_UNIT = 13;
var ACTION_CAPTURE_UNITS = 14;
var ACTION_FOUND_CITY = 15;
var ACTION_JOIN_CITY = 16;
var ACTION_COUNT = 17;

/* Used to signal a move without any action. */
var ACTION_MOVE = ACTION_COUNT;

/* The kind of universals_u (value_union_type was req_source_type).
 * Used in the network protocol. */
var VUT_NONE = 0;
var VUT_ADVANCE = 1;
var VUT_GOVERNMENT = 2;
var VUT_IMPROVEMENT = 3;
var VUT_TERRAIN = 4;
var VUT_NATION = 5;
var VUT_UTYPE = 6;
var VUT_UTFLAG = 7;
var VUT_UCLASS = 8;
var VUT_UCFLAG = 9;
var VUT_OTYPE = 10;
var VUT_SPECIALIST = 11;
var VUT_MINSIZE = 12;		/* Minimum size: at city range means city size */
var VUT_AI_LEVEL = 13;		/* AI level of the player */
var VUT_TERRAINCLASS = 14;	/* More generic terrain type, currently "Land" or "Ocean" */
var VUT_MINYEAR = 15;
var VUT_TERRAINALTER = 16;      /* Terrain alterations that are possible */
var VUT_CITYTILE = 17;          /* Target tile is used by city. */
var VUT_RESOURCE = 18;
var VUT_TERRFLAG = 19;
var VUT_NATIONALITY = 20;
var VUT_BASEFLAG = 21;
var VUT_ROADFLAG = 22;
var VUT_EXTRA = 23;
var VUT_TECHFLAG = 24;
var VUT_ACHIEVEMENT = 25;
var VUT_DIPLREL = 26;
var VUT_MAXTILEUNITS = 27;
var VUT_STYLE = 28;
var VUT_MINCULTURE = 29;
var VUT_UNITSTATE = 30;
var VUT_MINMOVES = 31;
var VUT_MINVETERAN = 32;
var VUT_MINHP = 33;
var VUT_AGE = 34
var VUT_COUNT = 35;             /* Keep this last. */

/* Sometimes we don't know (or don't care) if some requirements for effect
 * are currently fulfilled or not. This enum tells lower level functions
 * how to handle uncertain requirements.
 */
var RPT_POSSIBLE = 0; /* We want to know if it is possible that effect is active */
var RPT_CERTAIN = 1;  /* We want to know if it is certain that effect is active  */

var O_FOOD = 0;
var O_SHIELD = 1;
var O_TRADE = 2;
var O_GOLD = 3;
var O_LUXURY = 4;
var O_SCIENCE = 5;
