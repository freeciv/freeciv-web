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
#ifndef FC__EVENTS_H
#define FC__EVENTS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "support.h"            /* bool type */

/* Used in the network protocol. */
#define SPECENUM_NAME event_type
#define SPECENUM_VALUE0   E_CITY_CANTBUILD
#define SPECENUM_VALUE1   E_CITY_LOST
#define SPECENUM_VALUE2   E_CITY_LOVE
#define SPECENUM_VALUE3   E_CITY_DISORDER
#define SPECENUM_VALUE4   E_CITY_FAMINE
#define SPECENUM_VALUE5   E_CITY_FAMINE_FEARED
#define SPECENUM_VALUE6   E_CITY_GROWTH
#define SPECENUM_VALUE7   E_CITY_MAY_SOON_GROW
#define SPECENUM_VALUE8   E_CITY_AQUEDUCT
#define SPECENUM_VALUE9   E_CITY_AQ_BUILDING
#define SPECENUM_VALUE10  E_CITY_NORMAL
#define SPECENUM_VALUE11  E_CITY_NUKED
#define SPECENUM_VALUE12  E_CITY_CMA_RELEASE
#define SPECENUM_VALUE13  E_CITY_GRAN_THROTTLE
#define SPECENUM_VALUE14  E_CITY_TRANSFER
#define SPECENUM_VALUE15  E_CITY_BUILD
#define SPECENUM_VALUE16  E_CITY_PRODUCTION_CHANGED
#define SPECENUM_VALUE17  E_WORKLIST
#define SPECENUM_VALUE18  E_UPRISING
#define SPECENUM_VALUE19  E_CIVIL_WAR
#define SPECENUM_VALUE20  E_ANARCHY
#define SPECENUM_VALUE21  E_FIRST_CONTACT
#define SPECENUM_VALUE22  E_NEW_GOVERNMENT
#define SPECENUM_VALUE23  E_LOW_ON_FUNDS
#define SPECENUM_VALUE24  E_POLLUTION
#define SPECENUM_VALUE25  E_REVOLT_DONE
#define SPECENUM_VALUE26  E_REVOLT_START
#define SPECENUM_VALUE27  E_SPACESHIP
#define SPECENUM_VALUE28  E_MY_DIPLOMAT_BRIBE
#define SPECENUM_VALUE29  E_DIPLOMATIC_INCIDENT
#define SPECENUM_VALUE30  E_MY_DIPLOMAT_ESCAPE
#define SPECENUM_VALUE31  E_MY_DIPLOMAT_EMBASSY
#define SPECENUM_VALUE32  E_MY_DIPLOMAT_FAILED
#define SPECENUM_VALUE33  E_MY_DIPLOMAT_INCITE
#define SPECENUM_VALUE34  E_MY_DIPLOMAT_POISON
#define SPECENUM_VALUE35  E_MY_DIPLOMAT_SABOTAGE
#define SPECENUM_VALUE36  E_MY_DIPLOMAT_THEFT
#define SPECENUM_VALUE37  E_ENEMY_DIPLOMAT_BRIBE
#define SPECENUM_VALUE38  E_ENEMY_DIPLOMAT_EMBASSY
#define SPECENUM_VALUE39  E_ENEMY_DIPLOMAT_FAILED
#define SPECENUM_VALUE40  E_ENEMY_DIPLOMAT_INCITE
#define SPECENUM_VALUE41  E_ENEMY_DIPLOMAT_POISON
#define SPECENUM_VALUE42  E_ENEMY_DIPLOMAT_SABOTAGE
#define SPECENUM_VALUE43  E_ENEMY_DIPLOMAT_THEFT
#define SPECENUM_VALUE44  E_CARAVAN_ACTION
#define SPECENUM_VALUE45  E_SCRIPT
#define SPECENUM_VALUE46  E_BROADCAST_REPORT
#define SPECENUM_VALUE47  E_GAME_END
#define SPECENUM_VALUE48  E_GAME_START
#define SPECENUM_VALUE49  E_NATION_SELECTED
#define SPECENUM_VALUE50  E_DESTROYED
#define SPECENUM_VALUE51  E_REPORT
#define SPECENUM_VALUE52  E_TURN_BELL
#define SPECENUM_VALUE53  E_NEXT_YEAR
#define SPECENUM_VALUE54  E_GLOBAL_ECO
#define SPECENUM_VALUE55  E_NUKE
#define SPECENUM_VALUE56  E_HUT_BARB
#define SPECENUM_VALUE57  E_HUT_CITY
#define SPECENUM_VALUE58  E_HUT_GOLD
#define SPECENUM_VALUE59  E_HUT_BARB_KILLED
#define SPECENUM_VALUE60  E_HUT_MERC
#define SPECENUM_VALUE61  E_HUT_SETTLER
#define SPECENUM_VALUE62  E_HUT_TECH
#define SPECENUM_VALUE63  E_HUT_BARB_CITY_NEAR
#define SPECENUM_VALUE64  E_IMP_BUY
#define SPECENUM_VALUE65  E_IMP_BUILD
#define SPECENUM_VALUE66  E_IMP_AUCTIONED
#define SPECENUM_VALUE67  E_IMP_AUTO
#define SPECENUM_VALUE68  E_IMP_SOLD
#define SPECENUM_VALUE69  E_TECH_GAIN
#define SPECENUM_VALUE70  E_TECH_LEARNED
#define SPECENUM_VALUE71  E_TREATY_ALLIANCE
#define SPECENUM_VALUE72  E_TREATY_BROKEN
#define SPECENUM_VALUE73  E_TREATY_CEASEFIRE
#define SPECENUM_VALUE74  E_TREATY_PEACE
#define SPECENUM_VALUE75  E_TREATY_SHARED_VISION
#define SPECENUM_VALUE76  E_UNIT_LOST_ATT
#define SPECENUM_VALUE77  E_UNIT_WIN_ATT
#define SPECENUM_VALUE78  E_UNIT_BUY
#define SPECENUM_VALUE79  E_UNIT_BUILT
#define SPECENUM_VALUE80  E_UNIT_LOST_DEF
#define SPECENUM_VALUE81  E_UNIT_WIN_DEF
#define SPECENUM_VALUE82  E_UNIT_BECAME_VET
#define SPECENUM_VALUE83  E_UNIT_UPGRADED
#define SPECENUM_VALUE84  E_UNIT_RELOCATED
#define SPECENUM_VALUE85  E_UNIT_ORDERS
#define SPECENUM_VALUE86  E_WONDER_BUILD
#define SPECENUM_VALUE87  E_WONDER_OBSOLETE
#define SPECENUM_VALUE88  E_WONDER_STARTED
#define SPECENUM_VALUE89  E_WONDER_STOPPED
#define SPECENUM_VALUE90  E_WONDER_WILL_BE_BUILT
#define SPECENUM_VALUE91  E_DIPLOMACY
#define SPECENUM_VALUE92  E_TREATY_EMBASSY
/* Illegal command sent from client. */
#define SPECENUM_VALUE93  E_BAD_COMMAND
/* Messages for changed server settings */
#define SPECENUM_VALUE94  E_SETTING
/* Chatline messages */
#define SPECENUM_VALUE95  E_CHAT_MSG
/* Message from server operator */
#define SPECENUM_VALUE96  E_MESSAGE_WALL
/* Chatline errors (bad syntax, etc.) */
#define SPECENUM_VALUE97  E_CHAT_ERROR
/* Messages about acquired or lost connections */
#define SPECENUM_VALUE98  E_CONNECTION
/* AI debugging messages */
#define SPECENUM_VALUE99  E_AI_DEBUG
/* Warning messages */
#define SPECENUM_VALUE100 E_LOG_ERROR
#define SPECENUM_VALUE101 E_LOG_FATAL
/* Changed tech goal */
#define SPECENUM_VALUE102 E_TECH_GOAL
/* Non-battle unit deaths */
#define SPECENUM_VALUE103 E_UNIT_LOST_MISC
/* Plague within a city */
#define SPECENUM_VALUE104 E_CITY_PLAGUE
#define SPECENUM_VALUE105 E_VOTE_NEW
#define SPECENUM_VALUE106 E_VOTE_RESOLVED
#define SPECENUM_VALUE107 E_VOTE_ABORTED
/* Change of the city radius */
#define SPECENUM_VALUE108 E_CITY_RADIUS_SQ
/* A unit with population cost was built; the city shrinks. */
#define SPECENUM_VALUE109 E_UNIT_BUILT_POP_COST
#define SPECENUM_VALUE110 E_DISASTER
#define SPECENUM_VALUE111 E_ACHIEVEMENT
#define SPECENUM_VALUE112 E_TECH_LOST
/* Used to notify tech events for foreigner players. */
#define SPECENUM_VALUE113 E_TECH_EMBASSY
#define SPECENUM_VALUE114 E_MY_SPY_STEAL_GOLD
#define SPECENUM_VALUE115 E_ENEMY_SPY_STEAL_GOLD
#define SPECENUM_VALUE116 E_SPONTANEOUS_EXTRA
#define SPECENUM_VALUE117 E_UNIT_ILLEGAL_ACTION
#define SPECENUM_VALUE118 E_MY_SPY_STEAL_MAP
#define SPECENUM_VALUE119 E_ENEMY_SPY_STEAL_MAP
#define SPECENUM_VALUE120 E_MY_SPY_NUKE
#define SPECENUM_VALUE121 E_ENEMY_SPY_NUKE
#define SPECENUM_VALUE122 E_UNIT_WAS_EXPELLED
#define SPECENUM_VALUE123 E_UNIT_DID_EXPEL
#define SPECENUM_VALUE124 E_UNIT_ACTION_FAILED
#define SPECENUM_VALUE125 E_UNIT_ESCAPED
#define SPECENUM_VALUE126 E_DEPRECATION_WARNING
/* Used for messages about things experienced players already know. */
#define SPECENUM_VALUE127 E_BEGINNER_HELP
#define SPECENUM_VALUE128 E_MY_UNIT_DID_HEAL
#define SPECENUM_VALUE129 E_MY_UNIT_WAS_HEALED
/*
 * Note: If you add a new event, make sure you make a similar change
 * to the events array in "common/events.c" using GEN_EV, to
 * "data/stdsounds.soundspec", which serves as the documentation to
 * soundset authors, and to "data/misc/events.spec".
 */
#define SPECENUM_COUNT E_COUNT
/* The sound system also generates "e_game_quit", although there's no
 * corresponding identifier E_GAME_QUIT. */
#include "specenum_gen.h"
/* the maximum number of enumerators is set in generate_specnum.py */

extern enum event_type sorted_events[]; /* [E_COUNT], sorted by the
                                           translated message text */

const char *get_event_message_text(enum event_type event);
const char *get_event_tag(enum event_type event);

bool is_city_event(enum event_type event);

void events_init(void);
void events_free(void);


/* Iterates over all events, sorted by the message text string. */
#define sorted_event_iterate(event)                                          \
{                                                                            \
  enum event_type event;                                                     \
  enum event_type event##_index;                                             \
  for (event##_index = event_type_begin();                                   \
       event##_index != event_type_end();                                    \
       event##_index = event_type_next(event##_index)) {                     \
    event = sorted_events[event##_index];                                    \
    {

#define sorted_event_iterate_end                                             \
    }                                                                        \
  }                                                                          \
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__EVENTS_H */
