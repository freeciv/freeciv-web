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

/* ANSI */
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* utility */
#include "capability.h"
#include "log.h"
#include "registry.h"

/* common */
#include "actions.h"
#include "effects.h"
#include "game.h"
#include "requirements.h"
#include "unittype.h"

/* server */
#include "rssanity.h"
#include "ruleset.h"

#include "rscompat.h"

/**********************************************************************//**
  Initialize rscompat information structure
**************************************************************************/
void rscompat_init_info(struct rscompat_info *info)
{
  memset(info, 0, sizeof(*info));
}

/**********************************************************************//**
  Ruleset files should have a capabilities string datafile.options
  This checks the string and that the required capabilities are satisfied.
**************************************************************************/
int rscompat_check_capabilities(struct section_file *file,
                                const char *filename,
                                struct rscompat_info *info)
{
  const char *datafile_options;
  bool ok = FALSE;

  if (!(datafile_options = secfile_lookup_str(file, "datafile.options"))) {
    log_fatal("\"%s\": ruleset capability problem:", filename);
    ruleset_error(LOG_ERROR, "%s", secfile_error());

    return 0;
  }

  if (info->compat_mode) {
    /* Check alternative capstr first, so that when we do the main capstr check,
     * we already know that failures there are fatal (error message correct, can return
     * immediately) */

    if (has_capabilities(RULESET_COMPAT_CAP, datafile_options)
        && has_capabilities(datafile_options, RULESET_COMPAT_CAP)) {
      ok = TRUE;
    }
  }

  if (!ok) {
    if (!has_capabilities(RULESET_CAPABILITIES, datafile_options)) {
      log_fatal("\"%s\": ruleset datafile appears incompatible:", filename);
      log_fatal("  datafile options: %s", datafile_options);
      log_fatal("  supported options: %s", RULESET_CAPABILITIES);
      ruleset_error(LOG_ERROR, "Capability problem");

      return 0;
    }
    if (!has_capabilities(datafile_options, RULESET_CAPABILITIES)) {
      log_fatal("\"%s\": ruleset datafile claims required option(s)"
                " that we don't support:", filename);
      log_fatal("  datafile options: %s", datafile_options);
      log_fatal("  supported options: %s", RULESET_CAPABILITIES);
      ruleset_error(LOG_ERROR, "Capability problem");

      return 0;
    }
  }

  return secfile_lookup_int_default(file, 1, "datafile.format_version");
}

/**********************************************************************//**
  Find and return the first unused unit type user flag. If all unit type
  user flags are taken MAX_NUM_USER_UNIT_FLAGS is returned.
**************************************************************************/
static int first_free_unit_type_user_flag(void)
{
  int flag;

  /* Find the first unused user defined unit type flag. */
  for (flag = 0; flag < MAX_NUM_USER_UNIT_FLAGS; flag++) {
    if (unit_type_flag_id_name_cb(flag + UTYF_USER_FLAG_1) == NULL) {
      return flag;
    }
  }

  /* All unit type user flags are taken. */
  return MAX_NUM_USER_UNIT_FLAGS;
}

/**********************************************************************//**
  Do compatibility things with names before they are referred to. Runs
  after names are loaded from the ruleset but before the ruleset objects
  that may refer to them are loaded.

  This is needed when previously hard coded items that are referred to in
  the ruleset them self becomes ruleset defined.

  Returns FALSE if an error occurs.
**************************************************************************/
bool rscompat_names(struct rscompat_info *info)
{
  if (info->ver_units < 20) {
    /* Some unit type flags moved to the ruleset between 3.0 and 3.1.
     * Add them back as user flags.
     * XXX: ruleset might not need all of these, and may have enough
     * flags of its own that these additional ones prevent conversion. */
    const struct {
      const char *name;
      const char *helptxt;
    } new_flags_31[] = {
      { N_("Infra"), N_("Can build infrastructure.") },
    };

    int first_free;
    int i;

    /* Unit type flags. */
    first_free = first_free_unit_type_user_flag() + UTYF_USER_FLAG_1;

    for (i = 0; i < ARRAY_SIZE(new_flags_31); i++) {
      if (UTYF_USER_FLAG_1 + MAX_NUM_USER_UNIT_FLAGS <= first_free + i) {
        /* Can't add the user unit type flags. */
        ruleset_error(LOG_ERROR,
                      "Can't upgrade the ruleset. Not enough free unit type "
                      "user flags to add user flags for the unit type flags "
                      "that used to be hardcoded.");
        return FALSE;
      }
      /* Shouldn't be possible for valid old ruleset to have flag names that
       * clash with these ones */
      if (unit_type_flag_id_by_name(new_flags_31[i].name, fc_strcasecmp)
          != unit_type_flag_id_invalid()) {
        ruleset_error(LOG_ERROR,
                      "Ruleset had illegal user unit type flag '%s'",
                      new_flags_31[i].name);
        return FALSE;
      }
      set_user_unit_type_flag_name(first_free + i,
                                   new_flags_31[i].name,
                                   new_flags_31[i].helptxt);
    }
  }

  /* No errors encountered. */
  return TRUE;
}

/**********************************************************************//**
  Handle a universal being separated from an original universal.

  A universal may be split into two new universals. An effect may mention
  the universal that now has been split in its requirement list. In that
  case two effect - one for the original and one for the universal being
  separated from it - are needed.

  Check if the original universal is mentioned in the requirement list of
  peffect. Handle creating one effect for the original and one for the
  universal that has been separated out if it is.
**************************************************************************/
static bool effect_handle_split_universal(struct effect *peffect,
                                          struct universal original,
                                          struct universal separated)
{
  if (universal_is_mentioned_by_requirements(&peffect->reqs, &original)) {
    /* Copy the old effect. */
    effect_copy(peffect);

    /* Replace the original requirement with the separated requirement. */
    return universal_replace_in_req_vec(&peffect->reqs,
                                        &original, &separated);
  }

  return FALSE;
}

/**********************************************************************//**
  Adjust effects
**************************************************************************/
static bool effect_list_compat_cb(struct effect *peffect, void *data)
{
  struct rscompat_info *info = (struct rscompat_info *)data;

  if (info->ver_effects < 20) {
    /* Attack has been split in regular "Attack" and "Suicide Attack". */
    effect_handle_split_universal(peffect,
        universal_by_number(VUT_ACTION, ACTION_ATTACK),
        universal_by_number(VUT_ACTION, ACTION_SUICIDE_ATTACK));
  }

  /* Go to the next effect. */
  return TRUE;
}

/**********************************************************************//**
  Turn old effect to an action enabler.
**************************************************************************/
static void effect_to_enabler(action_id action, struct section_file *file,
                              const char *sec_name, struct rscompat_info *compat,
                              const char *type)
{
  int value = secfile_lookup_int_default(file, 1, "%s.value", sec_name);
  char buf[1024];

  if (value > 0) {
    /* It was an enabling effect. Add enabler */
    struct action_enabler *enabler;
    struct requirement_vector *reqs;
    struct requirement settler_req;

    enabler = action_enabler_new();
    enabler->action = action;

    reqs = lookup_req_list(file, compat, sec_name, "reqs", "old effect");

    /* TODO: Divide requirements to actor_reqs and target_reqs depending
     *       their type. */
    requirement_vector_copy(&enabler->actor_reqs, reqs);

    settler_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, FALSE, TRUE, FALSE,
                                  UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, settler_req);

    /* Add the enabler to the ruleset. */
    action_enabler_add(enabler);

    if (compat->log_cb != NULL) {
      fc_snprintf(buf, sizeof(buf),
                  "Converted effect %s to an action enabler. Make sure requirements "
                  "are correctly divided to actor and target requirements.",
                  type);
      compat->log_cb(buf);
    }
  } else if (value < 0) {
    if (compat->log_cb != NULL) {
      fc_snprintf(buf, sizeof(buf),
                  "%s effect with negative value can't be automatically converted "
                  "to an action enabler. Do that manually.", type);
      compat->log_cb(buf);
    }
  }
}

/**********************************************************************//**
  Check if effect name refers to one of the removed effects, and handle it
  if it does. Returns TRUE iff name was a valid old name.
**************************************************************************/
bool rscompat_old_effect_3_1(const char *type, struct section_file *file,
                             const char *sec_name, struct rscompat_info *compat)
{
  if (compat->ver_effects < 20) {
    if (!fc_strcasecmp(type, "Transform_Possible")) {
      effect_to_enabler(ACTION_TRANSFORM_TERRAIN, file, sec_name, compat, type);
      return TRUE;
    }
    if (!fc_strcasecmp(type, "Irrig_TF_Possible")) {
      effect_to_enabler(ACTION_IRRIGATE_TF, file, sec_name, compat, type);
      return TRUE;
    }
    if (!fc_strcasecmp(type, "Mining_TF_Possible")) {
      effect_to_enabler(ACTION_MINE_TF, file, sec_name, compat, type);
      return TRUE;
    }
    if (!fc_strcasecmp(type, "Mining_Possible")) {
      effect_to_enabler(ACTION_MINE, file, sec_name, compat, type);
      return TRUE;
    }
    if (!fc_strcasecmp(type, "Irrig_Possible")) {
      effect_to_enabler(ACTION_IRRIGATE, file, sec_name, compat, type);
      return TRUE;
    }
  }

  return FALSE;
}

/**********************************************************************//**
  Do compatibility things after regular ruleset loading.
**************************************************************************/
void rscompat_postprocess(struct rscompat_info *info)
{
  if (!info->compat_mode) {
    /* There isn't anything here that should be done outside of compat
     * mode. */
    return;
  }

  /* Upgrade existing effects. Done before new effects are added to prevent
   * the new effects from being upgraded by accident. */
  iterate_effect_cache(effect_list_compat_cb, info);

  if (info->ver_units < 20) {
    unit_type_iterate(ptype) {
      if (utype_has_flag(ptype, UTYF_SETTLERS)) {
        int flag;

        flag = unit_type_flag_id_by_name("Infra", fc_strcasecmp);
        fc_assert(unit_type_flag_id_is_valid(flag));
        BV_SET(ptype->flags, flag);
      }
    } unit_type_iterate_end;
  }

  if (info->ver_game < 20) {
    /* New enablers */
    struct action_enabler *enabler;
    struct requirement e_req;

    enabler = action_enabler_new();
    enabler->action = ACTION_PILLAGE;
    e_req = req_from_values(VUT_UCFLAG, REQ_RANGE_LOCAL, FALSE, TRUE, FALSE,
                            UCF_CAN_PILLAGE);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_FORTIFY;
    e_req = req_from_values(VUT_UCFLAG, REQ_RANGE_LOCAL, FALSE, TRUE, FALSE,
                            UCF_CAN_FORTIFY);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_ROAD;
    e_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, FALSE, TRUE, FALSE,
                            UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_CONVERT;
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_BASE;
    e_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, FALSE, TRUE, FALSE,
                            UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    /* Update action enablers. */
    action_enablers_iterate(ae) {
      if (action_enabler_obligatory_reqs_missing(ae)) {
        /* Add previously implicit obligatory hard requirement(s). */
        action_enabler_obligatory_reqs_add(ae);
      }

      /* "Attack" is split in a unit consuming and a non unit consuming
       * version. */
      if (ae->action == ACTION_ATTACK) {
        /* The old rule is represented with two action enablers. */
        enabler = action_enabler_copy(ae);

        /* One allows regular attacks. */
        requirement_vector_append(&ae->actor_reqs,
                                  req_from_str("UnitClassFlag", "Local",
                                               FALSE, FALSE, TRUE,
                                               "Missile"));

        /* The other allows suicide attacks. */
        enabler->action = ACTION_SUICIDE_ATTACK;
        requirement_vector_append(&enabler->actor_reqs,
                                  req_from_str("UnitClassFlag", "Local",
                                               FALSE, TRUE, TRUE,
                                               "Missile"));

        /* Add after the action was changed. */
        action_enabler_add(enabler);
      }
    } action_enablers_iterate_end;

    /* Enable all clause types */
    {
      int i;

      for (i = 0; i < CLAUSE_COUNT; i++) {
        struct clause_info *cinfo = clause_info_get(i);

        cinfo->enabled = TRUE;
      }
    }
  }

  /* The ruleset may need adjustments it didn't need before compatibility
   * post processing.
   *
   * If this isn't done a user of ruleset compatibility that ends up using
   * the rules risks bad rules. A user that saves the ruleset rather than
   * using it risks an unexpected change on the next load and save. */
  autoadjust_ruleset_data();
}

/**********************************************************************//**
  Replace deprecated requirement type names with currently valid ones.

  The extra arguments are for situation where some, but not all, instances
  of a requirement type should become something else.
**************************************************************************/
const char *rscompat_req_type_name_3_1(const char *old_type,
                                       const char *old_range,
                                       bool old_survives, bool old_present,
                                       bool old_quiet,
                                       const char *old_value)
{
  return old_type;
}

/**********************************************************************//**
  Replace deprecated requirement type names with currently valid ones.

  The extra arguments are for situation where some, but not all, instances
  of a requirement type should become something else.
**************************************************************************/
const char *rscompat_req_name_3_1(const char *type,
                                  const char *old_name)
{
  if (!fc_strcasecmp("DiplRel", type)
      && !fc_strcasecmp("Is foreign", old_name)) {
    return "Foreign";
  }

  return old_name;
}

/**********************************************************************//**
  Replace deprecated unit type flag names with currently valid ones.
**************************************************************************/
const char *rscompat_utype_flag_name_3_1(struct rscompat_info *compat,
                                         const char *old_type)
{
  if (compat->compat_mode) {
  }

  return old_type;
}
