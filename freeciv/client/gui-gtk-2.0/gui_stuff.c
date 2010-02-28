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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

#include "colors.h"
#include "options.h"
#include "gui_main.h"

#include "gui_stuff.h"


static GList *dialog_list;

static GtkSizeGroup *gui_action;


/**************************************************************************
...
**************************************************************************/
void gtk_expose_now(GtkWidget *w)
{
  gtk_widget_queue_draw(w);
}

/**************************************************************************
...
**************************************************************************/
void gtk_set_relative_position(GtkWidget *ref, GtkWidget *w, int px, int py)
{
  gint x, y, width, height;

  gtk_window_get_position(GTK_WINDOW(ref), &x, &y);
  gtk_window_get_size(GTK_WINDOW(ref), &width, &height);

  x += px*width/100;
  y += py*height/100;

  gtk_window_move(GTK_WINDOW(w), x, y);
}

/**************************************************************************
...
**************************************************************************/
GtkWidget *gtk_stockbutton_new(const gchar *stock, const gchar *label_text)
{
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *hbox;
  GtkWidget *align;
  GtkWidget *button;
  
  button = gtk_button_new();

  label = gtk_label_new_with_mnemonic(label_text);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);
  g_object_set_data(G_OBJECT(button), "label", label);

  image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_BUTTON);
  g_object_set_data(G_OBJECT(button), "image", image);

  hbox = gtk_hbox_new(FALSE, 2);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);

  gtk_box_pack_start(GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX (hbox), label, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(button), align);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  gtk_widget_show_all(align);
  return button;
}

/**************************************************************************
  Changes the lable (with mnemonic) on an existing stockbutton.  See
  gtk_stockbutton_new.
**************************************************************************/
void gtk_stockbutton_set_label(GtkWidget *button, const gchar *label_text)
{
  GtkWidget *label;

  label = g_object_get_data(G_OBJECT(button), "label");
  gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), label_text);
}

/**************************************************************************
  Returns gettext-converted list of n strings.  The individual strings
  in the list are as returned by gettext().  In case of no NLS, the strings
  will be the original strings, so caller should ensure that the originals
  persist for as long as required.  (For no NLS, still allocate the
  list, for consistency.)

  (This is not directly gui/gtk related, but it fits in here
  because so far it is used for doing i18n for gtk titles...)
**************************************************************************/
void intl_slist(int n, const char **s, bool *done)
{
  int i;

  if (!*done) {
    for(i=0; i<n; i++) {
      s[i] = Q_(s[i]);
    }

    *done = TRUE;
  }
}

/****************************************************************
...
*****************************************************************/
void itree_begin(GtkTreeModel *model, ITree *it)
{
  it->model = model;
  it->end = !gtk_tree_model_get_iter_first(it->model, &it->it);
}

/****************************************************************
...
*****************************************************************/
gboolean itree_end(ITree *it)
{
  return it->end;
}

/****************************************************************
...
*****************************************************************/
void itree_next(ITree *it)
{
  it->end = !gtk_tree_model_iter_next(it->model, &it->it);
}

/****************************************************************
...
*****************************************************************/
void itree_set(ITree *it, ...)
{
  va_list ap;
  
  va_start(ap, it);
  gtk_tree_store_set_valist(GTK_TREE_STORE(it->model), &it->it, ap);
  va_end(ap);
}

/****************************************************************
...
*****************************************************************/
void itree_get(ITree *it, ...)
{
  va_list ap;
  
  va_start(ap, it);
  gtk_tree_model_get_valist(it->model, &it->it, ap);
  va_end(ap);
}

/****************************************************************
...
*****************************************************************/
void tstore_append(GtkTreeStore *store, ITree *it, ITree *parent)
{
  it->model = GTK_TREE_MODEL(store);
  if (parent)
    gtk_tree_store_append(GTK_TREE_STORE(it->model), &it->it, &parent->it);
  else
    gtk_tree_store_append(GTK_TREE_STORE(it->model), &it->it, NULL);
  it->end = FALSE;
}

/****************************************************************
...
*****************************************************************/
gboolean itree_is_selected(GtkTreeSelection *selection, ITree *it)
{
  return gtk_tree_selection_iter_is_selected(selection, &it->it);
}

/****************************************************************
...
*****************************************************************/
void itree_select(GtkTreeSelection *selection, ITree *it)
{
  gtk_tree_selection_select_iter(selection, &it->it);
}

/****************************************************************
...
*****************************************************************/
void itree_unselect(GtkTreeSelection *selection, ITree *it)
{
  gtk_tree_selection_unselect_iter(selection, &it->it);
}

/**************************************************************************
  Return the selected row in a GtkTreeSelection.
  If no row is selected return -1.
**************************************************************************/
gint gtk_tree_selection_get_row(GtkTreeSelection *selection)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  gint row = -1;

  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    GtkTreePath *path;
    gint *idx;

    path = gtk_tree_model_get_path(model, &it);
    idx = gtk_tree_path_get_indices(path);
    row = idx[0];
    gtk_tree_path_free(path);
  }
  return row;
}

/**************************************************************************
...
**************************************************************************/
void gtk_tree_view_focus(GtkTreeView *view)
{
  GtkTreePath *path;

  if ((path = gtk_tree_path_new_first())) {
    gtk_tree_view_set_cursor(view, path, NULL, FALSE);
    gtk_tree_path_free(path);
    gtk_widget_grab_focus(GTK_WIDGET(view));
  }
}

/**************************************************************************
...
**************************************************************************/
static void close_callback(GtkDialog *dialog, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

/**********************************************************************
  This function handles new windows which are subwindows to the
  toplevel window. It must be called on every dialog in the game,
  so fullscreen windows are handled properly by the window manager.
***********************************************************************/
void setup_dialog(GtkWidget *shell, GtkWidget *parent)
{
  if (gui_gtk2_dialogs_on_top || fullscreen_mode) {
    gtk_window_set_transient_for(GTK_WINDOW(shell),
                                 GTK_WINDOW(parent));
    gtk_window_set_type_hint(GTK_WINDOW(shell),
                             GDK_WINDOW_TYPE_HINT_DIALOG);
  } else {
    gtk_window_set_type_hint(GTK_WINDOW(shell),
                             GDK_WINDOW_TYPE_HINT_NORMAL);
  }

  /* Close dialog window on Escape keypress. */
  if (GTK_IS_DIALOG(shell)) {
    g_signal_connect_after(shell, "close", G_CALLBACK(close_callback), shell);
  }
}


/**************************************************************************
  Emit a dialog response.
**************************************************************************/
static void gui_dialog_response(struct gui_dialog *dlg, int response)
{
  if (dlg->response_callback) {
    (*dlg->response_callback)(dlg, response, dlg->user_data);
  }
}

/**************************************************************************
  Default dialog response handler. Destroys the dialog.
**************************************************************************/
static void gui_dialog_destroyed(struct gui_dialog *dlg, int response,
                                 gpointer data)
{
  gui_dialog_destroy(dlg);
}

/**************************************************************************
  Cleanups the leftovers after a dialog is destroyed.
**************************************************************************/
static void gui_dialog_destroy_handler(GtkWidget *w, struct gui_dialog *dlg)
{
  if (dlg->type == GUI_DIALOG_TAB) {
    GtkWidget *notebook = dlg->v.tab.notebook;
    gulong handler_id = dlg->v.tab.handler_id;

    g_signal_handler_disconnect(notebook, handler_id);
  }

  g_object_unref(dlg->gui_button);

  if (*(dlg->source)) {
    *(dlg->source) = NULL;
  }

  dialog_list = g_list_remove(dialog_list, dlg);
  
  /* Raise the return dialog set by gui_dialog_set_return_dialog() */
  if (dlg->return_dialog_id != -1) {
    GList *it;
    for (it = dialog_list; it; it = g_list_next(it)) {
      struct gui_dialog * adialog = (struct gui_dialog *)it->data;
      if (adialog->id == dlg->return_dialog_id) {
        gui_dialog_raise(adialog);
	break;
      }
    }
  }
  
  if (dlg->title) {
    free(dlg->title);
  }
  
  free(dlg);
}

/**************************************************************************
  Emit a delete event response on dialog deletion in case the end-user
  needs to know when a deletion took place.
  Popup dialog version
**************************************************************************/
static gint gui_dialog_delete_handler(GtkWidget *widget,
				      GdkEventAny *ev, gpointer data)
{
  struct gui_dialog *dlg = data;

  /* emit response signal. */
  gui_dialog_response(dlg, GTK_RESPONSE_DELETE_EVENT);
                                                                               
  /* do the destroy by default. */
  return FALSE;
}

/**************************************************************************
  Emit a delete event response on dialog deletion in case the end-user
  needs to know when a deletion took place.
  TAB version
**************************************************************************/
static gint gui_dialog_delete_tab_handler(struct gui_dialog* dlg)
{
  GtkWidget* notebook;
  int n;
  
  notebook = dlg->v.tab.notebook;
  n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  if (gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n)
      != dlg->v.tab.child) {
    gui_dialog_set_return_dialog(dlg, NULL);
  }			                                  
  
  /* emit response signal. */
  gui_dialog_response(dlg, GTK_RESPONSE_DELETE_EVENT);
                                                                               
  /* do the destroy by default. */
  return FALSE;
}


/**************************************************************************
  Allow the user to close a dialog using Escape or CTRL+W.
**************************************************************************/
static gboolean gui_dialog_key_press_handler(GtkWidget *w, GdkEventKey *ev,
					     gpointer data)
{
  struct gui_dialog *dlg = data;

  if (ev->keyval == GDK_Escape
	|| ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_w)) {
    /* emit response signal. */
    gui_dialog_response(dlg, GTK_RESPONSE_DELETE_EVENT);
  }

  /* propagate event further. */
  return FALSE;
}

/**************************************************************************
  Resets tab colour on tab activation.
**************************************************************************/
static void gui_dialog_switch_page_handler(GtkNotebook *notebook,
					   GtkNotebookPage *page,
					   guint num,
					   struct gui_dialog *dlg)
{
  gint n;

  n = gtk_notebook_page_num(GTK_NOTEBOOK(dlg->v.tab.notebook), dlg->vbox);

  if (n == num) {
    GtkRcStyle *rc_style = gtk_widget_get_modifier_style(dlg->v.tab.label);

    rc_style->color_flags[GTK_STATE_ACTIVE] &= ~GTK_RC_FG;
    gtk_widget_modify_style(dlg->v.tab.label, rc_style);
  }
}

/**************************************************************************
  Changes a tab into a window.
**************************************************************************/
static void gui_dialog_detach(struct gui_dialog* dlg)
{
  gint n;
  GtkWidget *window, *notebook;
  gulong handler_id;

  if (dlg->type != GUI_DIALOG_TAB) {
    return;
  }
  dlg->type = GUI_DIALOG_WINDOW;
  
  /* Create a new reference to the main widget, so it won't be 
   * destroyed in gtk_notebook_remove_page() */
  g_object_ref(dlg->vbox);

  /* Remove widget from the notebook */
  notebook = dlg->v.tab.notebook;
  handler_id = dlg->v.tab.handler_id;
  g_signal_handler_disconnect(notebook, handler_id);

  n = gtk_notebook_page_num(GTK_NOTEBOOK(dlg->v.tab.notebook), dlg->vbox);
  gtk_notebook_remove_page(GTK_NOTEBOOK(dlg->v.tab.notebook), n);


  /* Create window and put the widget inside */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), dlg->title);
  setup_dialog(window, toplevel);

  gtk_container_add(GTK_CONTAINER(window), dlg->vbox);
  dlg->v.window = window;
  g_signal_connect(window, "delete_event",
    G_CALLBACK(gui_dialog_delete_handler), dlg);
	
  gtk_window_set_default_size(GTK_WINDOW(dlg->v.window),
                              dlg->default_width,
			      dlg->default_height);    
  gtk_widget_show_all(window);
}

/***************************************************************************
  Someone has clicked on a label in a notebook
***************************************************************************/
static gboolean click_on_tab_callback(GtkWidget* w,
                                     GdkEventButton* button,
				     gpointer data)
{
  if (button->type != GDK_2BUTTON_PRESS) {
    return FALSE;
  }
  if (button->button != 1) {
    return FALSE;
  }
  gui_dialog_detach((struct gui_dialog*) data);
  return TRUE;
}


/**************************************************************************
  Creates a new dialog. It will be a tab or a window depending on the
  current user setting of 'gui_gtk2_enable_tabs'.
  Sets pdlg to point to the dialog once it is create, Zeroes pdlg on
  dialog destruction.
  user_data will be passed through response function
**************************************************************************/
void gui_dialog_new(struct gui_dialog **pdlg, GtkNotebook *notebook,
                    gpointer user_data)
{
  struct gui_dialog *dlg;
  GtkWidget *vbox, *action_area;
  static int dialog_id_counter;

  dlg = fc_malloc(sizeof(*dlg));
  dialog_list = g_list_prepend(dialog_list, dlg);

  dlg->source = pdlg;
  *pdlg = dlg;
  dlg->user_data = user_data;
  dlg->title = NULL;
  
  dlg->default_width = 200;
  dlg->default_height = 300;

  if (gui_gtk2_enable_tabs) {
    dlg->type = GUI_DIALOG_TAB;
  } else {
    dlg->type = GUI_DIALOG_WINDOW;
  }

  if (!gui_action) {
    gui_action = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);
  }
  dlg->gui_button = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

  if (gui_gtk2_enable_tabs && notebook == GTK_NOTEBOOK(bottom_notebook)) {
    vbox = gtk_hbox_new(FALSE, 0);
    action_area = gtk_vbox_new(FALSE, 2);
  } else {
    vbox = gtk_vbox_new(FALSE, 0);
    action_area = gtk_hbox_new(FALSE, 2);
  }

  gtk_widget_show(vbox);
  gtk_box_pack_end(GTK_BOX(vbox), action_area, FALSE, TRUE, 0);
  gtk_widget_show(action_area);

  gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
  gtk_box_set_spacing(GTK_BOX(action_area), 10);
  gtk_container_set_border_width(GTK_CONTAINER(action_area), 5);

  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    {
      GtkWidget *window;

      window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_widget_set_name(window, "Freeciv");
      gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
      setup_dialog(window, toplevel);

      gtk_container_add(GTK_CONTAINER(window), vbox);
      dlg->v.window = window;
      g_signal_connect(window, "delete_event",
        G_CALLBACK(gui_dialog_delete_handler), dlg);
      
    }
    break;
  case GUI_DIALOG_TAB:
    {
      GtkWidget *hbox, *label, *image, *button, *event_box;
      gint w, h;
      char buf[256];

      gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &w, &h);

      hbox = gtk_hbox_new(FALSE, 0);

      label = gtk_label_new(NULL);
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      gtk_misc_set_padding(GTK_MISC(label), 4, 0);
      gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

      button = gtk_button_new();
      gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
      g_signal_connect_swapped(button, "clicked",
	  G_CALLBACK(gui_dialog_delete_tab_handler), dlg);

      my_snprintf(buf, sizeof(buf), _("Close Tab:\n%s"), _("Ctrl+W"));
      gtk_tooltips_set_tip(main_tips, button, buf, "");

      image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
      gtk_widget_set_size_request(button, w, h);
      gtk_container_add(GTK_CONTAINER(button), image);

      gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

      gtk_widget_show_all(hbox);
      
      event_box = gtk_event_box_new();
      gtk_container_add(GTK_CONTAINER(event_box), hbox);

      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, event_box);
      dlg->v.tab.handler_id =
	g_signal_connect(notebook, "switch_page",
	    G_CALLBACK(gui_dialog_switch_page_handler), dlg);
      dlg->v.tab.child = vbox;

      dlg->v.tab.label = label;
      dlg->v.tab.notebook = GTK_WIDGET(notebook);
      
      gtk_widget_add_events(event_box, GDK_BUTTON2_MOTION_MASK);
      g_signal_connect(event_box, "button-press-event",
                       G_CALLBACK(click_on_tab_callback), dlg);
    }
    break;
  }

  dlg->vbox = vbox;
  dlg->action_area = action_area;

  dlg->response_callback = gui_dialog_destroyed;
  
  dlg->id = dialog_id_counter;
  dialog_id_counter++;
  dlg->return_dialog_id = -1;

  g_signal_connect(vbox, "destroy",
      G_CALLBACK(gui_dialog_destroy_handler), dlg);
  g_signal_connect(vbox, "key_press_event",
      G_CALLBACK(gui_dialog_key_press_handler), dlg);

  g_object_set_data(G_OBJECT(vbox), "gui-dialog-data", dlg);
}

/**************************************************************************
  Called when a dialog button is activated.
**************************************************************************/
static void action_widget_activated(GtkWidget *button, GtkWidget *vbox)
{
  struct gui_dialog *dlg =
    g_object_get_data(G_OBJECT(vbox), "gui-dialog-data");
  gpointer arg2 =
    g_object_get_data(G_OBJECT(button), "gui-dialog-response-data");

  gui_dialog_response(dlg, GPOINTER_TO_INT(arg2));
}

/**************************************************************************
  Places a button into a dialog, taking care of setting up signals, etc.
**************************************************************************/
static void gui_dialog_pack_button(struct gui_dialog *dlg, GtkWidget *button,
				   int response)
{
  gint signal_id;

  g_return_if_fail(GTK_IS_BUTTON(button));

  g_object_set_data(G_OBJECT(button), "gui-dialog-response-data",
      GINT_TO_POINTER(response));

  if ((signal_id = g_signal_lookup("clicked", GTK_TYPE_BUTTON))) {
    GClosure *closure;

    closure = g_cclosure_new_object(G_CALLBACK(action_widget_activated),
	G_OBJECT(dlg->vbox));
    g_signal_connect_closure_by_id(button, signal_id, 0, closure, FALSE);
  }

  gtk_box_pack_end(GTK_BOX(dlg->action_area), button, FALSE, TRUE, 0);
  gtk_size_group_add_widget(gui_action, button);
  gtk_size_group_add_widget(dlg->gui_button, button);
}

/**************************************************************************
  Adds a button to a dialog, allowing the choice of a special stock item.
**************************************************************************/
GtkWidget *gui_dialog_add_stockbutton(struct gui_dialog *dlg,
				      const char *stock,
				      const char *text, int response)
{
  GtkWidget *button;

  button = gtk_stockbutton_new(stock, text);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gui_dialog_pack_button(dlg, button, response);

  return button;
}

/**************************************************************************
  Adds a button to a dialog.
**************************************************************************/
GtkWidget *gui_dialog_add_button(struct gui_dialog *dlg,
				 const char *text, int response)
{
  GtkWidget *button;

  button = gtk_button_new_from_stock(text);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gui_dialog_pack_button(dlg, button, response);

  return button;
}

/**************************************************************************
  Adds a widget to a dialog.
**************************************************************************/
GtkWidget *gui_dialog_add_widget(struct gui_dialog *dlg,
				 GtkWidget *widget)
{
  gtk_box_pack_start(GTK_BOX(dlg->action_area), widget, FALSE, TRUE, 0);
  gtk_size_group_add_widget(gui_action, widget);

  return widget;
}

/**************************************************************************
  Changes the default dialog response.
**************************************************************************/
void gui_dialog_set_default_response(struct gui_dialog *dlg, int response)
{
  GList *children;
  GList *list;

  children = gtk_container_get_children(GTK_CONTAINER(dlg->action_area));

  for (list = children; list; list = g_list_next(list)) {
    GtkWidget *button = list->data;

    if (GTK_IS_BUTTON(button)) {
      gpointer data = g_object_get_data(G_OBJECT(button),
	  "gui-dialog-response-data");

      if (response == GPOINTER_TO_INT(data)) {
	gtk_widget_grab_default(button);
      }
    }
  }

  g_list_free(children);
}

/**************************************************************************
  Change the sensitivity of a dialog button.
**************************************************************************/
void gui_dialog_set_response_sensitive(struct gui_dialog *dlg,
				       int response, bool setting)
{
  GList *children;
  GList *list;

  children = gtk_container_get_children(GTK_CONTAINER(dlg->action_area));

  for (list = children; list; list = g_list_next(list)) {
    GtkWidget *button = list->data;

    if (GTK_IS_BUTTON(button)) {
      gpointer data = g_object_get_data(G_OBJECT(button),
	  "gui-dialog-response-data");

      if (response == GPOINTER_TO_INT(data)) {
	gtk_widget_set_sensitive(button, setting);
      }
    }
  }

  g_list_free(children);
}

/**************************************************************************
  Get the dialog's toplevel window.
**************************************************************************/
GtkWidget *gui_dialog_get_toplevel(struct gui_dialog *dlg)
{
  return gtk_widget_get_toplevel(dlg->vbox);
}

/**************************************************************************
  Show the dialog contents, but not the dialog per se.
**************************************************************************/
void gui_dialog_show_all(struct gui_dialog *dlg)
{
  gtk_widget_show_all(dlg->vbox);

  if (dlg->type == GUI_DIALOG_TAB) {
    GList *children;
    GList *list;
    gint num_visible = 0;

    children = gtk_container_get_children(GTK_CONTAINER(dlg->action_area));

    for (list = children; list; list = g_list_next(list)) {
      GtkWidget *button = list->data;

      if (!GTK_IS_BUTTON(button)) {
	num_visible++;
      } else {
	gpointer data = g_object_get_data(G_OBJECT(button),
	    "gui-dialog-response-data");
	int response = GPOINTER_TO_INT(data);

	if (response != GTK_RESPONSE_CLOSE
	    && response != GTK_RESPONSE_CANCEL) {
	  num_visible++;
	} else {
	  gtk_widget_hide(button);
	}
      }
    }
    g_list_free(children);

    if (num_visible == 0) {
      gtk_widget_hide(dlg->action_area);
    }
  }
}

/**************************************************************************
  Notify the user the dialog has changed.
**************************************************************************/
void gui_dialog_present(struct gui_dialog *dlg)
{
  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    gtk_widget_show(dlg->v.window);
    break;
  case GUI_DIALOG_TAB:
    {
      GtkNotebook *notebook = GTK_NOTEBOOK(dlg->v.tab.notebook);
      gint current, n;

      current = gtk_notebook_get_current_page(notebook);
      n = gtk_notebook_page_num(notebook, dlg->vbox);

      if (current != n) {
	GtkWidget *label = dlg->v.tab.label;
	GdkColormap *cmap = gtk_widget_get_default_colormap();
	GdkColor color = {.red = 255 << 8, .green = 0, .blue = 0};

	gdk_rgb_find_color(cmap, &color);
	gtk_widget_modify_fg(label, GTK_STATE_ACTIVE, &color);
      }
    }
    break;
  }
}

/**************************************************************************
  Raise dialog to top.
**************************************************************************/
void gui_dialog_raise(struct gui_dialog *dlg)
{
  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    gtk_window_present(GTK_WINDOW(dlg->v.window));
    break;
  case GUI_DIALOG_TAB:
    {
      GtkNotebook *notebook = GTK_NOTEBOOK(dlg->v.tab.notebook);
      gint n;

      n = gtk_notebook_page_num(notebook, dlg->vbox);
      gtk_notebook_set_current_page(notebook, n);
    }
    break;
  }
}

/**************************************************************************
  Alert the user to an important event.
**************************************************************************/
void gui_dialog_alert(struct gui_dialog *dlg)
{
  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    break;
  case GUI_DIALOG_TAB:
    {
      GtkNotebook *notebook = GTK_NOTEBOOK(dlg->v.tab.notebook);
      gint current, n;

      current = gtk_notebook_get_current_page(notebook);
      n = gtk_notebook_page_num(notebook, dlg->vbox);

      if (current != n) {
	GtkWidget *label = dlg->v.tab.label;
	GdkColormap *cmap = gtk_widget_get_default_colormap();
	GdkColor color = {.red = 0, .green = 0, .blue = 255 << 8};

	gdk_rgb_find_color(cmap, &color);
	gtk_widget_modify_fg(label, GTK_STATE_ACTIVE, &color);
      }
    }
    break;
  }
}

/**************************************************************************
  Sets the dialog's default size (applies to toplevel windows only).
**************************************************************************/
void gui_dialog_set_default_size(struct gui_dialog *dlg, int width, int height)
{
  dlg->default_width = width;
  dlg->default_height = height;
  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    gtk_window_set_default_size(GTK_WINDOW(dlg->v.window), width, height);
    break;
  case GUI_DIALOG_TAB:
    break;
  }
}

/**************************************************************************
  Changes a dialog's title.
**************************************************************************/
void gui_dialog_set_title(struct gui_dialog *dlg, const char *title)
{
  if (dlg->title) {
    free(dlg->title);
  }
  dlg->title = mystrdup(title);
  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    gtk_window_set_title(GTK_WINDOW(dlg->v.window), title);
    break;
  case GUI_DIALOG_TAB:
    gtk_label_set_text_with_mnemonic(GTK_LABEL(dlg->v.tab.label), title);
    break;
  }
}

/**************************************************************************
  Destroy a dialog.
**************************************************************************/
void gui_dialog_destroy(struct gui_dialog *dlg)
{
  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    gtk_widget_destroy(dlg->v.window);
    break;
  case GUI_DIALOG_TAB:
    {
      gint n;

      n = gtk_notebook_page_num(GTK_NOTEBOOK(dlg->v.tab.notebook), dlg->vbox);
      gtk_notebook_remove_page(GTK_NOTEBOOK(dlg->v.tab.notebook), n);
    }
    break;
  }
}

/**************************************************************************
  Destroy all dialogs.
**************************************************************************/
void gui_dialog_destroy_all(void)
{
  GList *it, *it_next;

  for (it = dialog_list; it; it = it_next) {
    it_next = g_list_next(it);

    gui_dialog_destroy((struct gui_dialog *)it->data);
  }
}

/**************************************************************************
  Set the response callback for a dialog.
**************************************************************************/
void gui_dialog_response_set_callback(struct gui_dialog *dlg,
    GUI_DIALOG_RESPONSE_FUN fun)
{
  dlg->response_callback = fun;
}

/**************************************************************************
  When the dlg dialog is destroyed the return_dialog will be raised
**************************************************************************/
void gui_dialog_set_return_dialog(struct gui_dialog *dlg,
                                  struct gui_dialog *return_dialog)
{
  if (return_dialog == NULL) {
    dlg->return_dialog_id = -1;
  } else {
    dlg->return_dialog_id = return_dialog->id;
  }
}

/**************************************************************************
  Updates a gui font style from a client option.
**************************************************************************/
void gui_update_font_from_option(struct client_option *o) {
  if (o->type == COT_FONT) {
    char str[512];

    my_snprintf(str, sizeof(str),
	"style \"ext-%s\" {\n"
	"  font_name = \"%s\"\n"
	"}\n"
	"\n"
	"widget \"Freeciv*.%s\" style \"ext-%s\"",
	o->name,
	o->string.pvalue,
	o->name, o->name);

    gtk_rc_parse_string(str);
  }
}

/****************************************************************************
  Temporarily disable signal invocation of the given callback for the given
  GObject. Re-enable the signal with enable_gobject_callback.
****************************************************************************/
void disable_gobject_callback(GObject *obj, GCallback cb)
{
  gulong hid;

  if (!obj || !cb) {
    return;
  }

  hid = g_signal_handler_find(obj, G_SIGNAL_MATCH_FUNC,
                              0, 0, NULL, cb, NULL);
  g_signal_handler_block(obj, hid);
}

/****************************************************************************
  Re-enable a signal callback blocked by disable_gobject_callback.
****************************************************************************/
void enable_gobject_callback(GObject *obj, GCallback cb)
{
  gulong hid;

  if (!obj || !cb) {
    return;
  }

  hid = g_signal_handler_find(obj, G_SIGNAL_MATCH_FUNC,
                              0, 0, NULL, cb, NULL);
  g_signal_handler_unblock(obj, hid);
}

/**************************************************************************
  Convenience function to add a column to a GtkTreeView. Returns the added
  column, or NULL if an error occured.
**************************************************************************/
GtkTreeViewColumn *add_treeview_column(GtkWidget *view, const char *title,
                                       GType gtype, int model_index)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  const char *attr;

  g_return_val_if_fail(view != NULL, NULL);
  g_return_val_if_fail(GTK_IS_TREE_VIEW(view), NULL);
  g_return_val_if_fail(title != NULL, NULL);

  if (gtype == G_TYPE_BOOLEAN) {
    rend = gtk_cell_renderer_toggle_new();
    attr = "active";
  } else if (gtype == GDK_TYPE_PIXBUF) {
    rend = gtk_cell_renderer_pixbuf_new();
    attr = "pixbuf";
  } else {
    rend = gtk_cell_renderer_text_new();
    attr = "text";
  }

  col = gtk_tree_view_column_new_with_attributes(title, rend, attr,
                                                 model_index, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  return col;
}
