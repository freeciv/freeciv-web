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

#include "fc_prehdrs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
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

/* utility */
#include "capstr.h"
#include "dataio.h"
#include "fcintl.h"
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

extern char forced_tileset_name[512];
static struct fc_sockaddr_list *list = NULL;
static int name_count;

/**********************************************************************//**
  Close socket and cleanup.  This one doesn't print a message, so should
  do so before-hand if necessary.
**************************************************************************/
static void close_socket_nomessage(struct connection *pc)
{
  connection_common_close(pc);
  remove_net_input();
  popdown_races_dialog(); 
  close_connection_dialog();

  set_client_state(C_S_DISCONNECTED);
}

/**********************************************************************//**
  Client connection close socket callback. It shouldn't be called directy.
  Use connection_close() instead.
**************************************************************************/
static void client_conn_close_callback(struct connection *pconn)
{
  char reason[256];

  if (NULL != pconn->closing_reason) {
    fc_strlcpy(reason, pconn->closing_reason, sizeof(reason));
  } else {
    fc_strlcpy(reason, _("unknown reason"), sizeof(reason));
  }

  close_socket_nomessage(pconn);
  /* If we lost connection to the internal server - kill it. */
  client_kill_server(TRUE);
  log_error("Lost connection to server: %s.", reason);
  output_window_printf(ftc_client, _("Lost connection to server (%s)!"),
                       reason);
}

/**********************************************************************//**
  Get ready to [try to] connect to a server:
   - translate HOSTNAME and PORT (with defaults of "localhost" and
     DEFAULT_SOCK_PORT respectively) to a raw IP address and port number, and
     store them in the `names' variable
   - return 0 on success
     or put an error message in ERRBUF and return -1 on failure
**************************************************************************/
static int get_server_address(const char *hostname, int port,
                              char *errbuf, int errbufsize)
{
  if (port == 0) {
#ifdef FREECIV_JSON_CONNECTION
    port = FREECIV_JSON_PORT;
#else  /* FREECIV_JSON_CONNECTION */
    port = DEFAULT_SOCK_PORT;
#endif /* FREECIV_JSON_CONNECTION */
  }

  /* use name to find TCP/IP address of server */
  if (!hostname) {
    hostname = "localhost";
  }

  if (list != NULL) {
    fc_sockaddr_list_destroy(list);
  }

  /* Any supported family will do */
  list = net_lookup_service(hostname, port, FC_ADDR_ANY);

  name_count = fc_sockaddr_list_size(list);

  if (name_count <= 0) {
    (void) fc_strlcpy(errbuf, _("Failed looking up host."), errbufsize);
    return -1;
  }

  return 0;
}

/**********************************************************************//**
  Try to connect to a server (get_server_address() must be called first!):
   - try to create a TCP socket and connect it to `names'
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
  int sock = -1;
  fc_errno err = 0;

  connections_set_close_callback(client_conn_close_callback);

  /* connection in progress? wait. */
  if (client.conn.used) {
    (void) fc_strlcpy(errbuf, _("Connection in progress."), errbufsize);
    return -1;
  }

  /* Try all (IPv4, IPv6, ...) addresses until we have a connection. */
  sock = -1;
  fc_sockaddr_list_iterate(list, paddr) {
    if ((sock = socket(paddr->saddr.sa_family, SOCK_STREAM, 0)) == -1) {
      if (err == 0) {
        err = fc_get_errno();
      }
      /* Probably EAFNOSUPPORT or EPROTONOSUPPORT. */
      continue;
    }

    if (fc_connect(sock, &paddr->saddr,
                   sockaddr_size(paddr)) == -1) {
      err = fc_get_errno(); /* Save errno value before calling anything */
      fc_closesocket(sock);
      sock = -1;
      continue;
    } else {
      /* We have a connection! */
      break;
    }
  } fc_sockaddr_list_iterate_end;

  client.conn.sock = sock;
  if (client.conn.sock == -1) {
    (void) fc_strlcpy(errbuf, fc_strerror(err), errbufsize);
#ifdef FREECIV_HAVE_WINSOCK
    return -1;
#else
    return err;
#endif /* FREECIV_HAVE_WINSOCK */
  }

  make_connection(client.conn.sock, username);

  return 0;
}

/**********************************************************************//**
  Connect to a freeciv-server instance -- or at least try to.  On success,
  return 0; on failure, put an error message in ERRBUF and return -1.
**************************************************************************/
int connect_to_server(const char *username, const char *hostname, int port,
                      char *errbuf, int errbufsize)
{
  if (errbufsize > 0 && errbuf != NULL) {
    errbuf[0] = '\0';
  }

  if (0 != get_server_address(hostname, port, errbuf, errbufsize)) {
    return -1;
  }

  if (0 != try_to_connect(username, errbuf, errbufsize)) {
    return -1;
  }

  if (gui_options.use_prev_server) {
    sz_strlcpy(gui_options.default_server_host, hostname);
    gui_options.default_server_port = port;
  }

  return 0;
}

/**********************************************************************//**
  Called after a connection is completed (e.g., in try_to_connect).
**************************************************************************/
void make_connection(int sock, const char *username)
{
  struct packet_server_join_req req;

  connection_common_init(&client.conn);
  client.conn.sock = sock;
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

/**********************************************************************//**
  Get rid of server connection. This also kills internal server if it's
  used.
**************************************************************************/
void disconnect_from_server(void)
{
  const bool force = !client.conn.used;

  attribute_flush();

  stop_turn_change_wait();

  /* If it's internal server - kill him 
   * We assume that we are always connected to the internal server  */
  if (!force) {
    client_kill_server(FALSE);
  }
  close_socket_nomessage(&client.conn);
  if (force) {
    client_kill_server(TRUE);
  }
  output_window_append(ftc_client, _("Disconnected from server."));

  if (gui_options.save_options_on_exit) {
    options_save(NULL);
  }
}

/**********************************************************************//**
  A wrapper around read_socket_data() which also handles the case the
  socket becomes writeable and there is still data which should be sent
  to the server.

  Returns:
    -1  :  an error occurred - you should close the socket
    -2  :  the connection was closed
    >0  :  number of bytes read
    =0  :  no data read, would block
**************************************************************************/
static int read_from_connection(struct connection *pc, bool block)
{
  for (;;) {
    fd_set readfs, writefs, exceptfs;
    int socket_fd = pc->sock;
    bool have_data_for_server = (pc->used && pc->send_buffer
                                 && 0 < pc->send_buffer->ndata);
    int n;
    fc_timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FC_FD_ZERO(&readfs);
    FD_SET(socket_fd, &readfs);

    FC_FD_ZERO(&exceptfs);
    FD_SET(socket_fd, &exceptfs);

    if (have_data_for_server) {
      FC_FD_ZERO(&writefs);
      FD_SET(socket_fd, &writefs);
      n = fc_select(socket_fd + 1, &readfs, &writefs, &exceptfs,
                    block ? NULL : &tv);
    } else {
      n = fc_select(socket_fd + 1, &readfs, NULL, &exceptfs,
                    block ? NULL : &tv);
    }

    /* the socket is neither readable, writeable nor got an
     * exception */
    if (n == 0) {
      return 0;
    }

    if (n == -1) {
      if (errno == EINTR) {
        /* EINTR can happen sometimes, especially when compiling with -pg.
         * Generally we just want to run select again. */
        log_debug("select() returned EINTR");
        continue;
      }

      log_error("select() return=%d errno=%d (%s)",
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

/**********************************************************************//**
  This function is called when the client received a new input from the
  server.
**************************************************************************/
void input_from_server(int fd)
{
  int nb;

  fc_assert_ret(fd == client.conn.sock);

  nb = read_from_connection(&client.conn, FALSE);
  if (0 <= nb) {
    agents_freeze_hint();
    while (client.conn.used) {
      enum packet_type type;
      void *packet = get_packet_from_connection(&client.conn, &type);

      if (NULL != packet) {
	client_packet_input(packet, type);
	free(packet);
      } else {
	break;
      }
    }
    if (client.conn.used) {
      agents_thaw_hint();
    }
  } else if (-2 == nb) {
    connection_close(&client.conn, _("server disconnected"));
  } else {
    connection_close(&client.conn, _("read error"));
  }
}

/**********************************************************************//**
  This function will sniff at the given fd, get the packet and call
  client_packet_input. It will return if there is a network error or if
  the PACKET_PROCESSING_FINISHED packet for the given request is
  received.
**************************************************************************/
void input_from_server_till_request_got_processed(int fd, 
						  int expected_request_id)
{
  fc_assert_ret(expected_request_id);
  fc_assert_ret(fd == client.conn.sock);

  log_debug("input_from_server_till_request_got_processed("
            "expected_request_id=%d)", expected_request_id);

  while (TRUE) {
    int nb = read_from_connection(&client.conn, TRUE);

    if (0 <= nb) {
      enum packet_type type;

      while (TRUE) {
	void *packet = get_packet_from_connection(&client.conn, &type);
	if (NULL == packet) {
	  break;
	}

	client_packet_input(packet, type);
	free(packet);

	if (type == PACKET_PROCESSING_FINISHED) {
          log_debug("ifstrgp: expect=%d, seen=%d",
                    expected_request_id,
                    client.conn.client.last_processed_request_id_seen);
	  if (client.conn.client.last_processed_request_id_seen >=
	      expected_request_id) {
            log_debug("ifstrgp: got it; returning");
	    return;
	  }
	}
      }
    } else if (-2 == nb) {
      connection_close(&client.conn, _("server disconnected"));
      break;
    } else {
      connection_close(&client.conn, _("read error"));
      break;
    }
  }
}

static bool autoconnecting = FALSE;
/**********************************************************************//**
  Make an attempt to autoconnect to the server.
  It returns number of seconds it should be called again.
**************************************************************************/
double try_to_autoconnect(void)
{
  char errbuf[512];
  static int count = 0;
#ifndef FREECIV_MSWINDOWS
  static int warning_shown = 0;
#endif

  if (!autoconnecting) {
    return FC_INFINITY;
  }

  count++;

  if (count >= MAX_AUTOCONNECT_ATTEMPTS) {
    log_fatal(_("Failed to contact server \"%s\" at port "
                "%d as \"%s\" after %d attempts"),
              server_host, server_port, user_name, count);
    exit(EXIT_FAILURE);
  }

  switch (try_to_connect(user_name, errbuf, sizeof(errbuf))) {
  case 0:			/* Success! */
    /* Don't call me again */
    autoconnecting = FALSE;
    return FC_INFINITY;
#ifndef FREECIV_MSWINDOWS
  /* See PR#4042 for more info on issues with try_to_connect() and errno. */
  case ECONNREFUSED:		/* Server not available (yet) */
    if (!warning_shown) {
      log_error("Connection to server refused. Please start the server.");
      output_window_append(ftc_client, _("Connection to server refused. "
                                         "Please start the server."));
      warning_shown = 1;
    }
    /* Try again in 0.5 seconds */
    return 0.001 * AUTOCONNECT_INTERVAL;
#endif /* FREECIV_MSWINDOWS */
  default:			/* All other errors are fatal */
    log_fatal(_("Error contacting server \"%s\" at port %d "
                "as \"%s\":\n %s\n"),
              server_host, server_port, user_name, errbuf);
    exit(EXIT_FAILURE);
  }
}

/**********************************************************************//**
  Start trying to autoconnect to freeciv-server.  Calls
  get_server_address(), then arranges for try_to_autoconnect(), which
  calls try_to_connect(), to be called roughly every
  AUTOCONNECT_INTERVAL milliseconds, until success, fatal error or
  user intervention.
**************************************************************************/
void start_autoconnecting_to_server(void)
{
  char buf[512];

  output_window_printf(ftc_client,
                       _("Auto-connecting to server \"%s\" at port %d "
                         "as \"%s\" every %f second(s) for %d times"),
                       server_host, server_port, user_name,
                       0.001 * AUTOCONNECT_INTERVAL,
                       MAX_AUTOCONNECT_ATTEMPTS);

  if (get_server_address(server_host, server_port, buf, sizeof(buf)) < 0) {
    log_fatal(_("Error contacting server \"%s\" at port %d "
                "as \"%s\":\n %s\n"),
              server_host, server_port, user_name, buf);
    exit(EXIT_FAILURE);
  }
  autoconnecting = TRUE;
}
