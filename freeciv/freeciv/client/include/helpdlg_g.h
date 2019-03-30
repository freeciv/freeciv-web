/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__HELPDLG_G_H
#define FC__HELPDLG_G_H

#include "gui_proto_constructor.h"

/* This must be in same order as names in helpdata.c */
enum help_page_type { HELP_ANY, HELP_TEXT, HELP_UNIT, HELP_IMPROVEMENT,
		      HELP_WONDER, HELP_TECH, HELP_TERRAIN, HELP_EXTRA,
                      HELP_GOODS, HELP_SPECIALIST, HELP_GOVERNMENT,
                      HELP_RULESET, HELP_TILESET, HELP_NATIONS,
                      HELP_MULTIPLIER, HELP_LAST };

GUI_FUNC_PROTO(void, popup_help_dialog_string, const char *item)
GUI_FUNC_PROTO(void, popup_help_dialog_typed, const char *item,
               enum help_page_type)
GUI_FUNC_PROTO(void, popdown_help_dialog, void)

/* TRANS: "Overview" topic in built-in help */
#define HELP_OVERVIEW_ITEM N_("?help:Overview")
#define HELP_PLAYING_ITEM N_("Strategy and Tactics")
#define HELP_LANGUAGES_ITEM N_("Languages")
#define HELP_CONNECTING_ITEM N_("Connecting")
#define HELP_CHATLINE_ITEM N_("Chatline")
#define HELP_WORKLIST_EDITOR_ITEM N_("Worklist Editor")
#define HELP_CMA_ITEM N_("Citizen Governor")
#define HELP_CONTROLS_ITEM N_("Controls")
#define HELP_RULESET_ITEM N_("About Current Ruleset")
#define HELP_TILESET_ITEM N_("About Current Tileset")
#define HELP_NATIONS_ITEM N_("About Nations")
#define HELP_ECONOMY_ITEM N_("Economy")
#define HELP_CITIES_ITEM N_("Cities")
#define HELP_IMPROVEMENTS_ITEM N_("City Improvements")
#define HELP_UNITS_ITEM N_("Units")
#define HELP_COMBAT_ITEM N_("Combat")
#define HELP_ZOC_ITEM N_("Zones of Control")
#define HELP_TECHS_ITEM N_("Technology")
#define HELP_EXTRAS_ITEM N_("Extras")
#define HELP_TERRAIN_ITEM N_("Terrain")
#define HELP_WONDERS_ITEM N_("Wonders of the World")
#define HELP_GOVERNMENT_ITEM N_("Government")
#define HELP_DIPLOMACY_ITEM N_("Diplomacy")
#define HELP_SPACE_RACE_ITEM N_("Space Race")
#define HELP_COPYING_ITEM N_("Copying")
#define HELP_ABOUT_ITEM N_("About Freeciv")
#define HELP_MULTIPLIER_ITEM N_("Policies")

#endif  /* FC__HELPDLG_G_H */
