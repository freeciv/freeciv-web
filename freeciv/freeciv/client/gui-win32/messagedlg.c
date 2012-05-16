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
#include <commctrl.h>

#include "events.h"
#include "fcintl.h"
#include "game.h"
#include "packets.h"
#include "player.h"
#include "shared.h"

#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "optiondlg.h"
#include "options.h"

#include "messagedlg.h"

static HWND messageopt_dialog;
static HWND messageopt_toggles[E_LAST][NUM_MW];
static HWND messageopt_pages[10];
static HWND messageopt_tab;
static int num_pages;

/**************************************************************************

**************************************************************************/
static LONG CALLBACK messageopt_proc(HWND dlg,UINT message,
				     WPARAM wParam,LPARAM lParam)
{
  NMHDR *nmhdr;
  int i,j;
  switch(message) {
  case WM_CREATE:
  case WM_SIZE:
  case WM_GETMINMAXINFO:
    break;
  case WM_DESTROY:
    messageopt_dialog=NULL;
    break;
  case WM_CLOSE:
    DestroyWindow(dlg);
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam)) {
    case IDOK:
      for(i=0;i<E_LAST;i++)  {
	messages_where[i] = 0;
	for(j=0; j<NUM_MW; j++) {
	  if (Button_GetCheck(messageopt_toggles[i][j])==BST_CHECKED)
	    messages_where[i] |= (1<<j);
	}
      }
    case IDCANCEL:
      DestroyWindow(dlg);
      break;
    }
    break;
  case WM_NOTIFY:
    nmhdr = (LPNMHDR)lParam;
    if (nmhdr->hwndFrom == messageopt_tab) {
      int sel;
      sel = TabCtrl_GetCurSel(messageopt_tab);
      for (i = 0; i<num_pages; i++) {
	ShowWindow(messageopt_pages[i], SW_HIDE); 
      }
      if ((sel>=0) && (sel<num_pages))
	ShowWindow(messageopt_pages[sel],SW_SHOWNORMAL);
      
    }
    break;
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/**************************************************************************
... 
**************************************************************************/
static LONG CALLBACK messageopt_page_proc(HWND dlg, UINT message,
					  WPARAM wParam, LPARAM lParam)
{
  switch(message) {
  case WM_CREATE:
  case WM_DESTROY:
  case WM_SIZE:
  case WM_GETMINMAXINFO:
  case WM_COMMAND:
    break;
  default:
    return DefWindowProc(dlg, message, wParam, lParam);
  }
  return 0;
}

/**************************************************************************
... 
**************************************************************************/
static void fillin_message_dlg(HWND win, int start, int stop)
{
 
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  struct fcwin_box *table_vboxes[2*(NUM_MW+1)];
  int i,j;
  vbox=fcwin_vbox_new(win, FALSE);
  hbox=fcwin_hbox_new(win,FALSE); 
  fcwin_box_add_box(vbox, hbox, FALSE,FALSE,0);
  for(i=0;i<ARRAY_SIZE(table_vboxes);i++) {
    table_vboxes[i]=fcwin_vbox_new(win,TRUE);
    fcwin_box_add_box(hbox,table_vboxes[i],!(i%4),!(i%4),5);
  }
  fcwin_box_add_static(table_vboxes[0]," ",0,SS_LEFT,TRUE,TRUE,0);
  fcwin_box_add_static(table_vboxes[1],_("Out:"),0,SS_CENTER,TRUE,TRUE,0);
  fcwin_box_add_static(table_vboxes[2],_("Mes:"),0,SS_CENTER,TRUE,TRUE,0);
  fcwin_box_add_static(table_vboxes[3],_("Pop:"),0,SS_CENTER,TRUE,TRUE,0);
  fcwin_box_add_static(table_vboxes[4]," ",0,SS_LEFT,TRUE,TRUE,0);
  fcwin_box_add_static(table_vboxes[5],_("Out:"),0,SS_CENTER,TRUE,TRUE,0);
  fcwin_box_add_static(table_vboxes[6],_("Mes:"),0,SS_CENTER,TRUE,TRUE,0);
  fcwin_box_add_static(table_vboxes[7],_("Pop:"),0,SS_CENTER,TRUE,TRUE,0);
  for(i=start;i<stop;i++) {
    int is_col1 = (i-start)<((stop-start)/2);
    fcwin_box_add_static(table_vboxes[is_col1?0:4],
			 get_event_message_text(sorted_events[i]),
			 0,SS_LEFT,TRUE,TRUE,0);
    for(j=0; j<NUM_MW; j++) {
      messageopt_toggles[sorted_events[i]][j]=
	fcwin_box_add_checkbox(table_vboxes[(is_col1?1:5)+j],
			       " ",0,0,TRUE,TRUE,0);
    }
  }
  fcwin_set_box(win, vbox);
}

#define OPTIONS_PER_PAGE 40
/**************************************************************************
... 
**************************************************************************/
static void create_messageopt_dialog()
{
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  int i;
  char *titles[10];
  char titles_buf[10*5];
  void *user_data[10];
  WNDPROC wndprocs[10];

  messageopt_dialog=
    fcwin_create_layouted_window(messageopt_proc,
				 _("Message Options"),
				 WS_OVERLAPPEDWINDOW,
				 CW_USEDEFAULT, CW_USEDEFAULT,
				 root_window,NULL,
				 JUST_CLEANUP,
				 NULL);
  
  vbox=fcwin_vbox_new(messageopt_dialog,FALSE);
  fcwin_box_add_static(vbox,
		       _("Where to Display Messages\nOut = Output window,"
			 " Mes = Messages window,"
			 " Pop = Popup individual window"),
		       0,SS_CENTER,FALSE,FALSE,5);
  num_pages = ((E_LAST + OPTIONS_PER_PAGE-1) / OPTIONS_PER_PAGE);
  if (num_pages > ARRAY_SIZE(wndprocs))
    num_pages = ARRAY_SIZE(wndprocs);
  for (i = 0; i<num_pages; i++) {
    titles[i] = titles_buf + i * 5;
    strncpy(titles[i], 
	    get_event_message_text(sorted_events[i * OPTIONS_PER_PAGE]),
	    4);
    titles_buf[i * 5 + 4] = 0;
    wndprocs[i] = messageopt_page_proc;
    user_data[i] = NULL;
  }
  messageopt_tab = fcwin_box_add_tab(vbox, wndprocs,
				     messageopt_pages, titles,
				     user_data, num_pages,
				     0, 0, TRUE, TRUE, 0);
  for(i = 0; i<num_pages; i++) {
    int last;
    last = OPTIONS_PER_PAGE * (i + 1);
    if (last > E_LAST)
      last = E_LAST;
    fillin_message_dlg(messageopt_pages[i], i * OPTIONS_PER_PAGE,
		       last);
  }
  
  hbox=fcwin_hbox_new(messageopt_dialog,TRUE);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  fcwin_box_add_button(hbox,_("Ok"),IDOK,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Cancel"),IDCANCEL,0,TRUE,TRUE,5);
  fcwin_set_box(messageopt_dialog,vbox);
  ShowWindow(messageopt_pages[0], SW_SHOWNORMAL);
}
/**************************************************************************
... 
**************************************************************************/
void
popup_messageopt_dialog(void)
{
  int i, j, state;  
  if (!messageopt_dialog)
    create_messageopt_dialog();
  if (!messageopt_dialog)
    return;
  for(i=0; i<E_LAST; i++) {
    for(j=0; j<NUM_MW; j++) {
      state = messages_where[i] & (1<<j);
      Button_SetCheck(messageopt_toggles[i][j],
		      state?BST_CHECKED:BST_UNCHECKED);
    }
  }
  ShowWindow(messageopt_dialog,SW_SHOWNORMAL);
}
