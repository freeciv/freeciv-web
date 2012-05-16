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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "movement.h"
#include "support.h"

/* common */
#include "game.h"
#include "government.h"
#include "map.h"
#include "unit.h"
#include "unitlist.h"

/* client */
#include "chatline.h"
#include "cityrep.h"
#include "client_main.h"
#include "climisc.h"	/* can_units_do_connect */
#include "clinet.h"
#include "control.h" /* request_xxx and get_unit_in_focus */
#include "dialogs.h"
#include "finddlg.h"
#include "gotodlg.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "mapctrl.h" 
#include "messagedlg.h"
#include "messagewin.h"
#include "optiondlg.h"
#include "options.h"
#include "packhand.h"
#include "plrdlg.h"
#include "ratesdlg.h"
#include "repodlgs.h"
#include "spaceshipdlg.h"
#include "wldlg.h"

#include "menu.h"

/* stuff for run-time mutable menu text */

#define MAX_LEN_TERR_NAME_DISP  9
#define MAX_VARIANTS  3
#define ACEL_SPACE  1

#define TEXT_ORDER_CITY_BUILD   0
#define TEXT_ORDER_CITY_ADD_TO  1

#define TEXT_ORDER_ROAD_ROAD      0
#define TEXT_ORDER_ROAD_RAILROAD  1

#define TEXT_ORDER_IRRIGATE_IRRIGATE   0
#define TEXT_ORDER_IRRIGATE_FARMLAND   1
#define TEXT_ORDER_IRRIGATE_CHANGE_TO  2

#define TEXT_ORDER_MINE_MINE       0
#define TEXT_ORDER_MINE_CHANGE_TO  1

#define TEXT_ORDER_TRANSFORM_TERRAIN       0
#define TEXT_ORDER_TRANSFORM_TRANSFORM_TO  1

#define TEXT_ORDER_POLLUTION_POLLUTION  0
#define TEXT_ORDER_POLLUTION_PARADROP   1

/* The acel strings, is the string that show which is the acelerator key
 * in the menus. No check is made to ensure that this is the acelerator
 * key. This is use for translation-layout porpouses in i18n texts
 * added by Serrada */

struct MenuEntry {
  char *text[MAX_VARIANTS+1];
  char *acel;               /* Acelerator key */
  enum  MenuID id;
  Widget w;
};

struct Menu {
  Widget button, shell;
  int maxitemlen;
  int maxacellen;
  struct MenuEntry *entries;
};

static struct Menu *menus[MENU_LAST];

static struct MenuEntry game_menu_entries[]={
    { { N_("Local Options"), 0        },      "", MENU_GAME_OPTIONS, 0 },
    { { N_("Message Options"), 0      },      "", MENU_GAME_MSG_OPTIONS, 0 },
    { { N_("Save Settings"), 0        },      "", MENU_GAME_SAVE_SETTINGS, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Server Options"), 0       },      "", MENU_GAME_SERVER_OPTIONS, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Export Log"), 0           },      "", MENU_GAME_OUTPUT_LOG, 0 },
    { { N_("Clear Log"), 0            },      "", MENU_GAME_CLEAR_OUTPUT, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Disconnect"), 0           },      "", MENU_GAME_DISCONNECT, 0 },
    { { N_("Quit"), 0                 },      "Ctl-Q", MENU_GAME_QUIT, 0 },
    { { 0,                            },       0, MENU_END_OF_LIST, 0 }
};

static struct MenuEntry government_menu_entries[]={
    { { N_("Tax Rates..."), 0         },     "Ctl-T", MENU_GOVERNMENT_RATES, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Find City"), 0            },     "Ctl-F", MENU_GOVERNMENT_FIND_CITY, 0 },
    { { N_("Worklists"), 0            },     "Ctl-L", MENU_GOVERNMENT_WORKLISTS, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Revolution..."), 0        },     "Ctl-Shift-R", MENU_GOVERNMENT_REVOLUTION, 0 },
    { { 0,                            },       0, MENU_END_OF_LIST, 0 }
};

/* One entry for every government, appended to the government menu. */
static int num_government_entries = 0;
static Widget government_widgets[G_MAGIC];

static struct MenuEntry view_menu_entries[]={
    { { N_("Map Grid"), 0             }, "ctl-g", MENU_VIEW_SHOW_MAP_GRID, 0 },
    { { N_("National Borders"), 0     }, "ctl-b",
      MENU_VIEW_SHOW_NATIONAL_BORDERS, 0 },
    { { N_("City Names"), 0           }, "ctl-n", MENU_VIEW_SHOW_CITY_NAMES, 0 },
    { { N_("City Growth"), 0          }, "ctl-r",
      MENU_VIEW_SHOW_CITY_GROWTH, 0 },
    { { N_("City Production Levels"), 0 }, "ctl-p", MENU_VIEW_SHOW_CITY_PRODUCTIONS, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Terrain"), 0              },      "", MENU_VIEW_SHOW_TERRAIN, 0 },
    { { N_("Coastline"), 0            },      "", MENU_VIEW_SHOW_COASTLINE, 0 },
    { { N_("Roads & Rails"), 0        },      "", MENU_VIEW_SHOW_ROADS_RAILS, 0 },
    { { N_("Irrigation"), 0           },      "", MENU_VIEW_SHOW_IRRIGATION, 0 },
    { { N_("Mines"), 0                },      "", MENU_VIEW_SHOW_MINES, 0 },
    { { N_("Fortress & Airbase"), 0   },      "", MENU_VIEW_SHOW_FORTRESS_AIRBASE, 0 },
    { { N_("Specials"), 0             },      "", MENU_VIEW_SHOW_SPECIALS, 0 },
    { { N_("Pollution & Fallout"), 0  },      "", MENU_VIEW_SHOW_POLLUTION, 0 },
    { { N_("Cities"), 0               },      "", MENU_VIEW_SHOW_CITIES, 0 },
    { { N_("Units"), 0                },      "", MENU_VIEW_SHOW_UNITS, 0 },
    { { N_("Focus Unit"), 0           },      "", MENU_VIEW_SHOW_FOCUS_UNIT, 0 },
    { { N_("Fog of War"), 0           },      "", MENU_VIEW_SHOW_FOG_OF_WAR, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Center View"), 0          },     "c", MENU_VIEW_CENTER_VIEW, 0 },
    { { 0,                            },       0, MENU_END_OF_LIST, 0 }
};

static struct MenuEntry order_menu_entries[]={
    { { N_("Build City"),
        N_("Add to City"), 0          },     "b", MENU_ORDER_BUILD_CITY, 0 },
    { { N_("Build Road"),
        N_("Build Railroad"), 0       },     "r", MENU_ORDER_ROAD, 0 },
    { { N_("Build Irrigation"),
        N_("Build Farmland"),
        N_("Change to %s"), 0         },     "i", MENU_ORDER_IRRIGATE, 0 },
    { { N_("Build Mine"),
        N_("Change to %s"), 0         },     "m", MENU_ORDER_MINE, 0 },
    { { N_("Transform Terrain"),
        N_("Transform to %s"), 0      },     "o", MENU_ORDER_TRANSFORM, 0 },
    { { N_("Build Fortress"), 0       },     "f", MENU_ORDER_FORTRESS, 0 },
    { { N_("Build Airbase"), 0        },     "e", MENU_ORDER_AIRBASE, 0 },
    { { N_("Clean Pollution"),
	N_("Drop Paratrooper"), 0             },     "p", MENU_ORDER_POLLUTION, 0 },
    { { N_("Clean Nuclear Fallout"), 0},     "n", MENU_ORDER_FALLOUT, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Fortify Unit"), 0         },     "f", MENU_ORDER_FORTIFY, 0 },
    { { N_("Sentry Unit"), 0          },     "s", MENU_ORDER_SENTRY, 0 },
    { { N_("Pillage"), 0              },     "P", MENU_ORDER_PILLAGE, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Set Home City"), 0        },     "h", MENU_ORDER_HOMECITY, 0 },
    { { N_("Unload All From Transporter"), 0 }, "T",
      MENU_ORDER_UNLOAD_TRANSPORTER, 0 },
    { { N_("Load Unit"), 0            },     "l", MENU_ORDER_LOAD, 0 },
    { { N_("Unload Unit"), 0          },     "u", MENU_ORDER_UNLOAD, 0 },
    { { N_("Unsentry All On Tile"), 0 },     "S", MENU_ORDER_WAKEUP_OTHERS, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Auto Settler"), 0         },     "a", MENU_ORDER_AUTO_SETTLER, 0 },
    { { N_("Auto Attack"), 0          },     "a", MENU_ORDER_AUTO_ATTACK, 0 },
    { { N_("Auto Explore"), 0         },     "x", MENU_ORDER_AUTO_EXPLORE, 0 },
    { { N_("Connect With Road"), 0    },     "R", MENU_ORDER_CONNECT_ROAD, 0 },
    { { N_("Connect With Rail"), 0    },     "L", MENU_ORDER_CONNECT_RAIL, 0 },
    { { N_("Connect With Irrigation"), 0 },  "I", MENU_ORDER_CONNECT_IRRIGATE, 0 },
    { { N_("Patrol"), 0               },     "q", MENU_ORDER_PATROL, 0 },
    { { N_("Go to Tile"), 0           },     "g", MENU_ORDER_GOTO, 0 },
    { { N_("Go to/Airlift to City..."), 0 }, "t", MENU_ORDER_GOTO_CITY, 0 },
    { { N_("Return to Nearest City"), 0 },   "G", MENU_ORDER_RETURN, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Disband Unit"), 0         },     "D", MENU_ORDER_DISBAND, 0 },
    { { N_("Help Build Wonder"), 0    },     "b", MENU_ORDER_BUILD_WONDER, 0 },
    { { N_("Establish Trade Route"), 0 },    "r", MENU_ORDER_TRADEROUTE, 0 },
    { { N_("Diplomat/Spy Actions"), 0 },     "d", MENU_ORDER_DIPLOMAT_DLG, 0},
    { { N_("Explode Nuclear"), 0      },     "N", MENU_ORDER_NUKE, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Select Same Type Everywhere"), 0 }, "X", MENU_ORDER_SELECT_SAME_TYPE, 0 },
    { { N_("Wait"), 0                 },     "w", MENU_ORDER_WAIT, 0 },
    { { N_("Done"), 0                 },   "spc", MENU_ORDER_DONE, 0 },
    { { 0,                            },       0, MENU_END_OF_LIST, 0 }
};

static struct MenuEntry reports_menu_entries[]={
    { { N_("Units"), 0                },    "F2", MENU_REPORT_UNITS, 0 },
    /* TRANS: Nations report action */
    { { N_("Nations"), 0              },    "F3", MENU_REPORT_PLAYERS, 0 },
    { { N_("Cities"), 0               },    "F4", MENU_REPORT_CITIES, 0 },
    { { N_("Economy"), 0              },    "F5", MENU_REPORT_ECONOMY, 0 },
    /* TRANS: Research report action */
    { { N_("Research"), 0             },    "F6", MENU_REPORT_SCIENCE, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Wonders of the World"), 0 },    "F7", MENU_REPORT_WOW, 0 },
    { { N_("Top Five Cities"), 0      },    "F8", MENU_REPORT_TOP_CITIES, 0 },
    { { N_("Messages"), 0             },    "F9", MENU_REPORT_MESSAGES, 0 },
    { { N_("Demographics"), 0         },   "F11", MENU_REPORT_DEMOGRAPHIC, 0 },
    { { N_("Spaceship"), 0            },   "F12", MENU_REPORT_SPACESHIP, 0 },
    { { 0,                            },       0, MENU_END_OF_LIST, 0 }
};

static struct MenuEntry editor_menu_entries[] = {
    { { N_("Editing Mode"), 0         },      "", MENU_EDITOR_TOGGLE, 0 },
    { { N_("Tools"), 0                },      "", MENU_EDITOR_TOOLS, 0 },
    { { 0,                            },       0, MENU_END_OF_LIST, 0 }
};

static struct MenuEntry help_menu_entries[]={
    { { N_("Languages"), 0            },      "", MENU_HELP_LANGUAGES, 0 },
    { { N_("Connecting"), 0           },      "", MENU_HELP_CONNECTING, 0 },
    { { N_("Controls"), 0             },      "", MENU_HELP_CONTROLS, 0 },
    { { N_("Chatline"), 0             },      "", MENU_HELP_CHATLINE, 0 },
    { { N_("Playing"), 0              },      "", MENU_HELP_PLAYING, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Improvements"), 0         },      "", MENU_HELP_IMPROVEMENTS, 0 },
    { { N_("Units"), 0                },      "", MENU_HELP_UNITS, 0 },
    { { N_("Combat"), 0               },      "", MENU_HELP_COMBAT, 0 },
    { { N_("ZOC"), 0                  },      "", MENU_HELP_ZOC, 0 },
    { { N_("Technology"), 0           },      "", MENU_HELP_TECH, 0 },
    { { N_("Terrain"), 0              },      "", MENU_HELP_TERRAIN, 0 },
    { { N_("Wonders"), 0              },      "", MENU_HELP_WONDERS, 0 },
    { { N_("Government"), 0           },      "", MENU_HELP_GOVERNMENT, 0 },
    { { N_("Happiness"), 0            },      "", MENU_HELP_HAPPINESS, 0 },
    { { N_("Space Race"), 0           },      "", MENU_HELP_SPACE_RACE, 0 },
    { { 0                             },      "", MENU_SEPARATOR_LINE, 0 },
    { { N_("Copying"), 0              },      "", MENU_HELP_COPYING, 0 },
    { { N_("About"), 0                },      "", MENU_HELP_ABOUT, 0 },
    { { 0,                            },       0, MENU_END_OF_LIST, 0 }
};


static void create_menu(enum MenuIndex menu, char *name, struct MenuEntry entries[], 
			void (*menucallback)(Widget, XtPointer, XtPointer),
			Widget parent);
static void menu_entry_sensitive(enum MenuIndex menu, enum MenuID id, Bool s);
static void menu_entry_rename(enum MenuIndex menu, enum MenuID id, int var,
			      const char *terr);
static char *menu_entry_text(enum MenuIndex menu, int ent, int var,
			     const char *terr);

static void revolution_menu_callback(Widget w, XtPointer client_data,
				     XtPointer garbage);

/****************************************************************
...
*****************************************************************/
void update_menus(void)
{
  if (!can_client_change_view()) {
    XtSetSensitive(menus[MENU_REPORT]->button, False);
    XtSetSensitive(menus[MENU_ORDER]->button, False);
    XtSetSensitive(menus[MENU_VIEW]->button, False);
    XtSetSensitive(menus[MENU_GOVERNMENT]->button, False);

    menu_entry_sensitive(MENU_GAME, MENU_GAME_OPTIONS, 0);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_MSG_OPTIONS, 0);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_SAVE_SETTINGS, 0);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_SERVER_OPTIONS, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_OUTPUT_LOG, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_CLEAR_OUTPUT, 1);
  } else {
    int i;
    int any_cities = FALSE;

    players_iterate(pplayer) {
      if (city_list_size(pplayer->cities)) {
	any_cities = TRUE;
	break;
      }
    } players_iterate_end;

    XtSetSensitive(menus[MENU_REPORT]->button, True);
    XtSetSensitive(menus[MENU_VIEW]->button, True);
    XtSetSensitive(menus[MENU_GOVERNMENT]->button, True);

    menu_entry_sensitive(MENU_GOVERNMENT, MENU_GOVERNMENT_RATES,
                         can_client_issue_orders());
    menu_entry_sensitive(MENU_GOVERNMENT, MENU_GOVERNMENT_WORKLISTS,
                         can_client_issue_orders());
    menu_entry_sensitive(MENU_GOVERNMENT, MENU_GOVERNMENT_REVOLUTION,
                         can_client_issue_orders());

    /* Destroy all government-change entries and build them from scratch.
     * This could probably be done more efficiently if it were only done
     * when the client receives the rulesets or disconnects. */
    for (i = 0; i < num_government_entries; i++) {
      XtDestroyWidget(government_widgets[i]);
    }
    i = 0;
    government_iterate(pgovernment) {
      Widget w;

      if (pgovernment == game.government_during_revolution) {
	continue;
      }

      w = XtCreateManagedWidget(government_name_translation(pgovernment),
				smeBSBObjectClass,
				menus[MENU_GOVERNMENT]->shell, NULL, 0);
      XtAddCallback(w, XtNcallback, revolution_menu_callback, pgovernment);
      XtSetSensitive(w, can_change_to_government(client.conn.playing,
						 pgovernment));

      government_widgets[i] = w;
      i++;
    } government_iterate_end;
    num_government_entries = i;

    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_CITY_GROWTH,
			 draw_city_names);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_TERRAIN, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_COASTLINE, !draw_terrain);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_ROADS_RAILS, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_IRRIGATION, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_MINES, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_FORTRESS_AIRBASE, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_SPECIALS, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_POLLUTION, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_CITIES, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_UNITS, 1);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_FOCUS_UNIT, !draw_units);
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_FOG_OF_WAR, 1);

    menu_entry_sensitive(MENU_GAME, MENU_GAME_OPTIONS, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_MSG_OPTIONS, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_SAVE_SETTINGS, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_SERVER_OPTIONS, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_OUTPUT_LOG, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_CLEAR_OUTPUT, 1);
    menu_entry_sensitive(MENU_GAME, MENU_GAME_DISCONNECT, 1);

    if (NULL != client.conn.playing) {
      menu_entry_sensitive(MENU_REPORT, MENU_REPORT_SPACESHIP,
			   (SSHIP_NONE != client.conn.playing->spaceship.state));
    } else {
      menu_entry_sensitive(MENU_REPORT, MENU_REPORT_SPACESHIP, 0);
    }

    if ((get_num_units_in_focus() > 0) && can_client_issue_orders()) {
      struct terrain *tinfo;
      struct tile *ptile = NULL;
      bool can_build;
      struct unit_list *punits = get_units_in_focus();

      XtSetSensitive(menus[MENU_ORDER]->button, True);

      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_SELECT_SAME_TYPE, True);

      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_BUILD_CITY,
			   can_units_do(punits, can_unit_add_or_build_city));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_ROAD, 
			   can_units_do_activity(punits, ACTIVITY_ROAD)
			   || can_units_do_activity(punits,
						    ACTIVITY_RAILROAD));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_IRRIGATE, 
			   can_units_do_activity(punits, ACTIVITY_IRRIGATE));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_MINE, 
			   can_units_do_activity(punits, ACTIVITY_MINE));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_TRANSFORM, 
			   can_units_do_activity(punits, ACTIVITY_TRANSFORM));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_FORTRESS, 
			   can_units_do_base_gui(punits, BASE_GUI_FORTRESS));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_AIRBASE,
			   can_units_do_base_gui(punits, BASE_GUI_AIRBASE));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_POLLUTION, 
			   can_units_do_activity(punits, ACTIVITY_POLLUTION)
			   || can_units_do(punits, can_unit_paradrop));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_FALLOUT, 
			   can_units_do_activity(punits, ACTIVITY_FALLOUT));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_FORTIFY, 
			   can_units_do_activity(punits, ACTIVITY_FORTIFYING));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_SENTRY, 
			   can_units_do_activity(punits, ACTIVITY_SENTRY));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_PILLAGE, 
			   can_units_do_activity(punits, ACTIVITY_PILLAGE));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_HOMECITY, 
			   can_units_do(punits, can_unit_change_homecity));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_UNLOAD_TRANSPORTER, 
			   units_are_occupied(punits));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_LOAD,
			   units_can_load(punits));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_UNLOAD,
			   units_can_unload(punits));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_WAKEUP_OTHERS, 
			   units_have_activity_on_tile(punits,
						       ACTIVITY_SENTRY));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_AUTO_SETTLER,
			   can_units_do(punits, can_unit_do_autosettlers));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_AUTO_ATTACK, false);
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_DISBAND,
			   units_have_flag(punits, F_UNDISBANDABLE, FALSE));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_AUTO_EXPLORE, 
			   can_units_do_activity(punits, ACTIVITY_EXPLORE));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_CONNECT_ROAD,
			   can_units_do_connect(punits, ACTIVITY_ROAD));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_CONNECT_RAIL,
			   can_units_do_connect(punits, ACTIVITY_RAILROAD));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_CONNECT_IRRIGATE,
			   can_units_do_connect(punits, ACTIVITY_IRRIGATE));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_GOTO_CITY,
			   any_cities);
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_BUILD_WONDER,
			   can_units_do(punits,
				        unit_can_help_build_wonder_here));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_TRADEROUTE,
			   can_units_do(punits,
				        unit_can_est_traderoute_here));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_DIPLOMAT_DLG,
			   can_units_do_diplomat_action(punits,
							DIPLOMAT_ANY_ACTION));
      menu_entry_sensitive(MENU_ORDER, MENU_ORDER_NUKE,
                           units_have_flag(punits, F_NUCLEAR, TRUE));

      /* FiXME: very odd, iterating for the first entry! */
      unit_list_iterate(punits, punit) {
	ptile = punit->tile;
	break;
      } unit_list_iterate_end;

      tinfo = tile_terrain(ptile);
      can_build = !(tile_city(ptile));

      if (units_have_flag(punits, F_CITIES, TRUE)) {
	if (!can_build) {
	  menu_entry_rename(MENU_ORDER, MENU_ORDER_BUILD_CITY,
			  TEXT_ORDER_CITY_ADD_TO, NULL);
	} else {
	  menu_entry_rename(MENU_ORDER, MENU_ORDER_BUILD_CITY,
			  TEXT_ORDER_CITY_BUILD, NULL);
	}
      }

      if ((tinfo->irrigation_result != T_NONE)
	  && (tinfo->irrigation_result != tinfo)) {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_IRRIGATE,
			  TEXT_ORDER_IRRIGATE_CHANGE_TO,
			  terrain_name_translation(tinfo->irrigation_result));
      }
      else if (tile_has_special(ptile, S_IRRIGATION) &&
	       player_knows_techs_with_flag(client.conn.playing, TF_FARMLAND)) {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_IRRIGATE,
			  TEXT_ORDER_IRRIGATE_FARMLAND, NULL);
      } else {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_IRRIGATE,
			  TEXT_ORDER_IRRIGATE_IRRIGATE, NULL);
      }

      if ((tinfo->mining_result != T_NONE)
	  && (tinfo->mining_result != tinfo)) {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_MINE,
			  TEXT_ORDER_MINE_CHANGE_TO,
			  terrain_name_translation(tinfo->mining_result));
      } else {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_MINE,
			  TEXT_ORDER_MINE_MINE, NULL);
      }

      if ((tinfo->transform_result != T_NONE)
	  && (tinfo->transform_result != tinfo)) {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_TRANSFORM,
			  TEXT_ORDER_TRANSFORM_TRANSFORM_TO,
			  terrain_name_translation(tinfo->transform_result));
      } else {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_TRANSFORM,
			  TEXT_ORDER_TRANSFORM_TERRAIN, NULL);
      }

      if (units_have_flag(punits, F_PARATROOPERS, TRUE)) {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_POLLUTION,
			  TEXT_ORDER_POLLUTION_PARADROP, NULL);
      } else {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_POLLUTION,
			  TEXT_ORDER_POLLUTION_POLLUTION, NULL);
      }

      if (tile_has_special(ptile, S_ROAD)) {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_ROAD,
			  TEXT_ORDER_ROAD_RAILROAD, NULL);
      } else {
	menu_entry_rename(MENU_ORDER, MENU_ORDER_ROAD,
			  TEXT_ORDER_ROAD_ROAD, NULL);
      }
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void game_menu_callback(Widget w, XtPointer client_data,
			       XtPointer garbage)
{
  size_t pane_num = (size_t)client_data;

  switch(pane_num) {
  case MENU_GAME_OPTIONS:
    popup_option_dialog();
    break;
  case MENU_GAME_MSG_OPTIONS:
    popup_messageopt_dialog();
    break;
  case MENU_GAME_SAVE_SETTINGS:
    save_options();
    break;
  case MENU_GAME_SERVER_OPTIONS:
    popup_settable_options_dialog();
    break;
  case MENU_GAME_OUTPUT_LOG:
    log_output_window();
    break;
  case MENU_GAME_CLEAR_OUTPUT:
    clear_output_window();
    break;
  case MENU_GAME_DISCONNECT:
    disconnect_from_server();
    break;
  case MENU_GAME_QUIT:
    xaw_ui_exit();
    break;
  }
}

/****************************************************************
...
*****************************************************************/
static void government_menu_callback(Widget w, XtPointer client_data,
				     XtPointer garbage)
{
  size_t pane_num = (size_t)client_data;

  switch(pane_num) {
  case MENU_GOVERNMENT_FIND_CITY:
    popup_find_dialog();
    break;
  case MENU_GOVERNMENT_RATES:
    popup_rates_dialog();
    break;
  case MENU_GOVERNMENT_WORKLISTS:
    popup_worklists_dialog(client.conn.playing);
    break;
  case MENU_GOVERNMENT_REVOLUTION:
    popup_revolution_dialog(NULL);
    break;
  }
}

/****************************************************************************
  Callback for the government change entries in the government menu.
****************************************************************************/
static void revolution_menu_callback(Widget w, XtPointer client_data,
				     XtPointer garbage)
{
  struct government *pgovernment = client_data;

  if (!can_client_issue_orders()) {
    return;
  }
  if (-1 == client.conn.playing->revolution_finishes) {
    popup_revolution_dialog(pgovernment);
  } else {
    /* Player already has a revolution and should just choose a government */
    set_government_choice(pgovernment);
  }
}

/****************************************************************
...
*****************************************************************/
static void view_menu_callback(Widget w, XtPointer client_data,
			       XtPointer garbage)
{
  size_t pane_num = (size_t)client_data;

  switch(pane_num) {
  case MENU_VIEW_SHOW_MAP_GRID:
    key_map_grid_toggle();
    break;
  case MENU_VIEW_SHOW_NATIONAL_BORDERS:
    key_map_borders_toggle();
    break;
  case MENU_VIEW_SHOW_CITY_NAMES:
    key_city_names_toggle();
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_CITY_GROWTH,
			 draw_city_names);
    break;
  case MENU_VIEW_SHOW_CITY_GROWTH:
    key_city_growth_toggle();
    break;
  case MENU_VIEW_SHOW_CITY_PRODUCTIONS:
    key_city_productions_toggle();
    break;
  case MENU_VIEW_SHOW_TERRAIN:
    key_terrain_toggle();
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_COASTLINE, !draw_terrain);
    break;
  case MENU_VIEW_SHOW_COASTLINE:
    key_coastline_toggle();
    break;
  case MENU_VIEW_SHOW_ROADS_RAILS:
    key_roads_rails_toggle();
    break;
  case MENU_VIEW_SHOW_IRRIGATION:
    key_irrigation_toggle();
    break;
  case MENU_VIEW_SHOW_MINES:
    key_mines_toggle();
    break;
  case MENU_VIEW_SHOW_FORTRESS_AIRBASE:
    key_fortress_airbase_toggle();
    break;
  case MENU_VIEW_SHOW_SPECIALS:
    key_specials_toggle();
    break;
  case MENU_VIEW_SHOW_POLLUTION:
    key_pollution_toggle();
    break;
  case MENU_VIEW_SHOW_CITIES:
    key_cities_toggle();
    break;
  case MENU_VIEW_SHOW_UNITS:
    key_units_toggle();
    menu_entry_sensitive(MENU_VIEW, MENU_VIEW_SHOW_FOCUS_UNIT, !draw_units);
    break;
  case MENU_VIEW_SHOW_FOCUS_UNIT:
    key_focus_unit_toggle();
    break;
  case MENU_VIEW_SHOW_FOG_OF_WAR:
    key_fog_of_war_toggle();
    break;
  case MENU_VIEW_CENTER_VIEW:
    request_center_focus_unit();
    break;
  }
}

/****************************************************************
...
*****************************************************************/
static void orders_menu_callback(Widget w, XtPointer client_data,
				 XtPointer garbage)
{
  size_t pane_num = (size_t)client_data;

  switch(pane_num) {
  case MENU_ORDER_BUILD_CITY:
    key_unit_build_city();
    break;
  case MENU_ORDER_ROAD:
    key_unit_road();
    break;
  case MENU_ORDER_IRRIGATE:
    key_unit_irrigate();
    break;
  case MENU_ORDER_MINE:
    key_unit_mine();
    break;
  case MENU_ORDER_TRANSFORM:
    key_unit_transform();
    break;
  case MENU_ORDER_FORTRESS:
    key_unit_fortress();
    break;
  case MENU_ORDER_AIRBASE:
    key_unit_airbase(); 
    break;
  case MENU_ORDER_POLLUTION: /* or MENU_ORDER_PARADROP */
    unit_list_iterate(get_units_in_focus(), punit) {
      if (can_unit_paradrop(punit)) {
	key_unit_paradrop();
      } else {
	key_unit_pollution();
      }
    } unit_list_iterate_end;
    break;
  case MENU_ORDER_FALLOUT:
    key_unit_fallout();
    break;
  case MENU_ORDER_FORTIFY:
    key_unit_fortify();
    break;
  case MENU_ORDER_SENTRY:
    key_unit_sentry();
    break;
  case MENU_ORDER_PILLAGE:
    key_unit_pillage();
    break;
  case MENU_ORDER_HOMECITY:
    key_unit_homecity();
    break;
  case MENU_ORDER_UNLOAD_TRANSPORTER:
    key_unit_unload_all();
    break;
  case MENU_ORDER_LOAD:
    unit_list_iterate(get_units_in_focus(), punit) {
      request_unit_load(punit, NULL);
    } unit_list_iterate_end;
    break;
  case MENU_ORDER_UNLOAD:
    unit_list_iterate(get_units_in_focus(), punit) {
      request_unit_unload(punit);
    } unit_list_iterate_end;
    break;
  case MENU_ORDER_WAKEUP_OTHERS:
    key_unit_wakeup_others();
    break;
  case MENU_ORDER_AUTO_SETTLER:
    key_unit_auto_settle();
    break;
  case MENU_ORDER_AUTO_EXPLORE:
    key_unit_auto_explore();
    break;
  case MENU_ORDER_CONNECT_ROAD:
    key_unit_connect(ACTIVITY_ROAD);
    break;
  case MENU_ORDER_CONNECT_RAIL:
    key_unit_connect(ACTIVITY_RAILROAD);
    break;
  case MENU_ORDER_CONNECT_IRRIGATE:
    key_unit_connect(ACTIVITY_IRRIGATE);
    break;
  case MENU_ORDER_PATROL:
    key_unit_patrol();
    break;
  case MENU_ORDER_GOTO:
    key_unit_goto();
    break;
  case MENU_ORDER_GOTO_CITY:
    if (get_num_units_in_focus() > 0)
      popup_goto_dialog();
    break;
  case MENU_ORDER_RETURN:
    unit_list_iterate(get_units_in_focus(), punit) {
      request_unit_return(punit);
    } unit_list_iterate_end;
    break;
  case MENU_ORDER_DISBAND:
    key_unit_disband();
    break;
  case MENU_ORDER_BUILD_WONDER:
    key_unit_build_wonder();
    break;
  case MENU_ORDER_TRADEROUTE:
    key_unit_traderoute();
    break;
  case MENU_ORDER_DIPLOMAT_DLG:
    key_unit_diplomat_actions();
    break;
  case MENU_ORDER_NUKE:
    key_unit_nuke();
    break;
  case MENU_ORDER_SELECT_SAME_TYPE:
    request_unit_select(get_units_in_focus(), SELTYPE_SAME, SELLOC_ALL);
    break;
  case MENU_ORDER_WAIT:
    key_unit_wait();
    break;
  case MENU_ORDER_DONE:
    key_unit_done();
    break;
  }
}

/****************************************************************
...
*****************************************************************/
static void reports_menu_callback(Widget w, XtPointer client_data,
				  XtPointer garbage)
{
  size_t pane_num = (size_t)client_data;

  switch(pane_num) {
   case MENU_REPORT_CITIES:
    popup_city_report_dialog(0);
    break;
   case MENU_REPORT_UNITS:
    popup_activeunits_report_dialog(0);
    break;
  case MENU_REPORT_PLAYERS:
    popup_players_dialog(FALSE);
    break;
   case MENU_REPORT_ECONOMY:
    popup_economy_report_dialog(0);
    break;
   case MENU_REPORT_SCIENCE:
    popup_science_dialog(0);
    break;
   case MENU_REPORT_WOW:
    send_report_request(REPORT_WONDERS_OF_THE_WORLD);
    break;
   case MENU_REPORT_TOP_CITIES:
    send_report_request(REPORT_TOP_5_CITIES);
    break;
  case MENU_REPORT_MESSAGES:
    popup_meswin_dialog(FALSE);
    break;
   case MENU_REPORT_DEMOGRAPHIC:
    send_report_request(REPORT_DEMOGRAPHIC);
    break;
   case MENU_REPORT_SPACESHIP:
    if (NULL != client.conn.playing) {
      popup_spaceship_dialog(client.conn.playing);
    }
    break;
  }
}

/****************************************************************
  Callback for Editor menu entries.
*****************************************************************/
static void editor_menu_callback(Widget w, XtPointer client_data,
				 XtPointer garbage)
{
  size_t pane_num = (size_t)client_data;

  switch(pane_num) {
  case MENU_EDITOR_TOGGLE:
    key_editor_toggle();
    break;
  case MENU_EDITOR_TOOLS:
    break;
  }
}

/****************************************************************
...
*****************************************************************/
static void help_menu_callback(Widget w, XtPointer client_data,
			       XtPointer garbage)
{
  size_t pane_num = (size_t)client_data;

  switch(pane_num) {
  case MENU_HELP_LANGUAGES:
    popup_help_dialog_string(HELP_LANGUAGES_ITEM);
    break;
  case MENU_HELP_CONNECTING:
    popup_help_dialog_string(HELP_CONNECTING_ITEM);
    break;
  case MENU_HELP_CONTROLS:
    popup_help_dialog_string(HELP_CONTROLS_ITEM);
    break;
  case MENU_HELP_CHATLINE:
    popup_help_dialog_string(HELP_CHATLINE_ITEM);
    break;
  case MENU_HELP_PLAYING:
    popup_help_dialog_string(HELP_PLAYING_ITEM);
    break;
  case MENU_HELP_IMPROVEMENTS:
    popup_help_dialog_string(HELP_IMPROVEMENTS_ITEM);
    break;
  case MENU_HELP_UNITS:
    popup_help_dialog_string(HELP_UNITS_ITEM);
    break;
  case MENU_HELP_COMBAT:
    popup_help_dialog_string(HELP_COMBAT_ITEM);
    break;
  case MENU_HELP_ZOC:
    popup_help_dialog_string(HELP_ZOC_ITEM);
    break;
  case MENU_HELP_TECH:
    popup_help_dialog_string(HELP_TECHS_ITEM);
    break;
  case MENU_HELP_TERRAIN:
    popup_help_dialog_string(HELP_TERRAIN_ITEM);
    break;
  case MENU_HELP_WONDERS:
    popup_help_dialog_string(HELP_WONDERS_ITEM);
    break;
  case MENU_HELP_GOVERNMENT:
    popup_help_dialog_string(HELP_GOVERNMENT_ITEM);
    break;
  case MENU_HELP_HAPPINESS:
    popup_help_dialog_string(HELP_HAPPINESS_ITEM);
    break;
  case MENU_HELP_SPACE_RACE:
    popup_help_dialog_string(HELP_SPACE_RACE_ITEM);
    break;
  case MENU_HELP_COPYING:
    popup_help_dialog_string(HELP_COPYING_ITEM);
    break;
  case MENU_HELP_ABOUT:
    popup_help_dialog_string(HELP_ABOUT_ITEM);
    break;
  }
}

/****************************************************************
 Initialize menus.
*****************************************************************/
void setup_menus(Widget parent_form)
{
  create_menu(MENU_GAME, "gamemenu", 
	      game_menu_entries, game_menu_callback, 
	      parent_form);
  create_menu(MENU_GOVERNMENT, "governmentmenu", 
	      government_menu_entries, government_menu_callback, 
	      parent_form);
  create_menu(MENU_VIEW, "viewmenu", 
	      view_menu_entries, view_menu_callback, 
	      parent_form);
  create_menu(MENU_ORDER, "ordersmenu", 
	      order_menu_entries, orders_menu_callback, 
	      parent_form);
  create_menu(MENU_REPORT, "reportsmenu", 
	      reports_menu_entries, reports_menu_callback, 
	      parent_form);
  create_menu(MENU_EDITOR, "editormenu",
	      editor_menu_entries, editor_menu_callback,
	      parent_form);
  create_menu(MENU_HELP, "helpmenu", 
	      help_menu_entries, help_menu_callback, 
	      parent_form);
}

/****************************************************************
 Determine whether menu item is active or not.
*****************************************************************/
int is_menu_item_active(enum MenuIndex menu, enum MenuID id)
{
  struct Menu *pmenu = menus[menu];
  int i;

  for (i = 0; pmenu->entries[i].id != MENU_END_OF_LIST; i++) {
    if(pmenu->entries[i].id==id) {
      return XtIsSensitive(pmenu->entries[i].w);
    }
  }

  return FALSE;
}

/****************************************************************
...
*****************************************************************/
void create_menu(enum MenuIndex menu, char *name, struct MenuEntry entries[], 
		 void (*menucallback)(Widget, XtPointer, XtPointer),
		 Widget parent)
{
  int i, j;
  struct Menu *mymenu;
  const char *xlt;
  int lstr;
  int litem;
  int lacel;

  mymenu=fc_malloc(sizeof(struct Menu));
  menus[menu]=mymenu;
  mymenu->entries=entries;

  /* Calculate the longest string in this menu so if the font
   * is fixed then we will know where to put the accelerator key.- Serrada */
  litem=lacel=0;
  for (i = 0; entries[i].id != MENU_END_OF_LIST; i++) {
    if (entries[i].id != MENU_SEPARATOR_LINE) {
      lstr=strlen(entries[i].acel);
      if (lacel<lstr) {
	lacel=lstr;
      }
      for (j = 0; entries[i].text[j]; j++) {
	xlt=_(entries[i].text[j]);
	lstr=strlen(xlt);
	if (strstr(xlt, "%s")) {
	  lstr+=MAX_LEN_TERR_NAME_DISP;
	}
	if (litem<lstr) {
	  litem=lstr;
	}
      }
    }
  }
  if ((litem>0) && (lacel>0)) {
    litem+=(ACEL_SPACE/2);
    lacel+=(ACEL_SPACE-(ACEL_SPACE/2));
  }
  mymenu->maxitemlen=litem;
  mymenu->maxacellen=lacel;

  I_L(mymenu->button=XtVaCreateManagedWidget(name, 
					 menuButtonWidgetClass, 
					 parent,
					 NULL));

  mymenu->shell=XtCreatePopupShell("menu", simpleMenuWidgetClass, 
				   mymenu->button, NULL, 0);

  for (i = 0; entries[i].id != MENU_END_OF_LIST; i++) {
    if (entries[i].id == MENU_SEPARATOR_LINE) {
      entries[i].w = XtCreateManagedWidget(NULL, smeLineObjectClass, 
					   mymenu->shell, NULL, 0);
    } else {
      xlt=menu_entry_text(menu, i, 0, "");
      entries[i].w = XtCreateManagedWidget(xlt, smeBSBObjectClass, 
					   mymenu->shell, NULL, 0);
      XtAddCallback(entries[i].w, XtNcallback, menucallback, 
		    (XtPointer)entries[i].id );
    }
  }

  return;
}

/****************************************************************
...
*****************************************************************/
void menu_entry_rename(enum MenuIndex menu, enum MenuID id, int var,
		       const char *terr)
{
  struct Menu *pmenu = menus[menu];
  int i;
  char *item;

  for (i = 0; pmenu->entries[i].id != MENU_END_OF_LIST; i++) {
    if(pmenu->entries[i].id==id) {
      item=menu_entry_text(menu, i, var, terr);
      XtVaSetValues(pmenu->entries[i].w, XtNlabel, item, NULL);
      return;
    }
  }
}

/****************************************************************
...
*****************************************************************/
void menu_entry_sensitive(enum MenuIndex menu, enum MenuID id, Bool s)
{
  struct Menu *pmenu = menus[menu];
  int i;

  for (i = 0; pmenu->entries[i].id != MENU_END_OF_LIST; i++) {
    if(pmenu->entries[i].id==id) {
      XtSetSensitive(pmenu->entries[i].w, (s ? True : False));
      return;
    }
  }
}

/****************************************************************
...
*****************************************************************/
static char *menu_entry_text(enum MenuIndex menu, int ent, int var,
			     const char *terr)
{
  struct Menu *pmenu = menus[menu];
  static char retbuf[256];
  char tmp[256];
  const char *xlt;

  xlt=_(pmenu->entries[ent].text[var]);

  if (strstr(xlt, "%s")) {
    my_snprintf(tmp, sizeof(tmp), xlt, terr);
    xlt=tmp;
  }

  my_snprintf(retbuf, sizeof(retbuf), "%*.*s%*.*s",
	  -pmenu->maxitemlen, pmenu->maxitemlen, xlt,
	  pmenu->maxacellen, pmenu->maxacellen, pmenu->entries[ent].acel);

  return (retbuf);
}
