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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif

/* utility */
#include "capstr.h"
#include "dataio.h"
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "registry.h"
#include "support.h"

/* common */
#include "game.h"
#include "packets.h"
#include "version.h"

/* client */
#include "agents.h"
#include "attribute.h"
#include "chatline_g.h"
#include "client_main.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "connectdlg_g.h"
#include "dialogs_g.h"		/* popdown_races_dialog() */
#include "ggzclient.h"
#include "ggz_g.h"
#include "gui_main_g.h"		/* add_net_input(), remove_net_input() */
#include "mapview_common.h"	/* unqueue_mapview_update */
#include "menu_g.h"
#include "messagewin_g.h"
#include "options.h"
#include "packhand.h"
#include "pages_g.h"
#include "plrdlg_g.h"
#include "repodlgs_g.h"

#include "clinet.h"

/* In autoconnect mode, try to connect to once a second */
#define AUTOCONNECT_INTERVAL		500

/* In autoconnect mode, try to connect 100 times */
#define MAX_AUTOCONNECT_ATTEMPTS	100

static union fc_sockaddr server_addr;

/*************************************************************************
  Close socket and cleanup.  This one doesn't print a message, so should
  do so before-hand if necessary.
**************************************************************************/
static void close_socket_nomessage(struct connection *pc)
{
  if (with_ggz || in_ggz) {
    remove_ggz_input();
  }
  if (in_ggz) {
    gui_ggz_embed_leave_table();
  }
  connection_common_close(pc);
  remove_net_input();
  popdown_races_dialog(); 
  close_connection_dialog();

  reports_force_thaw();
  
  set_client_state(C_S_DISCONNECTED);
}

/**************************************************************************
...
**************************************************************************/
static void close_socket_callback(struct connection *pc)
{
  close_socket_nomessage(pc);
  /* If we lost connection to the internal server - kill him */
  client_kill_server(TRUE);
  freelog(LOG_ERROR, "Lost connection to server!");
  output_window_append(FTC_CLIENT_INFO, NULL,
                       _("Lost connection to server!"));
  if (with_ggz) {
    client_exit();
  }
}

/**************************************************************************
  Get ready to [try to] connect to a server:
   - translate HOSTNAME and PORT (with defaults of "localhost" and
     DEFAULT_SOCK_PORT respectively) to a raw IP address and port number, and
     store them in the `server_addr' variable
   - return 0 on success
     or put an error message in ERRBUF and return -1 on failure
**************************************************************************/
static int get_server_address(const char *hostname, int port,
                              char *errbuf, int errbufsize)
{
  if (port == 0)
    port = DEFAULT_SOCK_PORT;

  /* use name to find TCP/IP address of server */
  if (!hostname)
    hostname = "localhost";

  if (!net_lookup_service(hostname, port, &server_addr, FALSE)) {
    (void) mystrlcpy(errbuf, _("Failed looking up host."), errbufsize);
    return -1;
  }

  return 0;
}

/**************************************************************************
  Try to connect to a server (get_server_address() must be called first!):
   - try to create a TCP socket and connect it to `server_addr'
   - if successful:
	  - start monitoring the socket for packets from the server
	  - send a "login request" packet to the server
      and - return 0
   - if unable to create the connection, close the socket, put an error
     message in ERRBUF and return the Unix error code (ie., errno, which
     will be non-zero).
**************************************************************************/
static int try_to_connect(const char *username, char *errbuf, int errbufsize)
{
  close_socket_set_callback(close_socket_callback);

  /* connection in progress? wait. */
  if (client.conn.used) {
    (void) mystrlcpy(errbuf, _("Connection in progress."), errbufsize);
    return -1;
  }
  
  if ((client.conn.sock = socket(server_addr.saddr.sa_family,
                                 SOCK_STREAM, 0)) == -1) {
    (void) mystrlcpy(errbuf, fc_strerror(fc_get_errno()), errbufsize);
    return -1;
  }

  if (fc_connect(client.conn.sock, &server_addr.saddr,
                 sockaddr_size(&server_addr)) == -1) {
    (void) mystrlcpy(errbuf, fc_strerror(fc_get_errno()), errbufsize);
    fc_closesocket(client.conn.sock);
    client.conn.sock = -1;
#ifdef HAVE_WINSOCK
    return -1;
#else
    return errno;
#endif
  }

  make_connection(client.conn.sock, username);

  return 0;
}

/**************************************************************************
  Connect to a civserver instance -- or at least try to.  On success,
  return 0; on failure, put an error message in ERRBUF and return -1.
**************************************************************************/
int connect_to_server(const char *username, const char *hostname, int port,
		      char *errbuf, int errbufsize)
{
  if (0 != get_server_address(hostname, port, errbuf, errbufsize)) {
    return -1;
  }

  if (0 != try_to_connect(username, errbuf, errbufsize)) {
    return -1;
  }

  return 0;
}

/**************************************************************************
  Called after a connection is completed (e.g., in try_to_connect).
**************************************************************************/
void make_connection(int socket, const char *username)
{
  struct packet_server_join_req req;

  connection_common_init(&client.conn);
  client.conn.sock = socket;
  client.conn.is_server = FALSE;
  client.conn.client.last_request_id_used = 0;
  client.conn.client.last_processed_request_id_seen = 0;
  client.conn.client.request_id_of_currently_handled_packet = 0;
  client.conn.incoming_packet_notify = notify_about_incoming_packet;
  client.conn.outgoing_packet_notify = notify_about_outgoing_packet;

  /* call gui-dependent stuff in gui_main.c */
  add_net_input(client.conn.sock);

  /* now send join_request package */

  req.major_version = MAJOR_VERSION;
  req.minor_version = MINOR_VERSION;
  req.patch_version = PATCH_VERSION;
  sz_strlcpy(req.version_label, VERSION_LABEL);
  sz_strlcpy(req.capability, our_capability);
  sz_strlcpy(req.username, username);
  
  send_packet_server_join_req(&client.conn, &req);
}

/**************************************************************************
...
**************************************************************************/
void disconnect_from_server(void)
{
  const bool force = !client.conn.used;

  attribute_flush();
  /* If it's internal server - kill him 
   * We assume that we are always connected to the internal server  */
  if (!force) {
    client_kill_server(FALSE);
  }
  close_socket_nomessage(&client.conn);
  if (force) {
    client_kill_server(TRUE);
  }
  output_window_append(FTC_CLIENT_INFO, NULL,
                       _("Disconnected from server."));
  if (with_ggz) {
    client_exit();
  }
  if (save_options_on_exit) {
    save_options();
  }
}  

/**************************************************************************
A wrapper around read_socket_data() which also handles the case the
socket becomes writeable and there is still data which should be sent
to the server.

Returns:
    -1  :  an error occurred - you should close the socket
    >0  :  number of bytes read
    =0  :  no data read, would block
**************************************************************************/
static int read_from_connection(struct connection *pc, bool block)
{
  for (;;) {
    fd_set readfs, writefs, exceptfs;
    int socket_fd = pc->sock;
    bool have_data_for_server = (pc->used && pc->send_buffer
				&& pc->send_buffer->ndata > 0);
    int n;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    MY_FD_ZERO(&readfs);
    FD_SET(socket_fd, &readfs);

    MY_FD_ZERO(&exceptfs);
    FD_SET(socket_fd, &exceptfs);

    if (have_data_for_server) {
      MY_FD_ZERO(&writefs);
      FD_SET(socket_fd, &writefs);
      n =
	  fc_select(socket_fd + 1, &readfs, &writefs, &exceptfs,
		    block ? NULL : &tv);
    } else {
      n =
	  fc_select(socket_fd + 1, &readfs, NULL, &exceptfs,
		    block ? NULL : &tv);
    }

    /* the socket is neither readable, writeable nor got an
       exception */
    if (n == 0) {
      return 0;
    }

    if (n == -1) {
      if (errno == EINTR) {
	/* EINTR can happen sometimes, especially when compiling with -pg.
	 * Generally we just want to run select again. */
	freelog(LOG_DEBUG, "select() returned EINTR");
	continue;
      }

      freelog(LOG_ERROR, "select() return=%d errno=%d (%s)",
	      n, errno, fc_strerror(fc_get_errno()));
      return -1;
    }

    if (FD_ISSET(socket_fd, &exceptfs)) {
      return -1;
    }

    if (have_data_for_server && FD_ISSET(socket_fd, &writefs)) {
      flush_connection_send_buffer_all(pc);
    }

    if (FD_ISSET(socket_fd, &readfs)) {
      return read_socket_data(socket_fd, pc->buffer);
    }
  }
}

/**************************************************************************
 This function is called when the client received a new input from the
 server.
**************************************************************************/
void input_from_server(int fd)
{
  assert(fd == client.conn.sock);

  if (read_from_connection(&client.conn, FALSE) >= 0) {
    enum packet_type type;

    while (TRUE) {
      bool result;
      void *packet = get_packet_from_connection(&client.conn,
						&type, &result);

      if (result) {
	assert(packet != NULL);
	client_packet_input(packet, type);
	free(packet);
      } else {
	assert(packet == NULL);
	break;
      }
    }
  } else {
    close_socket_callback(&client.conn);
  }
}

/**************************************************************************
 This function will sniff at the given fd, get the packet and call
 client_packet_input. It will return if there is a network error or if
 the PACKET_PROCESSING_FINISHED packet for the given request is
 received.
**************************************************************************/
void input_from_server_till_request_got_processed(int fd, 
						  int expected_request_id)
{
  assert(expected_request_id);
  assert(fd == client.conn.sock);

  freelog(LOG_DEBUG,
	  "input_from_server_till_request_got_processed("
	  "expected_request_id=%d)", expected_request_id);

  while (TRUE) {
    if (read_from_connection(&client.conn, TRUE) >= 0) {
      enum packet_type type;

      while (TRUE) {
	bool result;
	void *packet = get_packet_from_connection(&client.conn,
						  &type, &result);
	if (!result) {
	  assert(packet == NULL);
	  break;
	}

	assert(packet != NULL);
	client_packet_input(packet, type);
	free(packet);

	if (type == PACKET_PROCESSING_FINISHED) {
	  freelog(LOG_DEBUG, "ifstrgp: expect=%d, seen=%d",
		  expected_request_id,
		  client.conn.client.last_processed_request_id_seen);
	  if (client.conn.client.last_processed_request_id_seen >=
	      expected_request_id) {
	    freelog(LOG_DEBUG, "ifstrgp: got it; returning");
	    return;
	  }
	}
      }
    } else {
      close_socket_callback(&client.conn);
      break;
    }
  }
}

static bool autoconnecting = FALSE;
/**************************************************************************
  Make an attempt to autoconnect to the server.
  It returns number of seconds it should be called again.
**************************************************************************/
double try_to_autoconnect(void)
{
  char errbuf[512];
  static int count = 0;
#ifndef WIN32_NATIVE
  static int warning_shown = 0;
#endif

  if (!autoconnecting) {
    return FC_INFINITY;
  }
  
  count++;

  if (count >= MAX_AUTOCONNECT_ATTEMPTS) {
    freelog(LOG_FATAL,
	    _("Failed to contact server \"%s\" at port "
	      "%d as \"%s\" after %d attempts"),
	    server_host, server_port, user_name, count);
    exit(EXIT_FAILURE);
  }

  switch (try_to_connect(user_name, errbuf, sizeof(errbuf))) {
  case 0:			/* Success! */
    /* Don't call me again */
    autoconnecting = FALSE;
    return FC_INFINITY;
#ifndef WIN32_NATIVE
  /* See PR#4042 for more info on issues with try_to_connect() and errno. */
  case ECONNREFUSED:		/* Server not available (yet) */
    if (!warning_shown) {
      freelog(LOG_ERROR, "Connection to server refused. "
                         "Please start the server.");
      output_window_append(FTC_CLIENT_INFO, NULL,
                           _("Connection to server refused. "
                             "Please start the server."));
      warning_shown = 1;
    }
    /* Try again in 0.5 seconds */
    return 0.001 * AUTOCONNECT_INTERVAL;
#endif
  default:			/* All other errors are fatal */
    freelog(LOG_FATAL,
	    _("Error contacting server \"%s\" at port %d "
	      "as \"%s\":\n %s\n"),
	    server_host, server_port, user_name, errbuf);
    exit(EXIT_FAILURE);
  }
}

/**************************************************************************
  Start trying to autoconnect to civserver.  Calls
  get_server_address(), then arranges for try_to_autoconnect(), which
  calls try_to_connect(), to be called roughly every
  AUTOCONNECT_INTERVAL milliseconds, until success, fatal error or
  user intervention.
**************************************************************************/
void start_autoconnecting_to_server(void)
{
  char buf[512];

  output_window_printf(FTC_CLIENT_INFO, NULL,
                       _("Auto-connecting to server \"%s\" at port %d "
                         "as \"%s\" every %f second(s) for %d times"),
                       server_host, server_port, user_name,
                       0.001 * AUTOCONNECT_INTERVAL,
                       MAX_AUTOCONNECT_ATTEMPTS);

  if (get_server_address(server_host, server_port, buf, sizeof(buf)) < 0) {
    freelog(LOG_FATAL,
            _("Error contacting server \"%s\" at port %d "
              "as \"%s\":\n %s\n"),
            server_host, server_port, user_name, buf);
    exit(EXIT_FAILURE);
  }
  autoconnecting = TRUE;
}
