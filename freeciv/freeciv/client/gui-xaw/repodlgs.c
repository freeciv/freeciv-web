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
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/AsciiText.h>  
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h> 
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Viewport.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "government.h"
#include "packets.h"
#include "unitlist.h"

/* client */
#include "chatline_common.h" /* for send_chat() */
#include "client_main.h"
#include "climisc.h"
#include "text.h"

#include "cityrep.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "optiondlg.h"

#include "repodlgs_common.h"
#include "repodlgs.h"

/******************************************************************/
static Widget science_dialog_shell;
static Widget science_label;
static Widget science_current_label, science_goal_label;
static Widget science_change_menu_button, science_goal_menu_button;
static Widget science_list, science_help_toggle;
static Widget science_help_note;
static Widget popupmenu, goalmenu;
static int science_dialog_shell_is_modal;

void create_science_dialog(bool make_modal);
void science_close_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data);
void science_help_callback(Widget w, XtPointer client_data, 
			   XtPointer call_data);
void science_change_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data);
void science_goal_callback(Widget w, XtPointer client_data, 
			   XtPointer call_data);


/******************************************************************/
static Widget economy_dialog_shell;
static Widget economy_label, economy_label2;
static Widget economy_list, economy_list_label;
static Widget sellall_command, sellobsolete_command;
static int economy_dialog_shell_is_modal;
static struct impr_type *economy_improvement_type[B_LAST];

void create_economy_report_dialog(bool make_modal);
void economy_close_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data);
void economy_selloff_callback(Widget w, XtPointer client_data,
                            XtPointer call_data);
void economy_list_callback(Widget w, XtPointer client_data,
                         XtPointer call_data);

/******************************************************************/
static Widget activeunits_dialog_shell;
static Widget activeunits_label, activeunits_label2;
static Widget activeunits_list, activeunits_list_label;
static Widget upgrade_command;
static int activeunits_dialog_shell_is_modal;
static int activeunits_type[U_LAST];

void create_activeunits_report_dialog(bool make_modal);
void activeunits_close_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data);
void activeunits_upgrade_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data);
void activeunits_refresh_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data);
void activeunits_list_callback(Widget w, XtPointer client_data, 
                           XtPointer call_data);
/******************************************************************/
static Widget settable_options_dialog_shell;
static Widget settable_options_form;
static Widget settable_options_label;
static Widget settable_options_viewport;
static Widget settable_options_scrollform;
static Widget *settable_options_widgets = NULL;
static Widget settable_options_ok_command;
static Widget settable_options_cancel_command;

void create_settable_options_dialog(void);
void update_settable_options_dialog(void);
void settable_options_toggle_callback(Widget w, XtPointer client_data,
				      XtPointer call_data);
void settable_options_ok_callback(Widget w, XtPointer client_data,
				  XtPointer call_data);
void settable_options_cancel_callback(Widget w, XtPointer client_data,
				      XtPointer call_data);

/******************************************************************
...
*******************************************************************/
void update_report_dialogs(void)
{
  if(is_report_dialogs_frozen()) return;
  activeunits_report_dialog_update();
  economy_report_dialog_update();
  city_report_dialog_update(); 
  science_dialog_update();
}

/****************************************************************
...
****************************************************************/
const char *get_centered_report_title(const char *report_name)
{
  return create_centered_string(get_report_title(report_name));
}

/****************************************************************
...
****************************************************************/
static const char *get_report_title_plus(const char *report_name,
					 const char *additional)
{
  char buf[512];
  
  my_snprintf(buf, sizeof(buf), "%s%s", get_report_title(report_name),
	      additional);

  return create_centered_string(buf);
}

/****************************************************************
...
************************ ***************************************/
void popup_science_dialog(bool make_modal)
{
  if (!science_dialog_shell && NULL != client.conn.playing) {
    Position x, y;
    Dimension width, height;
    
    science_dialog_shell_is_modal=make_modal;
    
    if(make_modal)
      XtSetSensitive(main_form, FALSE);
    
    create_science_dialog(make_modal);
    
    XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
    
    XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
		      &x, &y);
    XtVaSetValues(science_dialog_shell, XtNx, x, XtNy, y, NULL);
    
    XtPopup(science_dialog_shell, XtGrabNone);
  }
}

/****************************************************************
  Closes the science dialog.
*****************************************************************/
void popdown_science_dialog(void)
{
  if (science_dialog_shell) {
    if (science_dialog_shell_is_modal) {
      XtSetSensitive(main_form, TRUE);
    }
    XtDestroyWidget(science_dialog_shell);
    science_dialog_shell = 0;
  }
}

/****************************************************************
...
*****************************************************************/
void create_science_dialog(bool make_modal)
{
  Widget science_form;
  Widget  close_command;
  const static char *tech_list_names_ptrs[A_LAST + 1];
  Dimension width;
  char rate_text[128];
  char current_text[512];
  char goal_text[512];
  const char *report_title;
  int num_list, j = 0, flag = 0;

  if (NULL != client.conn.playing) {
    struct player_research *research = get_player_research(client.conn.playing);

    if (research->researching == A_UNSET) {
      my_snprintf(current_text, sizeof(current_text),
		  _("Researching %s: %d/%s"),
		  advance_name_translation(advance_by_number(A_NONE)),
		  research->bulbs_researched,
		  _("unknown"));
    } else {
      my_snprintf(current_text, sizeof(current_text),
		  _("Researching %s: %d/%d"),
		  advance_name_researching(client.conn.playing),
		  research->bulbs_researched,
		  total_bulbs_required(client.conn.playing));
    }

    if (research->tech_goal == A_UNSET) {
      my_snprintf(goal_text, sizeof(goal_text),
		  _("Goal: %s (%d steps)"),
		  advance_name_translation(advance_by_number(A_NONE)),
		  0);
    } else {
      my_snprintf(goal_text, sizeof(goal_text),
		  _("Goal: %s (%d steps)"),
		  advance_name_translation(advance_by_number(research->tech_goal)),
		  num_unknown_techs_for_goal(client.conn.playing,
					     research->tech_goal));
    }

    advance_index_iterate(A_FIRST, i) {
      if (TECH_KNOWN == player_invention_state(client.conn.playing, i)) {
	tech_list_names_ptrs[j] = advance_name_translation(advance_by_number(i));
	j++;
      }
    } advance_index_iterate_end;
    tech_list_names_ptrs[j] = 0;
    qsort(tech_list_names_ptrs, j, sizeof(char *), compare_strings_ptrs);
    num_list = j;

    science_dialog_shell =
      I_T(XtVaCreatePopupShell("sciencepopup",
			       (make_modal ? transientShellWidgetClass :
			        topLevelShellWidgetClass),
			        toplevel, NULL));

    science_form = XtVaCreateManagedWidget("scienceform",
					   formWidgetClass,
					   science_dialog_shell,
					   NULL);
    my_snprintf(rate_text, sizeof(rate_text), "\ntext not set yet");
    /* TRANS: Research report title */
    report_title = get_report_title_plus(_("Research"), rate_text);
    science_label = XtVaCreateManagedWidget("sciencelabel",
					    labelWidgetClass,
					    science_form,
					    XtNlabel, report_title,
					    NULL);
    free((void *) report_title);

    science_current_label =
      XtVaCreateManagedWidget("sciencecurrentlabel",
			      labelWidgetClass,
			      science_form,
			      XtNlabel, current_text,
			      NULL);

    science_goal_label =
      XtVaCreateManagedWidget("sciencegoallabel",
			      labelWidgetClass,
			      science_form,
			      XtNlabel, goal_text,
			      NULL);

    science_change_menu_button =
      I_L(XtVaCreateManagedWidget("sciencechangemenubutton",
				  menuButtonWidgetClass,
				  science_form, NULL));

    science_goal_menu_button =
      I_L(XtVaCreateManagedWidget("sciencegoalmenubutton",
				  menuButtonWidgetClass,
				  science_form, NULL));

    science_help_note =
      I_L(XtVaCreateManagedWidget("sciencehelpnote",
				  labelWidgetClass,
				  science_form, NULL));

    science_help_toggle =
      XtVaCreateManagedWidget("sciencehelptoggle",
			      toggleWidgetClass,
			      science_form,
			      NULL);

    science_list =
      XtVaCreateManagedWidget("sciencelist",
			      listWidgetClass,
			      science_form,
			      XtNlist, tech_list_names_ptrs,
			      NULL);

    close_command =
      I_L(XtVaCreateManagedWidget("scienceclosecommand",
				  commandWidgetClass,
				  science_form, NULL));

    popupmenu =
      XtVaCreatePopupShell("menu",
			   simpleMenuWidgetClass,
			   science_change_menu_button,
			   NULL);

    goalmenu =
      XtVaCreatePopupShell("menu",
			   simpleMenuWidgetClass,
			   science_goal_menu_button,
			   NULL);

    advance_index_iterate(A_FIRST, i) {
      if (TECH_PREREQS_KNOWN ==
            player_invention_state(client.conn.playing, i)) {
	Widget entry =
	  XtVaCreateManagedWidget(advance_name_translation(advance_by_number(i)),
				  smeBSBObjectClass,
				  popupmenu,
				  NULL);
	XtAddCallback(entry, XtNcallback, science_change_callback,
		      INT_TO_XTPOINTER(i));
      flag = 1;
      }
    } advance_index_iterate_end;

    if (!flag) {
      XtSetSensitive(science_change_menu_button, FALSE);
    }

    flag = 0;
    advance_index_iterate(A_FIRST, i) {
      if (player_invention_reachable(client.conn.playing, i)
	  && TECH_KNOWN != player_invention_state(client.conn.playing, i)
	  && (11 > num_unknown_techs_for_goal(client.conn.playing, i)
	      || i == research->tech_goal)) {
	Widget entry =
	  XtVaCreateManagedWidget(advance_name_translation(advance_by_number(i)),
				  smeBSBObjectClass,
				  goalmenu,
				  NULL);
	XtAddCallback(entry, XtNcallback, science_goal_callback, 
		      INT_TO_XTPOINTER(i)); 
	flag = 1;
      }
    } advance_index_iterate_end;

    if (!flag) {
      XtSetSensitive(science_goal_menu_button, FALSE);
    }

    XtAddCallback(close_command, XtNcallback, science_close_callback, NULL);
    XtAddCallback(science_list, XtNcallback, science_help_callback, NULL);
    XtAddCallback(science_help_toggle, XtNcallback, toggle_callback, NULL);

    if (num_list > 60) {
      int ncol;
      XtVaGetValues(science_list, XtNdefaultColumns, &ncol, NULL);
      XtVaSetValues(science_list, XtNdefaultColumns, ncol+1, NULL);
    }

    XtRealizeWidget(science_dialog_shell);

    if (!make_modal) {
      XSetWMProtocols(display, XtWindow(science_dialog_shell), 
		      &wm_delete_window, 1);
      XtOverrideTranslations(science_dialog_shell,
        XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-science-report()"));
    }

    width = 500;
    XtVaSetValues(science_label, XtNwidth, &width, NULL);

    toggle_callback(science_help_toggle, NULL, NULL);
    science_dialog_update();
  }
}


/****************************************************************
...
*****************************************************************/
void science_change_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data)
{
  size_t to = (size_t) client_data;
  Boolean b;

  XtVaGetValues(science_help_toggle, XtNstate, &b, NULL);
  if (b == TRUE) {
    popup_help_dialog_typed(advance_name_translation(advance_by_number(to)),
                            HELP_TECH);
  } else {
    dsend_packet_player_research(&client.conn, to);
  }
}

/****************************************************************
...
*****************************************************************/
void science_goal_callback(Widget w, XtPointer client_data, 
			   XtPointer call_data)
{
  size_t to = (size_t) client_data;
  Boolean b;

  XtVaGetValues(science_help_toggle, XtNstate, &b, NULL);
  if (b == TRUE) {
    popup_help_dialog_typed(advance_name_translation(advance_by_number(to)),
                            HELP_TECH);
  } else {
    dsend_packet_player_tech_goal(&client.conn, to);
  }
}


/****************************************************************
...
*****************************************************************/
void science_close_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  popdown_science_dialog();
}


/****************************************************************
...
*****************************************************************/
void sciencereport_msg_close(Widget w)
{
  science_close_callback(w, NULL, NULL);
}


/****************************************************************
...
*****************************************************************/
void science_help_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  XawListReturnStruct *ret=XawListShowCurrent(science_list);
  Boolean b;

  XtVaGetValues(science_help_toggle, XtNstate, &b, NULL);
  if (b == TRUE)
    {
      if(ret->list_index!=XAW_LIST_NONE)
	popup_help_dialog_typed(ret->string, HELP_TECH);
      else
	popup_help_dialog_string(HELP_TECHS_ITEM);
    }
}


/****************************************************************
...
*****************************************************************/
void science_dialog_update(void)
{
  if(is_report_dialogs_frozen()) return;
  if(science_dialog_shell) {
    char text[512];
    static const char *tech_list_names_ptrs[A_LAST + 1];
    int j, flag;
    const char *report_title;
    struct player_research *research = get_player_research(client.conn.playing);
    
    /* TRANS: Research report title */
    report_title = get_report_title_plus(_("Research"), science_dialog_text());
    xaw_set_label(science_label, report_title);
    free((void *) report_title);

    if (research->researching == A_UNSET) {
      my_snprintf(text, sizeof(text),
		  _("Researching %s: %d/%s"),
		  advance_name_translation(advance_by_number(A_NONE)),
		  research->bulbs_researched,
		  _("unknown"));
    } else {
      my_snprintf(text, sizeof(text),
		  _("Researching %s: %d/%d"),
		  advance_name_researching(client.conn.playing),
		  research->bulbs_researched,
		  total_bulbs_required(client.conn.playing));
    }

    xaw_set_label(science_current_label, text);

    if (research->tech_goal == A_UNSET) {
      my_snprintf(text, sizeof(text),
		  _("Goal: %s (%d steps)"),
		  advance_name_translation(advance_by_number(A_NONE)),
		  0);
    } else {
      my_snprintf(text, sizeof(text),
		  _("Goal: %s (%d steps)"),
		  advance_name_translation(advance_by_number(research->tech_goal)),
		  num_unknown_techs_for_goal(client.conn.playing,
					     research->tech_goal));
    }

    xaw_set_label(science_goal_label, text);

    j=0;
    advance_index_iterate(A_FIRST, i) {
      if (TECH_KNOWN == player_invention_state(client.conn.playing, i)) {
	tech_list_names_ptrs[j]=advance_name_translation(advance_by_number(i));
	j++;
      }
    } advance_index_iterate_end;
    tech_list_names_ptrs[j]=0;
    qsort(tech_list_names_ptrs, j, sizeof(char *), compare_strings_ptrs);

    XawListChange(science_list, (char **)tech_list_names_ptrs, 0/*j*/, 0, 1);

    XtDestroyWidget(popupmenu);
    
    popupmenu=XtVaCreatePopupShell("menu", 
				   simpleMenuWidgetClass, 
				   science_change_menu_button, 
				   NULL);
    
    flag=0;
    advance_index_iterate(A_FIRST, i) {
      if (TECH_PREREQS_KNOWN ==
            player_invention_state(client.conn.playing, i)) {
	Widget entry=
	  XtVaCreateManagedWidget(advance_name_translation(advance_by_number(i)),
				  smeBSBObjectClass,
				  popupmenu,
				  NULL);
	XtAddCallback(entry, XtNcallback, science_change_callback, 
		      INT_TO_XTPOINTER(i)); 
	flag=1;
      }
    } advance_index_iterate_end;
    if(!flag)
      XtSetSensitive(science_change_menu_button, FALSE);

    XtDestroyWidget(goalmenu);
    
    goalmenu=XtVaCreatePopupShell("menu", 
				  simpleMenuWidgetClass, 
				  science_goal_menu_button, 
				  NULL);
    
    flag=0;
    advance_index_iterate(A_FIRST, i) {
      if (player_invention_reachable(client.conn.playing, i)
	  && TECH_KNOWN != player_invention_state(client.conn.playing, i)
	  && (11 > num_unknown_techs_for_goal(client.conn.playing, i)
	      || i == research->tech_goal)) {
	Widget entry=
	  XtVaCreateManagedWidget(advance_name_translation(advance_by_number(i)),
				  smeBSBObjectClass,
				  goalmenu,
				  NULL);
	XtAddCallback(entry, XtNcallback, science_goal_callback, 
		      INT_TO_XTPOINTER(i)); 
	flag=1;
      }
    } advance_index_iterate_end;
    
    if(!flag)
      XtSetSensitive(science_goal_menu_button, FALSE);

  }
  
}

/****************************************************************

                      ECONOMY REPORT DIALOG
 
****************************************************************/

/****************************************************************
...
****************************************************************/
void popup_economy_report_dialog(bool make_modal)
{
  if(!economy_dialog_shell) {
      Position x, y;
      Dimension width, height;
      
      economy_dialog_shell_is_modal=make_modal;
    
      if(make_modal)
	XtSetSensitive(main_form, FALSE);
      
      create_economy_report_dialog(make_modal);
      
      XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
      
      XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
			&x, &y);
      XtVaSetValues(economy_dialog_shell, XtNx, x, XtNy, y, NULL);
      
      XtPopup(economy_dialog_shell, XtGrabNone);
   }
}

/****************************************************************
  Closes the economy report.
****************************************************************/
void popdown_economy_report_dialog(void)
{
  if (economy_dialog_shell) {
    if (economy_dialog_shell_is_modal) {
      XtSetSensitive(main_form, TRUE);
    }
    XtDestroyWidget(economy_dialog_shell);
    economy_dialog_shell = 0;
  }
}

/****************************************************************
...
*****************************************************************/
void create_economy_report_dialog(bool make_modal)
{
  Widget economy_form;
  Widget close_command;
  const char *report_title;
  
  economy_dialog_shell =
    I_T(XtVaCreatePopupShell("reporteconomypopup", 
			     (make_modal ? transientShellWidgetClass :
			      topLevelShellWidgetClass),
			     toplevel, NULL));

  economy_form = XtVaCreateManagedWidget("reporteconomyform", 
					 formWidgetClass,
					 economy_dialog_shell,
					 NULL);   

  report_title=get_centered_report_title(_("Economy"));
  economy_label = XtVaCreateManagedWidget("reporteconomylabel", 
				       labelWidgetClass, 
				       economy_form,
				       XtNlabel, report_title,
				       NULL);
  free((void *) report_title);

  economy_list_label =
    I_L(XtVaCreateManagedWidget("reporteconomylistlabel", labelWidgetClass, 
				economy_form, NULL));
  
  economy_list
    = XtVaCreateManagedWidget("reporteconomylist", listWidgetClass,
			      economy_form, NULL);

  economy_label2
    = XtVaCreateManagedWidget("reporteconomylabel2", labelWidgetClass, 
			      economy_form, NULL);

  close_command =
    I_L(XtVaCreateManagedWidget("reporteconomyclosecommand", commandWidgetClass,
				economy_form, NULL));

  sellobsolete_command =
    I_L(XtVaCreateManagedWidget("reporteconomysellobsoletecommand", 
				commandWidgetClass,
				economy_form,
				XtNsensitive, False,
				NULL));

  sellall_command  =
    I_L(XtVaCreateManagedWidget("reporteconomysellallcommand", 
				commandWidgetClass,
				economy_form,
				XtNsensitive, False,
				NULL));
	
  XtAddCallback(economy_list, XtNcallback, economy_list_callback, NULL);
  XtAddCallback(close_command, XtNcallback, economy_close_callback, NULL);
  XtAddCallback(sellobsolete_command, XtNcallback, economy_selloff_callback, (XtPointer)0);
  XtAddCallback(sellall_command, XtNcallback, economy_selloff_callback, (XtPointer)1);
  XtRealizeWidget(economy_dialog_shell);

  if(!make_modal)  {
    XSetWMProtocols(display, XtWindow(economy_dialog_shell), 
		    &wm_delete_window, 1);
    XtOverrideTranslations(economy_dialog_shell,
      XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-economy-report()"));
  }

  economy_report_dialog_update();
}



/****************************************************************
  Called when a building type is selected in the economy list.
*****************************************************************/
void economy_list_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(economy_list);

  if (ret->list_index != XAW_LIST_NONE) {
    /* The user has selected an improvement type. */
    struct impr_type *pimprove = economy_improvement_type[ret->list_index];
    bool is_sellable = can_sell_building(pimprove);

    XtSetSensitive(sellobsolete_command, is_sellable
		   && improvement_obsolete(client.conn.playing, pimprove));
    XtSetSensitive(sellall_command, is_sellable);
  } else {
    /* No selection has been made. */
    XtSetSensitive(sellobsolete_command, FALSE);
    XtSetSensitive(sellall_command, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
void economy_close_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data)
{
  popdown_economy_report_dialog();
}

/****************************************************************
...
*****************************************************************/
void economyreport_msg_close(Widget w)
{
  economy_close_callback(w, NULL, NULL);
}

/****************************************************************
...
*****************************************************************/
void economy_selloff_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  char str[1024];
  XawListReturnStruct *ret = XawListShowCurrent(economy_list);

  if (ret->list_index == XAW_LIST_NONE) {
    return;
  }

  sell_all_improvements(economy_improvement_type[ret->list_index],
			client_data == NULL, str, sizeof(str));
  popup_notify_dialog(_("Sell-Off:"), _("Results"), str);
}

/****************************************************************
...
*****************************************************************/
void economy_report_dialog_update(void)
{
  if(is_report_dialogs_frozen()) return;
  if(economy_dialog_shell) {
    int i, entries_used, tax, total;
    Dimension width; 
    static char *economy_list_names_ptrs[B_LAST+1];
    static char economy_list_names[B_LAST][200];
    const char *report_title;
    char economy_total[48];
    struct improvement_entry entries[B_LAST];
    
    report_title=get_centered_report_title(_("Economy"));
    xaw_set_label(economy_label, report_title);
    free((void *) report_title);

    get_economy_report_data(entries, &entries_used, &total, &tax);

    for (i = 0; i < entries_used; i++) {
      struct improvement_entry *p = &entries[i];

      my_snprintf(economy_list_names[i], sizeof(economy_list_names[i]),
		  "%-20s%5d%5d%6d",
		  improvement_name_translation(p->type),
		  p->count, p->cost, p->total_cost);
      economy_list_names_ptrs[i] = economy_list_names[i];
      economy_improvement_type[i] = p->type;
    }
    
    if (entries_used == 0) {
      sz_strlcpy(economy_list_names[0],
		 "                                          ");
      economy_list_names_ptrs[0] = economy_list_names[0];
      entries_used = 1;
    }
    economy_list_names_ptrs[entries_used] = NULL;

    my_snprintf(economy_total, sizeof(economy_total),
		_("Income:%6d    Total Costs: %6d"), tax, total); 
    xaw_set_label(economy_label2, economy_total); 
    
    XawListChange(economy_list, economy_list_names_ptrs, 0, 0, 1);

    XtVaGetValues(economy_list, XtNwidth, &width, NULL);
    XtVaSetValues(economy_list_label, XtNwidth, width, NULL); 

    XtVaSetValues(economy_label2, XtNwidth, width, NULL); 

    XtVaSetValues(economy_label, XtNwidth, width, NULL); 

  }
  
}

/****************************************************************

                      ACTIVE UNITS REPORT DIALOG
 
****************************************************************/

/****************************************************************
...
****************************************************************/
void popup_activeunits_report_dialog(bool make_modal)
{
  if(!activeunits_dialog_shell) {
      Position x, y;
      Dimension width, height;
      
      activeunits_dialog_shell_is_modal=make_modal;
    
      if(make_modal)
	XtSetSensitive(main_form, FALSE);
      
      create_activeunits_report_dialog(make_modal);
      
      XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
      
      XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
			&x, &y);
      XtVaSetValues(activeunits_dialog_shell, XtNx, x, XtNy, y, NULL);
      
      XtPopup(activeunits_dialog_shell, XtGrabNone);
   }
}

/****************************************************************
  Closes the active units report.
*****************************************************************/
void popdown_activeunits_report_dialog(void)
{
  if (activeunits_dialog_shell) {
    if (activeunits_dialog_shell_is_modal) {
      XtSetSensitive(main_form, TRUE);
    }
    XtDestroyWidget(activeunits_dialog_shell);
    activeunits_dialog_shell = 0;
  }
}

/****************************************************************
...
*****************************************************************/
void create_activeunits_report_dialog(bool make_modal)
{
  Widget activeunits_form;
  Widget close_command, refresh_command;
  const char *report_title;
  
  activeunits_dialog_shell =
    I_T(XtVaCreatePopupShell("reportactiveunitspopup", 
			     (make_modal ? transientShellWidgetClass :
			      topLevelShellWidgetClass),
			     toplevel, NULL));

  activeunits_form = XtVaCreateManagedWidget("reportactiveunitsform", 
					 formWidgetClass,
					 activeunits_dialog_shell,
					 NULL);   

  report_title=get_centered_report_title(_("Units"));
  activeunits_label = XtVaCreateManagedWidget("reportactiveunitslabel", 
				       labelWidgetClass, 
				       activeunits_form,
				       XtNlabel, report_title,
				       NULL);
  free((void *) report_title);

  activeunits_list_label =
    I_L(XtVaCreateManagedWidget("reportactiveunitslistlabel", 
				labelWidgetClass, 
				activeunits_form,
				NULL));
  
  activeunits_list = XtVaCreateManagedWidget("reportactiveunitslist", 
				      listWidgetClass,
				      activeunits_form,
				      NULL);

  activeunits_label2 = XtVaCreateManagedWidget("reportactiveunitslabel2", 
				       labelWidgetClass, 
				       activeunits_form,
                                       XtNlabel, 
				       _("Totals: ..."), 
				       NULL);

  close_command =
    I_L(XtVaCreateManagedWidget("reportactiveunitsclosecommand", 
				commandWidgetClass,
				activeunits_form,
				NULL));

  upgrade_command =
    I_L(XtVaCreateManagedWidget("reportactiveunitsupgradecommand", 
				commandWidgetClass,
				activeunits_form,
				XtNsensitive, False,
				NULL));

  refresh_command =
    I_L(XtVaCreateManagedWidget("reportactiveunitsrefreshcommand", 
				commandWidgetClass,
				activeunits_form,
				NULL));

  XtAddCallback(activeunits_list, XtNcallback, activeunits_list_callback, NULL);
  XtAddCallback(close_command, XtNcallback, activeunits_close_callback, NULL);
  XtAddCallback(upgrade_command, XtNcallback, activeunits_upgrade_callback, NULL);
  XtAddCallback(refresh_command, XtNcallback, activeunits_refresh_callback, NULL);

  XtRealizeWidget(activeunits_dialog_shell);

  if(!make_modal)  {
    XSetWMProtocols(display, XtWindow(activeunits_dialog_shell), 
		    &wm_delete_window, 1);
    XtOverrideTranslations(activeunits_dialog_shell,
      XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-units-report()"));
  }

  activeunits_report_dialog_update();
}

/****************************************************************
...
*****************************************************************/
void activeunits_list_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data)
{
  XawListReturnStruct *ret;
  int inx;
  int may_upgrade;

  ret = XawListShowCurrent (activeunits_list);
  inx = ret->list_index;

  may_upgrade =
    ((inx != XAW_LIST_NONE) &&
     (can_upgrade_unittype(client.conn.playing,
       utype_by_number(activeunits_type[inx])) != NULL));

  XtSetSensitive (upgrade_command, may_upgrade);
}

/****************************************************************
...
*****************************************************************/
static void upgrade_callback_yes(Widget w, XtPointer client_data, 
                                 XtPointer call_data)
{
  dsend_packet_unit_type_upgrade(&client.conn, (size_t) client_data);
  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void upgrade_callback_no(Widget w, XtPointer client_data, 
                                XtPointer call_data)
{
  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
void activeunits_upgrade_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data)
{
  char buf[512];
  struct unit_type *punittype1, *punittype2;

  XawListReturnStruct *ret;
  ret=XawListShowCurrent(activeunits_list);

  if(ret->list_index!=XAW_LIST_NONE) {
    punittype1 = utype_by_number(activeunits_type[ret->list_index]);
    CHECK_UNIT_TYPE(punittype1);

    punittype2 = can_upgrade_unittype(client.conn.playing, punittype1);

    my_snprintf(buf, sizeof(buf),
		_("Upgrade as many %s to %s as possible for %d gold each?\n"
		  "Treasury contains %d gold."),
		utype_name_translation(punittype1),
		utype_name_translation(punittype2),
		unit_upgrade_price(client.conn.playing, punittype1, punittype2),
		client.conn.playing->economic.gold);

    popup_message_dialog(toplevel, "upgradedialog", buf,
			 upgrade_callback_yes,
			 INT_TO_XTPOINTER(activeunits_type[ret->list_index]),
			 0, upgrade_callback_no, 0, 0, NULL);
  }
}

/****************************************************************
...
*****************************************************************/
void activeunits_close_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data)
{
  popdown_activeunits_report_dialog();
}

/****************************************************************
...
*****************************************************************/
void activeunits_refresh_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  activeunits_report_dialog_update();
}

/****************************************************************
...
*****************************************************************/
void activeunits_msg_close(Widget w)
{
  activeunits_close_callback(w, NULL, NULL);
}

/****************************************************************
...
*****************************************************************/
void activeunits_report_dialog_update(void)
{
  struct repoinfo {
    int active_count;
    int upkeep[O_LAST];
    int building_count;
  };

  if (NULL == client.conn.playing) {
    return;
  }

  if (is_report_dialogs_frozen()) {
    return;
  }

  if (activeunits_dialog_shell) {
    int k;
    Dimension width; 
    static char *activeunits_list_names_ptrs[U_LAST+1];
    static char activeunits_list_names[U_LAST][200];
    struct repoinfo unitarray[U_LAST];
    struct repoinfo unittotals;
    const char *report_title;
    char activeunits_total[100];
    Unit_type_id i;
    
    report_title = get_centered_report_title(_("Units"));
    xaw_set_label(activeunits_label, report_title);
    free((void *) report_title);

    memset(unitarray, '\0', sizeof(unitarray));

    city_list_iterate(client.conn.playing->cities, pcity) {
      unit_list_iterate(client.conn.playing->units, punit) {
        Unit_type_id uti = utype_index(unit_type(punit));

        (unitarray[uti].active_count)++;
        if (punit->homecity) {
          output_type_iterate(o) {
            unitarray[uti].upkeep[o] += punit->upkeep[o];
          } output_type_iterate_end;
        }
      } unit_list_iterate_end;
    } city_list_iterate_end;

    city_list_iterate(client.conn.playing->cities,pcity) {
      if (VUT_UTYPE == pcity->production.kind) {
	struct unit_type *punittype = pcity->production.value.utype;
	(unitarray[utype_index(punittype)].building_count)++;
      }
    }
    city_list_iterate_end;

    k = 0;
    memset(&unittotals, '\0', sizeof(unittotals));
    unit_type_iterate(punittype) {
      i = utype_index(punittype);
      if ((unitarray[i].active_count > 0) || (unitarray[i].building_count > 0)) {
	my_snprintf
	  (
	   activeunits_list_names[k],
	   sizeof(activeunits_list_names[k]),
	   "%-27s%c%9d%9d%9d%9d",
	   utype_name_translation(punittype),
	   can_upgrade_unittype(client.conn.playing, punittype) != NULL ? '*': '-',
	   unitarray[i].building_count,
	   unitarray[i].active_count,
           unitarray[i].upkeep[O_SHIELD],
           unitarray[i].upkeep[O_FOOD]
           /* TODO: add upkeep[O_GOLD] here */
	  );
	activeunits_list_names_ptrs[k]=activeunits_list_names[k];
	activeunits_type[k]=(unitarray[i].active_count > 0) ? i : U_LAST;
	k++;
	unittotals.active_count += unitarray[i].active_count;
        output_type_iterate(o) {
          unittotals.upkeep[o] = unitarray[i].upkeep[o];
        } output_type_iterate_end;
	unittotals.building_count += unitarray[i].building_count;
      }
    } unit_type_iterate_end;

    if (k==0) {
      sz_strlcpy(activeunits_list_names[0], "                                ");
      activeunits_list_names_ptrs[0]=activeunits_list_names[0];
      k=1;
    }

    my_snprintf(activeunits_total, sizeof(activeunits_total),
	    _("Totals:                     %9d%9d%9d%9d"),
	    unittotals.building_count, unittotals.active_count,
           unittotals.upkeep[O_SHIELD], unittotals.upkeep[O_FOOD]);
            /* TODO: add upkeep[O_GOLD] here */
    xaw_set_label(activeunits_label2, activeunits_total); 

    activeunits_list_names_ptrs[k]=0;
    XawListChange(activeunits_list, activeunits_list_names_ptrs, 0, 0, 1);

    XtVaGetValues(activeunits_list, XtNwidth, &width, NULL);
    XtVaSetValues(activeunits_list_label, XtNwidth, width, NULL); 
    XtVaSetValues(activeunits_label2, XtNwidth, width, NULL); 
    XtVaSetValues(activeunits_label, XtNwidth, width, NULL); 

    activeunits_list_callback(NULL, NULL, NULL);
  }
}

/****************************************************************
  Show a dialog with player statistics at endgame.
  TODO: Display all statistics in packet_endgame_report.
*****************************************************************/
void popup_endgame_report_dialog(struct packet_endgame_report *packet)
{
  char buffer[150 * MAX_NUM_PLAYERS];
  int i;
 
  buffer[0] = '\0';
  for (i = 0; i < packet->nscores; i++) {
    cat_snprintf(buffer, sizeof(buffer),
                 PL_("%2d: The %s ruler %s scored %d point\n",
                     "%2d: The %s ruler %s scored %d points\n",
                     packet->score[i]),
                 i + 1,
                 nation_adjective_for_player(player_by_number(packet->id[i])),
                 player_name(player_by_number(packet->id[i])),
                 packet->score[i]);
  }
  popup_notify_dialog(_("Final Report:"),
                      _("The Greatest Civilizations in the world."),
                      buffer);
}

/****************************************************************************
			SERVER OPTIONS DIALOG
****************************************************************************/

/****************************************************************************
  Show a dialog with the server options.
****************************************************************************/
void popup_settable_options_dialog(void)
{
  if (!settable_options_dialog_shell) {
    create_settable_options_dialog();
  }

  update_settable_options_dialog();

  XtPopup(settable_options_dialog_shell, XtGrabNone);
}

/****************************************************************************
  Popdown server options dialog.
****************************************************************************/
void popdown_settable_options_dialog(void)
{
  if (settable_options_dialog_shell) {
    XtDestroyWidget(settable_options_dialog_shell);
    settable_options_dialog_shell = NULL;
  }
}

/****************************************************************************
  Server options dialog.
****************************************************************************/
void create_settable_options_dialog(void)
{
  Widget prev_widget, longest_label = NULL;
  size_t longest_len = 0;
  Dimension width;
  int i;

  settable_options_dialog_shell =
    I_T(XtCreatePopupShell("settableoptionspopup", transientShellWidgetClass,
			   toplevel, NULL, 0));
  settable_options_form =
    XtVaCreateManagedWidget("settableoptionsform", formWidgetClass,
			    settable_options_dialog_shell, NULL);

  settable_options_label =
    I_L(XtVaCreateManagedWidget("settableoptionslabel", labelWidgetClass,
				settable_options_form, NULL));

  settable_options_viewport =
    XtVaCreateManagedWidget("settableoptionsviewport", viewportWidgetClass,
			    settable_options_form, NULL);
  settable_options_scrollform =
    XtVaCreateManagedWidget("settableoptionsscrollform", formWidgetClass, 
			    settable_options_viewport, NULL);

  if (!settable_options_widgets) {
    settable_options_widgets = fc_calloc(num_settable_options, sizeof(Widget));
  }

  prev_widget = NULL; /* init the prev-Widget */

  for (i = 0; i < num_settable_options; i++) {
    char buf[256];
    size_t len;
    struct options_settable *o = &settable_options[i];

    my_snprintf(buf, sizeof(buf), "%s: %s", o->name,
		_(o->short_help));
    len = strlen(buf);

    /* 
     * Remember widget so we can reset the vertical position.
     */
    settable_options_widgets[i] = prev_widget;

    if (prev_widget) {
      prev_widget =
	XtVaCreateManagedWidget("label", labelWidgetClass,
				settable_options_scrollform,
				XtNlabel, buf,
				XtNfromVert, prev_widget,
				NULL);
    } else {
      prev_widget =
	XtVaCreateManagedWidget("label", labelWidgetClass,
				settable_options_scrollform,
				XtNlabel, buf,
				NULL);
    }

    if (len > longest_len) {
      longest_len = len;
      longest_label = prev_widget;
    }

    /* 
     * The addition of a scrollbar screws things up. There must be a
     * better way to do this.
     */
    XtVaGetValues(prev_widget, XtNwidth, &width, NULL);
    XtVaSetValues(prev_widget, XtNwidth, width + 15, NULL);
  }

  XtVaGetValues(longest_label, XtNwidth, &width, NULL);
  XtVaSetValues(settable_options_label, XtNwidth, width + 15, NULL);

  for (i = 0; i < num_settable_options; i++) {
    struct options_settable *o = &settable_options[i];

    if (setting_class_is_changeable(o->sclass)
	&& o->is_visible) {
      switch (o->stype) {
      case SSET_BOOL:
	if (settable_options_widgets[i]) {
	  prev_widget =
	    XtVaCreateManagedWidget("toggle", toggleWidgetClass,
				    settable_options_scrollform,
				    XtNfromHoriz, settable_options_label,
				    XtNfromVert, settable_options_widgets[i],
				    NULL);
	} else {
	  prev_widget =
	    XtVaCreateManagedWidget("toggle", toggleWidgetClass,
				    settable_options_scrollform,
				    XtNfromHoriz, settable_options_label,
				    NULL);
	}
	XtAddCallback(prev_widget, XtNcallback,
		      settable_options_toggle_callback, NULL);
	break;
      case SSET_INT:
      case SSET_STRING:
	if (settable_options_widgets[i]) {
	  prev_widget =
	    XtVaCreateManagedWidget("input", asciiTextWidgetClass,
				    settable_options_scrollform,
				    XtNfromHoriz, settable_options_label,
				    XtNfromVert, settable_options_widgets[i],
				    NULL);
	} else {
	  prev_widget =
	    XtVaCreateManagedWidget("input", asciiTextWidgetClass,
				    settable_options_scrollform,
				    XtNfromHoriz, settable_options_label,
				    NULL);
	}
	break;
      }

    } else {
      if (settable_options_widgets[i]) {
	prev_widget =
	  XtVaCreateManagedWidget("rlabel", labelWidgetClass,
				  settable_options_scrollform,
				  XtNfromHoriz, settable_options_label,
				  XtNfromVert, settable_options_widgets[i],
				  NULL);
      } else {
	prev_widget =
	  XtVaCreateManagedWidget("rlabel", labelWidgetClass,
				  settable_options_scrollform,
				  XtNfromHoriz, settable_options_label,
				  NULL);
      }
    }
    /* store the final widget */
    settable_options_widgets[i] = prev_widget;
  }

  settable_options_ok_command =
    I_L(XtVaCreateManagedWidget("settableoptionsokcommand",
				commandWidgetClass,
				settable_options_form,
				NULL));

  settable_options_cancel_command =
    I_L(XtVaCreateManagedWidget("settableoptionscancelcommand",
				commandWidgetClass,
				settable_options_form,
				NULL));

  XtAddCallback(settable_options_ok_command, XtNcallback,
		settable_options_ok_callback, NULL);
  XtAddCallback(settable_options_cancel_command, XtNcallback,
		settable_options_cancel_callback, NULL);

  XtRealizeWidget(settable_options_dialog_shell);

  XSetWMProtocols(display, XtWindow(settable_options_dialog_shell),
		  &wm_delete_window, 1);
  XtOverrideTranslations(settable_options_dialog_shell,
    XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-settable-options()"));

  xaw_horiz_center(settable_options_label);
}

/****************************************************************************
  Update server options dialog.
****************************************************************************/
void update_settable_options_dialog(void)
{
  if (settable_options_dialog_shell) {
    char buf[256];
    int i;

    for (i = 0; i < num_settable_options; i++) {
      struct options_settable *o = &settable_options[i];

      if (setting_class_is_changeable(o->sclass)
	  && o->is_visible) {
	switch (o->stype) {
	case SSET_BOOL:
	  XtVaSetValues(settable_options_widgets[i],
			XtNstate, o->val ? True : False,
			XtNlabel,
			o->val ? _("Yes") : _("No"), NULL);
	  break;
	case SSET_INT:
	  my_snprintf(buf, sizeof(buf), "%d", o->val);
	  XtVaSetValues(settable_options_widgets[i], XtNstring, buf, NULL);
	  break;
	case SSET_STRING:
	  my_snprintf(buf, sizeof(buf), "%s", o->strval);
	  XtVaSetValues(settable_options_widgets[i], XtNstring, buf, NULL);
	  break;
	}
      } else {
	if (o->is_visible) {
	  switch (o->stype) {
	  case SSET_BOOL:
	    my_snprintf(buf, sizeof(buf), "%s",
	      o->val != 0 ? _("true") : _("false"));
	    break;
	  case SSET_INT:
	    my_snprintf(buf, sizeof(buf), "%d", o->val);
	    break;
	  case SSET_STRING:
	    my_snprintf(buf, sizeof(buf), "%s", o->strval);
	    break;
	  }
	} else {
	  my_snprintf(buf, sizeof(buf), _("(hidden)"));
	}
	XtVaSetValues(settable_options_widgets[i], XtNlabel, buf, NULL);
      }
    }
  }
}

/****************************************************************************
  Changes the label of the toggle widget to Yes/No depending on the state of
  the toggle.
****************************************************************************/
void settable_options_toggle_callback(Widget w, XtPointer client_data,
				      XtPointer call_data)
{
  Boolean b;

  XtVaGetValues(w, XtNstate, &b, NULL);
  XtVaSetValues(w, XtNlabel, b ? _("Yes") : _("No"), NULL);
}

/****************************************************************************
  OK button callback.
****************************************************************************/
void settable_options_ok_callback(Widget w, XtPointer client_data,
				  XtPointer call_data)
{
  if (settable_options_dialog_shell) {
    Boolean b, old_b;
    int val, i;
    XtPointer dp;

    for (i = 0; i < num_settable_options; i++) {
      struct options_settable *o = &settable_options[i];

      if (setting_class_is_changeable(o->sclass)
	  && o->is_visible) {

	switch (o->stype) {
	case SSET_BOOL:
	  old_b = o->val ? True: False;
	  XtVaGetValues(settable_options_widgets[i], XtNstate, &b, NULL);
	  if (b != old_b) {
	    send_chat_printf("/set %s %s",
                             o->name, b ? "1" : "0");
	  }
	  break;
	case SSET_INT:
	  XtVaGetValues(settable_options_widgets[i], XtNstring, &dp, NULL);
	  sscanf(dp, "%d", &val);
	  if (val != o->val) {
	    send_chat_printf("/set %s %d",
                             o->name, val);
	  }
	  break;
	case SSET_STRING:
	  XtVaGetValues(settable_options_widgets[i], XtNstring, &dp, NULL);
	  if (strcmp(o->strval, dp)) {
	    send_chat_printf("/set %s %s",
                             o->name, (char *)dp);
	  }
	  break;
	}
      }
    }

    popdown_settable_options_dialog();
  }
}

/****************************************************************************
  Cancel button callback.
****************************************************************************/
void settable_options_cancel_callback(Widget w, XtPointer client_data,
				      XtPointer call_data)
{
  popdown_settable_options_dialog();
}

/****************************************************************************
  Callback for overrided wm_delete_window of server options dialog.
****************************************************************************/
void settable_options_msg_close(Widget w)
{
  popdown_settable_options_dialog();
}
