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

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "government.h"

/* client */
#include "text.h"
#include "tilespec.h"

/* gui-gtk-2.0 */
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "happiness.h"
#include "mapview.h"

/* semi-arbitrary number that controls the width of the happiness widget */
#define HAPPINESS_PIX_WIDTH 23

#define	PIXCOMM_WIDTH	(HAPPINESS_PIX_WIDTH * tileset_small_sprite_width(tileset))
#define	PIXCOMM_HEIGHT	(tileset_small_sprite_height(tileset))

#define NUM_HAPPINESS_MODIFIERS 5

enum { CITIES, LUXURIES, BUILDINGS, UNITS, WONDERS };

struct happiness_dialog {
  struct city *pcity;
  GtkWidget *shell;
  GtkWidget *cityname_label;
  GtkWidget *hpixmaps[NUM_HAPPINESS_MODIFIERS];
  GtkWidget *hlabels[NUM_HAPPINESS_MODIFIERS];
  GtkWidget *close;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct happiness_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct happiness_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;
static struct happiness_dialog *get_happiness_dialog(struct city *pcity);
static struct happiness_dialog *create_happiness_dialog(struct city
							*pcity);

/****************************************************************
...
*****************************************************************/
void happiness_dialog_init()
{
  dialog_list = dialog_list_new();
}

/****************************************************************
...
*****************************************************************/
void happiness_dialog_done()
{
  dialog_list_free(dialog_list);
}

/****************************************************************
...
*****************************************************************/
static struct happiness_dialog *get_happiness_dialog(struct city *pcity)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pcity == pcity) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/**************************************************************************
...
**************************************************************************/
static struct happiness_dialog *create_happiness_dialog(struct city *pcity)
{
  int i;
  struct happiness_dialog *pdialog;
  GtkWidget *vbox;

  pdialog = fc_malloc(sizeof(struct happiness_dialog));
  pdialog->pcity = pcity;

  pdialog->shell = gtk_vbox_new(FALSE, 0);

  pdialog->cityname_label = gtk_frame_new(_("Happiness"));
  gtk_box_pack_start(GTK_BOX(pdialog->shell),
		     pdialog->cityname_label, TRUE, TRUE, 0);

  vbox = gtk_vbox_new(FALSE, 18);
  gtk_container_add(GTK_CONTAINER(pdialog->cityname_label), vbox);

  for (i = 0; i < NUM_HAPPINESS_MODIFIERS; i++) {
    GtkWidget *box;
    
    box = gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);
    
    pdialog->hpixmaps[i] = gtk_pixcomm_new(PIXCOMM_WIDTH, PIXCOMM_HEIGHT);
    gtk_box_pack_start(GTK_BOX(box), pdialog->hpixmaps[i], FALSE, FALSE, 0);

    pdialog->hlabels[i] = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(box), pdialog->hlabels[i], TRUE, FALSE, 0);

    gtk_misc_set_alignment(GTK_MISC(pdialog->hpixmaps[i]), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(pdialog->hlabels[i]), 0, 0);
    gtk_label_set_justify(GTK_LABEL(pdialog->hlabels[i]), GTK_JUSTIFY_LEFT);
    gtk_label_set_line_wrap(GTK_LABEL(pdialog->hlabels[i]), TRUE);
  }

  gtk_widget_show_all(pdialog->shell);

  dialog_list_prepend(dialog_list, pdialog);

  refresh_happiness_dialog(pcity);

  return pdialog;
}

/**************************************************************************
...
**************************************************************************/
static void refresh_pixcomm(GtkPixcomm *dst, struct city *pcity,
			    enum citizen_feeling index)
{
  enum citizen_category citizens[MAX_CITY_SIZE];
  int i;
  int num_citizens = get_city_citizen_types(pcity, index, citizens);
  int offset = MIN(tileset_small_sprite_width(tileset), PIXCOMM_WIDTH / num_citizens);

  gtk_pixcomm_freeze(dst);
  gtk_pixcomm_clear(dst);

  for (i = 0; i < num_citizens; i++) {
    gtk_pixcomm_copyto(dst, get_citizen_sprite(tileset, citizens[i], i, pcity),
		       i * offset, 0);
  }

  gtk_pixcomm_thaw(dst);
}

/**************************************************************************
...
**************************************************************************/
void refresh_happiness_dialog(struct city *pcity)
{
  int i;
  struct happiness_dialog *pdialog = get_happiness_dialog(pcity);

  for (i = 0; i < FEELING_LAST; i++) {
    refresh_pixcomm(GTK_PIXCOMM(pdialog->hpixmaps[i]), pdialog->pcity, i);
  }

  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[CITIES]),
		     text_happiness_cities(pdialog->pcity));
  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[LUXURIES]),
		     text_happiness_luxuries(pdialog->pcity));
  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[BUILDINGS]),
		     text_happiness_buildings(pdialog->pcity));
  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[UNITS]),
		     text_happiness_units(pdialog->pcity));
  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[WONDERS]),
		     text_happiness_wonders(pdialog->pcity));
}

/**************************************************************************
...
**************************************************************************/
void close_happiness_dialog(struct city *pcity)
{
  struct happiness_dialog *pdialog = get_happiness_dialog(pcity);
  if (pdialog == NULL) {
    /* City which is being investigated doesn't contain happiness tab */
    return;
  }

  gtk_widget_hide(pdialog->shell);

  dialog_list_unlink(dialog_list, pdialog);

  gtk_widget_destroy(pdialog->shell);
  free(pdialog);
}

/**************************************************************************
...
**************************************************************************/
GtkWidget *get_top_happiness_display(struct city *pcity)
{
  return create_happiness_dialog(pcity)->shell;
}
