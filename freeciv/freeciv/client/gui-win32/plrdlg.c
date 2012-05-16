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

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

/* common & utility */
#include "diptreaty.h"
#include "fcintl.h"
#include "packets.h"
#include "nation.h"
#include "player.h"
#include "support.h"

/* client */
#include "chatline.h"
#include "client_main.h"
#include "climisc.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "inteldlg.h"
#include "spaceshipdlg.h"
#include "colors.h"
#include "graphics.h"
#include "log.h"

#include "plrdlg.h"


static HWND players_dialog;
static int sort_dir=1;
static int sort_column=2;

#define ID_PLAYERS_LIST 101
#define ID_PLAYERS_INT 102
#define ID_PLAYERS_MEET 103
#define ID_PLAYERS_WAR 104
#define ID_PLAYERS_VISION 105
#define ID_PLAYERS_SSHIP 106

#define NUM_COLUMNS 9

/******************************************************************

*******************************************************************/
static void players_meet(int player_index)
{
  if (can_meet_with_player(player_by_number(player_index))) {
    dsend_packet_diplomacy_init_meeting_req(&client.conn, player_index);

  } else {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("You need an embassy to "
                           "establish a diplomatic meeting."));
  }
}

/******************************************************************

*******************************************************************/
static void players_war(int player_index)
{
  dsend_packet_diplomacy_cancel_pact(&client.conn, player_index,
				     CLAUSE_CEASEFIRE);
}

/******************************************************************

*******************************************************************/
static void players_vision(int player_index)
{
  dsend_packet_diplomacy_cancel_pact(&client.conn, player_index,
				     CLAUSE_VISION);
}

/******************************************************************

*******************************************************************/
static void players_intel(int player_index)
{
  struct player *pplayer = player_by_number(player_index);

  if (can_intel_with_player(pplayer)) {
    popup_intel_dialog(pplayer);
  } 
}

/******************************************************************

*******************************************************************/
static void players_sship(int player_index)
{
  struct player *pplayer = player_by_number(player_index);

  popup_spaceship_dialog(pplayer);
}

/******************************************************************
 * Builds the text for the cells of a row in the player report. If
 * update is 1, only the changable entries are build.

  FIXME: use plrdlg_common.c
*******************************************************************/
static void build_row(const char **row, int player_index, int update)
{
  static char namebuf[MAX_LEN_NAME],  aibuf[2], dsbuf[32],
      statebuf[32], idlebuf[32];
  const struct player_diplstate *pds;
  struct player *pplayer = player_by_number(player_index);

  /* we cassume that neither name nor the nation of a player changes */
  if (update == 0) {
    /* the playername */
    my_snprintf(namebuf, sizeof(namebuf), "%-16s", player_name(pplayer));
    row[0] = namebuf;


    /* the nation */
    row[1] = nation_adjective_for_player(pplayer);
  }

  /* text for name, plus AI marker */
  aibuf[0] = (pplayer->ai_data.control ? '*' : '\0');
  aibuf[1] = '\0';

  /* text for diplstate type and turns -- not applicable if this is me */
  if (pplayer == client.conn.playing) {
    strcpy(dsbuf, "-");
  } else {
    pds = pplayer_get_diplstate(client.conn.playing, pplayer);
    if (pds->type == DS_CEASEFIRE) {
      my_snprintf(dsbuf, sizeof(dsbuf), "%s (%d)",
		  diplstate_text(pds->type), pds->turns_left);
    } else {
      my_snprintf(dsbuf, sizeof(dsbuf), "%s", diplstate_text(pds->type));
    }
  }

  /* text for state */
  sz_strlcpy(statebuf, plrdlg_col_state(pplayer));

  /* text for idleness */
  if (pplayer->nturns_idle > 3) {
    my_snprintf(idlebuf, sizeof(idlebuf),
		PL_("(idle %d turn)",
		    "(idle %d turns)",
		    pplayer->nturns_idle - 1),
		pplayer->nturns_idle - 1);
  } else {
    idlebuf[0] = '\0';
  }

  /* assemble the whole lot */
  row[2] = aibuf;
  row[3] = get_embassy_status(client.conn.playing, pplayer);
  row[4] = dsbuf;
  row[5] = get_vision_status(client.conn.playing, pplayer);
  row[6] = statebuf;
  row[7] = (char *) player_addr_hack(pplayer);	/* Fixme */
  row[8] = idlebuf;
}



/******************************************************************

*******************************************************************/
static int CALLBACK sort_proc(LPARAM lParam1, LPARAM lParam2,
			      LPARAM lParamSort)
{
  char text1[128];
  char text2[128];
  const char *row_texts[NUM_COLUMNS];
  build_row(row_texts,lParam1,0);
  sz_strlcpy(text1,row_texts[lParamSort]);
  build_row(row_texts,lParam2,0);
  sz_strlcpy(text2,row_texts[lParamSort]);
  return sort_dir*mystrcasecmp(text1,text2);
}

/******************************************************************

 ******************************************************************/
static void enable_buttons(int player_index)
{
  struct player *pplayer = player_by_number(player_index);

  if (pplayer->spaceship.state!=SSHIP_NONE)
    EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_SSHIP),
		 TRUE);
  else
    EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_SSHIP),
		 FALSE);
  switch (pplayer_get_diplstate(client.conn.playing,
                                player_by_number(player_index))->type) {
  case DS_WAR:
  case DS_NO_CONTACT:
    EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_WAR), FALSE);
    break;
  default:
    EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_WAR),TRUE);
  }
  
  EnableWindow(GetDlgItem(players_dialog, ID_PLAYERS_VISION),
	       gives_shared_vision(client.conn.playing, pplayer));

  EnableWindow(GetDlgItem(players_dialog, ID_PLAYERS_MEET),
               can_meet_with_player(pplayer));
  EnableWindow(GetDlgItem(players_dialog, ID_PLAYERS_INT),
               can_intel_with_player(pplayer));
}

/******************************************************************

*******************************************************************/
static LONG CALLBACK players_proc(HWND dlg,UINT message,
				  WPARAM wParam,LPARAM lParam)
{
  int sel,i,n;
  int player_index;
  LV_ITEM lvi;

  sel=-1;
  if ((message==WM_COMMAND) || (message==WM_NOTIFY)) {
    n=ListView_GetItemCount(GetDlgItem(dlg,ID_PLAYERS_LIST));
    for(i=0;i<n;i++) {
      if (ListView_GetItemState(GetDlgItem(dlg,ID_PLAYERS_LIST),i,
				LVIS_SELECTED)) {
	sel=i;
	break;
      }
    }
  }
  switch(message) {
  case WM_CREATE:
  case WM_GETMINMAXINFO:
  case WM_SIZE:
    break;
  case WM_DESTROY:
    ListView_SetImageList(GetDlgItem(players_dialog,ID_PLAYERS_LIST),
			  NULL,LVSIL_SMALL);
    players_dialog=NULL;
    break;
  case WM_CLOSE:
    DestroyWindow(dlg);
    break;
  case WM_COMMAND:
    
    if (LOWORD(wParam)==IDCANCEL) {
      DestroyWindow(dlg);
      break;
    }
    if (sel<0)
      break;
    lvi.iItem=sel;
    lvi.mask=LVIF_PARAM;
    ListView_GetItem(GetDlgItem(players_dialog,ID_PLAYERS_LIST),&lvi);
    player_index=lvi.lParam;
    switch(LOWORD(wParam)) {
    case ID_PLAYERS_MEET:
      players_meet(player_index);
      break;
    case ID_PLAYERS_WAR:
      players_war(player_index);
      break;
    case ID_PLAYERS_VISION:
      players_vision(player_index);
      break;
    case ID_PLAYERS_INT:
      players_intel(player_index);
      break;
    case ID_PLAYERS_SSHIP:
      players_sship(player_index);
      break;
    }
    
    break;
  case WM_NOTIFY:
    if (LOWORD(wParam)==ID_PLAYERS_LIST) {
      NM_LISTVIEW *nmlv=(NM_LISTVIEW *)lParam;
      if (nmlv->hdr.code==LVN_COLUMNCLICK) {
	if (nmlv->iSubItem==sort_column)
	  sort_dir=-sort_dir;
	else
	  sort_dir=1;
	sort_column=nmlv->iSubItem;
	ListView_SortItems(GetDlgItem(dlg,ID_PLAYERS_LIST),
			   sort_proc,sort_column);
      } else if (nmlv->hdr.code==LVN_ITEMCHANGED) {
	if (sel>=0) {
      
	  lvi.iItem=sel;
	  lvi.mask=LVIF_PARAM;
	  ListView_GetItem(GetDlgItem(players_dialog,ID_PLAYERS_LIST),&lvi);
	  player_index=lvi.lParam;
	  enable_buttons(player_index);
	}
      }
    } /* fall through */
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/******************************************************************

*******************************************************************/
static void create_players_dialog(void)
{
  int i;
  static char *titles_[NUM_COLUMNS] =
    { N_("Name"), N_("Nation"), N_("AI"),
      N_("Embassy"), N_("Dipl.State"), N_("Vision"),
      N_("State"), N_("Host"), N_("Idle")
    };
  struct fcwin_box *vbox;
  struct fcwin_box *hbox;

  players_dialog=fcwin_create_layouted_window(players_proc,
					      /* TRANS: Nations report title */
					      _("Nations"),
					      WS_OVERLAPPEDWINDOW,
					      CW_USEDEFAULT,CW_USEDEFAULT,
					      root_window,NULL,
					      JUST_CLEANUP,
					      NULL);
  hbox=fcwin_hbox_new(players_dialog,FALSE);
  vbox=fcwin_vbox_new(players_dialog,FALSE);
  fcwin_box_add_listview(vbox,5,ID_PLAYERS_LIST,LVS_REPORT | LVS_SINGLESEL,
			 TRUE,TRUE,0);
  fcwin_box_add_button(hbox,_("Close"),IDCANCEL,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Intelligence"),ID_PLAYERS_INT,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Meet"),ID_PLAYERS_MEET,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Cancel Treaty"),ID_PLAYERS_WAR,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Withdraw vision"),ID_PLAYERS_VISION,
		       0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Spaceship"),ID_PLAYERS_SSHIP,0,TRUE,TRUE,5);
  EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_INT),FALSE);
  EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_MEET),FALSE);
  EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_WAR),FALSE);
  EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_VISION),FALSE);
  EnableWindow(GetDlgItem(players_dialog,ID_PLAYERS_SSHIP),FALSE);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  for(i=0;i<NUM_COLUMNS;i++) {
    LV_COLUMN lvc;
    lvc.pszText=_(titles_[i]);
    lvc.mask=LVCF_TEXT;
    ListView_InsertColumn(GetDlgItem(players_dialog,ID_PLAYERS_LIST),i,&lvc);
  }

  fcwin_set_box(players_dialog,vbox);
  update_players_dialog();
}

/******************************************************************

*******************************************************************/      
void
popup_players_dialog(bool raise)
{
  if (!players_dialog)
    create_players_dialog();
  ShowWindow(players_dialog,SW_SHOWNORMAL);
  if (raise) {
    SetFocus(players_dialog);
  }
}

/**************************************************************************
...
**************************************************************************/
void
update_players_dialog(void)
{
  if (players_dialog && !is_plrdlg_frozen()) {
    LV_ITEM lvi;
    HWND lv;
    const char *row_texts[NUM_COLUMNS];
    int i,row;
    lv=GetDlgItem(players_dialog,ID_PLAYERS_LIST);
    ListView_DeleteAllItems(lv);

    players_iterate(pplayer) {
      i = player_number(pplayer);
      build_row(row_texts, i, 0);
      row=fcwin_listview_add_row(lv,i,NUM_COLUMNS, (char **)row_texts);
      lvi.iItem=row;
      lvi.iSubItem=0;
      lvi.lParam=i;
      lvi.mask=LVIF_PARAM;
      ListView_SetItem(lv,&lvi);
    } players_iterate_end;
    ListView_SetColumnWidth(lv,0,LVSCW_AUTOSIZE);
    for (i=1;i<NUM_COLUMNS;i++) {
      ListView_SetColumnWidth(lv,i,LVSCW_AUTOSIZE_USEHEADER); 
    }
    fcwin_redo_layout(players_dialog);
    ListView_SortItems(lv,sort_proc,sort_column);
  }
}
