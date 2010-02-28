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
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Toggle.h>     

/* utility */
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "game.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "unit.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"
#include "goto.h"

/* gui-xaw */
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapctrl.h"
#include "mapview.h"

#include "gotodlg.h"

static Widget goto_dialog_shell;
static Widget goto_form;
static Widget goto_label;
static Widget goto_viewport;
static Widget goto_list;
static Widget goto_center_command;
static Widget goto_airlift_command;
static Widget goto_all_toggle;
static Widget goto_cancel_command;

void update_goto_dialog(Widget goto_list);

void goto_cancel_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
void goto_goto_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
void goto_airlift_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
void goto_all_toggle_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data);
void goto_list_callback(Widget w, XtPointer client_data, XtPointer call_data);

static void cleanup_goto_list(void);

static char *dummy_city_list[]={ 
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  0
};

static int ncities_total = 0;
static char **city_name_ptrs = NULL;
static struct tile *original_tile;


/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_goto_dialog(void)
{
  Position x, y;
  Dimension width, height;
  Boolean no_player_cities;

  if (!can_client_issue_orders() || get_num_units_in_focus() == 0) {
    return;
  }

  no_player_cities = !(city_list_size(client.conn.playing->cities));

  original_tile = get_center_tile_mapcanvas();
  
  XtSetSensitive(main_form, FALSE);
  
  goto_dialog_shell =
    I_T(XtCreatePopupShell("gotodialog", transientShellWidgetClass,
			   toplevel, NULL, 0));

  goto_form = XtVaCreateManagedWidget("gotoform", 
				      formWidgetClass, 
				      goto_dialog_shell, NULL);

  goto_label =
    I_L(XtVaCreateManagedWidget("gotolabel", labelWidgetClass, 
				goto_form, NULL));

  goto_viewport = XtVaCreateManagedWidget("gotoviewport", 
				      viewportWidgetClass, 
				      goto_form, 
				      NULL);

  goto_list = XtVaCreateManagedWidget("gotolist", 
				      listWidgetClass, 
				      goto_viewport, 
				      XtNlist, 
				      (XtArgVal)dummy_city_list,
				      NULL);

  goto_center_command =
    I_L(XtVaCreateManagedWidget("gotocentercommand", commandWidgetClass,
				goto_form, NULL));

  goto_airlift_command =
    I_L(XtVaCreateManagedWidget("gotoairliftcommand", commandWidgetClass,
				goto_form, NULL));

  goto_all_toggle =
    I_L(XtVaCreateManagedWidget("gotoalltoggle", toggleWidgetClass,
				goto_form,
				XtNstate, no_player_cities,
				XtNsensitive, !no_player_cities,
				NULL));

  goto_cancel_command =
    I_L(XtVaCreateManagedWidget("gotocancelcommand", commandWidgetClass,
				goto_form, NULL));

  XtAddCallback(goto_list, XtNcallback, goto_list_callback, NULL);
  XtAddCallback(goto_center_command, XtNcallback, 
		goto_goto_command_callback, NULL);
  XtAddCallback(goto_airlift_command, XtNcallback, 
		goto_airlift_command_callback, NULL);
  XtAddCallback(goto_all_toggle, XtNcallback,
		goto_all_toggle_callback, NULL);
  XtAddCallback(goto_cancel_command, XtNcallback, 
		goto_cancel_command_callback, NULL);

  XtRealizeWidget(goto_dialog_shell);

  update_goto_dialog(goto_list);

  XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);

  XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
		    &x, &y);
  XtVaSetValues(goto_dialog_shell, XtNx, x, XtNy, y, NULL);

  XtPopup(goto_dialog_shell, XtGrabNone);

  /* force refresh of viewport so the scrollbar is added.
   * Buggy sun athena requires this */
  XtVaSetValues(goto_viewport, XtNforceBars, True, NULL);
}

static struct city *get_selected_city(void)
{
  XawListReturnStruct *ret;
  int len;
  
  ret=XawListShowCurrent(goto_list);
  if(ret->list_index==XAW_LIST_NONE)
    return 0;

  len = strlen(ret->string);
  if(len>3 && strcmp(ret->string+len-3, "(A)")==0) {
    char name[MAX_LEN_NAME];
    mystrlcpy(name, ret->string, MIN(sizeof(name),len-2));
    return game_find_city_by_name(name);
  }
  return game_find_city_by_name(ret->string);
}

/**************************************************************************
...
**************************************************************************/
void update_goto_dialog(Widget goto_list)
{
  int j = 0;
  Boolean all_cities;

  if (!can_client_issue_orders()) {
    return;
  }

  XtVaGetValues(goto_all_toggle, XtNstate, &all_cities, NULL);

  cleanup_goto_list();

  if(all_cities) {
    ncities_total = 0;
    players_iterate(pplayer) {
      ncities_total += city_list_size(pplayer->cities);
    } players_iterate_end;
  } else {
    ncities_total = city_list_size(client.conn.playing->cities);
  }

  city_name_ptrs=fc_malloc(ncities_total*sizeof(char*));
  
  players_iterate(pplayer) {
    if (!all_cities && pplayer != client.conn.playing) {
      continue;
    }
    city_list_iterate(pplayer->cities, pcity) {
      char name[MAX_LEN_NAME+3];
      sz_strlcpy(name, city_name(pcity));
      /* FIXME: should use unit_can_airlift_to(). */
      if (pcity->airlift) {
	sz_strlcat(name, "(A)");
      }
      city_name_ptrs[j++]=mystrdup(name);
    } city_list_iterate_end;
  } players_iterate_end;

  if(ncities_total) {
    qsort(city_name_ptrs, ncities_total, sizeof(char *), compare_strings_ptrs);
    XawListChange(goto_list, city_name_ptrs, ncities_total, 0, True);
  }
}

/**************************************************************************
...
**************************************************************************/
static void popdown_goto_dialog(void)
{
  cleanup_goto_list();

  XtDestroyWidget(goto_dialog_shell);
  XtSetSensitive(main_form, TRUE);
}

/**************************************************************************
...
**************************************************************************/
void goto_list_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  XawListReturnStruct *ret;
  ret=XawListShowCurrent(goto_list);
  
  if (ret->list_index != XAW_LIST_NONE) {
    struct city *pdestcity;
    if ((pdestcity = get_selected_city())) {
      bool can_airlift = FALSE;
      unit_list_iterate(get_units_in_focus(), punit) {
        if (unit_can_airlift_to(punit, pdestcity)) {
	  can_airlift = TRUE;
	  break;
	}
      } unit_list_iterate_end;

      center_tile_mapcanvas(pdestcity->tile);
      if (can_airlift) {
	XtSetSensitive(goto_airlift_command, True);
	return;
      }
    }
  }
  XtSetSensitive(goto_airlift_command, False);
}

/**************************************************************************
...
**************************************************************************/
void goto_airlift_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  struct city *pdestcity=get_selected_city();
  if (pdestcity) {
    unit_list_iterate(get_units_in_focus(), punit) {
      request_unit_airlift(punit, pdestcity);
    } unit_list_iterate_end;
  }
  popdown_goto_dialog();
}

/**************************************************************************
...
**************************************************************************/
void goto_all_toggle_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  update_goto_dialog(goto_list);
}

/**************************************************************************
...
**************************************************************************/
void goto_goto_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  struct city *pdestcity = get_selected_city();
  if (pdestcity) {
    unit_list_iterate(get_units_in_focus(), punit) {
      send_goto_tile(punit, pdestcity->tile);
    } unit_list_iterate_end;
  }
  popdown_goto_dialog();
}

/**************************************************************************
...
**************************************************************************/
void goto_cancel_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  center_tile_mapcanvas(original_tile);
  popdown_goto_dialog();
}

/**************************************************************************
...
**************************************************************************/
static void cleanup_goto_list(void)
{
  int i;

  XawListChange(goto_list, dummy_city_list, 0, 0, FALSE);

  XtSetSensitive(goto_airlift_command, False);

  if(city_name_ptrs) {
    for(i=0; i<ncities_total; i++) {
      free(city_name_ptrs[i]);
    }
    free(city_name_ptrs);
  }
  ncities_total = 0;
  city_name_ptrs = NULL;
}
