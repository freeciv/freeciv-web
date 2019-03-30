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

/* tex ai */
#include "texaicity.h"
#include "texaimsg.h"
#include "texaiplayer.h"
#include "texaiworld.h"

const char *fc_ai_tex_capstr(void);
bool fc_ai_tex_setup(struct ai_type *ai);

static void texai_init_self(struct ai_type *ai);

static struct ai_type *self = NULL;

/**********************************************************************//**
  Set pointer to ai type of the tex ai.
**************************************************************************/
static void texai_init_self(struct ai_type *ai)
{
  self = ai;

  texai_init_threading();
}

/**********************************************************************//**
  Get pointer to ai type of the tex ai.
**************************************************************************/
struct ai_type *texai_get_self(void)
{
  return self;
}

#define TEXAI_AIT struct ai_type *ait = texai_get_self();
#define TEXAI_TFUNC(_func, ...) _func(ait, ## __VA_ARGS__ );
#define TEXAI_DFUNC(_func, ...) _func(ait, ## __VA_ARGS__ );

/**********************************************************************//**
  Free resources allocated by the tex AI module
**************************************************************************/
static void texai_module_close(void)
{
  TEXAI_AIT;

  FC_FREE(ait->private);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_player_alloc(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_TFUNC(texai_player_alloc, pplayer);
  /* Do not call default AI here, texai_player_alloc() does necessary parts */
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_player_free(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_TFUNC(texai_player_free, pplayer);
  /* Do not call default AI here, texai_player_free() does necessary parts */
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_player_save(struct player *pplayer,
                               struct section_file *file, int plrno)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_player_save, "texai", pplayer, file, plrno);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_player_load(struct player *pplayer,
                               const struct section_file *file,
                               int plrno)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_player_load, "texai", pplayer, file, plrno);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_control_gained(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_TFUNC(texai_control_gained, pplayer);
  TEXAI_DFUNC(dai_gained_control, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_control_lost(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_TFUNC(texai_control_lost, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_split_by_civil_war(struct player *original,
                                      struct player *created)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_assess_danger_player, original, &(wld.map));
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_created_by_civil_war(struct player *original,
                                        struct player *created)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_player_copy, original, created);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_phase_begin(struct player *pplayer, bool is_new_phase)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_data_phase_begin, pplayer, is_new_phase);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_phase_finished(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_TFUNC(texai_phase_finished, pplayer);
  TEXAI_DFUNC(dai_data_phase_finished, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_city_alloc(struct city *pcity)
{
  TEXAI_AIT;
  TEXAI_DFUNC(texai_city_alloc, pcity);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_city_free(struct city *pcity)
{
  TEXAI_AIT;
  TEXAI_DFUNC(texai_city_free, pcity);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_city_save(struct section_file *file,
                             const struct city *pcity, const char *citystr)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_city_save, "texai", file, pcity, citystr);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_city_load(const struct section_file *file,
                             struct city *pcity, const char *citystr)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_city_load, "texai", file, pcity, citystr);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_build_adv_override(struct city *pcity,
                                      struct adv_choice *choice)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_build_adv_override, pcity, choice);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_wonder_city_distance(struct player *pplayer,
                                        struct adv_data *adv)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_wonder_city_distance, pplayer, adv);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_build_adv_init(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_build_adv_init, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_build_adv_adjust(struct player *pplayer,
                                    struct city *wonder_city)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_build_adv_adjust, pplayer, wonder_city);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_gov_value(struct player *pplayer, struct government *gov,
                             adv_want *val, bool *override)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_gov_value, pplayer, gov, val, override);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_units_ruleset_init(void)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_units_ruleset_init);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_units_ruleset_close(void)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_units_ruleset_close);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_alloc(struct unit *punit)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_init, punit);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_free(struct unit *punit)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_close, punit);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_ferry_init_ferry(struct unit *ferry)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_ferry_init_ferry, ferry);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_ferry_transformed(struct unit *ferry,
                                     struct unit_type *old)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_ferry_transformed, ferry, old);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_ferry_lost(struct unit *punit)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_ferry_lost, punit);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_turn_end(struct unit *punit)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_turn_end, punit);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_move_or_attack(struct unit *punit,
                                       struct tile *ptile,
                                       struct pf_path *path, int step)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_move_or_attack, punit, ptile, path, step);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_new_adv_task(struct unit *punit,
                                     enum adv_unit_task task,
                                     struct tile *ptile)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_new_adv_task, punit, task, ptile);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_save(struct section_file *file,
                             const struct unit *punit, const char *unitstr)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_save, "texai", file, punit, unitstr);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_load(const struct section_file *file,
                             struct unit *punit, const char *unitstr)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_load, "texai", file, punit, unitstr);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_auto_settler_reset(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_auto_settler_reset, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_auto_settler_run(struct player *pplayer, struct unit *punit,
                                    struct settlermap *state)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_auto_settler_run, pplayer, punit, state);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_auto_settler_cont(struct player *pplayer,
                                     struct unit *punit,
                                     struct settlermap *state)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_auto_settler_cont, pplayer, punit, state);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_switch_to_explore(struct unit *punit, struct tile *target,
                                     enum override_bool *allow)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_switch_to_explore, punit, target, allow);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_first_activities(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_TFUNC(texai_first_activities, pplayer);
  TEXAI_DFUNC(dai_do_first_activities, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_diplomacy_actions(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_diplomacy_actions, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_last_activities(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_do_last_activities, pplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_treaty_evaluate(struct player *pplayer,
                                   struct player *aplayer,
                                   struct Treaty *ptreaty)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_treaty_evaluate, pplayer, aplayer, ptreaty);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_treaty_accepted(struct player *pplayer,
                                   struct player *aplayer, 
                                   struct Treaty *ptreaty)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_treaty_accepted, pplayer, aplayer, ptreaty);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_diplomacy_first_contact(struct player *pplayer,
                                           struct player *aplayer)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_diplomacy_first_contact, pplayer, aplayer);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_incident(enum incident_type type, struct player *violator,
                            struct player *victim)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_incident, type, violator, victim);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_city_log(char *buffer, int buflength,
                            const struct city *pcity)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_city_log, buffer, buflength, pcity);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_unit_log(char *buffer, int buflength,
                            const struct unit *punit)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_unit_log, buffer, buflength, punit);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_consider_plr_dangerous(struct player *plr1,
                                          struct player *plr2,
                                          enum override_bool *result)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_consider_plr_dangerous, plr1, plr2, result);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_consider_tile_dangerous(struct tile *ptile,
                                           struct unit *punit,
                                           enum override_bool *result)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_consider_tile_dangerous, ptile, punit, result);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_consider_wonder_city(struct city *pcity, bool *result)
{
  TEXAI_AIT;
  TEXAI_DFUNC(dai_consider_wonder_city, pcity, result);
}

/**********************************************************************//**
  Call default ai with tex ai type as parameter.
**************************************************************************/
static void texwai_refresh(struct player *pplayer)
{
  TEXAI_AIT;
  TEXAI_TFUNC(texai_refresh, pplayer);
}

/**********************************************************************//**
  Return module capability string
**************************************************************************/
const char *fc_ai_tex_capstr(void)
{
  return FC_AI_MOD_CAPSTR;
}

/**********************************************************************//**
  Setup player ai_funcs function pointers.
**************************************************************************/
bool fc_ai_tex_setup(struct ai_type *ai)
{
  struct dai_private_data *private;

  if (!has_thread_cond_impl()) {
    log_error(_("This Freeciv compilation has no full threads "
                "implementation, tex ai cannot be used."));
    return FALSE;
  }

  strncpy(ai->name, "tex", sizeof(ai->name));

  private = fc_malloc(sizeof(struct dai_private_data));
  private->contemplace_workers = FALSE; /* We use custom code to set worker want and type */
  ai->private = private;

  texai_init_self(ai);

  ai->funcs.module_close = texai_module_close;

  ai->funcs.map_alloc = texai_map_alloc;
  ai->funcs.map_ready = texai_whole_map_copy;
  ai->funcs.map_free = texai_map_free;

  ai->funcs.player_alloc = texwai_player_alloc;
  ai->funcs.player_free = texwai_player_free;
  ai->funcs.player_save = texwai_player_save;
  ai->funcs.player_load = texwai_player_load;
  ai->funcs.gained_control = texwai_control_gained;
  ai->funcs.lost_control = texwai_control_lost;
  ai->funcs.split_by_civil_war = texwai_split_by_civil_war;
  ai->funcs.created_by_civil_war = texwai_created_by_civil_war;

  ai->funcs.phase_begin = texwai_phase_begin;
  ai->funcs.phase_finished = texwai_phase_finished;

  ai->funcs.city_alloc = texwai_city_alloc;
  ai->funcs.city_free = texwai_city_free;
  ai->funcs.city_created = texai_city_created;
  ai->funcs.city_destroyed = texai_city_destroyed;
  ai->funcs.city_save = texwai_city_save;
  ai->funcs.city_load = texwai_city_load;
  ai->funcs.choose_building = texwai_build_adv_override;
  ai->funcs.build_adv_prepare = texwai_wonder_city_distance;
  ai->funcs.build_adv_init = texwai_build_adv_init;
  ai->funcs.build_adv_adjust_want = texwai_build_adv_adjust;

  ai->funcs.gov_value = texwai_gov_value;

  ai->funcs.units_ruleset_init = texwai_units_ruleset_init;
  ai->funcs.units_ruleset_close = texwai_units_ruleset_close;

  ai->funcs.unit_alloc = texwai_unit_alloc;
  ai->funcs.unit_free = texwai_unit_free;

  ai->funcs.unit_created = texai_unit_created;
  ai->funcs.unit_destroyed = texai_unit_destroyed;

  ai->funcs.unit_got = texwai_ferry_init_ferry;
  ai->funcs.unit_lost = texwai_ferry_lost;
  ai->funcs.unit_transformed = texwai_ferry_transformed;

  ai->funcs.unit_turn_end = texwai_unit_turn_end;
  ai->funcs.unit_move = texwai_unit_move_or_attack;
  ai->funcs.unit_move_seen = texai_unit_move_seen;
  ai->funcs.unit_task = texwai_unit_new_adv_task;

  ai->funcs.unit_save = texwai_unit_save;
  ai->funcs.unit_load = texwai_unit_load;

  ai->funcs.settler_reset = texwai_auto_settler_reset;
  ai->funcs.settler_run = texwai_auto_settler_run;
  ai->funcs.settler_cont = texwai_auto_settler_cont;

  ai->funcs.want_to_explore = texwai_switch_to_explore;

  ai->funcs.first_activities = texwai_first_activities;
  /* Do complete run after savegame loaded - we don't know what has been
     done before. */
  ai->funcs.restart_phase = texwai_first_activities;
  ai->funcs.diplomacy_actions = texwai_diplomacy_actions;
  ai->funcs.last_activities = texwai_last_activities;

  ai->funcs.treaty_evaluate = texwai_treaty_evaluate;
  ai->funcs.treaty_accepted = texwai_treaty_accepted;
  ai->funcs.first_contact = texwai_diplomacy_first_contact;
  ai->funcs.incident = texwai_incident;

  ai->funcs.log_fragment_city = texwai_city_log;
  ai->funcs.log_fragment_unit = texwai_unit_log;

  ai->funcs.consider_plr_dangerous = texwai_consider_plr_dangerous;
  ai->funcs.consider_tile_dangerous = texwai_consider_tile_dangerous;
  ai->funcs.consider_wonder_city = texwai_consider_wonder_city;

  ai->funcs.refresh = texwai_refresh;

  ai->funcs.tile_info = texai_tile_info;
  
  return TRUE;
}
