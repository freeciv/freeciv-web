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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/AsciiText.h>  
#include <X11/Xaw/Viewport.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "diptreaty.h"
#include "government.h"
#include "map.h"
#include "packets.h"
#include "player.h"

/* client */
#include "client_main.h"
#include "climisc.h"

/* gui-xaw */
#include "chatline.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"

#include "diplodlg.h"

#define MAX_NUM_CLAUSES 64

struct Diplomacy_dialog {
  struct Treaty treaty;
  
  Widget dip_dialog_shell;
  Widget dip_form, dip_main_form, dip_form0, dip_formm, dip_form1;
  Widget dip_view;
  
  Widget dip_headline0;
  Widget dip_headlinem;
  Widget dip_headline1;

  Widget dip_map_menubutton0;
  Widget dip_map_menubutton1;
  Widget dip_tech_menubutton0;
  Widget dip_tech_menubutton1;
  Widget dip_city_menubutton0;
  Widget dip_city_menubutton1;
  Widget dip_gold_label0;
  Widget dip_gold_label1;
  Widget dip_gold_input0;
  Widget dip_gold_input1;
  Widget dip_vision_button0;
  Widget dip_vision_button1;
  Widget dip_pact_menubutton;
  
  Widget dip_label;
  Widget dip_clauselabel;
  Widget dip_clauselist;
  Widget dip_acceptlabel0;
  Widget dip_acceptthumb0;
  Widget dip_acceptlabel1;
  Widget dip_acceptthumb1;
  
  Widget dip_accept_command;
  Widget dip_close_command;

  Widget dip_erase_clause_command;
  
  char clauselist_strings[MAX_NUM_CLAUSES+1][64];
  char *clauselist_strings_ptrs[MAX_NUM_CLAUSES+1];
};

static char *dummy_clause_list_strings[]
    = { "\n", "\n", "\n", "\n", "\n", "\n", 0 };

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct Diplomacy_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct Diplomacy_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list = NULL;
static bool dialog_list_list_has_been_initialised = FALSE;

struct Diplomacy_dialog *create_diplomacy_dialog(struct player *plr0, 
						 struct player *plr1);

static struct Diplomacy_dialog *find_diplomacy_dialog(int other_player_id);
static void popup_diplomacy_dialog(int other_player_id);
void diplomacy_dialog_close_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data);
void diplomacy_dialog_map_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
void diplomacy_dialog_seamap_callback(Widget w, XtPointer client_data, 
				      XtPointer call_data);
void diplomacy_dialog_erase_clause_callback(Widget w, XtPointer client_data, 
					    XtPointer call_data);
void diplomacy_dialog_accept_callback(Widget w, XtPointer client_data, 
				      XtPointer call_data);
void diplomacy_dialog_tech_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data);
void diplomacy_dialog_city_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data);
void diplomacy_dialog_ceasefire_callback(Widget w, XtPointer client_data,
					 XtPointer call_data);
void diplomacy_dialog_peace_callback(Widget w, XtPointer client_data,
					XtPointer call_data);
void diplomacy_dialog_alliance_callback(Widget w, XtPointer client_data,
					XtPointer call_data);
void diplomacy_dialog_vision_callback(Widget w, XtPointer client_data,
				      XtPointer call_data);
void close_diplomacy_dialog(struct Diplomacy_dialog *pdialog);
void update_diplomacy_dialog(struct Diplomacy_dialog *pdialog);


/****************************************************************
...
*****************************************************************/
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted,
				    bool other_accepted)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  pdialog->treaty.accept0 = I_accepted;
  pdialog->treaty.accept1 = other_accepted;

  update_diplomacy_dialog(pdialog);
}

/****************************************************************
...
*****************************************************************/
void handle_diplomacy_init_meeting(int counterpart, int initiated_from)
{
  popup_diplomacy_dialog(counterpart);
}

/****************************************************************
...
*****************************************************************/
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  close_diplomacy_dialog(pdialog);
}

/****************************************************************
...
*****************************************************************/
void handle_diplomacy_create_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  add_clause(&pdialog->treaty, player_by_number(giver), type, value);
  update_diplomacy_dialog(pdialog);
}

/****************************************************************
...
*****************************************************************/
void handle_diplomacy_remove_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  remove_clause(&pdialog->treaty, player_by_number(giver), type, value);
  update_diplomacy_dialog(pdialog);
}

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
static void popup_diplomacy_dialog(int other_player_id)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(other_player_id);

  if (NULL == client.conn.playing || client.conn.playing->ai_data.control) {
    return;			/* Don't show if we are AI controlled. */
  }

  if (!pdialog) {
    Position x, y;
    Dimension width, height;

    pdialog = create_diplomacy_dialog(client.conn.playing,
				      player_by_number(other_player_id));
    XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
    XtTranslateCoords(toplevel, (Position) width / 10,
		      (Position) height / 10, &x, &y);
    XtVaSetValues(pdialog->dip_dialog_shell, XtNx, x, XtNy, y, NULL);
  }

  XtPopup(pdialog->dip_dialog_shell, XtGrabNone);
}


/****************************************************************
...
*****************************************************************/
static int fill_diplomacy_tech_menu(Widget popupmenu, 
				    struct player *plr0, struct player *plr1)
{
  int flag = 0;
  
  advance_index_iterate(A_FIRST, i) {
    if (player_invention_state(plr0, i) == TECH_KNOWN
        && player_invention_reachable(plr1, i)
        && (player_invention_state(plr1, i) == TECH_UNKNOWN
	    || player_invention_state(plr1, i) == TECH_PREREQS_KNOWN)) {
      Widget entry=
	XtVaCreateManagedWidget(advance_name_translation(advance_by_number(i)),
				smeBSBObjectClass,
				popupmenu,
				NULL);
      XtAddCallback(entry, XtNcallback, diplomacy_dialog_tech_callback,
			 INT_TO_XTPOINTER((player_number(plr0) << 24) |
					 (player_number(plr1) << 16) |
					 i));
      flag=1;
    }
  } advance_index_iterate_end;
  return flag;
}

/****************************************************************
Creates a sorted list of plr0's cities, excluding the capital and
any cities not visible to plr1.  This means that you can only trade 
cities visible to requesting player.  

                            - Kris Bubendorfer
*****************************************************************/
static int fill_diplomacy_city_menu(Widget popupmenu, 
				    struct player *plr0, struct player *plr1)
{
  int i = 0, j = 0, n = city_list_size(plr0->cities);
  struct city **city_list_ptrs;
  if (n>0) {
    city_list_ptrs = fc_malloc(sizeof(struct city*)*n);
  } else {
    city_list_ptrs = NULL;
  }

  city_list_iterate(plr0->cities, pcity) {
    if (!is_capital(pcity)) {
      city_list_ptrs[i] = pcity;
      i++;
    }
  } city_list_iterate_end;

  qsort(city_list_ptrs, i, sizeof(struct city*), city_name_compare);
  
  for(j=0; j<i; j++) {
    Widget entry=
      XtVaCreateManagedWidget(city_name(city_list_ptrs[j]), smeBSBObjectClass, 
			      popupmenu, NULL);
    XtAddCallback(entry, XtNcallback, diplomacy_dialog_city_callback,
		  INT_TO_XTPOINTER((player_number(plr0) << 24) |
				   (player_number(plr1) << 16) |
				   city_list_ptrs[j]->id));
  }
  free(city_list_ptrs);
  return i;
}


/****************************************************************
...
*****************************************************************/
struct Diplomacy_dialog *create_diplomacy_dialog(struct player *plr0, 
						 struct player *plr1)
{
  char buf[512], *pheadlinem;
  struct Diplomacy_dialog *pdialog;
  Dimension width, height, maxwidth;
  Widget popupmenu;
  Widget entry;

  pdialog=fc_malloc(sizeof(struct Diplomacy_dialog));
  dialog_list_prepend(dialog_list, pdialog);
  
  init_treaty(&pdialog->treaty, plr0, plr1);
  
  pdialog->dip_dialog_shell =
    I_T(XtCreatePopupShell("dippopupshell", topLevelShellWidgetClass,
			   toplevel, NULL, 0));

  pdialog->dip_form = XtVaCreateManagedWidget("dipform", 
					      formWidgetClass, 
					      pdialog->dip_dialog_shell,
					      NULL);

  pdialog->dip_main_form = XtVaCreateManagedWidget("dipmainform", 
						   formWidgetClass, 
						   pdialog->dip_form,
						   NULL);
  
  pdialog->dip_form0 = XtVaCreateManagedWidget("dipform0", 
					       formWidgetClass, 
					       pdialog->dip_main_form, 
					       NULL);
  
  pdialog->dip_formm = XtVaCreateManagedWidget("dipformm", 
					       formWidgetClass, 
					       pdialog->dip_main_form, 
					       NULL);

  pdialog->dip_form1 = XtVaCreateManagedWidget("dipform1", 
					       formWidgetClass, 
					       pdialog->dip_main_form, 
					       NULL);
  
  my_snprintf(buf, sizeof(buf), _("The %s offerings"),
	      nation_adjective_for_player(plr0));
  pdialog->dip_headline0=XtVaCreateManagedWidget("dipheadline0", 
						 labelWidgetClass, 
						 pdialog->dip_form0, 
						 XtNlabel, buf,
						 NULL);   

  my_snprintf(buf, sizeof(buf), _("The %s offerings"),
	      nation_adjective_for_player(plr1));
  pdialog->dip_headline1=XtVaCreateManagedWidget("dipheadline1", 
						 labelWidgetClass, 
						 pdialog->dip_form1, 
						 XtNlabel, buf,
						 NULL);   

  
  pdialog->dip_map_menubutton0 =
    I_L(XtVaCreateManagedWidget("dipmapmenubutton0", 
				menuButtonWidgetClass, 
				pdialog->dip_form0, 
				NULL));
  popupmenu=XtVaCreatePopupShell("menu", 
				 simpleMenuWidgetClass, 
				 pdialog->dip_map_menubutton0, 
				 NULL);
  
  entry=XtVaCreateManagedWidget(_("World-map"), smeBSBObjectClass,
				popupmenu, NULL);
  XtAddCallback(entry, XtNcallback, diplomacy_dialog_map_callback,
		(XtPointer)pdialog);
  entry=XtVaCreateManagedWidget(_("Sea-map"), smeBSBObjectClass,
				popupmenu, NULL);
  XtAddCallback(entry, XtNcallback, diplomacy_dialog_seamap_callback,
		(XtPointer)pdialog);
  
  pdialog->dip_map_menubutton1 =
    I_L(XtVaCreateManagedWidget("dipmapmenubutton1", 
				menuButtonWidgetClass, 
				pdialog->dip_form1, 
				NULL));
  popupmenu=XtVaCreatePopupShell("menu", 
				 simpleMenuWidgetClass, 
				 pdialog->dip_map_menubutton1, 
				 NULL);
  entry=XtVaCreateManagedWidget(_("World-map"), smeBSBObjectClass,
				popupmenu, NULL);
  XtAddCallback(entry, XtNcallback, diplomacy_dialog_map_callback,
		(XtPointer)pdialog);
  entry=XtVaCreateManagedWidget(_("Sea-map"), smeBSBObjectClass,
				popupmenu, NULL);
  XtAddCallback(entry, XtNcallback, diplomacy_dialog_seamap_callback,
		(XtPointer)pdialog);
  

  pdialog->dip_tech_menubutton0 =
    I_L(XtVaCreateManagedWidget("diptechmenubutton0", 
				menuButtonWidgetClass,
				pdialog->dip_form0,
				NULL));
  popupmenu=XtVaCreatePopupShell("menu", 
				 simpleMenuWidgetClass, 
				 pdialog->dip_tech_menubutton0, 
				 NULL);

  if(!fill_diplomacy_tech_menu(popupmenu, plr0, plr1))
    XtSetSensitive(pdialog->dip_tech_menubutton0, FALSE);
  
  
  pdialog->dip_tech_menubutton1 =
    I_L(XtVaCreateManagedWidget("diptechmenubutton1", 
				menuButtonWidgetClass,
				pdialog->dip_form1,
				NULL));
  popupmenu=XtVaCreatePopupShell("menu", 
				 simpleMenuWidgetClass, 
				 pdialog->dip_tech_menubutton1, 
				 NULL);
  if(!fill_diplomacy_tech_menu(popupmenu, plr1, plr0))
    XtSetSensitive(pdialog->dip_tech_menubutton1, FALSE);

  /* Start of trade city code - Kris Bubendorfer */

  pdialog->dip_city_menubutton0 =
    I_L(XtVaCreateManagedWidget("dipcitymenubutton0", 
				menuButtonWidgetClass,
				pdialog->dip_form0,
				NULL));
  popupmenu=XtVaCreatePopupShell("menu", 
				 simpleMenuWidgetClass, 
				 pdialog->dip_city_menubutton0, 
				 NULL);
  
  XtSetSensitive(pdialog->dip_city_menubutton0, 
		 fill_diplomacy_city_menu(popupmenu, plr0, plr1));
  
  
  pdialog->dip_city_menubutton1 =
    I_L(XtVaCreateManagedWidget("dipcitymenubutton1", 
				menuButtonWidgetClass,
				pdialog->dip_form1,
				NULL));
  popupmenu=XtVaCreatePopupShell("menu", 
				 simpleMenuWidgetClass, 
				 pdialog->dip_city_menubutton1, 
				 NULL);
  
  XtSetSensitive(pdialog->dip_city_menubutton1, 
		 fill_diplomacy_city_menu(popupmenu, plr1, plr0));  
  
  /* End of trade city code */
  
  pdialog->dip_gold_input0=XtVaCreateManagedWidget("dipgoldinput0", 
						   asciiTextWidgetClass,
						   pdialog->dip_form0,
						   NULL);

  pdialog->dip_gold_input1=XtVaCreateManagedWidget("dipgoldinput1", 
						   asciiTextWidgetClass,
						   pdialog->dip_form1,
						   NULL);
  
  my_snprintf(buf, sizeof(buf), _("Gold(max %d)"), plr0->economic.gold);
  pdialog->dip_gold_label0=XtVaCreateManagedWidget("dipgoldlabel0", 
						   labelWidgetClass,
						   pdialog->dip_form0,
						   XtNlabel, buf,
						   NULL);

  my_snprintf(buf, sizeof(buf), _("Gold(max %d)"), plr1->economic.gold);
  pdialog->dip_gold_label1=XtVaCreateManagedWidget("dipgoldlabel1", 
						   labelWidgetClass,
						   pdialog->dip_form1,
						   XtNlabel, buf,
						   NULL);


  pdialog->dip_vision_button0 =
    I_L(XtVaCreateManagedWidget("dipvisionbutton0",
				commandWidgetClass, 
				pdialog->dip_form0,
				NULL));
  if (gives_shared_vision(plr0, plr1))
    XtSetSensitive(pdialog->dip_vision_button0, FALSE);

  pdialog->dip_vision_button1 =
    I_L(XtVaCreateManagedWidget("dipvisionbutton1",
				commandWidgetClass, 
				pdialog->dip_form1,
				NULL));
  if (gives_shared_vision(plr1, plr0))
    XtSetSensitive(pdialog->dip_vision_button1, FALSE);

  XtAddCallback(pdialog->dip_vision_button0, XtNcallback, 
		diplomacy_dialog_vision_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->dip_vision_button1, XtNcallback, 
		diplomacy_dialog_vision_callback, (XtPointer)pdialog);


  pdialog->dip_pact_menubutton=
    I_L(XtVaCreateManagedWidget("dippactmenubutton",
				menuButtonWidgetClass,
				pdialog->dip_form0,
				NULL));
  popupmenu=XtVaCreatePopupShell("menu", 
				 simpleMenuWidgetClass, 
				 pdialog->dip_pact_menubutton, 
				 NULL);
  entry=XtVaCreateManagedWidget(Q_("?diplomatic_state:Cease-fire"),
				smeBSBObjectClass, popupmenu, NULL);
  XtAddCallback(entry, XtNcallback, diplomacy_dialog_ceasefire_callback,
		(XtPointer)pdialog);
  entry=XtVaCreateManagedWidget(Q_("?diplomatic_state:Peace"),
				smeBSBObjectClass, popupmenu, NULL);
  XtAddCallback(entry, XtNcallback, diplomacy_dialog_peace_callback,
		(XtPointer)pdialog);
  entry=XtVaCreateManagedWidget(Q_("?diplomatic_state:Alliance"),
				smeBSBObjectClass, popupmenu, NULL);
  XtAddCallback(entry, XtNcallback, diplomacy_dialog_alliance_callback,
		(XtPointer)pdialog);
  
  my_snprintf(buf, sizeof(buf),
	      _("This Eternal Treaty\n"
		 "marks the results of the diplomatic work between\n"
		 "The %s %s %s\nand\nThe %s %s %s"),
	  nation_adjective_for_player(plr0),
	  ruler_title_translation(plr0),
	  player_name(plr0),
	  nation_adjective_for_player(plr1),
	  ruler_title_translation(plr1),
	  player_name(plr1));
  
  pheadlinem=create_centered_string(buf);
  pdialog->dip_headline1=XtVaCreateManagedWidget("dipheadlinem", 
						 labelWidgetClass, 
						 pdialog->dip_formm,
						 XtNlabel, pheadlinem,
						 NULL);
  
  pdialog->dip_clauselabel =
    I_L(XtVaCreateManagedWidget("dipclauselabel",
				labelWidgetClass, 
				pdialog->dip_formm, 
				NULL));   
  
  pdialog->dip_view =  XtVaCreateManagedWidget("dipview",
					       viewportWidgetClass, 
					       pdialog->dip_formm, 
					       NULL);
  
  
  pdialog->dip_clauselist = XtVaCreateManagedWidget("dipclauselist",
						    listWidgetClass, 
						    pdialog->dip_view, 
						    XtNlist, 
						    (XtArgVal)dummy_clause_list_strings,
						    NULL);

  XtVaGetValues(pdialog->dip_headline1, XtNwidth, &width, NULL);
  XtVaSetValues(pdialog->dip_view, XtNwidth, width, NULL); 
  XtVaSetValues(pdialog->dip_clauselist, XtNwidth, width, NULL); 

  my_snprintf(buf, sizeof(buf), _("%s view:"),
              nation_adjective_for_player(plr0));
  pdialog->dip_acceptlabel0=XtVaCreateManagedWidget("dipacceptlabel0",
						    labelWidgetClass, 
						    pdialog->dip_formm, 
						    XtNlabel, buf,
						    NULL);
  pdialog->dip_acceptthumb0=XtVaCreateManagedWidget("dipacceptthumb0",
						    labelWidgetClass, 
						    pdialog->dip_formm, 
						    XtNbitmap, get_thumb_pixmap(0),
						    NULL);
  my_snprintf(buf, sizeof(buf), _("%s view:"),
              nation_adjective_for_player(plr1));
  pdialog->dip_acceptlabel1=XtVaCreateManagedWidget("dipacceptlabel1",
						    labelWidgetClass, 
						    pdialog->dip_formm, 
						    XtNlabel, buf,
						    NULL);
  pdialog->dip_acceptthumb1=XtVaCreateManagedWidget("dipacceptthumb1",
						    labelWidgetClass, 
						    pdialog->dip_formm, 
						    NULL);

  
  pdialog->dip_erase_clause_command =
    I_L(XtVaCreateManagedWidget("diperaseclausecommand",
				commandWidgetClass, 
				pdialog->dip_main_form, 
				NULL));
  
  pdialog->dip_accept_command =
    I_L(XtVaCreateManagedWidget("dipacceptcommand", 
				commandWidgetClass, 
				pdialog->dip_form,
				NULL));

  pdialog->dip_close_command =
    I_L(XtVaCreateManagedWidget("dipclosecommand", 
				commandWidgetClass,
				pdialog->dip_form,
				NULL));

  XtAddCallback(pdialog->dip_close_command, XtNcallback, 
		diplomacy_dialog_close_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->dip_erase_clause_command, XtNcallback, 
		diplomacy_dialog_erase_clause_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->dip_accept_command, XtNcallback, 
		diplomacy_dialog_accept_callback, (XtPointer)pdialog);


  XtRealizeWidget(pdialog->dip_dialog_shell);


  XtVaGetValues(pdialog->dip_map_menubutton0, XtNwidth, &maxwidth, NULL);
  XtVaGetValues(pdialog->dip_tech_menubutton0, XtNwidth, &width, NULL);
  XtVaGetValues(pdialog->dip_city_menubutton0, XtNwidth, &width, NULL);
  maxwidth=MAX(width, maxwidth);
  XtVaGetValues(pdialog->dip_gold_input0, XtNwidth, &width, NULL);
  maxwidth=MAX(width, maxwidth);
  XtVaSetValues(pdialog->dip_map_menubutton0, XtNwidth, maxwidth, NULL);
  XtVaSetValues(pdialog->dip_tech_menubutton0, XtNwidth, maxwidth, NULL);
  XtVaSetValues(pdialog->dip_city_menubutton0, XtNwidth, maxwidth, NULL);
  XtVaSetValues(pdialog->dip_gold_input0,  XtNwidth, maxwidth, NULL);
  
  XtVaGetValues(pdialog->dip_formm, XtNheight, &height, NULL);
  XtVaSetValues(pdialog->dip_form0, XtNheight, height, NULL); 
  XtVaSetValues(pdialog->dip_form1, XtNheight, height, NULL); 


  free(pheadlinem);

  update_diplomacy_dialog(pdialog);
  
  return pdialog;
}


/**************************************************************************
...
**************************************************************************/
void update_diplomacy_dialog(struct Diplomacy_dialog *pdialog)
{
  int i = 0;
  const int n = sizeof(pdialog->clauselist_strings[0]);
  
  clause_list_iterate(pdialog->treaty.clauses, pclause) {
    client_diplomacy_clause_string(pdialog->clauselist_strings[i], n, pclause);
    pdialog->clauselist_strings_ptrs[i]=pdialog->clauselist_strings[i];
    i++;
  } clause_list_iterate_end;

  pdialog->clauselist_strings_ptrs[i]=0;
  XawListChange(pdialog->dip_clauselist, pdialog->clauselist_strings_ptrs, 
		0, 0, False);

/* force refresh of viewport so that scrollbar is added
   sun seems to need this */ 
  XtVaSetValues(pdialog->dip_view, XtNforceBars, False, NULL);
  XtVaSetValues(pdialog->dip_view, XtNforceBars, True, NULL);

  xaw_set_bitmap(pdialog->dip_acceptthumb0,
		 get_thumb_pixmap(pdialog->treaty.accept0));
  xaw_set_bitmap(pdialog->dip_acceptthumb1, 
		 get_thumb_pixmap(pdialog->treaty.accept1));
}

/****************************************************************
  Callback for trading techs
*****************************************************************/
void diplomacy_dialog_tech_callback(Widget w, XtPointer client_data,
				    XtPointer call_data)
{
  size_t choice = (size_t) client_data;
  int giver = (choice >> 24) & 0xff, dest = (choice >> 16) & 0xff, other;
  int tech = choice & 0xffff;

  if (player_by_number(giver) == client.conn.playing) {
    other = dest;
  } else {
    other = giver;
  }

  dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
					   CLAUSE_ADVANCE, tech);
}

/****************************************************************
Callback for trading cities
                              - Kris Bubendorfer
*****************************************************************/
void diplomacy_dialog_city_callback(Widget w, XtPointer client_data,
				    XtPointer call_data)
{
  size_t choice = (size_t) client_data;
  int giver = (choice >> 24) & 0xff, dest = (choice >> 16) & 0xff, other;
  int city = choice & 0xffff;

  if (player_by_number(giver) == client.conn.playing) {
    other = dest;
  } else {
    other = giver;
  }

  dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
					   CLAUSE_CITY, city);
}

/****************************************************************
...
*****************************************************************/
void diplomacy_dialog_erase_clause_callback(Widget w, XtPointer client_data, 
					    XtPointer call_data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) client_data;
  XawListReturnStruct *ret = XawListShowCurrent(pdialog->dip_clauselist);

  if (ret->list_index != XAW_LIST_NONE) {
    int i = 0;

    clause_list_iterate(pdialog->treaty.clauses, pclause) {
      if (i == ret->list_index) {
	dsend_packet_diplomacy_remove_clause_req(&client.conn,
						 player_number(pdialog->treaty.plr1),
						 player_number(pclause->from),
						 pclause->type,
						 pclause->value);
	return;
      }
      i++;
    } clause_list_iterate_end;
  }
}

/****************************************************************
  Callback for trading map
*****************************************************************/
void diplomacy_dialog_map_callback(Widget w, XtPointer client_data,
				   XtPointer call_data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) client_data;
  struct player *pgiver =
      (XtParent(XtParent(w)) ==
       pdialog->dip_map_menubutton0) ? pdialog->treaty.plr0 : pdialog->
      treaty.plr1;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), CLAUSE_MAP, 0);
}

/****************************************************************
  Callback for trading seamap
*****************************************************************/
void diplomacy_dialog_seamap_callback(Widget w, XtPointer client_data,
				      XtPointer call_data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) client_data;
  struct player *pgiver =
      (XtParent(XtParent(w)) ==
       pdialog->dip_map_menubutton0) ? pdialog->treaty.plr0 : pdialog->
      treaty.plr1;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), CLAUSE_SEAMAP,
					   0);
}

/****************************************************************
...
*****************************************************************/
void diplomacy_dialog_vision_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) client_data;
  struct player *pgiver = (w == pdialog->dip_vision_button0) ?
      pdialog->treaty.plr0 : pdialog->treaty.plr1;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), CLAUSE_VISION,
					   0);
}

/****************************************************************
Generic add-a-clause function for adding pact types
*****************************************************************/
static void diplomacy_dialog_add_pact_clause(Widget w, XtPointer client_data,
					     XtPointer call_data, int type)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) client_data;
  struct player *pgiver = (w == pdialog->dip_vision_button0) ?
      pdialog->treaty.plr0 : pdialog->treaty.plr1;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), type, 0);
}

/****************************************************************
The cease-fire widget was selected; add a cease-fire to the
clauses
*****************************************************************/
void diplomacy_dialog_ceasefire_callback(Widget w, XtPointer client_data, 
					 XtPointer call_data)
{
  diplomacy_dialog_add_pact_clause(w, client_data, call_data,
				   CLAUSE_CEASEFIRE);
}

/****************************************************************
The peace widget was selected; add a peace treaty to the
clauses
*****************************************************************/
void diplomacy_dialog_peace_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data)
{
  diplomacy_dialog_add_pact_clause(w, client_data, call_data,
				   CLAUSE_PEACE);
}

/****************************************************************
The alliance widget was selected; add an alliance to the
clauses
*****************************************************************/
void diplomacy_dialog_alliance_callback(Widget w, XtPointer client_data, 
					XtPointer call_data)
{
  diplomacy_dialog_add_pact_clause(w, client_data, call_data,
				   CLAUSE_ALLIANCE);
}


/****************************************************************
...
*****************************************************************/
void diplomacy_dialog_close_callback(Widget w, XtPointer client_data,
				     XtPointer call_data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) client_data;

  dsend_packet_diplomacy_cancel_meeting_req(&client.conn,
					    player_number(pdialog->treaty.plr1));
  close_diplomacy_dialog(pdialog);
}


/****************************************************************
...
*****************************************************************/
void diplomacy_dialog_accept_callback(Widget w, XtPointer client_data, 
				      XtPointer call_data)
{
  struct Diplomacy_dialog *pdialog = (struct Diplomacy_dialog *) client_data;

  dsend_packet_diplomacy_accept_treaty_req(&client.conn,
					   player_number(pdialog->treaty.plr1));
}


/*****************************************************************
...
*****************************************************************/
void close_diplomacy_dialog(struct Diplomacy_dialog *pdialog)
{
  XtDestroyWidget(pdialog->dip_dialog_shell);
  
  dialog_list_unlink(dialog_list, pdialog);
  free(pdialog);
}

/*****************************************************************
...
*****************************************************************/
static struct Diplomacy_dialog *find_diplomacy_dialog(int other_player_id)
{
  struct player *plr0 = client.conn.playing;
  struct player *plr1 = player_by_number(other_player_id);

  if (!dialog_list_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_list_has_been_initialised = TRUE;
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if ((pdialog->treaty.plr0 == plr0 && pdialog->treaty.plr1 == plr1) ||
	(pdialog->treaty.plr0 == plr1 && pdialog->treaty.plr1 == plr0)) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/*****************************************************************
...
*****************************************************************/
static struct Diplomacy_dialog *find_diplomacy_by_input(Widget w)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if ((pdialog->dip_gold_input0 == w) || (pdialog->dip_gold_input1 == w)) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/*****************************************************************
...
*****************************************************************/
void diplodlg_key_gold(Widget w)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_by_input(w);
  
  if (pdialog) {
    struct player *pgiver = (w == pdialog->dip_gold_input0) ?
	pdialog->treaty.plr0 : pdialog->treaty.plr1;
    XtPointer dp;
    int amount;
    
    XtVaGetValues(w, XtNstring, &dp, NULL);
    if (sscanf(dp, "%d", &amount) == 1 && amount >= 0
	&& amount <= pgiver->economic.gold) {
      dsend_packet_diplomacy_create_clause_req(&client.conn,
					       player_number(pdialog->treaty.plr1),
					       player_number(pgiver),
					       CLAUSE_GOLD, amount);
      XtVaSetValues(w, XtNstring, "", NULL);
    } else {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("Invalid amount of gold specified."));
    }
  }
}

/*****************************************************************
  Close all dialogs, for when client disconnects from game.
*****************************************************************/
void close_all_diplomacy_dialogs(void)
{
  if (!dialog_list_list_has_been_initialised) {
    return;
  }

  while (dialog_list_size(dialog_list) > 0) {
    close_diplomacy_dialog(dialog_list_get(dialog_list, 0));
  }
}
