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
#include "gui_main.h" 

/* client */
#include "client_main.h"
#include "gui_stuff.h"
#include "mapview.h"

#include "ratesdlg.h"

extern HINSTANCE freecivhinst;

static HWND ratesdlg;

int rates_tax_value, rates_lux_value, rates_sci_value;


/**************************************************************************

**************************************************************************/
static void rates_set_values(int tax, int no_tax_scroll,
                             int lux, int no_lux_scroll,
                             int sci, int no_sci_scroll)    
{
  HWND scroll;
  char buf[64];
  int tax_lock, lux_lock, sci_lock;
  int maxrate;  
  

  tax_lock=IsDlgButtonChecked(ratesdlg,ID_RATES_TAXLOCK);
  lux_lock=IsDlgButtonChecked(ratesdlg,ID_RATES_LUXURYLOCK);
  sci_lock=IsDlgButtonChecked(ratesdlg,ID_RATES_SCIENCELOCK);
  
  maxrate = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
  /* This's quite a simple-minded "double check".. */     
  tax=MIN(tax, maxrate);
  lux=MIN(lux, maxrate);
  sci=MIN(sci, maxrate);
  
  if(tax+sci+lux!=100)
    {                     
      if((tax!=rates_tax_value))
	{
	  if(!lux_lock)
	    lux=MIN(MAX(100-tax-sci, 0), maxrate);
	  if(!sci_lock)
	    sci=MIN(MAX(100-tax-lux, 0), maxrate);
	}
      else if((lux!=rates_lux_value))
	{
	  if(!tax_lock)
	    tax=MIN(MAX(100-lux-sci, 0), maxrate);
	  if(!sci_lock)
	    sci=MIN(MAX(100-lux-tax, 0), maxrate);
	}
      else if((sci!=rates_sci_value))
	{
	  if(!lux_lock)
	    lux=MIN(MAX(100-tax-sci, 0), maxrate);
	  if(!tax_lock)
	    tax=MIN(MAX(100-lux-sci, 0), maxrate);
	}
      
      if(tax+sci+lux!=100) {
	tax=rates_tax_value;
	lux=rates_lux_value;
	sci=rates_sci_value;
	
	rates_tax_value=-1;
	rates_lux_value=-1;
	rates_sci_value=-1;
	
	no_tax_scroll=0;
	no_lux_scroll=0;
	no_sci_scroll=0;
      }
      
    }
  
  if (tax!=rates_tax_value)
    {
      scroll=GetDlgItem(ratesdlg,ID_RATES_TAX);
      my_snprintf(buf, sizeof(buf), "%3d%%", tax); 
      SetWindowText(GetNextSibling(scroll),buf);
      
      if (!no_tax_scroll)
	{
	  ScrollBar_SetPos(scroll,tax/10,TRUE);
	}
      rates_tax_value=tax;
    }
  
  if (lux!=rates_lux_value)
    {
      scroll=GetDlgItem(ratesdlg,ID_RATES_LUXURY);
      my_snprintf(buf, sizeof(buf), "%3d%%", lux); 
      SetWindowText(GetNextSibling(scroll),buf);
      
      if (!no_lux_scroll)
	{
	  ScrollBar_SetPos(scroll,lux/10,TRUE);
	}
      rates_lux_value=lux;
    }

  if (sci!=rates_sci_value)
    {
      scroll=GetDlgItem(ratesdlg,ID_RATES_SCIENCE);
      my_snprintf(buf, sizeof(buf), "%3d%%", sci); 
      SetWindowText(GetNextSibling(scroll),buf);
      
      if (!no_sci_scroll)
	{
	  ScrollBar_SetPos(scroll,sci/10,TRUE);
	}
      rates_sci_value=sci;
    }
}

/**************************************************************************

**************************************************************************/
static void handle_hscroll(HWND hWnd,HWND hWndCtl,UINT code,int pos) 
{
  int PosCur,PosMax,PosMin,id;
  PosCur=ScrollBar_GetPos(hWndCtl);
  ScrollBar_GetRange(hWndCtl,&PosMin,&PosMax);
  switch(code)
    {
    case SB_LINELEFT: PosCur--; break;
    case SB_LINERIGHT: PosCur++; break;
    case SB_PAGELEFT: PosCur-=(PosMax-PosMin+1)/10; break;
    case SB_PAGERIGHT: PosCur+=(PosMax-PosMin+1)/10; break;
    case SB_LEFT: PosCur=PosMin; break;
    case SB_RIGHT: PosCur=PosMax; break;
    case SB_THUMBTRACK: PosCur=pos; break; 
    default:
      return;
    }
  id=GetDlgCtrlID(hWndCtl);
  pos = min(10, max(0, PosCur));
  if (id==ID_RATES_TAX)
    {
      int tax_value;
      
      tax_value=10*pos;
      tax_value=MIN(tax_value, 100);
      rates_set_values(tax_value,0, rates_lux_value,0, rates_sci_value,0);  
    }
  else if (id==ID_RATES_LUXURY)
    {
      int lux_value;
      
      lux_value=10*pos;
      lux_value=MIN(lux_value, 100);
      rates_set_values(rates_tax_value,0, lux_value,0, rates_sci_value,0);        
    }
  else
    {
      int sci_value;
      
      sci_value=10*pos;
      sci_value=MIN(sci_value, 100);
      rates_set_values(rates_tax_value,0, rates_lux_value,0, sci_value,0);        
    }
}

/**************************************************************************

**************************************************************************/
static void scroll_minsize(POINT *rcmin,void *data)
{
  rcmin->y=15;
  rcmin->x=300;
}

/**************************************************************************

**************************************************************************/
static void scroll_setsize(RECT *rc,void *data)
{
  MoveWindow((HWND)data,rc->left,rc->top,
	     rc->right-rc->left,
	     rc->bottom-rc->top,TRUE);
}

/**************************************************************************

**************************************************************************/
static void scroll_del(void *data)
{
  DestroyWindow((HWND)data);
}

/**************************************************************************

**************************************************************************/
static HWND ratesdlg_add_scroll(struct fcwin_box *fcb,int id)
{
  HWND scroll;
  scroll=CreateWindow("SCROLLBAR",NULL,
		      WS_CHILD | WS_VISIBLE | SBS_HORZ,
		      0,0,0,0,
		      ratesdlg,
		      (HMENU)id,
		      freecivhinst,NULL);
  fcwin_box_add_generic(fcb,scroll_minsize,scroll_setsize,scroll_del,
			scroll,TRUE,TRUE,15);
  return scroll;
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK ratesdlg_proc(HWND hWnd,
				   UINT message,
				   WPARAM wParam,
				   LPARAM lParam) 
{
  switch (message)
    {
    case WM_CREATE:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
      break;
    case WM_DESTROY:
      ratesdlg=NULL;
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      return TRUE;
    case WM_COMMAND:
      switch(LOWORD(wParam))
	{
	case IDCANCEL:
	  DestroyWindow(hWnd);
	  break;
	case IDOK:
	  DestroyWindow(hWnd);
	  dsend_packet_player_rates(&client.conn, rates_tax_value,
				    rates_lux_value, rates_sci_value);
	  break;
	}
      break;
    case WM_HSCROLL:
      HANDLE_WM_HSCROLL(hWnd,wParam,lParam,handle_hscroll);      
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return 0;
}

/**************************************************************************

**************************************************************************/
void
popup_rates_dialog(void)
{ 
  char buf[64];    
  rates_tax_value=-1;
  rates_lux_value=-1;
  rates_sci_value=-1;
  if (!ratesdlg) {
    struct fcwin_box *hbox;
    struct fcwin_box *vbox;
    ratesdlg=fcwin_create_layouted_window(ratesdlg_proc,
					  _("Select tax, luxury "
					    "and science rates"),
					  WS_OVERLAPPEDWINDOW,
					  CW_USEDEFAULT,
					  CW_USEDEFAULT,
					  root_window,
					  NULL,
					  JUST_CLEANUP,
					  NULL);
    
    vbox=fcwin_vbox_new(ratesdlg,FALSE);
    fcwin_box_add_static(vbox,"",ID_RATES_MAX,SS_CENTER,
			 FALSE,FALSE,10);
    hbox=fcwin_hbox_new(ratesdlg,FALSE);
    fcwin_box_add_groupbox(vbox,_("Tax"),hbox,0,FALSE,FALSE,10);
    ratesdlg_add_scroll(hbox,ID_RATES_TAX);
    fcwin_box_add_static(hbox,"100%",0,SS_RIGHT,FALSE,FALSE,20);
    fcwin_box_add_checkbox(hbox,_("Lock"),ID_RATES_TAXLOCK,0,FALSE,FALSE,20);
    
    hbox=fcwin_hbox_new(ratesdlg,FALSE);
    fcwin_box_add_groupbox(vbox,_("Luxury"),hbox,0,FALSE,FALSE,10);  
    ratesdlg_add_scroll(hbox,ID_RATES_LUXURY);
    fcwin_box_add_static(hbox,"100%",0,SS_RIGHT,FALSE,FALSE,20);
    fcwin_box_add_checkbox(hbox,_("Lock"),ID_RATES_LUXURYLOCK,0,FALSE,FALSE,20);

    hbox=fcwin_hbox_new(ratesdlg,FALSE);
    fcwin_box_add_groupbox(vbox,_("Science"),hbox,0,FALSE,FALSE,10);
    ratesdlg_add_scroll(hbox,ID_RATES_SCIENCE);
    fcwin_box_add_static(hbox,"100%",0,SS_RIGHT,FALSE,FALSE,20);
    fcwin_box_add_checkbox(hbox,_("Lock"),ID_RATES_SCIENCELOCK,0,FALSE,FALSE,20);
    hbox=fcwin_hbox_new(ratesdlg,TRUE);

    fcwin_box_add_button(hbox,_("Ok"),IDOK,0,TRUE,TRUE,20);
    fcwin_box_add_button(hbox,_("Cancel"),IDCANCEL,0,TRUE,TRUE,20);
    fcwin_box_add_box(vbox,hbox,TRUE,TRUE,10);

    my_snprintf(buf, sizeof(buf), _("%s max rate: %d%%"),
		government_name_for_player(client.conn.playing),
		get_player_bonus(client.conn.playing, EFT_MAX_RATES));
    SetWindowText(GetDlgItem(ratesdlg,ID_RATES_MAX),buf);
    ScrollBar_SetRange(GetDlgItem(ratesdlg,ID_RATES_TAX),0,10,TRUE);
    ScrollBar_SetRange(GetDlgItem(ratesdlg,ID_RATES_LUXURY),0,10,TRUE);
    ScrollBar_SetRange(GetDlgItem(ratesdlg,ID_RATES_SCIENCE),0,10,TRUE);
    rates_set_values( client.conn.playing->economic.tax, 0,
		      client.conn.playing->economic.luxury, 0,
		      client.conn.playing->economic.science, 0 );

    fcwin_set_box(ratesdlg,vbox);
    ShowWindow(ratesdlg,SW_SHOWNORMAL);
  }
}
