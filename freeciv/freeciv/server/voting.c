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

#include <math.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "capability.h"
#include "connection.h"
#include "packets.h"
#include "player.h"

/* server */
#include "commands.h"
#include "console.h"
#include "hand_gen.h"
#include "notify.h"
#include "settings.h"
#include "stdinhand.h"

#include "voting.h"

struct vote_list *vote_list = NULL;
int vote_number_sequence = 0;


/**********************************************************************//**
  Helper function that returns the current number of eligible voters.
**************************************************************************/
int count_voters(const struct vote *pvote)
{
  int num_voters = 0;

  conn_list_iterate(game.est_connections, pconn) {
    if (conn_can_vote(pconn, pvote)) {
      num_voters++;
    }
  } conn_list_iterate_end;

  return num_voters;
}

/**********************************************************************//**
  Tell clients that a new vote has been created.
**************************************************************************/
static void lsend_vote_new(struct conn_list *dest, struct vote *pvote)
{
  struct packet_vote_new packet;
  struct connection *pconn;

  if (pvote == NULL) {
    return;
  }

  pconn = conn_by_number(pvote->caller_id);
  if (pconn == NULL) {
    return;
  }

  log_debug("lsend_vote_new %p (%d) --> %p", pvote, pvote->vote_no, dest);

  packet.vote_no = pvote->vote_no;
  sz_strlcpy(packet.user, pconn->username);
  describe_vote(pvote, packet.desc, sizeof(packet.desc));

  packet.percent_required = 100 * pvote->need_pc;
  packet.flags = pvote->flags;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, conn) {
    if (!conn_can_see_vote(conn, pvote)) {
      continue;
    }
    send_packet_vote_new(conn, &packet);
  } conn_list_iterate_end;
}

/**********************************************************************//**
  Send updated status information about the given vote.
**************************************************************************/
static void lsend_vote_update(struct conn_list *dest, struct vote *pvote,
                              int num_voters)
{
  struct packet_vote_update packet;
  struct connection *pconn;

  if (pvote == NULL) {
    return;
  }

  pconn = conn_by_number(pvote->caller_id);
  if (pconn == NULL) {
    return;
  }

  log_debug("lsend_vote_update %p (%d) --> %p", pvote, pvote->vote_no, dest);

  packet.vote_no = pvote->vote_no;
  packet.yes = pvote->yes;
  packet.no = pvote->no;
  packet.abstain = pvote->abstain;
  packet.num_voters = num_voters;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, aconn) {
    if (!conn_can_see_vote(aconn, pvote)) {
      continue;
    }
    send_packet_vote_update(aconn, &packet);
  } conn_list_iterate_end;
}

/**********************************************************************//**
  Tell clients that the given vote no longer exists.
**************************************************************************/
static void lsend_vote_remove(struct conn_list *dest, struct vote *pvote)
{
  struct packet_vote_remove packet;

  if (!pvote) {
    return;
  }

  packet.vote_no = pvote->vote_no;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    send_packet_vote_remove(pconn, &packet);
  } conn_list_iterate_end;
}

/**********************************************************************//**
  Tell clients that the given vote resolved.
**************************************************************************/
static void lsend_vote_resolve(struct conn_list *dest,
                               struct vote *pvote, bool passed)
{
  struct packet_vote_resolve packet;

  if (!pvote) {
    return;
  }

  packet.vote_no = pvote->vote_no;
  packet.passed = passed;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    if (!conn_can_see_vote(pconn, pvote)) {
      continue;
    }
    send_packet_vote_resolve(pconn, &packet);
  } conn_list_iterate_end;
}

/**********************************************************************//**
  Free all memory used by the vote structure.
**************************************************************************/
static void free_vote(struct vote *pvote)
{
  if (!pvote) {
    return;
  }

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    free(pvc);
  } vote_cast_list_iterate_end;
  vote_cast_list_destroy(pvote->votes_cast);
  free(pvote);
}

/**********************************************************************//**
  Remove the given vote and send a vote_remove packet to clients.
**************************************************************************/
void remove_vote(struct vote *pvote)
{
  if (!vote_list || !pvote) {
    return;
  }

  vote_list_remove(vote_list, pvote);
  lsend_vote_remove(NULL, pvote);
  free_vote(pvote);
}

/**********************************************************************//**
  Remove all votes. Sends vote_remove packets to clients.
**************************************************************************/
void clear_all_votes(void)
{
  if (!vote_list) {
    return;
  }

  vote_list_iterate(vote_list, pvote) {
    lsend_vote_remove(NULL, pvote);
    free_vote(pvote);
  } vote_list_iterate_end;
  vote_list_clear(vote_list);
}

/**********************************************************************//**
  Returns TRUE if this vote is a "teamvote".
**************************************************************************/
bool vote_is_team_only(const struct vote *pvote)
{
  return pvote && (pvote->flags & VCF_TEAMONLY);
}

/**********************************************************************//**
  A user cannot vote if:
    * is not connected
    * access level < basic
    * isn't a player
    * the vote is a team vote and not on the caller's team
  NB: If 'pvote' is NULL, then the team condition is not checked.
**************************************************************************/
bool conn_can_vote(const struct connection *pconn, const struct vote *pvote)
{
  if (!pconn || !conn_controls_player(pconn)
      || conn_get_access(pconn) < ALLOW_BASIC) {
    return FALSE;
  }

  if (vote_is_team_only(pvote)) {
    const struct player *pplayer, *caller_plr;

    pplayer = conn_get_player(pconn);
    caller_plr = conn_get_player(vote_get_caller(pvote));
    if (!pplayer || !caller_plr
        || !players_on_same_team(pplayer, caller_plr)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Usually, all users can see, except in the team vote case.
**************************************************************************/
bool conn_can_see_vote(const struct connection *pconn,
                       const struct vote *pvote)
{
  if (!pconn) {
    return FALSE;
  }

  if (conn_is_global_observer(pconn)) {
    /* All is visible for global observer. */
    return TRUE;
  }

  if (vote_is_team_only(pvote)) {
    const struct player *pplayer, *caller_plr;

    pplayer = conn_get_player(pconn);
    caller_plr = conn_get_player(vote_get_caller(pvote));
    if (!pplayer || !caller_plr
        || !players_on_same_team(pplayer, caller_plr)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Returns the vote with vote number 'vote_no', or NULL.
**************************************************************************/
struct vote *get_vote_by_no(int vote_no)
{
  if (!vote_list) {
    return NULL;
  }

  vote_list_iterate(vote_list, pvote) {
    if (pvote->vote_no == vote_no) {
      return pvote;
    }
  } vote_list_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Returns the vote called by 'caller', or NULL if none exists.
**************************************************************************/
struct vote *get_vote_by_caller(const struct connection *caller)
{
  if (caller == NULL || !vote_list) {
    return NULL;
  }

  vote_list_iterate(vote_list, pvote) {
    if (pvote->caller_id == caller->id) {
      return pvote;
    }
  } vote_list_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Create and return a newly allocated vote for the command with id
  'command_id' and all arguments in the string 'allargs'.
**************************************************************************/
struct vote *vote_new(struct connection *caller,
                      const char *allargs,
                      int command_id)
{
  struct vote *pvote;
  const struct command *pcmd;

  if (!conn_can_vote(caller, NULL)) {
    return NULL;
  }

  /* Cancel previous vote */
  remove_vote(get_vote_by_caller(caller));

  /* Make a new vote */
  pvote = fc_malloc(sizeof(struct vote));
  pvote->caller_id = caller->id;
  pvote->command_id = command_id;
  pcmd = command_by_number(command_id);

  sz_strlcpy(pvote->cmdline, command_name(pcmd));
  if (allargs != NULL && allargs[0] != '\0') {
    sz_strlcat(pvote->cmdline, " ");
    sz_strlcat(pvote->cmdline, allargs);
  }

  pvote->turn_count = 0;
  pvote->votes_cast = vote_cast_list_new();
  pvote->vote_no = ++vote_number_sequence;

  vote_list_append(vote_list, pvote);

  pvote->flags = command_vote_flags(pcmd);
  pvote->need_pc = (double) command_vote_percent(pcmd) / 100.0;

  if (pvote->flags & VCF_NOPASSALONE) {
    int num_voters = count_voters(pvote);
    double min_pc = 1.0 / (double) num_voters;

    if (num_voters > 1 && min_pc > pvote->need_pc) {
      pvote->need_pc = MIN(0.5, 2.0 * min_pc);
    }
  }

  lsend_vote_new(NULL, pvote);

  return pvote;
}

/**********************************************************************//**
  Return whether the vote would pass immediately when the caller will vote
  for.
**************************************************************************/
bool vote_would_pass_immediately(const struct connection *caller,
                                 int command_id)
{
  struct vote virtual_vote;
  const struct command *pcmd;

  if (!conn_can_vote(caller, NULL)) {
    return FALSE;
  }

  pcmd = command_by_number(command_id);
  fc_assert(pcmd != NULL);
  memset(&virtual_vote, 0, sizeof(virtual_vote));
  virtual_vote.flags = command_vote_flags(pcmd);

  if (virtual_vote.flags & VCF_NOPASSALONE) {
    return FALSE;
  }

  virtual_vote.caller_id = caller->id;
  return (((double) (command_vote_percent(pcmd)
                     * count_voters(&virtual_vote)) / 100.0) < 1.0);
}

/**********************************************************************//**
  Check if we satisfy the criteria for resolving a vote, and resolve it
  if these critera are indeed met. Updates yes and no variables in voting
  struct as well.
**************************************************************************/
static void check_vote(struct vote *pvote)
{
  int num_cast = 0, num_voters = 0;
  bool resolve = FALSE, passed = FALSE;
  struct connection *pconn = NULL;
  double yes_pc = 0.0, no_pc = 0.0, rem_pc = 0.0, base = 0.0;
  int flags;
  double need_pc;
  char cmdline[MAX_LEN_CONSOLE_LINE];
  const double MY_EPSILON = 0.000001;
  const char *title;
  const struct player *callplr;

  if (!pvote) {
    return;
  }

  pvote->yes = 0;
  pvote->no = 0;
  pvote->abstain = 0;

  num_voters = count_voters(pvote);

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (!(pconn = conn_by_number(pvc->conn_id))
        || !conn_can_vote(pconn, pvote)) {
      continue;
    }
    num_cast++;

    switch (pvc->vote_cast) {
    case VOTE_YES:
      pvote->yes++;
      continue;
    case VOTE_NO:
      pvote->no++;
      continue;
    case VOTE_ABSTAIN:
      pvote->abstain++;
      continue;
    case VOTE_NUM:
      break;
    }

    log_error("Unknown vote cast variant: %d.", pvc->vote_cast);
    pvote->abstain++;
  } vote_cast_list_iterate_end;

  flags = pvote->flags;
  need_pc = pvote->need_pc;

  /* Check if we should resolve the vote. */
  if (num_voters > 0) {

    /* Players that abstain essentially remove themselves from
     * the voting pool. */
    base = num_voters - pvote->abstain;

    if (base > MY_EPSILON) {
      yes_pc = (double) pvote->yes / base;
      no_pc = (double) pvote->no / base;

      /* The fraction of people who have not voted at all. */
      rem_pc = (double) (num_voters - num_cast) / base;
    }

    if (flags & VCF_NODISSENT && no_pc > MY_EPSILON) {
      resolve = TRUE;
    }

    if (!resolve) {
      resolve = (/* We have enough yes votes. */
                 (yes_pc - need_pc > MY_EPSILON)
                 /* We have too many no votes. */
                 || (no_pc - 1.0 + need_pc > MY_EPSILON
                     || fabs(no_pc - 1.0 + need_pc) < MY_EPSILON)
                 /* We can't get enough no votes. */
                 || (no_pc + rem_pc - 1.0 + need_pc < -MY_EPSILON)
                 /* We can't get enough yes votes. */
                 || (yes_pc + rem_pc - need_pc < -MY_EPSILON
                     || fabs(yes_pc + rem_pc - need_pc) < MY_EPSILON));
    }

    /* Resolve if everyone voted already. */
    if (!resolve && fabs(rem_pc) < MY_EPSILON) {
      resolve = TRUE;
    }

    /* Resolve this vote if it has been around long enough. */
    if (!resolve && pvote->turn_count > 1) {
      resolve = TRUE;
    }

    /* Resolve this vote if everyone tries to abstain. */
    if (!resolve && fabs(base) < MY_EPSILON) {
      resolve = TRUE;
    }
  }

  log_debug("check_vote flags=%d need_pc=%0.2f yes_pc=%0.2f "
            "no_pc=%0.2f rem_pc=%0.2f base=%0.2f resolve=%d",
            flags, need_pc, yes_pc, no_pc, rem_pc, base, resolve);

  lsend_vote_update(NULL, pvote, num_voters);

  if (!resolve) {
    return;
  }

  passed = yes_pc - need_pc > MY_EPSILON;

  if (passed && flags & VCF_NODISSENT) {
    passed = fabs(no_pc) < MY_EPSILON;
  }

  if (vote_is_team_only(pvote)) {
    const struct connection *caller;

    /* TRANS: "Vote" as a process. Used as part of a sentence. */
    title = _("Teamvote");
    caller = vote_get_caller(pvote);
    callplr = conn_get_player(caller);
  } else {
    /* TRANS: "Vote" as a process. Used as part of a sentence. */
    title = _("Vote");
    callplr = NULL;
  }

  if (passed) {
    notify_team(callplr, NULL, E_VOTE_RESOLVED, ftc_vote_passed,
                /* TRANS: "[Vote|Teamvote] 3 \"proposed change\" is ..." */
                _("%s %d \"%s\" is passed %d to %d with "
                  "%d abstentions and %d who did not vote."),
                title, pvote->vote_no, pvote->cmdline, pvote->yes,
                pvote->no, pvote->abstain, num_voters - num_cast);
  } else {
    notify_team(callplr, NULL, E_VOTE_RESOLVED, ftc_vote_failed,
                /* TRANS: "[Vote|Teamvote] 3 \"proposed change\" failed ..." */
                _("%s %d \"%s\" failed with %d against, %d for, "
                  "%d abstentions and %d who did not vote."),
                title, pvote->vote_no, pvote->cmdline, pvote->no,
                pvote->yes, pvote->abstain, num_voters - num_cast);
  }

  lsend_vote_resolve(NULL, pvote, passed);

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (!(pconn = conn_by_number(pvc->conn_id))) {
      log_error("Got a vote from a lost connection");
      continue;
    } else if (!conn_can_vote(pconn, pvote)) {
      log_error("Got a vote from a non-voting connection");
      continue;
    }

    switch (pvc->vote_cast) {
    case VOTE_YES:
      notify_team(callplr, NULL, E_VOTE_RESOLVED, ftc_vote_yes,
                  _("%s %d: %s voted yes."),
                  title, pvote->vote_no, pconn->username);
      break;
    case VOTE_NO:
      notify_team(callplr, NULL, E_VOTE_RESOLVED, ftc_vote_no,
                  _("%s %d: %s voted no."),
                  title, pvote->vote_no, pconn->username);
      break;
    case VOTE_ABSTAIN:
      notify_team(callplr, NULL, E_VOTE_RESOLVED, ftc_vote_abstain,
                  _("%s %d: %s chose to abstain."),
                  title, pvote->vote_no, pconn->username);
      break;
    default:
      break;
    }
  } vote_cast_list_iterate_end;

  /* Remove the vote before executing the command because it's the
   * cause of many crashes due to the /cut command:
   *   - If the caller is the target.
   *   - If the target votes on this vote. */
  sz_strlcpy(cmdline, pvote->cmdline);
  remove_vote(pvote);

  if (passed) {
    handle_stdin_input(NULL, cmdline);
  }
}

/**********************************************************************//**
  Find the vote cast for the user id conn_id in a vote.
**************************************************************************/
static struct vote_cast *vote_cast_find(struct vote *pvote, int conn_id)
{
  if (!pvote) {
    return NULL;
  }

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (pvc->conn_id == conn_id) {
      return pvc;
    }
  } vote_cast_list_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Return a new vote cast.
**************************************************************************/
static struct vote_cast *vote_cast_new(struct vote *pvote)
{
  struct vote_cast *pvc;

  if (!pvote) {
    return NULL;
  }

  pvc = fc_malloc(sizeof(struct vote_cast));
  pvc->conn_id = -1;
  pvc->vote_cast = VOTE_ABSTAIN;

  vote_cast_list_append(pvote->votes_cast, pvc);

  return pvc;
}

/**********************************************************************//**
  Remove a vote cast. This unlinks it and frees its memory.
**************************************************************************/
static void remove_vote_cast(struct vote *pvote, struct vote_cast *pvc)
{
  if (!pvote || !pvc) {
    return;
  }

  vote_cast_list_remove(pvote->votes_cast, pvc);
  free(pvc);
  check_vote(pvote);            /* Maybe can pass */
}

/**********************************************************************//**
  Make the given connection vote 'type' on 'pvote', and check the vote.
**************************************************************************/
void connection_vote(struct connection *pconn,
                     struct vote *pvote,
                     enum vote_type type)
{
  struct vote_cast *pvc;

  if (!conn_can_vote(pconn, pvote)) {
    return;
  }

  /* Try to find a previous vote */
  if ((pvc = vote_cast_find(pvote, pconn->id))) {
    pvc->vote_cast = type;
  } else if ((pvc = vote_cast_new(pvote))) {
    pvc->vote_cast = type;
    pvc->conn_id = pconn->id;
  } else {
    /* Must never happen */
    log_error("Failed to create a vote cast for connection %s.",
              pconn->username);
    return;
  }
  check_vote(pvote);
}

/**********************************************************************//**
  Cancel the votes of a lost or a detached connection.
**************************************************************************/
void cancel_connection_votes(struct connection *pconn)
{
  if (!pconn || !vote_list) {
    return;
  }

  remove_vote(get_vote_by_caller(pconn));

  vote_list_iterate(vote_list, pvote) {
    remove_vote_cast(pvote, vote_cast_find(pvote, pconn->id));
  } vote_list_iterate_end;
}

/**********************************************************************//**
  Initialize data structures used by this module.
**************************************************************************/
void voting_init(void)
{
  if (!vote_list) {
    vote_list = vote_list_new();
    vote_number_sequence = 0;
  }
}

/**********************************************************************//**
  Check running votes. This should be called every turn.
**************************************************************************/
void voting_turn(void)
{
  if (!vote_list) {
    log_error("voting_turn() called before voting_init()");
    return;
  }

  vote_list_iterate(vote_list, pvote) {
    pvote->turn_count++;
    check_vote(pvote);
  } vote_list_iterate_end;
}

/**********************************************************************//**
  Free all memory used by this module.
**************************************************************************/
void voting_free(void)
{
  clear_all_votes();
  if (vote_list) {
    vote_list_destroy(vote_list);
    vote_list = NULL;
  }
}

/**********************************************************************//**
  Fills the supplied buffer with a string describing the given vote. This
  includes the vote command line, the percent required to pass, and any
  special conditions.
**************************************************************************/
int describe_vote(struct vote *pvote, char *buf, int buflen)
{
  int ret = 0;

  /* NB We don't handle votes with multiple flags here. */

  if (pvote->flags & VCF_NODISSENT) {
    ret = fc_snprintf(buf, buflen,
        /* TRANS: Describing a new vote that can only pass
         * if there are no dissenting votes. */
        _("%s (needs %0.0f%% and no dissent)."),
        pvote->cmdline, MIN(100.0, pvote->need_pc * 100.0 + 1));
  } else {
    ret = fc_snprintf(buf, buflen,
        /* TRANS: Describing a new vote that can pass only if the
         * given percentage of players votes 'yes'. */
        _("%s (needs %0.0f%% in favor)."),
        pvote->cmdline, MIN(100.0, pvote->need_pc * 100.0 + 1));
  }

  return ret;
}

/**********************************************************************//**
  Handle a vote submit packet sent from a client. This is basically just
  a Wrapper around connection_vote().
**************************************************************************/
void handle_vote_submit(struct connection *pconn, int vote_no, int value)
{
  struct vote *pvote;
  enum vote_type type;

  log_debug("Got vote submit (%d %d) from %s.",
            vote_no, value, conn_description(pconn));

  pvote = get_vote_by_no(vote_no);
  if (pvote == NULL) {
    /* The client is out of synchronization: this vote is probably just
     * resolved or cancelled. Not an error, let's just ignore the packet. */
    log_verbose("Submit request for unknown vote_no %d from %s ignored.",
                vote_no, conn_description(pconn));
    return;
  }

  if (value == 1) {
    type = VOTE_YES;
  } else if (value == -1) {
    type = VOTE_NO;
  } else if (value == 0) {
    type = VOTE_ABSTAIN;
  } else {
    log_error("Invalid packet data for submit of vote %d "
              "from %s ignored.", vote_no, conn_description(pconn));
    return;
  }

  connection_vote(pconn, pvote, type);
}

/**********************************************************************//**
  Sends a packet_vote_new to pconn for every currently running votes.
**************************************************************************/
void send_running_votes(struct connection *pconn, bool only_team_votes)
{
  if (NULL == vote_list
      || vote_list_size(vote_list) < 1
      || NULL == pconn
      || (only_team_votes && NULL == conn_get_player(pconn))) {
    return;
  }

  log_debug("Sending %s running votes to %s.",
            only_team_votes ? "team" : "all", conn_description(pconn));

  connection_do_buffer(pconn);
  vote_list_iterate(vote_list, pvote) {
    if (vote_is_team_only(pvote)) {
      if (conn_can_see_vote(pconn, pvote)) {
        lsend_vote_new(pconn->self, pvote);
        lsend_vote_update(pconn->self, pvote, count_voters(pvote));
      }
    } else if (!only_team_votes) {
      lsend_vote_new(pconn->self, pvote);
      lsend_vote_update(pconn->self, pvote, count_voters(pvote));
    }
  } vote_list_iterate_end;
  connection_do_unbuffer(pconn);
}

/**********************************************************************//**
  Sends a packet_vote_remove to pconn for every currently running team vote
  'pconn' can see.
**************************************************************************/
void send_remove_team_votes(struct connection *pconn)
{
  if (NULL == vote_list
      || vote_list_size(vote_list) < 1
      || NULL == pconn
      || NULL == conn_get_player(pconn)) {
    return;
  }

  log_debug("Sending remove info of the team votes to %s.",
            conn_description(pconn));

  connection_do_buffer(pconn);
  vote_list_iterate(vote_list, pvote) {
    if (vote_is_team_only(pvote) && conn_can_see_vote(pconn, pvote)) {
      lsend_vote_remove(pconn->self, pvote);
    }
  } vote_list_iterate_end;
  connection_do_unbuffer(pconn);
}

/**********************************************************************//**
  Sends a packet_vote_update to every conn in dest. If dest is NULL, then
  sends to all established connections.
**************************************************************************/
void send_updated_vote_totals(struct conn_list *dest)
{
  int num_voters;

  if (vote_list == NULL || vote_list_size(vote_list) <= 0) {
    return;
  }

  log_debug("Sending updated vote totals to conn_list %p", dest);

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_do_buffer(dest);
  vote_list_iterate(vote_list, pvote) {
    num_voters = count_voters(pvote);
    lsend_vote_update(dest, pvote, num_voters);
  } vote_list_iterate_end;
  conn_list_do_unbuffer(dest);
}

/**********************************************************************//**
  Returns the connection that called this vote.
**************************************************************************/
const struct connection *vote_get_caller(const struct vote *pvote)
{
  return conn_by_number(pvote->caller_id);
}
