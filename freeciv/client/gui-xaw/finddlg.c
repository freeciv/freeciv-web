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

/* utility */
#include "log.h"
#include "mem.h"

/* common */
#include "game.h"
#include "player.h"

/* gui-xaw */
#include "mapview.h"
#include "gui_main.h"
#include "gui_stuff.h"

#include "finddlg.h"

static Widget find_dialog_shell;
static Widget find_form;
static Widget find_label;
static Widget find_viewport;
static Widget find_list;
static Widget find_center_command;
static Widget find_cancel_command;

void update_find_dialog(Widget find_list);

void find_center_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
void find_cancel_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
void find_list_callback(Widget w, XtPointer client_data, XtPointer call_data);

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

static int ncities_total;
static char **city_name_ptrs;
static struct tile *original_tile;

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_find_dialog(void)
{
  Position x, y;
  Dimension width, height;

  original_tile = get_center_tile_mapcanvas();

  XtSetSensitive(main_form, FALSE);
  
  find_dialog_shell =
    I_T(XtCreatePopupShell("finddialog", transientShellWidgetClass,
			   toplevel, NULL, 0));

  find_form = XtVaCreateManagedWidget("findform", 
				      formWidgetClass, 
				      find_dialog_shell, NULL);

  
  find_label = I_L(XtVaCreateManagedWidget("findlabel", labelWidgetClass, 
					   find_form, NULL));

  find_viewport = XtVaCreateManagedWidget("findviewport", 
				      viewportWidgetClass, 
				      find_form, 
				      NULL);
  
  
  find_list = XtVaCreateManagedWidget("findlist", 
				      listWidgetClass, 
				      find_viewport, 
				      XtNlist, 
				      (XtArgVal)dummy_city_list,
				      NULL);
  
  find_center_command =
    I_L(XtVaCreateManagedWidget("findcentercommand", commandWidgetClass,
				find_form, NULL));

  find_cancel_command =
    I_L(XtVaCreateManagedWidget("findcancelcommand", commandWidgetClass,
				find_form, NULL));

  XtAddCallback(find_list, XtNcallback, find_list_callback, NULL);
  XtAddCallback(find_center_command, XtNcallback, 
		find_center_command_callback, NULL);
  XtAddCallback(find_cancel_command, XtNcallback, 
		find_cancel_command_callback, NULL);
  

  XtRealizeWidget(find_dialog_shell);

  update_find_dialog(find_list);

  XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);

  XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
		    &x, &y);
  XtVaSetValues(find_dialog_shell, XtNx, x, XtNy, y, NULL);

  XtPopup(find_dialog_shell, XtGrabNone);

  /* force refresh of viewport so the scrollbar is added.
   * Buggy sun athena requires this */
  XtVaSetValues(find_viewport, XtNforceBars, True, NULL);
}



/**************************************************************************
...
**************************************************************************/
void update_find_dialog(Widget find_list)
{
  int j = 0;

  ncities_total = 0;
  players_iterate(pplayer) {
    ncities_total += city_list_size(pplayer->cities);
  } players_iterate_end;

  city_name_ptrs=fc_malloc(ncities_total*sizeof(char*));
  
  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      *(city_name_ptrs+j++)=mystrdup(city_name(pcity));
    } city_list_iterate_end;
  } players_iterate_end;
  
  if(ncities_total) {
    qsort(city_name_ptrs, ncities_total, sizeof(char *), compare_strings_ptrs);
    XawListChange(find_list, city_name_ptrs, ncities_total, 0, True);
  }
}


/**************************************************************************
...
**************************************************************************/
static void popdown_find_dialog(void)
{
  int i;
  
  for(i=0; i<ncities_total; i++)
    free(*(city_name_ptrs+i));
  
  XtDestroyWidget(find_dialog_shell);
  free(city_name_ptrs);
  XtSetSensitive(main_form, TRUE);
}

/**************************************************************************
...
**************************************************************************/
void find_center_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  struct city *pcity;
  XawListReturnStruct *ret;
  
  ret=XawListShowCurrent(find_list);

  if(ret->list_index!=XAW_LIST_NONE)
    if((pcity=game_find_city_by_name(ret->string)))
      center_tile_mapcanvas(pcity->tile);
  
  popdown_find_dialog();
}

/**************************************************************************
...
**************************************************************************/
void find_cancel_command_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  center_tile_mapcanvas(original_tile);
  popdown_find_dialog();
}

/**************************************************************************
...
**************************************************************************/
void find_list_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct city *pcity;
  XawListReturnStruct *ret;
  
  ret=XawListShowCurrent(find_list);

  if(ret->list_index!=XAW_LIST_NONE)
    if((pcity=game_find_city_by_name(ret->string)))
      center_tile_mapcanvas(pcity->tile);
}
