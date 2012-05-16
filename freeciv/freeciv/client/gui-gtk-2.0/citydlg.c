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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "city.h"
#include "fcintl.h"
#include "game.h"
#include "map.h"
#include "mem.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "shared.h"
#include "support.h"
#include "unitlist.h"

#include "cityrep.h"
#include "client_main.h"
#include "cma_fe.h"
#include "cma_fec.h" 
#include "colors.h"
#include "control.h"
#include "climap.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "happiness.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapview.h"
#include "options.h"
#include "repodlgs.h"
#include "tilespec.h"
#include "wldlg.h"
#include "log.h"
#include "text.h"

#include "citydlg.h"

#define CITYMAP_WIDTH MIN(300, canvas_width)
#define CITYMAP_HEIGHT (CITYMAP_WIDTH * canvas_height / canvas_width)
#define CITYMAP_SCALE ((double)CITYMAP_WIDTH / (double)canvas_width)

struct city_dialog;

/* get 'struct dialog_list' and related function */
#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct city_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct city_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

struct unit_node {
  GtkWidget *cmd;
  GtkWidget *pix;
};

/* get 'struct unit_node' and related function */
#define SPECVEC_TAG unit_node
#define SPECVEC_TYPE struct unit_node
#include "specvec.h"

#define unit_node_vector_iterate(list, elt) \
    TYPED_VECTOR_ITERATE(struct unit_node, list, elt)
#define unit_node_vector_iterate_end  VECTOR_ITERATE_END

enum { OVERVIEW_PAGE, WORKLIST_PAGE,
  HAPPINESS_PAGE, CMA_PAGE, TRADE_PAGE, MISC_PAGE
};

enum info_style { NORMAL, ORANGE, RED, NUM_INFO_STYLES };

#define NUM_CITIZENS_SHOWN 23
#define NUM_INFO_FIELDS 12      /* number of fields in city_info */
#define NUM_PAGES 6             /* the number of pages in city dialog notebook 
                                 * (+1) if you change this, you must add an
                                 * entry to misc_whichtab_label[] */

static int citydialog_width, citydialog_height;

struct city_dialog {
  struct city *pcity;

  GtkWidget *shell;
  GtkWidget *name_label;
  GtkWidget *citizen_pixmap;
  GdkPixbuf *map_canvas_store, *map_pixbuf_unscaled;
  GdkPixmap *map_canvas_store_unscaled;
  GtkWidget *notebook;

  GtkTooltips *tips;
  GtkWidget *popup_menu;

  struct {
    GtkWidget *map_canvas;
    GtkWidget *map_canvas_pixmap;
    GtkWidget *production_bar;
    GtkWidget *production_combo;
    GtkWidget *buy_command;
    GtkWidget *improvement_list;

    GtkWidget *supported_units_frame;
    GtkWidget *supported_unit_table;

    GtkWidget *present_units_frame;
    GtkWidget *present_unit_table;

    struct unit_node_vector supported_units;
    struct unit_node_vector present_units;

    GtkWidget *info_ebox[NUM_INFO_FIELDS];
    GtkWidget *info_label[NUM_INFO_FIELDS];
    
    GtkListStore* change_production_store;
  } overview;
 
  struct { 
    GtkWidget *production_bar;
    GtkWidget *buy_command;
    GtkWidget *worklist;
  } production;

  struct {
    GtkWidget *map_canvas;
    GtkWidget *map_canvas_pixmap;
    GtkWidget *widget;
    GtkWidget *info_ebox[NUM_INFO_FIELDS];
    GtkWidget *info_label[NUM_INFO_FIELDS];
  } happiness;

  struct cma_dialog *cma_editor;

  struct {
    GtkWidget *rename_command;
    GtkWidget *new_citizens_radio[3];
    GtkWidget *disband_on_settler;
    GtkWidget *whichtab_radio[NUM_PAGES];
    short block_signal;
  } misc;

  GtkWidget *buy_shell, *sell_shell;
  GtkWidget *change_shell;
  GtkTreeSelection *change_selection;
  GtkWidget *rename_shell, *rename_input;

  GtkWidget *show_units_command;
  GtkWidget *prev_command, *next_command;

  Impr_type_id sell_id;

  int cwidth;
};

static GtkRcStyle *info_label_style[NUM_INFO_STYLES] = { NULL, NULL, NULL };

static struct dialog_list *dialog_list;
static bool city_dialogs_have_been_initialised;
static int canvas_width, canvas_height;
static int new_dialog_def_page = OVERVIEW_PAGE;
static int last_page = OVERVIEW_PAGE;

/****************************************/

static void initialize_city_dialogs(void);

static struct city_dialog *get_city_dialog(struct city *pcity);
static gboolean keyboard_handler(GtkWidget * widget, GdkEventKey * event,
				 struct city_dialog *pdialog);

static GtkWidget *create_city_info_table(struct city_dialog *pdialog,
    					 GtkWidget **info_ebox,
					 GtkWidget **info_label);
static void create_and_append_overview_page(struct city_dialog *pdialog);
static void create_and_append_worklist_page(struct city_dialog *pdialog);
static void create_and_append_happiness_page(struct city_dialog *pdialog);
static void create_and_append_cma_page(struct city_dialog *pdialog);
static void create_and_append_settings_page(struct city_dialog *pdialog);

static struct city_dialog *create_city_dialog(struct city *pcity);

static void city_dialog_update_title(struct city_dialog *pdialog);
static void city_dialog_update_citizens(struct city_dialog *pdialog);
static void city_dialog_update_information(GtkWidget **info_ebox,
					   GtkWidget **info_label,
                                           struct city_dialog *pdialog);
static void city_dialog_update_map(struct city_dialog *pdialog);
static void city_dialog_update_building(struct city_dialog *pdialog);
static void city_dialog_update_improvement_list(struct city_dialog
						*pdialog);
static void city_dialog_update_supported_units(struct city_dialog
					       *pdialog);
static void city_dialog_update_present_units(struct city_dialog *pdialog);
static void city_dialog_update_prev_next(void);

static void show_units_callback(GtkWidget * w, gpointer data);

static gboolean supported_unit_callback(GtkWidget * w, GdkEventButton * ev,
				        gpointer data);
static gboolean present_unit_callback(GtkWidget * w, GdkEventButton * ev,
				      gpointer data);
static gboolean supported_unit_middle_callback(GtkWidget * w,
					       GdkEventButton * ev,
					       gpointer data);
static gboolean present_unit_middle_callback(GtkWidget * w,
					     GdkEventButton * ev,
					     gpointer data);

static void unit_center_callback(GtkWidget * w, gpointer data);
static void unit_activate_callback(GtkWidget * w, gpointer data);
static void supported_unit_activate_close_callback(GtkWidget * w,
						   gpointer data);
static void present_unit_activate_close_callback(GtkWidget * w,
						 gpointer data);
static void unit_load_callback(GtkWidget * w, gpointer data);
static void unit_unload_callback(GtkWidget * w, gpointer data);
static void unit_sentry_callback(GtkWidget * w, gpointer data);
static void unit_fortify_callback(GtkWidget * w, gpointer data);
static void unit_disband_callback(GtkWidget * w, gpointer data);
static void unit_homecity_callback(GtkWidget * w, gpointer data);
static void unit_upgrade_callback(GtkWidget * w, gpointer data);

static gboolean citizens_callback(GtkWidget * w, GdkEventButton * ev,
			      gpointer data);
static gboolean button_down_citymap(GtkWidget * w, GdkEventButton * ev,
				    gpointer data);
static void draw_map_canvas(struct city_dialog *pdialog);

static void buy_callback(GtkWidget * w, gpointer data);
static void change_production_callback(GtkWidget* w, struct city_dialog*);

static void sell_callback(struct impr_type *pimprove, gpointer data);
static void sell_callback_response(GtkWidget *w, gint response, gpointer data);

static void impr_callback(GtkTreeView *view, GtkTreePath *path,
			  GtkTreeViewColumn *col, gpointer data);
static void switch_page_callback(GtkNotebook * notebook,
				 GtkNotebookPage * page, guint page_num,
				 gpointer data);

static void rename_callback(GtkWidget * w, gpointer data);
static gboolean rename_callback_delete(GtkWidget * widget, GdkEvent * event,
				       gpointer data);
static void rename_callback_no(GtkWidget * w, gpointer data);
static void rename_callback_yes(GtkWidget * w, gpointer data);
static void set_cityopt_values(struct city_dialog *pdialog);
static void cityopt_callback(GtkWidget * w, gpointer data);
static void misc_whichtab_callback(GtkWidget * w, gpointer data);

static void city_destroy_callback(GtkWidget *w, gpointer data);
static void close_city_dialog(struct city_dialog *pdialog);
static void close_callback(GtkWidget *w, gpointer data);
static void switch_city_callback(GtkWidget * w, gpointer data);

/****************************************************************
  Called to set the dimensions of the city dialog, both on
  startup and if the tileset is changed.
*****************************************************************/
static void init_citydlg_dimensions(void)
{
  canvas_width = get_citydlg_canvas_width();
  canvas_height = get_citydlg_canvas_height();
}

/****************************************************************
...
*****************************************************************/
static void initialize_city_dialogs(void)
{
  int i;
  GdkColor orange = { 0, 65535, 32768, 0 };	/* not currently used */
  GdkColor red = { 0, 65535, 0, 0 };

  assert(!city_dialogs_have_been_initialised);

  dialog_list = dialog_list_new();
  init_citydlg_dimensions();

  /* make the styles */

  for (i = 0; i < NUM_INFO_STYLES; i++) {
    info_label_style[i] = gtk_rc_style_new();
  }
  /* info_syle[NORMAL] is normal, don't change it */
  info_label_style[ORANGE]->color_flags[GTK_STATE_NORMAL] |= GTK_RC_FG;
  info_label_style[ORANGE]->fg[GTK_STATE_NORMAL] = orange;
  info_label_style[RED]->color_flags[GTK_STATE_NORMAL] |= GTK_RC_FG;
  info_label_style[RED]->fg[GTK_STATE_NORMAL] = red;

  city_dialogs_have_been_initialised = TRUE;
}

/****************************************************************
  Called when the tileset changes.
*****************************************************************/
void reset_city_dialogs(void)
{
  if (!city_dialogs_have_been_initialised) {
    return;
  }

  init_citydlg_dimensions();

  dialog_list_iterate(dialog_list, pdialog) {
    /* There's no reasonable way to resize a GtkPixcomm, so we don't try.
       Instead we just redraw the overview within the existing area.  The
       player has to close and reopen the dialog to fix this. */
    city_dialog_update_map(pdialog);
  } dialog_list_iterate_end;

  popdown_all_city_dialogs();
}

/****************************************************************
...
*****************************************************************/
static struct city_dialog *get_city_dialog(struct city *pcity)
{
  if (!city_dialogs_have_been_initialised)
    initialize_city_dialogs();

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pcity == pcity)
      return pdialog;
  }
  dialog_list_iterate_end;
  return NULL;
}

/****************************************************************
...
*****************************************************************/
void refresh_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog = get_city_dialog(pcity);

  if (city_owner(pcity) == client.conn.playing) {
    city_report_dialog_update_city(pcity);
    economy_report_dialog_update();
  }

  if (!pdialog)
    return;

  city_dialog_update_title(pdialog);
  city_dialog_update_citizens(pdialog);
  city_dialog_update_information(pdialog->overview.info_ebox,
				 pdialog->overview.info_label, pdialog);
  city_dialog_update_map(pdialog);
  city_dialog_update_building(pdialog);
  city_dialog_update_improvement_list(pdialog);
  city_dialog_update_supported_units(pdialog);
  city_dialog_update_present_units(pdialog);

  if (NULL == client.conn.playing
      || city_owner(pcity) == client.conn.playing) {
    bool have_present_units = (unit_list_size(pcity->tile->units) > 0);

    refresh_worklist(pdialog->production.worklist);

    city_dialog_update_information(pdialog->happiness.info_ebox,
				   pdialog->happiness.info_label, pdialog);
    refresh_happiness_dialog(pdialog->pcity);

    refresh_cma_dialog(pdialog->pcity, REFRESH_ALL);

    gtk_widget_set_sensitive(pdialog->show_units_command,
			     can_client_issue_orders() &&
			     have_present_units);
  } else {
    /* Set the buttons we do not want live while a Diplomat investigates */
    gtk_widget_set_sensitive(pdialog->show_units_command, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
void refresh_unit_city_dialogs(struct unit *punit)
{
  struct city *pcity_sup, *pcity_pre;
  struct city_dialog *pdialog;

  pcity_sup = game_find_city_by_number(punit->homecity);
  pcity_pre = tile_city(punit->tile);

  if (pcity_sup && (pdialog = get_city_dialog(pcity_sup)))
    city_dialog_update_supported_units(pdialog);

  if (pcity_pre && (pdialog = get_city_dialog(pcity_pre)))
    city_dialog_update_present_units(pdialog);
}

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;

  if (!(pdialog = get_city_dialog(pcity))) {
    pdialog = create_city_dialog(pcity);
  }

  gtk_window_present(GTK_WINDOW(pdialog->shell));
}

/****************************************************************
...
*****************************************************************/
bool city_dialog_is_open(struct city *pcity)
{
  return get_city_dialog(pcity) != NULL;
}

/****************************************************************
popdown the dialog 
*****************************************************************/
void popdown_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog = get_city_dialog(pcity);

  if (pdialog) {
    close_city_dialog(pdialog);
  }
}

/****************************************************************
popdown all dialogs
*****************************************************************/
void popdown_all_city_dialogs(void)
{
  if (!city_dialogs_have_been_initialised) {
    return;
  }
  while (dialog_list_size(dialog_list)) {
    close_city_dialog(dialog_list_get(dialog_list, 0));
  }
  dialog_list_free(dialog_list);
  city_dialogs_have_been_initialised = FALSE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean keyboard_handler(GtkWidget * widget, GdkEventKey * event,
				 struct city_dialog *pdialog)
{
  if (event->state & GDK_CONTROL_MASK) {
    switch (event->keyval) {
    case GDK_Left:
      gtk_notebook_prev_page(GTK_NOTEBOOK(pdialog->notebook));
      return TRUE;
      
    case GDK_Right:
      gtk_notebook_next_page(GTK_NOTEBOOK(pdialog->notebook));
      return TRUE;

    default:
      break;
    }
  }

  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean show_info_button_release(GtkWidget *w, GdkEventButton *ev,
					 gpointer data)
{
  gtk_grab_remove(w);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_widget_destroy(w);
  return FALSE;
}

enum { FIELD_FOOD, FIELD_SHIELD, FIELD_TRADE, FIELD_GOLD, FIELD_LUXURY,
       FIELD_SCIENCE, FIELD_GRANARY, FIELD_GROWTH, FIELD_CORRUPTION,
       FIELD_WASTE, FIELD_POLLUTION, FIELD_ILLNESS
};

/****************************************************************
...
*****************************************************************/
static gboolean show_info_popup(GtkWidget *w, GdkEventButton *ev,
    				gpointer data)
{
  struct city_dialog *pdialog = g_object_get_data(G_OBJECT(w), "pdialog");

  if (ev->button == 1) {
    GtkWidget *p, *label, *frame;
    char buf[1024];
    
    switch (GPOINTER_TO_UINT(data)) {
    case FIELD_FOOD:
      get_city_dialog_output_text(pdialog->pcity, O_FOOD, buf, sizeof(buf));
      break;
    case FIELD_SHIELD:
      get_city_dialog_output_text(pdialog->pcity, O_SHIELD,
				  buf, sizeof(buf));
      break;
    case FIELD_TRADE:
      get_city_dialog_output_text(pdialog->pcity, O_TRADE, buf, sizeof(buf));
      break;
    case FIELD_GOLD:
      get_city_dialog_output_text(pdialog->pcity, O_GOLD, buf, sizeof(buf));
      break;
    case FIELD_SCIENCE:
      get_city_dialog_output_text(pdialog->pcity, O_SCIENCE,
				  buf, sizeof(buf));
      break;
    case FIELD_LUXURY:
      get_city_dialog_output_text(pdialog->pcity, O_LUXURY,
				  buf, sizeof(buf));
      break;
    case FIELD_POLLUTION:
      get_city_dialog_pollution_text(pdialog->pcity, buf, sizeof(buf));
      break;
    case FIELD_ILLNESS:
      get_city_dialog_illness_text(pdialog->pcity, buf, sizeof(buf));
      break;
    default:
      return TRUE;
    }
    
    p = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_name(p, "Freeciv");
    gtk_container_set_border_width(GTK_CONTAINER(p), 2);
    gtk_window_set_position(GTK_WINDOW(p), GTK_WIN_POS_MOUSE);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(p), frame);

    label = gtk_label_new(buf);
    gtk_widget_set_name(label, "city_info_label");
    gtk_misc_set_padding(GTK_MISC(label), 4, 4);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_widget_show_all(p);

    gdk_pointer_grab(p->window, TRUE, GDK_BUTTON_RELEASE_MASK,
		     NULL, NULL, ev->time);
    gtk_grab_add(p);

    g_signal_connect_after(p, "button_release_event",
                           G_CALLBACK(show_info_button_release), NULL);
  }
  return TRUE;
}
/****************************************************************
 used once in the overview page and once in the happiness page
 **info_label points to the info_label in the respective struct
****************************************************************/
static GtkWidget *create_city_info_table(struct city_dialog *pdialog,
    					 GtkWidget **info_ebox,
					 GtkWidget **info_label)
{
  int i;
  GtkWidget *hbox, *table, *label, *ebox;

  static const char *output_label[NUM_INFO_FIELDS] = { N_("Food:"),
    N_("Prod:"),
    N_("Trade:"),
    N_("Gold:"),
    N_("Luxury:"),
    N_("Science:"),
    N_("Granary:"),
    N_("Change in:"),
    N_("Corruption:"),
    N_("Waste:"),
    N_("Pollution:"),
    N_("Plague Risk:")
  };
  static bool output_label_done;

  hbox = gtk_hbox_new(TRUE, 0);	/* to give the table padding inside the frame */

  table = gtk_table_new(NUM_INFO_FIELDS, 2, FALSE);
  gtk_table_set_row_spacing(GTK_TABLE(table), 2, 10);
  gtk_table_set_row_spacing(GTK_TABLE(table), 5, 10);
  gtk_table_set_row_spacing(GTK_TABLE(table), 7, 10);
  gtk_table_set_col_spacing(GTK_TABLE(table), 0, 5);
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);

  intl_slist(ARRAY_SIZE(output_label), output_label, &output_label_done);

  for (i = 0; i < NUM_INFO_FIELDS; i++) {
    label = gtk_label_new(output_label[i]);
    gtk_widget_set_name(label, "city_label");	/* for font style? */
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, i, i + 1, GTK_FILL, 0,
		     0, 0);

    ebox = gtk_event_box_new();
    g_object_set_data(G_OBJECT(ebox), "pdialog", pdialog);
    g_signal_connect(ebox, "button_press_event",
	G_CALLBACK(show_info_popup), GUINT_TO_POINTER(i));
    info_ebox[i] = ebox;

    label = gtk_label_new("");
    info_label[i] = label;
    gtk_widget_set_name(label, "city_label");	/* ditto */
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    gtk_container_add(GTK_CONTAINER(ebox), label);

    gtk_table_attach(GTK_TABLE(table), ebox, 1, 2, i, i + 1, GTK_FILL, 0,
		     0, 0);
  }

  gtk_widget_show_all(hbox);
  return hbox;
}

/****************************************************************
                  **** Overview page **** 
*****************************************************************/
static void create_and_append_overview_page(struct city_dialog *pdialog)
{
  GtkWidget *top, *vbox, *page, *align;
  GtkWidget *hbox, *ebox;
  GtkWidget *frame, *table, *label, *sw, *view, *bar, *production_combo;
  GtkCellRenderer *rend;
  GtkListStore *store;
  GtkListStore *production_store;
  const char *tab_title = _("_Overview");

  page = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);

  label = gtk_label_new_with_mnemonic(tab_title);

  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  /**** Citizens bar here ****/
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, TRUE, 0);

  ebox = gtk_event_box_new();
  gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
  gtk_box_pack_start(GTK_BOX(hbox), ebox, FALSE, FALSE, 0);
  pdialog->citizen_pixmap =
      gtk_pixcomm_new(tileset_small_sprite_width(tileset)
		      * (NUM_CITIZENS_SHOWN + 2),
		      tileset_small_sprite_height(tileset));
  gtk_misc_set_padding(GTK_MISC(pdialog->citizen_pixmap), 2, 2);
  gtk_container_add(GTK_CONTAINER(ebox), pdialog->citizen_pixmap);
  g_signal_connect(GTK_OBJECT(ebox), "button_press_event",
		     GTK_SIGNAL_FUNC(citizens_callback), pdialog);

  /* info/map/improvements */
  top = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(page), top, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Info"));
  gtk_box_pack_start(GTK_BOX(top), frame, FALSE, FALSE, 0);

  table = create_city_info_table(pdialog,
  				 pdialog->overview.info_ebox,
				 pdialog->overview.info_label);
  gtk_container_add(GTK_CONTAINER(frame), table);

  frame = gtk_frame_new(_("City map"));
  gtk_box_pack_start(GTK_BOX(top), frame, FALSE, FALSE, 0);

  align = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_container_add(GTK_CONTAINER(frame), align);

  pdialog->overview.map_canvas = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(align), pdialog->overview.map_canvas);
  gtk_widget_add_events(pdialog->overview.map_canvas, GDK_BUTTON_PRESS_MASK);

  pdialog->overview.map_canvas_pixmap
    = gtk_image_new_from_pixbuf(pdialog->map_canvas_store);
  gtk_container_add(GTK_CONTAINER(pdialog->overview.map_canvas),
		    pdialog->overview.map_canvas_pixmap);

  /* present and supported units (overview page) */
  pdialog->overview.supported_units_frame = gtk_frame_new("");
  gtk_box_pack_start(GTK_BOX(page), pdialog->overview.supported_units_frame,
		     FALSE, FALSE, 12);
  pdialog->overview.present_units_frame = gtk_frame_new("");
  gtk_box_pack_start(GTK_BOX(page), pdialog->overview.present_units_frame,
		     FALSE, FALSE, 0);


  /* supported units */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(pdialog->overview.supported_units_frame), sw);

  {
    int unit_height = tileset_tile_height(tileset) * 3 / 2;

    align = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
    gtk_widget_set_size_request(align, -1, unit_height);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), align);
  }

  table = gtk_table_new(0, 0, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);
  gtk_container_add(GTK_CONTAINER(align), table);
  
  gtk_container_set_focus_hadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(sw)));
  gtk_container_set_focus_vadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw)));
  
  pdialog->overview.supported_unit_table = table;
  unit_node_vector_init(&pdialog->overview.supported_units);


  /* present units */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(pdialog->overview.present_units_frame), sw);

  align = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
  gtk_widget_set_size_request(align, -1, tileset_full_tile_height(tileset));
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), align);

  table = gtk_table_new(0, 0, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);
  gtk_container_add(GTK_CONTAINER(align), table);
  
  gtk_container_set_focus_hadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(sw)));
  gtk_container_set_focus_vadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw)));
  
  pdialog->overview.present_unit_table = table;
  unit_node_vector_init(&pdialog->overview.present_units);
  

  /* start the right half of the page */
  vbox = gtk_vbox_new(FALSE, 4);	/* the right half */
  gtk_box_pack_start(GTK_BOX(top), vbox, TRUE, TRUE, 0);

  /* improvements */
  store = gtk_list_store_new(4, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			     G_TYPE_INT);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
  gtk_widget_set_name(view, "small_font");
  pdialog->overview.improvement_list = view;

  gtk_tooltips_set_tip(pdialog->tips,
		       view,
		       _("Press ENTER or double-click to sell an improvement."),
		       "");
    
  rend = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
      rend, "pixbuf", 1, NULL);
  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
      rend, "text", 2, NULL);
  rend = gtk_cell_renderer_text_new();
  g_object_set(rend, "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
      rend, "text", 3, NULL);

  g_signal_connect(view, "row_activated",
		   G_CALLBACK(impr_callback), pdialog);

  label = g_object_new(GTK_TYPE_LABEL,
		       "label", _("Production:"),
		       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);


  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  production_store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                        G_TYPE_INT);
  pdialog->overview.change_production_store = production_store;

  production_combo =
    gtk_combo_box_new_with_model(GTK_TREE_MODEL(production_store));
  pdialog->overview.production_combo = production_combo;
  gtk_box_pack_start(GTK_BOX(hbox), production_combo, TRUE, TRUE, 0);
  g_object_unref(production_store);
  g_signal_connect(production_combo, "changed",
                   G_CALLBACK(change_production_callback), pdialog);

  gtk_cell_layout_clear(GTK_CELL_LAYOUT(production_combo));
  rend = gtk_cell_renderer_pixbuf_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(production_combo), rend, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(production_combo),
                                 rend, "pixbuf", 0, NULL);
  g_object_set(rend, "xalign", 0.0, NULL);				 

  rend = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(production_combo), rend, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(production_combo),
                                 rend, "text", 1, NULL);

  bar = gtk_progress_bar_new();
  pdialog->overview.production_bar = bar;
  gtk_container_add(GTK_CONTAINER(production_combo), bar);
  gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(production_combo), 3);
  
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(bar), _("%d/%d %d turns"));

  pdialog->overview.buy_command = gtk_stockbutton_new(GTK_STOCK_EXECUTE,
						      _("_Buy"));
  gtk_box_pack_start(GTK_BOX(hbox), pdialog->overview.buy_command,
		     FALSE, FALSE, 0);
  g_signal_connect(pdialog->overview.buy_command, "clicked",
		   G_CALLBACK(buy_callback), pdialog);


  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", view,
		       "label", _("I_mprovements:"),
		       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER(sw), view);

  /* in terms of structural flow, should be put these above? */
  g_signal_connect(pdialog->overview.map_canvas,
		   "button_press_event",
		   G_CALLBACK(button_down_citymap), pdialog);

  gtk_widget_show_all(page);
}


/****************************************************************
...
*****************************************************************/
static void
target_drag_data_received(GtkWidget *w, GdkDragContext *context,
			  gint x, gint y, GtkSelectionData *data,
			  guint info, guint time, gpointer user_data)
{
  struct city_dialog *pdialog = (struct city_dialog *) user_data;
  GtkTreeModel *model;
  GtkTreePath *path;

  if (NULL != client.conn.playing
      && city_owner(pdialog->pcity) != client.conn.playing) {
    gtk_drag_finish(context, FALSE, FALSE, time);
  }
    
  if (gtk_tree_get_row_drag_data(data, &model, &path)) {
    GtkTreeIter it;

    if (gtk_tree_model_get_iter(model, &it, path)) {
      cid cid;
      gtk_tree_model_get(model, &it, 0, &cid, -1);
      city_change_production(pdialog->pcity, cid_production(cid));
      gtk_drag_finish(context, TRUE, FALSE, time);
    }
    gtk_tree_path_free(path);
  }

  gtk_drag_finish(context, FALSE, FALSE, time);
}

/****************************************************************
                    **** Production Page **** 
*****************************************************************/
static void create_and_append_worklist_page(struct city_dialog *pdialog)
{
  const char *tab_title = _("P_roduction");
  GtkWidget *label = gtk_label_new_with_mnemonic(tab_title);
  GtkWidget *page, *hbox, *editor, *bar;

  page = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);
  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  /* stuff that's being currently built */

  /* The label is set in city_dialog_update_building() */
  label = g_object_new(GTK_TYPE_LABEL,
		       "label", _("Production:"),
		       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 2);

  bar = gtk_progress_bar_new();
  pdialog->production.production_bar = bar;
  gtk_box_pack_start(GTK_BOX(hbox), bar, TRUE, TRUE, 0);
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(bar), _("%d/%d %d turns"));

  add_worklist_dnd_target(bar);
  g_signal_connect(bar, "drag_data_received",
		   G_CALLBACK(target_drag_data_received), pdialog);

  pdialog->production.buy_command = gtk_stockbutton_new(GTK_STOCK_EXECUTE,
						      _("_Buy"));
  gtk_box_pack_start(GTK_BOX(hbox), pdialog->production.buy_command,
		     FALSE, FALSE, 0);

  g_signal_connect(pdialog->production.buy_command, "clicked",
		   G_CALLBACK(buy_callback), pdialog);


  editor = create_worklist();
  reset_worklist(editor, &pdialog->pcity->worklist, pdialog->pcity);
  gtk_box_pack_start(GTK_BOX(page), editor, TRUE, TRUE, 6);
  pdialog->production.worklist = editor;

  gtk_widget_show_all(page);
}

/****************************************************************
                     **** Happiness Page **** 
*****************************************************************/
static void create_and_append_happiness_page(struct city_dialog *pdialog)
{
  GtkWidget *page, *vbox, *label, *table, *align;
  const char *tab_title = _("Happ_iness");

  page = gtk_hbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);

  label = gtk_label_new_with_mnemonic(tab_title);

  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  pdialog->happiness.widget = gtk_vbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(page), pdialog->happiness.widget, FALSE, FALSE,
		     0);

  gtk_box_pack_start(GTK_BOX(pdialog->happiness.widget),
		     get_top_happiness_display(pdialog->pcity), FALSE, FALSE,
		     0);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(page), vbox, FALSE, FALSE, 0);

  align = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_container_add(GTK_CONTAINER(vbox), align);

  pdialog->happiness.map_canvas = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(align), pdialog->happiness.map_canvas);
  gtk_widget_add_events(pdialog->happiness.map_canvas, GDK_BUTTON_PRESS_MASK);

  pdialog->happiness.map_canvas_pixmap
    = gtk_image_new_from_pixbuf(pdialog->map_canvas_store);
  gtk_container_add(GTK_CONTAINER(pdialog->happiness.map_canvas),
		    pdialog->happiness.map_canvas_pixmap);
  g_signal_connect(pdialog->happiness.map_canvas,
		   "button_press_event",
		   G_CALLBACK(button_down_citymap), pdialog);

  align = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_container_add(GTK_CONTAINER(vbox), align);
  
  table = create_city_info_table(pdialog,
				 pdialog->happiness.info_ebox,
				 pdialog->happiness.info_label);
  gtk_container_add(GTK_CONTAINER(align), table);

  gtk_widget_show_all(page);
}

/****************************************************************
            **** Citizen Management Agent (CMA) Page ****
*****************************************************************/
static void create_and_append_cma_page(struct city_dialog *pdialog)
{
  GtkWidget *page, *label;
  const char *tab_title = _("_Governor");

  page = gtk_vbox_new(FALSE, 0);

  label = gtk_label_new_with_mnemonic(tab_title);

  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  pdialog->cma_editor = create_cma_dialog(pdialog->pcity);
  gtk_box_pack_start(GTK_BOX(page), pdialog->cma_editor->shell, TRUE, TRUE, 0);

  gtk_widget_show(page);
}

/****************************************************************
                    **** Misc. Settings Page **** 
*****************************************************************/
static void create_and_append_settings_page(struct city_dialog *pdialog)
{
  int i;
  GtkWidget *vbox, *vbox2, *page, *frame, *label, *button;
  GtkSizeGroup *size;
  GSList *group;
  const char *tab_title = _("_Settings");

  static const char *new_citizens_label[] = {
    N_("Entertainers"),
    N_("Scientists"),
    N_("Taxmen")
  };

  static const char *disband_label = N_("Disband if build settler at size 1");

  static const char *misc_whichtab_label[NUM_PAGES] = {
    N_("Overview page"),
    N_("Production page"),
    N_("Happiness page"),
    N_("Governor page"),
    N_("This Settings page"),
    N_("Last active page")
  };

  static bool new_citizens_label_done;
  static bool misc_whichtab_label_done;

  /* initialize signal_blocker */
  pdialog->misc.block_signal = 0;


  page = gtk_table_new(2, 2, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(page), 18);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);
  
  size = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
  
  label = gtk_label_new_with_mnemonic(tab_title);

  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  vbox = gtk_vbox_new(FALSE, 12);
  gtk_table_attach(GTK_TABLE(page), vbox, 0, 1, 0, 1,
		   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_size_group_add_widget(size, vbox);

  /* new_citizens radio */
  frame = gtk_frame_new(_("New citizens are"));
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new(TRUE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);

  intl_slist(ARRAY_SIZE(new_citizens_label), new_citizens_label,
             &new_citizens_label_done);

  group = NULL;
  for (i = 0; i < ARRAY_SIZE(new_citizens_label); i++) {
    button = gtk_radio_button_new_with_mnemonic(group, new_citizens_label[i]);
    pdialog->misc.new_citizens_radio[i] = button;
    gtk_container_add(GTK_CONTAINER(vbox2), button);
    g_signal_connect(button, "toggled",
		     G_CALLBACK(cityopt_callback), pdialog);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
  }

  /* next is the next-time-open radio group in the right column */
  frame = gtk_frame_new(_("Next time open"));
  gtk_table_attach(GTK_TABLE(page), frame, 1, 2, 0, 1,
		   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_size_group_add_widget(size, frame);

  vbox2 = gtk_vbox_new(TRUE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);

  intl_slist(ARRAY_SIZE(misc_whichtab_label), misc_whichtab_label,
             &misc_whichtab_label_done);
  
  group = NULL;
  for (i = 0; i < ARRAY_SIZE(misc_whichtab_label); i++) {
    button = gtk_radio_button_new_with_mnemonic(group, misc_whichtab_label[i]);
    pdialog->misc.whichtab_radio[i] = button;
    gtk_container_add(GTK_CONTAINER(vbox2), button);
    g_signal_connect(button, "toggled",
		     G_CALLBACK(misc_whichtab_callback), GINT_TO_POINTER(i));
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
  }

  /* now we go back and fill the hbox rename */
  frame = gtk_frame_new(_("City"));
  gtk_table_attach(GTK_TABLE(page), frame, 0, 1, 1, 2,
		   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 12);

  vbox2 = gtk_vbox_new(TRUE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);

  button = gtk_button_new_with_mnemonic(_("R_ename..."));
  pdialog->misc.rename_command = button;
  gtk_container_add(GTK_CONTAINER(vbox2), button);
  g_signal_connect(button, "clicked",
		   G_CALLBACK(rename_callback), pdialog);

  gtk_widget_set_sensitive(button, can_client_issue_orders());
  
  /* the disband-if-size-1 button */
  button = gtk_check_button_new_with_mnemonic(_(disband_label));
  pdialog->misc.disband_on_settler = button;
  gtk_container_add(GTK_CONTAINER(vbox2), button);
  g_signal_connect(button, "toggled",
		   G_CALLBACK(cityopt_callback), pdialog);

  /* we choose which page to popup by default */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
			       (pdialog->
				misc.whichtab_radio[new_dialog_def_page]),
			       TRUE);

  set_cityopt_values(pdialog);

  gtk_widget_show_all(page);

  if (new_dialog_def_page == (NUM_PAGES - 1)) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(pdialog->notebook),
				  last_page);
  } else {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(pdialog->notebook),
				  new_dialog_def_page);
  }
}




/****************************************************************
...
*****************************************************************/
static struct city_dialog *create_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;

  GtkWidget *close_command;
  GtkWidget *vbox;

  if (!city_dialogs_have_been_initialised)
    initialize_city_dialogs();

  pdialog = fc_malloc(sizeof(struct city_dialog));
  pdialog->pcity = pcity;
  pdialog->change_shell = NULL;
  pdialog->buy_shell = NULL;
  pdialog->sell_shell = NULL;
  pdialog->rename_shell = NULL;
  pdialog->happiness.map_canvas = NULL;         /* make sure NULL if spy */
  pdialog->happiness.map_canvas_pixmap = NULL;  /* ditto */
  pdialog->map_canvas_store = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
					     CITYMAP_WIDTH, CITYMAP_HEIGHT);
  pdialog->map_pixbuf_unscaled = NULL;
  pdialog->map_canvas_store_unscaled
    = gdk_pixmap_new(root_window, canvas_width, canvas_height, -1);



  pdialog->shell = gtk_dialog_new_with_buttons(city_name(pcity),
	NULL,
  	0,
	NULL);
  setup_dialog(pdialog->shell, toplevel);
  gtk_window_set_role(GTK_WINDOW(pdialog->shell), "city");

  g_signal_connect(pdialog->shell, "destroy",
		   G_CALLBACK(city_destroy_callback), pdialog);
  gtk_window_set_position(GTK_WINDOW(pdialog->shell), GTK_WIN_POS_MOUSE);
  gtk_widget_set_name(pdialog->shell, "Freeciv");

  gtk_widget_realize(pdialog->shell);

  /* keep the icon of the executable on Windows (see PR#36491) */
#ifndef WIN32_NATIVE
  gtk_window_set_icon(GTK_WINDOW(pdialog->shell),
		sprite_get_pixbuf(get_icon_sprite(tileset, ICON_CITYDLG)));
#endif

  /* Set old size. The size isn't saved in any way. */
  if (citydialog_width && citydialog_height) {
    gtk_window_set_default_size(GTK_WINDOW(pdialog->shell),
				citydialog_width, citydialog_height);
  }

  pdialog->tips = gtk_tooltips_new();
  g_object_ref(pdialog->tips);
  gtk_object_sink(GTK_OBJECT(pdialog->tips));

  pdialog->popup_menu = gtk_menu_new();


  vbox = GTK_DIALOG(pdialog->shell)->vbox;

  pdialog->name_label = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(vbox), pdialog->name_label, FALSE, FALSE, 2);

  /**** -Start of Notebook- ****/

  pdialog->notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pdialog->notebook),
			   GTK_POS_BOTTOM);
  gtk_box_pack_start(GTK_BOX(vbox), pdialog->notebook, TRUE, TRUE, 0);

  create_and_append_overview_page(pdialog);
  create_and_append_worklist_page(pdialog);

  /* only create these tabs if not a spy */
  if (NULL == client.conn.playing
      || city_owner(pcity) == client.conn.playing) {
    create_and_append_happiness_page(pdialog);
    create_and_append_cma_page(pdialog);
  }

  if (city_owner(pcity) == client.conn.playing) {
    create_and_append_settings_page(pdialog);
  } else {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(pdialog->notebook),
				  OVERVIEW_PAGE);
  }

  g_signal_connect(pdialog->notebook, "switch-page",
		   G_CALLBACK(switch_page_callback), pdialog);

  /**** End of Notebook ****/

  /* bottom buttons */

  pdialog->show_units_command =
	gtk_button_new_with_mnemonic(_("_List present units..."));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(pdialog->shell)->action_area),
		    pdialog->show_units_command);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(GTK_DIALOG(pdialog->shell)->action_area),
      pdialog->show_units_command, TRUE);
  g_signal_connect(pdialog->show_units_command,
		   "clicked",
		   G_CALLBACK(show_units_callback), pdialog);

  pdialog->prev_command = gtk_stockbutton_new(GTK_STOCK_GO_BACK,
	_("_Prev city"));
  gtk_dialog_add_action_widget(GTK_DIALOG(pdialog->shell),
			       pdialog->prev_command, 1);

  pdialog->next_command = gtk_stockbutton_new(GTK_STOCK_GO_FORWARD,
	_("_Next city"));
  gtk_dialog_add_action_widget(GTK_DIALOG(pdialog->shell),
			       pdialog->next_command, 2);
  
  if (NULL == client.conn.playing
      || city_owner(pcity) != client.conn.playing) {
    gtk_widget_set_sensitive(pdialog->prev_command, FALSE);
    gtk_widget_set_sensitive(pdialog->next_command, FALSE);
  }

  close_command = gtk_dialog_add_button(GTK_DIALOG(pdialog->shell),
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  gtk_dialog_set_default_response(GTK_DIALOG(pdialog->shell),
	GTK_RESPONSE_CLOSE);

  g_signal_connect(close_command, "clicked",
		   G_CALLBACK(close_callback), pdialog);

  g_signal_connect(pdialog->prev_command, "clicked",
		   G_CALLBACK(switch_city_callback), pdialog);

  g_signal_connect(pdialog->next_command, "clicked",
		   G_CALLBACK(switch_city_callback), pdialog);

  /* some other things we gotta do */

  g_signal_connect(pdialog->shell, "key_press_event",
		   G_CALLBACK(keyboard_handler), pdialog);

  dialog_list_prepend(dialog_list, pdialog);

  refresh_city_dialog(pdialog->pcity);

  /* need to do this every time a new dialog is opened. */
  city_dialog_update_prev_next();

  gtk_widget_show_all(GTK_DIALOG(pdialog->shell)->vbox);
  gtk_widget_show_all(GTK_DIALOG(pdialog->shell)->action_area);

  gtk_window_set_focus(GTK_WINDOW(pdialog->shell), close_command);

  return pdialog;
}

/*********** Functions to update parts of the dialog ************/
/****************************************************************
...
*****************************************************************/
static void city_dialog_update_title(struct city_dialog *pdialog)
{
  char buf[512];
  const gchar *now;

  my_snprintf(buf, sizeof(buf), _("<b>%s</b> - %s citizens"),
	      city_name(pdialog->pcity),
	      population_to_text(city_population(pdialog->pcity)));

  if (city_unhappy(pdialog->pcity)) {
    mystrlcat(buf, _(" - DISORDER"), sizeof(buf));
  } else if (city_celebrating(pdialog->pcity)) {
    mystrlcat(buf, _(" - celebrating"), sizeof(buf));
  } else if (city_happy(pdialog->pcity)) {
    mystrlcat(buf, _(" - happy"), sizeof(buf));
  }

  now = gtk_label_get_text(GTK_LABEL(pdialog->name_label));
  if (strcmp(now, buf) != 0) {
    gtk_window_set_title(GTK_WINDOW(pdialog->shell), city_name(pdialog->pcity));
    gtk_label_set_markup(GTK_LABEL(pdialog->name_label), buf);
  }
}

/****************************************************************
...
*****************************************************************/
static void city_dialog_update_citizens(struct city_dialog *pdialog)
{
  enum citizen_category citizens[MAX_CITY_SIZE];
  int i, j, width;
  struct city *pcity = pdialog->pcity;
  int num_citizens = get_city_citizen_types(pcity, FEELING_FINAL, citizens);

  /* If there is not enough space we stack the icons. We draw from left to */
  /* right. width is how far we go to the right for each drawn pixmap. The */
  /* last icon is always drawn in full, and so has reserved                */
  /* tileset_small_sprite_width(tileset) pixels.                           */

  if (num_citizens > 1) {
    width = MIN(tileset_small_sprite_width(tileset),
		((NUM_CITIZENS_SHOWN - 1) * tileset_small_sprite_width(tileset)) /
		(num_citizens - 1));
  } else {
    width = tileset_small_sprite_width(tileset);
  }
  pdialog->cwidth = width;

  gtk_pixcomm_freeze(GTK_PIXCOMM(pdialog->citizen_pixmap));
  gtk_pixcomm_clear(GTK_PIXCOMM(pdialog->citizen_pixmap));

  i = 0;
  if (can_conn_edit(&client.conn)) {
    gtk_pixcomm_copyto(GTK_PIXCOMM(pdialog->citizen_pixmap),
                       get_arrow_sprite(tileset, ARROW_PLUS),
		       i++ * width, 0);
    gtk_pixcomm_copyto(GTK_PIXCOMM(pdialog->citizen_pixmap),
                       get_arrow_sprite(tileset, ARROW_MINUS),
		       i++ * width, 0);
  }
  for (j = 0; j < num_citizens; i++, j++) {
    gtk_pixcomm_copyto(GTK_PIXCOMM(pdialog->citizen_pixmap),
		       get_citizen_sprite(tileset, citizens[j], j, pcity),
		       i * width, 0);
  }

  gtk_pixcomm_thaw(GTK_PIXCOMM(pdialog->citizen_pixmap));

/*  gtk_widget_set_sensitive(pdialog->citizen_pixmap,*/
/*                           !cma_is_city_under_agent(pcity, NULL));*/
}
/****************************************************************
...
*****************************************************************/
static void city_dialog_update_information(GtkWidget **info_ebox,
					   GtkWidget **info_label,
                                           struct city_dialog *pdialog)
{
  int i, style;
  char buf[NUM_INFO_FIELDS][512];
  struct city *pcity = pdialog->pcity;
  int granaryturns;

  enum { FOOD, SHIELD, TRADE, GOLD, LUXURY, SCIENCE, 
	 GRANARY, GROWTH, CORRUPTION, WASTE, POLLUTION, ILLNESS
  };

  /* fill the buffers with the necessary info */

  my_snprintf(buf[FOOD], sizeof(buf[FOOD]), "%2d (%+2d)",
	      pcity->prod[O_FOOD], pcity->surplus[O_FOOD]);
  my_snprintf(buf[SHIELD], sizeof(buf[SHIELD]), "%2d (%+2d)",
	      pcity->prod[O_SHIELD] + pcity->waste[O_SHIELD],
	      pcity->surplus[O_SHIELD]);
  my_snprintf(buf[TRADE], sizeof(buf[TRADE]), "%2d (%+2d)",
	      pcity->surplus[O_TRADE] + pcity->waste[O_TRADE],
	      pcity->surplus[O_TRADE]);
  my_snprintf(buf[GOLD], sizeof(buf[GOLD]), "%2d (%+2d)",
	      pcity->prod[O_GOLD], pcity->surplus[O_GOLD]);
  my_snprintf(buf[LUXURY], sizeof(buf[LUXURY]), "%2d      ",
	      pcity->prod[O_LUXURY]);

  my_snprintf(buf[SCIENCE], sizeof(buf[SCIENCE]), "%2d",
	      pcity->prod[O_SCIENCE]);

  my_snprintf(buf[GRANARY], sizeof(buf[GRANARY]), "%d/%-d",
	      pcity->food_stock, city_granary_size(pcity->size));
	
  granaryturns = city_turns_to_grow(pcity);
  if (granaryturns == 0) {
    my_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("blocked"));
  } else if (granaryturns == FC_INFINITY) {
    my_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("never"));
  } else {
    /* A negative value means we'll have famine in that many turns.
       But that's handled down below. */
    my_snprintf(buf[GROWTH], sizeof(buf[GROWTH]),
		PL_("%d turn", "%d turns", abs(granaryturns)),
		abs(granaryturns));
  }
  my_snprintf(buf[CORRUPTION], sizeof(buf[CORRUPTION]), "%4d",
              pcity->waste[O_TRADE]);
  my_snprintf(buf[WASTE], sizeof(buf[WASTE]), "%4d",
              pcity->waste[O_SHIELD]);
  my_snprintf(buf[POLLUTION], sizeof(buf[POLLUTION]), "%4d",
              pcity->pollution);
  if (!game.info.illness_on) {
    my_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), " -.-");
  } else {
    my_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), "%4.1f",
                ((float)pcity->illness / 10.0));
  }

  /* stick 'em in the labels */

  for (i = 0; i < NUM_INFO_FIELDS; i++) {
    gtk_label_set_text(GTK_LABEL(info_label[i]), buf[i]);
  }

  /* 
   * Special style stuff for granary, growth and pollution below. The
   * "4" below is arbitrary. 3 turns should be enough of a warning.
   */
  style = (granaryturns > -4 && granaryturns < 0) ? RED : NORMAL;
  gtk_widget_modify_style(info_label[GRANARY], info_label_style[style]);

  style = (granaryturns == 0 || pcity->surplus[O_FOOD] < 0) ? RED : NORMAL;
  gtk_widget_modify_style(info_label[GROWTH], info_label_style[style]);

  /* someone could add the info_label_style[ORANGE]
   * style for better granularity here */

  style = (pcity->pollution >= 10) ? RED : NORMAL;
  gtk_widget_modify_style(info_label[POLLUTION], info_label_style[style]);

  style = (pcity->illness >= 20) ? RED : NORMAL;
  gtk_widget_modify_style(info_label[ILLNESS], info_label_style[style]);
}

/****************************************************************
...
*****************************************************************/
static void city_dialog_update_map(struct city_dialog *pdialog)
{
  struct canvas store = {
    .type = CANVAS_PIXMAP,
    .v = {.pixmap = pdialog->map_canvas_store_unscaled}
  };

  /* The drawing is done in three steps.
   *   1.  First we render to a pixmap with the appropriate canvas size.
   *   2.  Then the pixmap is rendered into a pixbuf of equal size.
   *   3.  Finally this pixbuf is composited and scaled onto the GtkImage's
   *       target pixbuf.
   */

  city_dialog_redraw_map(pdialog->pcity, &store);
  pdialog->map_pixbuf_unscaled
    = gdk_pixbuf_get_from_drawable(pdialog->map_pixbuf_unscaled,
				   pdialog->map_canvas_store_unscaled,
				   NULL,
				   0, 0, 0, 0,
				   canvas_width, canvas_height);
  gdk_pixbuf_composite(pdialog->map_pixbuf_unscaled,
		       pdialog->map_canvas_store,
		       0, 0, CITYMAP_WIDTH, CITYMAP_HEIGHT,
		       0.0, 0.0,
		       (double)CITYMAP_WIDTH / (double)canvas_width,
		       (double)CITYMAP_HEIGHT / (double)canvas_height,
		       GDK_INTERP_BILINEAR, 255);

  /* draw to real window */
  draw_map_canvas(pdialog);

  if(cma_is_city_under_agent(pdialog->pcity, NULL)) {
    gtk_widget_set_sensitive(pdialog->overview.map_canvas, FALSE);
    if (pdialog->happiness.map_canvas) {
      gtk_widget_set_sensitive(pdialog->happiness.map_canvas, FALSE);
    }
  } else {
    gtk_widget_set_sensitive(pdialog->overview.map_canvas, TRUE);
    if (pdialog->happiness.map_canvas) {
      gtk_widget_set_sensitive(pdialog->happiness.map_canvas, TRUE);
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void city_dialog_update_building(struct city_dialog *pdialog)
{
  char buf[32], buf2[200];
  gdouble pct;

  GtkListStore* store;
  GtkTreeIter iter;
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  int targets_used, item;
  struct city *pcity = pdialog->pcity;
  gboolean sensitive = city_can_buy(pcity);
  const char *descr = city_production_name_translation(pcity);
  int cost = city_production_build_shield_cost(pcity);

  gtk_widget_set_sensitive(pdialog->overview.buy_command, sensitive);
  gtk_widget_set_sensitive(pdialog->production.buy_command, sensitive);

  get_city_dialog_production(pcity, buf, sizeof(buf));

  if (cost > 0) {
    pct = (gdouble) pcity->shield_stock / (gdouble) cost;
    pct = CLAMP(pct, 0.0, 1.0);
  } else {
    pct = 1.0;
  }
  
  my_snprintf(buf2, sizeof(buf2), "%s%s: %s", descr,
	      worklist_is_empty(&pcity->worklist) ? "" : " (+)",
	      buf);
  gtk_progress_bar_set_text(
    GTK_PROGRESS_BAR(pdialog->overview.production_bar), buf2);
  gtk_progress_bar_set_text(
    GTK_PROGRESS_BAR(pdialog->production.production_bar), buf2);
  gtk_progress_bar_set_fraction(
    GTK_PROGRESS_BAR(pdialog->overview.production_bar), pct);
  gtk_progress_bar_set_fraction(
    GTK_PROGRESS_BAR(pdialog->production.production_bar), pct);
    
  store = pdialog->overview.change_production_store;  
  gtk_combo_box_set_active(GTK_COMBO_BOX(pdialog->overview.production_combo),
                           -1);
  gtk_list_store_clear(store);

  targets_used
    = collect_eventually_buildable_targets(targets, pdialog->pcity, FALSE);  
  name_and_sort_items(targets, targets_used, items, FALSE, pcity);

  for (item = 0; item < targets_used; item++) {
    if (can_city_build_now(pcity, items[item].item)) {
      const char* name;
      struct sprite* sprite;
      struct universal target = items[item].item;

      if (VUT_UTYPE == target.kind) {
	name = utype_name_translation(target.value.utype);
	sprite = get_unittype_sprite(tileset, target.value.utype);
      } else {
	name = improvement_name_translation(target.value.building);
	sprite = get_building_sprite(tileset, target.value.building);
      }
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter, 0, sprite_get_pixbuf(sprite),
                         1, name, 2, (gint)cid_encode(items[item].item), -1);
    }
  }

  /* work around GTK+ refresh bug. */
  gtk_widget_queue_resize(pdialog->overview.production_bar);
  gtk_widget_queue_resize(pdialog->production.production_bar);
}

/****************************************************************
...
*****************************************************************/
static void city_dialog_update_improvement_list(struct city_dialog *pdialog)
{
  int total, item, targets_used;
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  GtkTreeModel *model;
  GtkListStore *store;

  model =
    gtk_tree_view_get_model(GTK_TREE_VIEW(pdialog->overview.improvement_list));
  store = GTK_LIST_STORE(model);
  
  targets_used = collect_already_built_targets(targets, pdialog->pcity);
  name_and_sort_items(targets, targets_used, items, FALSE, pdialog->pcity);

  gtk_list_store_clear(store);  

  total = 0;
  for (item = 0; item < targets_used; item++) {
    GtkTreeIter it;
    int upkeep;
    struct sprite *sprite;
    struct universal target = items[item].item;

    assert(VUT_IMPROVEMENT == target.kind);
    /* This takes effects (like Adam Smith's) into account. */
    upkeep = city_improvement_upkeep(pdialog->pcity, target.value.building);
    sprite = get_building_sprite(tileset, target.value.building);

    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it,
		       0, target.value.building,
		       1, sprite_get_pixbuf(sprite),
	2, items[item].descr,
	3, upkeep,
	-1);

    total += upkeep;
  }
}

/****************************************************************
...
*****************************************************************/
static void city_dialog_update_supported_units(struct city_dialog *pdialog)
{
  struct unit_list *units;
  struct unit_node_vector *nodes;
  int n, m, i;
  char buf[30];
  int free_unhappy = get_city_bonus(pdialog->pcity, EFT_MAKE_CONTENT_MIL);

  if (NULL != client.conn.playing
      && city_owner(pdialog->pcity) != client.conn.playing) {
    units = pdialog->pcity->info_units_supported;
  } else {
    units = pdialog->pcity->units_supported;
  }

  nodes = &pdialog->overview.supported_units;

  n = unit_list_size(units);
  m = unit_node_vector_size(nodes);

  gtk_table_resize(GTK_TABLE(pdialog->overview.supported_unit_table),
		   1, MAX(n, 1));

  if (m > n) {
    i = 0;
    unit_node_vector_iterate(nodes, elt) {
      if (i++ >= n) {
	gtk_widget_destroy(elt->cmd);
      }
    } unit_node_vector_iterate_end;

    unit_node_vector_reserve(nodes, n);
  } else {
    for (i = m; i < n; i++) {
      GtkWidget *cmd, *pix;
      struct unit_node node;

      int unit_height = tileset_tile_height(tileset) * 3 / 2;

      cmd = gtk_button_new();
      node.cmd = cmd;

      gtk_button_set_relief(GTK_BUTTON(cmd), GTK_RELIEF_NONE);
      gtk_widget_add_events(cmd,
	  GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

      pix = gtk_pixcomm_new(CITYMAP_SCALE * tileset_full_tile_width(tileset),
			    CITYMAP_SCALE * unit_height);
      if (CITYMAP_WIDTH != canvas_width) {
	gtk_pixcomm_set_scale(GTK_PIXCOMM(pix), CITYMAP_SCALE);
      }
      node.pix = pix;

      gtk_container_add(GTK_CONTAINER(cmd), pix);

      gtk_table_attach_defaults(
	  GTK_TABLE(pdialog->overview.supported_unit_table),
	  cmd, i, i+1, 0, 1);
      unit_node_vector_append(nodes, &node);
    }
  }

  gtk_tooltips_disable(pdialog->tips);

  i = 0;
  unit_list_iterate(units, punit) {
    struct unit_node *pnode;
    int happy_cost = city_unit_unhappiness(punit, &free_unhappy);

    pnode = unit_node_vector_get(nodes, i);
    if (pnode) {
      GtkWidget *cmd, *pix;

      cmd = pnode->cmd;
      pix = pnode->pix;

      gtk_pixcomm_freeze(GTK_PIXCOMM(pix));
      put_unit_gpixmap(punit, GTK_PIXCOMM(pix));
      put_unit_gpixmap_city_overlays(punit, GTK_PIXCOMM(pix), punit->upkeep,
                                     happy_cost);
      gtk_pixcomm_thaw(GTK_PIXCOMM(pix));

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, supported_unit_callback, NULL);

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, supported_unit_middle_callback, NULL);

      gtk_tooltips_set_tip(pdialog->tips,
	  cmd, unit_description(punit), "");

      g_signal_connect(cmd, "button_press_event",
	  G_CALLBACK(supported_unit_callback),
	  GINT_TO_POINTER(punit->id));

      g_signal_connect(cmd, "button_release_event",
	  G_CALLBACK(supported_unit_middle_callback),
	  GINT_TO_POINTER(punit->id));

      if (city_owner(pdialog->pcity) != client.conn.playing) {
	gtk_widget_set_sensitive(cmd, FALSE);
      } else {
	gtk_widget_set_sensitive(cmd, TRUE);
      }

      gtk_widget_show(pix);
      gtk_widget_show(cmd);
    }
    i++;
  } unit_list_iterate_end;

  gtk_tooltips_enable(pdialog->tips);


  my_snprintf(buf, sizeof(buf), _("Supported units %d"), n);
  gtk_frame_set_label(GTK_FRAME(pdialog->overview.supported_units_frame), buf);
}

/****************************************************************
...
*****************************************************************/
static void city_dialog_update_present_units(struct city_dialog *pdialog)
{
  struct unit_list *units;
  struct unit_node_vector *nodes;
  int n, m, i;
  char buf[30];

  if (NULL != client.conn.playing
      && city_owner(pdialog->pcity) != client.conn.playing) {
    units = pdialog->pcity->info_units_present;
  } else {
    units = pdialog->pcity->tile->units;
  }

  nodes = &pdialog->overview.present_units;

  n = unit_list_size(units);
  m = unit_node_vector_size(nodes);

  gtk_table_resize(GTK_TABLE(pdialog->overview.present_unit_table),
		   1, MAX(n, 1));

  if (m > n) {
    i = 0;
    unit_node_vector_iterate(nodes, elt) {
      if (i++ >= n) {
	gtk_widget_destroy(elt->cmd);
      }
    } unit_node_vector_iterate_end;

    unit_node_vector_reserve(nodes, n);
  } else {
    for (i = m; i < n; i++) {
      GtkWidget *cmd, *pix;
      struct unit_node node;

      cmd = gtk_button_new();
      node.cmd = cmd;

      gtk_button_set_relief(GTK_BUTTON(cmd), GTK_RELIEF_NONE);
      gtk_widget_add_events(cmd,
	  GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

      pix = gtk_pixcomm_new(CITYMAP_SCALE * tileset_full_tile_width(tileset),
			    CITYMAP_SCALE * tileset_full_tile_height(tileset));
      if (CITYMAP_WIDTH != canvas_width) {
	gtk_pixcomm_set_scale(GTK_PIXCOMM(pix), CITYMAP_SCALE);
      }
      node.pix = pix;

      gtk_container_add(GTK_CONTAINER(cmd), pix);

      gtk_table_attach_defaults(
	  GTK_TABLE(pdialog->overview.present_unit_table),
	  cmd, i, i+1, 0, 1);
      unit_node_vector_append(nodes, &node);
    }
  }

  gtk_tooltips_disable(pdialog->tips);

  i = 0;
  unit_list_iterate(units, punit) {
    struct unit_node *pnode;
    
    pnode = unit_node_vector_get(nodes, i);
    if (pnode) {
      GtkWidget *cmd, *pix;

      cmd = pnode->cmd;
      pix = pnode->pix;

      put_unit_gpixmap(punit, GTK_PIXCOMM(pix));

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, present_unit_callback, NULL);

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, present_unit_middle_callback, NULL);

      gtk_tooltips_set_tip(pdialog->tips,
	  cmd, unit_description(punit), "");

      g_signal_connect(cmd, "button_press_event",
	  G_CALLBACK(present_unit_callback),
	  GINT_TO_POINTER(punit->id));

      g_signal_connect(cmd, "button_release_event",
	  G_CALLBACK(present_unit_middle_callback),
	  GINT_TO_POINTER(punit->id));

      if (city_owner(pdialog->pcity) != client.conn.playing) {
	gtk_widget_set_sensitive(cmd, FALSE);
      } else {
	gtk_widget_set_sensitive(cmd, TRUE);
      }

      gtk_widget_show(pix);
      gtk_widget_show(cmd);
    }
    i++;
  } unit_list_iterate_end;

  gtk_tooltips_enable(pdialog->tips);


  my_snprintf(buf, sizeof(buf), _("Present units %d"), n);
  gtk_frame_set_label(GTK_FRAME(pdialog->overview.present_units_frame), buf);
}

/****************************************************************
 updates the sensitivity of the the prev and next buttons.
 this does not need pdialog as a parameter, since it iterates
 over all the open dialogs.
 note: we still need the sensitivity code in create_city_dialog()
 for the spied dialogs.
*****************************************************************/
static void city_dialog_update_prev_next()
{
  int count = 0;
  int city_number;

  if (NULL != client.conn.playing) {
    city_number = city_list_size(client.conn.playing->cities);
  } else {
    city_number = FC_INFINITY; /* ? */
  }

  /* the first time, we see if all the city dialogs are open */

  dialog_list_iterate(dialog_list, pdialog) {
    if (city_owner(pdialog->pcity) == client.conn.playing)
      count++;
  }
  dialog_list_iterate_end;

  if (count == city_number) {	/* all are open, shouldn't prev/next */
    dialog_list_iterate(dialog_list, pdialog) {
      gtk_widget_set_sensitive(pdialog->prev_command, FALSE);
      gtk_widget_set_sensitive(pdialog->next_command, FALSE);
    }
    dialog_list_iterate_end;
  } else {
    dialog_list_iterate(dialog_list, pdialog) {
      if (city_owner(pdialog->pcity) == client.conn.playing) {
	gtk_widget_set_sensitive(pdialog->prev_command, TRUE);
	gtk_widget_set_sensitive(pdialog->next_command, TRUE);
      }
    }
    dialog_list_iterate_end;
  }
}

/****************************************************************
...
*****************************************************************/
static void show_units_callback(GtkWidget * w, gpointer data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;
  struct tile *ptile = pdialog->pcity->tile;

  if (unit_list_size(ptile->units))
    popup_unit_select_dialog(ptile);
}

/****************************************************************
...
*****************************************************************/
static void city_menu_position(GtkMenu *menu, gint *x, gint *y,
                               gboolean *push_in, gpointer data)
{
  GtkWidget *active;
  GtkWidget *widget;
  GtkRequisition requisition;
  gint xpos;
  gint ypos;
  gint width;

  g_return_if_fail(GTK_IS_BUTTON(data));

  widget = GTK_WIDGET(data);

  gtk_widget_get_child_requisition(GTK_WIDGET(menu), &requisition);
  width = requisition.width;

  active = gtk_menu_get_active(menu);
  gdk_window_get_origin(widget->window, &xpos, &ypos);

  xpos += widget->allocation.x;
  ypos += widget->allocation.y;

  *x = xpos;
  *y = ypos;
  *push_in = TRUE;
}

/****************************************************************
...
*****************************************************************/
static void destroy_func(GtkWidget *w, gpointer data)
{
  gtk_widget_destroy(w);
}

/****************************************************************
Pop-up menu to change attributes of supported units
*****************************************************************/
static gboolean supported_unit_callback(GtkWidget * w, GdkEventButton * ev,
				        gpointer data)
{
  GtkWidget *menu, *item;
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t) data);

  if (NULL != punit
   && NULL != (pcity = game_find_city_by_number(punit->homecity))
   && NULL != (pdialog = get_city_dialog(pcity))) {

    if (ev->type != GDK_BUTTON_PRESS || ev->button == 2 || ev->button == 3
	|| !can_client_issue_orders()) {
      return FALSE;
    }

    menu = pdialog->popup_menu;

    gtk_menu_popdown(GTK_MENU(menu));
    gtk_container_foreach(GTK_CONTAINER(menu), destroy_func, NULL);

    item = gtk_menu_item_new_with_mnemonic(_("Cen_ter"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_center_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    
    item = gtk_menu_item_new_with_mnemonic(_("_Activate unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_activate_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic(_("Activate unit, _close dialog"));
    g_signal_connect(item, "activate",
      G_CALLBACK(supported_unit_activate_close_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic(_("_Disband unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_disband_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
      city_menu_position, w, ev->button, ev->time);


  }
  return TRUE;
}

/****************************************************************
Pop-up menu to change attributes of units, ex. change homecity.
*****************************************************************/
static gboolean present_unit_callback(GtkWidget * w, GdkEventButton * ev,
				      gpointer data)
{
  GtkWidget *menu, *item;
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t) data);

  if (NULL != punit
   && NULL != (pcity = tile_city(punit->tile))
   && NULL != (pdialog = get_city_dialog(pcity))) {

    if (ev->type != GDK_BUTTON_PRESS || ev->button == 2 || ev->button == 3
	|| !can_client_issue_orders()) {
      return FALSE;
    }

    menu = pdialog->popup_menu;

    gtk_menu_popdown(GTK_MENU(menu));
    gtk_container_foreach(GTK_CONTAINER(menu), destroy_func, NULL);

    item = gtk_menu_item_new_with_mnemonic(_("_Activate unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_activate_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic(_("Activate unit, _close dialog"));
    g_signal_connect(item, "activate",
      G_CALLBACK(present_unit_activate_close_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic(_("_Load unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_load_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if (!find_transporter_for_unit(punit)) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    item = gtk_menu_item_new_with_mnemonic(_("_Unload unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_unload_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if (!can_unit_unload(punit, game_find_unit_by_number(punit->transported_by))
        || !can_unit_exist_at_tile(punit, punit->tile)) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    item = gtk_menu_item_new_with_mnemonic(_("_Sentry unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_sentry_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if (punit->activity == ACTIVITY_SENTRY
	|| !can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    item = gtk_menu_item_new_with_mnemonic(_("_Fortify unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_fortify_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if (punit->activity == ACTIVITY_FORTIFYING
	|| !can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    item = gtk_menu_item_new_with_mnemonic(_("_Disband unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_disband_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic(_("Set _Home City"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_homecity_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_set_sensitive(item, can_unit_change_homecity_to(punit, pcity));

    item = gtk_menu_item_new_with_mnemonic(_("U_pgrade unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_upgrade_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if (!can_client_issue_orders()
	|| NULL == can_upgrade_unittype(client.conn.playing, unit_type(punit))) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
      city_menu_position, w, ev->button, ev->time);
  }
  return TRUE;
}

/****************************************************************
 if user middle-clicked on a unit, activate it and close dialog
*****************************************************************/
static gboolean present_unit_middle_callback(GtkWidget * w,
					     GdkEventButton * ev,
					     gpointer data)
{
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t) data);

  if (NULL != punit
   && NULL != (pcity = tile_city(punit->tile))
   && NULL != (pdialog = get_city_dialog(pcity))
   && can_client_issue_orders()) {

    if (ev->button == 3) {
      set_unit_focus(punit);
    } else if (ev->button == 2) {
      set_unit_focus(punit);
      close_city_dialog(pdialog);
    }
  }

  return TRUE;
}

/****************************************************************
 if user middle-clicked on a unit, activate it and close dialog
*****************************************************************/
static gboolean supported_unit_middle_callback(GtkWidget * w,
					       GdkEventButton * ev,
					       gpointer data)
{
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t) data);

  if (NULL != punit
   && NULL != (pcity = game_find_city_by_number(punit->homecity))
   && NULL != (pdialog = get_city_dialog(pcity))
   && can_client_issue_orders()) {

    if (ev->button == 3) {
      set_unit_focus(punit);
    } else if (ev->button == 2) {
      set_unit_focus(punit);
      close_city_dialog(pdialog);
    }
  }

  return TRUE;
}

/****************************************************************
...
*****************************************************************/
static void unit_center_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    center_tile_mapcanvas(punit->tile);
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_activate_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    set_unit_focus(punit);
  }
}

/****************************************************************
...
*****************************************************************/
static void supported_unit_activate_close_callback(GtkWidget * w,
						   gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    struct city *pcity =
      player_find_city_by_id(client.conn.playing, punit->homecity);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	close_city_dialog(pdialog);
      }
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void present_unit_activate_close_callback(GtkWidget * w,
						 gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    struct city *pcity = tile_city(punit->tile);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	close_city_dialog(pdialog);
      }
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_load_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_load(punit, NULL);
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_unload_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_unload(punit);
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_sentry_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_sentry(punit);
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_fortify_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_fortify(punit);
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_disband_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_disband(punit);
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_homecity_callback(GtkWidget * w, gpointer data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_change_homecity(punit);
  }
}

/****************************************************************
...
*****************************************************************/
static void unit_upgrade_callback(GtkWidget *w, gpointer data)
{
  struct unit_list *punits;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL == punit) {
    return;
  }

  punits = unit_list_new();
  unit_list_append(punits, punit);
  popup_upgrade_dialog(punits);
  unit_list_free(punits);
}

/*** Callbacks for citizen bar, map funcs that are not update ***/
/****************************************************************
Somebody clicked our list of citizens. If they clicked a specialist
then change the type of him, else do nothing.
*****************************************************************/
static gboolean citizens_callback(GtkWidget * w, GdkEventButton * ev,
			      gpointer data)
{
  struct city_dialog *pdialog = data;
  struct city *pcity = pdialog->pcity;
  int citnum, tlen, len;

  if (!can_client_issue_orders()) {
    return FALSE;
  }

  tlen = tileset_small_sprite_width(tileset);
  len = (pcity->size - 1) * pdialog->cwidth + tlen;
  if (ev->x > len) {
    return FALSE;		/* no citizen that far to the right */
  }
  citnum = MIN(pcity->size - 1, ev->x / pdialog->cwidth);

  city_rotate_specialist(pcity, citnum);

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean button_down_citymap(GtkWidget * w, GdkEventButton * ev,
				    gpointer data)
{
  struct city_dialog *pdialog = data;
  int canvas_x, canvas_y, city_x, city_y;

  if (!can_client_issue_orders()) {
    return FALSE;
  }

  canvas_x = ev->x * (double)canvas_width / (double)CITYMAP_WIDTH;
  canvas_y = ev->y * (double)canvas_height / (double)CITYMAP_HEIGHT;

  if (canvas_to_city_pos(&city_x, &city_y, canvas_x, canvas_y)) {
    city_toggle_worker(pdialog->pcity, city_x, city_y);
  }

  return TRUE;
}

/****************************************************************
...
*****************************************************************/
static void draw_map_canvas(struct city_dialog *pdialog)
{
  gtk_widget_queue_draw(pdialog->overview.map_canvas_pixmap);
  if (pdialog->happiness.map_canvas_pixmap) {	/* in case of spy */
    gtk_widget_queue_draw(pdialog->happiness.map_canvas_pixmap);
  }
}

/********* Callbacks for Buy, Change, Sell, Worklist ************/
/****************************************************************
...
*****************************************************************/
static void buy_callback_response(GtkWidget *w, gint response, gpointer data)
{
  struct city_dialog *pdialog = data;

  if (response == GTK_RESPONSE_YES) {
    city_buy_production(pdialog->pcity);
  }
  gtk_widget_destroy(w);
}

/****************************************************************
...
*****************************************************************/
static void buy_callback(GtkWidget *w, gpointer data)
{
  GtkWidget *shell;
  struct city_dialog *pdialog = data;
  const char *name = city_production_name_translation(pdialog->pcity);
  int value = city_production_buy_gold_cost(pdialog->pcity);

  if (!can_client_issue_orders()) {
    return;
  }

  if (value <= client.conn.playing->economic.gold) {
    shell = gtk_message_dialog_new(NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("Buy %s for %d gold?\nTreasury contains %d gold."),
        name, value, client.conn.playing->economic.gold);
    setup_dialog(shell, pdialog->shell);
    gtk_window_set_title(GTK_WINDOW(shell), _("Buy It!"));
    gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_NO);
    g_signal_connect(shell, "response", G_CALLBACK(buy_callback_response),
	pdialog);
    gtk_window_present(GTK_WINDOW(shell));
  } else {
    shell = gtk_message_dialog_new(NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
        _("%s costs %d gold.\nTreasury contains %d gold."),
        name, value, client.conn.playing->economic.gold);
    setup_dialog(shell, pdialog->shell);
    gtk_window_set_title(GTK_WINDOW(shell), _("Buy It!"));
    g_signal_connect(shell, "response", G_CALLBACK(gtk_widget_destroy),
      NULL);
    gtk_window_present(GTK_WINDOW(shell));
  }
}

/****************************************************************
...
*****************************************************************/
static void change_production_callback(GtkWidget* w,
                                       struct city_dialog* pdial)
{
    GtkTreeIter iter;
    cid cid;
    
    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(w), &iter)) {
      gtk_tree_model_get(
        GTK_TREE_MODEL(pdial->overview.change_production_store),
        &iter, 2, &cid, -1);
      city_change_production(pdial->pcity, cid_production(cid)); 
    }
}

/****************************************************************
...
*****************************************************************/
static void sell_callback(struct impr_type *pimprove, gpointer data)
{
  GtkWidget *shl;
  struct city_dialog *pdialog = (struct city_dialog *) data;
  pdialog->sell_id = improvement_number(pimprove);
  
  if (!can_client_issue_orders()) {
    return;
  }

  if (pdialog->pcity->did_sell
      || city_owner(pdialog->pcity) != client.conn.playing) {
    return;
  }
  
  if (!can_city_sell_building(pdialog->pcity, pimprove)) {
    return;
  }

  shl = gtk_message_dialog_new(NULL,
    GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_YES_NO,
    _("Sell %s for %d gold?"),
    city_improvement_name_translation(pdialog->pcity, pimprove),
    impr_sell_gold(pimprove));
  setup_dialog(shl, pdialog->shell);
  pdialog->sell_shell = shl;
  
  gtk_window_set_title(GTK_WINDOW(shl), _("Sell It!"));
  gtk_window_set_position(GTK_WINDOW(shl), GTK_WIN_POS_CENTER_ON_PARENT);

  g_signal_connect(shl, "response",
		   G_CALLBACK(sell_callback_response), pdialog);
  
  gtk_window_present(GTK_WINDOW(shl));
}

/****************************************************************
...
*****************************************************************/
static void sell_callback_response(GtkWidget *w, gint response, gpointer data)
{
  struct city_dialog *pdialog = data;

  if (response == GTK_RESPONSE_YES) {
    city_sell_improvement(pdialog->pcity, pdialog->sell_id);
  }
  gtk_widget_destroy(w);
  
  pdialog->sell_shell = NULL;
}

/****************************************************************
 this is here because it's closely related to the sell stuff
*****************************************************************/
static void impr_callback(GtkTreeView *view, GtkTreePath *path,
			  GtkTreeViewColumn *col, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  GdkModifierType mask;
  struct impr_type *pimprove;

  model = gtk_tree_view_get_model(view);

  if (!gtk_tree_model_get_iter(model, &it, path)) {
    return;
  }

  gtk_tree_model_get(model, &it, 0, &pimprove, -1);
  gdk_window_get_pointer(NULL, NULL, NULL, &mask);

  if (!(mask & GDK_CONTROL_MASK)) {
    sell_callback(pimprove, data);
  } else {
    if (is_great_wonder(pimprove)) {
      popup_help_dialog_typed(improvement_name_translation(pimprove), HELP_WONDER);
    } else {
      popup_help_dialog_typed(improvement_name_translation(pimprove), HELP_IMPROVEMENT);
    }
  }
}

/****************************************************************
 If switching away from worklist, we commit it.
*****************************************************************/
static void switch_page_callback(GtkNotebook * notebook,
				 GtkNotebookPage * page, guint page_num,
				 gpointer data)
{
}

/******* Callbacks for stuff on the Misc. Settings page *********/
/****************************************************************
...
*****************************************************************/
static void rename_callback(GtkWidget * w, gpointer data)
{
  struct city_dialog *pdialog;

  pdialog = (struct city_dialog *) data;

  pdialog->rename_shell = input_dialog_create(GTK_WINDOW(pdialog->shell),
					      /*"shellrenamecity" */
					      _("Rename City"),
					      _("What should we rename the city to?"),
					      city_name(pdialog->pcity),
					      G_CALLBACK(rename_callback_yes),
					      pdialog,
					      G_CALLBACK(rename_callback_no),
					      pdialog);

  g_signal_connect(pdialog->rename_shell, "delete_event",
		   G_CALLBACK(rename_callback_delete), data);
}

/****************************************************************
...
*****************************************************************/
static gboolean rename_callback_delete(GtkWidget * widget, GdkEvent * event,
				       gpointer data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;
  pdialog->rename_shell = NULL;
  return FALSE;
}

/****************************************************************
...
*****************************************************************/
static void rename_callback_no(GtkWidget * w, gpointer data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;

  if (pdialog) {
    pdialog->rename_shell = NULL;
  }

  input_dialog_destroy(w);
}

/****************************************************************
...
*****************************************************************/
static void rename_callback_yes(GtkWidget * w, gpointer data)
{
  struct city_dialog *pdialog = data;

  if (pdialog) {
    city_rename(pdialog->pcity, input_dialog_get_input(w));

    pdialog->rename_shell = NULL;
  }

  input_dialog_destroy(w);
}

/****************************************************************
 Sets which page will be set on reopen of dialog
*****************************************************************/
static void misc_whichtab_callback(GtkWidget * w, gpointer data)
{
  new_dialog_def_page = GPOINTER_TO_INT(data);
}

/**************************************************************************
City options callbacks
**************************************************************************/
static void cityopt_callback(GtkWidget * w, gpointer data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;

  if (!can_client_issue_orders()) {
    return;
  }

  if(!pdialog->misc.block_signal){
    struct city *pcity = pdialog->pcity;
    bv_city_options new_options;

    assert(CITYO_LAST == 3);

    BV_CLR_ALL(new_options);
    if (GTK_TOGGLE_BUTTON(pdialog->misc.disband_on_settler)->active) {
      BV_SET(new_options, CITYO_DISBAND);
    }
    if (GTK_TOGGLE_BUTTON(pdialog->misc.new_citizens_radio[1])->active) {
      BV_SET(new_options, CITYO_NEW_EINSTEIN);
    }
    if (GTK_TOGGLE_BUTTON(pdialog->misc.new_citizens_radio[2])->active) {
      BV_SET(new_options, CITYO_NEW_TAXMAN);
    }

    dsend_packet_city_options_req(&client.conn, pcity->id,new_options);
  }
}

/**************************************************************************
 refresh the city options (auto_[land, air, sea, helicopter] and 
 disband-is-size-1) in the misc page.
**************************************************************************/
static void set_cityopt_values(struct city_dialog *pdialog)
{
  struct city *pcity = pdialog->pcity;

  pdialog->misc.block_signal = 1;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pdialog->misc.disband_on_settler),
			       is_city_option_set(pcity, CITYO_DISBAND));

  if (is_city_option_set(pcity, CITYO_NEW_EINSTEIN)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (pdialog->misc.new_citizens_radio[1]), TRUE);
  } else if (is_city_option_set(pcity, CITYO_NEW_TAXMAN)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (pdialog->misc.new_citizens_radio[2]), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (pdialog->misc.new_citizens_radio[0]), TRUE);
  }
  pdialog->misc.block_signal = 0;
}

/*************** Callbacks for: Close, Prev, Next. **************/
/****************************************************************
...
*****************************************************************/
static void close_callback(GtkWidget *w, gpointer data)
{
  close_city_dialog((struct city_dialog *) data);
}

/****************************************************************
...
*****************************************************************/
static void city_destroy_callback(GtkWidget *w, gpointer data)
{
  struct city_dialog *pdialog;

  pdialog = (struct city_dialog *) data;

  gtk_widget_hide(pdialog->shell);

  close_happiness_dialog(pdialog->pcity);
  close_cma_dialog(pdialog->pcity);

  citydialog_height = pdialog->shell->allocation.height;
  citydialog_width = pdialog->shell->allocation.width;

  last_page =
      gtk_notebook_get_current_page(GTK_NOTEBOOK(pdialog->notebook));

  /* else this will be called NUM_PAGES times as the pages are destroyed */
  g_signal_handlers_disconnect_matched(pdialog->notebook,
				       G_SIGNAL_MATCH_FUNC,
				       0, 0, NULL, switch_page_callback,
				       pdialog);

  g_object_unref(pdialog->tips);

  if (pdialog->popup_menu)
    gtk_widget_destroy(pdialog->popup_menu);

  dialog_list_unlink(dialog_list, pdialog);

  unit_node_vector_free(&pdialog->overview.supported_units);
  unit_node_vector_free(&pdialog->overview.present_units);

  if (pdialog->buy_shell)
    gtk_widget_destroy(pdialog->buy_shell);
  if (pdialog->sell_shell)
    gtk_widget_destroy(pdialog->sell_shell);
  if (pdialog->rename_shell)
    gtk_widget_destroy(pdialog->rename_shell);

  g_object_unref(pdialog->map_canvas_store);
  if (pdialog->map_pixbuf_unscaled) {
    g_object_unref(pdialog->map_pixbuf_unscaled);
  }

  unit_list_iterate(pdialog->pcity->info_units_supported, psunit) {
    free(psunit);
  }
  unit_list_iterate_end;

  unit_list_clear(pdialog->pcity->info_units_supported);

  unit_list_iterate(pdialog->pcity->info_units_present, psunit) {
    free(psunit);
  }
  unit_list_iterate_end;

  unit_list_clear(pdialog->pcity->info_units_present);

  free(pdialog);

  /* need to do this every time a new dialog is closed. */
  city_dialog_update_prev_next();
}


static void close_city_dialog(struct city_dialog *pdialog)
{
  gtk_widget_destroy(pdialog->shell);
}

/************************************************************************
  Callback for the prev/next buttons. Switches to the previous/next
  city.
*************************************************************************/
static void switch_city_callback(GtkWidget *w, gpointer data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;
  int i, j, dir, size;
  struct city *new_pcity = NULL;

  if (NULL == client.conn.playing) {
    return;
  }

  size = city_list_size(client.conn.playing->cities);

  assert(city_dialogs_have_been_initialised);
  assert(size >= 1);
  assert(city_owner(pdialog->pcity) == client.conn.playing);

  if (size == 1) {
    return;
  }

  /* dir = 1 will advance to the city, dir = -1 will get previous */
  if (w == pdialog->next_command) {
    dir = 1;
  } else if (w == pdialog->prev_command) {
    dir = -1;
  } else {
    assert(0);
    dir = 1;
  }

  for (i = 0; i < size; i++) {
    if (pdialog->pcity == city_list_get(client.conn.playing->cities, i)) {
      break;
    }
  }

  assert(i < size);

  for (j = 1; j < size; j++) {
    struct city *other_pcity = city_list_get(client.conn.playing->cities,
					     (i + dir * j + size) % size);
    struct city_dialog *other_pdialog = get_city_dialog(other_pcity);

    assert(other_pdialog != pdialog);
    if (!other_pdialog) {
      new_pcity = other_pcity;
      break;
    }
  }

  if (!new_pcity) {
    /* Every other city has an open city dialog. */
    return;
  }

  /* cleanup happiness dialog */
  close_happiness_dialog(pdialog->pcity);

  pdialog->pcity = new_pcity;

  /* reinitialize happiness, and cma dialogs */
  gtk_box_pack_start(GTK_BOX(pdialog->happiness.widget),
		     get_top_happiness_display(pdialog->pcity), TRUE, TRUE, 0);
  pdialog->cma_editor->pcity = new_pcity;

  reset_worklist(pdialog->production.worklist,
		 &pdialog->pcity->worklist, pdialog->pcity);

  can_slide = FALSE;
  center_tile_mapcanvas(pdialog->pcity->tile);
  can_slide = TRUE;
  set_cityopt_values(pdialog);	/* need not be in refresh_city_dialog */

  refresh_city_dialog(pdialog->pcity);
}
