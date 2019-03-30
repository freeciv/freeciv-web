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

#include <stdlib.h>

#include <gtk/gtk.h>

/* utility */
#include "fc_cmdline.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"

/* common */
#include "version.h"

/* modinst */
#include "download.h"
#include "mpcmdline.h"
#include "mpdb.h"

#include "modinst.h"

static GtkWidget *statusbar;
static GtkWidget *progressbar;
static GtkWidget *main_list;
static GtkListStore *main_store;
static GtkWidget *URL_input;
static gboolean downloading = FALSE;

struct fcmp_params fcmp = {
  .list_url = MODPACK_LIST_URL,
  .inst_prefix = NULL,
  .autoinstall = NULL
};

static gboolean quit_dialog_callback(void);

#define ML_COL_NAME    0
#define ML_COL_VER     1
#define ML_COL_INST    2
#define ML_COL_TYPE    3
#define ML_COL_SUBTYPE 4
#define ML_COL_LIC     5 
#define ML_COL_URL     6

#define ML_COL_COUNT   7

#define ML_TYPE        7
#define ML_NOTES       8
#define ML_STORE_SIZE  9

/**********************************************************************//**
  freeciv-modpack quit
**************************************************************************/
static void modinst_quit(void)
{
  close_mpdbs();

  fcmp_deinit();
  cmdline_option_values_free();

  exit(EXIT_SUCCESS);
}

/**********************************************************************//**
  This is the response callback for the dialog with the message:
  Are you sure you want to quit?
**************************************************************************/
static void quit_dialog_response(GtkWidget *dialog, gint response)
{
  gtk_widget_destroy(dialog);
  if (response == GTK_RESPONSE_YES) {
    modinst_quit();
  }
}

/**********************************************************************//**
  Popups the quit dialog if download in progress
**************************************************************************/
static gboolean quit_dialog_callback(void)
{
  if (downloading) {
    /* Download in progress. Confirm quit from user. */
    static GtkWidget *dialog;

    if (!dialog) {
      dialog = gtk_message_dialog_new(NULL,
                                      0,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_YES_NO,
      _("Modpack installation in progress.\nAre you sure you want to quit?"));

      gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

      g_signal_connect(dialog, "response", 
                       G_CALLBACK(quit_dialog_response), NULL);
      g_signal_connect(dialog, "destroy",
                       G_CALLBACK(gtk_widget_destroyed), &dialog);
    }

    gtk_window_present(GTK_WINDOW(dialog));

  } else {
    /* User loses no work by quitting, so let's not annoy him/her
     * with confirmation dialog. */
    modinst_quit();
  }

  /* Stop emission of event. */
  return TRUE;
}

/**********************************************************************//**
  Progress indications from downloader
**************************************************************************/
static void msg_callback(const char *msg)
{
  log_verbose("%s", msg);
  gtk_label_set_text(GTK_LABEL(statusbar), msg);
}

/**********************************************************************//**
  Progress indications from downloader
**************************************************************************/
static void pbar_callback(int downloaded, int max)
{
  /* Control file already downloaded */
  double fraction = (double) downloaded / (max);

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar), fraction);
}

struct msg_data {
  const char *msg;
};

/**********************************************************************//**
  Main thread handling of message sent from downloader thread.
**************************************************************************/
static gboolean msg_main_thread(gpointer user_data)
{
  struct msg_data *data = (struct msg_data *)user_data;

  msg_callback(data->msg);

  FC_FREE(data);

  return G_SOURCE_REMOVE;
}

/**********************************************************************//**
  Downloader thread message callback.
**************************************************************************/
static void msg_dl_thread(const char *msg)
{
  struct msg_data *data = fc_malloc(sizeof(*data));

  data->msg = msg;

  gdk_threads_add_idle(msg_main_thread, data);
}

struct pbar_data {
  int current;
  int max;
};

/**********************************************************************//**
  Main thread handling of progressbar update sent from downloader thread.
**************************************************************************/
static gboolean pbar_main_thread(gpointer user_data)
{
  struct pbar_data *data = (struct pbar_data *)user_data;

  pbar_callback(data->current, data->max);

  FC_FREE(data);

  return G_SOURCE_REMOVE;
}

/**********************************************************************//**
  Downloader thread progress bar callback.
**************************************************************************/
static void pbar_dl_thread(int current, int max)
{
  struct pbar_data *data = fc_malloc(sizeof(*data));

  data->current = current;
  data->max = max;

  gdk_threads_add_idle(pbar_main_thread, data);
}

/**********************************************************************//**
  Main thread handling of versionlist update requested by downloader thread
**************************************************************************/
static gboolean versionlist_update_main_thread(gpointer user_data)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(main_store), &iter)) {
    do {
      const char *name_str;
      int type_int;
      const char *new_inst;
      enum modpack_type type;

      gtk_tree_model_get(GTK_TREE_MODEL(main_store), &iter,
                         ML_COL_NAME, &name_str,
                         ML_TYPE, &type_int,
                         -1);

     type = type_int;

     new_inst = mpdb_installed_version(name_str, type);

     if (new_inst == NULL) {
       new_inst = _("Not installed");
     }

     gtk_list_store_set(main_store, &iter,
                        ML_COL_INST, new_inst,
                        -1);

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(main_store), &iter));
  }

  return G_SOURCE_REMOVE;
}

/**********************************************************************//**
  Downloader thread requests versionlist update.
**************************************************************************/
static void versionlist_update_dl_thread(void)
{
  gdk_threads_add_idle(versionlist_update_main_thread, NULL);
}

/**********************************************************************//**
  Entry point for downloader thread
**************************************************************************/
static gpointer download_thread(gpointer data)
{
  const char *errmsg;

  errmsg = download_modpack(data, &fcmp, msg_dl_thread, pbar_dl_thread);

  if (errmsg == NULL) {
    msg_dl_thread(_("Ready"));
  } else {
    msg_dl_thread(errmsg);
  }

  free(data);

  versionlist_update_dl_thread();

  downloading = FALSE;

  return NULL;
}

/**********************************************************************//**
  Download modpack, display error message dialogs
**************************************************************************/
static void gui_download_modpack(const char *URL)
{
  GThread *downloader;
  char *URLbuf;

  if (downloading) {
    gtk_label_set_text(GTK_LABEL(statusbar),
                       _("Another download already active"));
    return;
  }

  downloading = TRUE;

  URLbuf = fc_malloc(strlen(URL) + 1);

  strcpy(URLbuf, URL);

  downloader = g_thread_new("Downloader", download_thread, URLbuf);
  if (downloader == NULL) {
    gtk_label_set_text(GTK_LABEL(statusbar),
                       _("Failed to start downloader"));
    free(URLbuf);
  } else {
    g_thread_unref(downloader);
  }
}

/**********************************************************************//**
  Install modpack button clicked
**************************************************************************/
static void install_clicked(GtkWidget *w, gpointer data)
{
  GtkEntry *URL_in = data;
  const char *URL = gtk_entry_get_text(URL_in);

  gui_download_modpack(URL);
}

/**********************************************************************//**
  URL entered
**************************************************************************/
static void URL_return(GtkEntry *w, gpointer data)
{
  const char *URL;

  URL = gtk_entry_get_text(w);
  gui_download_modpack(URL);
}

/**********************************************************************//**
  Callback for getting main list row tooltip
**************************************************************************/
static gboolean query_main_list_tooltip_cb(GtkWidget *widget,
                                           gint x, gint y,
                                           gboolean keyboard_tip,
                                           GtkTooltip *tooltip,
                                           gpointer data)
{
  GtkTreeIter iter;
  GtkTreeView *tree_view = GTK_TREE_VIEW(widget);
  GtkTreeModel *model;
  const char *notes;

  if (!gtk_tree_view_get_tooltip_context(tree_view, &x, &y,
                                         keyboard_tip,
                                         &model, NULL, &iter)) {
    return FALSE;
  }

  gtk_tree_model_get(model, &iter,
                     ML_NOTES, &notes,
                     -1);

  if (notes != NULL) {
    gtk_tooltip_set_markup(tooltip, notes);

    return TRUE;
  }

  return FALSE;
}

/**********************************************************************//**
  Build main modpack list view
**************************************************************************/
static void setup_modpack_list(const char *name, const char *URL,
                               const char *version, const char *license,
                               enum modpack_type type, const char *subtype,
                               const char *notes)
{
  GtkTreeIter iter;
  const char *type_str;
  const char *lic_str;
  const char *inst_str;

  if (modpack_type_is_valid(type)) {
    type_str = _(modpack_type_name(type));
  } else {
    /* TRANS: Unknown modpack type */
    type_str = _("?");
  }

  if (license != NULL) {
    lic_str = license;
  } else {
    /* TRANS: License of modpack is not known */
    lic_str = Q_("?license:Unknown");
  }

  inst_str = mpdb_installed_version(name, type);
  if (inst_str == NULL) {
    inst_str = _("Not installed");
  }

  gtk_list_store_append(main_store, &iter);
  gtk_list_store_set(main_store, &iter,
                     ML_COL_NAME, name,
                     ML_COL_VER, version,
                     ML_COL_INST, inst_str,
                     ML_COL_TYPE, type_str,
                     ML_COL_SUBTYPE, subtype,
                     ML_COL_LIC, lic_str,
                     ML_COL_URL, URL,
                     ML_TYPE, type,
                     ML_NOTES, notes,
                     -1);
}

/**********************************************************************//**
  Callback called when entry from main modpack list selected
**************************************************************************/
static void select_from_list(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  const char *URL;

  if (!gtk_tree_selection_get_selected(select, &model, &it)) {
    return;
  }

  gtk_tree_model_get(model, &it, ML_COL_URL, &URL, -1);

  gtk_entry_set_text(GTK_ENTRY(URL_input), URL);
}

/**********************************************************************//**
  Build widgets
**************************************************************************/
static void modinst_setup_widgets(GtkWidget *toplevel)
{
  GtkWidget *mbox, *Ubox;
  GtkWidget *version_label;
  GtkWidget *install_button, *install_label;
  GtkWidget *URL_label;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  const char *errmsg;
  char verbuf[2048];
  const char *rev_ver;

  mbox = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(mbox),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing(GTK_GRID(mbox), 4);

  rev_ver = fc_git_revision();

  if (rev_ver == NULL) {
    fc_snprintf(verbuf, sizeof(verbuf), "%s%s", word_version(), VERSION_STRING);
  } else {
    fc_snprintf(verbuf, sizeof(verbuf), _("%s%s\ncommit: %s"),
                word_version(), VERSION_STRING, rev_ver);
  }

  version_label = gtk_label_new(verbuf);

  main_list = gtk_tree_view_new();
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(main_list),
                                              ML_COL_NAME,      
                                              _("Name"), renderer, "text", 0,
                                              NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(main_list),
                                              ML_COL_VER,
                                              _("Version"), renderer, "text", 1,
                                              NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(main_list),
                                              ML_COL_INST,
                                              _("Installed"), renderer, "text", 2,
                                              NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(main_list),
                                              ML_COL_TYPE,
                                              Q_("?modpack:Type"),
                                              renderer, "text", 3,
                                              NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(main_list),
                                              ML_COL_SUBTYPE,
                                              _("Subtype"),
                                              renderer, "text", 4,
                                              NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(main_list),
                                              ML_COL_LIC,
                                              /* TRANS: noun */
                                              _("License"), renderer, "text", 5,
                                              NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(main_list),
                                              ML_COL_URL,
                                              _("URL"), renderer, "text", 6,
                                              NULL);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(main_list));
  g_signal_connect(selection, "changed", G_CALLBACK(select_from_list), NULL);

  install_button = gtk_button_new();
  install_label = gtk_label_new(_("Install modpack"));
  gtk_label_set_mnemonic_widget(GTK_LABEL(install_label), install_button);
  g_object_set_data(G_OBJECT(install_button), "label", install_label);
  gtk_container_add(GTK_CONTAINER(install_button), install_label);

  Ubox = gtk_grid_new();
  gtk_widget_set_halign(Ubox, GTK_ALIGN_CENTER);
  gtk_grid_set_column_spacing(GTK_GRID(Ubox), 4);
  URL_label = gtk_label_new_with_mnemonic(_("Modpack URL"));

  URL_input = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(URL_input),
                            strlen(EXAMPLE_URL));
  gtk_entry_set_text(GTK_ENTRY(URL_input), DEFAULT_URL_START);
  g_signal_connect(URL_input, "activate",
		   G_CALLBACK(URL_return), NULL);

  g_signal_connect(install_button, "clicked",
                   G_CALLBACK(install_clicked), URL_input);

  gtk_container_add(GTK_CONTAINER(Ubox), URL_label);
  gtk_container_add(GTK_CONTAINER(Ubox), URL_input);

  progressbar = gtk_progress_bar_new();

  statusbar = gtk_label_new(_("Select modpack to install"));

  gtk_widget_set_hexpand(main_list, TRUE);
  gtk_widget_set_vexpand(main_list, TRUE);

  gtk_container_add(GTK_CONTAINER(mbox), version_label);
  gtk_container_add(GTK_CONTAINER(mbox), main_list);
  gtk_container_add(GTK_CONTAINER(mbox), Ubox);
  gtk_container_add(GTK_CONTAINER(mbox), install_button);
  gtk_container_add(GTK_CONTAINER(mbox), progressbar);
  gtk_container_add(GTK_CONTAINER(mbox), statusbar);

  gtk_container_add(GTK_CONTAINER(toplevel), mbox);

  main_store = gtk_list_store_new((ML_STORE_SIZE), G_TYPE_STRING, G_TYPE_STRING,
                                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
                                  G_TYPE_STRING);
  errmsg = download_modpack_list(&fcmp, setup_modpack_list, msg_callback);
  gtk_tree_view_set_model(GTK_TREE_VIEW(main_list), GTK_TREE_MODEL(main_store));

  g_object_set(main_list, "has-tooltip", TRUE, NULL);
  g_signal_connect(main_list, "query-tooltip",
                   G_CALLBACK(query_main_list_tooltip_cb), NULL);

  g_object_unref(main_store);

  if (errmsg != NULL) {
    gtk_label_set_text(GTK_LABEL(statusbar), errmsg);
  }
}

/**********************************************************************//**
  Entry point of the freeciv-modpack program
**************************************************************************/
int main(int argc, char *argv[])
{
  GtkWidget *toplevel;
  int ui_options;

  fcmp_init();

  /* This modifies argv! */
  ui_options = fcmp_parse_cmdline(argc, argv);

  if (ui_options != -1) {
    int i;

    for (i = 1; i <= ui_options; i++) {
      if (is_option("--help", argv[i])) {
        fc_fprintf(stderr,
             _("This modpack installer accepts the standard Gtk command-line options\n"
               "after '--'. See the Gtk documentation.\n\n"));

        /* TRANS: No full stop after the URL, could cause confusion. */
        fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);

        ui_options = -1;
      }
    }
  }

  if (ui_options != -1) {

    load_install_info_lists(&fcmp);

    /* Process GTK arguments */
    gtk_init(&ui_options, &argv);

    toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_widget_realize(toplevel);
    gtk_widget_set_name(toplevel, "Freeciv-modpack");
    gtk_window_set_title(GTK_WINDOW(toplevel),
                         _("Freeciv modpack installer (gtk3)"));

    /* Keep the icon of the executable on Windows */
#ifndef FREECIV_MSWINDOWS
    {
      /* Unlike main client, this only works if installed. Ignore any
       * errors loading the icon. */
      GError *err;

      (void) gtk_window_set_icon_from_file(GTK_WINDOW(toplevel), MPICON_PATH,
                                           &err);
    }
#endif /* FREECIV_MSWINDOWS */

    g_signal_connect(toplevel, "delete_event",
                     G_CALLBACK(quit_dialog_callback), NULL);

    modinst_setup_widgets(toplevel);

    gtk_widget_show_all(toplevel);

    if (fcmp.autoinstall != NULL) {
      gui_download_modpack(fcmp.autoinstall);
    }

    gtk_main();

    close_mpdbs();
  }

  fcmp_deinit();
  cmdline_option_values_free();

  return EXIT_SUCCESS;
}
