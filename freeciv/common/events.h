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
#ifndef FC__EVENTS_H
#define FC__EVENTS_H

#include "shared.h"          /* bool type */

/* Add new event types to the end. Client saves message settings by
 * type number and installing new event type in between would cause
 * erronous loading of existing .civclientrc
 * When adding events to stable branch, there is risk that TRUNK
 * already has allocated next slot for something else (and has
 * new event in upper slot) */
enum event_type {
  E_CITY_CANTBUILD,
  E_CITY_LOST,
  E_CITY_LOVE,
  E_CITY_DISORDER,
  E_CITY_FAMINE,
  E_CITY_FAMINE_FEARED,
  E_CITY_GROWTH,
  E_CITY_MAY_SOON_GROW,
  E_CITY_AQUEDUCT,
  E_CITY_AQ_BUILDING,
  E_CITY_NORMAL,
  E_CITY_NUKED,
  E_CITY_CMA_RELEASE,
  E_CITY_GRAN_THROTTLE,
  E_CITY_TRANSFER,
  E_CITY_BUILD,
  E_CITY_PRODUCTION_CHANGED,
  E_WORKLIST,
  E_UPRISING,
  E_CIVIL_WAR,
  E_ANARCHY,
  E_FIRST_CONTACT,
  E_NEW_GOVERNMENT,
  E_LOW_ON_FUNDS,
  E_POLLUTION,
  E_REVOLT_DONE,
  E_REVOLT_START,
  E_SPACESHIP,
  E_MY_DIPLOMAT_BRIBE,
  E_DIPLOMATIC_INCIDENT,
  E_MY_DIPLOMAT_ESCAPE,
  E_MY_DIPLOMAT_EMBASSY,
  E_MY_DIPLOMAT_FAILED,
  E_MY_DIPLOMAT_INCITE,
  E_MY_DIPLOMAT_POISON,
  E_MY_DIPLOMAT_SABOTAGE,
  E_MY_DIPLOMAT_THEFT,
  E_ENEMY_DIPLOMAT_BRIBE,
  E_ENEMY_DIPLOMAT_EMBASSY,
  E_ENEMY_DIPLOMAT_FAILED,
  E_ENEMY_DIPLOMAT_INCITE,
  E_ENEMY_DIPLOMAT_POISON,
  E_ENEMY_DIPLOMAT_SABOTAGE,
  E_ENEMY_DIPLOMAT_THEFT,
  E_CARAVAN_ACTION,
  E_SCRIPT,
  E_BROADCAST_REPORT,
  E_GAME_END,
  E_GAME_START,
  E_NATION_SELECTED,
  E_DESTROYED,
  E_REPORT,
  E_TURN_BELL,
  E_NEXT_YEAR,
  E_GLOBAL_ECO,
  E_NUKE,
  E_HUT_BARB,
  E_HUT_CITY,
  E_HUT_GOLD,
  E_HUT_BARB_KILLED,
  E_HUT_MERC,
  E_HUT_SETTLER,
  E_HUT_TECH,
  E_HUT_BARB_CITY_NEAR,
  E_IMP_BUY,
  E_IMP_BUILD,
  E_IMP_AUCTIONED,
  E_IMP_AUTO,
  E_IMP_SOLD,
  E_TECH_GAIN,
  E_TECH_LEARNED,
  E_TREATY_ALLIANCE,
  E_TREATY_BROKEN,
  E_TREATY_CEASEFIRE,
  E_TREATY_PEACE,
  E_TREATY_SHARED_VISION,
  E_UNIT_LOST_ATT,
  E_UNIT_WIN_ATT,
  E_UNIT_BUY,
  E_UNIT_BUILT,
  E_UNIT_LOST_DEF,
  E_UNIT_WIN,
  E_UNIT_BECAME_VET,
  E_UNIT_UPGRADED,
  E_UNIT_RELOCATED,
  E_UNIT_ORDERS,
  E_WONDER_BUILD,
  E_WONDER_OBSOLETE,
  E_WONDER_STARTED,
  E_WONDER_STOPPED,
  E_WONDER_WILL_BE_BUILT,
  E_DIPLOMACY,
  E_TREATY_EMBASSY,
  E_BAD_COMMAND,		/* Illegal command sent from client. */
  E_SETTING,			/* Messages for changed server settings */
  E_CHAT_MSG,			/* Chatline messages */
  E_MESSAGE_WALL,		/* Message from server operator */
  E_CHAT_ERROR,			/* Chatline errors (bad syntax, etc.) */
  E_CONNECTION,			/* Messages about acquired or lost connections */
  E_AI_DEBUG,			/* AI debugging messages */
  E_LOG_ERROR,			/* Warning messages */
  E_LOG_FATAL,
  E_TECH_GOAL,			/* Changed tech goal */
  E_UNIT_LOST_MISC,             /* Non-battle unit deaths */
  E_CITY_PLAGUE,		/* Plague within a city */
  E_VOTE_NEW,
  E_VOTE_RESOLVED,
  E_VOTE_ABORTED,
  /* 
   * Note: If you add a new event, make sure you make a similar change
   * to the events array in common/events.c using GEN_EV,
   * data/stdsoundes and to server/scripting/api.pkg
   */
  E_LAST
};

extern enum event_type sorted_events[]; /* [E_LAST], sorted by the
					   translated message text */

const char *get_event_message_text(enum event_type event);
const char *get_event_sound_tag(enum event_type event);

bool is_city_event(enum event_type event);

void events_init(void);
void events_free(void);


/* Iterates over all events, sorted by the message text string. */
#define sorted_event_iterate(event)                                           \
{                                                                             \
  enum event_type event;                                                      \
  enum event_type event##_index;                                              \
  for (event##_index = 0;                                                     \
       event##_index < E_LAST;                                                \
       event##_index++) {                                                     \
    event = sorted_events[event##_index];                                     \
    {

#define sorted_event_iterate_end                                              \
    }                                                                         \
  }                                                                           \
}

#endif /* FC__EVENTS_H */
