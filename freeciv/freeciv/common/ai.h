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
#ifndef FC__AI_H
#define FC__AI_H

#define AI_DEFAULT 1

struct Treaty;
struct player;
struct ai_choice;
struct city;
struct unit;

enum incident_type {
  INCIDENT_DIPLOMAT = 0, INCIDENT_WAR, INCIDENT_PILLAGE,
  INCIDENT_NUCLEAR, INCIDENT_LAST
};

struct ai_type
{
  struct {
    void (*init_city)(struct city *pcity);
    void (*close_city)(struct city *pcity);
    void (*auto_settlers)(struct player *pplayer);
    void (*building_advisor_init)(struct player *pplayer);
    void (*building_advisor)(struct city *pcity, struct ai_choice *choice);
    enum unit_move_result (*auto_explorer)(struct unit *punit);
    void (*first_activities)(struct player *pplayer);
    void (*diplomacy_actions)(struct player *pplayer);
    void (*last_activities)(struct player *pplayer);
    void (*before_auto_settlers)(struct player *pplayer);
    void (*treaty_evaluate)(struct player *pplayer, struct player *aplayer, struct Treaty *ptreaty);
    void (*treaty_accepted)(struct player *pplayer, struct player *aplayer, struct Treaty *ptreaty);
    void (*first_contact)(struct player *pplayer, struct player *aplayer);
    void (*incident)(enum incident_type type, struct player *violator,
                     struct player *victim);
  } funcs;
};

struct ai_type *get_ai_type(int id);
void init_ai(struct ai_type *ai);


#define ai_type_iterate(NAME_ai) \
do { \
  struct ai_type *NAME_ai = get_ai_type(AI_DEFAULT);

#define ai_type_iterate_end \
  } while (FALSE);

#endif  /* FC__AI_H */
