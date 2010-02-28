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
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/List.h>

/* common & utility */
#include "diptreaty.h"
#include "fcintl.h"
#include "packets.h"
#include "player.h"
#include "support.h"

/* client */
#include "chatline.h"
#include "client_main.h"
#include "climisc.h"
#include "diplodlg.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "inteldlg.h"
#include "spaceshipdlg.h"

#include "plrdlg.h"

static Widget players_dialog_shell;
static Widget players_form;
static Widget players_label;
static Widget players_list;
static Widget players_close_command;
static Widget players_int_command;
static Widget players_meet_command;
static Widget players_war_command;
static Widget players_vision_command;
static Widget players_sship_command;
static bool players_dialog_shell_is_raised;

static int list_index_to_player_index[MAX_NUM_PLAYERS];


static void create_players_dialog(bool raise);
static void players_close_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
static void players_meet_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
static void players_intel_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
static void players_list_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
static void players_war_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data);
static void players_vision_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data);
static void players_sship_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);


/******************************************************************/

/****************************************************************
popup the dialog somewhat inside the main-window 
*****************************************************************/
void popup_players_dialog(bool raise)
{
  players_dialog_shell_is_raised = raise;

  if(!players_dialog_shell)
    create_players_dialog(raise);

  if (raise) {
    XtSetSensitive(main_form, FALSE);
  }

  xaw_set_relative_position(toplevel, players_dialog_shell, 5, 25);
  XtPopup(players_dialog_shell, XtGrabNone);
}

/****************************************************************
  Closes the player list dialog.
*****************************************************************/
void popdown_players_dialog(void)
{
  if (players_dialog_shell) {
    if (players_dialog_shell_is_raised) {
      XtSetSensitive(main_form, TRUE);
    }
    XtDestroyWidget(players_dialog_shell);
    players_dialog_shell = 0;
  }
}

/****************************************************************
...
*****************************************************************/
void create_players_dialog(bool raise)
{
  players_dialog_shell =
    I_IN(I_T(XtCreatePopupShell("playerspopup", 
				raise ? transientShellWidgetClass
				: topLevelShellWidgetClass,
				toplevel, NULL, 0)));

  players_form = XtVaCreateManagedWidget("playersform", 
				       formWidgetClass, 
				       players_dialog_shell, NULL);

  players_label = I_L(XtVaCreateManagedWidget("playerslabel", 
					labelWidgetClass, 
					players_form, NULL));   

   
  players_list = XtVaCreateManagedWidget("playerslist", 
					 listWidgetClass, 
					 players_form, 
					 NULL);

  players_close_command =
    I_L(XtVaCreateManagedWidget("playersclosecommand", commandWidgetClass,
				players_form, NULL));

  players_int_command =
    I_L(XtVaCreateManagedWidget("playersintcommand", commandWidgetClass,
				players_form,
				XtNsensitive, False,
				NULL));

  players_meet_command =
    I_L(XtVaCreateManagedWidget("playersmeetcommand", commandWidgetClass,
				players_form,
				XtNsensitive, False,
				NULL));

  players_war_command =
    I_L(XtVaCreateManagedWidget("playerswarcommand", commandWidgetClass,
				players_form,
				XtNsensitive, False,
				NULL));

  players_vision_command =
    I_L(XtVaCreateManagedWidget("playersvisioncommand", commandWidgetClass,
				players_form,
				XtNsensitive, False,
				NULL));

  players_sship_command =
    I_L(XtVaCreateManagedWidget("playerssshipcommand", commandWidgetClass,
				players_form,
				XtNsensitive, False,
				NULL));

  XtAddCallback(players_list, XtNcallback, players_list_callback, 
		NULL);
  
  XtAddCallback(players_close_command, XtNcallback, players_close_callback, 
		NULL);

  XtAddCallback(players_meet_command, XtNcallback, players_meet_callback, 
		NULL);
  
  XtAddCallback(players_int_command, XtNcallback, players_intel_callback, 
		NULL);

  XtAddCallback(players_war_command, XtNcallback, players_war_callback, 
		NULL);

  XtAddCallback(players_vision_command, XtNcallback, players_vision_callback, 
		NULL);

  XtAddCallback(players_sship_command, XtNcallback, players_sship_callback, 
		NULL);
  
  update_players_dialog();

  XtRealizeWidget(players_dialog_shell);
  
  XSetWMProtocols(display, XtWindow(players_dialog_shell), 
		  &wm_delete_window, 1);
  XtOverrideTranslations(players_dialog_shell,
    XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-players()"));
}


/**************************************************************************
  Updates the player list dialog.

  FIXME: use plrdlg_common.c
**************************************************************************/
void update_players_dialog(void)
{
   if(players_dialog_shell && !is_plrdlg_frozen()) {
    int j = 0;
    Dimension width;
    static char *namelist_ptrs[MAX_NUM_PLAYERS];
    static char namelist_text[MAX_NUM_PLAYERS][256];
    const struct player_diplstate *pds;

    players_iterate(pplayer) {
      char idlebuf[32], statebuf[32], namebuf[32], dsbuf[32];
      
      /* skip barbarians */
      if(is_barbarian(pplayer))
        continue;

      /* text for state */
      sz_strlcpy(statebuf, plrdlg_col_state(pplayer));

      /* text for idleness */
      if(pplayer->nturns_idle>3) {
	my_snprintf(idlebuf, sizeof(idlebuf),
		    PL_("(idle %d turn)",
			"(idle %d turns)",
			pplayer->nturns_idle - 1),
		    pplayer->nturns_idle - 1);
      } else {
	idlebuf[0]='\0';
      }

      /* text for name, plus AI marker */       
      if (pplayer->ai_data.control) {
        my_snprintf(namebuf, sizeof(namebuf), "*%-15s",player_name(pplayer));
      } else {
        my_snprintf(namebuf, sizeof(namebuf), "%-16s",player_name(pplayer));
      }
      namebuf[16] = '\0';

      /* text for diplstate type and turns -- not applicable if this is me */
      if (NULL == client.conn.playing
          || pplayer == client.conn.playing) {
	strcpy(dsbuf, "-");
      } else {
	pds = pplayer_get_diplstate(client.conn.playing, pplayer);
	if (pds->type == DS_CEASEFIRE) {
	  my_snprintf(dsbuf, sizeof(dsbuf), "%s (%d)",
		      diplstate_text(pds->type), pds->turns_left);
	} else {
	  my_snprintf(dsbuf, sizeof(dsbuf), "%s",
		      diplstate_text(pds->type));
	}
      }

      /* assemble the whole lot */
      my_snprintf(namelist_text[j], sizeof(namelist_text[j]),
	      "%-16s %-12s %-8s %-15s %-8s %-6s   %-15s%s", 
	      namebuf,
	      nation_adjective_for_player(pplayer), 
	      get_embassy_status(client.conn.playing, pplayer),
	      dsbuf,
	      get_vision_status(client.conn.playing, pplayer),
	      statebuf,
	      player_addr_hack(pplayer),  /* Fixme for multi-conn */
	      idlebuf);

      namelist_ptrs[j]=namelist_text[j];
      list_index_to_player_index[j] = player_number(pplayer);
      j++;
    } players_iterate_end;
    
    XawListChange(players_list, namelist_ptrs, j, 0, True);

    XtVaGetValues(players_list, XtNwidth, &width, NULL);
    XtVaSetValues(players_label, XtNwidth, width, NULL); 
  }
}

/**************************************************************************
...
**************************************************************************/
void players_list_callback(Widget w, XtPointer client_data, 
			   XtPointer call_data)

{
  XawListReturnStruct *ret;

  ret = XawListShowCurrent(players_list);

  XtSetSensitive(players_meet_command, FALSE);
  XtSetSensitive(players_int_command, FALSE);
  if (ret->list_index != XAW_LIST_NONE) {
    struct player *pplayer = 
      player_by_number(list_index_to_player_index[ret->list_index]);

    if (pplayer->spaceship.state != SSHIP_NONE)
      XtSetSensitive(players_sship_command, TRUE);
    else
      XtSetSensitive(players_sship_command, FALSE);

    if (NULL != client.conn.playing && pplayer->is_alive) {
      XtSetSensitive(players_war_command,
		     client.conn.playing != pplayer
		     && !pplayers_at_war(client.conn.playing, pplayer));
    }

    if (NULL != client.conn.playing) {
      XtSetSensitive(players_vision_command,
		     gives_shared_vision(client.conn.playing, pplayer));

      XtSetSensitive(players_meet_command, can_meet_with_player(pplayer));
    }
    XtSetSensitive(players_int_command, can_intel_with_player(pplayer));
  }
}


/**************************************************************************
...
**************************************************************************/
void players_close_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  popdown_players_dialog();
}

/****************************************************************
...
*****************************************************************/
void plrdlg_msg_close(Widget w)
{
  players_close_callback(w, NULL, NULL);
}

/**************************************************************************
...
**************************************************************************/
void players_meet_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(players_list);

  if (ret->list_index != XAW_LIST_NONE) {
    int player_index = list_index_to_player_index[ret->list_index];
    struct player *pplayer = player_by_number(player_index);

    if (can_meet_with_player(pplayer)) {
      dsend_packet_diplomacy_init_meeting_req(&client.conn, player_index);
    } else {
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("You need an embassy to establish"
                             " a diplomatic meeting."));
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void players_intel_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(players_list);

  if (ret->list_index != XAW_LIST_NONE) {
    int player_index = list_index_to_player_index[ret->list_index];
    struct player *pplayer = player_by_number(player_index);

    if (can_intel_with_player(pplayer)) {
      popup_intel_dialog(pplayer);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void players_war_callback(Widget w, XtPointer client_data, 
                          XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(players_list);

  if (ret->list_index != XAW_LIST_NONE) {
    int player_index = list_index_to_player_index[ret->list_index];

    /* can be any pact clause */
    dsend_packet_diplomacy_cancel_pact(&client.conn, player_index,
				       CLAUSE_CEASEFIRE);
  }
}

/**************************************************************************
...
**************************************************************************/
void players_vision_callback(Widget w, XtPointer client_data, 
                          XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(players_list);

  if (ret->list_index != XAW_LIST_NONE) {
    int player_index = list_index_to_player_index[ret->list_index];

    dsend_packet_diplomacy_cancel_pact(&client.conn, player_index,
				       CLAUSE_VISION);
  }
}

/**************************************************************************
...
**************************************************************************/
void players_sship_callback(Widget w, XtPointer client_data,
			    XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(players_list);

  if (ret->list_index != XAW_LIST_NONE) {
    int player_index = list_index_to_player_index[ret->list_index];
    struct player *pplayer = player_by_number(player_index);

    popup_spaceship_dialog(pplayer);
  }
}
