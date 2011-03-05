
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "capability.h"
#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "support.h"

#include "packets.h"

static unsigned int hash_const(const void *vkey, unsigned int num_buckets)
{
  return 0;
}

static int cmp_const(const void *vkey1, const void *vkey2)
{
  return 0;
}

void delta_stats_report(void) {}

void delta_stats_reset(void) {}

void *get_packet_from_connection_helper(struct connection *pc,
    enum packet_type type)
{
  switch(type) {

  case PACKET_PROCESSING_STARTED:
    return receive_packet_processing_started(pc, type);

  case PACKET_PROCESSING_FINISHED:
    return receive_packet_processing_finished(pc, type);

  case PACKET_FREEZE_HINT:
    return receive_packet_freeze_hint(pc, type);

  case PACKET_THAW_HINT:
    return receive_packet_thaw_hint(pc, type);

  case PACKET_SERVER_JOIN_REQ:
    return receive_packet_server_join_req(pc, type);

  case PACKET_SERVER_JOIN_REPLY:
    return receive_packet_server_join_reply(pc, type);

  case PACKET_AUTHENTICATION_REQ:
    return receive_packet_authentication_req(pc, type);

  case PACKET_AUTHENTICATION_REPLY:
    return receive_packet_authentication_reply(pc, type);

  case PACKET_SERVER_SHUTDOWN:
    return receive_packet_server_shutdown(pc, type);

  case PACKET_NATION_SELECT_REQ:
    return receive_packet_nation_select_req(pc, type);

  case PACKET_PLAYER_READY:
    return receive_packet_player_ready(pc, type);

  case PACKET_ENDGAME_REPORT:
    return receive_packet_endgame_report(pc, type);

  case PACKET_TILE_INFO:
    return receive_packet_tile_info(pc, type);

  case PACKET_GAME_INFO:
    return receive_packet_game_info(pc, type);

  case PACKET_MAP_INFO:
    return receive_packet_map_info(pc, type);

  case PACKET_NUKE_TILE_INFO:
    return receive_packet_nuke_tile_info(pc, type);

  case PACKET_CHAT_MSG:
    return receive_packet_chat_msg(pc, type);

  case PACKET_CHAT_MSG_REQ:
    return receive_packet_chat_msg_req(pc, type);

  case PACKET_CONNECT_MSG:
    return receive_packet_connect_msg(pc, type);

  case PACKET_CITY_REMOVE:
    return receive_packet_city_remove(pc, type);

  case PACKET_CITY_INFO:
    return receive_packet_city_info(pc, type);

  case PACKET_CITY_SHORT_INFO:
    return receive_packet_city_short_info(pc, type);

  case PACKET_CITY_SELL:
    return receive_packet_city_sell(pc, type);

  case PACKET_CITY_BUY:
    return receive_packet_city_buy(pc, type);

  case PACKET_CITY_CHANGE:
    return receive_packet_city_change(pc, type);

  case PACKET_CITY_WORKLIST:
    return receive_packet_city_worklist(pc, type);

  case PACKET_CITY_MAKE_SPECIALIST:
    return receive_packet_city_make_specialist(pc, type);

  case PACKET_CITY_MAKE_WORKER:
    return receive_packet_city_make_worker(pc, type);

  case PACKET_CITY_CHANGE_SPECIALIST:
    return receive_packet_city_change_specialist(pc, type);

  case PACKET_CITY_RENAME:
    return receive_packet_city_rename(pc, type);

  case PACKET_CITY_OPTIONS_REQ:
    return receive_packet_city_options_req(pc, type);

  case PACKET_CITY_REFRESH:
    return receive_packet_city_refresh(pc, type);

  case PACKET_CITY_NAME_SUGGESTION_REQ:
    return receive_packet_city_name_suggestion_req(pc, type);

  case PACKET_CITY_NAME_SUGGESTION_INFO:
    return receive_packet_city_name_suggestion_info(pc, type);

  case PACKET_CITY_SABOTAGE_LIST:
    return receive_packet_city_sabotage_list(pc, type);

  case PACKET_PLAYER_REMOVE:
    return receive_packet_player_remove(pc, type);

  case PACKET_PLAYER_INFO:
    return receive_packet_player_info(pc, type);

  case PACKET_PLAYER_PHASE_DONE:
    return receive_packet_player_phase_done(pc, type);

  case PACKET_PLAYER_RATES:
    return receive_packet_player_rates(pc, type);

  case PACKET_PLAYER_CHANGE_GOVERNMENT:
    return receive_packet_player_change_government(pc, type);

  case PACKET_PLAYER_RESEARCH:
    return receive_packet_player_research(pc, type);

  case PACKET_PLAYER_TECH_GOAL:
    return receive_packet_player_tech_goal(pc, type);

  case PACKET_PLAYER_ATTRIBUTE_BLOCK:
    return receive_packet_player_attribute_block(pc, type);

  case PACKET_PLAYER_ATTRIBUTE_CHUNK:
    return receive_packet_player_attribute_chunk(pc, type);

  case PACKET_UNIT_REMOVE:
    return receive_packet_unit_remove(pc, type);

  case PACKET_UNIT_INFO:
    return receive_packet_unit_info(pc, type);

  case PACKET_UNIT_SHORT_INFO:
    return receive_packet_unit_short_info(pc, type);

  case PACKET_UNIT_COMBAT_INFO:
    return receive_packet_unit_combat_info(pc, type);

  case PACKET_UNIT_MOVE:
    return receive_packet_unit_move(pc, type);

  case PACKET_GOTO_PATH_REQ:
    return receive_packet_goto_path_req(pc, type);

  case PACKET_GOTO_PATH:
    return receive_packet_goto_path(pc, type);

  case PACKET_UNIT_BUILD_CITY:
    return receive_packet_unit_build_city(pc, type);

  case PACKET_UNIT_DISBAND:
    return receive_packet_unit_disband(pc, type);

  case PACKET_UNIT_CHANGE_HOMECITY:
    return receive_packet_unit_change_homecity(pc, type);

  case PACKET_UNIT_ESTABLISH_TRADE:
    return receive_packet_unit_establish_trade(pc, type);

  case PACKET_UNIT_BATTLEGROUP:
    return receive_packet_unit_battlegroup(pc, type);

  case PACKET_UNIT_HELP_BUILD_WONDER:
    return receive_packet_unit_help_build_wonder(pc, type);

  case PACKET_UNIT_ORDERS:
    return receive_packet_unit_orders(pc, type);

  case PACKET_UNIT_AUTOSETTLERS:
    return receive_packet_unit_autosettlers(pc, type);

  case PACKET_UNIT_LOAD:
    return receive_packet_unit_load(pc, type);

  case PACKET_UNIT_UNLOAD:
    return receive_packet_unit_unload(pc, type);

  case PACKET_UNIT_UPGRADE:
    return receive_packet_unit_upgrade(pc, type);

  case PACKET_UNIT_TRANSFORM:
    return receive_packet_unit_transform(pc, type);

  case PACKET_UNIT_NUKE:
    return receive_packet_unit_nuke(pc, type);

  case PACKET_UNIT_PARADROP_TO:
    return receive_packet_unit_paradrop_to(pc, type);

  case PACKET_UNIT_AIRLIFT:
    return receive_packet_unit_airlift(pc, type);

  case PACKET_UNIT_DIPLOMAT_QUERY:
    return receive_packet_unit_diplomat_query(pc, type);

  case PACKET_UNIT_TYPE_UPGRADE:
    return receive_packet_unit_type_upgrade(pc, type);

  case PACKET_UNIT_DIPLOMAT_ACTION:
    return receive_packet_unit_diplomat_action(pc, type);

  case PACKET_UNIT_DIPLOMAT_ANSWER:
    return receive_packet_unit_diplomat_answer(pc, type);

  case PACKET_UNIT_CHANGE_ACTIVITY:
    return receive_packet_unit_change_activity(pc, type);

  case PACKET_DIPLOMACY_INIT_MEETING_REQ:
    return receive_packet_diplomacy_init_meeting_req(pc, type);

  case PACKET_DIPLOMACY_INIT_MEETING:
    return receive_packet_diplomacy_init_meeting(pc, type);

  case PACKET_DIPLOMACY_CANCEL_MEETING_REQ:
    return receive_packet_diplomacy_cancel_meeting_req(pc, type);

  case PACKET_DIPLOMACY_CANCEL_MEETING:
    return receive_packet_diplomacy_cancel_meeting(pc, type);

  case PACKET_DIPLOMACY_CREATE_CLAUSE_REQ:
    return receive_packet_diplomacy_create_clause_req(pc, type);

  case PACKET_DIPLOMACY_CREATE_CLAUSE:
    return receive_packet_diplomacy_create_clause(pc, type);

  case PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ:
    return receive_packet_diplomacy_remove_clause_req(pc, type);

  case PACKET_DIPLOMACY_REMOVE_CLAUSE:
    return receive_packet_diplomacy_remove_clause(pc, type);

  case PACKET_DIPLOMACY_ACCEPT_TREATY_REQ:
    return receive_packet_diplomacy_accept_treaty_req(pc, type);

  case PACKET_DIPLOMACY_ACCEPT_TREATY:
    return receive_packet_diplomacy_accept_treaty(pc, type);

  case PACKET_DIPLOMACY_CANCEL_PACT:
    return receive_packet_diplomacy_cancel_pact(pc, type);

  case PACKET_PAGE_MSG:
    return receive_packet_page_msg(pc, type);

  case PACKET_REPORT_REQ:
    return receive_packet_report_req(pc, type);

  case PACKET_CONN_INFO:
    return receive_packet_conn_info(pc, type);

  case PACKET_CONN_PING_INFO:
    return receive_packet_conn_ping_info(pc, type);

  case PACKET_CONN_PING:
    return receive_packet_conn_ping(pc, type);

  case PACKET_CONN_PONG:
    return receive_packet_conn_pong(pc, type);

  case PACKET_CLIENT_INFO:
    return receive_packet_client_info(pc, type);

  case PACKET_END_PHASE:
    return receive_packet_end_phase(pc, type);

  case PACKET_START_PHASE:
    return receive_packet_start_phase(pc, type);

  case PACKET_NEW_YEAR:
    return receive_packet_new_year(pc, type);

  case PACKET_BEGIN_TURN:
    return receive_packet_begin_turn(pc, type);

  case PACKET_END_TURN:
    return receive_packet_end_turn(pc, type);

  case PACKET_FREEZE_CLIENT:
    return receive_packet_freeze_client(pc, type);

  case PACKET_THAW_CLIENT:
    return receive_packet_thaw_client(pc, type);

  case PACKET_SPACESHIP_LAUNCH:
    return receive_packet_spaceship_launch(pc, type);

  case PACKET_SPACESHIP_PLACE:
    return receive_packet_spaceship_place(pc, type);

  case PACKET_SPACESHIP_INFO:
    return receive_packet_spaceship_info(pc, type);

  case PACKET_RULESET_UNIT:
    return receive_packet_ruleset_unit(pc, type);

  case PACKET_RULESET_GAME:
    return receive_packet_ruleset_game(pc, type);

  case PACKET_RULESET_SPECIALIST:
    return receive_packet_ruleset_specialist(pc, type);

  case PACKET_RULESET_GOVERNMENT_RULER_TITLE:
    return receive_packet_ruleset_government_ruler_title(pc, type);

  case PACKET_RULESET_TECH:
    return receive_packet_ruleset_tech(pc, type);

  case PACKET_RULESET_GOVERNMENT:
    return receive_packet_ruleset_government(pc, type);

  case PACKET_RULESET_TERRAIN_CONTROL:
    return receive_packet_ruleset_terrain_control(pc, type);

  case PACKET_RULESET_NATION_GROUPS:
    return receive_packet_ruleset_nation_groups(pc, type);

  case PACKET_RULESET_NATION:
    return receive_packet_ruleset_nation(pc, type);

  case PACKET_RULESET_CITY:
    return receive_packet_ruleset_city(pc, type);

  case PACKET_RULESET_BUILDING:
    return receive_packet_ruleset_building(pc, type);

  case PACKET_RULESET_TERRAIN:
    return receive_packet_ruleset_terrain(pc, type);

  case PACKET_RULESET_UNIT_CLASS:
    return receive_packet_ruleset_unit_class(pc, type);

  case PACKET_RULESET_BASE:
    return receive_packet_ruleset_base(pc, type);

  case PACKET_RULESET_CONTROL:
    return receive_packet_ruleset_control(pc, type);

  case PACKET_SINGLE_WANT_HACK_REQ:
    return receive_packet_single_want_hack_req(pc, type);

  case PACKET_SINGLE_WANT_HACK_REPLY:
    return receive_packet_single_want_hack_reply(pc, type);

  case PACKET_RULESET_CHOICES:
    return receive_packet_ruleset_choices(pc, type);

  case PACKET_GAME_LOAD:
    return receive_packet_game_load(pc, type);

  case PACKET_OPTIONS_SETTABLE_CONTROL:
    return receive_packet_options_settable_control(pc, type);

  case PACKET_OPTIONS_SETTABLE:
    return receive_packet_options_settable(pc, type);

  case PACKET_RULESET_EFFECT:
    return receive_packet_ruleset_effect(pc, type);

  case PACKET_RULESET_EFFECT_REQ:
    return receive_packet_ruleset_effect_req(pc, type);

  case PACKET_RULESET_RESOURCE:
    return receive_packet_ruleset_resource(pc, type);

  case PACKET_SCENARIO_INFO:
    return receive_packet_scenario_info(pc, type);

  case PACKET_SAVE_SCENARIO:
    return receive_packet_save_scenario(pc, type);

  case PACKET_VOTE_NEW:
    return receive_packet_vote_new(pc, type);

  case PACKET_VOTE_UPDATE:
    return receive_packet_vote_update(pc, type);

  case PACKET_VOTE_REMOVE:
    return receive_packet_vote_remove(pc, type);

  case PACKET_VOTE_RESOLVE:
    return receive_packet_vote_resolve(pc, type);

  case PACKET_VOTE_SUBMIT:
    return receive_packet_vote_submit(pc, type);

  case PACKET_EDIT_MODE:
    return receive_packet_edit_mode(pc, type);

  case PACKET_EDIT_RECALCULATE_BORDERS:
    return receive_packet_edit_recalculate_borders(pc, type);

  case PACKET_EDIT_CHECK_TILES:
    return receive_packet_edit_check_tiles(pc, type);

  case PACKET_EDIT_TOGGLE_FOGOFWAR:
    return receive_packet_edit_toggle_fogofwar(pc, type);

  case PACKET_EDIT_TILE_TERRAIN:
    return receive_packet_edit_tile_terrain(pc, type);

  case PACKET_EDIT_TILE_RESOURCE:
    return receive_packet_edit_tile_resource(pc, type);

  case PACKET_EDIT_TILE_SPECIAL:
    return receive_packet_edit_tile_special(pc, type);

  case PACKET_EDIT_TILE_BASE:
    return receive_packet_edit_tile_base(pc, type);

  case PACKET_EDIT_STARTPOS:
    return receive_packet_edit_startpos(pc, type);

  case PACKET_EDIT_TILE:
    return receive_packet_edit_tile(pc, type);

  case PACKET_EDIT_UNIT_CREATE:
    return receive_packet_edit_unit_create(pc, type);

  case PACKET_EDIT_UNIT_REMOVE:
    return receive_packet_edit_unit_remove(pc, type);

  case PACKET_EDIT_UNIT_REMOVE_BY_ID:
    return receive_packet_edit_unit_remove_by_id(pc, type);

  case PACKET_EDIT_UNIT:
    return receive_packet_edit_unit(pc, type);

  case PACKET_EDIT_CITY_CREATE:
    return receive_packet_edit_city_create(pc, type);

  case PACKET_EDIT_CITY_REMOVE:
    return receive_packet_edit_city_remove(pc, type);

  case PACKET_EDIT_CITY:
    return receive_packet_edit_city(pc, type);

  case PACKET_EDIT_PLAYER_CREATE:
    return receive_packet_edit_player_create(pc, type);

  case PACKET_EDIT_PLAYER_REMOVE:
    return receive_packet_edit_player_remove(pc, type);

  case PACKET_EDIT_PLAYER:
    return receive_packet_edit_player(pc, type);

  case PACKET_EDIT_PLAYER_VISION:
    return receive_packet_edit_player_vision(pc, type);

  case PACKET_EDIT_GAME:
    return receive_packet_edit_game(pc, type);

  case PACKET_EDIT_OBJECT_CREATED:
    return receive_packet_edit_object_created(pc, type);

  default:
    freelog(LOG_ERROR, "unknown packet type %d received from %s",
	    type, conn_description(pc));
    remove_packet_from_buffer(pc->buffer);
    return NULL;
  };
}

const char *get_packet_name(enum packet_type type)
{
  switch(type) {

  case PACKET_PROCESSING_STARTED:
    return "PACKET_PROCESSING_STARTED";

  case PACKET_PROCESSING_FINISHED:
    return "PACKET_PROCESSING_FINISHED";

  case PACKET_FREEZE_HINT:
    return "PACKET_FREEZE_HINT";

  case PACKET_THAW_HINT:
    return "PACKET_THAW_HINT";

  case PACKET_SERVER_JOIN_REQ:
    return "PACKET_SERVER_JOIN_REQ";

  case PACKET_SERVER_JOIN_REPLY:
    return "PACKET_SERVER_JOIN_REPLY";

  case PACKET_AUTHENTICATION_REQ:
    return "PACKET_AUTHENTICATION_REQ";

  case PACKET_AUTHENTICATION_REPLY:
    return "PACKET_AUTHENTICATION_REPLY";

  case PACKET_SERVER_SHUTDOWN:
    return "PACKET_SERVER_SHUTDOWN";

  case PACKET_NATION_SELECT_REQ:
    return "PACKET_NATION_SELECT_REQ";

  case PACKET_PLAYER_READY:
    return "PACKET_PLAYER_READY";

  case PACKET_ENDGAME_REPORT:
    return "PACKET_ENDGAME_REPORT";

  case PACKET_TILE_INFO:
    return "PACKET_TILE_INFO";

  case PACKET_GAME_INFO:
    return "PACKET_GAME_INFO";

  case PACKET_MAP_INFO:
    return "PACKET_MAP_INFO";

  case PACKET_NUKE_TILE_INFO:
    return "PACKET_NUKE_TILE_INFO";

  case PACKET_CHAT_MSG:
    return "PACKET_CHAT_MSG";

  case PACKET_CHAT_MSG_REQ:
    return "PACKET_CHAT_MSG_REQ";

  case PACKET_CONNECT_MSG:
    return "PACKET_CONNECT_MSG";

  case PACKET_CITY_REMOVE:
    return "PACKET_CITY_REMOVE";

  case PACKET_CITY_INFO:
    return "PACKET_CITY_INFO";

  case PACKET_CITY_SHORT_INFO:
    return "PACKET_CITY_SHORT_INFO";

  case PACKET_CITY_SELL:
    return "PACKET_CITY_SELL";

  case PACKET_CITY_BUY:
    return "PACKET_CITY_BUY";

  case PACKET_CITY_CHANGE:
    return "PACKET_CITY_CHANGE";

  case PACKET_CITY_WORKLIST:
    return "PACKET_CITY_WORKLIST";

  case PACKET_CITY_MAKE_SPECIALIST:
    return "PACKET_CITY_MAKE_SPECIALIST";

  case PACKET_CITY_MAKE_WORKER:
    return "PACKET_CITY_MAKE_WORKER";

  case PACKET_CITY_CHANGE_SPECIALIST:
    return "PACKET_CITY_CHANGE_SPECIALIST";

  case PACKET_CITY_RENAME:
    return "PACKET_CITY_RENAME";

  case PACKET_CITY_OPTIONS_REQ:
    return "PACKET_CITY_OPTIONS_REQ";

  case PACKET_CITY_REFRESH:
    return "PACKET_CITY_REFRESH";

  case PACKET_CITY_NAME_SUGGESTION_REQ:
    return "PACKET_CITY_NAME_SUGGESTION_REQ";

  case PACKET_CITY_NAME_SUGGESTION_INFO:
    return "PACKET_CITY_NAME_SUGGESTION_INFO";

  case PACKET_CITY_SABOTAGE_LIST:
    return "PACKET_CITY_SABOTAGE_LIST";

  case PACKET_PLAYER_REMOVE:
    return "PACKET_PLAYER_REMOVE";

  case PACKET_PLAYER_INFO:
    return "PACKET_PLAYER_INFO";

  case PACKET_PLAYER_PHASE_DONE:
    return "PACKET_PLAYER_PHASE_DONE";

  case PACKET_PLAYER_RATES:
    return "PACKET_PLAYER_RATES";

  case PACKET_PLAYER_CHANGE_GOVERNMENT:
    return "PACKET_PLAYER_CHANGE_GOVERNMENT";

  case PACKET_PLAYER_RESEARCH:
    return "PACKET_PLAYER_RESEARCH";

  case PACKET_PLAYER_TECH_GOAL:
    return "PACKET_PLAYER_TECH_GOAL";

  case PACKET_PLAYER_ATTRIBUTE_BLOCK:
    return "PACKET_PLAYER_ATTRIBUTE_BLOCK";

  case PACKET_PLAYER_ATTRIBUTE_CHUNK:
    return "PACKET_PLAYER_ATTRIBUTE_CHUNK";

  case PACKET_UNIT_REMOVE:
    return "PACKET_UNIT_REMOVE";

  case PACKET_UNIT_INFO:
    return "PACKET_UNIT_INFO";

  case PACKET_UNIT_SHORT_INFO:
    return "PACKET_UNIT_SHORT_INFO";

  case PACKET_UNIT_COMBAT_INFO:
    return "PACKET_UNIT_COMBAT_INFO";

  case PACKET_UNIT_MOVE:
    return "PACKET_UNIT_MOVE";

  case PACKET_GOTO_PATH_REQ:
    return "PACKET_GOTO_PATH_REQ";

  case PACKET_GOTO_PATH:
    return "PACKET_GOTO_PATH";

  case PACKET_UNIT_BUILD_CITY:
    return "PACKET_UNIT_BUILD_CITY";

  case PACKET_UNIT_DISBAND:
    return "PACKET_UNIT_DISBAND";

  case PACKET_UNIT_CHANGE_HOMECITY:
    return "PACKET_UNIT_CHANGE_HOMECITY";

  case PACKET_UNIT_ESTABLISH_TRADE:
    return "PACKET_UNIT_ESTABLISH_TRADE";

  case PACKET_UNIT_BATTLEGROUP:
    return "PACKET_UNIT_BATTLEGROUP";

  case PACKET_UNIT_HELP_BUILD_WONDER:
    return "PACKET_UNIT_HELP_BUILD_WONDER";

  case PACKET_UNIT_ORDERS:
    return "PACKET_UNIT_ORDERS";

  case PACKET_UNIT_AUTOSETTLERS:
    return "PACKET_UNIT_AUTOSETTLERS";

  case PACKET_UNIT_LOAD:
    return "PACKET_UNIT_LOAD";

  case PACKET_UNIT_UNLOAD:
    return "PACKET_UNIT_UNLOAD";

  case PACKET_UNIT_UPGRADE:
    return "PACKET_UNIT_UPGRADE";

  case PACKET_UNIT_TRANSFORM:
    return "PACKET_UNIT_TRANSFORM";

  case PACKET_UNIT_NUKE:
    return "PACKET_UNIT_NUKE";

  case PACKET_UNIT_PARADROP_TO:
    return "PACKET_UNIT_PARADROP_TO";

  case PACKET_UNIT_AIRLIFT:
    return "PACKET_UNIT_AIRLIFT";

  case PACKET_UNIT_DIPLOMAT_QUERY:
    return "PACKET_UNIT_DIPLOMAT_QUERY";

  case PACKET_UNIT_TYPE_UPGRADE:
    return "PACKET_UNIT_TYPE_UPGRADE";

  case PACKET_UNIT_DIPLOMAT_ACTION:
    return "PACKET_UNIT_DIPLOMAT_ACTION";

  case PACKET_UNIT_DIPLOMAT_ANSWER:
    return "PACKET_UNIT_DIPLOMAT_ANSWER";

  case PACKET_UNIT_CHANGE_ACTIVITY:
    return "PACKET_UNIT_CHANGE_ACTIVITY";

  case PACKET_DIPLOMACY_INIT_MEETING_REQ:
    return "PACKET_DIPLOMACY_INIT_MEETING_REQ";

  case PACKET_DIPLOMACY_INIT_MEETING:
    return "PACKET_DIPLOMACY_INIT_MEETING";

  case PACKET_DIPLOMACY_CANCEL_MEETING_REQ:
    return "PACKET_DIPLOMACY_CANCEL_MEETING_REQ";

  case PACKET_DIPLOMACY_CANCEL_MEETING:
    return "PACKET_DIPLOMACY_CANCEL_MEETING";

  case PACKET_DIPLOMACY_CREATE_CLAUSE_REQ:
    return "PACKET_DIPLOMACY_CREATE_CLAUSE_REQ";

  case PACKET_DIPLOMACY_CREATE_CLAUSE:
    return "PACKET_DIPLOMACY_CREATE_CLAUSE";

  case PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ:
    return "PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ";

  case PACKET_DIPLOMACY_REMOVE_CLAUSE:
    return "PACKET_DIPLOMACY_REMOVE_CLAUSE";

  case PACKET_DIPLOMACY_ACCEPT_TREATY_REQ:
    return "PACKET_DIPLOMACY_ACCEPT_TREATY_REQ";

  case PACKET_DIPLOMACY_ACCEPT_TREATY:
    return "PACKET_DIPLOMACY_ACCEPT_TREATY";

  case PACKET_DIPLOMACY_CANCEL_PACT:
    return "PACKET_DIPLOMACY_CANCEL_PACT";

  case PACKET_PAGE_MSG:
    return "PACKET_PAGE_MSG";

  case PACKET_REPORT_REQ:
    return "PACKET_REPORT_REQ";

  case PACKET_CONN_INFO:
    return "PACKET_CONN_INFO";

  case PACKET_CONN_PING_INFO:
    return "PACKET_CONN_PING_INFO";

  case PACKET_CONN_PING:
    return "PACKET_CONN_PING";

  case PACKET_CONN_PONG:
    return "PACKET_CONN_PONG";

  case PACKET_CLIENT_INFO:
    return "PACKET_CLIENT_INFO";

  case PACKET_END_PHASE:
    return "PACKET_END_PHASE";

  case PACKET_START_PHASE:
    return "PACKET_START_PHASE";

  case PACKET_NEW_YEAR:
    return "PACKET_NEW_YEAR";

  case PACKET_BEGIN_TURN:
    return "PACKET_BEGIN_TURN";

  case PACKET_END_TURN:
    return "PACKET_END_TURN";

  case PACKET_FREEZE_CLIENT:
    return "PACKET_FREEZE_CLIENT";

  case PACKET_THAW_CLIENT:
    return "PACKET_THAW_CLIENT";

  case PACKET_SPACESHIP_LAUNCH:
    return "PACKET_SPACESHIP_LAUNCH";

  case PACKET_SPACESHIP_PLACE:
    return "PACKET_SPACESHIP_PLACE";

  case PACKET_SPACESHIP_INFO:
    return "PACKET_SPACESHIP_INFO";

  case PACKET_RULESET_UNIT:
    return "PACKET_RULESET_UNIT";

  case PACKET_RULESET_GAME:
    return "PACKET_RULESET_GAME";

  case PACKET_RULESET_SPECIALIST:
    return "PACKET_RULESET_SPECIALIST";

  case PACKET_RULESET_GOVERNMENT_RULER_TITLE:
    return "PACKET_RULESET_GOVERNMENT_RULER_TITLE";

  case PACKET_RULESET_TECH:
    return "PACKET_RULESET_TECH";

  case PACKET_RULESET_GOVERNMENT:
    return "PACKET_RULESET_GOVERNMENT";

  case PACKET_RULESET_TERRAIN_CONTROL:
    return "PACKET_RULESET_TERRAIN_CONTROL";

  case PACKET_RULESET_NATION_GROUPS:
    return "PACKET_RULESET_NATION_GROUPS";

  case PACKET_RULESET_NATION:
    return "PACKET_RULESET_NATION";

  case PACKET_RULESET_CITY:
    return "PACKET_RULESET_CITY";

  case PACKET_RULESET_BUILDING:
    return "PACKET_RULESET_BUILDING";

  case PACKET_RULESET_TERRAIN:
    return "PACKET_RULESET_TERRAIN";

  case PACKET_RULESET_UNIT_CLASS:
    return "PACKET_RULESET_UNIT_CLASS";

  case PACKET_RULESET_BASE:
    return "PACKET_RULESET_BASE";

  case PACKET_RULESET_CONTROL:
    return "PACKET_RULESET_CONTROL";

  case PACKET_SINGLE_WANT_HACK_REQ:
    return "PACKET_SINGLE_WANT_HACK_REQ";

  case PACKET_SINGLE_WANT_HACK_REPLY:
    return "PACKET_SINGLE_WANT_HACK_REPLY";

  case PACKET_RULESET_CHOICES:
    return "PACKET_RULESET_CHOICES";

  case PACKET_GAME_LOAD:
    return "PACKET_GAME_LOAD";

  case PACKET_OPTIONS_SETTABLE_CONTROL:
    return "PACKET_OPTIONS_SETTABLE_CONTROL";

  case PACKET_OPTIONS_SETTABLE:
    return "PACKET_OPTIONS_SETTABLE";

  case PACKET_RULESET_EFFECT:
    return "PACKET_RULESET_EFFECT";

  case PACKET_RULESET_EFFECT_REQ:
    return "PACKET_RULESET_EFFECT_REQ";

  case PACKET_RULESET_RESOURCE:
    return "PACKET_RULESET_RESOURCE";

  case PACKET_SCENARIO_INFO:
    return "PACKET_SCENARIO_INFO";

  case PACKET_SAVE_SCENARIO:
    return "PACKET_SAVE_SCENARIO";

  case PACKET_VOTE_NEW:
    return "PACKET_VOTE_NEW";

  case PACKET_VOTE_UPDATE:
    return "PACKET_VOTE_UPDATE";

  case PACKET_VOTE_REMOVE:
    return "PACKET_VOTE_REMOVE";

  case PACKET_VOTE_RESOLVE:
    return "PACKET_VOTE_RESOLVE";

  case PACKET_VOTE_SUBMIT:
    return "PACKET_VOTE_SUBMIT";

  case PACKET_EDIT_MODE:
    return "PACKET_EDIT_MODE";

  case PACKET_EDIT_RECALCULATE_BORDERS:
    return "PACKET_EDIT_RECALCULATE_BORDERS";

  case PACKET_EDIT_CHECK_TILES:
    return "PACKET_EDIT_CHECK_TILES";

  case PACKET_EDIT_TOGGLE_FOGOFWAR:
    return "PACKET_EDIT_TOGGLE_FOGOFWAR";

  case PACKET_EDIT_TILE_TERRAIN:
    return "PACKET_EDIT_TILE_TERRAIN";

  case PACKET_EDIT_TILE_RESOURCE:
    return "PACKET_EDIT_TILE_RESOURCE";

  case PACKET_EDIT_TILE_SPECIAL:
    return "PACKET_EDIT_TILE_SPECIAL";

  case PACKET_EDIT_TILE_BASE:
    return "PACKET_EDIT_TILE_BASE";

  case PACKET_EDIT_STARTPOS:
    return "PACKET_EDIT_STARTPOS";

  case PACKET_EDIT_TILE:
    return "PACKET_EDIT_TILE";

  case PACKET_EDIT_UNIT_CREATE:
    return "PACKET_EDIT_UNIT_CREATE";

  case PACKET_EDIT_UNIT_REMOVE:
    return "PACKET_EDIT_UNIT_REMOVE";

  case PACKET_EDIT_UNIT_REMOVE_BY_ID:
    return "PACKET_EDIT_UNIT_REMOVE_BY_ID";

  case PACKET_EDIT_UNIT:
    return "PACKET_EDIT_UNIT";

  case PACKET_EDIT_CITY_CREATE:
    return "PACKET_EDIT_CITY_CREATE";

  case PACKET_EDIT_CITY_REMOVE:
    return "PACKET_EDIT_CITY_REMOVE";

  case PACKET_EDIT_CITY:
    return "PACKET_EDIT_CITY";

  case PACKET_EDIT_PLAYER_CREATE:
    return "PACKET_EDIT_PLAYER_CREATE";

  case PACKET_EDIT_PLAYER_REMOVE:
    return "PACKET_EDIT_PLAYER_REMOVE";

  case PACKET_EDIT_PLAYER:
    return "PACKET_EDIT_PLAYER";

  case PACKET_EDIT_PLAYER_VISION:
    return "PACKET_EDIT_PLAYER_VISION";

  case PACKET_EDIT_GAME:
    return "PACKET_EDIT_GAME";

  case PACKET_EDIT_OBJECT_CREATED:
    return "PACKET_EDIT_OBJECT_CREATED";

  default:
    return "unknown";
  }
}

static struct packet_processing_started *receive_packet_processing_started_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_processing_started, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_processing_started_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_PROCESSING_STARTED);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_processing_started(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PROCESSING_STARTED] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PROCESSING_STARTED] = variant;
}

struct packet_processing_started *receive_packet_processing_started(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_processing_started at the server.");
  }
  ensure_valid_variant_packet_processing_started(pc);

  switch(pc->phs.variant[PACKET_PROCESSING_STARTED]) {
    case 100: return receive_packet_processing_started_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_processing_started(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_processing_started from the client.");
  }
  ensure_valid_variant_packet_processing_started(pc);

  switch(pc->phs.variant[PACKET_PROCESSING_STARTED]) {
    case 100: return send_packet_processing_started_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_processing_finished *receive_packet_processing_finished_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_processing_finished, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_processing_finished_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_PROCESSING_FINISHED);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_processing_finished(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PROCESSING_FINISHED] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PROCESSING_FINISHED] = variant;
}

struct packet_processing_finished *receive_packet_processing_finished(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_processing_finished at the server.");
  }
  ensure_valid_variant_packet_processing_finished(pc);

  switch(pc->phs.variant[PACKET_PROCESSING_FINISHED]) {
    case 100: return receive_packet_processing_finished_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_processing_finished(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_processing_finished from the client.");
  }
  ensure_valid_variant_packet_processing_finished(pc);

  switch(pc->phs.variant[PACKET_PROCESSING_FINISHED]) {
    case 100: return send_packet_processing_finished_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_freeze_hint *receive_packet_freeze_hint_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_freeze_hint, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_freeze_hint_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_FREEZE_HINT);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_freeze_hint(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_FREEZE_HINT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_FREEZE_HINT] = variant;
}

struct packet_freeze_hint *receive_packet_freeze_hint(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_freeze_hint at the server.");
  }
  ensure_valid_variant_packet_freeze_hint(pc);

  switch(pc->phs.variant[PACKET_FREEZE_HINT]) {
    case 100: return receive_packet_freeze_hint_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_freeze_hint(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_freeze_hint from the client.");
  }
  ensure_valid_variant_packet_freeze_hint(pc);

  switch(pc->phs.variant[PACKET_FREEZE_HINT]) {
    case 100: return send_packet_freeze_hint_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_freeze_hint(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_freeze_hint(pconn);
  } conn_list_iterate_end;
}

static struct packet_thaw_hint *receive_packet_thaw_hint_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_thaw_hint, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_thaw_hint_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_THAW_HINT);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_thaw_hint(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_THAW_HINT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_THAW_HINT] = variant;
}

struct packet_thaw_hint *receive_packet_thaw_hint(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_thaw_hint at the server.");
  }
  ensure_valid_variant_packet_thaw_hint(pc);

  switch(pc->phs.variant[PACKET_THAW_HINT]) {
    case 100: return receive_packet_thaw_hint_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_thaw_hint(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_thaw_hint from the client.");
  }
  ensure_valid_variant_packet_thaw_hint(pc);

  switch(pc->phs.variant[PACKET_THAW_HINT]) {
    case 100: return send_packet_thaw_hint_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_thaw_hint(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_thaw_hint(pconn);
  } conn_list_iterate_end;
}

static struct packet_server_join_req *receive_packet_server_join_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_server_join_req, real_packet);
  dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
  dio_get_string(&din, real_packet->capability, sizeof(real_packet->capability));
  dio_get_string(&din, real_packet->version_label, sizeof(real_packet->version_label));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->major_version = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->minor_version = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->patch_version = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_server_join_req_100(struct connection *pc, const struct packet_server_join_req *packet)
{
  const struct packet_server_join_req *real_packet = packet;
  SEND_PACKET_START(PACKET_SERVER_JOIN_REQ);

  dio_put_string(&dout, real_packet->username);
  dio_put_string(&dout, real_packet->capability);
  dio_put_string(&dout, real_packet->version_label);
  dio_put_uint32(&dout, real_packet->major_version);
  dio_put_uint32(&dout, real_packet->minor_version);
  dio_put_uint32(&dout, real_packet->patch_version);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_server_join_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SERVER_JOIN_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SERVER_JOIN_REQ] = variant;
}

struct packet_server_join_req *receive_packet_server_join_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_server_join_req at the client.");
  }
  ensure_valid_variant_packet_server_join_req(pc);

  switch(pc->phs.variant[PACKET_SERVER_JOIN_REQ]) {
    case 100: return receive_packet_server_join_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_server_join_req(struct connection *pc, const struct packet_server_join_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_server_join_req from the server.");
  }
  ensure_valid_variant_packet_server_join_req(pc);

  switch(pc->phs.variant[PACKET_SERVER_JOIN_REQ]) {
    case 100: return send_packet_server_join_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_server_join_req(struct connection *pc, const char *username, const char *capability, const char *version_label, int major_version, int minor_version, int patch_version)
{
  struct packet_server_join_req packet, *real_packet = &packet;

  sz_strlcpy(real_packet->username, username);
  sz_strlcpy(real_packet->capability, capability);
  sz_strlcpy(real_packet->version_label, version_label);
  real_packet->major_version = major_version;
  real_packet->minor_version = minor_version;
  real_packet->patch_version = patch_version;
  
  return send_packet_server_join_req(pc, real_packet);
}

static struct packet_server_join_reply *receive_packet_server_join_reply_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_server_join_reply, real_packet);
  dio_get_bool8(&din, &real_packet->you_can_join);
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
  dio_get_string(&din, real_packet->capability, sizeof(real_packet->capability));
  dio_get_string(&din, real_packet->challenge_file, sizeof(real_packet->challenge_file));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->conn_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_server_join_reply_100(struct connection *pc, const struct packet_server_join_reply *packet)
{
  const struct packet_server_join_reply *real_packet = packet;
  SEND_PACKET_START(PACKET_SERVER_JOIN_REPLY);

  dio_put_bool8(&dout, real_packet->you_can_join);
  dio_put_string(&dout, real_packet->message);
  dio_put_string(&dout, real_packet->capability);
  dio_put_string(&dout, real_packet->challenge_file);
  dio_put_uint8(&dout, real_packet->conn_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_server_join_reply(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SERVER_JOIN_REPLY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SERVER_JOIN_REPLY] = variant;
}

struct packet_server_join_reply *receive_packet_server_join_reply(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_server_join_reply at the server.");
  }
  ensure_valid_variant_packet_server_join_reply(pc);

  switch(pc->phs.variant[PACKET_SERVER_JOIN_REPLY]) {
    case 100: return receive_packet_server_join_reply_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_server_join_reply(struct connection *pc, const struct packet_server_join_reply *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_server_join_reply from the client.");
  }
  ensure_valid_variant_packet_server_join_reply(pc);

  switch(pc->phs.variant[PACKET_SERVER_JOIN_REPLY]) {
    case 100: return send_packet_server_join_reply_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_authentication_req *receive_packet_authentication_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_authentication_req, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_authentication_req_100(struct connection *pc, const struct packet_authentication_req *packet)
{
  const struct packet_authentication_req *real_packet = packet;
  SEND_PACKET_START(PACKET_AUTHENTICATION_REQ);

  dio_put_uint8(&dout, real_packet->type);
  dio_put_string(&dout, real_packet->message);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_authentication_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_AUTHENTICATION_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_AUTHENTICATION_REQ] = variant;
}

struct packet_authentication_req *receive_packet_authentication_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_authentication_req at the server.");
  }
  ensure_valid_variant_packet_authentication_req(pc);

  switch(pc->phs.variant[PACKET_AUTHENTICATION_REQ]) {
    case 100: return receive_packet_authentication_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_authentication_req(struct connection *pc, const struct packet_authentication_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_authentication_req from the client.");
  }
  ensure_valid_variant_packet_authentication_req(pc);

  switch(pc->phs.variant[PACKET_AUTHENTICATION_REQ]) {
    case 100: return send_packet_authentication_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_authentication_req(struct connection *pc, enum authentication_type type, const char *message)
{
  struct packet_authentication_req packet, *real_packet = &packet;

  real_packet->type = type;
  sz_strlcpy(real_packet->message, message);
  
  return send_packet_authentication_req(pc, real_packet);
}

static struct packet_authentication_reply *receive_packet_authentication_reply_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_authentication_reply, real_packet);
  dio_get_string(&din, real_packet->password, sizeof(real_packet->password));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_authentication_reply_100(struct connection *pc, const struct packet_authentication_reply *packet)
{
  const struct packet_authentication_reply *real_packet = packet;
  SEND_PACKET_START(PACKET_AUTHENTICATION_REPLY);

  dio_put_string(&dout, real_packet->password);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_authentication_reply(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_AUTHENTICATION_REPLY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_AUTHENTICATION_REPLY] = variant;
}

struct packet_authentication_reply *receive_packet_authentication_reply(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_authentication_reply at the client.");
  }
  ensure_valid_variant_packet_authentication_reply(pc);

  switch(pc->phs.variant[PACKET_AUTHENTICATION_REPLY]) {
    case 100: return receive_packet_authentication_reply_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_authentication_reply(struct connection *pc, const struct packet_authentication_reply *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_authentication_reply from the server.");
  }
  ensure_valid_variant_packet_authentication_reply(pc);

  switch(pc->phs.variant[PACKET_AUTHENTICATION_REPLY]) {
    case 100: return send_packet_authentication_reply_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_server_shutdown *receive_packet_server_shutdown_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_server_shutdown, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_server_shutdown_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_SERVER_SHUTDOWN);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_server_shutdown(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SERVER_SHUTDOWN] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SERVER_SHUTDOWN] = variant;
}

struct packet_server_shutdown *receive_packet_server_shutdown(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_server_shutdown at the server.");
  }
  ensure_valid_variant_packet_server_shutdown(pc);

  switch(pc->phs.variant[PACKET_SERVER_SHUTDOWN]) {
    case 100: return receive_packet_server_shutdown_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_server_shutdown(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_server_shutdown from the client.");
  }
  ensure_valid_variant_packet_server_shutdown(pc);

  switch(pc->phs.variant[PACKET_SERVER_SHUTDOWN]) {
    case 100: return send_packet_server_shutdown_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_server_shutdown(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_server_shutdown(pconn);
  } conn_list_iterate_end;
}

static struct packet_nation_select_req *receive_packet_nation_select_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_nation_select_req, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->player_no = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nation_no = readin;
  }
  dio_get_bool8(&din, &real_packet->is_male);
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_nation_select_req_100(struct connection *pc, const struct packet_nation_select_req *packet)
{
  const struct packet_nation_select_req *real_packet = packet;
  SEND_PACKET_START(PACKET_NATION_SELECT_REQ);

  dio_put_sint8(&dout, real_packet->player_no);
  dio_put_uint32(&dout, real_packet->nation_no);
  dio_put_bool8(&dout, real_packet->is_male);
  dio_put_string(&dout, real_packet->name);
  dio_put_uint8(&dout, real_packet->city_style);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_nation_select_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_NATION_SELECT_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_NATION_SELECT_REQ] = variant;
}

struct packet_nation_select_req *receive_packet_nation_select_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_nation_select_req at the client.");
  }
  ensure_valid_variant_packet_nation_select_req(pc);

  switch(pc->phs.variant[PACKET_NATION_SELECT_REQ]) {
    case 100: return receive_packet_nation_select_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_nation_select_req(struct connection *pc, const struct packet_nation_select_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_nation_select_req from the server.");
  }
  ensure_valid_variant_packet_nation_select_req(pc);

  switch(pc->phs.variant[PACKET_NATION_SELECT_REQ]) {
    case 100: return send_packet_nation_select_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_nation_select_req(struct connection *pc, int player_no, int nation_no, bool is_male, const char *name, int city_style)
{
  struct packet_nation_select_req packet, *real_packet = &packet;

  real_packet->player_no = player_no;
  real_packet->nation_no = nation_no;
  real_packet->is_male = is_male;
  sz_strlcpy(real_packet->name, name);
  real_packet->city_style = city_style;
  
  return send_packet_nation_select_req(pc, real_packet);
}

static struct packet_player_ready *receive_packet_player_ready_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_ready, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->player_no = readin;
  }
  dio_get_bool8(&din, &real_packet->is_ready);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_ready_100(struct connection *pc, const struct packet_player_ready *packet)
{
  const struct packet_player_ready *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_READY);

  dio_put_sint8(&dout, real_packet->player_no);
  dio_put_bool8(&dout, real_packet->is_ready);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_ready(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_READY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_READY] = variant;
}

struct packet_player_ready *receive_packet_player_ready(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_ready at the client.");
  }
  ensure_valid_variant_packet_player_ready(pc);

  switch(pc->phs.variant[PACKET_PLAYER_READY]) {
    case 100: return receive_packet_player_ready_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_ready(struct connection *pc, const struct packet_player_ready *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_ready from the server.");
  }
  ensure_valid_variant_packet_player_ready(pc);

  switch(pc->phs.variant[PACKET_PLAYER_READY]) {
    case 100: return send_packet_player_ready_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_player_ready(struct connection *pc, int player_no, bool is_ready)
{
  struct packet_player_ready packet, *real_packet = &packet;

  real_packet->player_no = player_no;
  real_packet->is_ready = is_ready;
  
  return send_packet_player_ready(pc, real_packet);
}

static struct packet_endgame_report *receive_packet_endgame_report_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_endgame_report, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->nscores = readin;
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->id[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->score[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->pop[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->bnp[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->mfg[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->cities[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->techs[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->mil_service[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->wonders[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->research[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->landarea[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->settledarea[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->literacy[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->spaceship[i] = readin;
  }
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_endgame_report_100(struct connection *pc, const struct packet_endgame_report *packet)
{
  const struct packet_endgame_report *real_packet = packet;
  SEND_PACKET_START(PACKET_ENDGAME_REPORT);

  dio_put_uint8(&dout, real_packet->nscores);

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_sint8(&dout, real_packet->id[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->score[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->pop[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->bnp[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->mfg[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->cities[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->techs[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->mil_service[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint8(&dout, real_packet->wonders[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->research[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->landarea[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->settledarea[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->literacy[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nscores; i++) {
        dio_put_uint32(&dout, real_packet->spaceship[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_endgame_report(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_ENDGAME_REPORT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_ENDGAME_REPORT] = variant;
}

struct packet_endgame_report *receive_packet_endgame_report(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_endgame_report at the server.");
  }
  ensure_valid_variant_packet_endgame_report(pc);

  switch(pc->phs.variant[PACKET_ENDGAME_REPORT]) {
    case 100: return receive_packet_endgame_report_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_endgame_report(struct connection *pc, const struct packet_endgame_report *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_endgame_report from the client.");
  }
  ensure_valid_variant_packet_endgame_report(pc);

  switch(pc->phs.variant[PACKET_ENDGAME_REPORT]) {
    case 100: return send_packet_endgame_report_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_endgame_report(struct conn_list *dest, const struct packet_endgame_report *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_endgame_report(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_tile_info *receive_packet_tile_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_tile_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->continent = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->known = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->worked = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->terrain = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->resource = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < S_LAST; i++) {
      dio_get_bool8(&din, &real_packet->special[i]);
    }
  }
  DIO_BV_GET(&din, real_packet->bases);
  dio_get_string(&din, real_packet->spec_sprite, sizeof(real_packet->spec_sprite));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nation_start = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_tile_info_100(struct connection *pc, bool force_send, const struct packet_tile_info *packet)
{
  const struct packet_tile_info *real_packet = packet;
  SEND_PACKET_START(PACKET_TILE_INFO);

  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_sint16(&dout, real_packet->continent);
  dio_put_uint8(&dout, real_packet->known);
  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint32(&dout, real_packet->worked);
  dio_put_uint8(&dout, real_packet->terrain);
  dio_put_uint8(&dout, real_packet->resource);

    {
      int i;

      for (i = 0; i < S_LAST; i++) {
        dio_put_bool8(&dout, real_packet->special[i]);
      }
    } 
DIO_BV_PUT(&dout, packet->bases);
  dio_put_string(&dout, real_packet->spec_sprite);
  dio_put_uint32(&dout, real_packet->nation_start);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_tile_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_TILE_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_TILE_INFO] = variant;
}

struct packet_tile_info *receive_packet_tile_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_tile_info at the server.");
  }
  ensure_valid_variant_packet_tile_info(pc);

  switch(pc->phs.variant[PACKET_TILE_INFO]) {
    case 100: return receive_packet_tile_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_tile_info(struct connection *pc, bool force_send, const struct packet_tile_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_tile_info from the client.");
  }
  ensure_valid_variant_packet_tile_info(pc);

  switch(pc->phs.variant[PACKET_TILE_INFO]) {
    case 100: return send_packet_tile_info_100(pc, force_send, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_tile_info(struct conn_list *dest, bool force_send, const struct packet_tile_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_tile_info(pconn, force_send, packet);
  } conn_list_iterate_end;
}

static struct packet_game_info *receive_packet_game_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_game_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->tech = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->skill_level = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->aifill = readin;
  }
  dio_get_bool8(&din, &real_packet->is_new_game);
  dio_get_bool8(&din, &real_packet->is_edit_mode);
  {
    int tmp;
    
    dio_get_uint32(&din, &tmp);
    real_packet->seconds_to_phasedone = (float)(tmp) / 10000.0;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->timeout = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->turn = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->phase = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->year = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->start_year = readin;
  }
  dio_get_bool8(&din, &real_packet->year_0_hack);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->end_turn = readin;
  }
  dio_get_string(&din, real_packet->positive_year_label, sizeof(real_packet->positive_year_label));
  dio_get_string(&din, real_packet->negative_year_label, sizeof(real_packet->negative_year_label));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->phase_mode = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->num_phases = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->min_players = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->max_players = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->globalwarming = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->heating = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->warminglevel = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nuclearwinter = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->cooling = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->coolinglevel = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->diplcost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->freecost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->conquercost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->angrycitizen = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->techpenalty = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->foodbox = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->shieldbox = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->sciencebox = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->diplomacy = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->dispersion = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->tcptimeout = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->netwait = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->pingtimeout = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->pingtime = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->diplchance = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->citymindist = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->civilwarsize = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->contactturns = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->rapturedelay = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->celebratesize = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->barbarianrate = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->onsetbarbarian = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->occupychance = readin;
  }
  dio_get_bool8(&din, &real_packet->autoattack);
  dio_get_bool8(&din, &real_packet->spacerace);
  dio_get_bool8(&din, &real_packet->endspaceship);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->aqueductloss = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->killcitizen = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->razechance = readin;
  }
  dio_get_bool8(&din, &real_packet->savepalace);
  dio_get_bool8(&din, &real_packet->natural_city_names);
  dio_get_bool8(&din, &real_packet->migration);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->mgr_turninterval = readin;
  }
  dio_get_bool8(&din, &real_packet->mgr_foodneeded);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->mgr_distance = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->mgr_nationchance = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->mgr_worldchance = readin;
  }
  dio_get_bool8(&din, &real_packet->turnblock);
  dio_get_bool8(&din, &real_packet->fixedlength);
  dio_get_bool8(&din, &real_packet->auto_ai_toggle);
  dio_get_bool8(&din, &real_packet->fogofwar);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->borders = readin;
  }
  dio_get_bool8(&din, &real_packet->happyborders);
  dio_get_bool8(&din, &real_packet->slow_invasions);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->add_to_size_limit = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->notradesize = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->fulltradesize = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->trademindist = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->allowed_city_names = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->palace_building = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->land_defend_building = readin;
  }
  dio_get_bool8(&din, &real_packet->changable_tax);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->forced_science = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->forced_luxury = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->forced_gold = readin;
  }
  dio_get_bool8(&din, &real_packet->vision_reveal_tiles);
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->min_city_center_output[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->min_dist_bw_cities = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->init_vis_radius_sq = readin;
  }
  dio_get_bool8(&din, &real_packet->pillage_select);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->nuke_contamination = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->gold_upkeep_style = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_GRANARY_INIS; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->granary_food_ini[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->granary_num_inis = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->granary_food_inc = readin;
  }
  dio_get_bool8(&din, &real_packet->illness_on);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->illness_min_size = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->illness_base_factor = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->illness_trade_infection = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->illness_pollution_factor = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->tech_cost_style = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->tech_leakage = readin;
  }
  dio_get_bool8(&din, &real_packet->killstack);
  dio_get_bool8(&din, &real_packet->tired_attack);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->border_city_radius_sq = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->border_size_effect = readin;
  }
  dio_get_bool8(&din, &real_packet->calendar_skip_0);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->upgrade_veteran_loss = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->autoupgrade_veteran_loss = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->incite_improvement_factor = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->incite_unit_factor = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->incite_total_factor = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->government_during_revolution_id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->revolution_length = readin;
  }
  {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->base_pollution = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->happy_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->food_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->base_bribe_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->base_incite_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->base_tech_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->ransom_gold = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->save_nturns = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->save_compress_level = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->save_compress_type = readin;
  }
  dio_get_string(&din, real_packet->start_units, sizeof(real_packet->start_units));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_teams = readin;
  }
  
  {
    int i;
  
    if(real_packet->num_teams > MAX_NUM_TEAMS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_teams = MAX_NUM_TEAMS;
    }
    for (i = 0; i < real_packet->num_teams; i++) {
      dio_get_string(&din, real_packet->team_names_orig[i], sizeof(real_packet->team_names_orig[i]));
    }
  }
  
  {
    int i;
  
    for (i = 0; i < A_LAST; i++) {
      dio_get_bool8(&din, &real_packet->global_advances[i]);
    }
  }
  
  {
    int i;
  
    for (i = 0; i < B_LAST; i++) {
      {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->great_wonder_owners[i] = readin;
  }
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_game_info_100(struct connection *pc, const struct packet_game_info *packet)
{
  const struct packet_game_info *real_packet = packet;
  SEND_PACKET_START(PACKET_GAME_INFO);

  dio_put_uint32(&dout, real_packet->gold);
  dio_put_uint32(&dout, real_packet->tech);
  dio_put_uint32(&dout, real_packet->skill_level);
  dio_put_uint8(&dout, real_packet->aifill);
  dio_put_bool8(&dout, real_packet->is_new_game);
  dio_put_bool8(&dout, real_packet->is_edit_mode);
  dio_put_uint32(&dout, (int)(real_packet->seconds_to_phasedone * 10000));
  dio_put_uint32(&dout, real_packet->timeout);
  dio_put_uint32(&dout, real_packet->turn);
  dio_put_uint32(&dout, real_packet->phase);
  dio_put_uint32(&dout, real_packet->year);
  dio_put_uint32(&dout, real_packet->start_year);
  dio_put_bool8(&dout, real_packet->year_0_hack);
  dio_put_uint32(&dout, real_packet->end_turn);
  dio_put_string(&dout, real_packet->positive_year_label);
  dio_put_string(&dout, real_packet->negative_year_label);
  dio_put_uint8(&dout, real_packet->phase_mode);
  dio_put_uint32(&dout, real_packet->num_phases);
  dio_put_sint8(&dout, real_packet->min_players);
  dio_put_sint8(&dout, real_packet->max_players);
  dio_put_uint32(&dout, real_packet->globalwarming);
  dio_put_uint32(&dout, real_packet->heating);
  dio_put_uint32(&dout, real_packet->warminglevel);
  dio_put_uint32(&dout, real_packet->nuclearwinter);
  dio_put_uint32(&dout, real_packet->cooling);
  dio_put_uint32(&dout, real_packet->coolinglevel);
  dio_put_uint8(&dout, real_packet->diplcost);
  dio_put_uint8(&dout, real_packet->freecost);
  dio_put_uint8(&dout, real_packet->conquercost);
  dio_put_uint8(&dout, real_packet->angrycitizen);
  dio_put_uint8(&dout, real_packet->techpenalty);
  dio_put_uint32(&dout, real_packet->foodbox);
  dio_put_uint32(&dout, real_packet->shieldbox);
  dio_put_uint32(&dout, real_packet->sciencebox);
  dio_put_uint8(&dout, real_packet->diplomacy);
  dio_put_uint8(&dout, real_packet->dispersion);
  dio_put_uint32(&dout, real_packet->tcptimeout);
  dio_put_uint32(&dout, real_packet->netwait);
  dio_put_uint32(&dout, real_packet->pingtimeout);
  dio_put_uint32(&dout, real_packet->pingtime);
  dio_put_uint8(&dout, real_packet->diplchance);
  dio_put_uint8(&dout, real_packet->citymindist);
  dio_put_uint8(&dout, real_packet->civilwarsize);
  dio_put_uint8(&dout, real_packet->contactturns);
  dio_put_uint8(&dout, real_packet->rapturedelay);
  dio_put_uint8(&dout, real_packet->celebratesize);
  dio_put_uint8(&dout, real_packet->barbarianrate);
  dio_put_uint32(&dout, real_packet->onsetbarbarian);
  dio_put_uint8(&dout, real_packet->occupychance);
  dio_put_bool8(&dout, real_packet->autoattack);
  dio_put_bool8(&dout, real_packet->spacerace);
  dio_put_bool8(&dout, real_packet->endspaceship);
  dio_put_uint8(&dout, real_packet->aqueductloss);
  dio_put_uint8(&dout, real_packet->killcitizen);
  dio_put_uint8(&dout, real_packet->razechance);
  dio_put_bool8(&dout, real_packet->savepalace);
  dio_put_bool8(&dout, real_packet->natural_city_names);
  dio_put_bool8(&dout, real_packet->migration);
  dio_put_uint8(&dout, real_packet->mgr_turninterval);
  dio_put_bool8(&dout, real_packet->mgr_foodneeded);
  dio_put_uint8(&dout, real_packet->mgr_distance);
  dio_put_uint8(&dout, real_packet->mgr_nationchance);
  dio_put_uint8(&dout, real_packet->mgr_worldchance);
  dio_put_bool8(&dout, real_packet->turnblock);
  dio_put_bool8(&dout, real_packet->fixedlength);
  dio_put_bool8(&dout, real_packet->auto_ai_toggle);
  dio_put_bool8(&dout, real_packet->fogofwar);
  dio_put_uint8(&dout, real_packet->borders);
  dio_put_bool8(&dout, real_packet->happyborders);
  dio_put_bool8(&dout, real_packet->slow_invasions);
  dio_put_uint8(&dout, real_packet->add_to_size_limit);
  dio_put_uint8(&dout, real_packet->notradesize);
  dio_put_uint8(&dout, real_packet->fulltradesize);
  dio_put_uint8(&dout, real_packet->trademindist);
  dio_put_uint8(&dout, real_packet->allowed_city_names);
  dio_put_uint8(&dout, real_packet->palace_building);
  dio_put_uint8(&dout, real_packet->land_defend_building);
  dio_put_bool8(&dout, real_packet->changable_tax);
  dio_put_uint8(&dout, real_packet->forced_science);
  dio_put_uint8(&dout, real_packet->forced_luxury);
  dio_put_uint8(&dout, real_packet->forced_gold);
  dio_put_bool8(&dout, real_packet->vision_reveal_tiles);

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint8(&dout, real_packet->min_city_center_output[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->min_dist_bw_cities);
  dio_put_uint8(&dout, real_packet->init_vis_radius_sq);
  dio_put_bool8(&dout, real_packet->pillage_select);
  dio_put_uint8(&dout, real_packet->nuke_contamination);
  dio_put_uint8(&dout, real_packet->gold_upkeep_style);

    {
      int i;

      for (i = 0; i < MAX_GRANARY_INIS; i++) {
        dio_put_uint8(&dout, real_packet->granary_food_ini[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->granary_num_inis);
  dio_put_uint8(&dout, real_packet->granary_food_inc);
  dio_put_bool8(&dout, real_packet->illness_on);
  dio_put_uint8(&dout, real_packet->illness_min_size);
  dio_put_uint8(&dout, real_packet->illness_base_factor);
  dio_put_uint8(&dout, real_packet->illness_trade_infection);
  dio_put_uint8(&dout, real_packet->illness_pollution_factor);
  dio_put_uint8(&dout, real_packet->tech_cost_style);
  dio_put_uint8(&dout, real_packet->tech_leakage);
  dio_put_bool8(&dout, real_packet->killstack);
  dio_put_bool8(&dout, real_packet->tired_attack);
  dio_put_uint8(&dout, real_packet->border_city_radius_sq);
  dio_put_uint8(&dout, real_packet->border_size_effect);
  dio_put_bool8(&dout, real_packet->calendar_skip_0);
  dio_put_uint8(&dout, real_packet->upgrade_veteran_loss);
  dio_put_uint8(&dout, real_packet->autoupgrade_veteran_loss);
  dio_put_uint32(&dout, real_packet->incite_improvement_factor);
  dio_put_uint32(&dout, real_packet->incite_unit_factor);
  dio_put_uint32(&dout, real_packet->incite_total_factor);
  dio_put_uint8(&dout, real_packet->government_during_revolution_id);
  dio_put_uint8(&dout, real_packet->revolution_length);
  dio_put_sint16(&dout, real_packet->base_pollution);
  dio_put_uint8(&dout, real_packet->happy_cost);
  dio_put_uint8(&dout, real_packet->food_cost);
  dio_put_uint32(&dout, real_packet->base_bribe_cost);
  dio_put_uint32(&dout, real_packet->base_incite_cost);
  dio_put_uint8(&dout, real_packet->base_tech_cost);
  dio_put_uint32(&dout, real_packet->ransom_gold);
  dio_put_uint8(&dout, real_packet->save_nturns);
  dio_put_uint8(&dout, real_packet->save_compress_level);
  dio_put_uint8(&dout, real_packet->save_compress_type);
  dio_put_string(&dout, real_packet->start_units);
  dio_put_uint8(&dout, real_packet->num_teams);

    {
      int i;

      for (i = 0; i < real_packet->num_teams; i++) {
        dio_put_string(&dout, real_packet->team_names_orig[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < A_LAST; i++) {
        dio_put_bool8(&dout, real_packet->global_advances[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < B_LAST; i++) {
        dio_put_sint8(&dout, real_packet->great_wonder_owners[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_game_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_GAME_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_GAME_INFO] = variant;
}

struct packet_game_info *receive_packet_game_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_game_info at the server.");
  }
  ensure_valid_variant_packet_game_info(pc);

  switch(pc->phs.variant[PACKET_GAME_INFO]) {
    case 100: return receive_packet_game_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_game_info(struct connection *pc, const struct packet_game_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_game_info from the client.");
  }
  ensure_valid_variant_packet_game_info(pc);

  switch(pc->phs.variant[PACKET_GAME_INFO]) {
    case 100: return send_packet_game_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_map_info *receive_packet_map_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_map_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->xsize = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->ysize = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->topology_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_map_info_100(struct connection *pc, const struct packet_map_info *packet)
{
  const struct packet_map_info *real_packet = packet;
  SEND_PACKET_START(PACKET_MAP_INFO);

  dio_put_uint32(&dout, real_packet->xsize);
  dio_put_uint32(&dout, real_packet->ysize);
  dio_put_uint8(&dout, real_packet->topology_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_map_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_MAP_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_MAP_INFO] = variant;
}

struct packet_map_info *receive_packet_map_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_map_info at the server.");
  }
  ensure_valid_variant_packet_map_info(pc);

  switch(pc->phs.variant[PACKET_MAP_INFO]) {
    case 100: return receive_packet_map_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_map_info(struct connection *pc, const struct packet_map_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_map_info from the client.");
  }
  ensure_valid_variant_packet_map_info(pc);

  switch(pc->phs.variant[PACKET_MAP_INFO]) {
    case 100: return send_packet_map_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_map_info(struct conn_list *dest, const struct packet_map_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_map_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_nuke_tile_info *receive_packet_nuke_tile_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_nuke_tile_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_nuke_tile_info_100(struct connection *pc, const struct packet_nuke_tile_info *packet)
{
  const struct packet_nuke_tile_info *real_packet = packet;
  SEND_PACKET_START(PACKET_NUKE_TILE_INFO);

  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_nuke_tile_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_NUKE_TILE_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_NUKE_TILE_INFO] = variant;
}

struct packet_nuke_tile_info *receive_packet_nuke_tile_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_nuke_tile_info at the server.");
  }
  ensure_valid_variant_packet_nuke_tile_info(pc);

  switch(pc->phs.variant[PACKET_NUKE_TILE_INFO]) {
    case 100: return receive_packet_nuke_tile_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_nuke_tile_info(struct connection *pc, const struct packet_nuke_tile_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_nuke_tile_info from the client.");
  }
  ensure_valid_variant_packet_nuke_tile_info(pc);

  switch(pc->phs.variant[PACKET_NUKE_TILE_INFO]) {
    case 100: return send_packet_nuke_tile_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_nuke_tile_info(struct conn_list *dest, const struct packet_nuke_tile_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_nuke_tile_info(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_nuke_tile_info(struct connection *pc, int x, int y)
{
  struct packet_nuke_tile_info packet, *real_packet = &packet;

  real_packet->x = x;
  real_packet->y = y;
  
  return send_packet_nuke_tile_info(pc, real_packet);
}

void dlsend_packet_nuke_tile_info(struct conn_list *dest, int x, int y)
{
  struct packet_nuke_tile_info packet, *real_packet = &packet;

  real_packet->x = x;
  real_packet->y = y;
  
  lsend_packet_nuke_tile_info(dest, real_packet);
}

static struct packet_chat_msg *receive_packet_chat_msg_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_chat_msg, real_packet);
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->event = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->conn_id = readin;
  }

  post_receive_packet_chat_msg(pc, real_packet);
  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_chat_msg_100(struct connection *pc, const struct packet_chat_msg *packet)
{
  const struct packet_chat_msg *real_packet = packet;
  SEND_PACKET_START(PACKET_CHAT_MSG);

  {
    struct packet_chat_msg *tmp = fc_malloc(sizeof(*tmp));

    *tmp = *packet;
    pre_send_packet_chat_msg(pc, tmp);
    real_packet = tmp;
  }

  dio_put_string(&dout, real_packet->message);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_sint32(&dout, real_packet->event);
  dio_put_uint8(&dout, real_packet->conn_id);


  if (real_packet != packet) {
    free((void *) real_packet);
  }
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_chat_msg(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CHAT_MSG] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CHAT_MSG] = variant;
}

struct packet_chat_msg *receive_packet_chat_msg(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_chat_msg at the server.");
  }
  ensure_valid_variant_packet_chat_msg(pc);

  switch(pc->phs.variant[PACKET_CHAT_MSG]) {
    case 100: return receive_packet_chat_msg_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_chat_msg(struct connection *pc, const struct packet_chat_msg *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_chat_msg from the client.");
  }
  ensure_valid_variant_packet_chat_msg(pc);

  switch(pc->phs.variant[PACKET_CHAT_MSG]) {
    case 100: return send_packet_chat_msg_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_chat_msg(struct conn_list *dest, const struct packet_chat_msg *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_chat_msg(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_chat_msg_req *receive_packet_chat_msg_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_chat_msg_req, real_packet);
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_chat_msg_req_100(struct connection *pc, const struct packet_chat_msg_req *packet)
{
  const struct packet_chat_msg_req *real_packet = packet;
  SEND_PACKET_START(PACKET_CHAT_MSG_REQ);

  dio_put_string(&dout, real_packet->message);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_chat_msg_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CHAT_MSG_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CHAT_MSG_REQ] = variant;
}

struct packet_chat_msg_req *receive_packet_chat_msg_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_chat_msg_req at the client.");
  }
  ensure_valid_variant_packet_chat_msg_req(pc);

  switch(pc->phs.variant[PACKET_CHAT_MSG_REQ]) {
    case 100: return receive_packet_chat_msg_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_chat_msg_req(struct connection *pc, const struct packet_chat_msg_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_chat_msg_req from the server.");
  }
  ensure_valid_variant_packet_chat_msg_req(pc);

  switch(pc->phs.variant[PACKET_CHAT_MSG_REQ]) {
    case 100: return send_packet_chat_msg_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_chat_msg_req(struct connection *pc, const char *message)
{
  struct packet_chat_msg_req packet, *real_packet = &packet;

  sz_strlcpy(real_packet->message, message);
  
  return send_packet_chat_msg_req(pc, real_packet);
}

static struct packet_connect_msg *receive_packet_connect_msg_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_connect_msg, real_packet);
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_connect_msg_100(struct connection *pc, const struct packet_connect_msg *packet)
{
  const struct packet_connect_msg *real_packet = packet;
  SEND_PACKET_START(PACKET_CONNECT_MSG);

  dio_put_string(&dout, real_packet->message);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_connect_msg(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CONNECT_MSG] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CONNECT_MSG] = variant;
}

struct packet_connect_msg *receive_packet_connect_msg(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_connect_msg at the server.");
  }
  ensure_valid_variant_packet_connect_msg(pc);

  switch(pc->phs.variant[PACKET_CONNECT_MSG]) {
    case 100: return receive_packet_connect_msg_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_connect_msg(struct connection *pc, const struct packet_connect_msg *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_connect_msg from the client.");
  }
  ensure_valid_variant_packet_connect_msg(pc);

  switch(pc->phs.variant[PACKET_CONNECT_MSG]) {
    case 100: return send_packet_connect_msg_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_connect_msg(struct connection *pc, const char *message)
{
  struct packet_connect_msg packet, *real_packet = &packet;

  sz_strlcpy(real_packet->message, message);
  
  return send_packet_connect_msg(pc, real_packet);
}

static struct packet_city_remove *receive_packet_city_remove_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_remove, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_remove_100(struct connection *pc, const struct packet_city_remove *packet)
{
  const struct packet_city_remove *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_REMOVE);

  dio_put_uint32(&dout, real_packet->city_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_remove(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_REMOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_REMOVE] = variant;
}

struct packet_city_remove *receive_packet_city_remove(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_remove at the server.");
  }
  ensure_valid_variant_packet_city_remove(pc);

  switch(pc->phs.variant[PACKET_CITY_REMOVE]) {
    case 100: return receive_packet_city_remove_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_remove(struct connection *pc, const struct packet_city_remove *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_remove from the client.");
  }
  ensure_valid_variant_packet_city_remove(pc);

  switch(pc->phs.variant[PACKET_CITY_REMOVE]) {
    case 100: return send_packet_city_remove_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_city_remove(struct conn_list *dest, const struct packet_city_remove *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_city_remove(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_city_remove(struct connection *pc, int city_id)
{
  struct packet_city_remove packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  
  return send_packet_city_remove(pc, real_packet);
}

void dlsend_packet_city_remove(struct conn_list *dest, int city_id)
{
  struct packet_city_remove packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  
  lsend_packet_city_remove(dest, real_packet);
}

static struct packet_city_info *receive_packet_city_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_happy[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_content[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_unhappy[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_angry[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->specialists_size = readin;
  }
  
  {
    int i;
  
    if(real_packet->specialists_size > SP_MAX) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->specialists_size = SP_MAX;
    }
    for (i = 0; i < real_packet->specialists_size; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->specialists[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->surplus[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->waste[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->unhappy_penalty[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->prod[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->citizen_base[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->usage[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->food_stock = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->shield_stock = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < NUM_TRADEROUTES; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->trade[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < NUM_TRADEROUTES; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->trade_value[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->pollution = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->illness = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->production_kind = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->production_value = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->turn_founded = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->turn_last_built = readin;
  }
  {
    int tmp;
    
    dio_get_uint32(&din, &tmp);
    real_packet->migration_score = (float)(tmp) / 10000.0;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->changed_from_kind = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->changed_from_value = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->before_change_shields = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->disbanded_shields = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->caravan_shields = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->last_turns_shield_surplus = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->airlift = readin;
  }
  dio_get_bool8(&din, &real_packet->did_buy);
  dio_get_bool8(&din, &real_packet->did_sell);
  dio_get_bool8(&din, &real_packet->was_happy);
  dio_get_bool8(&din, &real_packet->diplomat_investigate);
  dio_get_bool8(&din, &real_packet->walls);
  dio_get_string(&din, real_packet->can_build_unit, sizeof(real_packet->can_build_unit));
  dio_get_string(&din, real_packet->can_build_improvement, sizeof(real_packet->can_build_improvement));
  dio_get_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_info_100(struct connection *pc, const struct packet_city_info *packet)
{
  const struct packet_city_info *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_INFO);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint8(&dout, real_packet->size);

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_happy[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_content[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_unhappy[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_angry[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->specialists_size);

    {
      int i;

      for (i = 0; i < real_packet->specialists_size; i++) {
        dio_put_uint8(&dout, real_packet->specialists[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_sint16(&dout, real_packet->surplus[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint32(&dout, real_packet->waste[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_sint16(&dout, real_packet->unhappy_penalty[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint32(&dout, real_packet->prod[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_sint16(&dout, real_packet->citizen_base[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_sint16(&dout, real_packet->usage[i]);
      }
    } 
  dio_put_uint32(&dout, real_packet->food_stock);
  dio_put_uint32(&dout, real_packet->shield_stock);

    {
      int i;

      for (i = 0; i < NUM_TRADEROUTES; i++) {
        dio_put_uint32(&dout, real_packet->trade[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < NUM_TRADEROUTES; i++) {
        dio_put_uint8(&dout, real_packet->trade_value[i]);
      }
    } 
  dio_put_uint32(&dout, real_packet->pollution);
  dio_put_uint32(&dout, real_packet->illness);
  dio_put_uint8(&dout, real_packet->production_kind);
  dio_put_uint8(&dout, real_packet->production_value);
  dio_put_uint32(&dout, real_packet->turn_founded);
  dio_put_uint32(&dout, real_packet->turn_last_built);
  dio_put_uint32(&dout, (int)(real_packet->migration_score * 10000));
  dio_put_uint8(&dout, real_packet->changed_from_kind);
  dio_put_uint8(&dout, real_packet->changed_from_value);
  dio_put_uint32(&dout, real_packet->before_change_shields);
  dio_put_uint32(&dout, real_packet->disbanded_shields);
  dio_put_uint32(&dout, real_packet->caravan_shields);
  dio_put_uint32(&dout, real_packet->last_turns_shield_surplus);
  dio_put_uint8(&dout, real_packet->airlift);
  dio_put_bool8(&dout, real_packet->did_buy);
  dio_put_bool8(&dout, real_packet->did_sell);
  dio_put_bool8(&dout, real_packet->was_happy);
  dio_put_bool8(&dout, real_packet->diplomat_investigate);
  dio_put_bool8(&dout, real_packet->walls);
  dio_put_string(&dout, real_packet->can_build_unit);
  dio_put_string(&dout, real_packet->can_build_improvement);
  dio_put_string(&dout, real_packet->improvements);
  dio_put_string(&dout, real_packet->name);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_INFO] = variant;
}

struct packet_city_info *receive_packet_city_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_info at the server.");
  }
  ensure_valid_variant_packet_city_info(pc);

  switch(pc->phs.variant[PACKET_CITY_INFO]) {
    case 100: return receive_packet_city_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_info(struct connection *pc, const struct packet_city_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_info from the client.");
  }
  ensure_valid_variant_packet_city_info(pc);

  switch(pc->phs.variant[PACKET_CITY_INFO]) {
    case 100: return send_packet_city_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_city_info(struct conn_list *dest, const struct packet_city_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_city_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_city_short_info *receive_packet_city_short_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_short_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->tile_trade = readin;
  }
  dio_get_bool8(&din, &real_packet->occupied);
  dio_get_bool8(&din, &real_packet->walls);
  dio_get_bool8(&din, &real_packet->happy);
  dio_get_bool8(&din, &real_packet->unhappy);
  dio_get_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_short_info_100(struct connection *pc, const struct packet_city_short_info *packet)
{
  const struct packet_city_short_info *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_SHORT_INFO);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint8(&dout, real_packet->size);
  dio_put_uint32(&dout, real_packet->tile_trade);
  dio_put_bool8(&dout, real_packet->occupied);
  dio_put_bool8(&dout, real_packet->walls);
  dio_put_bool8(&dout, real_packet->happy);
  dio_put_bool8(&dout, real_packet->unhappy);
  dio_put_string(&dout, real_packet->improvements);
  dio_put_string(&dout, real_packet->name);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_short_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_SHORT_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_SHORT_INFO] = variant;
}

struct packet_city_short_info *receive_packet_city_short_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_short_info at the server.");
  }
  ensure_valid_variant_packet_city_short_info(pc);

  switch(pc->phs.variant[PACKET_CITY_SHORT_INFO]) {
    case 100: return receive_packet_city_short_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_short_info(struct connection *pc, const struct packet_city_short_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_short_info from the client.");
  }
  ensure_valid_variant_packet_city_short_info(pc);

  switch(pc->phs.variant[PACKET_CITY_SHORT_INFO]) {
    case 100: return send_packet_city_short_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_city_short_info(struct conn_list *dest, const struct packet_city_short_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_city_short_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_city_sell *receive_packet_city_sell_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_sell, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->build_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_sell_100(struct connection *pc, const struct packet_city_sell *packet)
{
  const struct packet_city_sell *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_SELL);

  dio_put_uint32(&dout, real_packet->city_id);
  dio_put_uint8(&dout, real_packet->build_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_sell(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_SELL] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_SELL] = variant;
}

struct packet_city_sell *receive_packet_city_sell(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_sell at the client.");
  }
  ensure_valid_variant_packet_city_sell(pc);

  switch(pc->phs.variant[PACKET_CITY_SELL]) {
    case 100: return receive_packet_city_sell_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_sell(struct connection *pc, const struct packet_city_sell *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_sell from the server.");
  }
  ensure_valid_variant_packet_city_sell(pc);

  switch(pc->phs.variant[PACKET_CITY_SELL]) {
    case 100: return send_packet_city_sell_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_sell(struct connection *pc, int city_id, int build_id)
{
  struct packet_city_sell packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->build_id = build_id;
  
  return send_packet_city_sell(pc, real_packet);
}

static struct packet_city_buy *receive_packet_city_buy_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_buy, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_buy_100(struct connection *pc, const struct packet_city_buy *packet)
{
  const struct packet_city_buy *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_BUY);

  dio_put_uint32(&dout, real_packet->city_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_buy(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_BUY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_BUY] = variant;
}

struct packet_city_buy *receive_packet_city_buy(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_buy at the client.");
  }
  ensure_valid_variant_packet_city_buy(pc);

  switch(pc->phs.variant[PACKET_CITY_BUY]) {
    case 100: return receive_packet_city_buy_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_buy(struct connection *pc, const struct packet_city_buy *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_buy from the server.");
  }
  ensure_valid_variant_packet_city_buy(pc);

  switch(pc->phs.variant[PACKET_CITY_BUY]) {
    case 100: return send_packet_city_buy_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_buy(struct connection *pc, int city_id)
{
  struct packet_city_buy packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  
  return send_packet_city_buy(pc, real_packet);
}

static struct packet_city_change *receive_packet_city_change_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_change, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->production_kind = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->production_value = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_change_100(struct connection *pc, const struct packet_city_change *packet)
{
  const struct packet_city_change *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_CHANGE);

  dio_put_uint32(&dout, real_packet->city_id);
  dio_put_uint8(&dout, real_packet->production_kind);
  dio_put_uint8(&dout, real_packet->production_value);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_change(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_CHANGE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_CHANGE] = variant;
}

struct packet_city_change *receive_packet_city_change(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_change at the client.");
  }
  ensure_valid_variant_packet_city_change(pc);

  switch(pc->phs.variant[PACKET_CITY_CHANGE]) {
    case 100: return receive_packet_city_change_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_change(struct connection *pc, const struct packet_city_change *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_change from the server.");
  }
  ensure_valid_variant_packet_city_change(pc);

  switch(pc->phs.variant[PACKET_CITY_CHANGE]) {
    case 100: return send_packet_city_change_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_change(struct connection *pc, int city_id, int production_kind, int production_value)
{
  struct packet_city_change packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->production_kind = production_kind;
  real_packet->production_value = production_value;
  
  return send_packet_city_change(pc, real_packet);
}

static struct packet_city_worklist *receive_packet_city_worklist_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_worklist, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  dio_get_worklist(&din, &real_packet->worklist);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_worklist_100(struct connection *pc, const struct packet_city_worklist *packet)
{
  const struct packet_city_worklist *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_WORKLIST);

  dio_put_uint32(&dout, real_packet->city_id);
  dio_put_worklist(&dout, &real_packet->worklist);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_worklist(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_WORKLIST] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_WORKLIST] = variant;
}

struct packet_city_worklist *receive_packet_city_worklist(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_worklist at the client.");
  }
  ensure_valid_variant_packet_city_worklist(pc);

  switch(pc->phs.variant[PACKET_CITY_WORKLIST]) {
    case 100: return receive_packet_city_worklist_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_worklist(struct connection *pc, const struct packet_city_worklist *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_worklist from the server.");
  }
  ensure_valid_variant_packet_city_worklist(pc);

  switch(pc->phs.variant[PACKET_CITY_WORKLIST]) {
    case 100: return send_packet_city_worklist_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_worklist(struct connection *pc, int city_id, struct worklist *worklist)
{
  struct packet_city_worklist packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  worklist_copy(&real_packet->worklist, worklist);
  
  return send_packet_city_worklist(pc, real_packet);
}

static struct packet_city_make_specialist *receive_packet_city_make_specialist_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_make_specialist, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->worker_x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->worker_y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_make_specialist_100(struct connection *pc, const struct packet_city_make_specialist *packet)
{
  const struct packet_city_make_specialist *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_MAKE_SPECIALIST);

  dio_put_uint32(&dout, real_packet->city_id);
  dio_put_uint32(&dout, real_packet->worker_x);
  dio_put_uint32(&dout, real_packet->worker_y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_make_specialist(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_MAKE_SPECIALIST] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_MAKE_SPECIALIST] = variant;
}

struct packet_city_make_specialist *receive_packet_city_make_specialist(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_make_specialist at the client.");
  }
  ensure_valid_variant_packet_city_make_specialist(pc);

  switch(pc->phs.variant[PACKET_CITY_MAKE_SPECIALIST]) {
    case 100: return receive_packet_city_make_specialist_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_make_specialist(struct connection *pc, const struct packet_city_make_specialist *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_make_specialist from the server.");
  }
  ensure_valid_variant_packet_city_make_specialist(pc);

  switch(pc->phs.variant[PACKET_CITY_MAKE_SPECIALIST]) {
    case 100: return send_packet_city_make_specialist_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_make_specialist(struct connection *pc, int city_id, int worker_x, int worker_y)
{
  struct packet_city_make_specialist packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->worker_x = worker_x;
  real_packet->worker_y = worker_y;
  
  return send_packet_city_make_specialist(pc, real_packet);
}

static struct packet_city_make_worker *receive_packet_city_make_worker_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_make_worker, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->worker_x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->worker_y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_make_worker_100(struct connection *pc, const struct packet_city_make_worker *packet)
{
  const struct packet_city_make_worker *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_MAKE_WORKER);

  dio_put_uint32(&dout, real_packet->city_id);
  dio_put_uint32(&dout, real_packet->worker_x);
  dio_put_uint32(&dout, real_packet->worker_y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_make_worker(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_MAKE_WORKER] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_MAKE_WORKER] = variant;
}

struct packet_city_make_worker *receive_packet_city_make_worker(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_make_worker at the client.");
  }
  ensure_valid_variant_packet_city_make_worker(pc);

  switch(pc->phs.variant[PACKET_CITY_MAKE_WORKER]) {
    case 100: return receive_packet_city_make_worker_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_make_worker(struct connection *pc, const struct packet_city_make_worker *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_make_worker from the server.");
  }
  ensure_valid_variant_packet_city_make_worker(pc);

  switch(pc->phs.variant[PACKET_CITY_MAKE_WORKER]) {
    case 100: return send_packet_city_make_worker_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_make_worker(struct connection *pc, int city_id, int worker_x, int worker_y)
{
  struct packet_city_make_worker packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->worker_x = worker_x;
  real_packet->worker_y = worker_y;
  
  return send_packet_city_make_worker(pc, real_packet);
}

static struct packet_city_change_specialist *receive_packet_city_change_specialist_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_change_specialist, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->from = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->to = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_change_specialist_100(struct connection *pc, const struct packet_city_change_specialist *packet)
{
  const struct packet_city_change_specialist *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_CHANGE_SPECIALIST);

  dio_put_uint32(&dout, real_packet->city_id);
  dio_put_uint8(&dout, real_packet->from);
  dio_put_uint8(&dout, real_packet->to);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_change_specialist(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_CHANGE_SPECIALIST] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_CHANGE_SPECIALIST] = variant;
}

struct packet_city_change_specialist *receive_packet_city_change_specialist(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_change_specialist at the client.");
  }
  ensure_valid_variant_packet_city_change_specialist(pc);

  switch(pc->phs.variant[PACKET_CITY_CHANGE_SPECIALIST]) {
    case 100: return receive_packet_city_change_specialist_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_change_specialist(struct connection *pc, const struct packet_city_change_specialist *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_change_specialist from the server.");
  }
  ensure_valid_variant_packet_city_change_specialist(pc);

  switch(pc->phs.variant[PACKET_CITY_CHANGE_SPECIALIST]) {
    case 100: return send_packet_city_change_specialist_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_change_specialist(struct connection *pc, int city_id, Specialist_type_id from, Specialist_type_id to)
{
  struct packet_city_change_specialist packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->from = from;
  real_packet->to = to;
  
  return send_packet_city_change_specialist(pc, real_packet);
}

static struct packet_city_rename *receive_packet_city_rename_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_rename, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_rename_100(struct connection *pc, const struct packet_city_rename *packet)
{
  const struct packet_city_rename *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_RENAME);

  dio_put_uint32(&dout, real_packet->city_id);
  dio_put_string(&dout, real_packet->name);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_rename(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_RENAME] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_RENAME] = variant;
}

struct packet_city_rename *receive_packet_city_rename(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_rename at the client.");
  }
  ensure_valid_variant_packet_city_rename(pc);

  switch(pc->phs.variant[PACKET_CITY_RENAME]) {
    case 100: return receive_packet_city_rename_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_rename(struct connection *pc, const struct packet_city_rename *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_rename from the server.");
  }
  ensure_valid_variant_packet_city_rename(pc);

  switch(pc->phs.variant[PACKET_CITY_RENAME]) {
    case 100: return send_packet_city_rename_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_rename(struct connection *pc, int city_id, const char *name)
{
  struct packet_city_rename packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  sz_strlcpy(real_packet->name, name);
  
  return send_packet_city_rename(pc, real_packet);
}

static struct packet_city_options_req *receive_packet_city_options_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_options_req, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  DIO_BV_GET(&din, real_packet->options);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_options_req_100(struct connection *pc, const struct packet_city_options_req *packet)
{
  const struct packet_city_options_req *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_OPTIONS_REQ);

  dio_put_uint32(&dout, real_packet->city_id);
DIO_BV_PUT(&dout, packet->options);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_options_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_OPTIONS_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_OPTIONS_REQ] = variant;
}

struct packet_city_options_req *receive_packet_city_options_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_options_req at the client.");
  }
  ensure_valid_variant_packet_city_options_req(pc);

  switch(pc->phs.variant[PACKET_CITY_OPTIONS_REQ]) {
    case 100: return receive_packet_city_options_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_options_req(struct connection *pc, const struct packet_city_options_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_options_req from the server.");
  }
  ensure_valid_variant_packet_city_options_req(pc);

  switch(pc->phs.variant[PACKET_CITY_OPTIONS_REQ]) {
    case 100: return send_packet_city_options_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_options_req(struct connection *pc, int city_id, bv_city_options options)
{
  struct packet_city_options_req packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->options = options;
  
  return send_packet_city_options_req(pc, real_packet);
}

static struct packet_city_refresh *receive_packet_city_refresh_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_refresh, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_refresh_100(struct connection *pc, const struct packet_city_refresh *packet)
{
  const struct packet_city_refresh *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_REFRESH);

  dio_put_uint32(&dout, real_packet->city_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_refresh(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_REFRESH] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_REFRESH] = variant;
}

struct packet_city_refresh *receive_packet_city_refresh(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_refresh at the client.");
  }
  ensure_valid_variant_packet_city_refresh(pc);

  switch(pc->phs.variant[PACKET_CITY_REFRESH]) {
    case 100: return receive_packet_city_refresh_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_refresh(struct connection *pc, const struct packet_city_refresh *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_refresh from the server.");
  }
  ensure_valid_variant_packet_city_refresh(pc);

  switch(pc->phs.variant[PACKET_CITY_REFRESH]) {
    case 100: return send_packet_city_refresh_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_refresh(struct connection *pc, int city_id)
{
  struct packet_city_refresh packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  
  return send_packet_city_refresh(pc, real_packet);
}

static struct packet_city_name_suggestion_req *receive_packet_city_name_suggestion_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_name_suggestion_req, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_name_suggestion_req_100(struct connection *pc, const struct packet_city_name_suggestion_req *packet)
{
  const struct packet_city_name_suggestion_req *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_NAME_SUGGESTION_REQ);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_name_suggestion_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_REQ] = variant;
}

struct packet_city_name_suggestion_req *receive_packet_city_name_suggestion_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_name_suggestion_req at the client.");
  }
  ensure_valid_variant_packet_city_name_suggestion_req(pc);

  switch(pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_REQ]) {
    case 100: return receive_packet_city_name_suggestion_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_name_suggestion_req(struct connection *pc, const struct packet_city_name_suggestion_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_name_suggestion_req from the server.");
  }
  ensure_valid_variant_packet_city_name_suggestion_req(pc);

  switch(pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_REQ]) {
    case 100: return send_packet_city_name_suggestion_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_city_name_suggestion_req(struct connection *pc, int unit_id)
{
  struct packet_city_name_suggestion_req packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_city_name_suggestion_req(pc, real_packet);
}

static struct packet_city_name_suggestion_info *receive_packet_city_name_suggestion_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_name_suggestion_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_name_suggestion_info_100(struct connection *pc, const struct packet_city_name_suggestion_info *packet)
{
  const struct packet_city_name_suggestion_info *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_NAME_SUGGESTION_INFO);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_string(&dout, real_packet->name);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_name_suggestion_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_INFO] = variant;
}

struct packet_city_name_suggestion_info *receive_packet_city_name_suggestion_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_name_suggestion_info at the server.");
  }
  ensure_valid_variant_packet_city_name_suggestion_info(pc);

  switch(pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_INFO]) {
    case 100: return receive_packet_city_name_suggestion_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_name_suggestion_info(struct connection *pc, const struct packet_city_name_suggestion_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_name_suggestion_info from the client.");
  }
  ensure_valid_variant_packet_city_name_suggestion_info(pc);

  switch(pc->phs.variant[PACKET_CITY_NAME_SUGGESTION_INFO]) {
    case 100: return send_packet_city_name_suggestion_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_city_name_suggestion_info(struct conn_list *dest, const struct packet_city_name_suggestion_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_city_name_suggestion_info(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_city_name_suggestion_info(struct connection *pc, int unit_id, const char *name)
{
  struct packet_city_name_suggestion_info packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  sz_strlcpy(real_packet->name, name);
  
  return send_packet_city_name_suggestion_info(pc, real_packet);
}

void dlsend_packet_city_name_suggestion_info(struct conn_list *dest, int unit_id, const char *name)
{
  struct packet_city_name_suggestion_info packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  sz_strlcpy(real_packet->name, name);
  
  lsend_packet_city_name_suggestion_info(dest, real_packet);
}

static struct packet_city_sabotage_list *receive_packet_city_sabotage_list_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_city_sabotage_list, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->diplomat_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }
  DIO_BV_GET(&din, real_packet->improvements);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_city_sabotage_list_100(struct connection *pc, const struct packet_city_sabotage_list *packet)
{
  const struct packet_city_sabotage_list *real_packet = packet;
  SEND_PACKET_START(PACKET_CITY_SABOTAGE_LIST);

  dio_put_uint32(&dout, real_packet->diplomat_id);
  dio_put_uint32(&dout, real_packet->city_id);
DIO_BV_PUT(&dout, packet->improvements);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_city_sabotage_list(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CITY_SABOTAGE_LIST] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CITY_SABOTAGE_LIST] = variant;
}

struct packet_city_sabotage_list *receive_packet_city_sabotage_list(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_sabotage_list at the server.");
  }
  ensure_valid_variant_packet_city_sabotage_list(pc);

  switch(pc->phs.variant[PACKET_CITY_SABOTAGE_LIST]) {
    case 100: return receive_packet_city_sabotage_list_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_city_sabotage_list(struct connection *pc, const struct packet_city_sabotage_list *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_city_sabotage_list from the client.");
  }
  ensure_valid_variant_packet_city_sabotage_list(pc);

  switch(pc->phs.variant[PACKET_CITY_SABOTAGE_LIST]) {
    case 100: return send_packet_city_sabotage_list_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_city_sabotage_list(struct conn_list *dest, const struct packet_city_sabotage_list *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_city_sabotage_list(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_player_remove *receive_packet_player_remove_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_remove, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->playerno = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_remove_100(struct connection *pc, const struct packet_player_remove *packet)
{
  const struct packet_player_remove *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_REMOVE);

  dio_put_sint8(&dout, real_packet->playerno);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_remove(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_REMOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_REMOVE] = variant;
}

struct packet_player_remove *receive_packet_player_remove(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_remove at the server.");
  }
  ensure_valid_variant_packet_player_remove(pc);

  switch(pc->phs.variant[PACKET_PLAYER_REMOVE]) {
    case 100: return receive_packet_player_remove_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_remove(struct connection *pc, const struct packet_player_remove *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_remove from the client.");
  }
  ensure_valid_variant_packet_player_remove(pc);

  switch(pc->phs.variant[PACKET_PLAYER_REMOVE]) {
    case 100: return send_packet_player_remove_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_player_remove(struct connection *pc, int playerno)
{
  struct packet_player_remove packet, *real_packet = &packet;

  real_packet->playerno = playerno;
  
  return send_packet_player_remove(pc, real_packet);
}

static struct packet_player_info *receive_packet_player_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_info, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->playerno = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->score = readin;
  }
  dio_get_bool8(&din, &real_packet->is_male);
  dio_get_bool8(&din, &real_packet->was_created);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_bool8(&din, &real_packet->embassy[i]);
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nation = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
  }
  dio_get_bool8(&din, &real_packet->is_ready);
  dio_get_bool8(&din, &real_packet->phase_done);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nturns_idle = readin;
  }
  dio_get_bool8(&din, &real_packet->is_alive);
  
  {
    int i;
  
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
    }
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->tax = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->science = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->luxury = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->bulbs_last_turn = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->current_research_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->science_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->future_tech = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
  }
  dio_get_bool8(&din, &real_packet->is_connected);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->revolution_finishes = readin;
  }
  dio_get_bool8(&din, &real_packet->ai);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ai_skill_level = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
  }
  dio_get_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
  
  {
    int i;
  
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->love[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < B_LAST; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->wonders[i] = readin;
  }
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_info_100(struct connection *pc, const struct packet_player_info *packet)
{
  const struct packet_player_info *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_INFO);

  dio_put_sint8(&dout, real_packet->playerno);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->username);
  dio_put_uint32(&dout, real_packet->score);
  dio_put_bool8(&dout, real_packet->is_male);
  dio_put_bool8(&dout, real_packet->was_created);
  dio_put_uint8(&dout, real_packet->government);
  dio_put_uint8(&dout, real_packet->target_government);

    {
      int i;

      for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
        dio_put_bool8(&dout, real_packet->embassy[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->city_style);
  dio_put_uint32(&dout, real_packet->nation);
  dio_put_uint8(&dout, real_packet->team);
  dio_put_bool8(&dout, real_packet->is_ready);
  dio_put_bool8(&dout, real_packet->phase_done);
  dio_put_uint32(&dout, real_packet->nturns_idle);
  dio_put_bool8(&dout, real_packet->is_alive);

    {
      int i;

      for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
        dio_put_diplstate(&dout, &real_packet->diplstates[i]);
      }
    } 
  dio_put_uint32(&dout, real_packet->gold);
  dio_put_uint32(&dout, real_packet->tax);
  dio_put_uint32(&dout, real_packet->science);
  dio_put_uint32(&dout, real_packet->luxury);
  dio_put_uint32(&dout, real_packet->bulbs_last_turn);
  dio_put_uint32(&dout, real_packet->bulbs_researched);
  dio_put_uint32(&dout, real_packet->techs_researched);
  dio_put_uint32(&dout, real_packet->current_research_cost);
  dio_put_uint8(&dout, real_packet->researching);
  dio_put_uint32(&dout, real_packet->science_cost);
  dio_put_uint32(&dout, real_packet->future_tech);
  dio_put_uint8(&dout, real_packet->tech_goal);
  dio_put_bool8(&dout, real_packet->is_connected);
  dio_put_uint32(&dout, real_packet->revolution_finishes);
  dio_put_bool8(&dout, real_packet->ai);
  dio_put_uint8(&dout, real_packet->ai_skill_level);
  dio_put_uint8(&dout, real_packet->barbarian_type);
  dio_put_uint32(&dout, real_packet->gives_shared_vision);
  dio_put_string(&dout, real_packet->inventions);

    {
      int i;

      for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
        dio_put_sint16(&dout, real_packet->love[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < B_LAST; i++) {
        dio_put_uint32(&dout, real_packet->wonders[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_INFO] = variant;
}

struct packet_player_info *receive_packet_player_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_info at the server.");
  }
  ensure_valid_variant_packet_player_info(pc);

  switch(pc->phs.variant[PACKET_PLAYER_INFO]) {
    case 100: return receive_packet_player_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_info(struct connection *pc, const struct packet_player_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_info from the client.");
  }
  ensure_valid_variant_packet_player_info(pc);

  switch(pc->phs.variant[PACKET_PLAYER_INFO]) {
    case 100: return send_packet_player_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_player_phase_done *receive_packet_player_phase_done_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_phase_done, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->turn = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_phase_done_100(struct connection *pc, const struct packet_player_phase_done *packet)
{
  const struct packet_player_phase_done *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_PHASE_DONE);

  dio_put_uint32(&dout, real_packet->turn);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_phase_done(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_PHASE_DONE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_PHASE_DONE] = variant;
}

struct packet_player_phase_done *receive_packet_player_phase_done(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_phase_done at the client.");
  }
  ensure_valid_variant_packet_player_phase_done(pc);

  switch(pc->phs.variant[PACKET_PLAYER_PHASE_DONE]) {
    case 100: return receive_packet_player_phase_done_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_phase_done(struct connection *pc, const struct packet_player_phase_done *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_phase_done from the server.");
  }
  ensure_valid_variant_packet_player_phase_done(pc);

  switch(pc->phs.variant[PACKET_PLAYER_PHASE_DONE]) {
    case 100: return send_packet_player_phase_done_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_player_phase_done(struct connection *pc, int turn)
{
  struct packet_player_phase_done packet, *real_packet = &packet;

  real_packet->turn = turn;
  
  return send_packet_player_phase_done(pc, real_packet);
}

static struct packet_player_rates *receive_packet_player_rates_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_rates, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->tax = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->luxury = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->science = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_rates_100(struct connection *pc, const struct packet_player_rates *packet)
{
  const struct packet_player_rates *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_RATES);

  dio_put_uint32(&dout, real_packet->tax);
  dio_put_uint32(&dout, real_packet->luxury);
  dio_put_uint32(&dout, real_packet->science);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_rates(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_RATES] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_RATES] = variant;
}

struct packet_player_rates *receive_packet_player_rates(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_rates at the client.");
  }
  ensure_valid_variant_packet_player_rates(pc);

  switch(pc->phs.variant[PACKET_PLAYER_RATES]) {
    case 100: return receive_packet_player_rates_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_rates(struct connection *pc, const struct packet_player_rates *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_rates from the server.");
  }
  ensure_valid_variant_packet_player_rates(pc);

  switch(pc->phs.variant[PACKET_PLAYER_RATES]) {
    case 100: return send_packet_player_rates_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_player_rates(struct connection *pc, int tax, int luxury, int science)
{
  struct packet_player_rates packet, *real_packet = &packet;

  real_packet->tax = tax;
  real_packet->luxury = luxury;
  real_packet->science = science;
  
  return send_packet_player_rates(pc, real_packet);
}

static struct packet_player_change_government *receive_packet_player_change_government_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_change_government, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_change_government_100(struct connection *pc, const struct packet_player_change_government *packet)
{
  const struct packet_player_change_government *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_CHANGE_GOVERNMENT);

  dio_put_uint8(&dout, real_packet->government);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_change_government(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_CHANGE_GOVERNMENT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_CHANGE_GOVERNMENT] = variant;
}

struct packet_player_change_government *receive_packet_player_change_government(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_change_government at the client.");
  }
  ensure_valid_variant_packet_player_change_government(pc);

  switch(pc->phs.variant[PACKET_PLAYER_CHANGE_GOVERNMENT]) {
    case 100: return receive_packet_player_change_government_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_change_government(struct connection *pc, const struct packet_player_change_government *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_change_government from the server.");
  }
  ensure_valid_variant_packet_player_change_government(pc);

  switch(pc->phs.variant[PACKET_PLAYER_CHANGE_GOVERNMENT]) {
    case 100: return send_packet_player_change_government_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_player_change_government(struct connection *pc, int government)
{
  struct packet_player_change_government packet, *real_packet = &packet;

  real_packet->government = government;
  
  return send_packet_player_change_government(pc, real_packet);
}

static struct packet_player_research *receive_packet_player_research_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_research, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->tech = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_research_100(struct connection *pc, const struct packet_player_research *packet)
{
  const struct packet_player_research *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_RESEARCH);

  dio_put_uint8(&dout, real_packet->tech);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_research(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_RESEARCH] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_RESEARCH] = variant;
}

struct packet_player_research *receive_packet_player_research(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_research at the client.");
  }
  ensure_valid_variant_packet_player_research(pc);

  switch(pc->phs.variant[PACKET_PLAYER_RESEARCH]) {
    case 100: return receive_packet_player_research_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_research(struct connection *pc, const struct packet_player_research *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_research from the server.");
  }
  ensure_valid_variant_packet_player_research(pc);

  switch(pc->phs.variant[PACKET_PLAYER_RESEARCH]) {
    case 100: return send_packet_player_research_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_player_research(struct connection *pc, int tech)
{
  struct packet_player_research packet, *real_packet = &packet;

  real_packet->tech = tech;
  
  return send_packet_player_research(pc, real_packet);
}

static struct packet_player_tech_goal *receive_packet_player_tech_goal_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_tech_goal, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->tech = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_tech_goal_100(struct connection *pc, const struct packet_player_tech_goal *packet)
{
  const struct packet_player_tech_goal *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_TECH_GOAL);

  dio_put_uint8(&dout, real_packet->tech);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_tech_goal(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_TECH_GOAL] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_TECH_GOAL] = variant;
}

struct packet_player_tech_goal *receive_packet_player_tech_goal(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_tech_goal at the client.");
  }
  ensure_valid_variant_packet_player_tech_goal(pc);

  switch(pc->phs.variant[PACKET_PLAYER_TECH_GOAL]) {
    case 100: return receive_packet_player_tech_goal_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_tech_goal(struct connection *pc, const struct packet_player_tech_goal *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_tech_goal from the server.");
  }
  ensure_valid_variant_packet_player_tech_goal(pc);

  switch(pc->phs.variant[PACKET_PLAYER_TECH_GOAL]) {
    case 100: return send_packet_player_tech_goal_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_player_tech_goal(struct connection *pc, int tech)
{
  struct packet_player_tech_goal packet, *real_packet = &packet;

  real_packet->tech = tech;
  
  return send_packet_player_tech_goal(pc, real_packet);
}

static struct packet_player_attribute_block *receive_packet_player_attribute_block_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_attribute_block, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_attribute_block_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_PLAYER_ATTRIBUTE_BLOCK);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_attribute_block(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_BLOCK] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_BLOCK] = variant;
}

struct packet_player_attribute_block *receive_packet_player_attribute_block(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_attribute_block at the client.");
  }
  ensure_valid_variant_packet_player_attribute_block(pc);

  switch(pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_BLOCK]) {
    case 100: return receive_packet_player_attribute_block_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_attribute_block(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_player_attribute_block from the server.");
  }
  ensure_valid_variant_packet_player_attribute_block(pc);

  switch(pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_BLOCK]) {
    case 100: return send_packet_player_attribute_block_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_player_attribute_chunk *receive_packet_player_attribute_chunk_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_player_attribute_chunk, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->offset = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->total_length = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->chunk_length = readin;
  }
  
    if(real_packet->chunk_length > ATTRIBUTE_CHUNK_SIZE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->chunk_length = ATTRIBUTE_CHUNK_SIZE;
    }
    dio_get_memory(&din, real_packet->data, real_packet->chunk_length);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_player_attribute_chunk_100(struct connection *pc, const struct packet_player_attribute_chunk *packet)
{
  const struct packet_player_attribute_chunk *real_packet = packet;
  SEND_PACKET_START(PACKET_PLAYER_ATTRIBUTE_CHUNK);

  {
    struct packet_player_attribute_chunk *tmp = fc_malloc(sizeof(*tmp));

    *tmp = *packet;
    pre_send_packet_player_attribute_chunk(pc, tmp);
    real_packet = tmp;
  }

  dio_put_uint32(&dout, real_packet->offset);
  dio_put_uint32(&dout, real_packet->total_length);
  dio_put_uint32(&dout, real_packet->chunk_length);
  dio_put_memory(&dout, &real_packet->data, real_packet->chunk_length);


  if (real_packet != packet) {
    free((void *) real_packet);
  }
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_attribute_chunk(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_CHUNK] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_CHUNK] = variant;
}

struct packet_player_attribute_chunk *receive_packet_player_attribute_chunk(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  ensure_valid_variant_packet_player_attribute_chunk(pc);

  switch(pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_CHUNK]) {
    case 100: return receive_packet_player_attribute_chunk_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_player_attribute_chunk(struct connection *pc, const struct packet_player_attribute_chunk *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  ensure_valid_variant_packet_player_attribute_chunk(pc);

  switch(pc->phs.variant[PACKET_PLAYER_ATTRIBUTE_CHUNK]) {
    case 100: return send_packet_player_attribute_chunk_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_unit_remove *receive_packet_unit_remove_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_remove, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_remove_100(struct connection *pc, const struct packet_unit_remove *packet)
{
  const struct packet_unit_remove *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_REMOVE);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_remove(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_REMOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_REMOVE] = variant;
}

struct packet_unit_remove *receive_packet_unit_remove(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_remove at the server.");
  }
  ensure_valid_variant_packet_unit_remove(pc);

  switch(pc->phs.variant[PACKET_UNIT_REMOVE]) {
    case 100: return receive_packet_unit_remove_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_remove(struct connection *pc, const struct packet_unit_remove *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_remove from the client.");
  }
  ensure_valid_variant_packet_unit_remove(pc);

  switch(pc->phs.variant[PACKET_UNIT_REMOVE]) {
    case 100: return send_packet_unit_remove_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_unit_remove(struct conn_list *dest, const struct packet_unit_remove *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_unit_remove(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_unit_remove(struct connection *pc, int unit_id)
{
  struct packet_unit_remove packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_remove(pc, real_packet);
}

void dlsend_packet_unit_remove(struct conn_list *dest, int unit_id)
{
  struct packet_unit_remove packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  lsend_packet_unit_remove(dest, real_packet);
}

static struct packet_unit_info *receive_packet_unit_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->homecity = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->upkeep[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
  }
  dio_get_bool8(&din, &real_packet->ai);
  dio_get_bool8(&din, &real_packet->paradropped);
  dio_get_bool8(&din, &real_packet->transported);
  dio_get_bool8(&din, &real_packet->done_moving);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->transported_by = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->movesleft = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity_count = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->occupy = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->goto_dest_x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->goto_dest_y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->activity_target = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity_base = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->battlegroup = readin;
  }
  dio_get_bool8(&din, &real_packet->has_orders);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->orders_length = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->orders_index = readin;
  }
  dio_get_bool8(&din, &real_packet->orders_repeat);
  dio_get_bool8(&din, &real_packet->orders_vigilant);
  
  {
    int i;
  
    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->orders[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->orders_dirs[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->orders_activities[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->orders_bases[i] = readin;
  }
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_info_100(struct connection *pc, const struct packet_unit_info *packet)
{
  const struct packet_unit_info *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_INFO);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint32(&dout, real_packet->homecity);

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint8(&dout, real_packet->upkeep[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->veteran);
  dio_put_bool8(&dout, real_packet->ai);
  dio_put_bool8(&dout, real_packet->paradropped);
  dio_put_bool8(&dout, real_packet->transported);
  dio_put_bool8(&dout, real_packet->done_moving);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint32(&dout, real_packet->transported_by);
  dio_put_uint8(&dout, real_packet->movesleft);
  dio_put_uint8(&dout, real_packet->hp);
  dio_put_uint8(&dout, real_packet->fuel);
  dio_put_uint8(&dout, real_packet->activity_count);
  dio_put_uint8(&dout, real_packet->occupy);
  dio_put_uint32(&dout, real_packet->goto_dest_x);
  dio_put_uint32(&dout, real_packet->goto_dest_y);
  dio_put_uint8(&dout, real_packet->activity);
  dio_put_uint32(&dout, real_packet->activity_target);
  dio_put_uint8(&dout, real_packet->activity_base);
  dio_put_sint8(&dout, real_packet->battlegroup);
  dio_put_bool8(&dout, real_packet->has_orders);
  dio_put_uint32(&dout, real_packet->orders_length);
  dio_put_uint32(&dout, real_packet->orders_index);
  dio_put_bool8(&dout, real_packet->orders_repeat);
  dio_put_bool8(&dout, real_packet->orders_vigilant);

    {
      int i;

      for (i = 0; i < real_packet->orders_length; i++) {
        dio_put_uint8(&dout, real_packet->orders[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->orders_length; i++) {
        dio_put_uint8(&dout, real_packet->orders_dirs[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->orders_length; i++) {
        dio_put_uint8(&dout, real_packet->orders_activities[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->orders_length; i++) {
        dio_put_uint8(&dout, real_packet->orders_bases[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_INFO] = variant;
}

struct packet_unit_info *receive_packet_unit_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_info at the server.");
  }
  ensure_valid_variant_packet_unit_info(pc);

  switch(pc->phs.variant[PACKET_UNIT_INFO]) {
    case 100: return receive_packet_unit_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_info(struct connection *pc, const struct packet_unit_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_info from the client.");
  }
  ensure_valid_variant_packet_unit_info(pc);

  switch(pc->phs.variant[PACKET_UNIT_INFO]) {
    case 100: return send_packet_unit_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_unit_info(struct conn_list *dest, const struct packet_unit_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_unit_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_unit_short_info *receive_packet_unit_short_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_short_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
  }
  dio_get_bool8(&din, &real_packet->occupied);
  dio_get_bool8(&din, &real_packet->goes_out_of_sight);
  dio_get_bool8(&din, &real_packet->transported);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity_base = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->transported_by = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->packet_use = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->info_city_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->serial_num = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_short_info_100(struct connection *pc, const struct packet_unit_short_info *packet)
{
  const struct packet_unit_short_info *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_SHORT_INFO);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint8(&dout, real_packet->veteran);
  dio_put_bool8(&dout, real_packet->occupied);
  dio_put_bool8(&dout, real_packet->goes_out_of_sight);
  dio_put_bool8(&dout, real_packet->transported);
  dio_put_uint8(&dout, real_packet->hp);
  dio_put_uint8(&dout, real_packet->activity);
  dio_put_uint8(&dout, real_packet->activity_base);
  dio_put_uint32(&dout, real_packet->transported_by);
  dio_put_uint8(&dout, real_packet->packet_use);
  dio_put_uint32(&dout, real_packet->info_city_id);
  dio_put_uint32(&dout, real_packet->serial_num);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_short_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_SHORT_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_SHORT_INFO] = variant;
}

struct packet_unit_short_info *receive_packet_unit_short_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_short_info at the server.");
  }
  ensure_valid_variant_packet_unit_short_info(pc);

  switch(pc->phs.variant[PACKET_UNIT_SHORT_INFO]) {
    case 100: return receive_packet_unit_short_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_short_info(struct connection *pc, const struct packet_unit_short_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_short_info from the client.");
  }
  ensure_valid_variant_packet_unit_short_info(pc);

  switch(pc->phs.variant[PACKET_UNIT_SHORT_INFO]) {
    case 100: return send_packet_unit_short_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_unit_short_info(struct conn_list *dest, const struct packet_unit_short_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_unit_short_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_unit_combat_info *receive_packet_unit_combat_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_combat_info, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->attacker_unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->defender_unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->attacker_hp = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->defender_hp = readin;
  }
  dio_get_bool8(&din, &real_packet->make_winner_veteran);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_combat_info_100(struct connection *pc, const struct packet_unit_combat_info *packet)
{
  const struct packet_unit_combat_info *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_COMBAT_INFO);

  dio_put_uint32(&dout, real_packet->attacker_unit_id);
  dio_put_uint32(&dout, real_packet->defender_unit_id);
  dio_put_uint32(&dout, real_packet->attacker_hp);
  dio_put_uint32(&dout, real_packet->defender_hp);
  dio_put_bool8(&dout, real_packet->make_winner_veteran);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_combat_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_COMBAT_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_COMBAT_INFO] = variant;
}

struct packet_unit_combat_info *receive_packet_unit_combat_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_combat_info at the server.");
  }
  ensure_valid_variant_packet_unit_combat_info(pc);

  switch(pc->phs.variant[PACKET_UNIT_COMBAT_INFO]) {
    case 100: return receive_packet_unit_combat_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_combat_info(struct connection *pc, const struct packet_unit_combat_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_combat_info from the client.");
  }
  ensure_valid_variant_packet_unit_combat_info(pc);

  switch(pc->phs.variant[PACKET_UNIT_COMBAT_INFO]) {
    case 100: return send_packet_unit_combat_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_unit_combat_info(struct conn_list *dest, const struct packet_unit_combat_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_unit_combat_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_unit_move *receive_packet_unit_move_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_move, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_move_100(struct connection *pc, const struct packet_unit_move *packet)
{
  const struct packet_unit_move *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_MOVE);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_move(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_MOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_MOVE] = variant;
}

struct packet_unit_move *receive_packet_unit_move(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_move at the client.");
  }
  ensure_valid_variant_packet_unit_move(pc);

  switch(pc->phs.variant[PACKET_UNIT_MOVE]) {
    case 100: return receive_packet_unit_move_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_move(struct connection *pc, const struct packet_unit_move *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_move from the server.");
  }
  ensure_valid_variant_packet_unit_move(pc);

  switch(pc->phs.variant[PACKET_UNIT_MOVE]) {
    case 100: return send_packet_unit_move_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_move(struct connection *pc, int unit_id, int x, int y)
{
  struct packet_unit_move packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->x = x;
  real_packet->y = y;
  
  return send_packet_unit_move(pc, real_packet);
}

static struct packet_goto_path_req *receive_packet_goto_path_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_goto_path_req, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_goto_path_req_100(struct connection *pc, const struct packet_goto_path_req *packet)
{
  const struct packet_goto_path_req *real_packet = packet;
  SEND_PACKET_START(PACKET_GOTO_PATH_REQ);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_goto_path_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_GOTO_PATH_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_GOTO_PATH_REQ] = variant;
}

struct packet_goto_path_req *receive_packet_goto_path_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_goto_path_req at the client.");
  }
  ensure_valid_variant_packet_goto_path_req(pc);

  switch(pc->phs.variant[PACKET_GOTO_PATH_REQ]) {
    case 100: return receive_packet_goto_path_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_goto_path_req(struct connection *pc, const struct packet_goto_path_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_goto_path_req from the server.");
  }
  ensure_valid_variant_packet_goto_path_req(pc);

  switch(pc->phs.variant[PACKET_GOTO_PATH_REQ]) {
    case 100: return send_packet_goto_path_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_goto_path_req(struct connection *pc, int unit_id, int x, int y)
{
  struct packet_goto_path_req packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->x = x;
  real_packet->y = y;
  
  return send_packet_goto_path_req(pc, real_packet);
}

static struct packet_goto_path *receive_packet_goto_path_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_goto_path, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->length = readin;
  }
  
  {
    int i;
  
    if(real_packet->length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->dir[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->dest_x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->dest_y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_goto_path_100(struct connection *pc, const struct packet_goto_path *packet)
{
  const struct packet_goto_path *real_packet = packet;
  SEND_PACKET_START(PACKET_GOTO_PATH);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint32(&dout, real_packet->length);

    {
      int i;

      for (i = 0; i < real_packet->length; i++) {
        dio_put_uint8(&dout, real_packet->dir[i]);
      }
    } 
  dio_put_uint32(&dout, real_packet->dest_x);
  dio_put_uint32(&dout, real_packet->dest_y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_goto_path(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_GOTO_PATH] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_GOTO_PATH] = variant;
}

struct packet_goto_path *receive_packet_goto_path(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_goto_path at the server.");
  }
  ensure_valid_variant_packet_goto_path(pc);

  switch(pc->phs.variant[PACKET_GOTO_PATH]) {
    case 100: return receive_packet_goto_path_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_goto_path(struct connection *pc, const struct packet_goto_path *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_goto_path from the client.");
  }
  ensure_valid_variant_packet_goto_path(pc);

  switch(pc->phs.variant[PACKET_GOTO_PATH]) {
    case 100: return send_packet_goto_path_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_goto_path(struct connection *pc, int unit_id, int length, enum direction8 *dir, int dest_x, int dest_y)
{
  struct packet_goto_path packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->length = length;
  {
    int i;

    for (i = 0; i < real_packet->length; i++) {
      real_packet->dir[i] = dir[i];
    }
  }
  real_packet->dest_x = dest_x;
  real_packet->dest_y = dest_y;
  
  return send_packet_goto_path(pc, real_packet);
}

static struct packet_unit_build_city *receive_packet_unit_build_city_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_build_city, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_build_city_100(struct connection *pc, const struct packet_unit_build_city *packet)
{
  const struct packet_unit_build_city *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_BUILD_CITY);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_string(&dout, real_packet->name);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_build_city(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_BUILD_CITY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_BUILD_CITY] = variant;
}

struct packet_unit_build_city *receive_packet_unit_build_city(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_build_city at the client.");
  }
  ensure_valid_variant_packet_unit_build_city(pc);

  switch(pc->phs.variant[PACKET_UNIT_BUILD_CITY]) {
    case 100: return receive_packet_unit_build_city_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_build_city(struct connection *pc, const struct packet_unit_build_city *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_build_city from the server.");
  }
  ensure_valid_variant_packet_unit_build_city(pc);

  switch(pc->phs.variant[PACKET_UNIT_BUILD_CITY]) {
    case 100: return send_packet_unit_build_city_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_build_city(struct connection *pc, int unit_id, const char *name)
{
  struct packet_unit_build_city packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  sz_strlcpy(real_packet->name, name);
  
  return send_packet_unit_build_city(pc, real_packet);
}

static struct packet_unit_disband *receive_packet_unit_disband_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_disband, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_disband_100(struct connection *pc, const struct packet_unit_disband *packet)
{
  const struct packet_unit_disband *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_DISBAND);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_disband(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_DISBAND] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_DISBAND] = variant;
}

struct packet_unit_disband *receive_packet_unit_disband(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_disband at the client.");
  }
  ensure_valid_variant_packet_unit_disband(pc);

  switch(pc->phs.variant[PACKET_UNIT_DISBAND]) {
    case 100: return receive_packet_unit_disband_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_disband(struct connection *pc, const struct packet_unit_disband *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_disband from the server.");
  }
  ensure_valid_variant_packet_unit_disband(pc);

  switch(pc->phs.variant[PACKET_UNIT_DISBAND]) {
    case 100: return send_packet_unit_disband_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_disband(struct connection *pc, int unit_id)
{
  struct packet_unit_disband packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_disband(pc, real_packet);
}

static struct packet_unit_change_homecity *receive_packet_unit_change_homecity_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_change_homecity, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_change_homecity_100(struct connection *pc, const struct packet_unit_change_homecity *packet)
{
  const struct packet_unit_change_homecity *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_CHANGE_HOMECITY);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint32(&dout, real_packet->city_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_change_homecity(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_CHANGE_HOMECITY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_CHANGE_HOMECITY] = variant;
}

struct packet_unit_change_homecity *receive_packet_unit_change_homecity(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_change_homecity at the client.");
  }
  ensure_valid_variant_packet_unit_change_homecity(pc);

  switch(pc->phs.variant[PACKET_UNIT_CHANGE_HOMECITY]) {
    case 100: return receive_packet_unit_change_homecity_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_change_homecity(struct connection *pc, const struct packet_unit_change_homecity *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_change_homecity from the server.");
  }
  ensure_valid_variant_packet_unit_change_homecity(pc);

  switch(pc->phs.variant[PACKET_UNIT_CHANGE_HOMECITY]) {
    case 100: return send_packet_unit_change_homecity_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_change_homecity(struct connection *pc, int unit_id, int city_id)
{
  struct packet_unit_change_homecity packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->city_id = city_id;
  
  return send_packet_unit_change_homecity(pc, real_packet);
}

static struct packet_unit_establish_trade *receive_packet_unit_establish_trade_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_establish_trade, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_establish_trade_100(struct connection *pc, const struct packet_unit_establish_trade *packet)
{
  const struct packet_unit_establish_trade *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_ESTABLISH_TRADE);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_establish_trade(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_ESTABLISH_TRADE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_ESTABLISH_TRADE] = variant;
}

struct packet_unit_establish_trade *receive_packet_unit_establish_trade(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_establish_trade at the client.");
  }
  ensure_valid_variant_packet_unit_establish_trade(pc);

  switch(pc->phs.variant[PACKET_UNIT_ESTABLISH_TRADE]) {
    case 100: return receive_packet_unit_establish_trade_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_establish_trade(struct connection *pc, const struct packet_unit_establish_trade *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_establish_trade from the server.");
  }
  ensure_valid_variant_packet_unit_establish_trade(pc);

  switch(pc->phs.variant[PACKET_UNIT_ESTABLISH_TRADE]) {
    case 100: return send_packet_unit_establish_trade_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_establish_trade(struct connection *pc, int unit_id)
{
  struct packet_unit_establish_trade packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_establish_trade(pc, real_packet);
}

static struct packet_unit_battlegroup *receive_packet_unit_battlegroup_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_battlegroup, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->battlegroup = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_battlegroup_100(struct connection *pc, const struct packet_unit_battlegroup *packet)
{
  const struct packet_unit_battlegroup *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_BATTLEGROUP);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_sint8(&dout, real_packet->battlegroup);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_battlegroup(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_BATTLEGROUP] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_BATTLEGROUP] = variant;
}

struct packet_unit_battlegroup *receive_packet_unit_battlegroup(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_battlegroup at the client.");
  }
  ensure_valid_variant_packet_unit_battlegroup(pc);

  switch(pc->phs.variant[PACKET_UNIT_BATTLEGROUP]) {
    case 100: return receive_packet_unit_battlegroup_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_battlegroup(struct connection *pc, const struct packet_unit_battlegroup *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_battlegroup from the server.");
  }
  ensure_valid_variant_packet_unit_battlegroup(pc);

  switch(pc->phs.variant[PACKET_UNIT_BATTLEGROUP]) {
    case 100: return send_packet_unit_battlegroup_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_battlegroup(struct connection *pc, int unit_id, int battlegroup)
{
  struct packet_unit_battlegroup packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->battlegroup = battlegroup;
  
  return send_packet_unit_battlegroup(pc, real_packet);
}

static struct packet_unit_help_build_wonder *receive_packet_unit_help_build_wonder_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_help_build_wonder, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_help_build_wonder_100(struct connection *pc, const struct packet_unit_help_build_wonder *packet)
{
  const struct packet_unit_help_build_wonder *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_HELP_BUILD_WONDER);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_help_build_wonder(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_HELP_BUILD_WONDER] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_HELP_BUILD_WONDER] = variant;
}

struct packet_unit_help_build_wonder *receive_packet_unit_help_build_wonder(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_help_build_wonder at the client.");
  }
  ensure_valid_variant_packet_unit_help_build_wonder(pc);

  switch(pc->phs.variant[PACKET_UNIT_HELP_BUILD_WONDER]) {
    case 100: return receive_packet_unit_help_build_wonder_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_help_build_wonder(struct connection *pc, const struct packet_unit_help_build_wonder *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_help_build_wonder from the server.");
  }
  ensure_valid_variant_packet_unit_help_build_wonder(pc);

  switch(pc->phs.variant[PACKET_UNIT_HELP_BUILD_WONDER]) {
    case 100: return send_packet_unit_help_build_wonder_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_help_build_wonder(struct connection *pc, int unit_id)
{
  struct packet_unit_help_build_wonder packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_help_build_wonder(pc, real_packet);
}

static struct packet_unit_orders *receive_packet_unit_orders_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_orders, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->src_x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->src_y = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->length = readin;
  }
  dio_get_bool8(&din, &real_packet->repeat);
  dio_get_bool8(&din, &real_packet->vigilant);
  
  {
    int i;
  
    if(real_packet->length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->orders[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->dir[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->length; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->base[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->dest_x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->dest_y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_orders_100(struct connection *pc, const struct packet_unit_orders *packet)
{
  const struct packet_unit_orders *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_ORDERS);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint32(&dout, real_packet->src_x);
  dio_put_uint32(&dout, real_packet->src_y);
  dio_put_uint32(&dout, real_packet->length);
  dio_put_bool8(&dout, real_packet->repeat);
  dio_put_bool8(&dout, real_packet->vigilant);

    {
      int i;

      for (i = 0; i < real_packet->length; i++) {
        dio_put_uint8(&dout, real_packet->orders[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->length; i++) {
        dio_put_uint8(&dout, real_packet->dir[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->length; i++) {
        dio_put_uint8(&dout, real_packet->activity[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->length; i++) {
        dio_put_uint8(&dout, real_packet->base[i]);
      }
    } 
  dio_put_uint32(&dout, real_packet->dest_x);
  dio_put_uint32(&dout, real_packet->dest_y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_orders(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_ORDERS] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_ORDERS] = variant;
}

struct packet_unit_orders *receive_packet_unit_orders(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_orders at the client.");
  }
  ensure_valid_variant_packet_unit_orders(pc);

  switch(pc->phs.variant[PACKET_UNIT_ORDERS]) {
    case 100: return receive_packet_unit_orders_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_orders(struct connection *pc, const struct packet_unit_orders *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_orders from the server.");
  }
  ensure_valid_variant_packet_unit_orders(pc);

  switch(pc->phs.variant[PACKET_UNIT_ORDERS]) {
    case 100: return send_packet_unit_orders_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_unit_autosettlers *receive_packet_unit_autosettlers_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_autosettlers, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_autosettlers_100(struct connection *pc, const struct packet_unit_autosettlers *packet)
{
  const struct packet_unit_autosettlers *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_AUTOSETTLERS);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_autosettlers(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_AUTOSETTLERS] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_AUTOSETTLERS] = variant;
}

struct packet_unit_autosettlers *receive_packet_unit_autosettlers(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_autosettlers at the client.");
  }
  ensure_valid_variant_packet_unit_autosettlers(pc);

  switch(pc->phs.variant[PACKET_UNIT_AUTOSETTLERS]) {
    case 100: return receive_packet_unit_autosettlers_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_autosettlers(struct connection *pc, const struct packet_unit_autosettlers *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_autosettlers from the server.");
  }
  ensure_valid_variant_packet_unit_autosettlers(pc);

  switch(pc->phs.variant[PACKET_UNIT_AUTOSETTLERS]) {
    case 100: return send_packet_unit_autosettlers_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_autosettlers(struct connection *pc, int unit_id)
{
  struct packet_unit_autosettlers packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_autosettlers(pc, real_packet);
}

static struct packet_unit_load *receive_packet_unit_load_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_load, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->cargo_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->transporter_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_load_100(struct connection *pc, const struct packet_unit_load *packet)
{
  const struct packet_unit_load *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_LOAD);

  dio_put_uint32(&dout, real_packet->cargo_id);
  dio_put_uint32(&dout, real_packet->transporter_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_load(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_LOAD] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_LOAD] = variant;
}

struct packet_unit_load *receive_packet_unit_load(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_load at the client.");
  }
  ensure_valid_variant_packet_unit_load(pc);

  switch(pc->phs.variant[PACKET_UNIT_LOAD]) {
    case 100: return receive_packet_unit_load_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_load(struct connection *pc, const struct packet_unit_load *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_load from the server.");
  }
  ensure_valid_variant_packet_unit_load(pc);

  switch(pc->phs.variant[PACKET_UNIT_LOAD]) {
    case 100: return send_packet_unit_load_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_load(struct connection *pc, int cargo_id, int transporter_id)
{
  struct packet_unit_load packet, *real_packet = &packet;

  real_packet->cargo_id = cargo_id;
  real_packet->transporter_id = transporter_id;
  
  return send_packet_unit_load(pc, real_packet);
}

static struct packet_unit_unload *receive_packet_unit_unload_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_unload, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->cargo_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->transporter_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_unload_100(struct connection *pc, const struct packet_unit_unload *packet)
{
  const struct packet_unit_unload *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_UNLOAD);

  dio_put_uint32(&dout, real_packet->cargo_id);
  dio_put_uint32(&dout, real_packet->transporter_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_unload(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_UNLOAD] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_UNLOAD] = variant;
}

struct packet_unit_unload *receive_packet_unit_unload(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_unload at the client.");
  }
  ensure_valid_variant_packet_unit_unload(pc);

  switch(pc->phs.variant[PACKET_UNIT_UNLOAD]) {
    case 100: return receive_packet_unit_unload_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_unload(struct connection *pc, const struct packet_unit_unload *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_unload from the server.");
  }
  ensure_valid_variant_packet_unit_unload(pc);

  switch(pc->phs.variant[PACKET_UNIT_UNLOAD]) {
    case 100: return send_packet_unit_unload_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_unload(struct connection *pc, int cargo_id, int transporter_id)
{
  struct packet_unit_unload packet, *real_packet = &packet;

  real_packet->cargo_id = cargo_id;
  real_packet->transporter_id = transporter_id;
  
  return send_packet_unit_unload(pc, real_packet);
}

static struct packet_unit_upgrade *receive_packet_unit_upgrade_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_upgrade, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_upgrade_100(struct connection *pc, const struct packet_unit_upgrade *packet)
{
  const struct packet_unit_upgrade *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_UPGRADE);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_upgrade(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_UPGRADE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_UPGRADE] = variant;
}

struct packet_unit_upgrade *receive_packet_unit_upgrade(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_upgrade at the client.");
  }
  ensure_valid_variant_packet_unit_upgrade(pc);

  switch(pc->phs.variant[PACKET_UNIT_UPGRADE]) {
    case 100: return receive_packet_unit_upgrade_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_upgrade(struct connection *pc, const struct packet_unit_upgrade *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_upgrade from the server.");
  }
  ensure_valid_variant_packet_unit_upgrade(pc);

  switch(pc->phs.variant[PACKET_UNIT_UPGRADE]) {
    case 100: return send_packet_unit_upgrade_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_upgrade(struct connection *pc, int unit_id)
{
  struct packet_unit_upgrade packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_upgrade(pc, real_packet);
}

static struct packet_unit_transform *receive_packet_unit_transform_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_transform, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_transform_100(struct connection *pc, const struct packet_unit_transform *packet)
{
  const struct packet_unit_transform *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_TRANSFORM);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_transform(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_TRANSFORM] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_TRANSFORM] = variant;
}

struct packet_unit_transform *receive_packet_unit_transform(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_transform at the client.");
  }
  ensure_valid_variant_packet_unit_transform(pc);

  switch(pc->phs.variant[PACKET_UNIT_TRANSFORM]) {
    case 100: return receive_packet_unit_transform_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_transform(struct connection *pc, const struct packet_unit_transform *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_transform from the server.");
  }
  ensure_valid_variant_packet_unit_transform(pc);

  switch(pc->phs.variant[PACKET_UNIT_TRANSFORM]) {
    case 100: return send_packet_unit_transform_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_transform(struct connection *pc, int unit_id)
{
  struct packet_unit_transform packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_transform(pc, real_packet);
}

static struct packet_unit_nuke *receive_packet_unit_nuke_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_nuke, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_nuke_100(struct connection *pc, const struct packet_unit_nuke *packet)
{
  const struct packet_unit_nuke *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_NUKE);

  dio_put_uint32(&dout, real_packet->unit_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_nuke(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_NUKE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_NUKE] = variant;
}

struct packet_unit_nuke *receive_packet_unit_nuke(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_nuke at the client.");
  }
  ensure_valid_variant_packet_unit_nuke(pc);

  switch(pc->phs.variant[PACKET_UNIT_NUKE]) {
    case 100: return receive_packet_unit_nuke_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_nuke(struct connection *pc, const struct packet_unit_nuke *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_nuke from the server.");
  }
  ensure_valid_variant_packet_unit_nuke(pc);

  switch(pc->phs.variant[PACKET_UNIT_NUKE]) {
    case 100: return send_packet_unit_nuke_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_nuke(struct connection *pc, int unit_id)
{
  struct packet_unit_nuke packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  
  return send_packet_unit_nuke(pc, real_packet);
}

static struct packet_unit_paradrop_to *receive_packet_unit_paradrop_to_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_paradrop_to, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_paradrop_to_100(struct connection *pc, const struct packet_unit_paradrop_to *packet)
{
  const struct packet_unit_paradrop_to *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_PARADROP_TO);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_paradrop_to(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_PARADROP_TO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_PARADROP_TO] = variant;
}

struct packet_unit_paradrop_to *receive_packet_unit_paradrop_to(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_paradrop_to at the client.");
  }
  ensure_valid_variant_packet_unit_paradrop_to(pc);

  switch(pc->phs.variant[PACKET_UNIT_PARADROP_TO]) {
    case 100: return receive_packet_unit_paradrop_to_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_paradrop_to(struct connection *pc, const struct packet_unit_paradrop_to *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_paradrop_to from the server.");
  }
  ensure_valid_variant_packet_unit_paradrop_to(pc);

  switch(pc->phs.variant[PACKET_UNIT_PARADROP_TO]) {
    case 100: return send_packet_unit_paradrop_to_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_paradrop_to(struct connection *pc, int unit_id, int x, int y)
{
  struct packet_unit_paradrop_to packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->x = x;
  real_packet->y = y;
  
  return send_packet_unit_paradrop_to(pc, real_packet);
}

static struct packet_unit_airlift *receive_packet_unit_airlift_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_airlift, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->city_id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_airlift_100(struct connection *pc, const struct packet_unit_airlift *packet)
{
  const struct packet_unit_airlift *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_AIRLIFT);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint32(&dout, real_packet->city_id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_airlift(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_AIRLIFT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_AIRLIFT] = variant;
}

struct packet_unit_airlift *receive_packet_unit_airlift(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_airlift at the client.");
  }
  ensure_valid_variant_packet_unit_airlift(pc);

  switch(pc->phs.variant[PACKET_UNIT_AIRLIFT]) {
    case 100: return receive_packet_unit_airlift_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_airlift(struct connection *pc, const struct packet_unit_airlift *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_airlift from the server.");
  }
  ensure_valid_variant_packet_unit_airlift(pc);

  switch(pc->phs.variant[PACKET_UNIT_AIRLIFT]) {
    case 100: return send_packet_unit_airlift_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_airlift(struct connection *pc, int unit_id, int city_id)
{
  struct packet_unit_airlift packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->city_id = city_id;
  
  return send_packet_unit_airlift(pc, real_packet);
}

static struct packet_unit_diplomat_query *receive_packet_unit_diplomat_query_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_diplomat_query, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->diplomat_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->target_id = readin;
  }
  {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->value = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->action_type = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_diplomat_query_100(struct connection *pc, const struct packet_unit_diplomat_query *packet)
{
  const struct packet_unit_diplomat_query *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_DIPLOMAT_QUERY);

  dio_put_uint32(&dout, real_packet->diplomat_id);
  dio_put_uint32(&dout, real_packet->target_id);
  dio_put_sint16(&dout, real_packet->value);
  dio_put_uint8(&dout, real_packet->action_type);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_diplomat_query(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_DIPLOMAT_QUERY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_DIPLOMAT_QUERY] = variant;
}

struct packet_unit_diplomat_query *receive_packet_unit_diplomat_query(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_diplomat_query at the client.");
  }
  ensure_valid_variant_packet_unit_diplomat_query(pc);

  switch(pc->phs.variant[PACKET_UNIT_DIPLOMAT_QUERY]) {
    case 100: return receive_packet_unit_diplomat_query_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_diplomat_query(struct connection *pc, const struct packet_unit_diplomat_query *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_diplomat_query from the server.");
  }
  ensure_valid_variant_packet_unit_diplomat_query(pc);

  switch(pc->phs.variant[PACKET_UNIT_DIPLOMAT_QUERY]) {
    case 100: return send_packet_unit_diplomat_query_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_diplomat_query(struct connection *pc, int diplomat_id, int target_id, int value, enum diplomat_actions action_type)
{
  struct packet_unit_diplomat_query packet, *real_packet = &packet;

  real_packet->diplomat_id = diplomat_id;
  real_packet->target_id = target_id;
  real_packet->value = value;
  real_packet->action_type = action_type;
  
  return send_packet_unit_diplomat_query(pc, real_packet);
}

static struct packet_unit_type_upgrade *receive_packet_unit_type_upgrade_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_type_upgrade, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_type_upgrade_100(struct connection *pc, const struct packet_unit_type_upgrade *packet)
{
  const struct packet_unit_type_upgrade *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_TYPE_UPGRADE);

  dio_put_uint8(&dout, real_packet->type);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_type_upgrade(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_TYPE_UPGRADE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_TYPE_UPGRADE] = variant;
}

struct packet_unit_type_upgrade *receive_packet_unit_type_upgrade(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_type_upgrade at the client.");
  }
  ensure_valid_variant_packet_unit_type_upgrade(pc);

  switch(pc->phs.variant[PACKET_UNIT_TYPE_UPGRADE]) {
    case 100: return receive_packet_unit_type_upgrade_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_type_upgrade(struct connection *pc, const struct packet_unit_type_upgrade *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_type_upgrade from the server.");
  }
  ensure_valid_variant_packet_unit_type_upgrade(pc);

  switch(pc->phs.variant[PACKET_UNIT_TYPE_UPGRADE]) {
    case 100: return send_packet_unit_type_upgrade_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_type_upgrade(struct connection *pc, Unit_type_id type)
{
  struct packet_unit_type_upgrade packet, *real_packet = &packet;

  real_packet->type = type;
  
  return send_packet_unit_type_upgrade(pc, real_packet);
}

static struct packet_unit_diplomat_action *receive_packet_unit_diplomat_action_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_diplomat_action, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->diplomat_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->target_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->action_type = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_diplomat_action_100(struct connection *pc, const struct packet_unit_diplomat_action *packet)
{
  const struct packet_unit_diplomat_action *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_DIPLOMAT_ACTION);

  dio_put_uint32(&dout, real_packet->diplomat_id);
  dio_put_uint32(&dout, real_packet->target_id);
  dio_put_uint32(&dout, real_packet->value);
  dio_put_uint8(&dout, real_packet->action_type);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_diplomat_action(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_DIPLOMAT_ACTION] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_DIPLOMAT_ACTION] = variant;
}

struct packet_unit_diplomat_action *receive_packet_unit_diplomat_action(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_diplomat_action at the client.");
  }
  ensure_valid_variant_packet_unit_diplomat_action(pc);

  switch(pc->phs.variant[PACKET_UNIT_DIPLOMAT_ACTION]) {
    case 100: return receive_packet_unit_diplomat_action_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_diplomat_action(struct connection *pc, const struct packet_unit_diplomat_action *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_diplomat_action from the server.");
  }
  ensure_valid_variant_packet_unit_diplomat_action(pc);

  switch(pc->phs.variant[PACKET_UNIT_DIPLOMAT_ACTION]) {
    case 100: return send_packet_unit_diplomat_action_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_diplomat_action(struct connection *pc, int diplomat_id, int target_id, int value, enum diplomat_actions action_type)
{
  struct packet_unit_diplomat_action packet, *real_packet = &packet;

  real_packet->diplomat_id = diplomat_id;
  real_packet->target_id = target_id;
  real_packet->value = value;
  real_packet->action_type = action_type;
  
  return send_packet_unit_diplomat_action(pc, real_packet);
}

static struct packet_unit_diplomat_answer *receive_packet_unit_diplomat_answer_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_diplomat_answer, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->diplomat_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->target_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->cost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->action_type = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_diplomat_answer_100(struct connection *pc, const struct packet_unit_diplomat_answer *packet)
{
  const struct packet_unit_diplomat_answer *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_DIPLOMAT_ANSWER);

  dio_put_uint32(&dout, real_packet->diplomat_id);
  dio_put_uint32(&dout, real_packet->target_id);
  dio_put_uint32(&dout, real_packet->cost);
  dio_put_uint8(&dout, real_packet->action_type);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_diplomat_answer(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_DIPLOMAT_ANSWER] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_DIPLOMAT_ANSWER] = variant;
}

struct packet_unit_diplomat_answer *receive_packet_unit_diplomat_answer(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_diplomat_answer at the server.");
  }
  ensure_valid_variant_packet_unit_diplomat_answer(pc);

  switch(pc->phs.variant[PACKET_UNIT_DIPLOMAT_ANSWER]) {
    case 100: return receive_packet_unit_diplomat_answer_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_diplomat_answer(struct connection *pc, const struct packet_unit_diplomat_answer *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_diplomat_answer from the client.");
  }
  ensure_valid_variant_packet_unit_diplomat_answer(pc);

  switch(pc->phs.variant[PACKET_UNIT_DIPLOMAT_ANSWER]) {
    case 100: return send_packet_unit_diplomat_answer_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_unit_diplomat_answer(struct conn_list *dest, const struct packet_unit_diplomat_answer *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_unit_diplomat_answer(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_unit_diplomat_answer(struct connection *pc, int diplomat_id, int target_id, int cost, enum diplomat_actions action_type)
{
  struct packet_unit_diplomat_answer packet, *real_packet = &packet;

  real_packet->diplomat_id = diplomat_id;
  real_packet->target_id = target_id;
  real_packet->cost = cost;
  real_packet->action_type = action_type;
  
  return send_packet_unit_diplomat_answer(pc, real_packet);
}

void dlsend_packet_unit_diplomat_answer(struct conn_list *dest, int diplomat_id, int target_id, int cost, enum diplomat_actions action_type)
{
  struct packet_unit_diplomat_answer packet, *real_packet = &packet;

  real_packet->diplomat_id = diplomat_id;
  real_packet->target_id = target_id;
  real_packet->cost = cost;
  real_packet->action_type = action_type;
  
  lsend_packet_unit_diplomat_answer(dest, real_packet);
}

static struct packet_unit_change_activity *receive_packet_unit_change_activity_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_unit_change_activity, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->unit_id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->activity_target = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity_base = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_unit_change_activity_100(struct connection *pc, const struct packet_unit_change_activity *packet)
{
  const struct packet_unit_change_activity *real_packet = packet;
  SEND_PACKET_START(PACKET_UNIT_CHANGE_ACTIVITY);

  dio_put_uint32(&dout, real_packet->unit_id);
  dio_put_uint8(&dout, real_packet->activity);
  dio_put_uint32(&dout, real_packet->activity_target);
  dio_put_uint8(&dout, real_packet->activity_base);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_unit_change_activity(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_UNIT_CHANGE_ACTIVITY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_UNIT_CHANGE_ACTIVITY] = variant;
}

struct packet_unit_change_activity *receive_packet_unit_change_activity(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_change_activity at the client.");
  }
  ensure_valid_variant_packet_unit_change_activity(pc);

  switch(pc->phs.variant[PACKET_UNIT_CHANGE_ACTIVITY]) {
    case 100: return receive_packet_unit_change_activity_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_unit_change_activity(struct connection *pc, const struct packet_unit_change_activity *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_change_activity from the server.");
  }
  ensure_valid_variant_packet_unit_change_activity(pc);

  switch(pc->phs.variant[PACKET_UNIT_CHANGE_ACTIVITY]) {
    case 100: return send_packet_unit_change_activity_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_unit_change_activity(struct connection *pc, int unit_id, enum unit_activity activity, enum tile_special_type activity_target, Base_type_id activity_base)
{
  struct packet_unit_change_activity packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->activity = activity;
  real_packet->activity_target = activity_target;
  real_packet->activity_base = activity_base;
  
  return send_packet_unit_change_activity(pc, real_packet);
}

static struct packet_diplomacy_init_meeting_req *receive_packet_diplomacy_init_meeting_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_init_meeting_req, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_init_meeting_req_100(struct connection *pc, const struct packet_diplomacy_init_meeting_req *packet)
{
  const struct packet_diplomacy_init_meeting_req *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_INIT_MEETING_REQ);

  dio_put_sint8(&dout, real_packet->counterpart);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_init_meeting_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING_REQ] = variant;
}

struct packet_diplomacy_init_meeting_req *receive_packet_diplomacy_init_meeting_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_init_meeting_req at the client.");
  }
  ensure_valid_variant_packet_diplomacy_init_meeting_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING_REQ]) {
    case 100: return receive_packet_diplomacy_init_meeting_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_init_meeting_req(struct connection *pc, const struct packet_diplomacy_init_meeting_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_init_meeting_req from the server.");
  }
  ensure_valid_variant_packet_diplomacy_init_meeting_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING_REQ]) {
    case 100: return send_packet_diplomacy_init_meeting_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_diplomacy_init_meeting_req(struct connection *pc, int counterpart)
{
  struct packet_diplomacy_init_meeting_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  
  return send_packet_diplomacy_init_meeting_req(pc, real_packet);
}

static struct packet_diplomacy_init_meeting *receive_packet_diplomacy_init_meeting_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_init_meeting, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->initiated_from = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_init_meeting_100(struct connection *pc, const struct packet_diplomacy_init_meeting *packet)
{
  const struct packet_diplomacy_init_meeting *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_INIT_MEETING);

  dio_put_sint8(&dout, real_packet->counterpart);
  dio_put_sint8(&dout, real_packet->initiated_from);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_init_meeting(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING] = variant;
}

struct packet_diplomacy_init_meeting *receive_packet_diplomacy_init_meeting(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_init_meeting at the server.");
  }
  ensure_valid_variant_packet_diplomacy_init_meeting(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING]) {
    case 100: return receive_packet_diplomacy_init_meeting_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_init_meeting(struct connection *pc, const struct packet_diplomacy_init_meeting *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_init_meeting from the client.");
  }
  ensure_valid_variant_packet_diplomacy_init_meeting(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_INIT_MEETING]) {
    case 100: return send_packet_diplomacy_init_meeting_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_diplomacy_init_meeting(struct conn_list *dest, const struct packet_diplomacy_init_meeting *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_diplomacy_init_meeting(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_diplomacy_init_meeting(struct connection *pc, int counterpart, int initiated_from)
{
  struct packet_diplomacy_init_meeting packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->initiated_from = initiated_from;
  
  return send_packet_diplomacy_init_meeting(pc, real_packet);
}

void dlsend_packet_diplomacy_init_meeting(struct conn_list *dest, int counterpart, int initiated_from)
{
  struct packet_diplomacy_init_meeting packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->initiated_from = initiated_from;
  
  lsend_packet_diplomacy_init_meeting(dest, real_packet);
}

static struct packet_diplomacy_cancel_meeting_req *receive_packet_diplomacy_cancel_meeting_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_cancel_meeting_req, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_cancel_meeting_req_100(struct connection *pc, const struct packet_diplomacy_cancel_meeting_req *packet)
{
  const struct packet_diplomacy_cancel_meeting_req *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_CANCEL_MEETING_REQ);

  dio_put_sint8(&dout, real_packet->counterpart);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_cancel_meeting_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING_REQ] = variant;
}

struct packet_diplomacy_cancel_meeting_req *receive_packet_diplomacy_cancel_meeting_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_cancel_meeting_req at the client.");
  }
  ensure_valid_variant_packet_diplomacy_cancel_meeting_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING_REQ]) {
    case 100: return receive_packet_diplomacy_cancel_meeting_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_cancel_meeting_req(struct connection *pc, const struct packet_diplomacy_cancel_meeting_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_cancel_meeting_req from the server.");
  }
  ensure_valid_variant_packet_diplomacy_cancel_meeting_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING_REQ]) {
    case 100: return send_packet_diplomacy_cancel_meeting_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_diplomacy_cancel_meeting_req(struct connection *pc, int counterpart)
{
  struct packet_diplomacy_cancel_meeting_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  
  return send_packet_diplomacy_cancel_meeting_req(pc, real_packet);
}

static struct packet_diplomacy_cancel_meeting *receive_packet_diplomacy_cancel_meeting_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_cancel_meeting, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->initiated_from = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_cancel_meeting_100(struct connection *pc, const struct packet_diplomacy_cancel_meeting *packet)
{
  const struct packet_diplomacy_cancel_meeting *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_CANCEL_MEETING);

  dio_put_sint8(&dout, real_packet->counterpart);
  dio_put_sint8(&dout, real_packet->initiated_from);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_cancel_meeting(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING] = variant;
}

struct packet_diplomacy_cancel_meeting *receive_packet_diplomacy_cancel_meeting(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_cancel_meeting at the server.");
  }
  ensure_valid_variant_packet_diplomacy_cancel_meeting(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING]) {
    case 100: return receive_packet_diplomacy_cancel_meeting_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_cancel_meeting(struct connection *pc, const struct packet_diplomacy_cancel_meeting *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_cancel_meeting from the client.");
  }
  ensure_valid_variant_packet_diplomacy_cancel_meeting(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING]) {
    case 100: return send_packet_diplomacy_cancel_meeting_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_diplomacy_cancel_meeting(struct conn_list *dest, const struct packet_diplomacy_cancel_meeting *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_diplomacy_cancel_meeting(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_diplomacy_cancel_meeting(struct connection *pc, int counterpart, int initiated_from)
{
  struct packet_diplomacy_cancel_meeting packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->initiated_from = initiated_from;
  
  return send_packet_diplomacy_cancel_meeting(pc, real_packet);
}

void dlsend_packet_diplomacy_cancel_meeting(struct conn_list *dest, int counterpart, int initiated_from)
{
  struct packet_diplomacy_cancel_meeting packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->initiated_from = initiated_from;
  
  lsend_packet_diplomacy_cancel_meeting(dest, real_packet);
}

static struct packet_diplomacy_create_clause_req *receive_packet_diplomacy_create_clause_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_create_clause_req, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->giver = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_create_clause_req_100(struct connection *pc, const struct packet_diplomacy_create_clause_req *packet)
{
  const struct packet_diplomacy_create_clause_req *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_CREATE_CLAUSE_REQ);

  dio_put_sint8(&dout, real_packet->counterpart);
  dio_put_sint8(&dout, real_packet->giver);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint32(&dout, real_packet->value);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_create_clause_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ] = variant;
}

struct packet_diplomacy_create_clause_req *receive_packet_diplomacy_create_clause_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_create_clause_req at the client.");
  }
  ensure_valid_variant_packet_diplomacy_create_clause_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ]) {
    case 100: return receive_packet_diplomacy_create_clause_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_create_clause_req(struct connection *pc, const struct packet_diplomacy_create_clause_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_create_clause_req from the server.");
  }
  ensure_valid_variant_packet_diplomacy_create_clause_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ]) {
    case 100: return send_packet_diplomacy_create_clause_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_diplomacy_create_clause_req(struct connection *pc, int counterpart, int giver, enum clause_type type, int value)
{
  struct packet_diplomacy_create_clause_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;
  
  return send_packet_diplomacy_create_clause_req(pc, real_packet);
}

static struct packet_diplomacy_create_clause *receive_packet_diplomacy_create_clause_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_create_clause, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->giver = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_create_clause_100(struct connection *pc, const struct packet_diplomacy_create_clause *packet)
{
  const struct packet_diplomacy_create_clause *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_CREATE_CLAUSE);

  dio_put_sint8(&dout, real_packet->counterpart);
  dio_put_sint8(&dout, real_packet->giver);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint32(&dout, real_packet->value);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_create_clause(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE] = variant;
}

struct packet_diplomacy_create_clause *receive_packet_diplomacy_create_clause(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_create_clause at the server.");
  }
  ensure_valid_variant_packet_diplomacy_create_clause(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE]) {
    case 100: return receive_packet_diplomacy_create_clause_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_create_clause(struct connection *pc, const struct packet_diplomacy_create_clause *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_create_clause from the client.");
  }
  ensure_valid_variant_packet_diplomacy_create_clause(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE]) {
    case 100: return send_packet_diplomacy_create_clause_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_diplomacy_create_clause(struct conn_list *dest, const struct packet_diplomacy_create_clause *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_diplomacy_create_clause(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_diplomacy_create_clause(struct connection *pc, int counterpart, int giver, enum clause_type type, int value)
{
  struct packet_diplomacy_create_clause packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;
  
  return send_packet_diplomacy_create_clause(pc, real_packet);
}

void dlsend_packet_diplomacy_create_clause(struct conn_list *dest, int counterpart, int giver, enum clause_type type, int value)
{
  struct packet_diplomacy_create_clause packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;
  
  lsend_packet_diplomacy_create_clause(dest, real_packet);
}

static struct packet_diplomacy_remove_clause_req *receive_packet_diplomacy_remove_clause_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_remove_clause_req, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->giver = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_remove_clause_req_100(struct connection *pc, const struct packet_diplomacy_remove_clause_req *packet)
{
  const struct packet_diplomacy_remove_clause_req *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ);

  dio_put_sint8(&dout, real_packet->counterpart);
  dio_put_sint8(&dout, real_packet->giver);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint32(&dout, real_packet->value);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_remove_clause_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ] = variant;
}

struct packet_diplomacy_remove_clause_req *receive_packet_diplomacy_remove_clause_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_remove_clause_req at the client.");
  }
  ensure_valid_variant_packet_diplomacy_remove_clause_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ]) {
    case 100: return receive_packet_diplomacy_remove_clause_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_remove_clause_req(struct connection *pc, const struct packet_diplomacy_remove_clause_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_remove_clause_req from the server.");
  }
  ensure_valid_variant_packet_diplomacy_remove_clause_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ]) {
    case 100: return send_packet_diplomacy_remove_clause_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_diplomacy_remove_clause_req(struct connection *pc, int counterpart, int giver, enum clause_type type, int value)
{
  struct packet_diplomacy_remove_clause_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;
  
  return send_packet_diplomacy_remove_clause_req(pc, real_packet);
}

static struct packet_diplomacy_remove_clause *receive_packet_diplomacy_remove_clause_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_remove_clause, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->giver = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_remove_clause_100(struct connection *pc, const struct packet_diplomacy_remove_clause *packet)
{
  const struct packet_diplomacy_remove_clause *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_REMOVE_CLAUSE);

  dio_put_sint8(&dout, real_packet->counterpart);
  dio_put_sint8(&dout, real_packet->giver);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint32(&dout, real_packet->value);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_remove_clause(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE] = variant;
}

struct packet_diplomacy_remove_clause *receive_packet_diplomacy_remove_clause(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_remove_clause at the server.");
  }
  ensure_valid_variant_packet_diplomacy_remove_clause(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE]) {
    case 100: return receive_packet_diplomacy_remove_clause_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_remove_clause(struct connection *pc, const struct packet_diplomacy_remove_clause *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_remove_clause from the client.");
  }
  ensure_valid_variant_packet_diplomacy_remove_clause(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE]) {
    case 100: return send_packet_diplomacy_remove_clause_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_diplomacy_remove_clause(struct conn_list *dest, const struct packet_diplomacy_remove_clause *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_diplomacy_remove_clause(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_diplomacy_remove_clause(struct connection *pc, int counterpart, int giver, enum clause_type type, int value)
{
  struct packet_diplomacy_remove_clause packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;
  
  return send_packet_diplomacy_remove_clause(pc, real_packet);
}

void dlsend_packet_diplomacy_remove_clause(struct conn_list *dest, int counterpart, int giver, enum clause_type type, int value)
{
  struct packet_diplomacy_remove_clause packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;
  
  lsend_packet_diplomacy_remove_clause(dest, real_packet);
}

static struct packet_diplomacy_accept_treaty_req *receive_packet_diplomacy_accept_treaty_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_accept_treaty_req, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_accept_treaty_req_100(struct connection *pc, const struct packet_diplomacy_accept_treaty_req *packet)
{
  const struct packet_diplomacy_accept_treaty_req *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_ACCEPT_TREATY_REQ);

  dio_put_sint8(&dout, real_packet->counterpart);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_accept_treaty_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ] = variant;
}

struct packet_diplomacy_accept_treaty_req *receive_packet_diplomacy_accept_treaty_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_accept_treaty_req at the client.");
  }
  ensure_valid_variant_packet_diplomacy_accept_treaty_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ]) {
    case 100: return receive_packet_diplomacy_accept_treaty_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_accept_treaty_req(struct connection *pc, const struct packet_diplomacy_accept_treaty_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_accept_treaty_req from the server.");
  }
  ensure_valid_variant_packet_diplomacy_accept_treaty_req(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ]) {
    case 100: return send_packet_diplomacy_accept_treaty_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_diplomacy_accept_treaty_req(struct connection *pc, int counterpart)
{
  struct packet_diplomacy_accept_treaty_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  
  return send_packet_diplomacy_accept_treaty_req(pc, real_packet);
}

static struct packet_diplomacy_accept_treaty *receive_packet_diplomacy_accept_treaty_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_accept_treaty, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->counterpart = readin;
  }
  dio_get_bool8(&din, &real_packet->I_accepted);
  dio_get_bool8(&din, &real_packet->other_accepted);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_accept_treaty_100(struct connection *pc, const struct packet_diplomacy_accept_treaty *packet)
{
  const struct packet_diplomacy_accept_treaty *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_ACCEPT_TREATY);

  dio_put_sint8(&dout, real_packet->counterpart);
  dio_put_bool8(&dout, real_packet->I_accepted);
  dio_put_bool8(&dout, real_packet->other_accepted);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_accept_treaty(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY] = variant;
}

struct packet_diplomacy_accept_treaty *receive_packet_diplomacy_accept_treaty(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_accept_treaty at the server.");
  }
  ensure_valid_variant_packet_diplomacy_accept_treaty(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY]) {
    case 100: return receive_packet_diplomacy_accept_treaty_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_accept_treaty(struct connection *pc, const struct packet_diplomacy_accept_treaty *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_accept_treaty from the client.");
  }
  ensure_valid_variant_packet_diplomacy_accept_treaty(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY]) {
    case 100: return send_packet_diplomacy_accept_treaty_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_diplomacy_accept_treaty(struct conn_list *dest, const struct packet_diplomacy_accept_treaty *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_diplomacy_accept_treaty(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_diplomacy_accept_treaty(struct connection *pc, int counterpart, bool I_accepted, bool other_accepted)
{
  struct packet_diplomacy_accept_treaty packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->I_accepted = I_accepted;
  real_packet->other_accepted = other_accepted;
  
  return send_packet_diplomacy_accept_treaty(pc, real_packet);
}

void dlsend_packet_diplomacy_accept_treaty(struct conn_list *dest, int counterpart, bool I_accepted, bool other_accepted)
{
  struct packet_diplomacy_accept_treaty packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->I_accepted = I_accepted;
  real_packet->other_accepted = other_accepted;
  
  lsend_packet_diplomacy_accept_treaty(dest, real_packet);
}

static struct packet_diplomacy_cancel_pact *receive_packet_diplomacy_cancel_pact_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_diplomacy_cancel_pact, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->other_player_id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->clause = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_diplomacy_cancel_pact_100(struct connection *pc, const struct packet_diplomacy_cancel_pact *packet)
{
  const struct packet_diplomacy_cancel_pact *real_packet = packet;
  SEND_PACKET_START(PACKET_DIPLOMACY_CANCEL_PACT);

  dio_put_sint8(&dout, real_packet->other_player_id);
  dio_put_uint8(&dout, real_packet->clause);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_diplomacy_cancel_pact(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_PACT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_DIPLOMACY_CANCEL_PACT] = variant;
}

struct packet_diplomacy_cancel_pact *receive_packet_diplomacy_cancel_pact(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_cancel_pact at the client.");
  }
  ensure_valid_variant_packet_diplomacy_cancel_pact(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_PACT]) {
    case 100: return receive_packet_diplomacy_cancel_pact_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_diplomacy_cancel_pact(struct connection *pc, const struct packet_diplomacy_cancel_pact *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_cancel_pact from the server.");
  }
  ensure_valid_variant_packet_diplomacy_cancel_pact(pc);

  switch(pc->phs.variant[PACKET_DIPLOMACY_CANCEL_PACT]) {
    case 100: return send_packet_diplomacy_cancel_pact_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_diplomacy_cancel_pact(struct connection *pc, int other_player_id, enum clause_type clause)
{
  struct packet_diplomacy_cancel_pact packet, *real_packet = &packet;

  real_packet->other_player_id = other_player_id;
  real_packet->clause = clause;
  
  return send_packet_diplomacy_cancel_pact(pc, real_packet);
}

static struct packet_page_msg *receive_packet_page_msg_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_page_msg, real_packet);
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->event = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_page_msg_100(struct connection *pc, const struct packet_page_msg *packet)
{
  const struct packet_page_msg *real_packet = packet;
  SEND_PACKET_START(PACKET_PAGE_MSG);

  dio_put_string(&dout, real_packet->message);
  dio_put_sint32(&dout, real_packet->event);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_page_msg(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_PAGE_MSG] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_PAGE_MSG] = variant;
}

struct packet_page_msg *receive_packet_page_msg(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_page_msg at the server.");
  }
  ensure_valid_variant_packet_page_msg(pc);

  switch(pc->phs.variant[PACKET_PAGE_MSG]) {
    case 100: return receive_packet_page_msg_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_page_msg(struct connection *pc, const struct packet_page_msg *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_page_msg from the client.");
  }
  ensure_valid_variant_packet_page_msg(pc);

  switch(pc->phs.variant[PACKET_PAGE_MSG]) {
    case 100: return send_packet_page_msg_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_page_msg(struct conn_list *dest, const struct packet_page_msg *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_page_msg(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_report_req *receive_packet_report_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_report_req, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_report_req_100(struct connection *pc, const struct packet_report_req *packet)
{
  const struct packet_report_req *real_packet = packet;
  SEND_PACKET_START(PACKET_REPORT_REQ);

  dio_put_uint8(&dout, real_packet->type);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_report_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_REPORT_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_REPORT_REQ] = variant;
}

struct packet_report_req *receive_packet_report_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_report_req at the client.");
  }
  ensure_valid_variant_packet_report_req(pc);

  switch(pc->phs.variant[PACKET_REPORT_REQ]) {
    case 100: return receive_packet_report_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_report_req(struct connection *pc, const struct packet_report_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_report_req from the server.");
  }
  ensure_valid_variant_packet_report_req(pc);

  switch(pc->phs.variant[PACKET_REPORT_REQ]) {
    case 100: return send_packet_report_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_report_req(struct connection *pc, enum report_type type)
{
  struct packet_report_req packet, *real_packet = &packet;

  real_packet->type = type;
  
  return send_packet_report_req(pc, real_packet);
}

static struct packet_conn_info *receive_packet_conn_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_conn_info, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_bool8(&din, &real_packet->used);
  dio_get_bool8(&din, &real_packet->established);
  dio_get_bool8(&din, &real_packet->observer);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->player_num = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->access_level = readin;
  }
  dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
  dio_get_string(&din, real_packet->addr, sizeof(real_packet->addr));
  dio_get_string(&din, real_packet->capability, sizeof(real_packet->capability));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_conn_info_100(struct connection *pc, const struct packet_conn_info *packet)
{
  const struct packet_conn_info *real_packet = packet;
  SEND_PACKET_START(PACKET_CONN_INFO);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_bool8(&dout, real_packet->used);
  dio_put_bool8(&dout, real_packet->established);
  dio_put_bool8(&dout, real_packet->observer);
  dio_put_sint8(&dout, real_packet->player_num);
  dio_put_uint8(&dout, real_packet->access_level);
  dio_put_string(&dout, real_packet->username);
  dio_put_string(&dout, real_packet->addr);
  dio_put_string(&dout, real_packet->capability);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_conn_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CONN_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CONN_INFO] = variant;
}

struct packet_conn_info *receive_packet_conn_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_conn_info at the server.");
  }
  ensure_valid_variant_packet_conn_info(pc);

  switch(pc->phs.variant[PACKET_CONN_INFO]) {
    case 100: return receive_packet_conn_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_conn_info(struct connection *pc, const struct packet_conn_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_conn_info from the client.");
  }
  ensure_valid_variant_packet_conn_info(pc);

  switch(pc->phs.variant[PACKET_CONN_INFO]) {
    case 100: return send_packet_conn_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_conn_info(struct conn_list *dest, const struct packet_conn_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_conn_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_conn_ping_info *receive_packet_conn_ping_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_conn_ping_info, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->connections = readin;
  }
  
  {
    int i;
  
    if(real_packet->connections > MAX_NUM_CONNECTIONS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_CONNECTIONS;
    }
    for (i = 0; i < real_packet->connections; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->conn_id[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->connections > MAX_NUM_CONNECTIONS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_CONNECTIONS;
    }
    for (i = 0; i < real_packet->connections; i++) {
      int tmp;
  
      dio_get_uint32(&din, &tmp);
      real_packet->ping_time[i] = (float)(tmp) / 1000000.0;
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_conn_ping_info_100(struct connection *pc, const struct packet_conn_ping_info *packet)
{
  const struct packet_conn_ping_info *real_packet = packet;
  SEND_PACKET_START(PACKET_CONN_PING_INFO);

  dio_put_uint8(&dout, real_packet->connections);

    {
      int i;

      for (i = 0; i < real_packet->connections; i++) {
        dio_put_uint8(&dout, real_packet->conn_id[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->connections; i++) {
          dio_put_uint32(&dout, (int)(real_packet->ping_time[i] * 1000000));
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_conn_ping_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CONN_PING_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CONN_PING_INFO] = variant;
}

struct packet_conn_ping_info *receive_packet_conn_ping_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_conn_ping_info at the server.");
  }
  ensure_valid_variant_packet_conn_ping_info(pc);

  switch(pc->phs.variant[PACKET_CONN_PING_INFO]) {
    case 100: return receive_packet_conn_ping_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_conn_ping_info(struct connection *pc, const struct packet_conn_ping_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_conn_ping_info from the client.");
  }
  ensure_valid_variant_packet_conn_ping_info(pc);

  switch(pc->phs.variant[PACKET_CONN_PING_INFO]) {
    case 100: return send_packet_conn_ping_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_conn_ping_info(struct conn_list *dest, const struct packet_conn_ping_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_conn_ping_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_conn_ping *receive_packet_conn_ping_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_conn_ping, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_conn_ping_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_CONN_PING);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_conn_ping(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CONN_PING] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CONN_PING] = variant;
}

struct packet_conn_ping *receive_packet_conn_ping(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_conn_ping at the server.");
  }
  ensure_valid_variant_packet_conn_ping(pc);

  switch(pc->phs.variant[PACKET_CONN_PING]) {
    case 100: return receive_packet_conn_ping_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_conn_ping(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_conn_ping from the client.");
  }
  ensure_valid_variant_packet_conn_ping(pc);

  switch(pc->phs.variant[PACKET_CONN_PING]) {
    case 100: return send_packet_conn_ping_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_conn_pong *receive_packet_conn_pong_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_conn_pong, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_conn_pong_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_CONN_PONG);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_conn_pong(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CONN_PONG] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CONN_PONG] = variant;
}

struct packet_conn_pong *receive_packet_conn_pong(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_conn_pong at the client.");
  }
  ensure_valid_variant_packet_conn_pong(pc);

  switch(pc->phs.variant[PACKET_CONN_PONG]) {
    case 100: return receive_packet_conn_pong_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_conn_pong(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_conn_pong from the server.");
  }
  ensure_valid_variant_packet_conn_pong(pc);

  switch(pc->phs.variant[PACKET_CONN_PONG]) {
    case 100: return send_packet_conn_pong_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_client_info *receive_packet_client_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_client_info, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->gui = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_client_info_100(struct connection *pc, const struct packet_client_info *packet)
{
  const struct packet_client_info *real_packet = packet;
  SEND_PACKET_START(PACKET_CLIENT_INFO);

  dio_put_uint8(&dout, real_packet->gui);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_client_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_CLIENT_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_CLIENT_INFO] = variant;
}

struct packet_client_info *receive_packet_client_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_client_info at the client.");
  }
  ensure_valid_variant_packet_client_info(pc);

  switch(pc->phs.variant[PACKET_CLIENT_INFO]) {
    case 100: return receive_packet_client_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_client_info(struct connection *pc, const struct packet_client_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_client_info from the server.");
  }
  ensure_valid_variant_packet_client_info(pc);

  switch(pc->phs.variant[PACKET_CLIENT_INFO]) {
    case 100: return send_packet_client_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_end_phase *receive_packet_end_phase_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_end_phase, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_end_phase_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_END_PHASE);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_end_phase(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_END_PHASE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_END_PHASE] = variant;
}

struct packet_end_phase *receive_packet_end_phase(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_end_phase at the server.");
  }
  ensure_valid_variant_packet_end_phase(pc);

  switch(pc->phs.variant[PACKET_END_PHASE]) {
    case 100: return receive_packet_end_phase_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_end_phase(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_end_phase from the client.");
  }
  ensure_valid_variant_packet_end_phase(pc);

  switch(pc->phs.variant[PACKET_END_PHASE]) {
    case 100: return send_packet_end_phase_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_end_phase(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_end_phase(pconn);
  } conn_list_iterate_end;
}

static struct packet_start_phase *receive_packet_start_phase_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_start_phase, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->phase = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_start_phase_100(struct connection *pc, const struct packet_start_phase *packet)
{
  const struct packet_start_phase *real_packet = packet;
  SEND_PACKET_START(PACKET_START_PHASE);

  dio_put_uint32(&dout, real_packet->phase);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_start_phase(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_START_PHASE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_START_PHASE] = variant;
}

struct packet_start_phase *receive_packet_start_phase(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_start_phase at the server.");
  }
  ensure_valid_variant_packet_start_phase(pc);

  switch(pc->phs.variant[PACKET_START_PHASE]) {
    case 100: return receive_packet_start_phase_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_start_phase(struct connection *pc, const struct packet_start_phase *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_start_phase from the client.");
  }
  ensure_valid_variant_packet_start_phase(pc);

  switch(pc->phs.variant[PACKET_START_PHASE]) {
    case 100: return send_packet_start_phase_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_start_phase(struct conn_list *dest, const struct packet_start_phase *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_start_phase(pconn, packet);
  } conn_list_iterate_end;
}

int dsend_packet_start_phase(struct connection *pc, int phase)
{
  struct packet_start_phase packet, *real_packet = &packet;

  real_packet->phase = phase;
  
  return send_packet_start_phase(pc, real_packet);
}

void dlsend_packet_start_phase(struct conn_list *dest, int phase)
{
  struct packet_start_phase packet, *real_packet = &packet;

  real_packet->phase = phase;
  
  lsend_packet_start_phase(dest, real_packet);
}

static struct packet_new_year *receive_packet_new_year_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_new_year, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->year = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->turn = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_new_year_100(struct connection *pc, const struct packet_new_year *packet)
{
  const struct packet_new_year *real_packet = packet;
  SEND_PACKET_START(PACKET_NEW_YEAR);

  dio_put_uint32(&dout, real_packet->year);
  dio_put_uint32(&dout, real_packet->turn);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_new_year(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_NEW_YEAR] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_NEW_YEAR] = variant;
}

struct packet_new_year *receive_packet_new_year(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_new_year at the server.");
  }
  ensure_valid_variant_packet_new_year(pc);

  switch(pc->phs.variant[PACKET_NEW_YEAR]) {
    case 100: return receive_packet_new_year_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_new_year(struct connection *pc, const struct packet_new_year *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_new_year from the client.");
  }
  ensure_valid_variant_packet_new_year(pc);

  switch(pc->phs.variant[PACKET_NEW_YEAR]) {
    case 100: return send_packet_new_year_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_new_year(struct conn_list *dest, const struct packet_new_year *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_new_year(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_begin_turn *receive_packet_begin_turn_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_begin_turn, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_begin_turn_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_BEGIN_TURN);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_begin_turn(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_BEGIN_TURN] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_BEGIN_TURN] = variant;
}

struct packet_begin_turn *receive_packet_begin_turn(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_begin_turn at the server.");
  }
  ensure_valid_variant_packet_begin_turn(pc);

  switch(pc->phs.variant[PACKET_BEGIN_TURN]) {
    case 100: return receive_packet_begin_turn_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_begin_turn(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_begin_turn from the client.");
  }
  ensure_valid_variant_packet_begin_turn(pc);

  switch(pc->phs.variant[PACKET_BEGIN_TURN]) {
    case 100: return send_packet_begin_turn_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_begin_turn(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_begin_turn(pconn);
  } conn_list_iterate_end;
}

static struct packet_end_turn *receive_packet_end_turn_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_end_turn, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_end_turn_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_END_TURN);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_end_turn(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_END_TURN] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_END_TURN] = variant;
}

struct packet_end_turn *receive_packet_end_turn(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_end_turn at the server.");
  }
  ensure_valid_variant_packet_end_turn(pc);

  switch(pc->phs.variant[PACKET_END_TURN]) {
    case 100: return receive_packet_end_turn_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_end_turn(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_end_turn from the client.");
  }
  ensure_valid_variant_packet_end_turn(pc);

  switch(pc->phs.variant[PACKET_END_TURN]) {
    case 100: return send_packet_end_turn_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_end_turn(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_end_turn(pconn);
  } conn_list_iterate_end;
}

static struct packet_freeze_client *receive_packet_freeze_client_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_freeze_client, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_freeze_client_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_FREEZE_CLIENT);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_freeze_client(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_FREEZE_CLIENT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_FREEZE_CLIENT] = variant;
}

struct packet_freeze_client *receive_packet_freeze_client(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_freeze_client at the server.");
  }
  ensure_valid_variant_packet_freeze_client(pc);

  switch(pc->phs.variant[PACKET_FREEZE_CLIENT]) {
    case 100: return receive_packet_freeze_client_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_freeze_client(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_freeze_client from the client.");
  }
  ensure_valid_variant_packet_freeze_client(pc);

  switch(pc->phs.variant[PACKET_FREEZE_CLIENT]) {
    case 100: return send_packet_freeze_client_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_freeze_client(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_freeze_client(pconn);
  } conn_list_iterate_end;
}

static struct packet_thaw_client *receive_packet_thaw_client_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_thaw_client, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_thaw_client_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_THAW_CLIENT);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_thaw_client(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_THAW_CLIENT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_THAW_CLIENT] = variant;
}

struct packet_thaw_client *receive_packet_thaw_client(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_thaw_client at the server.");
  }
  ensure_valid_variant_packet_thaw_client(pc);

  switch(pc->phs.variant[PACKET_THAW_CLIENT]) {
    case 100: return receive_packet_thaw_client_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_thaw_client(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_thaw_client from the client.");
  }
  ensure_valid_variant_packet_thaw_client(pc);

  switch(pc->phs.variant[PACKET_THAW_CLIENT]) {
    case 100: return send_packet_thaw_client_100(pc);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_thaw_client(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) {
    send_packet_thaw_client(pconn);
  } conn_list_iterate_end;
}

static struct packet_spaceship_launch *receive_packet_spaceship_launch_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_spaceship_launch, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_spaceship_launch_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_SPACESHIP_LAUNCH);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_spaceship_launch(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SPACESHIP_LAUNCH] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SPACESHIP_LAUNCH] = variant;
}

struct packet_spaceship_launch *receive_packet_spaceship_launch(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_spaceship_launch at the client.");
  }
  ensure_valid_variant_packet_spaceship_launch(pc);

  switch(pc->phs.variant[PACKET_SPACESHIP_LAUNCH]) {
    case 100: return receive_packet_spaceship_launch_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_spaceship_launch(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_spaceship_launch from the server.");
  }
  ensure_valid_variant_packet_spaceship_launch(pc);

  switch(pc->phs.variant[PACKET_SPACESHIP_LAUNCH]) {
    case 100: return send_packet_spaceship_launch_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_spaceship_place *receive_packet_spaceship_place_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_spaceship_place, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_spaceship_place_100(struct connection *pc, const struct packet_spaceship_place *packet)
{
  const struct packet_spaceship_place *real_packet = packet;
  SEND_PACKET_START(PACKET_SPACESHIP_PLACE);

  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint8(&dout, real_packet->num);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_spaceship_place(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SPACESHIP_PLACE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SPACESHIP_PLACE] = variant;
}

struct packet_spaceship_place *receive_packet_spaceship_place(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_spaceship_place at the client.");
  }
  ensure_valid_variant_packet_spaceship_place(pc);

  switch(pc->phs.variant[PACKET_SPACESHIP_PLACE]) {
    case 100: return receive_packet_spaceship_place_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_spaceship_place(struct connection *pc, const struct packet_spaceship_place *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_spaceship_place from the server.");
  }
  ensure_valid_variant_packet_spaceship_place(pc);

  switch(pc->phs.variant[PACKET_SPACESHIP_PLACE]) {
    case 100: return send_packet_spaceship_place_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_spaceship_place(struct connection *pc, enum spaceship_place_type type, int num)
{
  struct packet_spaceship_place packet, *real_packet = &packet;

  real_packet->type = type;
  real_packet->num = num;
  
  return send_packet_spaceship_place(pc, real_packet);
}

static struct packet_spaceship_info *receive_packet_spaceship_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_spaceship_info, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->player_num = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->sship_state = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->structurals = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->components = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->modules = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->propulsion = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->habitation = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->life_support = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->solar_panels = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->launch_year = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->population = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->mass = readin;
  }
  dio_get_bit_string(&din, real_packet->structure, sizeof(real_packet->structure));
  {
    int tmp;
    
    dio_get_uint32(&din, &tmp);
    real_packet->support_rate = (float)(tmp) / 10000.0;
  }
  {
    int tmp;
    
    dio_get_uint32(&din, &tmp);
    real_packet->energy_rate = (float)(tmp) / 10000.0;
  }
  {
    int tmp;
    
    dio_get_uint32(&din, &tmp);
    real_packet->success_rate = (float)(tmp) / 10000.0;
  }
  {
    int tmp;
    
    dio_get_uint32(&din, &tmp);
    real_packet->travel_time = (float)(tmp) / 10000.0;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_spaceship_info_100(struct connection *pc, const struct packet_spaceship_info *packet)
{
  const struct packet_spaceship_info *real_packet = packet;
  SEND_PACKET_START(PACKET_SPACESHIP_INFO);

  dio_put_sint8(&dout, real_packet->player_num);
  dio_put_uint8(&dout, real_packet->sship_state);
  dio_put_uint8(&dout, real_packet->structurals);
  dio_put_uint8(&dout, real_packet->components);
  dio_put_uint8(&dout, real_packet->modules);
  dio_put_uint8(&dout, real_packet->fuel);
  dio_put_uint8(&dout, real_packet->propulsion);
  dio_put_uint8(&dout, real_packet->habitation);
  dio_put_uint8(&dout, real_packet->life_support);
  dio_put_uint8(&dout, real_packet->solar_panels);
  dio_put_uint32(&dout, real_packet->launch_year);
  dio_put_uint32(&dout, real_packet->population);
  dio_put_uint32(&dout, real_packet->mass);
  dio_put_bit_string(&dout, real_packet->structure);
  dio_put_uint32(&dout, (int)(real_packet->support_rate * 10000));
  dio_put_uint32(&dout, (int)(real_packet->energy_rate * 10000));
  dio_put_uint32(&dout, (int)(real_packet->success_rate * 10000));
  dio_put_uint32(&dout, (int)(real_packet->travel_time * 10000));

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_spaceship_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SPACESHIP_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SPACESHIP_INFO] = variant;
}

struct packet_spaceship_info *receive_packet_spaceship_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_spaceship_info at the server.");
  }
  ensure_valid_variant_packet_spaceship_info(pc);

  switch(pc->phs.variant[PACKET_SPACESHIP_INFO]) {
    case 100: return receive_packet_spaceship_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_spaceship_info(struct connection *pc, const struct packet_spaceship_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_spaceship_info from the client.");
  }
  ensure_valid_variant_packet_spaceship_info(pc);

  switch(pc->phs.variant[PACKET_SPACESHIP_INFO]) {
    case 100: return send_packet_spaceship_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_spaceship_info(struct conn_list *dest, const struct packet_spaceship_info *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_spaceship_info(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_unit *receive_packet_ruleset_unit_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_unit, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
  dio_get_string(&din, real_packet->sound_move, sizeof(real_packet->sound_move));
  dio_get_string(&din, real_packet->sound_move_alt, sizeof(real_packet->sound_move_alt));
  dio_get_string(&din, real_packet->sound_fight, sizeof(real_packet->sound_fight));
  dio_get_string(&din, real_packet->sound_fight_alt, sizeof(real_packet->sound_fight_alt));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->unit_class_id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->build_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->pop_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->attack_strength = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->defense_strength = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->move_rate = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->tech_requirement = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->impr_requirement = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->gov_requirement = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->vision_radius_sq = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->transport_capacity = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->firepower = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->obsoleted_by = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->transformed_to = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->happy_cost = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->upkeep[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_range = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_mr_req = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_mr_sub = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      dio_get_string(&din, real_packet->veteran_name[i], sizeof(real_packet->veteran_name[i]));
    }
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int tmp;
  
      dio_get_uint32(&din, &tmp);
      real_packet->power_fact[i] = (float)(tmp) / 10000.0;
    }
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->move_bonus[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->bombard_rate = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->city_size = readin;
  }
  DIO_BV_GET(&din, real_packet->cargo);
  DIO_BV_GET(&din, real_packet->targets);
  dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
  DIO_BV_GET(&din, real_packet->flags);
  DIO_BV_GET(&din, real_packet->roles);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_unit_100(struct connection *pc, const struct packet_ruleset_unit *packet)
{
  const struct packet_ruleset_unit *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_UNIT);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);
  dio_put_string(&dout, real_packet->sound_move);
  dio_put_string(&dout, real_packet->sound_move_alt);
  dio_put_string(&dout, real_packet->sound_fight);
  dio_put_string(&dout, real_packet->sound_fight_alt);
  dio_put_uint8(&dout, real_packet->unit_class_id);
  dio_put_uint32(&dout, real_packet->build_cost);
  dio_put_uint8(&dout, real_packet->pop_cost);
  dio_put_uint8(&dout, real_packet->attack_strength);
  dio_put_uint8(&dout, real_packet->defense_strength);
  dio_put_uint8(&dout, real_packet->move_rate);
  dio_put_uint8(&dout, real_packet->tech_requirement);
  dio_put_uint8(&dout, real_packet->impr_requirement);
  dio_put_uint8(&dout, real_packet->gov_requirement);
  dio_put_uint32(&dout, real_packet->vision_radius_sq);
  dio_put_uint8(&dout, real_packet->transport_capacity);
  dio_put_uint8(&dout, real_packet->hp);
  dio_put_uint8(&dout, real_packet->firepower);
  dio_put_sint8(&dout, real_packet->obsoleted_by);
  dio_put_sint8(&dout, real_packet->transformed_to);
  dio_put_uint8(&dout, real_packet->fuel);
  dio_put_uint8(&dout, real_packet->happy_cost);

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint8(&dout, real_packet->upkeep[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->paratroopers_range);
  dio_put_uint8(&dout, real_packet->paratroopers_mr_req);
  dio_put_uint8(&dout, real_packet->paratroopers_mr_sub);

    {
      int i;

      for (i = 0; i < MAX_VET_LEVELS; i++) {
        dio_put_string(&dout, real_packet->veteran_name[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < MAX_VET_LEVELS; i++) {
          dio_put_uint32(&dout, (int)(real_packet->power_fact[i] * 10000));
      }
    } 

    {
      int i;

      for (i = 0; i < MAX_VET_LEVELS; i++) {
        dio_put_uint8(&dout, real_packet->move_bonus[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->bombard_rate);
  dio_put_uint8(&dout, real_packet->city_size);
DIO_BV_PUT(&dout, packet->cargo);
DIO_BV_PUT(&dout, packet->targets);
  dio_put_string(&dout, real_packet->helptext);
DIO_BV_PUT(&dout, packet->flags);
DIO_BV_PUT(&dout, packet->roles);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_unit(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_UNIT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_UNIT] = variant;
}

struct packet_ruleset_unit *receive_packet_ruleset_unit(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_unit at the server.");
  }
  ensure_valid_variant_packet_ruleset_unit(pc);

  switch(pc->phs.variant[PACKET_RULESET_UNIT]) {
    case 100: return receive_packet_ruleset_unit_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_unit(struct connection *pc, const struct packet_ruleset_unit *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_unit from the client.");
  }
  ensure_valid_variant_packet_ruleset_unit(pc);

  switch(pc->phs.variant[PACKET_RULESET_UNIT]) {
    case 100: return send_packet_ruleset_unit_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_unit(struct conn_list *dest, const struct packet_ruleset_unit *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_unit(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_game *receive_packet_ruleset_game_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_game, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->default_specialist = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->work_veteran_chance[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->veteran_chance[i] = readin;
  }
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_game_100(struct connection *pc, const struct packet_ruleset_game *packet)
{
  const struct packet_ruleset_game *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_GAME);

  dio_put_uint8(&dout, real_packet->default_specialist);

    {
      int i;

      for (i = 0; i < MAX_VET_LEVELS; i++) {
        dio_put_uint8(&dout, real_packet->work_veteran_chance[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < MAX_VET_LEVELS; i++) {
        dio_put_uint8(&dout, real_packet->veteran_chance[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_game(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_GAME] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_GAME] = variant;
}

struct packet_ruleset_game *receive_packet_ruleset_game(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_game at the server.");
  }
  ensure_valid_variant_packet_ruleset_game(pc);

  switch(pc->phs.variant[PACKET_RULESET_GAME]) {
    case 100: return receive_packet_ruleset_game_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_game(struct connection *pc, const struct packet_ruleset_game *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_game from the client.");
  }
  ensure_valid_variant_packet_ruleset_game(pc);

  switch(pc->phs.variant[PACKET_RULESET_GAME]) {
    case 100: return send_packet_ruleset_game_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_game(struct conn_list *dest, const struct packet_ruleset_game *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_game(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_specialist *receive_packet_ruleset_specialist_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_specialist, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->short_name, sizeof(real_packet->short_name));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->reqs_count = readin;
  }
  
  {
    int i;
  
    if(real_packet->reqs_count > MAX_NUM_REQS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->reqs_count = MAX_NUM_REQS;
    }
    for (i = 0; i < real_packet->reqs_count; i++) {
      dio_get_requirement(&din, &real_packet->reqs[i]);
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_specialist_100(struct connection *pc, const struct packet_ruleset_specialist *packet)
{
  const struct packet_ruleset_specialist *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_SPECIALIST);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->short_name);
  dio_put_uint8(&dout, real_packet->reqs_count);

    {
      int i;

      for (i = 0; i < real_packet->reqs_count; i++) {
        dio_put_requirement(&dout, &real_packet->reqs[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_specialist(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_SPECIALIST] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_SPECIALIST] = variant;
}

struct packet_ruleset_specialist *receive_packet_ruleset_specialist(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_specialist at the server.");
  }
  ensure_valid_variant_packet_ruleset_specialist(pc);

  switch(pc->phs.variant[PACKET_RULESET_SPECIALIST]) {
    case 100: return receive_packet_ruleset_specialist_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_specialist(struct connection *pc, const struct packet_ruleset_specialist *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_specialist from the client.");
  }
  ensure_valid_variant_packet_ruleset_specialist(pc);

  switch(pc->phs.variant[PACKET_RULESET_SPECIALIST]) {
    case 100: return send_packet_ruleset_specialist_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_specialist(struct conn_list *dest, const struct packet_ruleset_specialist *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_specialist(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_government_ruler_title *receive_packet_ruleset_government_ruler_title_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_government_ruler_title, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->gov = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nation = readin;
  }
  dio_get_string(&din, real_packet->male_title, sizeof(real_packet->male_title));
  dio_get_string(&din, real_packet->female_title, sizeof(real_packet->female_title));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_government_ruler_title_100(struct connection *pc, const struct packet_ruleset_government_ruler_title *packet)
{
  const struct packet_ruleset_government_ruler_title *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_GOVERNMENT_RULER_TITLE);

  dio_put_uint8(&dout, real_packet->gov);
  dio_put_uint8(&dout, real_packet->id);
  dio_put_uint32(&dout, real_packet->nation);
  dio_put_string(&dout, real_packet->male_title);
  dio_put_string(&dout, real_packet->female_title);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_government_ruler_title(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_GOVERNMENT_RULER_TITLE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_GOVERNMENT_RULER_TITLE] = variant;
}

struct packet_ruleset_government_ruler_title *receive_packet_ruleset_government_ruler_title(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_government_ruler_title at the server.");
  }
  ensure_valid_variant_packet_ruleset_government_ruler_title(pc);

  switch(pc->phs.variant[PACKET_RULESET_GOVERNMENT_RULER_TITLE]) {
    case 100: return receive_packet_ruleset_government_ruler_title_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_government_ruler_title(struct connection *pc, const struct packet_ruleset_government_ruler_title *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_government_ruler_title from the client.");
  }
  ensure_valid_variant_packet_ruleset_government_ruler_title(pc);

  switch(pc->phs.variant[PACKET_RULESET_GOVERNMENT_RULER_TITLE]) {
    case 100: return send_packet_ruleset_government_ruler_title_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_government_ruler_title(struct conn_list *dest, const struct packet_ruleset_government_ruler_title *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_government_ruler_title(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_tech *receive_packet_ruleset_tech_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_tech, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < 2; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->req[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->root_req = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->flags = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->preset_cost = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->num_reqs = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_tech_100(struct connection *pc, const struct packet_ruleset_tech *packet)
{
  const struct packet_ruleset_tech *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_TECH);

  dio_put_uint8(&dout, real_packet->id);

    {
      int i;

      for (i = 0; i < 2; i++) {
        dio_put_uint8(&dout, real_packet->req[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->root_req);
  dio_put_uint32(&dout, real_packet->flags);
  dio_put_uint32(&dout, real_packet->preset_cost);
  dio_put_uint32(&dout, real_packet->num_reqs);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->helptext);
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_tech(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_TECH] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_TECH] = variant;
}

struct packet_ruleset_tech *receive_packet_ruleset_tech(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_tech at the server.");
  }
  ensure_valid_variant_packet_ruleset_tech(pc);

  switch(pc->phs.variant[PACKET_RULESET_TECH]) {
    case 100: return receive_packet_ruleset_tech_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_tech(struct connection *pc, const struct packet_ruleset_tech *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_tech from the client.");
  }
  ensure_valid_variant_packet_ruleset_tech(pc);

  switch(pc->phs.variant[PACKET_RULESET_TECH]) {
    case 100: return send_packet_ruleset_tech_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_tech(struct conn_list *dest, const struct packet_ruleset_tech *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_tech(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_government *receive_packet_ruleset_government_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_government, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_ruler_titles = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
  dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_government_100(struct connection *pc, const struct packet_ruleset_government *packet)
{
  const struct packet_ruleset_government *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_GOVERNMENT);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_uint8(&dout, real_packet->num_ruler_titles);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);
  dio_put_string(&dout, real_packet->helptext);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_government(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_GOVERNMENT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_GOVERNMENT] = variant;
}

struct packet_ruleset_government *receive_packet_ruleset_government(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_government at the server.");
  }
  ensure_valid_variant_packet_ruleset_government(pc);

  switch(pc->phs.variant[PACKET_RULESET_GOVERNMENT]) {
    case 100: return receive_packet_ruleset_government_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_government(struct connection *pc, const struct packet_ruleset_government *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_government from the client.");
  }
  ensure_valid_variant_packet_ruleset_government(pc);

  switch(pc->phs.variant[PACKET_RULESET_GOVERNMENT]) {
    case 100: return send_packet_ruleset_government_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_government(struct conn_list *dest, const struct packet_ruleset_government *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_government(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_terrain_control *receive_packet_ruleset_terrain_control_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_terrain_control, real_packet);
  dio_get_bool8(&din, &real_packet->may_road);
  dio_get_bool8(&din, &real_packet->may_irrigate);
  dio_get_bool8(&din, &real_packet->may_mine);
  dio_get_bool8(&din, &real_packet->may_transform);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ocean_reclaim_requirement_pct = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->land_channel_requirement_pct = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->lake_max_size = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->river_move_mode = readin;
  }
  {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->river_defense_bonus = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->river_trade_incr = readin;
  }
  dio_get_string(&din, real_packet->river_help_text, sizeof(real_packet->river_help_text));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->road_superhighway_trade_bonus = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->rail_tile_bonus[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->pollution_tile_penalty[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->fallout_tile_penalty[i] = readin;
  }
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_terrain_control_100(struct connection *pc, const struct packet_ruleset_terrain_control *packet)
{
  const struct packet_ruleset_terrain_control *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_TERRAIN_CONTROL);

  dio_put_bool8(&dout, real_packet->may_road);
  dio_put_bool8(&dout, real_packet->may_irrigate);
  dio_put_bool8(&dout, real_packet->may_mine);
  dio_put_bool8(&dout, real_packet->may_transform);
  dio_put_uint8(&dout, real_packet->ocean_reclaim_requirement_pct);
  dio_put_uint8(&dout, real_packet->land_channel_requirement_pct);
  dio_put_uint8(&dout, real_packet->lake_max_size);
  dio_put_uint8(&dout, real_packet->river_move_mode);
  dio_put_sint16(&dout, real_packet->river_defense_bonus);
  dio_put_uint32(&dout, real_packet->river_trade_incr);
  dio_put_string(&dout, real_packet->river_help_text);
  dio_put_uint32(&dout, real_packet->road_superhighway_trade_bonus);

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint32(&dout, real_packet->rail_tile_bonus[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint8(&dout, real_packet->pollution_tile_penalty[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint8(&dout, real_packet->fallout_tile_penalty[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_terrain_control(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_TERRAIN_CONTROL] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_TERRAIN_CONTROL] = variant;
}

struct packet_ruleset_terrain_control *receive_packet_ruleset_terrain_control(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_terrain_control at the server.");
  }
  ensure_valid_variant_packet_ruleset_terrain_control(pc);

  switch(pc->phs.variant[PACKET_RULESET_TERRAIN_CONTROL]) {
    case 100: return receive_packet_ruleset_terrain_control_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_terrain_control(struct connection *pc, const struct packet_ruleset_terrain_control *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_terrain_control from the client.");
  }
  ensure_valid_variant_packet_ruleset_terrain_control(pc);

  switch(pc->phs.variant[PACKET_RULESET_TERRAIN_CONTROL]) {
    case 100: return send_packet_ruleset_terrain_control_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_terrain_control(struct conn_list *dest, const struct packet_ruleset_terrain_control *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_terrain_control(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_nation_groups *receive_packet_ruleset_nation_groups_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_nation_groups, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ngroups = readin;
  }
  
  {
    int i;
  
    if(real_packet->ngroups > MAX_NUM_NATION_GROUPS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->ngroups = MAX_NUM_NATION_GROUPS;
    }
    for (i = 0; i < real_packet->ngroups; i++) {
      dio_get_string(&din, real_packet->groups[i], sizeof(real_packet->groups[i]));
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_nation_groups_100(struct connection *pc, const struct packet_ruleset_nation_groups *packet)
{
  const struct packet_ruleset_nation_groups *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_NATION_GROUPS);

  dio_put_uint8(&dout, real_packet->ngroups);

    {
      int i;

      for (i = 0; i < real_packet->ngroups; i++) {
        dio_put_string(&dout, real_packet->groups[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_nation_groups(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_NATION_GROUPS] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_NATION_GROUPS] = variant;
}

struct packet_ruleset_nation_groups *receive_packet_ruleset_nation_groups(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_nation_groups at the server.");
  }
  ensure_valid_variant_packet_ruleset_nation_groups(pc);

  switch(pc->phs.variant[PACKET_RULESET_NATION_GROUPS]) {
    case 100: return receive_packet_ruleset_nation_groups_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_nation_groups(struct connection *pc, const struct packet_ruleset_nation_groups *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_nation_groups from the client.");
  }
  ensure_valid_variant_packet_ruleset_nation_groups(pc);

  switch(pc->phs.variant[PACKET_RULESET_NATION_GROUPS]) {
    case 100: return send_packet_ruleset_nation_groups_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_nation_groups(struct conn_list *dest, const struct packet_ruleset_nation_groups *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_nation_groups(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_nation *receive_packet_ruleset_nation_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_nation, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->adjective, sizeof(real_packet->adjective));
  dio_get_string(&din, real_packet->noun_plural, sizeof(real_packet->noun_plural));
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
  dio_get_string(&din, real_packet->legend, sizeof(real_packet->legend));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->init_units[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->init_buildings[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->init_government = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->leader_count = readin;
  }
  
  {
    int i;
  
    if(real_packet->leader_count > MAX_NUM_LEADERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->leader_count = MAX_NUM_LEADERS;
    }
    for (i = 0; i < real_packet->leader_count; i++) {
      dio_get_string(&din, real_packet->leader_name[i], sizeof(real_packet->leader_name[i]));
    }
  }
  
  {
    int i;
  
    if(real_packet->leader_count > MAX_NUM_LEADERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->leader_count = MAX_NUM_LEADERS;
    }
    for (i = 0; i < real_packet->leader_count; i++) {
      dio_get_bool8(&din, &real_packet->leader_sex[i]);
    }
  }
  dio_get_bool8(&din, &real_packet->is_available);
  dio_get_bool8(&din, &real_packet->is_playable);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ngroups = readin;
  }
  
  {
    int i;
  
    if(real_packet->ngroups > MAX_NUM_NATION_GROUPS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->ngroups = MAX_NUM_NATION_GROUPS;
    }
    for (i = 0; i < real_packet->ngroups; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->groups[i] = readin;
  }
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_nation_100(struct connection *pc, const struct packet_ruleset_nation *packet)
{
  const struct packet_ruleset_nation *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_NATION);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->adjective);
  dio_put_string(&dout, real_packet->noun_plural);
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);
  dio_put_string(&dout, real_packet->legend);
  dio_put_uint8(&dout, real_packet->city_style);

    {
      int i;

      for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
        dio_put_uint8(&dout, real_packet->init_units[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
        dio_put_uint8(&dout, real_packet->init_buildings[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->init_government);
  dio_put_uint8(&dout, real_packet->leader_count);

    {
      int i;

      for (i = 0; i < real_packet->leader_count; i++) {
        dio_put_string(&dout, real_packet->leader_name[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->leader_count; i++) {
        dio_put_bool8(&dout, real_packet->leader_sex[i]);
      }
    } 
  dio_put_bool8(&dout, real_packet->is_available);
  dio_put_bool8(&dout, real_packet->is_playable);
  dio_put_uint8(&dout, real_packet->barbarian_type);
  dio_put_uint8(&dout, real_packet->ngroups);

    {
      int i;

      for (i = 0; i < real_packet->ngroups; i++) {
        dio_put_uint8(&dout, real_packet->groups[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_nation(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_NATION] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_NATION] = variant;
}

struct packet_ruleset_nation *receive_packet_ruleset_nation(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_nation at the server.");
  }
  ensure_valid_variant_packet_ruleset_nation(pc);

  switch(pc->phs.variant[PACKET_RULESET_NATION]) {
    case 100: return receive_packet_ruleset_nation_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_nation(struct connection *pc, const struct packet_ruleset_nation *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_nation from the client.");
  }
  ensure_valid_variant_packet_ruleset_nation(pc);

  switch(pc->phs.variant[PACKET_RULESET_NATION]) {
    case 100: return send_packet_ruleset_nation_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_nation(struct conn_list *dest, const struct packet_ruleset_nation *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_nation(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_city *receive_packet_ruleset_city_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_city, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->style_id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->citizens_graphic, sizeof(real_packet->citizens_graphic));
  dio_get_string(&din, real_packet->citizens_graphic_alt, sizeof(real_packet->citizens_graphic_alt));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->reqs_count = readin;
  }
  
  {
    int i;
  
    if(real_packet->reqs_count > MAX_NUM_REQS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->reqs_count = MAX_NUM_REQS;
    }
    for (i = 0; i < real_packet->reqs_count; i++) {
      dio_get_requirement(&din, &real_packet->reqs[i]);
    }
  }
  dio_get_string(&din, real_packet->graphic, sizeof(real_packet->graphic));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
  dio_get_string(&din, real_packet->oceanic_graphic, sizeof(real_packet->oceanic_graphic));
  dio_get_string(&din, real_packet->oceanic_graphic_alt, sizeof(real_packet->oceanic_graphic_alt));
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->replaced_by = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_city_100(struct connection *pc, const struct packet_ruleset_city *packet)
{
  const struct packet_ruleset_city *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_CITY);

  dio_put_uint8(&dout, real_packet->style_id);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->citizens_graphic);
  dio_put_string(&dout, real_packet->citizens_graphic_alt);
  dio_put_uint8(&dout, real_packet->reqs_count);

    {
      int i;

      for (i = 0; i < real_packet->reqs_count; i++) {
        dio_put_requirement(&dout, &real_packet->reqs[i]);
      }
    } 
  dio_put_string(&dout, real_packet->graphic);
  dio_put_string(&dout, real_packet->graphic_alt);
  dio_put_string(&dout, real_packet->oceanic_graphic);
  dio_put_string(&dout, real_packet->oceanic_graphic_alt);
  dio_put_sint8(&dout, real_packet->replaced_by);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_city(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_CITY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_CITY] = variant;
}

struct packet_ruleset_city *receive_packet_ruleset_city(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_city at the server.");
  }
  ensure_valid_variant_packet_ruleset_city(pc);

  switch(pc->phs.variant[PACKET_RULESET_CITY]) {
    case 100: return receive_packet_ruleset_city_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_city(struct connection *pc, const struct packet_ruleset_city *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_city from the client.");
  }
  ensure_valid_variant_packet_ruleset_city(pc);

  switch(pc->phs.variant[PACKET_RULESET_CITY]) {
    case 100: return send_packet_ruleset_city_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_city(struct conn_list *dest, const struct packet_ruleset_city *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_city(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_building *receive_packet_ruleset_building_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_building, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->genus = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->build_cost = readin;
  }
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->reqs_count = readin;
  }
  
  {
    int i;
  
    if(real_packet->reqs_count > MAX_NUM_REQS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->reqs_count = MAX_NUM_REQS;
    }
    for (i = 0; i < real_packet->reqs_count; i++) {
      dio_get_requirement(&din, &real_packet->reqs[i]);
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->obsolete_by = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->replaced_by = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->upkeep = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->sabotage = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->flags = readin;
  }
  dio_get_string(&din, real_packet->soundtag, sizeof(real_packet->soundtag));
  dio_get_string(&din, real_packet->soundtag_alt, sizeof(real_packet->soundtag_alt));
  dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_building_100(struct connection *pc, const struct packet_ruleset_building *packet)
{
  const struct packet_ruleset_building *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_BUILDING);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_uint8(&dout, real_packet->genus);
  dio_put_string(&dout, real_packet->name);
  dio_put_uint32(&dout, real_packet->build_cost);
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);
  dio_put_uint32(&dout, real_packet->reqs_count);

    {
      int i;

      for (i = 0; i < real_packet->reqs_count; i++) {
        dio_put_requirement(&dout, &real_packet->reqs[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->obsolete_by);
  dio_put_uint8(&dout, real_packet->replaced_by);
  dio_put_uint32(&dout, real_packet->upkeep);
  dio_put_uint32(&dout, real_packet->sabotage);
  dio_put_uint32(&dout, real_packet->flags);
  dio_put_string(&dout, real_packet->soundtag);
  dio_put_string(&dout, real_packet->soundtag_alt);
  dio_put_string(&dout, real_packet->helptext);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_building(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_BUILDING] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_BUILDING] = variant;
}

struct packet_ruleset_building *receive_packet_ruleset_building(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_building at the server.");
  }
  ensure_valid_variant_packet_ruleset_building(pc);

  switch(pc->phs.variant[PACKET_RULESET_BUILDING]) {
    case 100: return receive_packet_ruleset_building_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_building(struct connection *pc, const struct packet_ruleset_building *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_building from the client.");
  }
  ensure_valid_variant_packet_ruleset_building(pc);

  switch(pc->phs.variant[PACKET_RULESET_BUILDING]) {
    case 100: return send_packet_ruleset_building_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_building(struct conn_list *dest, const struct packet_ruleset_building *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_building(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_terrain *receive_packet_ruleset_terrain_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_terrain, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  DIO_BV_GET(&din, real_packet->flags);
  DIO_BV_GET(&din, real_packet->native_to);
  dio_get_string(&din, real_packet->name_orig, sizeof(real_packet->name_orig));
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->movement_cost = readin;
  }
  {
    int readin;
  
    dio_get_sint16(&din, &readin);
    real_packet->defense_bonus = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->output[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_resources = readin;
  }
  
  {
    int i;
  
    if(real_packet->num_resources > MAX_NUM_RESOURCES) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_resources = MAX_NUM_RESOURCES;
    }
    for (i = 0; i < real_packet->num_resources; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->resources[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->road_trade_incr = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->road_time = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->irrigation_result = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->irrigation_food_incr = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->irrigation_time = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->mining_result = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->mining_shield_incr = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->mining_time = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->transform_result = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->transform_time = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->rail_time = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->clean_pollution_time = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->clean_fallout_time = readin;
  }
  dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_terrain_100(struct connection *pc, const struct packet_ruleset_terrain *packet)
{
  const struct packet_ruleset_terrain *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_TERRAIN);

  dio_put_uint8(&dout, real_packet->id);
DIO_BV_PUT(&dout, packet->flags);
DIO_BV_PUT(&dout, packet->native_to);
  dio_put_string(&dout, real_packet->name_orig);
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);
  dio_put_uint8(&dout, real_packet->movement_cost);
  dio_put_sint16(&dout, real_packet->defense_bonus);

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint8(&dout, real_packet->output[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->num_resources);

    {
      int i;

      for (i = 0; i < real_packet->num_resources; i++) {
        dio_put_uint8(&dout, real_packet->resources[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->road_trade_incr);
  dio_put_uint8(&dout, real_packet->road_time);
  dio_put_uint8(&dout, real_packet->irrigation_result);
  dio_put_uint8(&dout, real_packet->irrigation_food_incr);
  dio_put_uint8(&dout, real_packet->irrigation_time);
  dio_put_uint8(&dout, real_packet->mining_result);
  dio_put_uint8(&dout, real_packet->mining_shield_incr);
  dio_put_uint8(&dout, real_packet->mining_time);
  dio_put_uint8(&dout, real_packet->transform_result);
  dio_put_uint8(&dout, real_packet->transform_time);
  dio_put_uint8(&dout, real_packet->rail_time);
  dio_put_uint8(&dout, real_packet->clean_pollution_time);
  dio_put_uint8(&dout, real_packet->clean_fallout_time);
  dio_put_string(&dout, real_packet->helptext);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_terrain(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_TERRAIN] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_TERRAIN] = variant;
}

struct packet_ruleset_terrain *receive_packet_ruleset_terrain(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_terrain at the server.");
  }
  ensure_valid_variant_packet_ruleset_terrain(pc);

  switch(pc->phs.variant[PACKET_RULESET_TERRAIN]) {
    case 100: return receive_packet_ruleset_terrain_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_terrain(struct connection *pc, const struct packet_ruleset_terrain *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_terrain from the client.");
  }
  ensure_valid_variant_packet_ruleset_terrain(pc);

  switch(pc->phs.variant[PACKET_RULESET_TERRAIN]) {
    case 100: return send_packet_ruleset_terrain_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_terrain(struct conn_list *dest, const struct packet_ruleset_terrain *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_terrain(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_unit_class *receive_packet_ruleset_unit_class_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_unit_class, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->move_type = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->min_speed = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->hp_loss_pct = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->hut_behavior = readin;
  }
  DIO_BV_GET(&din, real_packet->flags);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_unit_class_100(struct connection *pc, const struct packet_ruleset_unit_class *packet)
{
  const struct packet_ruleset_unit_class *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_UNIT_CLASS);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name);
  dio_put_uint8(&dout, real_packet->move_type);
  dio_put_uint8(&dout, real_packet->min_speed);
  dio_put_uint8(&dout, real_packet->hp_loss_pct);
  dio_put_uint8(&dout, real_packet->hut_behavior);
DIO_BV_PUT(&dout, packet->flags);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_unit_class(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_UNIT_CLASS] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_UNIT_CLASS] = variant;
}

struct packet_ruleset_unit_class *receive_packet_ruleset_unit_class(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_unit_class at the server.");
  }
  ensure_valid_variant_packet_ruleset_unit_class(pc);

  switch(pc->phs.variant[PACKET_RULESET_UNIT_CLASS]) {
    case 100: return receive_packet_ruleset_unit_class_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_unit_class(struct connection *pc, const struct packet_ruleset_unit_class *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_unit_class from the client.");
  }
  ensure_valid_variant_packet_ruleset_unit_class(pc);

  switch(pc->phs.variant[PACKET_RULESET_UNIT_CLASS]) {
    case 100: return send_packet_ruleset_unit_class_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_unit_class(struct conn_list *dest, const struct packet_ruleset_unit_class *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_unit_class(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_base *receive_packet_ruleset_base_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_base, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_bool8(&din, &real_packet->buildable);
  dio_get_bool8(&din, &real_packet->pillageable);
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
  dio_get_string(&din, real_packet->activity_gfx, sizeof(real_packet->activity_gfx));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->reqs_count = readin;
  }
  
  {
    int i;
  
    if(real_packet->reqs_count > MAX_NUM_REQS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->reqs_count = MAX_NUM_REQS;
    }
    for (i = 0; i < real_packet->reqs_count; i++) {
      dio_get_requirement(&din, &real_packet->reqs[i]);
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->gui_type = readin;
  }
  DIO_BV_GET(&din, real_packet->native_to);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->build_time = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->defense_bonus = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->border_sq = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->vision_main_sq = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->vision_invis_sq = readin;
  }
  DIO_BV_GET(&din, real_packet->flags);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_base_100(struct connection *pc, const struct packet_ruleset_base *packet)
{
  const struct packet_ruleset_base *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_BASE);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name);
  dio_put_bool8(&dout, real_packet->buildable);
  dio_put_bool8(&dout, real_packet->pillageable);
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);
  dio_put_string(&dout, real_packet->activity_gfx);
  dio_put_uint8(&dout, real_packet->reqs_count);

    {
      int i;

      for (i = 0; i < real_packet->reqs_count; i++) {
        dio_put_requirement(&dout, &real_packet->reqs[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->gui_type);
DIO_BV_PUT(&dout, packet->native_to);
  dio_put_uint8(&dout, real_packet->build_time);
  dio_put_uint8(&dout, real_packet->defense_bonus);
  dio_put_uint8(&dout, real_packet->border_sq);
  dio_put_uint8(&dout, real_packet->vision_main_sq);
  dio_put_uint8(&dout, real_packet->vision_invis_sq);
DIO_BV_PUT(&dout, packet->flags);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_base(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_BASE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_BASE] = variant;
}

struct packet_ruleset_base *receive_packet_ruleset_base(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_base at the server.");
  }
  ensure_valid_variant_packet_ruleset_base(pc);

  switch(pc->phs.variant[PACKET_RULESET_BASE]) {
    case 100: return receive_packet_ruleset_base_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_base(struct connection *pc, const struct packet_ruleset_base *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_base from the client.");
  }
  ensure_valid_variant_packet_ruleset_base(pc);

  switch(pc->phs.variant[PACKET_RULESET_BASE]) {
    case 100: return send_packet_ruleset_base_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_base(struct conn_list *dest, const struct packet_ruleset_base *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_base(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_control *receive_packet_ruleset_control_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_control, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_unit_classes = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_unit_types = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_impr_types = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_tech_types = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_base_types = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->government_count = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->nation_count = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->styles_count = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->terrain_count = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->resource_count = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_specialist_types = readin;
  }
  dio_get_string(&din, real_packet->prefered_tileset, sizeof(real_packet->prefered_tileset));
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->description, sizeof(real_packet->description));

  post_receive_packet_ruleset_control(pc, real_packet);
  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_control_100(struct connection *pc, const struct packet_ruleset_control *packet)
{
  const struct packet_ruleset_control *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_CONTROL);

  dio_put_uint8(&dout, real_packet->num_unit_classes);
  dio_put_uint8(&dout, real_packet->num_unit_types);
  dio_put_uint8(&dout, real_packet->num_impr_types);
  dio_put_uint8(&dout, real_packet->num_tech_types);
  dio_put_uint8(&dout, real_packet->num_base_types);
  dio_put_uint8(&dout, real_packet->government_count);
  dio_put_uint8(&dout, real_packet->nation_count);
  dio_put_uint8(&dout, real_packet->styles_count);
  dio_put_uint8(&dout, real_packet->terrain_count);
  dio_put_uint8(&dout, real_packet->resource_count);
  dio_put_uint8(&dout, real_packet->num_specialist_types);
  dio_put_string(&dout, real_packet->prefered_tileset);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->description);

    post_send_packet_ruleset_control(pc, real_packet);
SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_control(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_CONTROL] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_CONTROL] = variant;
}

struct packet_ruleset_control *receive_packet_ruleset_control(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_control at the server.");
  }
  ensure_valid_variant_packet_ruleset_control(pc);

  switch(pc->phs.variant[PACKET_RULESET_CONTROL]) {
    case 100: return receive_packet_ruleset_control_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_control(struct connection *pc, const struct packet_ruleset_control *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_control from the client.");
  }
  ensure_valid_variant_packet_ruleset_control(pc);

  switch(pc->phs.variant[PACKET_RULESET_CONTROL]) {
    case 100: return send_packet_ruleset_control_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_control(struct conn_list *dest, const struct packet_ruleset_control *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_control(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_single_want_hack_req *receive_packet_single_want_hack_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_single_want_hack_req, real_packet);
  dio_get_string(&din, real_packet->token, sizeof(real_packet->token));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_single_want_hack_req_100(struct connection *pc, const struct packet_single_want_hack_req *packet)
{
  const struct packet_single_want_hack_req *real_packet = packet;
  SEND_PACKET_START(PACKET_SINGLE_WANT_HACK_REQ);

  dio_put_string(&dout, real_packet->token);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_single_want_hack_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SINGLE_WANT_HACK_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SINGLE_WANT_HACK_REQ] = variant;
}

struct packet_single_want_hack_req *receive_packet_single_want_hack_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_single_want_hack_req at the client.");
  }
  ensure_valid_variant_packet_single_want_hack_req(pc);

  switch(pc->phs.variant[PACKET_SINGLE_WANT_HACK_REQ]) {
    case 100: return receive_packet_single_want_hack_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_single_want_hack_req(struct connection *pc, const struct packet_single_want_hack_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_single_want_hack_req from the server.");
  }
  ensure_valid_variant_packet_single_want_hack_req(pc);

  switch(pc->phs.variant[PACKET_SINGLE_WANT_HACK_REQ]) {
    case 100: return send_packet_single_want_hack_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_single_want_hack_reply *receive_packet_single_want_hack_reply_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_single_want_hack_reply, real_packet);
  dio_get_bool8(&din, &real_packet->you_have_hack);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_single_want_hack_reply_100(struct connection *pc, const struct packet_single_want_hack_reply *packet)
{
  const struct packet_single_want_hack_reply *real_packet = packet;
  SEND_PACKET_START(PACKET_SINGLE_WANT_HACK_REPLY);

  dio_put_bool8(&dout, real_packet->you_have_hack);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_single_want_hack_reply(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SINGLE_WANT_HACK_REPLY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SINGLE_WANT_HACK_REPLY] = variant;
}

struct packet_single_want_hack_reply *receive_packet_single_want_hack_reply(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_single_want_hack_reply at the server.");
  }
  ensure_valid_variant_packet_single_want_hack_reply(pc);

  switch(pc->phs.variant[PACKET_SINGLE_WANT_HACK_REPLY]) {
    case 100: return receive_packet_single_want_hack_reply_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_single_want_hack_reply(struct connection *pc, const struct packet_single_want_hack_reply *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_single_want_hack_reply from the client.");
  }
  ensure_valid_variant_packet_single_want_hack_reply(pc);

  switch(pc->phs.variant[PACKET_SINGLE_WANT_HACK_REPLY]) {
    case 100: return send_packet_single_want_hack_reply_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_single_want_hack_reply(struct connection *pc, bool you_have_hack)
{
  struct packet_single_want_hack_reply packet, *real_packet = &packet;

  real_packet->you_have_hack = you_have_hack;
  
  return send_packet_single_want_hack_reply(pc, real_packet);
}

static struct packet_ruleset_choices *receive_packet_ruleset_choices_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_choices, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ruleset_count = readin;
  }
  
  {
    int i;
  
    if(real_packet->ruleset_count > MAX_NUM_RULESETS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->ruleset_count = MAX_NUM_RULESETS;
    }
    for (i = 0; i < real_packet->ruleset_count; i++) {
      dio_get_string(&din, real_packet->rulesets[i], sizeof(real_packet->rulesets[i]));
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_choices_100(struct connection *pc, const struct packet_ruleset_choices *packet)
{
  const struct packet_ruleset_choices *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_CHOICES);

  dio_put_uint8(&dout, real_packet->ruleset_count);

    {
      int i;

      for (i = 0; i < real_packet->ruleset_count; i++) {
        dio_put_string(&dout, real_packet->rulesets[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_choices(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_CHOICES] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_CHOICES] = variant;
}

struct packet_ruleset_choices *receive_packet_ruleset_choices(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_choices at the server.");
  }
  ensure_valid_variant_packet_ruleset_choices(pc);

  switch(pc->phs.variant[PACKET_RULESET_CHOICES]) {
    case 100: return receive_packet_ruleset_choices_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_choices(struct connection *pc, const struct packet_ruleset_choices *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_choices from the client.");
  }
  ensure_valid_variant_packet_ruleset_choices(pc);

  switch(pc->phs.variant[PACKET_RULESET_CHOICES]) {
    case 100: return send_packet_ruleset_choices_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_game_load *receive_packet_game_load_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_game_load, real_packet);
  dio_get_bool8(&din, &real_packet->load_successful);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->nplayers = readin;
  }
  dio_get_string(&din, real_packet->load_filename, sizeof(real_packet->load_filename));
  
  {
    int i;
  
    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->name[i], sizeof(real_packet->name[i]));
    }
  }
  
  {
    int i;
  
    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->username[i], sizeof(real_packet->username[i]));
    }
  }
  
  {
    int i;
  
    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nplayers; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nations[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_bool8(&din, &real_packet->is_alive[i]);
    }
  }
  
  {
    int i;
  
    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_bool8(&din, &real_packet->is_ai[i]);
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_game_load_100(struct connection *pc, const struct packet_game_load *packet)
{
  const struct packet_game_load *real_packet = packet;
  SEND_PACKET_START(PACKET_GAME_LOAD);

  dio_put_bool8(&dout, real_packet->load_successful);
  dio_put_sint8(&dout, real_packet->nplayers);
  dio_put_string(&dout, real_packet->load_filename);

    {
      int i;

      for (i = 0; i < real_packet->nplayers; i++) {
        dio_put_string(&dout, real_packet->name[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nplayers; i++) {
        dio_put_string(&dout, real_packet->username[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nplayers; i++) {
        dio_put_uint32(&dout, real_packet->nations[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nplayers; i++) {
        dio_put_bool8(&dout, real_packet->is_alive[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < real_packet->nplayers; i++) {
        dio_put_bool8(&dout, real_packet->is_ai[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_game_load(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_GAME_LOAD] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_GAME_LOAD] = variant;
}

struct packet_game_load *receive_packet_game_load(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_game_load at the server.");
  }
  ensure_valid_variant_packet_game_load(pc);

  switch(pc->phs.variant[PACKET_GAME_LOAD]) {
    case 100: return receive_packet_game_load_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_game_load(struct connection *pc, const struct packet_game_load *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_game_load from the client.");
  }
  ensure_valid_variant_packet_game_load(pc);

  switch(pc->phs.variant[PACKET_GAME_LOAD]) {
    case 100: return send_packet_game_load_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_game_load(struct conn_list *dest, const struct packet_game_load *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_game_load(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_options_settable_control *receive_packet_options_settable_control_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_options_settable_control, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->num_settings = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_categories = readin;
  }
  
  {
    int i;
  
    if(real_packet->num_categories > 256) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_categories = 256;
    }
    for (i = 0; i < real_packet->num_categories; i++) {
      dio_get_string(&din, real_packet->category_names[i], sizeof(real_packet->category_names[i]));
    }
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_options_settable_control_100(struct connection *pc, const struct packet_options_settable_control *packet)
{
  const struct packet_options_settable_control *real_packet = packet;
  SEND_PACKET_START(PACKET_OPTIONS_SETTABLE_CONTROL);

  dio_put_uint32(&dout, real_packet->num_settings);
  dio_put_uint8(&dout, real_packet->num_categories);

    {
      int i;

      for (i = 0; i < real_packet->num_categories; i++) {
        dio_put_string(&dout, real_packet->category_names[i]);
      }
    } 

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_options_settable_control(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_OPTIONS_SETTABLE_CONTROL] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_OPTIONS_SETTABLE_CONTROL] = variant;
}

struct packet_options_settable_control *receive_packet_options_settable_control(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_options_settable_control at the server.");
  }
  ensure_valid_variant_packet_options_settable_control(pc);

  switch(pc->phs.variant[PACKET_OPTIONS_SETTABLE_CONTROL]) {
    case 100: return receive_packet_options_settable_control_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_options_settable_control(struct connection *pc, const struct packet_options_settable_control *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_options_settable_control from the client.");
  }
  ensure_valid_variant_packet_options_settable_control(pc);

  switch(pc->phs.variant[PACKET_OPTIONS_SETTABLE_CONTROL]) {
    case 100: return send_packet_options_settable_control_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_options_settable_control(struct conn_list *dest, const struct packet_options_settable_control *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_options_settable_control(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_options_settable *receive_packet_options_settable_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_options_settable, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->short_help, sizeof(real_packet->short_help));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->stype = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->sclass = readin;
  }
  dio_get_bool8(&din, &real_packet->is_visible);
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->val = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->default_val = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->min = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->max = readin;
  }
  dio_get_string(&din, real_packet->strval, sizeof(real_packet->strval));
  dio_get_string(&din, real_packet->default_strval, sizeof(real_packet->default_strval));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->scategory = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_options_settable_100(struct connection *pc, const struct packet_options_settable *packet)
{
  const struct packet_options_settable *real_packet = packet;
  SEND_PACKET_START(PACKET_OPTIONS_SETTABLE);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->short_help);
  dio_put_uint8(&dout, real_packet->stype);
  dio_put_uint8(&dout, real_packet->sclass);
  dio_put_bool8(&dout, real_packet->is_visible);
  dio_put_sint32(&dout, real_packet->val);
  dio_put_sint32(&dout, real_packet->default_val);
  dio_put_sint32(&dout, real_packet->min);
  dio_put_sint32(&dout, real_packet->max);
  dio_put_string(&dout, real_packet->strval);
  dio_put_string(&dout, real_packet->default_strval);
  dio_put_uint8(&dout, real_packet->scategory);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_options_settable(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_OPTIONS_SETTABLE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_OPTIONS_SETTABLE] = variant;
}

struct packet_options_settable *receive_packet_options_settable(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_options_settable at the server.");
  }
  ensure_valid_variant_packet_options_settable(pc);

  switch(pc->phs.variant[PACKET_OPTIONS_SETTABLE]) {
    case 100: return receive_packet_options_settable_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_options_settable(struct connection *pc, const struct packet_options_settable *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_options_settable from the client.");
  }
  ensure_valid_variant_packet_options_settable(pc);

  switch(pc->phs.variant[PACKET_OPTIONS_SETTABLE]) {
    case 100: return send_packet_options_settable_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_options_settable(struct conn_list *dest, const struct packet_options_settable *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_options_settable(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_effect *receive_packet_ruleset_effect_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_effect, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->effect_type = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->effect_value = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_effect_100(struct connection *pc, const struct packet_ruleset_effect *packet)
{
  const struct packet_ruleset_effect *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_EFFECT);

  dio_put_uint8(&dout, real_packet->effect_type);
  dio_put_sint32(&dout, real_packet->effect_value);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_effect(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_EFFECT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_EFFECT] = variant;
}

struct packet_ruleset_effect *receive_packet_ruleset_effect(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_effect at the server.");
  }
  ensure_valid_variant_packet_ruleset_effect(pc);

  switch(pc->phs.variant[PACKET_RULESET_EFFECT]) {
    case 100: return receive_packet_ruleset_effect_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_effect(struct connection *pc, const struct packet_ruleset_effect *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_effect from the client.");
  }
  ensure_valid_variant_packet_ruleset_effect(pc);

  switch(pc->phs.variant[PACKET_RULESET_EFFECT]) {
    case 100: return send_packet_ruleset_effect_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_effect(struct conn_list *dest, const struct packet_ruleset_effect *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_effect(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_effect_req *receive_packet_ruleset_effect_req_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_effect_req, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->effect_id = readin;
  }
  dio_get_bool8(&din, &real_packet->neg);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->source_type = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->source_value = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->range = readin;
  }
  dio_get_bool8(&din, &real_packet->survives);
  dio_get_bool8(&din, &real_packet->negated);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_effect_req_100(struct connection *pc, const struct packet_ruleset_effect_req *packet)
{
  const struct packet_ruleset_effect_req *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_EFFECT_REQ);

  dio_put_uint32(&dout, real_packet->effect_id);
  dio_put_bool8(&dout, real_packet->neg);
  dio_put_uint8(&dout, real_packet->source_type);
  dio_put_sint32(&dout, real_packet->source_value);
  dio_put_uint8(&dout, real_packet->range);
  dio_put_bool8(&dout, real_packet->survives);
  dio_put_bool8(&dout, real_packet->negated);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_effect_req(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_EFFECT_REQ] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_EFFECT_REQ] = variant;
}

struct packet_ruleset_effect_req *receive_packet_ruleset_effect_req(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_effect_req at the server.");
  }
  ensure_valid_variant_packet_ruleset_effect_req(pc);

  switch(pc->phs.variant[PACKET_RULESET_EFFECT_REQ]) {
    case 100: return receive_packet_ruleset_effect_req_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_effect_req(struct connection *pc, const struct packet_ruleset_effect_req *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_effect_req from the client.");
  }
  ensure_valid_variant_packet_ruleset_effect_req(pc);

  switch(pc->phs.variant[PACKET_RULESET_EFFECT_REQ]) {
    case 100: return send_packet_ruleset_effect_req_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_effect_req(struct conn_list *dest, const struct packet_ruleset_effect_req *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_effect_req(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_ruleset_resource *receive_packet_ruleset_resource_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_ruleset_resource, real_packet);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name_orig, sizeof(real_packet->name_orig));
  
  {
    int i;
  
    for (i = 0; i < O_LAST; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->output[i] = readin;
  }
    }
  }
  dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
  dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_ruleset_resource_100(struct connection *pc, const struct packet_ruleset_resource *packet)
{
  const struct packet_ruleset_resource *real_packet = packet;
  SEND_PACKET_START(PACKET_RULESET_RESOURCE);

  dio_put_uint8(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name_orig);

    {
      int i;

      for (i = 0; i < O_LAST; i++) {
        dio_put_uint8(&dout, real_packet->output[i]);
      }
    } 
  dio_put_string(&dout, real_packet->graphic_str);
  dio_put_string(&dout, real_packet->graphic_alt);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_ruleset_resource(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_RULESET_RESOURCE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_RULESET_RESOURCE] = variant;
}

struct packet_ruleset_resource *receive_packet_ruleset_resource(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_resource at the server.");
  }
  ensure_valid_variant_packet_ruleset_resource(pc);

  switch(pc->phs.variant[PACKET_RULESET_RESOURCE]) {
    case 100: return receive_packet_ruleset_resource_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_ruleset_resource(struct connection *pc, const struct packet_ruleset_resource *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_ruleset_resource from the client.");
  }
  ensure_valid_variant_packet_ruleset_resource(pc);

  switch(pc->phs.variant[PACKET_RULESET_RESOURCE]) {
    case 100: return send_packet_ruleset_resource_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_ruleset_resource(struct conn_list *dest, const struct packet_ruleset_resource *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_ruleset_resource(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_scenario_info *receive_packet_scenario_info_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_scenario_info, real_packet);
  dio_get_bool8(&din, &real_packet->is_scenario);
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->description, sizeof(real_packet->description));
  dio_get_bool8(&din, &real_packet->players);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_scenario_info_100(struct connection *pc, const struct packet_scenario_info *packet)
{
  const struct packet_scenario_info *real_packet = packet;
  SEND_PACKET_START(PACKET_SCENARIO_INFO);

  dio_put_bool8(&dout, real_packet->is_scenario);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->description);
  dio_put_bool8(&dout, real_packet->players);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_scenario_info(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SCENARIO_INFO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SCENARIO_INFO] = variant;
}

struct packet_scenario_info *receive_packet_scenario_info(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  ensure_valid_variant_packet_scenario_info(pc);

  switch(pc->phs.variant[PACKET_SCENARIO_INFO]) {
    case 100: return receive_packet_scenario_info_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_scenario_info(struct connection *pc, const struct packet_scenario_info *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  ensure_valid_variant_packet_scenario_info(pc);

  switch(pc->phs.variant[PACKET_SCENARIO_INFO]) {
    case 100: return send_packet_scenario_info_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_save_scenario *receive_packet_save_scenario_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_save_scenario, real_packet);
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_save_scenario_100(struct connection *pc, const struct packet_save_scenario *packet)
{
  const struct packet_save_scenario *real_packet = packet;
  SEND_PACKET_START(PACKET_SAVE_SCENARIO);

  dio_put_string(&dout, real_packet->name);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_save_scenario(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_SAVE_SCENARIO] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_SAVE_SCENARIO] = variant;
}

struct packet_save_scenario *receive_packet_save_scenario(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_save_scenario at the client.");
  }
  ensure_valid_variant_packet_save_scenario(pc);

  switch(pc->phs.variant[PACKET_SAVE_SCENARIO]) {
    case 100: return receive_packet_save_scenario_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_save_scenario(struct connection *pc, const struct packet_save_scenario *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_save_scenario from the server.");
  }
  ensure_valid_variant_packet_save_scenario(pc);

  switch(pc->phs.variant[PACKET_SAVE_SCENARIO]) {
    case 100: return send_packet_save_scenario_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_save_scenario(struct connection *pc, const char *name)
{
  struct packet_save_scenario packet, *real_packet = &packet;

  sz_strlcpy(real_packet->name, name);
  
  return send_packet_save_scenario(pc, real_packet);
}

static struct packet_vote_new *receive_packet_vote_new_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_vote_new, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
  }
  dio_get_string(&din, real_packet->user, sizeof(real_packet->user));
  dio_get_string(&din, real_packet->desc, sizeof(real_packet->desc));
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->percent_required = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->flags = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_vote_new_100(struct connection *pc, const struct packet_vote_new *packet)
{
  const struct packet_vote_new *real_packet = packet;
  SEND_PACKET_START(PACKET_VOTE_NEW);

  dio_put_uint32(&dout, real_packet->vote_no);
  dio_put_string(&dout, real_packet->user);
  dio_put_string(&dout, real_packet->desc);
  dio_put_uint8(&dout, real_packet->percent_required);
  dio_put_uint32(&dout, real_packet->flags);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_vote_new(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_VOTE_NEW] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_VOTE_NEW] = variant;
}

struct packet_vote_new *receive_packet_vote_new(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_new at the server.");
  }
  ensure_valid_variant_packet_vote_new(pc);

  switch(pc->phs.variant[PACKET_VOTE_NEW]) {
    case 100: return receive_packet_vote_new_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_vote_new(struct connection *pc, const struct packet_vote_new *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_vote_new from the client.");
  }
  ensure_valid_variant_packet_vote_new(pc);

  switch(pc->phs.variant[PACKET_VOTE_NEW]) {
    case 100: return send_packet_vote_new_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_vote_update *receive_packet_vote_update_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_vote_update, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->yes = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->no = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->abstain = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->num_voters = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_vote_update_100(struct connection *pc, const struct packet_vote_update *packet)
{
  const struct packet_vote_update *real_packet = packet;
  SEND_PACKET_START(PACKET_VOTE_UPDATE);

  dio_put_uint32(&dout, real_packet->vote_no);
  dio_put_uint8(&dout, real_packet->yes);
  dio_put_uint8(&dout, real_packet->no);
  dio_put_uint8(&dout, real_packet->abstain);
  dio_put_uint8(&dout, real_packet->num_voters);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_vote_update(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_VOTE_UPDATE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_VOTE_UPDATE] = variant;
}

struct packet_vote_update *receive_packet_vote_update(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_update at the server.");
  }
  ensure_valid_variant_packet_vote_update(pc);

  switch(pc->phs.variant[PACKET_VOTE_UPDATE]) {
    case 100: return receive_packet_vote_update_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_vote_update(struct connection *pc, const struct packet_vote_update *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_vote_update from the client.");
  }
  ensure_valid_variant_packet_vote_update(pc);

  switch(pc->phs.variant[PACKET_VOTE_UPDATE]) {
    case 100: return send_packet_vote_update_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_vote_remove *receive_packet_vote_remove_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_vote_remove, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_vote_remove_100(struct connection *pc, const struct packet_vote_remove *packet)
{
  const struct packet_vote_remove *real_packet = packet;
  SEND_PACKET_START(PACKET_VOTE_REMOVE);

  dio_put_uint32(&dout, real_packet->vote_no);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_vote_remove(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_VOTE_REMOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_VOTE_REMOVE] = variant;
}

struct packet_vote_remove *receive_packet_vote_remove(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_remove at the server.");
  }
  ensure_valid_variant_packet_vote_remove(pc);

  switch(pc->phs.variant[PACKET_VOTE_REMOVE]) {
    case 100: return receive_packet_vote_remove_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_vote_remove(struct connection *pc, const struct packet_vote_remove *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_vote_remove from the client.");
  }
  ensure_valid_variant_packet_vote_remove(pc);

  switch(pc->phs.variant[PACKET_VOTE_REMOVE]) {
    case 100: return send_packet_vote_remove_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_vote_resolve *receive_packet_vote_resolve_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_vote_resolve, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
  }
  dio_get_bool8(&din, &real_packet->passed);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_vote_resolve_100(struct connection *pc, const struct packet_vote_resolve *packet)
{
  const struct packet_vote_resolve *real_packet = packet;
  SEND_PACKET_START(PACKET_VOTE_RESOLVE);

  dio_put_uint32(&dout, real_packet->vote_no);
  dio_put_bool8(&dout, real_packet->passed);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_vote_resolve(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_VOTE_RESOLVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_VOTE_RESOLVE] = variant;
}

struct packet_vote_resolve *receive_packet_vote_resolve(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_resolve at the server.");
  }
  ensure_valid_variant_packet_vote_resolve(pc);

  switch(pc->phs.variant[PACKET_VOTE_RESOLVE]) {
    case 100: return receive_packet_vote_resolve_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_vote_resolve(struct connection *pc, const struct packet_vote_resolve *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_vote_resolve from the client.");
  }
  ensure_valid_variant_packet_vote_resolve(pc);

  switch(pc->phs.variant[PACKET_VOTE_RESOLVE]) {
    case 100: return send_packet_vote_resolve_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_vote_submit *receive_packet_vote_submit_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_vote_submit, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->value = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_vote_submit_100(struct connection *pc, const struct packet_vote_submit *packet)
{
  const struct packet_vote_submit *real_packet = packet;
  SEND_PACKET_START(PACKET_VOTE_SUBMIT);

  dio_put_uint32(&dout, real_packet->vote_no);
  dio_put_sint8(&dout, real_packet->value);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_vote_submit(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_VOTE_SUBMIT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_VOTE_SUBMIT] = variant;
}

struct packet_vote_submit *receive_packet_vote_submit(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_submit at the client.");
  }
  ensure_valid_variant_packet_vote_submit(pc);

  switch(pc->phs.variant[PACKET_VOTE_SUBMIT]) {
    case 100: return receive_packet_vote_submit_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_vote_submit(struct connection *pc, const struct packet_vote_submit *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_vote_submit from the server.");
  }
  ensure_valid_variant_packet_vote_submit(pc);

  switch(pc->phs.variant[PACKET_VOTE_SUBMIT]) {
    case 100: return send_packet_vote_submit_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_edit_mode *receive_packet_edit_mode_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_mode, real_packet);
  dio_get_bool8(&din, &real_packet->state);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_mode_100(struct connection *pc, const struct packet_edit_mode *packet)
{
  const struct packet_edit_mode *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_MODE);

  dio_put_bool8(&dout, real_packet->state);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_mode(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_MODE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_MODE] = variant;
}

struct packet_edit_mode *receive_packet_edit_mode(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_mode at the client.");
  }
  ensure_valid_variant_packet_edit_mode(pc);

  switch(pc->phs.variant[PACKET_EDIT_MODE]) {
    case 100: return receive_packet_edit_mode_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_mode(struct connection *pc, const struct packet_edit_mode *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_mode from the server.");
  }
  ensure_valid_variant_packet_edit_mode(pc);

  switch(pc->phs.variant[PACKET_EDIT_MODE]) {
    case 100: return send_packet_edit_mode_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_mode(struct connection *pc, bool state)
{
  struct packet_edit_mode packet, *real_packet = &packet;

  real_packet->state = state;
  
  return send_packet_edit_mode(pc, real_packet);
}

static struct packet_edit_recalculate_borders *receive_packet_edit_recalculate_borders_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_recalculate_borders, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_recalculate_borders_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_EDIT_RECALCULATE_BORDERS);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_recalculate_borders(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_RECALCULATE_BORDERS] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_RECALCULATE_BORDERS] = variant;
}

struct packet_edit_recalculate_borders *receive_packet_edit_recalculate_borders(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_recalculate_borders at the client.");
  }
  ensure_valid_variant_packet_edit_recalculate_borders(pc);

  switch(pc->phs.variant[PACKET_EDIT_RECALCULATE_BORDERS]) {
    case 100: return receive_packet_edit_recalculate_borders_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_recalculate_borders(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_recalculate_borders from the server.");
  }
  ensure_valid_variant_packet_edit_recalculate_borders(pc);

  switch(pc->phs.variant[PACKET_EDIT_RECALCULATE_BORDERS]) {
    case 100: return send_packet_edit_recalculate_borders_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_edit_check_tiles *receive_packet_edit_check_tiles_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_check_tiles, real_packet);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_check_tiles_100(struct connection *pc)
{
  SEND_PACKET_START(PACKET_EDIT_CHECK_TILES);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_check_tiles(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_CHECK_TILES] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_CHECK_TILES] = variant;
}

struct packet_edit_check_tiles *receive_packet_edit_check_tiles(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_check_tiles at the client.");
  }
  ensure_valid_variant_packet_edit_check_tiles(pc);

  switch(pc->phs.variant[PACKET_EDIT_CHECK_TILES]) {
    case 100: return receive_packet_edit_check_tiles_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_check_tiles(struct connection *pc)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_check_tiles from the server.");
  }
  ensure_valid_variant_packet_edit_check_tiles(pc);

  switch(pc->phs.variant[PACKET_EDIT_CHECK_TILES]) {
    case 100: return send_packet_edit_check_tiles_100(pc);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_edit_toggle_fogofwar *receive_packet_edit_toggle_fogofwar_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_toggle_fogofwar, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->player = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_toggle_fogofwar_100(struct connection *pc, const struct packet_edit_toggle_fogofwar *packet)
{
  const struct packet_edit_toggle_fogofwar *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_TOGGLE_FOGOFWAR);

  dio_put_sint8(&dout, real_packet->player);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_toggle_fogofwar(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_TOGGLE_FOGOFWAR] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_TOGGLE_FOGOFWAR] = variant;
}

struct packet_edit_toggle_fogofwar *receive_packet_edit_toggle_fogofwar(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_toggle_fogofwar at the client.");
  }
  ensure_valid_variant_packet_edit_toggle_fogofwar(pc);

  switch(pc->phs.variant[PACKET_EDIT_TOGGLE_FOGOFWAR]) {
    case 100: return receive_packet_edit_toggle_fogofwar_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_toggle_fogofwar(struct connection *pc, const struct packet_edit_toggle_fogofwar *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_toggle_fogofwar from the server.");
  }
  ensure_valid_variant_packet_edit_toggle_fogofwar(pc);

  switch(pc->phs.variant[PACKET_EDIT_TOGGLE_FOGOFWAR]) {
    case 100: return send_packet_edit_toggle_fogofwar_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_toggle_fogofwar(struct connection *pc, int player)
{
  struct packet_edit_toggle_fogofwar packet, *real_packet = &packet;

  real_packet->player = player;
  
  return send_packet_edit_toggle_fogofwar(pc, real_packet);
}

static struct packet_edit_tile_terrain *receive_packet_edit_tile_terrain_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_tile_terrain, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->terrain = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_tile_terrain_100(struct connection *pc, const struct packet_edit_tile_terrain *packet)
{
  const struct packet_edit_tile_terrain *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_TILE_TERRAIN);

  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint8(&dout, real_packet->terrain);
  dio_put_uint8(&dout, real_packet->size);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_tile_terrain(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_TILE_TERRAIN] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_TILE_TERRAIN] = variant;
}

struct packet_edit_tile_terrain *receive_packet_edit_tile_terrain(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_tile_terrain at the client.");
  }
  ensure_valid_variant_packet_edit_tile_terrain(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_TERRAIN]) {
    case 100: return receive_packet_edit_tile_terrain_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_tile_terrain(struct connection *pc, const struct packet_edit_tile_terrain *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_tile_terrain from the server.");
  }
  ensure_valid_variant_packet_edit_tile_terrain(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_TERRAIN]) {
    case 100: return send_packet_edit_tile_terrain_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_tile_terrain(struct connection *pc, int x, int y, Terrain_type_id terrain, int size)
{
  struct packet_edit_tile_terrain packet, *real_packet = &packet;

  real_packet->x = x;
  real_packet->y = y;
  real_packet->terrain = terrain;
  real_packet->size = size;
  
  return send_packet_edit_tile_terrain(pc, real_packet);
}

static struct packet_edit_tile_resource *receive_packet_edit_tile_resource_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_tile_resource, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->resource = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_tile_resource_100(struct connection *pc, const struct packet_edit_tile_resource *packet)
{
  const struct packet_edit_tile_resource *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_TILE_RESOURCE);

  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint8(&dout, real_packet->resource);
  dio_put_uint8(&dout, real_packet->size);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_tile_resource(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_TILE_RESOURCE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_TILE_RESOURCE] = variant;
}

struct packet_edit_tile_resource *receive_packet_edit_tile_resource(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_tile_resource at the client.");
  }
  ensure_valid_variant_packet_edit_tile_resource(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_RESOURCE]) {
    case 100: return receive_packet_edit_tile_resource_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_tile_resource(struct connection *pc, const struct packet_edit_tile_resource *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_tile_resource from the server.");
  }
  ensure_valid_variant_packet_edit_tile_resource(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_RESOURCE]) {
    case 100: return send_packet_edit_tile_resource_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_tile_resource(struct connection *pc, int x, int y, Resource_type_id resource, int size)
{
  struct packet_edit_tile_resource packet, *real_packet = &packet;

  real_packet->x = x;
  real_packet->y = y;
  real_packet->resource = resource;
  real_packet->size = size;
  
  return send_packet_edit_tile_resource(pc, real_packet);
}

static struct packet_edit_tile_special *receive_packet_edit_tile_special_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_tile_special, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->special = readin;
  }
  dio_get_bool8(&din, &real_packet->remove);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_tile_special_100(struct connection *pc, const struct packet_edit_tile_special *packet)
{
  const struct packet_edit_tile_special *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_TILE_SPECIAL);

  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint32(&dout, real_packet->special);
  dio_put_bool8(&dout, real_packet->remove);
  dio_put_uint8(&dout, real_packet->size);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_tile_special(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_TILE_SPECIAL] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_TILE_SPECIAL] = variant;
}

struct packet_edit_tile_special *receive_packet_edit_tile_special(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_tile_special at the client.");
  }
  ensure_valid_variant_packet_edit_tile_special(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_SPECIAL]) {
    case 100: return receive_packet_edit_tile_special_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_tile_special(struct connection *pc, const struct packet_edit_tile_special *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_tile_special from the server.");
  }
  ensure_valid_variant_packet_edit_tile_special(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_SPECIAL]) {
    case 100: return send_packet_edit_tile_special_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_tile_special(struct connection *pc, int x, int y, enum tile_special_type special, bool remove, int size)
{
  struct packet_edit_tile_special packet, *real_packet = &packet;

  real_packet->x = x;
  real_packet->y = y;
  real_packet->special = special;
  real_packet->remove = remove;
  real_packet->size = size;
  
  return send_packet_edit_tile_special(pc, real_packet);
}

static struct packet_edit_tile_base *receive_packet_edit_tile_base_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_tile_base, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->base_type_id = readin;
  }
  dio_get_bool8(&din, &real_packet->remove);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_tile_base_100(struct connection *pc, const struct packet_edit_tile_base *packet)
{
  const struct packet_edit_tile_base *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_TILE_BASE);

  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint8(&dout, real_packet->base_type_id);
  dio_put_bool8(&dout, real_packet->remove);
  dio_put_uint8(&dout, real_packet->size);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_tile_base(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_TILE_BASE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_TILE_BASE] = variant;
}

struct packet_edit_tile_base *receive_packet_edit_tile_base(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_tile_base at the client.");
  }
  ensure_valid_variant_packet_edit_tile_base(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_BASE]) {
    case 100: return receive_packet_edit_tile_base_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_tile_base(struct connection *pc, const struct packet_edit_tile_base *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_tile_base from the server.");
  }
  ensure_valid_variant_packet_edit_tile_base(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE_BASE]) {
    case 100: return send_packet_edit_tile_base_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_tile_base(struct connection *pc, int x, int y, Base_type_id base_type_id, bool remove, int size)
{
  struct packet_edit_tile_base packet, *real_packet = &packet;

  real_packet->x = x;
  real_packet->y = y;
  real_packet->base_type_id = base_type_id;
  real_packet->remove = remove;
  real_packet->size = size;
  
  return send_packet_edit_tile_base(pc, real_packet);
}

static struct packet_edit_startpos *receive_packet_edit_startpos_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_startpos, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nation = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_startpos_100(struct connection *pc, const struct packet_edit_startpos *packet)
{
  const struct packet_edit_startpos *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_STARTPOS);

  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint32(&dout, real_packet->nation);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_startpos(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_STARTPOS] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_STARTPOS] = variant;
}

struct packet_edit_startpos *receive_packet_edit_startpos(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_startpos at the client.");
  }
  ensure_valid_variant_packet_edit_startpos(pc);

  switch(pc->phs.variant[PACKET_EDIT_STARTPOS]) {
    case 100: return receive_packet_edit_startpos_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_startpos(struct connection *pc, const struct packet_edit_startpos *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_startpos from the server.");
  }
  ensure_valid_variant_packet_edit_startpos(pc);

  switch(pc->phs.variant[PACKET_EDIT_STARTPOS]) {
    case 100: return send_packet_edit_startpos_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_startpos(struct connection *pc, int x, int y, int nation)
{
  struct packet_edit_startpos packet, *real_packet = &packet;

  real_packet->x = x;
  real_packet->y = y;
  real_packet->nation = nation;
  
  return send_packet_edit_startpos(pc, real_packet);
}

static struct packet_edit_tile *receive_packet_edit_tile_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_tile, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  DIO_BV_GET(&din, real_packet->specials);
  DIO_BV_GET(&din, real_packet->bases);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->resource = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->terrain = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->startpos_nation = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_tile_100(struct connection *pc, const struct packet_edit_tile *packet)
{
  const struct packet_edit_tile *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_TILE);

  dio_put_uint32(&dout, real_packet->id);
DIO_BV_PUT(&dout, packet->specials);
DIO_BV_PUT(&dout, packet->bases);
  dio_put_uint8(&dout, real_packet->resource);
  dio_put_uint8(&dout, real_packet->terrain);
  dio_put_uint32(&dout, real_packet->startpos_nation);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_tile(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_TILE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_TILE] = variant;
}

struct packet_edit_tile *receive_packet_edit_tile(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_tile at the client.");
  }
  ensure_valid_variant_packet_edit_tile(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE]) {
    case 100: return receive_packet_edit_tile_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_tile(struct connection *pc, const struct packet_edit_tile *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_tile from the server.");
  }
  ensure_valid_variant_packet_edit_tile(pc);

  switch(pc->phs.variant[PACKET_EDIT_TILE]) {
    case 100: return send_packet_edit_tile_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_edit_unit_create *receive_packet_edit_unit_create_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_unit_create, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->count = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->tag = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_unit_create_100(struct connection *pc, const struct packet_edit_unit_create *packet)
{
  const struct packet_edit_unit_create *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_UNIT_CREATE);

  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint8(&dout, real_packet->count);
  dio_put_sint32(&dout, real_packet->tag);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_unit_create(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_UNIT_CREATE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_UNIT_CREATE] = variant;
}

struct packet_edit_unit_create *receive_packet_edit_unit_create(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_unit_create at the client.");
  }
  ensure_valid_variant_packet_edit_unit_create(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT_CREATE]) {
    case 100: return receive_packet_edit_unit_create_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_unit_create(struct connection *pc, const struct packet_edit_unit_create *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_unit_create from the server.");
  }
  ensure_valid_variant_packet_edit_unit_create(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT_CREATE]) {
    case 100: return send_packet_edit_unit_create_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_unit_create(struct connection *pc, int owner, int x, int y, Unit_type_id type, int count, int tag)
{
  struct packet_edit_unit_create packet, *real_packet = &packet;

  real_packet->owner = owner;
  real_packet->x = x;
  real_packet->y = y;
  real_packet->type = type;
  real_packet->count = count;
  real_packet->tag = tag;
  
  return send_packet_edit_unit_create(pc, real_packet);
}

static struct packet_edit_unit_remove *receive_packet_edit_unit_remove_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_unit_remove, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->count = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_unit_remove_100(struct connection *pc, const struct packet_edit_unit_remove *packet)
{
  const struct packet_edit_unit_remove *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_UNIT_REMOVE);

  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint8(&dout, real_packet->type);
  dio_put_uint8(&dout, real_packet->count);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_unit_remove(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_UNIT_REMOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_UNIT_REMOVE] = variant;
}

struct packet_edit_unit_remove *receive_packet_edit_unit_remove(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_unit_remove at the client.");
  }
  ensure_valid_variant_packet_edit_unit_remove(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT_REMOVE]) {
    case 100: return receive_packet_edit_unit_remove_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_unit_remove(struct connection *pc, const struct packet_edit_unit_remove *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_unit_remove from the server.");
  }
  ensure_valid_variant_packet_edit_unit_remove(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT_REMOVE]) {
    case 100: return send_packet_edit_unit_remove_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_unit_remove(struct connection *pc, int owner, int x, int y, Unit_type_id type, int count)
{
  struct packet_edit_unit_remove packet, *real_packet = &packet;

  real_packet->owner = owner;
  real_packet->x = x;
  real_packet->y = y;
  real_packet->type = type;
  real_packet->count = count;
  
  return send_packet_edit_unit_remove(pc, real_packet);
}

static struct packet_edit_unit_remove_by_id *receive_packet_edit_unit_remove_by_id_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_unit_remove_by_id, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_unit_remove_by_id_100(struct connection *pc, const struct packet_edit_unit_remove_by_id *packet)
{
  const struct packet_edit_unit_remove_by_id *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_UNIT_REMOVE_BY_ID);

  dio_put_uint32(&dout, real_packet->id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_unit_remove_by_id(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_UNIT_REMOVE_BY_ID] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_UNIT_REMOVE_BY_ID] = variant;
}

struct packet_edit_unit_remove_by_id *receive_packet_edit_unit_remove_by_id(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_unit_remove_by_id at the client.");
  }
  ensure_valid_variant_packet_edit_unit_remove_by_id(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT_REMOVE_BY_ID]) {
    case 100: return receive_packet_edit_unit_remove_by_id_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_unit_remove_by_id(struct connection *pc, const struct packet_edit_unit_remove_by_id *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_unit_remove_by_id from the server.");
  }
  ensure_valid_variant_packet_edit_unit_remove_by_id(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT_REMOVE_BY_ID]) {
    case 100: return send_packet_edit_unit_remove_by_id_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_unit_remove_by_id(struct connection *pc, int id)
{
  struct packet_edit_unit_remove_by_id packet, *real_packet = &packet;

  real_packet->id = id;
  
  return send_packet_edit_unit_remove_by_id(pc, real_packet);
}

static struct packet_edit_unit *receive_packet_edit_unit_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_unit, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->utype = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->homecity = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->moves_left = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity_count = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->activity_base = readin;
  }
  dio_get_bool8(&din, &real_packet->debug);
  dio_get_bool8(&din, &real_packet->moved);
  dio_get_bool8(&din, &real_packet->paradropped);
  dio_get_bool8(&din, &real_packet->done_moving);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->transported_by = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_unit_100(struct connection *pc, const struct packet_edit_unit *packet)
{
  const struct packet_edit_unit *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_UNIT);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_uint8(&dout, real_packet->utype);
  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint32(&dout, real_packet->homecity);
  dio_put_uint8(&dout, real_packet->moves_left);
  dio_put_uint8(&dout, real_packet->hp);
  dio_put_uint8(&dout, real_packet->veteran);
  dio_put_uint8(&dout, real_packet->fuel);
  dio_put_uint8(&dout, real_packet->activity);
  dio_put_uint8(&dout, real_packet->activity_count);
  dio_put_uint8(&dout, real_packet->activity_base);
  dio_put_bool8(&dout, real_packet->debug);
  dio_put_bool8(&dout, real_packet->moved);
  dio_put_bool8(&dout, real_packet->paradropped);
  dio_put_bool8(&dout, real_packet->done_moving);
  dio_put_uint32(&dout, real_packet->transported_by);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_unit(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_UNIT] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_UNIT] = variant;
}

struct packet_edit_unit *receive_packet_edit_unit(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_unit at the client.");
  }
  ensure_valid_variant_packet_edit_unit(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT]) {
    case 100: return receive_packet_edit_unit_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_unit(struct connection *pc, const struct packet_edit_unit *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_unit from the server.");
  }
  ensure_valid_variant_packet_edit_unit(pc);

  switch(pc->phs.variant[PACKET_EDIT_UNIT]) {
    case 100: return send_packet_edit_unit_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_edit_city_create *receive_packet_edit_city_create_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_city_create, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->tag = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_city_create_100(struct connection *pc, const struct packet_edit_city_create *packet)
{
  const struct packet_edit_city_create *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_CITY_CREATE);

  dio_put_sint8(&dout, real_packet->owner);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_uint8(&dout, real_packet->size);
  dio_put_sint32(&dout, real_packet->tag);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_city_create(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_CITY_CREATE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_CITY_CREATE] = variant;
}

struct packet_edit_city_create *receive_packet_edit_city_create(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_city_create at the client.");
  }
  ensure_valid_variant_packet_edit_city_create(pc);

  switch(pc->phs.variant[PACKET_EDIT_CITY_CREATE]) {
    case 100: return receive_packet_edit_city_create_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_city_create(struct connection *pc, const struct packet_edit_city_create *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_city_create from the server.");
  }
  ensure_valid_variant_packet_edit_city_create(pc);

  switch(pc->phs.variant[PACKET_EDIT_CITY_CREATE]) {
    case 100: return send_packet_edit_city_create_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_city_create(struct connection *pc, int owner, int x, int y, int size, int tag)
{
  struct packet_edit_city_create packet, *real_packet = &packet;

  real_packet->owner = owner;
  real_packet->x = x;
  real_packet->y = y;
  real_packet->size = size;
  real_packet->tag = tag;
  
  return send_packet_edit_city_create(pc, real_packet);
}

static struct packet_edit_city_remove *receive_packet_edit_city_remove_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_city_remove, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_city_remove_100(struct connection *pc, const struct packet_edit_city_remove *packet)
{
  const struct packet_edit_city_remove *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_CITY_REMOVE);

  dio_put_uint32(&dout, real_packet->id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_city_remove(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_CITY_REMOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_CITY_REMOVE] = variant;
}

struct packet_edit_city_remove *receive_packet_edit_city_remove(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_city_remove at the client.");
  }
  ensure_valid_variant_packet_edit_city_remove(pc);

  switch(pc->phs.variant[PACKET_EDIT_CITY_REMOVE]) {
    case 100: return receive_packet_edit_city_remove_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_city_remove(struct connection *pc, const struct packet_edit_city_remove *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_city_remove from the server.");
  }
  ensure_valid_variant_packet_edit_city_remove(pc);

  switch(pc->phs.variant[PACKET_EDIT_CITY_REMOVE]) {
    case 100: return send_packet_edit_city_remove_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_city_remove(struct connection *pc, int id)
{
  struct packet_edit_city_remove packet, *real_packet = &packet;

  real_packet->id = id;
  
  return send_packet_edit_city_remove(pc, real_packet);
}

static struct packet_edit_city *receive_packet_edit_city_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_city, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->owner = readin;
  }
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->original = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_happy[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_content[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_unhappy[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < 5; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->ppl_angry[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->specialists_size = readin;
  }
  
  {
    int i;
  
    if(real_packet->specialists_size > SP_MAX) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->specialists_size = SP_MAX;
    }
    for (i = 0; i < real_packet->specialists_size; i++) {
      {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->specialists[i] = readin;
  }
    }
  }
  
  {
    int i;
  
    for (i = 0; i < NUM_TRADEROUTES; i++) {
      {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->trade[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->food_stock = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->shield_stock = readin;
  }
  dio_get_bool8(&din, &real_packet->airlift);
  dio_get_bool8(&din, &real_packet->debug);
  dio_get_bool8(&din, &real_packet->did_buy);
  dio_get_bool8(&din, &real_packet->did_sell);
  dio_get_bool8(&din, &real_packet->was_happy);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->anarchy = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->rapture = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->steal = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->turn_founded = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->turn_last_built = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < B_LAST; i++) {
      {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->built[i] = readin;
  }
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->production_kind = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->production_value = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->last_turns_shield_surplus = readin;
  }
  DIO_BV_GET(&din, real_packet->city_options);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_city_100(struct connection *pc, const struct packet_edit_city *packet)
{
  const struct packet_edit_city *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_CITY);

  dio_put_uint32(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name);
  dio_put_sint8(&dout, real_packet->owner);
  dio_put_sint8(&dout, real_packet->original);
  dio_put_uint8(&dout, real_packet->size);

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_happy[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_content[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_unhappy[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < 5; i++) {
        dio_put_uint8(&dout, real_packet->ppl_angry[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->specialists_size);

    {
      int i;

      for (i = 0; i < real_packet->specialists_size; i++) {
        dio_put_uint8(&dout, real_packet->specialists[i]);
      }
    } 

    {
      int i;

      for (i = 0; i < NUM_TRADEROUTES; i++) {
        dio_put_uint32(&dout, real_packet->trade[i]);
      }
    } 
  dio_put_uint32(&dout, real_packet->food_stock);
  dio_put_uint32(&dout, real_packet->shield_stock);
  dio_put_bool8(&dout, real_packet->airlift);
  dio_put_bool8(&dout, real_packet->debug);
  dio_put_bool8(&dout, real_packet->did_buy);
  dio_put_bool8(&dout, real_packet->did_sell);
  dio_put_bool8(&dout, real_packet->was_happy);
  dio_put_uint8(&dout, real_packet->anarchy);
  dio_put_uint8(&dout, real_packet->rapture);
  dio_put_uint8(&dout, real_packet->steal);
  dio_put_uint32(&dout, real_packet->turn_founded);
  dio_put_uint32(&dout, real_packet->turn_last_built);

    {
      int i;

      for (i = 0; i < B_LAST; i++) {
        dio_put_sint32(&dout, real_packet->built[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->production_kind);
  dio_put_uint8(&dout, real_packet->production_value);
  dio_put_uint32(&dout, real_packet->last_turns_shield_surplus);
DIO_BV_PUT(&dout, packet->city_options);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_city(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_CITY] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_CITY] = variant;
}

struct packet_edit_city *receive_packet_edit_city(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_city at the client.");
  }
  ensure_valid_variant_packet_edit_city(pc);

  switch(pc->phs.variant[PACKET_EDIT_CITY]) {
    case 100: return receive_packet_edit_city_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_city(struct connection *pc, const struct packet_edit_city *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_city from the server.");
  }
  ensure_valid_variant_packet_edit_city(pc);

  switch(pc->phs.variant[PACKET_EDIT_CITY]) {
    case 100: return send_packet_edit_city_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_edit_player_create *receive_packet_edit_player_create_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_player_create, real_packet);
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->tag = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_player_create_100(struct connection *pc, const struct packet_edit_player_create *packet)
{
  const struct packet_edit_player_create *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_PLAYER_CREATE);

  dio_put_sint32(&dout, real_packet->tag);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_player_create(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_PLAYER_CREATE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_PLAYER_CREATE] = variant;
}

struct packet_edit_player_create *receive_packet_edit_player_create(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_player_create at the client.");
  }
  ensure_valid_variant_packet_edit_player_create(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER_CREATE]) {
    case 100: return receive_packet_edit_player_create_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_player_create(struct connection *pc, const struct packet_edit_player_create *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_player_create from the server.");
  }
  ensure_valid_variant_packet_edit_player_create(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER_CREATE]) {
    case 100: return send_packet_edit_player_create_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_player_create(struct connection *pc, int tag)
{
  struct packet_edit_player_create packet, *real_packet = &packet;

  real_packet->tag = tag;
  
  return send_packet_edit_player_create(pc, real_packet);
}

static struct packet_edit_player_remove *receive_packet_edit_player_remove_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_player_remove, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_player_remove_100(struct connection *pc, const struct packet_edit_player_remove *packet)
{
  const struct packet_edit_player_remove *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_PLAYER_REMOVE);

  dio_put_sint8(&dout, real_packet->id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_player_remove(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_PLAYER_REMOVE] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_PLAYER_REMOVE] = variant;
}

struct packet_edit_player_remove *receive_packet_edit_player_remove(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_player_remove at the client.");
  }
  ensure_valid_variant_packet_edit_player_remove(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER_REMOVE]) {
    case 100: return receive_packet_edit_player_remove_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_player_remove(struct connection *pc, const struct packet_edit_player_remove *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_player_remove from the server.");
  }
  ensure_valid_variant_packet_edit_player_remove(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER_REMOVE]) {
    case 100: return send_packet_edit_player_remove_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_player_remove(struct connection *pc, int id)
{
  struct packet_edit_player_remove packet, *real_packet = &packet;

  real_packet->id = id;
  
  return send_packet_edit_player_remove(pc, real_packet);
}

static struct packet_edit_player *receive_packet_edit_player_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_player, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->id = readin;
  }
  dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
  dio_get_string(&din, real_packet->ranked_username, sizeof(real_packet->ranked_username));
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->user_turns = readin;
  }
  dio_get_bool8(&din, &real_packet->is_male);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nation = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
  }
  dio_get_bool8(&din, &real_packet->phase_done);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->nturns_idle = readin;
  }
  dio_get_bool8(&din, &real_packet->is_alive);
  dio_get_bool8(&din, &real_packet->surrendered);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->revolution_finishes = readin;
  }
  dio_get_bool8(&din, &real_packet->capital);
  DIO_BV_GET(&din, real_packet->embassy);
  
  {
    int i;
  
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
    }
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->tax = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->science = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->luxury = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->future_tech = readin;
  }
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
  }
  
  {
    int i;
  
    for (i = 0; i < A_LAST+1; i++) {
      dio_get_bool8(&din, &real_packet->inventions[i]);
    }
  }
  dio_get_bool8(&din, &real_packet->ai);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_player_100(struct connection *pc, const struct packet_edit_player *packet)
{
  const struct packet_edit_player *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_PLAYER);

  dio_put_sint8(&dout, real_packet->id);
  dio_put_string(&dout, real_packet->name);
  dio_put_string(&dout, real_packet->username);
  dio_put_string(&dout, real_packet->ranked_username);
  dio_put_uint32(&dout, real_packet->user_turns);
  dio_put_bool8(&dout, real_packet->is_male);
  dio_put_uint8(&dout, real_packet->government);
  dio_put_uint8(&dout, real_packet->target_government);
  dio_put_uint32(&dout, real_packet->nation);
  dio_put_uint8(&dout, real_packet->team);
  dio_put_bool8(&dout, real_packet->phase_done);
  dio_put_uint32(&dout, real_packet->nturns_idle);
  dio_put_bool8(&dout, real_packet->is_alive);
  dio_put_bool8(&dout, real_packet->surrendered);
  dio_put_uint32(&dout, real_packet->revolution_finishes);
  dio_put_bool8(&dout, real_packet->capital);
DIO_BV_PUT(&dout, packet->embassy);

    {
      int i;

      for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
        dio_put_diplstate(&dout, &real_packet->diplstates[i]);
      }
    } 
  dio_put_uint8(&dout, real_packet->city_style);
  dio_put_uint32(&dout, real_packet->gold);
  dio_put_uint32(&dout, real_packet->tax);
  dio_put_uint32(&dout, real_packet->science);
  dio_put_uint32(&dout, real_packet->luxury);
  dio_put_uint32(&dout, real_packet->future_tech);
  dio_put_uint8(&dout, real_packet->researching);
  dio_put_uint32(&dout, real_packet->bulbs_researched);

    {
      int i;

      for (i = 0; i < A_LAST+1; i++) {
        dio_put_bool8(&dout, real_packet->inventions[i]);
      }
    } 
  dio_put_bool8(&dout, real_packet->ai);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_player(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_PLAYER] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_PLAYER] = variant;
}

struct packet_edit_player *receive_packet_edit_player(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_player at the client.");
  }
  ensure_valid_variant_packet_edit_player(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER]) {
    case 100: return receive_packet_edit_player_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_player(struct connection *pc, const struct packet_edit_player *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_player from the server.");
  }
  ensure_valid_variant_packet_edit_player(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER]) {
    case 100: return send_packet_edit_player_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

void lsend_packet_edit_player(struct conn_list *dest, const struct packet_edit_player *packet)
{
  conn_list_iterate(dest, pconn) {
    send_packet_edit_player(pconn, packet);
  } conn_list_iterate_end;
}

static struct packet_edit_player_vision *receive_packet_edit_player_vision_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_player_vision, real_packet);
  {
    int readin;
  
    dio_get_sint8(&din, &readin);
    real_packet->player = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->x = readin;
  }
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->y = readin;
  }
  dio_get_bool8(&din, &real_packet->known);
  {
    int readin;
  
    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_player_vision_100(struct connection *pc, const struct packet_edit_player_vision *packet)
{
  const struct packet_edit_player_vision *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_PLAYER_VISION);

  dio_put_sint8(&dout, real_packet->player);
  dio_put_uint32(&dout, real_packet->x);
  dio_put_uint32(&dout, real_packet->y);
  dio_put_bool8(&dout, real_packet->known);
  dio_put_uint8(&dout, real_packet->size);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_player_vision(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_PLAYER_VISION] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_PLAYER_VISION] = variant;
}

struct packet_edit_player_vision *receive_packet_edit_player_vision(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_player_vision at the client.");
  }
  ensure_valid_variant_packet_edit_player_vision(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER_VISION]) {
    case 100: return receive_packet_edit_player_vision_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_player_vision(struct connection *pc, const struct packet_edit_player_vision *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_player_vision from the server.");
  }
  ensure_valid_variant_packet_edit_player_vision(pc);

  switch(pc->phs.variant[PACKET_EDIT_PLAYER_VISION]) {
    case 100: return send_packet_edit_player_vision_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_player_vision(struct connection *pc, int player, int x, int y, bool known, int size)
{
  struct packet_edit_player_vision packet, *real_packet = &packet;

  real_packet->player = player;
  real_packet->x = x;
  real_packet->y = y;
  real_packet->known = known;
  real_packet->size = size;
  
  return send_packet_edit_player_vision(pc, real_packet);
}

static struct packet_edit_game *receive_packet_edit_game_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_game, real_packet);
  {
    int readin;
  
    dio_get_uint32(&din, &readin);
    real_packet->year = readin;
  }
  dio_get_bool8(&din, &real_packet->scenario);
  dio_get_string(&din, real_packet->scenario_name, sizeof(real_packet->scenario_name));
  dio_get_string(&din, real_packet->scenario_desc, sizeof(real_packet->scenario_desc));
  dio_get_bool8(&din, &real_packet->scenario_players);

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_game_100(struct connection *pc, const struct packet_edit_game *packet)
{
  const struct packet_edit_game *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_GAME);

  dio_put_uint32(&dout, real_packet->year);
  dio_put_bool8(&dout, real_packet->scenario);
  dio_put_string(&dout, real_packet->scenario_name);
  dio_put_string(&dout, real_packet->scenario_desc);
  dio_put_bool8(&dout, real_packet->scenario_players);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_game(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_GAME] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_GAME] = variant;
}

struct packet_edit_game *receive_packet_edit_game(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_game at the client.");
  }
  ensure_valid_variant_packet_edit_game(pc);

  switch(pc->phs.variant[PACKET_EDIT_GAME]) {
    case 100: return receive_packet_edit_game_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_game(struct connection *pc, const struct packet_edit_game *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_game from the server.");
  }
  ensure_valid_variant_packet_edit_game(pc);

  switch(pc->phs.variant[PACKET_EDIT_GAME]) {
    case 100: return send_packet_edit_game_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

static struct packet_edit_object_created *receive_packet_edit_object_created_100(struct connection *pc, enum packet_type type)
{
  RECEIVE_PACKET_START(packet_edit_object_created, real_packet);
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->tag = readin;
  }
  {
    int readin;
  
    dio_get_sint32(&din, &readin);
    real_packet->id = readin;
  }

  RECEIVE_PACKET_END(real_packet);
}

static int send_packet_edit_object_created_100(struct connection *pc, const struct packet_edit_object_created *packet)
{
  const struct packet_edit_object_created *real_packet = packet;
  SEND_PACKET_START(PACKET_EDIT_OBJECT_CREATED);

  dio_put_sint32(&dout, real_packet->tag);
  dio_put_sint32(&dout, real_packet->id);

  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_edit_object_created(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[PACKET_EDIT_OBJECT_CREATED] != -1) {
    return;
  }

  if(FALSE) {
  } else if(TRUE) {
    variant = 100;
  } else {
    die("unknown variant");
  }
  pc->phs.variant[PACKET_EDIT_OBJECT_CREATED] = variant;
}

struct packet_edit_object_created *receive_packet_edit_object_created(struct connection *pc, enum packet_type type)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
  if (pc->is_server) {
    freelog(LOG_ERROR, "Receiving packet_edit_object_created at the server.");
  }
  ensure_valid_variant_packet_edit_object_created(pc);

  switch(pc->phs.variant[PACKET_EDIT_OBJECT_CREATED]) {
    case 100: return receive_packet_edit_object_created_100(pc, type);
    default: die("unknown variant"); return NULL;
  }
}

int send_packet_edit_object_created(struct connection *pc, const struct packet_edit_object_created *packet)
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
  if (!pc->is_server) {
    freelog(LOG_ERROR, "Sending packet_edit_object_created from the client.");
  }
  ensure_valid_variant_packet_edit_object_created(pc);

  switch(pc->phs.variant[PACKET_EDIT_OBJECT_CREATED]) {
    case 100: return send_packet_edit_object_created_100(pc, packet);
    default: die("unknown variant"); return -1;
  }
}

int dsend_packet_edit_object_created(struct connection *pc, int tag, int id)
{
  struct packet_edit_object_created packet, *real_packet = &packet;

  real_packet->tag = tag;
  real_packet->id = id;
  
  return send_packet_edit_object_created(pc, real_packet);
}

