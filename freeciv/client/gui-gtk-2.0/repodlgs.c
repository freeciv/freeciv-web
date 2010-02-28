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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* common & utility */
#include "fcintl.h"
#include "game.h" /* setting_class_is_changeable() */
#include "government.h"
#include "log.h"
#include "packets.h"
#include "shared.h"
#include "support.h"
#include "unitlist.h"

/* client */
#include "chatline_common.h"
#include "cityrep.h"
#include "client_main.h"
#include "climisc.h"
#include "control.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "mapview_common.h"
#include "options.h"
#include "packhand_gen.h"
#include "control.h"
#include "reqtree.h"
#include "text.h"

#include "canvas.h"
#include "repodlgs_common.h"
#include "repodlgs.h"

/******************************************************************/

static void create_science_dialog(bool make_modal);
static void science_change_callback(GtkWidget * widget, gpointer data);
static void science_goal_callback(GtkWidget * widget, gpointer data);
/******************************************************************/
static struct gui_dialog *science_dialog_shell = NULL;
static GtkWidget *science_label;
static GtkWidget *science_current_label, *science_goal_label;
static GtkWidget *science_change_menu_button, *science_goal_menu_button;
static GtkWidget *science_help_toggle;
static GtkWidget *science_drawing_area;
static int science_dialog_shell_is_modal;
static GtkWidget *popupmenu, *goalmenu;

/******************************************************************/
enum {
  ECONOMY_SELL_OBSOLETE = 1, ECONOMY_SELL_ALL
};

static void create_economy_report_dialog(bool make_modal);
static void economy_command_callback(struct gui_dialog *dlg, int response,
                                     gpointer data);
static void economy_selection_callback(GtkTreeSelection *selection,
				       gpointer data);
static struct universal economy_row_type[U_LAST + B_LAST];

static struct gui_dialog *economy_dialog_shell = NULL;
static GtkWidget *economy_label2;
static GtkListStore *economy_store;
static GtkTreeSelection *economy_selection;
static GtkWidget *sellall_command, *sellobsolete_command;
static int economy_dialog_shell_is_modal;

/******************************************************************/
static void create_activeunits_report_dialog(bool make_modal);
static void activeunits_command_callback(struct gui_dialog *dlg, int response,
                                         gpointer data);
static void activeunits_selection_callback(GtkTreeSelection *selection,
					   gpointer data);
static struct gui_dialog *activeunits_dialog_shell = NULL;
static GtkListStore *activeunits_store;
static GtkTreeSelection *activeunits_selection;

enum {
  ACTIVEUNITS_NEAREST = 1, ACTIVEUNITS_UPGRADE
};

static int activeunits_dialog_shell_is_modal;

/******************************************************************/
static void create_endgame_report(struct packet_endgame_report *packet);

static GtkListStore *scores_store;
static struct gui_dialog *endgame_report_shell;
static GtkWidget *scores_list;
static GtkWidget *sw;

#define NUM_SCORE_COLS 14                

/******************************************************************/
static GtkWidget *settable_options_dialog_shell;

/******************************************************************/

/******************************************************************
...
*******************************************************************/
void update_report_dialogs(void)
{
  if(is_report_dialogs_frozen()) return;
  activeunits_report_dialog_update();
  economy_report_dialog_update();
  city_report_dialog_update(); 
  science_dialog_update();
}


/****************************************************************
...
*****************************************************************/
void popup_science_dialog(bool raise)
{
  if(!science_dialog_shell) {
    science_dialog_shell_is_modal = FALSE;
    
    create_science_dialog(FALSE);
  }

  if (can_client_issue_orders()
      && A_UNSET == get_player_research(client.conn.playing)->tech_goal
      && A_UNSET == get_player_research(client.conn.playing)->researching) {
    gui_dialog_alert(science_dialog_shell);
  } else {
    gui_dialog_present(science_dialog_shell);
  }

  if (raise) {
    gui_dialog_raise(science_dialog_shell);
  }
}


/****************************************************************
 Closes the science dialog.
*****************************************************************/
void popdown_science_dialog(void)
{
  if (science_dialog_shell) {
    gui_dialog_destroy(science_dialog_shell);
  }
}

/****************************************************************************
 Change tech goal, research or open help dialog
****************************************************************************/
static void button_release_event_callback(GtkWidget *widget,
					  GdkEventButton *event,
                                          gpointer *data)
{
  struct reqtree *tree = g_object_get_data(G_OBJECT(widget), "reqtree");
  int x = event->x, y = event->y;
  Tech_type_id tech = get_tech_on_reqtree(tree, x, y);

  if (tech == A_NONE) {
    return;
  }

  if (event->button == 3) {
    /* RMB: get help */
    /* FIXME: this should work for ctrl+LMB or shift+LMB (?) too */
    popup_help_dialog_typed(advance_name_for_player(client.conn.playing, tech), HELP_TECH);
  } else {
    if (event->button == 1 && can_client_issue_orders()) {
      /* LMB: set research or research goal */
      switch (player_invention_state(client.conn.playing, tech)) {
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
}

/****************************************************************************
  Draw the invalidated portion of the reqtree.
****************************************************************************/
static void update_science_drawing_area(GtkWidget *widget, gpointer data)
{
  /* FIXME: this currently redraws everything! */
  struct canvas canvas = {.type = CANVAS_PIXMAP,
			  .v.pixmap = GTK_LAYOUT(widget)->bin_window};
  struct reqtree *reqtree = g_object_get_data(G_OBJECT(widget), "reqtree");
  int width, height;

  get_reqtree_dimensions(reqtree, &width, &height);
  draw_reqtree(reqtree, &canvas, 0, 0, 0, 0, width, height);
}

/****************************************************************************
  Return main widget of new technology diagram.
  This is currently GtkScrolledWindow 
****************************************************************************/
static GtkWidget *create_reqtree_diagram(void)
{
  GtkWidget *sw;
  struct reqtree *reqtree;
  GtkAdjustment* adjustment;
  int width, height;
  int x;
  Tech_type_id researching;

  if (can_conn_edit(&client.conn)) {
    /* Show all techs in editor mode, not only currently reachable ones */
    reqtree = create_reqtree(NULL);
  } else {
    /* Show only at some point reachable techs */
    reqtree = create_reqtree(client.conn.playing);
  }

  get_reqtree_dimensions(reqtree, &width, &height);

  sw = gtk_scrolled_window_new(NULL, NULL);
  science_drawing_area = gtk_layout_new(NULL, NULL);
  g_object_set_data_full(G_OBJECT(science_drawing_area), "reqtree", reqtree,
			 (GDestroyNotify)destroy_reqtree);
  g_signal_connect(G_OBJECT(science_drawing_area), "expose_event",
		   G_CALLBACK(update_science_drawing_area), NULL);
  g_signal_connect(G_OBJECT(science_drawing_area), "button-release-event",
                   G_CALLBACK(button_release_event_callback), NULL);
  gtk_widget_add_events(science_drawing_area,
                        GDK_BUTTON_RELEASE_MASK | GDK_BUTTON2_MOTION_MASK | 
			GDK_BUTTON3_MOTION_MASK);
			 
  gtk_layout_set_size(GTK_LAYOUT(science_drawing_area), width, height);

  gtk_container_add(GTK_CONTAINER(sw), science_drawing_area);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);

  adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(sw));
  
  /* Center on currently researched node */
  if (NULL != client.conn.playing) {
    researching = get_player_research(client.conn.playing)->researching;
  } else {
    researching = A_UNSET;
  }
  if (find_tech_on_reqtree(reqtree, researching,
			   &x, NULL, NULL, NULL)) {
    /* FIXME: this is just an approximation */
    gtk_adjustment_set_value(adjustment, x - 100);
  }

  return sw;
}

/****************************************************************
...
*****************************************************************/
void create_science_dialog(bool make_modal)
{
  GtkWidget *frame, *hbox, *w;
  GtkWidget *science_diagram;

  gui_dialog_new(&science_dialog_shell, GTK_NOTEBOOK(top_notebook), NULL);
  /* TRANS: Research report title */
  gui_dialog_set_title(science_dialog_shell, _("Research"));

  gui_dialog_add_button(science_dialog_shell,
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gui_dialog_set_default_response(science_dialog_shell,
      GTK_RESPONSE_CLOSE);

  science_label = gtk_label_new("no text set yet");

  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox),
        science_label, FALSE, FALSE, 0);

  frame = gtk_frame_new(_("Researching"));
  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox),
        frame, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(TRUE, 4);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  science_change_menu_button = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(hbox), science_change_menu_button,
      TRUE, TRUE, 0);

  popupmenu = gtk_menu_new();
  gtk_widget_show_all(popupmenu);

  science_current_label=gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(hbox), science_current_label, TRUE, TRUE, 0);
  gtk_widget_set_size_request(science_current_label, -1, 25);
  
  science_help_toggle = gtk_check_button_new_with_label (_("Help"));
  gtk_box_pack_start(GTK_BOX(hbox), science_help_toggle, TRUE, FALSE, 0);

  frame = gtk_frame_new( _("Goal"));
  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox),
        frame, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(TRUE, 4);
  gtk_container_add(GTK_CONTAINER(frame),hbox);

  science_goal_menu_button = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(hbox), science_goal_menu_button,
      TRUE, TRUE, 0);

  goalmenu = gtk_menu_new();
  gtk_widget_show_all(goalmenu);

  science_goal_label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), science_goal_label, TRUE, TRUE, 0);
  gtk_widget_set_size_request(science_goal_label, -1, 25);

  w = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, FALSE, 0);
  
  science_diagram = create_reqtree_diagram();
  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox), science_diagram, TRUE, TRUE, 0);

  gui_dialog_show_all(science_dialog_shell);

  science_dialog_update();
  gtk_widget_grab_focus(science_change_menu_button);
}

/****************************************************************************
  Called to set several texts in the science dialog.
****************************************************************************/
static void update_science_text(void)
{
  double pct;
  const char *text = get_science_target_text(&pct);

  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(science_current_label), text);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(science_current_label), pct);

  /* work around GTK+ refresh bug. */
  gtk_widget_queue_resize(science_current_label);
}

/****************************************************************
...
*****************************************************************/
void science_change_callback(GtkWidget *widget, gpointer data)
{
  size_t to = (size_t) data;

  if (GTK_TOGGLE_BUTTON(science_help_toggle)->active) {
    popup_help_dialog_typed(advance_name_for_player(client.conn.playing, to), HELP_TECH);
    /* Following is to make the menu go back to the current research;
     * there may be a better way to do this?  --dwp */
    science_dialog_update();
  } else {

    gtk_widget_set_sensitive(science_change_menu_button,
			     can_client_issue_orders());
    update_science_text();
    
    dsend_packet_player_research(&client.conn, to);
  }
}

/****************************************************************
...
*****************************************************************/
void science_goal_callback(GtkWidget *widget, gpointer data)
{
  size_t to = (size_t) data;

  if (GTK_TOGGLE_BUTTON(science_help_toggle)->active) {
    popup_help_dialog_typed(advance_name_for_player(client.conn.playing, to), HELP_TECH);
    /* Following is to make the menu go back to the current goal;
     * there may be a better way to do this?  --dwp */
    science_dialog_update();
  }
  else {  
    gtk_label_set_text(GTK_LABEL(science_goal_label),
		       get_science_goal_text(to));
    dsend_packet_player_tech_goal(&client.conn, to);
  }
}

/****************************************************************
...
*****************************************************************/
static gint cmp_func(gconstpointer a_p, gconstpointer b_p)
{
  const gchar *a_str, *b_str;
  gint a = GPOINTER_TO_INT(a_p), b = GPOINTER_TO_INT(b_p);

  a_str = advance_name_for_player(client.conn.playing, a);
  b_str = advance_name_for_player(client.conn.playing, b);

  return strcmp(a_str,b_str);
}

/****************************************************************
...
*****************************************************************/
void science_dialog_update(void)
{
  if(science_dialog_shell) {
  int i, hist;
  char text[512];
  GtkWidget *item;
  GList *sorting_list = NULL, *it;
  GtkSizeGroup *group1, *group2;
  struct player_research *research = get_player_research(client.conn.playing);

  if (!research || is_report_dialogs_frozen()) {
    return;
  }

  gtk_widget_queue_draw(science_drawing_area);

  gtk_label_set_text(GTK_LABEL(science_label), science_dialog_text());
  
  /* collect all researched techs in sorting_list */
  advance_index_iterate(A_FIRST, i) {
    if (TECH_KNOWN == player_invention_state(client.conn.playing, i)) {
      sorting_list = g_list_prepend(sorting_list, GINT_TO_POINTER(i));
    }
  } advance_index_iterate_end;

  /* sort them, and install them in the list */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  g_list_free(sorting_list);
  sorting_list = NULL;

  gtk_widget_destroy(popupmenu);
  popupmenu = gtk_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(science_change_menu_button),
	popupmenu);
  gtk_widget_set_sensitive(science_change_menu_button,
			   can_client_issue_orders());

  update_science_text();

  /* work around GTK+ refresh bug. */
  gtk_widget_queue_resize(science_current_label);
 
  if (research->researching == A_UNSET) {
    item = gtk_menu_item_new_with_label(advance_name_for_player(client.conn.playing,
						      A_NONE));
    gtk_menu_shell_append(GTK_MENU_SHELL(popupmenu), item);
  }

  /* collect all techs which are reachable in the next step
   * hist will hold afterwards the techid of the current choice
   */
  hist=0;
  if (!is_future_tech(research->researching)) {
    advance_index_iterate(A_FIRST, i) {
      if (TECH_PREREQS_KNOWN !=
            player_invention_state(client.conn.playing, i)) {
	continue;
      }

      if (i == research->researching)
	hist=i;
      sorting_list = g_list_prepend(sorting_list, GINT_TO_POINTER(i));
    } advance_index_iterate_end;
  } else {
    int value = (advance_count() + research->future_tech + 1);

    sorting_list = g_list_prepend(sorting_list, GINT_TO_POINTER(value));
  }

  /* sort the list and build from it the menu */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  for (i = 0; i < g_list_length(sorting_list); i++) {
    const gchar *data;

    if (GPOINTER_TO_INT(g_list_nth_data(sorting_list, i)) < advance_count()) {
      data = advance_name_for_player(client.conn.playing,
			GPOINTER_TO_INT(g_list_nth_data(sorting_list, i)));
    } else {
      my_snprintf(text, sizeof(text), _("Future Tech. %d"),
		  GPOINTER_TO_INT(g_list_nth_data(sorting_list, i))
		  - advance_count());
      data=text;
    }

    item = gtk_menu_item_new_with_label(data);
    gtk_menu_shell_append(GTK_MENU_SHELL(popupmenu), item);
    if (strlen(data) > 0)
      g_signal_connect(item, "activate",
		       G_CALLBACK(science_change_callback),
		       g_list_nth_data(sorting_list, i));
  }

  gtk_widget_show_all(popupmenu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(science_change_menu_button),
	g_list_index(sorting_list, GINT_TO_POINTER(hist)));
  g_list_free(sorting_list);
  sorting_list = NULL;

  gtk_widget_destroy(goalmenu);
  goalmenu = gtk_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(science_goal_menu_button),
	goalmenu);
  gtk_widget_set_sensitive(science_goal_menu_button,
			   can_client_issue_orders());
  
  gtk_label_set_text(GTK_LABEL(science_goal_label),
		get_science_goal_text(
		  research->tech_goal));

  if (research->tech_goal == A_UNSET) {
    item = gtk_menu_item_new_with_label(advance_name_for_player(client.conn.playing,
						      A_NONE));
    gtk_menu_shell_append(GTK_MENU_SHELL(goalmenu), item);
  }

  /* collect all techs which are reachable in under 11 steps
   * hist will hold afterwards the techid of the current choice
   */
  hist=0;
  advance_index_iterate(A_FIRST, i) {
    if (player_invention_reachable(client.conn.playing, i)
        && TECH_KNOWN != player_invention_state(client.conn.playing, i)
        && (11 > num_unknown_techs_for_goal(client.conn.playing, i)
	    || i == research->tech_goal)) {
      if (i == research->tech_goal) {
	hist = i;
      }
      sorting_list = g_list_prepend(sorting_list, GINT_TO_POINTER(i));
    }
  } advance_index_iterate_end;

  group1 = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  group2 = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  /* sort the list and build from it the menu */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  for (it = g_list_first(sorting_list); it; it = g_list_next(it)) {
    GtkWidget *hbox, *label;
    char text[512];
    gint tech = GPOINTER_TO_INT(g_list_nth_data(it, 0));

    item = gtk_menu_item_new();
    hbox = gtk_hbox_new(FALSE, 18);
    gtk_container_add(GTK_CONTAINER(item), hbox);

    label = gtk_label_new(advance_name_for_player(client.conn.playing, tech));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    gtk_size_group_add_widget(group1, label);

    my_snprintf(text, sizeof(text), "%d",
	num_unknown_techs_for_goal(client.conn.playing, tech));

    label = gtk_label_new(text);
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_size_group_add_widget(group2, label);

    gtk_menu_shell_append(GTK_MENU_SHELL(goalmenu), item);
    g_signal_connect(item, "activate",
		     G_CALLBACK(science_goal_callback),
		     GINT_TO_POINTER(tech));
  }

  gtk_widget_show_all(goalmenu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(science_goal_menu_button),
	g_list_index(sorting_list, GINT_TO_POINTER(hist)));
  g_list_free(sorting_list);
  sorting_list = NULL;
 
  }
}


/****************************************************************

                      ECONOMY REPORT DIALOG
 
****************************************************************/

/****************************************************************
...
****************************************************************/
void popup_economy_report_dialog(bool raise)
{
  if(!economy_dialog_shell) {
    economy_dialog_shell_is_modal = FALSE;

    create_economy_report_dialog(FALSE);
  }

  gui_dialog_present(economy_dialog_shell);
  if (raise) {
    gui_dialog_raise(economy_dialog_shell);
  }
}

/****************************************************************
 Close the economy report dialog.
****************************************************************/
void popdown_economy_report_dialog(void)
{
  if (economy_dialog_shell) {
    gui_dialog_destroy(economy_dialog_shell);
  }
}
 
/****************************************************************
...
*****************************************************************/
void create_economy_report_dialog(bool make_modal)
{
  const char *titles[5] = {
    /* TRANS: Image header */
    _("Type"),
    Q_("?Building:Name"),
    _("Count"),
    _("Cost"),
    /* TRANS: Upkeep total, count*cost */
    _("U Total")
  };
  int i;

  static GType model_types[5] = {
    G_TYPE_NONE,
    G_TYPE_STRING,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT
  };
  GtkWidget *view, *sw, *align;

  model_types[0] = GDK_TYPE_PIXBUF;

  gui_dialog_new(&economy_dialog_shell, GTK_NOTEBOOK(top_notebook), NULL);
  gui_dialog_set_title(economy_dialog_shell, _("Economy"));

  align = gtk_alignment_new(0.5, 0.0, 0.0, 1.0);
  gtk_box_pack_start(GTK_BOX(economy_dialog_shell->vbox), align,
      TRUE, TRUE, 0);

  economy_store = gtk_list_store_newv(ARRAY_SIZE(model_types), model_types);

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(align), sw);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(economy_store));
  g_object_unref(economy_store);
  gtk_widget_set_name(view, "small_font");
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  economy_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_signal_connect(economy_selection, "changed",
		   G_CALLBACK(economy_selection_callback), NULL);

  for (i=0; i<ARRAY_SIZE(model_types); i++) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    if (model_types[i] == GDK_TYPE_PIXBUF) {
      renderer = gtk_cell_renderer_pixbuf_new();

      col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
	  "pixbuf", i, NULL);
    } else {
      renderer = gtk_cell_renderer_text_new();

      col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
	  "text", i, NULL);

      if (i > 1) {
	GValue value = { 0, };

	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 1.0);
	g_object_set_property(G_OBJECT(renderer), "xalign", &value);
	g_value_unset(&value);

	gtk_tree_view_column_set_alignment(col, 1.0);
      }
    }

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  }
  gtk_container_add(GTK_CONTAINER(sw), view);

  economy_label2 = gtk_label_new(_("Total Cost:"));
  gtk_box_pack_start(GTK_BOX(economy_dialog_shell->vbox), economy_label2,
      FALSE, FALSE, 0);
  gtk_misc_set_padding(GTK_MISC(economy_label2), 5, 5);

  sellobsolete_command =
    gui_dialog_add_button(economy_dialog_shell, _("Sell _Obsolete"),
	ECONOMY_SELL_OBSOLETE);
  gtk_widget_set_sensitive(sellobsolete_command, FALSE);

  sellall_command =
    gui_dialog_add_button(economy_dialog_shell, _("Sell _All"),
	ECONOMY_SELL_ALL);
  gtk_widget_set_sensitive(sellall_command, FALSE);

  gui_dialog_add_button(economy_dialog_shell,
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  gui_dialog_response_set_callback(economy_dialog_shell,
      economy_command_callback);

  economy_report_dialog_update();
  gui_dialog_set_default_size(economy_dialog_shell, -1, 350);

  gui_dialog_set_default_response(economy_dialog_shell, GTK_RESPONSE_CLOSE);

  gui_dialog_show_all(economy_dialog_shell);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}


/****************************************************************
  Called when a building type is selected in the economy list.
*****************************************************************/
static void economy_selection_callback(GtkTreeSelection *selection,
				       gpointer data)
{
  gint row = gtk_tree_selection_get_row(selection);

  if (row >= 0) {
    switch (economy_row_type[row].kind) {
    case VUT_IMPROVEMENT:
    {
      /* The user has selected an improvement type. */
      struct impr_type *pimprove = economy_row_type[row].value.building;
      bool is_sellable = can_sell_building(pimprove);

      gtk_widget_set_sensitive(sellobsolete_command, is_sellable
			       && can_client_issue_orders()
			       && improvement_obsolete(client.conn.playing, pimprove));
      gtk_widget_set_sensitive(sellall_command, is_sellable
			       && can_client_issue_orders());
      break;
    }
    case VUT_UTYPE:
      /* An unit has been selected */
      gtk_widget_set_sensitive(sellall_command, can_client_issue_orders());
      break;
    default:
      assert(0);
      break;
    };
  } else {
    /* No selection has been made. */
    gtk_widget_set_sensitive(sellobsolete_command, FALSE);
    gtk_widget_set_sensitive(sellall_command, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void economy_command_callback(struct gui_dialog *dlg, int response,
                                     gpointer callback)
{
  gint row;
  GtkWidget *shell;
  char buf[1024];

  if (response != ECONOMY_SELL_OBSOLETE && response != ECONOMY_SELL_ALL) {
    gui_dialog_destroy(dlg);
    return;
  }

  /* sell obsolete and sell all. */
  row = gtk_tree_selection_get_row(economy_selection);

  switch (economy_row_type[row].kind) {
  case VUT_IMPROVEMENT:
  {
    struct impr_type *pimprove = economy_row_type[row].value.building;
    if (response == ECONOMY_SELL_ALL) {
      shell = gtk_message_dialog_new(
	  NULL,
	  GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
	  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
	  _("Do you really wish to sell your %s?\n"),
	  improvement_name_translation(pimprove));
      setup_dialog(shell, gui_dialog_get_toplevel(dlg));
      gtk_window_set_title(GTK_WINDOW(shell), _("Sell Improvements"));

      if (gtk_dialog_run(GTK_DIALOG(shell)) == GTK_RESPONSE_YES) {
	gtk_widget_destroy(shell);
      } else {
	gtk_widget_destroy(shell);
	return;
      }
    }

    sell_all_improvements(pimprove, response!= ECONOMY_SELL_ALL, buf, sizeof(buf));
    break;
  }
  case VUT_UTYPE:
    if (response== ECONOMY_SELL_OBSOLETE) {
      return;
    }
    disband_all_units(economy_row_type[row].value.utype, FALSE, buf, sizeof(buf));
    break;
  default:
    assert(0);
    break;
  };

  shell = gtk_message_dialog_new(
      NULL,
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
      "%s", buf);
  setup_dialog(shell, gui_dialog_get_toplevel(dlg));

  g_signal_connect(shell, "response", G_CALLBACK(gtk_widget_destroy), NULL);
  gtk_window_set_title(GTK_WINDOW(shell), _("Sell-Off: Results"));
  gtk_window_present(GTK_WINDOW(shell));
}

/****************************************************************
...
*****************************************************************/
void economy_report_dialog_update(void)
{
  if(!is_report_dialogs_frozen() && economy_dialog_shell) {
    int tax, total_impr, total_unit, i, entries_used, nbr_impr;
    char economy_total[48];
    struct improvement_entry entries[B_LAST];
    struct unit_entry entries_units[U_LAST];
    GtkTreeIter it;
    GValue value = { 0, };

    gtk_list_store_clear(economy_store);

    get_economy_report_data(entries, &entries_used, &total_impr, &tax);

    for (i = 0; i < entries_used; i++) {
      struct improvement_entry *p = &entries[i];
      struct sprite *sprite = get_building_sprite(tileset, p->type);

      gtk_list_store_append(economy_store, &it);
      gtk_list_store_set(economy_store, &it,
			 0, sprite_get_pixbuf(sprite),
			 2, p->count,
			 3, p->cost,
			 4, p->total_cost, -1);
      g_value_init(&value, G_TYPE_STRING);
      g_value_set_static_string(&value, improvement_name_translation(p->type));
      gtk_list_store_set_value(economy_store, &it, 1, &value);
      g_value_unset(&value);

      economy_row_type[i].kind = VUT_IMPROVEMENT;
      economy_row_type[i].value.building = p->type;
    }

    nbr_impr = entries_used;
    entries_used = 0;
    get_economy_report_units_data(entries_units, &entries_used, &total_unit);

    for (i = 0; i < entries_used; i++) {
      gtk_list_store_append(economy_store, &it);
      gtk_list_store_set(economy_store, &it,
			 2, entries_units[i].count,
			 3, entries_units[i].cost,
			 4, entries_units[i].total_cost, -1);
      g_value_init(&value, G_TYPE_STRING);
      g_value_set_static_string(&value, utype_name_translation(entries_units[i].type));
      gtk_list_store_set_value(economy_store, &it, 1, &value);
      g_value_unset(&value);
    
      economy_row_type[i + nbr_impr].kind = VUT_UTYPE;
      economy_row_type[i + nbr_impr].value.utype = entries_units[i].type;
    }

    my_snprintf(economy_total, sizeof(economy_total),
		_("Income: %d    Total Costs: %d"), tax, total_impr + total_unit); 
    gtk_label_set_text(GTK_LABEL(economy_label2), economy_total);
  }  
}

/****************************************************************

                      ACTIVE UNITS REPORT DIALOG
 
****************************************************************/

#define AU_COL 7

/****************************************************************
...
****************************************************************/
void popup_activeunits_report_dialog(bool raise)
{
  if(!activeunits_dialog_shell) {
    activeunits_dialog_shell_is_modal = FALSE;
    
    create_activeunits_report_dialog(FALSE);
  }

  gui_dialog_present(activeunits_dialog_shell);
  if (raise) {
    gui_dialog_raise(activeunits_dialog_shell);
  }
}

/****************************************************************
 Closes the units report dialog.
****************************************************************/
void popdown_activeunits_report_dialog(void)
{
  if (activeunits_dialog_shell) {
    gui_dialog_destroy(activeunits_dialog_shell);
  }
}

 
/****************************************************************
...
*****************************************************************/
void create_activeunits_report_dialog(bool make_modal)
{
  static const char *titles[AU_COL] = {
    N_("Unit Type"),
    N_("?Upgradable unit [short]:U"),
    N_("In-Prog"),
    N_("Active"),
    N_("Shield"),
    N_("Food"),
    N_("Gold")
  };
  static bool titles_done;
  int i;

  static GType model_types[AU_COL+2] = {
    G_TYPE_STRING,
    G_TYPE_BOOLEAN,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_BOOLEAN,
    G_TYPE_INT
  };
  GtkWidget *view, *sw, *align;

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

  gui_dialog_new(&activeunits_dialog_shell, GTK_NOTEBOOK(top_notebook), NULL);
  gui_dialog_set_title(activeunits_dialog_shell, _("Units"));

  align = gtk_alignment_new(0.5, 0.0, 0.0, 1.0);
  gtk_box_pack_start(GTK_BOX(activeunits_dialog_shell->vbox), align,
      TRUE, TRUE, 0);

  activeunits_store = gtk_list_store_newv(ARRAY_SIZE(model_types), model_types);

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(align), sw);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(activeunits_store));
  g_object_unref(activeunits_store);
  gtk_widget_set_name(view, "small_font");
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  activeunits_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_signal_connect(activeunits_selection, "changed",
	G_CALLBACK(activeunits_selection_callback), NULL);

  for (i=0; i<AU_COL; i++) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    if (model_types[i] == G_TYPE_BOOLEAN) {
      renderer = gtk_cell_renderer_toggle_new();
      
      col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
	"active", i, "visible", AU_COL, NULL);
    } else {
      renderer = gtk_cell_renderer_text_new();
      
      col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
	"text", i, NULL);
    }

    if (i > 0) {
      g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_alignment(col, 1.0);
    }

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  }
  gtk_container_add(GTK_CONTAINER(sw), view);

  gui_dialog_add_stockbutton(activeunits_dialog_shell, GTK_STOCK_FIND,
      _("Find _Nearest"), ACTIVEUNITS_NEAREST);
  gui_dialog_set_response_sensitive(activeunits_dialog_shell,
      				    ACTIVEUNITS_NEAREST, FALSE);	
  
  gui_dialog_add_button(activeunits_dialog_shell,
      _("_Upgrade"), ACTIVEUNITS_UPGRADE);
  gui_dialog_set_response_sensitive(activeunits_dialog_shell,
      				    ACTIVEUNITS_UPGRADE, FALSE);	

  gui_dialog_add_button(activeunits_dialog_shell, GTK_STOCK_CLOSE,
      GTK_RESPONSE_CLOSE);

  gui_dialog_set_default_response(activeunits_dialog_shell,
      GTK_RESPONSE_CLOSE);

  gui_dialog_response_set_callback(activeunits_dialog_shell,
      activeunits_command_callback);

  activeunits_report_dialog_update();
  gui_dialog_set_default_size(activeunits_dialog_shell, -1, 350);

  gui_dialog_show_all(activeunits_dialog_shell);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/****************************************************************
...
*****************************************************************/
static void activeunits_selection_callback(GtkTreeSelection *selection,
					   gpointer data)
{
  struct unit_type *utype;
  GtkTreeModel *model;
  GtkTreeIter it;

  utype = NULL;
  if (gtk_tree_selection_get_selected(activeunits_selection, &model, &it)) {
    int ut;

    gtk_tree_model_get(model, &it, AU_COL + 1, &ut, -1);
    utype = utype_by_number(ut);
  }


  if (utype == NULL) {
    gui_dialog_set_response_sensitive(activeunits_dialog_shell,
				      ACTIVEUNITS_NEAREST, FALSE);

    gui_dialog_set_response_sensitive(activeunits_dialog_shell,
				      ACTIVEUNITS_UPGRADE, FALSE);
  } else {
    gui_dialog_set_response_sensitive(activeunits_dialog_shell,
				      ACTIVEUNITS_NEAREST,
				      can_client_issue_orders());	
    
    gui_dialog_set_response_sensitive(activeunits_dialog_shell,
		ACTIVEUNITS_UPGRADE,
		(can_client_issue_orders()
		 && NULL != can_upgrade_unittype(client.conn.playing, utype)));
  }
}

/****************************************************************
...
*****************************************************************/
static struct unit *find_nearest_unit(const struct unit_type *type,
				      struct tile *ptile)
{
  struct unit *best_candidate;
  int best_dist = FC_INFINITY;

  best_candidate = NULL;
  if (NULL == client.conn.playing) {
    return NULL;
  }

  unit_list_iterate(client.conn.playing->units, punit) {
    if (unit_type(punit) == type) {
      if (punit->focus_status==FOCUS_AVAIL
	  && punit->moves_left > 0
	  && !punit->done_moving
	  && !punit->ai.control) {
	int d;
	d=sq_map_distance(punit->tile, ptile);
	if(d<best_dist) {
	  best_candidate = punit;
	  best_dist = d;
	}
      }
    }
  }
  unit_list_iterate_end;
  return best_candidate;
}

/****************************************************************
...
*****************************************************************/
static void activeunits_command_callback(struct gui_dialog *dlg, int response,
                                         gpointer data)
{
  struct unit_type *utype1 = NULL;
  GtkTreeModel *model;
  GtkTreeIter   it;

  switch (response) {
    case ACTIVEUNITS_NEAREST:
    case ACTIVEUNITS_UPGRADE:
      break;
    default:
      gui_dialog_destroy(dlg);
      return;
  }

  /* nearest & upgrade commands. */
  if (gtk_tree_selection_get_selected(activeunits_selection, &model, &it)) {
    int ut;

    gtk_tree_model_get(model, &it, AU_COL + 1, &ut, -1);
    utype1 = utype_by_number(ut);
  }

  CHECK_UNIT_TYPE(utype1);

  if (response == ACTIVEUNITS_NEAREST) {
    struct tile *ptile;
    struct unit *punit;

    ptile = get_center_tile_mapcanvas();
    if ((punit = find_nearest_unit(utype1, ptile))) {
      center_tile_mapcanvas(punit->tile);

      if (punit->activity == ACTIVITY_IDLE
	  || punit->activity == ACTIVITY_SENTRY) {
	if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
	  set_unit_focus_and_select(punit);
	}
      }
    }
  } else if (can_client_issue_orders()) {
    GtkWidget *shell;
    struct unit_type *ut2 = can_upgrade_unittype(client.conn.playing, utype1);

    shell = gtk_message_dialog_new(
	  NULL,
	  GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
	  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
	  _("Upgrade as many %s to %s as possible for %d gold each?\n"
	    "Treasury contains %d gold."),
	  utype_name_translation(utype1),
	  utype_name_translation(ut2),
	  unit_upgrade_price(client.conn.playing, utype1, ut2),
	  client.conn.playing->economic.gold);
    setup_dialog(shell, gui_dialog_get_toplevel(dlg));

    gtk_window_set_title(GTK_WINDOW(shell), _("Upgrade Obsolete Units"));

    if (gtk_dialog_run(GTK_DIALOG(shell)) == GTK_RESPONSE_YES) {
      dsend_packet_unit_type_upgrade(&client.conn, utype_number(utype1));
    }

    gtk_widget_destroy(shell);
  }
}

/****************************************************************
...
*****************************************************************/
void activeunits_report_dialog_update(void)
{
  struct repoinfo {
    int active_count;
    int upkeep[O_LAST];
    int building_count;
  };

  if (is_report_dialogs_frozen() || !activeunits_dialog_shell) {
    return;
  }

  gtk_list_store_clear(activeunits_store);
  if (NULL != client.conn.playing) {
    int    k, can;
    struct repoinfo unitarray[U_LAST];
    struct repoinfo unittotals;
    GtkTreeIter it;
    GValue value = { 0, };

    gtk_list_store_clear(activeunits_store);

    memset(unitarray, '\0', sizeof(unitarray));
    city_list_iterate(client.conn.playing->cities, pcity) {
      unit_list_iterate(pcity->units_supported, punit) {
        Unit_type_id uti = utype_index(unit_type(punit));

        (unitarray[uti].active_count)++;
        if (punit->homecity) {
          output_type_iterate(o) {
            unitarray[uti].upkeep[o] += punit->upkeep[o];
          } output_type_iterate_end;
        }
      } unit_list_iterate_end;
    } city_list_iterate_end;
    city_list_iterate(client.conn.playing->cities,pcity) {
      if (VUT_UTYPE == pcity->production.kind) {
        struct unit_type *punittype = pcity->production.value.utype;
	(unitarray[utype_index(punittype)].building_count)++;
      }
    }
    city_list_iterate_end;

    k = 0;
    memset(&unittotals, '\0', sizeof(unittotals));
    unit_type_iterate(punittype) {
      Unit_type_id uti = utype_index(punittype);
      if (unitarray[uti].active_count > 0
	  || unitarray[uti].building_count > 0) {
	can = (NULL != can_upgrade_unittype(client.conn.playing, punittype));
	
        gtk_list_store_append(activeunits_store, &it);
	gtk_list_store_set(activeunits_store, &it,
		1, can,
		2, unitarray[uti].building_count,
		3, unitarray[uti].active_count,
		4, unitarray[uti].upkeep[O_SHIELD],
		5, unitarray[uti].upkeep[O_FOOD],
		6, unitarray[uti].upkeep[O_GOLD],
		7, TRUE,
		8, ((unitarray[uti].active_count > 0)
		    ? uti : U_LAST),
		-1);
	g_value_init(&value, G_TYPE_STRING);
	g_value_set_static_string(&value, utype_name_translation(punittype));
	gtk_list_store_set_value(activeunits_store, &it, 0, &value);
	g_value_unset(&value);

	k++;
	unittotals.active_count += unitarray[uti].active_count;
	output_type_iterate(o) {
	  unittotals.upkeep[o] += unitarray[uti].upkeep[o];
	} output_type_iterate_end;
	unittotals.building_count += unitarray[uti].building_count;
      }
    } unit_type_iterate_end;

    gtk_list_store_append(activeunits_store, &it);
    gtk_list_store_set(activeunits_store, &it,
	    1, FALSE,
    	    2, unittotals.building_count,
    	    3, unittotals.active_count,
    	    4, unittotals.upkeep[O_SHIELD],
    	    5, unittotals.upkeep[O_FOOD],
	    6, unittotals.upkeep[O_GOLD],
	    7, FALSE,
	    8, U_LAST, -1);
    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, _("Totals:"));
    gtk_list_store_set_value(activeunits_store, &it, 0, &value);
    g_value_unset(&value);
  }
}

/****************************************************************

                      FINAL REPORT DIALOG
 
****************************************************************/

/****************************************************************
  Prepare the Final Report dialog, and fill it with 
  statistics for each player.
*****************************************************************/
static void create_endgame_report(struct packet_endgame_report *packet)
{
  int i;
  static bool titles_done;
  GtkTreeIter it;
      
  static const char *titles[NUM_SCORE_COLS] = {
    N_("Player\n"),
    N_("Score\n"),
    N_("Population\n"),
    N_("Trade\n(M goods)"), 
    N_("Production\n(M tons)"), 
    N_("Cities\n"),
    N_("Technologies\n"),
    N_("Military Service\n(months)"), 
    N_("Wonders\n"),
    N_("Research Speed\n(%)"), 
    N_("Land Area\n(sq. mi.)"), 
    N_("Settled Area\n(sq. mi.)"), 
    N_("Literacy\n(%)"), 
    N_("Spaceship\n")
  };

  static GType model_types[NUM_SCORE_COLS] = {
    G_TYPE_STRING,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT
  };

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

  gui_dialog_new(&endgame_report_shell, GTK_NOTEBOOK(top_notebook), NULL);
  gui_dialog_set_title(endgame_report_shell, _("Score"));
  gui_dialog_add_button(endgame_report_shell, GTK_STOCK_CLOSE,
      GTK_RESPONSE_CLOSE);

  gui_dialog_set_default_size(endgame_report_shell, 700, 420);

  scores_store = gtk_list_store_newv(ARRAY_SIZE(model_types), model_types);
  scores_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(scores_store));
  g_object_unref(scores_store);
  gtk_widget_set_name(scores_list, "small_font");
    
  for (i = 0; i < NUM_SCORE_COLS; i++) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;
      
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
                                                   "text", i, NULL);
    gtk_tree_view_column_set_sort_column_id(col, i);
    gtk_tree_view_append_column(GTK_TREE_VIEW(scores_list), col);
  }  

  /* Setup the layout. */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(sw), scores_list);
  gtk_box_pack_start(GTK_BOX(endgame_report_shell->vbox), sw, TRUE, TRUE, 0);
  gui_dialog_set_default_response(endgame_report_shell, GTK_RESPONSE_CLOSE);
  gui_dialog_show_all(endgame_report_shell);
  
  /* Insert score statistics into table.  */
  gtk_list_store_clear(scores_store);
  for (i = 0; i < packet->nscores; i++) {
    gtk_list_store_append(scores_store, &it);
    gtk_list_store_set(scores_store, &it,
                       0, (gchar *)player_name(player_by_number(packet->id[i])),
                       1, packet->score[i],
                       2, packet->pop[i],
                       3, packet->bnp[i],
                       4, packet->mfg[i],
                       5, packet->cities[i],
                       6, packet->techs[i],
                       7, packet->mil_service[i],
                       8, packet->wonders[i],
                       9, packet->research[i],
                       10, packet->landarea[i],
                       11, packet->settledarea[i],
                       12, packet->literacy[i],
                       13, packet->spaceship[i],
                       -1);
  }
}

/**************************************************************************
  Show a dialog with player statistics at endgame.
**************************************************************************/
void popup_endgame_report_dialog(struct packet_endgame_report *packet)
{
  if (!endgame_report_shell){
    create_endgame_report(packet);
  }
  gui_dialog_present(endgame_report_shell);
}

/*************************************************************************
  helper function for server options dialog
*************************************************************************/
static void option_changed_callback(GtkWidget *widget, gpointer data) 
{
  /* pass along the pointer to the changed option */
  g_object_set_data(G_OBJECT(widget), "changed", data); 
}

/*************************************************************************
  helper function for server options dialog
*************************************************************************/
static void settable_options_processing(GtkWidget *final)
{
  char buffer[MAX_LEN_MSG];
  const char *desired_string;
  GtkWidget *w = final;

  while (NULL != w) {
    struct options_settable *o =
      (struct options_settable *)g_object_get_data(G_OBJECT(w), "changed");

    /* if the entry has been changed, then send the changes to the server */
    if (NULL != o) {
      /* append the name of the option */
      my_snprintf(buffer, sizeof(buffer), "/set %s ", gtk_widget_get_name(w));

      /* append the setting */
      switch (o->stype) {
      case SSET_BOOL:
        o->desired_val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
        sz_strlcat(buffer, o->desired_val ? "1" : "0");
        break;
      case SSET_INT:
        o->desired_val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
        sz_strlcat(buffer, gtk_entry_get_text(GTK_ENTRY(w)));
        break;
      case SSET_STRING:
        desired_string = gtk_entry_get_text(GTK_ENTRY(w));
        if (NULL != desired_string) {
          if (NULL != o->desired_strval) {
            free(o->desired_strval);
          }
          o->desired_strval = mystrdup(desired_string);
          sz_strlcat(buffer, desired_string);
        }
        break;
      default:
        freelog(LOG_ERROR,
                "settable_options_processing() bad type %d.",
                o->stype);
        break;
      };
      send_chat(buffer);
    }

    /* using the linked list, work backwards and check the previous widget */
    w = (GtkWidget *)g_object_get_data(G_OBJECT(w), "prev");
  }
}

/****************************************************************
...
*****************************************************************/
static void settable_options_callback(GtkWidget *win, gint rid, GtkWidget *w)
{
  switch (rid) {
  case GTK_RESPONSE_ACCEPT:
    settable_options_processing(w);
    save_options();
    break;
  case GTK_RESPONSE_APPLY:
    settable_options_processing(w);
    break;
  default:
    break;
  };
  gtk_widget_destroy(win);
}

/*************************************************************************
  Server options dialog.
*************************************************************************/
static void create_settable_options_dialog(void)
{
  int i;
  GtkWidget *win, *book, **vbox, *label;
  GtkWidget *prev_widget = NULL;
  GtkTooltips *tips = gtk_tooltips_new();
  bool *used = fc_calloc(num_options_categories, sizeof(*used));

  settable_options_dialog_shell =
    gtk_dialog_new_with_buttons(_("Game Settings"),
      NULL, 0,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL);
  win = settable_options_dialog_shell;

  gtk_dialog_set_has_separator(GTK_DIALOG(win), FALSE);
  g_signal_connect(settable_options_dialog_shell, "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &settable_options_dialog_shell);
  setup_dialog(win, toplevel);

  /* create a notebook for the options */
  book = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(win)->vbox), book, FALSE, FALSE, 2);

  /* create a number of notebook pages for each category */
  vbox = fc_calloc(num_options_categories, sizeof(*vbox));

  for (i = 0; i < num_options_categories; i++) {
    vbox[i] = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox[i]), 6);
    label = gtk_label_new(_(options_categories[i]));
    gtk_notebook_append_page(GTK_NOTEBOOK(book), vbox[i], label);
  }

  /* fill each category */
  for (i = 0; i < num_settable_options; i++) {
    GtkWidget *ebox, *hbox, *ent = NULL;
    struct options_settable *o = &settable_options[i];

    /* create a box for the new option and insert it in the correct page */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox[o->scategory]), hbox, FALSE, FALSE, 0);
    used[o->scategory] = TRUE;

    /* create an event box for the option label */
    ebox = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX(hbox), ebox, FALSE, FALSE, 5);

    /* insert the option short help as the label into the event box */
    label = gtk_label_new(_(o->short_help));
    gtk_container_add(GTK_CONTAINER(ebox), label);

    /* if we have extra help, use that as a tooltip */
    if ('\0' != o->extra_help[0]) {
      char buf[4096];

      my_snprintf(buf, sizeof(buf), "%s\n\n%s",
		  o->name,
		  _(o->extra_help));
      gtk_tooltips_set_tip(tips, ebox, buf, NULL);
    }

    if (setting_class_is_changeable(o->sclass)
	&& o->is_visible) {
      double step, max, min;

      /* create the proper entry method depending on the type */
      switch (o->stype) {
      case SSET_BOOL:
	/* boolean */
	ent = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ent), o->val);

	g_signal_connect(ent, "toggled", 
			 G_CALLBACK(option_changed_callback), o);
	break;

      case SSET_INT:
	/* integer */

	min = o->min;
	max = o->max;
 
	/* pick a reasonable step size */
	step = ceil((max - min) / 100.0);
	if (step > 100.0) {
	  /* this is ridiculous, the bounds must be meaningless */
	  step = 5.0;
	}

	ent = gtk_spin_button_new_with_range(min, max, step);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent), o->val);

	g_signal_connect(ent, "changed", 
			 G_CALLBACK(option_changed_callback), o);
	break;
      case SSET_STRING:
	/* string */
	ent = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(ent), o->strval);

	g_signal_connect(ent, "changed", 
			 G_CALLBACK(option_changed_callback), o);
	break;
      }
    } else {
      char buf[1024];

      if (o->is_visible) {
	switch (o->stype) {
	case SSET_BOOL:
	  my_snprintf(buf, sizeof(buf), "%s",
		      o->val != 0 ? _("true") : _("false"));
	  break;
	case SSET_INT:
	  my_snprintf(buf, sizeof(buf), "%d", o->val);
	  break;
	case SSET_STRING:
	  my_snprintf(buf, sizeof(buf), "%s", o->strval);
	  break;
	}
      } else {
	my_snprintf(buf, sizeof(buf), "%s", _("(hidden)"));
      }
      ent = gtk_label_new(buf);
    }
    gtk_box_pack_end(GTK_BOX(hbox), ent, FALSE, FALSE, 0);

    /* set up a linked list so we can work our way through the widgets */
    gtk_widget_set_name(ent, o->name);
    g_object_set_data(G_OBJECT(ent), "prev", prev_widget);
    g_object_set_data(G_OBJECT(ent), "changed", NULL);
    prev_widget = ent;
  }

  /* remove any unused categories pages */
  for (i = num_options_categories - 1; i >= 0; i--) {
    if (!used[i]) {
      gtk_notebook_remove_page(GTK_NOTEBOOK(book), i);
    }
  }
  free(used);

  g_signal_connect(win, "response",
		   G_CALLBACK(settable_options_callback), prev_widget);

  gtk_widget_show_all(GTK_DIALOG(win)->vbox);
}

/**************************************************************************
  Show a dialog with the server options.
**************************************************************************/
void popup_settable_options_dialog(void)
{
  if (!settable_options_dialog_shell) {
    create_settable_options_dialog();
    gtk_window_set_position(GTK_WINDOW(settable_options_dialog_shell), GTK_WIN_POS_MOUSE);
  }
  gtk_window_present(GTK_WINDOW(settable_options_dialog_shell));
}

