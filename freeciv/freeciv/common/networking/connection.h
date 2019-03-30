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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <time.h>	/* time_t */

#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef FREECIV_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef FREECIV_JSON_CONNECTION
#include <jansson.h>
#endif  /* FREECIV_JSON_CONNECTION */

#ifndef FREECIV_JSON_CONNECTION
#define USE_COMPRESSION
#endif  /* FREECIV_JSON_CONNECTION */

/**************************************************************************
  The connection struct and related stuff.
  Includes cmdlevel stuff, which is connection-based.
***************************************************************************/

/* utility */
#include "shared.h"             /* MAX_LEN_ADDR */
#include "support.h"            /* bool type */
#include "timing.h"

/* common */
#include "fc_types.h"

struct conn_pattern_list;
struct genhash;
struct packet_handlers;
struct timer_list;

/* Used in the network protocol. */
#define MAX_LEN_PACKET   4096
#define MAX_LEN_CAPSTR    512
#define MAX_LEN_PASSWORD  512 /* do not change this under any circumstances */
#define MAX_LEN_CONTENT  (MAX_LEN_PACKET - 20)

#define MAX_LEN_BUFFER   (MAX_LEN_PACKET * 128)

/****************************************************************************
  Command access levels for client-side use; at present, they are only
  used to control access to server commands typed at the client chatline.
  Used in the network protocol.
****************************************************************************/
#define SPECENUM_NAME cmdlevel
/* User may issue no commands at all. */
#define SPECENUM_VALUE0 ALLOW_NONE
#define SPECENUM_VALUE0NAME "none"
/* Informational or observer commands only. */
#define SPECENUM_VALUE1 ALLOW_INFO
#define SPECENUM_VALUE1NAME "info"
/* User may issue basic player commands. */
#define SPECENUM_VALUE2 ALLOW_BASIC
#define SPECENUM_VALUE2NAME "basic"
/* User may issue commands that affect game & users
 * (starts a vote if the user's level is 'basic'). */
#define SPECENUM_VALUE3 ALLOW_CTRL
#define SPECENUM_VALUE3NAME "ctrl"
/* User may issue commands that affect the server. */
#define SPECENUM_VALUE4 ALLOW_ADMIN
#define SPECENUM_VALUE4NAME "admin"
/* User may issue *all* commands - dangerous! */
#define SPECENUM_VALUE5 ALLOW_HACK
#define SPECENUM_VALUE5NAME "hack"
#define SPECENUM_COUNT CMDLEVEL_COUNT
#include "specenum_gen.h"

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

struct packet_header {
  unsigned int length : 4;      /* Actually 'enum data_type' */
  unsigned int type : 4;        /* Actually 'enum data_type' */
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
  struct packet_header packet_header;
  char *closing_reason;

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
#ifdef FREECIV_JSON_CONNECTION
  bool json_mode;
  json_t *json_packet;
#endif /* FREECIV_JSON_CONNECTION */

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
  enum cmdlevel access_level;

  enum gui_type client_gui;

  void (*notify_of_writable_data) (struct connection * pc,
                                   bool data_available_and_socket_full);

  union {
    struct {
      /* Increases for every packet send to the server. */
      int last_request_id_used;

      /* Increases for every received PACKET_PROCESSING_FINISHED packet. */
      int last_processed_request_id_seen;

      /* Holds the id of the request which caused this packet. Can be zero. */
      int request_id_of_currently_handled_packet;
    } client;

    struct {
      /* Holds the id of the request which is processed now. Can be zero. */
      int currently_processed_request_id;

      /* Will increase for every received packet. */
      int last_request_id_seen;

      /* The start times of the PACKET_CONN_PING which have been sent but
       * weren't PACKET_CONN_PONGed yet? */
      struct timer_list *ping_timers;

      /* Holds number of tries for authentication from client. */
      int auth_tries;

      /* the time that the server will respond after receiving an auth reply.
       * this is used to throttle the connection. Also used to reject a 
       * connection if we've waited too long for a password. */
      time_t auth_settime;

      /* used to follow where the connection is in the authentication
       * process */
      enum auth_status status;
      char password[MAX_LEN_PASSWORD];

      /* for reverse lookup and blacklisting in db */
      char ipaddr[MAX_LEN_ADDR];

      /* The access level initially given to the client upon connection. */
      enum cmdlevel granted_access_level;

      /* The list of ignored connection patterns. */
      struct conn_pattern_list *ignore_list;

      /* Something has occurred that means the connection should be closed,
       * but the closing has been postponed. */
      bool is_closing;

      /* If we use delegation the original player (playing) is replaced. Save
       * it here to easily restore it. */
      struct {
        bool status; /* TRUE if player currently delegated to us */
        struct player *playing;
        bool observer;
      } delegation;
    } server;
  };

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
    struct genhash **sent;
    struct genhash **received;
    const struct packet_handlers *handlers;
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


typedef void (*conn_close_fn_t) (struct connection *pconn);
void connections_set_close_callback(conn_close_fn_t func);
void connection_close(struct connection *pconn, const char *reason);

int read_socket_data(int sock, struct socket_packet_buffer *buffer);
void flush_connection_send_buffer_all(struct connection *pc);
bool connection_send_data(struct connection *pconn,
                          const unsigned char *data, int len);

void connection_do_buffer(struct connection *pc);
void connection_do_unbuffer(struct connection *pc);

void conn_list_do_buffer(struct conn_list *dest);
void conn_list_do_unbuffer(struct conn_list *dest);

struct connection *conn_by_user(const char *user_name);
struct connection *conn_by_user_prefix(const char *user_name,
                                       enum m_pre_result *result);
struct connection *conn_by_number(int id);

struct socket_packet_buffer *new_socket_packet_buffer(void);
void connection_common_init(struct connection *pconn);
void connection_common_close(struct connection *pconn);
void conn_set_capability(struct connection *pconn, const char *capability);
void free_compression_queue(struct connection *pconn);
void conn_reset_delta_state(struct connection *pconn);

void conn_compression_freeze(struct connection *pconn);
bool conn_compression_thaw(struct connection *pconn);
bool conn_compression_frozen(const struct connection *pconn);
void conn_list_compression_freeze(const struct conn_list *pconn_list);
void conn_list_compression_thaw(const struct conn_list *pconn_list);

const char *conn_description(const struct connection *pconn);
bool conn_controls_player(const struct connection *pconn);
bool conn_is_global_observer(const struct connection *pconn);
enum cmdlevel conn_get_access(const struct connection *pconn);

struct player;
struct player *conn_get_player(const struct connection *pconn);

bool can_conn_edit(const struct connection *pconn);
bool can_conn_enable_editing(const struct connection *pconn);

int get_next_request_id(int old_request_id);

extern const char blank_addr_str[];


/* Connection patterns. */
struct conn_pattern;

#define SPECLIST_TAG conn_pattern
#define SPECLIST_TYPE struct conn_pattern
#include "speclist.h"
#define conn_pattern_list_iterate(plist, ppatern) \
  TYPED_LIST_ITERATE(struct conn_pattern, plist, ppatern)
#define conn_pattern_list_iterate_end LIST_ITERATE_END

#define SPECENUM_NAME conn_pattern_type
#define SPECENUM_VALUE0 CPT_USER
#define SPECENUM_VALUE0NAME "user"
#define SPECENUM_VALUE1 CPT_HOST
#define SPECENUM_VALUE1NAME "host"
#define SPECENUM_VALUE2 CPT_IP
#define SPECENUM_VALUE2NAME "ip"
#include "specenum_gen.h"

struct conn_pattern *conn_pattern_new(enum conn_pattern_type type,
                                      const char *wildcard);
void conn_pattern_destroy(struct conn_pattern *ppattern);

bool conn_pattern_match(const struct conn_pattern *ppattern,
                        const struct connection *pconn);
bool conn_pattern_list_match(const struct conn_pattern_list *plist,
                             const struct connection *pconn);

size_t conn_pattern_to_string(const struct conn_pattern *ppattern,
                              char *buf, size_t buf_len);
struct conn_pattern *conn_pattern_from_string(const char *pattern,
                                              enum conn_pattern_type prefer,
                                              char *error_buf,
                                              size_t error_buf_len);

bool conn_is_valid(const struct connection *pconn);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__CONNECTION_H */
