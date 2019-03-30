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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* client */
#include "options.h"

/* client/gui-gtk-3.22 */
#include "colors.h"
#include "gui_main.h"

#include "gui_stuff.h"


static GList *dialog_list;

static GtkSizeGroup *gui_action;

static GtkCssProvider *dlg_tab_provider = NULL;


/**********************************************************************//**
  Draw widget now
**************************************************************************/
void gtk_expose_now(GtkWidget *w)
{
  gtk_widget_queue_draw(w);
}

/**********************************************************************//**
  Set window position relative to reference window
**************************************************************************/
void set_relative_window_position(GtkWindow *ref, GtkWindow *w, int px, int py)
{
  gint x, y, width, height;

  gtk_window_get_position(ref, &x, &y);
  gtk_window_get_size(ref, &width, &height);

  x += px * width / 100;
  y += py * height / 100;

  gtk_window_move(w, x, y);
}

/**********************************************************************//**
  Create new icon button with label
**************************************************************************/
GtkWidget *icon_label_button_new(const gchar *icon_name,
                                 const gchar *label_text)
{
  GtkWidget *button;
  GtkWidget *image;

  button = gtk_button_new_with_mnemonic(label_text);

  if (icon_name != NULL) {
    image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
  }

  return button;
}

/**********************************************************************//**
  Changes the label (with mnemonic) on an existing stockbutton.  See
  gtk_stockbutton_new.
**************************************************************************/
void gtk_stockbutton_set_label(GtkWidget *button, const gchar *label_text)
{
  gtk_button_set_label(GTK_BUTTON(button), label_text);
}

/**********************************************************************//**
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
    for (i = 0; i < n; i++) {
      s[i] = Q_(s[i]);
    }

    *done = TRUE;
  }
}

/**********************************************************************//**
  Set itree to the beginning
**************************************************************************/
void itree_begin(GtkTreeModel *model, ITree *it)
{
  it->model = model;
  it->end = !gtk_tree_model_get_iter_first(it->model, &it->it);
}

/**********************************************************************//**
  Return whether itree end has been reached
**************************************************************************/
gboolean itree_end(ITree *it)
{
  return it->end;
}

/**********************************************************************//**
  Make itree to go forward one step
**************************************************************************/
void itree_next(ITree *it)
{
  it->end = !gtk_tree_model_iter_next(it->model, &it->it);
}

/**********************************************************************//**
  Store values to itree
**************************************************************************/
void itree_set(ITree *it, ...)
{
  va_list ap;

  va_start(ap, it);
  gtk_tree_store_set_valist(GTK_TREE_STORE(it->model), &it->it, ap);
  va_end(ap);
}

/**********************************************************************//**
  Get values from itree
**************************************************************************/
void itree_get(ITree *it, ...)
{
  va_list ap;

  va_start(ap, it);
  gtk_tree_model_get_valist(it->model, &it->it, ap);
  va_end(ap);
}

/**********************************************************************//**
  Append one item to the end of tree store
**************************************************************************/
void tstore_append(GtkTreeStore *store, ITree *it, ITree *parent)
{
  it->model = GTK_TREE_MODEL(store);
  if (parent) {
    gtk_tree_store_append(GTK_TREE_STORE(it->model), &it->it, &parent->it);
  } else {
    gtk_tree_store_append(GTK_TREE_STORE(it->model), &it->it, NULL);
  }
  it->end = FALSE;
}

/**********************************************************************//**
  Return whether current itree item is selected 
**************************************************************************/
gboolean itree_is_selected(GtkTreeSelection *selection, ITree *it)
{
  return gtk_tree_selection_iter_is_selected(selection, &it->it);
}

/**********************************************************************//**
  Add current itree item to selection
**************************************************************************/
void itree_select(GtkTreeSelection *selection, ITree *it)
{
  gtk_tree_selection_select_iter(selection, &it->it);
}

/**********************************************************************//**
  Remove current itree item from selection
**************************************************************************/
void itree_unselect(GtkTreeSelection *selection, ITree *it)
{
  gtk_tree_selection_unselect_iter(selection, &it->it);
}

/**********************************************************************//**
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

/**********************************************************************//**
  Give focus to view
**************************************************************************/
void gtk_tree_view_focus(GtkTreeView *view)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;

  if ((model = gtk_tree_view_get_model(view))
      && gtk_tree_model_get_iter_first(model, &iter)
      && (path = gtk_tree_model_get_path(model, &iter))) {
    gtk_tree_view_set_cursor(view, path, NULL, FALSE);
    gtk_tree_path_free(path);
    gtk_widget_grab_focus(GTK_WIDGET(view));
  }
}

/**********************************************************************//**
  Create an auxiliary menubar (i.e., not the main menubar at the top of
  the window).
**************************************************************************/
GtkWidget *gtk_aux_menu_bar_new(void)
{
  GtkWidget *menubar = gtk_menu_bar_new();

  /*
   * Ubuntu Linux's Ayatana/Unity desktop environment likes to steal the
   * application's main menu bar from its window and put it at the top of
   * the screen. It needs a hint in order not to steal menu bars other
   * than the main one. Gory details at
   * https://bugs.launchpad.net/ubuntu/+source/freeciv/+bug/743265
   */
  if (g_object_class_find_property(
        G_OBJECT_CLASS(GTK_MENU_BAR_GET_CLASS(menubar)), "ubuntu-local")) {
    g_object_set(G_OBJECT(menubar), "ubuntu-local", TRUE, NULL);
  }

  return menubar;
}

/**********************************************************************//**
  Generic close callback for all widgets
**************************************************************************/
static void close_callback(GtkDialog *dialog, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

/**********************************************************************//**
  This function handles new windows which are subwindows to the
  toplevel window. It must be called on every dialog in the game,
  so fullscreen windows are handled properly by the window manager.
**************************************************************************/
void setup_dialog(GtkWidget *shell, GtkWidget *parent)
{
  if (GUI_GTK_OPTION(dialogs_on_top) || GUI_GTK_OPTION(fullscreen)) {
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

/**********************************************************************//**
  Emit a dialog response.
**************************************************************************/
static void gui_dialog_response(struct gui_dialog *dlg, int response)
{
  if (dlg->response_callback) {
    (*dlg->response_callback)(dlg, response, dlg->user_data);
  }
}

/**********************************************************************//**
  Default dialog response handler. Destroys the dialog.
**************************************************************************/
static void gui_dialog_destroyed(struct gui_dialog *dlg, int response,
                                 gpointer data)
{
  gui_dialog_destroy(dlg);
}

/**********************************************************************//**
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

/**********************************************************************//**
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

/**********************************************************************//**
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

/**********************************************************************//**
  Allow the user to close a dialog using Escape or CTRL+W.
**************************************************************************/
static gboolean gui_dialog_key_press_handler(GtkWidget *w, GdkEventKey *ev,
                                             gpointer data)
{
  struct gui_dialog *dlg = data;

  if (ev->keyval == GDK_KEY_Escape
	|| ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_w)) {
    /* emit response signal. */
    gui_dialog_response(dlg, GTK_RESPONSE_DELETE_EVENT);
  }

  /* propagate event further. */
  return FALSE;
}

/**********************************************************************//**
  Resets tab colour on tab activation.
**************************************************************************/
static void gui_dialog_switch_page_handler(GtkNotebook *notebook,
                                           GtkWidget *page,
                                           guint num,
                                           struct gui_dialog *dlg)
{
  gint n;

  n = gtk_notebook_page_num(GTK_NOTEBOOK(dlg->v.tab.notebook), dlg->vbox);

  if (n == num) {
    gtk_style_context_remove_class(gtk_widget_get_style_context(dlg->v.tab.label),
                                   "alert");
  }
}

/**********************************************************************//**
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

/**********************************************************************//**
  Someone has clicked on a label in a notebook
**************************************************************************/
static gboolean click_on_tab_callback(GtkWidget *w,
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


/**********************************************************************//**
  Creates a new dialog. It will be a tab or a window depending on the
  current user setting of 'enable_tabs' gtk-gui option.
  Sets pdlg to point to the dialog once it is create, Zeroes pdlg on
  dialog destruction.
  user_data will be passed through response function
  check_top indicates if the layout deision should depend on the parent.
**************************************************************************/
void gui_dialog_new(struct gui_dialog **pdlg, GtkNotebook *notebook,
                    gpointer user_data, bool check_top)
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

  if (GUI_GTK_OPTION(enable_tabs)) {
    dlg->type = GUI_DIALOG_TAB;
  } else {
    dlg->type = GUI_DIALOG_WINDOW;
  }

  if (!gui_action) {
    gui_action = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);
  }
  dlg->gui_button = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

  vbox = gtk_grid_new();
  action_area = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(action_area), 4);
  gtk_grid_set_column_spacing(GTK_GRID(action_area), 4);
  if (GUI_GTK_OPTION(enable_tabs) &&
      (check_top && notebook != GTK_NOTEBOOK(top_notebook))
      && !GUI_GTK_OPTION(small_display_layout)) {
    /* We expect this to be short (as opposed to tall); maximise usable
     * height by putting buttons down the right hand side */
    gtk_orientable_set_orientation(GTK_ORIENTABLE(action_area),
                                   GTK_ORIENTATION_VERTICAL);
  } else {
    /* We expect this to be reasonably tall; maximise usable width by
     * putting buttons along the bottom */
    gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                   GTK_ORIENTATION_VERTICAL);
  }

  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(vbox), action_area);
  gtk_widget_show(action_area);

  gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
  gtk_container_set_border_width(GTK_CONTAINER(action_area), 2);

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
      gchar *buf;

      hbox = gtk_grid_new();

      label = gtk_label_new(NULL);
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
      gtk_widget_set_margin_start(label, 4);
      gtk_widget_set_margin_end(label, 4);
      gtk_widget_set_margin_top(label, 0);
      gtk_widget_set_margin_bottom(label, 0);
      gtk_container_add(GTK_CONTAINER(hbox), label);

      button = gtk_button_new();
      gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
      g_signal_connect_swapped(button, "clicked",
                               G_CALLBACK(gui_dialog_delete_tab_handler), dlg);

      buf = g_strdup_printf(_("Close Tab:\n%s"), _("Ctrl+W"));
      gtk_widget_set_tooltip_text(button, buf);
      g_free(buf);

      image = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
      gtk_widget_set_margin_start(image, 0);
      gtk_widget_set_margin_end(image, 0);
      gtk_widget_set_margin_top(image, 0);
      gtk_widget_set_margin_bottom(image, 0);
      gtk_button_set_image(GTK_BUTTON(button), image);

      gtk_container_add(GTK_CONTAINER(hbox), button);

      gtk_widget_show_all(hbox);

      event_box = gtk_event_box_new();
      gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
      gtk_container_add(GTK_CONTAINER(event_box), hbox);

      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, event_box);
      dlg->v.tab.handler_id =
        g_signal_connect(notebook, "switch-page",
                         G_CALLBACK(gui_dialog_switch_page_handler), dlg);
      dlg->v.tab.child = vbox;

      gtk_style_context_add_provider(gtk_widget_get_style_context(label),
                                     GTK_STYLE_PROVIDER(dlg_tab_provider),
                                     GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
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

/**********************************************************************//**
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

/**********************************************************************//**
  Places a button into a dialog, taking care of setting up signals, etc.
**************************************************************************/
static void gui_dialog_pack_button(struct gui_dialog *dlg, GtkWidget *button,
                                   int response)
{
  gint signal_id;

  fc_assert_ret(GTK_IS_BUTTON(button));

  g_object_set_data(G_OBJECT(button), "gui-dialog-response-data",
      GINT_TO_POINTER(response));

  if ((signal_id = g_signal_lookup("clicked", GTK_TYPE_BUTTON))) {
    GClosure *closure;

    closure = g_cclosure_new_object(G_CALLBACK(action_widget_activated),
	G_OBJECT(dlg->vbox));
    g_signal_connect_closure_by_id(button, signal_id, 0, closure, FALSE);
  }

  gtk_container_add(GTK_CONTAINER(dlg->action_area), button);
  gtk_size_group_add_widget(gui_action, button);
  gtk_size_group_add_widget(dlg->gui_button, button);
}

/**********************************************************************//**
  Adds a button to a dialog.
**************************************************************************/
GtkWidget *gui_dialog_add_button(struct gui_dialog *dlg,
                                 const char *icon_name,
                                 const char *text, int response)
{
  GtkWidget *button;

  button = icon_label_button_new(icon_name, text);
  gtk_widget_set_can_default(button, TRUE);
  gui_dialog_pack_button(dlg, button, response);

  return button;
}

/**********************************************************************//**
  Adds a widget to a dialog.
**************************************************************************/
GtkWidget *gui_dialog_add_widget(struct gui_dialog *dlg,
                                 GtkWidget *widget)
{
  gtk_container_add(GTK_CONTAINER(dlg->action_area), widget);
  gtk_size_group_add_widget(gui_action, widget);

  return widget;
}

/**********************************************************************//**
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

/**********************************************************************//**
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

/**********************************************************************//**
  Get the dialog's toplevel window.
**************************************************************************/
GtkWidget *gui_dialog_get_toplevel(struct gui_dialog *dlg)
{
  return gtk_widget_get_toplevel(dlg->vbox);
}

/**********************************************************************//**
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

/**********************************************************************//**
  Notify the user the dialog has changed.
**************************************************************************/
void gui_dialog_present(struct gui_dialog *dlg)
{
  fc_assert_ret(NULL != dlg);

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

        gtk_style_context_add_class(gtk_widget_get_style_context(label),
                                    "alert");
      }
    }
    break;
  }
}

/**********************************************************************//**
  Raise dialog to top.
**************************************************************************/
void gui_dialog_raise(struct gui_dialog *dlg)
{
  fc_assert_ret(NULL != dlg);

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

/**********************************************************************//**
  Alert the user to an important event.
**************************************************************************/
void gui_dialog_alert(struct gui_dialog *dlg)
{
  fc_assert_ret(NULL != dlg);

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

        gtk_style_context_add_class(gtk_widget_get_style_context(label),
                                    "alert");
      }
    }
    break;
  }
}

/**********************************************************************//**
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

/**********************************************************************//**
  Changes a dialog's title.
**************************************************************************/
void gui_dialog_set_title(struct gui_dialog *dlg, const char *title)
{
  if (dlg->title) {
    free(dlg->title);
  }
  dlg->title = fc_strdup(title);
  switch (dlg->type) {
  case GUI_DIALOG_WINDOW:
    gtk_window_set_title(GTK_WINDOW(dlg->v.window), title);
    break;
  case GUI_DIALOG_TAB:
    gtk_label_set_text_with_mnemonic(GTK_LABEL(dlg->v.tab.label), title);
    break;
  }
}

/**********************************************************************//**
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

/**********************************************************************//**
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

/**********************************************************************//**
  Set the response callback for a dialog.
**************************************************************************/
void gui_dialog_response_set_callback(struct gui_dialog *dlg,
    GUI_DIALOG_RESPONSE_FUN fun)
{
  dlg->response_callback = fun;
}

/**********************************************************************//**
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

/**********************************************************************//**
  Updates a gui font style.
**************************************************************************/
void gui_update_font(const char *font_name, const char *font_value)
{
  char *str;
  GtkCssProvider *provider;
  PangoFontDescription *desc;
  int size;
  const char *fam;
  const char *style;
  const char *weight;

  desc = pango_font_description_from_string(font_value);

  if (desc == NULL) {
    return;
  }

  fam = pango_font_description_get_family(desc);

  if (fam == NULL) {
    return;
  }

  if (pango_font_description_get_style(desc) == PANGO_STYLE_ITALIC) {
    style = "\n font-style: italic;";
  } else {
    style = "";
  }

  if (pango_font_description_get_weight(desc) >= 700) {
    weight = "\n font-weight: bold;";
  } else {
    weight = "";
  }

  size = pango_font_description_get_size(desc);

  if (size != 0) {
    str = g_strdup_printf("#Freeciv #%s { font-family: %s; font-size: %dpx;%s%s}",
                          font_name, fam, size / PANGO_SCALE, style, weight);
  } else {
    str = g_strdup_printf("#Freeciv #%s { font-family: %s;%s%s}",
                          font_name, fam, style, weight);
  }

  pango_font_description_free(desc);

  provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider),
    str, -1, NULL);
  gtk_style_context_add_provider_for_screen(
    gtk_widget_get_screen(toplevel), GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free(str);
}

/**********************************************************************//**
  Update a font option which is not attached to a widget.
**************************************************************************/
void gui_update_font_full(const char *font_name, const char *font_value,
                          PangoFontDescription **font_desc)
{
  PangoFontDescription *f_desc;

  gui_update_font(font_name, font_value);

  f_desc = pango_font_description_from_string(font_value);
  pango_font_description_free(*font_desc);

  *font_desc = f_desc;
}

/**********************************************************************//**
  Temporarily disable signal invocation of the given callback for the given
  GObject. Re-enable the signal with enable_gobject_callback.
**************************************************************************/
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

/**********************************************************************//**
  Re-enable a signal callback blocked by disable_gobject_callback.
**************************************************************************/
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

/**********************************************************************//**
  Convenience function to add a column to a GtkTreeView. Returns the added
  column, or NULL if an error occurred.
**************************************************************************/
GtkTreeViewColumn *add_treeview_column(GtkWidget *view, const char *title,
                                       GType gtype, int model_index)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  const char *attr;

  fc_assert_ret_val(view != NULL, NULL);
  fc_assert_ret_val(GTK_IS_TREE_VIEW(view), NULL);
  fc_assert_ret_val(title != NULL, NULL);

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

/**********************************************************************//**
  Prepare dialog tab style provider.
**************************************************************************/
void dlg_tab_provider_prepare(void)
{
  dlg_tab_provider = gtk_css_provider_new();

  gtk_css_provider_load_from_data(dlg_tab_provider,
                                  ".alert {\n"
                                  "color: rgba(255, 0, 0, 255);\n"
                                  "}\n",
                                  -1, NULL);
}
