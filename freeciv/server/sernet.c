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
#include <time.h>

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
#ifdef HAVE_LIBREADLINE
#include <readline/history.h>
#include <readline/readline.h>
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif

#include "fciconv.h"

#include "capability.h"
#include "dataio.h"
#include "events.h"
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "packets.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

#include "auth.h"
#include "connecthand.h"
#include "console.h"
#include "ggzserver.h"
#include "meta.h"
#include "plrhand.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "voting.h"

#include "sernet.h"

static struct connection connections[MAX_NUM_CONNECTIONS];

#ifdef GENERATING_MAC      /* mac network globals */
TEndpointInfo serv_info;
EndpointRef serv_ep;
#else
static int sock;
static int socklan;
#endif

#if defined(__VMS)
#  if defined(_VAX_)
#    define lib$stop LIB$STOP
#    define sys$qiow SYS$QIOW
#    define sys$assign SYS$ASSIGN
#  endif
#  include <descrip.h>
#  include <iodef.h>
#  include <stsdef.h>
#  include <starlet.h>
#  include <lib$routines.h>
#  include <efndef.h>
   static unsigned long int tt_chan;
   static char input_char = 0;
   static char got_input = 0;
   void user_interrupt_callback();
#endif

#define SPECLIST_TAG timer
#define SPECLIST_TYPE struct timer
#include "speclist.h"
#define timer_list_iterate(ARG_list, NAME_item) \
  TYPED_LIST_ITERATE(struct timer, (ARG_list), NAME_item)
#define timer_list_iterate_end LIST_ITERATE_END

#define PROCESSING_TIME_STATISTICS 0

static int server_accept_connection(int sockfd);
static void start_processing_request(struct connection *pconn,
				     int request_id);
static void finish_processing_request(struct connection *pconn);
static void ping_connection(struct connection *pconn);
static void send_ping_times_to_all(void);

static void get_lanserver_announcement(void);
static void send_lanserver_response(void);

static bool no_input = FALSE;

/* Avoid compiler warning about defined, but unused function
 * by defining it only when needed */
#if defined(HAVE_LIBREADLINE) || \
    (!defined(SOCKET_ZERO_ISNT_STDIN) && !defined(HAVE_LIBREADLINE))  
/*****************************************************************************
  This happens if you type an EOF character with nothing on the current line.
*****************************************************************************/
static void handle_stdin_close(void)
{
  /* Note this function may be called even if SOCKET_ZERO_ISNT_STDIN, so
   * the preprocessor check has to come inside the function body.  But
   * perhaps we want to do this even when SOCKET_ZERO_ISNT_STDIN? */
#ifndef SOCKET_ZERO_ISNT_STDIN
  freelog(LOG_NORMAL, _("Server cannot read standard input. Ignoring input."));
  no_input = TRUE;
#endif /* SOCKET_ZERO_ISNT_STDIN */
}

#endif /* HAVE_LIBREADLINE || (!SOCKET_ZERO_ISNT_STDIN && !HAVE_LIBREADLINE) */

#ifdef HAVE_LIBREADLINE
/****************************************************************************/

#define HISTORY_FILENAME  ".civserver_history"
#define HISTORY_LENGTH    100

static char *history_file = NULL;

static bool readline_handled_input = FALSE;

static bool readline_initialized = FALSE;

/*****************************************************************************
...
*****************************************************************************/
static void handle_readline_input_callback(char *line)
{
  char *line_internal;

  if (no_input)
    return;

  if (!line) {
    handle_stdin_close();	/* maybe print an 'are you sure?' message? */
    return;
  }

  if (line[0] != '\0')
    add_history(line);

  con_prompt_enter();		/* just got an 'Enter' hit */
  line_internal = local_to_internal_string_malloc(line);
  (void) handle_stdin_input(NULL, line_internal, FALSE);
  free(line_internal);
  free(line);

  readline_handled_input = TRUE;
}

#endif /* HAVE_LIBREADLINE */
/*****************************************************************************
...
*****************************************************************************/
void close_connection(struct connection *pconn)
{
  if (!pconn) {
    return;
  }

  cancel_connection_votes(pconn);

  cancel_connection_votes(pconn);

  if (pconn->server.ping_timers != NULL) {
    timer_list_iterate(pconn->server.ping_timers, timer) {
      free_timer(timer);
    } timer_list_iterate_end;
    timer_list_free(pconn->server.ping_timers);
    pconn->server.ping_timers = NULL;
  }

  /* safe to do these even if not in lists: */
  conn_list_unlink(game.all_connections, pconn);
  conn_list_unlink(game.est_connections, pconn);

  pconn->playing = NULL;
  pconn->access_level = ALLOW_NONE;
  connection_common_close(pconn);

  send_updated_vote_totals(NULL);
}

/*****************************************************************************
...
*****************************************************************************/
void close_connections_and_socket(void)
{
  int i;
  lsend_packet_server_shutdown(game.all_connections);

  for(i=0; i<MAX_NUM_CONNECTIONS; i++) {
    if(connections[i].used) {
      close_connection(&connections[i]);
    }
    conn_list_free(connections[i].self);
  }

  /* Remove the game connection lists and make sure they are empty. */
  conn_list_free(game.all_connections);
  conn_list_free(game.est_connections);

  fc_closesocket(sock);
  fc_closesocket(socklan);

#ifdef HAVE_LIBREADLINE
  if (history_file) {
    write_history(history_file);
    history_truncate_file(history_file, HISTORY_LENGTH);
  }
#endif

  send_server_info_to_metaserver(META_GOODBYE);
  server_close_meta();

  fc_shutdown_network();
}

/*****************************************************************************
  Used for errors and other strangeness.  As well as some direct uses, is
  passed to packet routines as callback for when packet sending has a write
  error.  Closes the connection cleanly, calling lost_connection_to_client()
  to clean up server variables, notify other clients, etc.
*****************************************************************************/
static void close_socket_callback(struct connection *pc)
{
  lost_connection_to_client(pc);
  close_connection(pc);
}

/****************************************************************************
  If a connection lags too much this function is called and we try to cut
  it.
****************************************************************************/
static void cut_lagging_connection(struct connection *pconn)
{
  if (game.info.tcptimeout != 0
      && pconn->last_write
      && conn_list_size(game.all_connections) > 1
      && pconn->access_level != ALLOW_HACK
      && read_timer_seconds(pconn->last_write) > game.info.tcptimeout) {
    /* Cut the connections to players who lag too much.  This
     * usually happens because client animation slows the client
     * too much and it can't keep up with the server.  We don't
     * cut HACK connections, or cut in single-player games, since
     * it wouldn't help the game progress.  For other connections
     * the best thing to do when they lag too much is to be
     * disconnected and reconnect. */
    freelog(LOG_ERROR, "connection (%s) cut due to lagging player",
	    conn_description(pconn));
    close_socket_callback(pconn);
  }
}

/****************************************************************************
  Attempt to flush all information in the send buffers for upto 'netwait'
  seconds.
*****************************************************************************/
void flush_packets(void)
{
  int i;
  int max_desc;
  fd_set writefs, exceptfs;
  struct timeval tv;
  time_t start;

  (void) time(&start);

  for(;;) {
    tv.tv_sec = (game.info.netwait - (time(NULL) - start));
    tv.tv_usec=0;

    if (tv.tv_sec < 0)
      return;

    MY_FD_ZERO(&writefs);
    MY_FD_ZERO(&exceptfs);
    max_desc=-1;

    for(i=0; i<MAX_NUM_CONNECTIONS; i++) {
      struct connection *pconn = &connections[i];
      if(pconn->used && pconn->send_buffer->ndata > 0) {
	FD_SET(pconn->sock, &writefs);
	FD_SET(pconn->sock, &exceptfs);
	max_desc=MAX(pconn->sock, max_desc);
      }
    }

    if (max_desc == -1) {
      return;
    }

    if(fc_select(max_desc+1, NULL, &writefs, &exceptfs, &tv)<=0) {
      return;
    }

    for(i=0; i<MAX_NUM_CONNECTIONS; i++) {   /* check for freaky players */
      struct connection *pconn = &connections[i];
      if(pconn->used) {
        if(FD_ISSET(pconn->sock, &exceptfs)) {
	  freelog(LOG_ERROR, "connection (%s) cut due to exception data",
		  conn_description(pconn));
	  close_socket_callback(pconn);
        } else {
	  if(pconn->send_buffer && pconn->send_buffer->ndata > 0) {
	    if(FD_ISSET(pconn->sock, &writefs)) {
	      flush_connection_send_buffer_all(pconn);
	    } else {
	      cut_lagging_connection(pconn);
	    }
	  }
        }
      }
    }
  }
}

struct packet_to_handle {
  void *data;
  enum packet_type type;
};

/*****************************************************************************
  Simplify a loop by wrapping get_packet_from_connection.
*****************************************************************************/
static bool get_packet(struct connection *pconn, 
                       struct packet_to_handle *ppacket)
{
  bool got_packet;

  ppacket->data = get_packet_from_connection(pconn, &ppacket->type, 
                                             &got_packet);
  return got_packet;
}

/*****************************************************************************
  Handle all incoming packets on a client connection.
  Precondition - we have read_socket_data.
  Postcondition - there are no more packets to handle on this connection.
*****************************************************************************/
static void incoming_client_packets(struct connection *pconn) 
{
  struct packet_to_handle packet;
#if PROCESSING_TIME_STATISTICS
  struct timer *request_time = NULL;
#endif

  while (get_packet(pconn, &packet)) {
    bool command_ok;
    int request_id;

#if PROCESSING_TIME_STATISTICS
    request_time = renew_timer_start(request_time, TIMER_USER, TIMER_ACTIVE);
#endif

    request_id = pconn->server.last_request_id_seen
      = get_next_request_id(pconn->server.last_request_id_seen);

    connection_do_buffer(pconn);
    start_processing_request(pconn, pconn->server.last_request_id_seen);

    command_ok = server_packet_input(pconn, packet.data, packet.type);
    free(packet.data);

    finish_processing_request(pconn);
    connection_do_unbuffer(pconn);

#if PROCESSING_TIME_STATISTICS
    freelog(LOG_VERBOSE,
            "processed request %d in %gms", request_id, 
            read_timer_seconds(request_time) * 1000.0);
#endif

    if (!command_ok) {
      close_connection(pconn);
    }
  }

#if PROCESSING_TIME_STATISTICS
  free_timer(request_time);
#endif
}


/*****************************************************************************
Get and handle:
- new connections,
- input from connections,
- input from server operator in stdin

This function also handles prompt printing, via the con_prompt_*
functions.  That is, other functions should not need to do so.  --dwp
*****************************************************************************/
enum server_events server_sniff_all_input(void)
{
  int i;
  int max_desc;
  fd_set readfs, writefs, exceptfs;
  struct timeval tv;
#ifdef SOCKET_ZERO_ISNT_STDIN
  char *bufptr;    
#endif

  con_prompt_init();

#ifdef HAVE_LIBREADLINE
  {
    if (!no_input && !readline_initialized) {
      char *home_dir = user_home_dir();

      if (home_dir) {
	history_file
	  = fc_malloc(strlen(home_dir) + 1 + strlen(HISTORY_FILENAME) + 1);
	if (history_file) {
	  strcpy(history_file, home_dir);
	  strcat(history_file, "/");
	  strcat(history_file, HISTORY_FILENAME);
	  using_history();
	  read_history(history_file);
	}
      }

      rl_initialize();
      rl_callback_handler_install((char *) "> ",
				  handle_readline_input_callback);
      rl_attempted_completion_function = freeciv_completion;

      readline_initialized = TRUE;
      atexit(rl_callback_handler_remove);
    }
  }
#endif /* HAVE_LIBREADLINE */

  while (TRUE) {
    con_prompt_on();		/* accepting new input */
    
    if (force_end_of_sniff) {
      force_end_of_sniff = FALSE;
      con_prompt_off();
      return S_E_FORCE_END_OF_SNIFF;
    }

    get_lanserver_announcement();

    /* end server if no players for 'srvarg.quitidle' seconds,
     * but only if at least one player has previously connected. */
    if (srvarg.quitidle != 0) {
      static time_t last_noplayers;
      static bool connections;

      if (conn_list_size(game.est_connections) > 0) {
	connections = TRUE;
      }
      if (connections && conn_list_size(game.est_connections) == 0) {
	if (last_noplayers != 0) {
	  if (time(NULL) > last_noplayers + srvarg.quitidle) {
	    //save_game_auto("Lost all connections", "quitidle");
	    set_meta_message_string(N_("restarting for lack of players"));
	    freelog(LOG_NORMAL, "%s", Q_(get_meta_message_string()));
	    (void) send_server_info_to_metaserver(META_INFO);

            set_server_state(S_S_OVER);
            force_end_of_sniff = TRUE;
            conn_list_iterate(game.est_connections, pconn) {
              lost_connection_to_client(pconn);
              close_connection(pconn);
            } conn_list_iterate_end;

	    if (srvarg.exit_on_end) {
	      /* No need for anything more; just quit. */
	      server_quit();
	    }

            /* Do not restart before someone has connected and left again */
            connections = FALSE;
	  }
	} else {
	  last_noplayers = time(NULL);

	  freelog(LOG_NORMAL,
		  _("restarting in %d seconds for lack of players"),
		  srvarg.quitidle);

          set_meta_message_string(N_("restarting soon for lack of players"));
	  (void) send_server_info_to_metaserver(META_INFO);
	}
      } else {
        last_noplayers = 0;
      }
    }

    /* Pinging around for statistics */
    if (time(NULL) > (game.server.last_ping + game.info.pingtime)) {
      /* send data about the previous run */
      send_ping_times_to_all();

      conn_list_iterate(game.all_connections, pconn) {
	if ((timer_list_size(pconn->server.ping_timers) > 0
	     && read_timer_seconds(timer_list_get(pconn->server.ping_timers, 0))
	        > game.info.pingtimeout) 
            || pconn->ping_time > game.info.pingtimeout) {
	  /* cut mute players, except for hack-level ones */
	  if (pconn->access_level == ALLOW_HACK) {
	    freelog(LOG_ERROR,
		    "connection (%s) [hack-level] ping timeout ignored",
		    conn_description(pconn));
	  } else {
	    freelog(LOG_ERROR, "connection (%s) cut due to ping timeout",
		    conn_description(pconn));
	    close_socket_callback(pconn);
	  }
	} else {
	  ping_connection(pconn);
	}
      } conn_list_iterate_end;
      game.server.last_ping = time(NULL);
    }

    /* if we've waited long enough after a failure, respond to the client */
    conn_list_iterate(game.all_connections, pconn) {
      if (srvarg.auth_enabled && pconn->server.status != AS_ESTABLISHED) {
        process_authentication_status(pconn);
      }
    } conn_list_iterate_end

    /* Don't wait if timeout == -1 (i.e. on auto games) */
    if (S_S_INITIAL != server_state() && game.info.timeout == -1) {
      (void) send_server_info_to_metaserver(META_REFRESH);
      return S_E_END_OF_TURN_TIMEOUT;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    MY_FD_ZERO(&readfs);
    MY_FD_ZERO(&writefs);
    MY_FD_ZERO(&exceptfs);

    if (!no_input) {
#ifdef SOCKET_ZERO_ISNT_STDIN
      my_init_console();
#else /* SOCKET_ZERO_ISNT_STDIN */
#   if !defined(__VMS)
      FD_SET(0, &readfs);
#   endif /* VMS */	
#endif /* SOCKET_ZERO_ISNT_STDIN */
    }

    if (with_ggz) {
#ifdef GGZ_SERVER
      int ggz_sock = get_ggz_socket();

      FD_SET(ggz_sock, &readfs);
      max_desc = MAX(sock, ggz_sock);
#endif /* GGZ_SERVER */
    } else {
      FD_SET(sock, &readfs);
      FD_SET(sock, &exceptfs);
      max_desc = sock;
    }

    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      if (connections[i].used) {
	FD_SET(connections[i].sock, &readfs);
	if(connections[i].send_buffer->ndata > 0) {
	  FD_SET(connections[i].sock, &writefs);
	}
	FD_SET(connections[i].sock, &exceptfs);
        max_desc = MAX(connections[i].sock, max_desc);
      }
    }
    con_prompt_off();		/* output doesn't generate a new prompt */

    if (fc_select(max_desc + 1, &readfs, &writefs, &exceptfs, &tv) == 0) {
      /* timeout */
      (void) send_server_info_to_metaserver(META_REFRESH);
      if (game.info.timeout > 0
	  && S_S_RUNNING == server_state()
	  && game.server.phase_timer
	  && (read_timer_seconds(game.server.phase_timer)
	      > game.info.seconds_to_phasedone)) {
	con_prompt_off();
	return S_E_END_OF_TURN_TIMEOUT;
      }

      if (!no_input) {
#if defined(__VMS)
	{
	  struct {
	    short numchars;
	    char firstchar;
	    char reserved;
	    int reserved2;
	  } ttchar;
	  unsigned long status;

	  status = sys$qiow(EFN$C_ENF, tt_chan,
			    IO$_SENSEMODE | IO$M_TYPEAHDCNT, 0, 0, 0,
			    &ttchar, sizeof(ttchar), 0, 0, 0, 0);
	  if (!$VMS_STATUS_SUCCESS(status)) {
	    lib$stop(status);
	  }
	  if (ttchar.numchars) {
	    FD_SET(0, &readfs);
	  } else {
	    continue;
	  }
	}
#else  /* !__VMS */
#ifndef SOCKET_ZERO_ISNT_STDIN
      continue;
#endif /* SOCKET_ZERO_ISNT_STDIN */
#endif /* !__VMS */
      }
    }

    if (!with_ggz) { /* No listening socket when using GGZ. */
      if (FD_ISSET(sock, &exceptfs)) {	     /* handle Ctrl-Z suspend/resume */
	continue;
      }
      if(FD_ISSET(sock, &readfs)) {	     /* new players connects */
	freelog(LOG_VERBOSE, "got new connection");
	if(server_accept_connection(sock)==-1) {
	  freelog(LOG_ERROR, "failed accepting connection");
	}
      }
    }
    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      /* check for freaky players */
      struct connection *pconn = &connections[i];

      if (pconn->used && FD_ISSET(pconn->sock, &exceptfs)) {
 	freelog(LOG_ERROR, "connection (%s) cut due to exception data",
		conn_description(pconn));
	close_socket_callback(pconn);
      }
    }
#ifdef GGZ_SERVER
    if (with_ggz) {
      /* This is intentionally after all the player socket handling because
       * it may cut a client. */
      int ggz_sock = get_ggz_socket();

      if (FD_ISSET(ggz_sock, &readfs)) {
	input_from_ggz(ggz_sock);
      }
    }
#endif /* GGZ_SERVER */
    
#ifdef SOCKET_ZERO_ISNT_STDIN
    if (!no_input && (bufptr = my_read_console())) {
      char *bufptr_internal = local_to_internal_string_malloc(bufptr);

      con_prompt_enter();	/* will need a new prompt, regardless */
      handle_stdin_input(NULL, bufptr_internal, FALSE);
      free(bufptr_internal);
    }
#else  /* !SOCKET_ZERO_ISNT_STDIN */
    if(!no_input && FD_ISSET(0, &readfs)) {    /* input from server operator */
#ifdef HAVE_LIBREADLINE
      rl_callback_read_char();
      if (readline_handled_input) {
	readline_handled_input = FALSE;
	con_prompt_enter_clear();
      }
      continue;
#else  /* !HAVE_LIBREADLINE */
      ssize_t didget;
      char *buffer = NULL; /* Must be NULL when calling getline() */
      char *buf_internal;

#ifdef HAVE_GETLINE
      size_t len = 0;

      didget = getline(&buffer, &len, stdin);
      if (didget >= 1) {
        buffer[didget-1] = '\0'; /* overwrite newline character */
        didget--;
        freelog(LOG_DEBUG, "Got line: \"%s\" (%ld, %ld)", buffer,
                (long int) didget, (long int) len);
      }
#else  /* HAVE_GETLINE */
      buffer = malloc(BUF_SIZE + 1);

      didget = read(0, buffer, BUF_SIZE);
      if (didget < 0) {
        didget = 0; /* Avoid buffer underrun below. */
      }
      *(buffer+didget)='\0';
#endif /* HAVE_GETLINE */
      if (didget <= 0) {
        handle_stdin_close();
      }

      con_prompt_enter();	/* will need a new prompt, regardless */

      if (didget >= 0) {
        buf_internal = local_to_internal_string_malloc(buffer);
        handle_stdin_input(NULL, buf_internal, FALSE);
        free(buf_internal);
      }
      free(buffer);
#endif /* !HAVE_LIBREADLINE */
    } else
#endif /* !SOCKET_ZERO_ISNT_STDIN */
     
    {                             /* input from a player */
      for(i = 0; i < MAX_NUM_CONNECTIONS; i++) {
  	struct connection *pconn = &connections[i];

        if (!pconn->used || !FD_ISSET(pconn->sock, &readfs)) {
          continue;
	}

        if (read_socket_data(pconn->sock, pconn->buffer) >= 0) {
          /* We read packets; now handle them. */
          incoming_client_packets(pconn);
	} else {
          /* Read failure; the connection is closed. */
	  close_socket_callback(pconn);
	}
      }

      for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
        struct connection *pconn = &connections[i];

        if (pconn->used && pconn->send_buffer
	    && pconn->send_buffer->ndata > 0) {
	  if (FD_ISSET(pconn->sock, &writefs)) {
	    flush_connection_send_buffer_all(pconn);
	  } else {
	    cut_lagging_connection(pconn);
	  }
        }
      }
    }
    break;
  }
  con_prompt_off();

  if (game.info.timeout > 0
      && S_S_RUNNING == server_state()
      && game.server.phase_timer
      && (read_timer_seconds(game.server.phase_timer)
          > game.info.seconds_to_phasedone)) {
    return S_E_END_OF_TURN_TIMEOUT;
  }
  return S_E_OTHERWISE;
}

/********************************************************************
  Make up a name for the connection, before we get any data from
  it to use as a sensible name.  Name will be 'c' + integer,
  guaranteed not to be the same as any other connection name,
  nor player name nor user name, nor connection id (avoid possible
  confusions).   Returns pointer to static buffer, and fills in
  (*id) with chosen value.
********************************************************************/
static const char *makeup_connection_name(int *id)
{
  static unsigned short i = 0;
  static char name[MAX_LEN_NAME];

  for(;;) {
    if (i==(unsigned short)-1) i++;              /* don't use 0 */
    my_snprintf(name, sizeof(name), "c%u", (unsigned int)++i);
    if (!find_player_by_name(name)
	&& !find_player_by_user(name)
	&& !find_conn_by_id(i)
	&& !find_conn_by_user(name)) {
      *id = i;
      return name;
    }
  }
}
  
/********************************************************************
  Server accepts connection from client:
  Low level socket stuff, and basic-initialize the connection struct.
  Returns 0 on success, -1 on failure (bad accept(), or too many
  connections).
********************************************************************/
static int server_accept_connection(int sockfd)
{
  /* This used to have size_t for some platforms.  If this is necessary
   * it should be done with a configure check not a platform check. */
  socklen_t fromlen;

  int new_sock;
  union fc_sockaddr fromend;
  bool nameinfo = FALSE;
#ifdef IPV6_SUPPORT
  char host[NI_MAXHOST], service[NI_MAXSERV];
  char dst[INET6_ADDRSTRLEN];
#else  /* IPv6 support */
  struct hostent *from;
  const char *host = NULL;
  const char *dst;
#endif /* IPv6 support */

  fromlen = sizeof(fromend);

  if ((new_sock = accept(sockfd, &fromend.saddr, &fromlen)) == -1) {
    freelog(LOG_ERROR, "accept failed: %s", fc_strerror(fc_get_errno()));
    return -1;
  }

#ifdef IPV6_SUPPORT
  if (!getnameinfo(&fromend.saddr, fromlen, host, NI_MAXHOST,
                   service, NI_MAXSERV, NI_NUMERICSERV)) {
    nameinfo = TRUE;
  }
  if (fromend.saddr.sa_family == AF_INET6) {
    inet_ntop(AF_INET6, &fromend.saddr_in6.sin6_addr,
              dst, sizeof(dst));
  } else {
    inet_ntop(AF_INET, &fromend.saddr_in4.sin_addr, dst, sizeof(dst));
  }
#else  /* IPv6 support */
  from =
    gethostbyaddr((char *) &fromend.saddr_in4.sin_addr,
                  sizeof(fromend.saddr_in4.sin_addr), AF_INET);
  if (from) {
    host = from->h_name;
    nameinfo = TRUE;
  }
  dst = inet_ntoa(fromend.saddr_in4.sin_addr);
#endif /* IPv6 support */

  return server_make_connection(new_sock,
				(nameinfo ? host : dst), dst);
}

/********************************************************************
  Server accepts connection from client:
  Low level socket stuff, and basic-initialize the connection struct.
  Returns 0 on success, -1 on failure (bad accept(), or too many
  connections).
********************************************************************/
int server_make_connection(int new_sock, const char *client_addr, const char *client_ip)
{
  int i;

  fc_nonblock(new_sock);

  for(i=0; i<MAX_NUM_CONNECTIONS; i++) {
    struct connection *pconn = &connections[i];
    if (!pconn->used) {
      connection_common_init(pconn);
      pconn->sock = new_sock;
      pconn->observer = FALSE;
      pconn->playing = NULL;
      pconn->capability[0] = '\0';
      pconn->access_level = access_level_for_next_connection();
      pconn->delayed_disconnect = FALSE;
      pconn->notify_of_writable_data = NULL;
      pconn->is_server = TRUE;
      pconn->server.currently_processed_request_id = 0;
      pconn->server.last_request_id_seen = 0;
      pconn->server.auth_tries = 0;
      pconn->server.auth_settime = 0;
      pconn->server.status = AS_NOT_ESTABLISHED;
      pconn->server.ping_timers = timer_list_new();
      pconn->server.granted_access_level = pconn->access_level;
      pconn->ping_time = -1.0;
      pconn->incoming_packet_notify = NULL;
      pconn->outgoing_packet_notify = NULL;

      sz_strlcpy(pconn->username, makeup_connection_name(&pconn->id));
      sz_strlcpy(pconn->addr, client_addr);
      sz_strlcpy(pconn->server.ipaddr, client_ip);

      conn_list_append(game.all_connections, pconn);
  
      freelog(LOG_VERBOSE, "connection (%s) from %s (%s)", 
              pconn->username, pconn->addr, pconn->server.ipaddr);
      ping_connection(pconn);
      return 0;
    }
  }

  freelog(LOG_ERROR, "maximum number of connections reached");
  return -1;
}

/********************************************************************
 open server socket to be used to accept client connections
 and open a server socket for server LAN announcements.
********************************************************************/
int server_open_socket(void)
{
  /* setup socket address */
  union fc_sockaddr src;
  union fc_sockaddr addr;
  struct ip_mreq mreq4;
  const char *group;
  int opt;
  int lan_family;

#ifdef IPV6_SUPPORT
  struct ipv6_mreq mreq6;
#endif

  if (!net_lookup_service(srvarg.bind_addr, srvarg.port, &src, FALSE)) {
    freelog(LOG_FATAL, _("Server: bad address: <%s:%d>."),
	    srvarg.bind_addr, srvarg.port);
    exit(EXIT_FAILURE);
  }

  /* Create socket for client connections. */
  if((sock = socket(src.saddr.sa_family, SOCK_STREAM, 0)) == -1) {
    fc_errno error = fc_get_errno();

#ifdef IPV6_SUPPORT
#ifdef EAFNOSUPPORT
    bool still_error = TRUE;

    if (error == EAFNOSUPPORT && src.saddr.sa_family == AF_INET6 && srvarg.bind_addr == NULL) {
      /* Let's try IPv4 socket instead */
      freelog(LOG_NORMAL, _("Cannot open IPv6 socket, trying IPv4 instead"));

      if (!net_lookup_service(NULL, srvarg.port, &src, TRUE)) {
	freelog(LOG_FATAL, _("IPv4 service lookup failed <%d>."),
		srvarg.port);
	exit(EXIT_FAILURE);
      }

      /* Create socket for client connections. */
      if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	fc_errno error2 = fc_get_errno();
	freelog(LOG_ERROR, "Even IPv4 socket failed: %s", fc_strerror(error2));
      } else {
	still_error = FALSE;
      }
    }

    if (still_error)
#endif /* EAFNOSUPPORT */
#endif /* IPv6 support */
    {
      die("socket failed: %s", fc_strerror(error));
    }
  }

  opt = 1;
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
		(char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s", fc_strerror(fc_get_errno()));
  }

  if(bind(sock, &src.saddr, sockaddr_size(&src)) == -1) {
    freelog(LOG_FATAL, "Server bind failed: %s", fc_strerror(fc_get_errno()));
    sockaddr_debug(&src);
    exit(EXIT_FAILURE);
  }

  if(listen(sock, MAX_NUM_CONNECTIONS) == -1) {
    freelog(LOG_FATAL, "listen failed: %s", fc_strerror(fc_get_errno()));
    exit(EXIT_FAILURE);
  }

  if (srvarg.announce == ANNOUNCE_NONE) {
    return 0;
  }

#ifdef IPV6_SUPPORT
  if (srvarg.announce == ANNOUNCE_IPV6) {
    lan_family = AF_INET6;
  } else
#endif /* IPV6 used */
  {
    lan_family = AF_INET;
  }

  /* Create socket for server LAN announcements */
  if ((socklan = socket(lan_family, SOCK_DGRAM, 0)) < 0) {
    freelog(LOG_ERROR, "socket failed: %s", fc_strerror(fc_get_errno()));
  }

  if (setsockopt(socklan, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s", fc_strerror(fc_get_errno()));
  }

  fc_nonblock(socklan);

  group = get_multicast_group(srvarg.announce == ANNOUNCE_IPV6);

  memset(&addr, 0, sizeof(addr));

  addr.saddr.sa_family = lan_family;

#ifdef IPV6_SUPPORT
  if (addr.saddr.sa_family == AF_INET6) {
    addr.saddr_in6.sin6_family = AF_INET6;
    addr.saddr_in6.sin6_port = htons(SERVER_LAN_PORT);
    addr.saddr_in6.sin6_addr = in6addr_any;
  } else
#endif /* IPv6 support */
  {
    addr.saddr_in4.sin_family = AF_INET;
    addr.saddr_in4.sin_port = htons(SERVER_LAN_PORT);
    addr.saddr_in4.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  if (bind(socklan, &addr.saddr, sockaddr_size(&addr)) < 0) {
    freelog(LOG_ERROR, "Lan bind failed: %s", fc_strerror(fc_get_errno()));
  }

#ifndef IPV6_SUPPORT
  {
#ifdef HAVE_INET_ATON
    inet_aton(group, &mreq4.imr_multiaddr);
#else  /* HEVE_INET_ATON */
    mreq4.imr_multiaddr.s_addr = inet_addr(group);
#endif /* HAVE_INET_ATON */
#else  /* IPv6 support */
  if (addr.saddr.sa_family == AF_INET6) {
    inet_pton(AF_INET6, group, &mreq6.ipv6mr_multiaddr.s6_addr);
    mreq6.ipv6mr_interface = 0; /* TODO: Interface selection */
    if (setsockopt(socklan, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                   (const char*)&mreq6, sizeof(mreq6)) < 0) {
      freelog(LOG_ERROR, "IPV6_ADD_MEMBERSHIP (%s) failed: %s",
              group, fc_strerror(fc_get_errno()));
    }
  } else {
    inet_pton(AF_INET, group, &mreq4.imr_multiaddr.s_addr);
#endif /* IPv6 support */
    mreq4.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(socklan, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   (const char*)&mreq4, sizeof(mreq4)) < 0) {
      freelog(LOG_ERROR, "IP_ADD_MEMBERSHIP (%s) failed: %s",
              group, fc_strerror(fc_get_errno()));
    }
  }

  close_socket_set_callback(close_socket_callback);

  return 0;
}


/********************************************************************
...
********************************************************************/
void init_connections(void)
{
  int i;

  game.all_connections = conn_list_new();
  game.est_connections = conn_list_new();

  for(i=0; i<MAX_NUM_CONNECTIONS; i++) { 
    struct connection *pconn = &connections[i];
    pconn->used = FALSE;
    pconn->self = conn_list_new();
    conn_list_prepend(pconn->self, pconn);
  }
#if defined(__VMS)
  {
    unsigned long status;
    $DESCRIPTOR (tt_desc, "SYS$INPUT");
    status = sys$assign(&tt_desc,&tt_chan,0,0);
    if (!$VMS_STATUS_SUCCESS(status)) lib$stop(status);
  }
#endif /* VMS */
}

/**************************************************************************
...
**************************************************************************/
static void start_processing_request(struct connection *pconn,
				     int request_id)
{
  assert(request_id);
  assert(pconn->server.currently_processed_request_id == 0);
  freelog(LOG_DEBUG, "start processing packet %d from connection %d",
	  request_id, pconn->id);
  send_packet_processing_started(pconn);
  pconn->server.currently_processed_request_id = request_id;
}

/**************************************************************************
...
**************************************************************************/
static void finish_processing_request(struct connection *pconn)
{
  if (!pconn || !pconn->used) {
    return;
  }
  assert(pconn->server.currently_processed_request_id);
  freelog(LOG_DEBUG, "finish processing packet %d from connection %d",
	  pconn->server.currently_processed_request_id, pconn->id);
  send_packet_processing_finished(pconn);
  pconn->server.currently_processed_request_id = 0;
}

/**************************************************************************
...
**************************************************************************/
static void ping_connection(struct connection *pconn)
{
  freelog(LOG_DEBUG, "sending ping to %s (open=%d)",
	  conn_description(pconn),
	  timer_list_size(pconn->server.ping_timers));
  timer_list_append(pconn->server.ping_timers,
			 new_timer_start(TIMER_USER, TIMER_ACTIVE));
  send_packet_conn_ping(pconn);
}

/**************************************************************************
...
**************************************************************************/
void handle_conn_pong(struct connection *pconn)
{
  struct timer *timer;

  if (timer_list_size(pconn->server.ping_timers) == 0) {
    freelog(LOG_ERROR, "got unexpected pong from %s",
	    conn_description(pconn));
    return;
  }

  timer = timer_list_get(pconn->server.ping_timers, 0);
  timer_list_unlink(pconn->server.ping_timers, timer);
  pconn->ping_time = read_timer_seconds_free(timer);
  freelog(LOG_DEBUG, "got pong from %s (open=%d); ping time = %fs",
	  conn_description(pconn),
	  timer_list_size(pconn->server.ping_timers), pconn->ping_time);
}

/**************************************************************************
...
**************************************************************************/
static void send_ping_times_to_all(void)
{
  struct packet_conn_ping_info packet;
  int i;

  i = 0;
  conn_list_iterate(game.est_connections, pconn) {
    if (!pconn->used) {
      continue;
    }
    assert(i < ARRAY_SIZE(packet.conn_id));
    packet.conn_id[i] = pconn->id;
    packet.ping_time[i] = pconn->ping_time;
    i++;
  } conn_list_iterate_end;
  packet.connections = i;

  lsend_packet_conn_ping_info(game.est_connections, &packet);
}

/********************************************************************
  Listen for UDP packets multicasted from clients requesting
  announcement of servers on the LAN.
********************************************************************/
static void get_lanserver_announcement(void)
{
  char msgbuf[128];
  struct data_in din;
  int type;
  fd_set readfs, exceptfs;
  struct timeval tv;

  if (with_ggz) {
    return;
  }

  FD_ZERO(&readfs);
  FD_ZERO(&exceptfs);
  FD_SET(socklan, &exceptfs);
  FD_SET(socklan, &readfs);

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  while (fc_select(socklan + 1, &readfs, NULL, &exceptfs, &tv) == -1) {
    if (errno != EINTR) {
      freelog(LOG_ERROR, "select failed: %s", fc_strerror(fc_get_errno()));
      return;
    }
    /* EINTR can happen sometimes, especially when compiling with -pg.
     * Generally we just want to run select again. */
  }

  if (FD_ISSET(socklan, &readfs)) {
    if (0 < recvfrom(socklan, msgbuf, sizeof(msgbuf), 0, NULL, NULL)) {
      dio_input_init(&din, msgbuf, 1);
      dio_get_uint8(&din, &type);
      if (type == SERVER_LAN_VERSION) {
        freelog(LOG_DEBUG, "Received request for server LAN announcement.");
        send_lanserver_response();
      } else {
        freelog(LOG_DEBUG,
                "Received invalid request for server LAN announcement.");
      }
    }
  }
}

/********************************************************************
  This function broadcasts an UDP packet to clients with
  that requests information about the server state.
********************************************************************/
static void send_lanserver_response(void)
{
#ifndef HAVE_WINSOCK
  unsigned char buffer[MAX_LEN_PACKET];
#else  /* HAVE_WINSOCK */
  char buffer[MAX_LEN_PACKET];
#endif /* HAVE_WINSOCK */
  char hostname[512];
  char port[256];
  char version[256];
  char players[256];
  char status[256];
  struct data_out dout;
  union fc_sockaddr addr;
  int socksend, setting = 1;
  const char *group;
  size_t size;
#ifndef HAVE_WINSOCK
  unsigned char ttl;
#endif

  /* Create a socket to broadcast to client. */
  if ((socksend = socket(AF_INET,SOCK_DGRAM, 0)) < 0) {
    freelog(LOG_ERROR, "socket failed: %s", fc_strerror(fc_get_errno()));
    return;
  }

  /* Set the UDP Multicast group IP address of the packet. */
  group = get_multicast_group(srvarg.announce == ANNOUNCE_IPV6);
  memset(&addr, 0, sizeof(addr));
  addr.saddr_in4.sin_family = AF_INET;
  addr.saddr_in4.sin_addr.s_addr = inet_addr(group);
  addr.saddr_in4.sin_port = htons(SERVER_LAN_PORT + 1);

/* this setsockopt call fails on Windows 98, so we stick with the default
 * value of 1 on Windows, which should be fine in most cases */
#ifndef HAVE_WINSOCK
  /* Set the Time-to-Live field for the packet.  */
  ttl = SERVER_LAN_TTL;
  if (setsockopt(socksend, IPPROTO_IP, IP_MULTICAST_TTL, 
                 (const char*)&ttl, sizeof(ttl))) {
    freelog(LOG_ERROR, "setsockopt failed: %s", fc_strerror(fc_get_errno()));
    return;
  }
#endif

  if (setsockopt(socksend, SOL_SOCKET, SO_BROADCAST, 
                 (const char*)&setting, sizeof(setting))) {
    freelog(LOG_ERROR, "setsockopt failed: %s", fc_strerror(fc_get_errno()));
    return;
  }

  /* Create a description of server state to send to clients.  */
  if (my_gethostname(hostname, sizeof(hostname)) != 0) {
    sz_strlcpy(hostname, "none");
  }

  my_snprintf(version, sizeof(version), "%d.%d.%d%s",
              MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, VERSION_LABEL);

  switch (server_state()) {
  case S_S_INITIAL:
    /* TRANS: Game state for local server */
    my_snprintf(status, sizeof(status), _("Pregame"));
    break;
  case S_S_RUNNING:
    /* TRANS: Game state for local server */
    my_snprintf(status, sizeof(status), _("Running"));
    break;
  case S_S_OVER:
    /* TRANS: Game state for local server */
    my_snprintf(status, sizeof(status), _("Game over"));
    break;
  case S_S_GENERATING_WAITING:
    /* TRANS: Game state for local server */
    my_snprintf(status, sizeof(status), _("Waiting"));
  }

   my_snprintf(players, sizeof(players), "%d",
               normal_player_count());
   my_snprintf(port, sizeof(port), "%d",
              srvarg.port );

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8(&dout, SERVER_LAN_VERSION);
  dio_put_string(&dout, hostname);
  dio_put_string(&dout, port);
  dio_put_string(&dout, version);
  dio_put_string(&dout, status);
  dio_put_string(&dout, players);
  dio_put_string(&dout, get_meta_message_string());
  size = dio_output_used(&dout);

  /* Sending packet to client with the information gathered above. */
  if (sendto(socksend, buffer,  size, 0, &addr.saddr,
      sockaddr_size(&addr)) < 0) {
    freelog(LOG_ERROR, "landserver response sendto failed: %s",
	    fc_strerror(fc_get_errno()));
    return;
  }

  fc_closesocket(socksend);
}
