/********************************************************************** 
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

/* This must be in same order as names in helpdata.c */
enum help_page_type { HELP_ANY, HELP_TEXT, HELP_UNIT, HELP_IMPROVEMENT,
		      HELP_WONDER, HELP_TECH, HELP_TERRAIN,
                      HELP_GOVERNMENT, HELP_RULESET, HELP_LAST };

void popup_help_dialog_string(const char *item);
void popup_help_dialog_typed(const char *item, enum help_page_type);
void popdown_help_dialog(void);

#define HELP_PLAYING_ITEM "Strategy and tactics"
#define HELP_LANGUAGES_ITEM "Languages"
#define HELP_CONNECTING_ITEM "Connecting"
#define HELP_CHATLINE_ITEM "Chatline"
#define HELP_WORKLIST_EDITOR_ITEM "Worklist Editor"
#define HELP_CMA_ITEM "Citizen Governor"
#define HELP_CONTROLS_ITEM "Controls"
#define HELP_RULESET_ITEM "Ruleset"
#define HELP_IMPROVEMENTS_ITEM "City Improvements"
#define HELP_UNITS_ITEM "Units"
#define HELP_COMBAT_ITEM "Combat"
#define HELP_ZOC_ITEM "Zones of Control"
#define HELP_TECHS_ITEM "Technology"
#define HELP_TERRAIN_ITEM "Terrain"
#define HELP_WONDERS_ITEM "Wonders of the World"
#define HELP_GOVERNMENT_ITEM "Government"
#define HELP_HAPPINESS_ITEM "Happiness"
#define HELP_SPACE_RACE_ITEM "Space Race"
#define HELP_COPYING_ITEM "Copying"
#define HELP_ABOUT_ITEM "About"

#endif  /* FC__HELPDLG_G_H */
