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

#include <windows.h>
#include <windowsx.h>

/* common & utility */
#include "fcintl.h"
#include "government.h"
#include "packets.h"
#include "player.h"
#include "shared.h"
#include "support.h"

/* client */
#include "client_main.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"


#include "inteldlg.h"

static HWND intel_dialog;

/****************************************************************

*****************************************************************/
static LONG CALLBACK intel_proc(HWND dlg,UINT message,WPARAM wParam,LPARAM lParam)
{
  switch(message) {
  case WM_SIZE:
  case WM_GETMINMAXINFO:
  case WM_CREATE:
    break;
  case WM_DESTROY:
    intel_dialog=NULL;
    break;
  case WM_CLOSE:
    DestroyWindow(intel_dialog);
    break;
  case WM_COMMAND:
    if (LOWORD(wParam)==IDCANCEL)
      DestroyWindow(intel_dialog);
    break;
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/****************************************************************

*****************************************************************/
static void intel_create_dialog(struct player *p)
{
  HWND lb;
  char buf[64];
  struct city *pcity;
  
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  

  static char tech_list_names[A_LAST+1][200];
  int j = 0;
  
  intel_dialog=fcwin_create_layouted_window(intel_proc,
					    _("Foreign Intelligence Report"),
					    WS_OVERLAPPEDWINDOW,
					    CW_USEDEFAULT,CW_USEDEFAULT,
					    root_window,NULL,
					    JUST_CLEANUP,
					    NULL);
  vbox=fcwin_vbox_new(intel_dialog,FALSE);

  my_snprintf(buf, sizeof(buf),
              _("Intelligence Information for the %s Empire"), 
              nation_adjective_for_player(p));
  fcwin_box_add_static(vbox,buf,0,SS_LEFT,FALSE,FALSE,5);
  hbox=fcwin_hbox_new(intel_dialog,FALSE);
  
  my_snprintf(buf, sizeof(buf), _("Ruler: %s %s"), 
              ruler_title_translation(p),
              player_name(p));
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);

  my_snprintf(buf, sizeof(buf), _("Government: %s"),  
	      government_name_for_player(p));
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  
  hbox=fcwin_hbox_new(intel_dialog,FALSE);
  
  my_snprintf(buf, sizeof(buf), _("Gold: %d"), p->economic.gold);
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);
  
  my_snprintf(buf, sizeof(buf), _("Tax: %d%%"), p->economic.tax);
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);
  
  my_snprintf(buf, sizeof(buf), _("Science: %d%%"), p->economic.science);
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);
  
  my_snprintf(buf, sizeof(buf), _("Luxury: %d%%"), p->economic.luxury);
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);
  
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  
  hbox=fcwin_hbox_new(intel_dialog,FALSE);
   
  switch (get_player_research(p)->researching) {
  case A_UNKNOWN:
    my_snprintf(buf, sizeof(buf), _("Researching: (Unknown)"));
    break;
  case A_UNSET:
    my_snprintf(buf, sizeof(buf), _("Researching: Unknown(%d/-)"),
		get_player_research(p)->bulbs_researched);
    break;
  default:
    my_snprintf(buf, sizeof(buf), _("Researching: %s(%d/%d)"),
	        advance_name_researching(p),
	        get_player_research(p)->bulbs_researched,
	        total_bulbs_required(p));
    break;
  };
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);
  
  pcity = find_palace(p);
  my_snprintf(buf, sizeof(buf), _("Capital: %s"),
              /* TRANS: "unknown" location */
              (!pcity) ? _("(unknown)") : city_name(pcity));
  fcwin_box_add_static(hbox,buf,0,SS_CENTER,TRUE,TRUE,10);
  
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);

  lb=fcwin_box_add_list(vbox,10,0,LBS_NOSEL | LBS_SORT | WS_VSCROLL,
			TRUE,TRUE,5);
  
  advance_index_iterate(A_FIRST, i) {
    if(player_invention_state(p, i)==TECH_KNOWN) {
      if (TECH_KNOWN == player_invention_state(client.conn.playing, i)) {
        sz_strlcpy(tech_list_names[j], advance_name_translation(advance_by_number(i)));
      } else {
        my_snprintf(tech_list_names[j], sizeof(tech_list_names[j]),
                    "%s*", advance_name_translation(advance_by_number(i)));
      }
      ListBox_AddString(lb,tech_list_names[j]);
      j++;
    }
  } advance_index_iterate_end;

  fcwin_box_add_button(vbox,_("Close"),IDCANCEL,0,FALSE,FALSE,5);
  fcwin_set_box(intel_dialog,vbox);
}

/****************************************************************
... 
*****************************************************************/
void
popup_intel_dialog(struct player *p)
{
  if(!intel_dialog) {
    intel_create_dialog(p);
    ShowWindow(intel_dialog,SW_SHOWNORMAL);
  }
}

/****************************************************************************
  Update the intelligence dialog for the given player.  This is called by
  the core client code when that player's information changes.
****************************************************************************/
void update_intel_dialog(struct player *p)
{
  /* PORTME */
}
