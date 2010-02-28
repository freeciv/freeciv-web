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
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/AsciiText.h>  
#include <X11/IntrinsicP.h>

#include "canvas.h"
#include "pixcomm.h"

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
#include "spaceship.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "repodlgs.h"
#include "text.h"
#include "tilespec.h"

#include "spaceshipdlg.h"

struct spaceship_dialog {
  struct player *pplayer;
  Widget shell;
  Widget main_form;
  Widget player_label;
  Widget info_label;
  Widget image_canvas;
  Widget launch_command;
  Widget close_command;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct spaceship_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct spaceship_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list = NULL;
static bool dialog_list_has_been_initialised = FALSE;

struct spaceship_dialog *get_spaceship_dialog(struct player *pplayer);
struct spaceship_dialog *create_spaceship_dialog(struct player *pplayer);
void close_spaceship_dialog(struct spaceship_dialog *pdialog);

void spaceship_dialog_update_image(struct spaceship_dialog *pdialog);
void spaceship_dialog_update_info(struct spaceship_dialog *pdialog);

void spaceship_close_callback(Widget w, XtPointer client_data, XtPointer call_data);
void spaceship_launch_callback(Widget w, XtPointer client_data, XtPointer call_data);

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
  struct spaceship_dialog *pdialog;
  struct player_spaceship *pship;

  if(!(pdialog=get_spaceship_dialog(pplayer)))
    return;

  pship=&(pdialog->pplayer->spaceship);

  if (game.info.spacerace
     && pplayer == client.conn.playing
     && pship->state == SSHIP_STARTED
     && pship->success_rate > 0) {
    XtSetSensitive(pdialog->launch_command, TRUE);
  } else {
    XtSetSensitive(pdialog->launch_command, FALSE);
  }
  
  spaceship_dialog_update_info(pdialog);
  spaceship_dialog_update_image(pdialog);
}

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  
  if(!(pdialog=get_spaceship_dialog(pplayer)))
    pdialog=create_spaceship_dialog(pplayer);

  xaw_set_relative_position(toplevel, pdialog->shell, 10, 10);
  XtPopup(pdialog->shell, XtGrabNone);
}

/****************************************************************
popdown the dialog 
*****************************************************************/
void popdown_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  
  if((pdialog=get_spaceship_dialog(pplayer)))
    close_spaceship_dialog(pdialog);
}



/****************************************************************
...
*****************************************************************/
static void spaceship_image_canvas_expose(Widget w, XEvent *event,
					  Region exposed, void *client_data)
{
  struct spaceship_dialog *pdialog;
  
  pdialog=(struct spaceship_dialog *)client_data;
  spaceship_dialog_update_image(pdialog);
}


/****************************************************************
...
*****************************************************************/
struct spaceship_dialog *create_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  int ss_w, ss_h;

  pdialog=fc_malloc(sizeof(struct spaceship_dialog));
  pdialog->pplayer=pplayer;

  pdialog->shell=XtVaCreatePopupShell(player_name(pplayer),
				      topLevelShellWidgetClass,
				      toplevel, 
				      XtNallowShellResize, True, 
				      NULL);
  
  pdialog->main_form=
    XtVaCreateManagedWidget("spaceshipmainform", 
			    formWidgetClass, 
			    pdialog->shell, 
			    NULL);
  pdialog->player_label=
    XtVaCreateManagedWidget("spaceshipplayerlabel", 
			    labelWidgetClass,
			    pdialog->main_form,
			    NULL);

  XtVaSetValues(pdialog->player_label, XtNlabel, (XtArgVal)player_name(pplayer), NULL);

  get_spaceship_dimensions(&ss_w, &ss_h);
  pdialog->image_canvas=
    XtVaCreateManagedWidget("spaceshipimagecanvas",
			    xfwfcanvasWidgetClass,
			    pdialog->main_form,
			    "exposeProc", (XtArgVal)spaceship_image_canvas_expose,
			    "exposeProcData", (XtArgVal)pdialog,
			    XtNwidth, ss_w,
			    XtNheight, ss_h,
			    NULL);

  pdialog->info_label=
    XtVaCreateManagedWidget("spaceshipinfolabel", 
			    labelWidgetClass,
			    pdialog->main_form,
			    NULL);

  
  pdialog->close_command=
    I_L(XtVaCreateManagedWidget("spaceshipclosecommand", commandWidgetClass,
				pdialog->main_form, NULL));

  pdialog->launch_command=
    I_L(XtVaCreateManagedWidget("spaceshiplaunchcommand", commandWidgetClass,
				pdialog->main_form, NULL));

  XtAddCallback(pdialog->launch_command, XtNcallback, spaceship_launch_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->close_command, XtNcallback, spaceship_close_callback,
		(XtPointer)pdialog);

  dialog_list_prepend(dialog_list, pdialog);

  XtRealizeWidget(pdialog->shell);

  refresh_spaceship_dialog(pdialog->pplayer);

  XSetWMProtocols(display, XtWindow(pdialog->shell), &wm_delete_window, 1);
  XtOverrideTranslations(pdialog->shell, 
    XtParseTranslationTable ("<Message>WM_PROTOCOLS: msg-close-spaceship()"));

  XtSetKeyboardFocus(pdialog->shell, pdialog->close_command);

  return pdialog;
}

/****************************************************************
...
*****************************************************************/
void spaceship_dialog_update_info(struct spaceship_dialog *pdialog)
{
  xaw_set_label(pdialog->info_label,
		get_spaceship_descr(&pdialog->pplayer->spaceship));
}

/****************************************************************
...
Should also check connectedness, and show non-connected
parts differently.
*****************************************************************/
void spaceship_dialog_update_image(struct spaceship_dialog *pdialog)
{
  struct canvas canvas = {.pixmap = XtWindow(pdialog->image_canvas)};

  put_spaceship(&canvas, 0, 0, pdialog->pplayer);
}


/****************************************************************
...
*****************************************************************/
void close_spaceship_dialog(struct spaceship_dialog *pdialog)
{
  XtDestroyWidget(pdialog->shell);
  dialog_list_unlink(dialog_list, pdialog);

  free(pdialog);
}

/****************************************************************
...
*****************************************************************/
void spaceshipdlg_key_close(Widget w)
{
  spaceshipdlg_msg_close(XtParent(XtParent(w)));
}

/****************************************************************
...
*****************************************************************/
void spaceshipdlg_msg_close(Widget w)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->shell == w) {
      close_spaceship_dialog(pdialog);
      return;
    }
  } dialog_list_iterate_end;
}


/****************************************************************
...
*****************************************************************/
void spaceship_close_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  close_spaceship_dialog((struct spaceship_dialog *)client_data);
}


/****************************************************************
...
*****************************************************************/
void spaceship_launch_callback(Widget w, XtPointer client_data,
			       XtPointer call_data)
{
  send_packet_spaceship_launch(&client.conn);
  /* close_spaceship_dialog((struct spaceship_dialog *)client_data); */
}
