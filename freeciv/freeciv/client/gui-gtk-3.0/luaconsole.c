/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <gdk/gdkkeysyms.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"

/* common */
#include "featured_text.h"
#include "game.h"

/* client */
#include "options.h"

/* client/gui-gtk-3.0 */
#include "chatline.h"
#include "gui_main.h"
#include "gui_stuff.h"

/* client/luascript */
#include "script_client.h"

#include "luaconsole.h"

enum luaconsole_res {
  LUACONSOLE_RES_OPEN
};

#define MAX_LUACONSOLE_HISTORY 20

struct luaconsole_data {
  struct gui_dialog *shell;
  GtkTextBuffer *message_buffer;
  GtkTextView *message_area;
  GtkWidget *entry;

  struct genlist *history_list;
  int history_pos;
};

static struct luaconsole_data *luaconsole = NULL;


static struct luaconsole_data *luaconsole_dialog_get(void);
static void luaconsole_dialog_create(struct luaconsole_data *pdialog);
static void luaconsole_dialog_refresh(struct luaconsole_data *pdialog);
static void luaconsole_dialog_destroy(struct luaconsole_data *pdialog);

static void luaconsole_dialog_area_size_allocate(GtkWidget *widget,
                                                 GtkAllocation *allocation,
                                                 gpointer data);
static void luaconsole_dialog_scroll_to_bottom(void);

static void luaconsole_input_return(GtkEntry *w, gpointer data);
static gboolean luaconsole_input_handler(GtkWidget *w, GdkEventKey *ev);

static void luaconsole_response_callback(struct gui_dialog *pgui_dialog,
                                         int response, gpointer data);
static void luaconsole_load_file_popup(void);
static void luaconsole_load_file_callback(GtkWidget *widget, gint response,
                                          gpointer data);

/*************************************************************************//**
  Create a lua console.
*****************************************************************************/
void luaconsole_dialog_init(void)
{
  fc_assert_ret(luaconsole == NULL);

  /* Create a container for the dialog. */
  luaconsole = fc_calloc(1, sizeof(*luaconsole));
  luaconsole->message_buffer = gtk_text_buffer_new(NULL);
  luaconsole->shell = NULL;

  luaconsole->history_list = genlist_new();
  luaconsole->history_pos = -1;

  luaconsole_welcome_message();
}

/*************************************************************************//**
  Free a script lua console.
*****************************************************************************/
void luaconsole_dialog_done(void)
{
  if (luaconsole != NULL) {
    if (luaconsole->history_list) {
        genlist_destroy(luaconsole->history_list);
    }

    luaconsole_dialog_popdown();
    free(luaconsole);

    luaconsole = NULL;
  }
}

/*************************************************************************//**
  Get the data for the lua console.
*****************************************************************************/
static struct luaconsole_data *luaconsole_dialog_get(void)
{
  return luaconsole;
}

/*************************************************************************//**
  Popup the lua console inside the main-window, and optionally raise it.
*****************************************************************************/
void luaconsole_dialog_popup(bool raise)
{
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret(pdialog);
  if (pdialog->shell == NULL) {
    luaconsole_dialog_create(pdialog);
  }

  gui_dialog_present(pdialog->shell);
  if (raise) {
    gui_dialog_raise(pdialog->shell);
  }
}

/*************************************************************************//**
  Closes the lua console; the content is saved till the client is done.
*****************************************************************************/
void luaconsole_dialog_popdown(void)
{
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret(pdialog);
  if (pdialog->shell == NULL) {
    luaconsole_dialog_destroy(pdialog);
  }
}

/*************************************************************************//**
  Return TRUE iff the lua console is open.
*****************************************************************************/
bool luaconsole_dialog_is_open(void)
{
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret_val(pdialog, FALSE);

  return (NULL != pdialog->shell);
}

/*************************************************************************//**
  Update the lua console.
*****************************************************************************/
void real_luaconsole_dialog_update(void)
{
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret(pdialog);

  if (NULL != pdialog->shell) {
    luaconsole_dialog_refresh(pdialog);
  }
}

/*************************************************************************//**
  Initialize a lua console.
*****************************************************************************/
static void luaconsole_dialog_create(struct luaconsole_data *pdialog)
{
  GtkWidget *entry, *box, *vbox, *sw, *text, *notebook;

  fc_assert_ret(NULL != pdialog);

  if (GUI_GTK_OPTION(message_chat_location) == GUI_GTK_MSGCHAT_SPLIT) {
    notebook = right_notebook;
  } else {
    notebook = bottom_notebook;
  }

  gui_dialog_new(&pdialog->shell, GTK_NOTEBOOK(notebook), pdialog, TRUE);
  gui_dialog_set_title(pdialog->shell, _("Client Lua Console"));

  box = pdialog->shell->vbox;

  vbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_container_add(GTK_CONTAINER(box), vbox);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  text = gtk_text_view_new_with_buffer(pdialog->message_buffer);
  gtk_widget_set_hexpand(text, TRUE);
  gtk_widget_set_vexpand(text, TRUE);
  set_message_buffer_view_link_handlers(text);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_container_add(GTK_CONTAINER(sw), text);
  g_signal_connect(text, "size-allocate",
                   G_CALLBACK(luaconsole_dialog_area_size_allocate), NULL);

  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_widget_realize(text);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 5);

  pdialog->message_area = GTK_TEXT_VIEW(text);
  g_signal_connect(text, "destroy", G_CALLBACK(gtk_widget_destroyed),
                   &pdialog->message_area);

  /* The lua console input line. */
  entry = gtk_entry_new();
  g_object_set(entry, "margin", 2, NULL);
  gtk_container_add(GTK_CONTAINER(vbox), entry);
  g_signal_connect(entry, "activate", G_CALLBACK(luaconsole_input_return),
                   NULL);
  g_signal_connect(entry, "key_press_event",
                   G_CALLBACK(luaconsole_input_handler), NULL);
  pdialog->entry = entry;
  g_signal_connect(entry, "destroy", G_CALLBACK(gtk_widget_destroyed),
                   &pdialog->entry);

  /* Load lua script command button. */
  gui_dialog_add_button(pdialog->shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  gui_dialog_add_stockbutton(pdialog->shell, GTK_STOCK_OPEN,
                             _("Load Lua Script"), LUACONSOLE_RES_OPEN);
  gui_dialog_response_set_callback(pdialog->shell,
                                   luaconsole_response_callback);

  luaconsole_dialog_refresh(pdialog);
  gui_dialog_show_all(pdialog->shell);
}

/*************************************************************************//**
  Called when the return key is pressed.
*****************************************************************************/
static void luaconsole_input_return(GtkEntry *w, gpointer data)
{
  const char *theinput;
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret(pdialog);
  fc_assert_ret(pdialog->history_list);

  theinput = gtk_entry_get_text(w);

  if (*theinput) {
    luaconsole_printf(ftc_luaconsole_input, "(input)> %s", theinput);
    script_client_do_string(theinput);

    if (genlist_size(pdialog->history_list) >= MAX_LUACONSOLE_HISTORY) {
      void *history_data;

      history_data = genlist_get(pdialog->history_list, -1);
      genlist_remove(pdialog->history_list, history_data);
      free(history_data);
    }

    genlist_prepend(pdialog->history_list, fc_strdup(theinput));
    pdialog->history_pos = -1;
  }

  gtk_entry_set_text(w, "");
}

/*************************************************************************//**
  Dialog response callback.
*****************************************************************************/
static void luaconsole_response_callback(struct gui_dialog *pgui_dialog,
                                         int response, gpointer data)
{
  switch (response) {
  case LUACONSOLE_RES_OPEN:
    break;
  default:
    gui_dialog_destroy(pgui_dialog);
    return;
  }

  switch (response) {
  case LUACONSOLE_RES_OPEN:
    luaconsole_load_file_popup();
    break;
  }
}

/*************************************************************************//**
  Create a file selector for loading a lua file..
*****************************************************************************/
static void luaconsole_load_file_popup(void)
{
  GtkWidget *filesel;

  /* Create the selector */
  filesel = gtk_file_chooser_dialog_new("Load Lua file", GTK_WINDOW(toplevel),
                GTK_FILE_CHOOSER_ACTION_OPEN,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                NULL);
  setup_dialog(filesel, toplevel);
  gtk_window_set_position(GTK_WINDOW(filesel), GTK_WIN_POS_MOUSE);

  g_signal_connect(filesel, "response",
                   G_CALLBACK(luaconsole_load_file_callback), NULL);

  /* Display that dialog */
  gtk_window_present(GTK_WINDOW(filesel));
}

/*************************************************************************//**
  Callback for luaconsole_load_file_popup().
*****************************************************************************/
static void luaconsole_load_file_callback(GtkWidget *widget, gint response,
                                          gpointer data)
{
  if (response == GTK_RESPONSE_OK) {
    gchar *filename = g_filename_to_utf8(gtk_file_chooser_get_filename
                                         (GTK_FILE_CHOOSER(widget)),
                                         -1, NULL, NULL, NULL);

    if (NULL != filename) {
      luaconsole_printf(ftc_luaconsole_input, "(file)> %s", filename);
      script_client_do_file(filename);
      g_free(filename);
    }
  }
  gtk_widget_destroy(widget);
}

/*************************************************************************//**
  Called when a key is pressed.
*****************************************************************************/
static gboolean luaconsole_input_handler(GtkWidget *w, GdkEventKey *ev)
{
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret_val(pdialog, FALSE);
  fc_assert_ret_val(pdialog->history_list, FALSE);

  switch (ev->keyval) {
  case GDK_KEY_Up:
    if (pdialog->history_pos < genlist_size(pdialog->history_list) - 1) {
      gtk_entry_set_text(GTK_ENTRY(w), genlist_get(pdialog->history_list,
                                                   ++pdialog->history_pos));
      gtk_editable_set_position(GTK_EDITABLE(w), -1);
    }
    return TRUE;

  case GDK_KEY_Down:
    if (pdialog->history_pos >= 0) {
      pdialog->history_pos--;
    }

    if (pdialog->history_pos >= 0) {
      gtk_entry_set_text(GTK_ENTRY(w), genlist_get(pdialog->history_list,
                                                   pdialog->history_pos));
    } else {
      gtk_entry_set_text(GTK_ENTRY(w), "");
    }
    gtk_editable_set_position(GTK_EDITABLE(w), -1);
    return TRUE;

  default:
    break;
  }

  return FALSE;
}

/*************************************************************************//**
  When the luaconsole text view is resized, scroll it to the bottom. This
  prevents users from accidentally missing messages when the chatline
  gets scrolled up a small amount and stops scrolling down automatically.
*****************************************************************************/
static void luaconsole_dialog_area_size_allocate(GtkWidget *widget,
                                                 GtkAllocation *allocation,
                                                 gpointer data)
{
  static int old_width = 0, old_height = 0;
  if (allocation->width != old_width
      || allocation->height != old_height) {
    luaconsole_dialog_scroll_to_bottom();
    old_width = allocation->width;
    old_height = allocation->height;
  }
}

/*************************************************************************//**
  Scrolls the luaconsole all the way to the bottom.
  If delayed is TRUE, it will be done in a idle_callback.

  Modified copy of chatline_scroll_to_bottom().
*****************************************************************************/
static void luaconsole_dialog_scroll_to_bottom(void)
{
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret(pdialog);

  if (pdialog->shell) {
    GtkTextIter end;

    fc_assert_ret(NULL != pdialog->message_buffer);
    fc_assert_ret(NULL != pdialog->message_area);

    gtk_text_buffer_get_end_iter(pdialog->message_buffer, &end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(pdialog->message_area),
                                 &end, 0.0, TRUE, 1.0, 0.0);
  }
}

/*************************************************************************//**
  Refresh a lua console.
*****************************************************************************/
static void luaconsole_dialog_refresh(struct luaconsole_data *pdialog)
{
  fc_assert_ret(NULL != pdialog);
}

/*************************************************************************//**
  Closes a lua console.
*****************************************************************************/
static void luaconsole_dialog_destroy(struct luaconsole_data *pdialog)
{
  fc_assert_ret(NULL != pdialog);

  if (pdialog->shell) {
    gui_dialog_destroy(pdialog->shell);
    fc_assert(NULL == pdialog->shell);
  }
  fc_assert(NULL == pdialog->message_area);
  fc_assert(NULL == pdialog->entry);
}

/*************************************************************************//**
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
*****************************************************************************/
void real_luaconsole_append(const char *astring,
                            const struct text_tag_list *tags)
{
  GtkTextBuffer *buf;
  GtkTextIter iter;
  GtkTextMark *mark;
  ft_offset_t text_start_offset;
  struct luaconsole_data *pdialog = luaconsole_dialog_get();

  fc_assert_ret(pdialog);

  buf = pdialog->message_buffer;
  gtk_text_buffer_get_end_iter(buf, &iter);
  gtk_text_buffer_insert(buf, &iter, "\n", -1);
  mark = gtk_text_buffer_create_mark(buf, NULL, &iter, TRUE);

  if (GUI_GTK_OPTION(show_chat_message_time)) {
    char timebuf[64];
    time_t now;
    struct tm *now_tm;

    now = time(NULL);
    now_tm = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "[%H:%M:%S] ", now_tm);
    gtk_text_buffer_insert(buf, &iter, timebuf, -1);
  }

  text_start_offset = gtk_text_iter_get_offset(&iter);
  gtk_text_buffer_insert(buf, &iter, astring, -1);
  text_tag_list_iterate(tags, ptag) {
    apply_text_tag(ptag, buf, text_start_offset, astring);
  } text_tag_list_iterate_end;

  if (pdialog->message_area) {
    scroll_if_necessary(GTK_TEXT_VIEW(pdialog->message_area), mark);
  }
  gtk_text_buffer_delete_mark(buf, mark);
}
