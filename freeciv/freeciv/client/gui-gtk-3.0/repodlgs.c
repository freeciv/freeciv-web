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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "fc_types.h" /* LINE_BREAK */
#include "game.h"
#include "government.h"
#include "packets.h"
#include "research.h"
#include "tech.h"
#include "unitlist.h"

/* client */
#include "chatline_common.h"
#include "client_main.h"
#include "climisc.h"
#include "control.h"
#include "mapview_common.h"
#include "options.h"
#include "packhand_gen.h"
#include "control.h"
#include "reqtree.h"
#include "text.h"

/* client/gui-gtk-3.0 */
#include "canvas.h"
#include "cityrep.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "plrdlg.h"
#include "sprite.h"

#include "repodlgs.h"


/****************************************************************************
                         RESEARCH REPORT DIALOG
****************************************************************************/
struct science_report {
  struct gui_dialog *shell;
  GtkComboBox *reachable_techs;
  GtkComboBox *reachable_goals;
  GtkWidget *button_show_all;
  GtkLabel *main_label;         /* Gets science_dialog_text(). */
  GtkProgressBar *progress_bar;
  GtkLabel *goal_label;
  GtkLayout *drawing_area;
};

static GtkListStore *science_report_store_new(void);
static inline void science_report_store_set(GtkListStore *store,
                                            GtkTreeIter *iter,
                                            Tech_type_id tech);
static bool science_report_combo_get_active(GtkComboBox *combo,
                                            Tech_type_id *tech,
                                            const char **name);
static void science_report_combo_set_active(GtkComboBox *combo,
                                            Tech_type_id tech);
static gboolean science_diagram_button_release_callback(GtkWidget *widget,
    GdkEventButton *event, gpointer data);
static gboolean science_diagram_update(GtkWidget *widget,
                                       cairo_t *cr,
                                       gpointer data);
static GtkWidget *science_diagram_new(void);
static void science_diagram_data(GtkWidget *widget, bool show_all);
static void science_diagram_center(GtkWidget *diagram, Tech_type_id tech);
static void science_report_redraw(struct science_report *preport);
static gint cmp_func(gconstpointer a_p, gconstpointer b_p);
static void science_report_update(struct science_report *preport);
static void science_report_current_callback(GtkComboBox *combo,
                                            gpointer data);
static void science_report_show_all_callback(GtkComboBox *combo,
                                             gpointer data);
static void science_report_goal_callback(GtkComboBox *combo, gpointer data);
static void science_report_init(struct science_report *preport);
static void science_report_free(struct science_report *preport);

static struct science_report science_report = { NULL, };
static bool science_report_no_combo_callback = FALSE;

/* Those values must match the function science_report_store_new(). */
enum science_report_columns {
  SRD_COL_NAME,
  SRD_COL_STEPS,

  /* Not visible. */
  SRD_COL_ID,           /* Tech_type_id */

  SRD_COL_NUM
};

/************************************************************************//**
  Create a science report list store.
****************************************************************************/
static GtkListStore *science_report_store_new(void)
{
  return gtk_list_store_new(SRD_COL_NUM,
                            G_TYPE_STRING,      /* SRD_COL_NAME */
                            G_TYPE_INT,         /* SRD_COL_STEPS */
                            G_TYPE_INT);        /* SRD_COL_ID */
}

/************************************************************************//**
  Append a technology to the list store.
****************************************************************************/
static inline void science_report_store_set(GtkListStore *store,
                                            GtkTreeIter *iter,
                                            Tech_type_id tech)
{
  const struct research *presearch = research_get(client_player());

  gtk_list_store_set(store, iter,
                     SRD_COL_NAME,
                     research_advance_name_translation(presearch, tech),
                     SRD_COL_STEPS,
                     research_goal_unknown_techs(presearch, tech),
                     SRD_COL_ID, tech,
                     -1);
}

/************************************************************************//**
  Get the active tech of the combo.
****************************************************************************/
static bool science_report_combo_get_active(GtkComboBox *combo,
                                            Tech_type_id *tech,
                                            const char **name)
{
  GtkTreeIter iter;

  if (science_report_no_combo_callback
      || !gtk_combo_box_get_active_iter(combo, &iter)) {
    return FALSE;
  }

  gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter,
                     SRD_COL_NAME, name,
                     SRD_COL_ID, tech,
                     -1);
  return TRUE;
}

/************************************************************************//**
  Set the active tech of the combo.
****************************************************************************/
static void science_report_combo_set_active(GtkComboBox *combo,
                                            Tech_type_id tech)
{
  ITree iter;
  Tech_type_id iter_tech;

  for (itree_begin(gtk_combo_box_get_model(combo), &iter);
       !itree_end(&iter); itree_next(&iter)) {
    itree_get(&iter, SRD_COL_ID, &iter_tech, -1);
    if (iter_tech == tech) {
      science_report_no_combo_callback = TRUE;
      gtk_combo_box_set_active_iter(combo, &iter.it);
      science_report_no_combo_callback = FALSE;
      return;
    }
  }
  log_error("%s(): Tech %d not found in the combo.", __FUNCTION__, tech);
}

/************************************************************************//**
  Change tech goal, research or open help dialog.
****************************************************************************/
static gboolean science_diagram_button_release_callback(GtkWidget *widget,
    GdkEventButton *event, gpointer data)
{
  const struct research *presearch = research_get(client_player());
  struct reqtree *reqtree = g_object_get_data(G_OBJECT(widget), "reqtree");
  Tech_type_id tech = get_tech_on_reqtree(reqtree, event->x, event->y);

  if (tech == A_NONE) {
    return TRUE;
  }

  if (event->button == 3) {
    /* RMB: get help */
    popup_help_dialog_typed(research_advance_name_translation(presearch,
                                                              tech),
                            HELP_TECH);
  } else {
    if (event->button == 1 && can_client_issue_orders()) {
      /* LMB: set research or research goal */
      switch (research_invention_state(research_get(client_player()),
                                       tech)) {
       case TECH_PREREQS_KNOWN:
         dsend_packet_player_research(&client.conn, tech);
         break;
       case TECH_UNKNOWN:
         dsend_packet_player_tech_goal(&client.conn, tech);
         break;
       case TECH_KNOWN:
         break;
      }
    }
  }
  return TRUE;
}

/************************************************************************//**
  Draw the invalidated portion of the reqtree.
****************************************************************************/
static gboolean science_diagram_update(GtkWidget *widget, cairo_t *cr, gpointer data)
{
  /* FIXME: this currently redraws everything! */
  struct canvas canvas = FC_STATIC_CANVAS_INIT;
  struct reqtree *reqtree = g_object_get_data(G_OBJECT(widget), "reqtree");
  int width, height;
  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;
  gint hadjustment_value;
  gint vadjustment_value;

  if (!tileset_is_fully_loaded()) {
    return TRUE;
  }

  hadjustment = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widget));
  vadjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widget));

  hadjustment_value = (gint)gtk_adjustment_get_value(hadjustment);
  vadjustment_value = (gint)gtk_adjustment_get_value(vadjustment);

  cairo_translate(cr, -hadjustment_value, -vadjustment_value);

  canvas.drawable = cr;

  get_reqtree_dimensions(reqtree, &width, &height);
  draw_reqtree(reqtree, &canvas, 0, 0, 0, 0, width, height);

  return TRUE;
}

/************************************************************************//**
  Return the drawing area widget of new technology diagram. Set in 'x' the
  position of the current tech to center to it.
****************************************************************************/
static GtkWidget *science_diagram_new(void)
{
  GtkWidget *diagram;

  diagram = gtk_layout_new(NULL, NULL);
  gtk_widget_add_events(diagram,
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                        | GDK_BUTTON2_MOTION_MASK | GDK_BUTTON3_MOTION_MASK);
  g_signal_connect(diagram, "draw",
                   G_CALLBACK(science_diagram_update), NULL);
  g_signal_connect(diagram, "button-release-event",
                   G_CALLBACK(science_diagram_button_release_callback),
                   NULL);

  return diagram;
}

/************************************************************************//**
  Recreate the req tree.
****************************************************************************/
static void science_diagram_data(GtkWidget *widget, bool show_all)
{
  struct reqtree *reqtree;
  int width, height;

  if (can_conn_edit(&client.conn)) {
    /* Show all techs in editor mode, not only currently reachable ones */
    reqtree = create_reqtree(NULL, TRUE);
  } else {
    /* Show only at some point reachable techs */
    reqtree = create_reqtree(client_player(), show_all);
  }

  get_reqtree_dimensions(reqtree, &width, &height);
  gtk_layout_set_size(GTK_LAYOUT(widget), width, height);
  g_object_set_data_full(G_OBJECT(widget), "reqtree", reqtree,
                         (GDestroyNotify) destroy_reqtree);
}

/************************************************************************//**
  Set the diagram parent to point to 'tech' location.
****************************************************************************/
static void science_diagram_center(GtkWidget *diagram, Tech_type_id tech)
{
  GtkScrolledWindow *sw = GTK_SCROLLED_WINDOW(gtk_widget_get_parent(diagram));
  struct reqtree *reqtree;
  int x, y, width, height;

  if (!GTK_IS_SCROLLED_WINDOW(sw)) {
    return;
  }

  reqtree = g_object_get_data(G_OBJECT(diagram), "reqtree");
  get_reqtree_dimensions(reqtree, &width, &height);
  if (find_tech_on_reqtree(reqtree, tech, &x, &y, NULL, NULL)) {
    GtkAdjustment *adjust = NULL;
    gdouble value;

    adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(sw));
    value = (gtk_adjustment_get_lower(adjust)
      + gtk_adjustment_get_upper(adjust)
      - gtk_adjustment_get_page_size(adjust)) / width * x;
    gtk_adjustment_set_value(adjust, value);
    gtk_adjustment_value_changed(adjust);

    adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw));
    value = (gtk_adjustment_get_lower(adjust)
      + gtk_adjustment_get_upper(adjust)
      - gtk_adjustment_get_page_size(adjust)) / height * y;
    gtk_adjustment_set_value(adjust, value);
    gtk_adjustment_value_changed(adjust);
  }
}

/************************************************************************//**
  Resize and redraw the requirement tree.
****************************************************************************/
static void science_report_redraw(struct science_report *preport)
{
  Tech_type_id researching;

  fc_assert_ret(NULL != preport);

  science_diagram_data(GTK_WIDGET(preport->drawing_area),
                       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
                         preport->button_show_all)));

  if (client_has_player()) {
    researching = research_get(client_player())->researching;
  } else {
    researching = A_UNSET;
  }
  science_diagram_center(GTK_WIDGET(preport->drawing_area), researching);

  gtk_widget_queue_draw(GTK_WIDGET(preport->drawing_area));
}

/************************************************************************//**
  Utility for g_list_sort.
****************************************************************************/
static gint cmp_func(gconstpointer a_p, gconstpointer b_p)
{
  const gchar *a_str, *b_str;
  gint a = GPOINTER_TO_INT(a_p), b = GPOINTER_TO_INT(b_p);
  const struct research *presearch = research_get(client_player());

  a_str = research_advance_name_translation(presearch, a);
  b_str = research_advance_name_translation(presearch, b);

  return fc_strcoll(a_str, b_str);
}

/************************************************************************//**
  Update a science report dialog.
****************************************************************************/
static void science_report_update(struct science_report *preport)
{
  GtkListStore *store;
  GtkTreeIter iter;
  GList *sorting_list, *item;
  struct research *presearch = research_get(client_player());
  const char *text;
  double pct;
  Tech_type_id tech;

  fc_assert_ret(NULL != preport);
  fc_assert_ret(NULL != presearch);

  /* Disable callbacks. */
  science_report_no_combo_callback = TRUE;

  gtk_widget_queue_draw(GTK_WIDGET(preport->drawing_area));

  gtk_label_set_text(preport->main_label, science_dialog_text());

  /* Update the progress bar. */
  text = get_science_target_text(&pct);
  gtk_progress_bar_set_text(preport->progress_bar, text);
  gtk_progress_bar_set_fraction(preport->progress_bar, pct);
  /* Work around GTK+ refresh bug? */
  gtk_widget_queue_resize(GTK_WIDGET(preport->progress_bar));

  /* Update reachable techs. */
  store = GTK_LIST_STORE(gtk_combo_box_get_model(preport->reachable_techs));
  gtk_list_store_clear(store);
  sorting_list = NULL;
  if (A_UNSET == presearch->researching
      || is_future_tech(presearch->researching)) {
    gtk_list_store_append(store, &iter);
    science_report_store_set(store, &iter, presearch->researching);
    gtk_combo_box_set_active_iter(preport->reachable_techs, &iter);
  }

  /* Collect all techs which are reachable in the next step. */
  advance_index_iterate(A_FIRST, i) {
    if (TECH_PREREQS_KNOWN == presearch->inventions[i].state) {
      sorting_list = g_list_prepend(sorting_list, GINT_TO_POINTER(i));
    }
  } advance_index_iterate_end;

  /* Sort the list, append it to the store. */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  for (item = sorting_list; NULL != item; item = g_list_next(item)) {
    tech = GPOINTER_TO_INT(item->data);
    gtk_list_store_append(store, &iter);
    science_report_store_set(store, &iter, tech);
    if (tech == presearch->researching) {
      gtk_combo_box_set_active_iter(preport->reachable_techs, &iter);
    }
  }

  /* Free, re-init. */
  g_list_free(sorting_list);
  sorting_list = NULL;
  store = GTK_LIST_STORE(gtk_combo_box_get_model(preport->reachable_goals));
  gtk_list_store_clear(store);

  /* Update the tech goal. */
  gtk_label_set_text(preport->goal_label,
                     get_science_goal_text(presearch->tech_goal));

  if (A_UNSET == presearch->tech_goal) {
    gtk_list_store_append(store, &iter);
    science_report_store_set(store, &iter, A_UNSET);
    gtk_combo_box_set_active_iter(preport->reachable_goals, &iter);
  }

  /* Collect all techs which are reachable in next 10 steps. */
  advance_index_iterate(A_FIRST, i) {
    if (research_invention_reachable(presearch, i)
        && TECH_KNOWN != presearch->inventions[i].state
        && (i == presearch->tech_goal
            || 10 >= presearch->inventions[i].num_required_techs)) {
      sorting_list = g_list_prepend(sorting_list, GINT_TO_POINTER(i));
    }
  } advance_index_iterate_end;

  /* Sort the list, append it to the store. */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  for (item = sorting_list; NULL != item; item = g_list_next(item)) {
    tech = GPOINTER_TO_INT(item->data);
    gtk_list_store_append(store, &iter);
    science_report_store_set(store, &iter, tech);
    if (tech == presearch->tech_goal) {
      gtk_combo_box_set_active_iter(preport->reachable_goals, &iter);
    }
  }

  /* Free. */
  g_list_free(sorting_list);

  /* Re-enable callbacks. */
  science_report_no_combo_callback = FALSE;
}

/************************************************************************//**
  Actived item in the reachable techs combo box.
****************************************************************************/
static void science_report_current_callback(GtkComboBox *combo,
                                            gpointer data)
{
  Tech_type_id tech;
  const char *tech_name;

  if (!science_report_combo_get_active(combo, &tech, &tech_name)) {
    return;
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data))) {
    popup_help_dialog_typed(tech_name, HELP_TECH);
  } else if (can_client_issue_orders()) {
    dsend_packet_player_research(&client.conn, tech);
  }
  /* Revert, or we will be not synchron with the server. */
  science_report_combo_set_active(combo, research_get
                                  (client_player())->researching);
}

/************************************************************************//**
  Show or hide unreachable techs.
****************************************************************************/
static void science_report_show_all_callback(GtkComboBox *combo,
                                             gpointer data)
{
  struct science_report *preport = (struct science_report *) data;

  science_report_redraw(preport);
}

/************************************************************************//**
  Actived item in the reachable goals combo box.
****************************************************************************/
static void science_report_goal_callback(GtkComboBox *combo, gpointer data)
{
  Tech_type_id tech;
  const char *tech_name;

  if (!science_report_combo_get_active(combo, &tech, &tech_name)) {
    return;
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data))) {
    popup_help_dialog_typed(tech_name, HELP_TECH);
  } else if (can_client_issue_orders()) {
    dsend_packet_player_tech_goal(&client.conn, tech);
  }
  /* Revert, or we will be not synchron with the server. */
  science_report_combo_set_active(combo, research_get
                                  (client_player())->tech_goal);
}

/************************************************************************//**
  Initialize a science report.
****************************************************************************/
static void science_report_init(struct science_report *preport)
{
  GtkWidget *frame, *table, *help_button, *show_all_button, *sw, *w;
  GtkSizeGroup *group;
  GtkContainer *vbox;
  GtkListStore *store;
  GtkCellRenderer *renderer;

  fc_assert_ret(NULL != preport);

  gui_dialog_new(&preport->shell, GTK_NOTEBOOK(top_notebook), NULL, TRUE);
  /* TRANS: Research report title */
  gui_dialog_set_title(preport->shell, _("Research"));

  gui_dialog_add_button(preport->shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gui_dialog_set_default_response(preport->shell, GTK_RESPONSE_CLOSE);

  vbox = GTK_CONTAINER(preport->shell->vbox);
  group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  w = gtk_label_new(NULL);
  gtk_container_add(vbox, w);
  preport->main_label = GTK_LABEL(w);

  /* Current research target line. */
  frame = gtk_frame_new(_("Researching"));
  gtk_container_add(vbox, frame);

  table = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(table), 4);
  gtk_container_add(GTK_CONTAINER(frame), table);

  help_button = gtk_check_button_new_with_label(_("Help"));
  gtk_grid_attach(GTK_GRID(table), help_button, 5, 0, 1, 1);

  store = science_report_store_new();
  w = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
  gtk_size_group_add_widget(group, w);
  g_object_unref(G_OBJECT(store));
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(w), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(w), renderer, "text",
                                 SRD_COL_NAME, NULL);
  gtk_widget_set_sensitive(w, can_client_issue_orders());
  g_signal_connect(w, "changed", G_CALLBACK(science_report_current_callback),
                   help_button);
  gtk_grid_attach(GTK_GRID(table), w, 0, 0, 1, 1);
  preport->reachable_techs = GTK_COMBO_BOX(w);

  w = gtk_progress_bar_new();
  gtk_widget_set_hexpand(w, TRUE);
  gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(w), TRUE);
  gtk_grid_attach(GTK_GRID(table), w, 2, 0, 1, 1);
  gtk_widget_set_size_request(w, -1, 25);
  preport->progress_bar = GTK_PROGRESS_BAR(w);

  /* Research goal line. */
  frame = gtk_frame_new( _("Goal"));
  gtk_container_add(vbox, frame);

  table = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(table), 4);
  gtk_container_add(GTK_CONTAINER(frame),table);

  store = science_report_store_new();
  w = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
  gtk_size_group_add_widget(group, w);
  g_object_unref(G_OBJECT(store));
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(w), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(w), renderer, "text",
                                 SRD_COL_NAME, NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(w), renderer, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(w), renderer, "text",
                                 SRD_COL_STEPS, NULL);
  gtk_widget_set_sensitive(w, can_client_issue_orders());
  g_signal_connect(w, "changed", G_CALLBACK(science_report_goal_callback),
                   help_button);
  gtk_grid_attach(GTK_GRID(table), w, 0, 0, 1, 1);
  preport->reachable_goals = GTK_COMBO_BOX(w);

  w = gtk_label_new(NULL);
  gtk_widget_set_hexpand(w, TRUE);
  gtk_grid_attach(GTK_GRID(table), w, 2, 0, 1, 1);
  gtk_widget_set_size_request(w, -1, 25);
  preport->goal_label = GTK_LABEL(w);

  /* Toggle "Show All" button. */
  /* TRANS: As in 'Show all (even not reachable) techs'. */
  show_all_button = gtk_toggle_button_new_with_label(_("Show all"));
  gtk_grid_attach(GTK_GRID(table), show_all_button, 5, 0, 1, 1);
  g_signal_connect(show_all_button, "toggled",
                   G_CALLBACK(science_report_show_all_callback), preport);
  gtk_widget_set_sensitive(show_all_button, can_client_issue_orders()
                                             && !client_is_global_observer());
  preport->button_show_all = show_all_button;

  /* Science diagram. */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(vbox, sw);

  w = science_diagram_new();
  gtk_widget_set_hexpand(w, TRUE);
  gtk_widget_set_vexpand(w, TRUE);
  gtk_container_add(GTK_CONTAINER(sw), w);
  preport->drawing_area = GTK_LAYOUT(w);

  science_report_update(preport);
  gui_dialog_show_all(preport->shell);
  gtk_widget_queue_draw(GTK_WIDGET(preport->drawing_area));
  g_object_unref(group);

  /* This must be _after_ the dialog is drawn to really center it ... */
  science_report_redraw(preport);
}

/************************************************************************//**
  Free a science report.
****************************************************************************/
static void science_report_free(struct science_report *preport)
{
  fc_assert_ret(NULL != preport);

  gui_dialog_destroy(preport->shell);
  fc_assert(NULL == preport->shell);

  memset(preport, 0, sizeof(*preport));
}

/************************************************************************//**
  Create the science report is needed.
****************************************************************************/
void science_report_dialog_popup(bool raise)
{
  struct research *presearch = research_get(client_player());

  if (NULL == science_report.shell) {
    science_report_init(&science_report);
  }

  if (NULL != presearch
      && A_UNSET == presearch->tech_goal
      && A_UNSET == presearch->researching) {
    gui_dialog_alert(science_report.shell);
  } else {
    gui_dialog_present(science_report.shell);
  }

  if (raise) {
    gui_dialog_raise(science_report.shell);
  }
}

/************************************************************************//**
  Closes the science report dialog.
****************************************************************************/
void science_report_dialog_popdown(void)
{
  if (NULL != science_report.shell) {
    science_report_free(&science_report);
    fc_assert(NULL == science_report.shell);
  }
}

/************************************************************************//**
  Update the science report dialog.
****************************************************************************/
void real_science_report_dialog_update(void *unused)
{
  if (NULL != science_report.shell) {
    science_report_update(&science_report);
  }
}

/************************************************************************//**
  Resize and redraw the requirement tree.
****************************************************************************/
void science_report_dialog_redraw(void)
{
  if (NULL != science_report.shell) {
    science_report_redraw(&science_report);
  }
}


/****************************************************************************
                      ECONOMY REPORT DIALOG
****************************************************************************/
struct economy_report {
  struct gui_dialog *shell;
  GtkTreeView *tree_view;
  GtkLabel *label;
};

static struct economy_report economy_report = { NULL, NULL, NULL };

enum economy_report_response {
  ERD_RES_SELL_REDUNDANT = 1,
  ERD_RES_SELL_ALL,
  ERD_RES_DISBAND_UNITS
};

/* Those values must match the functions economy_report_store_new() and
 * economy_report_column_name(). */
enum economy_report_columns {
  ERD_COL_SPRITE,
  ERD_COL_NAME,
  ERD_COL_REDUNDANT,
  ERD_COL_COUNT,
  ERD_COL_COST,
  ERD_COL_TOTAL_COST,

  /* Not visible. */
  ERD_COL_IS_IMPROVEMENT,
  ERD_COL_CID,

  ERD_COL_NUM
};

/************************************************************************//**
  Create a new economy report list store.
****************************************************************************/
static GtkListStore *economy_report_store_new(void)
{
  return gtk_list_store_new(ERD_COL_NUM,
                            GDK_TYPE_PIXBUF,    /* ERD_COL_SPRITE */
                            G_TYPE_STRING,      /* ERD_COL_NAME */
                            G_TYPE_INT,         /* ERD_COL_REDUNDANT */
                            G_TYPE_INT,         /* ERD_COL_COUNT */
                            G_TYPE_INT,         /* ERD_COL_COST */
                            G_TYPE_INT,         /* ERD_COL_TOTAL_COST */
                            G_TYPE_BOOLEAN,     /* ERD_COL_IS_IMPROVEMENT */
                            G_TYPE_INT,         /* ERD_COL_UNI_KIND */
                            G_TYPE_INT);        /* ERD_COL_UNI_VALUE_ID */
}

/************************************************************************//**
  Returns the title of the column (translated).
****************************************************************************/
static const char *
economy_report_column_name(enum economy_report_columns col)
{
  switch (col) {
  case ERD_COL_SPRITE:
    /* TRANS: Image header */
    return _("Type");
  case ERD_COL_NAME:
    return Q_("?Building or Unit type:Name");
  case ERD_COL_REDUNDANT:
    return _("Redundant");
  case ERD_COL_COUNT:
    return _("Count");
  case ERD_COL_COST:
    return _("Cost");
  case ERD_COL_TOTAL_COST:
    /* TRANS: Upkeep total, count*cost. */
    return _("U Total");
  case ERD_COL_IS_IMPROVEMENT:
  case ERD_COL_CID:
  case ERD_COL_NUM:
    break;
  }

  return NULL;
}

/************************************************************************//**
  Update the economy report dialog.
****************************************************************************/
static void economy_report_update(struct economy_report *preport)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;
  GdkPixbuf *pix;
  struct improvement_entry building_entries[B_LAST];
  struct unit_entry unit_entries[U_LAST];
  int entries_used, building_total, unit_total, tax, i;
  char buf[256];
  cid selected;

  fc_assert_ret(NULL != preport);

  /* Save the selection. */
  selection = gtk_tree_view_get_selection(preport->tree_view);
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, ERD_COL_CID, &selected, -1);
  } else {
    selected = -1;
  }

  model = gtk_tree_view_get_model(preport->tree_view);
  store = GTK_LIST_STORE(model);
  gtk_list_store_clear(store);

  /* Buildings. */
  get_economy_report_data(building_entries, &entries_used,
                          &building_total, &tax);
  for (i = 0; i < entries_used; i++) {
    struct improvement_entry *pentry = building_entries + i;
    struct impr_type *pimprove = pentry->type;
    struct sprite *sprite = get_building_sprite(tileset, pimprove);
    cid id = cid_encode_building(pimprove);

    pix = sprite_get_pixbuf(sprite);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       ERD_COL_SPRITE, pix,
                       ERD_COL_NAME, improvement_name_translation(pimprove),
                       ERD_COL_REDUNDANT, pentry->redundant,
                       ERD_COL_COUNT, pentry->count,
                       ERD_COL_COST, pentry->cost,
                       ERD_COL_TOTAL_COST, pentry->total_cost,
                       ERD_COL_IS_IMPROVEMENT, TRUE,
                       ERD_COL_CID, id,
                       -1);
    g_object_unref(G_OBJECT(pix));
    if (selected == id) {
      /* Restore the selection. */
      gtk_tree_selection_select_iter(selection, &iter);
    }
  }

  /* Units. */
  get_economy_report_units_data(unit_entries, &entries_used, &unit_total);
  for (i = 0; i < entries_used; i++) {
    struct unit_entry *pentry = unit_entries + i;
    struct unit_type *putype = pentry->type;
    struct sprite *sprite = get_unittype_sprite(tileset, putype,
                                                direction8_invalid());
    cid id = cid_encode_unit(putype);

    pix = sprite_get_pixbuf(sprite);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       ERD_COL_SPRITE, pix,
                       ERD_COL_NAME, utype_name_translation(putype),
                       ERD_COL_REDUNDANT, 0,
                       ERD_COL_COUNT, pentry->count,
                       ERD_COL_COST, pentry->cost,
                       ERD_COL_TOTAL_COST, pentry->total_cost,
                       ERD_COL_IS_IMPROVEMENT, FALSE,
                       ERD_COL_CID, id,
                       -1);
    g_object_unref(G_OBJECT(pix));
    if (selected == id) {
      /* Restore the selection. */
      gtk_tree_selection_select_iter(selection, &iter);
    }
  }

  /* Update the label. */
  fc_snprintf(buf, sizeof(buf), _("Income: %d    Total Costs: %d"),
              tax, building_total + unit_total);
  gtk_label_set_text(preport->label, buf);
}

/************************************************************************//**
  Issue a command on the economy report.
****************************************************************************/
static void economy_report_command_callback(struct gui_dialog *pdialog,
                                            int response,
                                            gpointer data)
{
  struct economy_report *preport = data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection(preport->tree_view);
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkWidget *shell;
  struct universal selected;
  cid id;
  char buf[256] = "";

  switch (response) {
  case ERD_RES_SELL_REDUNDANT:
  case ERD_RES_SELL_ALL:
  case ERD_RES_DISBAND_UNITS:
    break;
  default:
    gui_dialog_destroy(pdialog);
    return;
  }

  if (!can_client_issue_orders()
      || !gtk_tree_selection_get_selected(selection, &model, &iter)) {
    return;
  }

  gtk_tree_model_get(model, &iter, ERD_COL_CID, &id, -1);
  selected = cid_decode(id);

  switch (selected.kind) {
  case VUT_IMPROVEMENT:
    {
      struct impr_type *pimprove = selected.value.building;

      if (can_sell_building(pimprove)
          && (ERD_RES_SELL_ALL == response
              || (ERD_RES_SELL_REDUNDANT == response))) {
        bool redundant = (ERD_RES_SELL_REDUNDANT == response);
        gint count;
        gtk_tree_model_get(model, &iter,
                           redundant ? ERD_COL_REDUNDANT : ERD_COL_COUNT,
                           &count, -1);
        if (count == 0) {
          break;
        }
        shell = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL
                                       | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_QUESTION,
                                       GTK_BUTTONS_YES_NO,
                                       redundant
                                       /* TRANS: %s is an improvement */
                                       ? _("Do you really wish to sell "
                                           "every redundant %s (%d total)?")
                                       /* TRANS: %s is an improvement */
                                       : _("Do you really wish to sell "
                                           "every %s (%d total)?"),
                                       improvement_name_translation(pimprove),
                                       count);
        setup_dialog(shell, gui_dialog_get_toplevel(pdialog));
        gtk_window_set_title(GTK_WINDOW(shell), _("Sell Improvements"));

        if (GTK_RESPONSE_YES == gtk_dialog_run(GTK_DIALOG(shell))) {
          sell_all_improvements(pimprove, redundant, buf, sizeof(buf));
        }
        gtk_widget_destroy(shell);
      }
    }
    break;
  case VUT_UTYPE:
    {
      if (ERD_RES_DISBAND_UNITS == response) {
        struct unit_type *putype = selected.value.utype;
        gint count;
        gtk_tree_model_get(model, &iter, ERD_COL_COUNT, &count, -1);

        shell = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL
                                       | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_QUESTION,
                                       GTK_BUTTONS_YES_NO,
                                       /* TRANS: %s is a unit */
                                       _("Do you really wish to disband "
                                         "every %s (%d total)?"),
                                       utype_name_translation(putype),
                                       count);
        setup_dialog(shell, gui_dialog_get_toplevel(pdialog));
        gtk_window_set_title(GTK_WINDOW(shell), _("Disband Units"));

        if (GTK_RESPONSE_YES == gtk_dialog_run(GTK_DIALOG(shell))) {
          disband_all_units(putype, FALSE, buf, sizeof(buf));
        }
        gtk_widget_destroy(shell);
      }
    }
    break;
  default:
    log_error("Not supported type: %d.", selected.kind);
  }

  if ('\0' != buf[0]) {
    shell = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                                   "%s", buf);
    setup_dialog(shell, gui_dialog_get_toplevel(pdialog));
    g_signal_connect(shell, "response", G_CALLBACK(gtk_widget_destroy),
                     NULL);
    gtk_window_set_title(GTK_WINDOW(shell), _("Sell-Off: Results"));
    gtk_window_present(GTK_WINDOW(shell));
  }
}

/************************************************************************//**
  Called when a building or a unit type is selected in the economy list.
****************************************************************************/
static void economy_report_selection_callback(GtkTreeSelection *selection,
                                              gpointer data)
{
  struct gui_dialog *pdialog = ((struct economy_report *)data)->shell;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (can_client_issue_orders()
      && gtk_tree_selection_get_selected(selection, &model, &iter)) {
    struct universal selected;
    cid id;

    gtk_tree_model_get(model, &iter, ERD_COL_CID, &id, -1);
    selected = cid_decode(id);
    switch (selected.kind) {
    case VUT_IMPROVEMENT:
      {
        bool can_sell = can_sell_building(selected.value.building);
        gint redundant;
        gtk_tree_model_get(model, &iter, ERD_COL_REDUNDANT, &redundant, -1);

        gui_dialog_set_response_sensitive(pdialog, ERD_RES_SELL_REDUNDANT,
                                          can_sell && redundant > 0);
        gui_dialog_set_response_sensitive(pdialog, ERD_RES_SELL_ALL, can_sell);
        gui_dialog_set_response_sensitive(pdialog, ERD_RES_DISBAND_UNITS,
                                          FALSE);
      }
      return;
    case VUT_UTYPE:
      gui_dialog_set_response_sensitive(pdialog, ERD_RES_SELL_REDUNDANT,
                                        FALSE);
      gui_dialog_set_response_sensitive(pdialog, ERD_RES_SELL_ALL, FALSE);
      gui_dialog_set_response_sensitive(pdialog, ERD_RES_DISBAND_UNITS,
                                        TRUE);
      return;
    default:
      log_error("Not supported type: %d.", selected.kind);
      break;
    }
  }

  gui_dialog_set_response_sensitive(pdialog, ERD_RES_SELL_REDUNDANT, FALSE);
  gui_dialog_set_response_sensitive(pdialog, ERD_RES_SELL_ALL, FALSE);
  gui_dialog_set_response_sensitive(pdialog, ERD_RES_DISBAND_UNITS, FALSE);
}

/************************************************************************//**
  Create a new economy report.
****************************************************************************/
static void economy_report_init(struct economy_report *preport)
{
  GtkWidget *view, *sw, *label, *button;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkContainer *vbox;
  const char *title;
  enum economy_report_columns i;

  fc_assert_ret(NULL != preport);

  gui_dialog_new(&preport->shell, GTK_NOTEBOOK(top_notebook), preport, TRUE);
  gui_dialog_set_title(preport->shell, _("Economy"));
  vbox = GTK_CONTAINER(preport->shell->vbox);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_halign(sw, GTK_ALIGN_CENTER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  store = economy_report_store_new();
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_vexpand(view, TRUE);
  g_object_unref(store);
  gtk_widget_set_name(view, "small_font");
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  gtk_container_add(GTK_CONTAINER(sw), view);
  preport->tree_view = GTK_TREE_VIEW(view);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_signal_connect(selection, "changed",
                   G_CALLBACK(economy_report_selection_callback), preport);

  for (i = 0; (title = economy_report_column_name(i)); i++) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;
    GType type = gtk_tree_model_get_column_type(GTK_TREE_MODEL(store), i);

    if (GDK_TYPE_PIXBUF == type) {
      renderer = gtk_cell_renderer_pixbuf_new();
      col = gtk_tree_view_column_new_with_attributes(title, renderer,
                                                     "pixbuf", i, NULL);
#if 0
    } else if (G_TYPE_BOOLEAN == type) {
      renderer = gtk_cell_renderer_toggle_new();
      col = gtk_tree_view_column_new_with_attributes(title, renderer,
                                                     "active", i, NULL);
#endif
    } else {
      bool is_redundant = (i == ERD_COL_REDUNDANT);
      renderer = gtk_cell_renderer_text_new();
      if (is_redundant) {
        /* Special treatment: hide "Redundant" column for units */
        col = gtk_tree_view_column_new_with_attributes(title, renderer,
                                                       "text", i,
                                                       "visible",
                                                       ERD_COL_IS_IMPROVEMENT,
                                                       NULL);
      } else {
        col = gtk_tree_view_column_new_with_attributes(title, renderer,
                                                       "text", i, NULL);
      }
    }

    if (i > 1) {
      g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_alignment(col, 1.0);
    }

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  }

  label = gtk_label_new(NULL);
  gtk_container_add(vbox, label);
  gtk_widget_set_margin_left(label, 5);
  gtk_widget_set_margin_right(label, 5);
  gtk_widget_set_margin_top(label, 5);
  gtk_widget_set_margin_bottom(label, 5);
  preport->label = GTK_LABEL(label);

  gui_dialog_add_button(preport->shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  button = gui_dialog_add_button(preport->shell, _("_Disband"),
                                 ERD_RES_DISBAND_UNITS);
  gtk_widget_set_sensitive(button, FALSE);

  button = gui_dialog_add_button(preport->shell, _("Sell _All"),
                                 ERD_RES_SELL_ALL);
  gtk_widget_set_sensitive(button, FALSE);

  button = gui_dialog_add_button(preport->shell, _("Sell _Redundant"),
                                 ERD_RES_SELL_REDUNDANT);
  gtk_widget_set_sensitive(button, FALSE);

  gui_dialog_set_default_response(preport->shell, GTK_RESPONSE_CLOSE);
  gui_dialog_response_set_callback(preport->shell,
                                   economy_report_command_callback);

  gui_dialog_set_default_size(preport->shell, -1, 350);
  gui_dialog_show_all(preport->shell);

  economy_report_update(preport);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/************************************************************************//**
  Free an economy report.
****************************************************************************/
static void economy_report_free(struct economy_report *preport)
{
  fc_assert_ret(NULL != preport);

  gui_dialog_destroy(preport->shell);
  fc_assert(NULL == preport->shell);

  memset(preport, 0, sizeof(*preport));
}

/************************************************************************//**
  Create the economy report if needed.
****************************************************************************/
void economy_report_dialog_popup(bool raise)
{
  if (NULL == economy_report.shell) {
    economy_report_init(&economy_report);
  }

  gui_dialog_present(economy_report.shell);
  if (raise) {
    gui_dialog_raise(economy_report.shell);
  }
}

/************************************************************************//**
  Close the economy report dialog.
****************************************************************************/
void economy_report_dialog_popdown(void)
{
  if (NULL != economy_report.shell) {
    economy_report_free(&economy_report);
  }
}

/************************************************************************//**
  Update the economy report dialog.
****************************************************************************/
void real_economy_report_dialog_update(void *unused)
{
  if (NULL != economy_report.shell) {
    economy_report_update(&economy_report);
  }
}


/****************************************************************************
                           UNITS REPORT DIALOG
****************************************************************************/
struct units_report {
  struct gui_dialog *shell;
  GtkTreeView *tree_view;
};

static struct units_report units_report = { NULL, NULL };

enum units_report_response {
  URD_RES_NEAREST = 1,
  URD_RES_UPGRADE
};

/* Those values must match the order of unit_report_columns[]. */
enum units_report_columns {
  URD_COL_UTYPE_NAME,
  URD_COL_UPGRADABLE,
  URD_COL_N_UPGRADABLE,
  URD_COL_IN_PROGRESS,
  URD_COL_ACTIVE,
  URD_COL_SHIELD,
  URD_COL_FOOD,
  URD_COL_GOLD,

  /* Not visible. */
  URD_COL_TEXT_WEIGHT,
  URD_COL_UPG_VISIBLE,
  URD_COL_NUPG_VISIBLE,
  URD_COL_UTYPE_ID,

  URD_COL_NUM
};

static const struct {
  GType type;
  const char *title;
  const char *tooltip;
  bool rightalign;
  int visible_col;
} unit_report_columns[] = {
  { /* URD_COL_UTYPE_NAME */   G_TYPE_STRING,  N_("Unit Type"),
    NULL,                         FALSE,  -1 },
  { /* URD_COL_UPGRADABLE */   G_TYPE_BOOLEAN, N_("?Upgradable unit [short]:U"),
    N_("Upgradable"),             TRUE,   URD_COL_UPG_VISIBLE },
  { /* URD_COL_N_UPGRADABLE */ G_TYPE_INT,     "" /* merge with previous col */,
    NULL,                         TRUE,   URD_COL_NUPG_VISIBLE },
  /* TRANS: "In progress" abbreviation. */
  { /* URD_COL_IN_PROGRESS */  G_TYPE_INT,     N_("In-Prog"),
    N_("In progress"),            TRUE,   -1 },
  { /* URD_COL_ACTIVE */       G_TYPE_INT,     N_("Active"),
    NULL,                         TRUE,   -1 },
  { /* URD_COL_SHIELD */       G_TYPE_INT,     N_("Shield"),
    N_("Total shield upkeep"),    TRUE,   -1 },
  { /* URD_COL_FOOD */         G_TYPE_INT,     N_("Food"),
    N_("Total food upkeep"),      TRUE,   -1 },
  { /* URD_COL_GOLD */         G_TYPE_INT,     N_("Gold"),
    N_("Total gold upkeep"),      TRUE,   -1 },
  { /* URD_COL_TEXT_WEIGHT */  G_TYPE_INT,     NULL /* ... */ },
  { /* URD_COL_UPG_VISIBLE */  G_TYPE_BOOLEAN, NULL /* ... */ },
  { /* URD_COL_NUPG_VISIBLE */ G_TYPE_BOOLEAN, NULL /* ... */ },
  { /* URD_COL_UTYPE_ID */     G_TYPE_INT,     NULL /* ... */ }
};

/************************************************************************//**
  Create a new units report list store.
****************************************************************************/
static GtkListStore *units_report_store_new(void)
{
  int i;
  GType cols[URD_COL_NUM];
  fc_assert(ARRAY_SIZE(unit_report_columns) == URD_COL_NUM);

  for (i=0; i<URD_COL_NUM; i++) {
    cols[i] = unit_report_columns[i].type;
  }
  
  return gtk_list_store_newv(URD_COL_NUM, cols);
}

/************************************************************************//**
  Update the units report.
****************************************************************************/
static void units_report_update(struct units_report *preport)
{
  struct urd_info {
    int active_count;
    int building_count;
    int upkeep[O_LAST];
  };

  struct urd_info unit_array[utype_count()];
  struct urd_info unit_totals;
  struct urd_info *info;
  int total_upgradable_count = 0;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;
  Unit_type_id selected, utype_id;

  fc_assert_ret(NULL != preport);

  memset(unit_array, '\0', sizeof(unit_array));
  memset(&unit_totals, '\0', sizeof(unit_totals));

  /* Count units. */
  players_iterate(pplayer) {
    if (client_has_player() && pplayer != client_player()) {
      continue;
    }

    unit_list_iterate(pplayer->units, punit) {
      info = unit_array + utype_index(unit_type_get(punit));

      if (0 != punit->homecity) {
        output_type_iterate(o) {
          info->upkeep[o] += punit->upkeep[o];
        } output_type_iterate_end;
      }
      info->active_count++;
    } unit_list_iterate_end;
    city_list_iterate(pplayer->cities, pcity) {
      if (VUT_UTYPE == pcity->production.kind) {
        int num_units;
        info = unit_array + utype_index(pcity->production.value.utype);
        /* Account for build slots in city */
        (void) city_production_build_units(pcity, TRUE, &num_units);
        /* Unit is in progress even if it won't be done this turn */
        num_units = MAX(num_units, 1);
        info->building_count += num_units;
      }
    } city_list_iterate_end;
  } players_iterate_end;

  /* Save selection. */
  selection = gtk_tree_view_get_selection(preport->tree_view);
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, URD_COL_UTYPE_ID, &selected, -1);
  } else {
    selected = -1;
  }

  /* Make the store. */
  model = gtk_tree_view_get_model(preport->tree_view);
  store = GTK_LIST_STORE(model);
  gtk_list_store_clear(store);

  unit_type_iterate(utype) {
    bool upgradable;

    utype_id = utype_index(utype);
    info = unit_array + utype_id;

    if (0 == info->active_count && 0 == info->building_count) {
      continue;         /* We don't need a row for this type. */
    }

    upgradable = client_has_player()
                 && NULL != can_upgrade_unittype(client_player(), utype);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       URD_COL_UTYPE_NAME, utype_name_translation(utype),
                       URD_COL_UPGRADABLE, upgradable,
                       URD_COL_N_UPGRADABLE, 0, /* never displayed */
                       URD_COL_IN_PROGRESS, info->building_count,
                       URD_COL_ACTIVE, info->active_count,
                       URD_COL_SHIELD, info->upkeep[O_SHIELD],
                       URD_COL_FOOD, info->upkeep[O_FOOD],
                       URD_COL_GOLD, info->upkeep[O_GOLD],
                       URD_COL_TEXT_WEIGHT, PANGO_WEIGHT_NORMAL,
                       URD_COL_UPG_VISIBLE, TRUE,
                       URD_COL_NUPG_VISIBLE, FALSE,
                       URD_COL_UTYPE_ID, utype_id,
                       -1);
    if (selected == utype_id) {
      /* Restore the selection. */
      gtk_tree_selection_select_iter(selection, &iter);
    }

    /* Update totals. */
    unit_totals.active_count += info->active_count;
    output_type_iterate(o) {
      unit_totals.upkeep[o] += info->upkeep[o];
    } output_type_iterate_end;
    unit_totals.building_count += info->building_count;
    if (upgradable) {
      total_upgradable_count += info->active_count;
    }
  } unit_type_iterate_end;

  /* Add the total row. */
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter,
                     URD_COL_UTYPE_NAME, _("Totals:"),
                     URD_COL_UPGRADABLE, FALSE, /* never displayed */
                     URD_COL_N_UPGRADABLE, total_upgradable_count,
                     URD_COL_IN_PROGRESS, unit_totals.building_count,
                     URD_COL_ACTIVE, unit_totals.active_count,
                     URD_COL_SHIELD, unit_totals.upkeep[O_SHIELD],
                     URD_COL_FOOD, unit_totals.upkeep[O_FOOD],
                     URD_COL_GOLD, unit_totals.upkeep[O_GOLD],
                     URD_COL_TEXT_WEIGHT, PANGO_WEIGHT_BOLD,
                     URD_COL_UPG_VISIBLE, FALSE,
                     URD_COL_NUPG_VISIBLE, TRUE,
                     URD_COL_UTYPE_ID, U_LAST,
                     -1);
  if (selected == U_LAST) {
    /* Restore the selection. */
    gtk_tree_selection_select_iter(selection, &iter);
  }
}

/************************************************************************//**
  GtkTreeSelection "changed" signal handler.
****************************************************************************/
static void units_report_selection_callback(GtkTreeSelection *selection,
                                            gpointer data)
{
  struct units_report *preport = data;
  GtkTreeModel *model;
  GtkTreeIter it;
  int active_count;
  struct unit_type *utype = NULL;

  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    int ut;

    gtk_tree_model_get(model, &it,
                       URD_COL_ACTIVE, &active_count,
                       URD_COL_UTYPE_ID, &ut,
                       -1);
    if (0 < active_count) {
      utype = utype_by_number(ut);
    }
  }

  if (NULL == utype) {
    gui_dialog_set_response_sensitive(preport->shell, URD_RES_NEAREST,
                                      FALSE);
    gui_dialog_set_response_sensitive(preport->shell, URD_RES_UPGRADE,
                                      FALSE);
  } else {
    gui_dialog_set_response_sensitive(preport->shell, URD_RES_NEAREST, TRUE);
    gui_dialog_set_response_sensitive(preport->shell, URD_RES_UPGRADE,
        (can_client_issue_orders()
         && NULL != can_upgrade_unittype(client_player(), utype)));
  }
}

/************************************************************************//**
  Returns the nearest unit of the type 'utype'.
****************************************************************************/
static struct unit *find_nearest_unit(const struct unit_type *utype,
                                      struct tile *ptile)
{
  struct unit *best_candidate = NULL;
  int best_dist = FC_INFINITY, dist;

  players_iterate(pplayer) {
    if (client_has_player() && pplayer != client_player()) {
      continue;
    }

    unit_list_iterate(pplayer->units, punit) {
      if (utype == unit_type_get(punit)
          && FOCUS_AVAIL == punit->client.focus_status
          && 0 < punit->moves_left
          && !punit->done_moving
          && !punit->ai_controlled) {
        dist = sq_map_distance(unit_tile(punit), ptile);
        if (dist < best_dist) {
          best_candidate = punit;
          best_dist = dist;
        }
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  return best_candidate;
}

/************************************************************************//**
  Gui dialog handler.
****************************************************************************/
static void units_report_command_callback(struct gui_dialog *pdialog,
                                          int response,
                                          gpointer data)
{
  struct units_report *preport = data;
  struct unit_type *utype = NULL;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;

  switch (response) {
  case URD_RES_NEAREST:
  case URD_RES_UPGRADE:
    break;
  default:
    gui_dialog_destroy(pdialog);
    return;
  }

  /* Nearest & upgrade commands. */
  selection = gtk_tree_view_get_selection(preport->tree_view);
  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    int ut;

    gtk_tree_model_get(model, &it, URD_COL_UTYPE_ID, &ut, -1);
    utype = utype_by_number(ut);
  }

  if (response == URD_RES_NEAREST) {
    struct tile *ptile;
    struct unit *punit;

    ptile = get_center_tile_mapcanvas();
    if ((punit = find_nearest_unit(utype, ptile))) {
      center_tile_mapcanvas(unit_tile(punit));

      if (ACTIVITY_IDLE == punit->activity
          || ACTIVITY_SENTRY == punit->activity) {
        if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
          unit_focus_set_and_select(punit);
        }
      }
    }
  } else if (can_client_issue_orders()) {
    GtkWidget *shell;
    struct unit_type *upgrade = can_upgrade_unittype(client_player(), utype);
    char buf[1024];
    int price = unit_upgrade_price(client_player(), utype, upgrade);

    fc_snprintf(buf, ARRAY_SIZE(buf), PL_("Treasury contains %d gold.",
                                          "Treasury contains %d gold.",
                                          client_player()->economic.gold),
                client_player()->economic.gold);

    shell = gtk_message_dialog_new(NULL,
                                   GTK_DIALOG_MODAL
                                   | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                   /* TRANS: Last %s is pre-pluralised
                                    * "Treasury contains %d gold." */
                                   PL_("Upgrade as many %s to %s as possible "
                                       "for %d gold each?\n%s",
                                       "Upgrade as many %s to %s as possible "
                                       "for %d gold each?\n%s", price),
                                   utype_name_translation(utype),
                                   utype_name_translation(upgrade),
                                   price, buf);
    setup_dialog(shell, gui_dialog_get_toplevel(preport->shell));

    gtk_window_set_title(GTK_WINDOW(shell), _("Upgrade Obsolete Units"));

    if (GTK_RESPONSE_YES == gtk_dialog_run(GTK_DIALOG(shell))) {
      dsend_packet_unit_type_upgrade(&client.conn, utype_number(utype));
    }

    gtk_widget_destroy(shell);
  }
}

/************************************************************************//**
  Create a units report.
****************************************************************************/
static void units_report_init(struct units_report *preport)
{
  GtkWidget *view, *sw, *button;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkContainer *vbox;
  GtkTreeViewColumn *col = NULL;
  enum units_report_columns i;

  fc_assert_ret(NULL != preport);

  gui_dialog_new(&preport->shell, GTK_NOTEBOOK(top_notebook), preport, TRUE);
  gui_dialog_set_title(preport->shell, _("Units"));
  vbox = GTK_CONTAINER(preport->shell->vbox);

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_halign(sw, GTK_ALIGN_CENTER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  store = units_report_store_new();
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_vexpand(view, TRUE);
  g_object_unref(store);
  gtk_widget_set_name(view, "small_font");
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  gtk_container_add(GTK_CONTAINER(sw), view);
  preport->tree_view = GTK_TREE_VIEW(view);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_signal_connect(selection, "changed",
                   G_CALLBACK(units_report_selection_callback), preport);

  for (i = 0; unit_report_columns[i].title != NULL; i++) {
    GtkCellRenderer *renderer;

    if (strlen(unit_report_columns[i].title) > 0) {
      GtkWidget *header = gtk_label_new(Q_(unit_report_columns[i].title));
      if (unit_report_columns[i].tooltip) {
        gtk_widget_set_tooltip_text(header,
                                    Q_(unit_report_columns[i].tooltip));
      }
      gtk_widget_show(header);
      col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_widget(col, header);
      if (unit_report_columns[i].rightalign) {
        gtk_tree_view_column_set_alignment(col, 1.0);
      }
      gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
    } /* else add new renderer to previous TreeViewColumn */

    fc_assert(col != NULL);
    if (G_TYPE_BOOLEAN == unit_report_columns[i].type) {
      renderer = gtk_cell_renderer_toggle_new();
      gtk_tree_view_column_pack_start(col, renderer, FALSE);
      gtk_tree_view_column_add_attribute(col, renderer, "active", i);
    } else {
      renderer = gtk_cell_renderer_text_new();
      gtk_tree_view_column_pack_start(col, renderer, TRUE);
      gtk_tree_view_column_add_attribute(col, renderer, "text", i);
      gtk_tree_view_column_add_attribute(col, renderer,
                                         "weight", URD_COL_TEXT_WEIGHT);
    }

    if (unit_report_columns[i].visible_col >= 0) {
      gtk_tree_view_column_add_attribute(col, renderer, "visible",
                                         unit_report_columns[i].visible_col);
    }

    if (unit_report_columns[i].rightalign) {
      g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
    }
  }

  gui_dialog_add_button(preport->shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  button = gui_dialog_add_button(preport->shell, _("_Upgrade"),
                                 URD_RES_UPGRADE);
  gtk_widget_set_sensitive(button, FALSE);

  button = gui_dialog_add_stockbutton(preport->shell, GTK_STOCK_FIND,
                                      _("Find _Nearest"), URD_RES_NEAREST);
  gtk_widget_set_sensitive(button, FALSE);

  gui_dialog_set_default_response(preport->shell, GTK_RESPONSE_CLOSE);
  gui_dialog_response_set_callback(preport->shell,
                                   units_report_command_callback);

  gui_dialog_set_default_size(preport->shell, -1, 350);
  gui_dialog_show_all(preport->shell);

  units_report_update(preport);
  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/************************************************************************//**
  Free an units report.
****************************************************************************/
static void units_report_free(struct units_report *preport)
{
  fc_assert_ret(NULL != preport);

  gui_dialog_destroy(preport->shell);
  fc_assert(NULL == preport->shell);

  memset(preport, 0, sizeof(*preport));
}

/************************************************************************//**
  Create the units report if needed.
****************************************************************************/
void units_report_dialog_popup(bool raise)
{
  if (NULL == units_report.shell) {
    units_report_init(&units_report);
  }

  gui_dialog_present(units_report.shell);
  if (raise) {
    gui_dialog_raise(units_report.shell);
  }
}

/************************************************************************//**
  Closes the units report dialog.
****************************************************************************/
void units_report_dialog_popdown(void)
{
  if (NULL != units_report.shell) {
    units_report_free(&units_report);
    fc_assert(NULL == units_report.shell);
  }
}

/************************************************************************//**
  Update the units report dialog.
****************************************************************************/
void real_units_report_dialog_update(void *unused)
{
  if (NULL != units_report.shell) {
    units_report_update(&units_report);
  }
}


/****************************************************************************
                         FINAL REPORT DIALOG
****************************************************************************/
struct endgame_report {
  struct gui_dialog *shell;
  GtkTreeView *tree_view;
  GtkListStore *store;
  int player_count;
  int players_received;
};

enum endgame_report_columns {
  FRD_COL_PLAYER,
  FRD_COL_NATION,
  FRD_COL_SCORE,

  FRD_COL_NUM
};

static struct endgame_report endgame_report = { NULL, NULL };

/************************************************************************//**
  Returns the title of the column (translated).
****************************************************************************/
static const char *
endgame_report_column_name(enum endgame_report_columns col)
{
  switch (col) {
  case FRD_COL_PLAYER:
    return _("Player\n");
  case FRD_COL_NATION:
    return _("Nation\n");
  case FRD_COL_SCORE:
    return _("Score\n");
  case FRD_COL_NUM:
    break;
  }

  return NULL;
}

/************************************************************************//**
  Fill a final report with statistics for each player.
****************************************************************************/
static void endgame_report_update(struct endgame_report *preport,
                                  const struct packet_endgame_report *packet)
{
  const size_t col_num = packet->category_num + FRD_COL_NUM;
  GType col_types[col_num];
  GtkListStore *store;
  GtkTreeViewColumn *col;
  int i;

  fc_assert_ret(NULL != preport);

  /* Remove the old columns. */
  while ((col = gtk_tree_view_get_column(preport->tree_view, 0))) {
    gtk_tree_view_remove_column(preport->tree_view, col);
  }

  /* Create the new model. */
  col_types[FRD_COL_PLAYER] = G_TYPE_STRING;
  col_types[FRD_COL_NATION] = GDK_TYPE_PIXBUF;
  col_types[FRD_COL_SCORE] = G_TYPE_INT;
  for (i = FRD_COL_NUM; (guint)i < col_num; i++) {
    col_types[i] = G_TYPE_INT;
  }
  store = gtk_list_store_newv(col_num, col_types);
  gtk_tree_view_set_model(preport->tree_view, GTK_TREE_MODEL(store));
  g_object_unref(G_OBJECT(store));

  /* Create the new columns. */
  for (i = 0; (guint)i < col_num; i++) {
    GtkCellRenderer *renderer;
    const char *title;
    const char *attribute;

    if (GDK_TYPE_PIXBUF == col_types[i]) {
      renderer = gtk_cell_renderer_pixbuf_new();
      attribute = "pixbuf";
    } else {
      renderer = gtk_cell_renderer_text_new();
      attribute = "text";
    }

    if (i < FRD_COL_NUM) {
      title = endgame_report_column_name(i);
    } else {
      title = packet->category_name[i - FRD_COL_NUM];
    }

    col = gtk_tree_view_column_new_with_attributes(Q_(title), renderer,
                                                   attribute, i, NULL);
    gtk_tree_view_append_column(preport->tree_view, col);
    if (GDK_TYPE_PIXBUF != col_types[i]) {
      gtk_tree_view_column_set_sort_column_id(col, i);
    }
  }

  preport->store = store;
  preport->player_count = packet->player_num;
  preport->players_received = 0;
}

/************************************************************************//**
  Handle endgame report information about one player.
****************************************************************************/
void endgame_report_dialog_player(const struct packet_endgame_player *packet)
{
  /* Fill the model with player stats. */
  struct endgame_report *preport = &endgame_report;
  const struct player *pplayer = player_by_number(packet->player_id);
  GtkTreeIter iter;
  int i;

  gtk_list_store_append(preport->store, &iter);
  gtk_list_store_set(preport->store, &iter,
                     FRD_COL_PLAYER, player_name(pplayer),
                     FRD_COL_NATION, get_flag(nation_of_player(pplayer)),
                     FRD_COL_SCORE, packet->score,
                     -1);
  for (i = 0; i < packet->category_num; i++) {
    gtk_list_store_set(preport->store, &iter,
                       i + FRD_COL_NUM, packet->category_score[i],
                       -1);
  }

  preport->players_received++;

  if (preport->players_received == preport->player_count) {
    gui_dialog_present(preport->shell);
  }
}

/************************************************************************//**
  Prepare a final report.
****************************************************************************/
static void endgame_report_init(struct endgame_report *preport)
{
  GtkWidget *sw, *view;

  fc_assert_ret(NULL != preport);

  gui_dialog_new(&preport->shell, GTK_NOTEBOOK(top_notebook), NULL, TRUE);
  gui_dialog_set_title(preport->shell, _("Score"));

  gui_dialog_set_default_size(preport->shell, 700, 420);

  /* Setup the layout. */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(preport->shell->vbox), sw);

  view = gtk_tree_view_new();
  gtk_widget_set_name(view, "small_font");
  gtk_container_add(GTK_CONTAINER(sw), view);
  preport->tree_view = GTK_TREE_VIEW(view);

  if (preport->shell->type == GUI_DIALOG_TAB) {
    gtk_widget_set_hexpand(GTK_WIDGET(view), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(view), TRUE);
  }

  gui_dialog_show_all(preport->shell);
}

/************************************************************************//**
  Start building a dialog with player statistics at endgame.
****************************************************************************/
void endgame_report_dialog_start(const struct packet_endgame_report *packet)
{
  if (NULL == endgame_report.shell) {
    endgame_report_init(&endgame_report);
  }
  endgame_report_update(&endgame_report, packet);
}
