/***********************************************************************
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
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
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/SimpleMenu.h>

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "improvement.h"
#include "tech.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"

#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"

/******************************************************************/
static Widget spy_tech_shell;
static Widget spy_advances_list, spy_advances_list_label;
static Widget spy_steal_command;

static int spy_tech_shell_is_modal;
static int advance_type[A_LAST+1];
static int steal_advance = 0;

/******************************************************************/
static Widget spy_sabotage_shell;
static Widget spy_improvements_list, spy_improvements_list_label;
static Widget spy_sabotage_command;

static int spy_sabotage_shell_is_modal;
static int improvement_type[B_LAST+1];
static int sabotage_improvement = 0;

static Widget diplomat_dialog;
int diplomat_id;
int diplomat_target_id;

/****************************************************************
...
*****************************************************************/
static void diplomat_bribe_yes_callback(Widget w, XtPointer client_data, 
					XtPointer call_data)
{
  request_diplomat_action(DIPLOMAT_BRIBE, diplomat_id,
			  diplomat_target_id, 0);

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_bribe_no_callback(Widget w, XtPointer client_data, 
				       XtPointer call_data)
{
  destroy_message_dialog(w);
}

/**************************************************************************
  Asks the server how much the bribe is
**************************************************************************/
static void diplomat_bribe_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data)
{
  destroy_message_dialog(w);

  if (game_find_unit_by_number(diplomat_id)
   && game_find_unit_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_BRIBE, diplomat_id,
			    diplomat_target_id, 0);
  }
}

/**************************************************************************
  Creates and popups the bribe dialog
**************************************************************************/
void popup_bribe_dialog(struct unit *punit, int cost)
{
  char buf[128];

  if (unit_has_type_flag(punit, F_UNBRIBABLE)) {
    popup_message_dialog(toplevel, "diplomatbribedialog",
                         _("This unit cannot be bribed!"),
                         diplomat_bribe_no_callback, 0, 0, NULL);
  } else if (cost <= client.conn.playing->economic.gold) {
    my_snprintf(buf, sizeof(buf),
		_("Bribe unit for %d gold?\n"
		  "Treasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
    popup_message_dialog(toplevel, "diplomatbribedialog", buf,
			 diplomat_bribe_yes_callback, 0, 0,
			 diplomat_bribe_no_callback, 0, 0,
			 NULL);
  } else {
    my_snprintf(buf, sizeof(buf),
		_("Bribing the unit costs %d gold.\n"
		  "Treasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
    popup_message_dialog(toplevel, "diplomatnogolddialog", buf,
			 diplomat_bribe_no_callback, 0, 0,
			 NULL);
  }
}

/****************************************************************
...
*****************************************************************/
static void diplomat_sabotage_callback(Widget w, XtPointer client_data, 
				       XtPointer call_data)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if(game_find_unit_by_number(diplomat_id) && 
     game_find_city_by_number(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, -1);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_embassy_callback(Widget w, XtPointer client_data, 
				      XtPointer call_data)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if(game_find_unit_by_number(diplomat_id) && 
     (game_find_city_by_number(diplomat_target_id))) { 
    request_diplomat_action(DIPLOMAT_EMBASSY, diplomat_id,
			    diplomat_target_id, 0);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_investigate_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if(game_find_unit_by_number(diplomat_id) && 
     (game_find_city_by_number(diplomat_target_id))) { 
    request_diplomat_action(DIPLOMAT_INVESTIGATE, diplomat_id,
			    diplomat_target_id, 0);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void spy_sabotage_unit_callback(Widget w, XtPointer client_data, 
				       XtPointer call_data)
{
  request_diplomat_action(SPY_SABOTAGE_UNIT, diplomat_id,
			  diplomat_target_id, 0);

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void spy_poison_callback(Widget w, XtPointer client_data, 
				XtPointer call_data)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if(game_find_unit_by_number(diplomat_id) && 
     (game_find_city_by_number(diplomat_target_id))) { 
    request_diplomat_action(SPY_POISON, diplomat_id, diplomat_target_id, 0);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_steal_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if(game_find_unit_by_number(diplomat_id) && 
     game_find_city_by_number(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
			    diplomat_target_id, A_UNSET);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void spy_close_tech_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data)
{
  if(spy_tech_shell_is_modal)
    XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(spy_tech_shell);
  spy_tech_shell=0;

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void spy_close_sabotage_callback(Widget w, XtPointer client_data, 
					XtPointer call_data)
{
  if(spy_sabotage_shell_is_modal)
    XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(spy_sabotage_shell);
  spy_sabotage_shell=0;

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void spy_select_tech_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data)
{
  XawListReturnStruct *ret;
  ret=XawListShowCurrent(spy_advances_list);
  
  if(ret->list_index!=XAW_LIST_NONE && advance_type[ret->list_index] != -1){
    steal_advance = advance_type[ret->list_index];
    XtSetSensitive(spy_steal_command, TRUE);
    return;
  }
  XtSetSensitive(spy_steal_command, FALSE);
}

/****************************************************************
...
*****************************************************************/
static void spy_select_improvement_callback(Widget w, XtPointer client_data, 
					    XtPointer call_data)
{
  XawListReturnStruct *ret;
  ret=XawListShowCurrent(spy_improvements_list);
  
  if(ret->list_index!=XAW_LIST_NONE){
    sabotage_improvement = improvement_type[ret->list_index];
    XtSetSensitive(spy_sabotage_command, TRUE);
    return;
  }
  XtSetSensitive(spy_sabotage_command, FALSE);
}

/****************************************************************
...
*****************************************************************/
static void spy_steal_callback(Widget w, XtPointer client_data, 
			       XtPointer call_data)
{  
  XtDestroyWidget(spy_tech_shell);
  spy_tech_shell = 0l;
  
  if(!steal_advance){
    freelog(LOG_ERROR, "Bug in spy steal tech code");
    process_diplomat_arrival(NULL, 0);
    return;
  }
  
  if(game_find_unit_by_number(diplomat_id) && 
     game_find_city_by_number(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
			    diplomat_target_id, steal_advance);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void spy_sabotage_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{  
  XtDestroyWidget(spy_sabotage_shell);
  spy_sabotage_shell = 0l;
  
  if(!sabotage_improvement){
    freelog(LOG_ERROR, "Bug in spy sabotage code");
    process_diplomat_arrival(NULL, 0);
    return;
  }
  
  if(game_find_unit_by_number(diplomat_id) && 
     game_find_city_by_number(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, sabotage_improvement + 1);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static int create_advances_list(struct player *pplayer,
				struct player *pvictim, bool make_modal)
{  
  Widget spy_tech_form;
  Widget close_command;
  Dimension width1, width2; 
  int j;

  static const char *advances_can_steal[A_LAST+1]; 

  spy_tech_shell =
    I_T(XtVaCreatePopupShell("spystealtechpopup", 
			     (make_modal ? transientShellWidgetClass :
			      topLevelShellWidgetClass),
			     toplevel, NULL));  
  
  spy_tech_form = XtVaCreateManagedWidget("spystealtechform", 
					     formWidgetClass,
					     spy_tech_shell,
					     NULL);   

  spy_advances_list_label =
    I_L(XtVaCreateManagedWidget("spystealtechlistlabel", labelWidgetClass, 
				spy_tech_form, NULL));

  spy_advances_list = XtVaCreateManagedWidget("spystealtechlist", 
					      listWidgetClass,
					      spy_tech_form,
					      NULL);

  close_command =
    I_L(XtVaCreateManagedWidget("spystealtechclosecommand", commandWidgetClass,
				spy_tech_form, NULL));
  
  spy_steal_command =
    I_L(XtVaCreateManagedWidget("spystealtechcommand", commandWidgetClass,
				spy_tech_form,
				XtNsensitive, False,
				NULL));
  

  XtAddCallback(spy_advances_list, XtNcallback, spy_select_tech_callback, NULL);
  XtAddCallback(close_command, XtNcallback, spy_close_tech_callback, NULL);
  XtAddCallback(spy_steal_command, XtNcallback, spy_steal_callback, NULL);
  XtRealizeWidget(spy_tech_shell);

  /* Now populate the list */
  
  j = 0;
  advances_can_steal[j] = _("NONE");
  advance_type[j] = -1;

  if (pvictim) { /* you don't want to know what lag can do -- Syela */
    advance_index_iterate(A_FIRST, i) {
      if(player_invention_state(pvictim, i)==TECH_KNOWN && 
         (player_invention_state(pplayer, i)==TECH_UNKNOWN || 
          player_invention_state(pplayer, i)==TECH_PREREQS_KNOWN)) {
      
        advances_can_steal[j] = advance_name_translation(advance_by_number(i));
        advance_type[j++] = i;
      }
    }
    advances_can_steal[j] = _("At Spy's Discretion");
    advance_type[j++] = A_UNSET;
  } advance_index_iterate_end;

  if(j == 0) j++;
  advances_can_steal[j] = NULL; 
  
  XtSetSensitive(spy_steal_command, FALSE);
  
  XawListChange(spy_advances_list, (char **)advances_can_steal, 0, 0, 1);
  XtVaGetValues(spy_advances_list, XtNwidth, &width1, NULL);
  XtVaGetValues(spy_advances_list_label, XtNwidth, &width2, NULL);
  XtVaSetValues(spy_advances_list, XtNwidth, MAX(width1,width2), NULL); 
  XtVaSetValues(spy_advances_list_label, XtNwidth, MAX(width1,width2), NULL); 

  return j;
}

/****************************************************************
...
*****************************************************************/
static int create_improvements_list(struct player *pplayer,
				    struct city *pcity, bool make_modal)
{  
  Widget spy_sabotage_form;
  Widget close_command;
  Dimension width1, width2; 
  int j;

  static const char *improvements_can_sabotage[B_LAST+1]; 
  
  spy_sabotage_shell =
    I_T(XtVaCreatePopupShell("spysabotageimprovementspopup", 
			     (make_modal ? transientShellWidgetClass :
			      topLevelShellWidgetClass),
			     toplevel, NULL));  
  
  spy_sabotage_form = XtVaCreateManagedWidget("spysabotageimprovementsform", 
					     formWidgetClass,
					     spy_sabotage_shell,
					     NULL);   

  spy_improvements_list_label =
    I_L(XtVaCreateManagedWidget("spysabotageimprovementslistlabel", 
				labelWidgetClass, 
				spy_sabotage_form,
				NULL));

  spy_improvements_list = XtVaCreateManagedWidget("spysabotageimprovementslist", 
					      listWidgetClass,
					      spy_sabotage_form,
					      NULL);

  close_command =
    I_L(XtVaCreateManagedWidget("spysabotageimprovementsclosecommand", 
				commandWidgetClass,
				spy_sabotage_form,
				NULL));
  
  spy_sabotage_command =
    I_L(XtVaCreateManagedWidget("spysabotageimprovementscommand", 
				commandWidgetClass,
				spy_sabotage_form,
				XtNsensitive, False,
				NULL));
  

  XtAddCallback(spy_improvements_list, XtNcallback, spy_select_improvement_callback, NULL);
  XtAddCallback(close_command, XtNcallback, spy_close_sabotage_callback, NULL);
  XtAddCallback(spy_sabotage_command, XtNcallback, spy_sabotage_callback, NULL);
  XtRealizeWidget(spy_sabotage_shell);

  /* Now populate the list */
  
  j = 0;
  improvements_can_sabotage[j] = _("City Production");
  improvement_type[j++] = -1;

  city_built_iterate(pcity, pimprove) {
    if (pimprove->sabotage > 0) {
      improvements_can_sabotage[j] = city_improvement_name_translation(pcity, pimprove);
      improvement_type[j++] = improvement_number(pimprove);
    }  
  } city_built_iterate_end;

  if(j > 1) {
    improvements_can_sabotage[j] = _("At Spy's Discretion");
    improvement_type[j++] = B_LAST;
  } else {
    improvement_type[0] = B_LAST; /* fake "discretion", since must be production */
  }

  improvements_can_sabotage[j] = NULL;
  
  XtSetSensitive(spy_sabotage_command, FALSE);
  
  XawListChange(spy_improvements_list, (String *) improvements_can_sabotage,
		0, 0, 1);
  XtVaGetValues(spy_improvements_list, XtNwidth, &width1, NULL);
  XtVaGetValues(spy_improvements_list_label, XtNwidth, &width2, NULL);
  XtVaSetValues(spy_improvements_list, XtNwidth, MAX(width1,width2), NULL); 
  XtVaSetValues(spy_improvements_list_label, XtNwidth, MAX(width1,width2), NULL); 

  return j;
}

/****************************************************************
...
*****************************************************************/
static void spy_steal_popup(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  struct city *pvcity = game_find_city_by_number(diplomat_target_id);
  struct player *pvictim = NULL;

  if(pvcity)
    pvictim = city_owner(pvcity);

/* it is concievable that pvcity will not be found, because something
has happened to the city during latency.  Therefore we must initialize
pvictim to NULL and account for !pvictim in create_advances_list. -- Syela */
  
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if(!spy_tech_shell){
    Position x, y;
    Dimension width, height;
    spy_tech_shell_is_modal=1;

    create_advances_list(client.conn.playing, pvictim, spy_tech_shell_is_modal);
    
    XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
    
    XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
		      &x, &y);
    XtVaSetValues(spy_tech_shell, XtNx, x, XtNy, y, NULL);
    
    XtPopup(spy_tech_shell, XtGrabNone);
  }
}

/**************************************************************************
  Requests up-to-date list of improvements, the return of
  which will trigger the popup_sabotage_dialog() function.
**************************************************************************/
static void spy_request_sabotage_list(Widget w, XtPointer client_data,
				      XtPointer call_data)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if (game_find_unit_by_number(diplomat_id)
   && game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, 0);
  }
}

/**************************************************************************
  Pops-up the Spy sabotage dialog, upon return of list of
  available improvements requested by the above function.
**************************************************************************/
void popup_sabotage_dialog(struct city *pcity)
{  
  if(!spy_sabotage_shell){
    Position x, y;
    Dimension width, height;
    spy_sabotage_shell_is_modal=1;

    create_improvements_list(client.conn.playing, pcity, spy_sabotage_shell_is_modal);
    
    XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
    
    XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
		      &x, &y);
    XtVaSetValues(spy_sabotage_shell, XtNx, x, XtNy, y, NULL);
    
    XtPopup(spy_sabotage_shell, XtGrabNone);
  }
}

/****************************************************************
...
*****************************************************************/
static void diplomat_incite_yes_callback(Widget w, XtPointer client_data, 
					 XtPointer call_data)
{
  request_diplomat_action(DIPLOMAT_INCITE, diplomat_id,
			  diplomat_target_id, 0);

  destroy_message_dialog(w);

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_incite_no_callback(Widget w, XtPointer client_data, 
					XtPointer call_data)
{
  destroy_message_dialog(w);

  process_diplomat_arrival(NULL, 0);
}


/**************************************************************************
  Asks the server how much the revolt is going to cost us
**************************************************************************/
static void diplomat_incite_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if (game_find_unit_by_number(diplomat_id)
   && game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_INCITE, diplomat_id,
			    diplomat_target_id, 0);
  }
}

/**************************************************************************
  Popup the yes/no dialog for inciting, since we know the cost now
**************************************************************************/
void popup_incite_dialog(struct city *pcity, int cost)
{
  char buf[128];

  if (INCITE_IMPOSSIBLE_COST == cost) {
    my_snprintf(buf, sizeof(buf), _("You can't incite a revolt in %s."),
		city_name(pcity));
    popup_message_dialog(toplevel, "diplomatnogolddialog", buf,
			 diplomat_incite_no_callback, 0, 0, NULL);
  } else if (cost <= client.conn.playing->economic.gold) {
    my_snprintf(buf, sizeof(buf),
		_("Incite a revolt for %d gold?\n"
		  "Treasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
   diplomat_target_id = pcity->id;
   popup_message_dialog(toplevel, "diplomatrevoltdialog", buf,
			diplomat_incite_yes_callback, 0, 0,
			diplomat_incite_no_callback, 0, 0,
			NULL);
  } else {
   my_snprintf(buf, sizeof(buf),
	       _("Inciting a revolt costs %d gold.\n"
		 "Treasury contains %d gold."), 
	       cost, client.conn.playing->economic.gold);
   popup_message_dialog(toplevel, "diplomatnogolddialog", buf,
			diplomat_incite_no_callback, 0, 0,
			NULL);
  }
}


/**************************************************************************
  Callback from diplomat/spy dialog for "keep moving".
  (This should only occur when entering allied city.)
**************************************************************************/
static void diplomat_keep_moving_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data)
{
  struct unit *punit;
  struct city *pcity;
  
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  if( (punit=game_find_unit_by_number(diplomat_id))
      && (pcity=game_find_city_by_number(diplomat_target_id))
      && !same_pos(punit->tile, pcity->tile)) {
    request_diplomat_action(DIPLOMAT_MOVE, diplomat_id,
			    diplomat_target_id, 0);
  }
  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_cancel_callback(Widget w, XtPointer a, XtPointer b)
{
  destroy_message_dialog(w);
  diplomat_dialog = NULL;

  process_diplomat_arrival(NULL, 0);
}


/****************************************************************
  Popups the diplomat dialog
*****************************************************************/
void popup_diplomat_dialog(struct unit *punit, struct tile *dest_tile)
{
  struct city *pcity;
  struct unit *ptunit;
  char buf[128];

  diplomat_id=punit->id;

  if((pcity=tile_city(dest_tile))){
    /* Spy/Diplomat acting against a city */

    diplomat_target_id=pcity->id;
    my_snprintf(buf, sizeof(buf),
		_("Your %s has arrived at %s.\nWhat is your command?"),
		unit_name_translation(punit),
		city_name(pcity));

    if (!unit_has_type_flag(punit, F_SPY)) {
      diplomat_dialog =
        popup_message_dialog(toplevel, "diplomatdialog", buf,
			       diplomat_embassy_callback, 0, 1,
			       diplomat_investigate_callback, 0, 1,
			       diplomat_sabotage_callback, 0, 1,
			       diplomat_steal_callback, 0, 1,
			       diplomat_incite_callback, 0, 1,
			       diplomat_keep_moving_callback, 0, 1,
			       diplomat_cancel_callback, 0, 0,
			       NULL);
      
      if(!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button0"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button1"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button2"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_STEAL, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button3"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_INCITE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button4"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_MOVE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button5"), FALSE);
    } else {
      diplomat_dialog =
        popup_message_dialog(toplevel, "spydialog", buf,
			       diplomat_embassy_callback, 0,  1,
			       diplomat_investigate_callback, 0, 1,
			       spy_poison_callback,0, 1,
			       spy_request_sabotage_list, 0, 1,
			       spy_steal_popup, 0, 1,
			       diplomat_incite_callback, 0, 1,
			       diplomat_keep_moving_callback, 0, 1,
			       diplomat_cancel_callback, 0, 0,
			       NULL);
      
      if(!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button0"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button1"), FALSE);
      if(!diplomat_can_do_action(punit, SPY_POISON, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button2"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button3"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_STEAL, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button4"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_INCITE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button5"), FALSE);
      if(!diplomat_can_do_action(punit, DIPLOMAT_MOVE, dest_tile))
	XtSetSensitive(XtNameToWidget(diplomat_dialog, "*button6"), FALSE);
    }
  } else { 
    if ((ptunit = unit_list_get(dest_tile->units, 0))) {
      /* Spy/Diplomat acting against a unit */
      
      Widget shl;
      const char *message = !unit_has_type_flag(punit, F_SPY)
	? _("The diplomat is waiting for your command")
	: _("The spy is waiting for your command");
      
      diplomat_target_id=ptunit->id;

      shl=popup_message_dialog(toplevel, "spybribedialog",
			       message,
			       diplomat_bribe_callback, 0, 0,
			       spy_sabotage_unit_callback, 0, 0,
			       diplomat_cancel_callback, 0, 0,
			       NULL);
	
      if(!diplomat_can_do_action(punit, DIPLOMAT_BRIBE, dest_tile))
	XtSetSensitive(XtNameToWidget(shl, "*button0"), FALSE);
      if(!diplomat_can_do_action(punit, SPY_SABOTAGE_UNIT, dest_tile))
	XtSetSensitive(XtNameToWidget(shl, "*button1"), FALSE);
    }
  }
}

/**************************************************************************
  Returns id of a diplomat currently handled in diplomat dialog
**************************************************************************/
int diplomat_handled_in_diplomat_dialog(void)
{
  if (diplomat_dialog == NULL) {
    return -1;
  }
  return diplomat_id;
}

/**************************************************************************
  Closes the diplomat dialog
**************************************************************************/
void close_diplomat_dialog(void)
{
  if (diplomat_dialog != NULL) {
    XtDestroyWidget(diplomat_dialog);
  }
}
