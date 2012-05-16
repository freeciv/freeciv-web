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
#ifndef FC__CIVCLIENT_H
#define FC__CIVCLIENT_H

#include "fc_types.h"

#include "packets.h"		/* enum report_type */
#include "worklist.h"

/*
 * Every TIMER_INTERVAL milliseconds real_timer_callback is
 * called. TIMER_INTERVAL has to stay 500 because real_timer_callback
 * also updates the timeout info.
 */
#define TIMER_INTERVAL (int)(real_timer_callback() * 1000)

/* Client states (see also enum server_states in srv_main.h).
 * Changing those values don't break the network compatibility.
 *
 * C_S_INITIAL:      Client boot, only used once on program start.
 * C_S_DISCONNECTED: The state when the client is not connected
 *                   to a server.  In this state, neither game nor ruleset
 *                   is in effect.
 * C_S_PREPARING:    Connected in pregame.  Game and ruleset are done.
 * C_S_RUNNING:      Connected ith game in progress.
 * C_S_OVER:         Connected with game over.
 */
enum client_states { 
  C_S_INITIAL,
  C_S_DISCONNECTED,
  C_S_PREPARING,
  C_S_RUNNING,
  C_S_OVER,
};

int client_main(int argc, char *argv[]);

void client_packet_input(void *packet, int type);

void send_report_request(enum report_type type);
void send_attribute_block_request(void);
void send_turn_done(void);

void user_ended_turn(void);

void set_client_state(enum client_states newstate);
enum client_states client_state(void);
void set_server_busy(bool busy);
bool is_server_busy(void);

void client_remove_cli_conn(struct connection *pconn);
void client_remove_all_cli_conn(void);

extern char *logfile;
extern char *scriptfile;
extern char sound_plugin_name[512];
extern char sound_set_name[512];
extern char server_host[512];
extern char user_name[512];
extern char password[MAX_LEN_PASSWORD];
extern char metaserver[512];
extern int  server_port;
extern bool auto_connect;
extern bool waiting_for_end_turn;
extern bool turn_done_sent;
extern bool in_ggz;

/* Structure for holding global client data.
 *
 * TODO: Lots more variables could be added here. */
extern struct civclient {
  /* this is the client's connection to the server */
  struct connection conn;
  struct worklist worklists[MAX_NUM_WORKLISTS];
} client;

void wait_till_request_got_processed(int request_id);
bool client_is_observer(void);
bool client_is_global_observer(void);
int client_player_number(void);
bool client_has_player(void);
struct player *client_player(void);
void set_seconds_to_turndone(double seconds);
int get_seconds_to_turndone(void);
double real_timer_callback(void);
bool can_client_control(void);
bool can_client_issue_orders(void);
bool can_client_change_view(void);
bool can_meet_with_player(const struct player *pplayer);
bool can_intel_with_player(const struct player *pplayer);

void client_exit(void);

/* Set in GUI code. */
extern const char * const gui_character_encoding;
extern const bool gui_use_transliteration;

#endif  /* FC__CIVCLIENT_H */
