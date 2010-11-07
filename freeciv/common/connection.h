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
#ifndef FC__CONNECTION_H
#define FC__CONNECTION_H

#include <time.h>	/* time_t */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

//#define USE_COMPRESSION   /* webclient doesn't like compression*/

/**************************************************************************
  The connection struct and related stuff.
  Includes cmdlevel stuff, which is connection-based.
***************************************************************************/

#include "shared.h"		/* MAX_LEN_ADDR, bool type */
#include "timing.h"

#include "fc_types.h"

struct hash_table;
struct timer_list;

#define MAX_LEN_PACKET   4096 * 4

#define MAX_LEN_BUFFER   (MAX_LEN_PACKET * 128)
#define MAX_LEN_CAPSTR    512
#define MAX_LEN_PASSWORD  512 /* do not change this under any circumstances */

/**************************************************************************
  Command access levels for client-side use; at present, they are only
  used to control access to server commands typed at the client chatline.
***************************************************************************/
enum cmdlevel_id {    /* access levels for users to issue commands        */
  ALLOW_NONE = 0,     /* user may issue no commands at all                */
  ALLOW_INFO,         /* informational or observer commands only          */
  ALLOW_BASIC,        /* user may issue basic player commands             */
  ALLOW_CTRL,         /* user may issue commands that affect game & users */
                      /* (starts a vote if the user's level is 'basic')   */
  ALLOW_ADMIN,        /* user may issue commands that affect the server   */
  ALLOW_HACK,         /* user may issue *all* commands - dangerous!       */

  ALLOW_NUM,          /* the number of levels                             */
  ALLOW_UNRECOGNIZED  /* used as a failure return code                    */
};
/*  the set command is a special case:                                    */
/*    - ALLOW_CTRL is required for SSET_TO_CLIENT options                 */
/*    - ALLOW_HACK is required for SSET_SERVER_ONLY options               */

/***************************************************************************
  On the distinction between nations(formerly races), players, and users,
  see doc/HACKING
***************************************************************************/

/* where the connection is in the authentication process */
enum auth_status {
  AS_NOT_ESTABLISHED = 0,
  AS_FAILED,
  AS_REQUESTING_NEW_PASS,
  AS_REQUESTING_OLD_PASS,
  AS_ESTABLISHED
};

/* get 'struct conn_list' and related functions: */
/* do this with forward definition of struct connection, so that
 * connection struct can contain a struct conn_list */
struct connection;
#define SPECLIST_TAG conn
#define SPECLIST_TYPE struct connection
#include "speclist.h"

#define conn_list_iterate(connlist, pconn) \
    TYPED_LIST_ITERATE(struct connection, connlist, pconn)
#define conn_list_iterate_end  LIST_ITERATE_END

/***********************************************************
  This is a buffer where the data is first collected,
  whenever it arrives to the client/server.
***********************************************************/
struct socket_packet_buffer {
  int ndata;
  int do_buffer_sends;
  int nsize;
  unsigned char *data;
};

#define SPECVEC_TAG byte
#define SPECVEC_TYPE unsigned char
#include "specvec.h"

/***********************************************************
  The connection struct represents a single client or server
  at the other end of a network connection.
***********************************************************/
struct connection {
  int id;			/* used for server/client communication */
  int sock;
  bool used;
  bool established;		/* have negotiated initial packets */

  /* connection is "observer", not controller; may be observing
   * specific player, or all (implementation incomplete).
   */
  bool observer;

  /* NULL for connections not yet associated with a specific player.
   */
  struct player *playing;

  struct socket_packet_buffer *buffer;
  struct socket_packet_buffer *send_buffer;
  struct timer *last_write;

  double ping_time;
  
  struct conn_list *self;     /* list with this connection as single element */
  char username[MAX_LEN_NAME];
  char addr[MAX_LEN_ADDR];

  /* 
   * "capability" gives the capability string of the executable (be it
   * a client or server) at the other end of the connection.
   */
  char capability[MAX_LEN_CAPSTR];

  /* 
   * "access_level" stores the current access level of the client
   * corresponding to this connection.
   */
  enum cmdlevel_id access_level;

  /* 
   * Something has occurred that means the connection should be
   * closed, but the closing has been postponed. 
   */
  bool delayed_disconnect;

  void (*notify_of_writable_data) (struct connection * pc,
				   bool data_available_and_socket_full);

  /* Determines whether client or server behavior should be used. */
  bool is_server;
  struct {
    /* 
     * Increases for every packet send to the server.
     */
    int last_request_id_used;

    /* 
     * Increases for every received PACKET_PROCESSING_FINISHED packet.
     */
    int last_processed_request_id_seen;

    /* 
     * Holds the id of the request which caused this packet. Can be
     * zero.
     */
    int request_id_of_currently_handled_packet;
  } client;

  struct {
    /* 
     * Holds the id of the request which is processed now. Can be
     * zero.
     */
    int currently_processed_request_id;

    /* 
     * Will increase for every received packet.
     */
    int last_request_id_seen;

    /* 
     * The start times of the PACKET_CONN_PING which have been sent
     * but weren't PACKET_CONN_PONGed yet? 
     */
    struct timer_list *ping_timers;
   
    /* Holds number of tries for authentication from client. */
    int auth_tries;

    /* the time that the server will respond after receiving an auth reply.
     * this is used to throttle the connection. Also used to reject a 
     * connection if we've waited too long for a password. */
    time_t auth_settime;

    /* used to follow where the connection is in the authentication process */
    enum auth_status status;
    char password[MAX_LEN_PASSWORD];

    /* for reverse lookup and blacklisting in db */
    char ipaddr[MAX_LEN_ADDR];

    /* The access level initially given to the client upon connection. */
    enum cmdlevel_id granted_access_level;
  } server;

  /*
   * Called before an incoming packet is processed. The packet_type
   * argument should really be a "enum packet_type". However due
   * circular dependency this is impossible.
   */
  void (*incoming_packet_notify) (struct connection * pc,
				  int packet_type, int size);

  /*
   * Called before a packet is sent. The packet_type argument should
   * really be a "enum packet_type". However due circular dependency
   * this is impossible.
   */
  void (*outgoing_packet_notify) (struct connection * pc,
				  int packet_type, int size,
				  int request_id);
  struct {
    struct hash_table **sent;
    struct hash_table **received;
    int *variant;
  } phs;

#ifdef USE_COMPRESSION
  struct {
    int frozen_level;

    struct byte_vector queue;
  } compression;
#endif
  struct {
    int bytes_send;
  } statistics;
};


const char *cmdlevel_name(enum cmdlevel_id lvl);
enum cmdlevel_id cmdlevel_named(const char *token);


typedef void (*CLOSE_FUN) (struct connection *pc);
void close_socket_set_callback(CLOSE_FUN fun);
CLOSE_FUN close_socket_get_callback(void);

int read_socket_data(int sock, struct socket_packet_buffer *buffer);
void flush_connection_send_buffer_all(struct connection *pc);
void send_connection_data(struct connection *pc, const unsigned char *data,
			  int len);

void connection_do_buffer(struct connection *pc);
void connection_do_unbuffer(struct connection *pc);

void conn_list_do_buffer(struct conn_list *dest);
void conn_list_do_unbuffer(struct conn_list *dest);

struct connection *find_conn_by_user(const char *user_name);
struct connection *find_conn_by_user_prefix(const char *user_name,
                                             enum m_pre_result *result);
struct connection *find_conn_by_id(int id);

struct socket_packet_buffer *new_socket_packet_buffer(void);
void connection_common_init(struct connection *pconn);
void connection_common_close(struct connection *pconn);
void free_compression_queue(struct connection *pconn);
void conn_clear_packet_cache(struct connection *pconn);

const char *conn_description(const struct connection *pconn);
bool conn_controls_player(const struct connection *pconn);
bool conn_is_global_observer(const struct connection *pconn);
enum cmdlevel_id conn_get_access(const struct connection *pconn);

struct player;
struct player *conn_get_player(const struct connection *pconn);

bool can_conn_edit(const struct connection *pconn);
bool can_conn_enable_editing(const struct connection *pconn);

int get_next_request_id(int old_request_id);

extern const char blank_addr_str[];

extern int delayed_disconnect;

#endif  /* FC__CONNECTION_H */
