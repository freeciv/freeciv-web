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

/* common */
#include "ai.h"

/* default ai */
#include "aicity.h"
#include "aidata.h"
#include "aiferry.h"
#include "aihand.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aisettler.h"
#include "aitools.h"
#include "daidiplomacy.h"
#include "daidomestic.h"
#include "daimilitary.h"

/* threaded ai */
#include "taimsg.h"
#include "taiplayer.h"

const char *fc_ai_threaded_capstr(void);
bool fc_ai_threaded_setup(struct ai_type *ai);

static void tai_init_self(struct ai_type *ai);
static struct ai_type *tai_get_self(void);

static struct ai_type *self = NULL;

/**********************************************************************//**
  Set pointer to ai type of the threaded ai.
**************************************************************************/
static void tai_init_self(struct ai_type *ai)
{
  self = ai;

  tai_init_threading();
}

/**********************************************************************//**
  Get pointer to ai type of the threaded ai.
**************************************************************************/
static struct ai_type *tai_get_self(void)
{
  return self;
}

#define TAI_AIT struct ai_type *ait = tai_get_self();
#define TAI_TFUNC(_func, ...) _func(ait, ## __VA_ARGS__ );
#define TAI_DFUNC(_func, ...) _func(ait, ## __VA_ARGS__ );

/**********************************************************************//**
  Free resources allocated by the threaded AI module
**************************************************************************/
static void tai_module_close(void)
{
  TAI_AIT;

  FC_FREE(ait->private);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_player_alloc(struct player *pplayer)
{
  TAI_AIT;
  TAI_TFUNC(tai_player_alloc, pplayer);
  /* Do not call default AI here, tai_player_alloc() does necessary parts */
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_player_free(struct player *pplayer)
{
  TAI_AIT;
  TAI_TFUNC(tai_player_free, pplayer);
  /* Do not call default AI here, tai_player_free() does necessary parts */
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_player_save(struct player *pplayer, struct section_file *file,
                             int plrno)
{
  TAI_AIT;
  TAI_DFUNC(dai_player_save, "tai", pplayer, file, plrno);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_player_load(struct player *pplayer,
                             const struct section_file *file,
                             int plrno)
{
  TAI_AIT;
  TAI_DFUNC(dai_player_load, "tai", pplayer, file, plrno);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_control_gained(struct player *pplayer)
{
  TAI_AIT;
  TAI_TFUNC(tai_control_gained, pplayer);
  TAI_DFUNC(dai_gained_control, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_control_lost(struct player *pplayer)
{
  TAI_AIT;
  TAI_TFUNC(tai_control_lost, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_split_by_civil_war(struct player *original,
                                    struct player *created)
{
  TAI_AIT;
  TAI_DFUNC(dai_assess_danger_player, original, &(wld.map));
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_created_by_civil_war(struct player *original,
                                      struct player *created)
{
  TAI_AIT;
  TAI_DFUNC(dai_player_copy, original, created);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_phase_begin(struct player *pplayer, bool is_new_phase)
{
  TAI_AIT;
  TAI_DFUNC(dai_data_phase_begin, pplayer, is_new_phase);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_phase_finished(struct player *pplayer)
{
  TAI_AIT;
  TAI_TFUNC(tai_phase_finished, pplayer);
  TAI_DFUNC(dai_data_phase_finished, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_city_alloc(struct city *pcity)
{
  TAI_AIT;
  TAI_DFUNC(dai_city_alloc, pcity);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_city_free(struct city *pcity)
{
  TAI_AIT;
  TAI_DFUNC(dai_city_free, pcity);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_city_save(struct section_file *file, const struct city *pcity,
                           const char *citystr)
{
  TAI_AIT;
  TAI_DFUNC(dai_city_save, "tai", file, pcity, citystr);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_city_load(const struct section_file *file, struct city *pcity,
                           const char *citystr)
{
  TAI_AIT;
  TAI_DFUNC(dai_city_load, "tai", file, pcity, citystr);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_build_adv_override(struct city *pcity, struct adv_choice *choice)
{
  TAI_AIT;
  TAI_DFUNC(dai_build_adv_override, pcity, choice);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_wonder_city_distance(struct player *pplayer, struct adv_data *adv)
{
  TAI_AIT;
  TAI_DFUNC(dai_wonder_city_distance, pplayer, adv);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_build_adv_init(struct player *pplayer)
{
  TAI_AIT;
  TAI_DFUNC(dai_build_adv_init, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_build_adv_adjust(struct player *pplayer, struct city *wonder_city)
{
  TAI_AIT;
  TAI_DFUNC(dai_build_adv_adjust, pplayer, wonder_city);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_gov_value(struct player *pplayer, struct government *gov,
                           adv_want *val, bool *override)
{
  TAI_AIT;
  TAI_DFUNC(dai_gov_value, pplayer, gov, val, override);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_units_ruleset_init(void)
{
  TAI_AIT;
  TAI_DFUNC(dai_units_ruleset_init);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_units_ruleset_close(void)
{
  TAI_AIT;
  TAI_DFUNC(dai_units_ruleset_close);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_alloc(struct unit *punit)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_init, punit);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_free(struct unit *punit)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_close, punit);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_ferry_init_ferry(struct unit *ferry)
{
  TAI_AIT;
  TAI_DFUNC(dai_ferry_init_ferry, ferry);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_ferry_transformed(struct unit *ferry, struct unit_type *old)
{
  TAI_AIT;
  TAI_DFUNC(dai_ferry_transformed, ferry, old);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_ferry_lost(struct unit *punit)
{
  TAI_AIT;
  TAI_DFUNC(dai_ferry_lost, punit);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_turn_end(struct unit *punit)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_turn_end, punit);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_move_or_attack(struct unit *punit, struct tile *ptile,
                                     struct pf_path *path, int step)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_move_or_attack, punit, ptile, path, step);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_new_adv_task(struct unit *punit, enum adv_unit_task task,
                                   struct tile *ptile)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_new_adv_task, punit, task, ptile);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_save(struct section_file *file, const struct unit *punit,
                           const char *unitstr)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_save, "tai", file, punit, unitstr);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_load(const struct section_file *file, struct unit *punit,
                           const char *unitstr)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_load, "tai", file, punit, unitstr);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_auto_settler_reset(struct player *pplayer)
{
  TAI_AIT;
  TAI_DFUNC(dai_auto_settler_reset, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_auto_settler_run(struct player *pplayer, struct unit *punit,
                                  struct settlermap *state)
{
  TAI_AIT;
  TAI_DFUNC(dai_auto_settler_run, pplayer, punit, state);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_auto_settler_cont(struct player *pplayer, struct unit *punit,
                                   struct settlermap *state)
{
  TAI_AIT;
  TAI_DFUNC(dai_auto_settler_cont, pplayer, punit, state);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_switch_to_explore(struct unit *punit, struct tile *target,
                                  enum override_bool *allow)
{
  TAI_AIT;
  TAI_DFUNC(dai_switch_to_explore, punit, target, allow);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_first_activities(struct player *pplayer)
{
  TAI_AIT;
  TAI_TFUNC(tai_first_activities, pplayer);
  TAI_DFUNC(dai_do_first_activities, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_diplomacy_actions(struct player *pplayer)
{
  TAI_AIT;
  TAI_DFUNC(dai_diplomacy_actions, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_last_activities(struct player *pplayer)
{
  TAI_AIT;
  TAI_DFUNC(dai_do_last_activities, pplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_treaty_evaluate(struct player *pplayer, struct player *aplayer,
                                 struct Treaty *ptreaty)
{
  TAI_AIT;
  TAI_DFUNC(dai_treaty_evaluate, pplayer, aplayer, ptreaty);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_treaty_accepted(struct player *pplayer, struct player *aplayer, 
                                 struct Treaty *ptreaty)
{
  TAI_AIT;
  TAI_DFUNC(dai_treaty_accepted, pplayer, aplayer, ptreaty);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_diplomacy_first_contact(struct player *pplayer,
                                         struct player *aplayer)
{
  TAI_AIT;
  TAI_DFUNC(dai_diplomacy_first_contact, pplayer, aplayer);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_incident(enum incident_type type, struct player *violator,
                          struct player *victim)
{
  TAI_AIT;
  TAI_DFUNC(dai_incident, type, violator, victim);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_city_log(char *buffer, int buflength, const struct city *pcity)
{
  TAI_AIT;
  TAI_DFUNC(dai_city_log, buffer, buflength, pcity);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_unit_log(char *buffer, int buflength, const struct unit *punit)
{
  TAI_AIT;
  TAI_DFUNC(dai_unit_log, buffer, buflength, punit);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_consider_plr_dangerous(struct player *plr1, struct player *plr2,
                                        enum override_bool *result)
{
  TAI_AIT;
  TAI_DFUNC(dai_consider_plr_dangerous, plr1, plr2, result);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_consider_tile_dangerous(struct tile *ptile, struct unit *punit,
                                         enum override_bool *result)
{
  TAI_AIT;
  TAI_DFUNC(dai_consider_tile_dangerous, ptile, punit, result);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_consider_wonder_city(struct city *pcity, bool *result)
{
  TAI_AIT;
  TAI_DFUNC(dai_consider_wonder_city, pcity, result);
}

/**********************************************************************//**
  Call default ai with threaded ai type as parameter.
**************************************************************************/
static void twai_refresh(struct player *pplayer)
{
  TAI_AIT;
  TAI_TFUNC(tai_refresh, pplayer);
}

/**********************************************************************//**
  Return module capability string
**************************************************************************/
const char *fc_ai_threaded_capstr(void)
{
  return FC_AI_MOD_CAPSTR;
}

/**********************************************************************//**
  Setup player ai_funcs function pointers.
**************************************************************************/
bool fc_ai_threaded_setup(struct ai_type *ai)
{
  struct dai_private_data *private;

  if (!has_thread_cond_impl()) {
    log_error(_("This Freeciv compilation has no full threads "
                "implementation, threaded ai cannot be used."));
    return FALSE;
  }

  strncpy(ai->name, "threaded", sizeof(ai->name));

  private = fc_malloc(sizeof(struct dai_private_data));
  private->contemplace_workers = TRUE;
  ai->private = private;

  tai_init_self(ai);

  ai->funcs.module_close = tai_module_close;
  ai->funcs.player_alloc = twai_player_alloc;
  ai->funcs.player_free = twai_player_free;
  ai->funcs.player_save = twai_player_save;
  ai->funcs.player_load = twai_player_load;
  ai->funcs.gained_control = twai_control_gained;
  ai->funcs.lost_control = twai_control_lost;
  ai->funcs.split_by_civil_war = twai_split_by_civil_war;
  ai->funcs.created_by_civil_war = twai_created_by_civil_war;

  ai->funcs.phase_begin = twai_phase_begin;
  ai->funcs.phase_finished = twai_phase_finished;

  ai->funcs.city_alloc = twai_city_alloc;
  ai->funcs.city_free = twai_city_free;
  ai->funcs.city_save = twai_city_save;
  ai->funcs.city_load = twai_city_load;
  ai->funcs.choose_building = twai_build_adv_override;
  ai->funcs.build_adv_prepare = twai_wonder_city_distance;
  ai->funcs.build_adv_init = twai_build_adv_init;
  ai->funcs.build_adv_adjust_want = twai_build_adv_adjust;

  ai->funcs.gov_value = twai_gov_value;

  ai->funcs.units_ruleset_init = twai_units_ruleset_init;
  ai->funcs.units_ruleset_close = twai_units_ruleset_close;

  ai->funcs.unit_alloc = twai_unit_alloc;
  ai->funcs.unit_free = twai_unit_free;

  ai->funcs.unit_got = twai_ferry_init_ferry;
  ai->funcs.unit_lost = twai_ferry_lost;
  ai->funcs.unit_transformed = twai_ferry_transformed;

  ai->funcs.unit_turn_end = twai_unit_turn_end;
  ai->funcs.unit_move = twai_unit_move_or_attack;
  ai->funcs.unit_task = twai_unit_new_adv_task;

  ai->funcs.unit_save = twai_unit_save;
  ai->funcs.unit_load = twai_unit_load;

  ai->funcs.settler_reset = twai_auto_settler_reset;
  ai->funcs.settler_run = twai_auto_settler_run;
  ai->funcs.settler_cont = twai_auto_settler_cont;

  ai->funcs.want_to_explore = twai_switch_to_explore;

  ai->funcs.first_activities = twai_first_activities;
  /* Do complete run after savegame loaded - we don't know what has been
     done before. */
  ai->funcs.restart_phase = twai_first_activities;
  ai->funcs.diplomacy_actions = twai_diplomacy_actions;
  ai->funcs.last_activities = twai_last_activities;

  ai->funcs.treaty_evaluate = twai_treaty_evaluate;
  ai->funcs.treaty_accepted = twai_treaty_accepted;
  ai->funcs.first_contact = twai_diplomacy_first_contact;
  ai->funcs.incident = twai_incident;

  ai->funcs.log_fragment_city = twai_city_log;
  ai->funcs.log_fragment_unit = twai_unit_log;

  ai->funcs.consider_plr_dangerous = twai_consider_plr_dangerous;
  ai->funcs.consider_tile_dangerous = twai_consider_tile_dangerous;
  ai->funcs.consider_wonder_city = twai_consider_wonder_city;

  ai->funcs.refresh = twai_refresh;

  return TRUE;
}
