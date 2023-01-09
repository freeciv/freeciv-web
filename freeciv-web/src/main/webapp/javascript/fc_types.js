/***********************************************************************
    Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

var TRUE = true;
var FALSE = false;

var TRI_NO = 0;
var TRI_YES = 1;
var TRI_MAYBE = 2;

var MAX_NUM_ITEMS = 200;
var MAX_NUM_ADVANCES = 250;
var MAX_NUM_UNITS = 250;
var MAX_NUM_BUILDINGS = 200;
var MAX_EXTRA_TYPES = 128;
var MAX_LEN_NAME = 48;
var MAX_LEN_CITYNAME = 50;

var FC_INFINITY = (1000 * 1000 * 1000);

var ACTIVITY_IDLE = 0;
var ACTIVITY_POLLUTION = 1;
var ACTIVITY_MINE = 2;
var ACTIVITY_IRRIGATE = 3;
var ACTIVITY_FORTIFIED = 4;
var ACTIVITY_SENTRY = 5;
var ACTIVITY_PILLAGE = 6;
var ACTIVITY_GOTO = 7;
var ACTIVITY_EXPLORE = 8;
var ACTIVITY_TRANSFORM = 9;
var ACTIVITY_FORTIFYING = 10;
var ACTIVITY_FALLOUT = 11;
var ACTIVITY_BASE = 12;        /* Building base */
var ACTIVITY_GEN_ROAD = 13;
var ACTIVITY_CONVERT = 14;
var ACTIVITY_CULTIVATE = 15;
var ACTIVITY_PLANT = 16;
var ACTIVITY_CLEAN = 17;
var ACTIVITY_LAST = 18;        /* Leave this one last */

/* enum action_result */
var ACTRES_ESTABLISH_EMBASSY = 0;
var ACTRES_SPY_INVESTIGATE_CITY = 1;
var ACTRES_SPY_POISON = 2;
var ACTRES_SPY_STEAL_GOLD = 3;
var ACTRES_SPY_SABOTAGE_CITY = 4;
var ACTRES_SPY_TARGETED_SABOTAGE_CITY = 5;
var ACTRES_SPY_SABOTAGE_CITY_PRODUCTION = 6;
var ACTRES_SPY_STEAL_TECH = 7;
var ACTRES_SPY_TARGETED_STEAL_TECH = 8;
var ACTRES_SPY_INCITE_CITY = 9;
var ACTRES_TRADE_ROUTE = 10;
var ACTRES_MARKETPLACE = 11;
var ACTRES_HELP_WONDER = 12;
var ACTRES_SPY_BRIBE_UNIT = 13;
var ACTRES_SPY_SABOTAGE_UNIT = 14;
var ACTRES_CAPTURE_UNITS = 15;
var ACTRES_FOUND_CITY = 16;
var ACTRES_JOIN_CITY = 17;
var ACTRES_STEAL_MAPS = 18;
var ACTRES_BOMBARD = 19;
var ACTRES_SPY_NUKE = 20;
var ACTRES_NUKE = 21;
var ACTRES_NUKE_UNITS = 22;
var ACTRES_DESTROY_CITY = 23;
var ACTRES_EXPEL_UNIT = 24;
var ACTRES_RECYCLE_UNIT = 25;
var ACTRES_DISBAND_UNIT = 26;
var ACTRES_HOME_CITY = 27;
var ACTRES_UPGRADE_UNIT = 28;
var ACTRES_PARADROP = 29;
var ACTRES_AIRLIFT = 30;
var ACTRES_ATTACK = 31;
var ACTRES_STRIKE_BUILDING = 32;
var ACTRES_STRIKE_PRODUCTION = 33;
var ACTRES_CONQUER_CITY = 34;
var ACTRES_HEAL_UNIT = 35;
var ACTRES_TRANSFORM_TERRAIN = 36;
var ACTRES_CULTIVATE = 37;
var ACTRES_PLANT = 38;
var ACTRES_PILLAGE = 39;
var ACTRES_FORTIFY = 40;
var ACTRES_ROAD = 41;
var ACTRES_CONVERT = 42;
var ACTRES_BASE = 43;
var ACTRES_MINE = 44;
var ACTRES_IRRIGATE = 45;
var ACTRES_CLEAN_POLLUTION = 46;
var ACTRES_CLEAN_FALLOUT = 47;
var ACTRES_TRANSPORT_ALIGHT = 48;
var ACTRES_TRANSPORT_UNLOAD = 49;
var ACTRES_TRANSPORT_DISEMBARK = 50;
var ACTRES_TRANSPORT_BOARD = 51;
var ACTRES_TRANSPORT_EMBARK = 52;
var ACTRES_SPY_SPREAD_PLAGUE = 53;
var ACTRES_SPY_ATTACK = 54;
var ACTRES_CONQUER_EXTRAS = 55;
var ACTRES_HUT_ENTER = 56;
var ACTRES_HUT_FRIGHTEN = 57;
var ACTRES_UNIT_MOVE = 58;
var ACTRES_PARADROP_CONQUER = 59;
var ACTRES_HOMELESS = 60;
var ACTRES_WIPE_UNITS = 61;
var ACTRES_SPY_ESCAPE = 62;
var ACTRES_TRANSPORT_LOAD = 63;
var ACTRES_NONE = 64;

/* enum action_sub_result */
var ACT_SUB_RES_HUT_ENTER = 0;
var ACT_SUB_RES_HUT_FRIGHTEN = 1;
var ACT_SUB_RES_MAY_EMBARK = 2;
var ACT_SUB_RES_NON_LETHAL = 3;
var ACT_SUB_RES_COUNT = 4;

var IDENTITY_NUMBER_ZERO = 0;

/* Corresponds to the enum action_target_kind */
var ATK_CITY  = 0;
var ATK_UNIT  = 1;
var ATK_UNITS = 2;
var ATK_TILE  = 3;
var ATK_EXTRAS = 4;
var ATK_SELF  = 5;
var ATK_COUNT = 6;

/* Corresponds to the enum action_sub_target_kind */
var ASTK_NONE = 0;
var ASTK_BUILDING = 1;
var ASTK_TECH = 2;
var ASTK_EXTRA = 3;
var ASTK_EXTRA_NOT_THERE = 4;
var ASTK_COUNT = 5;

/* Actions */
var ACTION_ESTABLISH_EMBASSY = 0;
var ACTION_ESTABLISH_EMBASSY_STAY = 1;
var ACTION_SPY_INVESTIGATE_CITY = 2;
var ACTION_INV_CITY_SPEND = 3;
var ACTION_SPY_POISON = 4;
var ACTION_SPY_POISON_ESC = 5;
var ACTION_SPY_STEAL_GOLD = 6;
var ACTION_SPY_STEAL_GOLD_ESC = 7;
var ACTION_SPY_SABOTAGE_CITY = 8;
var ACTION_SPY_SABOTAGE_CITY_ESC = 9;
var ACTION_SPY_TARGETED_SABOTAGE_CITY = 10;
var ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC = 11;
var ACTION_SPY_SABOTAGE_CITY_PRODUCTION = 12;
var ACTION_SPY_SABOTAGE_CITY_PRODUCTION_ESC = 13;
var ACTION_SPY_STEAL_TECH = 14;
var ACTION_SPY_STEAL_TECH_ESC = 15;
var ACTION_SPY_TARGETED_STEAL_TECH = 16;
var ACTION_SPY_TARGETED_STEAL_TECH_ESC = 17;
var ACTION_SPY_INCITE_CITY = 18;
var ACTION_SPY_INCITE_CITY_ESC = 19;
var ACTION_TRADE_ROUTE = 20;
var ACTION_MARKETPLACE = 21;
var ACTION_HELP_WONDER = 22;
var ACTION_SPY_BRIBE_UNIT = 23;
var ACTION_CAPTURE_UNITS = 24;
var ACTION_SPY_SABOTAGE_UNIT = 25;
var ACTION_SPY_SABOTAGE_UNIT_ESC = 26;
var ACTION_FOUND_CITY = 27;
var ACTION_JOIN_CITY = 28;
var ACTION_STEAL_MAPS = 29;
var ACTION_STEAL_MAPS_ESC = 30;
var ACTION_SPY_NUKE = 31;
var ACTION_SPY_NUKE_ESC = 32;
var ACTION_NUKE = 33;
var ACTION_NUKE_CITY = 34;
var ACTION_NUKE_UNITS = 35;
var ACTION_DESTROY_CITY = 36;
var ACTION_EXPEL_UNIT = 37;
var ACTION_DISBAND_UNIT_RECOVER = 38;
var ACTION_DISBAND_UNIT = 39;
var ACTION_HOME_CITY = 40;
var ACTION_HOMELESS = 41;
var ACTION_UPGRADE_UNIT = 42;
var ACTION_CONVERT = 43;
var ACTION_AIRLIFT = 44;
var ACTION_ATTACK = 45;
var ACTION_SUICIDE_ATTACK = 46;
var ACTION_STRIKE_BUILDING = 47;
var ACTION_STRIKE_PRODUCTION = 48;
var ACTION_CONQUER_CITY = 49;
var ACTION_CONQUER_CITY2 = 50;
var ACTION_CONQUER_CITY3 = 51;
var ACTION_CONQUER_CITY4 = 52;
var ACTION_BOMBARD = 53;
var ACTION_BOMBARD2 = 54;
var ACTION_BOMBARD3 = 55;
var ACTION_BOMBARD_LETHAL = 56;
var ACTION_FORTIFY = 57;
var ACTION_CULTIVATE = 58;
var ACTION_PLANT = 59;
var ACTION_TRANSFORM_TERRAIN = 60;
var ACTION_ROAD = 61;
var ACTION_IRRIGATE = 62;
var ACTION_MINE = 63;
var ACTION_BASE = 64;
var ACTION_PILLAGE = 65;
var ACTION_CLEAN_POLLUTION = 66;
var ACTION_CLEAN_FALLOUT = 67;
var ACTION_TRANSPORT_BOARD = 68;
var ACTION_TRANSPORT_BOARD2 = 69;
var ACTION_TRANSPORT_BOARD3 = 70;
var ACTION_TRANSPORT_ALIGHT = 71;
var ACTION_TRANSPORT_EMBARK = 72;
var ACTION_TRANSPORT_EMBARK2 = 73;
var ACTION_TRANSPORT_EMBARK3 = 74;
var ACTION_TRANSPORT_EMBARK4 = 75;
var ACTION_TRANSPORT_DISEMBARK1 = 76;
var ACTION_TRANSPORT_DISEMBARK2 = 77;
var ACTION_TRANSPORT_DISEMBARK3 = 78;
var ACTION_TRANSPORT_DISEMBARK4 = 79;
var ACTION_TRANSPORT_LOAD = 80;
var ACTION_TRANSPORT_LOAD2 = 81;
var ACTION_TRANSPORT_LOAD3 = 82;
var ACTION_TRANSPORT_UNLOAD = 83;
var ACTION_SPY_SPREAD_PLAGUE = 84;
var ACTION_SPY_ATTACK = 85;
var ACTION_CONQUER_EXTRAS = 86;
var ACTION_CONQUER_EXTRAS2 = 87;
var ACTION_CONQUER_EXTRAS3 = 88;
var ACTION_CONQUER_EXTRAS4 = 89;
var ACTION_HUT_ENTER = 90;
var ACTION_HUT_ENTER2 = 91;
var ACTION_HUT_ENTER3 = 92;
var ACTION_HUT_ENTER4 = 93;
var ACTION_HUT_FRIGHTEN = 94;
var ACTION_HUT_FRIGHTEN2 = 95;
var ACTION_HUT_FRIGHTEN3 = 96;
var ACTION_HUT_FRIGHTEN4 = 97;
var ACTION_HEAL_UNIT = 98;
var ACTION_HEAL_UNIT2 = 99;
var ACTION_PARADROP = 100;
var ACTION_PARADROP_CONQUER = 101;
var ACTION_PARADROP_FRIGHTEN = 102;
var ACTION_PARADROP_FRIGHTEN_CONQUER = 103;
var ACTION_PARADROP_ENTER = 104;
var ACTION_PARADROP_ENTER_CONQUER = 105;
var ACTION_WIPE_UNITS = 106;
var ACTION_SPY_ESCAPE = 107;
var ACTION_UNIT_MOVE = 108;
var ACTION_UNIT_MOVE2 = 109;
var ACTION_UNIT_MOVE3 = 110;
var ACTION_CLEAN = 111;
var ACTION_USER_ACTION1 = 112;
var ACTION_USER_ACTION2 = 113;
var ACTION_USER_ACTION3 = 114;
var ACTION_USER_ACTION4 = 115;
var ACTION_COUNT = 116;

/* The action_decision enum */
/* Doesn't need the player to decide what action to take. */
var ACT_DEC_NOTHING = 0;
/* Wants a decision because of something done to the actor. */
var ACT_DEC_PASSIVE = 1;
/* Wants a decision because of something the actor did. */
var ACT_DEC_ACTIVE = 2;

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
var VUT_GOOD = 18;
var VUT_TERRFLAG = 19;
var VUT_NATIONALITY = 20;
var VUT_ROADFLAG = 21;
var VUT_EXTRA = 22;
var VUT_TECHFLAG = 23;
var VUT_ACHIEVEMENT = 24;
var VUT_DIPLREL = 25;
var VUT_MAXTILEUNITS = 26;
var VUT_STYLE = 27;
var VUT_MINCULTURE = 28;
var VUT_UNITSTATE = 29;
var VUT_MINMOVES = 30;
var VUT_MINVETERAN = 31;
var VUT_MINHP = 32;
var VUT_AGE = 33;
var VUT_NATIONGROUP = 34;
var VUT_TOPO = 35;
var VUT_IMPR_GENUS = 36;
var VUT_ACTION = 37;
var VUT_MINTECHS = 38;
var VUT_EXTRAFLAG = 39;
var VUT_MINCALFRAG = 40;
var VUT_SERVERSETTING = 41;
var VUT_CITYSTATUS = 42;
var VUT_MINFOREIGNPCT = 43;
var VUT_ACTIVITY = 44;
var VUT_DIPLREL_TILE = 45;
var VUT_DIPLREL_TILE_O = 46;
var VUT_DIPLREL_UNITANY = 47;
var VUT_DIPLREL_UNITANY_O = 48;
var VUT_COUNT = 49;             /* Keep this last. */

/* Freeciv's gui_type enum */
/* Used for options which do not belong to any gui. */
var GUI_STUB    = 0;
var GUI_GTK2    = 1;
var GUI_GTK3    = 2;
var GUI_GTK3_22 = 3;
/* GUI_SDL remains for now for keeping client options alive until
 * user has migrated them to sdl2-client */
var GUI_SDL     = 4;
var GUI_QT      = 5;
var GUI_SDL2    = 6;
var GUI_WEB     = 7;
var GUI_GTK3x   = 8;

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

/* vision_layer enum */
var V_MAIN = 0;
var V_INVIS = 1;
var V_SUBSURFACE = 2;
var V_COUNT = 3;

/* causes for extra */
var EC_IRRIGATION = 0;
var EC_MINE = 1;
var EC_ROAD = 2;
var EC_BASE = 3;
var EC_POLLUTION = 4;
var EC_FALLOUT = 5;
var EC_HUT = 6;
var EC_APPEARANCE = 7;
var EC_RESOURCE = 8;

/* causes for extra removal */
var ERM_PILLAGE = 0;
var ERM_CLEANPOLLUTION = 1;
var ERM_CLEANFALLOUT = 2;
var ERM_DISAPPEARANCE = 3;

/* barbarian types */
var NOT_A_BARBARIAN = 0;
var LAND_BARBARIAN = 1;
var SEA_BARBARIAN = 2;
var ANIMAL_BARBARIAN = 3;
var LAND_AND_SEA_BARBARIAN = 4;

var CAPITAL_NOT = 0;
var CAPITAL_SECONDARY = 1;
var CAPITAL_PRIMARY = 2;
