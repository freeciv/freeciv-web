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

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "game.h"
#include "map.h"
#include "packets.h"
#include "player.h"

/* client */
#include "canvas.h"
#include "client_main.h"
#include "climisc.h"
#include "colors.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "repodlgs.h"
#include "spaceship.h"
#include "tilespec.h"
#include "text.h"

#include "spaceshipdlg.h"

struct spaceship_dialog {
  struct player *pplayer;
  HWND mainwin;
  HWND info_label;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct spaceship_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct spaceship_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;
static bool dialog_list_has_been_initialised = FALSE;

struct spaceship_dialog *get_spaceship_dialog(struct player *pplayer);
struct spaceship_dialog *create_spaceship_dialog(struct player *pplayer);
void close_spaceship_dialog(struct spaceship_dialog *pdialog);

void spaceship_dialog_update_image(HDC hdc,struct spaceship_dialog *pdialog);
void spaceship_dialog_update_info(struct spaceship_dialog *pdialog);

/****************************************************************
...
*****************************************************************/
struct spaceship_dialog *get_spaceship_dialog(struct player *pplayer)
{
  if (!dialog_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_has_been_initialised = TRUE;
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pplayer == pplayer) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/****************************************************************
...
*****************************************************************/
void refresh_spaceship_dialog(struct player *pplayer)
{
  HDC hdc;
  struct spaceship_dialog *pdialog;
  struct player_spaceship *pship;

  if(!(pdialog=get_spaceship_dialog(pplayer)))
    return;

  pship=&(pdialog->pplayer->spaceship);

  if(game.info.spacerace
     && pplayer == client.conn.playing
     && pship->state == SSHIP_STARTED
     && pship->success_rate > 0) {
    EnableWindow(GetDlgItem(pdialog->mainwin,IDOK),TRUE);
  } else {
    EnableWindow(GetDlgItem(pdialog->mainwin,IDOK),FALSE);
  }
  
  spaceship_dialog_update_info(pdialog);
  hdc=GetDC(pdialog->mainwin);
  spaceship_dialog_update_image(hdc,pdialog);
  ReleaseDC(pdialog->mainwin,hdc);
}

/****************************************************************
popup the dialog
*****************************************************************/
void
popup_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  
  if(!(pdialog=get_spaceship_dialog(pplayer)))
    pdialog=create_spaceship_dialog(pplayer);
  ShowWindow(pdialog->mainwin,SW_SHOWNORMAL);
}

/***************************************************************

****************************************************************/
void
popdown_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  
  if((pdialog=get_spaceship_dialog(pplayer)))
    close_spaceship_dialog(pdialog);
  
}

/****************************************************************

*****************************************************************/
static LONG CALLBACK spaceship_proc(HWND dlg,UINT message,
				    WPARAM wParam,LPARAM lParam)
{
 
  struct spaceship_dialog *pdialog;
  pdialog=fcwin_get_user_data(dlg);
  switch(message) {
  case WM_SIZE:
  case WM_GETMINMAXINFO:
    break;
  case WM_CREATE:
    break;
  case WM_PAINT: 
    { 
      HDC hdc;
      PAINTSTRUCT ps;
      hdc=BeginPaint(dlg,(LPPAINTSTRUCT)&ps);
      spaceship_dialog_update_image(hdc,pdialog);
      EndPaint(dlg,(LPPAINTSTRUCT)&ps);
    }
    break;
  case WM_CLOSE:
    DestroyWindow(dlg);
    break;
  case WM_DESTROY:
    dialog_list_unlink(dialog_list, pdialog);
    free(pdialog);
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam)) {
    case IDOK: {
      send_packet_spaceship_launch(&client.conn);
    }
    break;
    case IDCANCEL:
      DestroyWindow(dlg);
      break;
    }
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/****************************************************************

*****************************************************************/
static void image_minsize(POINT *minsize,void *data)
{
  int width, height;
  get_spaceship_dimensions(&width, &height);
  minsize->x = width;
  minsize->y = height;
}

/****************************************************************

*****************************************************************/
static void image_setsize(RECT *size,void *data)
{
}

/****************************************************************
...
*****************************************************************/
struct spaceship_dialog *create_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  
  pdialog=fc_malloc(sizeof(struct spaceship_dialog));
  pdialog->pplayer=pplayer;
  pdialog->mainwin=fcwin_create_layouted_window(spaceship_proc,
						player_name(pplayer),
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,CW_USEDEFAULT,
						root_window,NULL,
						JUST_CLEANUP,
						pdialog);
  vbox=fcwin_vbox_new(pdialog->mainwin,FALSE);
  hbox=fcwin_hbox_new(pdialog->mainwin,FALSE);
  fcwin_box_add_generic(hbox,image_minsize,image_setsize,NULL,
			NULL,TRUE,TRUE,3);
  pdialog->info_label =
      fcwin_box_add_static(hbox, get_spaceship_descr(NULL), 0, SS_LEFT,
			   FALSE, FALSE, 2);
  fcwin_box_add_box(vbox,hbox,TRUE,TRUE,5);
  hbox=fcwin_hbox_new(pdialog->mainwin,TRUE);
  fcwin_box_add_button(hbox,_("Close"),IDCANCEL,0,TRUE,TRUE,20);
  fcwin_box_add_button(hbox,_("Launch"),IDOK,0,TRUE,TRUE,20);
  fcwin_box_add_box(vbox,hbox,TRUE,TRUE,5);
  fcwin_set_box(pdialog->mainwin,vbox);
  
  dialog_list_prepend(dialog_list, pdialog);
  refresh_spaceship_dialog(pdialog->pplayer);

  return pdialog;
}

/****************************************************************
...
*****************************************************************/
void spaceship_dialog_update_info(struct spaceship_dialog *pdialog)
{
  SetWindowText(pdialog->info_label,
		get_spaceship_descr(&pdialog->pplayer->spaceship));
}

/****************************************************************
...
Should also check connectedness, and show non-connected
parts differently.
*****************************************************************/
void spaceship_dialog_update_image(HDC hdc,struct spaceship_dialog *pdialog)
{
  struct canvas dialog_canvas;
  dialog_canvas.type = CANVAS_DC;
  dialog_canvas.hdc = hdc;
  dialog_canvas.tmp = 0;

  put_spaceship(&dialog_canvas, 0, 0, pdialog->pplayer);
}

/****************************************************************
...
*****************************************************************/
void close_spaceship_dialog(struct spaceship_dialog *pdialog)
{
  DestroyWindow(pdialog->mainwin);
}
