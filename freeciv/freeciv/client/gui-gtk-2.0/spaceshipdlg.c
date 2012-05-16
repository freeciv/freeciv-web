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

#include <gtk/gtk.h>

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
#include "options.h"
#include "text.h"
#include "tilespec.h"

/* gui-gtk-2.0 */
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "repodlgs.h"

#include "spaceshipdlg.h"

struct spaceship_dialog {
  struct player *pplayer;
  struct gui_dialog *shell;
  GtkWidget *main_form;
  GtkWidget *info_label;
  GtkWidget *image_canvas;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct spaceship_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct spaceship_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;

static struct spaceship_dialog *get_spaceship_dialog(struct player *pplayer);
static struct spaceship_dialog *create_spaceship_dialog(struct player
							*pplayer);

static void spaceship_dialog_update_image(struct spaceship_dialog *pdialog);
static void spaceship_dialog_update_info(struct spaceship_dialog *pdialog);

/****************************************************************
...
*****************************************************************/
void spaceship_dialog_init()
{
  dialog_list = dialog_list_new();
}

/****************************************************************
...
*****************************************************************/
void spaceship_dialog_done()
{
  dialog_list_free(dialog_list);
}

/****************************************************************
...
*****************************************************************/
struct spaceship_dialog *get_spaceship_dialog(struct player *pplayer)
{
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
     && pship->success_rate > 0.0) {
    gui_dialog_set_response_sensitive(pdialog->shell,
	GTK_RESPONSE_ACCEPT, TRUE);
  } else {
    gui_dialog_set_response_sensitive(pdialog->shell,
	GTK_RESPONSE_ACCEPT, FALSE);
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

  gui_dialog_raise(pdialog->shell);
}

/****************************************************************
popdown the dialog 
*****************************************************************/
void popdown_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  
  if((pdialog=get_spaceship_dialog(pplayer)))
    gui_dialog_destroy(pdialog->shell);
}



/****************************************************************
...
*****************************************************************/
static gboolean spaceship_image_canvas_expose(GtkWidget *widget,
					      GdkEventExpose *ev,
					      gpointer data)
{
  struct spaceship_dialog *pdialog;
  
  pdialog=(struct spaceship_dialog *)data;
  spaceship_dialog_update_image(pdialog);
  return TRUE;
}

/****************************************************************
...
*****************************************************************/
static void spaceship_destroy_callback(GtkWidget *w, gpointer data)
{
  struct spaceship_dialog *pdialog = (struct spaceship_dialog *)data;
  
  dialog_list_unlink(dialog_list, pdialog);

  free(pdialog);
}

/****************************************************************
...
*****************************************************************/
static void spaceship_response(struct gui_dialog *dlg, int response,
                               gpointer data)
{
  switch (response) {
  case GTK_RESPONSE_ACCEPT:
    {
      send_packet_spaceship_launch(&client.conn);
    }
    break;

  default:
    gui_dialog_destroy(dlg);
    break;
  }
}
  
/****************************************************************
...
*****************************************************************/
struct spaceship_dialog *create_spaceship_dialog(struct player *pplayer)
{
  struct spaceship_dialog *pdialog;
  GtkWidget *hbox, *frame;
  int w, h;
  
  pdialog=fc_malloc(sizeof(struct spaceship_dialog));
  pdialog->pplayer=pplayer;

  gui_dialog_new(&pdialog->shell, GTK_NOTEBOOK(top_notebook), NULL);
  gui_dialog_set_title(pdialog->shell, player_name(pplayer));

  gui_dialog_add_button(pdialog->shell,
      _("_Launch"), GTK_RESPONSE_ACCEPT);
  gui_dialog_add_button(pdialog->shell,
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  g_signal_connect(pdialog->shell->vbox, "destroy",
		   G_CALLBACK(spaceship_destroy_callback), pdialog);
  gui_dialog_response_set_callback(pdialog->shell, spaceship_response);

  hbox=gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(pdialog->shell->vbox), hbox, FALSE, FALSE, 0);

  frame=gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

  pdialog->image_canvas=gtk_drawing_area_new();
  GTK_WIDGET_SET_FLAGS(pdialog->image_canvas, GTK_CAN_FOCUS);
  get_spaceship_dimensions(&w, &h);
  gtk_widget_set_size_request(pdialog->image_canvas, w, h);

  gtk_widget_set_events(pdialog->image_canvas, GDK_EXPOSURE_MASK);
  gtk_container_add(GTK_CONTAINER(frame), pdialog->image_canvas);
  gtk_widget_realize(pdialog->image_canvas);
  
  g_signal_connect(pdialog->image_canvas, "expose_event",
		   G_CALLBACK(spaceship_image_canvas_expose), pdialog);

  pdialog->info_label = gtk_label_new(get_spaceship_descr(NULL));

  gtk_label_set_justify(GTK_LABEL(pdialog->info_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(pdialog->info_label), 0.0, 0.0);

  gtk_box_pack_start(GTK_BOX(hbox), pdialog->info_label, FALSE, FALSE, 0);
  gtk_widget_set_name(pdialog->info_label, "spaceship_label");

  dialog_list_prepend(dialog_list, pdialog);

  gtk_widget_grab_focus(pdialog->image_canvas);

  gui_dialog_show_all(pdialog->shell);

  refresh_spaceship_dialog(pdialog->pplayer);

  return pdialog;
}

/****************************************************************
...
*****************************************************************/
void spaceship_dialog_update_info(struct spaceship_dialog *pdialog)
{
  gtk_label_set_text(GTK_LABEL(pdialog->info_label),
		     get_spaceship_descr(&pdialog->pplayer->spaceship));
}

/****************************************************************
...
Should also check connectedness, and show non-connected
parts differently.
*****************************************************************/
void spaceship_dialog_update_image(struct spaceship_dialog *pdialog)
{
  struct canvas store = {.type = CANVAS_PIXMAP,
			 .v = {.pixmap = pdialog->image_canvas->window}};

  put_spaceship(&store, 0, 0, pdialog->pplayer);
}

