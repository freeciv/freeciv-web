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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "bitvector.h"
#include "deprecations.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "shared.h"
#include "string_vector.h"
#include "support.h"

/* common */
#include "achievements.h"
#include "actions.h"
#include "ai.h"
#include "base.h"
#include "capability.h"
#include "city.h"
#include "effects.h"
#include "extras.h"
#include "fc_types.h"
#include "featured_text.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "multipliers.h"
#include "name_translation.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "requirements.h"
#include "rgbcolor.h"
#include "road.h"
#include "specialist.h"
#include "style.h"
#include "tech.h"
#include "traderoutes.h"
#include "unit.h"
#include "unittype.h"

/* server */
#include "citytools.h"
#include "notify.h"
#include "plrhand.h"
#include "rscompat.h"
#include "rssanity.h"
#include "settings.h"
#include "srv_main.h"

/* server/advisors */
#include "advruleset.h"

/* server/scripting */
#include "script_server.h"

#include "ruleset.h"

/* RULESET_SUFFIX already used, no leading dot here */
#define RULES_SUFFIX "ruleset"
#define SCRIPT_SUFFIX "lua"

#define ADVANCE_SECTION_PREFIX "advance_"
#define TECH_CLASS_SECTION_PREFIX "techclass_"
#define BUILDING_SECTION_PREFIX "building_"
#define CITYSTYLE_SECTION_PREFIX "citystyle_"
#define MUSICSTYLE_SECTION_PREFIX "musicstyle_"
#define EFFECT_SECTION_PREFIX "effect_"
#define GOVERNMENT_SECTION_PREFIX "government_"
#define NATION_SET_SECTION_PREFIX "nset" /* without underscore? */
#define NATION_GROUP_SECTION_PREFIX "ngroup" /* without underscore? */
#define NATION_SECTION_PREFIX "nation" /* without underscore? */
#define STYLE_SECTION_PREFIX "style_"
#define CLAUSE_SECTION_PREFIX "clause_"
#define EXTRA_SECTION_PREFIX "extra_"
#define BASE_SECTION_PREFIX "base_"
#define ROAD_SECTION_PREFIX "road_"
#define RESOURCE_SECTION_PREFIX "resource_"
#define GOODS_SECTION_PREFIX "goods_"
#define SPECIALIST_SECTION_PREFIX "specialist_"
#define TERRAIN_SECTION_PREFIX "terrain_"
#define UNIT_CLASS_SECTION_PREFIX "unitclass_"
#define UNIT_SECTION_PREFIX "unit_"
#define DISASTER_SECTION_PREFIX "disaster_"
#define ACHIEVEMENT_SECTION_PREFIX "achievement_"
#define ACTION_ENABLER_SECTION_PREFIX "actionenabler_"
#define MULTIPLIER_SECTION_PREFIX "multiplier_"

#define check_name(name) (check_strlen(name, MAX_LEN_NAME, NULL))
#define check_cityname(name) (check_strlen(name, MAX_LEN_CITYNAME, NULL))

/* avoid re-reading files */
static const char name_too_long[] = "Name \"%s\" too long; truncating.";
#define MAX_SECTION_LABEL 64
#define section_strlcpy(dst, src) \
	(void) loud_strlcpy(dst, src, MAX_SECTION_LABEL, name_too_long)
static char *resource_sections = NULL;
static char *terrain_sections = NULL;
static char *extra_sections = NULL;
static char *base_sections = NULL;
static char *road_sections = NULL;

static struct requirement_vector reqs_list;

static bool load_rulesetdir(const char *rsdir, bool compat_mode,
                            rs_conversion_logger logger,
                            bool act, bool buffer_script);
static struct section_file *openload_ruleset_file(const char *whichset,
                                                  const char *rsdir);

static bool load_game_names(struct section_file *file,
                            struct rscompat_info *compat);
static bool load_tech_names(struct section_file *file,
                            struct rscompat_info *compat);
static bool load_unit_names(struct section_file *file,
                            struct rscompat_info *compat);
static bool load_building_names(struct section_file *file,
                                struct rscompat_info *compat);
static bool load_government_names(struct section_file *file,
                                  struct rscompat_info *compat);
static bool load_terrain_names(struct section_file *file,
                               struct rscompat_info *compat);
static bool load_style_names(struct section_file *file,
                             struct rscompat_info *compat);
static bool load_nation_names(struct section_file *file,
                              struct rscompat_info *compat);
static bool load_city_name_list(struct section_file *file,
                                struct nation_type *pnation,
                                const char *secfile_str1,
                                const char *secfile_str2,
                                const char **allowed_terrains,
                                size_t atcount);

static bool load_ruleset_techs(struct section_file *file,
                               struct rscompat_info *compat);
static bool load_ruleset_units(struct section_file *file,
                               struct rscompat_info *compat);
static bool load_ruleset_buildings(struct section_file *file,
                                   struct rscompat_info *compat);
static bool load_ruleset_governments(struct section_file *file,
                                     struct rscompat_info *compat);
static bool load_ruleset_terrain(struct section_file *file,
                                 struct rscompat_info *compat);
static bool load_ruleset_styles(struct section_file *file,
                                struct rscompat_info *compat);
static bool load_ruleset_cities(struct section_file *file,
                                struct rscompat_info *compat);
static bool load_ruleset_effects(struct section_file *file,
                                 struct rscompat_info *compat);
static bool load_ruleset_game(struct section_file *file, bool act,
                              struct rscompat_info *compat);

static void send_ruleset_tech_classes(struct conn_list *dest);
static void send_ruleset_techs(struct conn_list *dest);
static void send_ruleset_unit_classes(struct conn_list *dest);
static void send_ruleset_units(struct conn_list *dest);
static void send_ruleset_buildings(struct conn_list *dest);
static void send_ruleset_terrain(struct conn_list *dest);
static void send_ruleset_resources(struct conn_list *dest);
static void send_ruleset_extras(struct conn_list *dest);
static void send_ruleset_bases(struct conn_list *dest);
static void send_ruleset_roads(struct conn_list *dest);
static void send_ruleset_goods(struct conn_list *dest);
static void send_ruleset_governments(struct conn_list *dest);
static void send_ruleset_styles(struct conn_list *dest);
static void send_ruleset_clauses(struct conn_list *dest);
static void send_ruleset_musics(struct conn_list *dest);
static void send_ruleset_cities(struct conn_list *dest);
static void send_ruleset_game(struct conn_list *dest);
static void send_ruleset_team_names(struct conn_list *dest);

static bool load_ruleset_veteran(struct section_file *file,
                                 const char *path,
                                 struct veteran_system **vsystem, char *err,
                                 size_t err_len);

char *script_buffer = NULL;
char *parser_buffer = NULL;

/**********************************************************************//**
  Notifications about ruleset errors to clients. Especially important in
  case of internal server crashing.
**************************************************************************/
void ruleset_error_real(const char *file, const char *function,
                        int line, enum log_level level,
                        const char *format, ...)
{
  va_list args;
  char buf[1024];

  va_start(args, format);
  vdo_log(file, function, line, FALSE, level, buf, sizeof(buf), format, args);
  va_end(args);

  if (LOG_FATAL >= level) {
    exit(EXIT_FAILURE);
  }
}

/**********************************************************************//**
  datafilename() wrapper: tries to match in two ways.
  Returns NULL on failure, the (statically allocated) filename on success.
**************************************************************************/
static const char *valid_ruleset_filename(const char *subdir,
                                          const char *name,
                                          const char *extension,
                                          bool optional)
{
  char filename[512];
  const char *dfilename;

  fc_assert_ret_val(subdir && name && extension, NULL);

  fc_snprintf(filename, sizeof(filename), "%s" DIR_SEPARATOR "%s.%s",
              subdir, name, extension);
  log_verbose("Trying \"%s\".", filename);
  dfilename = fileinfoname(get_data_dirs(), filename);
  if (dfilename) {
    return dfilename;
  }

  fc_snprintf(filename, sizeof(filename), "default" DIR_SEPARATOR "%s.%s", name, extension);
  log_verbose("Trying \"%s\": default ruleset directory.", filename);
  dfilename = fileinfoname(get_data_dirs(), filename);
  if (dfilename) {
    return dfilename;
  }

  fc_snprintf(filename, sizeof(filename), "%s_%s.%s",
              subdir, name, extension);
  log_verbose("Trying \"%s\": alternative ruleset filename syntax.",
              filename);
  dfilename = fileinfoname(get_data_dirs(), filename);
  if (dfilename) {
    return dfilename;
  } else if (!optional) {
    ruleset_error(LOG_ERROR,
                  /* TRANS: message about an installation error. */
                  _("Could not find a readable \"%s.%s\" ruleset file."),
                  name, extension);
  }

  return NULL;
}

/**********************************************************************//**
  Return current script.lua buffer.
**************************************************************************/
char *get_script_buffer(void)
{
  return script_buffer;
}

/**********************************************************************//**
  Return current parser.lua buffer.
**************************************************************************/
char *get_parser_buffer(void)
{
  return parser_buffer;
}

/**********************************************************************//**
  Do initial section_file_load on a ruleset file.
  "whichset" = "techs", "units", "buildings", "terrain", ...
**************************************************************************/
static struct section_file *openload_ruleset_file(const char *whichset,
                                                  const char *rsdir)
{
  char sfilename[512];
  const char *dfilename = valid_ruleset_filename(rsdir, whichset,
                                                 RULES_SUFFIX, FALSE);
  struct section_file *secfile;

  if (dfilename == NULL) {
    return NULL;
  }

  /* Need to save a copy of the filename for following message, since
     section_file_load() may call datafilename() for includes. */
  sz_strlcpy(sfilename, dfilename);
  secfile = secfile_load(sfilename, FALSE);

  if (secfile == NULL) {
    ruleset_error(LOG_ERROR, "Could not load ruleset '%s':\n%s",
                  sfilename, secfile_error());
  }

  return secfile;
}

/**********************************************************************//**
  Parse script file.
**************************************************************************/
static enum fc_tristate openload_script_file(const char *whichset, const char *rsdir,
                                             char **buffer, bool optional)
{
  const char *dfilename = valid_ruleset_filename(rsdir, whichset,
                                                 SCRIPT_SUFFIX, optional);

  if (dfilename == NULL) {
    return optional ? TRI_MAYBE : TRI_NO;
  }

  if (buffer == NULL) {
    if (!script_server_do_file(NULL, dfilename)) {
      ruleset_error(LOG_ERROR, "\"%s\": could not load ruleset script.",
                    dfilename);

      return TRI_NO;
    }
  } else {
    script_server_load_file(dfilename, buffer);
  }

  return TRI_YES;
}

/**********************************************************************//**
  Load optional luadata.txt
**************************************************************************/
static struct section_file *openload_luadata_file(const char *rsdir)
{
  struct section_file *secfile;
  char sfilename[512];
  const char *dfilename = valid_ruleset_filename(rsdir, "luadata",
                                                 "txt", TRUE);

  if (dfilename == NULL) {
    return NULL;
  }

  /* Need to save a copy of the filename for following message, since
     section_file_load() may call datafilename() for includes. */
  sz_strlcpy(sfilename, dfilename);
  secfile = secfile_load(sfilename, FALSE);

  if (secfile == NULL) {
    ruleset_error(LOG_ERROR, "Could not load luadata '%s':\n%s",
                  sfilename, secfile_error());
  }

  return secfile;
}

/**********************************************************************//**
  Load a requirement list.  The list is returned as a static vector
  (callers need not worry about freeing anything).
**************************************************************************/
struct requirement_vector *lookup_req_list(struct section_file *file,
                                           struct rscompat_info *compat,
                                           const char *sec,
                                           const char *sub,
                                           const char *rfor)
{
  const char *type, *name;
  int j;
  const char *filename;

  filename = secfile_name(file);

  requirement_vector_reserve(&reqs_list, 0);

  for (j = 0; (type = secfile_lookup_str_default(file, NULL, "%s.%s%d.type",
                                                 sec, sub, j)); j++) {
    char buf[MAX_LEN_NAME];
    const char *range;
    bool survives, present, quiet;
    struct entry *pentry;
    struct requirement req;

    if (!(pentry = secfile_entry_lookup(file, "%s.%s%d.name",
                                        sec, sub, j))) {
      ruleset_error(LOG_ERROR, "%s", secfile_error());

      return NULL;
    }
    name = NULL;
    switch (entry_type(pentry)) {
    case ENTRY_BOOL:
      {
        bool val;

        if (entry_bool_get(pentry, &val)) {
          fc_snprintf(buf, sizeof(buf), "%d", val);
          name = buf;
        }
      }
      break;
    case ENTRY_INT:
      {
        int val;

        if (entry_int_get(pentry, &val)) {
          fc_snprintf(buf, sizeof(buf), "%d", val);
          name = buf;
        }
      }
      break;
    case ENTRY_STR:
      (void) entry_str_get(pentry, &name);
      break;
    case ENTRY_FLOAT:
      fc_assert(entry_type(pentry) != ENTRY_FLOAT);
      ruleset_error(LOG_ERROR,
                    "\"%s\": trying to have an floating point entry as a requirement name in '%s.%s%d'.",
                    filename, sec, sub, j);
      break;
    case ENTRY_FILEREFERENCE:
      fc_assert(entry_type(pentry) != ENTRY_FILEREFERENCE);
    }
    if (NULL == name) {
      ruleset_error(LOG_ERROR,
                    "\"%s\": error in handling requirement name for '%s.%s%d'.",
                    filename, sec, sub, j);
      return NULL;
    }

    if (!(range = secfile_lookup_str(file, "%s.%s%d.range", sec, sub, j))) {
      ruleset_error(LOG_ERROR, "%s", secfile_error());

      return NULL;
    }

    survives = FALSE;
    if ((pentry = secfile_entry_lookup(file, "%s.%s%d.survives",
                                        sec, sub, j))
        && !entry_bool_get(pentry, &survives)) {
      ruleset_error(LOG_ERROR,
                    "\"%s\": invalid boolean value for survives for "
                    "'%s.%s%d'.", filename, sec, sub, j);
    }

    present = TRUE;
    if ((pentry = secfile_entry_lookup(file, "%s.%s%d.present",
                                        sec, sub, j))
        && !entry_bool_get(pentry, &present)) {
      ruleset_error(LOG_ERROR,
                    "\"%s\": invalid boolean value for present for "
                    "'%s.%s%d'.", filename, sec, sub, j);
    }
    quiet = FALSE;
    if ((pentry = secfile_entry_lookup(file, "%s.%s%d.quiet",
                                        sec, sub, j))
        && !entry_bool_get(pentry, &quiet)) {
      ruleset_error(LOG_ERROR,
                    "\"%s\": invalid boolean value for quiet for "
                    "'%s.%s%d'.", filename, sec, sub, j);
    }

    if (compat->compat_mode) {
      if (!fc_strcasecmp(type, universals_n_name(VUT_UTFLAG))) {
        name = rscompat_utype_flag_name_3_1(compat, name);
      }
    }

    if (compat->compat_mode) {
      name = rscompat_req_name_3_1(type, name);
    }

    req = req_from_str(type, range, survives, present, quiet, name);
    if (req.source.kind == universals_n_invalid()) {
      ruleset_error(LOG_ERROR, "\"%s\" [%s] has invalid or unknown req: "
                               "\"%s\" \"%s\".",
                    filename, sec, type, name);

      return NULL;
    }

    requirement_vector_append(&reqs_list, req);
  }

  if (j > MAX_NUM_REQS) {
    ruleset_error(LOG_ERROR, "Too many (%d) requirements for %s. Max is %d",
                  j, rfor, MAX_NUM_REQS);

    return NULL;
  }

  return &reqs_list;
}

/**********************************************************************//**
  Load combat bonus list
**************************************************************************/
static bool lookup_cbonus_list(struct rscompat_info *compat,
                               struct combat_bonus_list *list,
                               struct section_file *file,
                               const char *sec,
                               const char *sub)
{
  const char *flag;
  int j;
  const char *filename;
  bool success = TRUE;

  filename = secfile_name(file);

  for (j = 0; (flag = secfile_lookup_str_default(file, NULL, "%s.%s%d.flag",
                                                 sec, sub, j)); j++) {
    struct combat_bonus *bonus = fc_malloc(sizeof(*bonus));
    const char *type;

    bonus->flag = unit_type_flag_id_by_name(rscompat_utype_flag_name_3_1(compat, flag),
                                            fc_strcasecmp);
    if (!unit_type_flag_id_is_valid(bonus->flag)) {
      log_error("\"%s\": unknown flag name \"%s\" in '%s.%s'.",
                filename, flag, sec, sub);
      FC_FREE(bonus);
      success = FALSE;
      continue;
    }
    type = secfile_lookup_str(file, "%s.%s%d.type", sec, sub, j);
    bonus->type = combat_bonus_type_by_name(type, fc_strcasecmp);
    if (!combat_bonus_type_is_valid(bonus->type)) {
      log_error("\"%s\": unknown bonus type \"%s\" in '%s.%s'.",
                filename, type, sec, sub);
      FC_FREE(bonus);
      success = FALSE;
      continue;
    }
    if (!secfile_lookup_int(file, &bonus->value, "%s.%s%d.value",
                            sec, sub, j)) {
      log_error("\"%s\": failed to get value from '%s.%s%d'.",
                filename, sec, sub, j);
      FC_FREE(bonus);
      success = FALSE;
      continue;
    }
    bonus->quiet = secfile_lookup_bool_default(file, FALSE,
                                               "%s.%s%d.quiet",
                                               sec, sub, j);
    combat_bonus_list_append(list, bonus);
  }

  return success;
}

/**********************************************************************//**
  Lookup a string prefix.entry in the file and return the corresponding
  advances pointer.  If (!required), return A_NEVER for match "Never" or
  can't match.  If (required), die when can't match.  Note the first tech
  should have name "None" so that will always match.
  If description is not NULL, it is used in the warning message
  instead of prefix (eg pass unit->name instead of prefix="units2.u27")
**************************************************************************/
static bool lookup_tech(struct section_file *file,
                        struct advance **result,
                        const char *prefix, const char *entry,
                        const char *filename,
                        const char *description)
{
  const char *sval;

  sval = secfile_lookup_str_default(file, NULL, "%s.%s", prefix, entry);
  if (!sval || !strcmp(sval, "Never")) {
    *result = A_NEVER;
  } else {
    *result = advance_by_rule_name(sval);

    if (A_NEVER == *result) {
      ruleset_error(LOG_ERROR,
                    "\"%s\" %s %s: couldn't match \"%s\".",
                    filename, (description ? description : prefix), entry, sval);
      return FALSE;
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Lookup a string prefix.entry in the file and return the corresponding
  improvement pointer. Return B_NEVER for match "None" or
  can't match.
  If description is not NULL, it is used in the warning message
  instead of prefix (eg pass unit->name instead of prefix="units2.u27")
**************************************************************************/
static bool lookup_building(struct section_file *file,
                            const char *prefix, const char *entry,
                            struct impr_type **result,
                            const char *filename,
                            const char *description)
{
  const char *sval;
  bool ok = TRUE;
 
  sval = secfile_lookup_str_default(file, NULL, "%s.%s", prefix, entry);
  if (!sval || strcmp(sval, "None") == 0) {
    *result = B_NEVER;
  } else {
    *result = improvement_by_rule_name(sval);

    if (B_NEVER == *result) {
      ruleset_error(LOG_ERROR,
                    "\"%s\" %s %s: couldn't match \"%s\".",
                    filename, (description ? description : prefix), entry, sval);
      ok = FALSE;
    }
  }

  return ok;
}

/**********************************************************************//**
  Lookup a prefix.entry string vector in the file and fill in the
  array, which should hold MAX_NUM_UNIT_LIST items. The output array is
  either NULL terminated or full (contains MAX_NUM_UNIT_LIST
  items). If the vector is not found and the required parameter is set,
  we report it as an error, otherwise we just punt.
**************************************************************************/
static bool lookup_unit_list(struct section_file *file, const char *prefix,
                             const char *entry,
                             struct unit_type **output, 
                             const char *filename)
{
  const char **slist;
  size_t nval;
  int i;
  bool ok = TRUE;

  /* pre-fill with NULL: */
  for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
    output[i] = NULL;
  }
  slist = secfile_lookup_str_vec(file, &nval, "%s.%s", prefix, entry);
  if (nval == 0) {
    /* 'No vector' is considered same as empty vector */
    if (slist != NULL) {
      free(slist);
    }
    return TRUE;
  }
  if (nval > MAX_NUM_UNIT_LIST) {
    ruleset_error(LOG_ERROR,
                  "\"%s\": string vector %s.%s too long (%d, max %d)",
                  filename, prefix, entry, (int) nval, MAX_NUM_UNIT_LIST);
    ok = FALSE;
  } else if (nval == 1 && strcmp(slist[0], "") == 0) {
    free(slist);
    return TRUE;
  }
  if (ok) {
    for (i = 0; i < nval; i++) {
      const char *sval = slist[i];
      struct unit_type *punittype = unit_type_by_rule_name(sval);

      if (!punittype) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" %s.%s (%d): couldn't match \"%s\".",
                      filename, prefix, entry, i, sval);
        ok = FALSE;
        break;
      }
      output[i] = punittype;
      log_debug("\"%s\" %s.%s (%d): %s (%d)", filename, prefix, entry, i, sval,
                utype_number(punittype));
    }
  }
  free(slist);

  return ok;
}

/**********************************************************************//**
  Lookup a prefix.entry string vector in the file and fill in the
  array, which should hold MAX_NUM_TECH_LIST items. The output array is
  either A_LAST terminated or full (contains MAX_NUM_TECH_LIST
  items). All valid entries of the output array are guaranteed to
  exist.
**************************************************************************/
static bool lookup_tech_list(struct section_file *file, const char *prefix,
                             const char *entry, int *output,
                             const char *filename)
{
  const char **slist;
  size_t nval;
  int i;
  bool ok = TRUE;

  /* pre-fill with A_LAST: */
  for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
    output[i] = A_LAST;
  }
  slist = secfile_lookup_str_vec(file, &nval, "%s.%s", prefix, entry);
  if (slist == NULL || nval == 0) {
    return TRUE;
  } else if (nval > MAX_NUM_TECH_LIST) {
    ruleset_error(LOG_ERROR,
                  "\"%s\": string vector %s.%s too long (%d, max %d)",
                  filename, prefix, entry, (int) nval, MAX_NUM_TECH_LIST);
    ok = FALSE;
  }

  if (ok) {
    if (nval == 1 && strcmp(slist[0], "") == 0) {
      FC_FREE(slist);
      return TRUE;
    }
    for (i = 0; i < nval && ok; i++) {
      const char *sval = slist[i];
      struct advance *padvance = advance_by_rule_name(sval);

      if (NULL == padvance) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" %s.%s (%d): couldn't match \"%s\".",
                      filename, prefix, entry, i, sval);
        ok = FALSE;
      }
      if (!valid_advance(padvance)) {
        ruleset_error(LOG_ERROR, "\"%s\" %s.%s (%d): \"%s\" is removed.",
                      filename, prefix, entry, i, sval);
        ok = FALSE;
      }

      if (ok) {
        output[i] = advance_number(padvance);
        log_debug("\"%s\" %s.%s (%d): %s (%d)", filename, prefix, entry, i, sval,
                  advance_number(padvance));
      }
    }
  }
  FC_FREE(slist);

  return ok;
}

/**********************************************************************//**
  Lookup a prefix.entry string vector in the file and fill in the
  array, which should hold MAX_NUM_BUILDING_LIST items. The output array is
  either B_LAST terminated or full (contains MAX_NUM_BUILDING_LIST
  items). [All valid entries of the output array are guaranteed to pass
  improvement_exist()?]
**************************************************************************/
static bool lookup_building_list(struct section_file *file,
                                 const char *prefix, const char *entry,
                                 int *output, const char *filename)
{
  const char **slist;
  size_t nval;
  int i;
  bool ok = TRUE;

  /* pre-fill with B_LAST: */
  for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
    output[i] = B_LAST;
  }
  slist = secfile_lookup_str_vec(file, &nval, "%s.%s", prefix, entry);
  if (nval > MAX_NUM_BUILDING_LIST) {
    ruleset_error(LOG_ERROR,
                  "\"%s\": string vector %s.%s too long (%d, max %d)",
                  filename, prefix, entry, (int) nval, MAX_NUM_BUILDING_LIST);
    ok = FALSE;
  } else if (nval == 0 || (nval == 1 && strcmp(slist[0], "") == 0)) {
    if (slist != NULL) {
      FC_FREE(slist);
    }
    return TRUE;
  }
  if (ok) {
    for (i = 0; i < nval; i++) {
      const char *sval = slist[i];
      struct impr_type *pimprove = improvement_by_rule_name(sval);

      if (NULL == pimprove) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" %s.%s (%d): couldn't match \"%s\".",
                      filename, prefix, entry, i, sval);
        ok = FALSE;
        break;
      }
      output[i] = improvement_number(pimprove);
      log_debug("%s.%s,%d %s %d", prefix, entry, i, sval, output[i]);
    }
  }
  free(slist);

  return ok;
}

/**********************************************************************//**
  Lookup a string prefix.entry in the file and set result to the
  corresponding unit_type.
  If description is not NULL, it is used in the warning message
  instead of prefix (eg pass unit->name instead of prefix="units2.u27")
**************************************************************************/
static bool lookup_unit_type(struct section_file *file,
                             const char *prefix,
                             const char *entry,
                             struct unit_type **result,
                             const char *filename,
                             const char *description)
{
  const char *sval;
  
  sval = secfile_lookup_str_default(file, "None", "%s.%s", prefix, entry);

  if (strcmp(sval, "None") == 0) {
    *result = NULL;
  } else {
    *result = unit_type_by_rule_name(sval);
    if (*result == NULL) {
      ruleset_error(LOG_ERROR,
                    "\"%s\" %s %s: couldn't match \"%s\".",
                    filename, (description ? description : prefix), entry, sval);

      return FALSE;
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Lookup entry in the file and return the corresponding government index.
  filename is for error message.
**************************************************************************/
static struct government *lookup_government(struct section_file *file,
					    const char *entry,
					    const char *filename,
                                            struct government *fallback)
{
  const char *sval;
  struct government *gov;

  sval = secfile_lookup_str_default(file, NULL, "%s", entry);
  if (!sval) {
    gov = fallback;
  } else {
    gov = government_by_rule_name(sval);
  }
  if (!gov) {
    ruleset_error(LOG_ERROR,
                  "\"%s\" %s: couldn't match \"%s\".",
                  filename, entry, sval);
  }
  return gov;
}

/**********************************************************************//**
  Lookup optional string, returning allocated memory or NULL.
**************************************************************************/
static char *lookup_string(struct section_file *file, const char *prefix,
                           const char *suffix)
{
  const char *sval = secfile_lookup_str(file, "%s.%s", prefix, suffix);

  if (NULL != sval) {
    char copy[strlen(sval) + 1];

    strcpy(copy, sval);
    remove_leading_trailing_spaces(copy);
    if (strlen(copy) > 0) {
      return fc_strdup(copy);
    }
  }
  return NULL;
}

/**********************************************************************//**
  Lookup optional string vector, returning allocated memory or NULL.
**************************************************************************/
static struct strvec *lookup_strvec(struct section_file *file,
                                    const char *prefix, const char *suffix)
{
  size_t dim;
  const char **vec = secfile_lookup_str_vec(file, &dim,
                                            "%s.%s", prefix, suffix);

  if (NULL != vec) {
    struct strvec *dest = strvec_new();

    strvec_store(dest, vec, dim);
    free(vec);
    return dest;
  }
  return NULL;
}

/**********************************************************************//**
  Look up the resource section name and return its pointer.
**************************************************************************/
static struct extra_type *lookup_resource(const char *filename,
                                          const char *name,
                                          const char *jsection)
{
  struct extra_type *pres;

  pres = extra_type_by_rule_name(name);

  if (pres == NULL) {
    ruleset_error(LOG_ERROR,
                  "\"%s\" [%s] has unknown \"%s\".",
                  filename,
                  jsection,
                  name);
  }

  return pres;
}

/**********************************************************************//**
  Look up the terrain by name and return its pointer.
  filename is for error message.
**************************************************************************/
static bool lookup_terrain(struct section_file *file,
                           const char *entry,
                           const char *filename,
                           struct terrain *pthis,
                           struct terrain **result)
{
  const int j = terrain_index(pthis);
  const char *jsection = &terrain_sections[j * MAX_SECTION_LABEL];
  const char *name = secfile_lookup_str(file, "%s.%s", jsection, entry);
  struct terrain *pterr;

  if (NULL == name
      || *name == '\0'
      || (0 == strcmp(name, "none"))
      || (0 == strcmp(name, "no"))) {
    *result = T_NONE;

    return TRUE;
  }
  if (0 == strcmp(name, "yes")) {
    *result = pthis;

    return TRUE;
  }

  pterr = terrain_by_rule_name(name);
  *result = pterr;

  if (pterr == NULL) {
    ruleset_error(LOG_ERROR, "\"%s\" [%s] has unknown \"%s\".",
                  secfile_name(file), jsection, name);
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Look up a value comparable to activity_count (road_time, etc).
  item_name describes the thing which has the time property, if non-NULL,
  for any error message.
  Returns FALSE if not found in secfile, but TRUE even if validation failed.
  Sets *ok to FALSE if validation failed, leaves it alone otherwise.
**************************************************************************/
static bool lookup_time(const struct section_file *secfile, int *turns,
                        const char *sec_name, const char *property_name,
                        const char *filename, const char *item_name,
                        bool *ok)
{
  /* Assumes that PACKET_UNIT_INFO.activity_count in packets.def is UINT16 */
  const int max_turns = 65535 / ACTIVITY_FACTOR;

  if (!secfile_lookup_int(secfile, turns, "%s.%s", sec_name, property_name)) {
    return FALSE;
  }

  if (*turns > max_turns) {
    ruleset_error(LOG_ERROR,
                  "\"%s\": \"%s\": \"%s\" value %d too large (max %d)",
                  filename, item_name ? item_name : sec_name,
                  property_name, *turns, max_turns);
    *ok = FALSE;
  }

  return TRUE; /* we found _something */
}

/**********************************************************************//**
  Load "name" and (optionally) "rule_name" into a struct name_translation.
**************************************************************************/
static bool ruleset_load_names(struct name_translation *pname,
                               const char *domain,
                               struct section_file *file,
                               const char *sec_name)
{
  const char *name = secfile_lookup_str(file, "%s.name", sec_name);
  const char *rule_name = secfile_lookup_str(file, "%s.rule_name", sec_name);

  if (!name) {
    ruleset_error(LOG_ERROR,
                  "\"%s\" [%s]: no \"name\" specified.",
                  secfile_name(file), sec_name);
    return FALSE;
  }

  names_set(pname, domain, name, rule_name);

  return TRUE;
}

/**********************************************************************//**
  Load trait values to array.
**************************************************************************/
static void ruleset_load_traits(struct trait_limits *out,
                                struct section_file *file,
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

  for (tr = trait_begin(); tr != trait_end() && trait_names[tr] != NULL; tr = trait_next(tr)) {
    out[tr].min = secfile_lookup_int_default(file, -1, "%s.%s%s_min",
                                             secname,
                                             field_prefix,
                                             trait_names[tr]);
    out[tr].max = secfile_lookup_int_default(file, -1, "%s.%s%s_max",
                                             secname,
                                             field_prefix,
                                             trait_names[tr]);
    out[tr].fixed = secfile_lookup_int_default(file, -1, "%s.%s%s_default",
                                               secname,
                                               field_prefix,
                                               trait_names[tr]);
  }

  fc_assert(tr == trait_end()); /* number of trait_names correct */
}

/**********************************************************************//**
  Load names from game.ruleset so other rulesets can refer to objects
  with their name.
**************************************************************************/
static bool load_game_names(struct section_file *file,
                            struct rscompat_info *compat)
{
  struct section_list *sec;
  int nval;
  const char *filename = secfile_name(file);
  bool ok = TRUE;

  /* section: datafile */
  compat->ver_game = rscompat_check_capabilities(file, filename, compat);
  if (compat->ver_game <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  sec = secfile_sections_by_name_prefix(file, ACHIEVEMENT_SECTION_PREFIX);
  nval = (NULL != sec ? section_list_size(sec) : 0);
  if (nval > MAX_ACHIEVEMENT_TYPES) {
    int num = nval; /* No "size_t" to printf */

    ruleset_error(LOG_ERROR, "\"%s\": Too many achievement types (%d, max %d)",
                  filename, num, MAX_ACHIEVEMENT_TYPES);
    ok = FALSE;
  } else {
    game.control.num_achievement_types = nval;
  }

  if (ok) {
    achievements_iterate(pach) {
      const char *sec_name = section_name(section_list_get(sec, achievement_index(pach)));

      if (!ruleset_load_names(&pach->name, NULL, file, sec_name)) {
        ruleset_error(LOG_ERROR, "\"%s\": Cannot load achievement names",
                      filename);
        ok = FALSE;
        break;
      }
    } achievements_iterate_end;
  }

  section_list_destroy(sec);

  if (compat->ver_game >= 10) {
    if (ok) {
      sec = secfile_sections_by_name_prefix(file, GOODS_SECTION_PREFIX);

      nval = (NULL != sec ? section_list_size(sec) : 0);
      if (nval > MAX_GOODS_TYPES) {
        int num = nval; /* No "size_t" to printf */

        ruleset_error(LOG_ERROR,
                      "\"%s\": Too many goods types (%d, max %d)",
                      filename, num, MAX_GOODS_TYPES);
        section_list_destroy(sec);
        ok = FALSE;
      } else if (nval < 1) {
        ruleset_error(LOG_ERROR, "\"%s\": At least one goods type needed",
                      filename);
        section_list_destroy(sec);
        ok = FALSE;
      } else {
        game.control.num_goods_types = nval;
      }

      if (ok) {
        goods_type_iterate(pgood) {
          const char *sec_name
              = section_name(section_list_get(sec, goods_index(pgood)));

          if (!ruleset_load_names(&pgood->name, NULL, file, sec_name)) {
            ruleset_error(LOG_ERROR, "\"%s\": Cannot load goods names",
                          filename);
            ok = FALSE;
            break;
          }
        } goods_type_iterate_end;
      }
      section_list_destroy(sec);
    }
  }

  return ok;
}


/**********************************************************************//**
  Load names of technologies so other rulesets can refer to techs with
  their name.
**************************************************************************/
static bool load_tech_names(struct section_file *file,
                            struct rscompat_info *compat)
{
  struct section_list *sec = NULL;
  /* Number of techs in the ruleset (means without A_NONE). */
  int num_techs = 0;
  int i;
  const char *filename = secfile_name(file);
  bool ok = TRUE;
  const char *flag;

  compat->ver_techs = rscompat_check_capabilities(file, filename, compat);
  if (compat->ver_techs <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  /* User tech flag names */
  for (i = 0; (flag = secfile_lookup_str_default(file, NULL, "control.flags%d.name", i)) ;
       i++) {
    const char *helptxt = secfile_lookup_str_default(file, NULL, "control.flags%d.helptxt",
                                                     i);
    if (tech_flag_id_by_name(flag, fc_strcasecmp) != tech_flag_id_invalid()) {
      ruleset_error(LOG_ERROR, "\"%s\": Duplicate tech flag name '%s'",
                    filename, flag);
      ok = FALSE;
      break;
    }
    if (i > MAX_NUM_USER_TECH_FLAGS) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many user tech flags!",
                    filename);
      ok = FALSE;
      break;
    }

    set_user_tech_flag_name(TECH_USER_1 + i, flag, helptxt);
  }

  if (ok) {
    size_t nval;

    for (; i < MAX_NUM_USER_TECH_FLAGS; i++) {
      set_user_tech_flag_name(TECH_USER_1 + i, NULL, NULL);
    }

    /* Tech classes */
    sec = secfile_sections_by_name_prefix(file, TECH_CLASS_SECTION_PREFIX);

    nval = (NULL != sec ? section_list_size(sec) : 0);
    if (nval > MAX_NUM_TECH_CLASSES) {
      int num = nval; /* No "size_t" to printf */

      ruleset_error(LOG_ERROR,
                    "\"%s\": Too many tech classes (%d, max %d)",
                    filename, num, MAX_NUM_TECH_CLASSES);
      section_list_destroy(sec);
      ok = FALSE;
    } else {
      game.control.num_tech_classes = nval;
    }

    if (ok) {
      tech_class_iterate(ptclass) {
        const char *sec_name
          = section_name(section_list_get(sec, tech_class_index(ptclass)));

        if (!ruleset_load_names(&ptclass->name, NULL, file, sec_name)) {
          ruleset_error(LOG_ERROR, "\"%s\": Cannot load tech class names",
                        filename);
          ok = FALSE;
          break;
        }
      } tech_class_iterate_end;
    }
  }

  if (ok) {
    /* The techs: */
    sec = secfile_sections_by_name_prefix(file, ADVANCE_SECTION_PREFIX);
    if (NULL == sec || 0 == (num_techs = section_list_size(sec))) {
      ruleset_error(LOG_ERROR, "\"%s\": No Advances?!?", filename);
      ok = FALSE;
    } else {
      log_verbose("%d advances (including possibly unused)", num_techs);
      if (num_techs + A_FIRST > A_LAST) {
        ruleset_error(LOG_ERROR, "\"%s\": Too many advances (%d, max %d)",
                      filename, num_techs, A_LAST - A_FIRST);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    game.control.num_tech_types = num_techs + A_FIRST; /* includes A_NONE */

    i = 0;
    advance_iterate(A_FIRST, a) {
      if (!ruleset_load_names(&a->name, NULL, file, section_name(section_list_get(sec, i)))) {
        ok = FALSE;
        break;
      }
      i++;
    } advance_iterate_end;
  }
  section_list_destroy(sec);

  return ok;
}

/**********************************************************************//**
  Load technologies related ruleset data
**************************************************************************/
static bool load_ruleset_techs(struct section_file *file,
                               struct rscompat_info *compat)
{
  struct section_list *sec;
  const char **slist;
  int i;
  size_t nval;
  struct advance *a_none = advance_by_number(A_NONE);
  const char *filename = secfile_name(file);
  bool ok = TRUE;

  sec = secfile_sections_by_name_prefix(file, TECH_CLASS_SECTION_PREFIX);

  i = 0;
  tech_class_iterate(ptclass) {
    const char *sec_name = section_name(section_list_get(sec, i));

    ptclass->cost_pct = secfile_lookup_int_default(file, 100, "%s.%s",
                                                   sec_name, "cost_pct");

    i++;
  } tech_class_iterate_end;

  sec = secfile_sections_by_name_prefix(file, ADVANCE_SECTION_PREFIX);

  i = 0;
  advance_iterate(A_FIRST, a) {
    const char *sec_name = section_name(section_list_get(sec, i));
    const char *sval;
    int j, ival;
    struct requirement_vector *research_reqs;

    if (!lookup_tech(file, &a->require[AR_ONE], sec_name, "req1",
                     filename, rule_name_get(&a->name))
        || !lookup_tech(file, &a->require[AR_TWO], sec_name, "req2",
                        filename, rule_name_get(&a->name))
        || !lookup_tech(file, &a->require[AR_ROOT], sec_name, "root_req",
                        filename, rule_name_get(&a->name))) {
      ok = FALSE;
      break;
    }

    if ((A_NEVER == a->require[AR_ONE] && A_NEVER != a->require[AR_TWO])
        || (A_NEVER != a->require[AR_ONE] && A_NEVER == a->require[AR_TWO])) {
      ruleset_error(LOG_ERROR, "\"%s\" [%s] \"%s\": \"Never\" with non-\"Never\".",
                    filename, sec_name, rule_name_get(&a->name));
      ok = FALSE;
      break;
    }
    if (a_none == a->require[AR_ONE] && a_none != a->require[AR_TWO]) {
      ruleset_error(LOG_ERROR, "\"%s\" [%s] \"%s\": should have \"None\" second.",
                    filename, sec_name, rule_name_get(&a->name));
      ok = FALSE;
      break;
    }

    if (game.control.num_tech_classes == 0) {
      a->tclass = NULL;
    } else {
      const char *classname;

      classname = lookup_string(file, sec_name, "class");
      if (classname != NULL) {
        classname = Q_(classname);
        a->tclass = tech_class_by_rule_name(classname);
        if (a->tclass == NULL) {
          ruleset_error(LOG_ERROR, "\"%s\" [%s] \"%s\": Uknown tech class \"%s\".",
                        filename, sec_name, rule_name_get(&a->name), classname);
          ok = FALSE;
          break;
        }
      } else {
        a->tclass = NULL; /* Default */
      }
    }

    research_reqs = lookup_req_list(file, compat, sec_name, "research_reqs",
                                    rule_name_get(&a->name));
    if (research_reqs == NULL) {
      ok = FALSE;
      break;
    }

    requirement_vector_copy(&a->research_reqs, research_reqs);

    BV_CLR_ALL(a->flags);

    slist = secfile_lookup_str_vec(file, &nval, "%s.flags", sec_name);
    for (j = 0; j < nval; j++) {
      sval = slist[j];
      if (strcmp(sval, "") == 0) {
        continue;
      }
      ival = tech_flag_id_by_name(sval, fc_strcasecmp);
      if (!tech_flag_id_is_valid(ival)) {
        ruleset_error(LOG_ERROR, "\"%s\" [%s] \"%s\": bad flag name \"%s\".",
                      filename, sec_name, rule_name_get(&a->name), sval);
        ok = FALSE;
        break;
      } else {
        BV_SET(a->flags, ival);
      }
    }
    free(slist);

    if (!ok) {
      break;
    }

    sz_strlcpy(a->graphic_str,
               secfile_lookup_str_default(file, "-", "%s.graphic", sec_name));
    sz_strlcpy(a->graphic_alt,
               secfile_lookup_str_default(file, "-",
                                          "%s.graphic_alt", sec_name));

    a->helptext = lookup_strvec(file, sec_name, "helptext");
    a->bonus_message = lookup_string(file, sec_name, "bonus_message");
    a->cost = secfile_lookup_int_default(file, -1, "%s.%s",
                                         sec_name, "cost");
    a->num_reqs = 0;

    i++;
  } advance_iterate_end;

  /* Propagate a root tech up into the tech tree. If a technology
   * X has Y has a root tech, then any technology requiring X (in the
   * normal way or as a root tech) also has Y as a root tech.
   * Later techs may gain a whole set of root techs in this way. The one
   * we store in AR_ROOT is a more or less arbitrary one of these,
   * also signalling that the set is non-empty; after this, you'll still
   * have to walk the tech tree to find them all. */
restart:

  if (ok) {
    advance_iterate(A_FIRST, a) {
      if (valid_advance(a)
          && A_NEVER != a->require[AR_ROOT]) {
        bool out_of_order = FALSE;

        /* Now find any tech depending on this technology and update its
         * root_req. */
        advance_iterate(A_FIRST, b) {
          if (valid_advance(b)
              && A_NEVER == b->require[AR_ROOT]
              && (a == b->require[AR_ONE] || a == b->require[AR_TWO])) {
            b->require[AR_ROOT] = a->require[AR_ROOT];
            b->inherited_root_req = TRUE;
            if (b < a) {
              out_of_order = TRUE;
            }
          }
        } advance_iterate_end;

        if (out_of_order) {
          /* HACK: If we just changed the root_tech of a lower-numbered
           * technology, we need to go back so that we can propagate the
           * root_tech up to that technology's parents... */
          goto restart;
        }
      }
    } advance_iterate_end;

    /* Now rename A_NEVER to A_NONE for consistency */
    advance_iterate(A_NONE, a) {
      if (A_NEVER == a->require[AR_ROOT]) {
        a->require[AR_ROOT] = a_none;
      }
    } advance_iterate_end;

    /* Some more consistency checking: 
       Non-removed techs depending on removed techs is too
       broken to fix by default, so die.
    */
    advance_iterate(A_FIRST, a) {
      if (valid_advance(a)) {
        /* We check for recursive tech loops later,
         * in build_required_techs_helper. */
        if (!valid_advance(a->require[AR_ONE])) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" tech \"%s\": req1 leads to removed tech.",
                        filename,
                        advance_rule_name(a));
          ok = FALSE;
          break;
        }
        if (!valid_advance(a->require[AR_TWO])) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" tech \"%s\": req2 leads to removed tech.",
                        filename,
                        advance_rule_name(a));
          ok = FALSE;
          break;
        }
      }
    } advance_iterate_end;
  }

  section_list_destroy(sec);
  if (ok) {
    secfile_check_unused(file);
  }

  return ok;
}

/**********************************************************************//**
  Load names of units so other rulesets can refer to units with
  their name.
**************************************************************************/
static bool load_unit_names(struct section_file *file,
                            struct rscompat_info *compat)
{
  struct section_list *sec = NULL;
  int nval = 0;
  int i;
  const char *filename = secfile_name(file);
  const char *flag;
  bool ok = TRUE;

  compat->ver_units = rscompat_check_capabilities(file, filename, compat);
  if (compat->ver_units <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  /* User unit flag names */
  for (i = 0; (flag = secfile_lookup_str_default(file, NULL, "control.flags%d.name", i)) ;
       i++) {
    const char *helptxt = secfile_lookup_str_default(file, NULL, "control.flags%d.helptxt",
                                                     i);

    if (unit_type_flag_id_by_name(rscompat_utype_flag_name_3_1(compat, flag),
                                  fc_strcasecmp)
        != unit_type_flag_id_invalid()) {
      ruleset_error(LOG_ERROR, "\"%s\": Duplicate unit flag name '%s'",
                    filename, flag);
      ok = FALSE;
      break;
    }
    if (i > MAX_NUM_USER_UNIT_FLAGS) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many user unit type flags!",
                    filename);
      ok = FALSE;
      break;
    }

    set_user_unit_type_flag_name(UTYF_USER_FLAG_1 + i, flag, helptxt);
  }

  if (ok) {
    /* Blank the remaining unit type user flags. */
    for (; i < MAX_NUM_USER_UNIT_FLAGS; i++) {
      set_user_unit_type_flag_name(UTYF_USER_FLAG_1 + i, NULL, NULL);
    }
  }

  if (ok) {
    /* User unit class flag names */
    for (i = 0;
         (flag = secfile_lookup_str_default(file, NULL,
                                            "control.class_flags%d.name",
                                            i));
         i++) {
      const char *helptxt = secfile_lookup_str_default(file, NULL,
          "control.class_flags%d.helptxt", i);

      if (unit_class_flag_id_by_name(flag, fc_strcasecmp)
          != unit_class_flag_id_invalid()) {
        ruleset_error(LOG_ERROR, "\"%s\": Duplicate unit class flag name "
                                 "'%s'",
                      filename, flag);
        ok = FALSE;
        break;
      }
      if (i > MAX_NUM_USER_UCLASS_FLAGS) {
        ruleset_error(LOG_ERROR, "\"%s\": Too many user unit class flags!",
                      filename);
        ok = FALSE;
        break;
      }

      set_user_unit_class_flag_name(UCF_USER_FLAG_1 + i, flag, helptxt);
    }
  }

  if (ok) {
    /* Blank the remaining unit class user flags. */
    for (; i < MAX_NUM_USER_UCLASS_FLAGS; i++) {
      set_user_unit_class_flag_name(UCF_USER_FLAG_1 + i, NULL, NULL);
    }
  }

  if (ok) {
    /* Unit classes */
    sec = secfile_sections_by_name_prefix(file, UNIT_CLASS_SECTION_PREFIX);
    if (NULL == sec || 0 == (nval = section_list_size(sec))) {
      ruleset_error(LOG_ERROR, "\"%s\": No unit classes?!?", filename);
      ok = FALSE;
    } else {
      log_verbose("%d unit classes", nval);
      if (nval > UCL_LAST) {
        ruleset_error(LOG_ERROR, "\"%s\": Too many unit classes (%d, max %d)",
                  filename, nval, UCL_LAST);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    game.control.num_unit_classes = nval;

    unit_class_iterate(punitclass) {
      const int pci = uclass_index(punitclass);

      if (!ruleset_load_names(&punitclass->name, NULL, file,
                              section_name(section_list_get(sec, pci)))) {
        ok = FALSE;
        break;
      }
    } unit_class_iterate_end;
  }
  section_list_destroy(sec);
  sec = NULL;

  /* The names: */
  if (ok) {
    sec = secfile_sections_by_name_prefix(file, UNIT_SECTION_PREFIX);
    if (NULL == sec || 0 == (nval = section_list_size(sec))) {
      ruleset_error(LOG_ERROR, "\"%s\": No unit types?!?", filename);
      ok = FALSE;
    } else {
      log_verbose("%d unit types (including possibly unused)", nval);
      if (nval > U_LAST) {
        ruleset_error(LOG_ERROR, "\"%s\": Too many unit types (%d, max %d)",
                      filename, nval, U_LAST);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    game.control.num_unit_types = nval;

    unit_type_iterate(punittype) {
      const int utypei = utype_index(punittype);
      if (!ruleset_load_names(&punittype->name, NULL, file,
                              section_name(section_list_get(sec, utypei)))) {
        ok = FALSE;
        break;
      }
    } unit_type_iterate_end;
  }
  section_list_destroy(sec);

  return ok;
}

/**********************************************************************//**
  Load veteran levels.
**************************************************************************/
static bool load_ruleset_veteran(struct section_file *file,
                                 const char *path,
                                 struct veteran_system **vsystem, char *err,
                                 size_t err_len)
{
  const char **vlist_name;
  int *vlist_power, *vlist_raise, *vlist_wraise, *vlist_move;
  size_t count_name, count_power, count_raise, count_wraise, count_move;
  int i;
  bool ret = TRUE;

  /* The pointer should be uninitialised. */
  if (*vsystem != NULL) {
    fc_snprintf(err, err_len, "Veteran system is defined?!");
    return FALSE;
  }

  /* Load data. */
  vlist_name = secfile_lookup_str_vec(file, &count_name,
                                      "%s.veteran_names", path);
  vlist_power = secfile_lookup_int_vec(file, &count_power,
                                      "%s.veteran_power_fact", path);
  vlist_raise = secfile_lookup_int_vec(file, &count_raise,
                                       "%s.veteran_raise_chance", path);
  vlist_wraise = secfile_lookup_int_vec(file, &count_wraise,
                                        "%s.veteran_work_raise_chance",
                                        path);
  vlist_move = secfile_lookup_int_vec(file, &count_move,
                                      "%s.veteran_move_bonus", path);

  if (count_name > MAX_VET_LEVELS) {
    ret = FALSE;
    fc_snprintf(err, err_len, "\"%s\": Too many veteran levels (section "
                              "'%s': %lu, max %d)", secfile_name(file), path,
                (long unsigned)count_name, MAX_VET_LEVELS);
  } else if (count_name != count_power
             || count_name != count_raise
             || count_name != count_wraise
             || count_name != count_move) {
    ret = FALSE;
    fc_snprintf(err, err_len, "\"%s\": Different lengths for the veteran "
                              "settings in section '%s'", secfile_name(file),
                path);
  } else if (count_name == 0) {
    /* Nothing defined. */
    *vsystem = NULL;
  } else {
    /* Generate the veteran system. */
    *vsystem = veteran_system_new((int)count_name);

#define rs_sanity_veteran(_path, _entry, _i, _condition, _action)            \
  if (_condition) {                                                          \
    log_error("Invalid veteran definition '%s.%s[%d]'!",                     \
              _path, _entry, _i);                                            \
    log_debug("Failed check: '%s'. Update value: '%s'.",                     \
              #_condition, #_action);                                        \
    _action;                                                                 \
  }
    for (i = 0; i < count_name; i++) {
      /* Some sanity checks. */
      rs_sanity_veteran(path, "veteran_power_fact", i,
                        (vlist_power[i] < 0), vlist_power[i] = 0);
      rs_sanity_veteran(path, "veteran_raise_chance", i,
                        (vlist_raise[i] < 0), vlist_raise[i] = 0);
      rs_sanity_veteran(path, "veteran_work_raise_chance", i,
                        (vlist_wraise[i] < 0), vlist_wraise[i] = 0);
      rs_sanity_veteran(path, "veteran_move_bonus", i,
                        (vlist_move[i] < 0), vlist_move[i] = 0);
      if (i == 0) {
        /* First element.*/
        rs_sanity_veteran(path, "veteran_power_fact", i,
                          (vlist_power[i] != 100), vlist_power[i] = 100);
      } else if (i == count_name - 1) {
        /* Last element. */
        rs_sanity_veteran(path, "veteran_power_fact", i,
                          (vlist_power[i] < vlist_power[i - 1]),
                          vlist_power[i] = vlist_power[i - 1]);
        rs_sanity_veteran(path, "veteran_raise_chance", i,
                          (vlist_raise[i] != 0), vlist_raise[i] = 0);
        rs_sanity_veteran(path, "veteran_work_raise_chance", i,
                          (vlist_wraise[i] != 0), vlist_wraise[i] = 0);
      } else {
        /* All elements inbetween. */
        rs_sanity_veteran(path, "veteran_power_fact", i,
                          (vlist_power[i] < vlist_power[i - 1]),
                          vlist_power[i] = vlist_power[i - 1]);
        rs_sanity_veteran(path, "veteran_raise_chance", i,
                          (vlist_raise[i] > 100), vlist_raise[i] = 100);
        rs_sanity_veteran(path, "veteran_work_raise_chance", i,
                          (vlist_wraise[i] > 100), vlist_wraise[i] = 100);
      }

      veteran_system_definition(*vsystem, i, vlist_name[i], vlist_power[i],
                                vlist_move[i], vlist_raise[i],
                                vlist_wraise[i]);
    }
#undef rs_sanity_veteran
  }

  if (vlist_name) {
    free(vlist_name);
  }
  if (vlist_power) {
    free(vlist_power);
  }
  if (vlist_raise) {
    free(vlist_raise);
  }
  if (vlist_wraise) {
    free(vlist_wraise);
  }
  if (vlist_move) {
    free(vlist_move);
  }

  return ret;
}

/**********************************************************************//**
  Load units related ruleset data.
**************************************************************************/
static bool load_ruleset_units(struct section_file *file,
                               struct rscompat_info *compat)
{
  int j, ival;
  size_t nval;
  struct section_list *sec, *csec;
  const char *sval, **slist;
  const char *filename = secfile_name(file);
  char msg[MAX_LEN_MSG];
  bool ok = TRUE;

  if (!load_ruleset_veteran(file, "veteran_system", &game.veteran, msg,
                            sizeof(msg)) || game.veteran == NULL) {
    ruleset_error(LOG_ERROR, "Error loading the default veteran system: %s",
                  msg);
    ok = FALSE;
  }

  sec = secfile_sections_by_name_prefix(file, UNIT_SECTION_PREFIX);
  nval = (NULL != sec ? section_list_size(sec) : 0);

  csec = secfile_sections_by_name_prefix(file, UNIT_CLASS_SECTION_PREFIX);
  nval = (NULL != csec ? section_list_size(csec) : 0);

  if (ok) {
    unit_class_iterate(uc) {
      int i = uclass_index(uc);
      const char *hut_str;
      const char *sec_name = section_name(section_list_get(csec, i));

      if (secfile_lookup_int(file, &uc->min_speed, "%s.min_speed", sec_name)) {
        uc->min_speed *= SINGLE_MOVE;
      } else {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }
      if (!secfile_lookup_int(file, &uc->hp_loss_pct,
                              "%s.hp_loss_pct", sec_name)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }

      uc->non_native_def_pct = secfile_lookup_int_default(file, 100,
                                                          "%s.non_native_def_pct",
                                                          sec_name);

      hut_str = secfile_lookup_str_default(file, "Normal", "%s.hut_behavior", sec_name);
      if (fc_strcasecmp(hut_str, "Normal") == 0) {
        uc->hut_behavior = HUT_NORMAL;
      } else if (fc_strcasecmp(hut_str, "Nothing") == 0) {
        uc->hut_behavior = HUT_NOTHING;
      } else if (fc_strcasecmp(hut_str, "Frighten") == 0) {
        uc->hut_behavior = HUT_FRIGHTEN;
      } else {
        ruleset_error(LOG_ERROR,
                      "\"%s\" unit_class \"%s\":"
                      " Illegal hut behavior \"%s\".",
                      filename,
                      uclass_rule_name(uc),
                      hut_str);
        ok = FALSE;
        break;
      }

      BV_CLR_ALL(uc->flags);
      slist = secfile_lookup_str_vec(file, &nval, "%s.flags", sec_name);
      for (j = 0; j < nval; j++) {
        sval = slist[j];
        if (strcmp(sval, "") == 0) {
          continue;
        }
        ival = unit_class_flag_id_by_name(sval, fc_strcasecmp);
        if (!unit_class_flag_id_is_valid(ival)) {
          ok = FALSE;
          ival = unit_type_flag_id_by_name(rscompat_utype_flag_name_3_1(compat, sval),
                                           fc_strcasecmp);
          if (unit_type_flag_id_is_valid(ival)) {
            ruleset_error(LOG_ERROR,
                          "\"%s\" unit_class \"%s\": unit_type flag \"%s\"!",
                          filename, uclass_rule_name(uc), sval);
          } else {
            ruleset_error(LOG_ERROR,
                          "\"%s\" unit_class \"%s\": bad flag name \"%s\".",
                          filename, uclass_rule_name(uc), sval);
          }
          break;
        } else {
          BV_SET(uc->flags, ival);
        }
      }
      free(slist);

      uc->helptext = lookup_strvec(file, sec_name, "helptext");

      if (!ok) {
        break;
      }
    } unit_class_iterate_end;
  }

  if (ok) {
    /* Tech and Gov requirements; per unit veteran system */
    unit_type_iterate(u) {
      const int i = utype_index(u);
      const struct section *psection = section_list_get(sec, i);
      const char *sec_name = section_name(psection);

      if (!lookup_tech(file, &u->require_advance, sec_name,
                       "tech_req", filename,
                       rule_name_get(&u->name))) {
        ok = FALSE;
        break;
      }
      if (NULL != section_entry_by_name(psection, "gov_req")) {
        char tmp[200] = "\0";
        fc_strlcat(tmp, section_name(psection), sizeof(tmp));
        fc_strlcat(tmp, ".gov_req", sizeof(tmp));
        u->need_government = lookup_government(file, tmp, filename, NULL);
        if (u->need_government == NULL) {
          ok = FALSE;
          break;
        }
      } else {
        u->need_government = NULL; /* no requirement */
      }

      if (!load_ruleset_veteran(file, sec_name, &u->veteran,
                                msg, sizeof(msg))) {
        ruleset_error(LOG_ERROR, "Error loading the veteran system: %s",
                      msg);
        ok = FALSE;
        break;
      }

      if (!lookup_unit_type(file, sec_name, "obsolete_by",
                            &u->obsoleted_by, filename,
                            rule_name_get(&u->name))
          || !lookup_unit_type(file, sec_name, "convert_to",
                               &u->converted_to, filename,
                               rule_name_get(&u->name))) {
        ok = FALSE;
        break;
      }
      u->convert_time = 1; /* default */
      lookup_time(file, &u->convert_time, sec_name, "convert_time",
                  filename, rule_name_get(&u->name), &ok);
    } unit_type_iterate_end;
  }

  if (ok) {
    /* main stats: */
    unit_type_iterate(u) {
      const int i = utype_index(u);
      struct unit_class *pclass;
      const char *sec_name = section_name(section_list_get(sec, i));
      const char *string;

      if (!lookup_building(file, sec_name, "impr_req",
                           &u->need_improvement, filename,
                           rule_name_get(&u->name))) {
        ok = FALSE;
        break;
      }

      sval = secfile_lookup_str(file, "%s.class", sec_name);
      pclass = unit_class_by_rule_name(sval);
      if (!pclass) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" unit_type \"%s\":"
                      " bad class \"%s\".",
                      filename,
                      utype_rule_name(u),
                      sval);
        ok = FALSE;
        break;
      }
      u->uclass = pclass;
    
      sz_strlcpy(u->sound_move,
                 secfile_lookup_str_default(file, "-", "%s.sound_move",
                                            sec_name));
      sz_strlcpy(u->sound_move_alt,
                 secfile_lookup_str_default(file, "-", "%s.sound_move_alt",
                                            sec_name));
      sz_strlcpy(u->sound_fight,
                 secfile_lookup_str_default(file, "-", "%s.sound_fight",
                                            sec_name));
      sz_strlcpy(u->sound_fight_alt,
                 secfile_lookup_str_default(file, "-", "%s.sound_fight_alt",
                                            sec_name));

      if ((string = secfile_lookup_str(file, "%s.graphic", sec_name))) {
        sz_strlcpy(u->graphic_str, string);
      } else {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }
      sz_strlcpy(u->graphic_alt,
                 secfile_lookup_str_default(file, "-", "%s.graphic_alt",
                                            sec_name));

      if (!secfile_lookup_int(file, &u->build_cost,
                              "%s.build_cost", sec_name)
          || !secfile_lookup_int(file, &u->pop_cost,
                                 "%s.pop_cost", sec_name)
          || !secfile_lookup_int(file, &u->attack_strength,
                                 "%s.attack", sec_name)
          || !secfile_lookup_int(file, &u->defense_strength,
                                 "%s.defense", sec_name)
          || !secfile_lookup_int(file, &u->move_rate,
                                 "%s.move_rate", sec_name)
          || !secfile_lookup_int(file, &u->vision_radius_sq,
                                 "%s.vision_radius_sq", sec_name)
          || !secfile_lookup_int(file, &u->transport_capacity,
                                 "%s.transport_cap", sec_name)
          || !secfile_lookup_int(file, &u->hp,
                                 "%s.hitpoints", sec_name)
          || !secfile_lookup_int(file, &u->firepower,
                                 "%s.firepower", sec_name)
          || !secfile_lookup_int(file, &u->fuel,
                                 "%s.fuel", sec_name)
          || !secfile_lookup_int(file, &u->happy_cost,
                                 "%s.uk_happy", sec_name)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }
      u->move_rate *= SINGLE_MOVE;

      if (u->firepower <= 0) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" unit_type \"%s\":"
                      " firepower is %d,"
                      " but must be at least 1. "
                      "  If you want no attack ability,"
                      " set the unit's attack strength to 0.",
                      filename,
                      utype_rule_name(u),
                      u->firepower);
        ok = FALSE;
        break;
      }

      lookup_cbonus_list(compat, u->bonuses, file, sec_name, "bonuses");

      output_type_iterate(o) {
        u->upkeep[o] = secfile_lookup_int_default(file, 0, "%s.uk_%s",
                                                  sec_name,
                                                  get_output_identifier(o));
      } output_type_iterate_end;

      slist = secfile_lookup_str_vec(file, &nval, "%s.cargo", sec_name);
      BV_CLR_ALL(u->cargo);
      for (j = 0; j < nval; j++) {
        struct unit_class *uclass = unit_class_by_rule_name(slist[j]);

        if (!uclass) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" unit_type \"%s\":"
                        "has unknown unit class %s as cargo.",
                        filename,
                        utype_rule_name(u),
                        slist[j]);
          ok = FALSE;
          break;
        }

        BV_SET(u->cargo, uclass_index(uclass));
      }
      free(slist);

      if (!ok) {
        break;
      }

      slist = secfile_lookup_str_vec(file, &nval, "%s.targets", sec_name);
      BV_CLR_ALL(u->targets);
      for (j = 0; j < nval; j++) {
        struct unit_class *uclass = unit_class_by_rule_name(slist[j]);

        if (!uclass) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" unit_type \"%s\":"
                        "has unknown unit class %s as target.",
                        filename,
                        utype_rule_name(u),
                        slist[j]);
          ok = FALSE;
          break;
        }

        BV_SET(u->targets, uclass_index(uclass));
      }
      free(slist);

      if (!ok) {
        break;
      }

      slist = secfile_lookup_str_vec(file, &nval, "%s.embarks", sec_name);
      BV_CLR_ALL(u->embarks);
      for (j = 0; j < nval; j++) {
        struct unit_class *uclass = unit_class_by_rule_name(slist[j]);

        if (!uclass) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" unit_type \"%s\":"
                        "has unknown unit class %s as embarkable.",
                        filename,
                        utype_rule_name(u),
                        slist[j]);
          ok = FALSE;
          break;
        }

        BV_SET(u->embarks, uclass_index(uclass));
      }
      free(slist);

      if (!ok) {
        break;
      }

      slist = secfile_lookup_str_vec(file, &nval, "%s.disembarks", sec_name);
      BV_CLR_ALL(u->disembarks);
      for (j = 0; j < nval; j++) {
        struct unit_class *uclass = unit_class_by_rule_name(slist[j]);

        if (!uclass) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" unit_type \"%s\":"
                        "has unknown unit class %s as disembarkable.",
                        filename,
                        utype_rule_name(u),
                        slist[j]);
          ok = FALSE;
          break;
        }

        BV_SET(u->disembarks, uclass_index(uclass));
      }
      free(slist);

      if (!ok) {
        break;
      }

      /* Set also all classes that are never unreachable as targets,
       * embarks, and disembarks. */
      unit_class_iterate(preachable) {
        if (!uclass_has_flag(preachable, UCF_UNREACHABLE)) {
          BV_SET(u->targets, uclass_index(preachable));
          BV_SET(u->embarks, uclass_index(preachable));
          BV_SET(u->disembarks, uclass_index(preachable));
        }
      } unit_class_iterate_end;

      u->vlayer = vision_layer_by_name(secfile_lookup_str_default(file, "Main",
                                                                  "%s.vision_layer",
                                                                  sec_name),
                                       fc_strcasecmp);
      if (!vision_layer_is_valid(u->vlayer)) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" unit_type \"%s\":"
                      "has unknown vision layer %s.",
                      filename,
                      utype_rule_name(u),
                      slist[j]);
        ok = FALSE;
        break;
      }

      u->helptext = lookup_strvec(file, sec_name, "helptext");

      u->paratroopers_range = secfile_lookup_int_default(file,
          0, "%s.paratroopers_range", sec_name);
      u->paratroopers_mr_req = SINGLE_MOVE * secfile_lookup_int_default(file,
          0, "%s.paratroopers_mr_req", sec_name);
      u->paratroopers_mr_sub = SINGLE_MOVE * secfile_lookup_int_default(file,
          0, "%s.paratroopers_mr_sub", sec_name);
      u->bombard_rate = secfile_lookup_int_default(file, 0,
                                                   "%s.bombard_rate", sec_name);
      u->city_slots = secfile_lookup_int_default(file, 0,
                                                 "%s.city_slots", sec_name);
      u->city_size = secfile_lookup_int_default(file, 1,
                                                "%s.city_size", sec_name);
    } unit_type_iterate_end;
  }

  if (ok) {
    /* flags */
    unit_type_iterate(u) {
      const int i = utype_index(u);

      BV_CLR_ALL(u->flags);
      fc_assert(!utype_has_flag(u, UTYF_LAST_USER_FLAG - 1));

      slist = secfile_lookup_str_vec(file, &nval, "%s.flags",
                                     section_name(section_list_get(sec, i)));
      for (j = 0; j < nval; j++) {
        sval = slist[j];
        if (0 == strcmp(sval, "")) {
          continue;
        }
        if (compat->compat_mode && !fc_strcasecmp("Partial_Invis", sval)) {
          u->vlayer = V_INVIS;
        } else {
          ival = unit_type_flag_id_by_name(rscompat_utype_flag_name_3_1(compat, sval),
                                           fc_strcasecmp);
          if (!unit_type_flag_id_is_valid(ival)) {
            ok = FALSE;
            ival = unit_class_flag_id_by_name(sval, fc_strcasecmp);
            if (unit_class_flag_id_is_valid(ival)) {
              ruleset_error(LOG_ERROR, "\"%s\" unit_type \"%s\": unit_class flag!",
                            filename, utype_rule_name(u));
            } else {
              ruleset_error(LOG_ERROR,
                            "\"%s\" unit_type \"%s\": bad flag name \"%s\".",
                            filename, utype_rule_name(u),  sval);
            }
            break;
          } else {
            BV_SET(u->flags, ival);
          }
          fc_assert(utype_has_flag(u, ival));
        }
      }
      free(slist);

      if (!ok) {
        break;
      }
    } unit_type_iterate_end;
  }

  /* roles */
  if (ok) {
    unit_type_iterate(u) {
      const int i = utype_index(u);

      BV_CLR_ALL(u->roles);
    
      slist = secfile_lookup_str_vec(file, &nval, "%s.roles",
                                     section_name(section_list_get(sec, i)));
      for (j = 0; j < nval; j++) {
        sval = slist[j];
        if (strcmp(sval, "") == 0) {
          continue;
        }
        ival = unit_role_id_by_name(sval, fc_strcasecmp);
        if (!unit_role_id_is_valid(ival)) {
          ruleset_error(LOG_ERROR, "\"%s\" unit_type \"%s\": bad role name \"%s\".",
                        filename, utype_rule_name(u), sval);
          ok = FALSE;
          break;
        } else {
          BV_SET(u->roles, ival - L_FIRST);
        }
        fc_assert(utype_has_role(u, ival));
      }
      free(slist);
    } unit_type_iterate_end;
  }

  if (ok) { 
    /* Some more consistency checking: */
    unit_type_iterate(u) {
      if (!valid_advance(u->require_advance)) {
        ruleset_error(LOG_ERROR, "\"%s\" unit_type \"%s\": depends on removed tech \"%s\".",
                       filename, utype_rule_name(u),
                       advance_rule_name(u->require_advance));
        u->require_advance = A_NEVER;
        ok = FALSE;
        break;
      }

      if (utype_has_flag(u, UTYF_SETTLERS)
          && u->city_size <= 0) {
        ruleset_error(LOG_ERROR, "\"%s\": Unit %s would build size %d cities",
                      filename, utype_rule_name(u), u->city_size);
        u->city_size = 1;
        ok = FALSE;
        break;
      }
    } unit_type_iterate_end;
  }

  section_list_destroy(csec);
  section_list_destroy(sec);

  if (ok) {
    secfile_check_unused(file);
  }

  return ok;
}

/**********************************************************************//**
  Load names of buildings so other rulesets can refer to buildings with
  their name.
**************************************************************************/
static bool load_building_names(struct section_file *file,
                                struct rscompat_info *compat)
{
  struct section_list *sec;
  int i, nval = 0;
  const char *filename = secfile_name(file);
  bool ok = TRUE;

  compat->ver_buildings = rscompat_check_capabilities(file, filename,
                                                      compat);
  if (compat->ver_buildings <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  /* The names: */
  sec = secfile_sections_by_name_prefix(file, BUILDING_SECTION_PREFIX);
  if (NULL == sec || 0 == (nval = section_list_size(sec))) {
    ruleset_error(LOG_ERROR, "\"%s\": No improvements?!?", filename);
    ok = FALSE;
  } else {
    log_verbose("%d improvement types (including possibly unused)", nval);
    if (nval > B_LAST) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many improvements (%d, max %d)",
                    filename, nval, B_LAST);
      ok = FALSE;
    }
  }

  if (ok) {
    game.control.num_impr_types = nval;

    for (i = 0; i < nval; i++) {
      struct impr_type *b = improvement_by_number(i);

      if (!ruleset_load_names(&b->name, NULL, file, section_name(section_list_get(sec, i)))) {
        ok = FALSE;
        break;
      }
    }
  }

  section_list_destroy(sec);

  return ok;
}

/**********************************************************************//**
  Load buildings related ruleset data
**************************************************************************/
static bool load_ruleset_buildings(struct section_file *file,
                                   struct rscompat_info *compat)
{
  struct section_list *sec;
  const char *item;
  int i, nval;
  const char *filename = secfile_name(file);
  bool ok = TRUE;

  sec = secfile_sections_by_name_prefix(file, BUILDING_SECTION_PREFIX);
  nval = (NULL != sec ? section_list_size(sec) : 0);

  for (i = 0; i < nval && ok; i++) {
    struct impr_type *b = improvement_by_number(i);
    const char *sec_name = section_name(section_list_get(sec, i));
    struct requirement_vector *reqs =
      lookup_req_list(file, compat, sec_name, "reqs",
                      improvement_rule_name(b));

    if (reqs == NULL) {
      ok = FALSE;
      break;
    } else {
      const char *sval, **slist;
      int j, ival;
      size_t nflags;

      item = secfile_lookup_str(file, "%s.genus", sec_name);
      b->genus = impr_genus_id_by_name(item, fc_strcasecmp);
      if (!impr_genus_id_is_valid(b->genus)) {
        ruleset_error(LOG_ERROR, "\"%s\" improvement \"%s\": couldn't match "
                      "genus \"%s\".", filename,
                      improvement_rule_name(b), item);
        ok = FALSE;
        break;
      }

      slist = secfile_lookup_str_vec(file, &nflags, "%s.flags", sec_name);
      BV_CLR_ALL(b->flags);

      for (j = 0; j < nflags; j++) {
        sval = slist[j];
        if (strcmp(sval,"") == 0) {
          continue;
        }
        ival = impr_flag_id_by_name(sval, fc_strcasecmp);
        if (!impr_flag_id_is_valid(ival)) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" improvement \"%s\": bad flag name \"%s\".",
                        filename, improvement_rule_name(b), sval);
          ok = FALSE;
          break;
        } else {
          BV_SET(b->flags, ival);
        }
      }
      free(slist);

      if (!ok) {
        break;
      }

      requirement_vector_copy(&b->reqs, reqs);

      {
        struct requirement_vector *obs_reqs =
          lookup_req_list(file, compat, sec_name, "obsolete_by",
                          improvement_rule_name(b));

        if (obs_reqs == NULL) {
          ok = FALSE;
          break;
        } else {
          requirement_vector_copy(&b->obsolete_by, obs_reqs);
        }
      }

      if (!secfile_lookup_int(file, &b->build_cost,
                              "%s.build_cost", sec_name)
          || !secfile_lookup_int(file, &b->upkeep,
                                 "%s.upkeep", sec_name)
          || !secfile_lookup_int(file, &b->sabotage,
                                 "%s.sabotage", sec_name)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }

      sz_strlcpy(b->graphic_str,
                 secfile_lookup_str_default(file, "-",
                                            "%s.graphic", sec_name));
      sz_strlcpy(b->graphic_alt,
                 secfile_lookup_str_default(file, "-",
                                            "%s.graphic_alt", sec_name));

      sz_strlcpy(b->soundtag,
                 secfile_lookup_str_default(file, "-",
                                            "%s.sound", sec_name));
      sz_strlcpy(b->soundtag_alt,
                 secfile_lookup_str_default(file, "-",
                                            "%s.sound_alt", sec_name));
      b->helptext = lookup_strvec(file, sec_name, "helptext");
    }
  }

  section_list_destroy(sec);
  if (ok) {
    secfile_check_unused(file);
  }

  return ok;
}

/**********************************************************************//**
  Load names of terrain types so other rulesets can refer to terrains with
  their name.
**************************************************************************/
static bool load_terrain_names(struct section_file *file,
                               struct rscompat_info *compat)
{
  int nval = 0;
  struct section_list *sec = NULL;
  const char *flag;
  int i;
  const char *filename = secfile_name(file);
  bool ok = TRUE;

  compat->ver_terrain = rscompat_check_capabilities(file, filename,
                                                    compat);
  if (compat->ver_terrain <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  /* User terrain flag names */
  for (i = 0; (flag = secfile_lookup_str_default(file, NULL, "control.flags%d.name", i)) ;
       i++) {
    const char *helptxt = secfile_lookup_str_default(file, NULL, "control.flags%d.helptxt",
                                                     i);

    if (terrain_flag_id_by_name(flag, fc_strcasecmp)
        != terrain_flag_id_invalid()) {
      ruleset_error(LOG_ERROR, "\"%s\": Duplicate terrain flag name '%s'",
                    filename, flag);
      ok = FALSE;
      break;
    }
    if (i > MAX_NUM_USER_TER_FLAGS) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many user terrain flags!",
                    filename);
      ok = FALSE;
      break;
    }

    set_user_terrain_flag_name(TER_USER_1 + i, flag, helptxt);
  }

  if (ok) {
    /* Blank the remaining terrain user flag slots. */
    for (; i < MAX_NUM_USER_TER_FLAGS; i++) {
      set_user_terrain_flag_name(TER_USER_1 + i, NULL, NULL);
    }
  }

  /* User extra flag names */
  for (i = 0;
       (flag = secfile_lookup_str_default(file, NULL,
                                          "control.extra_flags%d.name",
                                          i));
       i++) {
    const char *helptxt = secfile_lookup_str_default(file, NULL,
        "control.extra_flags%d.helptxt", i);

    if (extra_flag_id_by_name(flag, fc_strcasecmp)
        != extra_flag_id_invalid()) {
      ruleset_error(LOG_ERROR, "\"%s\": Duplicate extra flag name '%s'",
                    filename, flag);
      ok = FALSE;
      break;
    }
    if (i > MAX_NUM_USER_EXTRA_FLAGS) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many user extra flags!",
                    filename);
      ok = FALSE;
      break;
    }

    set_user_extra_flag_name(EF_USER_FLAG_1 + i, flag, helptxt);
  }

  if (ok) {
    /* Blank the remaining extra user flag slots. */
    for (; i < MAX_NUM_USER_EXTRA_FLAGS; i++) {
      set_user_extra_flag_name(EF_USER_FLAG_1 + i, NULL, NULL);
    }

    /* terrain names */

    sec = secfile_sections_by_name_prefix(file, TERRAIN_SECTION_PREFIX);
    if (NULL == sec || 0 == (nval = section_list_size(sec))) {
      ruleset_error(LOG_ERROR, "\"%s\": ruleset doesn't have any terrains.",
                    filename);
      ok = FALSE;
    } else {
      if (nval > MAX_NUM_TERRAINS) {
        ruleset_error(LOG_ERROR, "\"%s\": Too many terrains (%d, max %d)",
                      filename, nval, MAX_NUM_TERRAINS);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    game.control.terrain_count = nval;

    /* avoid re-reading files */
    if (terrain_sections) {
      free(terrain_sections);
    }
    terrain_sections = fc_calloc(nval, MAX_SECTION_LABEL);

    terrain_type_iterate(pterrain) {
      const int terri = terrain_index(pterrain);
      const char *sec_name = section_name(section_list_get(sec, terri));

      if (!ruleset_load_names(&pterrain->name, NULL, file, sec_name)) {
        ok = FALSE;
        break;
      }

      section_strlcpy(&terrain_sections[terri * MAX_SECTION_LABEL], sec_name);
    } terrain_type_iterate_end;
  }

  section_list_destroy(sec);
  sec = NULL;

  /* extra names */

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, EXTRA_SECTION_PREFIX);
    nval = (NULL != sec ? section_list_size(sec) : 0);
    if (nval > MAX_EXTRA_TYPES) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many extra types (%d, max %d)",
                    filename, nval, MAX_EXTRA_TYPES);
      ok = FALSE;
    }
  }

  if (ok) {
    int idx;

    game.control.num_extra_types = nval;

    if (extra_sections) {
      free(extra_sections);
    }
    extra_sections = fc_calloc(nval, MAX_SECTION_LABEL);

    if (ok) {
      for (idx = 0; idx < nval; idx++) {
        const char *sec_name = section_name(section_list_get(sec, idx));
        struct extra_type *pextra = extra_by_number(idx);

        if (!ruleset_load_names(&pextra->name, NULL, file, sec_name)) {
          ok = FALSE;
          break;
        }
        section_strlcpy(&extra_sections[idx * MAX_SECTION_LABEL], sec_name);
      }
    }
  }

  section_list_destroy(sec);
  sec = NULL;

  /* base names */

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, BASE_SECTION_PREFIX);
    nval = (NULL != sec ? section_list_size(sec) : 0);
    if (nval > MAX_BASE_TYPES) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many base types (%d, max %d)",
                    filename, nval, MAX_BASE_TYPES);
      ok = FALSE;
    }

    game.control.num_base_types = nval;
  }

  if (ok) {
    int idx;

    if (base_sections) {
      free(base_sections);
    }
    base_sections = fc_calloc(nval, MAX_SECTION_LABEL);

    /* Cannot use base_type_iterate() before bases are added to
     * EC_BASE caused_by list. Have to get them by extra_type_by_rule_name() */
    for (idx = 0; idx < nval; idx++) {
      const char *sec_name = section_name(section_list_get(sec, idx));
      const char *base_name = secfile_lookup_str(file, "%s.extra", sec_name);

      if (base_name != NULL) {
        struct extra_type *pextra = extra_type_by_rule_name(base_name);

        if (pextra != NULL) {
          base_type_init(pextra, idx);
          section_strlcpy(&base_sections[idx * MAX_SECTION_LABEL], sec_name);
        } else {
          ruleset_error(LOG_ERROR,
                        "No extra definition matching base definition \"%s\"",
                        base_name);
          ok = FALSE;
        }
      } else {
        ruleset_error(LOG_ERROR,
                      "Base section \"%s\" does not associate base with any extra",
                      sec_name);
        ok = FALSE;
      }
    }
  }

  section_list_destroy(sec);
  sec = NULL;

  /* road names */

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, ROAD_SECTION_PREFIX);
    nval = (NULL != sec ? section_list_size(sec) : 0);
    if (nval > MAX_ROAD_TYPES) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many road types (%d, max %d)",
                    filename, nval, MAX_ROAD_TYPES);
      ok = FALSE;
    }

    game.control.num_road_types = nval;
  }

  if (ok) {
    int idx;

    if (road_sections) {
      free(road_sections);
    }
    road_sections = fc_calloc(nval, MAX_SECTION_LABEL);

    /* Cannot use extra_type_by_cause_iterate(EC_ROAD) before roads are added to
     * EC_ROAD caused_by list. Have to get them by extra_type_by_rule_name() */
    for (idx = 0; idx < nval; idx++) {
      const char *sec_name = section_name(section_list_get(sec, idx));
      const char *road_name = secfile_lookup_str(file, "%s.extra", sec_name);

      if (road_name != NULL) {
        struct extra_type *pextra = extra_type_by_rule_name(road_name);

        if (pextra != NULL) {
          road_type_init(pextra, idx);
          section_strlcpy(&road_sections[idx * MAX_SECTION_LABEL], sec_name);
        } else {
          ruleset_error(LOG_ERROR,
                        "No extra definition matching road definition \"%s\"",
                        road_name);
          ok = FALSE;
        }
      } else {
        ruleset_error(LOG_ERROR,
                      "Road section \"%s\" does not associate road with any extra",
                      sec_name);
        ok = FALSE;
      }
    }
  }

  section_list_destroy(sec);
  sec = NULL;

  /* resource names */

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, RESOURCE_SECTION_PREFIX);
    nval = (NULL != sec ? section_list_size(sec) : 0);
    if (nval > MAX_RESOURCE_TYPES) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many resource types (%d, max %d)",
                    filename, nval, MAX_RESOURCE_TYPES);
      ok = FALSE;
    }

    game.control.num_resource_types = nval;
  }

  if (ok) {
    int idx;

    if (resource_sections) {
      free(resource_sections);
    }
    resource_sections = fc_calloc(nval, MAX_SECTION_LABEL);

    /* Cannot use resource_type_iterate() before resource are added to
     * EC_RESOURCE caused_by list. Have to get them by extra_type_by_rule_name() */
    for (idx = 0; idx < nval; idx++) {
      const char *sec_name = section_name(section_list_get(sec, idx));
      const char *resource_name;
      struct extra_type *pextra = NULL;

      resource_name = secfile_lookup_str_default(file, NULL, "%s.extra", sec_name);

      if (resource_name != NULL) {
        pextra = extra_type_by_rule_name(resource_name);

        if (pextra != NULL) {
          resource_type_init(pextra);
          section_strlcpy(&resource_sections[idx * MAX_SECTION_LABEL], sec_name);
        } else {
          ruleset_error(LOG_ERROR,
                        "No extra definition matching resource definition \"%s\"",
                        resource_name);
          ok = FALSE;
        }
      } else {
        ruleset_error(LOG_ERROR,
                      "Resource section %s does not list extra this resource belongs to.",
                      sec_name);
        ok = FALSE;
      }
    }
  }

  section_list_destroy(sec);

  return ok;
}

/**********************************************************************//**
  Load terrain types related ruleset data
**************************************************************************/
static bool load_ruleset_terrain(struct section_file *file,
                                 struct rscompat_info *compat)
{
  size_t nval;
  int j;
  bool compat_road = FALSE;
  bool compat_rail = FALSE;
  bool compat_river = FALSE;
  const char **res;
  const char *filename = secfile_name(file);
  const char *text;
  bool ok = TRUE;

  /* parameters */

  terrain_control.ocean_reclaim_requirement_pct
    = secfile_lookup_int_default(file, 101,
				 "parameters.ocean_reclaim_requirement");
  terrain_control.land_channel_requirement_pct
    = secfile_lookup_int_default(file, 101,
				 "parameters.land_channel_requirement");
  terrain_control.terrain_thaw_requirement_pct
    = secfile_lookup_int_default(file, 101,
				 "parameters.thaw_requirement");
  terrain_control.terrain_freeze_requirement_pct
    = secfile_lookup_int_default(file, 101,
				 "parameters.freeze_requirement");
  terrain_control.lake_max_size
    = secfile_lookup_int_default(file, 0,
				 "parameters.lake_max_size");
  terrain_control.min_start_native_area
    = secfile_lookup_int_default(file, 0,
                                 "parameters.min_start_native_area");
  terrain_control.move_fragments
    = secfile_lookup_int_default(file, 3,
                                 "parameters.move_fragments");
  if (terrain_control.move_fragments < 1) {
    ruleset_error(LOG_ERROR, "\"%s\": move_fragments must be at least 1",
                  filename);
    ok = FALSE;
  }
  init_move_fragments();
  terrain_control.igter_cost
    = secfile_lookup_int_default(file, 1,
                                 "parameters.igter_cost");
  if (terrain_control.igter_cost < 1) {
    ruleset_error(LOG_ERROR, "\"%s\": igter_cost must be at least 1",
                  filename);
    ok = FALSE;
  }
  terrain_control.pythagorean_diagonal
    = secfile_lookup_bool_default(file, RS_DEFAULT_PYTHAGOREAN_DIAGONAL,
                                  "parameters.pythagorean_diagonal");

  wld.map.server.ocean_resources
    = secfile_lookup_bool_default(file, FALSE,
                                  "parameters.ocean_resources");

  text = secfile_lookup_str_default(file,
                                    N_("?gui_type:Build Type A Base"),
                                    "extraui.ui_name_base_fortress");
  sz_strlcpy(terrain_control.gui_type_base0, text);

  text = secfile_lookup_str_default(file,
                                    N_("?gui_type:Build Type B Base"),
                                    "extraui.ui_name_base_airbase");
  sz_strlcpy(terrain_control.gui_type_base1, text);

  if (ok) {
    /* terrain details */

    terrain_type_iterate(pterrain) {
      const char **slist;
      const int i = terrain_index(pterrain);
      const char *tsection = &terrain_sections[i * MAX_SECTION_LABEL];
      const char *cstr;

      sz_strlcpy(pterrain->graphic_str,
                 secfile_lookup_str(file,"%s.graphic", tsection));
      sz_strlcpy(pterrain->graphic_alt,
                 secfile_lookup_str(file,"%s.graphic_alt", tsection));

      pterrain->identifier
        = secfile_lookup_str(file, "%s.identifier", tsection)[0];
      if ('\0' == pterrain->identifier) {
        ruleset_error(LOG_ERROR, "\"%s\" [%s] identifier missing value.",
                      filename, tsection);
        ok = FALSE;
        break;
      }
      if (TERRAIN_UNKNOWN_IDENTIFIER == pterrain->identifier) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" [%s] cannot use '%c' as an identifier;"
                      " it is reserved for unknown terrain.",
                      filename, tsection, pterrain->identifier);
        ok = FALSE;
        break;
      }
      for (j = T_FIRST; j < i; j++) {
        if (pterrain->identifier == terrain_by_number(j)->identifier) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" [%s] has the same identifier as [%s].",
                        filename,
                        tsection,
                        &terrain_sections[j * MAX_SECTION_LABEL]);
          ok = FALSE;
          break;
        }
      }

      if (!ok) {
        break;
      }

      cstr = secfile_lookup_str(file, "%s.class", tsection);
      pterrain->tclass = terrain_class_by_name(cstr, fc_strcasecmp);
      if (!terrain_class_is_valid(pterrain->tclass)) {
        ruleset_error(LOG_ERROR, "\"%s\": [%s] unknown class \"%s\"",
                      filename, tsection, cstr);
        ok = FALSE;
        break;
      }

      if (!secfile_lookup_int(file, &pterrain->movement_cost,
                              "%s.movement_cost", tsection)
          || !secfile_lookup_int(file, &pterrain->defense_bonus,
                                 "%s.defense_bonus", tsection)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }

      output_type_iterate(o) {
        pterrain->output[o]
          = secfile_lookup_int_default(file, 0, "%s.%s", tsection,
                                       get_output_identifier(o));
      } output_type_iterate_end;

      res = secfile_lookup_str_vec(file, &nval, "%s.resources", tsection);
      pterrain->resources = fc_calloc(nval + 1, sizeof(*pterrain->resources));
      for (j = 0; j < nval; j++) {
        pterrain->resources[j] = lookup_resource(filename, res[j], tsection);
        if (pterrain->resources[j] == NULL) {
          ok = FALSE;
          break;
        }
      }
      pterrain->resources[nval] = NULL;
      free(res);
      res = NULL;

      if (!ok) {
        break;
      }

      output_type_iterate(o) {
        pterrain->road_output_incr_pct[o]
          = secfile_lookup_int_default(file, 0, "%s.road_%s_incr_pct",
                                       tsection, get_output_identifier(o));
      } output_type_iterate_end;

      if (!lookup_time(file, &pterrain->base_time, tsection, "base_time",
                       filename, NULL, &ok)
          || !lookup_time(file, &pterrain->road_time, tsection, "road_time",
                          filename, NULL, &ok)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }

      if (!lookup_terrain(file, "irrigation_result", filename, pterrain,
                          &pterrain->irrigation_result)) {
        ok = FALSE;
        break;
      }
      if (!secfile_lookup_int(file, &pterrain->irrigation_food_incr,
                              "%s.irrigation_food_incr", tsection)
          || !lookup_time(file, &pterrain->irrigation_time,
                          tsection, "irrigation_time", filename, NULL, &ok)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }

      if (!lookup_terrain(file, "mining_result", filename, pterrain,
                          &pterrain->mining_result)) {
        ok = FALSE;
        break;
      }
      if (!secfile_lookup_int(file, &pterrain->mining_shield_incr,
                              "%s.mining_shield_incr", tsection)
          || !lookup_time(file, &pterrain->mining_time,
                          tsection, "mining_time", filename, NULL, &ok)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }

      if (!lookup_unit_type(file, tsection, "animal",
                            &pterrain->animal, filename,
                            rule_name_get(&pterrain->name))) {
        ok = FALSE;
        break;
      }

      if (!lookup_terrain(file, "transform_result", filename, pterrain,
                          &pterrain->transform_result)) {
        ok = FALSE;
        break;
      }
      if (!lookup_time(file, &pterrain->transform_time,
                       tsection, "transform_time", filename, NULL, &ok)) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }
      pterrain->pillage_time = 1; /* default */
      lookup_time(file, &pterrain->pillage_time,
                  tsection, "pillage_time", filename, NULL, &ok);
      pterrain->clean_pollution_time = 3; /* default */
      lookup_time(file, &pterrain->clean_pollution_time,
                  tsection, "clean_pollution_time", filename, NULL, &ok);
      pterrain->clean_fallout_time = 3; /* default */
      lookup_time(file, &pterrain->clean_fallout_time,
                  tsection, "clean_fallout_time", filename, NULL, &ok);

      if (!lookup_terrain(file, "warmer_wetter_result", filename, pterrain,
                         &pterrain->warmer_wetter_result)
          || !lookup_terrain(file, "warmer_drier_result", filename, pterrain,
                             &pterrain->warmer_drier_result)
          || !lookup_terrain(file, "cooler_wetter_result", filename, pterrain,
                             &pterrain->cooler_wetter_result)
          || !lookup_terrain(file, "cooler_drier_result", filename, pterrain,
                             &pterrain->cooler_drier_result)) {
        ok = FALSE;
        break;
      }

      slist = secfile_lookup_str_vec(file, &nval, "%s.flags", tsection);
      BV_CLR_ALL(pterrain->flags);
      for (j = 0; j < nval; j++) {
        const char *sval = slist[j];
        enum terrain_flag_id flag
          = terrain_flag_id_by_name(sval, fc_strcasecmp);

        if (!terrain_flag_id_is_valid(flag)) {
          ruleset_error(LOG_ERROR, "\"%s\" [%s] has unknown flag \"%s\".",
                        filename, tsection, sval);
          ok = FALSE;
          break;
        } else {
          BV_SET(pterrain->flags, flag);
        }
      }
      free(slist);

      if (!ok) {
        break;
      }

      {
        enum mapgen_terrain_property mtp;
        for (mtp = mapgen_terrain_property_begin();
             mtp != mapgen_terrain_property_end();
             mtp = mapgen_terrain_property_next(mtp)) {
          pterrain->property[mtp]
            = secfile_lookup_int_default(file, 0, "%s.property_%s", tsection,
                                         mapgen_terrain_property_name(mtp));
        }
      }

      slist = secfile_lookup_str_vec(file, &nval, "%s.native_to", tsection);
      BV_CLR_ALL(pterrain->native_to);
      for (j = 0; j < nval; j++) {
        struct unit_class *class = unit_class_by_rule_name(slist[j]);

        if (!class) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" [%s] is native to unknown unit class \"%s\".",
                        filename, tsection, slist[j]);
          ok = FALSE;
          break;
        } else {
          BV_SET(pterrain->native_to, uclass_index(class));
        }
      }
      free(slist);

      if (!ok) {
        break;
      }

      /* get terrain color */
      {
        fc_assert_ret_val(pterrain->rgb == NULL, FALSE);
        if (!rgbcolor_load(file, &pterrain->rgb, "%s.color", tsection)) {
          ruleset_error(LOG_ERROR, "Missing terrain color definition: %s",
                        secfile_error());
          ok = FALSE;
          break;
        }
      }

      pterrain->helptext = lookup_strvec(file, tsection, "helptext");
    } terrain_type_iterate_end;
  }

  if (ok) {
    /* extra details */
    extra_type_iterate(pextra) {
      BV_CLR_ALL(pextra->conflicts);
    } extra_type_iterate_end;

    extra_type_iterate(pextra) {
      if (!compat->compat_mode || compat->ver_terrain >= 10 || pextra->category != ECAT_RESOURCE) {
        const char *section = &extra_sections[extra_index(pextra) * MAX_SECTION_LABEL];
        const char **slist;
        struct requirement_vector *reqs;
        const char *catname;
        int cj;
        enum extra_cause cause;
        enum extra_rmcause rmcause;
        const char *eus_name;
        const char *vis_req_name;
        const struct advance *vis_req;

        catname = secfile_lookup_str(file, "%s.category", section);
        if (catname == NULL) {
          ruleset_error(LOG_ERROR, "\"%s\" extra \"%s\" has no category.",
                        filename,
                        extra_rule_name(pextra));
          ok = FALSE;
          break;
        }
        pextra->category = extra_category_by_name(catname, fc_strcasecmp);
        if (!extra_category_is_valid(pextra->category)) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" extra \"%s\" has invalid category \"%s\".",
                        filename, extra_rule_name(pextra), catname);
          ok = FALSE;
          break;
        }

        slist = secfile_lookup_str_vec(file, &nval, "%s.causes", section);
        pextra->causes = 0;
        for (cj = 0; cj < nval; cj++) {
          const char *sval = slist[cj];
          cause = extra_cause_by_name(sval, fc_strcasecmp);

          if (!extra_cause_is_valid(cause)) {
            ruleset_error(LOG_ERROR, "\"%s\" extra \"%s\": unknown cause \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          sval);
            ok = FALSE;
            break;
          } else {
            pextra->causes |= (1 << cause);
            extra_to_caused_by_list(pextra, cause);
          }
        }

        if (pextra->causes == 0) {
          /* Extras that do not have any causes added to EC_NONE list */
          extra_to_caused_by_list(pextra, EC_NONE);
        }

        if (!is_extra_caused_by(pextra, EC_BASE)
            && !is_extra_caused_by(pextra, EC_ROAD)
            && !is_extra_caused_by(pextra, EC_RESOURCE)) {
          /* Not a base, road, nor resource, so special */
          pextra->data.special_idx = extra_type_list_size(extra_type_list_by_cause(EC_SPECIAL));
          extra_to_caused_by_list(pextra, EC_SPECIAL);
        }

        free(slist);

        slist = secfile_lookup_str_vec(file, &nval, "%s.rmcauses", section);
        pextra->rmcauses = 0;
        for (j = 0; j < nval; j++) {
          const char *sval = slist[j];
          rmcause = extra_rmcause_by_name(sval, fc_strcasecmp);

          if (!extra_rmcause_is_valid(rmcause)) {
            ruleset_error(LOG_ERROR, "\"%s\" extra \"%s\": unknown rmcause \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          sval);
            ok = FALSE;
            break;
          } else {
            pextra->rmcauses |= (1 << rmcause);
            extra_to_removed_by_list(pextra, rmcause);
          }
        }

        free(slist);

        sz_strlcpy(pextra->activity_gfx,
                   secfile_lookup_str_default(file, "-",
                                              "%s.activity_gfx", section));
        sz_strlcpy(pextra->act_gfx_alt,
                   secfile_lookup_str_default(file, "-",
                                              "%s.act_gfx_alt", section));
        sz_strlcpy(pextra->act_gfx_alt2,
                   secfile_lookup_str_default(file, "-",
                                              "%s.act_gfx_alt2", section));
        sz_strlcpy(pextra->rmact_gfx,
                   secfile_lookup_str_default(file, "-",
                                              "%s.rmact_gfx", section));
        sz_strlcpy(pextra->rmact_gfx_alt,
                   secfile_lookup_str_default(file, "-",
                                              "%s.rmact_gfx_alt", section));
        sz_strlcpy(pextra->graphic_str,
                   secfile_lookup_str_default(file, "-", "%s.graphic", section));
        sz_strlcpy(pextra->graphic_alt,
                   secfile_lookup_str_default(file, "-",
                                              "%s.graphic_alt", section));

        reqs = lookup_req_list(file, compat, section, "reqs", extra_rule_name(pextra));
        if (reqs == NULL) {
          ok = FALSE;
          break;
        }
        requirement_vector_copy(&pextra->reqs, reqs);

        reqs = lookup_req_list(file, compat, section, "rmreqs", extra_rule_name(pextra));
        if (reqs == NULL) {
          ok = FALSE;
          break;
        }
        requirement_vector_copy(&pextra->rmreqs, reqs);

        reqs = lookup_req_list(file, compat, section, "appearance_reqs", extra_rule_name(pextra));
        if (reqs == NULL) {
          ok = FALSE;
          break;
        }
        requirement_vector_copy(&pextra->appearance_reqs, reqs);

        reqs = lookup_req_list(file, compat, section, "disappearance_reqs", extra_rule_name(pextra));
        if (reqs == NULL) {
          ok = FALSE;
          break;
        }
        requirement_vector_copy(&pextra->disappearance_reqs, reqs);

        pextra->buildable = secfile_lookup_bool_default(file,
                                                        is_extra_caused_by_worker_action(pextra),
                                                        "%s.buildable", section);
        pextra->generated = secfile_lookup_bool_default(file, TRUE,
                                                        "%s.generated", section);

        pextra->build_time = 0; /* default */
        lookup_time(file, &pextra->build_time, section, "build_time",
                    filename, extra_rule_name(pextra), &ok);
        pextra->build_time_factor = secfile_lookup_int_default(file, 1,
                                                               "%s.build_time_factor", section);
        pextra->removal_time = 0; /* default */
        lookup_time(file, &pextra->removal_time, section, "removal_time",
                    filename, extra_rule_name(pextra), &ok);
        pextra->removal_time_factor = secfile_lookup_int_default(file, 1,
                                                                 "%s.removal_time_factor", section);

        pextra->defense_bonus  = secfile_lookup_int_default(file, 0,
                                                            "%s.defense_bonus",
                                                            section);
        if (pextra->defense_bonus != 0) {
          if (extra_has_flag(pextra, EF_NATURAL_DEFENSE)) {
            extra_to_caused_by_list(pextra, EC_NATURAL_DEFENSIVE);
          } else {
            extra_to_caused_by_list(pextra, EC_DEFENSIVE);
          }
        }

        eus_name = secfile_lookup_str_default(file, "Normal", "%s.unit_seen", section);
        pextra->eus = extra_unit_seen_type_by_name(eus_name, fc_strcasecmp);
        if (!extra_unit_seen_type_is_valid(pextra->eus)) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" extra \"%s\" has illegal unit_seen value \"%s\".",
                        filename, extra_rule_name(pextra),
                        eus_name); 
          ok = FALSE;
          break;
        }
        if (pextra->eus == EUS_HIDDEN) {
          extra_type_list_append(extra_type_list_of_unit_hiders(), pextra);
        }

        pextra->appearance_chance = secfile_lookup_int_default(file, RS_DEFAULT_EXTRA_APPEARANCE,
                                                               "%s.appearance_chance",
                                                               section);
        pextra->disappearance_chance = secfile_lookup_int_default(file,
                                                                  RS_DEFAULT_EXTRA_DISAPPEARANCE,
                                                                  "%s.disappearance_chance",
                                                                  section);

        slist = secfile_lookup_str_vec(file, &nval, "%s.native_to", section);
        BV_CLR_ALL(pextra->native_to);
        for (j = 0; j < nval; j++) {
          struct unit_class *uclass = unit_class_by_rule_name(slist[j]);

          if (uclass == NULL) {
            ruleset_error(LOG_ERROR,
                          "\"%s\" extra \"%s\" is native to unknown unit class \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          slist[j]);
            ok = FALSE;
            break;
          } else {
            BV_SET(pextra->native_to, uclass_index(uclass));
          }
        }
        free(slist);

        if (!ok) {
          break;
        }

        slist = secfile_lookup_str_vec(file, &nval, "%s.flags", section);
        BV_CLR_ALL(pextra->flags);
        for (j = 0; j < nval; j++) {
          const char *sval = slist[j];
          enum extra_flag_id flag = extra_flag_id_by_name(sval, fc_strcasecmp);

          if (!extra_flag_id_is_valid(flag)) {
            ruleset_error(LOG_ERROR, "\"%s\" extra \"%s\": unknown flag \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          sval);
            ok = FALSE;
            break;
          } else {
            BV_SET(pextra->flags, flag);
          }
        }
        free(slist);

        if (!ok) {
          break;
        }

        slist = secfile_lookup_str_vec(file, &nval, "%s.conflicts", section);
        for (j = 0; j < nval; j++) {
          const char *sval = slist[j];
          struct extra_type *pextra2 = extra_type_by_rule_name(sval);

          if (pextra2 == NULL) {
            ruleset_error(LOG_ERROR, "\"%s\" extra \"%s\": unknown conflict extra \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          sval);
            ok = FALSE;
            break;
          } else {
            BV_SET(pextra->conflicts, extra_index(pextra2));
            BV_SET(pextra2->conflicts, extra_index(pextra));
          }
        }
    
        free(slist);

        if (!ok) {
          break;
        }

        slist = secfile_lookup_str_vec(file, &nval, "%s.hidden_by", section);
        BV_CLR_ALL(pextra->hidden_by);
        for (j = 0; j < nval; j++) {
          const char *sval = slist[j];
          const struct extra_type *top = extra_type_by_rule_name(sval);

          if (top == NULL) {
            ruleset_error(LOG_ERROR, "\"%s\" extra \"%s\" hidden by unknown extra \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          sval);
            ok = FALSE;
            break;
          } else {
            BV_SET(pextra->hidden_by, extra_index(top));
          }
        }
        free(slist);

        if (!ok) {
          break;
        }

        slist = secfile_lookup_str_vec(file, &nval, "%s.bridged_over", section);
        BV_CLR_ALL(pextra->bridged_over);
        for (j = 0; j < nval; j++) {
          const char *sval = slist[j];
          const struct extra_type *top = extra_type_by_rule_name(sval);

          if (top == NULL) {
            ruleset_error(LOG_ERROR, "\"%s\" extra \"%s\" bridged over unknown extra \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          sval);
            ok = FALSE;
            break;
          } else {
            BV_SET(pextra->bridged_over, extra_index(top));
          }
        }
        free(slist);

        if (!ok) {
          break;
        }

        vis_req_name = secfile_lookup_str_default(file, "None",
                                                  "%s.visibility_req", section);
        vis_req = advance_by_rule_name(vis_req_name);

        if (vis_req == NULL) {
          ruleset_error(LOG_ERROR, "\"%s\" %s: unknown visibility_req %s.",
                        filename, section, vis_req_name);
          ok = FALSE;
          break;
        }

        pextra->visibility_req = advance_number(vis_req);

        pextra->helptext = lookup_strvec(file, section, "helptext");
      }
    } extra_type_iterate_end;
  }

  if (ok) {
    int i = 0;
    /* resource details */

    extra_type_by_cause_iterate(EC_RESOURCE, presource) {
      char identifier[MAX_LEN_NAME];
      const char *rsection = &resource_sections[i * MAX_SECTION_LABEL];

      if (!presource->data.resource) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" extra \"%s\" has \"Resource\" cause but no "
                      "corresponding [resource_*] section",
                      filename, extra_rule_name(presource));
        ok = FALSE;
        break;
      }

      output_type_iterate (o) {
        presource->data.resource->output[o] =
	  secfile_lookup_int_default(file, 0, "%s.%s", rsection,
                                     get_output_identifier(o));
      } output_type_iterate_end;

      sz_strlcpy(identifier,
                 secfile_lookup_str(file,"%s.identifier", rsection));
      presource->data.resource->id_old_save = identifier[0];
      if (RESOURCE_NULL_IDENTIFIER == presource->data.resource->id_old_save) {
        ruleset_error(LOG_ERROR, "\"%s\" [%s] identifier missing value.",
                      filename, rsection);
        ok = FALSE;
        break;
      }
      if (RESOURCE_NONE_IDENTIFIER == presource->data.resource->id_old_save) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" [%s] cannot use '%c' as an identifier;"
                      " it is reserved.",
                      filename, rsection, presource->data.resource->id_old_save);
        ok = FALSE;
        break;
      }

      if (!ok) {
        break;
      }

      i++;
    } extra_type_by_cause_iterate_end;

    for (j = 0; ok && j < game.control.num_resource_types; j++) {
      const char *section = &resource_sections[j * MAX_SECTION_LABEL];
      const char *extra_name = secfile_lookup_str(file, "%s.extra", section);
      struct extra_type *pextra = extra_type_by_rule_name(extra_name);

      if (!is_extra_caused_by(pextra, EC_RESOURCE)) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" resource section [%s]: extra \"%s\" does not "
                      "have \"Resource\" in its causes",
                      filename, section, extra_name);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    /* This can't be part of previous loop as we don't want random data from previous
     * ruleset to play havoc on us when we have only some resource identifiers loaded
     * from the new ruleset. */
    extra_type_by_cause_iterate(EC_RESOURCE, pres) {
      extra_type_by_cause_iterate(EC_RESOURCE, pres2) {
        if (pres->data.resource->id_old_save == pres2->data.resource->id_old_save
            && pres != pres2) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" [%s] has the same identifier as [%s].",
                        filename,
                        extra_rule_name(pres),
                        extra_rule_name(pres2));
          ok = FALSE;
          break;
        }
      } extra_type_by_cause_iterate_end;

      if (!ok) {
        break;
      }
    } extra_type_by_cause_iterate_end;
  }

  if (ok) {
    /* base details */
    extra_type_by_cause_iterate(EC_BASE, pextra) {
      struct base_type *pbase = extra_base_get(pextra);
      const char *section;
      int bj;
      const char **slist;
      const char *gui_str;

      if (!pbase) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" extra \"%s\" has \"Base\" cause but no "
                      "corresponding [base_*] section",
                      filename, extra_rule_name(pextra));
        ok = FALSE;
        break;
      }
      section = &base_sections[base_number(pbase) * MAX_SECTION_LABEL];

      gui_str = secfile_lookup_str(file,"%s.gui_type", section);
      pbase->gui_type = base_gui_type_by_name(gui_str, fc_strcasecmp);
      if (!base_gui_type_is_valid(pbase->gui_type)) {
        ruleset_error(LOG_ERROR, "\"%s\" base \"%s\": unknown gui_type \"%s\".",
                      filename,
                      extra_rule_name(pextra),
                      gui_str);
        ok = FALSE;
        break;
      }

      pbase->border_sq  = secfile_lookup_int_default(file, -1, "%s.border_sq",
                                                     section);
      pbase->vision_main_sq   = secfile_lookup_int_default(file, -1,
                                                           "%s.vision_main_sq",
                                                           section);
      pbase->vision_invis_sq  = secfile_lookup_int_default(file, -1,
                                                           "%s.vision_invis_sq",
                                                           section);
      pbase->vision_subs_sq  = secfile_lookup_int_default(file, -1,
                                                          "%s.vision_subs_sq",
                                                          section);

      slist = secfile_lookup_str_vec(file, &nval, "%s.flags", section);
      BV_CLR_ALL(pbase->flags);
      for (bj = 0; bj < nval; bj++) {
        const char *sval = slist[bj];
        enum base_flag_id flag = base_flag_id_by_name(sval, fc_strcasecmp);

        if (!base_flag_id_is_valid(flag)) {
          ruleset_error(LOG_ERROR, "\"%s\" base \"%s\": unknown flag \"%s\".",
                        filename,
                        extra_rule_name(pextra),
                        sval);
          ok = FALSE;
          break;
        } else if ((!compat->compat_mode || compat->ver_terrain >= 20)
                   && base_flag_is_retired(flag)) {
          ruleset_error(LOG_ERROR, "\"%s\" base \"%s\": retired flag "
                                   "\"%s\". Please update the ruleset.",
                        filename,
                        extra_rule_name(pextra),
                        sval);
          ok = FALSE;
        } else {
          BV_SET(pbase->flags, flag);
        }
      }

      free(slist);

      if (!ok) {
        break;
      }

      if (territory_claiming_base(pbase)) {
        extra_type_by_cause_iterate(EC_BASE, pextra2) {
          struct base_type *pbase2;

          if (pextra == pextra2) {
            /* End of the fully initialized bases iteration. */
            break;
          }

          pbase2 = extra_base_get(pextra2);
          if (territory_claiming_base(pbase2)) {
            BV_SET(pextra->conflicts, extra_index(pextra2));
            BV_SET(pextra2->conflicts, extra_index(pextra));
          }
        } extra_type_by_cause_iterate_end;
      }
    } extra_type_by_cause_iterate_end;

    for (j = 0; ok && j < game.control.num_base_types; j++) {
      const char *section = &base_sections[j * MAX_SECTION_LABEL];
      const char *extra_name = secfile_lookup_str(file, "%s.extra", section);
      struct extra_type *pextra = extra_type_by_rule_name(extra_name);

      if (!is_extra_caused_by(pextra, EC_BASE)) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" base section [%s]: extra \"%s\" does not have "
                      "\"Base\" in its causes",
                      filename, section, extra_name);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    bv_extras compat_bridged;

    if (compat->compat_mode) {
      BV_CLR_ALL(compat_bridged);
      extra_type_by_cause_iterate(EC_ROAD, pextra) {
        struct road_type *proad = extra_road_get(pextra);
        const char *section = &road_sections[road_number(proad) * MAX_SECTION_LABEL];
        const char **slist;

        slist = secfile_lookup_str_vec(file, &nval, "%s.flags", section);

        for (j = 0; j < nval; j++) {
          if (!fc_strcasecmp("PreventsOtherRoads", slist[j])) {
            BV_SET(compat_bridged, pextra->id);
          }
        }
      } extra_type_by_cause_iterate_end;
    }
    extra_type_by_cause_iterate(EC_ROAD, pextra) {
      struct road_type *proad = extra_road_get(pextra);
      const char *section;
      const char **slist;
      const char *special;
      const char *modestr;
      struct requirement_vector *reqs;

      if (!proad) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" extra \"%s\" has \"Road\" cause but no "
                      "corresponding [road_*] section",
                      filename, extra_rule_name(pextra));
        ok = FALSE;
        break;
      }
      section = &road_sections[road_number(proad) * MAX_SECTION_LABEL];

      reqs = lookup_req_list(file, compat, section, "first_reqs", extra_rule_name(pextra));
      if (reqs == NULL) {
        ok = FALSE;
        break;
      }
      requirement_vector_copy(&proad->first_reqs, reqs);

      if (!secfile_lookup_int(file, &proad->move_cost,
                              "%s.move_cost", section)) {
        ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
        ok = FALSE;
        break;
      }

      modestr = secfile_lookup_str_default(file, "FastAlways", "%s.move_mode",
                                           section);
      proad->move_mode = road_move_mode_by_name(modestr, fc_strcasecmp);
      if (!road_move_mode_is_valid(proad->move_mode)) {
        ruleset_error(LOG_ERROR, "Illegal move_mode \"%s\" for road \"%s\"",
                      modestr, extra_rule_name(pextra));
        ok = FALSE;
        break;
      }

      output_type_iterate(o) {
        proad->tile_incr_const[o] =
          secfile_lookup_int_default(file, 0, "%s.%s_incr_const",
                                     section, get_output_identifier(o));
        proad->tile_incr[o] =
          secfile_lookup_int_default(file, 0, "%s.%s_incr",
                                     section, get_output_identifier(o));
        proad->tile_bonus[o] =
          secfile_lookup_int_default(file, 0, "%s.%s_bonus",
                                     section, get_output_identifier(o));
      } output_type_iterate_end;

      special = secfile_lookup_str_default(file, "None", "%s.compat_special", section);
      if (!fc_strcasecmp(special, "Road")) {
        if (compat_road) {
          ruleset_error(LOG_ERROR, "Multiple roads marked as compatibility \"Road\"");
          ok = FALSE;
        }
        compat_road = TRUE;
        proad->compat = ROCO_ROAD;
      } else if (!fc_strcasecmp(special, "Railroad")) {
        if (compat_rail) {
          ruleset_error(LOG_ERROR, "Multiple roads marked as compatibility \"Railroad\"");
          ok = FALSE;
        }
        compat_rail = TRUE;
        proad->compat = ROCO_RAILROAD;
      } else if (!fc_strcasecmp(special, "River")) {
        if (compat_river) {
          ruleset_error(LOG_ERROR, "Multiple roads marked as compatibility \"River\"");
          ok = FALSE;
        }
        compat_river = TRUE;
        proad->compat = ROCO_RIVER;
      } else if (!fc_strcasecmp(special, "None")) {
        proad->compat = ROCO_NONE;
      } else {
        ruleset_error(LOG_ERROR, "Illegal compatibility special \"%s\" for road %s",
                      special, extra_rule_name(pextra));
        ok = FALSE;
      }

      if (!ok) {
        break;
      }

      slist = secfile_lookup_str_vec(file, &nval, "%s.integrates", section);
      BV_CLR_ALL(proad->integrates);
      for (j = 0; j < nval; j++) {
        const char *sval = slist[j];
        struct extra_type *textra = extra_type_by_rule_name(sval);
        struct road_type *top = NULL;

        if (textra != NULL) {
          top = extra_road_get(textra);
        }

        if (top == NULL) {
          ruleset_error(LOG_ERROR, "\"%s\" road \"%s\" integrates with unknown road \"%s\".",
                        filename,
                        extra_rule_name(pextra),
                        sval);
          ok = FALSE;
          break;
        } else {
          BV_SET(proad->integrates, road_number(top));
        }
      }
      free(slist);

      if (!ok) {
        break;
      }

      slist = secfile_lookup_str_vec(file, &nval, "%s.flags", section);
      BV_CLR_ALL(proad->flags);
      for (j = 0; j < nval; j++) {
        const char *sval = slist[j];

        if (compat->compat_mode && !fc_strcasecmp("PreventsOtherRoads", sval)) {
          /* Nothing to do here */
        } else if (compat->compat_mode && !fc_strcasecmp("RequiresBridge", sval)) {
          extra_type_iterate(pbridged) {
            if (BV_ISSET(compat_bridged, pbridged->id)) {
              BV_SET(pextra->bridged_over, pbridged->id);
            }
          } extra_type_iterate_end;
        } else {
          enum road_flag_id flag = road_flag_id_by_name(sval, fc_strcasecmp);

          if (!road_flag_id_is_valid(flag)) {
            ruleset_error(LOG_ERROR, "\"%s\" road \"%s\": unknown flag \"%s\".",
                          filename,
                          extra_rule_name(pextra),
                          sval);
            ok = FALSE;
            break;
          } else {
            BV_SET(proad->flags, flag);
          }
        }
      }
      free(slist);

      if (!ok) {
        break;
      }
    } extra_type_by_cause_iterate_end;

    for (j = 0; ok && j < game.control.num_road_types; j++) {
      const char *section = &road_sections[j * MAX_SECTION_LABEL];
      const char *extra_name = secfile_lookup_str(file, "%s.extra", section);
      struct extra_type *pextra = extra_type_by_rule_name(extra_name);

      if (!is_extra_caused_by(pextra, EC_ROAD)) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" road section [%s]: extra \"%s\" does not have "
                      "\"Road\" in its causes",
                      filename, section, extra_name);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    extra_type_iterate(pextra) {
      pextra->bridged = extra_type_list_new();
      extra_type_iterate(pbridged) {
        if (BV_ISSET(pextra->bridged_over, pbridged->id)) {
          extra_type_list_append(pextra->bridged, pbridged);
        }
      } extra_type_iterate_end;
    } extra_type_iterate_end;
  }

  if (ok) {
    secfile_check_unused(file);
  }

  return ok;
}

/**********************************************************************//**
  Load names of governments so other rulesets can refer to governments with
  their name. Also load multiplier names/count from governments.ruleset.
**************************************************************************/
static bool load_government_names(struct section_file *file,
                                  struct rscompat_info *compat)
{
  int nval = 0;
  struct section_list *sec;
  const char *filename = secfile_name(file);
  bool ok = TRUE;

  compat->ver_governments = rscompat_check_capabilities(file, filename,
                                                        compat);
  if (compat->ver_governments <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  sec = secfile_sections_by_name_prefix(file, GOVERNMENT_SECTION_PREFIX);
  if (NULL == sec || 0 == (nval = section_list_size(sec))) {
    ruleset_error(LOG_ERROR, "\"%s\": No governments?!?", filename);
    ok = FALSE;
  } else if (nval > G_LAST) {
    ruleset_error(LOG_ERROR, "\"%s\": Too many governments (%d, max %d)",
                  filename, nval, G_LAST);
    ok = FALSE;
  }

  if (ok) {
    governments_alloc(nval);

    /* Government names are needed early so that get_government_by_name will
     * work. */
    governments_iterate(gov) {
      const char *sec_name =
        section_name(section_list_get(sec, government_index(gov)));

      if (!ruleset_load_names(&gov->name, NULL, file, sec_name)) {
        ok = FALSE;
        break;
      }
    } governments_iterate_end;
  }

  section_list_destroy(sec);

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, MULTIPLIER_SECTION_PREFIX);
    nval = (NULL != sec ? section_list_size(sec) : 0);

    if (nval > MAX_NUM_MULTIPLIERS) {
      ruleset_error(LOG_ERROR, "\"%s\": Too many multipliers (%d, max %d)",
                    filename, nval, MAX_NUM_MULTIPLIERS);

      ok = FALSE;
    } else {
      game.control.num_multipliers = nval;
    }

    if (ok) {
      multipliers_iterate(pmul) {
        const char *sec_name =
          section_name(section_list_get(sec, multiplier_index(pmul)));

        if (!ruleset_load_names(&pmul->name, NULL, file, sec_name)) {
          ruleset_error(LOG_ERROR, "\"%s\": Cannot load multiplier names",
                        filename);
          ok = FALSE;
          break;
        }
      } multipliers_iterate_end;
    }
  }

  section_list_destroy(sec);

  return ok;
}

/**********************************************************************//**
  This loads information from given governments.ruleset
**************************************************************************/
static bool load_ruleset_governments(struct section_file *file,
                                     struct rscompat_info *compat)
{
  struct section_list *sec;
  const char *filename = secfile_name(file);
  bool ok = TRUE;

  sec = secfile_sections_by_name_prefix(file, GOVERNMENT_SECTION_PREFIX);

  game.government_during_revolution
    = lookup_government(file, "governments.during_revolution", filename, NULL);
  if (game.government_during_revolution == NULL) {
    ok = FALSE;
  }

  if (ok) {
    game.info.government_during_revolution_id =
      government_number(game.government_during_revolution);

    /* easy ones: */
    governments_iterate(g) {
      const int i = government_index(g);
      const char *sec_name = section_name(section_list_get(sec, i));
      struct requirement_vector *reqs =
        lookup_req_list(file, compat, sec_name, "reqs", government_rule_name(g));

      if (reqs == NULL) {
        ok = FALSE;
        break;
      }

      if (NULL != secfile_entry_lookup(file, "%s.ai_better", sec_name)) {
        char entry[100];

        fc_snprintf(entry, sizeof(entry), "%s.ai_better", sec_name);
        g->ai.better = lookup_government(file, entry, filename, NULL);
        if (g->ai.better == NULL) {
          ok = FALSE;
          break;
        }
      } else {
        g->ai.better = NULL;
      }
      requirement_vector_copy(&g->reqs, reqs);
    
      sz_strlcpy(g->graphic_str,
                 secfile_lookup_str(file, "%s.graphic", sec_name));
      sz_strlcpy(g->graphic_alt,
                 secfile_lookup_str(file, "%s.graphic_alt", sec_name));

      g->helptext = lookup_strvec(file, sec_name, "helptext");
    } governments_iterate_end;
  }


  if (ok) {
    /* titles */
    governments_iterate(g) {
      const char *sec_name =
        section_name(section_list_get(sec, government_index(g)));
      const char *male, *female;

      if (!(male = secfile_lookup_str(file, "%s.ruler_male_title", sec_name))
          || !(female = secfile_lookup_str(file, "%s.ruler_female_title",
                                           sec_name))) {
        ruleset_error(LOG_ERROR, "Lack of default ruler titles for "
                      "government \"%s\" (nb %d): %s",
                      government_rule_name(g), government_number(g),
                      secfile_error());
        ok = FALSE;
        break;
      } else if (NULL == government_ruler_title_new(g, NULL, male, female)) {
        ruleset_error(LOG_ERROR, "Lack of default ruler titles for "
                      "government \"%s\" (nb %d).",
                      government_rule_name(g), government_number(g));
        ok = FALSE;
        break;
      }
    } governments_iterate_end;
  }

  section_list_destroy(sec);

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, MULTIPLIER_SECTION_PREFIX);
    multipliers_iterate(pmul) {
      int id = multiplier_index(pmul);
      const char *sec_name = section_name(section_list_get(sec, id));
      struct requirement_vector *reqs;

      if (!secfile_lookup_int(file, &pmul->start, "%s.start", sec_name)) {
        ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
        ok = FALSE;
        break;
      }
      if (!secfile_lookup_int(file, &pmul->stop, "%s.stop", sec_name)) {
        ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
        ok = FALSE;
        break;
      }
      if (pmul->stop <= pmul->start) {
        ruleset_error(LOG_ERROR, "Multiplier \"%s\" stop (%d) must be greater "
                      "than start (%d)", multiplier_rule_name(pmul),
                      pmul->stop, pmul->start);
        ok = FALSE;
        break;
      }
      if (!secfile_lookup_int(file, &pmul->step, "%s.step", sec_name)) {
        ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
        ok = FALSE;
        break;
      }
      if (((pmul->stop - pmul->start) % pmul->step) != 0) {
        ruleset_error(LOG_ERROR, "Multiplier \"%s\" step (%d) does not fit "
                      "exactly into interval start-stop (%d to %d)",
                      multiplier_rule_name(pmul), pmul->step,
                      pmul->start, pmul->stop);
        ok = FALSE;
        break;
      }
      if (!secfile_lookup_int(file, &pmul->def, "%s.default", sec_name)) {
        ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
        ok = FALSE;
        break;
      }
      if (pmul->def < pmul->start || pmul->def > pmul->stop) {
        ruleset_error(LOG_ERROR, "Multiplier \"%s\" default (%d) not within "
                      "legal range (%d to %d)", multiplier_rule_name(pmul),
                      pmul->def, pmul->start, pmul->stop);
        ok = FALSE;
        break;
      }
      if (((pmul->def - pmul->start) % pmul->step) != 0) {
        ruleset_error(LOG_ERROR, "Multiplier \"%s\" default (%d) not legal "
                      "with respect to step size %d",
                      multiplier_rule_name(pmul), pmul->def, pmul->step);
        ok = FALSE;
        break;
      }
      pmul->offset = secfile_lookup_int_default(file, 0,
                                                "%s.offset", sec_name);
      pmul->factor = secfile_lookup_int_default(file, 100,
                                                "%s.factor", sec_name);
      if (pmul->factor == 0) {
        ruleset_error(LOG_ERROR, "Multiplier \"%s\" scaling factor must "
                      "not be zero", multiplier_rule_name(pmul));
        ok = FALSE;
        break;
      }

      reqs = lookup_req_list(file, compat, sec_name, "reqs",
                             multiplier_rule_name(pmul));
      if (reqs == NULL) {
        ok = FALSE;
        break;
      }
      requirement_vector_copy(&pmul->reqs, reqs);

      pmul->helptext = lookup_strvec(file, sec_name, "helptext");   
    } multipliers_iterate_end;
    section_list_destroy(sec);
  }

  if (ok) {
    secfile_check_unused(file);
  }

  return ok;
}

/**********************************************************************//**
  Send information in packet_ruleset_control (numbers of units etc, and
  other miscellany) to specified connections.

  The client assumes that exactly one ruleset control packet is sent as
  a part of each ruleset send.  So after sending this packet we have to
  resend every other part of the rulesets (and none of them should be
  is-info in the network code!).  The client frees ruleset data when
  receiving this packet and then re-initializes as it receives the
  individual ruleset packets.  See packhand.c.
**************************************************************************/
static void send_ruleset_control(struct conn_list *dest)
{
  int desc_left = game.control.desc_length;
  int idx = 0;

  lsend_packet_ruleset_control(dest, &(game.control));

  if (game.ruleset_summary != NULL) {
    struct packet_ruleset_summary summary;

    sz_strlcpy(summary.text, game.ruleset_summary);

    lsend_packet_ruleset_summary(dest, &summary);
  }

  while (desc_left > 0) {
    struct packet_ruleset_description_part part;
    int this_len = desc_left;

    if (this_len > MAX_LEN_CONTENT - 21) {
      this_len = MAX_LEN_CONTENT - 1;
    }

    part.text[this_len] = '\0';

    strncpy(part.text, &game.ruleset_description[idx], this_len);
    idx += this_len;
    desc_left -= this_len;

    lsend_packet_ruleset_description_part(dest, &part);
  }
}

/**********************************************************************//**
  Check for duplicate leader names in nation.
  If no duplicates return NULL; if yes return pointer to name which is
  repeated.
**************************************************************************/
static const char *check_leader_names(struct nation_type *pnation)
{
  nation_leader_list_iterate(nation_leaders(pnation), pleader) {
    const char *name = nation_leader_name(pleader);

    nation_leader_list_iterate(nation_leaders(pnation), prev_leader) {
      if (prev_leader == pleader) {
        break;
      } else if (0 == fc_strcasecmp(name, nation_leader_name(prev_leader))) {
        return name;
      }
    } nation_leader_list_iterate_end;
  } nation_leader_list_iterate_end;
  return NULL;
}

/**********************************************************************//**
  Load names of nations so other rulesets can refer to nations with
  their name.
**************************************************************************/
static bool load_nation_names(struct section_file *file,
                              struct rscompat_info *compat)
{
  struct section_list *sec;
  int j;
  bool ok = TRUE;
  const char *filename = secfile_name(file);

  compat->ver_nations = rscompat_check_capabilities(file, filename,
                                                    compat);
  if (compat->ver_nations <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  sec = secfile_sections_by_name_prefix(file, NATION_SECTION_PREFIX);
  if (NULL == sec) {
    ruleset_error(LOG_ERROR, "No available nations in this ruleset!");
    ok = FALSE;
  } else if (section_list_size(sec) > MAX_NUM_NATIONS) {
    ruleset_error(LOG_ERROR, "Too many nations (max %d, we have %d)!",
                  MAX_NUM_NATIONS, section_list_size(sec));
    ok = FALSE;
  } else {
    game.control.nation_count = section_list_size(sec);
    nations_alloc(game.control.nation_count);

    nations_iterate(pl) {
      const int i = nation_index(pl);
      const char *sec_name = section_name(section_list_get(sec, i));
      const char *domain = secfile_lookup_str_default(file, NULL,
                                                      "%s.translation_domain", sec_name);
      const char *noun_plural = secfile_lookup_str(file,
                                                   "%s.plural", sec_name);


      if (domain == NULL) {
        domain = "freeciv-nations";
      }

      if (!strcmp("freeciv", domain)) {
        pl->translation_domain = NULL;
      } else if (!strcmp("freeciv-nations", domain)) {
        pl->translation_domain = fc_malloc(strlen(domain) + 1);
        strcpy(pl->translation_domain, domain);
      } else {
        ruleset_error(LOG_ERROR, "Unsupported translation domain \"%s\" for %s",
                      domain, sec_name);
        ok = FALSE;
        break;
      }

      if (!ruleset_load_names(&pl->adjective, domain, file, sec_name)) {
        ok = FALSE;
        break;
      }
      name_set(&pl->noun_plural, domain, noun_plural);

      /* Check if nation name is already defined. */
      for (j = 0; j < i && ok; j++) {
        struct nation_type *n2 = nation_by_number(j);

        /* Compare strings after stripping off qualifiers -- we don't want
         * two nations to end up with identical adjectives displayed to users.
         * (This check only catches English, not localisations, of course.) */
        if (0 == strcmp(Qn_(untranslated_name(&n2->adjective)),
                        Qn_(untranslated_name(&pl->adjective)))) {
          ruleset_error(LOG_ERROR,
                        "Two nations defined with the same adjective \"%s\": "
                        "in section \'%s\' and section \'%s\'",
                        Qn_(untranslated_name(&pl->adjective)),
                        section_name(section_list_get(sec, j)), sec_name);
          ok = FALSE;
        } else if (!strcmp(rule_name_get(&n2->adjective),
                           rule_name_get(&pl->adjective))) {
          /* We cannot have the same rule name, as the game needs them to be
           * distinct. */
          ruleset_error(LOG_ERROR,
                        "Two nations defined with the same rule_name \"%s\": "
                        "in section \'%s\' and section \'%s\'",
                        rule_name_get(&pl->adjective),
                        section_name(section_list_get(sec, j)), sec_name);
          ok = FALSE;
        } else if (0 == strcmp(Qn_(untranslated_name(&n2->noun_plural)),
                               Qn_(untranslated_name(&pl->noun_plural)))) {
          /* We don't want identical English plural names either. */
          ruleset_error(LOG_ERROR,
                        "Two nations defined with the same plural name \"%s\": "
                        "in section \'%s\' and section \'%s\'",
                        Qn_(untranslated_name(&pl->noun_plural)),
                        section_name(section_list_get(sec, j)), sec_name);
          ok = FALSE;
        }
      }
      if (!ok) {
        break;
      }
    } nations_iterate_end;
  }

  section_list_destroy(sec);

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, NATION_GROUP_SECTION_PREFIX);
    if (sec) {
      section_list_iterate(sec, psection) {
        struct nation_group *pgroup;
        const char *name;

        name = secfile_lookup_str(file, "%s.name", section_name(psection));
        if (NULL == name) {
          ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
          ok = FALSE;
          break;
        }
        pgroup = nation_group_new(name);
        if (pgroup == NULL) {
          ok = FALSE;
          break;
        }
      } section_list_iterate_end;
      section_list_destroy(sec);
      sec = NULL;
    }
  }

  return ok;
}

/**********************************************************************//**
  Check if a string is in a vector (case-insensitively).
**************************************************************************/
static bool is_on_allowed_list(const char *name, const char **list, size_t len)
{
  int i;  

  for (i = 0; i < len; i++) {
    if (!fc_strcasecmp(name, list[i])) {
      return TRUE;
    }
  }
  return FALSE;
}

/**********************************************************************//**
  This function loads a city name list from a section file.  The file and
  two section names (which will be concatenated) are passed in.
**************************************************************************/
static bool load_city_name_list(struct section_file *file,
                                struct nation_type *pnation,
                                const char *secfile_str1,
                                const char *secfile_str2,
                                const char **allowed_terrains,
                                size_t atcount)
{
  size_t dim, j;
  bool ok = TRUE;
  const char **cities = secfile_lookup_str_vec(file, &dim, "%s.%s",
                                               secfile_str1, secfile_str2);

  /* Each string will be of the form "<cityname> (<label>, <label>, ...)".
   * The cityname is just the name for this city, while each "label" matches
   * a terrain type for the city (or "river"), with a preceeding ! to negate
   * it. The parentheses are optional (but necessary to have the settings,
   * of course). Our job is now to parse it. */
  for (j = 0; j < dim; j++) {
    size_t len = strlen(cities[j]);
    char city_name[len + 1], *p, *next, *end;
    struct nation_city *pncity;

    sz_strlcpy(city_name, cities[j]);

    /* Now we wish to determine values for all of the city labels. A value
     * of NCP_NONE means no preference (which is necessary so that the use
     * of this is optional); NCP_DISLIKE means the label is negated and
     * NCP_LIKE means it's labelled. Mostly the parsing just involves
     * a lot of ugly string handling... */
    if ((p = strchr(city_name, '('))) {
      *p++ = '\0';

      if (!(end = strchr(p, ')'))) {
        ruleset_error(LOG_ERROR, "\"%s\" [%s] %s: city name \"%s\" "
                      "unmatched parenthesis.", secfile_name(file),
                      secfile_str1, secfile_str2, cities[j]);
        ok = FALSE;
      } else {
        for (*end++ = '\0'; '\0' != *end; end++) {
          if (!fc_isspace(*end)) {
            ruleset_error(LOG_ERROR, "\"%s\" [%s] %s: city name \"%s\" "
                          "contains characters after last parenthesis.",
                          secfile_name(file), secfile_str1, secfile_str2,
                          cities[j]);
            ok = FALSE;
            break;
          }
        }
      }
    }

    /* Build the nation_city. */
    remove_leading_trailing_spaces(city_name);
    if (check_cityname(city_name)) {
      /* The ruleset contains a name that is too long. This shouldn't
       * happen - if it does, the author should get immediate feedback. */
      ruleset_error(LOG_ERROR, "\"%s\" [%s] %s: city name \"%s\" "
                    "is too long.", secfile_name(file),
                    secfile_str1, secfile_str2, city_name);
      ok = FALSE;
      city_name[MAX_LEN_CITYNAME - 1] = '\0';
    }
    pncity = nation_city_new(pnation, city_name);

    if (NULL != p) {
      /* Handle the labels one at a time. */
      do {
        enum nation_city_preference prefer;

        if ((next = strchr(p, ','))) {
          *next = '\0';
        }
        remove_leading_trailing_spaces(p);

        /* The ! is used to mark a negative, which is recorded with
         * NCP_DISLIKE. Otherwise we use a NCP_LIKE.
         */
        if (*p == '!') {
          p++;
          prefer = NCP_DISLIKE;
        } else {
          prefer = NCP_LIKE;
        }

        if (0 == fc_strcasecmp(p, "river")) {
          if (game.server.ruledit.allowed_terrains != NULL
              && !is_on_allowed_list(p,
                                     game.server.ruledit.allowed_terrains, atcount)) {
            ruleset_error(LOG_ERROR, "\"%s\" [%s] %s: city \"%s\" "
                          "has terrain hint \"%s\" not in allowed_terrains.",
                          secfile_name(file), secfile_str1, secfile_str2,
                          city_name, p);
            ok = FALSE;
          } else {
            nation_city_set_river_preference(pncity, prefer);
          }
        } else {
          const struct terrain *pterrain = terrain_by_rule_name(p);

          if (NULL == pterrain) {
            /* Try with removing frequent trailing 's'. */
            size_t l = strlen(p);

            if (0 < l && 's' == fc_tolower(p[l - 1])) {
              p[l - 1] = '\0';
            }
            pterrain = terrain_by_rule_name(p);
          }

          /* Nationset may have been devised with a specific set of terrains
           * in mind which don't quite match this ruleset, in which case we
           * (a) quietly ignore any hints mentioned that don't happen to be in
           * the current ruleset, (b) enforce that terrains mentioned by nations
           * must be on the list */
          if (pterrain != NULL && game.server.ruledit.allowed_terrains != NULL) {
            if (!is_on_allowed_list(p,
                                    game.server.ruledit.allowed_terrains, atcount)) {
              /* Terrain exists, but not intended for these nations */
              ruleset_error(LOG_ERROR, "\"%s\" [%s] %s: city \"%s\" "
                            "has terrain hint \"%s\" not in allowed_terrains.",
                            secfile_name(file), secfile_str1, secfile_str2,
                            city_name, p);
              ok = FALSE;
              break;
            }
          } else if (!pterrain) {
            /* Terrain doesn't exist; only complain if it's not on any list */
            if (game.server.ruledit.allowed_terrains == NULL
                || !is_on_allowed_list(p,
                                       game.server.ruledit.allowed_terrains, atcount)) {
              ruleset_error(LOG_ERROR, "\"%s\" [%s] %s: city \"%s\" "
                            "has unknown terrain hint \"%s\".",
                            secfile_name(file), secfile_str1, secfile_str2,
                            city_name, p);
              ok = FALSE;
              break;
            }
          }
          if (NULL != pterrain) {
            nation_city_set_terrain_preference(pncity, pterrain, prefer);
          }
        }

        p = next ? next + 1 : NULL;
      } while (NULL != p && '\0' != *p);
    }
  }

  if (NULL != cities) {
    free(cities);
  }

  return ok;
}

/**********************************************************************//**
  Load nations.ruleset file
**************************************************************************/
static bool load_ruleset_nations(struct section_file *file,
                                 struct rscompat_info *compat)
{
  struct government *gov;
  int j;
  size_t dim;
  char temp_name[MAX_LEN_NAME];
  const char **vec;
  const char *name, *bad_leader;
  const char *sval;
  int default_set;
  const char *filename = secfile_name(file);
  struct section_list *sec;
  enum trait tr;
  bool ok = TRUE;

  name = secfile_lookup_str_default(file, NULL, "ruledit.nationlist");
  if (name != NULL) {
    game.server.ruledit.nationlist = fc_strdup(name);
  }
  vec  = secfile_lookup_str_vec(file, &game.server.ruledit.embedded_nations_count,
                                "ruledit.embedded_nations");

  if (vec != NULL) {
    /* Copy to persistent vector */
    game.server.ruledit.embedded_nations
      = fc_malloc(game.server.ruledit.embedded_nations_count * sizeof(char *));

    for (j = 0; j < game.server.ruledit.embedded_nations_count; j++) {
      game.server.ruledit.embedded_nations[j] = fc_strdup(vec[j]);
    }

    free(vec);
  }

  game.default_government = NULL;

  ruleset_load_traits(game.server.default_traits, file, "default_traits", "");
  for (tr = trait_begin(); tr != trait_end(); tr = trait_next(tr)) {
    if (game.server.default_traits[tr].min < 0) {
      game.server.default_traits[tr].min = TRAIT_DEFAULT_VALUE;
    }
    if (game.server.default_traits[tr].max < 0) {
      game.server.default_traits[tr].max = TRAIT_DEFAULT_VALUE;
    }
    if (game.server.default_traits[tr].fixed < 0) {
      int diff = game.server.default_traits[tr].max - game.server.default_traits[tr].min;

      /* TODO: Should sometimes round the a / 2 = x.5 results up */
      game.server.default_traits[tr].fixed = diff / 2 + game.server.default_traits[tr].min;
    }
    if (game.server.default_traits[tr].max < game.server.default_traits[tr].min) {
      ruleset_error(LOG_ERROR, "Default values for trait %s not sane.",
                    trait_name(tr));
      ok = FALSE;
      break;
    }
  }

  if (ok) {
    vec = secfile_lookup_str_vec(file, &game.server.ruledit.ag_count,
                                 "compatibility.allowed_govs");
    if (vec != NULL) {
      /* Copy to persistent vector */
      game.server.ruledit.nc_agovs
        = fc_malloc(game.server.ruledit.ag_count * sizeof(char *));
      game.server.ruledit.allowed_govs =
        (const char **)game.server.ruledit.nc_agovs;

      for (j = 0; j < game.server.ruledit.ag_count; j++) {
        game.server.ruledit.allowed_govs[j] = fc_strdup(vec[j]);
      }

      free(vec);
    }

    vec = secfile_lookup_str_vec(file, &game.server.ruledit.at_count,
                                 "compatibility.allowed_terrains");
    if (vec != NULL) {
      /* Copy to persistent vector */
      game.server.ruledit.nc_aterrs
        = fc_malloc(game.server.ruledit.at_count * sizeof(char *));
      game.server.ruledit.allowed_terrains =
        (const char **)game.server.ruledit.nc_aterrs;

      for (j = 0; j < game.server.ruledit.at_count; j++) {
        game.server.ruledit.allowed_terrains[j] = fc_strdup(vec[j]);
      }

      free(vec);
    }

    vec = secfile_lookup_str_vec(file, &game.server.ruledit.as_count,
                                 "compatibility.allowed_styles");
    if (vec != NULL) {
      /* Copy to persistent vector */
      game.server.ruledit.nc_astyles
        = fc_malloc(game.server.ruledit.as_count * sizeof(char *));
      game.server.ruledit.allowed_styles =
        (const char **)game.server.ruledit.nc_astyles;

      for (j = 0; j < game.server.ruledit.as_count; j++) {
        game.server.ruledit.allowed_styles[j] = fc_strdup(vec[j]);
      }

      free(vec);
    }

    sval = secfile_lookup_str_default(file, NULL,
                                      "compatibility.default_government");
    /* We deliberately don't check this against allowed_govs. It's only
     * specified once so not vulnerable to typos, and may usefully be set in
     * a specific ruleset to a gov not explicitly known by the nation set. */
    if (sval != NULL) {
      game.default_government = government_by_rule_name(sval);
      if (game.default_government == NULL) {
        ruleset_error(LOG_ERROR,
                      "Tried to set unknown government type \"%s\" as default_government!",
                      sval);
        ok = FALSE;
      } else {
        game.info.default_government_id
          = government_number(game.default_government);
      }
    }
  }

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, NATION_SET_SECTION_PREFIX);
    if (sec) {
      section_list_iterate(sec, psection) {
        const char *set_name, *set_rule_name, *set_description;

        set_name = secfile_lookup_str(file, "%s.name", section_name(psection));
        set_rule_name =
          secfile_lookup_str(file, "%s.rule_name", section_name(psection));
        set_description = secfile_lookup_str_default(file, "", "%s.description",
                                                   section_name(psection));
        if (NULL == set_name || NULL == set_rule_name) {
          ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
          ok = FALSE;
          break;
        }
        if (nation_set_new(set_name, set_rule_name, set_description) == NULL) {
          ok = FALSE;
          break;
        }
      } section_list_iterate_end;
      section_list_destroy(sec);
      sec = NULL;
    } else {
      ruleset_error(LOG_ERROR,
                    "At least one nation set [" NATION_SET_SECTION_PREFIX "_*] "
                    "must be defined.");
      ok = FALSE;
    }
  }

  if (ok) {
    /* Default set that every nation is a member of. */
    sval = secfile_lookup_str_default(file, NULL,
                                      "compatibility.default_nationset");
    if (sval != NULL) {
      const struct nation_set *pset = nation_set_by_rule_name(sval);
      if (pset != NULL) {
        default_set = nation_set_number(pset);
      } else {
        ruleset_error(LOG_ERROR,
                      "Unknown default_nationset \"%s\".", sval);
        ok = FALSE;
      }
    } else if (nation_set_count() == 1) {
      /* If there's only one set defined, every nation is implicitly a
       * member of that set. */
      default_set = 0;
    } else {
      /* No default nation set; every nation must explicitly specify at
       * least one set to be a member of. */
      default_set = -1;
    }
  }

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, NATION_GROUP_SECTION_PREFIX);
    if (sec) {
      section_list_iterate(sec, psection) {
        struct nation_group *pgroup;
        bool hidden;

        name = secfile_lookup_str(file, "%s.name", section_name(psection));
        pgroup = nation_group_by_rule_name(name);
        if (pgroup == NULL) {
          ok = FALSE;
          break;
        }

        hidden = secfile_lookup_bool_default(file, FALSE, "%s.hidden",
                                             section_name(psection));
        nation_group_set_hidden(pgroup, hidden);

        if (!secfile_lookup_int(file, &j, "%s.match", section_name(psection))) {
          ruleset_error(LOG_ERROR, "Error: %s", secfile_error());
          ok = FALSE;
          break;
        }
        nation_group_set_match(pgroup, j);
      } section_list_iterate_end;
      section_list_destroy(sec);
      sec = NULL;
    }
  }

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, NATION_SECTION_PREFIX);
    nations_iterate(pnation) {
      struct nation_type *pconflict;
      const int i = nation_index(pnation);
      char tmp[200] = "\0";
      const char *barb_type;
      const char *sec_name = section_name(section_list_get(sec, i));
      const char *legend;

      /* Nation sets and groups. */
      if (default_set >= 0) {
        nation_set_list_append(pnation->sets,
                               nation_set_by_number(default_set));
      }
      vec = secfile_lookup_str_vec(file, &dim, "%s.groups", sec_name);
      for (j = 0; j < dim; j++) {
        struct nation_set *pset = nation_set_by_rule_name(vec[j]);
        struct nation_group *pgroup = nation_group_by_rule_name(vec[j]);

        fc_assert(pset == NULL || pgroup == NULL);

        if (NULL != pset) {
          nation_set_list_append(pnation->sets, pset);
        } else if (NULL != pgroup) {
          nation_group_list_append(pnation->groups, pgroup);
        } else {
          /* For nation authors, this would probably be considered an error.
           * But it can happen normally. The civ1 compatibility ruleset only
           * uses the nations that were in civ1, so not all of the links will
           * exist. */
          log_verbose("Nation %s: Unknown set/group \"%s\".",
                      nation_rule_name(pnation), vec[j]);
        }
      }
      if (NULL != vec) {
        free(vec);
      }
      if (nation_set_list_size(pnation->sets) < 1) {
        ruleset_error(LOG_ERROR,
                      "Nation %s is not a member of any nation set",
                      nation_rule_name(pnation));
        ok = FALSE;
        break;
      }

      /* Nation conflicts. */
      vec = secfile_lookup_str_vec(file, &dim, "%s.conflicts_with", sec_name);
      for (j = 0; j < dim; j++) {
        pconflict = nation_by_rule_name(vec[j]);

        if (pnation == pconflict) {
          ruleset_error(LOG_ERROR, "Nation %s conflicts with itself",
                        nation_rule_name(pnation));
          ok = FALSE;
          break;
        } else if (NULL != pconflict) {
          nation_list_append(pnation->server.conflicts_with, pconflict);
        } else {
          /* For nation authors, this would probably be considered an error.
           * But it can happen normally. The civ1 compatibility ruleset only
           * uses the nations that were in civ1, so not all of the links will
           * exist. */
          log_verbose("Nation %s: conflicts_with nation \"%s\" is unknown.",
                      nation_rule_name(pnation), vec[j]);
        }
      }
      if (NULL != vec) {
        free(vec);
      }
      if (!ok) {
        break;
      }

      /* Nation leaders. */
      for (j = 0; j < MAX_NUM_LEADERS; j++) {
        const char *sex;
        bool is_male = FALSE;

        name = secfile_lookup_str(file, "%s.leaders%d.name", sec_name, j);
        if (NULL == name) {
          /* No more to read. */
          break;
        }

        if (check_name(name)) {
          /* The ruleset contains a name that is too long. This shouldn't
           * happen - if it does, the author should get immediate feedback */
          sz_strlcpy(temp_name, name);
          ruleset_error(LOG_ERROR, "Nation %s: leader name \"%s\" "
                        "is too long.",
                        nation_rule_name(pnation), name);
          ok = FALSE;
          break;
        }

        sex = secfile_lookup_str(file, "%s.leaders%d.sex", sec_name, j);
        if (NULL == sex) {
          ruleset_error(LOG_ERROR, "Nation %s: leader \"%s\": %s.",
                        nation_rule_name(pnation), name, secfile_error());
          ok = FALSE;
          break;
        } else if (0 == fc_strcasecmp("Male", sex)) {
          is_male = TRUE;
        } else if (0 != fc_strcasecmp("Female", sex)) {
          ruleset_error(LOG_ERROR, "Nation %s: leader \"%s\" has unsupported "
                        "sex variant \"%s\".",
                        nation_rule_name(pnation), name, sex);
          ok = FALSE;
          break;
        }
        (void) nation_leader_new(pnation, name, is_male);
      }
      if (!ok) {
        break;
      }

      /* Check the number of leaders. */
      if (MAX_NUM_LEADERS == j) {
        /* Too much leaders, get the real number defined in the ruleset. */
        while (NULL != secfile_entry_lookup(file, "%s.leaders%d.name",
                                            sec_name, j)) {
          j++;
        }
        ruleset_error(LOG_ERROR, "Nation %s: Too many leaders; max is %d",
                      nation_rule_name(pnation), MAX_NUM_LEADERS);
        ok = FALSE;
        break;
      } else if (0 == j) {
        ruleset_error(LOG_ERROR,
                      "Nation %s: no leaders; at least one is required.",
                      nation_rule_name(pnation));
        ok = FALSE;
        break;
      }

      /* Check if leader name is not already defined in this nation. */
      if ((bad_leader = check_leader_names(pnation))) {
        ruleset_error(LOG_ERROR,
                      "Nation %s: leader \"%s\" defined more than once.",
                      nation_rule_name(pnation), bad_leader);
        ok = FALSE;
        break;
      }

      /* Nation player color preference, if any */
      fc_assert_ret_val(pnation->server.rgb == NULL, FALSE);
      (void) rgbcolor_load(file, &pnation->server.rgb, "%s.color", sec_name);

      /* Load nation traits */
      ruleset_load_traits(pnation->server.traits, file, sec_name, "trait_");
      for (tr = trait_begin(); tr != trait_end(); tr = trait_next(tr)) {
        bool server_traits_used = TRUE;

        if (pnation->server.traits[tr].min < 0) {
          pnation->server.traits[tr].min = game.server.default_traits[tr].min;
        } else {
          server_traits_used = FALSE;
        }
        if (pnation->server.traits[tr].max < 0) {
          pnation->server.traits[tr].max = game.server.default_traits[tr].max;
        } else {
          server_traits_used = FALSE;
        }
        if (pnation->server.traits[tr].fixed < 0) {
          if (server_traits_used) {
            pnation->server.traits[tr].fixed = game.server.default_traits[tr].fixed;
          } else {
            int diff = pnation->server.traits[tr].max - pnation->server.traits[tr].min;

            /* TODO: Should sometimes round the a / 2 = x.5 results up */
            pnation->server.traits[tr].fixed = diff / 2 + pnation->server.traits[tr].min;
          }
        }
        if (pnation->server.traits[tr].max < pnation->server.traits[tr].min) {
          ruleset_error(LOG_ERROR, "%s values for trait %s not sane.",
                        nation_rule_name(pnation), trait_name(tr));
          ok = FALSE;
          break;
        }
      }

      if (!ok) {
        break;
      }

      pnation->is_playable =
        secfile_lookup_bool_default(file, TRUE, "%s.is_playable", sec_name);

      /* Check barbarian type. Default is "None" meaning not a barbarian */
      barb_type = secfile_lookup_str_default(file, "None",
                                             "%s.barbarian_type", sec_name);
      pnation->barb_type = barbarian_type_by_name(barb_type, fc_strcasecmp);
      if (!barbarian_type_is_valid(pnation->barb_type)) {
        ruleset_error(LOG_ERROR,
                      "Nation %s, barbarian_type is invalid (\"%s\")",
                      nation_rule_name(pnation), barb_type);
        ok = FALSE;
        break;
      }

      if (pnation->barb_type != NOT_A_BARBARIAN
          && pnation->is_playable) {
        /* We can't allow players to use barbarian nations, barbarians
         * may run out of nations */
        ruleset_error(LOG_ERROR,
                      "Nation %s marked both barbarian and playable.",
                      nation_rule_name(pnation));
        ok = FALSE;
        break;
      }

      /* Flags */
      sz_strlcpy(pnation->flag_graphic_str,
                 secfile_lookup_str_default(file, "-", "%s.flag", sec_name));
      sz_strlcpy(pnation->flag_graphic_alt,
                 secfile_lookup_str_default(file, "-",
                                            "%s.flag_alt", sec_name));

      /* Ruler titles */
      for (j = 0;; j++) {
        const char *male, *female;

        name = secfile_lookup_str_default(file, NULL,
                                          "%s.ruler_titles%d.government",
                                          sec_name, j);
        if (NULL == name) {
          /* End of the list of ruler titles. */
          break;
        }

        /* NB: even if the government doesn't exist, we load the entries for
         * the ruler titles to avoid warnings about unused entries. */
        male = secfile_lookup_str(file, "%s.ruler_titles%d.male_title",
                                  sec_name, j);
        female = secfile_lookup_str(file, "%s.ruler_titles%d.female_title",
                                    sec_name, j);
        gov = government_by_rule_name(name);

        /* Nationset may have been devised with a specific set of govs in
         * mind which don't quite match this ruleset, in which case we
         * (a) quietly ignore any govs mentioned that don't happen to be in
         * the current ruleset, (b) enforce that govs mentioned by nations
         * must be on the list */
        if (gov != NULL && game.server.ruledit.allowed_govs != NULL) {
          if (!is_on_allowed_list(name,
                                  game.server.ruledit.allowed_govs,
                                  game.server.ruledit.ag_count)) {
            /* Gov exists, but not intended for these nations */
            gov = NULL;
            ruleset_error(LOG_ERROR,
                          "Nation %s: government \"%s\" not in allowed_govs.",
                          nation_rule_name(pnation), name);
            ok = FALSE;
            break;
          }
        } else if (!gov) {
          /* Gov doesn't exist; only complain if it's not on any list */
          if (game.server.ruledit.allowed_govs == NULL
              || !is_on_allowed_list(name,
                                     game.server.ruledit.allowed_govs,
                                     game.server.ruledit.ag_count)) {
            ruleset_error(LOG_ERROR, "Nation %s: government \"%s\" not found.",
                          nation_rule_name(pnation), name);
            ok = FALSE;
            break;
          }
        }
        if (NULL != male && NULL != female) {
          if (gov) {
            (void) government_ruler_title_new(gov, pnation, male, female);
          }
        } else {
          ruleset_error(LOG_ERROR, "%s", secfile_error());
          ok = FALSE;
          break;
        }
      }
      if (!ok) {
        break;
      }

      /* City styles */
      name = secfile_lookup_str(file, "%s.style", sec_name);
      if (!name) {
        ruleset_error(LOG_ERROR, "%s", secfile_error());
        ok = FALSE;
        break;
      }
      pnation->style = style_by_rule_name(name);
      if (pnation->style == NULL) {
        if (game.server.ruledit.allowed_styles == NULL
            || !is_on_allowed_list(name,
                                   game.server.ruledit.allowed_styles,
                                   game.server.ruledit.as_count)) {
          ruleset_error(LOG_ERROR, "Nation %s: Illegal style \"%s\"",
                        nation_rule_name(pnation), name);
          ok = FALSE;
          break;
        } else {
          log_verbose("Nation %s: style \"%s\" not supported in this "
                      "ruleset; using default.",
                      nation_rule_name(pnation), name);
          pnation->style = style_by_number(0);
        }
      }

      /* Civilwar nations */
      vec = secfile_lookup_str_vec(file, &dim,
                                   "%s.civilwar_nations", sec_name);
      for (j = 0; j < dim; j++) {
        pconflict = nation_by_rule_name(vec[j]);

        /* No test for duplicate nations is performed.  If there is a duplicate
         * entry it will just cause that nation to have an increased
         * probability of being chosen. */
        if (pconflict == pnation) {
          ruleset_error(LOG_ERROR, "Nation %s is its own civil war nation",
                        nation_rule_name(pnation));
          ok = FALSE;
          break;
        } else if (NULL != pconflict) {
          nation_list_append(pnation->server.civilwar_nations, pconflict);
          nation_list_append(pconflict->server.parent_nations, pnation);
        } else {
          /* For nation authors, this would probably be considered an error.
           * But it can happen normally. The civ1 compatability ruleset only
           * uses the nations that were in civ1, so not all of the links will
           * exist. */
          log_verbose("Nation %s: civil war nation \"%s\" is unknown.",
                      nation_rule_name(pnation), vec[j]);
        }
      }
      if (NULL != vec) {
        free(vec);
      }
      if (!ok) {
        break;
      }

      /* Load nation specific initial items */
      if (!lookup_tech_list(file, sec_name, "init_techs",
                            pnation->init_techs, filename)) {
        ok = FALSE;
        break;
      }
      if (!lookup_building_list(file, sec_name, "init_buildings",
                                pnation->init_buildings, filename)) {
        ok = FALSE;
        break;
      }
      if (!lookup_unit_list(file, sec_name, "init_units",
                            pnation->init_units, filename)) {
        ok = FALSE;
        break;
      }
      fc_strlcat(tmp, sec_name, 200);
      fc_strlcat(tmp, ".init_government", 200);
      if (secfile_entry_by_path(file, tmp)) {
        pnation->init_government = lookup_government(file, tmp, filename,
                                                     NULL);
        /* If specified, init_government has to be in this specific ruleset,
         * not just allowed_govs */
        if (pnation->init_government == NULL) {
          ok = FALSE;
          break;
        }
        /* ...but if a list of govs has been specified, enforce that this
         * nation's init_government is on the list. */
        if (game.server.ruledit.allowed_govs != NULL
            && !is_on_allowed_list(government_rule_name(pnation->init_government),
                                   game.server.ruledit.allowed_govs,
                                   game.server.ruledit.ag_count)) {
          ruleset_error(LOG_ERROR,
                        "Nation %s: init_government \"%s\" not allowed.",
                        nation_rule_name(pnation),
                        government_rule_name(pnation->init_government));
          ok = FALSE;
          break;
        }
      }

      /* Read default city names. */
      if (!load_city_name_list(file, pnation, sec_name, "cities",
                               game.server.ruledit.allowed_terrains,
                               game.server.ruledit.at_count)) {
        ok = FALSE;
        break;
      }

      legend = secfile_lookup_str_default(file, "", "%s.legend", sec_name);
      pnation->legend = fc_strdup(legend);
      if (check_strlen(pnation->legend, MAX_LEN_MSG, NULL)) {
        ruleset_error(LOG_ERROR,
                      "Nation %s: legend \"%s\" is too long.",
                      nation_rule_name(pnation),
                      pnation->legend);
        ok = FALSE;
        break;
      }

      pnation->player = NULL;
    } nations_iterate_end;
    section_list_destroy(sec);
    sec = NULL;
  }

  /* Clean up on aborted load */
  if (sec) {
    fc_assert(!ok);
    section_list_destroy(sec);
  }

  if (ok) {
    secfile_check_unused(file);
  }

  if (ok) {
    /* Update cached number of playable nations in the current set */
    count_playable_nations();

    /* Sanity checks on all sets */
    nation_sets_iterate(pset) {
      int num_playable = 0, barb_land_count = 0, barb_sea_count = 0, barb_both_count = 0;

      nations_iterate(pnation) {
        if (nation_is_in_set(pnation, pset)) {
          switch (nation_barbarian_type(pnation)) {
          case NOT_A_BARBARIAN:
            if (is_nation_playable(pnation)) {
              num_playable++;
            }
            break;
          case LAND_BARBARIAN:
            barb_land_count++;
            break;
          case SEA_BARBARIAN:
            barb_sea_count++;
            break;
          case ANIMAL_BARBARIAN:
            /* Animals are optional */
            break;
          case LAND_AND_SEA_BARBARIAN:
            barb_both_count++;
            break;
          default:
            fc_assert_ret_val(FALSE, FALSE);
          }
        }
      } nations_iterate_end;
      if (num_playable < 1) {
        ruleset_error(LOG_ERROR,
                      "Nation set \"%s\" has no playable nations. "
                      "At least one required!", nation_set_rule_name(pset));
        ok = FALSE;
        break;
      }
      if (barb_land_count == 0 && barb_both_count == 0) {
        ruleset_error(LOG_ERROR,
                      "No land barbarian nation defined in set \"%s\". "
                      "At least one required!", nation_set_rule_name(pset));
        ok = FALSE;
        break;
      }
      if (barb_sea_count == 0 && barb_both_count == 0) {
        ruleset_error(LOG_ERROR,
                      "No sea barbarian nation defined in set \"%s\". "
                      "At least one required!", nation_set_rule_name(pset));
        ok = FALSE;
        break;
      }
    } nation_sets_iterate_end;
  }

  return ok;
}

/**********************************************************************//**
  Load names of nation styles so other rulesets can refer to styles with
  their name.
**************************************************************************/
static bool load_style_names(struct section_file *file,
                             struct rscompat_info *compat)
{
  bool ok = TRUE;
  struct section_list *sec;
  const char *filename = secfile_name(file);

  compat->ver_styles = rscompat_check_capabilities(file, filename, compat);
  if (compat->ver_styles <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  sec = secfile_sections_by_name_prefix(file, STYLE_SECTION_PREFIX);
  if (NULL == sec) {
    ruleset_error(LOG_ERROR, "No available nation styles in this ruleset!");
    ok = FALSE;
  } else {
    game.control.num_styles = section_list_size(sec);

    styles_alloc(game.control.num_styles);

    styles_iterate(ps) {
      const int i = style_index(ps);
      const char *sec_name = section_name(section_list_get(sec, i));

      ruleset_load_names(&ps->name, NULL, file, sec_name);
    } styles_iterate_end;
  }

  section_list_destroy(sec);

  if (ok) {
    /* The citystyle sections: */
    int i = 0;

    sec = secfile_sections_by_name_prefix(file, CITYSTYLE_SECTION_PREFIX);
    if (NULL != sec) {
      city_styles_alloc(section_list_size(sec));
      section_list_iterate(sec, style) {
        if (!ruleset_load_names(&city_styles[i].name, NULL, file, section_name(style))) {
          ok = FALSE;
          break;
        }
        i++;
      } section_list_iterate_end;

      section_list_destroy(sec);
    } else {
      city_styles_alloc(0);
    }
  }

  return ok;
}

/**********************************************************************//**
  Load styles.ruleset file
**************************************************************************/
static bool load_ruleset_styles(struct section_file *file,
                                struct rscompat_info *compat)
{
  struct section_list *sec;
  int i;
  bool ok = TRUE;

  /* City Styles ... */

  sec = secfile_sections_by_name_prefix(file, CITYSTYLE_SECTION_PREFIX);

  /* Get rest: */
  for (i = 0; i < game.control.styles_count; i++) {
    struct requirement_vector *reqs;
    const char *sec_name = section_name(section_list_get(sec, i));

    sz_strlcpy(city_styles[i].graphic, 
               secfile_lookup_str(file, "%s.graphic", sec_name));
    sz_strlcpy(city_styles[i].graphic_alt, 
               secfile_lookup_str(file, "%s.graphic_alt", sec_name));
    sz_strlcpy(city_styles[i].citizens_graphic,
               secfile_lookup_str_default(file, "-", 
                                          "%s.citizens_graphic", sec_name));
    sz_strlcpy(city_styles[i].citizens_graphic_alt, 
               secfile_lookup_str_default(file, "generic", 
                                          "%s.citizens_graphic_alt", sec_name));

    reqs = lookup_req_list(file, compat, sec_name, "reqs", city_style_rule_name(i));
    if (reqs == NULL) {
      ok = FALSE;
      break;
    }
    requirement_vector_copy(&city_styles[i].reqs, reqs);
  }

  section_list_destroy(sec);

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, MUSICSTYLE_SECTION_PREFIX);

    if (sec != NULL) {
      int musi;

      game.control.num_music_styles = section_list_size(sec);
      music_styles_alloc(game.control.num_music_styles);
      musi = 0;

      section_list_iterate(sec, psection) {
        struct requirement_vector *reqs;
        struct music_style *pmus = music_style_by_number(musi);
        const char *sec_name = section_name(psection);

        sz_strlcpy(pmus->music_peaceful,
                   secfile_lookup_str_default(file, "-",
                                              "%s.music_peaceful", sec_name));
        sz_strlcpy(pmus->music_combat,
                   secfile_lookup_str_default(file, "-",
                                              "%s.music_combat", sec_name));

        reqs = lookup_req_list(file, compat, sec_name, "reqs", "Music Style");
        if (reqs == NULL) {
          ok = FALSE;
          break;
        }
        requirement_vector_copy(&pmus->reqs, reqs);

        musi++;
      } section_list_iterate_end;
    }

    section_list_destroy(sec);
  }

  return ok;
}

/**********************************************************************//**
  Load a list of unit type flags that must be absent from the actor unit
  if an action auto performer should be triggered into an action auto
  performer.
**************************************************************************/
static bool load_action_auto_uflag_block(struct section_file *file,
                                         struct action_auto_perf *auto_perf,
                                         const char *uflags_path,
                                         const char *filename)
{
  /* Add each listed protected unit type flag as a !present
   * requirement. */
  if (secfile_entry_lookup(file, "%s", uflags_path)) {
    enum unit_type_flag_id *protecor_flag;
    size_t psize;
    int i;

    protecor_flag =
        secfile_lookup_enum_vec(file, &psize, unit_type_flag_id,
                                "%s", uflags_path);

    if (!protecor_flag) {
      /* Entity exists but couldn't read it. */
      ruleset_error(LOG_ERROR,
                    "\"%s\": %s: bad unit type flag list.",
                    filename, uflags_path);

      return FALSE;
    }

    for (i = 0; i < psize; i++) {
      requirement_vector_append(&auto_perf->reqs,
                                req_from_values(VUT_UTFLAG,
                                                REQ_RANGE_LOCAL,
                                                FALSE, FALSE, TRUE,
                                                protecor_flag[i]));
    }

    free(protecor_flag);
  }

  return TRUE;
}

/**********************************************************************//**
  Load the list of actions an action auto performer should try. The
  actions will be tried in the given order.
**************************************************************************/
static bool load_action_auto_actions(struct section_file *file,
                                     struct action_auto_perf *auto_perf,
                                     const char *actions_path,
                                     const char *filename)
{
  /* Read the alternative actions. */
  if (secfile_entry_lookup(file, "%s", actions_path)) {
    enum gen_action *unit_acts;
    size_t asize;
    int i;

    unit_acts = secfile_lookup_enum_vec(file, &asize, gen_action,
                                        "%s", actions_path);

    if (!unit_acts) {
      /* Entity exists but couldn't read it. */
      ruleset_error(LOG_ERROR,
                    "\"%s\": %s: bad action list",
                    filename, actions_path);

      return FALSE;
    }

    for (i = 0; i < asize; i++) {
      auto_perf->alternatives[i] = unit_acts[i];
    }

    free(unit_acts);
  }

  return TRUE;
}

/**********************************************************************//**
  Load missing unit upkeep ruleset settings as action auto performers.
**************************************************************************/
static bool load_muuk_as_action_auto(struct section_file *file,
                                     struct action_auto_perf *auto_perf,
                                     const char *item,
                                     const char *filename)
{
  char uflags_path[100];
  char action_path[100];

  fc_snprintf(uflags_path, sizeof(uflags_path),
              "missing_unit_upkeep.%s_protected", item);
  fc_snprintf(action_path, sizeof(action_path),
              "missing_unit_upkeep.%s_unit_act", item);

  return (load_action_auto_uflag_block(file, auto_perf, uflags_path,
                                       filename)
          && load_action_auto_actions(file, auto_perf, action_path,
                                      filename));
}

/**********************************************************************//**
  Load cities.ruleset file
**************************************************************************/
static bool load_ruleset_cities(struct section_file *file,
                                struct rscompat_info *compat)
{
  const char *filename = secfile_name(file);
  const char *item;
  struct section_list *sec;
  bool ok = TRUE;

  compat->ver_cities = rscompat_check_capabilities(file, filename, compat);
  if (compat->ver_cities <= 0) {
    return FALSE;
  }

  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  /* Specialist options */
  sec = secfile_sections_by_name_prefix(file, SPECIALIST_SECTION_PREFIX);
  if (section_list_size(sec) >= SP_MAX) {
    ruleset_error(LOG_ERROR, "\"%s\": Too many specialists (%d, max %d).",
                  filename, section_list_size(sec), SP_MAX);
    ok = FALSE;
  }

  if (ok) {
    int i = 0;
    const char *tag;

    game.control.num_specialist_types = section_list_size(sec);

    section_list_iterate(sec, psection) {
      struct specialist *s = specialist_by_number(i);
      struct requirement_vector *reqs;
      const char *sec_name = section_name(psection);

      if (!ruleset_load_names(&s->name, NULL, file, sec_name)) {
        ok = FALSE;
        break;
      }

      item = secfile_lookup_str_default(file, untranslated_name(&s->name),
                                        "%s.short_name", sec_name);
      name_set(&s->abbreviation, NULL, item);

      tag = secfile_lookup_str(file, "%s.graphic", sec_name);
      if (tag == NULL) {
        ruleset_error(LOG_ERROR,
                      "\"%s\": No graphic tag for specialist at %s.",
                      filename, sec_name);
        ok = FALSE;
        break;
      }
      sz_strlcpy(s->graphic_str, tag);
      sz_strlcpy(s->graphic_alt,
                 secfile_lookup_str_default(file, "-",
                                            "%s.graphic_alt", sec_name));

      reqs = lookup_req_list(file, compat, sec_name, "reqs", specialist_rule_name(s));
      if (reqs == NULL) {
        ok = FALSE;
        break;
      }
      requirement_vector_copy(&s->reqs, reqs);

      s->helptext = lookup_strvec(file, sec_name, "helptext");

      if (requirement_vector_size(&s->reqs) == 0 && DEFAULT_SPECIALIST == -1) {
        DEFAULT_SPECIALIST = i;
      }
      i++;
    } section_list_iterate_end;
  }

  if (ok && DEFAULT_SPECIALIST == -1) {
    ruleset_error(LOG_ERROR,
                  "\"%s\": must give a min_size of 0 for at least one "
                  "specialist type.", filename);
    ok = FALSE;
  }
  section_list_destroy(sec);
  sec = NULL;

  if (ok) {
    /* City Parameters */

    game.info.celebratesize = 
      secfile_lookup_int_default(file, GAME_DEFAULT_CELEBRATESIZE,
                                 "parameters.celebrate_size_limit");
    game.info.add_to_size_limit =
      secfile_lookup_int_default(file, GAME_DEFAULT_ADDTOSIZE, "parameters.add_to_size_limit");
    game.info.angrycitizen =
      secfile_lookup_bool_default(file, GAME_DEFAULT_ANGRYCITIZEN,
                                  "parameters.angry_citizens");

    game.info.changable_tax = 
      secfile_lookup_bool_default(file, GAME_DEFAULT_CHANGABLE_TAX, "parameters.changable_tax");
    game.info.forced_science = 
      secfile_lookup_int_default(file, 0, "parameters.forced_science");
    game.info.forced_luxury = 
      secfile_lookup_int_default(file, 100, "parameters.forced_luxury");
    game.info.forced_gold = 
      secfile_lookup_int_default(file, 0, "parameters.forced_gold");
    if (game.info.forced_science + game.info.forced_luxury
        + game.info.forced_gold != 100) {
      ruleset_error(LOG_ERROR,
                    "\"%s\": Forced taxes do not add up in ruleset!",
                    filename);
      ok = FALSE;
    }
  }

  if (ok) {
    /* civ1 & 2 didn't reveal tiles */
    game.server.vision_reveal_tiles =
      secfile_lookup_bool_default(file, GAME_DEFAULT_VISION_REVEAL_TILES,
                                  "parameters.vision_reveal_tiles");

    game.info.pop_report_zeroes =
      secfile_lookup_int_default(file, 1, "parameters.pop_report_zeroes");

    /* Citizens configuration. */
    game.info.citizen_nationality =
      secfile_lookup_bool_default(file, GAME_DEFAULT_NATIONALITY,
                                  "citizen.nationality");
    game.info.citizen_convert_speed =
      secfile_lookup_int_default(file, GAME_DEFAULT_CONVERT_SPEED,
                                 "citizen.convert_speed");
    game.info.citizen_partisans_pct =
      secfile_lookup_int_default(file, 0, "citizen.partisans_pct");
    game.info.conquest_convert_pct =
      secfile_lookup_int_default(file, 0, "citizen.conquest_convert_pct");
  }

  if (ok) {
    /* Missing unit upkeep. */
    struct action_auto_perf *auto_perf;

    /* Can't pay food upkeep! */
    auto_perf = action_auto_perf_slot_number(ACTION_AUTO_UPKEEP_FOOD);
    auto_perf->cause = AAPC_UNIT_UPKEEP;

    /* This is about food upkeep. */
    requirement_vector_append(&auto_perf->reqs,
                              req_from_str("OutputType", "Local",
                                           FALSE, TRUE, TRUE,
                                           "Food"));

    /* Internally represented as an action auto performer rule. */
    if (!load_muuk_as_action_auto(file, auto_perf, "food", filename)) {
      ok = FALSE;
    }

    game.info.muuk_food_wipe =
        secfile_lookup_bool_default(file, RS_DEFAULT_MUUK_FOOD_WIPE,
                                    "missing_unit_upkeep.food_wipe");

    /* Can't pay gold upkeep! */
    auto_perf = action_auto_perf_slot_number(ACTION_AUTO_UPKEEP_GOLD);
    auto_perf->cause = AAPC_UNIT_UPKEEP;

    /* This is about gold upkeep. */
    requirement_vector_append(&auto_perf->reqs,
                              req_from_str("OutputType", "Local",
                                           FALSE, TRUE, TRUE,
                                           "Gold"));

    /* Internally represented as an action auto performer rule. */
    if (!load_muuk_as_action_auto(file, auto_perf, "gold", filename)) {
      ok = FALSE;
    }

    game.info.muuk_gold_wipe =
        secfile_lookup_bool_default(file, RS_DEFAULT_MUUK_GOLD_WIPE,
                                    "missing_unit_upkeep.gold_wipe");

    /* Can't pay shield upkeep! */
    auto_perf = action_auto_perf_slot_number(ACTION_AUTO_UPKEEP_SHIELD);
    auto_perf->cause = AAPC_UNIT_UPKEEP;

    /* This is about shield upkeep. */
    requirement_vector_append(&auto_perf->reqs,
                              req_from_str("OutputType", "Local",
                                           FALSE, TRUE, TRUE,
                                           "Shield"));

    /* Internally represented as an action auto performer rule. */
    if (!load_muuk_as_action_auto(file, auto_perf, "shield", filename)) {
      ok = FALSE;
    }

    game.info.muuk_shield_wipe =
        secfile_lookup_bool_default(file, RS_DEFAULT_MUUK_SHIELD_WIPE,
                                    "missing_unit_upkeep.shield_wipe");
  }

  if (ok) {
    secfile_check_unused(file);
  }

  return ok;
}

/**********************************************************************//**
  Load effects.ruleset file
**************************************************************************/
static bool load_ruleset_effects(struct section_file *file,
                                 struct rscompat_info *compat)
{
  struct section_list *sec;
  const char *type;
  const char *filename;
  bool ok = TRUE;

  filename = secfile_name(file);

  compat->ver_effects = rscompat_check_capabilities(file, filename, compat);
  if (compat->ver_effects <= 0) {
    return FALSE;
  }
  (void) secfile_entry_by_path(file, "datafile.description");   /* unused */
  (void) secfile_entry_by_path(file, "datafile.ruledit");       /* unused */

  /* Parse effects and add them to the effects ruleset cache. */
  sec = secfile_sections_by_name_prefix(file, EFFECT_SECTION_PREFIX);
  section_list_iterate(sec, psection) {
    enum effect_type eff;
    int value;
    struct multiplier *pmul;
    struct effect *peffect;
    const char *sec_name = section_name(psection);
    struct requirement_vector *reqs;

    type = secfile_lookup_str(file, "%s.type", sec_name);

    if (type == NULL) {
      ruleset_error(LOG_ERROR, "\"%s\" [%s] missing effect type.", filename, sec_name);
      ok = FALSE;
      break;
    }

    if (compat->compat_mode && rscompat_old_effect_3_1(type, file, sec_name, compat)) {
      continue;
    }

    eff = effect_type_by_name(type, fc_strcasecmp);
    if (!effect_type_is_valid(eff)) {
      ruleset_error(LOG_ERROR, "\"%s\" [%s] lists unknown effect type \"%s\".",
                    filename, sec_name, type);
      ok = FALSE;
      break;
    }

    value = secfile_lookup_int_default(file, 1, "%s.value", sec_name);

    {
      const char *multiplier_name
        = secfile_lookup_str(file, "%s.multiplier", sec_name);

      if (multiplier_name) {
        pmul = multiplier_by_rule_name(multiplier_name);
        if (!pmul) {
          ruleset_error(LOG_ERROR, "\"%s\" [%s] has unknown multiplier \"%s\".",
                        filename, sec_name, multiplier_name);
          ok = FALSE;
          break;
        }
      } else {
        pmul = NULL;
      }
    }

    peffect = effect_new(eff, value, pmul);

    reqs = lookup_req_list(file, compat, sec_name, "reqs", type);
    if (reqs == NULL) {
      ok = FALSE;
      break;
    }

    requirement_vector_iterate(reqs, preq) {
      effect_req_append(peffect, *preq);
    } requirement_vector_iterate_end;

    if (compat->compat_mode) {
      reqs = lookup_req_list(file, compat, sec_name, "nreqs", type);
      if (reqs == NULL) {
        ok = FALSE;
        break;
      }
      requirement_vector_iterate(reqs, preq) {
        preq->present = !preq->present;
        effect_req_append(peffect, *preq);
      } requirement_vector_iterate_end;
    }
  } section_list_iterate_end;
  section_list_destroy(sec);

  if (ok) {
    secfile_check_unused(file);
  }

  return ok;
}

/**********************************************************************//**
  Print an error message if the value is out of range.
**************************************************************************/
static int secfile_lookup_int_default_min_max(struct section_file *file,
                                              int def, int min, int max,
                                              const char *path, ...)
                                              fc__attribute((__format__ (__printf__, 5, 6)));
static int secfile_lookup_int_default_min_max(struct section_file *file,
                                              int def, int min, int max,
                                              const char *path, ...)
{
  char fullpath[256];
  int ival;
  va_list args;

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!secfile_lookup_int(file, &ival, "%s", fullpath)) {
    ival = def;
  }

  if (ival < min) {
    ruleset_error(LOG_ERROR,"\"%s\" should be in the interval [%d, %d] "
                  "but is %d; using the minimal value.",
                  fullpath, min, max, ival);
    ival = min;
  }

  if (ival > max) {
    ruleset_error(LOG_ERROR,"\"%s\" should be in the interval [%d, %d] "
                  "but is %d; using the maximal value.",
                  fullpath, min, max, ival);
    ival = max;
  }

  return ival;
}

/**********************************************************************//**
  Load ui_name of one action
**************************************************************************/
static bool load_action_ui_name(struct section_file *file, int act,
                                const char *entry_name)
{
  const char *text;

  text = secfile_lookup_str_default(file,
                                    action_ui_name_default(act),
                                    "actions.%s", entry_name);
  sz_strlcpy(action_by_number(act)->ui_name, text);

  return TRUE;
}

/**********************************************************************//**
  Load ruleset file.
**************************************************************************/
static bool load_ruleset_game(struct section_file *file, bool act,
                              struct rscompat_info *compat)
{
  const char *sval, **svec;
  const char *filename;
  int *food_ini;
  int i;
  size_t teams;
  const char *pref_text;
  size_t gni_tmp;
  struct section_list *sec;
  size_t nval;
  const char *name;
  bool ok = TRUE;

  if (file == NULL) {
    return FALSE;
  }
  filename = secfile_name(file);

  name = secfile_lookup_str_default(file, NULL, "ruledit.description_file");
  if (name != NULL) {
    game.server.ruledit.description_file = fc_strdup(name);
  }

  /* section: tileset */
  pref_text = secfile_lookup_str_default(file, "", "tileset.prefered");
  if (pref_text[0] != '\0') {
    log_deprecation("Entry tileset.prefered in game.ruleset."
                    " Use correct spelling tileset.preferred instead");
  }
  pref_text = secfile_lookup_str_default(file, pref_text, "tileset.preferred");
  if (pref_text[0] != '\0') {
    /* There was tileset suggestion */
    sz_strlcpy(game.control.preferred_tileset, pref_text);
  } else {
    /* No tileset suggestions */
    game.control.preferred_tileset[0] = '\0';
  }

  /* section: soundset */
  pref_text = secfile_lookup_str_default(file, "", "soundset.prefered");
  if (pref_text[0] != '\0') {
    log_deprecation("Entry soundset.prefered in game.ruleset."
                    " Use correct spelling soundset.preferred instead");
  }
  pref_text = secfile_lookup_str_default(file, pref_text, "soundset.preferred");
  if (pref_text[0] != '\0') {
    /* There was soundset suggestion */
    sz_strlcpy(game.control.preferred_soundset, pref_text);
  } else {
    /* No soundset suggestions */
    game.control.preferred_soundset[0] = '\0';
  }

  /* section: musicset */
  pref_text = secfile_lookup_str_default(file, "", "musicset.prefered");
  if (pref_text[0] != '\0') {
    log_deprecation("Entry musicset.prefered in game.ruleset."
                    " Use correct spelling musicset.preferred instead");
  }
  pref_text = secfile_lookup_str_default(file, pref_text, "musicset.preferred");
  if (pref_text[0] != '\0') {
    /* There was musicset suggestion */
    sz_strlcpy(game.control.preferred_musicset, pref_text);
  } else {
    /* No musicset suggestions */
    game.control.preferred_musicset[0] = '\0';
  }

  /* section: about */
  pref_text = secfile_lookup_str(file, "about.name");
  /* Ruleset/modpack name found */
  sz_strlcpy(game.control.name, pref_text);

  pref_text = secfile_lookup_str_default(file, "", "about.version");
  if (pref_text[0] != '\0') {
    /* Ruleset/modpack version found */
    sz_strlcpy(game.control.version, pref_text);
  } else {
    /* No version information */
    game.control.version[0] = '\0';
  }

  pref_text = secfile_lookup_str_default(file, "", "about.summary");
  if (pref_text[0] != '\0') {
    int len;

    /* Ruleset/modpack summary found */
    len = strlen(pref_text);
    game.ruleset_summary = fc_malloc(len + 1);
    fc_strlcpy(game.ruleset_summary, pref_text, len + 1);
  } else {
    /* No summary */
    if (game.ruleset_summary != NULL) {
      free(game.ruleset_summary);
      game.ruleset_summary = NULL;
    }
  }

  pref_text = secfile_lookup_str_default(file, "", "about.description");
  if (pref_text[0] != '\0') {
    int len;

    /* Ruleset/modpack description found */
    len = strlen(pref_text);
    game.ruleset_description = fc_malloc(len + 1);
    fc_strlcpy(game.ruleset_description, pref_text, len + 1);
    game.control.desc_length = len;
  } else {
    /* No description */
    if (game.ruleset_description != NULL) {
      free(game.ruleset_description);
      game.ruleset_description = NULL;
    }
    game.control.desc_length = 0;
  }

  pref_text = secfile_lookup_str_default(file, "", "about.capabilities");
  if (pref_text[0] != '\0') {
    int len = strlen(pref_text);

    game.ruleset_capabilities = fc_malloc(len + 1);
    fc_strlcpy(game.ruleset_capabilities, pref_text, len +1);
  } else {
    game.ruleset_capabilities = fc_malloc(1);
    game.ruleset_capabilities[0] = '\0';
  }

  /* section: options */
  if (!lookup_tech_list(file, "options", "global_init_techs",
                        game.rgame.global_init_techs, filename)) {
    ok = FALSE;
  }

  if (ok) {
    if (!lookup_building_list(file, "options", "global_init_buildings",
                              game.rgame.global_init_buildings, filename)) {
      ok = FALSE;
    }
  }

  if (ok) {
    const char **slist;
    int j;

    game.control.popup_tech_help = secfile_lookup_bool_default(file, FALSE,
                                                               "options.popup_tech_help");

    /* section: civstyle */
    game.info.base_pollution
      = secfile_lookup_int_default(file, RS_DEFAULT_BASE_POLLUTION,
                                   "civstyle.base_pollution");

    game.info.gameloss_style = GAMELOSS_STYLE_CLASSICAL;

    slist = secfile_lookup_str_vec(file, &nval, "civstyle.gameloss_style");
    for (j = 0; j < nval; j++) {
      enum gameloss_style style;

      sval = slist[j];
      if (strcmp(sval, "") == 0) {
        continue;
      }
      style = gameloss_style_by_name(sval, fc_strcasecmp);
      if (!gameloss_style_is_valid(style)) {
        ruleset_error(LOG_ERROR, "\"%s\": bad value \"%s\" for gameloss_style.",
                      filename, sval);
        ok = FALSE;
        break;
      } else {
        game.info.gameloss_style |= style;
      }
    }
    free(slist);
  }

  if (ok) {
    game.info.happy_cost
      = secfile_lookup_int_def_min_max(file,
                                       RS_DEFAULT_HAPPY_COST,
                                       RS_MIN_HAPPY_COST,
                                       RS_MAX_HAPPY_COST,
                                       "civstyle.happy_cost");
    game.info.food_cost
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_FOOD_COST,
                                           RS_MIN_FOOD_COST,
                                           RS_MAX_FOOD_COST,
                                           "civstyle.food_cost");
    game.info.civil_war_enabled
      = secfile_lookup_bool_default(file, TRUE, "civstyle.civil_war_enabled");

    game.info.paradrop_to_transport
      = secfile_lookup_bool_default(file, FALSE,
                                    "civstyle.paradrop_to_transport");

    /* TODO: move to global_unit_options */
    game.info.base_bribe_cost
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_BASE_BRIBE_COST,
                                           RS_MIN_BASE_BRIBE_COST,
                                           RS_MAX_BASE_BRIBE_COST,
                                           "civstyle.base_bribe_cost");
    /* TODO: move to global_unit_options */
    game.server.ransom_gold
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_RANSOM_GOLD,
                                           RS_MIN_RANSOM_GOLD,
                                           RS_MAX_RANSOM_GOLD,
                                           "civstyle.ransom_gold");
    /* TODO: move to global_unit_options */
    game.info.pillage_select
      = secfile_lookup_bool_default(file, RS_DEFAULT_PILLAGE_SELECT,
                                    "civstyle.pillage_select");

   game.info.tech_steal_allow_holes
      = secfile_lookup_bool_default(file, RS_DEFAULT_TECH_STEAL_HOLES,
                                    "civstyle.tech_steal_allow_holes");
   game.info.tech_trade_allow_holes
      = secfile_lookup_bool_default(file, RS_DEFAULT_TECH_TRADE_HOLES,
                                    "civstyle.tech_trade_allow_holes");
   game.info.tech_trade_loss_allow_holes
      = secfile_lookup_bool_default(file, RS_DEFAULT_TECH_TRADE_LOSS_HOLES,
                                    "civstyle.tech_trade_loss_allow_holes");
   game.info.tech_parasite_allow_holes
      = secfile_lookup_bool_default(file, RS_DEFAULT_TECH_PARASITE_HOLES,
                                    "civstyle.tech_parasite_allow_holes");
   game.info.tech_loss_allow_holes
      = secfile_lookup_bool_default(file, RS_DEFAULT_TECH_LOSS_HOLES,
                                    "civstyle.tech_loss_allow_holes");

    /* TODO: move to global_unit_options */
    game.server.upgrade_veteran_loss
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_UPGRADE_VETERAN_LOSS,
                                           RS_MIN_UPGRADE_VETERAN_LOSS,
                                           RS_MAX_UPGRADE_VETERAN_LOSS,
                                           "civstyle.upgrade_veteran_loss");
    /* TODO: move to global_unit_options */
    game.server.autoupgrade_veteran_loss
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_UPGRADE_VETERAN_LOSS,
                                           RS_MIN_UPGRADE_VETERAN_LOSS,
                                           RS_MAX_UPGRADE_VETERAN_LOSS,
                                           "civstyle.autoupgrade_veteran_loss");

    game.info.base_tech_cost
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_BASE_TECH_COST,
                                           RS_MIN_BASE_TECH_COST,
                                           RS_MAX_BASE_TECH_COST,
                                           "research.base_tech_cost");

    food_ini = secfile_lookup_int_vec(file, &gni_tmp,
                                      "civstyle.granary_food_ini");
    game.info.granary_num_inis = (int) gni_tmp;

    if (game.info.granary_num_inis > MAX_GRANARY_INIS) {
      ruleset_error(LOG_ERROR,
                    "Too many granary_food_ini entries (%d, max %d)",
                    game.info.granary_num_inis, MAX_GRANARY_INIS);
      ok = FALSE;
    } else if (game.info.granary_num_inis == 0) {
      log_error("No values for granary_food_ini. Using default "
                "value %d.", RS_DEFAULT_GRANARY_FOOD_INI);
      game.info.granary_num_inis = 1;
      game.info.granary_food_ini[0] = RS_DEFAULT_GRANARY_FOOD_INI;
    } else {
      int gi;

      /* check for <= 0 entries */
      for (gi = 0; gi < game.info.granary_num_inis; gi++) {
        if (food_ini[gi] <= 0) {
          if (gi == 0) {
            food_ini[gi] = RS_DEFAULT_GRANARY_FOOD_INI;
          } else {
            food_ini[gi] = food_ini[gi - 1];
          }
          log_error("Bad value for granary_food_ini[%i]. Using %i.",
                    gi, food_ini[gi]);
        }
        game.info.granary_food_ini[gi] = food_ini[gi];
      }
    }
    free(food_ini);
  }

  if (ok) {
    game.info.granary_food_inc
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_GRANARY_FOOD_INC,
                                           RS_MIN_GRANARY_FOOD_INC,
                                           RS_MAX_GRANARY_FOOD_INC,
                                           "civstyle.granary_food_inc");

    output_type_iterate(o) {
      game.info.min_city_center_output[o]
        = secfile_lookup_int_default_min_max(file,
                                             RS_DEFAULT_CITY_CENTER_OUTPUT,
                                             RS_MIN_CITY_CENTER_OUTPUT,
                                             RS_MAX_CITY_CENTER_OUTPUT,
                                             "civstyle.min_city_center_%s",
                                             get_output_identifier(o));
    } output_type_iterate_end;
  }

  if (ok) {
    const char *tus_text;

    game.server.init_vis_radius_sq
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_VIS_RADIUS_SQ,
                                           RS_MIN_VIS_RADIUS_SQ,
                                           RS_MAX_VIS_RADIUS_SQ,
                                           "civstyle.init_vis_radius_sq");

    game.info.init_city_radius_sq
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_CITY_RADIUS_SQ,
                                           RS_MIN_CITY_RADIUS_SQ,
                                           RS_MAX_CITY_RADIUS_SQ,
                                           "civstyle.init_city_radius_sq");

    tus_text = secfile_lookup_str_default(file, RS_DEFAULT_GOLD_UPKEEP_STYLE,
                                          "civstyle.gold_upkeep_style");
    game.info.gold_upkeep_style = gold_upkeep_style_by_name(tus_text,
                                                            fc_strcasecmp);
    if (!gold_upkeep_style_is_valid(game.info.gold_upkeep_style)) {
      ruleset_error(LOG_ERROR, "Unknown gold upkeep style \"%s\"",
                    tus_text);
      ok = FALSE;
    }

    /* section: illness */
    game.info.illness_on
      = secfile_lookup_bool_default(file, RS_DEFAULT_ILLNESS_ON,
                                    "illness.illness_on");
    game.info.illness_base_factor
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_ILLNESS_BASE_FACTOR,
                                           RS_MIN_ILLNESS_BASE_FACTOR,
                                           RS_MAX_ILLNESS_BASE_FACTOR,
                                           "illness.illness_base_factor");
    game.info.illness_min_size
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_ILLNESS_MIN_SIZE,
                                           RS_MIN_ILLNESS_MIN_SIZE,
                                           RS_MAX_ILLNESS_MIN_SIZE,
                                           "illness.illness_min_size");
    game.info.illness_trade_infection
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_ILLNESS_TRADE_INFECTION_PCT,
                                           RS_MIN_ILLNESS_TRADE_INFECTION_PCT,
                                           RS_MAX_ILLNESS_TRADE_INFECTION_PCT,
                                           "illness.illness_trade_infection");
    game.info.illness_pollution_factor
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_ILLNESS_POLLUTION_PCT,
                                           RS_MIN_ILLNESS_POLLUTION_PCT,
                                           RS_MAX_ILLNESS_POLLUTION_PCT,
                                           "illness.illness_pollution_factor");

    /* section: incite_cost */
    game.server.base_incite_cost
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_INCITE_BASE_COST,
                                           RS_MIN_INCITE_BASE_COST,
                                           RS_MAX_INCITE_BASE_COST,
                                           "incite_cost.base_incite_cost");
    game.server.incite_improvement_factor
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_INCITE_IMPROVEMENT_FCT,
                                           RS_MIN_INCITE_IMPROVEMENT_FCT,
                                           RS_MAX_INCITE_IMPROVEMENT_FCT,
                                           "incite_cost.improvement_factor");
    game.server.incite_unit_factor
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_INCITE_UNIT_FCT,
                                           RS_MIN_INCITE_UNIT_FCT,
                                           RS_MAX_INCITE_UNIT_FCT,
                                           "incite_cost.unit_factor");
    game.server.incite_total_factor
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_INCITE_TOTAL_FCT,
                                           RS_MIN_INCITE_TOTAL_FCT,
                                           RS_MAX_INCITE_TOTAL_FCT,
                                           "incite_cost.total_factor");

    /* section: global_unit_options */
    game.info.slow_invasions
      = secfile_lookup_bool_default(file, RS_DEFAULT_SLOW_INVASIONS,
                                    "global_unit_options.slow_invasions");

    if (ok) {
      /* Auto attack. */
      struct action_auto_perf *auto_perf;

      /* A unit moved next to this unit and the autoattack server setting
       * is enabled. */
      auto_perf = action_auto_perf_slot_number(ACTION_AUTO_MOVED_ADJ);
      auto_perf->cause = AAPC_UNIT_MOVED_ADJ;

      /* Auto attack happens during war. */
      requirement_vector_append(&auto_perf->reqs,
                                req_from_values(VUT_DIPLREL,
                                                REQ_RANGE_LOCAL,
                                                FALSE, TRUE, TRUE, DS_WAR));

      /* Needs a movement point to auto attack. */
      requirement_vector_append(&auto_perf->reqs,
                                req_from_values(VUT_MINMOVES,
                                                REQ_RANGE_LOCAL,
                                                FALSE, TRUE, TRUE, 1));

      /* Internally represented as an action auto performer rule. */
      if (!load_action_auto_uflag_block(file, auto_perf,
                                         "auto_attack.will_never",
                                         filename)) {
        ok = FALSE;
      }

      /* TODO: It would be great if unit_survive_autoattack() could be made
       * flexible enough to also handle diplomatic actions etc. */
      auto_perf->alternatives[0] = ACTION_CAPTURE_UNITS;
      auto_perf->alternatives[1] = ACTION_BOMBARD;
      auto_perf->alternatives[2] = ACTION_ATTACK;
      auto_perf->alternatives[3] = ACTION_SUICIDE_ATTACK;
    }

    /* section: actions */
    if (ok) {
      int force_capture_units, force_bombard, force_explode_nuclear;

      if (secfile_lookup_bool_default(file, RS_DEFAULT_FORCE_TRADE_ROUTE,
                                      "actions.force_trade_route")) {
        /* Forbid entering the marketplace when a trade route can be
         * established. */
        BV_SET(action_by_number(ACTION_MARKETPLACE)->blocked_by,
               ACTION_TRADE_ROUTE);
      }

      /* Forbid bombarding, exploading nuclear or attacking when it is
       * legal to capture units. */
      force_capture_units
        = secfile_lookup_bool_default(file, RS_DEFAULT_FORCE_CAPTURE_UNITS,
                                      "actions.force_capture_units");

      if (force_capture_units) {
        BV_SET(action_by_number(ACTION_BOMBARD)->blocked_by,
               ACTION_CAPTURE_UNITS);
        BV_SET(action_by_number(ACTION_NUKE)->blocked_by,
               ACTION_CAPTURE_UNITS);
        BV_SET(action_by_number(ACTION_SUICIDE_ATTACK)->blocked_by,
               ACTION_CAPTURE_UNITS);
        BV_SET(action_by_number(ACTION_ATTACK)->blocked_by,
               ACTION_CAPTURE_UNITS);
        BV_SET(action_by_number(ACTION_CONQUER_CITY)->blocked_by,
               ACTION_CAPTURE_UNITS);
      }

      /* Forbid exploding nuclear or attacking when it is legal to
       * bombard. */
      force_bombard
        = secfile_lookup_bool_default(file, RS_DEFAULT_FORCE_BOMBARD,
                                      "actions.force_bombard");

      if (force_bombard) {
        BV_SET(action_by_number(ACTION_NUKE)->blocked_by,
               ACTION_BOMBARD);
        BV_SET(action_by_number(ACTION_SUICIDE_ATTACK)->blocked_by,
               ACTION_BOMBARD);
        BV_SET(action_by_number(ACTION_ATTACK)->blocked_by,
               ACTION_BOMBARD);
        BV_SET(action_by_number(ACTION_CONQUER_CITY)->blocked_by,
               ACTION_BOMBARD);
      }

      /* Forbid attacking when it is legal to do explode nuclear. */
      force_explode_nuclear
        = secfile_lookup_bool_default(file,
                                      RS_DEFAULT_FORCE_EXPLODE_NUCLEAR,
                                      "actions.force_explode_nuclear");

      if (force_explode_nuclear) {
        BV_SET(action_by_number(ACTION_SUICIDE_ATTACK)->blocked_by,
               ACTION_NUKE);
        BV_SET(action_by_number(ACTION_ATTACK)->blocked_by,
               ACTION_NUKE);
        BV_SET(action_by_number(ACTION_CONQUER_CITY)->blocked_by,
               ACTION_NUKE);
      }

      /* If the "Poison City" action or the "Poison City Escape" action
       * should empty the granary. */
      /* TODO: empty granary and reduce population should become separate
       * action effect flags when actions are generalized. */
      game.info.poison_empties_food_stock
        = secfile_lookup_bool_default(file,
                                      RS_DEFAULT_POISON_EMPTIES_FOOD_STOCK,
                                      "actions.poison_empties_food_stock");

      /* Allow setting max distance for bombardment before generalized
       * actions. */
      {
        struct entry *pentry;
        int max_range;

        pentry = secfile_entry_lookup(file, "actions.bombard_max_range");

        if (!pentry) {
          max_range = RS_DEFAULT_BOMBARD_MAX_RANGE;
        } else {
          switch (entry_type(pentry)) {
          case ENTRY_INT:
            if (entry_int_get(pentry, &max_range)) {
              break;
            }
            /* Fall through to error handling. */
          case ENTRY_STR:
            {
              const char *custom;

              if (entry_str_get(pentry, &custom)
                  && !fc_strcasecmp(custom, RS_ACTION_NO_MAX_DISTANCE)) {
                max_range = ACTION_DISTANCE_UNLIMITED;
                break;
              }
            }
            /* Fall through to error handling. */
          default:
            ruleset_error(LOG_ERROR, "Bad actions.bombard_max_range");
            ok = FALSE;
            max_range = RS_DEFAULT_BOMBARD_MAX_RANGE;
            break;
          }
        }

        action_by_number(ACTION_BOMBARD)->max_distance = max_range;
      }

      action_iterate(act_id) {
        load_action_ui_name(file, act_id,
                            action_ui_name_ruleset_var_name(act_id));
      } action_iterate_end;

      /* The quiet (don't auto generate help for) property of all actions
       * live in a single enum vector. This avoids generic action
       * expectations. */
      if (secfile_entry_by_path(file, "actions.quiet_actions")) {
        enum gen_action *quiet_actions;
        size_t asize;
        int j;

        quiet_actions =
            secfile_lookup_enum_vec(file, &asize, gen_action,
                                    "actions.quiet_actions");

        if (!quiet_actions) {
          /* Entity exists but couldn't read it. */
          ruleset_error(LOG_ERROR,
                        "\"%s\": actions.quiet_actions: bad action list",
                        filename);

          ok = FALSE;
        }

        for (j = 0; j < asize; j++) {
          /* Don't auto generate help text for this action. */
          action_by_number(quiet_actions[j])->quiet = TRUE;
        }

        free(quiet_actions);
      }
    }

    if (ok) {
      sec = secfile_sections_by_name_prefix(file,
                                            ACTION_ENABLER_SECTION_PREFIX);

      if (sec) {
        section_list_iterate(sec, psection) {
          struct action_enabler *enabler;
          const char *sec_name = section_name(psection);
          struct action *paction;
          struct requirement_vector *actor_reqs;
          struct requirement_vector *target_reqs;
          const char *action_text;

          enabler = action_enabler_new();

          action_text = secfile_lookup_str(file, "%s.action", sec_name);

          if (action_text == NULL) {
            ruleset_error(LOG_ERROR, "\"%s\" [%s] missing action to enable.",
                          filename, sec_name);
            ok = FALSE;
            break;
          }

          paction = action_by_rule_name(action_text);
          if (!paction) {
            ruleset_error(LOG_ERROR, "\"%s\" [%s] lists unknown action type \"%s\".",
                          filename, sec_name, action_text);
            ok = FALSE;
            break;
          }

          enabler->action = paction->id;

          actor_reqs = lookup_req_list(file, compat, sec_name, "actor_reqs", action_text);
          if (actor_reqs == NULL) {
            ok = FALSE;
            break;
          }

          requirement_vector_copy(&enabler->actor_reqs, actor_reqs);

          target_reqs = lookup_req_list(file, compat, sec_name, "target_reqs", action_text);
          if (target_reqs == NULL) {
            ok = FALSE;
            break;
          }

          requirement_vector_copy(&enabler->target_reqs, target_reqs);

          action_enabler_add(enabler);
        } section_list_iterate_end;
        section_list_destroy(sec);
      }
    }
  }

  if (ok) {
    const char *tus_text;

    /* section: combat_rules */
    game.info.tired_attack
      = secfile_lookup_bool_default(file, RS_DEFAULT_TIRED_ATTACK,
                                    "combat_rules.tired_attack");

    /* section: borders */
    game.info.border_city_radius_sq
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_BORDER_RADIUS_SQ_CITY,
                                           RS_MIN_BORDER_RADIUS_SQ_CITY,
                                           RS_MAX_BORDER_RADIUS_SQ_CITY,
                                           "borders.radius_sq_city");
    game.info.border_size_effect
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_BORDER_SIZE_EFFECT,
                                           RS_MIN_BORDER_SIZE_EFFECT,
                                           RS_MAX_BORDER_SIZE_EFFECT,
                                           "borders.size_effect");

    game.info.border_city_permanent_radius_sq
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_BORDER_RADIUS_SQ_CITY_PERMANENT,
                                           RS_MIN_BORDER_RADIUS_SQ_CITY_PERMANENT,
                                           RS_MAX_BORDER_RADIUS_SQ_CITY_PERMANENT,
                                           "borders.radius_sq_city_permanent");

    /* section: research */
    tus_text = secfile_lookup_str_default(file, RS_DEFAULT_TECH_COST_STYLE,
                                          "research.tech_cost_style");
    game.info.tech_cost_style = tech_cost_style_by_name(tus_text,
                                                        fc_strcasecmp);
    if (!tech_cost_style_is_valid(game.info.tech_cost_style)) {
      ruleset_error(LOG_ERROR, "Unknown tech cost style \"%s\"",
                    tus_text);
      ok = FALSE;
    }

    tus_text = secfile_lookup_str_default(file, RS_DEFAULT_TECH_LEAKAGE,
                                          "research.tech_leakage");
    game.info.tech_leakage = tech_leakage_style_by_name(tus_text,
                                                        fc_strcasecmp);
    if (!tech_leakage_style_is_valid(game.info.tech_leakage)) {
      ruleset_error(LOG_ERROR, "Unknown tech leakage \"%s\"",
                    tus_text);
      ok = FALSE;
    }
    if (game.info.tech_cost_style == TECH_COST_CIV1CIV2
        && game.info.tech_leakage != TECH_LEAKAGE_NONE) {
      log_error("Only tech_leakage \"%s\" supported with "
                "tech_cost_style \"%s\". ",
                tech_leakage_style_name(TECH_LEAKAGE_NONE),
                tech_cost_style_name(TECH_COST_CIV1CIV2));
      log_error("Switching to tech_leakage \"%s\".",
                tech_leakage_style_name(TECH_LEAKAGE_NONE));
      game.info.tech_leakage = TECH_LEAKAGE_NONE;
    }
    game.info.base_tech_cost
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_BASE_TECH_COST,
                                           RS_MIN_BASE_TECH_COST,
                                           RS_MAX_BASE_TECH_COST,
                                           "research.base_tech_cost");

    tus_text = secfile_lookup_str_default(file, RS_DEFAULT_TECH_UPKEEP_STYLE,
                                          "research.tech_upkeep_style");

    game.info.tech_upkeep_style = tech_upkeep_style_by_name(tus_text, fc_strcasecmp);

    if (!tech_upkeep_style_is_valid(game.info.tech_upkeep_style)) {
      ruleset_error(LOG_ERROR, "Unknown tech upkeep style \"%s\"",
                    tus_text);
      ok = FALSE;
    }
  }

  if (ok) {
    game.info.tech_upkeep_divider
      = secfile_lookup_int_default_min_max(file,
                                           RS_DEFAULT_TECH_UPKEEP_DIVIDER,
                                           RS_MIN_TECH_UPKEEP_DIVIDER,
                                           RS_MAX_TECH_UPKEEP_DIVIDER,
                                           "research.tech_upkeep_divider");

    sval = secfile_lookup_str_default(file, NULL, "research.free_tech_method");
    if (sval == NULL) {
      ruleset_error(LOG_ERROR, "No free_tech_method given");
      ok = FALSE;
    } else {
      game.info.free_tech_method = free_tech_method_by_name(sval, fc_strcasecmp);
      if (!free_tech_method_is_valid(game.info.free_tech_method)) {
        ruleset_error(LOG_ERROR, "Bad value %s for free_tech_method.", sval);
        ok = FALSE;
      }
    }
  }

  if (ok) {
    int cf;

    /* section: culture */
    game.info.culture_vic_points
      = secfile_lookup_int_default(file, RS_DEFAULT_CULTURE_VIC_POINTS,
                                   "culture.victory_min_points");
    game.info.culture_vic_lead
      = secfile_lookup_int_default(file, RS_DEFAULT_CULTURE_VIC_LEAD,
                                   "culture.victory_lead_pct");
    game.info.culture_migration_pml
      = secfile_lookup_int_default(file, RS_DEFAULT_CULTURE_MIGRATION_PML,
                                   "culture.migration_pml");

    /* section: calendar */
    game.calendar.calendar_skip_0
      = secfile_lookup_bool_default(file, RS_DEFAULT_CALENDAR_SKIP_0,
                                    "calendar.skip_year_0");
    game.server.start_year
      = secfile_lookup_int_default(file, GAME_START_YEAR,
                                   "calendar.start_year");
    game.calendar.calendar_fragments
      = secfile_lookup_int_default(file, 0, "calendar.fragments");

    if (game.calendar.calendar_fragments > MAX_CALENDAR_FRAGMENTS) {
      ruleset_error(LOG_ERROR, "Too many calendar fragments. Max is %d",
                    MAX_CALENDAR_FRAGMENTS);
      ok = FALSE;
      game.calendar.calendar_fragments = 0;
    }
    sz_strlcpy(game.calendar.positive_year_label,
               secfile_lookup_str_default(file,
                                          RS_DEFAULT_POS_YEAR_LABEL,
                                          "calendar.positive_label"));
    sz_strlcpy(game.calendar.negative_year_label,
               secfile_lookup_str_default(file,
                                          RS_DEFAULT_NEG_YEAR_LABEL,
                                          "calendar.negative_label"));

    for (cf = 0; cf < game.calendar.calendar_fragments; cf++) {
      const char *fname;

      fname = secfile_lookup_str_default(file, NULL, "calendar.fragment_name%d", cf);
      if (fname != NULL) {
        strncpy(game.calendar.calendar_fragment_name[cf], fname,
                sizeof(game.calendar.calendar_fragment_name[cf]));
      }
    }
  }

  if (ok) {
    /* section playercolors */
    struct rgbcolor *prgbcolor = NULL;
    bool color_read = TRUE;

    /* Check if the player list is defined and empty. */
    if (playercolor_count() != 0) {
      ok = FALSE;
    } else {
      i = 0;

      while (color_read) {
        prgbcolor = NULL;

        color_read = rgbcolor_load(file, &prgbcolor, "playercolors.colorlist%d", i);
        if (color_read) {
          playercolor_add(prgbcolor);
        }

        i++;
      }

      if (playercolor_count() == 0) {
        ruleset_error(LOG_ERROR, "No player colors defined!");
        ok = FALSE;
      }

      if (ok) {
        fc_assert(game.plr_bg_color == NULL);
        if (!rgbcolor_load(file, &game.plr_bg_color, "playercolors.background")) {
          ruleset_error(LOG_ERROR, "No background player color defined! (%s)",
                        secfile_error());
          ok = FALSE;
        }
      }
    }
  }

  if (ok) {
    /* section: teams */
    svec = secfile_lookup_str_vec(file, &teams, "teams.names");
    if (team_slot_count() < teams) {
      teams = team_slot_count();
    }
    game.server.ruledit.named_teams = teams;
    for (i = 0; i < teams; i++) {
      team_slot_set_defined_name(team_slot_by_number(i), svec[i]);
    }
    free(svec);

    sec = secfile_sections_by_name_prefix(file, DISASTER_SECTION_PREFIX);
    nval = (NULL != sec ? section_list_size(sec) : 0);
    if (nval > MAX_DISASTER_TYPES) {
      int num = nval; /* No "size_t" to printf */

      ruleset_error(LOG_ERROR, "\"%s\": Too many disaster types (%d, max %d)",
                    filename, num, MAX_DISASTER_TYPES);
      section_list_destroy(sec);
      ok = FALSE;
    } else {
      game.control.num_disaster_types = nval;
    }
  }

  if (ok) {
    disaster_type_iterate(pdis) {
      int id = disaster_index(pdis);
      int j;
      size_t eff_count;
      struct requirement_vector *reqs;
      const char *sec_name = section_name(section_list_get(sec, id));

      if (!ruleset_load_names(&pdis->name, NULL, file, sec_name)) {
        ruleset_error(LOG_ERROR, "\"%s\": Cannot load disaster names",
                      filename);
        ok = FALSE;
        break;
      }

      reqs = lookup_req_list(file, compat, sec_name, "reqs", disaster_rule_name(pdis));
      if (reqs == NULL) {
        ok = FALSE;
        break;
      }
      requirement_vector_copy(&pdis->reqs, reqs);

      pdis->frequency = secfile_lookup_int_default(file, GAME_DEFAULT_DISASTER_FREQ,
                                                   "%s.frequency", sec_name);

      svec = secfile_lookup_str_vec(file, &eff_count, "%s.effects", sec_name);

      BV_CLR_ALL(pdis->effects);
      for (j = 0; j < eff_count; j++) {
        const char *dsval = svec[j];
        enum disaster_effect_id effect;

        effect = disaster_effect_id_by_name(dsval, fc_strcasecmp);

        if (!disaster_effect_id_is_valid(effect)) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" disaster \"%s\": unknown effect \"%s\".",
                        filename,
                        disaster_rule_name(pdis),
                        dsval);
          ok = FALSE;
          break;
        } else {
          BV_SET(pdis->effects, effect);
        }
      }

      free(svec);

      if (!ok) {
        break;
      }
    } disaster_type_iterate_end;
    section_list_destroy(sec);
  }

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, ACHIEVEMENT_SECTION_PREFIX);

    achievements_iterate(pach) {
      int id = achievement_index(pach);
      const char *sec_name = section_name(section_list_get(sec, id));
      const char *typename;
      const char *msg;

      typename = secfile_lookup_str_default(file, NULL, "%s.type", sec_name);

      pach->type = achievement_type_by_name(typename, fc_strcasecmp);
      if (!achievement_type_is_valid(pach->type)) {
        ruleset_error(LOG_ERROR, "Achievement has unknown type \"%s\".",
                      typename != NULL ? typename : "(NULL)");
        ok = FALSE;
      }

      if (ok) {
        pach->unique = secfile_lookup_bool_default(file, GAME_DEFAULT_ACH_UNIQUE,
                                                   "%s.unique", sec_name);

        pach->value = secfile_lookup_int_default(file, GAME_DEFAULT_ACH_VALUE,
                                                 "%s.value", sec_name);
        pach->culture = secfile_lookup_int_default(file, 0,
                                                   "%s.culture", sec_name);

        msg = secfile_lookup_str_default(file, NULL, "%s.first_msg", sec_name);
        if (msg == NULL) {
          ruleset_error(LOG_ERROR, "Achievement %s has no first msg!", sec_name);
          ok = FALSE;
        } else {
          pach->first_msg = fc_strdup(msg);
        }
      }

      if (ok) {
        msg = secfile_lookup_str_default(file, NULL, "%s.cons_msg", sec_name);
        if (msg == NULL) {
          if (!pach->unique) {
            ruleset_error(LOG_ERROR, "Achievement %s has no msg for consecutive gainers!", sec_name);
            ok = FALSE;
          }
        } else {
          pach->cons_msg = fc_strdup(msg);
        }
      }

      if (!ok) {
        break;
      }
    } achievements_iterate_end;
    section_list_destroy(sec);
  }

  if (ok) {
    for (i = 0; (name = secfile_lookup_str_default(file, NULL,
                                                   "trade.settings%d.type",
                                                   i)); i++) {
      enum trade_route_type type = trade_route_type_by_name(name);

      if (type == TRT_LAST) {
        ruleset_error(LOG_ERROR,
                      "\"%s\" unknown trade route type \"%s\".",
                      filename, name);
        ok = FALSE;
      } else {
        struct trade_route_settings *set = trade_route_settings_by_type(type);
        const char *cancelling;
        const char *bonus;

        set->trade_pct = secfile_lookup_int_default(file, 100,
                                                    "trade.settings%d.pct", i);
        cancelling = secfile_lookup_str_default(file, "Active",
                                                "trade.settings%d.cancelling", i);
        set->cancelling = traderoute_cancelling_type_by_name(cancelling);
        if (set->cancelling == TRI_LAST) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" unknown traderoute cancelling type \"%s\".",
                        filename, cancelling);
          ok = FALSE;
        }

        bonus = secfile_lookup_str_default(file, "None", "trade.settings%d.bonus", i);

        set->bonus_type = traderoute_bonus_type_by_name(bonus, fc_strcasecmp);

        if (!traderoute_bonus_type_is_valid(set->bonus_type)) {
          ruleset_error(LOG_ERROR,
                        "\"%s\" unknown traderoute bonus type \"%s\".",
                        filename, bonus);
          ok = FALSE;
        }
      }
    }
  }

  if (ok) {
    const char *str = secfile_lookup_str_default(file, "Leaving", "trade.goods_selection");

    game.info.goods_selection = goods_selection_method_by_name(str, fc_strcasecmp);

    if (!goods_selection_method_is_valid(game.info.goods_selection)) {
      ruleset_error(LOG_ERROR,
                    "\"%s\" goods selection method \"%s\" unknown.",
                    filename, str);
      ok = FALSE;
    }
  }

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, GOODS_SECTION_PREFIX);

    goods_type_iterate(pgood) {
      int id = goods_index(pgood);
      const char *sec_name = section_name(section_list_get(sec, id));
      struct requirement_vector *reqs;
      const char **slist;
      int j;

      reqs = lookup_req_list(file, compat, sec_name, "reqs", goods_rule_name(pgood));
      if (reqs == NULL) {
        ok = FALSE;
        break;
      }
      requirement_vector_copy(&pgood->reqs, reqs);

      pgood->from_pct = secfile_lookup_int_default(file, 100,
                                                   "%s.from_pct", sec_name);
      pgood->to_pct = secfile_lookup_int_default(file, 100,
                                                 "%s.to_pct", sec_name);
      pgood->onetime_pct = secfile_lookup_int_default(file, 100,
                                                      "%s.onetime_pct", sec_name);

      slist = secfile_lookup_str_vec(file, &nval, "%s.flags", sec_name);
      BV_CLR_ALL(pgood->flags);
      for (j = 0; j < nval; j++) {
        enum goods_flag_id flag;

        sval = slist[j];
        flag = goods_flag_id_by_name(sval, fc_strcasecmp);
        if (!goods_flag_id_is_valid(flag)) {
          ruleset_error(LOG_ERROR, "\"%s\" good \"%s\": unknown flag \"%s\".",
                        filename,
                        goods_rule_name(pgood),
                        sval);
          ok = FALSE;
          break;
        } else {
          BV_SET(pgood->flags, flag);
        }
      }
      free(slist);

      pgood->helptext = lookup_strvec(file, sec_name, "helptext");
    } goods_type_iterate_end;
    section_list_destroy(sec);
  }

  if (ok) {
    sec = secfile_sections_by_name_prefix(file, CLAUSE_SECTION_PREFIX);

    if (sec != NULL) {
      int num = section_list_size(sec);

      for (i = 0; i < num; i++) {
        const char *sec_name = section_name(section_list_get(sec, i));
        const char *clause_name = secfile_lookup_str_default(file, NULL,
                                                             "%s.type", sec_name);
        enum clause_type type = clause_type_by_name(clause_name, fc_strcasecmp);
        struct clause_info *info;

        if (!clause_type_is_valid(type)) {
          ruleset_error(LOG_ERROR, "\"%s\" unknown clause type \"%s\".",
                        filename, clause_name);
          ok = FALSE;
          break;
        }

        info = clause_info_get(type);

        if (info->enabled) {
          ruleset_error(LOG_ERROR, "\"%s\" dublicate clause type \"%s\" definition.",
                        filename, clause_name);
          ok = FALSE;
          break;
        }

        info->enabled = TRUE;
      }
    }
  }

  /* secfile_check_unused() is not here, but only after also settings section
   * has been loaded. */

  return ok;
}

/**********************************************************************//**
  Send the units ruleset information (all individual unit classes) to the
  specified connections.
**************************************************************************/
static void send_ruleset_unit_classes(struct conn_list *dest)
{
  struct packet_ruleset_unit_class packet;
  struct packet_ruleset_unit_class_flag fpacket;
  int i;

  for (i = 0; i < MAX_NUM_USER_UCLASS_FLAGS; i++) {
    const char *flagname;
    const char *helptxt;

    fpacket.id = i + UCF_USER_FLAG_1;

    flagname = unit_class_flag_id_name(i + UCF_USER_FLAG_1);
    if (flagname == NULL) {
      fpacket.name[0] = '\0';
    } else {
      sz_strlcpy(fpacket.name, flagname);
    }

    helptxt = unit_class_flag_helptxt(i + UCF_USER_FLAG_1);
    if (helptxt == NULL) {
      fpacket.helptxt[0] = '\0';
    } else {
      sz_strlcpy(fpacket.helptxt, helptxt);
    }

    lsend_packet_ruleset_unit_class_flag(dest, &fpacket);
  }

  unit_class_iterate(c) {
    packet.id = uclass_number(c);
    sz_strlcpy(packet.name, untranslated_name(&c->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&c->name));
    packet.min_speed = c->min_speed;
    packet.hp_loss_pct = c->hp_loss_pct;
    packet.hut_behavior = c->hut_behavior;
    packet.non_native_def_pct = c->non_native_def_pct;
    packet.flags = c->flags;

    PACKET_STRVEC_COMPUTE(packet.helptext, c->helptext);

    lsend_packet_ruleset_unit_class(dest, &packet);
  } unit_class_iterate_end;
}

/**********************************************************************//**
  Send the units ruleset information (all individual units) to the
  specified connections.
**************************************************************************/
static void send_ruleset_units(struct conn_list *dest)
{
  struct packet_ruleset_unit packet;
#ifdef FREECIV_WEB
  struct packet_web_ruleset_unit_addition web_packet;
#endif /* FREECIV_WEB */
  struct packet_ruleset_unit_flag fpacket;
  int i;

  for (i = 0; i < MAX_NUM_USER_UNIT_FLAGS; i++) {
    const char *flagname;
    const char *helptxt;

    fpacket.id = i + UTYF_USER_FLAG_1;

    flagname = unit_type_flag_id_name(i + UTYF_USER_FLAG_1);
    if (flagname == NULL) {
      fpacket.name[0] = '\0';
    } else {
      sz_strlcpy(fpacket.name, flagname);
    }

    helptxt = unit_type_flag_helptxt(i + UTYF_USER_FLAG_1);
    if (helptxt == NULL) {
      fpacket.helptxt[0] = '\0';
    } else {
      sz_strlcpy(fpacket.helptxt, helptxt);
    }

    lsend_packet_ruleset_unit_flag(dest, &fpacket);
  }

  unit_type_iterate(u) {
    packet.id = utype_number(u);
    sz_strlcpy(packet.name, untranslated_name(&u->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&u->name));
    sz_strlcpy(packet.sound_move, u->sound_move);
    sz_strlcpy(packet.sound_move_alt, u->sound_move_alt);
    sz_strlcpy(packet.sound_fight, u->sound_fight);
    sz_strlcpy(packet.sound_fight_alt, u->sound_fight_alt);
    sz_strlcpy(packet.graphic_str, u->graphic_str);
    sz_strlcpy(packet.graphic_alt, u->graphic_alt);
    packet.unit_class_id = uclass_number(utype_class(u));
    packet.build_cost = u->build_cost;
    packet.pop_cost = u->pop_cost;
    packet.attack_strength = u->attack_strength;
    packet.defense_strength = u->defense_strength;
    packet.move_rate = u->move_rate;
    packet.tech_requirement = u->require_advance
                              ? advance_number(u->require_advance)
                              : advance_count();
    packet.impr_requirement = u->need_improvement
                              ? improvement_number(u->need_improvement)
                              : improvement_count();
    packet.gov_requirement = u->need_government
                             ? government_number(u->need_government)
                             : government_count();
    packet.vision_radius_sq = u->vision_radius_sq;
    packet.transport_capacity = u->transport_capacity;
    packet.hp = u->hp;
    packet.firepower = u->firepower;
    packet.obsoleted_by = u->obsoleted_by
                          ? utype_number(u->obsoleted_by)
                          : utype_count();
    packet.converted_to = u->converted_to
                          ? utype_number(u->converted_to)
                          : utype_count();
    packet.convert_time = u->convert_time;
    packet.fuel = u->fuel;
    packet.flags = u->flags;
    packet.roles = u->roles;
    packet.happy_cost = u->happy_cost;
    output_type_iterate(o) {
      packet.upkeep[o] = u->upkeep[o];
    } output_type_iterate_end;
    packet.paratroopers_range = u->paratroopers_range;
    packet.paratroopers_mr_req = u->paratroopers_mr_req;
    packet.paratroopers_mr_sub = u->paratroopers_mr_sub;
    packet.bombard_rate = u->bombard_rate;
    packet.city_size = u->city_size;
    packet.city_slots = u->city_slots;
    packet.cargo = u->cargo;
    packet.targets = u->targets;
    packet.embarks = u->embarks;
    packet.disembarks = u->disembarks;
    packet.vlayer = u->vlayer;

    if (u->veteran == NULL) {
      /* Use the default veteran system. */
      packet.veteran_levels = 0;
    } else {
      /* Per unit veteran system definition. */
      packet.veteran_levels = utype_veteran_levels(u);

      for (i = 0; i < packet.veteran_levels; i++) {
        const struct veteran_level *vlevel = utype_veteran_level(u, i);

        sz_strlcpy(packet.veteran_name[i], untranslated_name(&vlevel->name));
        packet.power_fact[i] = vlevel->power_fact;
        packet.move_bonus[i] = vlevel->move_bonus;
        packet.raise_chance[i] = vlevel->raise_chance;
        packet.work_raise_chance[i] = vlevel->work_raise_chance;
      }
    }
    PACKET_STRVEC_COMPUTE(packet.helptext, u->helptext);

    packet.worker = u->adv.worker;

#ifdef FREECIV_WEB
    web_packet.id = utype_number(u);

    BV_CLR_ALL(web_packet.utype_actions);

    action_iterate(act) {
      if (utype_can_do_action(u, act)) {
        BV_SET(web_packet.utype_actions, act);
      }
    } action_iterate_end;
#endif /* FREECIV_WEB */

    lsend_packet_ruleset_unit(dest, &packet);
    web_lsend_packet(ruleset_unit_addition, dest, &web_packet);

    combat_bonus_list_iterate(u->bonuses, pbonus) {
      struct packet_ruleset_unit_bonus bonuspacket;

      bonuspacket.unit  = packet.id;
      bonuspacket.flag  = pbonus->flag;
      bonuspacket.type  = pbonus->type;
      bonuspacket.value = pbonus->value;
      bonuspacket.quiet = pbonus->quiet;

      lsend_packet_ruleset_unit_bonus(dest, &bonuspacket);
    } combat_bonus_list_iterate_end;
  } unit_type_iterate_end;
}

/**********************************************************************//**
  Send the specialists ruleset information (all individual specialist
  types) to the specified connections.
**************************************************************************/
static void send_ruleset_specialists(struct conn_list *dest)
{
  struct packet_ruleset_specialist packet;

  specialist_type_iterate(spec_id) {
    struct specialist *s = specialist_by_number(spec_id);
    int j;

    packet.id = spec_id;
    sz_strlcpy(packet.plural_name, untranslated_name(&s->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&s->name));
    sz_strlcpy(packet.short_name, untranslated_name(&s->abbreviation));
    sz_strlcpy(packet.graphic_str, s->graphic_str);
    sz_strlcpy(packet.graphic_alt, s->graphic_alt);
    j = 0;
    requirement_vector_iterate(&s->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;

    PACKET_STRVEC_COMPUTE(packet.helptext, s->helptext);

    lsend_packet_ruleset_specialist(dest, &packet);
  } specialist_type_iterate_end;
}
/**********************************************************************//**
  Send the techs class information to the specified connections.
**************************************************************************/
static void send_ruleset_tech_classes(struct conn_list *dest)
{
  struct packet_ruleset_tech_class packet;

  tech_class_iterate(ptclass) {
    packet.id = ptclass->idx;
    sz_strlcpy(packet.name, untranslated_name(&ptclass->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&ptclass->name));
    packet.cost_pct = ptclass->cost_pct;

    lsend_packet_ruleset_tech_class(dest, &packet);
  } tech_class_iterate_end;
}

/**********************************************************************//**
  Send the techs ruleset information (all individual advances) to the
  specified connections.
**************************************************************************/
static void send_ruleset_techs(struct conn_list *dest)
{
  struct packet_ruleset_tech packet;
  struct packet_ruleset_tech_flag fpacket;
  int i;

  for (i = 0; i < MAX_NUM_USER_TECH_FLAGS; i++) {
    const char *flagname;
    const char *helptxt;

    fpacket.id = i + TECH_USER_1;

    flagname = tech_flag_id_name_cb(i + TECH_USER_1);
    if (flagname == NULL) {
      fpacket.name[0] = '\0';
    } else {
      sz_strlcpy(fpacket.name, flagname);
    }

    helptxt = tech_flag_helptxt(i + TECH_USER_1);
    if (helptxt == NULL) {
      fpacket.helptxt[0] = '\0';
    } else {
      sz_strlcpy(fpacket.helptxt, helptxt);
    }

    lsend_packet_ruleset_tech_flag(dest, &fpacket);
  }

  advance_iterate(A_FIRST, a) {
    packet.id = advance_number(a);
    packet.removed = !valid_advance(a);
    if (a->tclass == NULL) {
      packet.tclass = 0;
    } else {
      packet.tclass = a->tclass->idx;
    }
    sz_strlcpy(packet.name, untranslated_name(&a->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&a->name));
    sz_strlcpy(packet.graphic_str, a->graphic_str);
    sz_strlcpy(packet.graphic_alt, a->graphic_alt);

    /* Current size of the packet's research_reqs requirement vector. */
    i = 0;

    /* The requirements req1 and req2 are needed to research a tech. Send
     * them in the research_reqs requirement vector. Range is set to player
     * since pooled research is configurable. */

    if ((a->require[AR_ONE] != A_NEVER)
        && advance_number(a->require[AR_ONE]) > A_NONE) {
      packet.research_reqs[i++]
          = req_from_values(VUT_ADVANCE, REQ_RANGE_PLAYER,
                            FALSE, TRUE, FALSE,
                            advance_number(a->require[AR_ONE]));
    }

    if ((a->require[AR_TWO] != A_NEVER)
        && advance_number(a->require[AR_TWO]) > A_NONE) {
      packet.research_reqs[i++]
          = req_from_values(VUT_ADVANCE, REQ_RANGE_PLAYER,
                            FALSE, TRUE, FALSE,
                            advance_number(a->require[AR_TWO]));;
    }

    /* The requirements of the tech's research_reqs also goes in the
     * packet's research_reqs requirement vector. */
    requirement_vector_iterate(&a->research_reqs, req) {
      packet.research_reqs[i++] = *req;
    } requirement_vector_iterate_end;

    /* The packet's research_reqs should contain req1, req2 and the
     * requirements of the tech's research_reqs. */
    packet.research_reqs_count = i;

    packet.root_req = a->require[AR_ROOT]
                      ? advance_number(a->require[AR_ROOT])
                      : advance_count();

    packet.flags = a->flags;
    packet.cost = a->cost;
    packet.num_reqs = a->num_reqs;
    PACKET_STRVEC_COMPUTE(packet.helptext, a->helptext);

    lsend_packet_ruleset_tech(dest, &packet);
  } advance_iterate_end;
}

/**********************************************************************//**
  Send the buildings ruleset information (all individual improvements and
  wonders) to the specified connections.
**************************************************************************/
static void send_ruleset_buildings(struct conn_list *dest)
{
  improvement_iterate(b) {
    struct packet_ruleset_building packet;
    int j;

    packet.id = improvement_number(b);
    packet.genus = b->genus;
    sz_strlcpy(packet.name, untranslated_name(&b->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&b->name));
    sz_strlcpy(packet.graphic_str, b->graphic_str);
    sz_strlcpy(packet.graphic_alt, b->graphic_alt);
    j = 0;
    requirement_vector_iterate(&b->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;
    j = 0;
    requirement_vector_iterate(&b->obsolete_by, pobs) {
      packet.obs_reqs[j++] = *pobs;
    } requirement_vector_iterate_end;
    packet.obs_count = j;
    packet.build_cost = b->build_cost;
    packet.upkeep = b->upkeep;
    packet.sabotage = b->sabotage;
    packet.flags = b->flags;
    sz_strlcpy(packet.soundtag, b->soundtag);
    sz_strlcpy(packet.soundtag_alt, b->soundtag_alt);
    PACKET_STRVEC_COMPUTE(packet.helptext, b->helptext);

    lsend_packet_ruleset_building(dest, &packet);
  } improvement_iterate_end;
}

/**********************************************************************//**
  Send the terrain ruleset information (terrain_control, and the individual
  terrain types) to the specified connections.
**************************************************************************/
static void send_ruleset_terrain(struct conn_list *dest)
{
  struct packet_ruleset_terrain packet;
  struct packet_ruleset_terrain_flag fpacket;
  int i;

  lsend_packet_ruleset_terrain_control(dest, &terrain_control);

  for (i = 0; i < MAX_NUM_USER_TER_FLAGS; i++) {
    const char *flagname;
    const char *helptxt;

    fpacket.id = i + TER_USER_1;

    flagname = terrain_flag_id_name_cb(i + TER_USER_1);
    if (flagname == NULL) {
      fpacket.name[0] = '\0';
    } else {
      sz_strlcpy(fpacket.name, flagname);
    }

    helptxt = terrain_flag_helptxt(i + TER_USER_1);
    if (helptxt == NULL) {
      fpacket.helptxt[0] = '\0';
    } else {
      sz_strlcpy(fpacket.helptxt, helptxt);
    }

    lsend_packet_ruleset_terrain_flag(dest, &fpacket);
  }

  terrain_type_iterate(pterrain) {
    struct extra_type **r;

    packet.id = terrain_number(pterrain);
    packet.tclass = pterrain->tclass;
    packet.native_to = pterrain->native_to;

    sz_strlcpy(packet.name, untranslated_name(&pterrain->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&pterrain->name));
    sz_strlcpy(packet.graphic_str, pterrain->graphic_str);
    sz_strlcpy(packet.graphic_alt, pterrain->graphic_alt);

    packet.movement_cost = pterrain->movement_cost;
    packet.defense_bonus = pterrain->defense_bonus;

    output_type_iterate(o) {
      packet.output[o] = pterrain->output[o];
    } output_type_iterate_end;

    packet.num_resources = 0;
    for (r = pterrain->resources; *r; r++) {
      packet.resources[packet.num_resources++] = extra_number(*r);
    }

    output_type_iterate(o) {
      packet.road_output_incr_pct[o] = pterrain->road_output_incr_pct[o];
    } output_type_iterate_end;

    packet.base_time = pterrain->base_time;
    packet.road_time = pterrain->road_time;

    packet.irrigation_result = (pterrain->irrigation_result
				? terrain_number(pterrain->irrigation_result)
				: terrain_count());
    packet.irrigation_food_incr = pterrain->irrigation_food_incr;
    packet.irrigation_time = pterrain->irrigation_time;

    packet.mining_result = (pterrain->mining_result
			    ? terrain_number(pterrain->mining_result)
			    : terrain_count());
    packet.mining_shield_incr = pterrain->mining_shield_incr;
    packet.mining_time = pterrain->mining_time;

    packet.animal = (pterrain->animal == NULL ? -1 : utype_number(pterrain->animal));
    packet.transform_result = (pterrain->transform_result
			       ? terrain_number(pterrain->transform_result)
			       : terrain_count());
    packet.pillage_time = pterrain->pillage_time;
    packet.transform_time = pterrain->transform_time;
    packet.clean_pollution_time = pterrain->clean_pollution_time;
    packet.clean_fallout_time = pterrain->clean_fallout_time;

    packet.flags = pterrain->flags;

    packet.color_red = pterrain->rgb->r;
    packet.color_green = pterrain->rgb->g;
    packet.color_blue = pterrain->rgb->b;

    PACKET_STRVEC_COMPUTE(packet.helptext, pterrain->helptext);

    lsend_packet_ruleset_terrain(dest, &packet);
  } terrain_type_iterate_end;
}

/**********************************************************************//**
  Send the resource ruleset information to the specified connections.
**************************************************************************/
static void send_ruleset_resources(struct conn_list *dest)
{
  struct packet_ruleset_resource packet;

  extra_type_by_cause_iterate(EC_RESOURCE, presource) {
    packet.id = extra_index(presource);

    output_type_iterate(o) {
      packet.output[o] = presource->data.resource->output[o];
    } output_type_iterate_end;

    lsend_packet_ruleset_resource(dest, &packet);
  } extra_type_by_cause_iterate_end;
}

/**********************************************************************//**
  Send the extra ruleset information (all individual extra types) to the
  specified connections.
**************************************************************************/
static void send_ruleset_extras(struct conn_list *dest)
{
  struct packet_ruleset_extra packet;
  struct packet_ruleset_extra_flag fpacket;
  int i;

  for (i = 0; i < MAX_NUM_USER_EXTRA_FLAGS; i++) {
    const char *flagname;
    const char *helptxt;

    fpacket.id = i + EF_USER_FLAG_1;

    flagname = extra_flag_id_name(i + EF_USER_FLAG_1);
    if (flagname == NULL) {
      fpacket.name[0] = '\0';
    } else {
      sz_strlcpy(fpacket.name, flagname);
    }

    helptxt = extra_flag_helptxt(i + EF_USER_FLAG_1);
    if (helptxt == NULL) {
      fpacket.helptxt[0] = '\0';
    } else {
      sz_strlcpy(fpacket.helptxt, helptxt);
    }

    lsend_packet_ruleset_extra_flag(dest, &fpacket);
  }

  extra_type_iterate(e) {
    int j;

    packet.id = extra_number(e);
    sz_strlcpy(packet.name, untranslated_name(&e->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&e->name));

    packet.category = e->category;

    BV_CLR_ALL(packet.causes);
    for (j = 0; j < EC_COUNT; j++) {
      if (is_extra_caused_by(e, j)) {
        BV_SET(packet.causes, j);
      }
    }

    BV_CLR_ALL(packet.rmcauses);
    for (j = 0; j < ERM_COUNT; j++) {
      if (is_extra_removed_by(e, j)) {
        BV_SET(packet.rmcauses, j);
      }
    }

    sz_strlcpy(packet.activity_gfx, e->activity_gfx);
    sz_strlcpy(packet.act_gfx_alt, e->act_gfx_alt);
    sz_strlcpy(packet.act_gfx_alt2, e->act_gfx_alt2);
    sz_strlcpy(packet.rmact_gfx, e->rmact_gfx);
    sz_strlcpy(packet.rmact_gfx_alt, e->rmact_gfx_alt);
    sz_strlcpy(packet.graphic_str, e->graphic_str);
    sz_strlcpy(packet.graphic_alt, e->graphic_alt);

    j = 0;
    requirement_vector_iterate(&e->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;

    j = 0;
    requirement_vector_iterate(&e->rmreqs, preq) {
      packet.rmreqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.rmreqs_count = j;

    packet.appearance_chance = e->appearance_chance;
    j = 0;
    requirement_vector_iterate(&e->appearance_reqs, preq) {
      packet.appearance_reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.appearance_reqs_count = j;

    packet.disappearance_chance = e->disappearance_chance;
    j = 0;
    requirement_vector_iterate(&e->disappearance_reqs, preq) {
      packet.disappearance_reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.disappearance_reqs_count = j;

    packet.visibility_req = e->visibility_req;
    packet.buildable = e->buildable;
    packet.generated = e->generated;
    packet.build_time = e->build_time;
    packet.build_time_factor = e->build_time_factor;
    packet.removal_time = e->removal_time;
    packet.removal_time_factor = e->removal_time_factor;
    packet.defense_bonus = e->defense_bonus;
    packet.eus = e->eus;

    packet.native_to = e->native_to;

    packet.flags = e->flags;
    packet.hidden_by = e->hidden_by;
    packet.bridged_over = e->bridged_over;
    packet.conflicts = e->conflicts;

    PACKET_STRVEC_COMPUTE(packet.helptext, e->helptext);

    lsend_packet_ruleset_extra(dest, &packet);
  } extra_type_iterate_end;
}

/**********************************************************************//**
  Send the base ruleset information (all individual base types) to the
  specified connections.
**************************************************************************/
static void send_ruleset_bases(struct conn_list *dest)
{
  extra_type_by_cause_iterate(EC_BASE, pextra) {
    struct base_type *b = extra_base_get(pextra);
    struct packet_ruleset_base packet;

    packet.id = base_number(b);

    packet.gui_type = b->gui_type;
    packet.border_sq = b->border_sq;
    packet.vision_main_sq = b->vision_main_sq;
    packet.vision_invis_sq = b->vision_invis_sq;
    packet.vision_subs_sq = b->vision_subs_sq;

    packet.flags = b->flags;

    lsend_packet_ruleset_base(dest, &packet);
  } extra_type_by_cause_iterate_end;
}

/**********************************************************************//**
  Send the road ruleset information (all individual road types) to the
  specified connections.
**************************************************************************/
static void send_ruleset_roads(struct conn_list *dest)
{
  struct packet_ruleset_road packet;

  extra_type_by_cause_iterate(EC_ROAD, pextra) {
    struct road_type *r = extra_road_get(pextra);
    int j;

    packet.id = road_number(r);

    j = 0;
    requirement_vector_iterate(&r->first_reqs, preq) {
      packet.first_reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.first_reqs_count = j;

    packet.move_cost = r->move_cost;
    packet.move_mode = r->move_mode;

    output_type_iterate(o) {
      packet.tile_incr_const[o] = r->tile_incr_const[o];
      packet.tile_incr[o] = r->tile_incr[o];
      packet.tile_bonus[o] = r->tile_bonus[o];
    } output_type_iterate_end;

    packet.compat = r->compat;

    packet.integrates = r->integrates;
    packet.flags = r->flags;

    lsend_packet_ruleset_road(dest, &packet);
  } extra_type_by_cause_iterate_end;
}

/**********************************************************************//**
  Send the goods ruleset information (all individual goods types) to the
  specified connections.
**************************************************************************/
static void send_ruleset_goods(struct conn_list *dest)
{
  struct packet_ruleset_goods packet;

  goods_type_iterate(g) {
    int j;

    packet.id = goods_number(g);
    sz_strlcpy(packet.name, untranslated_name(&g->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&g->name));

    j = 0;
    requirement_vector_iterate(&g->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;

    packet.from_pct = g->from_pct;
    packet.to_pct = g->to_pct;
    packet.onetime_pct = g->onetime_pct;
    packet.flags = g->flags;

    PACKET_STRVEC_COMPUTE(packet.helptext, g->helptext);

    lsend_packet_ruleset_goods(dest, &packet);
  } goods_type_iterate_end;
}

/**********************************************************************//**
  Send the disaster ruleset information (all individual disaster types) to the
  specified connections.
**************************************************************************/
static void send_ruleset_disasters(struct conn_list *dest)
{
  struct packet_ruleset_disaster packet;

  disaster_type_iterate(d) {
    int j;

    packet.id = disaster_number(d);

    sz_strlcpy(packet.name, untranslated_name(&d->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&d->name));

    j = 0;
    requirement_vector_iterate(&d->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;

    packet.frequency = d->frequency;

    packet.effects = d->effects;

    lsend_packet_ruleset_disaster(dest, &packet);
  } disaster_type_iterate_end;
}

/**********************************************************************//**
  Send the achievement ruleset information (all individual achievement types)
  to the specified connections.
**************************************************************************/
static void send_ruleset_achievements(struct conn_list *dest)
{
  struct packet_ruleset_achievement packet;

  achievements_iterate(a) {
    packet.id = achievement_number(a);

    sz_strlcpy(packet.name, untranslated_name(&a->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&a->name));

    packet.type = a->type;
    packet.unique = a->unique;
    packet.value = a->value;

    lsend_packet_ruleset_achievement(dest, &packet);
  } achievements_iterate_end;
}

/**********************************************************************//**
  Send action ruleset information to the specified connections.
**************************************************************************/
static void send_ruleset_actions(struct conn_list *dest)
{
  struct packet_ruleset_action packet;

  action_iterate(act) {
    packet.id = act;
    sz_strlcpy(packet.ui_name, action_by_number(act)->ui_name);
    packet.quiet = action_by_number(act)->quiet;

    packet.act_kind = action_by_number(act)->actor_kind;
    packet.tgt_kind = action_by_number(act)->target_kind;

    packet.min_distance = action_by_number(act)->min_distance;
    packet.max_distance = action_by_number(act)->max_distance;
    packet.blocked_by = action_by_number(act)->blocked_by;

    lsend_packet_ruleset_action(dest, &packet);
  } action_iterate_end;
}

/**********************************************************************//**
  Send the action enabler ruleset information to the specified connections.
**************************************************************************/
static void send_ruleset_action_enablers(struct conn_list *dest)
{
  int counter;
  struct packet_ruleset_action_enabler packet;

  action_enablers_iterate(enabler) {
    packet.enabled_action = enabler->action;

    counter = 0;
    requirement_vector_iterate(&enabler->actor_reqs, req) {
      packet.actor_reqs[counter++] = *req;
    } requirement_vector_iterate_end;
    packet.actor_reqs_count = counter;

    counter = 0;
    requirement_vector_iterate(&enabler->target_reqs, req) {
      packet.target_reqs[counter++] = *req;
    } requirement_vector_iterate_end;
    packet.target_reqs_count = counter;

    lsend_packet_ruleset_action_enabler(dest, &packet);
  } action_enablers_iterate_end;
}

/**********************************************************************//**
  Send action auto performer ruleset information to the specified
  connections.
**************************************************************************/
static void send_ruleset_action_auto_performers(struct conn_list *dest)
{
  int counter;
  int id;
  struct packet_ruleset_action_auto packet;

  id = 0;
  action_auto_perf_iterate(aperf) {
    packet.id = id++;

    packet.cause = aperf->cause;

    counter = 0;
    requirement_vector_iterate(&aperf->reqs, req) {
      packet.reqs[counter++] = *req;
    } requirement_vector_iterate_end;
    packet.reqs_count = counter;

    for (counter = 0;
         /* Can't list more actions than all actions. */
         counter < NUM_ACTIONS
         /* ACTION_NONE terminates the list. */
         && aperf->alternatives[counter] != ACTION_NONE;
         counter++) {
      packet.alternatives[counter] = aperf->alternatives[counter];
    }
    packet.alternatives_count = counter;

    lsend_packet_ruleset_action_auto(dest, &packet);
  } action_auto_perf_iterate_end;
}

/**********************************************************************//**
  Send the trade route types ruleset information (all individual
  trade route types) to the specified connections.
**************************************************************************/
static void send_ruleset_trade_routes(struct conn_list *dest)
{
  struct packet_ruleset_trade packet;
  enum trade_route_type type;

  for (type = TRT_NATIONAL; type < TRT_LAST; type++) {
    struct trade_route_settings *set = trade_route_settings_by_type(type);

    packet.id = type;
    packet.trade_pct = set->trade_pct;
    packet.cancelling = set->cancelling;
    packet.bonus_type = set->bonus_type;

    lsend_packet_ruleset_trade(dest, &packet);
  }
}

/**********************************************************************//**
  Send the government ruleset information to the specified connections.
  One packet per government type, and for each type one per ruler title.
**************************************************************************/
static void send_ruleset_governments(struct conn_list *dest)
{
  struct packet_ruleset_government gov;
  struct packet_ruleset_government_ruler_title title;
  int j;

  governments_iterate(g) {
    /* send one packet_government */
    gov.id = government_number(g);

    j = 0;
    requirement_vector_iterate(&g->reqs, preq) {
      gov.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    gov.reqs_count = j;

    sz_strlcpy(gov.name, untranslated_name(&g->name));
    sz_strlcpy(gov.rule_name, rule_name_get(&g->name));
    sz_strlcpy(gov.graphic_str, g->graphic_str);
    sz_strlcpy(gov.graphic_alt, g->graphic_alt);
    PACKET_STRVEC_COMPUTE(gov.helptext, g->helptext);

    lsend_packet_ruleset_government(dest, &gov);

    /* Send one packet_government_ruler_title per ruler title. */
    ruler_titles_iterate(government_ruler_titles(g), pruler_title) {
      const struct nation_type *pnation = ruler_title_nation(pruler_title);

      title.gov = government_number(g);
      title.nation = pnation ? nation_number(pnation) : nation_count();
      sz_strlcpy(title.male_title,
                 ruler_title_male_untranslated_name(pruler_title));
      sz_strlcpy(title.female_title,
                 ruler_title_female_untranslated_name(pruler_title));
      lsend_packet_ruleset_government_ruler_title(dest, &title);
    } ruler_titles_iterate_end;
  } governments_iterate_end;
}

/**********************************************************************//**
  Send the nations ruleset information (info on each nation) to the
  specified connections.
**************************************************************************/
static void send_ruleset_nations(struct conn_list *dest)
{
  struct packet_ruleset_nation_sets sets_packet;
  struct packet_ruleset_nation_groups groups_packet;
  struct packet_ruleset_nation packet;
  int i;

  sets_packet.nsets = nation_set_count();
  i = 0;
  nation_sets_iterate(pset) {
    sz_strlcpy(sets_packet.names[i], nation_set_untranslated_name(pset));
    sz_strlcpy(sets_packet.rule_names[i], nation_set_rule_name(pset));
    sz_strlcpy(sets_packet.descriptions[i], nation_set_description(pset));
    i++;
  } nation_sets_iterate_end;
  lsend_packet_ruleset_nation_sets(dest, &sets_packet);

  groups_packet.ngroups = nation_group_count();
  i = 0;
  nation_groups_iterate(pgroup) {
    sz_strlcpy(groups_packet.groups[i],
               nation_group_untranslated_name(pgroup));
    groups_packet.hidden[i] = pgroup->hidden;
    i++;
  } nation_groups_iterate_end;
  lsend_packet_ruleset_nation_groups(dest, &groups_packet);

  nations_iterate(n) {
    packet.id = nation_number(n);
    if (n->translation_domain == NULL) {
      packet.translation_domain[0] = '\0';
    } else {
      sz_strlcpy(packet.translation_domain, n->translation_domain);
    }
    sz_strlcpy(packet.adjective, untranslated_name(&n->adjective));
    sz_strlcpy(packet.rule_name, rule_name_get(&n->adjective));
    sz_strlcpy(packet.noun_plural, untranslated_name(&n->noun_plural));
    sz_strlcpy(packet.graphic_str, n->flag_graphic_str);
    sz_strlcpy(packet.graphic_alt, n->flag_graphic_alt);

    i = 0;
    nation_leader_list_iterate(nation_leaders(n), pleader) {
      sz_strlcpy(packet.leader_name[i], nation_leader_name(pleader));
      packet.leader_is_male[i] = nation_leader_is_male(pleader);
      i++;
    } nation_leader_list_iterate_end;
    packet.leader_count = i;

    packet.style = style_number(n->style);
    packet.is_playable = n->is_playable;
    packet.barbarian_type = n->barb_type;

    sz_strlcpy(packet.legend, n->legend);

    i = 0;
    nation_set_list_iterate(n->sets, pset) {
      packet.sets[i++] = nation_set_number(pset);
    } nation_set_list_iterate_end;
    packet.nsets = i;

    i = 0;
    nation_group_list_iterate(n->groups, pgroup) {
      packet.groups[i++] = nation_group_number(pgroup);
    } nation_group_list_iterate_end;
    packet.ngroups = i;

    packet.init_government_id = n->init_government
      ? government_number(n->init_government) : government_count();
    fc_assert(ARRAY_SIZE(packet.init_techs) == ARRAY_SIZE(n->init_techs));
    for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
      if (n->init_techs[i] != A_LAST) {
        packet.init_techs[i] = n->init_techs[i];
      } else {
        break;
      }
    }
    packet.init_techs_count = i;
    fc_assert(ARRAY_SIZE(packet.init_units) == ARRAY_SIZE(n->init_units));
    for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
      const struct unit_type *t = n->init_units[i];
      if (t) {
        packet.init_units[i] = utype_number(t);
      } else {
        break;
      }
    }
    packet.init_units_count = i;
    fc_assert(ARRAY_SIZE(packet.init_buildings)
              == ARRAY_SIZE(n->init_buildings));
    for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
      if (n->init_buildings[i] != B_LAST) {
        /* Impr_type_id to int */
        packet.init_buildings[i] = n->init_buildings[i];
      } else {
        break;
      }
    }
    packet.init_buildings_count = i;

    lsend_packet_ruleset_nation(dest, &packet);
  } nations_iterate_end;

  /* Send initial values of is_pickable */
  send_nation_availability(dest, FALSE);
}

/**********************************************************************//**
  Send the nation style ruleset information (each style) to the specified
  connections.
**************************************************************************/
static void send_ruleset_styles(struct conn_list *dest)
{
  struct packet_ruleset_style packet;

  styles_iterate(s) {
    packet.id = style_index(s);
    sz_strlcpy(packet.name, untranslated_name(&s->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&s->name));

    lsend_packet_ruleset_style(dest, &packet);
  } styles_iterate_end;
}

/**********************************************************************//**
  Send the clause type ruleset information to the specified connections.
**************************************************************************/
static void send_ruleset_clauses(struct conn_list *dest)
{
  struct packet_ruleset_clause packet;
  int i;

  for (i = 0; i < CLAUSE_COUNT; i++) {
    struct clause_info *info = clause_info_get(i);

    packet.type = i;
    packet.enabled = info->enabled;

    lsend_packet_ruleset_clause(dest, &packet);
  }
}

/**********************************************************************//**
  Send the multiplier ruleset information to the specified
  connections.
**************************************************************************/
static void send_ruleset_multipliers(struct conn_list *dest)
{
  multipliers_iterate(pmul) {
    int j;
    struct packet_ruleset_multiplier packet;

    packet.id     = multiplier_number(pmul);
    packet.start  = pmul->start;
    packet.stop   = pmul->stop;
    packet.step   = pmul->step;
    packet.def    = pmul->def;
    packet.offset = pmul->offset;
    packet.factor = pmul->factor;

    sz_strlcpy(packet.name, untranslated_name(&pmul->name));
    sz_strlcpy(packet.rule_name, rule_name_get(&pmul->name));

    j = 0;
    requirement_vector_iterate(&pmul->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;

    PACKET_STRVEC_COMPUTE(packet.helptext, pmul->helptext);

    lsend_packet_ruleset_multiplier(dest, &packet);
  } multipliers_iterate_end;
}

/**********************************************************************//**
  Send the city-style ruleset information (each style) to the specified
  connections.
**************************************************************************/
static void send_ruleset_cities(struct conn_list *dest)
{
  struct packet_ruleset_city city_p;
  int k, j;

  for (k = 0; k < game.control.styles_count; k++) {
    city_p.style_id = k;

    j = 0;
    requirement_vector_iterate(&city_styles[k].reqs, preq) {
      city_p.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    city_p.reqs_count = j;

    sz_strlcpy(city_p.name, untranslated_name(&city_styles[k].name));
    sz_strlcpy(city_p.rule_name, rule_name_get(&city_styles[k].name));
    sz_strlcpy(city_p.graphic, city_styles[k].graphic);
    sz_strlcpy(city_p.graphic_alt, city_styles[k].graphic_alt);
    sz_strlcpy(city_p.citizens_graphic, city_styles[k].citizens_graphic);
    sz_strlcpy(city_p.citizens_graphic_alt,
               city_styles[k].citizens_graphic_alt);

    lsend_packet_ruleset_city(dest, &city_p);
  }
}

/**********************************************************************//**
  Send the music-style ruleset information (each style) to the specified
  connections.
**************************************************************************/
static void send_ruleset_musics(struct conn_list *dest)
{
  struct packet_ruleset_music packet;

  music_styles_iterate(pmus) {
    int j;

    packet.id = pmus->id;

    sz_strlcpy(packet.music_peaceful, pmus->music_peaceful);
    sz_strlcpy(packet.music_combat, pmus->music_combat);

    j = 0;
    requirement_vector_iterate(&(pmus->reqs), preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;

    lsend_packet_ruleset_music(dest, &packet);
  } music_styles_iterate_end;
}

/**********************************************************************//**
  Send information in packet_ruleset_game (miscellaneous rules) to the
  specified connections.
**************************************************************************/
static void send_ruleset_game(struct conn_list *dest)
{
  struct packet_ruleset_game misc_p;
  int i;

  fc_assert_ret(game.veteran != NULL);

  /* Per unit veteran system definition. */
  misc_p.veteran_levels = game.veteran->levels;

  for (i = 0; i < misc_p.veteran_levels; i++) {
    const struct veteran_level *vlevel = game.veteran->definitions + i;

    sz_strlcpy(misc_p.veteran_name[i], untranslated_name(&vlevel->name));
    misc_p.power_fact[i] = vlevel->power_fact;
    misc_p.move_bonus[i] = vlevel->move_bonus;
    misc_p.raise_chance[i] = vlevel->raise_chance;
    misc_p.work_raise_chance[i] = vlevel->work_raise_chance;
  }

  fc_assert(sizeof(misc_p.global_init_techs)
            == sizeof(game.rgame.global_init_techs));
  fc_assert(ARRAY_SIZE(misc_p.global_init_techs)
            == ARRAY_SIZE(game.rgame.global_init_techs));
  for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
    if (game.rgame.global_init_techs[i] != A_LAST) {
      misc_p.global_init_techs[i] = game.rgame.global_init_techs[i];
    } else {
      break;
    }
  }
  misc_p.global_init_techs_count = i;

  fc_assert(ARRAY_SIZE(misc_p.global_init_buildings)
            == ARRAY_SIZE(game.rgame.global_init_buildings));
  for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
    if (game.rgame.global_init_buildings[i] != B_LAST) {
      /* Impr_type_id to int */
      misc_p.global_init_buildings[i] =
          game.rgame.global_init_buildings[i];
    } else {
      break;
    }
  }
  misc_p.global_init_buildings_count = i;

  misc_p.default_specialist = DEFAULT_SPECIALIST;

  fc_assert_ret(game.plr_bg_color != NULL);

  misc_p.background_red = game.plr_bg_color->r;
  misc_p.background_green = game.plr_bg_color->g;
  misc_p.background_blue = game.plr_bg_color->b;

  lsend_packet_ruleset_game(dest, &misc_p);
}

/**********************************************************************//**
  Send all team names defined in the ruleset file(s) to the
  specified connections.
**************************************************************************/
static void send_ruleset_team_names(struct conn_list *dest)
{
  struct packet_team_name_info team_name_info_p;

  team_slots_iterate(tslot) {
    const char *name = team_slot_defined_name(tslot);

    if (NULL == name) {
      /* End of defined names. */
      break;
    }

    team_name_info_p.team_id = team_slot_index(tslot);
    sz_strlcpy(team_name_info_p.team_name, name);

    lsend_packet_team_name_info(dest, &team_name_info_p);
  } team_slots_iterate_end;
}

/**********************************************************************//**
  Make it clear to everyone that requested ruleset has not been loaded.
**************************************************************************/
static void notify_ruleset_fallback(const char *msg)
{
  notify_conn(NULL, NULL, E_LOG_FATAL, ftc_warning, "%s", msg);
}

/**********************************************************************//**
  Loads the rulesets.
**************************************************************************/
bool load_rulesets(const char *restore, bool compat_mode,
                   rs_conversion_logger logger,
                   bool act, bool buffer_script)
{
  if (load_rulesetdir(game.server.rulesetdir, compat_mode, logger,
                      act, buffer_script)) {
    return TRUE;
  }

  /* Fallback to previous one. */
  if (restore != NULL) {
    if (load_rulesetdir(restore, compat_mode, logger, act, buffer_script)) {
      sz_strlcpy(game.server.rulesetdir, restore);

      notify_ruleset_fallback(_("Ruleset couldn't be loaded. Keeping previous one."));

      /* We're in sane state as restoring previous ruleset succeeded,
       * but return failure to indicate that this is not what caller
       * wanted. */
      return FALSE;
    }
  }

  /* Fallback to default one, but not if that's what we tried already */
  if (strcmp(GAME_DEFAULT_RULESETDIR, game.server.rulesetdir)
      && (restore == NULL || strcmp(GAME_DEFAULT_RULESETDIR, restore))) {
    if (load_rulesetdir(GAME_DEFAULT_RULESETDIR, FALSE, NULL, act, buffer_script)) {
      /* We're in sane state as fallback ruleset loading succeeded,
       * but return failure to indicate that this is not what caller
       * wanted. */
      sz_strlcpy(game.server.rulesetdir, GAME_DEFAULT_RULESETDIR);

      notify_ruleset_fallback(_("Ruleset couldn't be loaded. Switching to default one."));

      return FALSE;
    }
  }

#ifdef FREECIV_WEB
  log_normal(_("Cannot load any ruleset. Freeciv-web ruleset is available from "
               "https://github.com/freeciv/freeciv-web"));
#endif /* FREECIV_WEB */

  /* Cannot load even default ruleset, we're in completely unusable state */
  exit(EXIT_FAILURE);
}

/**********************************************************************//**
  Destroy secfile. Handle NULL parameter gracefully.
**************************************************************************/
static void nullcheck_secfile_destroy(struct section_file *file)
{
  if (file != NULL) {
    secfile_destroy(file);
  }
}

/**********************************************************************//**
  Completely deinitialize ruleset system. Server is not in usable
  state after this.
**************************************************************************/
void rulesets_deinit(void)
{
  script_server_free();
  requirement_vector_free(&reqs_list);
}

/**********************************************************************//**
  Loads the rulesets from directory.
  This may be called more than once and it will free any stale data.
**************************************************************************/
static bool load_rulesetdir(const char *rsdir, bool compat_mode,
                            rs_conversion_logger logger,
                            bool act, bool buffer_script)
{
  struct section_file *techfile, *unitfile, *buildfile, *govfile, *terrfile;
  struct section_file *stylefile, *cityfile, *nationfile, *effectfile, *gamefile;
  bool ok = TRUE;
  struct rscompat_info compat_info;

  log_normal(_("Loading rulesets."));

  rscompat_init_info(&compat_info);
  compat_info.compat_mode = compat_mode;
  compat_info.log_cb = logger;

  game_ruleset_free();
  /* Reset the list of available player colors. */
  playercolor_free();
  playercolor_init();
  game_ruleset_init();

  if (script_buffer != NULL) {
    FC_FREE(script_buffer);
    script_buffer = NULL;
  }
  if (parser_buffer != NULL) {
    FC_FREE(parser_buffer);
    parser_buffer = NULL;
  }

  server.playable_nations = 0;

  techfile = openload_ruleset_file("techs", rsdir);
  buildfile = openload_ruleset_file("buildings", rsdir);
  govfile = openload_ruleset_file("governments", rsdir);
  unitfile = openload_ruleset_file("units", rsdir);
  terrfile = openload_ruleset_file("terrain", rsdir);
  stylefile = openload_ruleset_file("styles", rsdir);
  cityfile = openload_ruleset_file("cities", rsdir);
  nationfile = openload_ruleset_file("nations", rsdir);
  effectfile = openload_ruleset_file("effects", rsdir);
  gamefile = openload_ruleset_file("game", rsdir);
  game.server.luadata = openload_luadata_file(rsdir);

  if (techfile == NULL
      || buildfile  == NULL
      || govfile    == NULL
      || unitfile   == NULL
      || terrfile   == NULL
      || stylefile  == NULL
      || cityfile   == NULL
      || nationfile == NULL
      || effectfile == NULL
      || gamefile == NULL) {
    ok = FALSE;
  }

  if (ok) {
    ok = load_game_names(gamefile, &compat_info)
      && load_tech_names(techfile, &compat_info)
      && load_building_names(buildfile, &compat_info)
      && load_government_names(govfile, &compat_info)
      && load_unit_names(unitfile, &compat_info)
      && load_terrain_names(terrfile, &compat_info)
      && load_style_names(stylefile, &compat_info)
      && load_nation_names(nationfile, &compat_info);
  }

  if (ok) {
    ok = rscompat_names(&compat_info);
  }

  if (ok) {
    ok = load_ruleset_techs(techfile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_styles(stylefile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_cities(cityfile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_governments(govfile, &compat_info);
  }
  if (ok) {
    /* terrain must precede nations and units */
    ok = load_ruleset_terrain(terrfile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_units(unitfile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_buildings(buildfile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_nations(nationfile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_effects(effectfile, &compat_info);
  }
  if (ok) {
    ok = load_ruleset_game(gamefile, act, &compat_info);
  }

  if (ok) {
    /* Init nations we just loaded. */
    update_nations_with_startpos();

    /* Needed by role_unit_precalcs(). */
    unit_type_action_cache_init();

    /* Prepare caches we want to sanity check. */
    role_unit_precalcs();
    road_integrators_cache_init();
    actions_rs_pre_san_gen();

    ok = autoadjust_ruleset_data()
      && sanity_check_ruleset_data(compat_info.compat_mode);
  }

  if (ok) {
    /* Only load settings for a sane ruleset */
    ok = settings_ruleset(gamefile, "settings", act);

    if (ok) {
      secfile_check_unused(gamefile);
    }
  }

  nullcheck_secfile_destroy(techfile);
  nullcheck_secfile_destroy(stylefile);
  nullcheck_secfile_destroy(cityfile);
  nullcheck_secfile_destroy(govfile);
  nullcheck_secfile_destroy(terrfile);
  nullcheck_secfile_destroy(unitfile);
  nullcheck_secfile_destroy(buildfile);
  nullcheck_secfile_destroy(nationfile);
  nullcheck_secfile_destroy(effectfile);
  nullcheck_secfile_destroy(gamefile);

  if (extra_sections) {
    free(extra_sections);
    extra_sections = NULL;
  }
  if (base_sections) {
    free(base_sections);
    base_sections = NULL;
  }
  if (road_sections) {
    free(road_sections);
    road_sections = NULL;
  }
  if (resource_sections) {
    free(resource_sections);
    resource_sections = NULL;
  }
  if (terrain_sections) {
    free(terrain_sections);
    terrain_sections = NULL;
  }

  if (ok) {
    rscompat_postprocess(&compat_info);
  }

  if (ok) {
    char **buffer = buffer_script ? &script_buffer : NULL;

    script_server_free();

    script_server_init();

    ok = (openload_script_file("script", rsdir, buffer, FALSE) == TRI_YES);
  }

  if (ok) {
    enum fc_tristate pret;
    char **buffer = buffer_script ? &parser_buffer : NULL;

    pret = openload_script_file("parser", rsdir, buffer, compat_info.compat_mode);

    if (pret == TRI_MAYBE && buffer_script) {
      parser_buffer = fc_strdup(
       "-- This file is for lua-functionality for parsing luadata.txt\n-- of this ruleset.");
    }

    ok = (pret != TRI_NO);
  }

  if (ok && !buffer_script) {
    ok = (openload_script_file("default", rsdir, NULL, FALSE) == TRI_YES);
  }

  if (ok && act) {
    /* Populate remaining caches. */
    techs_precalc_data();
    improvement_feature_cache_init();
    unit_class_iterate(pclass) {
      set_unit_class_caches(pclass);
    } unit_class_iterate_end;
    unit_type_iterate(ptype) {
      ptype->unknown_move_cost = utype_unknown_move_cost(ptype);
      set_unit_type_caches(ptype);
    } unit_type_iterate_end;
    city_production_caravan_shields_init();

    /* Build advisors unit class cache corresponding to loaded rulesets */
    adv_units_ruleset_init();
    CALL_FUNC_EACH_AI(units_ruleset_init);

    /* We may need to adjust the number of AI players
     * if the number of available nations changed. */
    (void) aifill(game.info.aifill);
  }

  return ok;
}

/**********************************************************************//**
  Reload the game settings saved in the ruleset file.
**************************************************************************/
bool reload_rulesets_settings(void)
{
  struct section_file *file;
  bool ok = TRUE;

  file = openload_ruleset_file("game", game.server.rulesetdir);
  if (file == NULL) {
    ruleset_error(LOG_ERROR, "Could not load game.ruleset:\n%s",
                  secfile_error());
    ok = FALSE;
  }
  if (ok) {
    settings_ruleset(file, "settings", TRUE);
    secfile_destroy(file);
  }

  return ok;
}

/**********************************************************************//**
  Send all ruleset information to the specified connections.
**************************************************************************/
void send_rulesets(struct conn_list *dest)
{
  conn_list_compression_freeze(dest);

  /* ruleset_control also indicates to client that ruleset sending starts. */
  send_ruleset_control(dest);

  send_ruleset_game(dest);
  send_ruleset_disasters(dest);
  send_ruleset_achievements(dest);
  send_ruleset_trade_routes(dest);
  send_ruleset_team_names(dest);
  send_ruleset_actions(dest);
  send_ruleset_action_enablers(dest);
  send_ruleset_action_auto_performers(dest);
  send_ruleset_tech_classes(dest);
  send_ruleset_techs(dest);
  send_ruleset_governments(dest);
  send_ruleset_unit_classes(dest);
  send_ruleset_units(dest);
  send_ruleset_specialists(dest);
  send_ruleset_extras(dest);
  send_ruleset_bases(dest);
  send_ruleset_roads(dest);
  send_ruleset_resources(dest);
  send_ruleset_terrain(dest);
  send_ruleset_goods(dest);
  send_ruleset_buildings(dest);
  send_ruleset_nations(dest);
  send_ruleset_styles(dest);
  send_ruleset_clauses(dest);
  send_ruleset_cities(dest);
  send_ruleset_multipliers(dest);
  send_ruleset_musics(dest);
  send_ruleset_cache(dest);

  /* Indicate client that all rulesets have now been sent. */
  lsend_packet_rulesets_ready(dest);

  /* changed game settings will be send in
   * connecthand.c:establish_new_connection() */

  conn_list_compression_thaw(dest);
}
