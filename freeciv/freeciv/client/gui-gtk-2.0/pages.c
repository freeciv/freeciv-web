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

#include <stdio.h>
#include <time.h>

#include <sys/stat.h>

#include <gtk/gtk.h>

#ifdef GGZ_GTK
#  include <ggz-gtk.h>
#endif

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "dataio.h"
#include "game.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "packhand.h"
#include "servers.h"

/* gui-gtk-2.0 */
#include "chatline.h"
#include "connectdlg.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "plrdlg.h"             /* get_flag() */
#include "repodlgs.h"
#include "voteinfo_bar.h"

#include "pages.h"

GtkWidget *start_message_area;

static GtkWidget *start_options_table;
GtkWidget *take_button, *ready_button, *nation_button;

static GtkWidget *scenario_description;

static GtkListStore *load_store, *scenario_store,
  *nation_store, *meta_store, *lan_store; 

static GtkListStore *server_playerlist_store;
static GtkWidget *server_playerlist_view;

static GtkTreeSelection *load_selection, *scenario_selection,
  *nation_selection, *meta_selection, *lan_selection;

/* This is the current page, or the next we will switch to when the idle
 * callback will be called. */
static enum client_pages next_page = -1;

static void set_page_callback(GtkWidget *w, gpointer data);
static void update_nation_page(struct packet_game_load *packet);

static guint meta_scan_timer, lan_scan_timer;
static struct server_scan *meta_scan, *lan_scan;

static GtkWidget *statusbar, *statusbar_frame;
static GQueue *statusbar_queue;
static guint statusbar_timer = 0;

static GtkWidget *ruleset_combo;

static bool save_scenario = FALSE;

/**************************************************************************
  spawn a server, if there isn't one, using the default settings.
**************************************************************************/
static void start_new_game_callback(GtkWidget *w, gpointer data)
{
  if (is_server_running() || client_start_server()) {
    /* saved settings are sent in client/options.c load_settable_options() */
  }
}

/**************************************************************************
  go to the scenario page, spawning a server,
**************************************************************************/
static void start_scenario_callback(GtkWidget *w, gpointer data)
{
  set_client_page(PAGE_SCENARIO);
  client_start_server();
}

/**************************************************************************
  go to the load page, spawning a server.
**************************************************************************/
static void load_saved_game_callback(GtkWidget *w, gpointer data)
{
  set_client_page(PAGE_LOAD);
  client_start_server();
}

/**************************************************************************
  cancel, by terminating the connection and going back to main page.
**************************************************************************/
static void main_callback(GtkWidget *w, gpointer data)
{
  if (client.conn.used) {
    disconnect_from_server();
  } else {
    set_client_page(in_ggz ? PAGE_GGZ : PAGE_MAIN);
  }
}

/**************************************************************************
  this is called whenever the intro graphic needs a graphics refresh.
**************************************************************************/
static gboolean intro_expose(GtkWidget *w, GdkEventExpose *ev)
{
  static PangoLayout *layout;
  static int width, height;

  if (!layout) {
    char msgbuf[128];

    layout = pango_layout_new(gdk_pango_context_get());
    pango_layout_set_font_description(layout, main_font);

    my_snprintf(msgbuf, sizeof(msgbuf), "%s%s",
	word_version(), VERSION_STRING);
    pango_layout_set_text(layout, msgbuf, -1);

    pango_layout_get_pixel_size(layout, &width, &height);
  }
 
  gtk_draw_shadowed_string(w->window,
      w->style->black_gc,
      w->style->white_gc,
      w->allocation.x + w->allocation.width - width - 4,
      w->allocation.y + w->allocation.height - height - 4,
      layout);
  return TRUE;
}

#ifdef GGZ_GTK
/****************************************************************************
  Callback to raise the login dialog when the gaming zone login button is
  clicked.
****************************************************************************/
static void ggz_login(void)
{
  set_client_page(PAGE_GGZ);
}
#endif

/**************************************************************************
  create the main page.
**************************************************************************/
GtkWidget *create_main_page(void)
{
  GtkWidget *widget, *align, *vbox, *frame, *image, *button, *table;
  GtkSizeGroup *size;

  size = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

  vbox = gtk_vbox_new(FALSE, 0);
  widget = vbox;

  align = gtk_alignment_new(0.5, 0.0, 0.0, 0.0);
  gtk_container_set_border_width(GTK_CONTAINER(align), 18);
  gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, 0);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(align), frame);

  image = gtk_image_new_from_file(tileset_main_intro_filename(tileset));
  g_signal_connect_after(image, "expose_event",
                         G_CALLBACK(intro_expose), NULL);
  gtk_container_add(GTK_CONTAINER(frame), image);

#if IS_BETA_VERSION
  {
    GtkWidget *label;

    label = gtk_label_new(beta_message());
    gtk_widget_set_name(label, "beta_label");
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_container_add(GTK_CONTAINER(sbox), label);
  }
#endif

  table = gtk_table_new(3, 2, TRUE);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), align, TRUE, TRUE, 12);

  gtk_table_set_row_spacings(GTK_TABLE(table), 8);
  gtk_table_set_col_spacings(GTK_TABLE(table), 18);
  gtk_container_add(GTK_CONTAINER(align), table);

  button = gtk_button_new_with_mnemonic(_("Start _New Game"));
  gtk_size_group_add_widget(size, button);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 0, 1, 0, 1);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(start_new_game_callback), NULL);

  button = gtk_button_new_with_mnemonic(_("Start _Scenario Game"));
  gtk_size_group_add_widget(size, button);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 0, 1, 1, 2);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(start_scenario_callback), NULL);

  button = gtk_button_new_with_mnemonic(_("_Load Saved Game"));
  gtk_size_group_add_widget(size, button);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 0, 1, 2, 3);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(load_saved_game_callback), NULL);

  button = gtk_button_new_with_mnemonic(_("C_onnect to Network Game"));
  gtk_size_group_add_widget(size, button);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 0, 1);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(set_page_callback),
                   GUINT_TO_POINTER(PAGE_NETWORK));

#ifdef GGZ_GTK
  button = gtk_button_new_with_mnemonic(_("Connect to Gaming _Zone"));
  gtk_size_group_add_widget(size, button);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 1, 2);
  g_signal_connect(button, "clicked", ggz_login, NULL);
#endif

  button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
  gtk_size_group_add_widget(size, button);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 2, 3);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(gtk_main_quit), NULL);

  return widget;
}


/**************************************************************************
                                 NETWORK PAGE
**************************************************************************/
static GtkWidget *network_login_label, *network_login;
static GtkWidget *network_host_label, *network_host;
static GtkWidget *network_port_label, *network_port;
static GtkWidget *network_password_label, *network_password;
static GtkWidget *network_confirm_password_label, *network_confirm_password;

/**************************************************************************
  update a server list.
**************************************************************************/
static void update_server_list(enum server_scan_type sstype,
                               const struct server_list *list)
{
  GtkTreeSelection *sel = NULL;
  GtkTreeView *view;
  GtkTreeIter it;
  GtkListStore *store;
  const gchar *host, *portstr;
  int port;

  switch (sstype) {
  case SERVER_SCAN_LOCAL:
    sel = lan_selection;
    break;
  case SERVER_SCAN_GLOBAL:
    sel = meta_selection;
    break;
  default:
    break;
  }

  if (!sel) {
    return;
  }

  view = gtk_tree_selection_get_tree_view(sel);
  store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
  gtk_list_store_clear(store);

  if (!list) {
    return;
  }

  host = gtk_entry_get_text(GTK_ENTRY(network_host));
  portstr = gtk_entry_get_text(GTK_ENTRY(network_port));
  port = atoi(portstr);

  server_list_iterate(list, pserver) {
    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it,
                       0, pserver->host,
                       1, pserver->port,
                       2, pserver->version,
                       3, _(pserver->state),
                       4, pserver->nplayers,
                       5, pserver->message,
                       -1);
    if (strcmp(host, pserver->host) == 0 && port == pserver->port) {
      gtk_tree_selection_select_iter(sel, &it);
    }
  } server_list_iterate_end;
}

/**************************************************************************
  Free the server scans.
**************************************************************************/
static void destroy_server_scans(void)
{
  if (meta_scan) {
    server_scan_finish(meta_scan);
    meta_scan = NULL;
  }
  if (meta_scan_timer != 0) {
    g_source_remove(meta_scan_timer);
    meta_scan_timer = 0;
  }
  if (lan_scan) {
    server_scan_finish(lan_scan);
    lan_scan = NULL;
  }
  if (lan_scan_timer != 0) {
    g_source_remove(lan_scan_timer);
    lan_scan_timer = 0;
  }
}

/**************************************************************************
  This function updates the list of servers every so often.
**************************************************************************/
static gboolean check_server_scan(gpointer data)
{
  struct server_scan *scan = data;
  const struct server_list *servers;
  enum server_scan_status stat;

  if (!scan) {
    return FALSE;
  }

  stat = server_scan_poll(scan);
  if (stat >= SCAN_STATUS_PARTIAL) {
    enum server_scan_type type;
    type = server_scan_get_type(scan);
    servers = server_scan_get_list(scan);
    update_server_list(type, servers);
  }

  if (stat == SCAN_STATUS_ERROR || stat == SCAN_STATUS_DONE) {
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
  Callback function for when there's an error in the server scan.
**************************************************************************/
static void server_scan_error(struct server_scan *scan,
			      const char *message)
{
  output_window_append(FTC_CLIENT_INFO, NULL, message);
  freelog(LOG_ERROR, "%s", message);

  switch (server_scan_get_type(scan)) {
  case SERVER_SCAN_LOCAL:
    server_scan_finish(lan_scan);
    lan_scan = NULL;
    break;
  case SERVER_SCAN_GLOBAL:
    server_scan_finish(meta_scan);
    meta_scan = NULL;
    break;
  case SERVER_SCAN_LAST:
    break;
  }
}

/**************************************************************************
  Stop and restart the metaserver and lan server scans.
**************************************************************************/
static void update_network_lists(void)
{
  destroy_server_scans();

  meta_scan = server_scan_begin(SERVER_SCAN_GLOBAL, server_scan_error);
  meta_scan_timer = g_timeout_add(200, check_server_scan, meta_scan);

  lan_scan = server_scan_begin(SERVER_SCAN_LOCAL, server_scan_error);
  lan_scan_timer = g_timeout_add(500, check_server_scan, lan_scan);
}

/**************************************************************************
  network connection state defines.
**************************************************************************/
enum connection_state {
  LOGIN_TYPE, 
  NEW_PASSWORD_TYPE, 
  ENTER_PASSWORD_TYPE,
  WAITING_TYPE
};

static enum connection_state connection_status;

/**************************************************************************
  update statusbar label text.
**************************************************************************/
static gboolean update_network_statusbar(gpointer data)
{
  if (!g_queue_is_empty(statusbar_queue)) {
    char *txt;

    txt = g_queue_pop_head(statusbar_queue);
    gtk_label_set_text(GTK_LABEL(statusbar), txt);
    free(txt);
  }

  return TRUE;
}

/**************************************************************************
  clear statusbar queue.
**************************************************************************/
static void clear_network_statusbar(void)
{
  while (!g_queue_is_empty(statusbar_queue)) {
    char *txt;

    txt = g_queue_pop_head(statusbar_queue);
    free(txt);
  }
  gtk_label_set_text(GTK_LABEL(statusbar), "");
}

/**************************************************************************
  queue statusbar label text change.
**************************************************************************/
void append_network_statusbar(const char *text, bool force)
{
  if (GTK_WIDGET_VISIBLE(statusbar_frame)) {
    if (force) {
      clear_network_statusbar();
      gtk_label_set_text(GTK_LABEL(statusbar), text);
    } else {
      g_queue_push_tail(statusbar_queue, mystrdup(text));
    }
  }
}

/**************************************************************************
  create statusbar.
**************************************************************************/
GtkWidget *create_statusbar(void)
{
  statusbar_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(statusbar_frame), GTK_SHADOW_IN);

  statusbar = gtk_label_new("");
  gtk_misc_set_padding(GTK_MISC(statusbar), 2, 2);
  gtk_container_add(GTK_CONTAINER(statusbar_frame), statusbar);

  statusbar_queue = g_queue_new();
  statusbar_timer = g_timeout_add(2000, update_network_statusbar, NULL);

  return statusbar_frame;
}

/**************************************************************************
  update network page connection state.
**************************************************************************/
static void set_connection_state(enum connection_state state)
{
  switch (state) {
  case LOGIN_TYPE:
    append_network_statusbar("", FALSE);

    gtk_entry_set_text(GTK_ENTRY(network_password), "");
    gtk_entry_set_text(GTK_ENTRY(network_confirm_password), "");

    gtk_widget_set_sensitive(network_host, TRUE);
    gtk_widget_set_sensitive(network_port, TRUE);
    gtk_widget_set_sensitive(network_login, TRUE);
    gtk_widget_set_sensitive(network_password_label, FALSE);
    gtk_widget_set_sensitive(network_password, FALSE);
    gtk_widget_set_sensitive(network_confirm_password_label, FALSE);
    gtk_widget_set_sensitive(network_confirm_password, FALSE);
    break;
  case NEW_PASSWORD_TYPE:
    gtk_entry_set_text(GTK_ENTRY(network_password), "");
    gtk_entry_set_text(GTK_ENTRY(network_confirm_password), "");

    gtk_widget_set_sensitive(network_host, FALSE);
    gtk_widget_set_sensitive(network_port, FALSE);
    gtk_widget_set_sensitive(network_login, FALSE);
    gtk_widget_set_sensitive(network_password_label, TRUE);
    gtk_widget_set_sensitive(network_password, TRUE);
    gtk_widget_set_sensitive(network_confirm_password_label, TRUE);
    gtk_widget_set_sensitive(network_confirm_password, TRUE);

    gtk_widget_grab_focus(network_password);
    break;
  case ENTER_PASSWORD_TYPE:

    set_client_page(PAGE_NETWORK);
    gtk_entry_set_text(GTK_ENTRY(network_password), "");
    gtk_entry_set_text(GTK_ENTRY(network_confirm_password), "");

    gtk_widget_set_sensitive(network_host, FALSE);
    gtk_widget_set_sensitive(network_port, FALSE);
    gtk_widget_set_sensitive(network_login, FALSE);
    gtk_widget_set_sensitive(network_password_label, TRUE);
    gtk_widget_set_sensitive(network_password, TRUE);
    gtk_widget_set_sensitive(network_confirm_password_label, FALSE);
    gtk_widget_set_sensitive(network_confirm_password, FALSE);

    gtk_widget_grab_focus(network_password);
    break;
  case WAITING_TYPE:
    append_network_statusbar("", TRUE);

    gtk_widget_set_sensitive(network_login, FALSE);
    gtk_widget_set_sensitive(network_password_label, FALSE);
    gtk_widget_set_sensitive(network_password, FALSE);
    gtk_widget_set_sensitive(network_confirm_password_label, FALSE);
    gtk_widget_set_sensitive(network_confirm_password, FALSE);
    break;
  }

  connection_status = state;
}

/**************************************************************************
 configure the dialog depending on what type of authentication request the
 server is making.
**************************************************************************/
void handle_authentication_req(enum authentication_type type, char *message)
{
  append_network_statusbar(message, TRUE);

  switch (type) {
  case AUTH_NEWUSER_FIRST:
  case AUTH_NEWUSER_RETRY:
    set_connection_state(NEW_PASSWORD_TYPE);
    break;
  case AUTH_LOGIN_FIRST:
    /* if we magically have a password already present in 'password'
     * then, use that and skip the password entry dialog */
    if (password[0] != '\0') {
      struct packet_authentication_reply reply;

      sz_strlcpy(reply.password, password);
      send_packet_authentication_reply(&client.conn, &reply);
      return;
    } else {
      set_connection_state(ENTER_PASSWORD_TYPE);
    }
    break;
  case AUTH_LOGIN_RETRY:
    set_connection_state(ENTER_PASSWORD_TYPE);
    break;
  default:
    assert(0);
  }
}

/**************************************************************************
 if on the network page, switch page to the login page (with new server
 and port). if on the login page, send connect and/or authentication 
 requests to the server.
**************************************************************************/
static void connect_callback(GtkWidget *w, gpointer data)
{
  char errbuf [512];
  struct packet_authentication_reply reply;

  switch (connection_status) {
  case LOGIN_TYPE:
    sz_strlcpy(user_name, gtk_entry_get_text(GTK_ENTRY(network_login)));
    sz_strlcpy(server_host, gtk_entry_get_text(GTK_ENTRY(network_host)));
    server_port = atoi(gtk_entry_get_text(GTK_ENTRY(network_port)));
  
    if (connect_to_server(user_name, server_host, server_port,
                          errbuf, sizeof(errbuf)) != -1) {
    } else {
      append_network_statusbar(errbuf, TRUE);

      output_window_append(FTC_CLIENT_INFO, NULL, errbuf);
    }
    break; 
  case NEW_PASSWORD_TYPE:
    if (w != network_password) {
      sz_strlcpy(password,
	  gtk_entry_get_text(GTK_ENTRY(network_password)));
      sz_strlcpy(reply.password,
	  gtk_entry_get_text(GTK_ENTRY(network_confirm_password)));
      if (strncmp(reply.password, password, MAX_LEN_NAME) == 0) {
	password[0] = '\0';
	send_packet_authentication_reply(&client.conn, &reply);

	set_connection_state(WAITING_TYPE);
      } else { 
	append_network_statusbar(_("Passwords don't match, enter password."),
	    TRUE);

	set_connection_state(NEW_PASSWORD_TYPE);
      }
    }
    break;
  case ENTER_PASSWORD_TYPE:
    sz_strlcpy(reply.password,
	gtk_entry_get_text(GTK_ENTRY(network_password)));
    send_packet_authentication_reply(&client.conn, &reply);

    set_connection_state(WAITING_TYPE);
    break;
  case WAITING_TYPE:
    break;
  default:
    assert(0);
  }
}

/**************************************************************************
  connect on list item double-click.
***************************************************************************/
static void network_activate_callback(GtkTreeView *view,
                      		      GtkTreePath *arg1,
				      GtkTreeViewColumn *arg2,
				      gpointer data)
{
  connect_callback(NULL, data);
}

/**************************************************************************
  Fills the server player list with the players in the given server, or
  clears it if there is no player data.
**************************************************************************/
static void update_server_playerlist(const struct server *pserver)
{
  GtkListStore *store;
  GtkTreeIter iter;
  int n, i;

  store = server_playerlist_store;
  g_return_if_fail(store != NULL);

  gtk_list_store_clear(store);
  if (!pserver || !pserver->players) {
    return;
  }

  n = pserver->nplayers;
  for (i = 0; i < n; i++) {
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, pserver->players[i].name,
                       1, pserver->players[i].type,
                       2, pserver->players[i].host,
                       3, pserver->players[i].nation,
                       -1);
  }
}

/**************************************************************************
  Sets the host, port and player list of the selected server.
**************************************************************************/
static void network_list_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  const char *host;
  int port;
  char portstr[32];
  const struct server *pserver = NULL;

  if (!gtk_tree_selection_get_selected(select, &model, &it)) {
    return;
  }

  if (select == meta_selection) {
    GtkTreePath *path;
    const struct server_list *servers;

    servers = server_scan_get_list(meta_scan);
    path = gtk_tree_model_get_path(model, &it);
    if (servers && path) {
      gint pos = gtk_tree_path_get_indices(path)[0];
      pserver = server_list_get(servers, pos);
    }
    gtk_tree_path_free(path);
  }
  update_server_playerlist(pserver);

  gtk_tree_model_get(model, &it, 0, &host, 1, &port, -1);

  gtk_entry_set_text(GTK_ENTRY(network_host), host);
  my_snprintf(portstr, sizeof(portstr), "%d", port);
  gtk_entry_set_text(GTK_ENTRY(network_port), portstr);
}

/**************************************************************************
  update the network page.
**************************************************************************/
static void update_network_page(void)
{
  char buf[256];

  gtk_tree_selection_unselect_all(lan_selection);
  gtk_tree_selection_unselect_all(meta_selection);

  gtk_entry_set_text(GTK_ENTRY(network_login), user_name);
  gtk_entry_set_text(GTK_ENTRY(network_host), server_host);
  my_snprintf(buf, sizeof(buf), "%d", server_port);
  gtk_entry_set_text(GTK_ENTRY(network_port), buf);

  set_connection_state(LOGIN_TYPE);
}

/**************************************************************************
  create the network page.
**************************************************************************/
GtkWidget *create_network_page(void)
{
  GtkWidget *box, *sbox, *bbox, *hbox, *notebook;
  GtkWidget *button, *label, *view, *sw, *table;
  GtkTreeSelection *selection;
  GtkListStore *store;

  box = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  notebook = gtk_notebook_new();
  gtk_container_add(GTK_CONTAINER(box), notebook);

  /* LAN pane. */
  lan_store = gtk_list_store_new(6, G_TYPE_STRING, /* host */
                                 G_TYPE_INT,       /* port */
                                 G_TYPE_STRING,    /* version */
                                 G_TYPE_STRING,    /* state */
                                 G_TYPE_INT,       /* nplayers */
                                 G_TYPE_STRING);   /* message */

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(lan_store));
  g_object_unref(lan_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  lan_selection = selection;
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  g_signal_connect(view, "focus",
      		   G_CALLBACK(gtk_true), NULL);
  g_signal_connect(view, "row_activated",
		   G_CALLBACK(network_activate_callback), NULL);
  g_signal_connect(selection, "changed",
                   G_CALLBACK(network_list_callback), NULL);

  add_treeview_column(view, _("Server Name"), G_TYPE_STRING, 0);
  add_treeview_column(view, _("Port"), G_TYPE_INT, 1);
  add_treeview_column(view, _("Version"), G_TYPE_STRING, 2);
  add_treeview_column(view, _("Status"), G_TYPE_STRING, 3);
  add_treeview_column(view, _("Players"), G_TYPE_INT, 4);
  add_treeview_column(view, _("Comment"), G_TYPE_STRING, 5);

  label = gtk_label_new_with_mnemonic(_("Local _Area Network"));

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(sw), 4);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);


  /* Metaserver pane. */
  meta_store = gtk_list_store_new(6, G_TYPE_STRING, /* host */
                                  G_TYPE_INT,       /* port */
                                  G_TYPE_STRING,    /* version */
                                  G_TYPE_STRING,    /* state */
                                  G_TYPE_INT,       /* nplayers */
                                  G_TYPE_STRING);   /* message */

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(meta_store));
  g_object_unref(meta_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  meta_selection = selection;
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  g_signal_connect(view, "focus",
      		   G_CALLBACK(gtk_true), NULL);
  g_signal_connect(view, "row_activated",
		   G_CALLBACK(network_activate_callback), NULL);
  g_signal_connect(selection, "changed",
                   G_CALLBACK(network_list_callback), NULL);

  add_treeview_column(view, _("Server Name"), G_TYPE_STRING, 0);
  add_treeview_column(view, _("Port"), G_TYPE_INT, 1);
  add_treeview_column(view, _("Version"), G_TYPE_STRING, 2);
  add_treeview_column(view, _("Status"), G_TYPE_STRING, 3);
  add_treeview_column(view, _("Players"), G_TYPE_INT, 4);
  add_treeview_column(view, _("Comment"), G_TYPE_STRING, 5);

  label = gtk_label_new_with_mnemonic(_("Internet _Metaserver"));

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(sw), 4);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);
  if (gui_gtk2_metaserver_tab_first) {
    gtk_notebook_prepend_page(GTK_NOTEBOOK(notebook), sw, label);
  } else {
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);
  }

  /* Bottom part of the page, outside the inner notebook. */
  sbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), sbox, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(sbox), hbox, FALSE, FALSE, 8);

  table = gtk_table_new(6, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);
  gtk_table_set_row_spacing(GTK_TABLE(table), 2, 12);
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

  network_host = gtk_entry_new();
  g_signal_connect(network_host, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_table_attach(GTK_TABLE(table), network_host, 1, 2, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", network_host,
		       "label", _("_Host:"),
		       "xalign", 0.0,
		       "yalign", 0.5,
		       NULL);
  network_host_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0);

  network_port = gtk_entry_new();
  g_signal_connect(network_port, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_table_attach(GTK_TABLE(table), network_port, 1, 2, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", network_port,
		       "label", _("_Port:"),
		       "xalign", 0.0,
		       "yalign", 0.5,
		       NULL);
  network_port_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
		   GTK_FILL, GTK_FILL, 0, 0);

  network_login = gtk_entry_new();
  g_signal_connect(network_login, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_table_attach(GTK_TABLE(table), network_login, 1, 2, 3, 4,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", network_login,
		       "label", _("_Login:"),
		       "xalign", 0.0,
		       "yalign", 0.5,
		       NULL);
  network_login_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
		   GTK_FILL, GTK_FILL, 0, 0);

  network_password = gtk_entry_new();
  g_signal_connect(network_password, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_entry_set_visibility(GTK_ENTRY(network_password), FALSE);
  gtk_table_attach(GTK_TABLE(table), network_password, 1, 2, 4, 5,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", network_password,
		       "label", _("Pass_word:"),
		       "xalign", 0.0,
		       "yalign", 0.5,
		       NULL);
  network_password_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
		   GTK_FILL, GTK_FILL, 0, 0);

  network_confirm_password = gtk_entry_new();
  g_signal_connect(network_confirm_password, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_entry_set_visibility(GTK_ENTRY(network_confirm_password), FALSE);
  gtk_table_attach(GTK_TABLE(table), network_confirm_password, 1, 2, 5, 6,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", network_confirm_password,
		       "label", _("Conf_irm Password:"),
		       "xalign", 0.0,
		       "yalign", 0.5,
		       NULL);
  network_confirm_password_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
		   GTK_FILL, GTK_FILL, 0, 0);

  /* Server player list. */
  store = gtk_list_store_new(4, G_TYPE_STRING,
                             G_TYPE_STRING,
                             G_TYPE_STRING,
                             G_TYPE_STRING);
  server_playerlist_store = store;

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  add_treeview_column(view, _("Name"), G_TYPE_STRING, 0);
  add_treeview_column(view, _("Type"), G_TYPE_STRING, 1);
  add_treeview_column(view, _("Host"), G_TYPE_STRING, 2);
  add_treeview_column(view, _("Nation"), G_TYPE_STRING, 3);
  server_playerlist_view = view;

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);


  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(sbox), bbox, FALSE, FALSE, 2);

  button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(update_network_lists), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(main_callback), NULL);

  button = gtk_button_new_with_mnemonic(_("C_onnect"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(connect_callback), NULL);

  return box;
}


/**************************************************************************
                                  START PAGE
**************************************************************************/
static GtkWidget *start_aifill_spin;

/**************************************************************************
  request the game options dialog.
**************************************************************************/
static void game_options_callback(GtkWidget *w, gpointer data)
{
  popup_settable_options_dialog();
}

/**************************************************************************
  AI skill setting callback.
**************************************************************************/
static void ai_skill_callback(GtkWidget *w, gpointer data)
{
  const char *name;
  enum ai_level level = GPOINTER_TO_UINT(data);

  if (level == AI_LEVEL_LAST) {
    level = AI_LEVEL_DEFAULT;
  }

  name = ai_level_cmd(level);

  send_chat_printf("/%s", name);
}

/* HACK: sometimes when creating the ruleset combo the value is set without
 * the user's control.  In this case we don't want to do a /read. */
static bool no_ruleset_callback = FALSE;

/**************************************************************************
  Ruleset setting callback
**************************************************************************/
static void ruleset_callback(GtkWidget *w, gpointer data)
{
  const char *name;

  name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ruleset_combo)->entry));

  if (name && name[0] != '\0' && !no_ruleset_callback) {
    set_ruleset(name);
  }
}

/**************************************************************************
  AI fill setting callback.
**************************************************************************/
static bool send_new_aifill_to_server = TRUE;
static void ai_fill_callback(GtkWidget *w, gpointer data)
{
  if (send_new_aifill_to_server) {
    send_chat_printf("/set aifill %d",
                     gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w)));
  }
}

/**************************************************************************
  start game callback.
**************************************************************************/
static void start_start_callback(GtkWidget *w, gpointer data)
{
  if (NULL != client.conn.playing) {
    dsend_packet_player_ready(&client.conn, player_number(client.conn.playing),
			      !client.conn.playing->is_ready);
  }
}

/**************************************************************************
  Called when "pick nation" is clicked.
**************************************************************************/
static void pick_nation_callback(GtkWidget *w, gpointer data)
{
  if (can_client_control()) {
    popup_races_dialog(client_player());
  } else if (game.info.is_new_game) {
    send_chat("/take -");
  }
}

/**************************************************************************
  Called when "observe" is clicked.
**************************************************************************/
static void take_callback(GtkWidget *w, gpointer data)
{
  struct player *plr = client_player();

  if (NULL != plr) {
    const char *name = player_name(plr);
  
    if (client_is_observer()) {
      if (plr->ai_data.control) {
        send_chat_printf("/aitoggle \"%s\"", name);
      }
      send_chat_printf("/take \"%s\"", name);
    } else {
      if (!plr->ai_data.control) {
        /* Make sure player reverts to AI control. This is much more neat,
         * and hides the ugly double username in the name list because
         * the player username equals the connection username. */
        send_chat_printf("/aitoggle \"%s\"", name);
      }
      send_chat("/detach");
      send_chat("/observe");
    }
  } else if (!client.conn.observer) {
    send_chat("/observe");
  } else {
    send_chat("/detach");
  }
}

/**************************************************************************
  update the start page.
**************************************************************************/
void update_start_page(void)
{
  bool old = send_new_aifill_to_server;
  send_new_aifill_to_server = FALSE;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(start_aifill_spin),
                            game.info.aifill);
  send_new_aifill_to_server = old;
}

static struct player *conn_menu_player;
static struct connection *conn_menu_conn;

/****************************************************************************
  Callback for when a team is chosen from the conn menu.
****************************************************************************/
static void conn_menu_team_chosen(GtkMenuItem *menuitem, gpointer data)
{
  struct team *pteam = data;

  if (pteam != conn_menu_player->team) {
    send_chat_printf("/team \"%s\" \"%s\"",
                     player_name(conn_menu_player),
                     team_rule_name(pteam));
  }
}

/****************************************************************************
  Callback for when the "ready" entry is chosen from the conn menu.
****************************************************************************/
static void conn_menu_ready_chosen(GtkMenuItem *menuitem, gpointer data)
{
  struct player *pplayer = conn_menu_player;

  dsend_packet_player_ready(&client.conn,
			    player_number(pplayer), !pplayer->is_ready);
}

/****************************************************************************
  Callback for when the pick-nation entry is chosen from the conn menu.
****************************************************************************/
static void conn_menu_nation_chosen(GtkMenuItem *menuitem, gpointer data)
{
  popup_races_dialog(conn_menu_player);
}

/****************************************************************************
  Miscellaneous callback for the conn menu that allows an arbitrary command
  (/observe, /remove, /hard) to be run on the player.
****************************************************************************/
static void conn_menu_player_command(GtkMenuItem *menuitem, gpointer data)
{
  char *command = data;

  assert(command != NULL);
  assert(conn_menu_player != NULL);

  send_chat_printf("/%s \"%s\"", command, player_name(conn_menu_player));
}

/****************************************************************************
  Take command in the conn menu.
****************************************************************************/
static void conn_menu_player_take(GtkMenuItem *menuitem, gpointer data)
{
  if (conn_menu_player->ai_data.control) {
    /* See comment on detach command for why */
    send_chat_printf("/aitoggle \"%s\"", player_name(conn_menu_player));
  }
  send_chat_printf("/take \"%s\"", player_name(conn_menu_player));
}

/****************************************************************************
  Miscellaneous callback for the conn menu that allows an arbitrary command
  (/cmdlevel, /cut) to be run on the connection.
****************************************************************************/
static void conn_menu_connection_command(GtkMenuItem *menuitem, gpointer data)
{
  const char *command = data;

  assert(conn_menu_conn != NULL);
  assert(command != NULL);

  send_chat_printf("/%s \"%s\"", command, conn_menu_conn->username);
}

/**************************************************************************
 Show details about a user in the Connected Users dialog in a popup.
**************************************************************************/
static void show_conn_popup(struct player *pplayer, struct connection *pconn)
{
  GtkWidget *popup;
  char buf[1024] = "";

  if (pconn) {
    cat_snprintf(buf, sizeof(buf), _("Connection name: %s"),
                 pconn->username);
  } else {
    cat_snprintf(buf, sizeof(buf), _("Player name: %s"),
                 player_name(pplayer));
  }
  cat_snprintf(buf, sizeof(buf), "\n");
  if (pconn) {
    cat_snprintf(buf, sizeof(buf), _("Host: %s"), pconn->addr);
  }
  cat_snprintf(buf, sizeof(buf), "\n");

  /* Show popup. */
  popup = gtk_message_dialog_new(NULL, 0,
				 GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
				 "%s", buf);
  gtk_window_set_title(GTK_WINDOW(popup), _("Player/conn info"));
  setup_dialog(popup, toplevel);
  g_signal_connect(popup, "response", G_CALLBACK(gtk_widget_destroy), NULL);
  gtk_window_present(GTK_WINDOW(popup));
}

/****************************************************************************
  Callback for when the "info" entry is chosen from the conn menu.
****************************************************************************/
static void conn_menu_info_chosen(GtkMenuItem *menuitem, gpointer data)
{
  show_conn_popup(conn_menu_player, conn_menu_conn);
}

/****************************************************************************
  Called when you click on a player; this function pops up a menu
  to allow changing the team.
****************************************************************************/
static GtkWidget *create_conn_menu(struct player *pplayer,
				   struct connection *pconn)
{
  GtkWidget *menu;
  GtkWidget *entry;
  char buf[128];

  menu = gtk_menu_new();

  my_snprintf(buf, sizeof(buf), _("%s info"),
	      pconn ? pconn->username : player_name(pplayer));
  entry = gtk_menu_item_new_with_label(buf);
  g_object_set_data_full(G_OBJECT(menu),
			 "info", entry,
			 (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add(GTK_CONTAINER(menu), entry);
  g_signal_connect(GTK_OBJECT(entry), "activate",
		   GTK_SIGNAL_FUNC(conn_menu_info_chosen), NULL);

  if (pplayer) {
    entry = gtk_menu_item_new_with_label(_("Toggle player ready"));
    gtk_widget_set_sensitive(entry, pplayer && !pplayer->ai_data.control);
    g_object_set_data_full(G_OBJECT(menu), "ready", entry,
                           (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);
    g_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(conn_menu_ready_chosen), NULL);

    entry = gtk_menu_item_new_with_label(_("Pick nation"));
    gtk_widget_set_sensitive(entry,
                             can_conn_edit_players_nation(&client.conn,
                                                          pplayer));
    g_object_set_data_full(G_OBJECT(menu), "nation", entry,
                           (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);
    g_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(conn_menu_nation_chosen), NULL);

    entry = gtk_menu_item_new_with_label(_("Observe this player"));
    g_object_set_data_full(G_OBJECT(menu), "observe", entry,
                           (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);
    g_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(conn_menu_player_command), "observe");

    entry = gtk_menu_item_new_with_label(_("Take this player"));
    g_object_set_data_full(G_OBJECT(menu), "take", entry,
                           (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);
    g_signal_connect(GTK_OBJECT(entry), "activate",
                     GTK_SIGNAL_FUNC(conn_menu_player_take), "take");
  }

  if (ALLOW_CTRL <= client.conn.access_level && NULL != pconn
      && (pconn->id != client.conn.id || NULL != pplayer)) {
    entry = gtk_separator_menu_item_new();
    g_object_set_data_full(G_OBJECT(menu),
			   "ctrl", entry,
			   (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);

    if (pconn->id != client.conn.id) {
      entry = gtk_menu_item_new_with_label(_("Cut connection"));
      g_object_set_data_full(G_OBJECT(menu), "cut", entry,
			     (GtkDestroyNotify) gtk_widget_unref);
      gtk_container_add(GTK_CONTAINER(menu), entry);
      g_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(conn_menu_connection_command), "cut");
    }
  }

  if (ALLOW_CTRL <= client.conn.access_level && NULL != pplayer) {
    entry = gtk_menu_item_new_with_label(_("Aitoggle player"));
    g_object_set_data_full(G_OBJECT(menu), "aitoggle", entry,
			   (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);
    g_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(conn_menu_player_command), "aitoggle");

    if (pplayer != client.conn.playing
        && game.info.is_new_game) {
      entry = gtk_menu_item_new_with_label(_("Remove player"));
      g_object_set_data_full(G_OBJECT(menu), "remove", entry,
			     (GtkDestroyNotify) gtk_widget_unref);
      gtk_container_add(GTK_CONTAINER(menu), entry);
      g_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(conn_menu_player_command), "remove");
    }
  }

  if (ALLOW_HACK == client.conn.access_level && NULL != pconn
      && pconn->id != client.conn.id) {
    entry = gtk_menu_item_new_with_label(_("Give info access"));
    g_object_set_data_full(G_OBJECT(menu), "cmdlevel-info", entry,
			   (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);
    g_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(conn_menu_connection_command),
		     "cmdlevel info");

    entry = gtk_menu_item_new_with_label(_("Give ctrl access"));
    g_object_set_data_full(G_OBJECT(menu), "cmdlevel-ctrl", entry,
			   (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);
    g_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(conn_menu_connection_command),
                     "cmdlevel ctrl");

    /* No entry for hack access; that would be a serious security hole. */
  }

  if (ALLOW_CTRL <= client.conn.access_level
      && NULL != pplayer && pplayer->ai_data.control) {
    enum ai_level level;

    entry = gtk_separator_menu_item_new();
    g_object_set_data_full(G_OBJECT(menu),
			   "sep-ai-skill", entry,
			   (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);

    for (level = 0; level < AI_LEVEL_LAST; level++) {
      if (is_settable_ai_level(level)) {
        const char *level_name = ai_level_name(level);
        const char *level_cmd = ai_level_cmd(level);
        static char lvl_cmd_tmp[AI_LEVEL_LAST][50];

        /* Copy to non-const string */
        mystrlcpy(lvl_cmd_tmp[level], level_cmd, sizeof(lvl_cmd_tmp[level]));

        entry = gtk_menu_item_new_with_label(level_name);
        g_object_set_data_full(G_OBJECT(menu),
                               lvl_cmd_tmp[level], entry,
                               (GtkDestroyNotify) gtk_widget_unref);
        gtk_container_add(GTK_CONTAINER(menu), entry);
        g_signal_connect(GTK_OBJECT(entry), "activate",
                         GTK_SIGNAL_FUNC(conn_menu_player_command),
                         lvl_cmd_tmp[level]);
      }
    }
  }

  if (pplayer && game.info.is_new_game) {
    const int count = pplayer->team ? pplayer->team->players : 0;
    bool need_empty_team = (count != 1);
    int index;

    entry = gtk_separator_menu_item_new();
    g_object_set_data_full(G_OBJECT(menu),
			   "sep-team", entry,
			   (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add(GTK_CONTAINER(menu), entry);

    /* Can't use team_iterate here since it skips empty teams. */
    for (index = 0; index < MAX_NUM_TEAMS; index++) {
      struct team *pteam = team_by_number(index);
      char text[128];

      if (pteam->players == 0) {
	if (!need_empty_team) {
	  continue;
	}
	need_empty_team = FALSE;
      }

      /* TRANS: e.g., "Put on Team 5" */
      my_snprintf(text, sizeof(text), _("Put on %s"), team_name_translation(pteam));
      entry = gtk_menu_item_new_with_label(text);
      g_object_set_data_full(G_OBJECT(menu),
			     team_rule_name(pteam), entry,
			     (GtkDestroyNotify) gtk_widget_unref);
      gtk_container_add(GTK_CONTAINER(menu), entry);
      g_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(conn_menu_team_chosen),
		       pteam);
    }
  }

  conn_menu_player = pplayer;
  conn_menu_conn = pconn;

  gtk_widget_show_all(menu);

  return menu;
}

/**************************************************************************
  Called on a button event on the pregame player list.
**************************************************************************/
static gboolean playerlist_event(GtkWidget *widget, GdkEventButton *event,
				 gpointer data)
{
  GtkTreeView *tree = GTK_TREE_VIEW(widget);
  GtkTreeModel *model = gtk_tree_view_get_model(tree);
  GtkTreeIter iter;
  GtkTreePath *path = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkWidget *menu;
  int player_no, conn_id;
  struct player *pplayer;
  struct connection *pconn;

  if (event->type != GDK_BUTTON_PRESS
      || event->button != 3
      || !gtk_tree_view_get_path_at_pos(tree,
					event->x, event->y,
					&path, &column, NULL, NULL)) {
    return FALSE;
  }

  gtk_tree_model_get_iter(model, &iter, path);

  gtk_tree_model_get(model, &iter, CL_COL_PLAYER_NUMBER, &player_no, -1);
  pplayer = valid_player_by_number(player_no);

  gtk_tree_model_get(model, &iter, CL_COL_CONN_ID, &conn_id, -1);
  pconn = find_conn_by_id(conn_id);

  menu = create_conn_menu(pplayer, pconn);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
                 NULL, NULL, event->button, 0);

  gtk_tree_path_free(path);
  return TRUE;
}

/**************************************************************************
  Helper function for adding columns to a tree view. If 'key' is not NULL
  then the added column is added to the object data of the treeview using
  g_object_set_data under 'key'.
**************************************************************************/
static void add_tree_col(GtkWidget *treeview, GType gtype,
                         const char *title, int colnum, const char *key)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  const char *attr;

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
                                                 colnum, NULL);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

  if (key != NULL) {
    g_object_set_data(G_OBJECT(treeview), key, col);
  }
}

/**************************************************************************
  create start page.
**************************************************************************/
GtkWidget *create_start_page(void)
{
  GtkWidget *box, *sbox, *table, *align, *vbox;
  GtkWidget *view, *sw, *text, *toolkit_view, *button, *spin, *option;
  GtkWidget *label, *menu, *item;
  GtkTreeStore *store;
  enum ai_level level;

  box = gtk_vbox_new(FALSE, 8);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  sbox = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(box), sbox, FALSE, FALSE, 0);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width(GTK_CONTAINER(align), 12);
  gtk_box_pack_start(GTK_BOX(sbox), align, FALSE, FALSE, 0);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(align), vbox);

  table = gtk_table_new(2, 3, FALSE);
  start_options_table = table;
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

  spin = gtk_spin_button_new_with_range(1, MAX_NUM_PLAYERS, 1);
  start_aifill_spin = spin;
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), 
                                    GTK_UPDATE_IF_VALID);
  g_signal_connect_after(spin, "value_changed",
                         G_CALLBACK(ai_fill_callback), NULL);

  gtk_table_attach_defaults(GTK_TABLE(table), spin, 1, 2, 0, 1);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", spin,
                       "label", _("Number of _Players (including AI):"),
                       "xalign", 0.0,
                       "yalign", 0.5,
                       NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

  option = gtk_option_menu_new();

  menu = gtk_menu_new();
  for (level = 0; level < AI_LEVEL_LAST; level++) {
    if (is_settable_ai_level(level)) {
      const char *level_name = ai_level_name(level);

      item = gtk_menu_item_new_with_label(level_name);
      g_signal_connect(item, "activate",
                       G_CALLBACK(ai_skill_callback), GUINT_TO_POINTER(level));

      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_table_attach_defaults(GTK_TABLE(table), option, 1, 2, 1, 2);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", option,
                       "label", _("AI Skill _Level:"),
                       "xalign", 0.0,
                       "yalign", 0.5,
                       NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

  ruleset_combo = gtk_combo_new();
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ruleset_combo)->entry), "default");
  g_signal_connect(GTK_COMBO(ruleset_combo)->entry, "changed",
                   G_CALLBACK(ruleset_callback),
                   GUINT_TO_POINTER(AI_LEVEL_LAST));

  gtk_table_attach_defaults(GTK_TABLE(table), ruleset_combo, 1, 2, 2, 3);

  label = g_object_new(GTK_TYPE_LABEL,
		       "use-underline", TRUE,
		       "mnemonic-widget", GTK_COMBO(ruleset_combo)->entry,
                       "label", _("Ruleset _Version:"),
                       "xalign", 0.0,
                       "yalign", 0.5,
                       NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  button = gtk_stockbutton_new(GTK_STOCK_PREFERENCES,
      _("_More Game Options..."));
  g_signal_connect(button, "clicked",
      G_CALLBACK(game_options_callback), NULL);
  gtk_container_add(GTK_CONTAINER(align), button);
  gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, 8);

  /* NB: Must match order and type of enum
   * connection_list_columns in gui_main.h. */
  store = gtk_tree_store_new(CL_NUM_COLUMNS, G_TYPE_INT,
                             G_TYPE_STRING, G_TYPE_BOOLEAN,
                             G_TYPE_STRING, GDK_TYPE_PIXBUF,
			     G_TYPE_STRING, G_TYPE_STRING,
			     G_TYPE_STRING, G_TYPE_STRING,
			     G_TYPE_INT);
  connection_list_store = store;

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), TRUE);
  connection_list_view = GTK_TREE_VIEW(view);

  add_tree_col(view, G_TYPE_STRING, _("Name"),
               CL_COL_USER_NAME, NULL);
  add_tree_col(view, G_TYPE_STRING, _("Record"),
               CL_COL_GGZ_RECORD, "record_col");
  add_tree_col(view, G_TYPE_STRING, _("Rating"),
               CL_COL_GGZ_RATING, "rating_col");
  add_tree_col(view, G_TYPE_BOOLEAN, _("Ready"),
               CL_COL_READY_STATE, NULL);
  add_tree_col(view, G_TYPE_STRING, Q_("?player:Leader"),
               CL_COL_PLAYER_NAME, NULL);
  add_tree_col(view, GDK_TYPE_PIXBUF, _("Flag"),
               CL_COL_FLAG, NULL);
  add_tree_col(view, G_TYPE_STRING, _("Nation"),
               CL_COL_NATION, NULL);
  add_tree_col(view, G_TYPE_STRING, _("Team"),
               CL_COL_TEAM, NULL);

  g_signal_connect(view, "button-press-event",
		   G_CALLBACK(playerlist_event), NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_ALWAYS);
  gtk_widget_set_size_request(sw, -1, 200);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);


  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(box), sw, TRUE, TRUE, 0);

  text = gtk_text_view_new_with_buffer(message_buffer);
  start_message_area = text;
  gtk_widget_set_name(text, "chatline");
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 5);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_container_add(GTK_CONTAINER(sw), text);


  /* Vote widgets. */
  if (pregame_votebar == NULL) {
    pregame_votebar = voteinfo_bar_new();
  }
  gtk_box_pack_start(GTK_BOX(box), pregame_votebar, FALSE, FALSE, 0);
  

  toolkit_view = inputline_toolkit_view_new();
  gtk_box_pack_start(GTK_BOX(box), toolkit_view, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  inputline_toolkit_view_append_button(toolkit_view, button);
  g_signal_connect(button, "clicked", G_CALLBACK(main_callback), NULL);

  nation_button = gtk_stockbutton_new(GTK_STOCK_PROPERTIES,
                                      _("Pick _Nation"));
  inputline_toolkit_view_append_button(toolkit_view, nation_button);
  g_signal_connect(nation_button, "clicked",
                   G_CALLBACK(pick_nation_callback), NULL);

  take_button = gtk_stockbutton_new(GTK_STOCK_ZOOM_IN, _("_Observe"));
  inputline_toolkit_view_append_button(toolkit_view, take_button);
  g_signal_connect(take_button, "clicked",
                   G_CALLBACK(take_callback), NULL);

  ready_button = gtk_stockbutton_new(GTK_STOCK_EXECUTE, _("_Ready"));
  inputline_toolkit_view_append_button(toolkit_view, ready_button);
  g_signal_connect(ready_button, "clicked",
                   G_CALLBACK(start_start_callback), NULL);

  return box;
}


/**************************************************************************
  this regenerates the player information from a loaded game on the server.
**************************************************************************/
void handle_game_load(struct packet_game_load *packet)
{
  if (!packet->load_successful) {
  } else {
    if (game.info.is_new_game) {
      set_client_page(PAGE_START);
      
      /* It's pregame. Create a player and connect to him */
      send_chat("/take -");
    } else {
      update_nation_page(packet);
      set_client_page(PAGE_NATION);
    }
  }
}

/**************************************************************************
  loads the currently selected game.
**************************************************************************/
static void load_callback(void)
{
  GtkTreeIter it;
  char *filename;

  if (!gtk_tree_selection_get_selected(load_selection, NULL, &it)) {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(load_store), &it, 1, &filename, -1);

  if (is_server_running()) {
    send_chat_printf("/load %s", filename);
  }
}

/**************************************************************************
  call the default GTK+ requester for saved game loading.
**************************************************************************/
static void load_browse_callback(GtkWidget *w, gpointer data)
{
  create_file_selection(_("Choose Saved Game to Load"), FALSE);
}

/**************************************************************************
  update the saved games list store.
**************************************************************************/
static void update_saves_store(GtkListStore *store, const char *dir)
{
  struct datafile_list *files;

  gtk_list_store_clear(store);

  /* search for user saved games. */
  files = datafilelist_infix(dir, ".sav", FALSE);
  datafile_list_iterate(files, pfile) {
    GtkTreeIter it;

    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it,
	0, pfile->name, 1, pfile->fullname, -1);

    free(pfile->name);
    free(pfile->fullname);
    free(pfile);
  } datafile_list_iterate_end;

  datafile_list_free(files);

  files = datafilelist_infix(NULL, ".sav", FALSE);
  datafile_list_iterate(files, pfile) {
    GtkTreeIter it;

    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it,
	0, pfile->name, 1, pfile->fullname, -1);

    free(pfile->name);
    free(pfile->fullname);
    free(pfile);
  } datafile_list_iterate_end;

  datafile_list_free(files);
}

/**************************************************************************
 update the load page.
**************************************************************************/
static void update_load_page(void)
{
  update_saves_store(load_store, "saves");
}

/**************************************************************************
  create the load page.
**************************************************************************/
GtkWidget *create_load_page(void)
{
  GtkWidget *align, *box, *sbox, *bbox;

  GtkWidget *button, *label, *view, *sw;
  GtkCellRenderer *rend;

  box = gtk_vbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 1.0);
  gtk_box_pack_start(GTK_BOX(box), align, TRUE, TRUE, 0);

  load_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(load_store));
  g_object_unref(load_store);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
      -1, NULL, rend, "text", 0, NULL);

  load_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  gtk_tree_selection_set_mode(load_selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated",
                   G_CALLBACK(load_callback), NULL);
  
  sbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(align), sbox);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("Choose Saved Game to _Load:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
  				 GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(sw, 300, -1);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(box), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("_Browse..."));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(load_browse_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(main_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(load_callback), NULL);

  return box;
}


/**************************************************************************
  Updates the info for the currently selected scenario.
**************************************************************************/
static void scenario_list_callback(void)
{
  GtkTreeIter it;
  GtkTextBuffer *buffer;
  char *description;

  if (gtk_tree_selection_get_selected(scenario_selection, NULL, &it)) {
    gtk_tree_model_get(GTK_TREE_MODEL(scenario_store), &it,
		       2, &description, -1);
  } else {
    description = "";
  }

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(scenario_description));
  gtk_text_buffer_set_text(buffer, description, -1);
}

/**************************************************************************
  loads the currently selected scenario.
**************************************************************************/
static void scenario_callback(void)
{
  GtkTreeIter it;
  char *filename;

  if (!gtk_tree_selection_get_selected(scenario_selection, NULL, &it)) {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(scenario_store), &it, 1, &filename, -1);

  if (is_server_running()) {
    send_chat_printf("/load %s", filename);
  }
}

/**************************************************************************
  call the default GTK+ requester for scenario loading.
**************************************************************************/
static void scenario_browse_callback(GtkWidget *w, gpointer data)
{
  create_file_selection(_("Choose a Scenario"), FALSE);
}

/**************************************************************************
  update the scenario page.
**************************************************************************/
static void update_scenario_page(void)
{
  struct datafile_list *files;

  gtk_list_store_clear(scenario_store);

  /* search for scenario files. */
  files = datafilelist_infix("scenario", ".sav", TRUE);
  datafile_list_iterate(files, pfile) {
    GtkTreeIter it;
    struct section_file sf;

    gtk_list_store_append(scenario_store, &it);

    if (section_file_load_section(&sf, pfile->fullname, "scenario")) {
      char *sname = secfile_lookup_str_default(&sf, NULL, "scenario.name");
      char *sdescription = secfile_lookup_str_default(&sf,
					       NULL, "scenario.description");

      gtk_list_store_set(scenario_store, &it,
			 0, sname ? Q_(sname) : pfile->name,
			 1, pfile->fullname,
			 2, sdescription ? Q_(sdescription) : "",
			-1);
      section_file_free(&sf);
    } else {
      gtk_list_store_set(scenario_store, &it,
			 0, pfile->name,
			 1, pfile->fullname,
			 2, "",
			-1);
    }

    free(pfile->name);
    free(pfile->fullname);
    free(pfile);
  } datafile_list_iterate_end;

  datafile_list_free(files);
}

/**************************************************************************
  create the scenario page.
**************************************************************************/
GtkWidget *create_scenario_page(void)
{
  GtkWidget *vbox, *hbox, *sbox, *bbox;

  GtkWidget *align, *button, *label, *view, *sw, *text;
  GtkCellRenderer *rend;

  vbox = gtk_vbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

  scenario_store = gtk_list_store_new(3, G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING);
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(scenario_store));
  g_object_unref(scenario_store);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
      -1, NULL, rend, "text", 0, NULL);

  scenario_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_signal_connect(scenario_selection, "changed",
                   G_CALLBACK(scenario_list_callback), NULL);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  gtk_tree_selection_set_mode(scenario_selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated",
                   G_CALLBACK(scenario_callback), NULL);
  
  sbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), sbox, TRUE, TRUE, 0);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("Choose a _Scenario:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(TRUE, 12);
  gtk_box_pack_start(GTK_BOX(sbox), hbox, TRUE, TRUE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
  				 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);

  align = gtk_alignment_new(0.5, 0.0, 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox), align, TRUE, TRUE, 0);

  text = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 2);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  scenario_description = text;

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
  				 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), text);
  gtk_container_add(GTK_CONTAINER(align), sw);

  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("_Browse..."));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(scenario_browse_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(main_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(scenario_callback), NULL);

  return vbox;
}


/**************************************************************************
  change the player name to that of the nation's leader and start the game.
**************************************************************************/
static void nation_start_callback(void)
{
  GtkTreeIter it;
  char *name;

  if (!gtk_tree_selection_get_selected(nation_selection, NULL, &it)) {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(nation_store), &it, 0, &name, -1);
  sz_strlcpy(leader_name, name);

  send_start_saved_game();
}

/**************************************************************************
...
**************************************************************************/
static void update_nation_page(struct packet_game_load *packet)
{
  int i;

  set_player_count(packet->nplayers);

  gtk_list_store_clear(nation_store);

  for (i = 0; i < packet->nplayers; i++) {
    GtkTreeIter iter;
    const char *nation_name;
    struct nation_type *pnation = nation_by_number(packet->nations[i]);

    if (pnation == NO_NATION_SELECTED) {
      nation_name = "";
    } else {
      nation_name = nation_adjective_translation(pnation);
    }

    gtk_list_store_append(nation_store, &iter);
    gtk_list_store_set(nation_store, &iter, 
	0, packet->name[i],
	2, nation_name,
	3, packet->is_alive[i] ? _("Alive") : _("Dead"),
	4, packet->is_ai[i] ? _("AI") : _("Human"), -1);

    /* set flag if we've got one to set. */
    if (pnation != NO_NATION_SELECTED) {
      GdkPixbuf *flag = get_flag(pnation);

      if (flag) {
	gtk_list_store_set(nation_store, &iter, 1, flag, -1);
	g_object_unref(flag);
      }
    }
  }

  /* if nplayers is zero, we suppose it's a scenario */
  if (packet->nplayers == 0) {
    GtkTreeIter iter;
    send_chat("/take -");

    /* create a false entry */
    gtk_list_store_append(nation_store, &iter);
    gtk_list_store_set(nation_store, &iter,
	0, user_name,
	3, _("Alive"),
	4, _("Human"), -1);
  }
}

/**************************************************************************
  create the nation page.
**************************************************************************/
GtkWidget *create_nation_page(void)
{
  GtkWidget *box, *sbox, *label, *view, *sw, *bbox, *button;
  GtkCellRenderer *trenderer, *prenderer;

  box = gtk_vbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  sbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(box), sbox);

  nation_store = gtk_list_store_new(5, G_TYPE_STRING, GDK_TYPE_PIXBUF,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(nation_store));
  nation_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_object_unref(nation_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  trenderer = gtk_cell_renderer_text_new();
  prenderer = gtk_cell_renderer_pixbuf_new();

  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
        -1, _("Name"), trenderer, "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
        -1, _("Flag"), prenderer, "pixbuf", 1, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
        -1, _("Nation"), trenderer, "text", 2, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
        -1, _("Status"), trenderer, "text", 3, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
        -1, _("Type"), trenderer, "text", 4, NULL);

  gtk_tree_selection_set_mode(nation_selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated",
                   G_CALLBACK(nation_start_callback), NULL);

  label = g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
		       "mnemonic-widget", view,
                       "label", _("Choose a _nation to play:"),
                       "xalign", 0.0,
                       "yalign", 0.5,
                       NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 2);
  
  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(box), bbox, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_PREFERENCES,
      _("_More Game Options..."));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(game_options_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(main_callback), NULL);

  button = gtk_stockbutton_new(GTK_STOCK_EXECUTE, _("_Start"));
  g_signal_connect(button, "clicked",
      G_CALLBACK(nation_start_callback), NULL);
  gtk_container_add(GTK_CONTAINER(bbox), button);

  return box;
}

/**************************************************************************
  Returns current client page
**************************************************************************/
enum client_pages get_client_page(void)
{
  return next_page;
}

/**************************************************************************
  changes the current page.
  this is basically a state machine. jumps actions are hardcoded.
**************************************************************************/
static gboolean set_client_page_callback(gpointer data)
{
  /* Invalid value at start, to be sure that it won't be catch throught a
   * switch(). */
  static enum client_pages old_page = -1;
  /* Don't use next_page directly here because maybe it could be modified
   * before we reach the end of this function. */
  enum client_pages new_page = next_page;

  /* Remove GSource id. */
  *((guint *) data) = 0;

  /* If the page remains the same, don't do anything. */
  if (old_page == new_page) {
    return FALSE;
  }

  switch (old_page) {
  case PAGE_SCENARIO:
  case PAGE_LOAD:
    break;
  case PAGE_NETWORK:
    destroy_server_scans();
    break;
  case PAGE_GAME:
    enable_menus(FALSE);
    gtk_window_unmaximize(GTK_WINDOW(toplevel));
    break;
  default:
    break;
  }

  switch (new_page) {
  case PAGE_MAIN:
  case PAGE_GGZ:
    break;
  case PAGE_START:
    if (is_server_running()) {
      gtk_widget_show(start_options_table);
      update_start_page();
    } else {
      gtk_widget_hide(start_options_table);
    }
    voteinfo_gui_update();
    break;
  case PAGE_NATION:
    break;
  case PAGE_GAME:
    reset_unit_table();
    enable_menus(TRUE);
    gtk_window_maximize(GTK_WINDOW(toplevel));
    voteinfo_gui_update();
    break;
  case PAGE_LOAD:
    update_load_page();
    break;
  case PAGE_SCENARIO:
    update_scenario_page();
    break;
  case PAGE_NETWORK:
    update_network_page();
    break;
  }

  /* hide/show statusbar. */
  if (new_page == PAGE_START || new_page == PAGE_GAME) {
    clear_network_statusbar();
    gtk_widget_hide(statusbar_frame);
  } else {
    gtk_widget_show(statusbar_frame);
  }

  gtk_notebook_set_current_page(GTK_NOTEBOOK(toplevel_tabs), new_page);

  /* Update the GUI. */
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }

  switch (new_page) {
  case PAGE_MAIN:
    break;
  case PAGE_START:
    chatline_scroll_to_bottom(FALSE);
    inputline_grab_focus();
    break;
  case PAGE_GGZ:
    break;
  case PAGE_NATION:
    gtk_tree_view_focus(gtk_tree_selection_get_tree_view(nation_selection));
    break;
  case PAGE_LOAD:
    gtk_tree_view_focus(gtk_tree_selection_get_tree_view(load_selection));
    break;
  case PAGE_SCENARIO:
    gtk_tree_view_focus(gtk_tree_selection_get_tree_view(scenario_selection));
    break;
  case PAGE_GAME:
    chatline_scroll_to_bottom(FALSE);
    refresh_chat_buttons();
    break;
  case PAGE_NETWORK:
    update_network_lists();
    gtk_widget_grab_focus(network_login);
    gtk_editable_set_position(GTK_EDITABLE(network_login), 0);
    break;
  }

  old_page = new_page;

  return FALSE;
}

/**************************************************************************
  Changes the current page.  The action is delayed.
**************************************************************************/
void set_client_page(enum client_pages page)
{
  static guint callback_id = 0;

  next_page = page;
  if (0 == callback_id) {
    callback_id = g_idle_add(set_client_page_callback, &callback_id);
  }
}

/**************************************************************************
...
**************************************************************************/
static void set_page_callback(GtkWidget *w, gpointer data)
{
  set_client_page(GPOINTER_TO_UINT(data));
}

/**************************************************************************
                                SAVE GAME DIALOG
**************************************************************************/
static GtkWidget *save_dialog_shell, *save_entry;
static GtkListStore *save_store;
static GtkTreeSelection *save_selection;

enum {
  SAVE_BROWSE,
  SAVE_DELETE,
  SAVE_SAVE
};

/**************************************************************************
  update save dialog.
**************************************************************************/
static void update_save_dialog(void)
{
  update_saves_store(save_store, save_scenario ? "scenario" : "saves");
}

/**************************************************************************
  handle save dialog response.
**************************************************************************/
static void save_response_callback(GtkWidget *w, gint arg)
{
  switch (arg) {
  case SAVE_BROWSE:
    create_file_selection(_("Select Location to Save"), TRUE);
    break;
  case SAVE_DELETE:
    {
      char *filename;
      GtkTreeIter it;

      if (!gtk_tree_selection_get_selected(save_selection, NULL, &it)) {
	return;
      }

      gtk_tree_model_get(GTK_TREE_MODEL(save_store), &it, 1, &filename, -1);
      remove(filename);
      update_save_dialog();
    }
    return;
  case SAVE_SAVE:
    {
      const char *text;
      char *filename;

      text = gtk_entry_get_text(GTK_ENTRY(save_entry));
      filename = g_filename_from_utf8(text, -1, NULL, NULL, NULL);
      if (!filename) {
        break;
      }
      if (save_scenario) {
        dsend_packet_save_scenario(&client.conn, filename);
      } else {
        send_save_game(filename);
      }
      g_free(filename);
    }
    break;
  default:
    break;
  }
  gtk_widget_destroy(save_dialog_shell);
}

/**************************************************************************
  handle save list double click.
**************************************************************************/
static void save_row_callback(void)
{
  save_response_callback(NULL, SAVE_SAVE);
}

/**************************************************************************
  handle save filename entry activation.
**************************************************************************/
static void save_entry_callback(GtkEntry *w, gpointer data)
{
  save_response_callback(NULL, SAVE_SAVE);
}

/**************************************************************************
  handle save list selection change.
**************************************************************************/
static void save_list_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  char *name;

  if (!gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_dialog_set_response_sensitive(GTK_DIALOG(save_dialog_shell),
	SAVE_DELETE, FALSE);
    return;
  }

  gtk_dialog_set_response_sensitive(GTK_DIALOG(save_dialog_shell),
      SAVE_DELETE, TRUE);

  gtk_tree_model_get(model, &it, 0, &name, -1);
  gtk_entry_set_text(GTK_ENTRY(save_entry), name);
}

/**************************************************************************
  create save dialog.
**************************************************************************/
static void create_save_dialog(bool scenario)
{
  GtkWidget *shell;

  GtkWidget *sbox, *sw;

  GtkWidget *label, *view, *entry;
  GtkCellRenderer *rend;
  GtkTreeSelection *selection;

  save_scenario = scenario;

  shell = gtk_dialog_new_with_buttons(scenario ? _("Save Scenario") : _("Save Game"),
      NULL,
      0,
      _("_Browse..."),
      SAVE_BROWSE,
      GTK_STOCK_DELETE,
      SAVE_DELETE,
      GTK_STOCK_CANCEL,
      GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE,
      SAVE_SAVE,
      NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CANCEL);
  save_dialog_shell = shell;
  setup_dialog(shell, toplevel);

  save_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(save_store));
  g_object_unref(save_store);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
      -1, NULL, rend, "text", 0, NULL);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  save_selection = selection;
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated",
                   G_CALLBACK(save_row_callback), NULL);
  g_signal_connect(selection, "changed",
                   G_CALLBACK(save_list_callback), NULL);

  sbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(shell)->vbox), sbox, TRUE, TRUE, 0);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("Saved _Games:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
  				 GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(sw, 300, 300);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);


  sbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(shell)->vbox), sbox, FALSE, FALSE, 12);

  entry = gtk_entry_new();
  save_entry = entry;
  g_signal_connect(entry, "activate",
      G_CALLBACK(save_entry_callback), NULL);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", entry,
    "label", _("Save _Filename:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(sbox), entry, FALSE, FALSE, 0);


  g_signal_connect(shell, "response",
      G_CALLBACK(save_response_callback), NULL);
  g_signal_connect(shell, "destroy",
      G_CALLBACK(gtk_widget_destroyed), &save_dialog_shell);

  gtk_dialog_set_response_sensitive(GTK_DIALOG(save_dialog_shell),
      SAVE_DELETE, FALSE);

  gtk_window_set_focus(GTK_WINDOW(shell), entry);

  gtk_widget_show_all(GTK_DIALOG(shell)->vbox);
}

/**************************************************************************
  popup save dialog.
**************************************************************************/
void popup_save_dialog(bool scenario)
{
  if (!save_dialog_shell) {
    create_save_dialog(scenario);
  }
  update_save_dialog();
 
  gtk_window_present(GTK_WINDOW(save_dialog_shell));
}

/****************************************************************************
  Set the list of available rulesets.  The default ruleset should be
  "default", and if the user changes this then set_ruleset() should be
  called.
****************************************************************************/
void gui_set_rulesets(int num_rulesets, char **rulesets)
{
  int i;
  GList *opts = NULL;

  for (i = 0; i < num_rulesets; i++){
    opts = g_list_append(opts, rulesets[i]);
  }

  no_ruleset_callback = TRUE;
  gtk_combo_set_popdown_strings(GTK_COMBO(ruleset_combo), opts);

  /* HACK: server should tell us the current ruleset. */
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ruleset_combo)->entry),
		     "default");
  no_ruleset_callback = FALSE;

  g_list_free(opts);
}
