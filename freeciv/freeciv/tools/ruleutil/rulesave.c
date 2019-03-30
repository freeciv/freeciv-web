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

/* utility */
#include "registry.h"
#include "string_vector.h"

/* common */
#include "achievements.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "multipliers.h"
#include "specialist.h"
#include "style.h"
#include "unittype.h"
#include "version.h"

/* server */
#include "ruleset.h"
#include "settings.h"

/* tools/ruleutil */
#include "comments.h"

#include "rulesave.h"

/* Ruleset format version */
/*
 * 1  - Freeciv-2.6 
 * 10 - Freeciv-3.0
 * 20 - Freeciv-3.1
 */
#define FORMAT_VERSION 20

/**********************************************************************//**
  Create new ruleset section file with common header.
**************************************************************************/
static struct section_file *create_ruleset_file(const char *rsname,
                                                const char *rstype)
{
  struct section_file *sfile = secfile_new(TRUE);
  char buf[500];

  comment_file_header(sfile);

  if (rsname != NULL && rsname[0] != '\0') {
    fc_snprintf(buf, sizeof(buf), "%s %s data for Freeciv", rsname, rstype);
  } else {
    fc_snprintf(buf, sizeof(buf), "Template %s data for Freeciv", rstype);
  }

  secfile_insert_str(sfile, buf, "datafile.description");
  secfile_insert_str(sfile, freeciv_datafile_version(), "datafile.ruledit");
  secfile_insert_str(sfile, RULESET_CAPABILITIES, "datafile.options");
  secfile_insert_int(sfile, FORMAT_VERSION, "datafile.format_version");

  return sfile;
}

/**********************************************************************//**
  Save int value that has default applied upon loading.
**************************************************************************/
static bool save_default_int(struct section_file *sfile, int value,
                             int default_value, const char *path, const char *entry)
{
  if (value != default_value) {
    if (entry != NULL) {
      secfile_insert_int(sfile, value,
                         "%s.%s", path, entry);
    } else {
      secfile_insert_int(sfile, value,
                         "%s", path);
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Save bool value that has default applied upon loading.
**************************************************************************/
static bool save_default_bool(struct section_file *sfile, bool value,
                              bool default_value, const char *path, const char *entry)
{
  if ((value && !default_value)
      || (!value && default_value)) {
    if (entry != NULL) {
      secfile_insert_bool(sfile, value,
                          "%s.%s", path, entry);
    } else {
      secfile_insert_bool(sfile, value,
                          "%s", path);
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Save name of the object.
**************************************************************************/
static bool save_name_translation(struct section_file *sfile,
                                  struct name_translation *name,
                                  const char *path)
{
  struct entry *mod_entry;

  mod_entry = secfile_insert_str(sfile,
                                 untranslated_name(name),
                                 "%s.name", path);
  entry_str_set_gt_marking(mod_entry, TRUE);
  if (strcmp(skip_intl_qualifier_prefix(untranslated_name(name)),
             rule_name_get(name))) {
    secfile_insert_str(sfile,
                       rule_name_get(name),
                       "%s.rule_name", path);
  }

  return TRUE;
}

/**********************************************************************//**
  Save vector of requirements
**************************************************************************/
static bool save_reqs_vector(struct section_file *sfile,
                             const struct requirement_vector *reqs,
                             const char *path, const char *entry)
{
  int i;
  bool includes_negated = FALSE;
  bool includes_surviving = FALSE;
  bool includes_quiet = FALSE;

  requirement_vector_iterate(reqs, preq) {
    if (!preq->present) {
      includes_negated = TRUE;
    }
    if (preq->survives) {
      includes_surviving = TRUE;
    }
    if (preq->quiet) {
      includes_quiet = TRUE;
    }
  } requirement_vector_iterate_end;

  i = 0;
  requirement_vector_iterate(reqs, preq) {
    secfile_insert_str(sfile,
                       universals_n_name(preq->source.kind),
                       "%s.%s%d.type", path, entry, i);
    secfile_insert_str(sfile,
                       universal_rule_name(&(preq->source)),
                       "%s.%s%d.name", path, entry, i);
    secfile_insert_str(sfile,
                       req_range_name(preq->range),
                       "%s.%s%d.range", path, entry, i);

    if (includes_surviving) {
      secfile_insert_bool(sfile,
                          preq->survives,
                          "%s.%s%d.survives", path, entry, i);
    }

    if (includes_negated) {
      secfile_insert_bool(sfile,
                          preq->present,
                          "%s.%s%d.present", path, entry, i);
    }

    if (includes_quiet) {
      secfile_insert_bool(sfile,
                          preq->quiet,
                          "%s.%s%d.quiet", path, entry, i);
    }

    i++;
  } requirement_vector_iterate_end;

  return TRUE;
}

/**********************************************************************//**
  Save techs vector. Input is A_LAST terminated array of techs to save.
**************************************************************************/
static bool save_tech_list(struct section_file *sfile, int *input,
                           const char *path, const char *entry)
{
  const char *tech_names[MAX_NUM_TECH_LIST];
  int set_count;
  int i;

  set_count = 0;
  for (i = 0; input[i] != A_LAST && i < MAX_NUM_TECH_LIST; i++) {
    tech_names[set_count++] = advance_rule_name(advance_by_number(input[i]));
  }

  if (set_count > 0) {
    secfile_insert_str_vec(sfile, tech_names, set_count,
                           "%s.%s", path, entry);
  }

  return TRUE;
}

/**********************************************************************//**
  Save tech reference
**************************************************************************/
static bool save_tech_ref(struct section_file *sfile,
                          const struct advance *padv,
                          const char *path, const char *entry)
{
   if (padv == A_NEVER) {
     secfile_insert_str(sfile, "Never", "%s.%s", path, entry);
   } else {
     secfile_insert_str(sfile, advance_rule_name(padv),
                        "%s.%s", path, entry);
   }

   return TRUE;
}

/**********************************************************************//**
  Save terrain reference
**************************************************************************/
static bool save_terrain_ref(struct section_file *sfile,
                             const struct terrain *save,
                             const struct terrain *pthis,
                             const char *path, const char *entry)
{
  if (save == NULL) {
    secfile_insert_str(sfile, "none", "%s.%s", path, entry);
  } else if (save == pthis) {
    secfile_insert_str(sfile, "yes", "%s.%s", path, entry);   
  } else {
    secfile_insert_str(sfile, terrain_rule_name(save),
                        "%s.%s", path, entry);
  }

  return TRUE;
}

/**********************************************************************//**
  Save government reference
**************************************************************************/
static bool save_gov_ref(struct section_file *sfile,
                         const struct government *gov,
                         const char *path, const char *entry)
{
  secfile_insert_str(sfile, government_rule_name(gov), "%s.%s", path, entry);

  return TRUE;
}

/**********************************************************************//**
  Save buildings vector. Input is B_LAST terminated array of buildings
  to save.
**************************************************************************/
static bool save_building_list(struct section_file *sfile, int *input,
                               const char *path, const char *entry)
{
  const char *building_names[MAX_NUM_BUILDING_LIST];
  int set_count;
  int i;

  set_count = 0;
  for (i = 0; input[i] != B_LAST && i < MAX_NUM_BUILDING_LIST; i++) {
    building_names[set_count++] = improvement_rule_name(improvement_by_number(input[i]));
  }

  if (set_count > 0) {
    secfile_insert_str_vec(sfile, building_names, set_count,
                           "%s.%s", path, entry);
  }

  return TRUE;
}

/**********************************************************************//**
  Save units vector. Input is NULL terminated array of units
  to save.
**************************************************************************/
static bool save_unit_list(struct section_file *sfile, struct unit_type **input,
                           const char *path, const char *entry)
{
  const char *unit_names[MAX_NUM_UNIT_LIST];
  int set_count;
  int i;

  set_count = 0;
  for (i = 0; input[i] != NULL && i < MAX_NUM_UNIT_LIST; i++) {
    unit_names[set_count++] = utype_rule_name(input[i]);
  }

  if (set_count > 0) {
    secfile_insert_str_vec(sfile, unit_names, set_count,
                           "%s.%s", path, entry);
  }

  return TRUE;
}

/**********************************************************************//**
  Save vector of unit class names based on bitvector bits
**************************************************************************/
static bool save_uclass_vec(struct section_file *sfile,
                            bv_unit_classes *bits,
                            const char *path, const char *entry,
                            bool unreachable_only)
{
  const char *class_names[UCL_LAST];
  int classes = 0;

  unit_class_iterate(pcargo) {
    if (BV_ISSET(*(bits), uclass_index(pcargo))
        && (uclass_has_flag(pcargo, UCF_UNREACHABLE)
            || !unreachable_only)) {
      class_names[classes++] = uclass_rule_name(pcargo);
    }
  } unit_class_iterate_end;

  if (classes > 0) {
    secfile_insert_str_vec(sfile, class_names, classes,
                           "%s.%s", path, entry);
  }

  return TRUE;
}

/**********************************************************************//**
  Save strvec as ruleset vector of strings
**************************************************************************/
static bool save_strvec(struct section_file *sfile,
                        struct strvec *to_save,
                        const char *path, const char *entry)
{
  if (to_save != NULL) {
    int sect_count = strvec_size(to_save);
    const char *sections[sect_count];
    int i;

    for (i = 0; i < sect_count; i++) {
      sections[i] = strvec_get(to_save, i);
    }

    secfile_insert_str_vec(sfile, sections, sect_count, "%s.%s", path, entry);
  }

  return TRUE;
}

/**********************************************************************//**
  Save ruleset file.
**************************************************************************/
static bool save_ruleset_file(struct section_file *sfile, const char *filename)
{
  return secfile_save(sfile, filename, 0, FZ_PLAIN);
}

/**********************************************************************//**
  Save buildings.ruleset
**************************************************************************/
static bool save_buildings_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "building");
  int sect_idx;

  if (sfile == NULL) {
    return FALSE;
  }

  comment_buildings(sfile);

  sect_idx = 0;
  improvement_re_active_iterate(pb) {
    if (!pb->ruledit_disabled) {
      char path[512];
      const char *flag_names[IF_COUNT];
      int set_count;
      int flagi;

      fc_snprintf(path, sizeof(path), "building_%d", sect_idx++);

      save_name_translation(sfile, &(pb->name), path);

      secfile_insert_str(sfile, impr_genus_id_name(pb->genus),
                         "%s.genus", path);

      if (strcmp(pb->graphic_str, "-")) {
        secfile_insert_str(sfile, pb->graphic_str, "%s.graphic", path);
      }
      if (strcmp(pb->graphic_alt, "-")) {
        secfile_insert_str(sfile, pb->graphic_alt, "%s.graphic_alt", path);
      }
      if (strcmp(pb->soundtag, "-")) {
        secfile_insert_str(sfile, pb->soundtag, "%s.sound", path);
      }
      if (strcmp(pb->soundtag_alt, "-")) {
        secfile_insert_str(sfile, pb->soundtag_alt, "%s.sound_alt", path);
      }

      save_reqs_vector(sfile, &(pb->reqs), path, "reqs");
      save_reqs_vector(sfile, &(pb->obsolete_by), path, "obsolete_by");

      secfile_insert_int(sfile, pb->build_cost, "%s.build_cost", path);
      secfile_insert_int(sfile, pb->upkeep, "%s.upkeep", path);
      secfile_insert_int(sfile, pb->sabotage, "%s.sabotage", path);

      set_count = 0;
      for (flagi = 0; flagi < IF_COUNT; flagi++) {
        if (improvement_has_flag(pb, flagi)) {
          flag_names[set_count++] = impr_flag_id_name(flagi);
        }
      }

      if (set_count > 0) {
        secfile_insert_str_vec(sfile, flag_names, set_count,
                               "%s.flags", path);
      }

      save_strvec(sfile, pb->helptext, path, "helptext");
    }
  } improvement_re_active_iterate_end;

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save styles.ruleset
**************************************************************************/
static bool save_styles_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "styles");
  int sect_idx;
  int i;

  if (sfile == NULL) {
    return FALSE;
  }

  comment_styles(sfile);

  sect_idx = 0;
  styles_re_active_iterate(pstyle) {
    char path[512];

    fc_snprintf(path, sizeof(path), "style_%d", sect_idx++);

    save_name_translation(sfile, &(pstyle->name), path);
  } styles_re_active_iterate_end;

  comment_citystyles(sfile);

  sect_idx = 0;
  for (i = 0; i < game.control.styles_count; i++) {
    char path[512];

    fc_snprintf(path, sizeof(path), "citystyle_%d", sect_idx++);

    save_name_translation(sfile, &(city_styles[i].name), path);

    secfile_insert_str(sfile, city_styles[i].graphic, "%s.graphic", path);
    secfile_insert_str(sfile, city_styles[i].graphic_alt, "%s.graphic_alt", path);
    if (strcmp(city_styles[i].citizens_graphic, "-")) {
      secfile_insert_str(sfile, city_styles[i].citizens_graphic,
                         "%s.citizens_graphic", path);
    }
    if (strcmp(city_styles[i].citizens_graphic_alt, "generic")) {
      secfile_insert_str(sfile, city_styles[i].citizens_graphic_alt,
                         "%s.citizens_graphic_alt", path);
    }

    save_reqs_vector(sfile, &(city_styles[i].reqs), path, "reqs");
  }

  comment_musicstyles(sfile);

  sect_idx = 0;
  music_styles_iterate(pmus) {
    char path[512];

    fc_snprintf(path, sizeof(path), "musicstyle_%d", sect_idx++);

    secfile_insert_str(sfile, pmus->music_peaceful, "%s.music_peaceful", path);
    secfile_insert_str(sfile, pmus->music_combat, "%s.music_combat", path);

    save_reqs_vector(sfile, &(pmus->reqs), path, "reqs");
  } music_styles_iterate_end;

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save an action auto performer's !present utype reqs as a regular setting.
  This is done because the Action Auto Perform rules system isn't ready to
  be exposed to the ruleset yet. The setting is a list of utype flags that
  prevents the auto action performer.
**************************************************************************/
static bool save_action_auto_uflag_block(struct section_file *sfile,
                                         const int aap,
                                         const char *uflags_path,
                                         bool (*unexpected_req)(
                                           const struct requirement *preq))
{
  enum unit_type_flag_id protecor_flag[MAX_NUM_USER_UNIT_FLAGS];
  size_t i;
  size_t ret;

  const struct action_auto_perf *auto_perf =
      action_auto_perf_by_number(aap);

  i = 0;
  requirement_vector_iterate(&auto_perf->reqs, req) {
    fc_assert(req->range == REQ_RANGE_LOCAL);

    if (req->source.kind == VUT_UTFLAG) {
      fc_assert(!req->present);

      protecor_flag[i++] = req->source.value.unitflag;
    } else if (unexpected_req(req)) {
      log_error("Can't handle action auto performer requirement %s",
                req_to_fstring(req));

      return FALSE;
    }
  } requirement_vector_iterate_end;

  ret = secfile_insert_enum_vec(sfile, &protecor_flag, i,
                                unit_type_flag_id,
                                "%s", uflags_path);

  if (ret != i) {
    log_error("%s: didn't save all unit type flags.", uflags_path);

    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Save an action auto performer's action list as a regular setting. This
  is done because the Action Auto Perform rules system isn't ready to be
  exposed to the ruleset yet. The setting is a list of actions in the
  order they should be tried.
**************************************************************************/
static bool save_action_auto_actions(struct section_file *sfile,
                                         const int aap,
                                         const char *actions_path)
{
  enum gen_action unit_acts[MAX_NUM_ACTIONS];
  size_t i;
  size_t ret;

  const struct action_auto_perf *auto_perf =
      action_auto_perf_by_number(aap);

  i = 0;
  for (i = 0;
       i < NUM_ACTIONS && auto_perf->alternatives[i] != ACTION_NONE;
       i++) {
    unit_acts[i] = auto_perf->alternatives[i];
  }

  ret = secfile_insert_enum_vec(sfile, &unit_acts, i, gen_action,
                                "%s", actions_path);

  if (ret != i) {
    log_error("%s: didn't save all actions.", actions_path);

    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Missing unit upkeep should only contain output type and absence of
  blocking unit type flag requirements.
**************************************************************************/
static bool unexpected_non_otype(const struct requirement *req)
{
  return !(req->source.kind == VUT_OTYPE && req->present);
}

/**********************************************************************//**
  Save the action a unit should perform when its missing food, gold or
  shield upkeep. Save as regular settings since the Action Auto Perform
  rules system isn't ready to be exposed to the ruleset yet.
**************************************************************************/
static bool save_muuk_action_auto(struct section_file *sfile,
                                  const int aap,
                                  const char *item)
{
  char uflags_path[100];
  char action_path[100];

  fc_snprintf(uflags_path, sizeof(uflags_path),
              "missing_unit_upkeep.%s_protected", item);
  fc_snprintf(action_path, sizeof(action_path),
              "missing_unit_upkeep.%s_unit_act", item);

  return (save_action_auto_uflag_block(sfile, aap, uflags_path,
                                       unexpected_non_otype)
          && save_action_auto_actions(sfile, aap, action_path));
}

/**********************************************************************//**
  Save cities.ruleset
**************************************************************************/
static bool save_cities_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "cities");
  int sect_idx;

  if (sfile == NULL) {
    return FALSE;
  }

  comment_specialists(sfile);

  sect_idx = 0;
  specialist_type_iterate(sp) {
    struct specialist *s = specialist_by_number(sp);
    char path[512];

    fc_snprintf(path, sizeof(path), "specialist_%d", sect_idx++);

    save_name_translation(sfile, &(s->name), path);

    if (strcmp(rule_name_get(&s->name), rule_name_get(&s->abbreviation))) {
      secfile_insert_str(sfile, rule_name_get(&s->abbreviation),
                         "%s.short_name", path);
    }

    save_reqs_vector(sfile, &(s->reqs), path, "reqs");

    secfile_insert_str(sfile, s->graphic_str, "%s.graphic", path);
    if (strcmp(s->graphic_alt, "-")) {
      secfile_insert_str(sfile, s->graphic_alt, "%s.graphic_alt", path);
    }

    save_strvec(sfile, s->helptext, path, "helptext");

  } specialist_type_iterate_end;

  if (game.info.celebratesize != GAME_DEFAULT_CELEBRATESIZE) {
    secfile_insert_int(sfile, game.info.celebratesize,
                       "parameters.celebrate_size_limit");
  }
  if (game.info.add_to_size_limit != GAME_DEFAULT_ADDTOSIZE) {
    secfile_insert_int(sfile, game.info.add_to_size_limit,
                       "parameters.add_to_size_limit");
  }
  if (game.info.angrycitizen != GAME_DEFAULT_ANGRYCITIZEN) {
    secfile_insert_bool(sfile, game.info.angrycitizen,
                       "parameters.angry_citizens");
  }
  if (game.info.changable_tax != GAME_DEFAULT_CHANGABLE_TAX) {
    secfile_insert_bool(sfile, game.info.changable_tax,
                       "parameters.changable_tax");
  }
  if (game.info.forced_science != 0) {
    secfile_insert_int(sfile, game.info.forced_science,
                       "parameters.forced_science");
  }
  if (game.info.forced_luxury != 100) {
    secfile_insert_int(sfile, game.info.forced_luxury,
                       "parameters.forced_luxury");
  }
  if (game.info.forced_gold != 0) {
    secfile_insert_int(sfile, game.info.forced_gold,
                       "parameters.forced_gold");
  }
  if (game.server.vision_reveal_tiles != GAME_DEFAULT_VISION_REVEAL_TILES) {
    secfile_insert_bool(sfile, game.server.vision_reveal_tiles,
                       "parameters.vision_reveal_tiles");
  }
  if (game.info.pop_report_zeroes != 1) {
    secfile_insert_int(sfile, game.info.pop_report_zeroes,
                       "parameters.pop_report_zeroes");
  }
  if (game.info.citizen_nationality != GAME_DEFAULT_NATIONALITY) {
    secfile_insert_bool(sfile, game.info.citizen_nationality,
                       "citizen.nationality");
  }
  if (game.info.citizen_convert_speed != GAME_DEFAULT_CONVERT_SPEED) {
    secfile_insert_int(sfile, game.info.citizen_convert_speed,
                       "citizen.convert_speed");
  }
  if (game.info.conquest_convert_pct != 0) {
    secfile_insert_int(sfile, game.info.conquest_convert_pct,
                       "citizen.conquest_convert_pct");
  }
  if (game.info.citizen_partisans_pct != 0) {
    secfile_insert_int(sfile, game.info.citizen_partisans_pct,
                       "citizen.partisans_pct");
  }

  save_muuk_action_auto(sfile, ACTION_AUTO_UPKEEP_FOOD,
                        "food");
  if (game.info.muuk_food_wipe != RS_DEFAULT_MUUK_FOOD_WIPE) {
    secfile_insert_bool(sfile, game.info.muuk_food_wipe,
                        "missing_unit_upkeep.food_wipe");
  }

  save_muuk_action_auto(sfile, ACTION_AUTO_UPKEEP_GOLD,
                        "gold");
  if (game.info.muuk_gold_wipe != RS_DEFAULT_MUUK_GOLD_WIPE) {
    secfile_insert_bool(sfile, game.info.muuk_gold_wipe,
                        "missing_unit_upkeep.gold_wipe");
  }

  save_muuk_action_auto(sfile, ACTION_AUTO_UPKEEP_SHIELD,
                        "shield");
  if (game.info.muuk_shield_wipe != RS_DEFAULT_MUUK_SHIELD_WIPE) {
    secfile_insert_bool(sfile, game.info.muuk_shield_wipe,
                        "missing_unit_upkeep.shield_wipe");
  }

  return save_ruleset_file(sfile, filename);
}

/**************************************************************************
  Effect saving callback data structure.
**************************************************************************/
typedef struct {
  int idx;
  struct section_file *sfile;
} effect_cb_data;

/**********************************************************************//**
  Save one effect. Callback called for each effect in cache.
**************************************************************************/
static bool effect_save(struct effect *peffect, void *data)
{
  effect_cb_data *cbdata = (effect_cb_data *)data;
  char path[512];

  fc_snprintf(path, sizeof(path), "effect_%d", cbdata->idx++);

  secfile_insert_str(cbdata->sfile,
                     effect_type_name(peffect->type),
                     "%s.type", path);
  secfile_insert_int(cbdata->sfile, peffect->value, "%s.value", path);

  save_reqs_vector(cbdata->sfile, &peffect->reqs, path, "reqs");

  return TRUE;
}

/**********************************************************************//**
  Save effects.ruleset
**************************************************************************/
static bool save_effects_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "effect");
  effect_cb_data data;

  if (sfile == NULL) {
    return FALSE;
  }

  data.idx = 0;
  data.sfile = sfile;

  comment_effects(sfile);

  if (!iterate_effect_cache(effect_save, &data)) {
    return FALSE;
  }

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Auto attack should only require war, remaining movement and the absence
  of blocking utype flags.
**************************************************************************/
static bool unexpected_auto_attack(const struct requirement *req)
{
  return !((req->source.kind == VUT_DIPLREL
            && req->source.value.diplrel == DS_WAR
            && req->present)
           || (req->source.kind == VUT_MINMOVES
               && req->source.value.minmoves == 1
               && req->present));
}

/**********************************************************************//**
  Save ui_name of one action.
**************************************************************************/
static bool save_action_ui_name(struct section_file *sfile,
                                int act, const char *entry_name)
{
  const char *ui_name = action_by_number(act)->ui_name;

  if (strcmp(ui_name, action_ui_name_default(act))) {
    secfile_insert_str(sfile, ui_name,
                       "actions.%s", entry_name);
  }

  return TRUE;
}

/**********************************************************************//**
  Save game.ruleset
**************************************************************************/
static bool save_game_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "game");
  int sect_idx;
  int col_idx;
  int set_count;
  enum gameloss_style gs;
  const char *style_names[32]; /* FIXME: Should determine max length automatically.
                                * currently it's 3 (bits 0,1, and 2) so there's plenty of
                                * safety margin here. */
  const char *tnames[game.server.ruledit.named_teams];
  enum trade_route_type trt;
  int i;
  enum gen_action quiet_actions[MAX_NUM_ACTIONS];
  bool locks;
  int force_capture_units, force_bombard, force_explode_nuclear;

  if (sfile == NULL) {
    return FALSE;
  }

  if (game.server.ruledit.description_file != NULL) {
    secfile_insert_str(sfile, game.server.ruledit.description_file,
                       "ruledit.description_file");
  }

  if (game.control.preferred_tileset[0] != '\0') {
    secfile_insert_str(sfile, game.control.preferred_tileset,
                       "tileset.preferred");
  }
  if (game.control.preferred_soundset[0] != '\0') {
    secfile_insert_str(sfile, game.control.preferred_soundset,
                       "soundset.preferred");
  }
  if (game.control.preferred_musicset[0] != '\0') {
    secfile_insert_str(sfile, game.control.preferred_musicset,
                       "musicset.preferred");
  }

  secfile_insert_str(sfile, game.control.name, "about.name");
  secfile_insert_str(sfile, game.control.version, "about.version");

  if (game.ruleset_summary != NULL) {
    struct entry *mod_entry;

    mod_entry = secfile_insert_str(sfile, game.ruleset_summary,
                                   "about.summary");
    entry_str_set_gt_marking(mod_entry, TRUE);
  }

  if (game.ruleset_description != NULL) {
    if (game.server.ruledit.description_file == NULL) {
      secfile_insert_str(sfile, game.ruleset_description,
                         "about.description");
    } else {
      secfile_insert_filereference(sfile, game.server.ruledit.description_file,
                                   "about.description");
    }
  }

  if (game.ruleset_capabilities != NULL) {
    secfile_insert_str(sfile, game.ruleset_capabilities,
                       "about.capabilities");
  } else {
    secfile_insert_str(sfile, "", "about.capabilities");
  }

  save_tech_list(sfile, game.rgame.global_init_techs,
                 "options", "global_init_techs");
  save_building_list(sfile, game.rgame.global_init_buildings,
                     "options", "global_init_buildings");

  save_default_bool(sfile, game.control.popup_tech_help,
                    FALSE,
                    "options.popup_tech_help", NULL);
  save_default_int(sfile, game.info.base_pollution,
                   RS_DEFAULT_BASE_POLLUTION,
                   "civstyle.base_pollution", NULL);

  set_count = 0;
  for (gs = gameloss_style_begin(); gs != gameloss_style_end(); gs = gameloss_style_next(gs)) {
    if (game.info.gameloss_style & gs) {
      style_names[set_count++] = gameloss_style_name(gs);
    }
  }

  if (set_count > 0) {
    secfile_insert_str_vec(sfile, style_names, set_count,
                           "civstyle.gameloss_style");
  }

  save_default_int(sfile, game.info.happy_cost,
                   RS_DEFAULT_HAPPY_COST,
                   "civstyle.happy_cost", NULL);
  save_default_int(sfile, game.info.food_cost,
                   RS_DEFAULT_FOOD_COST,
                   "civstyle.food_cost", NULL);
  save_default_bool(sfile, game.info.civil_war_enabled,
                    TRUE,
                    "civstyle.civil_war_enabled", NULL);
  save_default_bool(sfile, game.info.paradrop_to_transport,
                    FALSE,
                    "civstyle.paradrop_to_transport", NULL);
  save_default_int(sfile, game.info.base_bribe_cost,
                   RS_DEFAULT_BASE_BRIBE_COST,
                   "civstyle.base_bribe_cost", NULL);
  save_default_int(sfile, game.server.ransom_gold,
                   RS_DEFAULT_RANSOM_GOLD,
                   "civstyle.ransom_gold", NULL);
  save_default_bool(sfile, game.info.pillage_select,
                    RS_DEFAULT_PILLAGE_SELECT,
                    "civstyle.pillage_select", NULL);
  save_default_bool(sfile, game.info.tech_steal_allow_holes,
                    RS_DEFAULT_TECH_STEAL_HOLES,
                    "civstyle.tech_steal_allow_holes", NULL);
  save_default_bool(sfile, game.info.tech_trade_allow_holes,
                    RS_DEFAULT_TECH_TRADE_HOLES,
                    "civstyle.tech_trade_allow_holes", NULL);
  save_default_bool(sfile, game.info.tech_trade_loss_allow_holes,
                    RS_DEFAULT_TECH_TRADE_LOSS_HOLES,
                    "civstyle.tech_trade_loss_allow_holes", NULL);
  save_default_bool(sfile, game.info.tech_parasite_allow_holes,
                    RS_DEFAULT_TECH_PARASITE_HOLES,
                    "civstyle.tech_parasite_allow_holes", NULL);
  save_default_bool(sfile, game.info.tech_loss_allow_holes,
                    RS_DEFAULT_TECH_LOSS_HOLES,
                    "civstyle.tech_loss_allow_holes", NULL);
  save_default_int(sfile, game.server.upgrade_veteran_loss,
                   RS_DEFAULT_UPGRADE_VETERAN_LOSS,
                   "civstyle.upgrade_veteran_loss", NULL);
  save_default_int(sfile, game.server.autoupgrade_veteran_loss,
                   RS_DEFAULT_UPGRADE_VETERAN_LOSS,
                   "civstyle.autoupgrade_veteran_loss", NULL);

  secfile_insert_int_vec(sfile, game.info.granary_food_ini,
                         game.info.granary_num_inis,
                         "civstyle.granary_food_ini");

  save_default_int(sfile, game.info.granary_food_inc,
                   RS_DEFAULT_GRANARY_FOOD_INC,
                   "civstyle.granary_food_inc", NULL);

  output_type_iterate(o) {
    char buffer[256];

    fc_snprintf(buffer, sizeof(buffer),
                "civstyle.min_city_center_%s",
                get_output_identifier(o));

    save_default_int(sfile, game.info.min_city_center_output[o],
                     RS_DEFAULT_CITY_CENTER_OUTPUT,
                     buffer, NULL);
  } output_type_iterate_end;

  save_default_int(sfile, game.server.init_vis_radius_sq,
                   RS_DEFAULT_VIS_RADIUS_SQ,
                   "civstyle.init_vis_radius_sq", NULL);
  save_default_int(sfile, game.info.init_city_radius_sq,
                   RS_DEFAULT_CITY_RADIUS_SQ,
                   "civstyle.init_city_radius_sq", NULL);
  if (0 != fc_strcasecmp(gold_upkeep_style_name(game.info.gold_upkeep_style),
                         RS_DEFAULT_GOLD_UPKEEP_STYLE)) {
    secfile_insert_str(sfile,
                       gold_upkeep_style_name(game.info.gold_upkeep_style),
                       "civstyle.gold_upkeep_style");
  }
  save_default_bool(sfile, game.info.illness_on,
                    RS_DEFAULT_ILLNESS_ON,
                    "illness.illness_on", NULL);
  save_default_int(sfile, game.info.illness_base_factor,
                   RS_DEFAULT_ILLNESS_BASE_FACTOR,
                   "illness.illness_base_factor", NULL);
  save_default_int(sfile, game.info.illness_min_size,
                   RS_DEFAULT_ILLNESS_MIN_SIZE,
                   "illness.illness_min_size", NULL);
  save_default_int(sfile, game.info.illness_trade_infection,
                   RS_DEFAULT_ILLNESS_TRADE_INFECTION_PCT,
                   "illness.illness_trade_infection", NULL);
  save_default_int(sfile, game.info.illness_pollution_factor,
                   RS_DEFAULT_ILLNESS_POLLUTION_PCT,
                   "illness.illness_pollution_factor", NULL);
  save_default_int(sfile, game.server.base_incite_cost,
                   RS_DEFAULT_INCITE_BASE_COST,
                   "incite_cost.base_incite_cost", NULL);
  save_default_int(sfile, game.server.incite_improvement_factor,
                   RS_DEFAULT_INCITE_IMPROVEMENT_FCT,
                   "incite_cost.improvement_factor", NULL);
  save_default_int(sfile, game.server.incite_unit_factor,
                   RS_DEFAULT_INCITE_UNIT_FCT,
                   "incite_cost.unit_factor", NULL);
  save_default_int(sfile, game.server.incite_total_factor,
                   RS_DEFAULT_INCITE_TOTAL_FCT,
                   "incite_cost.total_factor", NULL);
  save_default_bool(sfile, game.info.slow_invasions,
                    RS_DEFAULT_SLOW_INVASIONS,
                    "global_unit_options.slow_invasions", NULL);

  save_action_auto_uflag_block(sfile, ACTION_AUTO_MOVED_ADJ,
                               "auto_attack.will_never",
                               unexpected_auto_attack);

  save_default_bool(sfile,
                    action_id_would_be_blocked_by(ACTION_MARKETPLACE,
                                                  ACTION_TRADE_ROUTE),
                    RS_DEFAULT_FORCE_TRADE_ROUTE,
                    "actions.force_trade_route", NULL);

  /* The ruleset options force_capture_units, force_bombard and
   * force_explode_nuclear sets their respective action to block many other
   * actions' blocked_by. Checking one is therefore enough. */
  force_capture_units =
      BV_ISSET(action_by_number(ACTION_ATTACK)->blocked_by,
               ACTION_CAPTURE_UNITS);
  save_default_bool(sfile, force_capture_units,
                    RS_DEFAULT_FORCE_CAPTURE_UNITS,
                    "actions.force_capture_units", NULL);
  force_bombard =
      BV_ISSET(action_by_number(ACTION_ATTACK)->blocked_by, ACTION_BOMBARD);
  save_default_bool(sfile, force_bombard,
                    RS_DEFAULT_FORCE_BOMBARD,
                    "actions.force_bombard", NULL);
  force_explode_nuclear =
      BV_ISSET(action_by_number(ACTION_ATTACK)->blocked_by, ACTION_NUKE);
  save_default_bool(sfile, force_explode_nuclear,
                    RS_DEFAULT_FORCE_EXPLODE_NUCLEAR,
                    "actions.force_explode_nuclear", NULL);

  save_default_bool(sfile, game.info.poison_empties_food_stock,
                    RS_DEFAULT_POISON_EMPTIES_FOOD_STOCK,
                    "actions.poison_empties_food_stock", NULL);

  if (action_by_number(ACTION_BOMBARD)->max_distance
      == ACTION_DISTANCE_UNLIMITED) {
    secfile_insert_str(sfile, RS_ACTION_NO_MAX_DISTANCE,
                       "actions.bombard_max_range");
  } else {
    save_default_int(sfile, action_by_number(ACTION_BOMBARD)->max_distance,
                     RS_DEFAULT_BOMBARD_MAX_RANGE,
                     "actions.bombard_max_range", NULL);
  }

  action_iterate(act_id) {
    save_action_ui_name(sfile,
                        act_id, action_ui_name_ruleset_var_name(act_id));
  } action_iterate_end;

  i = 0;
  action_iterate(act) {
    if (action_by_number(act)->quiet) {
      quiet_actions[i] = act;
      i++;
    }
  } action_iterate_end;

  if (secfile_insert_enum_vec(sfile, &quiet_actions, i, gen_action,
                              "actions.quiet_actions") != i) {
    log_error("Didn't save all quiet actions.");

    return FALSE;
  }

  comment_enablers(sfile);
  sect_idx = 0;
  action_enablers_iterate(pae) {
    char path[512];

    fc_snprintf(path, sizeof(path), "actionenabler_%d", sect_idx++);

    secfile_insert_str(sfile, action_id_rule_name(pae->action),
                       "%s.action", path);

    save_reqs_vector(sfile, &(pae->actor_reqs), path, "actor_reqs");
    save_reqs_vector(sfile, &(pae->target_reqs), path, "target_reqs");
  } action_enablers_iterate_end;

  save_default_bool(sfile, game.info.tired_attack,
                    RS_DEFAULT_TIRED_ATTACK,
                    "combat_rules.tired_attack", NULL);
  save_default_int(sfile, game.info.border_city_radius_sq,
                   RS_DEFAULT_BORDER_RADIUS_SQ_CITY,
                   "borders.radius_sq_city", NULL);
  save_default_int(sfile, game.info.border_size_effect,
                   RS_DEFAULT_BORDER_SIZE_EFFECT,
                   "borders.size_effect", NULL);
  save_default_int(sfile, game.info.border_city_permanent_radius_sq,
                   RS_DEFAULT_BORDER_RADIUS_SQ_CITY_PERMANENT,
                   "borders.radius_sq_city_permanent", NULL);
  secfile_insert_str(sfile, tech_cost_style_name(game.info.tech_cost_style),
                     "research.tech_cost_style");
  save_default_int(sfile, game.info.base_tech_cost,
                   RS_DEFAULT_BASE_TECH_COST,
                   "research.base_tech_cost", NULL);
  secfile_insert_str(sfile, tech_leakage_style_name(game.info.tech_leakage),
                     "research.tech_leakage");
  secfile_insert_str(sfile, tech_upkeep_style_name(game.info.tech_upkeep_style),
                     "research.tech_upkeep_style");
  save_default_int(sfile, game.info.tech_upkeep_divider,
                   RS_DEFAULT_TECH_UPKEEP_DIVIDER,
                   "research.tech_upkeep_divider", NULL);
  secfile_insert_str(sfile, free_tech_method_name(game.info.free_tech_method),
                     "research.free_tech_method");

  save_default_int(sfile, game.info.culture_vic_points,
                   RS_DEFAULT_CULTURE_VIC_POINTS,
                   "culture.victory_min_points", NULL);
  save_default_int(sfile, game.info.culture_vic_lead,
                   RS_DEFAULT_CULTURE_VIC_LEAD,
                   "culture.victory_lead_pct", NULL);
  save_default_int(sfile, game.info.culture_migration_pml,
                   RS_DEFAULT_CULTURE_MIGRATION_PML,
                   "culture.migration_pml", NULL);

  save_default_bool(sfile, game.calendar.calendar_skip_0,
                    RS_DEFAULT_CALENDAR_SKIP_0,
                    "calendar.skip_year_0", NULL);
  save_default_int(sfile, game.server.start_year,
                   GAME_START_YEAR,
                   "calendar.start_year", NULL);
  save_default_int(sfile, game.calendar.calendar_fragments,
                   0, "calendar.fragments", NULL);

  for (i = 0; i < MAX_CALENDAR_FRAGMENTS; i++) {
    if (game.calendar.calendar_fragment_name[i][0] != '\0') {
      secfile_insert_str(sfile, game.calendar.calendar_fragment_name[i],
                         "calendar.fragment_name%d", i);
    }
  }

  if (strcmp(game.calendar.positive_year_label, RS_DEFAULT_POS_YEAR_LABEL)) {
    secfile_insert_str(sfile, game.calendar.positive_year_label,
                       "calendar.positive_label");
  }
  if (strcmp(game.calendar.negative_year_label, RS_DEFAULT_NEG_YEAR_LABEL)) {
    secfile_insert_str(sfile, game.calendar.negative_year_label,
                       "calendar.negative_label");
  }

  if (game.plr_bg_color != NULL) {
    rgbcolor_save(sfile, game.plr_bg_color, "playercolors.background");
  }

  col_idx = 0;
  rgbcolor_list_iterate(game.server.plr_colors, pcol) {
    rgbcolor_save(sfile, pcol, "playercolors.colorlist%d", col_idx++);
  } rgbcolor_list_iterate_end;


  if (game.server.ruledit.named_teams > 0) {
    for (i = 0; i < game.server.ruledit.named_teams; i++) {
      tnames[i] = team_slot_rule_name(team_slot_by_number(i));
    }

    secfile_insert_str_vec(sfile, tnames,
                           game.server.ruledit.named_teams,
                           "teams.names");
  }

  comment_disasters(sfile);

  sect_idx = 0;
  disaster_type_iterate(pd) {
    char path[512];
    enum disaster_effect_id de;
    const char *effect_names[DE_COUNT];

    fc_snprintf(path, sizeof(path), "disaster_%d", sect_idx++);

    save_name_translation(sfile, &(pd->name), path);
    save_reqs_vector(sfile, &(pd->reqs), path, "reqs");
    if (pd->frequency != GAME_DEFAULT_DISASTER_FREQ) {
      secfile_insert_int(sfile, pd->frequency,
                         "%s.frequency", path);
    }

    set_count = 0;
    for (de = disaster_effect_id_begin();
         de != disaster_effect_id_end();
         de = disaster_effect_id_next(de)) {
      if (BV_ISSET(pd->effects, de)) {
        effect_names[set_count++] = disaster_effect_id_name(de);
      }
    }

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, effect_names, set_count,
                             "%s.effects", path);
    }
  } disaster_type_iterate_end;

  comment_achievements(sfile);

  sect_idx = 0;
  achievements_iterate(pach) {
    char path[512];

    fc_snprintf(path, sizeof(path), "achievement_%d", sect_idx++);

    save_name_translation(sfile, &(pach->name), path);

    secfile_insert_str(sfile, achievement_type_name(pach->type),
                       "%s.type", path);

    save_default_bool(sfile, pach->unique,
                      GAME_DEFAULT_ACH_UNIQUE,
                      path, "unique");
    save_default_int(sfile, pach->value,
                     GAME_DEFAULT_ACH_VALUE,
                     path, "value");
    save_default_int(sfile, pach->culture,
                     0, path, "culture");

    secfile_insert_str(sfile, pach->first_msg, "%s.first_msg", path);
    if (pach->cons_msg != NULL) {
      secfile_insert_str(sfile, pach->cons_msg, "%s.cons_msg", path);
    }

  } achievements_iterate_end;

  set_count = 0;
  for (trt = 0; trt < TRT_LAST; trt++) {
    struct trade_route_settings *set = trade_route_settings_by_type(trt);
    const char *cancelling = traderoute_cancelling_type_name(set->cancelling);

    if (set->trade_pct != 100 || strcmp(cancelling, "Active")) {
      char path[256];

      fc_snprintf(path, sizeof(path),
                  "trade.settings%d", set_count++);

      secfile_insert_str(sfile, trade_route_type_name(trt),
                         "%s.type", path);
      secfile_insert_int(sfile, set->trade_pct,
                         "%s.pct", path);
      secfile_insert_str(sfile, cancelling,
                         "%s.cancelling", path);
      secfile_insert_str(sfile, traderoute_bonus_type_name(set->bonus_type),
                         "%s.bonus", path);
    }
  }

  /* Goods */
  comment_goods(sfile);

  sect_idx = 0;
  goods_type_re_active_iterate(pgood) {
    char path[512];
    const char *flag_names[GF_COUNT];
    int flagi;

    fc_snprintf(path, sizeof(path), "goods_%d", sect_idx++);

    save_name_translation(sfile, &(pgood->name), path);

    save_reqs_vector(sfile, &(pgood->reqs), path, "reqs");

    save_default_int(sfile, pgood->from_pct, 100, path, "from_pct");
    save_default_int(sfile, pgood->to_pct, 100, path, "to_pct");
    save_default_int(sfile, pgood->onetime_pct, 100, path, "onetime_pct");

    set_count = 0;
    for (flagi = 0; flagi < GF_COUNT; flagi++) {
      if (goods_has_flag(pgood, flagi)) {
        flag_names[set_count++] = goods_flag_id_name(flagi);
      }
    }

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, flag_names, set_count,
                             "%s.flags", path);
    }

    save_strvec(sfile, pgood->helptext, path, "helptext");
  } goods_type_re_active_iterate_end;

  /* Clauses */
  comment_clauses(sfile);

  sect_idx = 0;
  for (i = 0; i < CLAUSE_COUNT; i++) {
    struct clause_info *info = clause_info_get(i);

    if (info->enabled) {
      char path[512];

      fc_snprintf(path, sizeof(path), "clause_%d", sect_idx++);

      secfile_insert_str(sfile, clause_type_name(info->type),
                         "%s.type", path);
    }
  }

  locks = FALSE;
  settings_iterate(SSET_ALL, pset) {
    if (setting_locked(pset)) {
      locks = TRUE;
      break;
    }
  } settings_iterate_end;

  set_count = 0;
  settings_iterate(SSET_ALL, pset) {
    if (setting_get_setdef(pset) == SETDEF_RULESET || setting_locked(pset)) {
      secfile_insert_str(sfile, setting_name(pset),
                         "settings.set%d.name", set_count);
      switch (setting_type(pset)) {
      case SST_BOOL:
        secfile_insert_bool(sfile, setting_bool_get(pset),
                            "settings.set%d.value", set_count);
        break;
      case SST_INT:
        secfile_insert_int(sfile, setting_int_get(pset),
                           "settings.set%d.value", set_count);
        break;
      case SST_STRING:
        secfile_insert_str(sfile, setting_str_get(pset),
                           "settings.set%d.value", set_count);
        break;
      case SST_ENUM:
        secfile_insert_enum_data(sfile, read_enum_value(pset), FALSE,
                                 setting_enum_secfile_str, pset,
                                 "settings.set%d.value", set_count);
        break;
      case SST_BITWISE:
        secfile_insert_enum_data(sfile, setting_bitwise_get(pset), TRUE,
                                 setting_bitwise_secfile_str, pset,
                                 "settings.set%d.value", set_count);
        break;
      case SST_COUNT:
        fc_assert(setting_type(pset) != SST_COUNT);
        secfile_insert_str(sfile, "Unknown setting type",
                           "settings.set%d.value", set_count);
        break;
      }

      if (locks) {
        secfile_insert_bool(sfile, setting_locked(pset),
                            "settings.set%d.lock", set_count);
      }

      set_count++;
    }
  } settings_iterate_end;

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save governments.ruleset
**************************************************************************/
static bool save_governments_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "government");
  int sect_idx;

  if (sfile == NULL) {
    return FALSE;
  }

  save_gov_ref(sfile, game.government_during_revolution, "governments",
               "during_revolution");

  comment_govs(sfile);

  sect_idx = 0;
  governments_re_active_iterate(pg) {
    char path[512];
    struct ruler_title *prtitle;

    fc_snprintf(path, sizeof(path), "government_%d", sect_idx++);

    save_name_translation(sfile, &(pg->name), path);

    secfile_insert_str(sfile, pg->graphic_str, "%s.graphic", path);
    secfile_insert_str(sfile, pg->graphic_alt, "%s.graphic_alt", path);

    save_reqs_vector(sfile, &(pg->reqs), path, "reqs");

    if (pg->ai.better != NULL) {
      save_gov_ref(sfile, pg->ai.better, path,
                   "ai_better");
    }

    ruler_title_hash_lookup(pg->ruler_titles, NULL,
                            &prtitle);
    if (prtitle != NULL) {
      const char *title;

      title = ruler_title_male_untranslated_name(prtitle);
      if (title != NULL) {
        secfile_insert_str(sfile, title,
                           "%s.ruler_male_title", path);
      }

      title = ruler_title_female_untranslated_name(prtitle);
      if (title != NULL) {
        secfile_insert_str(sfile, title,
                           "%s.ruler_female_title", path);
      }
    }

    save_strvec(sfile, pg->helptext, path, "helptext");

  } governments_re_active_iterate_end;

  comment_policies(sfile);

  sect_idx = 0;
  multipliers_iterate(pmul) {
    if (!pmul->ruledit_disabled) {
      char path[512];

      fc_snprintf(path, sizeof(path), "multiplier_%d", sect_idx++);

      save_name_translation(sfile, &(pmul->name), path);

      secfile_insert_int(sfile, pmul->start, "%s.start", path);
      secfile_insert_int(sfile, pmul->stop, "%s.stop", path);
      secfile_insert_int(sfile, pmul->step, "%s.step", path);
      secfile_insert_int(sfile, pmul->def, "%s.default", path);

      save_reqs_vector(sfile, &(pmul->reqs), path, "reqs");

      save_strvec(sfile, pmul->helptext, path, "helptext");
    }
  } multipliers_iterate_end;

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save list of AI traits
**************************************************************************/
static bool save_traits(struct trait_limits *traits,
                        struct trait_limits *default_traits,
                        struct section_file *sfile,
                        const char *secname, const char *field_prefix)
{
  enum trait tr;

 /* FIXME: Use specenum trait names without duplicating them here.
   *        Just needs to take care of case. */
  const char *trait_names[] = {
    "expansionist",
    "trader",
    "aggressive",
    NULL
  };

  for (tr = trait_begin(); tr != trait_end() && trait_names[tr] != NULL;
       tr = trait_next(tr)) {
    int default_default;

    default_default = (traits[tr].min + traits[tr].max) / 2;

    if ((default_traits == NULL && traits[tr].min != TRAIT_DEFAULT_VALUE)
        || (default_traits != NULL && traits[tr].min != default_traits[tr].min)) {
      secfile_insert_int(sfile, traits[tr].min, "%s.%s%s_min", secname, field_prefix,
                         trait_names[tr]);
    }
    if ((default_traits == NULL && traits[tr].max != TRAIT_DEFAULT_VALUE)
        || (default_traits != NULL && traits[tr].max != default_traits[tr].max)) {
      secfile_insert_int(sfile, traits[tr].max, "%s.%s%s_max", secname, field_prefix,
                         trait_names[tr]);
    }
    if (default_default != traits[tr].fixed) {
      secfile_insert_int(sfile, traits[tr].fixed, "%s.%s%s_default", secname, field_prefix,
                         trait_names[tr]);
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Save a single nation.
**************************************************************************/
static bool save_nation(struct section_file *sfile, struct nation_type *pnat,
                        int sect_idx)
{
  char path[512];
  int max_items = nation_city_list_size(pnat->server.default_cities);
  char *city_str[max_items];
  max_items = MAX(max_items, MAX_NUM_NATION_SETS + MAX_NUM_NATION_GROUPS);
  max_items = MAX(max_items, game.control.nation_count);
  const char *list_items[max_items];
  int set_count;
  int subsect_idx;

  fc_snprintf(path, sizeof(path), "nation_%d", sect_idx++);

  if (pnat->translation_domain == NULL) {
    secfile_insert_str(sfile, "freeciv", "%s.translation_domain", path);
  } else {
    secfile_insert_str(sfile, pnat->translation_domain, "%s.translation_domain", path);
  }

  save_name_translation(sfile, &(pnat->adjective), path);
  secfile_insert_str(sfile, untranslated_name(&(pnat->noun_plural)), "%s.plural", path);

  set_count = 0;
  nation_sets_iterate(pset) {
    if (nation_is_in_set(pnat, pset)) {
      list_items[set_count++] = nation_set_rule_name(pset);
    }
  } nation_sets_iterate_end;
  nation_groups_iterate(pgroup) {
    if (nation_is_in_group(pnat, pgroup)) {
      list_items[set_count++] = nation_group_rule_name(pgroup);
    } 
  } nation_groups_iterate_end;

  if (set_count > 0) {
    secfile_insert_str_vec(sfile, list_items, set_count, "%s.groups", path);
  }

  set_count = 0;
  nation_list_iterate(pnat->server.conflicts_with, pconfl) {
    list_items[set_count++] = nation_rule_name(pconfl);
  } nation_list_iterate_end;
  if (set_count > 0) {
    secfile_insert_str_vec(sfile, list_items, set_count, "%s.conflicts_with", path);
  }

  subsect_idx = 0;
  nation_leader_list_iterate(pnat->leaders, pleader) {
    secfile_insert_str(sfile, nation_leader_name(pleader), "%s.leaders%d.name",
                       path, subsect_idx);
    secfile_insert_str(sfile, nation_leader_is_male(pleader) ? "Male" : "Female",
                       "%s.leaders%d.sex", path, subsect_idx++);
  } nation_leader_list_iterate_end;

  if (pnat->server.rgb != NULL) {
    rgbcolor_save(sfile, pnat->server.rgb, "%s.color", path);
  }

  save_traits(pnat->server.traits, game.server.default_traits,
              sfile, path, "trait_");

  if (!pnat->is_playable) {
    secfile_insert_bool(sfile, pnat->is_playable, "%s.is_playable", path);
  }

  if (pnat->barb_type != NOT_A_BARBARIAN) {
    secfile_insert_str(sfile, barbarian_type_name(pnat->barb_type),
                       "%s.barbarian_type", path);
  }

  if (strcmp(pnat->flag_graphic_str, "-")) {
    secfile_insert_str(sfile, pnat->flag_graphic_str, "%s.flag", path);
  }
  if (strcmp(pnat->flag_graphic_alt, "-")) {
    secfile_insert_str(sfile, pnat->flag_graphic_alt, "%s.flag_alt", path);
  }

  subsect_idx = 0;
  governments_iterate(pgov) {
    struct ruler_title *prtitle;

    if (ruler_title_hash_lookup(pgov->ruler_titles, pnat, &prtitle)) {
      secfile_insert_str(sfile, government_rule_name(pgov),
                         "%s.ruler_titles%d.government", path, subsect_idx);
      secfile_insert_str(sfile, ruler_title_male_untranslated_name(prtitle),
                         "%s.ruler_titles%d.male_title", path, subsect_idx);
      secfile_insert_str(sfile, ruler_title_female_untranslated_name(prtitle),
                         "%s.ruler_titles%d.female_title", path, subsect_idx++);
    }
  } governments_iterate_end;

  secfile_insert_str(sfile, style_rule_name(pnat->style), "%s.style", path);

  set_count = 0;
  nation_list_iterate(pnat->server.civilwar_nations, pconfl) {
    list_items[set_count++] = nation_rule_name(pconfl);
  } nation_list_iterate_end;
  if (set_count > 0) {
    secfile_insert_str_vec(sfile, list_items, set_count, "%s.civilwar_nations", path);
  }

  save_tech_list(sfile, pnat->init_techs, path, "init_techs");
  save_building_list(sfile, pnat->init_buildings, path, "init_buildings");
  save_unit_list(sfile, pnat->init_units, path, "init_units");

  if (pnat->init_government) {
    secfile_insert_str(sfile, government_rule_name(pnat->init_government),
                       "%s.init_government", path);
  }

  set_count = 0;
  nation_city_list_iterate(pnat->server.default_cities, pncity) {
    bool list_started = FALSE;

    city_str[set_count] = fc_malloc(strlen(nation_city_name(pncity)) + strlen(" (!river")
                                    + strlen(")")
                                    + MAX_NUM_TERRAINS * (strlen(", ") + MAX_LEN_NAME));

    strcpy(city_str[set_count], nation_city_name(pncity));
    switch(nation_city_river_preference(pncity)) {
    case NCP_DISLIKE:
      strcat(city_str[set_count], " (!river");
      list_started = TRUE;
      break;
    case NCP_LIKE:
      strcat(city_str[set_count], " (river");
      list_started = TRUE;
      break;
    case NCP_NONE:
      break;
    }

    terrain_type_iterate(pterr) {
      const char *pref = NULL;

      switch(nation_city_terrain_preference(pncity, pterr)) {
      case NCP_DISLIKE:
        pref = "!";
        break;
      case NCP_LIKE:
        pref = "";
        break;
      case NCP_NONE:
        pref = NULL;
        break;
      }

      if (pref != NULL) {
        if (list_started) {
          strcat(city_str[set_count], ", ");
        } else {
          strcat(city_str[set_count], " (");
          list_started = TRUE;
        }
        strcat(city_str[set_count], pref);
        strcat(city_str[set_count], terrain_rule_name(pterr));
      }

    } terrain_type_iterate_end;

    if (list_started) {
      strcat(city_str[set_count], ")");
    }

    list_items[set_count] = city_str[set_count];
    set_count++;
  } nation_city_list_iterate_end;
  if (set_count > 0) {
    int i;

    secfile_insert_str_vec(sfile, list_items, set_count, "%s.cities", path);

    for (i = 0; i < set_count; i++) {
      FC_FREE(city_str[i]);
    }
  }

  secfile_insert_str(sfile, pnat->legend, "%s.legend", path);

  return TRUE;
}

/**********************************************************************//**
  Save nations.ruleset
**************************************************************************/
static bool save_nations_ruleset(const char *filename, const char *name,
                                 struct rule_data *data)
{
  struct section_file *sfile = create_ruleset_file(name, "nation");

  if (sfile == NULL) {
    return FALSE;
  }

  if (data->nationlist != NULL) {
    secfile_insert_str(sfile, data->nationlist, "ruledit.nationlist");
  }
  if (game.server.ruledit.embedded_nations != NULL) {
    int i;
    const char **tmp = fc_malloc(game.server.ruledit.embedded_nations_count * sizeof(char *));

    /* Dance around the secfile_insert_str_vec() parameter type (requires extra const)
     * resrictions */
    for (i = 0; i < game.server.ruledit.embedded_nations_count; i++) {
      tmp[i] = game.server.ruledit.embedded_nations[i];
    }

    secfile_insert_str_vec(sfile, tmp,
                           game.server.ruledit.embedded_nations_count,
                           "ruledit.embedded_nations");
    free(tmp);
  }

  save_traits(game.server.default_traits, NULL, sfile,
              "default_traits", "");

  if (data->nationlist == NULL) {
    if (game.server.ruledit.allowed_govs != NULL) {
      secfile_insert_str_vec(sfile, game.server.ruledit.allowed_govs,
                             game.server.ruledit.ag_count,
                             "compatibility.allowed_govs");
    }
    if (game.server.ruledit.allowed_terrains != NULL) {
      secfile_insert_str_vec(sfile, game.server.ruledit.allowed_terrains,
                             game.server.ruledit.at_count,
                             "compatibility.allowed_terrains");
    }
    if (game.server.ruledit.allowed_styles != NULL) {
      secfile_insert_str_vec(sfile, game.server.ruledit.allowed_styles,
                             game.server.ruledit.as_count,
                             "compatibility.allowed_styles");
    }
  }

  if (game.default_government != NULL) {
    secfile_insert_str(sfile, government_rule_name(game.default_government),
                       "compatibility.default_government");
  }

  if (data->nationlist != NULL) {
    secfile_insert_include(sfile, data->nationlist);

    if (game.server.ruledit.embedded_nations != NULL) {
      int sect_idx;

      comment_nations(sfile);

      for (sect_idx = 0; sect_idx < game.server.ruledit.embedded_nations_count;
           sect_idx++) {
        struct nation_type *pnat
          = nation_by_rule_name(game.server.ruledit.embedded_nations[sect_idx]);

        if (pnat == NULL) {
          log_error("Embedded nation \"%s\" not found!",
                    game.server.ruledit.embedded_nations[sect_idx]);
        } else {
          save_nation(sfile, pnat, sect_idx);
        }
      }
    }
  } else {
    int sect_idx = 0;

    comment_nationsets(sfile);

    nation_sets_iterate(pset) {
      char path[512];

      fc_snprintf(path, sizeof(path), "nset_%d", sect_idx++);

      /* We don't use save_name_translation() for this as name and rule_name must
       * always be saved separately */
      secfile_insert_str(sfile, nation_set_untranslated_name(pset), "%s.name", path);
      secfile_insert_str(sfile, nation_set_rule_name(pset), "%s.rule_name", path);
      secfile_insert_str(sfile, nation_set_description(pset), "%s.description", path);
    } nation_sets_iterate_end;

    comment_nationgroups(sfile);

    sect_idx = 0;
    nation_groups_iterate(pgroup) {
      char path[512];

      fc_snprintf(path, sizeof(path), "ngroup_%d", sect_idx++);

      save_name_translation(sfile, &(pgroup->name), path);

      secfile_insert_int(sfile, pgroup->server.match, "%s.match", path);
      if (pgroup->hidden) {
        secfile_insert_bool(sfile, pgroup->hidden, "%s.hidden", path);
      }
    } nation_groups_iterate_end;

    comment_nations(sfile);

    sect_idx = 0;
    nations_iterate(pnat) {
      save_nation(sfile, pnat, sect_idx++);
    } nations_iterate_end;
  }

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save techs.ruleset
**************************************************************************/
static bool save_techs_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "tech");
  int i;
  int sect_idx;
  struct advance *a_none = advance_by_number(A_NONE);


  if (sfile == NULL) {
    return FALSE;
  }

  for (i = 0; i < MAX_NUM_USER_TECH_FLAGS; i++) {
    const char *flagname = tech_flag_id_name_cb(i + TECH_USER_1);
    const char *helptxt = tech_flag_helptxt(i + TECH_USER_1);

    if (flagname != NULL) {
      secfile_insert_str(sfile, flagname, "control.flags%d.name", i);

      /* Save the user flag help text even when it is undefined. That makes
       * the formatting code happy. The resulting "" is ignored when the
       * ruleset is loaded. */
      secfile_insert_str(sfile, helptxt, "control.flags%d.helptxt", i);
    }
  }

  comment_tech_classes(sfile);

  sect_idx = 0;
  tech_class_iterate(ptclass) {
    char path[512];

    fc_snprintf(path, sizeof(path), "techclass_%d", sect_idx++);

    save_name_translation(sfile, &(ptclass->name), path);
  } tech_class_iterate_end;

  comment_techs(sfile);

  sect_idx = 0;
  advance_re_active_iterate(pa) {
    if (pa->require[AR_ONE] != A_NEVER) {
      char path[512];
      const char *flag_names[TF_COUNT];
      int set_count;
      int flagi;

      fc_snprintf(path, sizeof(path), "advance_%d", sect_idx++);

      save_name_translation(sfile, &(pa->name), path);

      if (game.control.num_tech_classes > 0) {
        if (pa->tclass != NULL) {
          secfile_insert_str(sfile, tech_class_rule_name(pa->tclass),
                             "%s.class", path);
        }
      }

      save_tech_ref(sfile, pa->require[AR_ONE], path, "req1");
      save_tech_ref(sfile, pa->require[AR_TWO], path, "req2");
      if (pa->require[AR_ROOT] != a_none && !pa->inherited_root_req) {
        save_tech_ref(sfile, pa->require[AR_ROOT], path, "root_req");
      }

      save_reqs_vector(sfile, &(pa->research_reqs), path,
                       "research_reqs");

      secfile_insert_str(sfile, pa->graphic_str, "%s.graphic", path);
      if (strcmp("-", pa->graphic_alt)) {
        secfile_insert_str(sfile, pa->graphic_alt, "%s.graphic_alt", path);
      }
      if (pa->bonus_message != NULL) {
        secfile_insert_str(sfile, pa->bonus_message, "%s.bonus_message", path);
      }

      set_count = 0;
      for (flagi = 0; flagi < TF_COUNT; flagi++) {
        if (advance_has_flag(advance_index(pa), flagi)) {
          flag_names[set_count++] = tech_flag_id_name(flagi);
        }
      }

      if (set_count > 0) {
        secfile_insert_str_vec(sfile, flag_names, set_count,
                               "%s.flags", path);
      }
      if (pa->cost >= 0) {
        secfile_insert_int(sfile, pa->cost, "%s.cost", path);
      }

      save_strvec(sfile, pa->helptext, path, "helptext");
    }

  } advance_re_active_iterate_end;

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save terrain.ruleset
**************************************************************************/
static bool save_terrain_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "terrain");
  int sect_idx;
  int i;

  if (sfile == NULL) {
    return FALSE;
  }

  for (i = 0; i < MAX_NUM_USER_TER_FLAGS; i++) {
    const char *flagname = terrain_flag_id_name_cb(i + TER_USER_1);
    const char *helptxt = terrain_flag_helptxt(i + TER_USER_1);

    if (flagname != NULL) {
      secfile_insert_str(sfile, flagname, "control.flags%d.name", i);

      /* Save the user flag help text even when it is undefined. That makes
       * the formatting code happy. The resulting "" is ignored when the
       * ruleset is loaded. */
      secfile_insert_str(sfile, helptxt, "control.flags%d.helptxt", i);
    }
  }

  for (i = 0; i < MAX_NUM_USER_EXTRA_FLAGS; i++) {
    const char *flagname = extra_flag_id_name_cb(i + EF_USER_FLAG_1);
    const char *helptxt = extra_flag_helptxt(i + EF_USER_FLAG_1);

    if (flagname != NULL) {
      secfile_insert_str(sfile, flagname, "control.extra_flags%d.name", i);

      /* Save the user flag help text even when it is undefined. That makes
       * the formatting code happy. The resulting "" is ignored when the
       * ruleset is loaded. */
      secfile_insert_str(sfile, helptxt,
                         "control.extra_flags%d.helptxt", i);
    }
  }

  if (terrain_control.ocean_reclaim_requirement_pct <= 100) {
    secfile_insert_int(sfile, terrain_control.ocean_reclaim_requirement_pct,
                       "parameters.ocean_reclaim_requirement");
  }
  if (terrain_control.land_channel_requirement_pct <= 100) {
    secfile_insert_int(sfile, terrain_control.land_channel_requirement_pct,
                       "parameters.land_channel_requirement");
  }
  if (terrain_control.terrain_thaw_requirement_pct <= 100) {
    secfile_insert_int(sfile, terrain_control.terrain_thaw_requirement_pct,
                       "parameters.thaw_requirement");
  }
  if (terrain_control.terrain_freeze_requirement_pct <= 100) {
    secfile_insert_int(sfile, terrain_control.terrain_freeze_requirement_pct,
                       "parameters.freeze_requirement");
  }
  if (terrain_control.lake_max_size != 0) {
    secfile_insert_int(sfile, terrain_control.lake_max_size,
                       "parameters.lake_max_size");
  }
  if (terrain_control.min_start_native_area != 0) {
    secfile_insert_int(sfile, terrain_control.min_start_native_area,
                       "parameters.min_start_native_area");
  }
  if (terrain_control.move_fragments != 3) {
    secfile_insert_int(sfile, terrain_control.move_fragments,
                       "parameters.move_fragments");
  }
  if (terrain_control.igter_cost != 1) {
    secfile_insert_int(sfile, terrain_control.igter_cost,
                       "parameters.igter_cost");
  }
  if (terrain_control.pythagorean_diagonal != RS_DEFAULT_PYTHAGOREAN_DIAGONAL) {
    secfile_insert_bool(sfile, TRUE,
                        "parameters.pythagorean_diagonal");
  }
  if (wld.map.server.ocean_resources) {
    secfile_insert_bool(sfile, TRUE,
                       "parameters.ocean_resources");
  }

  comment_terrains(sfile);

  sect_idx = 0;
  terrain_type_iterate(pterr) {
    char path[512];
    char identifier[2];
    int r;
    const char *flag_names[TER_USER_LAST];
    const char *puc_names[UCL_LAST];
    int flagi;
    int set_count;

    fc_snprintf(path, sizeof(path), "terrain_%d", sect_idx++);

    save_name_translation(sfile, &(pterr->name), path);

    secfile_insert_str(sfile, pterr->graphic_str, "%s.graphic", path);
    secfile_insert_str(sfile, pterr->graphic_alt, "%s.graphic_alt", path);
    identifier[0] = pterr->identifier;
    identifier[1] = '\0';
    secfile_insert_str(sfile, identifier, "%s.identifier", path);

    secfile_insert_str(sfile, terrain_class_name(pterr->tclass),
                       "%s.class", path);

    secfile_insert_int(sfile, pterr->movement_cost, "%s.movement_cost", path);
    secfile_insert_int(sfile, pterr->defense_bonus, "%s.defense_bonus", path);

    output_type_iterate(o) {
      if (pterr->output[o] != 0) {
        secfile_insert_int(sfile, pterr->output[o], "%s.%s", path,
                           get_output_identifier(o));
      }
    } output_type_iterate_end;

    /* Check resource count */
    for (r = 0; pterr->resources[r] != NULL; r++) {
      /* Just increasing r as long as there is resources */
    }

    {
      const char *resource_names[r];

      for (r = 0; pterr->resources[r] != NULL; r++) {
        resource_names[r] = extra_rule_name(pterr->resources[r]);
      }

      secfile_insert_str_vec(sfile, resource_names, r,
                             "%s.resources", path);
    }

    output_type_iterate(o) {
      if (pterr->road_output_incr_pct[o] != 0) {
        secfile_insert_int(sfile, pterr->road_output_incr_pct[o],
                           "%s.road_%s_incr_pct", path,
                           get_output_identifier(o));
      }
    } output_type_iterate_end;

    secfile_insert_int(sfile, pterr->base_time, "%s.base_time", path);
    secfile_insert_int(sfile, pterr->road_time, "%s.road_time", path);

    save_terrain_ref(sfile, pterr->irrigation_result, pterr, path,
                     "irrigation_result");
    secfile_insert_int(sfile, pterr->irrigation_food_incr,
                       "%s.irrigation_food_incr", path);
    secfile_insert_int(sfile, pterr->irrigation_time,
                       "%s.irrigation_time", path);

    save_terrain_ref(sfile, pterr->mining_result, pterr, path,
                     "mining_result");
    secfile_insert_int(sfile, pterr->mining_shield_incr,
                       "%s.mining_shield_incr", path);
    secfile_insert_int(sfile, pterr->mining_time,
                       "%s.mining_time", path);

    save_terrain_ref(sfile, pterr->transform_result, pterr, path,
                     "transform_result");
    secfile_insert_int(sfile, pterr->transform_time,
                       "%s.transform_time", path);

    if (pterr->animal != NULL) {
      secfile_insert_str(sfile, utype_rule_name(pterr->animal),
                         "%s.animal", path);
    } else {
      secfile_insert_str(sfile, "None",
                         "%s.animal", path);
    }

    secfile_insert_int(sfile, pterr->pillage_time,
                       "%s.pillage_time", path);
    secfile_insert_int(sfile, pterr->clean_pollution_time,
                       "%s.clean_pollution_time", path);
    secfile_insert_int(sfile, pterr->clean_fallout_time,
                       "%s.clean_fallout_time", path);

    save_terrain_ref(sfile, pterr->warmer_wetter_result, pterr, path,
                     "warmer_wetter_result");
    save_terrain_ref(sfile, pterr->warmer_drier_result, pterr, path,
                     "warmer_drier_result");
    save_terrain_ref(sfile, pterr->cooler_wetter_result, pterr, path,
                     "cooler_wetter_result");
    save_terrain_ref(sfile, pterr->cooler_drier_result, pterr, path,
                     "cooler_drier_result");

    set_count = 0;
    for (flagi = 0; flagi < TER_USER_LAST; flagi++) {
      if (terrain_has_flag(pterr, flagi)) {
        flag_names[set_count++] = terrain_flag_id_name(flagi);
      }
    }

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, flag_names, set_count,
                             "%s.flags", path);
    }

    {
      enum mapgen_terrain_property mtp;

      for (mtp = mapgen_terrain_property_begin();
           mtp != mapgen_terrain_property_end();
           mtp = mapgen_terrain_property_next(mtp)) {
        if (pterr->property[mtp] != 0) {
          secfile_insert_int(sfile, pterr->property[mtp],
                             "%s.property_%s", path,
                             mapgen_terrain_property_name(mtp));
        }
      }
    }

    set_count = 0;
    unit_class_iterate(puc) {
      if (BV_ISSET(pterr->native_to, uclass_index(puc))) {
        puc_names[set_count++] = uclass_rule_name(puc);
      }
    } unit_class_iterate_end;

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, puc_names, set_count,
                             "%s.native_to", path);
    }

    rgbcolor_save(sfile, pterr->rgb, "%s.color", path);

    save_strvec(sfile, pterr->helptext, path, "helptext");

  } terrain_type_iterate_end;

  comment_resources(sfile);

  sect_idx = 0;
  extra_type_by_cause_iterate(EC_RESOURCE, pres) {
    if (!pres->ruledit_disabled) {
      char path[512];
      char identifier[2];

      fc_snprintf(path, sizeof(path), "resource_%d", sect_idx++);

      secfile_insert_str(sfile, extra_rule_name(pres),
                         "%s.extra", path);

      output_type_iterate(o) {
        if (pres->data.resource->output[o] != 0) {
          secfile_insert_int(sfile, pres->data.resource->output[o], "%s.%s",
                             path, get_output_identifier(o));
        }
      } output_type_iterate_end;

      identifier[0] = pres->data.resource->id_old_save;
      identifier[1] = '\0';
      secfile_insert_str(sfile, identifier, "%s.identifier", path);
    }
  } extra_type_by_cause_iterate_end;

  secfile_insert_str(sfile, terrain_control.gui_type_base0,
                     "extraui.ui_name_base_fortress");
  secfile_insert_str(sfile, terrain_control.gui_type_base1,
                     "extraui.ui_name_base_airbase");

  comment_extras(sfile);

  sect_idx = 0;
  extra_type_re_active_iterate(pextra) {
    char path[512];
    const char *flag_names[EF_COUNT];
    const char *cause_names[EC_COUNT];
    const char *puc_names[UCL_LAST];
    const char *extra_names[MAX_EXTRA_TYPES];
    int flagi;
    int causei;
    int set_count;

    fc_snprintf(path, sizeof(path), "extra_%d", sect_idx++);

    save_name_translation(sfile, &(pextra->name), path);

    secfile_insert_str(sfile, extra_category_name(pextra->category),
                       "%s.category", path);

    set_count = 0;
    for (causei = 0; causei < EC_COUNT; causei++) {
      if (is_extra_caused_by(pextra, causei)) {
        cause_names[set_count++] = extra_cause_name(causei);
      }
    }

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, cause_names, set_count,
                             "%s.causes", path);
    }

    set_count = 0;
    for (causei = 0; causei < ERM_COUNT; causei++) {
      if (is_extra_removed_by(pextra, causei)) {
        cause_names[set_count++] = extra_rmcause_name(causei);
      }
    }

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, cause_names, set_count,
                             "%s.rmcauses", path);
    }

    if (strcmp(pextra->graphic_str, "-")) {
      secfile_insert_str(sfile, pextra->graphic_str, "%s.graphic", path);
    }
    if (strcmp(pextra->graphic_alt, "-")) {
      secfile_insert_str(sfile, pextra->graphic_alt, "%s.graphic_alt", path);
    }
    if (strcmp(pextra->activity_gfx, "-")) {
      secfile_insert_str(sfile, pextra->activity_gfx, "%s.activity_gfx", path);
    }
    if (strcmp(pextra->act_gfx_alt, "-")) {
      secfile_insert_str(sfile, pextra->act_gfx_alt, "%s.act_gfx_alt", path);
    }
    if (strcmp(pextra->act_gfx_alt2, "-")) {
      secfile_insert_str(sfile, pextra->act_gfx_alt2, "%s.act_gfx_alt2", path);
    }
    if (strcmp(pextra->rmact_gfx, "-")) {
      secfile_insert_str(sfile, pextra->rmact_gfx, "%s.rmact_gfx", path);
    }
    if (strcmp(pextra->rmact_gfx_alt, "-")) {
      secfile_insert_str(sfile, pextra->rmact_gfx_alt, "%s.rmact_gfx_alt", path);
    }

    save_reqs_vector(sfile, &(pextra->reqs), path, "reqs");
    save_reqs_vector(sfile, &(pextra->rmreqs), path, "rmreqs");
    save_reqs_vector(sfile, &(pextra->appearance_reqs), path, "appearance_reqs");
    save_reqs_vector(sfile, &(pextra->disappearance_reqs), path, "disappearance_reqs");

    if (!pextra->buildable) {
      secfile_insert_bool(sfile, pextra->buildable, "%s.buildable", path);
    }
    if (!pextra->generated) {
      secfile_insert_bool(sfile, pextra->generated, "%s.generated", path);
    }
    secfile_insert_int(sfile, pextra->build_time, "%s.build_time", path);
    secfile_insert_int(sfile, pextra->removal_time, "%s.removal_time", path);
    if (pextra->build_time_factor != 1) {
      secfile_insert_int(sfile, pextra->build_time_factor, "%s.build_time_factor", path);
    }
    if (pextra->removal_time_factor != 1) {
      secfile_insert_int(sfile, pextra->removal_time_factor, "%s.removal_time_factor", path);
    }
    if (pextra->defense_bonus != 0) {
      secfile_insert_int(sfile, pextra->defense_bonus, "%s.defense_bonus", path);
    }
    if (pextra->eus != EUS_NORMAL) {
      secfile_insert_str(sfile, extra_unit_seen_type_name(pextra->eus),
                         "%s.unit_seen", path);
    }
    if (is_extra_caused_by(pextra, EC_APPEARANCE)
        && pextra->appearance_chance != RS_DEFAULT_EXTRA_APPEARANCE) {
      secfile_insert_int(sfile, pextra->appearance_chance, "%s.appearance_chance", path);
    }
    if (is_extra_removed_by(pextra, ERM_DISAPPEARANCE)
        && pextra->disappearance_chance != RS_DEFAULT_EXTRA_DISAPPEARANCE) {
      secfile_insert_int(sfile, pextra->disappearance_chance, "%s.disappearance_chance",
                         path);
    }

    set_count = 0;
    unit_class_iterate(puc) {
      if (BV_ISSET(pextra->native_to, uclass_index(puc))) {
        puc_names[set_count++] = uclass_rule_name(puc);
      }
    } unit_class_iterate_end;

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, puc_names, set_count,
                             "%s.native_to", path);
    }

    set_count = 0;
    for (flagi = 0; flagi < EF_COUNT; flagi++) {
      if (extra_has_flag(pextra, flagi)) {
        flag_names[set_count++] = extra_flag_id_name(flagi);
      }
    }

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, flag_names, set_count,
                             "%s.flags", path);
    }

    set_count = 0;
    extra_type_iterate(confl) {
      if (!can_extras_coexist(pextra, confl)) {
        extra_names[set_count++] = extra_rule_name(confl);
      }
    } extra_type_iterate_end;

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, extra_names, set_count,
                             "%s.conflicts", path);
    }

    set_count = 0;
    extra_type_iterate(top) {
      if (BV_ISSET(pextra->hidden_by, extra_index(top))) {
        extra_names[set_count++] = extra_rule_name(top);
      }
    } extra_type_iterate_end;

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, extra_names, set_count,
                             "%s.hidden_by", path);
    }

    set_count = 0;
    extra_type_iterate(top) {
      if (BV_ISSET(pextra->bridged_over, extra_index(top))) {
        extra_names[set_count++] = extra_rule_name(top);
      }
    } extra_type_iterate_end;

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, extra_names, set_count,
                             "%s.bridged_over", path);
    }

    save_strvec(sfile, pextra->helptext, path, "helptext");

  } extra_type_re_active_iterate_end;

  comment_bases(sfile);

  sect_idx = 0;
  extra_type_by_cause_iterate(EC_BASE, pextra) {
    if (!pextra->ruledit_disabled) {
      char path[512];
      struct base_type *pbase = extra_base_get(pextra);
      const char *flag_names[BF_COUNT];
      int flagi;
      int set_count;

      fc_snprintf(path, sizeof(path), "base_%d", sect_idx++);

      secfile_insert_str(sfile, extra_rule_name(pextra),
                         "%s.extra", path);

      secfile_insert_str(sfile, base_gui_type_name(pbase->gui_type),
                         "%s.gui_type", path);

      if (pbase->border_sq >= 0) {
        secfile_insert_int(sfile, pbase->border_sq, "%s.border_sq", path);
      }
      if (pbase->vision_main_sq >= 0) {
        secfile_insert_int(sfile, pbase->vision_main_sq, "%s.vision_main_sq", path);
      }
      if (pbase->vision_invis_sq >= 0) {
        secfile_insert_int(sfile, pbase->vision_invis_sq, "%s.vision_invis_sq", path);
      }
      if (pbase->vision_subs_sq >= 0) {
        secfile_insert_int(sfile, pbase->vision_subs_sq, "%s.vision_subs_sq", path);
      }

      set_count = 0;
      for (flagi = 0; flagi < BF_COUNT; flagi++) {
        if (base_has_flag(pbase, flagi)) {
          flag_names[set_count++] = base_flag_id_name(flagi);
        }
      }

      if (set_count > 0) {
        secfile_insert_str_vec(sfile, flag_names, set_count,
                               "%s.flags", path);
      }
    }
  } extra_type_by_cause_iterate_end;

  comment_roads(sfile);

  sect_idx = 0;
  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    if (!pextra->ruledit_disabled) {
      struct road_type *proad = extra_road_get(pextra);
      char path[512];
      const char *flag_names[RF_COUNT];
      int flagi;
      int set_count;

      fc_snprintf(path, sizeof(path), "road_%d", sect_idx++);

      secfile_insert_str(sfile, extra_rule_name(pextra),
                         "%s.extra", path);

      secfile_insert_int(sfile, proad->move_cost, "%s.move_cost", path);

      if (proad->move_mode != RMM_FAST_ALWAYS) {
        secfile_insert_str(sfile, road_move_mode_name(proad->move_mode),
                           "%s.move_mode", path);
      }

      output_type_iterate(o) {
        if (proad->tile_incr_const[o] != 0) {
          secfile_insert_int(sfile, proad->tile_incr_const[o],
                             "%s.%s_incr_const", path, get_output_identifier(o));
        }
        if (proad->tile_incr[o] != 0) {
          secfile_insert_int(sfile, proad->tile_incr[o],
                             "%s.%s_incr", path, get_output_identifier(o));
        }
        if (proad->tile_bonus[o] != 0) {
          secfile_insert_int(sfile, proad->tile_bonus[o],
                             "%s.%s_bonus", path, get_output_identifier(o));
        }
      } output_type_iterate_end;

      switch (proad->compat) {
      case ROCO_ROAD:
        secfile_insert_str(sfile, "Road", "%s.compat_special", path);
        break;
      case ROCO_RAILROAD:
        secfile_insert_str(sfile, "Railroad", "%s.compat_special", path);
        break;
      case ROCO_RIVER:
        secfile_insert_str(sfile, "River", "%s.compat_special", path);
        break;
      case ROCO_NONE:
        secfile_insert_str(sfile, "None", "%s.compat_special", path);
        break;
      }

      set_count = 0;
      for (flagi = 0; flagi < RF_COUNT; flagi++) {
        if (road_has_flag(proad, flagi)) {
          flag_names[set_count++] = road_flag_id_name(flagi);
        }
      }

      if (set_count > 0) {
        secfile_insert_str_vec(sfile, flag_names, set_count,
                               "%s.flags", path);
      }
    }
  } extra_type_by_cause_iterate_end;

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save one veteran system.
**************************************************************************/
static bool save_veteran_system(struct section_file *sfile, const char *path,
                                struct veteran_system *vsystem)
{
  const char *vlist_name[vsystem->levels];
  int vlist_power[vsystem->levels];
  int vlist_raise[vsystem->levels];
  int vlist_wraise[vsystem->levels];
  int vlist_move[vsystem->levels];
  int i;

  for (i = 0; i < vsystem->levels; i++) {
    vlist_name[i] = rule_name_get(&(vsystem->definitions[i].name));
    vlist_power[i] = vsystem->definitions[i].power_fact;
    vlist_raise[i] = vsystem->definitions[i].raise_chance;
    vlist_wraise[i] = vsystem->definitions[i].work_raise_chance;
    vlist_move[i] = vsystem->definitions[i].move_bonus;
  }

  secfile_insert_str_vec(sfile, vlist_name, vsystem->levels,
                         "%s.veteran_names", path);
  secfile_insert_int_vec(sfile, vlist_power, vsystem->levels,
                         "%s.veteran_power_fact", path);
  secfile_insert_int_vec(sfile, vlist_raise, vsystem->levels,
                         "%s.veteran_raise_chance", path);
  secfile_insert_int_vec(sfile, vlist_wraise, vsystem->levels,
                         "%s.veteran_work_raise_chance", path);
  secfile_insert_int_vec(sfile, vlist_move, vsystem->levels,
                         "%s.veteran_move_bonus", path);

  return TRUE;
}

/**********************************************************************//**
  Save unit combat bonuses list.
**************************************************************************/
static bool save_combat_bonuses(struct section_file *sfile,
                                struct unit_type *put,
                                char *path)
{
  int i;
  bool has_quiet = FALSE;

  combat_bonus_list_iterate(put->bonuses, pbonus) {
    if (pbonus->quiet) {
      has_quiet = TRUE;
    }
  } combat_bonus_list_iterate_end;

  i = 0;

  combat_bonus_list_iterate(put->bonuses, pbonus) {
    secfile_insert_str(sfile, unit_type_flag_id_name(pbonus->flag),
                       "%s.bonuses%d.flag", path, i);
    secfile_insert_str(sfile, combat_bonus_type_name(pbonus->type),
                       "%s.bonuses%d.type", path, i);
    secfile_insert_int(sfile, pbonus->value,
                       "%s.bonuses%d.value", path, i);

    if (has_quiet) {
      secfile_insert_bool(sfile, pbonus->quiet,
                          "%s.bonuses%d.quiet", path, i);
    }

    i++;
  } combat_bonus_list_iterate_end;

  return TRUE;
}

/**********************************************************************//**
  Save units.ruleset
**************************************************************************/
static bool save_units_ruleset(const char *filename, const char *name)
{
  struct section_file *sfile = create_ruleset_file(name, "unit");
  int i;
  int sect_idx;

  if (sfile == NULL) {
    return FALSE;
  }

  for (i = 0; i < MAX_NUM_USER_UNIT_FLAGS; i++) {
    const char *flagname = unit_type_flag_id_name_cb(i + UTYF_USER_FLAG_1);
    const char *helptxt = unit_type_flag_helptxt(i + UTYF_USER_FLAG_1);

    if (flagname != NULL) {
      secfile_insert_str(sfile, flagname, "control.flags%d.name", i);

      /* Save the user flag help text even when it is undefined. That makes
       * the formatting code happy. The resulting "" is ignored when the
       * ruleset is loaded. */
      secfile_insert_str(sfile, helptxt, "control.flags%d.helptxt", i);
    }
  }

  for (i = 0; i < MAX_NUM_USER_UCLASS_FLAGS; i++) {
    const char *flagname = unit_class_flag_id_name_cb(i + UCF_USER_FLAG_1);
    const char *helptxt = unit_class_flag_helptxt(i + UCF_USER_FLAG_1);

    if (flagname != NULL) {
      secfile_insert_str(sfile, flagname, "control.class_flags%d.name", i);

      /* Save the user flag help text even when it is undefined. That makes
       * the formatting code happy. The resulting "" is ignored when the
       * ruleset is loaded. */
      secfile_insert_str(sfile, helptxt,
                         "control.class_flags%d.helptxt", i);
    }
  }

  save_veteran_system(sfile, "veteran_system", game.veteran);

  comment_uclasses(sfile);

  sect_idx = 0;
  unit_class_re_active_iterate(puc) {
    char path[512];
    char *hut_str = NULL;
    const char *flag_names[UCF_COUNT];
    int flagi;
    int set_count;

    fc_snprintf(path, sizeof(path), "unitclass_%d", sect_idx++);

    save_name_translation(sfile, &(puc->name), path);

    secfile_insert_int(sfile, puc->min_speed / SINGLE_MOVE,
                       "%s.min_speed", path);
    secfile_insert_int(sfile, puc->hp_loss_pct, "%s.hp_loss_pct", path);
    if (puc->non_native_def_pct != 100) {
      secfile_insert_int(sfile, puc->non_native_def_pct,
                         "%s.non_native_def_pct", path);
    }
    if (puc->hut_behavior != HUT_NORMAL) {
      switch (puc->hut_behavior) {
      case HUT_NORMAL:
        hut_str = "Normal";
        break;
      case HUT_NOTHING:
        hut_str = "Nothing";
        break;
      case HUT_FRIGHTEN:
        hut_str = "Frighten";
        break;
      }
      fc_assert(hut_str != NULL);
      secfile_insert_str(sfile, hut_str, "%s.hut_behavior", path);
    }

    set_count = 0;
    for (flagi = 0; flagi < UCF_COUNT; flagi++) {
      if (uclass_has_flag(puc, flagi)) {
        flag_names[set_count++] = unit_class_flag_id_name(flagi);
      }
    }

    if (set_count > 0) {
      secfile_insert_str_vec(sfile, flag_names, set_count,
                             "%s.flags", path);
    }

    save_strvec(sfile, puc->helptext, path, "helptext");

  } unit_class_re_active_iterate_end;

  comment_utypes(sfile);

  sect_idx = 0;
  unit_type_re_active_iterate(put) {
    if (!put->ruledit_disabled) {
      char path[512];
      const char *flag_names[UTYF_LAST_USER_FLAG + 1];
      int flagi;
      int set_count;

      fc_snprintf(path, sizeof(path), "unit_%d", sect_idx++);

      save_name_translation(sfile, &(put->name), path);

      secfile_insert_str(sfile, uclass_rule_name(put->uclass),
                         "%s.class", path);

      save_tech_ref(sfile, put->require_advance, path, "tech_req");

      if (put->need_government != NULL) {
        secfile_insert_str(sfile, government_rule_name(put->need_government),
                           "%s.gov_req", path);
      }

      if (put->need_improvement != NULL) {
        secfile_insert_str(sfile, improvement_rule_name(put->need_improvement),
                           "%s.impr_req", path);
      }

      if (put->obsoleted_by != NULL) {
        secfile_insert_str(sfile, utype_rule_name(put->obsoleted_by),
                           "%s.obsolete_by", path);
      }

      secfile_insert_str(sfile, put->graphic_str, "%s.graphic", path);
      if (strcmp("-", put->graphic_alt)) {
        secfile_insert_str(sfile, put->graphic_alt, "%s.graphic_alt", path);
      }
      if (strcmp("-", put->sound_move)) {
        secfile_insert_str(sfile, put->sound_move, "%s.sound_move", path);
      }
      if (strcmp("-", put->sound_move_alt)) {
        secfile_insert_str(sfile, put->sound_move_alt, "%s.sound_move_alt", path);
      }
      if (strcmp("-", put->sound_fight)) {
        secfile_insert_str(sfile, put->sound_fight, "%s.sound_fight", path);
      }
      if (strcmp("-", put->sound_fight_alt)) {
        secfile_insert_str(sfile, put->sound_fight_alt, "%s.sound_fight_alt", path);
      }

      secfile_insert_int(sfile, put->build_cost, "%s.build_cost", path);
      secfile_insert_int(sfile, put->pop_cost, "%s.pop_cost", path);
      secfile_insert_int(sfile, put->attack_strength, "%s.attack", path);
      secfile_insert_int(sfile, put->defense_strength, "%s.defense", path);
      secfile_insert_int(sfile, put->move_rate / SINGLE_MOVE, "%s.move_rate", path);
      secfile_insert_int(sfile, put->vision_radius_sq, "%s.vision_radius_sq", path);
      secfile_insert_int(sfile, put->transport_capacity, "%s.transport_cap", path);

      save_uclass_vec(sfile, &(put->cargo), path, "cargo", FALSE);
      save_uclass_vec(sfile, &(put->embarks), path, "embarks", TRUE);
      save_uclass_vec(sfile, &(put->disembarks), path, "disembarks", TRUE);

      if (put->vlayer != V_MAIN) {
        secfile_insert_str(sfile, vision_layer_name(put->vlayer),
                           "%s.vision_layer", path);
      }

      secfile_insert_int(sfile, put->hp, "%s.hitpoints", path);
      secfile_insert_int(sfile, put->firepower, "%s.firepower", path);
      secfile_insert_int(sfile, put->fuel, "%s.fuel", path);
      secfile_insert_int(sfile, put->happy_cost, "%s.uk_happy", path);

      output_type_iterate(o) {
        if (put->upkeep[o] != 0) {
          secfile_insert_int(sfile, put->upkeep[o], "%s.uk_%s",
                             path, get_output_identifier(o));
        }
      } output_type_iterate_end;

      if (put->converted_to != NULL) {
        secfile_insert_str(sfile, utype_rule_name(put->converted_to),
                           "%s.convert_to", path);
      }
      if (put->convert_time != 1) {
        secfile_insert_int(sfile, put->convert_time, "%s.convert_time", path);
      }

      save_combat_bonuses(sfile, put, path);
      save_uclass_vec(sfile, &(put->targets), path, "targets", TRUE);

      if (put->veteran != NULL) {
        save_veteran_system(sfile, path, put->veteran);
      }

      if (put->paratroopers_range != 0) {
        secfile_insert_int(sfile, put->paratroopers_range,
                           "%s.paratroopers_range", path);
        secfile_insert_int(sfile, put->paratroopers_mr_req / SINGLE_MOVE,
                           "%s.paratroopers_mr_req", path);
        secfile_insert_int(sfile, put->paratroopers_mr_sub / SINGLE_MOVE,
                           "%s.paratroopers_mr_sub", path);
      }
      if (put->bombard_rate != 0) {
        secfile_insert_int(sfile, put->bombard_rate,
                           "%s.bombard_rate", path);
      }
      if (put->city_slots != 0) {
        secfile_insert_int(sfile, put->city_slots,
                           "%s.city_slots", path);
      }
      if (put->city_size != 1) {
        secfile_insert_int(sfile, put->city_size,
                           "%s.city_size", path);
      }

      set_count = 0;
      for (flagi = 0; flagi <= UTYF_LAST_USER_FLAG; flagi++) {
        if (utype_has_flag(put, flagi)) {
          flag_names[set_count++] = unit_type_flag_id_name(flagi);
        }
      }

      if (set_count > 0) {
        secfile_insert_str_vec(sfile, flag_names, set_count,
                               "%s.flags", path);
      }

      set_count = 0;
      for (flagi = L_FIRST; flagi < L_LAST; flagi++) {
        if (utype_has_role(put, flagi)) {
          flag_names[set_count++] = unit_role_id_name(flagi);
        }
      }

      if (set_count > 0) {
        secfile_insert_str_vec(sfile, flag_names, set_count,
                               "%s.roles", path);
      }

      save_strvec(sfile, put->helptext, path, "helptext");
    }
  } unit_type_re_active_iterate_end;

  return save_ruleset_file(sfile, filename);
}

/**********************************************************************//**
  Save script.lua
**************************************************************************/
static bool save_script_lua(const char *filename, const char *name,
                            const char *buffer)
{
  if (buffer != NULL) {
    FILE *ffile = fc_fopen(filename, "w");
    int full_len = strlen(buffer);
    int len;

    if (ffile != NULL) {
      len = fwrite(buffer, 1, full_len, ffile);

      if (len != full_len) {
        return FALSE;
      }

      fclose(ffile);
    } else {
      return FALSE;
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Save luadata.txt
**************************************************************************/
static bool save_luadata(const char *filename)
{
  if (game.server.luadata != NULL) {
    return secfile_save(game.server.luadata, filename, 0, FZ_PLAIN);
  }

  return TRUE;
}

/**********************************************************************//**
  Save ruleset to directory given.
**************************************************************************/
bool save_ruleset(const char *path, const char *name, struct rule_data *data)
{
  if (make_dir(path)) {
    bool success = TRUE;
    char filename[500];

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/buildings.ruleset", path);
      success = save_buildings_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/styles.ruleset", path);
      success = save_styles_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/cities.ruleset", path);
      success = save_cities_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/effects.ruleset", path);
      success = save_effects_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/game.ruleset", path);
      success = save_game_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/governments.ruleset", path);
      success = save_governments_ruleset(filename, name);
    }
    
    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/nations.ruleset", path);
      success = save_nations_ruleset(filename, name, data);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/techs.ruleset", path);
      success = save_techs_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/terrain.ruleset", path);
      success = save_terrain_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/units.ruleset", path);
      success = save_units_ruleset(filename, name);
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/script.lua", path);
      success = save_script_lua(filename, name, get_script_buffer());
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/parser.lua", path);
      success = save_script_lua(filename, name, get_parser_buffer());
    }

    if (success) {
      fc_snprintf(filename, sizeof(filename), "%s/luadata.txt", path);
      success = save_luadata(filename);
    }

    return success;
  } else {
    log_error("Failed to create directory %s", path);
    return FALSE;
  }

  return TRUE;
}
