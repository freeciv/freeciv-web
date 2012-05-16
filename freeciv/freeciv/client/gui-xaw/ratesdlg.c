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
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Toggle.h>     

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

#include "ratesdlg.h"

/******************************************************************/
static Widget rates_dialog_shell;
static Widget rates_gov_label;
static Widget rates_tax_label, rates_tax_scroll, rates_tax_toggle;
static Widget rates_lux_label, rates_lux_scroll, rates_lux_toggle;
static Widget rates_sci_label, rates_sci_scroll, rates_sci_toggle;
/******************************************************************/

static int rates_tax_value, rates_lux_value, rates_sci_value;

void create_rates_dialog(void);

void rates_cancel_command_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
void rates_ok_command_callback(Widget w, XtPointer client_data, 
			       XtPointer call_data);



void rates_scroll_jump_callback(Widget w, XtPointer client_data,
				XtPointer percent_ptr);
void rates_scroll_scroll_callback(Widget w, XtPointer client_data,
			     XtPointer position_val);

void rates_set_values(int tax, int no_tax_scroll, 
		      int lux, int no_lux_scroll,
		      int sci, int no_sci_scroll);

/****************************************************************
... 
*****************************************************************/
void popup_rates_dialog(void)
{
  Position x, y;
  Dimension width, height;
  char buf[64];

  if (!can_client_issue_orders()) {
    return;
  }

  XtSetSensitive(main_form, FALSE);

  create_rates_dialog();
  
  XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
  
  XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
		    &x, &y);
  XtVaSetValues(rates_dialog_shell, XtNx, x, XtNy, y, NULL);

  my_snprintf(buf, sizeof(buf), _("%s max rate: %d%%"),
	      government_name_for_player(client.conn.playing),
	      get_player_bonus(client.conn.playing, EFT_MAX_RATES));
  xaw_set_label(rates_gov_label, buf);
  
  XtPopup(rates_dialog_shell, XtGrabNone);
}


/****************************************************************
...
*****************************************************************/
void create_rates_dialog(void)
{
  Widget rates_form;
  Widget rates_ok_command, rates_cancel_command;
  
  if (!can_client_issue_orders()) {
    return;
  }

  rates_dialog_shell =
    I_T(XtCreatePopupShell("ratespopup", transientShellWidgetClass,
			   toplevel, NULL, 0));

  rates_form = XtVaCreateManagedWidget("ratesform", 
				       formWidgetClass, 
				       rates_dialog_shell, NULL);   

  I_L(XtVaCreateManagedWidget("rateslabel", labelWidgetClass, 
			      rates_form, NULL));   

  rates_gov_label = XtVaCreateManagedWidget("ratesgovlabel",
					    labelWidgetClass,
					    rates_form, NULL);
  
  rates_tax_label = XtVaCreateManagedWidget("ratestaxlabel", 
					    labelWidgetClass, 
					    rates_form, NULL);

  rates_tax_scroll = XtVaCreateManagedWidget("ratestaxscroll", 
					     scrollbarWidgetClass, 
					     rates_form,
					     NULL);

  rates_tax_toggle = I_L(XtVaCreateManagedWidget("ratestaxtoggle", 
						 toggleWidgetClass, 
						 rates_form,
						 NULL));
    
  rates_lux_label = XtVaCreateManagedWidget("ratesluxlabel", 
					    labelWidgetClass, 
					    rates_form, NULL);

  rates_lux_scroll = XtVaCreateManagedWidget("ratesluxscroll", 
					     scrollbarWidgetClass, 
					     rates_form,
					     NULL);

  rates_lux_toggle = I_L(XtVaCreateManagedWidget("ratesluxtoggle", 
						 toggleWidgetClass, 
						 rates_form,
						 NULL));
  
  rates_sci_label = XtVaCreateManagedWidget("ratesscilabel", 
					    labelWidgetClass, 
					    rates_form, NULL);

  rates_sci_scroll = XtVaCreateManagedWidget("ratessciscroll", 
					     scrollbarWidgetClass, 
					     rates_form,
					     NULL);
  
  rates_sci_toggle = I_L(XtVaCreateManagedWidget("ratesscitoggle", 
						 toggleWidgetClass, 
						 rates_form,
						 NULL));
  
  rates_ok_command = I_L(XtVaCreateManagedWidget("ratesokcommand", 
						 commandWidgetClass,
						 rates_form,
						 NULL));

  rates_cancel_command = I_L(XtVaCreateManagedWidget("ratescancelcommand", 
						     commandWidgetClass,
						     rates_form,
						     NULL));

  XtAddCallback(rates_ok_command, XtNcallback, 
		rates_ok_command_callback, NULL);
  XtAddCallback(rates_cancel_command, XtNcallback, 
		rates_cancel_command_callback, NULL);

  XtAddCallback(rates_tax_scroll, XtNjumpProc, 
		rates_scroll_jump_callback, NULL);
  XtAddCallback(rates_tax_scroll, XtNscrollProc,
		rates_scroll_scroll_callback, NULL);

  
  XtAddCallback(rates_lux_scroll, XtNjumpProc, 
		rates_scroll_jump_callback, NULL);
  XtAddCallback(rates_lux_scroll, XtNscrollProc,
		rates_scroll_scroll_callback, NULL);

  
  XtAddCallback(rates_sci_scroll, XtNjumpProc, 
		rates_scroll_jump_callback, NULL);
  XtAddCallback(rates_sci_scroll, XtNscrollProc,
		rates_scroll_scroll_callback, NULL);

  XtRealizeWidget(rates_dialog_shell);

  
  rates_tax_value=-1;
  rates_lux_value=-1;
  rates_sci_value=-1;
  
  rates_set_values(client.conn.playing->economic.tax, 0,
		   client.conn.playing->economic.luxury, 0,
		   client.conn.playing->economic.science, 0);
}





/**************************************************************************
...
**************************************************************************/
void rates_ok_command_callback(Widget w, XtPointer client_data, 
			       XtPointer call_data)
{
  XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(rates_dialog_shell);

  dsend_packet_player_rates(&client.conn, rates_tax_value, rates_lux_value,
			    rates_sci_value);
}

/**************************************************************************
...
**************************************************************************/
void rates_cancel_command_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data)
{
  XtSetSensitive(main_form, TRUE);
  
  XtDestroyWidget(rates_dialog_shell);
}




/**************************************************************************
...
**************************************************************************/
void rates_set_values(int tax, int no_tax_scroll, 
		      int lux, int no_lux_scroll,
		      int sci, int no_sci_scroll)
{
  char buf[64];
  Boolean tax_lock, lux_lock, sci_lock;
  int maxrate;
  
  if (!can_client_issue_orders()) {
    return;
  }

  XtVaGetValues(rates_tax_toggle, XtNstate, &tax_lock, NULL);
  XtVaGetValues(rates_lux_toggle, XtNstate, &lux_lock, NULL);
  XtVaGetValues(rates_sci_toggle, XtNstate, &sci_lock, NULL);
  
  maxrate = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
  /* This's quite a simple-minded "double check".. */
  tax=MIN(tax, maxrate);
  lux=MIN(lux, maxrate);
  sci=MIN(sci, maxrate);
  
  if(tax+sci+lux!=100) {
    if(tax!=rates_tax_value) {
      if(!lux_lock)
	lux=MIN(MAX(100-tax-sci, 0), maxrate);
      if(!sci_lock)
	sci=MIN(MAX(100-tax-lux, 0), maxrate);
    }
    else if(lux!=rates_lux_value) {
      if(!tax_lock)
	tax=MIN(MAX(100-lux-sci, 0), maxrate);
      if(!sci_lock)
	sci=MIN(MAX(100-lux-tax, 0), maxrate);
    }
    else if(sci!=rates_sci_value) {
      if(!lux_lock)
	lux=MIN(MAX(100-tax-sci, 0), maxrate);
      if(!tax_lock)
	tax=MIN(MAX(100-lux-sci, 0), maxrate);
    }
    
    if(tax+sci+lux!=100) {
      lux=rates_lux_value;
      sci=rates_sci_value;
      tax=rates_tax_value;
      rates_tax_value=-1;
      rates_lux_value=-1;
      rates_sci_value=-1;
      no_tax_scroll=0;
      no_lux_scroll=0;
      no_sci_scroll=0;
    }

  }
  
  if(tax!=rates_tax_value) {
    my_snprintf(buf, sizeof(buf), _("Tax: %d%%"), tax);
    xaw_set_label(rates_tax_label, buf);
    if(!no_tax_scroll)
      XawScrollbarSetThumb(rates_tax_scroll, (tax/10)*1/11.0f, 1/11.0f);
    rates_tax_value=tax;
  }

  if(lux!=rates_lux_value) {
    my_snprintf(buf, sizeof(buf), _("Luxury: %d%%"), lux);
    xaw_set_label(rates_lux_label, buf);
    if(!no_lux_scroll)
      XawScrollbarSetThumb(rates_lux_scroll, (lux/10)*1/11.0f, 1/11.0f);
    rates_lux_value=lux;
  }

  if(sci!=rates_sci_value) {
    my_snprintf(buf, sizeof(buf), _("Science: %d%%"), sci);
    xaw_set_label(rates_sci_label, buf);
    if(!no_sci_scroll)
      XawScrollbarSetThumb(rates_sci_scroll, (sci/10)*1/11.0f, 1/11.0f);
    rates_sci_value=sci;
  }
  
}


/**************************************************************************
...
**************************************************************************/
void rates_scroll_jump_callback(Widget w, XtPointer client_data,
				XtPointer percent_ptr)
{
  float percent=*(float*)percent_ptr;

  if(w==rates_tax_scroll) {
    int tax_value;
    tax_value=10*(int)(11*percent);
    tax_value=MIN(tax_value, 100);
    rates_set_values(tax_value,1, rates_lux_value,0, rates_sci_value,0);
  }
  else if(w==rates_lux_scroll) {
    int lux_value;
    lux_value=10*(int)(11*percent);
    lux_value=MIN(lux_value, 100);
    rates_set_values(rates_tax_value,0, lux_value,1, rates_sci_value,0);
  }
  else {
    int sci_value;
    sci_value=10*(int)(11*percent);
    sci_value=MIN(sci_value, 100);
    rates_set_values(rates_tax_value,0, rates_lux_value,0, sci_value,1);
  }

}


/**************************************************************************
...
**************************************************************************/
void rates_scroll_scroll_callback(Widget w, XtPointer client_data,
				  XtPointer position_val)
{
  int val, pos = XTPOINTER_TO_INT(position_val);
  
  if(w==rates_tax_scroll) {
    if(pos<0)
      val=MAX(rates_tax_value-10, 0);
    else
      val=MIN(rates_tax_value+10, 100);
    rates_set_values(val,0, rates_lux_value,0, rates_sci_value,0);
  }
  else if(w==rates_lux_scroll) {
    if(pos<0)
      val=MAX(rates_lux_value-10, 0);
    else
      val=MIN(rates_lux_value+10, 100);
    rates_set_values(rates_tax_value,0, val,0, rates_sci_value,0);
  }
  else {
    if(pos<0)
      val=MAX(rates_sci_value-10, 0);
    else
      val=MIN(rates_sci_value+10, 100);
    rates_set_values(rates_tax_value,0, rates_lux_value,0, val,0);
  }
}
