/**********************************************************************
 Freeciv - Copyright (C) 2001 - R. Falke, M. Kaufman
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

#include <gdk/gdkkeysyms.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "events.h"
#include "game.h"

/* client */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "client_main.h"
#include "cma_fec.h"
#include "messagewin_g.h"

#include "cityrep.h"
#include "dialogs.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"

#include "cma_fe.h"

#define BUFFER_SIZE             64

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct cma_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct cma_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;

static int allow_refreshes = 1;

static struct cma_dialog *get_cma_dialog(struct city *pcity);

static void update_cma_preset_list(struct cma_dialog *pdialog);

static gboolean cma_preset_key_pressed_callback(GtkWidget *w, GdkEventKey *ev,
						gpointer data);
static void cma_del_preset_callback(GtkWidget *w, gpointer data);
static void cma_preset_remove(struct cma_dialog *pdialog, int preset_index);
static void cma_preset_remove_response(GtkWidget *w, gint response,
				       gpointer data);

static void cma_add_preset_callback(GtkWidget *w, gpointer data);
static void cma_preset_add_callback_yes(GtkWidget *w, gpointer data);
static void cma_preset_add_callback_no(GtkWidget *w, gpointer data);
static void cma_preset_add_callback_destroy(GtkWidget *w, gpointer data);

static void cma_active_callback(GtkWidget *w, gpointer data);
static void cma_activate_preset_callback(GtkTreeView *view, GtkTreePath *path,
				         GtkTreeViewColumn *col, gpointer data);

static void hscale_changed(GtkAdjustment *get, gpointer data);
static void set_hscales(const struct cm_parameter *const parameter,
			struct cma_dialog *pdialog);

/**************************************************************************
...
**************************************************************************/
void cma_fe_init()
{
  dialog_list = dialog_list_new();
}

/**************************************************************************
...
**************************************************************************/
void cma_fe_done()
{
  dialog_list_free(dialog_list);
}

/**************************************************************************
 only called when the city dialog is closed.
**************************************************************************/
void close_cma_dialog(struct city *pcity)
{
  struct cma_dialog *pdialog = get_cma_dialog(pcity);

  if (pdialog == NULL) {
    /* A city which is being investigated doesn't contain cma dialog */
    return;
  }
  gtk_widget_destroy(pdialog->shell);
}

/**************************************************************************
...
**************************************************************************/
static void cma_dialog_destroy_callback(GtkWidget *w, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;

  g_object_unref(pdialog->tips);

  dialog_list_unlink(dialog_list, pdialog);
  free(pdialog);
}

/****************************************************************
 return the cma_dialog for a given city.
*****************************************************************/
struct cma_dialog *get_cma_dialog(struct city *pcity)
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
static gboolean button_press_callback(GtkTreeView *view, GdkEventButton *ev,
				      gpointer data)
{
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(view),
	ev->x, ev->y, &path, &column, NULL, NULL)) {
    if (ev->type == GDK_BUTTON_PRESS) {
      cma_activate_preset_callback(view, path, column, data);
    } else if (ev->type == GDK_2BUTTON_PRESS) {
      struct cma_dialog *pdialog = (struct cma_dialog *) data;
      struct cm_parameter param;

      cmafec_get_fe_parameter(pdialog->pcity, &param);
      cma_put_city_under_agent(pdialog->pcity, &param);
      refresh_city_dialog(pdialog->pcity);
    }
  }
  gtk_tree_path_free(path);

  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static void help_callback(GtkWidget *w, gpointer data)
{
  popup_help_dialog_string(HELP_CMA_ITEM);
}

/**************************************************************************
...
**************************************************************************/
static void cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *cell,
			   GtkTreeModel *model, GtkTreeIter *it, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;
  const char *s1, *s2;
  int i1, i2;
  struct cm_parameter param;
  GtkTreePath *path;

  gtk_tree_model_get(model, it, 0, &s1, -1); 
  path = gtk_tree_model_get_path(model, it);
  i1 = gtk_tree_path_get_indices(path) [0];
  gtk_tree_path_free(path);

  cmafec_get_fe_parameter(pdialog->pcity, &param);
  s2 = cmafec_get_short_descr(&param);
  i2 = cmafec_preset_get_index_of_parameter(&param);

  if (!strcmp(s1, s2) && i1 == i2) {
    g_object_set(G_OBJECT(cell), "style", PANGO_STYLE_ITALIC,
		 "weight", PANGO_WEIGHT_BOLD, NULL);
  } else {
    g_object_set(G_OBJECT(cell), "style", PANGO_STYLE_NORMAL,
		 "weight", PANGO_WEIGHT_NORMAL, NULL);
  }
}

/**************************************************************************
 instantiates a new struct for each city_dialog window that is open.
**************************************************************************/
struct cma_dialog *create_cma_dialog(struct city *pcity)
{
  struct cma_dialog *pdialog;
  struct cm_parameter param;
  GtkWidget *frame, *page, *hbox, *label, *table;
  GtkWidget *vbox, *sw, *hscale, *button, *align, *image;
  GtkListStore *store;
  GtkCellRenderer *rend;
  GtkWidget *view;
  GtkTreeViewColumn *column;

  cmafec_get_fe_parameter(pcity, &param);
  pdialog = fc_malloc(sizeof(struct cma_dialog));
  pdialog->pcity = pcity;
  pdialog->shell = gtk_vbox_new(FALSE, 8);
  gtk_container_set_border_width(GTK_CONTAINER(pdialog->shell), 8);
  g_signal_connect(pdialog->shell, "destroy",
		   G_CALLBACK(cma_dialog_destroy_callback), pdialog);

  pdialog->tips = gtk_tooltips_new();
  g_object_ref(pdialog->tips);
  gtk_object_sink(GTK_OBJECT(pdialog->tips));

  page = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(pdialog->shell), page, TRUE, TRUE, 0);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(page), vbox, TRUE, TRUE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  store = gtk_list_store_new(1, G_TYPE_STRING);
  pdialog->store = store;

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
  pdialog->preset_list = view;
  pdialog->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

  g_signal_connect(pdialog->preset_list, "button_press_event",
      		   G_CALLBACK(button_press_callback), pdialog);

  gtk_tooltips_set_tip(pdialog->tips, view,
		       _("For information on\n"
		         "the citizen governor and governor presets,\n"
			 "including sample presets,\n"
		         "see README.cma."),
		       "");

  rend = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, rend,
      "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
  gtk_tree_view_column_set_cell_data_func(column, rend, cell_data_func,
      pdialog, NULL);

  label = g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", view,
                       "label", _("Prese_ts:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  g_signal_connect(view, "row_activated",
		   G_CALLBACK(cma_activate_preset_callback), pdialog);
  g_signal_connect(view, "key-press-event",
		   G_CALLBACK(cma_preset_key_pressed_callback), pdialog);

  hbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_EDGE);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("Ne_w"));
  image = gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON(button), image);
  gtk_container_add(GTK_CONTAINER(hbox), button);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(cma_add_preset_callback), pdialog);
  pdialog->add_preset_command = button;

  pdialog->del_preset_command = gtk_button_new_from_stock(GTK_STOCK_DELETE);
  gtk_container_add(GTK_CONTAINER(hbox), pdialog->del_preset_command);
  g_signal_connect(pdialog->del_preset_command, "clicked",
		   G_CALLBACK(cma_del_preset_callback), pdialog);

  /* the right-hand side */

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(page), vbox, FALSE, FALSE, 2);

  /* Result */

  frame = gtk_frame_new(_("Results"));
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, FALSE, 0);

  pdialog->result_label =
      gtk_label_new("food\n prod\n trade\n\n people\n grow\n prod\n name");
  gtk_widget_set_name(pdialog->result_label, "city_label");
  gtk_container_add(GTK_CONTAINER(frame), pdialog->result_label);
  gtk_label_set_justify(GTK_LABEL(pdialog->result_label), GTK_JUSTIFY_LEFT);

  /* Minimal Surplus and Factor */

  table = gtk_table_new(O_LAST + 2, 3, TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 2);

  label = gtk_label_new(_("Minimal Surplus"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.1, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);
  label = gtk_label_new(_("Factor"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.1, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 0, 1);

  output_type_iterate(i) {
    label = gtk_label_new(get_output_name(i));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, i + 1, i + 2);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    pdialog->minimal_surplus[i] =
	GTK_ADJUSTMENT(gtk_adjustment_new(-20, -20, 20, 1, 1, 0));

    hscale = gtk_hscale_new(GTK_ADJUSTMENT(pdialog->minimal_surplus[i]));
    gtk_table_attach_defaults(GTK_TABLE(table), hscale, 1, 2, i + 1, i + 2);
    gtk_scale_set_digits(GTK_SCALE(hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_LEFT);

    g_signal_connect(pdialog->minimal_surplus[i],
		     "value_changed",
		     G_CALLBACK(hscale_changed), pdialog);

    pdialog->factor[i] =
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 25, 1, 1, 0));

    hscale = gtk_hscale_new(GTK_ADJUSTMENT(pdialog->factor[i]));
    gtk_table_attach_defaults(GTK_TABLE(table), hscale, 2, 3, i + 1, i + 2);
    gtk_scale_set_digits(GTK_SCALE(hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_LEFT);

    g_signal_connect(pdialog->factor[i], "value_changed",
		     G_CALLBACK(hscale_changed), pdialog);
  } output_type_iterate_end;

  /* Happy Surplus and Factor */

  label = gtk_label_new(_("Celebrate"));
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1,
			    O_LAST + 1, O_LAST + 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2,
			    O_LAST + 1, O_LAST + 2);

  pdialog->happy_button = gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox), pdialog->happy_button, FALSE, FALSE,
		     20);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pdialog->happy_button),
			       FALSE);

  g_signal_connect(pdialog->happy_button, "toggled",
		   G_CALLBACK(hscale_changed), pdialog);

  pdialog->factor[O_LAST] =
      GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 50, 1, 0, 0));

  hscale = gtk_hscale_new(GTK_ADJUSTMENT(pdialog->factor[O_LAST]));
  gtk_table_attach_defaults(GTK_TABLE(table), hscale, 2, 3,
			    O_LAST + 1, O_LAST + 2);
  gtk_scale_set_digits(GTK_SCALE(hscale), 0);
  gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_LEFT);

  g_signal_connect(pdialog->factor[O_LAST],
		   "value_changed",
		   G_CALLBACK(hscale_changed), pdialog);

  /* buttons */

  hbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_EDGE);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock(GTK_STOCK_HELP);
  g_signal_connect(button, "clicked",
		   G_CALLBACK(help_callback), NULL);
  gtk_container_add(GTK_CONTAINER(hbox), button);

  pdialog->active_command = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(hbox), pdialog->active_command);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(pdialog->active_command), align);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(align), vbox);

  pdialog->active_image = gtk_image_new();
  gtk_box_pack_start(GTK_BOX(vbox), pdialog->active_image, FALSE, FALSE, 0);

  pdialog->active_label = gtk_label_new(NULL);
  gtk_widget_set_name(pdialog->active_label, "comment_label");
  gtk_box_pack_end(GTK_BOX(vbox), pdialog->active_label, FALSE, FALSE, 0);

  gtk_widget_show_all(pdialog->shell);

  dialog_list_prepend(dialog_list, pdialog);

  update_cma_preset_list(pdialog);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));

  /* refresh is done in refresh_city_dialog */

  return pdialog;
}

/**************************************************************************
 refreshes the cma dialog
**************************************************************************/
void refresh_cma_dialog(struct city *pcity, enum cma_refresh refresh)
{
  struct cm_result result;
  struct cm_parameter param;
  struct cma_dialog *pdialog = get_cma_dialog(pcity);
  int controlled = cma_is_city_under_agent(pcity, NULL);

  cmafec_get_fe_parameter(pcity, &param);

  /* fill in result label */
  cm_result_from_main_map(&result, pcity, TRUE);
  gtk_label_set_text(GTK_LABEL(pdialog->result_label),
		     cmafec_get_result_descr(pcity, &result, &param));

  /* if called from a hscale, we _don't_ want to do this */
  if (refresh != DONT_REFRESH_HSCALES) {
    set_hscales(&param, pdialog);
  }

  gtk_widget_queue_draw(pdialog->preset_list);

  gtk_widget_set_sensitive(pdialog->active_command, can_client_issue_orders());

  g_signal_handlers_disconnect_matched(pdialog->active_command,
      G_SIGNAL_MATCH_FUNC,
      0, 0, NULL, cma_active_callback, NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pdialog->active_command),
      controlled);
  g_signal_connect(pdialog->active_command, "clicked",
      G_CALLBACK(cma_active_callback), pdialog);

  if (controlled) {
    gtk_image_set_from_stock(GTK_IMAGE(pdialog->active_image),
	GTK_STOCK_YES, GTK_ICON_SIZE_DND);
    gtk_label_set_text_with_mnemonic(GTK_LABEL(pdialog->active_label),
	_("Governor Enabl_ed"));
  } else {
    gtk_image_set_from_stock(GTK_IMAGE(pdialog->active_image),
	GTK_STOCK_NO, GTK_ICON_SIZE_DND);
    gtk_label_set_text_with_mnemonic(GTK_LABEL(pdialog->active_label),
	_("Governor Disabl_ed"));
  }
  gtk_widget_set_sensitive(pdialog->result_label, controlled);
}

/**************************************************************************
 fills in the preset list
**************************************************************************/
static void update_cma_preset_list(struct cma_dialog *pdialog)
{
  char buf[BUFFER_SIZE];
  GtkTreeIter it;
  int i;

  /* Fill preset list */
  gtk_list_store_clear(pdialog->store);

  /* Append the presets */
  if (cmafec_preset_num()) {
    gtk_tooltips_disable(pdialog->tips);

    for (i = 0; i < cmafec_preset_num(); i++) {
      mystrlcpy(buf, cmafec_preset_get_descr(i), sizeof(buf));
      gtk_list_store_append(pdialog->store, &it);
      gtk_list_store_set(pdialog->store, &it, 0, buf, -1);
    }
  } else {
    gtk_tooltips_enable(pdialog->tips);
  }
}

/****************************************************************
 callback for selecting a preset from the preset view
*****************************************************************/
static void cma_activate_preset_callback(GtkTreeView *view, GtkTreePath *path,
				         GtkTreeViewColumn *col, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;
  int preset_index;
  const struct cm_parameter *pparam;

  preset_index = gtk_tree_path_get_indices(path) [0];

  pparam = cmafec_preset_get_parameter(preset_index);

  /* save the change */
  cmafec_set_fe_parameter(pdialog->pcity, pparam);

  if (cma_is_city_under_agent(pdialog->pcity, NULL)) {
    cma_release_city(pdialog->pcity);
    cma_put_city_under_agent(pdialog->pcity, pparam);
  }
  refresh_city_dialog(pdialog->pcity);
}

/**************************************************************************
 pops up a dialog to allow to name your new preset
**************************************************************************/
static void cma_add_preset_callback(GtkWidget *w, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;
  const char *default_name;
  GtkWidget *parent = gtk_widget_get_toplevel(pdialog->shell);
  int index;

  if ((index = gtk_tree_selection_get_row(pdialog->selection)) != -1) {
    default_name = cmafec_preset_get_descr(index);
  } else {
    default_name = _("new preset");
  }

  pdialog->name_shell = input_dialog_create(GTK_WINDOW(parent),
				    _("Name new preset"),
				    _("What should we name the preset?"),
				    default_name,
				    G_CALLBACK(cma_preset_add_callback_yes),
				    pdialog,
				    G_CALLBACK(cma_preset_add_callback_no),
				    pdialog);

  g_signal_connect(pdialog->name_shell, "destroy",
		   G_CALLBACK(cma_preset_add_callback_destroy), data);
}

/****************************************************************
 callback for the add_preset popup (delete popup)
*****************************************************************/
static void cma_preset_add_callback_destroy(GtkWidget *w, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;

  pdialog->name_shell = NULL;
}

/****************************************************************
 callback for the add_preset popup (don't add it)
*****************************************************************/
static void cma_preset_add_callback_no(GtkWidget *w, gpointer data)
{
  input_dialog_destroy(w);
}

/****************************************************************
 callback for the add_preset popup (add it)
*****************************************************************/
static void cma_preset_add_callback_yes(GtkWidget *w, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;

  if (pdialog) {
    struct cm_parameter param;

    cmafec_get_fe_parameter(pdialog->pcity, &param);
    cmafec_preset_add(input_dialog_get_input(w), &param);
    update_cma_preset_list(pdialog);
    refresh_cma_dialog(pdialog->pcity, DONT_REFRESH_HSCALES);
    /* if this or other cities have this set as "custom" */
    city_report_dialog_update();
  }

  input_dialog_destroy(w);
}

/****************************************************************
  Key pressed in preset list
*****************************************************************/
static gboolean cma_preset_key_pressed_callback(GtkWidget *w, GdkEventKey *ev,
						gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;
  int index;

  if ((index = gtk_tree_selection_get_row(pdialog->selection)) == -1) {
    return FALSE;
  }

  if (ev->type == GDK_KEY_PRESS) {
    switch (ev->keyval) {
    case GDK_Delete:
      cma_preset_remove(pdialog, index);
      break;
    case GDK_Insert:
      cma_add_preset_callback(NULL, pdialog);
      break;
    default:
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}


/**************************************************************************
 callback for del_preset 
**************************************************************************/
static void cma_del_preset_callback(GtkWidget *w, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;
  int index;

  if ((index = gtk_tree_selection_get_row(pdialog->selection)) == -1) {
    return;
  }

  cma_preset_remove(pdialog, index);
}

/**************************************************************************
 pops up a dialog to remove a preset
**************************************************************************/
static void cma_preset_remove(struct cma_dialog *pdialog, int preset_index)
{
  GtkWidget *parent = gtk_widget_get_toplevel(pdialog->shell), *shl;

  pdialog->id = preset_index;
  shl = gtk_message_dialog_new(NULL,
			       GTK_DIALOG_DESTROY_WITH_PARENT,
			       GTK_MESSAGE_QUESTION,
			       GTK_BUTTONS_YES_NO,
			       _("Remove this preset?"));
  setup_dialog(shl, parent);
  pdialog->preset_remove_shell = shl;

  gtk_window_set_title(GTK_WINDOW(shl), cmafec_preset_get_descr(preset_index));
  gtk_window_set_position(GTK_WINDOW(shl), GTK_WIN_POS_CENTER_ON_PARENT);

  g_signal_connect(shl, "response",
		   G_CALLBACK(cma_preset_remove_response), pdialog);

  gtk_window_present(GTK_WINDOW(shl));
}

/****************************************************************
 callback for the remove_preset popup
*****************************************************************/
static void cma_preset_remove_response(GtkWidget *w, gint response,
				       gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;

  if (response == GTK_RESPONSE_YES) {
    cmafec_preset_remove(pdialog->id);
    pdialog->id = -1;
    update_cma_preset_list(pdialog);
    refresh_cma_dialog(pdialog->pcity, DONT_REFRESH_HSCALES);
    /* if this or other cities have this set, reset to "custom" */
    city_report_dialog_update();
  }
  gtk_widget_destroy(w);
  
  pdialog->preset_remove_shell = NULL;
}

/**************************************************************************
 activates/deactivates agent control.
**************************************************************************/
static void cma_active_callback(GtkWidget *w, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;

  if (cma_is_city_under_agent(pdialog->pcity, NULL)) {
    cma_release_city(pdialog->pcity);
  } else {
    struct cm_parameter param;

    cmafec_get_fe_parameter(pdialog->pcity, &param);
    cma_put_city_under_agent(pdialog->pcity, &param);
  }
  refresh_city_dialog(pdialog->pcity);
}

/****************************************************************
 called to adjust the sliders when a preset is selected
 notice that we don't want to call update_result here. 
*****************************************************************/
static void set_hscales(const struct cm_parameter *const parameter,
			struct cma_dialog *pdialog)
{
  allow_refreshes = 0;
  output_type_iterate(i) {
    gtk_adjustment_set_value(pdialog->minimal_surplus[i],
			     parameter->minimal_surplus[i]);
    gtk_adjustment_set_value(pdialog->factor[i], parameter->factor[i]);
  } output_type_iterate_end;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pdialog->happy_button),
			       parameter->require_happy);
  gtk_adjustment_set_value(pdialog->factor[O_LAST],
			   parameter->happy_factor);
  allow_refreshes = 1;
}

/************************************************************************
 callback if we moved the sliders.
*************************************************************************/
static void hscale_changed(GtkAdjustment *get, gpointer data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;
  struct cm_parameter param;

  if (!allow_refreshes) {
    return;
  }

  cmafec_get_fe_parameter(pdialog->pcity, &param);
  output_type_iterate(i) {
    param.minimal_surplus[i] = (int) (pdialog->minimal_surplus[i]->value);
    param.factor[i] = (int) (pdialog->factor[i]->value);
  } output_type_iterate_end;
  param.require_happy =
      (GTK_TOGGLE_BUTTON(pdialog->happy_button)->active ? 1 : 0);
  param.happy_factor = (int) (pdialog->factor[O_LAST]->value);

  /* save the change */
  cmafec_set_fe_parameter(pdialog->pcity, &param);

  /* refreshes the cma */
  if (cma_is_city_under_agent(pdialog->pcity, NULL)) {
    cma_release_city(pdialog->pcity);
    cma_put_city_under_agent(pdialog->pcity, &param);
    refresh_city_dialog(pdialog->pcity);
  } else {
    refresh_cma_dialog(pdialog->pcity, DONT_REFRESH_HSCALES);
  }
}

