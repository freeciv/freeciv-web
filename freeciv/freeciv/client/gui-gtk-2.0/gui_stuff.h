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
#ifndef FC__GUI_STUFF_H
#define FC__GUI_STUFF_H

#include <gtk/gtk.h>

#include "shared.h"

GtkWidget *gtk_stockbutton_new(const gchar *stock, const gchar *label_text);
void gtk_stockbutton_set_label(GtkWidget *button, const gchar *label_text);
void gtk_expose_now(GtkWidget *w);
void gtk_set_relative_position(GtkWidget *ref, GtkWidget *w, int px, int py);

void intl_slist(int n, const char **s, bool *done);

/* the standard GTK+ 2.0 API is braindamaged. this is slightly better! */

typedef struct
{
  GtkTreeModel *model;
  gboolean end;
  GtkTreeIter it;
} ITree;

#define TREE_ITER_PTR(x)	(&(x).it)

void itree_begin(GtkTreeModel *model, ITree *it);
gboolean itree_end(ITree *it);
void itree_next(ITree *it);
void itree_get(ITree *it, ...);
void itree_set(ITree *it, ...);

void tstore_append(GtkTreeStore *store, ITree *it, ITree *parent);

gboolean itree_is_selected(GtkTreeSelection *selection, ITree *it);
void itree_select(GtkTreeSelection *selection, ITree *it);
void itree_unselect(GtkTreeSelection *selection, ITree *it);

gint gtk_tree_selection_get_row(GtkTreeSelection *selection);
void gtk_tree_view_focus(GtkTreeView *view);
void setup_dialog(GtkWidget *shell, GtkWidget *parent);
GtkTreeViewColumn *add_treeview_column(GtkWidget *view, const char *title,
                                       GType gtype, int model_index);


enum gui_dialog_type {
  GUI_DIALOG_WINDOW,
  GUI_DIALOG_TAB
};

struct gui_dialog;

typedef void (*GUI_DIALOG_RESPONSE_FUN)(struct gui_dialog *, int, gpointer);

struct gui_dialog
{
  /* public. */
  GtkWidget *vbox;
  GtkWidget *action_area;

  /* private. */
  char* title;
  enum gui_dialog_type type;
  int id;
  int return_dialog_id;
  
  int default_width;
  int default_height;

  union {
    GtkWidget *window;
    struct {
      GtkWidget *label;
      GtkWidget *notebook;
      gulong handler_id;
      GtkWidget *child;
    } tab;
  } v;

  struct gui_dialog **source;

  GUI_DIALOG_RESPONSE_FUN response_callback;
  gpointer user_data;

  GtkSizeGroup *gui_button;
};

void gui_dialog_new(struct gui_dialog **pdlg, GtkNotebook *notebook, gpointer user_data);
void gui_dialog_set_default_response(struct gui_dialog *dlg, int response);
GtkWidget *gui_dialog_add_button(struct gui_dialog *dlg,
    const char *text, int response);
GtkWidget *gui_dialog_add_stockbutton(struct gui_dialog *dlg,
    const char *stock, const char *text, int response);
GtkWidget *gui_dialog_add_widget(struct gui_dialog *dlg,
				 GtkWidget *widget);
void gui_dialog_set_default_size(struct gui_dialog *dlg,
    int width, int height);
void gui_dialog_set_title(struct gui_dialog *dlg, const char *title);
void gui_dialog_set_response_sensitive(struct gui_dialog *dlg,
    int response, bool setting);
void gui_dialog_show_all(struct gui_dialog *dlg);
void gui_dialog_present(struct gui_dialog *dlg);
void gui_dialog_raise(struct gui_dialog *dlg);
void gui_dialog_alert(struct gui_dialog *dlg);
void gui_dialog_destroy(struct gui_dialog *dlg);
void gui_dialog_destroy_all(void);
GtkWidget *gui_dialog_get_toplevel(struct gui_dialog *dlg);
void gui_dialog_response_set_callback(struct gui_dialog *dlg,
    GUI_DIALOG_RESPONSE_FUN fun);
void gui_dialog_set_return_dialog(struct gui_dialog *dlg,
                                  struct gui_dialog *return_dialog);

struct client_option;
void gui_update_font_from_option(struct client_option *o);

void disable_gobject_callback(GObject *obj, GCallback cb);
void enable_gobject_callback(GObject *obj, GCallback cb);

#endif  /* FC__GUI_STUFF_H */
