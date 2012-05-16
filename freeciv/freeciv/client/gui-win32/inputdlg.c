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
 
#include "fcintl.h"

#include "mem.h"
#include "gui_main.h" 
#include "gui_stuff.h"
#include "shared.h"
#include "support.h"
#include "inputdlg.h"              

extern HINSTANCE freecivhinst;
struct input_dialog_data
{
  char text[256];
  void *okdata;
  void *canceldata;
  void (*ok_callback)(HWND,void *);
  void (*cancel_callback)(HWND,void *);
};
/**************************************************************************

**************************************************************************/

void input_dialog_destroy(HWND button)
{
  void *data;
  data=(struct input_dialog_data*)fcwin_get_user_data(button);
  free(data);
  DestroyWindow(button);
}

/**************************************************************************

**************************************************************************/
char *input_dialog_get_input(HWND button)
{
  struct input_dialog_data *idata;
  idata=(struct input_dialog_data *)fcwin_get_user_data(button);
  GetWindowText(GetDlgItem(button,ID_INPUT_TEXT),idata->text,255);
  return idata->text;
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK inputdlg_proc(HWND hWnd,
				   UINT message,
				   WPARAM wParam,
				   LPARAM lParam)
{
  struct input_dialog_data *id;
  id=(struct input_dialog_data *)fcwin_get_user_data(hWnd);
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_CLOSE:
      id->cancel_callback(hWnd,id->canceldata);
      break;
    case WM_DESTROY:
      break;
    case WM_GETMINMAXINFO:
      break;
    case WM_SIZE:
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
        {
	case ID_INPUT_TEXT: 
	  {
	    static char buf[128];
	    char buf2[128];
	    GetWindowText((HWND)lParam, buf2, sizeof(buf2));
	    if (strchr(buf2,'\n')) {
	      SetWindowText((HWND)lParam, buf);
	      id->ok_callback(hWnd, id->okdata);
	    } else {
	      sz_strlcpy(buf, buf2);
	    }
	  }
	  break;
        case ID_INPUT_OK:
          id->ok_callback(hWnd,id->okdata);
	  break;
        case ID_INPUT_CANCEL:
          id->cancel_callback(hWnd,id->canceldata);
	  break;
        }
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return 0;       
} 

/**************************************************************************

**************************************************************************/
HWND input_dialog_create(HWND parent, char *dialogname,
			 char *text, char *postinputtext,
			 void *ok_callback,void *ok_cli_data,
			 void *cancel_callback, void * cancel_cli_data)
{
  HWND dlg;
  struct input_dialog_data *idnew;
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  
  idnew=fc_malloc(sizeof(struct input_dialog_data));
  idnew->okdata=ok_cli_data;
  idnew->canceldata=cancel_cli_data;
  idnew->ok_callback=ok_callback;
  idnew->cancel_callback=cancel_callback;

  dlg=fcwin_create_layouted_window(inputdlg_proc,dialogname,
				   WS_OVERLAPPEDWINDOW,
				   CW_USEDEFAULT,CW_USEDEFAULT,
				   parent,NULL,
				   REAL_CHILD,
				   idnew);
  
  hbox=fcwin_hbox_new(dlg,TRUE);
  fcwin_box_add_button(hbox,_("OK"),ID_INPUT_OK,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Cancel"),ID_INPUT_CANCEL,0,TRUE,TRUE,5);
  vbox=fcwin_vbox_new(dlg,FALSE);
  fcwin_box_add_static(vbox,text,0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_edit(vbox,postinputtext,MAX(30,strlen(postinputtext)),
		     ID_INPUT_TEXT,
		     ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN,
		     FALSE,FALSE,5);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  fcwin_set_box(dlg,vbox);
  ShowWindow(dlg,SW_SHOWNORMAL);
  SetFocus(GetDlgItem(dlg, ID_INPUT_TEXT));
  return dlg;
}
