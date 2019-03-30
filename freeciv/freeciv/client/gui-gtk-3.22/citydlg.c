/***********************************************************************
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
#include <fc_config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* utility */
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "unitlist.h"

/* client */
#include "chatline_common.h"
#include "client_main.h"
#include "colors.h"
#include "control.h"
#include "climap.h"
#include "options.h"
#include "text.h"
#include "tilespec.h"

/* client/agents */
#include "cma_fec.h" 

/* client/gui-gtk-3.22 */
#include "choice_dialog.h"
#include "citizensinfo.h"
#include "cityrep.h"
#include "cma_fe.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "happiness.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapview.h"
#include "repodlgs.h"
#include "wldlg.h"

#include "citydlg.h"

#define CITYMAP_WIDTH MIN(512, canvas_width)
#define CITYMAP_HEIGHT (CITYMAP_WIDTH * canvas_height / canvas_width)
#define CITYMAP_SCALE ((double)CITYMAP_WIDTH / (double)canvas_width)

#define TINYSCREEN_MAX_HEIGHT (500 - 1)

/* Only CDLGR_UNITS button currently uses these, others have
 * direct callback. */
enum citydlg_response { CDLGR_UNITS, CDLGR_PREV, CDLGR_NEXT };

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
  int height;
};

/* get 'struct unit_node' and related function */
#define SPECVEC_TAG unit_node
#define SPECVEC_TYPE struct unit_node
#include "specvec.h"

#define unit_node_vector_iterate(list, elt) \
    TYPED_VECTOR_ITERATE(struct unit_node, list, elt)
#define unit_node_vector_iterate_end  VECTOR_ITERATE_END

enum { OVERVIEW_PAGE, MAP_PAGE, BUILDINGS_PAGE, WORKLIST_PAGE,
  HAPPINESS_PAGE, CMA_PAGE, TRADE_PAGE, MISC_PAGE
};

#define NUM_CITIZENS_SHOWN 30
#define NUM_INFO_FIELDS 13      /* number of fields in city_info */
#define NUM_PAGES 6             /* the number of pages in city dialog notebook 
                                 * (+1) if you change this, you must add an
                                 * entry to misc_whichtab_label[] */

/* minimal size for the city map scrolling windows*/
#define CITY_MAP_MIN_SIZE_X  200
#define CITY_MAP_MIN_SIZE_Y  150

struct city_map_canvas {
  GtkWidget *sw;
  GtkWidget *ebox;
  GtkWidget *darea;
};

struct city_dialog {
  struct city *pcity;

  GtkWidget *shell;
  GtkWidget *name_label;
  cairo_surface_t *map_canvas_store_unscaled;
  GtkWidget *notebook;

  GtkWidget *popup_menu;
  GtkWidget *citizen_images;
  cairo_surface_t *citizen_surface;

  struct {
    struct city_map_canvas map_canvas;

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
    GtkWidget *production_label;
    GtkWidget *production_bar;
    GtkWidget *buy_command;
    GtkWidget *worklist;
  } production;

  struct {
    struct city_map_canvas map_canvas;

    GtkWidget *widget;
    GtkWidget *info_ebox[NUM_INFO_FIELDS];
    GtkWidget *info_label[NUM_INFO_FIELDS];
    GtkWidget *citizens;
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
  GtkTreeSelection *change_selection;
  GtkWidget *rename_shell, *rename_input;

  GtkWidget *show_units_command;
  GtkWidget *prev_command;
  GtkWidget *next_command;

  Impr_type_id sell_id;

  int cwidth;
};

static struct dialog_list *dialog_list;
static bool city_dialogs_have_been_initialised = FALSE;
static int canvas_width, canvas_height;
static int new_dialog_def_page = OVERVIEW_PAGE;
static int last_page = OVERVIEW_PAGE;

static bool is_showing_workertask_dialog = FALSE;

static struct
{
  struct city *owner;
  struct tile *loc;
} workertask_req;

static bool low_citydlg;

/****************************************/

static void initialize_city_dialogs(void);
static void city_dialog_map_create(struct city_dialog *pdialog,
                                   struct city_map_canvas *cmap_canvas);
static void city_dialog_map_recenter(GtkWidget *map_canvas_sw);

static struct city_dialog *get_city_dialog(struct city *pcity);
static gboolean keyboard_handler(GtkWidget * widget, GdkEventKey * event,
				 struct city_dialog *pdialog);

static GtkWidget *create_city_info_table(struct city_dialog *pdialog,
    					 GtkWidget **info_ebox,
					 GtkWidget **info_label);
static void create_and_append_overview_page(struct city_dialog *pdialog);
static void create_and_append_map_page(struct city_dialog *pdialog);
static void create_and_append_buildings_page(struct city_dialog *pdialog);
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

static void show_units_response(void *data);

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
static void change_production_callback(GtkComboBox *combo,
                                       struct city_dialog *pdialog);

static void sell_callback(struct impr_type *pimprove, gpointer data);
static void sell_callback_response(GtkWidget *w, gint response, gpointer data);

static void impr_callback(GtkTreeView *view, GtkTreePath *path,
			  GtkTreeViewColumn *col, gpointer data);

static void rename_callback(GtkWidget * w, gpointer data);
static void rename_popup_callback(gpointer data, gint response,
                                  const char *input);
static void set_cityopt_values(struct city_dialog *pdialog);
static void cityopt_callback(GtkWidget * w, gpointer data);
static void misc_whichtab_callback(GtkWidget * w, gpointer data);

static void city_destroy_callback(GtkWidget *w, gpointer data);
static void close_city_dialog(struct city_dialog *pdialog);
static void citydlg_response_callback(GtkDialog *dlg, gint response,
                                      void *data);
static void close_callback(GtkWidget *w, gpointer data);
static void switch_city_callback(GtkWidget *w, gpointer data);

/**********************************************************************//**
  Called to set the dimensions of the city dialog, both on
  startup and if the tileset is changed.
**************************************************************************/
static void init_citydlg_dimensions(void)
{
  canvas_width = get_citydlg_canvas_width();
  canvas_height = get_citydlg_canvas_height();
}

/**********************************************************************//**
  Initialize stuff needed for city dialogs
**************************************************************************/
static void initialize_city_dialogs(void)
{
  int height;

  fc_assert_ret(!city_dialogs_have_been_initialised);

  dialog_list = dialog_list_new();
  init_citydlg_dimensions();
  height = screen_height();

  /* Use default layout when height cannot be determined
   * (when height == 0) */
  if (height > 0 && height <= TINYSCREEN_MAX_HEIGHT) {
    low_citydlg = TRUE;
  } else {
    low_citydlg = FALSE;
  }

  city_dialogs_have_been_initialised = TRUE;
}

/**********************************************************************//**
  Called when the tileset changes.
**************************************************************************/
void reset_city_dialogs(void)
{
  if (!city_dialogs_have_been_initialised) {
    return;
  }

  init_citydlg_dimensions();

  dialog_list_iterate(dialog_list, pdialog) {
    /* There's no reasonable way to resize a GtkImage, so we don't try.
       Instead we just redraw the overview within the existing area.  The
       player has to close and reopen the dialog to fix this. */
    city_dialog_update_map(pdialog);
  } dialog_list_iterate_end;

  popdown_all_city_dialogs();
}

/**********************************************************************//**
  Return city dialog of the given city, or NULL is it doesn't
  already exist
**************************************************************************/
static struct city_dialog *get_city_dialog(struct city *pcity)
{
  if (!city_dialogs_have_been_initialised) {
    initialize_city_dialogs();
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pcity == pcity)
      return pdialog;
  }
  dialog_list_iterate_end;
  return NULL;
}

/**********************************************************************//**
  Redraw map canvas on expose.
**************************************************************************/
static gboolean canvas_exposed_cb(GtkWidget *w, cairo_t *cr,
                                  gpointer data)
{
  struct city_dialog *pdialog = data;

  cairo_scale(cr, CITYMAP_SCALE, CITYMAP_SCALE);
  cairo_set_source_surface(cr, pdialog->map_canvas_store_unscaled, 0, 0);
  if (!gtk_widget_get_sensitive(pdialog->overview.map_canvas.ebox)) {
    cairo_paint_with_alpha(cr, 0.5);
  } else {
    cairo_paint(cr);
  }

  return TRUE;
}

/**********************************************************************//**
  Create a city map widget; used in the overview and in the happiness page.
**************************************************************************/
static void city_dialog_map_create(struct city_dialog *pdialog,
                                   struct city_map_canvas *cmap_canvas)
{
  GtkWidget *sw, *ebox, *darea;

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(sw),
                                            CITYMAP_WIDTH);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(sw),
                                             CITYMAP_HEIGHT);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_NONE);

  ebox = gtk_event_box_new();
  gtk_widget_set_halign(ebox, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(ebox, GTK_ALIGN_CENTER);
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(ebox), FALSE);
  gtk_container_add(GTK_CONTAINER(sw), ebox);

  darea = gtk_drawing_area_new();
  gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_size_request(darea, CITYMAP_WIDTH, CITYMAP_HEIGHT);
  g_signal_connect(ebox, "button-press-event",
                   G_CALLBACK(button_down_citymap), pdialog);
  g_signal_connect(darea, "draw",
                   G_CALLBACK(canvas_exposed_cb), pdialog);
  gtk_container_add(GTK_CONTAINER(ebox), darea);

  /* save all widgets for the city map */
  cmap_canvas->sw = sw;
  cmap_canvas->ebox = ebox;
  cmap_canvas->darea = darea;
}

/**********************************************************************//**
  Center city dialog map.
**************************************************************************/
static void city_dialog_map_recenter(GtkWidget *map_canvas_sw)
{
  GtkAdjustment *adjust = NULL;
  gdouble value;

  fc_assert_ret(map_canvas_sw != NULL);

  adjust = gtk_scrolled_window_get_hadjustment(
    GTK_SCROLLED_WINDOW(map_canvas_sw));
  value = (gtk_adjustment_get_lower(adjust)
    + gtk_adjustment_get_upper(adjust)
    - gtk_adjustment_get_page_size(adjust)) / 2;
  gtk_adjustment_set_value(adjust, value);

  adjust = gtk_scrolled_window_get_vadjustment(
    GTK_SCROLLED_WINDOW(map_canvas_sw));
  value = (gtk_adjustment_get_lower(adjust)
    + gtk_adjustment_get_upper(adjust)
    - gtk_adjustment_get_page_size(adjust)) / 2;
  gtk_adjustment_set_value(adjust, value);
}

/**********************************************************************//**
  Refresh city dialog of the given city
**************************************************************************/
void real_city_dialog_refresh(struct city *pcity)
{
  struct city_dialog *pdialog = get_city_dialog(pcity);

  log_debug("CITYMAP_WIDTH:  %d", CITYMAP_WIDTH);
  log_debug("CITYMAP_HEIGHT: %d", CITYMAP_HEIGHT);
  log_debug("CITYMAP_SCALE:  %.3f", CITYMAP_SCALE);

  if (city_owner(pcity) == client.conn.playing) {
    city_report_dialog_update_city(pcity);
    economy_report_dialog_update();
  }

  if (!pdialog) {
    return;
  }

  city_dialog_update_title(pdialog);
  city_dialog_update_citizens(pdialog);
  city_dialog_update_information(pdialog->overview.info_ebox,
				 pdialog->overview.info_label, pdialog);
  city_dialog_update_map(pdialog);
  city_dialog_update_building(pdialog);
  city_dialog_update_improvement_list(pdialog);
  city_dialog_update_supported_units(pdialog);
  city_dialog_update_present_units(pdialog);

  if (!client_has_player() || city_owner(pcity) == client_player()) {
    bool have_present_units = (unit_list_size(pcity->tile->units) > 0);

    refresh_worklist(pdialog->production.worklist);

    if (!low_citydlg) {
      city_dialog_update_information(pdialog->happiness.info_ebox,
                                     pdialog->happiness.info_label, pdialog);
    }
    refresh_happiness_dialog(pdialog->pcity);
    if (game.info.citizen_nationality) {
      citizens_dialog_refresh(pdialog->pcity);
    }

    if (!client_is_observer()) {
      refresh_cma_dialog(pdialog->pcity, REFRESH_ALL);
    }

    gtk_widget_set_sensitive(pdialog->show_units_command,
			     can_client_issue_orders() &&
			     have_present_units);
  } else {
    /* Set the buttons we do not want live while a Diplomat investigates */
    gtk_widget_set_sensitive(pdialog->show_units_command, FALSE);
  }
}

/**********************************************************************//**
  Refresh city dialogs of unit's homecity and city where unit
  currently is.
**************************************************************************/
void refresh_unit_city_dialogs(struct unit *punit)
{
  struct city *pcity_sup, *pcity_pre;
  struct city_dialog *pdialog;

  pcity_sup = game_city_by_number(punit->homecity);
  pcity_pre = tile_city(unit_tile(punit));

  if (pcity_sup && (pdialog = get_city_dialog(pcity_sup))) {
    city_dialog_update_supported_units(pdialog);
  }

  if (pcity_pre && (pdialog = get_city_dialog(pcity_pre))) {
    city_dialog_update_present_units(pdialog);
  }
}

/**********************************************************************//**
  Popup the dialog 10% inside the main-window
**************************************************************************/
void real_city_dialog_popup(struct city *pcity)
{
  struct city_dialog *pdialog;

  if (!(pdialog = get_city_dialog(pcity))) {
    pdialog = create_city_dialog(pcity);
  }

  gtk_window_present(GTK_WINDOW(pdialog->shell));

  /* center the city map(s); this must be *after* the city dialog was drawn
   * else the size information is missing! */
  city_dialog_map_recenter(pdialog->overview.map_canvas.sw);
  if (pdialog->happiness.map_canvas.sw) {
    city_dialog_map_recenter(pdialog->happiness.map_canvas.sw);
  }
}

/**********************************************************************//**
  Return whether city dialog for given city is open
**************************************************************************/
bool city_dialog_is_open(struct city *pcity)
{
  return get_city_dialog(pcity) != NULL;
}

/**********************************************************************//**
  Popdown the dialog
**************************************************************************/
void popdown_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog = get_city_dialog(pcity);

  if (pdialog) {
    close_city_dialog(pdialog);
  }
}

/**********************************************************************//**
  Popdown all dialogs
**************************************************************************/
void popdown_all_city_dialogs(void)
{
  if (!city_dialogs_have_been_initialised) {
    return;
  }

  while (dialog_list_size(dialog_list)) {
    close_city_dialog(dialog_list_get(dialog_list, 0));
  }
  dialog_list_destroy(dialog_list);

  city_dialogs_have_been_initialised = FALSE;
}

/**********************************************************************//**
  Keyboard handler for city dialog
**************************************************************************/
static gboolean keyboard_handler(GtkWidget * widget, GdkEventKey * event,
                                 struct city_dialog *pdialog)
{
  if (event->state & GDK_CONTROL_MASK) {
    switch (event->keyval) {
    case GDK_KEY_Left:
      gtk_notebook_prev_page(GTK_NOTEBOOK(pdialog->notebook));
      return TRUE;

    case GDK_KEY_Right:
      gtk_notebook_next_page(GTK_NOTEBOOK(pdialog->notebook));
      return TRUE;

    default:
      break;
    }
  }

  return FALSE;
}

/**********************************************************************//**
  Destroy info popup dialog when button released
**************************************************************************/
static gboolean show_info_button_release(GtkWidget *w, GdkEventButton *ev,
                                         gpointer data)
{
  gtk_grab_remove(w);
  gdk_seat_ungrab(gdk_device_get_seat(ev->device));
  gtk_widget_destroy(w);

  return FALSE;
}

enum { FIELD_FOOD, FIELD_SHIELD, FIELD_TRADE, FIELD_GOLD, FIELD_LUXURY,
       FIELD_SCIENCE, FIELD_GRANARY, FIELD_GROWTH, FIELD_CORRUPTION,
       FIELD_WASTE, FIELD_CULTURE, FIELD_POLLUTION, FIELD_ILLNESS
};

/**********************************************************************//**
  Popup info dialog
**************************************************************************/
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
    case FIELD_CULTURE:
      get_city_dialog_culture_text(pdialog->pcity, buf, sizeof(buf));
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
    gtk_window_set_transient_for(GTK_WINDOW(p), GTK_WINDOW(pdialog->shell));
    gtk_window_set_position(GTK_WINDOW(p), GTK_WIN_POS_MOUSE);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(p), frame);

    label = gtk_label_new(buf);
    gtk_widget_set_name(label, "city_info_label");
    gtk_widget_set_margin_start(label, 4);
    gtk_widget_set_margin_end(label, 4);
    gtk_widget_set_margin_top(label, 4);
    gtk_widget_set_margin_bottom(label, 4);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_widget_show_all(p);

    gdk_seat_grab(gdk_device_get_seat(ev->device), gtk_widget_get_window(p),
                  GDK_SEAT_CAPABILITY_ALL_POINTING,
                  TRUE, NULL, (GdkEvent *)ev, NULL, NULL);
    gtk_grab_add(p);

    g_signal_connect_after(p, "button_release_event",
                           G_CALLBACK(show_info_button_release), NULL);
  }
  return TRUE;
}

/**********************************************************************//**
  Used once in the overview page and once in the happiness page
  **info_label points to the info_label in the respective struct
**************************************************************************/
static GtkWidget *create_city_info_table(struct city_dialog *pdialog,
    					 GtkWidget **info_ebox,
                                         GtkWidget **info_label)
{
  int i;
  GtkWidget *table, *label, *ebox;

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
    N_("Culture:"),
    N_("Pollution:"),
    N_("Plague Risk:")
  };
  static bool output_label_done;

  table = gtk_grid_new();
  g_object_set(table, "margin", 4, NULL);

  intl_slist(ARRAY_SIZE(output_label), output_label, &output_label_done);

  for (i = 0; i < NUM_INFO_FIELDS; i++) {
    label = gtk_label_new(output_label[i]);
    switch (i) {
      case 2:
      case 5:
      case 7:
        gtk_widget_set_margin_bottom(label, 5);
        break;
      case 3:
      case 6:
      case 8:
        gtk_widget_set_margin_top(label, 5);
        break;
      default:
        break;
    }
    gtk_widget_set_margin_end(label, 5);
    gtk_widget_set_name(label, "city_label");	/* for font style? */
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(table), label, 0, i, 1, 1);

    ebox = gtk_event_box_new();
    switch (i) {
      case 2:
      case 5:
      case 7:
        gtk_widget_set_margin_bottom(ebox, 5);
        break;
      case 3:
      case 6:
      case 8:
        gtk_widget_set_margin_top(ebox, 5);
        break;
      default:
        break;
    }
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ebox), FALSE);
    g_object_set_data(G_OBJECT(ebox), "pdialog", pdialog);
    g_signal_connect(ebox, "button_press_event",
	G_CALLBACK(show_info_popup), GUINT_TO_POINTER(i));
    info_ebox[i] = ebox;

    label = gtk_label_new("");
    info_label[i] = label;
    gtk_widget_set_name(label, "city_label");	/* ditto */
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

    gtk_container_add(GTK_CONTAINER(ebox), label);

    gtk_grid_attach(GTK_GRID(table), ebox, 1, i, 1, 1);
  }

  gtk_widget_show_all(table);

  return table;
}

/**********************************************************************//**
  Create main citydlg map
**************************************************************************/
static void create_citydlg_main_map(struct city_dialog *pdialog,
                                    GtkWidget *container)
{
  GtkWidget *frame;

  frame = gtk_frame_new(_("City map"));
  gtk_widget_set_size_request(frame, CITY_MAP_MIN_SIZE_X,
                              CITY_MAP_MIN_SIZE_Y);
  gtk_container_add(GTK_CONTAINER(container), frame);

  city_dialog_map_create(pdialog, &pdialog->overview.map_canvas);
  gtk_container_add(GTK_CONTAINER(frame), pdialog->overview.map_canvas.sw);
}

/**********************************************************************//**
  Create improvements list
**************************************************************************/
static GtkWidget *create_citydlg_improvement_list(struct city_dialog *pdialog,
                                                  GtkWidget *vbox)
{
  GtkWidget *view;
  GtkListStore *store;
  GtkCellRenderer *rend;

  /* improvements */
  store = gtk_list_store_new(5, G_TYPE_POINTER, GDK_TYPE_PIXBUF,
                             G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_hexpand(view, TRUE);
  gtk_widget_set_vexpand(view, TRUE);
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
  gtk_widget_set_name(view, "small_font");
  pdialog->overview.improvement_list = view;

  gtk_widget_set_tooltip_markup(view,
                                _("Press <b>ENTER</b> or double-click to sell an improvement."));

  rend = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
                                              rend, "pixbuf", 1, NULL);
  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
                                              rend, "text", 2,
                                              "strikethrough", 4, NULL);
  rend = gtk_cell_renderer_text_new();
  g_object_set(rend, "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
                                              rend, "text", 3,
                                              "strikethrough", 4, NULL);

  g_signal_connect(view, "row_activated", G_CALLBACK(impr_callback),
                   pdialog);

  return view;
}

/**********************************************************************//**
                  **** Overview page ****
 +- GtkWidget *page ------------------------------------------+
 | +- GtkWidget *middle -----------+------------------------+ |
 | | City map                      |  Production            | |
 | +-------------------------------+------------------------+ |
 +------------------------------------------------------------+
 | +- GtkWidget *bottom -------+----------------------------+ |
 | | Info                      | +- GtkWidget *right -----+ | |
 | |                           | | supported units        | | |
 | |                           | +------------------------+ | |
 | |                           | | present units          | | |
 | |                           | +------------------------+ | |
 | +---------------------------+----------------------------+ |
 +------------------------------------------------------------+
**************************************************************************/
static void create_and_append_overview_page(struct city_dialog *pdialog)
{
  GtkWidget *page, *bottom;
  GtkWidget *hbox, *right, *vbox, *frame, *table;
  GtkWidget *label, *sw, *view, *bar, *production_combo;
  GtkCellRenderer *rend;
  GtkListStore *production_store;
  /* TRANS: Overview tab in city dialog */
  const char *tab_title = _("_Overview");
  int unit_height = tileset_unit_with_upkeep_height(tileset);

  /* main page */
  page = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(page),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);
  label = gtk_label_new_with_mnemonic(tab_title);
  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  if (!low_citydlg) {
    GtkWidget *middle;

    /* middle: city map, improvements */
    middle = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(middle), 6);
    gtk_container_add(GTK_CONTAINER(page), middle);

    /* city map */
    create_citydlg_main_map(pdialog, middle);

    /* improvements */
    vbox = gtk_grid_new();
    gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                   GTK_ORIENTATION_VERTICAL);
    gtk_container_add(GTK_CONTAINER(middle), vbox);

    view = create_citydlg_improvement_list(pdialog, middle);

    label = g_object_new(GTK_TYPE_LABEL, "label", _("Production:"),
                         "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_container_add(GTK_CONTAINER(vbox), label);

    hbox = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(hbox), 10);
    gtk_container_add(GTK_CONTAINER(vbox), hbox);

    production_store = gtk_list_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                          G_TYPE_INT, G_TYPE_BOOLEAN);
    pdialog->overview.change_production_store = production_store;

    production_combo =
      gtk_combo_box_new_with_model(GTK_TREE_MODEL(production_store));
    gtk_widget_set_hexpand(production_combo, TRUE);
    pdialog->overview.production_combo = production_combo;
    gtk_container_add(GTK_CONTAINER(hbox), production_combo);
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
                                   rend, "text", 1, "strikethrough", 3, NULL);

    bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(bar), TRUE);
    pdialog->overview.production_bar = bar;
    gtk_container_add(GTK_CONTAINER(production_combo), bar);
    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(production_combo), 3);

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(bar), _("%d/%d %d turns"));

    pdialog->overview.buy_command
      = icon_label_button_new("system-run", _("_Buy"));
    gtk_container_add(GTK_CONTAINER(hbox), GTK_WIDGET(pdialog->overview.buy_command));
    g_signal_connect(pdialog->overview.buy_command, "clicked",
                     G_CALLBACK(buy_callback), pdialog);

    label = g_object_new(GTK_TYPE_LABEL, "use-underline", TRUE,
                         "mnemonic-widget", view,
                         "label", _("I_mprovements:"),
                         "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_container_add(GTK_CONTAINER(vbox), label);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                        GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(vbox), sw);

    gtk_container_add(GTK_CONTAINER(sw), view);
  } else {
    pdialog->overview.buy_command = NULL;
    pdialog->overview.production_bar = NULL;
    pdialog->overview.production_combo = NULL;
    pdialog->overview.change_production_store = NULL;
  }

  /* bottom: info, units */
  bottom = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(bottom), 6);
  gtk_container_add(GTK_CONTAINER(page), bottom);

  /* info */
  frame = gtk_frame_new(_("Info"));
  gtk_container_add(GTK_CONTAINER(bottom), frame);

  table = create_city_info_table(pdialog,
                                 pdialog->overview.info_ebox,
                                 pdialog->overview.info_label);
  gtk_widget_set_halign(table, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(table, GTK_ALIGN_CENTER);
  gtk_container_add(GTK_CONTAINER(frame), table);

  /* right: present and supported units (overview page) */
  right = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(right),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(bottom), right);

  pdialog->overview.supported_units_frame = gtk_frame_new("");
  gtk_container_add(GTK_CONTAINER(right),
                    pdialog->overview.supported_units_frame);
  pdialog->overview.present_units_frame = gtk_frame_new("");
  gtk_container_add(GTK_CONTAINER(right),
                    pdialog->overview.present_units_frame);

  /* supported units */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_hexpand(sw, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(pdialog->overview.supported_units_frame),
                                  sw);


  table = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(table), 2);
  gtk_widget_set_size_request(table, -1, unit_height);
  gtk_container_add(GTK_CONTAINER(sw), table);

  gtk_container_set_focus_hadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(sw)));
  gtk_container_set_focus_vadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw)));

  pdialog->overview.supported_unit_table = table;
  unit_node_vector_init(&pdialog->overview.supported_units);

  /* present units */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_hexpand(sw, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(pdialog->overview.present_units_frame), sw);

  table = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(table), 2);
  gtk_widget_set_size_request(table, -1, unit_height);
  gtk_container_add(GTK_CONTAINER(sw), table);

  gtk_container_set_focus_hadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(sw)));
  gtk_container_set_focus_vadjustment(GTK_CONTAINER(table),
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw)));

  pdialog->overview.present_unit_table = table;
  unit_node_vector_init(&pdialog->overview.present_units);

  /* show page */
  gtk_widget_show_all(page);
}

/**********************************************************************//**
  Create map page for small screens
**************************************************************************/
static void create_and_append_map_page(struct city_dialog *pdialog)
{
  if (low_citydlg) {
    GtkWidget *page;
    GtkWidget *label;
    const char *tab_title = _("Citymap");

    page = gtk_grid_new();
    gtk_orientable_set_orientation(GTK_ORIENTABLE(page),
                                   GTK_ORIENTATION_VERTICAL);
    gtk_container_set_border_width(GTK_CONTAINER(page), 8);
    label = gtk_label_new_with_mnemonic(tab_title);
    gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

    create_citydlg_main_map(pdialog, page);

    gtk_widget_show_all(page);
  }
}

/**********************************************************************//**
  Something dragged to worklist dialog.
**************************************************************************/
static void target_drag_data_received(GtkWidget *w,
                                      GdkDragContext *context,
                                      gint x, gint y,
                                      GtkSelectionData *data,
                                      guint info, guint time,
                                      gpointer user_data)
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
      cid id;
      struct universal univ;

      gtk_tree_model_get(model, &it, 0, &id, -1);
      univ = cid_production(id);
      city_change_production(pdialog->pcity, &univ);
      gtk_drag_finish(context, TRUE, FALSE, time);
    }
    gtk_tree_path_free(path);
  }

  gtk_drag_finish(context, FALSE, FALSE, time);
}

/**********************************************************************//**
  Create production page header - what tab this actually is,
  depends on screen size and layout.
**************************************************************************/
static void create_production_header(struct city_dialog *pdialog, GtkContainer *contain)
{
  GtkWidget *hbox, *bar;

  hbox = gtk_grid_new();
  g_object_set(hbox, "margin", 2, NULL);
  gtk_grid_set_column_spacing(GTK_GRID(hbox), 10);
  gtk_container_add(contain, hbox);

  /* The label is set in city_dialog_update_building() */
  bar = gtk_progress_bar_new();
  gtk_widget_set_hexpand(bar, TRUE);
  gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(bar), TRUE);
  pdialog->production.production_bar = bar;
  gtk_container_add(GTK_CONTAINER(hbox), bar);
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(bar), _("%d/%d %d turns"));

  add_worklist_dnd_target(bar);

  g_signal_connect(bar, "drag_data_received",
                   G_CALLBACK(target_drag_data_received), pdialog);

  pdialog->production.buy_command
    = icon_label_button_new("system-run", _("_Buy"));
  gtk_container_add(GTK_CONTAINER(hbox), GTK_WIDGET(pdialog->production.buy_command));

  g_signal_connect(pdialog->production.buy_command, "clicked",
                   G_CALLBACK(buy_callback), pdialog);
}

/**********************************************************************//**
  Create buildings list page for small screens
**************************************************************************/
static void create_and_append_buildings_page(struct city_dialog *pdialog)
{
  if (low_citydlg) {
    GtkWidget *page;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *view;
    const char *tab_title = _("Buildings");

    page = gtk_grid_new();
    gtk_orientable_set_orientation(GTK_ORIENTABLE(page),
                                   GTK_ORIENTATION_VERTICAL);
    gtk_container_set_border_width(GTK_CONTAINER(page), 8);
    label = gtk_label_new_with_mnemonic(tab_title);

    create_production_header(pdialog, GTK_CONTAINER(page));
    gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

    vbox = gtk_grid_new();
    gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                   GTK_ORIENTATION_VERTICAL);
    gtk_container_add(GTK_CONTAINER(page), vbox);

    view = create_citydlg_improvement_list(pdialog, vbox);

    gtk_container_add(GTK_CONTAINER(vbox), view);

    gtk_widget_show_all(page);
  }
}

/**********************************************************************//**
                    **** Production Page **** 
**************************************************************************/
static void create_and_append_worklist_page(struct city_dialog *pdialog)
{
  const char *tab_title = _("P_roduction");
  GtkWidget *label = gtk_label_new_with_mnemonic(tab_title);
  GtkWidget *page, *editor;

  page = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(page),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);
  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  /* stuff that's being currently built */
  if (!low_citydlg) {
    label = g_object_new(GTK_TYPE_LABEL,
                         "label", _("Production:"),
                         "xalign", 0.0, "yalign", 0.5, NULL);
    pdialog->production.production_label = label;
    gtk_container_add(GTK_CONTAINER(page), label);

    create_production_header(pdialog, GTK_CONTAINER(page));
  } else {
    pdialog->production.production_label = NULL;
  }

  editor = create_worklist();
  g_object_set(editor, "margin", 6, NULL);
  reset_city_worklist(editor, pdialog->pcity);
  gtk_container_add(GTK_CONTAINER(page), editor);
  pdialog->production.worklist = editor;

  gtk_widget_show_all(page);
}

/**********************************************************************//**
                     **** Happiness Page ****
 +- GtkWidget *page ----------+-------------------------------------------+
 | +- GtkWidget *left ------+ | +- GtkWidget *right --------------------+ |
 | | Info                   | | | City map                              | |
 | +- GtkWidget *citizens --+ | +- GtkWidget pdialog->happiness.widget -+ |
 | | Citizens data          | | | Happiness                             | |
 | +------------------------+ | +---------------------------------------+ |
 +----------------------------+-------------------------------------------+
**************************************************************************/
static void create_and_append_happiness_page(struct city_dialog *pdialog)
{
  GtkWidget *page, *label, *table, *right, *left, *frame;
  const char *tab_title = _("Happ_iness");

  /* main page */
  page = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(page), 6);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);
  label = gtk_label_new_with_mnemonic(tab_title);
  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  /* left: info, citizens */
  left = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(left),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(page), left);

  if (!low_citydlg) {
    /* upper left: info */
    frame = gtk_frame_new(_("Info"));
    gtk_container_add(GTK_CONTAINER(left), frame);

    table = create_city_info_table(pdialog,
                                   pdialog->happiness.info_ebox,
                                   pdialog->happiness.info_label);
    gtk_widget_set_halign(table, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(frame), table);
  }

  /* lower left: citizens */
  if (game.info.citizen_nationality) {
    pdialog->happiness.citizens = gtk_grid_new();
    gtk_orientable_set_orientation(
        GTK_ORIENTABLE(pdialog->happiness.citizens),
        GTK_ORIENTATION_VERTICAL);
    gtk_container_add(GTK_CONTAINER(left), pdialog->happiness.citizens);
    gtk_container_add(GTK_CONTAINER(pdialog->happiness.citizens),
                       citizens_dialog_display(pdialog->pcity));
  }

  /* right: city map, happiness */
  right = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(right),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(page), right);

  if (!low_citydlg) {
    /* upper right: city map */
    frame = gtk_frame_new(_("City map"));
    gtk_widget_set_size_request(frame, CITY_MAP_MIN_SIZE_X,
                                CITY_MAP_MIN_SIZE_Y);
    gtk_container_add(GTK_CONTAINER(right), frame);

    city_dialog_map_create(pdialog, &pdialog->happiness.map_canvas);
    gtk_container_add(GTK_CONTAINER(frame), pdialog->happiness.map_canvas.sw);
  }

  /* lower right: happiness */
  pdialog->happiness.widget = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(pdialog->happiness.widget),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(right), pdialog->happiness.widget);
  gtk_container_add(GTK_CONTAINER(pdialog->happiness.widget),
                    get_top_happiness_display(pdialog->pcity, low_citydlg, pdialog->shell));

  /* show page */
  gtk_widget_show_all(page);
}

/**********************************************************************//**
            **** Citizen Management Agent (CMA) Page ****
**************************************************************************/
static void create_and_append_cma_page(struct city_dialog *pdialog)
{
  GtkWidget *page, *label;
  const char *tab_title = _("_Governor");

  page = gtk_grid_new();

  label = gtk_label_new_with_mnemonic(tab_title);

  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  pdialog->cma_editor = create_cma_dialog(pdialog->pcity, low_citydlg);
  gtk_container_add(GTK_CONTAINER(page), pdialog->cma_editor->shell);

  gtk_widget_show(page);
}

/**********************************************************************//**
                    **** Misc. Settings Page **** 
**************************************************************************/
static void create_and_append_settings_page(struct city_dialog *pdialog)
{
  int i;
  GtkWidget *vbox2, *page, *frame, *label, *button;
  GtkSizeGroup *size;
  GSList *group;
  const char *tab_title = _("_Settings");

  static const char *new_citizens_output_label[] = {
    N_("Luxury"),
    N_("Science"),
    N_("Gold")
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


  page = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(page), 18);
  gtk_container_set_border_width(GTK_CONTAINER(page), 8);
  
  size = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
  
  label = gtk_label_new_with_mnemonic(tab_title);

  gtk_notebook_append_page(GTK_NOTEBOOK(pdialog->notebook), page, label);

  /* new_citizens radio */
  frame = gtk_frame_new(_("New citizens produce"));
  gtk_grid_attach(GTK_GRID(page), frame, 0, 0, 1, 1);
  gtk_size_group_add_widget(size, frame);

  vbox2 = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox2),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);

  intl_slist(ARRAY_SIZE(new_citizens_output_label), new_citizens_output_label,
             &new_citizens_label_done);

  group = NULL;
  for (i = 0; i < ARRAY_SIZE(new_citizens_output_label); i++) {
    button = gtk_radio_button_new_with_mnemonic(group, new_citizens_output_label[i]);
    pdialog->misc.new_citizens_radio[i] = button;
    gtk_container_add(GTK_CONTAINER(vbox2), button);
    g_signal_connect(button, "toggled",
		     G_CALLBACK(cityopt_callback), pdialog);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
  }

  /* next is the next-time-open radio group in the right column */
  frame = gtk_frame_new(_("Next time open"));
  gtk_grid_attach(GTK_GRID(page), frame, 1, 0, 1, 1);
  gtk_size_group_add_widget(size, frame);

  vbox2 = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox2),
                                 GTK_ORIENTATION_VERTICAL);
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
  gtk_widget_set_margin_top(frame, 12);
  gtk_widget_set_margin_bottom(frame, 12);
  gtk_grid_attach(GTK_GRID(page), frame, 0, 1, 1, 1);

  vbox2 = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox2),
                                 GTK_ORIENTATION_VERTICAL);
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

/**********************************************************************//**
                     **** Main City Dialog ****
 +----------------------------+-------------------------------+
 | GtkWidget *top: Citizens   | city name                     |
 +----------------------------+-------------------------------+
 | <notebook tab>                                             |
 +------------------------------------------------------------+
**************************************************************************/
static struct city_dialog *create_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;

  GtkWidget *close_command;
  GtkWidget *vbox, *hbox, *cbox, *ebox;
  int citizen_bar_width;
  int citizen_bar_height;

  if (!city_dialogs_have_been_initialised) {
    initialize_city_dialogs();
  }

  pdialog = fc_malloc(sizeof(struct city_dialog));
  pdialog->pcity = pcity;
  pdialog->buy_shell = NULL;
  pdialog->sell_shell = NULL;
  pdialog->rename_shell = NULL;
  pdialog->happiness.map_canvas.sw = NULL;      /* make sure NULL if spy */
  pdialog->happiness.map_canvas.ebox = NULL;    /* ditto */
  pdialog->happiness.map_canvas.darea = NULL;   /* ditto */
  pdialog->happiness.citizens = NULL;           /* ditto */
  pdialog->cma_editor = NULL;
  pdialog->map_canvas_store_unscaled
    = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
            canvas_width, canvas_height);

  pdialog->shell = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(pdialog->shell), city_name_get(pcity));
  setup_dialog(pdialog->shell, toplevel);
  gtk_window_set_role(GTK_WINDOW(pdialog->shell), "city");

  g_signal_connect(pdialog->shell, "destroy",
		   G_CALLBACK(city_destroy_callback), pdialog);
  gtk_window_set_position(GTK_WINDOW(pdialog->shell), GTK_WIN_POS_MOUSE);
  gtk_widget_set_name(pdialog->shell, "Freeciv");

  gtk_widget_realize(pdialog->shell);

  /* keep the icon of the executable on Windows (see PR#36491) */
#ifndef FREECIV_MSWINDOWS
  {
    GdkPixbuf *pixbuf = sprite_get_pixbuf(get_icon_sprite(tileset, ICON_CITYDLG));

    /* Only call this after tileset_load_tiles is called. */
    gtk_window_set_icon(GTK_WINDOW(pdialog->shell), pixbuf);
    g_object_unref(pixbuf);
  }
#endif /* FREECIV_MSWINDOWS */

  /* Restore size of the city dialog. */
  gtk_window_set_default_size(GTK_WINDOW(pdialog->shell),
                              GUI_GTK_OPTION(citydlg_xsize),
                              GUI_GTK_OPTION(citydlg_ysize));

  pdialog->popup_menu = gtk_menu_new();

  vbox = gtk_dialog_get_content_area(GTK_DIALOG(pdialog->shell));
  hbox = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(hbox), TRUE);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  /**** Citizens bar here ****/
  cbox = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(hbox), cbox);

  ebox = gtk_event_box_new();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(ebox), FALSE);
  gtk_container_add(GTK_CONTAINER(cbox), ebox);

  citizen_bar_width = tileset_small_sprite_width(tileset) * NUM_CITIZENS_SHOWN;
  citizen_bar_height = tileset_small_sprite_height(tileset);

  pdialog->citizen_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                        citizen_bar_width, citizen_bar_height);
  pdialog->citizen_images = gtk_image_new_from_surface(pdialog->citizen_surface);

  gtk_widget_add_events(pdialog->citizen_images, GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_margin_start(pdialog->citizen_images, 2);
  gtk_widget_set_margin_end(pdialog->citizen_images, 2);
  gtk_widget_set_margin_top(pdialog->citizen_images, 2);
  gtk_widget_set_margin_bottom(pdialog->citizen_images, 2);
  gtk_widget_set_halign(pdialog->citizen_images, GTK_ALIGN_START);
  gtk_widget_set_valign(pdialog->citizen_images, GTK_ALIGN_CENTER);
  gtk_container_add(GTK_CONTAINER(ebox), pdialog->citizen_images);
  g_signal_connect(G_OBJECT(ebox), "button-press-event",
                   G_CALLBACK(citizens_callback), pdialog);

  /**** City name label here ****/
  pdialog->name_label = gtk_label_new(NULL);
  gtk_widget_set_hexpand(pdialog->name_label, TRUE);
  gtk_widget_set_halign(pdialog->name_label, GTK_ALIGN_START);
  gtk_widget_set_valign(pdialog->name_label, GTK_ALIGN_CENTER);
  gtk_container_add(GTK_CONTAINER(hbox), pdialog->name_label);

  /**** -Start of Notebook- ****/

  pdialog->notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pdialog->notebook),
			   GTK_POS_BOTTOM);
  gtk_container_add(GTK_CONTAINER(vbox), pdialog->notebook);

  create_and_append_overview_page(pdialog);
  create_and_append_map_page(pdialog);
  create_and_append_buildings_page(pdialog);
  create_and_append_worklist_page(pdialog);

  /* only create these tabs if not a spy */
  if (!client_has_player() || city_owner(pcity) == client_player()) {
    create_and_append_happiness_page(pdialog);
  }

  if (city_owner(pcity) == client_player()
      && !client_is_observer()) {
    create_and_append_cma_page(pdialog);
    create_and_append_settings_page(pdialog);
  } else {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(pdialog->notebook),
                                  OVERVIEW_PAGE);
  }

  /**** End of Notebook ****/

  /* bottom buttons */

  pdialog->show_units_command =
    gtk_dialog_add_button(GTK_DIALOG(pdialog->shell), _("_List present units..."), CDLGR_UNITS);

  g_signal_connect(GTK_DIALOG(pdialog->shell), "response",
                   G_CALLBACK(citydlg_response_callback), pdialog);

  pdialog->prev_command = gtk_button_new_from_icon_name("go-previous", 0);
  gtk_dialog_add_action_widget(GTK_DIALOG(pdialog->shell),
                               GTK_WIDGET(pdialog->prev_command), 1);

  pdialog->next_command = gtk_button_new_from_icon_name("go-next", 0);
  gtk_dialog_add_action_widget(GTK_DIALOG(pdialog->shell),
                               GTK_WIDGET(pdialog->next_command), 2);

  if (city_owner(pcity) != client.conn.playing) {
    gtk_widget_set_sensitive(GTK_WIDGET(pdialog->prev_command), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(pdialog->next_command), FALSE);
  }

  close_command = gtk_dialog_add_button(GTK_DIALOG(pdialog->shell),
                                        _("Close"), GTK_RESPONSE_CLOSE);

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

  real_city_dialog_refresh(pdialog->pcity);

  /* need to do this every time a new dialog is opened. */
  city_dialog_update_prev_next();

  gtk_widget_show_all(pdialog->shell);

  gtk_window_set_focus(GTK_WINDOW(pdialog->shell), close_command);

  return pdialog;
}

/**************** Functions to update parts of the dialog ****************/
/**********************************************************************//**
  Update title of city dialog.
**************************************************************************/
static void city_dialog_update_title(struct city_dialog *pdialog)
{
  gchar *buf;
  const gchar *now;

  if (city_unhappy(pdialog->pcity)) {
    /* TRANS: city dialog title */
    buf = g_strdup_printf(_("<b>%s</b> - %s citizens - DISORDER"),
                          city_name_get(pdialog->pcity),
                          population_to_text(city_population(pdialog->pcity)));
  } else if (city_celebrating(pdialog->pcity)) {
    /* TRANS: city dialog title */
    buf = g_strdup_printf(_("<b>%s</b> - %s citizens - celebrating"),
                          city_name_get(pdialog->pcity),
                          population_to_text(city_population(pdialog->pcity)));
  } else if (city_happy(pdialog->pcity)) {
    /* TRANS: city dialog title */
    buf = g_strdup_printf(_("<b>%s</b> - %s citizens - happy"),
                          city_name_get(pdialog->pcity),
                          population_to_text(city_population(pdialog->pcity)));
  } else {
    /* TRANS: city dialog title */
    buf = g_strdup_printf(_("<b>%s</b> - %s citizens"),
                          city_name_get(pdialog->pcity),
                          population_to_text(city_population(pdialog->pcity)));
  }

  now = gtk_label_get_text(GTK_LABEL(pdialog->name_label));
  if (strcmp(now, buf) != 0) {
    gtk_window_set_title(GTK_WINDOW(pdialog->shell), city_name_get(pdialog->pcity));
    gtk_label_set_markup(GTK_LABEL(pdialog->name_label), buf);
  }

  g_free(buf);
}

/**********************************************************************//**
  Update citizens in city dialog
**************************************************************************/
static void city_dialog_update_citizens(struct city_dialog *pdialog)
{
  enum citizen_category categories[MAX_CITY_SIZE];
  int i, width;
  int citizen_bar_height;
  struct city *pcity = pdialog->pcity;
  int num_citizens = get_city_citizen_types(pcity, FEELING_FINAL, categories);
  cairo_t *cr;

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

  /* overview page */
  citizen_bar_height = tileset_small_sprite_height(tileset);

  cr = cairo_create(pdialog->citizen_surface);

  for (i = 0; i < num_citizens; i++) {
    cairo_set_source_surface(cr,
                             get_citizen_sprite(tileset, categories[i], i, pcity)->surface,
                             i * width, 0);
    cairo_rectangle(cr, i * width, 0, width, citizen_bar_height);
    cairo_fill(cr);
  }

  cairo_destroy(cr);

  gtk_widget_queue_draw(pdialog->citizen_images);
}

/**********************************************************************//**
  Update textual info fields in city dialog
**************************************************************************/
static void city_dialog_update_information(GtkWidget **info_ebox,
					   GtkWidget **info_label,
                                           struct city_dialog *pdialog)
{
  int i, illness = 0;
  char buf[NUM_INFO_FIELDS][512];
  struct city *pcity = pdialog->pcity;
  int granaryturns;
  static GtkCssProvider *emergency_provider = NULL;

  enum { FOOD, SHIELD, TRADE, GOLD, LUXURY, SCIENCE,
         GRANARY, GROWTH, CORRUPTION, WASTE, CULTURE,
         POLLUTION, ILLNESS
  };

  /* fill the buffers with the necessary info */
  fc_snprintf(buf[FOOD], sizeof(buf[FOOD]), "%3d (%+4d)",
              pcity->prod[O_FOOD], pcity->surplus[O_FOOD]);
  fc_snprintf(buf[SHIELD], sizeof(buf[SHIELD]), "%3d (%+4d)",
              pcity->prod[O_SHIELD] + pcity->waste[O_SHIELD],
              pcity->surplus[O_SHIELD]);
  fc_snprintf(buf[TRADE], sizeof(buf[TRADE]), "%3d (%+4d)",
              pcity->surplus[O_TRADE] + pcity->waste[O_TRADE],
              pcity->surplus[O_TRADE]);
  fc_snprintf(buf[GOLD], sizeof(buf[GOLD]), "%3d (%+4d)",
              pcity->prod[O_GOLD], pcity->surplus[O_GOLD]);
  fc_snprintf(buf[LUXURY], sizeof(buf[LUXURY]), "%3d",
              pcity->prod[O_LUXURY]);
  fc_snprintf(buf[SCIENCE], sizeof(buf[SCIENCE]), "%3d",
              pcity->prod[O_SCIENCE]);
  fc_snprintf(buf[GRANARY], sizeof(buf[GRANARY]), "%4d/%-4d",
              pcity->food_stock, city_granary_size(city_size_get(pcity)));

  granaryturns = city_turns_to_grow(pcity);
  if (granaryturns == 0) {
    /* TRANS: city growth is blocked.  Keep short. */
    fc_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("blocked"));
  } else if (granaryturns == FC_INFINITY) {
    /* TRANS: city is not growing.  Keep short. */
    fc_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("never"));
  } else {
    /* A negative value means we'll have famine in that many turns.
       But that's handled down below. */
    /* TRANS: city growth turns.  Keep short. */
    fc_snprintf(buf[GROWTH], sizeof(buf[GROWTH]),
                PL_("%d turn", "%d turns", abs(granaryturns)),
                abs(granaryturns));
  }
  fc_snprintf(buf[CORRUPTION], sizeof(buf[CORRUPTION]), "%4d",
              pcity->waste[O_TRADE]);
  fc_snprintf(buf[WASTE], sizeof(buf[WASTE]), "%4d",
              pcity->waste[O_SHIELD]);
  fc_snprintf(buf[CULTURE], sizeof(buf[CULTURE]), "%4d",
              pcity->client.culture);
  fc_snprintf(buf[POLLUTION], sizeof(buf[POLLUTION]), "%4d",
              pcity->pollution);
  if (!game.info.illness_on) {
    fc_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), " -.-");
  } else {
    illness = city_illness_calc(pcity, NULL, NULL, NULL, NULL);
    /* illness is in tenth of percent */
    fc_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), "%4.1f",
                (float)illness / 10.0);
  }

  /* stick 'em in the labels */
  for (i = 0; i < NUM_INFO_FIELDS; i++) {
    gtk_label_set_text(GTK_LABEL(info_label[i]), buf[i]);
  }

  /*
   * Special style stuff for granary, growth and pollution below. The
   * "4" below is arbitrary. 3 turns should be enough of a warning.
   */

  if (emergency_provider == NULL) {
    emergency_provider = gtk_css_provider_new();

    gtk_css_provider_load_from_data(emergency_provider,
                                    ".emergency {\n color: rgba(255, 0.0, 0.0, 255);\n}",
                                    -1, NULL);

    gtk_style_context_add_provider(gtk_widget_get_style_context(info_label[GRANARY]),
                                   GTK_STYLE_PROVIDER(emergency_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_provider(gtk_widget_get_style_context(info_label[GROWTH]),
                                   GTK_STYLE_PROVIDER(emergency_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_provider(gtk_widget_get_style_context(info_label[POLLUTION]),
                                   GTK_STYLE_PROVIDER(emergency_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_provider(gtk_widget_get_style_context(info_label[ILLNESS]),
                                   GTK_STYLE_PROVIDER(emergency_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }

  if (granaryturns > -4 && granaryturns < 0) {
    gtk_style_context_add_class(gtk_widget_get_style_context(info_label[GRANARY]), "emergency");
  } else {
    gtk_style_context_remove_class(gtk_widget_get_style_context(info_label[GRANARY]), "emergency");
  }

  if (granaryturns == 0 || pcity->surplus[O_FOOD] < 0) {
    gtk_style_context_add_class(gtk_widget_get_style_context(info_label[GROWTH]), "emergency");
  } else {
    gtk_style_context_remove_class(gtk_widget_get_style_context(info_label[GROWTH]), "emergency");
  }

  /* someone could add the color &orange for better granularity here */
  if (pcity->pollution >= 10) {
    gtk_style_context_add_class(gtk_widget_get_style_context(info_label[POLLUTION]), "emergency");
  } else {
    gtk_style_context_remove_class(gtk_widget_get_style_context(info_label[POLLUTION]), "emergency");
  }

  /* illness is in tenth of percent, i.e 100 != 10.0% */
  if (illness >= 100) {
    gtk_style_context_add_class(gtk_widget_get_style_context(info_label[ILLNESS]), "emergency");
  } else {
    gtk_style_context_remove_class(gtk_widget_get_style_context(info_label[ILLNESS]), "emergency");
  }
}

/**********************************************************************//**
  Update map display of city dialog
**************************************************************************/
static void city_dialog_update_map(struct city_dialog *pdialog)
{
  struct canvas store = FC_STATIC_CANVAS_INIT;

  store.surface = pdialog->map_canvas_store_unscaled;

  /* The drawing is done in three steps.
   *   1.  First we render to a pixmap with the appropriate canvas size.
   *   2.  Then the pixmap is rendered into a pixbuf of equal size.
   *   3.  Finally this pixbuf is composited and scaled onto the GtkImage's
   *       target pixbuf.
   */

  city_dialog_redraw_map(pdialog->pcity, &store);

  /* draw to real window */
  draw_map_canvas(pdialog);

  if (cma_is_city_under_agent(pdialog->pcity, NULL)) {
    gtk_widget_set_sensitive(pdialog->overview.map_canvas.ebox, FALSE);
    if (pdialog->happiness.map_canvas.ebox) {
      gtk_widget_set_sensitive(pdialog->happiness.map_canvas.ebox, FALSE);
    }
  } else {
    gtk_widget_set_sensitive(pdialog->overview.map_canvas.ebox, TRUE);
    if (pdialog->happiness.map_canvas.ebox) {
      gtk_widget_set_sensitive(pdialog->happiness.map_canvas.ebox, TRUE);
    }
  }
}

/**********************************************************************//**
  Update what city is building and buy cost in city dialog
**************************************************************************/
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

  if (pdialog->overview.buy_command != NULL) {
    gtk_widget_set_sensitive(GTK_WIDGET(pdialog->overview.buy_command), sensitive);
  }
  if (pdialog->production.buy_command != NULL) {
    gtk_widget_set_sensitive(GTK_WIDGET(pdialog->production.buy_command), sensitive);
  }

  /* Make sure build slots info is up to date */
  if (pdialog->production.production_label != NULL) {
    int build_slots = city_build_slots(pcity);

    /* Only display extra info if more than one slot is available */
    if (build_slots > 1) {
      fc_snprintf(buf2, sizeof(buf2),
                  /* TRANS: never actually used with built_slots <= 1 */
                  PL_("Production (up to %d unit per turn):",
                      "Production (up to %d units per turn):", build_slots),
                  build_slots);
      gtk_label_set_text(
        GTK_LABEL(pdialog->production.production_label), buf2);
    } else {
      gtk_label_set_text(
        GTK_LABEL(pdialog->production.production_label), _("Production:"));
    }
  }

  /* Update what the city is working on */
  get_city_dialog_production(pcity, buf, sizeof(buf));

  if (cost > 0) {
    pct = (gdouble) pcity->shield_stock / (gdouble) cost;
    pct = CLAMP(pct, 0.0, 1.0);
  } else {
    pct = 1.0;
  }

  if (pdialog->overview.production_bar != NULL) {
    fc_snprintf(buf2, sizeof(buf2), "%s%s\n%s", descr,
                worklist_is_empty(&pcity->worklist) ? "" : " (+)", buf);
    gtk_progress_bar_set_text(
      GTK_PROGRESS_BAR(pdialog->overview.production_bar), buf2);
    gtk_progress_bar_set_fraction(
      GTK_PROGRESS_BAR(pdialog->overview.production_bar), pct);
  }

  if (pdialog->production.production_bar != NULL) {
    fc_snprintf(buf2, sizeof(buf2), "%s%s: %s", descr,
                worklist_is_empty(&pcity->worklist) ? "" : " (+)", buf);
    gtk_progress_bar_set_text(
      GTK_PROGRESS_BAR(pdialog->production.production_bar), buf2);
    gtk_progress_bar_set_fraction(
      GTK_PROGRESS_BAR(pdialog->production.production_bar), pct);
  }

  if (pdialog->overview.production_combo != NULL) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(pdialog->overview.production_combo),
                             -1);
  }

  store = pdialog->overview.change_production_store;
  if (store != NULL) {
    gtk_list_store_clear(pdialog->overview.change_production_store);

    targets_used
      = collect_eventually_buildable_targets(targets, pdialog->pcity, FALSE);  
    name_and_sort_items(targets, targets_used, items, FALSE, pcity);

    for (item = 0; item < targets_used; item++) {
      if (can_city_build_now(pcity, &items[item].item)) {
        const char* name;
        struct sprite* sprite;
        GdkPixbuf *pix;
        struct universal target = items[item].item;
        bool useless;

        if (VUT_UTYPE == target.kind) {
          name = utype_name_translation(target.value.utype);
          sprite = get_unittype_sprite(tileset, target.value.utype,
                                       direction8_invalid());
          useless = FALSE;
        } else {
          name = improvement_name_translation(target.value.building);
          sprite = get_building_sprite(tileset, target.value.building);
          useless = is_improvement_redundant(pcity, target.value.building);
        }
        pix = sprite_get_pixbuf(sprite);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, pix,
                           1, name, 3, useless,
                           2, (gint)cid_encode(items[item].item),-1);
        g_object_unref(G_OBJECT(pix));
      }
    }
  }

  /* work around GTK+ refresh bug. */
  if (pdialog->overview.production_bar != NULL) {
    gtk_widget_queue_resize(pdialog->overview.production_bar);
  }
  if (pdialog->production.production_bar != NULL) {
    gtk_widget_queue_resize(pdialog->production.production_bar);
  }
}

/**********************************************************************//**
  Update list of improvements in city dialog
**************************************************************************/
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
    GdkPixbuf *pix;
    GtkTreeIter it;
    int upkeep;
    struct sprite *sprite;
    struct universal target = items[item].item;

    fc_assert_action(VUT_IMPROVEMENT == target.kind, continue);
    /* This takes effects (like Adam Smith's) into account. */
    upkeep = city_improvement_upkeep(pdialog->pcity, target.value.building);
    sprite = get_building_sprite(tileset, target.value.building);

    pix = sprite_get_pixbuf(sprite);
    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it,
		       0, target.value.building,
		       1, pix,
	2, items[item].descr,
	3, upkeep,
        4, is_improvement_redundant(pdialog->pcity, target.value.building),
	-1);
    g_object_unref(G_OBJECT(pix));

    total += upkeep;
  }
}

/**********************************************************************//**
  Update list of supported units in city dialog
**************************************************************************/
static void city_dialog_update_supported_units(struct city_dialog *pdialog)
{
  struct unit_list *units;
  struct unit_node_vector *nodes;
  int n, m, i;
  gchar *buf;
  int free_unhappy = get_city_bonus(pdialog->pcity, EFT_MAKE_CONTENT_MIL);

  if (NULL != client.conn.playing
      && city_owner(pdialog->pcity) != client.conn.playing) {
    units = pdialog->pcity->client.info_units_supported;
  } else {
    units = pdialog->pcity->units_supported;
  }

  nodes = &pdialog->overview.supported_units;

  n = unit_list_size(units);
  m = unit_node_vector_size(nodes);

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

      pix = gtk_image_new();
      node.pix = pix;
      node.height = tileset_unit_with_upkeep_height(tileset);

      gtk_container_add(GTK_CONTAINER(cmd), pix);

      gtk_grid_attach(GTK_GRID(pdialog->overview.supported_unit_table),
                      cmd, i, 0, 1, 1);
      unit_node_vector_append(nodes, node);
    }
  }

  i = 0;
  unit_list_iterate(units, punit) {
    struct unit_node *pnode;
    int happy_cost = city_unit_unhappiness(punit, &free_unhappy);

    pnode = unit_node_vector_get(nodes, i);
    if (pnode) {
      GtkWidget *cmd, *pix;

      cmd = pnode->cmd;
      pix = pnode->pix;

      put_unit_image_city_overlays(punit, GTK_IMAGE(pix), pnode->height,
                                   punit->upkeep, happy_cost);

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, supported_unit_callback, NULL);

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, supported_unit_middle_callback, NULL);

      gtk_widget_set_tooltip_text(cmd, unit_description(punit));

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

  buf = g_strdup_printf(_("Supported units %d"), n);
  gtk_frame_set_label(GTK_FRAME(pdialog->overview.supported_units_frame), buf);
  g_free(buf);
}

/**********************************************************************//**
  Update list of present units in city dialog
**************************************************************************/
static void city_dialog_update_present_units(struct city_dialog *pdialog)
{
  struct unit_list *units;
  struct unit_node_vector *nodes;
  int n, m, i;
  gchar *buf;

  if (NULL != client.conn.playing
      && city_owner(pdialog->pcity) != client.conn.playing) {
    units = pdialog->pcity->client.info_units_present;
  } else {
    units = pdialog->pcity->tile->units;
  }

  nodes = &pdialog->overview.present_units;

  n = unit_list_size(units);
  m = unit_node_vector_size(nodes);

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

      pix = gtk_image_new();
      node.pix = pix;
      node.height = tileset_full_tile_height(tileset);

      gtk_container_add(GTK_CONTAINER(cmd), pix);

      gtk_grid_attach(GTK_GRID(pdialog->overview.present_unit_table),
                      cmd, i, 0, 1, 1);
      unit_node_vector_append(nodes, node);
    }
  }

  i = 0;
  unit_list_iterate(units, punit) {
    struct unit_node *pnode;
    
    pnode = unit_node_vector_get(nodes, i);
    if (pnode) {
      GtkWidget *cmd, *pix;

      cmd = pnode->cmd;
      pix = pnode->pix;

      put_unit_image(punit, GTK_IMAGE(pix), pnode->height);

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, present_unit_callback, NULL);

      g_signal_handlers_disconnect_matched(cmd,
	  G_SIGNAL_MATCH_FUNC,
	  0, 0, NULL, present_unit_middle_callback, NULL);

      gtk_widget_set_tooltip_text(cmd, unit_description(punit));

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

  buf = g_strdup_printf(_("Present units %d"), n);
  gtk_frame_set_label(GTK_FRAME(pdialog->overview.present_units_frame), buf);
  g_free(buf);
}

/**********************************************************************//**
  Updates the sensitivity of the prev and next buttons.
  this does not need pdialog as a parameter, since it iterates
  over all the open dialogs.
  note: we still need the sensitivity code in create_city_dialog()
  for the spied dialogs.
**************************************************************************/
static void city_dialog_update_prev_next(void)
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
      gtk_widget_set_sensitive(GTK_WIDGET(pdialog->prev_command), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(pdialog->next_command), FALSE);
    }
    dialog_list_iterate_end;
  } else {
    dialog_list_iterate(dialog_list, pdialog) {
      if (city_owner(pdialog->pcity) == client.conn.playing) {
	gtk_widget_set_sensitive(GTK_WIDGET(pdialog->prev_command), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(pdialog->next_command), TRUE);
      }
    }
    dialog_list_iterate_end;
  }
}

/**********************************************************************//**
  User clicked button from action area.
**************************************************************************/
static void citydlg_response_callback(GtkDialog *dlg, gint response,
                                      void *data)
{
  switch (response) {
  case CDLGR_UNITS:
    show_units_response(data);
    break;
  }
}

/**********************************************************************//**
  User has clicked show units
**************************************************************************/
static void show_units_response(void *data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;
  struct tile *ptile = pdialog->pcity->tile;

  if (unit_list_size(ptile->units)) {
    unit_select_dialog_popup(ptile);
  }
}

/**********************************************************************//**
  Destroy widget -callback
**************************************************************************/
static void destroy_func(GtkWidget *w, gpointer data)
{
  gtk_widget_destroy(w);
}

/**********************************************************************//**
  Pop-up menu to change attributes of supported units
**************************************************************************/
static gboolean supported_unit_callback(GtkWidget *w, GdkEventButton *ev,
                                        gpointer data)
{
  GtkWidget *menu, *item;
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t) data);

  if (NULL != punit
      && NULL != (pcity = game_city_by_number(punit->homecity))
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

    if (!unit_can_do_action(punit, ACTION_DISBAND_UNIT)) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    gtk_widget_show_all(menu);

    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
  }

  return TRUE;
}

/**********************************************************************//**
  Pop-up menu to change attributes of units, ex. change homecity.
**************************************************************************/
static gboolean present_unit_callback(GtkWidget *w, GdkEventButton *ev,
                                      gpointer data)
{
  GtkWidget *menu, *item;
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t) data);

  if (NULL != punit
      && NULL != (pcity = tile_city(unit_tile(punit)))
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

    if (!unit_can_load(punit)) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    item = gtk_menu_item_new_with_mnemonic(_("_Unload unit"));
    g_signal_connect(item, "activate",
      G_CALLBACK(unit_unload_callback),
      GINT_TO_POINTER(punit->id));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if (!can_unit_unload(punit, unit_transport_get(punit))
        || !can_unit_exist_at_tile(&(wld.map), punit, unit_tile(punit))) {
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

    if (!unit_can_do_action(punit, ACTION_DISBAND_UNIT)) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    item = gtk_menu_item_new_with_mnemonic(
          action_id_name_translation(ACTION_HOME_CITY));
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
	|| NULL == can_upgrade_unittype(client.conn.playing,
                                        unit_type_get(punit))) {
      gtk_widget_set_sensitive(item, FALSE);
    }

    gtk_widget_show_all(menu);

    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
  }

  return TRUE;
}

/**********************************************************************//**
  If user middle-clicked on a unit, activate it and close dialog
**************************************************************************/
static gboolean present_unit_middle_callback(GtkWidget *w,
                                             GdkEventButton *ev,
                                             gpointer data)
{
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t) data);

  if (NULL != punit
      && NULL != (pcity = tile_city(unit_tile(punit)))
      && NULL != (pdialog = get_city_dialog(pcity))
      && can_client_issue_orders()) {

    if (ev->button == 3) {
      unit_focus_set(punit);
    } else if (ev->button == 2) {
      unit_focus_set(punit);
      close_city_dialog(pdialog);
    }
  }

  return TRUE;
}

/**********************************************************************//**
  If user middle-clicked on a unit, activate it and close dialog
**************************************************************************/
static gboolean supported_unit_middle_callback(GtkWidget *w,
                                               GdkEventButton *ev,
                                               gpointer data)
{
  struct city_dialog *pdialog;
  struct city *pcity;
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t) data);

  if (NULL != punit
      && NULL != (pcity = game_city_by_number(punit->homecity))
      && NULL != (pdialog = get_city_dialog(pcity))
      && can_client_issue_orders()) {

    if (ev->button == 3) {
      unit_focus_set(punit);
    } else if (ev->button == 2) {
      unit_focus_set(punit);
      close_city_dialog(pdialog);
    }
  }

  return TRUE;
}

/**********************************************************************//**
  User has requested centering to unit
**************************************************************************/
static void unit_center_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    center_tile_mapcanvas(unit_tile(punit));
  }
}

/**********************************************************************//**
  User has requested unit activation
**************************************************************************/
static void unit_activate_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    unit_focus_set(punit);
  }
}

/**********************************************************************//**
  User has requested some supported unit to be activated and
  city dialog to be closed
**************************************************************************/
static void supported_unit_activate_close_callback(GtkWidget *w,
                                                   gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    struct city *pcity =
      player_city_by_number(client_player(), punit->homecity);

    unit_focus_set(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	close_city_dialog(pdialog);
      }
    }
  }
}

/**********************************************************************//**
  User has requested some present unit to be activated and
  city dialog to be closed
**************************************************************************/
static void present_unit_activate_close_callback(GtkWidget *w,
                                                 gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    struct city *pcity = tile_city(unit_tile(punit));

    unit_focus_set(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	close_city_dialog(pdialog);
      }
    }
  }
}

/**********************************************************************//**
  User has requested unit to be loaded to transport
**************************************************************************/
static void unit_load_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    request_transport(punit, unit_tile(punit));
  }
}

/**********************************************************************//**
  User has requested unit to be unloaded from transport
**************************************************************************/
static void unit_unload_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    request_unit_unload(punit);
  }
}

/**********************************************************************//**
  User has requested unit to be sentried
**************************************************************************/
static void unit_sentry_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    request_unit_sentry(punit);
  }
}

/**********************************************************************//**
  User has requested unit to be fortified
**************************************************************************/
static void unit_fortify_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    request_unit_fortify(punit);
  }
}

/**********************************************************************//**
  User has requested unit to be disbanded
**************************************************************************/
static void unit_disband_callback(GtkWidget *w, gpointer data)
{
  struct unit_list *punits;
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL == punit) {
    return;
  }

  punits = unit_list_new();
  unit_list_append(punits, punit);
  popup_disband_dialog(punits);
  unit_list_destroy(punits);
}

/**********************************************************************//**
  User has requested unit to change homecity to city where it
  currently is
**************************************************************************/
static void unit_homecity_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL != punit) {
    request_unit_change_homecity(punit);
  }
}

/**********************************************************************//**
  User has requested unit to be upgraded
**************************************************************************/
static void unit_upgrade_callback(GtkWidget *w, gpointer data)
{
  struct unit_list *punits;
  struct unit *punit =
    player_unit_by_number(client_player(), (size_t)data);

  if (NULL == punit) {
    return;
  }

  punits = unit_list_new();
  unit_list_append(punits, punit);
  popup_upgrade_dialog(punits);
  unit_list_destroy(punits);
}

/******** Callbacks for citizen bar, map funcs that are not update *******/
/**********************************************************************//**
  Somebody clicked our list of citizens. If they clicked a specialist
  then change the type of him, else do nothing.
**************************************************************************/
static gboolean citizens_callback(GtkWidget *w, GdkEventButton *ev,
                                  gpointer data)
{
  struct city_dialog *pdialog = data;
  struct city *pcity = pdialog->pcity;
  int citnum, tlen, len;

  if (!can_client_issue_orders()) {
    return FALSE;
  }

  tlen = tileset_small_sprite_width(tileset);
  len = (city_size_get(pcity) - 1) * pdialog->cwidth + tlen;
  if (ev->x > len) {
    /* no citizen that far to the right */
    return FALSE;
  }
  citnum = MIN(city_size_get(pcity) - 1, ev->x / pdialog->cwidth);

  city_rotate_specialist(pcity, citnum);

  return TRUE;
}

/**********************************************************************//**
  Set requested workertask
**************************************************************************/
static void set_city_workertask(GtkWidget *w, gpointer data)
{
  enum unit_activity act = (enum unit_activity)GPOINTER_TO_INT(data);
  struct city *pcity = workertask_req.owner;
  struct tile *ptile = workertask_req.loc;
  struct packet_worker_task task;

  task.city_id = pcity->id;

  if (act == ACTIVITY_LAST) {
    task.tgt = -1;
    task.want = 0;
  } else {
    enum extra_cause cause = activity_to_extra_cause(act);
    enum extra_rmcause rmcause = activity_to_extra_rmcause(act);
    struct extra_type *tgt;

    if (cause != EC_NONE) {
      tgt = next_extra_for_tile(ptile, cause, city_owner(pcity), NULL);
    } else if (rmcause != ERM_NONE) {
      tgt = prev_extra_in_tile(ptile, rmcause, city_owner(pcity), NULL);
    } else {
      tgt = NULL;
    }

    if (tgt == NULL) {
      struct terrain *pterr = tile_terrain(ptile);

      if ((act != ACTIVITY_TRANSFORM
           || pterr->transform_result == NULL || pterr->transform_result == pterr)
          && (act != ACTIVITY_IRRIGATE
              || pterr->irrigation_result == NULL || pterr->irrigation_result == pterr)
          && (act != ACTIVITY_MINE
              || pterr->mining_result == NULL || pterr->mining_result == pterr)) {
        /* No extra to order */
        output_window_append(ftc_client, _("There's no suitable extra to order."));

        return;
      }

      task.tgt = -1;
    } else {
      task.tgt = extra_index(tgt);
    }

    task.want = 100;
  }

  task.tile_id = ptile->index;
  task.activity = act;

  send_packet_worker_task(&client.conn, &task);
}

/**********************************************************************//**
  Destroy workertask dlg
**************************************************************************/
static void workertask_dlg_destroy(GtkWidget *w, gpointer data)
{
  is_showing_workertask_dialog = FALSE;
}

/**********************************************************************//**
  Open dialog for setting worker task
**************************************************************************/
static void popup_workertask_dlg(struct city *pcity, struct tile *ptile)
{
  if (!is_showing_workertask_dialog) {
    GtkWidget *shl;
    struct terrain *pterr = tile_terrain(ptile);
    struct universal for_terr = { .kind = VUT_TERRAIN,
                                  .value = { .terrain = pterr }};
    struct worker_task *ptask;

    is_showing_workertask_dialog = TRUE;
    workertask_req.owner = pcity;
    workertask_req.loc = ptile;

    shl = choice_dialog_start(GTK_WINDOW(toplevel),
			       _("What Action to Request"),
			       _("Select autosettler activity:"));

    ptask = worker_task_list_get(pcity->task_reqs, 0);
    if (ptask != NULL) {
      choice_dialog_add(shl, _("Clear request"),
                        G_CALLBACK(set_city_workertask),
                        GINT_TO_POINTER(ACTIVITY_LAST), FALSE, NULL);
    }

    if ((pterr->mining_result == pterr
         && univs_have_action_enabler(ACTION_MINE, NULL, &for_terr))
        || (pterr->mining_result != pterr && pterr->mining_result != NULL
            && univs_have_action_enabler(ACTION_MINE_TF, NULL, &for_terr))) {
      choice_dialog_add(shl, _("Mine"),
                        G_CALLBACK(set_city_workertask),
                        GINT_TO_POINTER(ACTIVITY_MINE), FALSE, NULL);
    }
    if ((pterr->irrigation_result == pterr
         && univs_have_action_enabler(ACTION_IRRIGATE, NULL, &for_terr))
        || (pterr->irrigation_result != pterr && pterr->irrigation_result != NULL
            && univs_have_action_enabler(ACTION_IRRIGATE_TF, NULL, &for_terr))) {
      choice_dialog_add(shl, _("Irrigate"),
                        G_CALLBACK(set_city_workertask),
                        GINT_TO_POINTER(ACTIVITY_IRRIGATE), FALSE, NULL);
    }
    if (next_extra_for_tile(ptile, EC_ROAD, city_owner(pcity), NULL) != NULL) {
      choice_dialog_add(shl, _("Road"),
                        G_CALLBACK(set_city_workertask),
                        GINT_TO_POINTER(ACTIVITY_GEN_ROAD), FALSE, NULL);
    }
    if (pterr->transform_result != pterr && pterr->transform_result != NULL
        && univs_have_action_enabler(ACTION_TRANSFORM_TERRAIN, NULL, &for_terr)) {
      choice_dialog_add(shl, _("Transform"),
                        G_CALLBACK(set_city_workertask),
                        GINT_TO_POINTER(ACTIVITY_TRANSFORM), FALSE, NULL);
    }
    if (prev_extra_in_tile(ptile, ERM_CLEANPOLLUTION,
                           city_owner(pcity), NULL) != NULL) {
      choice_dialog_add(shl, _("Clean Pollution"),
                        G_CALLBACK(set_city_workertask),
                        GINT_TO_POINTER(ACTIVITY_POLLUTION), FALSE, NULL);
    }
    if (prev_extra_in_tile(ptile, ERM_CLEANFALLOUT,
                           city_owner(pcity), NULL) != NULL) {
      choice_dialog_add(shl, _("Clean Fallout"),
                        G_CALLBACK(set_city_workertask),
                        GINT_TO_POINTER(ACTIVITY_FALLOUT), FALSE, NULL);
    }

    choice_dialog_add(shl, _("Cancel"), 0, 0, FALSE, NULL);
    choice_dialog_end(shl);

    g_signal_connect(shl, "destroy", G_CALLBACK(workertask_dlg_destroy),
		     NULL);
  }
}

/**********************************************************************//**
  User has pressed button on citymap
**************************************************************************/
static gboolean button_down_citymap(GtkWidget *w, GdkEventButton *ev,
                                    gpointer data)
{
  struct city_dialog *pdialog = data;
  int canvas_x, canvas_y, city_x, city_y;

  if (!can_client_issue_orders()) {
    return FALSE;
  }

  canvas_x = ev->x * (double)canvas_width / (double)CITYMAP_WIDTH;
  canvas_y = ev->y * (double)canvas_height / (double)CITYMAP_HEIGHT;

  if (canvas_to_city_pos(&city_x, &city_y,
                         city_map_radius_sq_get(pdialog->pcity),
                         canvas_x, canvas_y)) {
    if (ev->button == 1) {
      city_toggle_worker(pdialog->pcity, city_x, city_y);
    } else if (ev->button == 3) {
      struct city *pcity = pdialog->pcity;

      popup_workertask_dlg(pdialog->pcity,
                           city_map_to_tile(pcity->tile, city_map_radius_sq_get(pcity),
                                            city_x, city_y));
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Set map canvas to be drawn
**************************************************************************/
static void draw_map_canvas(struct city_dialog *pdialog)
{
  gtk_widget_queue_draw(pdialog->overview.map_canvas.darea);
  if (pdialog->happiness.map_canvas.darea) { /* in case of spy */
    gtk_widget_queue_draw(pdialog->happiness.map_canvas.darea);
  }
}

/************** Callbacks for Buy, Change, Sell, Worklist ****************/
/**********************************************************************//**
  User has answered buy cost dialog
**************************************************************************/
static void buy_callback_response(GtkWidget *w, gint response, gpointer data)
{
  struct city_dialog *pdialog = data;

  if (response == GTK_RESPONSE_YES) {
    city_buy_production(pdialog->pcity);
  }
  gtk_widget_destroy(w);
}

/**********************************************************************//**
  User has clicked buy-button
**************************************************************************/
static void buy_callback(GtkWidget *w, gpointer data)
{
  GtkWidget *shell;
  struct city_dialog *pdialog = data;
  const char *name = city_production_name_translation(pdialog->pcity);
  int value = pdialog->pcity->client.buy_cost;
  char buf[1024];

  if (!can_client_issue_orders()) {
    return;
  }

  fc_snprintf(buf, ARRAY_SIZE(buf), PL_("Treasury contains %d gold.",
                                        "Treasury contains %d gold.",
                                        client_player()->economic.gold),
              client_player()->economic.gold);

  if (value <= client_player()->economic.gold) {
    shell = gtk_message_dialog_new(NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        /* TRANS: Last %s is pre-pluralised "Treasury contains %d gold." */
        PL_("Buy %s for %d gold?\n%s",
            "Buy %s for %d gold?\n%s", value),
        name, value, buf);
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
        /* TRANS: Last %s is pre-pluralised "Treasury contains %d gold." */
        PL_("%s costs %d gold.\n%s",
            "%s costs %d gold.\n%s", value),
        name, value, buf);
    setup_dialog(shell, pdialog->shell);
    gtk_window_set_title(GTK_WINDOW(shell), _("Buy It!"));
    g_signal_connect(shell, "response", G_CALLBACK(gtk_widget_destroy),
      NULL);
    gtk_window_present(GTK_WINDOW(shell));
  }
}

/**********************************************************************//**
  Callback for the dropdown production menu.
**************************************************************************/
static void change_production_callback(GtkComboBox *combo,
                                       struct city_dialog *pdialog)
{
  GtkTreeIter iter;

  if (can_client_issue_orders()
      && gtk_combo_box_get_active_iter(combo, &iter)) {
    cid id;
    struct universal univ;

    gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter, 2, &id, -1);
    univ = cid_production(id);
    city_change_production(pdialog->pcity, &univ);
  }
}

/**********************************************************************//**
  User has clicked sell-button
**************************************************************************/
static void sell_callback(struct impr_type *pimprove, gpointer data)
{
  GtkWidget *shl;
  struct city_dialog *pdialog = (struct city_dialog *) data;
  pdialog->sell_id = improvement_number(pimprove);
  int price;
  
  if (!can_client_issue_orders()) {
    return;
  }

  if (test_player_sell_building_now(client.conn.playing, pdialog->pcity,
                                    pimprove) != TR_SUCCESS) {
    return;
  }

  price = impr_sell_gold(pimprove);
  shl = gtk_message_dialog_new(NULL,
    GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_YES_NO,
    PL_("Sell %s for %d gold?",
        "Sell %s for %d gold?", price),
    city_improvement_name_translation(pdialog->pcity, pimprove), price);
  setup_dialog(shl, pdialog->shell);
  pdialog->sell_shell = shl;
  
  gtk_window_set_title(GTK_WINDOW(shl), _("Sell It!"));
  gtk_window_set_position(GTK_WINDOW(shl), GTK_WIN_POS_CENTER_ON_PARENT);

  g_signal_connect(shl, "response",
		   G_CALLBACK(sell_callback_response), pdialog);
  
  gtk_window_present(GTK_WINDOW(shl));
}

/**********************************************************************//**
  User has responded to sell price dialog
**************************************************************************/
static void sell_callback_response(GtkWidget *w, gint response, gpointer data)
{
  struct city_dialog *pdialog = data;

  if (response == GTK_RESPONSE_YES) {
    city_sell_improvement(pdialog->pcity, pdialog->sell_id);
  }
  gtk_widget_destroy(w);
  
  pdialog->sell_shell = NULL;
}

/**********************************************************************//**
  This is here because it's closely related to the sell stuff
**************************************************************************/
static void impr_callback(GtkTreeView *view, GtkTreePath *path,
                          GtkTreeViewColumn *col, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  GdkWindow *win;
  GdkSeat *seat;
  GdkModifierType mask;
  struct impr_type *pimprove;

  model = gtk_tree_view_get_model(view);

  if (!gtk_tree_model_get_iter(model, &it, path)) {
    return;
  }

  gtk_tree_model_get(model, &it, 0, &pimprove, -1);

  win = gdk_get_default_root_window();
  seat = gdk_display_get_default_seat(gdk_window_get_display(win));

  gdk_window_get_device_position(win, gdk_seat_get_pointer(seat),
                                 NULL, NULL, &mask);

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

/************ Callbacks for stuff on the Misc. Settings page *************/
/**********************************************************************//**
  Called when Rename button pressed
**************************************************************************/
static void rename_callback(GtkWidget *w, gpointer data)
{
  struct city_dialog *pdialog;

  pdialog = (struct city_dialog *) data;

  pdialog->rename_shell = input_dialog_create(GTK_WINDOW(pdialog->shell),
                                              /* "shellrenamecity" */
                                              _("Rename City"),
                                              _("What should we rename the city to?"),
                                              city_name_get(pdialog->pcity),
                                              rename_popup_callback, pdialog);
}

/**********************************************************************//**
  Called when user has finished with "Rename City" popup
**************************************************************************/
static void rename_popup_callback(gpointer data, gint response,
                                  const char *input)
{
  struct city_dialog *pdialog = data;

  if (pdialog) {
    if (response == GTK_RESPONSE_OK) {
      city_rename(pdialog->pcity, input);
    } /* else CANCEL or DELETE_EVENT */

    pdialog->rename_shell = NULL;
  }
}

/**********************************************************************//**
  Sets which page will be set on reopen of dialog
**************************************************************************/
static void misc_whichtab_callback(GtkWidget * w, gpointer data)
{
  new_dialog_def_page = GPOINTER_TO_INT(data);
}

/**********************************************************************//**
  City options callbacks
**************************************************************************/
static void cityopt_callback(GtkWidget * w, gpointer data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;

  if (!can_client_issue_orders()) {
    return;
  }

  if (!pdialog->misc.block_signal){
    struct city *pcity = pdialog->pcity;
    bv_city_options new_options;

    fc_assert(CITYO_LAST == 3);

    BV_CLR_ALL(new_options);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pdialog->misc.disband_on_settler))) {
      BV_SET(new_options, CITYO_DISBAND);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pdialog->misc.new_citizens_radio[1]))) {
      BV_SET(new_options, CITYO_SCIENCE_SPECIALISTS);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pdialog->misc.new_citizens_radio[2]))) {
      BV_SET(new_options, CITYO_GOLD_SPECIALISTS);
    }

    dsend_packet_city_options_req(&client.conn, pcity->id,new_options);
  }
}

/**********************************************************************//**
  Refresh the city options (auto_[land, air, sea, helicopter] and
  disband-is-size-1) in the misc page.
**************************************************************************/
static void set_cityopt_values(struct city_dialog *pdialog)
{
  struct city *pcity = pdialog->pcity;

  pdialog->misc.block_signal = 1;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pdialog->misc.disband_on_settler),
                               is_city_option_set(pcity, CITYO_DISBAND));

  if (is_city_option_set(pcity, CITYO_SCIENCE_SPECIALISTS)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                 (pdialog->misc.new_citizens_radio[1]), TRUE);
  } else if (is_city_option_set(pcity, CITYO_GOLD_SPECIALISTS)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                 (pdialog->misc.new_citizens_radio[2]), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                 (pdialog->misc.new_citizens_radio[0]), TRUE);
  }
  pdialog->misc.block_signal = 0;
}

/******************** Callbacks for: Close, Prev, Next. ******************/
/**********************************************************************//**
  User has clicked rename city-button
**************************************************************************/
static void close_callback(GtkWidget *w, gpointer data)
{
  close_city_dialog((struct city_dialog *) data);
}

/**********************************************************************//**
  User has closed rename city dialog
**************************************************************************/
static void city_destroy_callback(GtkWidget *w, gpointer data)
{
  struct city_dialog *pdialog;

  pdialog = (struct city_dialog *) data;

  gtk_widget_hide(pdialog->shell);

  if (game.info.citizen_nationality) {
    citizens_dialog_close(pdialog->pcity);
  }
  close_happiness_dialog(pdialog->pcity);
  close_cma_dialog(pdialog->pcity);

  /* Save size of the city dialog. */
  GUI_GTK_OPTION(citydlg_xsize)
    = CLIP(GUI_GTK3_22_CITYDLG_MIN_XSIZE,
           gtk_widget_get_allocated_width(pdialog->shell),
           GUI_GTK3_22_CITYDLG_MAX_XSIZE);
  GUI_GTK_OPTION(citydlg_ysize)
    = CLIP(GUI_GTK3_22_CITYDLG_MIN_XSIZE,
           gtk_widget_get_allocated_height(pdialog->shell),
           GUI_GTK3_22_CITYDLG_MAX_XSIZE);

  last_page
    = gtk_notebook_get_current_page(GTK_NOTEBOOK(pdialog->notebook));

  if (pdialog->popup_menu) {
    gtk_widget_destroy(pdialog->popup_menu);
  }

  dialog_list_remove(dialog_list, pdialog);

  unit_node_vector_free(&pdialog->overview.supported_units);
  unit_node_vector_free(&pdialog->overview.present_units);

  if (pdialog->buy_shell) {
    gtk_widget_destroy(pdialog->buy_shell);
  }
  if (pdialog->sell_shell) {
    gtk_widget_destroy(pdialog->sell_shell);
  }
  if (pdialog->rename_shell) {
    gtk_widget_destroy(pdialog->rename_shell);
  }

  cairo_surface_destroy(pdialog->map_canvas_store_unscaled);
  cairo_surface_destroy(pdialog->citizen_surface);

  free(pdialog);

  /* need to do this every time a new dialog is closed. */
  city_dialog_update_prev_next();
}

/**********************************************************************//**
  Close city dialog
**************************************************************************/
static void close_city_dialog(struct city_dialog *pdialog)
{
  gtk_widget_destroy(pdialog->shell);
}

/**********************************************************************//**
  Callback for the prev/next buttons. Switches to the previous/next
  city.
**************************************************************************/
static void switch_city_callback(GtkWidget *w, gpointer data)
{
  struct city_dialog *pdialog = (struct city_dialog *) data;
  int i, j, dir, size;
  struct city *new_pcity = NULL;

  if (client_is_global_observer()) {
    return;
  }

  size = city_list_size(client.conn.playing->cities);

  fc_assert_ret(city_dialogs_have_been_initialised);
  fc_assert_ret(size >= 1);
  fc_assert_ret(city_owner(pdialog->pcity) == client.conn.playing);

  if (size == 1) {
    return;
  }

  /* dir = 1 will advance to the city, dir = -1 will get previous */
  if (w == GTK_WIDGET(pdialog->next_command)) {
    dir = 1;
  } else if (w == GTK_WIDGET(pdialog->prev_command)) {
    dir = -1;
  } else {
    /* Always fails. */
    fc_assert_ret(w == GTK_WIDGET(pdialog->next_command)
                  || w == GTK_WIDGET(pdialog->prev_command));
    dir = 1;
  }

  for (i = 0; i < size; i++) {
    if (pdialog->pcity == city_list_get(client.conn.playing->cities, i)) {
      break;
    }
  }

  fc_assert_ret(i < size);

  for (j = 1; j < size; j++) {
    struct city *other_pcity = city_list_get(client.conn.playing->cities,
					     (i + dir * j + size) % size);
    struct city_dialog *other_pdialog = get_city_dialog(other_pcity);

    fc_assert_ret(other_pdialog != pdialog);
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
  if (game.info.citizen_nationality) {
    citizens_dialog_close(pdialog->pcity);
  }
  close_happiness_dialog(pdialog->pcity);

  pdialog->pcity = new_pcity;

  /* reinitialize happiness, and cma dialogs */
  if (game.info.citizen_nationality) {
    gtk_container_add(GTK_CONTAINER(pdialog->happiness.citizens),
                      citizens_dialog_display(pdialog->pcity));
  }
  gtk_container_add(GTK_CONTAINER(pdialog->happiness.widget),
                    get_top_happiness_display(pdialog->pcity, low_citydlg, pdialog->shell));
  if (!client_is_observer()) {
    fc_assert(pdialog->cma_editor != NULL);
    pdialog->cma_editor->pcity = new_pcity;
  }

  reset_city_worklist(pdialog->production.worklist, pdialog->pcity);

  can_slide = FALSE;
  center_tile_mapcanvas(pdialog->pcity->tile);
  can_slide = TRUE;
  if (!client_is_observer()) {
    set_cityopt_values(pdialog);  /* need not be in real_city_dialog_refresh */
  }

  real_city_dialog_refresh(pdialog->pcity);

  /* recenter the city map(s) */
  city_dialog_map_recenter(pdialog->overview.map_canvas.sw);
  if (pdialog->happiness.map_canvas.sw) {
    city_dialog_map_recenter(pdialog->happiness.map_canvas.sw);
  }
}
