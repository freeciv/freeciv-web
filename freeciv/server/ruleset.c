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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* common */
#include "base.h"
#include "capability.h"
#include "city.h"
#include "effects.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "packets.h"
#include "requirements.h"
#include "specialist.h"
#include "tech.h"
#include "unit.h"

/* server */
#include "citytools.h"
#include "plrhand.h"
#include "script.h"

#include "aiunit.h"		/* update_simple_ai_types */

#include "ruleset.h"
#include "srv_main.h"

/* RULESET_SUFFIX already used, no leading dot here */
#define RULES_SUFFIX "ruleset"
#define SCRIPT_SUFFIX "lua"

#define ADVANCE_SECTION_PREFIX "advance_"
#define BUILDING_SECTION_PREFIX "building_"
#define CITYSTYLE_SECTION_PREFIX "citystyle_"
#define EFFECT_SECTION_PREFIX "effect_"
#define GOVERNMENT_SECTION_PREFIX "government_"
#define NATION_GROUP_SECTION_PREFIX "ngroup" /* without underscore? */
#define NATION_SECTION_PREFIX "nation" /* without underscore? */
#define RESOURCE_SECTION_PREFIX "resource_"
#define BASE_SECTION_PREFIX "base_"
#define SPECIALIST_SECTION_PREFIX "specialist_"
#define TERRAIN_CAPABILITY "+1.9+2007.Oct.26"
#define TERRAIN_SECTION_PREFIX "terrain_"
#define UNIT_CLASS_SECTION_PREFIX "unitclass_"
#define UNIT_SECTION_PREFIX "unit_"

static const char name_too_long[] = "Name \"%s\" too long; truncating.";
#define check_name(name) (check_strlen(name, MAX_LEN_NAME, name_too_long))
#define name_strlcpy(dst, src) \
	((void) sz_loud_strlcpy(dst, src, name_too_long))

/* avoid re-reading files */
#define MAX_SECTION_LABEL 64
#define section_strlcpy(dst, src) \
	(void) loud_strlcpy(dst, src, MAX_SECTION_LABEL, name_too_long)
static char *resource_sections = NULL;
static char *terrain_sections = NULL;
static char *base_sections = NULL;


static void openload_ruleset_file(struct section_file *file,
				  const char *whichset);
static char *check_ruleset_capabilities(struct section_file *file,
					const char *us_capstr,
					const char *filename);

static void load_tech_names(struct section_file *file);
static void load_unit_names(struct section_file *file);
static void load_building_names(struct section_file *file);
static void load_government_names(struct section_file *file);
static void load_terrain_names(struct section_file *file);
static void load_citystyle_names(struct section_file *file);
static void load_nation_names(struct section_file *file);
static struct nation_city* load_city_name_list(struct section_file *file,
					     const char *secfile_str1,
					     const char *secfile_str2);

static void load_ruleset_techs(struct section_file *file);
static void load_ruleset_units(struct section_file *file);
static void load_ruleset_buildings(struct section_file *file);
static void load_ruleset_governments(struct section_file *file);
static void load_ruleset_terrain(struct section_file *file);
static void load_ruleset_cities(struct section_file *file);
static void load_ruleset_effects(struct section_file *file);

static void load_ruleset_game(void);

static void send_ruleset_techs(struct conn_list *dest);
static void send_ruleset_unit_classes(struct conn_list *dest);
static void send_ruleset_units(struct conn_list *dest);
static void send_ruleset_buildings(struct conn_list *dest);
static void send_ruleset_terrain(struct conn_list *dest);
static void send_ruleset_resources(struct conn_list *dest);
static void send_ruleset_bases(struct conn_list *dest);
static void send_ruleset_governments(struct conn_list *dest);
static void send_ruleset_cities(struct conn_list *dest);
static void send_ruleset_game(struct conn_list *dest);

static bool nation_has_initial_tech(struct nation_type *pnation,
                                    struct advance *tech);
static bool sanity_check_ruleset_data(void);
static void ruleset_error(int loglevel, const char *format, ...)
                          fc__attribute((__format__ (__printf__, 2, 3)));

/**************************************************************************
  Notifications about ruleset errors to clients. Especially important in
  case of internal server crashing.
**************************************************************************/
static void ruleset_error(int loglevel, const char *format, ...)
{
  va_list args;

  va_start(args, format);

  vreal_freelog(loglevel, format, args);

  va_end(args);

  if (LOG_FATAL >= loglevel) {
    exit(EXIT_FAILURE);
  }
}

/**************************************************************************
  datafilename() wrapper: tries to match in two ways.
  Returns NULL on failure, the (statically allocated) filename on success.
**************************************************************************/
static char *valid_ruleset_filename(const char *subdir,
				    const char *name, const char *extension)
{
  char filename[512], *dfilename;

  assert(subdir && name && extension);

  my_snprintf(filename, sizeof(filename), "%s/%s.%s", subdir, name, extension);
  freelog(LOG_VERBOSE, "Trying \"%s\".",
                       filename);
  dfilename = datafilename(filename);
  if (dfilename) {
    return dfilename;
  }

  my_snprintf(filename, sizeof(filename), "default/%s.%s", name, extension);
  freelog(LOG_VERBOSE, "Trying \"%s\": default ruleset directory.",
                       filename);
  dfilename = datafilename(filename);
  if (dfilename) {
    return dfilename;
  }

  my_snprintf(filename, sizeof(filename), "%s_%s.%s", subdir, name, extension);
  freelog(LOG_VERBOSE, "Trying \"%s\": alternative ruleset filename syntax.",
                       filename);
  dfilename = datafilename(filename);
  if (dfilename) {
    return dfilename;
  } else {
    ruleset_error(LOG_FATAL,
                  /* TRANS: message about an installation error. */
                  _("Could not find a readable \"%s.%s\" ruleset file."),
                  name, extension);
  }

  return(NULL);
}

/**************************************************************************
  Do initial section_file_load on a ruleset file.
  "whichset" = "techs", "units", "buildings", "terrain", ...
**************************************************************************/
static void openload_ruleset_file(struct section_file *file,
			          const char *whichset)
{
  char sfilename[512];
  char *dfilename = valid_ruleset_filename(game.server.rulesetdir,
					   whichset, RULES_SUFFIX);

  /* Need to save a copy of the filename for following message, since
     section_file_load() may call datafilename() for includes. */

  sz_strlcpy(sfilename, dfilename);

  if (!section_file_load_nodup(file, sfilename)) {
    ruleset_error(LOG_FATAL, "\"%s\": could not load ruleset.",
                  sfilename);
  }
}

/**************************************************************************
  Parse script file.
**************************************************************************/
static void openload_script_file(const char *whichset)
{
  char *dfilename = valid_ruleset_filename(game.server.rulesetdir,
					   whichset, SCRIPT_SUFFIX);

  if (!script_do_file(dfilename)) {
    ruleset_error(LOG_FATAL, "\"%s\": could not load ruleset script.",
                  dfilename);
  }
}

/**************************************************************************
  Ruleset files should have a capabilities string datafile.options
  This gets and returns that string, and checks that the required
  capabilities specified are satisified.
**************************************************************************/
static char *check_ruleset_capabilities(struct section_file *file,
					const char *us_capstr, const char *filename)
{
  char *datafile_options;
  
  datafile_options = secfile_lookup_str(file, "datafile.options");
  if (!has_capabilities(us_capstr, datafile_options)) {
    freelog(LOG_FATAL, "\"%s\": ruleset datafile appears incompatible:",
                        filename);
    freelog(LOG_FATAL, "  datafile options: %s", datafile_options);
    freelog(LOG_FATAL, "  supported options: %s", us_capstr);
    ruleset_error(LOG_FATAL, "Capability problem");
  }
  if (!has_capabilities(datafile_options, us_capstr)) {
    freelog(LOG_FATAL, "\"%s\": ruleset datafile claims required option(s)"
                         " that we don't support:", filename);
    freelog(LOG_FATAL, "  datafile options: %s", datafile_options);
    freelog(LOG_FATAL, "  supported options: %s", us_capstr);
    ruleset_error(LOG_FATAL, "Capability problem");
  }
  return datafile_options;
}

/**************************************************************************
  Load a requirement list.  The list is returned as a static vector
  (callers need not worry about freeing anything).
**************************************************************************/
static struct requirement_vector *lookup_req_list(struct section_file *file,
						  const char *sec,
						  const char *sub,
                                                  const char *rfor)
{
  char *type, *name;
  int j;
  const char *filename;
  static struct requirement_vector list;

  filename = secfile_filename(file);

  requirement_vector_reserve(&list, 0);

  for (j = 0;
      (type = secfile_lookup_str_default(file, NULL, "%s.%s%d.type",
					 sec, sub, j));
      j++) {
    char *range;
    bool survives, negated;
    struct requirement req;

    name = secfile_lookup_str(file, "%s.%s%d.name", sec, sub, j);
    range = secfile_lookup_str(file, "%s.%s%d.range", sec, sub, j);

    survives = secfile_lookup_bool_default(file, FALSE,
	"%s.%s%d.survives", sec, sub, j);
    negated = secfile_lookup_bool_default(file, FALSE,
	"%s.%s%d.negated", sec, sub, j);

    req = req_from_str(type, range, survives, negated, name);
    if (VUT_LAST == req.source.kind) {
      /* Error.  Log it, clear the req and continue. */
      freelog(LOG_ERROR,
          "\"%s\" [%s] has unknown req: \"%s\" \"%s\".",
          filename, sec, type, name);
      req.source.kind = VUT_NONE;
    }

    requirement_vector_append(&list, &req);
  }

  if (j > MAX_NUM_REQS) {
    ruleset_error(LOG_FATAL, "Too many (%d) requirements for %s. Max is %d",
                  j, rfor, MAX_NUM_REQS);
  }

  return &list;
}

/**************************************************************************
 Lookup a string prefix.entry in the file and return the corresponding
 advances pointer.  If (!required), return A_NEVER for match "Never" or
 can't match.  If (required), die when can't match.  Note the first tech
 should have name "None" so that will always match.
 If description is not NULL, it is used in the warning message
 instead of prefix (eg pass unit->name instead of prefix="units2.u27")
**************************************************************************/
static struct advance *lookup_tech(struct section_file *file,
				   const char *prefix, const char *entry,
				   int loglevel, const char *filename,
				   const char *description)
{
  char *sval;
  struct advance *padvance;
  
  sval = secfile_lookup_str_default(file, NULL, "%s.%s", prefix, entry);
  if (!sval || (LOG_FATAL < loglevel && strcmp(sval, "Never") == 0)) {
    padvance = A_NEVER;
  } else {
    padvance = find_advance_by_rule_name(sval);

    if (A_NEVER == padvance) {
      ruleset_error(loglevel,
                    "\"%s\" %s %s: couldn't match \"%s\".",
                    filename, (description ? description : prefix), entry, sval);

      /* ruleset_error returned only if error was not fatal. */
    }
  }
  return padvance;
}

/**************************************************************************
 Lookup a string prefix.entry in the file and return the corresponding
 improvement pointer.  If (!required), return B_NEVER for match "None" or
 can't match.  If (required), die when can't match.
 If description is not NULL, it is used in the warning message
 instead of prefix (eg pass unit->name instead of prefix="units2.u27")
**************************************************************************/
static struct impr_type *lookup_building(struct section_file *file,
					 const char *prefix, const char *entry,
					 int loglevel, const char *filename,
					 const char *description)
{
  char *sval;
  struct impr_type *pimprove;
  
  sval = secfile_lookup_str_default(file, NULL, "%s.%s", prefix, entry);
  if (!sval || (LOG_FATAL < loglevel && strcmp(sval, "None") == 0)) {
    pimprove = B_NEVER;
  } else {
    pimprove = find_improvement_by_rule_name(sval);

    if (B_NEVER == pimprove) {
      ruleset_error(loglevel,
                    "\"%s\" %s %s: couldn't match \"%s\".",
                    filename, (description ? description : prefix), entry, sval);

      /* ruleset_error() returned only if error was not fatal */
    }
  }
  return pimprove;
}

/**************************************************************************
 Lookup a prefix.entry string vector in the file and fill in the
 array, which should hold MAX_NUM_UNIT_LIST items. The output array is
 either U_LAST terminated or full (contains MAX_NUM_UNIT_LIST
 items). If the vector is not found and the required parameter is set,
 we report it as an error, otherwise we just punt.
**************************************************************************/
static void lookup_unit_list(struct section_file *file, const char *prefix,
			     const char *entry, int loglevel,
			     struct unit_type **output, 
                             const char *filename)
{
  char **slist;
  int i, nval;

  /* pre-fill with NULL: */
  for(i = 0; i < MAX_NUM_UNIT_LIST; i++) {
    output[i] = NULL;
  }
  slist = secfile_lookup_str_vec(file, &nval, "%s.%s", prefix, entry);
  if (nval == 0) {
    if (LOG_FATAL >= loglevel) {
      ruleset_error(LOG_FATAL, "\"%s\": missing string vector %s.%s",
                    filename, prefix, entry);
    }
    return;
  }
  if (nval > MAX_NUM_UNIT_LIST) {
    ruleset_error(LOG_FATAL,
                  "\"%s\": string vector %s.%s too long (%d, max %d)",
                  filename, prefix, entry, nval, MAX_NUM_UNIT_LIST);
  }
  if (nval == 1 && strcmp(slist[0], "") == 0) {
    free(slist);
    return;
  }
  for (i = 0; i < nval; i++) {
    char *sval = slist[i];
    struct unit_type *punittype = find_unit_type_by_rule_name(sval);

    if (!punittype) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" %s.%s (%d): couldn't match \"%s\".",
                    filename, prefix, entry, i, sval);
    }
    output[i] = punittype;
    freelog(LOG_DEBUG, "\"%s\" %s.%s (%d): %s (%d)",
            filename, prefix, entry, i, sval,
            utype_number(punittype));
  }
  free(slist);
  return;
}

/**************************************************************************
 Lookup a prefix.entry string vector in the file and fill in the
 array, which should hold MAX_NUM_TECH_LIST items. The output array is
 either A_LAST terminated or full (contains MAX_NUM_TECH_LIST
 items). All valid entries of the output array are guaranteed to
 exist. There should be at least one value, but it may be "",
 meaning empty list.
**************************************************************************/
static void lookup_tech_list(struct section_file *file, const char *prefix,
			     const char *entry, int *output, const char *filename)
{
  char **slist;
  int i, nval;

  /* pre-fill with A_LAST: */
  for(i=0; i<MAX_NUM_TECH_LIST; i++) {
    output[i] = A_LAST;
  }
  slist = secfile_lookup_str_vec(file, &nval, "%s.%s", prefix, entry);
  if (nval==0) {
    ruleset_error(LOG_FATAL, "\"%s\": missing string vector %s.%s",
                  filename, prefix, entry);
  }
  if (nval>MAX_NUM_TECH_LIST) {
    ruleset_error(LOG_FATAL,
                  "\"%s\": string vector %s.%s too long (%d, max %d)",
                  filename, prefix, entry, nval, MAX_NUM_TECH_LIST);
  }
  if (nval==1 && strcmp(slist[0], "")==0) {
    free(slist);
    return;
  }
  for (i=0; i<nval; i++) {
    char *sval = slist[i];
    struct advance *padvance = find_advance_by_rule_name(sval);

    if (NULL == padvance) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" %s.%s (%d): couldn't match \"%s\".",
                    filename, prefix, entry, i, sval);
    }
    if (!valid_advance(padvance)) {
      ruleset_error(LOG_FATAL, "\"%s\" %s.%s (%d): \"%s\" is removed.",
                    filename, prefix, entry, i, sval);
    }
    output[i] = advance_number(padvance);
    freelog(LOG_DEBUG, "\"%s\" %s.%s (%d): %s (%d)",
            filename, prefix, entry, i, sval,
            advance_number(padvance));
  }
  free(slist);
  return;
}

/**************************************************************************
  Lookup a prefix.entry string vector in the file and fill in the
  array, which should hold MAX_NUM_BUILDING_LIST items. The output array is
  either B_LAST terminated or full (contains MAX_NUM_BUILDING_LIST
  items). [All valid entries of the output array are guaranteed to pass
  improvement_exist()?] There should be at least one value, but it may be
  "", meaning an empty list.
**************************************************************************/
static void lookup_building_list(struct section_file *file, const char *prefix,
				 const char *entry, int *output,
				 const char *filename)
{
  char **slist;
  int i, nval;

  /* pre-fill with B_LAST: */
  for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
    output[i] = B_LAST;
  }
  slist = secfile_lookup_str_vec(file, &nval, "%s.%s", prefix, entry);
  if (nval == 0) {
    ruleset_error(LOG_FATAL, "\"%s\": missing string vector %s.%s",
                  filename, prefix, entry);
  }
  if (nval > MAX_NUM_BUILDING_LIST) {
    ruleset_error(LOG_FATAL,
                  "\"%s\": string vector %s.%s too long (%d, max %d)",
                  filename, prefix, entry, nval, MAX_NUM_BUILDING_LIST);
  }
  if (nval == 1 && strcmp(slist[0], "") == 0) {
    free(slist);
    return;
  }
  for (i = 0; i < nval; i++) {
    char *sval = slist[i];
    struct impr_type *pimprove = find_improvement_by_rule_name(sval);

    if (NULL == pimprove) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" %s.%s (%d): couldn't match \"%s\".",
                    filename, prefix, entry, i, sval);
    }
    output[i] = improvement_number(pimprove);
    freelog(LOG_DEBUG, "%s.%s,%d %s %d", prefix, entry, i, sval, output[i]);
  }
  free(slist);
}

/**************************************************************************
 Lookup a string prefix.entry in the file and return the corresponding
 unit_type id.  If (!required), return NULL if match "None" or can't match.
 If (required), die if can't match.
 If description is not NULL, it is used in the warning message
 instead of prefix (eg pass unit->name instead of prefix="units2.u27")
**************************************************************************/
static struct unit_type *lookup_unit_type(struct section_file *file,
					  const char *prefix,
					  const char *entry,
					  int loglevel,
					  const char *filename,
					  const char *description)
{
  char *sval;
  struct unit_type *punittype;
  
  if (LOG_FATAL >= loglevel) {
    sval = secfile_lookup_str(file, "%s.%s", prefix, entry);
  } else {
    sval = secfile_lookup_str_default(file, "None", "%s.%s", prefix, entry);
  }

  if (strcmp(sval, "None")==0) {
    punittype = NULL;
  } else {
    punittype = find_unit_type_by_rule_name(sval);
    if (!punittype) {
      ruleset_error(loglevel,
                    "\"%s\" %s %s: couldn't match \"%s\".",
                    filename, (description ? description : prefix), entry, sval);

      /* We continue if error was not fatal. */
      punittype = NULL;
    }
  }
  return punittype;
}

/**************************************************************************
  Lookup entry in the file and return the corresponding government index;
  dies if can't find/match.  filename is for error message.
**************************************************************************/
static struct government *lookup_government(struct section_file *file,
					    const char *entry,
					    const char *filename)
{
  char *sval;
  struct government *gov;
  
  sval = secfile_lookup_str(file, "%s", entry);
  gov = find_government_by_rule_name(sval);
  if (!gov) {
    ruleset_error(LOG_FATAL,
                  "\"%s\" %s: couldn't match \"%s\".",
                  filename, entry, sval);
  }
  return gov;
}

/**************************************************************************
  Lookup entry in the file and return the corresponding move_type index;
  dies if can't find/match.  filename is for error message.
**************************************************************************/
static enum unit_move_type lookup_move_type(struct section_file *file,
					    const char *entry,
					    const char *filename)
{
  char *sval;
  enum unit_move_type mt;
  
  sval = secfile_lookup_str(file, "%s", entry);
  mt = move_type_from_str(sval);
  if (mt == MOVETYPE_LAST) {
    ruleset_error(LOG_FATAL,
                  "\"%s\" %s: couldn't match \"%s\".",
                  filename, entry, sval);
  }
  return mt;
}

/**************************************************************************
  Lookup optional string, returning allocated memory or NULL.
**************************************************************************/
static char *lookup_string(struct section_file *file, const char *prefix,
			   const char *suffix)
{
  char *sval;
  
  sval = secfile_lookup_str_default(file, NULL, "%s.%s", prefix, suffix);
  if (sval) {
    sval = skip_leading_spaces(sval);
    if (strlen(sval) > 0) {
      return mystrdup(sval);
    }
  }
  return NULL;
}

/**************************************************************************
  Lookup optional helptext, returning allocated memory or NULL.
**************************************************************************/
static char *lookup_helptext(struct section_file *file, const char *prefix)
{
  return lookup_string(file, prefix, "helptext");
}

/**************************************************************************
  Look up the resource section name and return its pointer.
**************************************************************************/
static struct resource *lookup_resource(const char *filename,
					const char *name,
					const char *jsection)
{
  resource_type_iterate(presource) {
    const int i = resource_index(presource);
    const char *isection = &resource_sections[i * MAX_SECTION_LABEL];
    if (0 == mystrcasecmp(isection, name)) {
      return presource;
    }
  } resource_type_iterate_end;

  ruleset_error(LOG_ERROR,
                "\"%s\" [%s] has unknown \"%s\".",
                filename,
                jsection,
                name);
  return NULL;
}

/**************************************************************************
  Look up the terrain section name and return its pointer.
**************************************************************************/
static struct terrain *lookup_terrain(struct section_file *file,
				      const char *item,
				      struct terrain *pthis)
{
  const int j = terrain_index(pthis);
  const char *jsection = &terrain_sections[j * MAX_SECTION_LABEL];
  char *name = secfile_lookup_str(file, "%s.%s", jsection, item);

  if (NULL == name
      || *name == '\0'
      || (0 == strcmp(name, "none"))
      || (0 == strcmp(name, "no"))) {
    return T_NONE;
  }
  if (0 == strcmp(name, "yes")) {
    return pthis;
  }

  terrain_type_iterate(pterrain) {
    const int i = terrain_index(pterrain);
    const char *isection = &terrain_sections[i * MAX_SECTION_LABEL];
    if (0 == mystrcasecmp(isection, name)) {
      return pterrain;
    }
  } terrain_type_iterate_end;

  ruleset_error(LOG_ERROR,
                "\"%s\" [%s] has unknown \"%s\".",
                secfile_filename(file),
                jsection,
                name);
  return T_NONE;
}

/**************************************************************************
  ...
**************************************************************************/
static void load_tech_names(struct section_file *file)
{
  char **sec;
  int num_techs; /* number of techs in the ruleset (means without A_NONE)*/
  int i;
  const char *filename = secfile_filename(file);

  (void) section_file_lookup(file, "datafile.description");	/* unused */

  /* The names: */
  sec = secfile_get_secnames_prefix(file, ADVANCE_SECTION_PREFIX, &num_techs);
  freelog(LOG_VERBOSE, "%d advances (including possibly unused)", num_techs);
  if(num_techs == 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No Advances?!?", filename);
  }

  if(num_techs + A_FIRST > A_LAST_REAL) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many advances (%d, max %d)",
                  filename, num_techs, A_LAST_REAL-A_FIRST);
  }

  game.control.num_tech_types = num_techs + A_FIRST; /* includes A_NONE */

  i = 0;
  advance_iterate(A_FIRST, a) {
    char *name = secfile_lookup_str(file, "%s.name", sec[i]);
    name_strlcpy(a->name.vernacular, name);
    a->name.translated = NULL;
    i++;
  } advance_iterate_end;
  free(sec);
}

/**************************************************************************
  ...
**************************************************************************/
static void load_ruleset_techs(struct section_file *file)
{
  char **sec;
  int num_techs; /* number of techs in the ruleset (means without A_NONE)*/
  int i;
  struct advance *a_none = advance_by_number(A_NONE);
  const char *filename = secfile_filename(file);
  
  (void) check_ruleset_capabilities(file, "+1.9", filename);
  sec = secfile_get_secnames_prefix(file, ADVANCE_SECTION_PREFIX, &num_techs);

  /* Initialize dummy tech A_NONE */
  a_none->require[AR_ONE] = a_none;
  a_none->require[AR_TWO] = a_none;
  a_none->require[AR_ROOT] = A_NEVER;
  a_none->flags = 0;

  i = 0;
  advance_iterate(A_FIRST, a) {
    char *sval, **slist;
    int j,ival,nval;

    a->require[AR_ONE] = lookup_tech(file, sec[i], "req1", LOG_ERROR,
				     filename, a->name.vernacular);
    a->require[AR_TWO] = lookup_tech(file, sec[i], "req2", LOG_ERROR,
				     filename, a->name.vernacular);
    a->require[AR_ROOT] = lookup_tech(file, sec[i], "root_req", LOG_ERROR,
				      filename, a->name.vernacular);

    if ((A_NEVER == a->require[AR_ONE] && A_NEVER != a->require[AR_TWO])
     || (A_NEVER != a->require[AR_ONE] && A_NEVER == a->require[AR_TWO])) {
      freelog(LOG_ERROR, "\"%s\" [%s] \"%s\": \"Never\" with non-\"Never\".",
              filename,
              sec[i],
              a->name.vernacular);
      a->require[AR_ONE] = a->require[AR_TWO] = A_NEVER;
    }
    if (a_none == a->require[AR_ONE] && a_none != a->require[AR_TWO]) {
      freelog(LOG_ERROR, "\"%s\" [%s] \"%s\": should have \"None\" second.",
              filename,
              sec[i],
              a->name.vernacular);
      a->require[AR_ONE] = a->require[AR_TWO];
      a->require[AR_TWO] = a_none;
    }

    a->flags = 0;

    slist = secfile_lookup_str_vec(file, &nval, "%s.flags", sec[i]);
    for(j=0; j<nval; j++) {
      sval = slist[j];
      if(strcmp(sval,"")==0) {
        continue;
      }
      ival = find_advance_flag_by_rule_name(sval);
      if (ival==TF_LAST) {
        freelog(LOG_ERROR, "\"%s\" [%s] \"%s\": bad flag name \"%s\".",
                filename,
                sec[i],
                a->name.vernacular,
                sval);
      } else {
        a->flags |= (1<<ival);
      }
    }
    free(slist);

    sz_strlcpy(a->graphic_str,
	       secfile_lookup_str_default(file, "-", "%s.graphic", sec[i]));
    sz_strlcpy(a->graphic_alt,
	       secfile_lookup_str_default(file, "-",
					  "%s.graphic_alt", sec[i]));
    
    a->helptext = lookup_helptext(file, sec[i]);    
    a->bonus_message = lookup_string(file, sec[i], "bonus_message");
    a->preset_cost =
	secfile_lookup_int_default(file, -1, "%s.%s", sec[i], "cost");
    a->num_reqs = 0;
    
    i++;
  } advance_iterate_end;

  /* Propagate a root tech up into the tech tree.  Thus if a technology
   * X has Y has a root tech, then any technology requiring X also has
   * Y as a root tech. */
restart:
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
    if (!valid_advance(a)) {
      /* We check for recursive tech loops later,
       * in build_required_techs_helper. */
      if (!valid_advance(a->require[AR_ONE])) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" tech \"%s\": req1 leads to removed tech.",
                      filename,
                      advance_rule_name(a));
      } 
      if (!valid_advance(a->require[AR_TWO])) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" tech \"%s\": req2 leads to removed tech.",
                      filename,
                      advance_rule_name(a));
      }
    }
  } advance_iterate_end;

  free(sec);
  section_file_check_unused(file, filename);
  section_file_free(file);
}

/**************************************************************************
  ...  
**************************************************************************/
static void load_unit_names(struct section_file *file)
{
  char **sec;
  int nval;
  int user_flags;
  char **flaglist;
  int i;
  const char *filename = secfile_filename(file);

  (void) section_file_lookup(file, "datafile.description");	/* unused */

  /* User unit flag names */
  flaglist = secfile_lookup_str_vec(file, &user_flags,
		  		    "flags.names");

  if (user_flags > MAX_NUM_USER_UNIT_FLAGS) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many user unit type flags!",
                  filename);
  }

  for (i = 0; i < user_flags; i++) {
    set_user_unit_flag_name(F_USER_FLAG_1 + i, flaglist[i]);
  }
  for (; i < MAX_NUM_USER_UNIT_FLAGS; i++) {
    set_user_unit_flag_name(F_USER_FLAG_1 + i, NULL);
  }
  if (flaglist) {
    free(flaglist);
  }

  /* Unit classes */
  sec = secfile_get_secnames_prefix(file, UNIT_CLASS_SECTION_PREFIX, &nval);
  freelog(LOG_VERBOSE, "%d unit classes", nval);
  if (nval == 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No unit classes?!?", filename);
  }
  if(nval > UCL_LAST) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many unit classes (%d, max %d)",
                  filename, nval, UCL_LAST);
  }

  game.control.num_unit_classes = nval;

  unit_class_iterate(punitclass) {
    const int i = uclass_index(punitclass);
    char *name = secfile_lookup_str(file, "%s.name", sec[i]);

    name_strlcpy(punitclass->name.vernacular, name);
    punitclass->name.translated = NULL;
  } unit_class_iterate_end;

  /* The names: */
  sec = secfile_get_secnames_prefix(file, UNIT_SECTION_PREFIX, &nval);
  freelog(LOG_VERBOSE, "%d unit types (including possibly unused)", nval);
  if(nval == 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No unit types?!?", filename);
  }
  if(nval > U_LAST) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many unit types (%d, max %d)",
                  filename, nval, U_LAST);
  }

  game.control.num_unit_types = nval;

  unit_type_iterate(punittype) {
    const int i = utype_index(punittype);
    char *name = secfile_lookup_str(file, "%s.name", sec[i]);

    name_strlcpy(punittype->name.vernacular, name);
    punittype->name.translated = NULL;
  } unit_type_iterate_end;

  free(sec);
}

/**************************************************************************
...  
**************************************************************************/
static void load_ruleset_units(struct section_file *file)
{
  struct unit_type *u;
  int i, j, ival, nval, vet_levels, vet_levels_default;
  char *sval, **slist, **sec, **csec;
  const char *filename = secfile_filename(file);
  char **vnlist, **def_vnlist;
  int *vblist, *def_vblist;

  (void) check_ruleset_capabilities(file, "+1.9", filename);

  /*
   * Load up expanded veteran system values.
   */
  sec = secfile_get_secnames_prefix(file, UNIT_SECTION_PREFIX, &nval);

#define CHECK_VETERAN_LIMIT(_count, _string)				\
if (_count > MAX_VET_LEVELS) {						\
  ruleset_error(LOG_FATAL, "\"%s\": Too many " _string " entries (%d, max %d)", \
                filename, _count, MAX_VET_LEVELS);                      \
}

  /* level names */
  def_vnlist = secfile_lookup_str_vec(file, &vet_levels_default,
		  		"veteran_system.veteran_names");
  CHECK_VETERAN_LIMIT(vet_levels_default, "veteran_names");

  unit_type_iterate(u) {
    const int i = utype_index(u);

    vnlist = secfile_lookup_str_vec(file, &vet_levels,
                                    "%s.veteran_names", sec[i]);
    CHECK_VETERAN_LIMIT(vet_levels, "veteran_names");
    if (vnlist) {
      /* unit has own veterancy settings */
      for (j = 0; j < vet_levels; j++) {
        sz_strlcpy(u->veteran[j].name, vnlist[j]);
      }
      free(vnlist);
    } else {
      /* apply defaults */  
      for (j = 0; j < vet_levels_default; j++) {
        sz_strlcpy(u->veteran[j].name, def_vnlist[j]);
      }
    }
    /* We check for this value to determine how many veteran levels
     * a unit type has */
    for (; j < MAX_VET_LEVELS; j++) {
      u->veteran[j].name[0] = '\0';
    }
  } unit_type_iterate_end;
  free(def_vnlist);

  /* power factor */
  def_vblist = secfile_lookup_int_vec(file, &vet_levels_default,
                                      "veteran_system.veteran_power_fact");
  CHECK_VETERAN_LIMIT(vet_levels_default, "veteran_power_fact");
  unit_type_iterate(u) {
    const int i = utype_index(u);

    vblist = secfile_lookup_int_vec(file, &vet_levels,
                                    "%s.veteran_power_fact", sec[i]);
    CHECK_VETERAN_LIMIT(vet_levels, "veteran_power_fact");
    if (vblist) {
      for (j = 0; j < vet_levels; j++) {
        u->veteran[j].power_fact = ((double)vblist[j]) / 100;
      }
      free(vblist);
    } else {
      for (j = 0; j < vet_levels_default; j++) {
        u->veteran[j].power_fact = ((double)def_vblist[j]) / 100;
      }
    }
  } unit_type_iterate_end;
  if (def_vblist) {
    free(def_vblist);
  }

  /* raise chance */
  def_vblist = secfile_lookup_int_vec(file, &vet_levels_default,
                                      "veteran_system.veteran_raise_chance");
  CHECK_VETERAN_LIMIT(vet_levels_default, "veteran_raise_chance");
  for (i = 0; i < vet_levels_default; i++) {
    game.veteran_chance[i] = def_vblist[i];
  }
  for (; i < MAX_VET_LEVELS; i++) {
    game.veteran_chance[i] = 0;
  }
  if (def_vblist) {
    free(def_vblist);
  }

  /* work raise chance */
  def_vblist = secfile_lookup_int_vec(file, &vet_levels_default,
                                    "veteran_system.veteran_work_raise_chance");
  CHECK_VETERAN_LIMIT(vet_levels_default, "veteran_work_raise_chance");
  for (i = 0; i < vet_levels_default; i++) {
    game.work_veteran_chance[i] = def_vblist[i];
  }
  for (; i < MAX_VET_LEVELS; i++) {
    game.work_veteran_chance[i] = 0;
  }
  if (def_vblist) {
    free(def_vblist);
  }

  /* move bonus */
  def_vblist = secfile_lookup_int_vec(file, &vet_levels_default,
                                      "veteran_system.veteran_move_bonus");
  CHECK_VETERAN_LIMIT(vet_levels_default, "veteran_move_bonus");
  unit_type_iterate(u) {
    const int i = utype_index(u);

    vblist = secfile_lookup_int_vec(file, &vet_levels,
  		  	"%s.veteran_move_bonus", sec[i]);
    CHECK_VETERAN_LIMIT(vet_levels, "veteran_move_bonus");
    if (vblist) {
      for (j = 0; j < vet_levels; j++) {
        u->veteran[j].move_bonus = vblist[j];
      }
      free(vblist);
    } else {
      for (j = 0; j < vet_levels_default; j++) {
        u->veteran[j].move_bonus = def_vblist[j];
      }
    }
  } unit_type_iterate_end;
  if (def_vblist) {
    free(def_vblist);
  }

  csec = secfile_get_secnames_prefix(file, UNIT_CLASS_SECTION_PREFIX, &nval);

  unit_class_iterate(ut) {
    int i = uclass_index(ut);
    char tmp[200] = "\0";
    char *hut_str;

    mystrlcat(tmp, csec[i], 200);
    mystrlcat(tmp, ".move_type", 200);
    ut->move_type = lookup_move_type(file, tmp, filename);
    ut->min_speed = SINGLE_MOVE * secfile_lookup_int(file, "%s.min_speed", csec[i]);
    ut->hp_loss_pct = secfile_lookup_int(file,"%s.hp_loss_pct", csec[i]);

    hut_str = secfile_lookup_str_default(file, "Normal", "%s.hut_behavior", csec[i]);
    if (mystrcasecmp(hut_str, "Normal") == 0) {
      ut->hut_behavior = HUT_NORMAL;
    } else if (mystrcasecmp(hut_str, "Nothing") == 0) {
      ut->hut_behavior = HUT_NOTHING;
    } else if (mystrcasecmp(hut_str, "Frighten") == 0) {
      ut->hut_behavior = HUT_FRIGHTEN;
    } else {
      ruleset_error(LOG_FATAL,
                    "\"%s\" unit_class \"%s\":"
                    " Illegal hut behavior \"%s\".",
                    filename,
                    uclass_rule_name(ut),
                    hut_str);
    }

    BV_CLR_ALL(ut->flags);
    slist = secfile_lookup_str_vec(file, &nval, "%s.flags", csec[i]);
    for(j = 0; j < nval; j++) {
      sval = slist[j];
      if(strcmp(sval,"") == 0) {
	continue;
      }
      ival = find_unit_class_flag_by_rule_name(sval);
      if (ival == UCF_LAST) {
        freelog(LOG_ERROR, "\"%s\" unit_class \"%s\": bad flag name \"%s\".",
                filename,
                uclass_rule_name(ut),
                sval);
        ival = find_unit_flag_by_rule_name(sval);
        if (ival != F_LAST) {
          freelog(LOG_ERROR, "\"%s\" unit_class \"%s\": unit_type flag!",
                  filename,
                  uclass_rule_name(ut));
        }
      } else if (ut->move_type == SEA_MOVING
                 && ( ival == UCF_ROAD_NATIVE || ival == UCF_RIVER_NATIVE)) {
        freelog(LOG_ERROR, "\"%s\" unit_class \"%s\": cannot give \"%s\" flag"
                " to sea moving unit",
                filename,
                uclass_rule_name(ut),
                sval);
      } else {
        BV_SET(ut->flags, ival);
      }
    }
    free(slist);

  } unit_class_iterate_end;

  /* Tech and Gov requirements */  
  unit_type_iterate(u) {
    const int i = utype_index(u);

    u->require_advance = lookup_tech(file, sec[i], "tech_req", LOG_FATAL,
				     filename, u->name.vernacular);
    if (section_file_lookup(file, "%s.gov_req", sec[i])) {
      char tmp[200] = "\0";
      mystrlcat(tmp, sec[i], 200);
      mystrlcat(tmp, ".gov_req", 200);
      u->need_government = lookup_government(file, tmp, filename);
    } else {
      u->need_government = NULL; /* no requirement */
    }
  } unit_type_iterate_end;
  
  unit_type_iterate(u) {
    const int i = utype_index(u);

    u->obsoleted_by = lookup_unit_type(file, sec[i], "obsolete_by", LOG_ERROR,
				       filename, u->name.vernacular);
    u->transformed_to = lookup_unit_type(file, sec[i], "transform_to",
                                         LOG_ERROR, filename,
                                         u->name.vernacular);
  } unit_type_iterate_end;

  /* main stats: */
  unit_type_iterate(u) {
    const int i = utype_index(u);
    struct unit_class *pclass;

    u->need_improvement = lookup_building(file, sec[i], "impr_req", LOG_ERROR,
					  filename, u->name.vernacular);

    sval = secfile_lookup_str(file, "%s.class", sec[i]);
    pclass = find_unit_class_by_rule_name(sval);
    if (!pclass) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" unit_type \"%s\":"
                    " bad class \"%s\".",
                    filename,
                    utype_rule_name(u),
                    sval);
    }
    u->uclass = pclass;
    
    sz_strlcpy(u->sound_move,
	       secfile_lookup_str_default(file, "-", "%s.sound_move",
					  sec[i]));
    sz_strlcpy(u->sound_move_alt,
	       secfile_lookup_str_default(file, "-", "%s.sound_move_alt",
					  sec[i]));
    sz_strlcpy(u->sound_fight,
	       secfile_lookup_str_default(file, "-", "%s.sound_fight",
					  sec[i]));
    sz_strlcpy(u->sound_fight_alt,
	       secfile_lookup_str_default(file, "-", "%s.sound_fight_alt",
					  sec[i]));
    
    sz_strlcpy(u->graphic_str,
	       secfile_lookup_str(file,"%s.graphic", sec[i]));
    sz_strlcpy(u->graphic_alt,
	       secfile_lookup_str_default(file, "-", "%s.graphic_alt", sec[i]));
    
    u->build_cost =
      secfile_lookup_int(file,"%s.build_cost", sec[i]);
    u->pop_cost =
      secfile_lookup_int(file,"%s.pop_cost", sec[i]);
    u->attack_strength =
      secfile_lookup_int(file,"%s.attack", sec[i]);
    u->defense_strength =
      secfile_lookup_int(file,"%s.defense", sec[i]);
    u->move_rate =
      SINGLE_MOVE*secfile_lookup_int(file,"%s.move_rate", sec[i]);
    
    u->vision_radius_sq =
      secfile_lookup_int(file,"%s.vision_radius_sq", sec[i]);
    u->transport_capacity =
      secfile_lookup_int(file,"%s.transport_cap", sec[i]);
    u->hp = secfile_lookup_int(file,"%s.hitpoints", sec[i]);
    u->firepower = secfile_lookup_int(file,"%s.firepower", sec[i]);
    if (u->firepower <= 0) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" unit_type \"%s\":"
                    " firepower is %d,"
                    " but must be at least 1. "
                    "  If you want no attack ability,"
                    " set the unit's attack strength to 0.",
                    filename,
                    utype_rule_name(u),
                    u->firepower);
    }
    u->fuel = secfile_lookup_int(file,"%s.fuel", sec[i]);

    u->happy_cost  = secfile_lookup_int(file, "%s.uk_happy", sec[i]);
    output_type_iterate(o) {
      u->upkeep[o] = secfile_lookup_int_default(file, 0, "%s.uk_%s", sec[i],
						get_output_identifier(o));
    } output_type_iterate_end;

    slist = secfile_lookup_str_vec(file, &nval, "%s.cargo", sec[i]);
    BV_CLR_ALL(u->cargo);
    for (j = 0; j < nval; j++) {
      struct unit_class *class = find_unit_class_by_rule_name(slist[j]);

      if (!class) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" unit_type \"%s\":"
                      "has unknown unit class %s as cargo.",
                      filename,
                      utype_rule_name(u),
                      slist[j]);
      }

      BV_SET(u->cargo, uclass_index(class));
    }
    free(slist);

    slist = secfile_lookup_str_vec(file, &nval, "%s.targets", sec[i]);
    BV_CLR_ALL(u->targets);
    for (j = 0; j < nval; j++) {
      struct unit_class *class = find_unit_class_by_rule_name(slist[j]);

      if (!class) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" unit_type \"%s\":"
                      "has unknown unit class %s as target.",
                      filename,
                      utype_rule_name(u),
                      slist[j]);
      }

      BV_SET(u->targets, uclass_index(class));
    }
    free(slist);

    /* Set also all classes that are never unreachable as targets. */
    unit_class_iterate(pclass) {
      if (!uclass_has_flag(pclass, UCF_UNREACHABLE)) {
        BV_SET(u->targets, uclass_index(pclass));
      }
    } unit_class_iterate_end;

    u->helptext = lookup_helptext(file, sec[i]);

    u->paratroopers_range = secfile_lookup_int_default(file,
        0, "%s.paratroopers_range", sec[i]);
    u->paratroopers_mr_req = SINGLE_MOVE * secfile_lookup_int_default(file,
        0, "%s.paratroopers_mr_req", sec[i]);
    u->paratroopers_mr_sub = SINGLE_MOVE * secfile_lookup_int_default(file,
        0, "%s.paratroopers_mr_sub", sec[i]);
    u->bombard_rate = secfile_lookup_int_default(file,
	0, "%s.bombard_rate", sec[i]);
    u->city_size = secfile_lookup_int_default(file, 1, "%s.city_size", sec[i]);
  } unit_type_iterate_end;
  
  /* flags */
  unit_type_iterate(u) {
    const int i = utype_index(u);

    BV_CLR_ALL(u->flags);
    assert(!utype_has_flag(u, F_LAST-1));

    slist = secfile_lookup_str_vec(file, &nval, "%s.flags", sec[i]);
    for(j=0; j<nval; j++) {
      sval = slist[j];
      if(strcmp(sval,"")==0) {
	continue;
      }
      ival = find_unit_flag_by_rule_name(sval);
      if (ival==F_LAST) {
        freelog(LOG_ERROR, "\"%s\" unit_type \"%s\": bad flag name \"%s\".",
                filename,
                utype_rule_name(u),
                sval);
        ival = find_unit_class_flag_by_rule_name(sval);
        if (ival != UCF_LAST) {
          freelog(LOG_ERROR, "\"%s\" unit_type \"%s\": unit_class flag!",
                  filename,
                  utype_rule_name(u));
        }
      } else {
        BV_SET(u->flags, ival);
      }
      assert(utype_has_flag(u, ival));
    }
    free(slist);
  } unit_type_iterate_end;
    
  /* roles */
  unit_type_iterate(u) {
    const int i = utype_index(u);

    BV_CLR_ALL(u->roles);
    
    slist = secfile_lookup_str_vec(file, &nval, "%s.roles", sec[i] );
    for(j=0; j<nval; j++) {
      sval = slist[j];
      if(strcmp(sval,"")==0) {
	continue;
      }
      ival = find_unit_role_by_rule_name(sval);
      if (ival==L_LAST) {
        freelog(LOG_ERROR, "\"%s\" unit_type \"%s\": bad role name \"%s\".",
                filename, utype_rule_name(u), sval);
      } else if ((ival == L_FERRYBOAT || ival == L_BARBARIAN_BOAT)
                 && u->uclass->move_type == LAND_MOVING) {
        freelog(LOG_ERROR,
                "\"%s\" unit_type \"%s\": role \"%s\" for land moving unit.",
                filename, utype_rule_name(u), sval);
      } else {
        BV_SET(u->roles, ival - L_FIRST);
      }
      assert(utype_has_role(u, ival));
    }
    free(slist);
  } unit_type_iterate_end;

  /* Some more consistency checking: */
  unit_type_iterate(u) {
    if (!valid_advance(u->require_advance)) {
      freelog(LOG_ERROR,
              "\"%s\" unit_type \"%s\": depends on removed tech \"%s\".",
              filename,
              utype_rule_name(u),
              advance_rule_name(u->require_advance));
      u->require_advance = A_NEVER;
    }

    if (utype_has_flag(u, F_SETTLERS)
        && u->city_size <= 0) {
      ruleset_error(LOG_ERROR, "\"%s\": Unit %s would build size %d cities",
                    filename, utype_rule_name(u), u->city_size);
      u->city_size = 1;
    }
  } unit_type_iterate_end;

  /* Setup roles and flags pre-calcs: */
  role_unit_precalcs();
     
  /* Check some required flags and roles etc: */
  if(num_role_units(F_CITIES)==0) {
    ruleset_error(LOG_FATAL, "\"%s\": No flag=cities units?", filename);
  }
  if(num_role_units(F_SETTLERS)==0) {
    ruleset_error(LOG_FATAL, "\"%s\": No flag=settler units?", filename);
  }
  if(num_role_units(L_EXPLORER)==0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=explorer units?", filename);
  }
  if(num_role_units(L_FERRYBOAT)==0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=ferryboat units?", filename);
  }
  if(num_role_units(L_FIRSTBUILD)==0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=firstbuild units?", filename);
  }
  if (num_role_units(L_BARBARIAN) == 0 && game.info.barbarianrate > 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=barbarian units?", filename);
  }
  if (num_role_units(L_BARBARIAN_LEADER) == 0 && game.info.barbarianrate > 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=barbarian leader units?", filename);
  }
  if (num_role_units(L_BARBARIAN_BUILD) == 0 && game.info.barbarianrate > 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=barbarian build units?", filename);
  }
  if (num_role_units(L_BARBARIAN_BOAT) == 0 && game.info.barbarianrate > 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=barbarian ship units?", filename);
  } else if (num_role_units(L_BARBARIAN_BOAT) > 0) {
    u = get_role_unit(L_BARBARIAN_BOAT,0);
    if(utype_move_type(u) != SEA_MOVING) {
      ruleset_error(LOG_FATAL,
                    "\"%s\": Barbarian boat (%s) needs to be a sea unit.",
                    filename,
                    utype_rule_name(u));
    }
  }
  if (num_role_units(L_BARBARIAN_SEA) == 0 && game.info.barbarianrate > 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No role=sea raider barbarian units?",
                  filename);
  }

  update_simple_ai_types();

  free(csec);
  free(sec);
  section_file_check_unused(file, filename);
  section_file_free(file);
}

/**************************************************************************
  ...  
**************************************************************************/
static void load_building_names(struct section_file *file)
{
  char **sec;
  int i, nval;
  const char *filename = secfile_filename(file);

  (void) section_file_lookup(file, "datafile.description");	/* unused */

  /* The names: */
  sec = secfile_get_secnames_prefix(file, BUILDING_SECTION_PREFIX, &nval);
  freelog(LOG_VERBOSE, "%d improvement types (including possibly unused)", nval);
  if (nval == 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No improvements?!?", filename);
  }
  if (nval > B_LAST) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many improvements (%d, max %d)",
                  filename, nval, B_LAST);
  }

  game.control.num_impr_types = nval;

  for (i = 0; i < nval; i++) {
    char *name = secfile_lookup_str(file, "%s.name", sec[i]);
    struct impr_type *b = improvement_by_number(i);

    name_strlcpy(b->name.vernacular, name);
    b->name.translated = NULL;
  }

  free(sec);
}

/**************************************************************************
...  
**************************************************************************/
static void load_ruleset_buildings(struct section_file *file)
{
  char **sec, *item;
  int i, nval;
  const char *filename = secfile_filename(file);

  (void) check_ruleset_capabilities(file, "+1.10.1", filename);

  sec = secfile_get_secnames_prefix(file, BUILDING_SECTION_PREFIX, &nval);

  for (i = 0; i < nval; i++) {
    struct impr_type *b = improvement_by_number(i);
    struct requirement_vector *reqs =
      lookup_req_list(file, sec[i], "reqs",
                      improvement_rule_name(b));
    char *sval, **slist;
    int j, nflags, ival;

    item = secfile_lookup_str(file, "%s.genus", sec[i]);
    b->genus = find_genus_by_rule_name(item);
    if (b->genus == IG_LAST) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" improvement \"%s\": couldn't match genus \"%s\".",
                    filename,
                    improvement_rule_name(b),
                    item);
    }

    slist = secfile_lookup_str_vec(file, &nflags, "%s.flags", sec[i]);
    b->flags = 0;

    for(j=0; j<nflags; j++) {
      sval = slist[j];
      if(strcmp(sval,"")==0) {
	continue;
      }
      ival = find_improvement_flag_by_rule_name(sval);
      if (ival==IF_LAST) {
	freelog(LOG_ERROR,
	        "\"%s\" improvement \"%s\": bad flag name \"%s\".",
		filename,
		improvement_rule_name(b),
		sval);
      } else {
        b->flags |= (1<<ival);
      }
    }
    free(slist);

    requirement_vector_copy(&b->reqs, reqs);

    b->obsolete_by = lookup_tech(file, sec[i], "obsolete_by", LOG_ERROR,
				 filename, b->name.vernacular);
    if (advance_by_number(A_NONE) == b->obsolete_by) {
      /* 
       * The ruleset can specify "None" for a never-obsoleted
       * improvement.  Currently this means A_NONE, which is an
       * unnecessary special-case.  We use A_NEVER to flag a
       * never-obsoleted improvement in the code instead.
       * (Test for valid_advance() later.)
       */
      b->obsolete_by = A_NEVER;
    }

    b->replaced_by = lookup_building(file, sec[i], "replaced_by", LOG_ERROR,
				     filename, b->name.vernacular);

    b->build_cost = secfile_lookup_int(file, "%s.build_cost", sec[i]);

    b->upkeep = secfile_lookup_int(file, "%s.upkeep", sec[i]);

    b->sabotage = secfile_lookup_int(file, "%s.sabotage", sec[i]);

    sz_strlcpy(b->graphic_str,
	       secfile_lookup_str_default(file, "-", "%s.graphic", sec[i]));
    sz_strlcpy(b->graphic_alt,
	    secfile_lookup_str_default(file, "-", "%s.graphic_alt", sec[i]));

    sz_strlcpy(b->soundtag,
	       secfile_lookup_str_default(file, "-", "%s.sound", sec[i]));
    sz_strlcpy(b->soundtag_alt,
	       secfile_lookup_str_default(file, "-", "%s.sound_alt",
					  sec[i]));
    b->helptext = lookup_helptext(file, sec[i]);

    b->allows_units = FALSE;
    unit_type_iterate(ut) {
      if (ut->need_improvement == b) {
        b->allows_units = TRUE;
        break;
      }
    } unit_type_iterate_end;
  }

  /* Some more consistency checking: */
  improvement_iterate(b) {
    if (valid_improvement(b)) {
      if (A_NEVER != b->obsolete_by
          && !valid_advance(b->obsolete_by)) {
        freelog(LOG_ERROR,
                "\"%s\" improvement \"%s\": obsoleted by removed tech \"%s\".",
                filename,
                improvement_rule_name(b),
                advance_rule_name(b->obsolete_by));
	b->obsolete_by = A_NEVER;
      }
    }
  } improvement_iterate_end;

  free(sec);
  section_file_check_unused(file, filename);
  section_file_free(file);
}

/**************************************************************************
  ...  
**************************************************************************/
static void load_terrain_names(struct section_file *file)
{
  int nval;
  char **sec;
  const char *filename = secfile_filename(file);

  (void) section_file_lookup(file, "datafile.description");	/* unused */

  /* terrain names */

  sec = secfile_get_secnames_prefix(file, TERRAIN_SECTION_PREFIX, &nval);
  if (nval == 0) {
    ruleset_error(LOG_FATAL, "\"%s\": ruleset doesn't have any terrains.",
                  filename);
  }
  if (nval > MAX_NUM_TERRAINS) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many terrains (%d, max %d)",
                  filename,
                  nval,
                  MAX_NUM_TERRAINS);
  }
  game.control.terrain_count = nval;

  /* avoid re-reading files */
  if (terrain_sections) {
    free(terrain_sections);
  }
  terrain_sections = fc_calloc(nval, MAX_SECTION_LABEL);

  terrain_type_iterate(pterrain) {
    const int i = terrain_index(pterrain);
    char *name = secfile_lookup_str(file, "%s.name", sec[i]);

    name_strlcpy(pterrain->name.vernacular, name);
    if (0 == strcmp(pterrain->name.vernacular, "unused")) {
      pterrain->name.vernacular[0] = '\0';
    }
    pterrain->name.translated = NULL;

    section_strlcpy(&terrain_sections[i * MAX_SECTION_LABEL], sec[i]);
  } terrain_type_iterate_end;

  free(sec);

  /* resource names */

  sec = secfile_get_secnames_prefix(file, RESOURCE_SECTION_PREFIX, &nval);
  if (nval > MAX_NUM_RESOURCES) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many resources (%d, max %d)",
                  filename,
                  nval,
                  MAX_NUM_RESOURCES);
  }
  game.control.resource_count = nval;

  /* avoid re-reading files */
  if (resource_sections) {
    free(resource_sections);
  }
  resource_sections = fc_calloc(nval, MAX_SECTION_LABEL);

  resource_type_iterate(presource) {
    const int i = resource_index(presource);
    char *name = secfile_lookup_str(file, "%s.name", sec[i]);

    name_strlcpy(presource->name.vernacular, name);
    if (0 == strcmp(presource->name.vernacular, "unused")) {
      presource->name.vernacular[0] = '\0';
    }
    presource->name.translated = NULL;

    section_strlcpy(&resource_sections[i * MAX_SECTION_LABEL], sec[i]);
  } resource_type_iterate_end;

  free(sec);

  /* base names */

  sec = secfile_get_secnames_prefix(file, BASE_SECTION_PREFIX, &nval);
  if (nval > MAX_BASE_TYPES) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many base types (%d, max %d)",
                  filename,
                  nval,
                  MAX_BASE_TYPES);
  }
  game.control.num_base_types = nval;

  if (base_sections) {
    free(base_sections);
  }
  base_sections = fc_calloc(nval, MAX_SECTION_LABEL);

  base_type_iterate(pbase) {
    const int i = base_index(pbase);
    char *name = secfile_lookup_str(file, "%s.name", sec[i]);

    name_strlcpy(pbase->name.vernacular, name);
    pbase->name.translated = NULL;

    section_strlcpy(&base_sections[i * MAX_SECTION_LABEL], sec[i]);
  } base_type_iterate_end;

  free(sec);
}

/**************************************************************************
...  
**************************************************************************/
static void load_ruleset_terrain(struct section_file *file)
{
  int nval;
  int j;
  char **res;
  const char *filename = secfile_filename(file);
  /* char *datafile_options = */ (void)
    check_ruleset_capabilities(file, TERRAIN_CAPABILITY, filename);

  /* options */
  terrain_control.may_road =
    secfile_lookup_bool_default(file, TRUE, "options.may_road");
  terrain_control.may_irrigate =
    secfile_lookup_bool_default(file, TRUE, "options.may_irrigate");
  terrain_control.may_mine =
    secfile_lookup_bool_default(file, TRUE, "options.may_mine");
  terrain_control.may_transform =
    secfile_lookup_bool_default(file, TRUE, "options.may_transform");

  /* parameters */

  terrain_control.ocean_reclaim_requirement_pct
    = secfile_lookup_int_default(file, 101,
				 "parameters.ocean_reclaim_requirement");
  terrain_control.land_channel_requirement_pct
    = secfile_lookup_int_default(file, 101,
				 "parameters.land_channel_requirement");
  terrain_control.lake_max_size
    = secfile_lookup_int_default(file, 0,
				 "parameters.lake_max_size");
  map.server.ocean_resources
    = secfile_lookup_bool_default(file, FALSE,
                                  "parameters.ocean_resources");
  terrain_control.river_move_mode =
    secfile_lookup_int_default(file, RMV_FAST_STRICT, "parameters.river_move_mode");
  terrain_control.river_defense_bonus =
    secfile_lookup_int_default(file, 50, "parameters.river_defense_bonus");
  terrain_control.river_trade_incr =
    secfile_lookup_int_default(file, 1, "parameters.river_trade_incr");
  {
    char *s = secfile_lookup_str_default(file, "",
      "parameters.river_help_text");
    sz_strlcpy(terrain_control.river_help_text, s);
  }

  terrain_control.road_superhighway_trade_bonus =
    secfile_lookup_int_default(file, 50, "parameters.road_superhighway_trade_bonus");
  output_type_iterate(o) {
    terrain_control.rail_tile_bonus[o] =
      secfile_lookup_int_default(file, 0, "parameters.rail_%s_bonus",
				 get_output_identifier(o));
    terrain_control.pollution_tile_penalty[o]
      = secfile_lookup_int_default(file, 50,
				   "parameters.pollution_%s_penalty",
				   get_output_identifier(o));
    terrain_control.fallout_tile_penalty[o]
      = secfile_lookup_int_default(file, 50,
				   "parameters.fallout_%s_penalty",
				   get_output_identifier(o));
  } output_type_iterate_end;

  /* terrain details */

  terrain_type_iterate(pterrain) {
    char **slist;
    const int i = terrain_index(pterrain);
    const char *tsection = &terrain_sections[i * MAX_SECTION_LABEL];

    sz_strlcpy(pterrain->graphic_str,
	       secfile_lookup_str(file,"%s.graphic", tsection));
    sz_strlcpy(pterrain->graphic_alt,
	       secfile_lookup_str(file,"%s.graphic_alt", tsection));

    pterrain->identifier
      = secfile_lookup_str(file, "%s.identifier", tsection)[0];
    if ('\0' == pterrain->identifier) {
      ruleset_error(LOG_FATAL, "\"%s\" [%s] identifier missing value.",
                    filename, tsection);
    }
    if (TERRAIN_UNKNOWN_IDENTIFIER == pterrain->identifier) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" [%s] cannot use '%c' as an identifier;"
                    " it is reserved for unknown terrain.",
                    filename, tsection, pterrain->identifier);
    }
    for (j = T_FIRST; j < i; j++) {
      if (pterrain->identifier == terrain_by_number(j)->identifier) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" [%s] has the same identifier as [%s].",
                      filename,
                      tsection,
                      &terrain_sections[j * MAX_SECTION_LABEL]);
      }
    }

    pterrain->movement_cost
      = secfile_lookup_int(file, "%s.movement_cost", tsection);
    pterrain->defense_bonus
      = secfile_lookup_int(file, "%s.defense_bonus", tsection);

    output_type_iterate(o) {
      pterrain->output[o]
	= secfile_lookup_int_default(file, 0, "%s.%s", tsection,
				     get_output_identifier(o));
    } output_type_iterate_end;

    res = secfile_lookup_str_vec(file, &nval, "%s.resources", tsection);
    pterrain->resources = fc_calloc(nval + 1, sizeof(*pterrain->resources));
    for (j = 0; j < nval; j++) {
      pterrain->resources[j] = lookup_resource(filename, res[j], tsection);
    }
    pterrain->resources[nval] = NULL;
    free(res);
    res = NULL;

    pterrain->road_trade_incr
      = secfile_lookup_int(file, "%s.road_trade_incr", tsection);
    pterrain->road_time = secfile_lookup_int(file, "%s.road_time", tsection);

    pterrain->irrigation_result
      = lookup_terrain(file, "irrigation_result", pterrain);
    pterrain->irrigation_food_incr
      = secfile_lookup_int(file, "%s.irrigation_food_incr", tsection);
    pterrain->irrigation_time
      = secfile_lookup_int(file, "%s.irrigation_time", tsection);

    pterrain->mining_result
      = lookup_terrain(file, "mining_result", pterrain);
    pterrain->mining_shield_incr
      = secfile_lookup_int(file, "%s.mining_shield_incr", tsection);
    pterrain->mining_time
      = secfile_lookup_int(file, "%s.mining_time", tsection);

    pterrain->transform_result
      = lookup_terrain(file, "transform_result", pterrain);
    pterrain->transform_time
      = secfile_lookup_int(file, "%s.transform_time", tsection);
    pterrain->rail_time
      = secfile_lookup_int_default(file, 3, "%s.rail_time", tsection);
    pterrain->clean_pollution_time
      = secfile_lookup_int_default(file, 3, "%s.clean_pollution_time", tsection);
    pterrain->clean_fallout_time
      = secfile_lookup_int_default(file, 3, "%s.clean_fallout_time", tsection);

    pterrain->warmer_wetter_result
      = lookup_terrain(file, "warmer_wetter_result", pterrain);
    pterrain->warmer_drier_result
      = lookup_terrain(file, "warmer_drier_result", pterrain);
    pterrain->cooler_wetter_result
      = lookup_terrain(file, "cooler_wetter_result", pterrain);
    pterrain->cooler_drier_result
      = lookup_terrain(file, "cooler_drier_result", pterrain);

    slist = secfile_lookup_str_vec(file, &nval, "%s.flags", tsection);
    BV_CLR_ALL(pterrain->flags);
    for (j = 0; j < nval; j++) {
      const char *sval = slist[j];
      enum terrain_flag_id flag = find_terrain_flag_by_rule_name(sval);

      if (flag == TER_LAST) {
        ruleset_error(LOG_FATAL, "\"%s\" [%s] has unknown flag \"%s\".",
                      filename, tsection, sval);
      } else {
	BV_SET(pterrain->flags, flag);
      }
    }
    free(slist);

    for (j = 0; j < MG_LAST; j++) {
      const char *mg_names[] = {
	"mountainous", "green", "foliage",
	"tropical", "temperate", "cold", "frozen",
	"wet", "dry", "ocean_depth"
      };
      assert(ARRAY_SIZE(mg_names) == MG_LAST);

      pterrain->property[j] = secfile_lookup_int_default(file, 0,
							 "%s.property_%s",
							 tsection, mg_names[j]);
    }

    slist = secfile_lookup_str_vec(file, &nval, "%s.native_to", tsection);
    BV_CLR_ALL(pterrain->native_to);
    for (j = 0; j < nval; j++) {
      struct unit_class *class = find_unit_class_by_rule_name(slist[j]);

      if (!class) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" [%s] is native to unknown unit class \"%s\".",
                      filename, tsection, slist[j]);
      } else if (is_ocean(pterrain) && class->move_type == LAND_MOVING) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" oceanic [%s] is native to land units.",
                      filename, tsection);
      } else if (!is_ocean(pterrain) && class->move_type == SEA_MOVING) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" non-oceanic [%s] is native to sea units.",
                      filename, tsection);
      } else {
        BV_SET(pterrain->native_to, uclass_index(class));
      }
    }
    free(slist);

    pterrain->helptext = lookup_helptext(file, tsection);
  } terrain_type_iterate_end;

  /* resource details */

  resource_type_iterate(presource) {
    char identifier[MAX_LEN_NAME];
    const int i = resource_index(presource);
    const char *rsection = &resource_sections[i * MAX_SECTION_LABEL];

    output_type_iterate (o) {
      presource->output[o] =
	  secfile_lookup_int_default(file, 0, "%s.%s", rsection,
				       get_output_identifier(o));
    } output_type_iterate_end;
    sz_strlcpy(presource->graphic_str,
	       secfile_lookup_str(file,"%s.graphic", rsection));
    sz_strlcpy(presource->graphic_alt,
	       secfile_lookup_str(file,"%s.graphic_alt", rsection));

    sz_strlcpy(identifier,
	       secfile_lookup_str(file,"%s.identifier", rsection));
    presource->identifier = identifier[0];
    if (RESOURCE_NULL_IDENTIFIER == presource->identifier) {
      ruleset_error(LOG_FATAL, "\"%s\" [%s] identifier missing value.",
                    filename, rsection);
    }
    if (RESOURCE_NONE_IDENTIFIER == presource->identifier) {
      ruleset_error(LOG_FATAL,
                    "\"%s\" [%s] cannot use '%c' as an identifier;"
                    " it is reserved.",
                    filename, rsection, presource->identifier);
    }
    for (j = 0; j < i; j++) {
      if (presource->identifier == resource_by_number(j)->identifier) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" [%s] has the same identifier as [%s].",
                      filename,
                      rsection,
                      &resource_sections[j * MAX_SECTION_LABEL]);
      }
    }

  } resource_type_iterate_end;

  /* base details */
  base_type_iterate(pbase) {
    BV_CLR_ALL(pbase->conflicts);
  } base_type_iterate_end;

  base_type_iterate(pbase) {
    const char *section = &base_sections[base_index(pbase) * MAX_SECTION_LABEL];
    int j;
    char **slist;
    struct requirement_vector *reqs;
    char *gui_str;

    pbase->buildable = secfile_lookup_bool_default(file, TRUE,
                                                  "%s.buildable", section);
    pbase->pillageable = secfile_lookup_bool_default(file, TRUE,
                                                     "%s.pillageable", section);

    sz_strlcpy(pbase->graphic_str,
               secfile_lookup_str_default(file, "-", "%s.graphic", section));
    sz_strlcpy(pbase->graphic_alt,
               secfile_lookup_str_default(file, "-",
                                          "%s.graphic_alt", section));
    sz_strlcpy(pbase->activity_gfx,
               secfile_lookup_str_default(file, "-",
                                          "%s.activity_gfx", section));

    reqs = lookup_req_list(file, section, "reqs", base_rule_name(pbase));
    requirement_vector_copy(&pbase->reqs, reqs);

    slist = secfile_lookup_str_vec(file, &nval, "%s.native_to", section);
    BV_CLR_ALL(pbase->native_to);
    for (j = 0; j < nval; j++) {
      struct unit_class *class = find_unit_class_by_rule_name(slist[j]);

      if (!class) {
        ruleset_error(LOG_FATAL,
                      "\"%s\" base \"%s\" is native to unknown unit class \"%s\".",
                      filename,
                      base_rule_name(pbase),
                      slist[j]);
      } else {
        BV_SET(pbase->native_to, uclass_index(class));
      }
    }
    free(slist);

    gui_str = secfile_lookup_str(file,"%s.gui_type", section);
    pbase->gui_type = base_gui_type_from_str(gui_str);
    if (pbase->gui_type == BASE_GUI_LAST) {
      ruleset_error(LOG_FATAL, "\"%s\" base \"%s\": unknown gui_type \"%s\".",
                    filename,
                    base_rule_name(pbase),
                    gui_str);
    }

    pbase->build_time = secfile_lookup_int(file, "%s.build_time", section);
    pbase->border_sq  = secfile_lookup_int_default(file, -1, "%s.border_sq",
                                                   section);
    pbase->vision_main_sq   = secfile_lookup_int_default(file, -1,
                                                         "%s.vision_main_sq",
                                                         section);
    pbase->vision_invis_sq  = secfile_lookup_int_default(file, -1,
                                                         "%s.vision_invis_sq",
                                                         section);
    pbase->defense_bonus  = secfile_lookup_int_default(file, 0,
                                                       "%s.defense_bonus",
                                                       section);

    slist = secfile_lookup_str_vec(file, &nval, "%s.flags", section);
    BV_CLR_ALL(pbase->flags);
    for (j = 0; j < nval; j++) {
      const char *sval = slist[j];
      enum base_flag_id flag = base_flag_from_str(sval);

      if (flag == BF_LAST) {
        ruleset_error(LOG_FATAL, "\"%s\" base \"%s\": unknown flag \"%s\".",
                      filename,
                      base_rule_name(pbase),
                      sval);
      } else {
        BV_SET(pbase->flags, flag);
      }
    }

    free(slist);

    slist = secfile_lookup_str_vec(file, &nval, "%s.conflicts", section);
    for (j = 0; j < nval; j++) {
      const char *sval = slist[j];
      struct base_type *pbase2 = find_base_type_by_rule_name(sval);

      if (pbase2 == NULL) {
        ruleset_error(LOG_FATAL, "\"%s\" base \"%s\": unknown conflict base \"%s\".",
                      filename,
                      base_rule_name(pbase),
                      sval);
      } else {
        BV_SET(pbase->conflicts, base_index(pbase2));
        BV_SET(pbase2->conflicts, base_index(pbase));
      }
    }
    
    free(slist);

    if (territory_claiming_base(pbase)) {
      base_type_iterate(pbase2) {
        if (pbase2 > pbase && territory_claiming_base(pbase2)) {
          BV_SET(pbase->conflicts, base_index(pbase2));
          BV_SET(pbase2->conflicts, base_index(pbase));
        }
      } base_type_iterate_end;
    }

  } base_type_iterate_end;

  section_file_check_unused(file, filename);
  section_file_free(file);
}

/**************************************************************************
  ...  
**************************************************************************/
static void load_government_names(struct section_file *file)
{
  int nval;
  char **sec;
  const char *filename = secfile_filename(file);

  (void) section_file_lookup(file, "datafile.description");	/* unused */

  sec = secfile_get_secnames_prefix(file, GOVERNMENT_SECTION_PREFIX, &nval);
  if (nval == 0) {
    ruleset_error(LOG_FATAL, "\"%s\": No governments?!?", filename);
  } else if(nval > G_MAGIC) {
    /* upper limit is really about 255 for 8-bit id values, but
       use G_MAGIC elsewhere as a sanity check, and should be plenty
       big enough --dwp */
    ruleset_error(LOG_FATAL, "\"%s\": Too many governments (%d, max %d)",
                  filename, nval, G_MAGIC);
  }
  governments_alloc(nval);

  /* Government names are needed early so that get_government_by_name will
   * work. */
  government_iterate(gov) {
    char *name = secfile_lookup_str(file, "%s.name",
                                    sec[government_index(gov)]);

    name_strlcpy(gov->name.vernacular, name);
    gov->name.translated = NULL;
  } government_iterate_end;
  free(sec);
}

/**************************************************************************
  This loads information from given governments.ruleset
**************************************************************************/
static void load_ruleset_governments(struct section_file *file)
{
  int nval;
  char **sec;
  const char *filename = secfile_filename(file);

  (void) check_ruleset_capabilities(file, "+1.9", filename);

  sec = secfile_get_secnames_prefix(file, GOVERNMENT_SECTION_PREFIX, &nval);

  game.government_during_revolution
    = lookup_government(file, "governments.during_revolution", filename);
  game.info.government_during_revolution_id =
    government_number(game.government_during_revolution);

  /* easy ones: */
  government_iterate(g) {
    const int i = government_index(g);
    struct requirement_vector *reqs =
      lookup_req_list(file, sec[i], "reqs", government_rule_name(g));

    if (section_file_lookup(file, "%s.ai_better", sec[i])) {
      char entry[100];

      my_snprintf(entry, sizeof(entry), "%s.ai_better", sec[i]);
      g->ai.better = lookup_government(file, entry, filename);
    } else {
      g->ai.better = NULL;
    }
    requirement_vector_copy(&g->reqs, reqs);
    
    sz_strlcpy(g->graphic_str,
	       secfile_lookup_str(file, "%s.graphic", sec[i]));
    sz_strlcpy(g->graphic_alt,
	       secfile_lookup_str(file, "%s.graphic_alt", sec[i]));

    g->helptext = lookup_helptext(file, sec[i]);
  } government_iterate_end;

  
  /* titles */
  government_iterate(g) {
    struct ruler_title *title;
    const int i = government_index(g);

    g->num_ruler_titles = 1;
    g->ruler_titles = fc_calloc(1, sizeof(*g->ruler_titles));
    title = &(g->ruler_titles[0]);

    title->nation = DEFAULT_TITLE;
    sz_strlcpy(title->male.vernacular,
	       secfile_lookup_str(file, "%s.ruler_male_title", sec[i]));
    title->male.translated = NULL;
    sz_strlcpy(title->female.vernacular,
	       secfile_lookup_str(file, "%s.ruler_female_title", sec[i]));
    title->female.translated = NULL;
  } government_iterate_end;

  free(sec);
  section_file_check_unused(file, filename);
  section_file_free(file);
}

/**************************************************************************
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
  struct packet_ruleset_control packet;

  packet = game.control;
  lsend_packet_ruleset_control(dest, &packet);
}

/**************************************************************************
This checks if nations[pos] leader names are not already defined in any 
previous nation, or twice in its own leader name table.
If not return NULL, if yes return pointer to name which is repeated
and id of a conflicting nation as second parameter.
**************************************************************************/
static char *check_leader_names(Nation_type_id nation,
             Nation_type_id *conflict_nation)
{
  int k;
  struct nation_type *pnation = nation_by_number(nation);

  for (k = 0; k < pnation->leader_count; k++) {
    char *leader = pnation->leaders[k].name;
    int i;
    Nation_type_id nation2;

    for (i = 0; i < k; i++) {
      if (0 == strcmp(leader, pnation->leaders[i].name)) {
        *conflict_nation = nation;
	return leader;
      }
    }

    for (nation2 = 0; nation2 < nation; nation2++) {
      struct nation_type *pnation2 = nation_by_number(nation2);

      for (i = 0; i < pnation2->leader_count; i++) {
	if (0 == strcmp(leader, pnation2->leaders[i].name)) {
	  *conflict_nation = nation2;
	  return leader;
	}
      }
    }
  }
  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
static void load_nation_names(struct section_file *file)
{
  char **sec;
  int j;

  (void) section_file_lookup(file, "datafile.description");	/* unused */

  sec = secfile_get_secnames_prefix(file, NATION_SECTION_PREFIX, &game.control.nation_count);
  nations_alloc(game.control.nation_count);

  nations_iterate(pl) {
    const int i = nation_index(pl);
    char *adjective   = secfile_lookup_str(file, "%s.name", sec[i]);
    char *noun_plural = secfile_lookup_str(file, "%s.plural", sec[i]);

    name_strlcpy(pl->adjective.vernacular, adjective);
    pl->adjective.translated = NULL;
    name_strlcpy(pl->noun_plural.vernacular, noun_plural);
    pl->noun_plural.translated = NULL;

    /* Check if nation name is already defined. */
    for(j = 0; j < i; j++) {
      struct nation_type *n2 = nation_by_number(j);

      if (0 == strcmp(n2->adjective.vernacular, pl->adjective.vernacular)
	|| 0 == strcmp(n2->noun_plural.vernacular, pl->noun_plural.vernacular)) {
        ruleset_error(LOG_FATAL,
                      "%s nation (the %s) defined twice; "
                      "in section nation%d and section nation%d",
                      adjective, noun_plural, j, i);
      }
    }
  } nations_iterate_end;
  free(sec);
}

/**************************************************************************
  This function loads a city name list from a section file.  The file and
  two section names (which will be concatenated) are passed in.  The
  malloc'ed city name list (which is all filled out) will be returned.
**************************************************************************/
static struct nation_city* load_city_name_list(struct section_file *file,
					     const char *secfile_str1,
					     const char *secfile_str2)
{
  int dim, j;
  struct nation_city *city_names;
  int value;

  /* First we read the strings from the section file (above). */
  char **cities = secfile_lookup_str_vec(file, &dim, "%s.%s",
                                         secfile_str1, secfile_str2);

  /*
   * Now we allocate enough room in the city_names array to store
   * all the name data.  The array is NULL-terminated by
   * having a NULL name at the end.
   */
  city_names = fc_calloc(dim + 1, sizeof(*city_names));
  city_names[dim].name = NULL;

  /*
   * Each string will be of the form
   * "<cityname> (<label>, <label>, ...)".  The cityname is just the
   * name for this city, while each "label" matches a terrain type
   * for the city (or "river"), with a preceeding ! to negate it.
   * The parentheses are optional (but necessary to have the
   * settings, of course).  Our job is now to parse this into the
   * nation_city structure.
   */
  for (j = 0, value = 1; j < dim; j++, value++) {
    char *name = strchr(cities[j], '(');

    /*
     * Now we wish to determine values for all of the city labels.
     * A value of 0 means no preference (which is necessary so that
     * the use of this is optional); -1 means the label is negated
     * and 1 means it's labelled.  Mostly the parsing just involves
     * a lot of ugly string handling...
     */
    memset(city_names[j].terrain, 0,
	   terrain_count() * sizeof(city_names[j].terrain[0]));
    city_names[j].river = 0;

    if (name) {
      /*
       * 0-terminate the original string, then find the
       * close-parenthesis so that we can make sure we stop there.
       */
      char *next = strchr(name + 1, ')');
      if (!next) {
	ruleset_error(LOG_ERROR,
		      "\"%s\" [%s] %s: city name \"%s\" unmatched parenthesis.",
		      secfile_filename(file),
		      secfile_str1,
		      secfile_str2,
		      cities[j]);
      } else { /* if (!next) */
        name[0] = next[0] = '\0';
        name++;

        /* Handle the labels one at a time. */
        do {
	  int setting;

	  next = strchr(name, ',');
	  if (next) {
	    next[0] = '\0';
	  }
	  remove_leading_trailing_spaces(name);
	
	  /*
	   * The ! is used to mark a negative, which is recorded
	   * with a -1.  Otherwise we use a 1.
	   */
	  if (name[0] == '!') {
	    name++;
	    setting = -1;
	  } else {
	    setting = 1;
	  }
	
	  if (mystrcasecmp(name, "river") == 0) {
	    city_names[j].river = setting;
	  } else {
	    /* "handled" tracks whether we find a match (for error handling) */
	    bool handled = FALSE;
	    int l = strlen(name);

	    if (l > 0  && 's' == my_tolower(name[l-1])) {
	      /* remove frequent trailing 's' */
	      name[--l] = '\0';
	    }

	    terrain_type_iterate(pterrain) {
	      const int i = terrain_index(pterrain);
	      const char *isection = &terrain_sections[i * MAX_SECTION_LABEL];
              /*
               * Note that the section name is unique (by definition).
               * The sub-strings are carefully crafted for this function.
               */
	      if (NULL != mystrcasestr(isection, name)) {
	        city_names[j].terrain[i] = setting;
	        handled = TRUE;
		break;
	      }
	    } terrain_type_iterate_end;
	    if (!handled) {
	      ruleset_error(LOG_ERROR,
			    "\"%s\" [%s] %s: terrain \"%s\" not found;"
			    " skipping it.",
			    secfile_filename(file),
			    secfile_str1,
			    secfile_str2,
			    name);
	    }
	  }
	  name = next ? next + 1 : NULL;
        } while (name && name[0] != '\0');
      } /* if (!next) */
    } /* if (name) */
    remove_leading_trailing_spaces(cities[j]);
    city_names[j].name = mystrdup(cities[j]);
    if (check_name(city_names[j].name)) {
      /* The ruleset contains a name that is too long.  This shouldn't
	 happen - if it does, the author should get immediate feedback */
      ruleset_error(LOG_ERROR, 
		    "\"%s\" [%s] %s: city name \"%s\" is too long;"
		    " shortening it.",
		    secfile_filename(file),
		    secfile_str1,
		    secfile_str2,
		    city_names[j].name);
      city_names[j].name[MAX_LEN_NAME - 1] = '\0';
    }
  }
  if (cities) {
    free(cities);
  }
  return city_names;
}

/**************************************************************************
Load nations.ruleset file
**************************************************************************/
static void load_ruleset_nations(struct section_file *file)
{
  char *govern;
  struct government *gov;
  int dim, i, j, k, nval, numgroups;
  char temp_name[MAX_LEN_NAME];
  char **sec, **civilwar_nations, **groups, **conflicts;
  char* name;
  int barb_land_count = 0;
  int barb_sea_count = 0;
  const char *filename = secfile_filename(file);

  (void) check_ruleset_capabilities(file, "+1.9", filename);
  
  groups = secfile_get_secnames_prefix(file, NATION_GROUP_SECTION_PREFIX, &numgroups);
  for (i = 0; i < numgroups; i++) {
    struct nation_group* group;
    name = secfile_lookup_str(file, "%s.name", groups[i]);
    group = add_new_nation_group(name);
    group->match = secfile_lookup_int(file, "%s.match", groups[i]);
  }
  free(groups);

  sec = secfile_get_secnames_prefix(file, NATION_SECTION_PREFIX, &nval);

  nations_iterate(pl) {
    const int i = nation_index(pl);
    char tmp[200] = "\0";
    char *barb_type;

    groups = secfile_lookup_str_vec(file, &dim, "%s.groups", sec[i]);
    pl->num_groups = dim;
    pl->groups = fc_calloc(dim + 1, sizeof(*(pl->groups)));

    for (j = 0; j < dim; j++) {
      pl->groups[j] = find_nation_group_by_rule_name(groups[j]);
      if (!pl->groups[j]) {
	freelog(LOG_ERROR, "Nation %s: Unknown group \"%s\".",
		nation_rule_name(pl),
		groups[j]);
      }
    }
    pl->groups[dim] = NULL; /* extra at end of list */
    free(groups);
    
    conflicts = 
      secfile_lookup_str_vec(file, &dim, "%s.conflicts_with", sec[i]);
    pl->num_conflicts = dim;
    pl->conflicts_with = fc_calloc(dim + 1, sizeof(*(pl->conflicts_with)));

    for (j = 0; j < dim; j++) {
      /* NO_NATION_SELECTED is allowed here */
      pl->conflicts_with[j] = find_nation_by_rule_name(conflicts[j]);
    }
    pl->conflicts_with[j] = NO_NATION_SELECTED; /* extra at end of list */
    free(conflicts);


    /* Nation leaders (backported from Freeciv 2.3.x). */
    for (j = 0; j < MAX_NUM_LEADERS; j++) {

      name = secfile_lookup_str(file, "%s.leaders%d.name", sec[i], j);
      if (NULL == name) {
        pl->leader_count = j;
        pl->leaders = fc_calloc(j, sizeof(*(pl->leaders)));

        break;
      }
    }

    for (j = 0; j < MAX_NUM_LEADERS; j++) {
      const char *sex;
      bool is_male = FALSE;

      name = secfile_lookup_str(file, "%s.leaders%d.name", sec[i], j);
      if (NULL == name) {
        /* No more to read. */
        break;
      }

      if (check_name(name)) {
        /* The ruleset contains a name that is too long. This shouldn't
         * happen - if it does, the author should get immediate feedback */
        sz_strlcpy(temp_name, name);
        /*ruleset_error(LOG_ERROR, "Nation %s: leader name \"%s\" "
                      "is too long; shortening it to \"%s\".",
                      nation_rule_name(pnation), name, temp_name);*/
        name = temp_name;
      }

      sex = secfile_lookup_str(file, "%s.leaders%d.sex", sec[i], j);
      if (NULL == sex) {
        /*ruleset_error(LOG_FATAL, "Nation %s: leader \"%s\": %s.",
                      nation_rule_name(pnation), name, secfile_error());*/
      } else if (0 == mystrcasecmp("Male", sex)) {
        is_male = TRUE;
      } else if (0 != mystrcasecmp("Female", sex)) {
        /*ruleset_error(LOG_FATAL, "Nation %s: leader \"%s\" has unsupported "
                      "sex variant \"%s\".",
                      nation_rule_name(pnation), name, sex);*/
      }

      pl->leaders[j].name = mystrdup(name);
      if (check_name(name)) {
	pl->leaders[j].name[MAX_LEN_NAME - 1] = '\0';
      }
      pl->leaders[j].is_male = is_male;

      //(void) nation_leader_new(pnation, name, is_male);
    }

    
    pl->is_available = secfile_lookup_bool_default(file, TRUE,
                                                   "%s.is_available", sec[i]);

    pl->is_playable = secfile_lookup_bool_default(file, TRUE,
						  "%s.is_playable", sec[i]);
    if (pl->is_playable) {
      server.playable_nations++;
    }

    /* Check barbarian type. Default is "None" meaning not a barbarian */    
    barb_type = secfile_lookup_str_default(file, "None",
                                           "%s.barbarian_type", sec[i]);
    if (mystrcasecmp(barb_type, "None") == 0) {
      pl->barb_type = NOT_A_BARBARIAN;
    } else if (mystrcasecmp(barb_type, "Land") == 0) {
      if (pl->is_playable) {
        /* We can't allow players to use barbarian nations, barbarians
         * may run out of nations */
        ruleset_error(LOG_FATAL,
                      "Nation %s marked both barbarian and playable.",
                      nation_rule_name(pl));
      }
      pl->barb_type = LAND_BARBARIAN;
      barb_land_count++;
    } else if (mystrcasecmp(barb_type, "Sea") == 0) {
      if (pl->is_playable) {
        /* We can't allow players to use barbarian nations, barbarians
         * may run out of nations */
        ruleset_error(LOG_FATAL,
                      "Nation %s marked both barbarian and playable.",
                      nation_rule_name(pl));
      }
      pl->barb_type = SEA_BARBARIAN;
      barb_sea_count++;
    } else {
      ruleset_error(LOG_FATAL,
                    "Nation %s, barbarian_type is \"%s\". Must be "
                    "\"None\" or \"Land\" or \"Sea\".",
                    nation_rule_name(pl),
                    barb_type);
    }

    /* Flags */

    sz_strlcpy(pl->flag_graphic_str,
	       secfile_lookup_str(file, "%s.flag", sec[i]));
    sz_strlcpy(pl->flag_graphic_alt,
	       secfile_lookup_str(file, "%s.flag_alt", sec[i]));

    /* Ruler titles */

    j = -1;
    while ((govern = secfile_lookup_str_default(file, NULL,
					   "%s.ruler_titles%d.government",
					   sec[i], ++j))) {
      char *male_name;
      char *female_name;
      
      male_name = secfile_lookup_str(file, "%s.ruler_titles%d.male_title",
				     sec[i], j);
      female_name = secfile_lookup_str(file, "%s.ruler_titles%d.female_title",
				       sec[i], j);

      gov = find_government_by_rule_name(govern);
      if (gov) {
	struct ruler_title *title;

	gov->num_ruler_titles++;
	gov->ruler_titles
	  = fc_realloc(gov->ruler_titles,
		       gov->num_ruler_titles * sizeof(*gov->ruler_titles));
	title = &(gov->ruler_titles[gov->num_ruler_titles-1]);

	title->nation = pl;

	name_strlcpy(title->male.vernacular, male_name);
	title->male.translated = NULL;

	name_strlcpy(title->female.vernacular, female_name);
	title->female.translated = NULL;
      } else {
	/* LOG_VERBOSE rather than LOG_ERROR so that can use single nation
	   ruleset file with variety of government ruleset files: */
        freelog(LOG_VERBOSE,
		"Nation %s: government \"%s\" not found.",
		nation_rule_name(pl),
		govern);
      }
    }

    /* City styles */

    sz_strlcpy(temp_name,
	       secfile_lookup_str(file, "%s.city_style", sec[i]));
    pl->city_style = find_city_style_by_rule_name(temp_name);
    if (pl->city_style < 0) {
      freelog(LOG_ERROR,
	      "Nation %s: city style \"%s\" is unknown, using default.", 
	      nation_rule_name(pl),
	      temp_name);
      pl->city_style = 0;
    }

    while (city_style_has_requirements(&city_styles[pl->city_style])) {
      if (pl->city_style == 0) {
	ruleset_error(LOG_FATAL,
                      "Nation %s: the default city style is not available "
                      "from the beginning!",
                      nation_rule_name(pl));
	/* Note that we can't use temp_name here. */
      }
      freelog(LOG_ERROR,
	      "Nation %s: city style \"%s\" is not available from beginning; "
	      "using default.",
	      nation_rule_name(pl),
	      temp_name);
      pl->city_style = 0;
    }

    /* Civilwar nations */

    civilwar_nations = secfile_lookup_str_vec(file, &dim,
					      "%s.civilwar_nations", sec[i]);
    pl->civilwar_nations = fc_calloc(dim + 1, sizeof(*(pl->civilwar_nations)));

    for (j = 0, k = 0; k < dim; j++, k++) {
      pl->civilwar_nations[j] = find_nation_by_rule_name(civilwar_nations[k]);

      /* No test for duplicate nations is performed.  If there is a duplicate
       * entry it will just cause that nation to have an increased probability
       * of being chosen. */

      if (pl->civilwar_nations[j] == NO_NATION_SELECTED) {
	j--;
	/* For nation authors, this would probably be considered an error.
	 * But it can happen normally.  The civ1 compatability ruleset only
	 * uses the nations that were in civ1, so not all of the links will
	 * exist. */
	freelog(LOG_VERBOSE,
		"Nation %s: civil war nation \"%s\" is unknown.",
		nation_rule_name(pl),
		civilwar_nations[k]);
      }
    }
    pl->civilwar_nations[j] = NO_NATION_SELECTED; /* end of list */
    free(civilwar_nations);

    /* Load nation specific initial items */
    lookup_tech_list(file, sec[i], "init_techs", pl->init_techs, filename);
    lookup_building_list(file, sec[i], "init_buildings", pl->init_buildings,
			 filename);
    lookup_unit_list(file, sec[i], "init_units", LOG_ERROR, pl->init_units,
                     filename);
    mystrlcat(tmp, sec[i], 200);
    mystrlcat(tmp, ".init_government", 200);
    pl->init_government = lookup_government(file, tmp, filename);

    /* read "normal" city names */

    pl->city_names = load_city_name_list(file, sec[i], "cities");

    pl->legend = mystrdup(secfile_lookup_str(file, "%s.legend", sec[i]));
    if (check_strlen(pl->legend, MAX_LEN_MSG, "Legend '%s' is too long")) {
      pl->legend[MAX_LEN_MSG - 1] = '\0';
    }

    pl->player = NULL;
  } nations_iterate_end;

  /* Calculate parent nations.  O(n^2) algorithm. */
  nations_iterate(pl) {
    struct nation_type *parents[nation_count()];
    int count = 0;

    nations_iterate(p2) {
      for (k = 0; p2->civilwar_nations[k] != NO_NATION_SELECTED; k++) {
	if (p2->civilwar_nations[k] == pl) {
	  parents[count] = p2;
	  count++;
	}
      }
    } nations_iterate_end;

    assert(sizeof(parents[0]) == sizeof(*pl->parent_nations));
    pl->parent_nations = fc_malloc((count + 1) * sizeof(parents[0]));
    memcpy(pl->parent_nations, parents, count * sizeof(parents[0]));
    pl->parent_nations[count] = NO_NATION_SELECTED;
  } nations_iterate_end;

  free(sec);
  section_file_check_unused(file, filename);
  section_file_free(file);

  if (barb_land_count == 0) {
    ruleset_error(LOG_FATAL,
                  "No land barbarian nation defined. At least one required!");
  }
  if (barb_sea_count == 0) {
    ruleset_error(LOG_FATAL,
                  "No sea barbarian nation defined. At least one required!");
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void load_citystyle_names(struct section_file *file)
{
  char **styles;
  int nval, i;

  (void) section_file_lookup(file, "datafile.description");	/* unused */

  /* The sections: */
  styles = secfile_get_secnames_prefix(file, CITYSTYLE_SECTION_PREFIX, &nval);
  city_styles_alloc(nval);

  /* Get names, so can lookup for replacements: */
  for (i = 0; i < game.control.styles_count; i++) {
    char *style_name = secfile_lookup_str(file, "%s.name", styles[i]);
    name_strlcpy(city_styles[i].name.vernacular, style_name);
    city_styles[i].name.translated = NULL;
  }
  free(styles);
}

/**************************************************************************
Load cities.ruleset file
**************************************************************************/
static void load_ruleset_cities(struct section_file *file)
{
  char **styles, **sec, *replacement;
  int i, nval;
  const char *filename = secfile_filename(file);
  char *item;

  (void) check_ruleset_capabilities(file, "+1.9", filename);

  /* Specialist options */
  sec = secfile_get_secnames_prefix(file, SPECIALIST_SECTION_PREFIX, &nval);
  if (nval >= SP_MAX) {
    ruleset_error(LOG_FATAL, "\"%s\": Too many specialists (%d, max %d).",
            filename, nval, SP_MAX);
  }
  game.control.num_specialist_types = nval;

  for (i = 0; i < nval; i++) {
    struct specialist *s = specialist_by_number(i);
    struct requirement_vector *reqs;

    item = secfile_lookup_str(file, "%s.name", sec[i]);
    sz_strlcpy(s->name.vernacular, item);
    s->name.translated = NULL;

    item = secfile_lookup_str_default(file, s->name.vernacular,
                                      "%s.short_name",
                                      sec[i]);
    sz_strlcpy(s->abbreviation.vernacular, item);
    s->abbreviation.translated = NULL;

    reqs = lookup_req_list(file, sec[i], "reqs", specialist_rule_name(s));
    requirement_vector_copy(&s->reqs, reqs);

    if (requirement_vector_size(&s->reqs) == 0 && DEFAULT_SPECIALIST == -1) {
      DEFAULT_SPECIALIST = i;
    }
  }
  if (DEFAULT_SPECIALIST == -1) {
    ruleset_error(LOG_FATAL,
                  "\"%s\": must give a min_size of 0 for at least one "
                  "specialist type.", filename);
  }
  free(sec);

  /* City Parameters */

  game.info.celebratesize = 
    secfile_lookup_int_default(file, GAME_DEFAULT_CELEBRATESIZE,
                               "parameters.celebrate_size_limit");
  game.info.add_to_size_limit =
    secfile_lookup_int_default(file, 9, "parameters.add_to_size_limit");
  game.info.angrycitizen =
    secfile_lookup_bool_default(file, GAME_DEFAULT_ANGRYCITIZEN,
                                "parameters.angry_citizens");

  game.info.changable_tax = 
    secfile_lookup_bool_default(file, TRUE, "parameters.changable_tax");
  game.info.forced_science = 
    secfile_lookup_int_default(file, 0, "parameters.forced_science");
  game.info.forced_luxury = 
    secfile_lookup_int_default(file, 100, "parameters.forced_luxury");
  game.info.forced_gold = 
    secfile_lookup_int_default(file, 0, "parameters.forced_gold");
  if (game.info.forced_science + game.info.forced_luxury
      + game.info.forced_gold != 100) {
    ruleset_error(LOG_FATAL,
                  "\"%s\": Forced taxes do not add up in ruleset!",
                  filename);
  }

  /* civ1 & 2 didn't reveal tiles */
  game.info.vision_reveal_tiles =
    secfile_lookup_bool_default(file, FALSE, "parameters.vision_reveal_tiles");

  /* City Styles ... */

  styles = secfile_get_secnames_prefix(file, CITYSTYLE_SECTION_PREFIX, &nval);

  /* Get rest: */
  for (i = 0; i < game.control.styles_count; i++) {
    struct requirement_vector *reqs;

    sz_strlcpy(city_styles[i].graphic, 
	       secfile_lookup_str(file, "%s.graphic", styles[i]));
    sz_strlcpy(city_styles[i].graphic_alt, 
	       secfile_lookup_str(file, "%s.graphic_alt", styles[i]));
    sz_strlcpy(city_styles[i].oceanic_graphic, 
	       secfile_lookup_str_default(file, "",
                                          "%s.oceanic_graphic", styles[i]));
    sz_strlcpy(city_styles[i].oceanic_graphic_alt, 
	       secfile_lookup_str_default(file, "",
                                          "%s.oceanic_graphic_alt", styles[i]));
    sz_strlcpy(city_styles[i].citizens_graphic,
	       secfile_lookup_str_default(file, "-", 
	    		"%s.citizens_graphic", styles[i]));
    sz_strlcpy(city_styles[i].citizens_graphic_alt, 
	       secfile_lookup_str_default(file, "generic", 
	    		"%s.citizens_graphic_alt", styles[i]));

    reqs = lookup_req_list(file, styles[i], "reqs", city_style_rule_name(i));
    requirement_vector_copy(&city_styles[i].reqs, reqs);

    replacement = secfile_lookup_str(file, "%s.replaced_by", styles[i]);
    if( strcmp(replacement, "-") == 0) {
      city_styles[i].replaced_by = -1;
    } else {
      city_styles[i].replaced_by = find_city_style_by_rule_name(replacement);
      if (city_styles[i].replaced_by < 0) {
        freelog(LOG_ERROR, "\"%s\": style \"%s\" replacement \"%s\" not found",
                filename,
                city_style_rule_name(i),
                replacement);
      }
    }
  }
  free(styles);

  section_file_check_unused(file, filename);
  section_file_free(file);
}

/**************************************************************************
Load effects.ruleset file
**************************************************************************/
static void load_ruleset_effects(struct section_file *file)
{
  char **sec, *type;
  int i, nval;
  const char *filename;

  filename = secfile_filename(file);
  (void) check_ruleset_capabilities(file, "+1.0", filename);
  (void) section_file_lookup(file, "datafile.description");	/* unused */

  /* Parse effects and add them to the effects ruleset cache. */
  sec = secfile_get_secnames_prefix(file, EFFECT_SECTION_PREFIX, &nval);
  for (i = 0; i < nval; i++) {
    enum effect_type eff;
    int value;
    struct effect *peffect;

    type = secfile_lookup_str(file, "%s.name", sec[i]);

    if ((eff = effect_type_from_str(type)) == EFT_LAST) {
      freelog(LOG_ERROR,
              "\"%s\" [%s] lists unknown effect type \"%s\".",
              filename, sec[i], type);
      continue;
    }

    value = secfile_lookup_int_default(file, 1, "%s.value", sec[i]);

    peffect = effect_new(eff, value);

    requirement_vector_iterate(lookup_req_list(file, sec[i], "reqs", type),
                               req) {
      struct requirement *preq = fc_malloc(sizeof(*preq));

      *preq = *req;
      effect_req_append(peffect, FALSE, preq);
    } requirement_vector_iterate_end;
    requirement_vector_iterate(lookup_req_list(file, sec[i], "nreqs", type),
                               req) {
      struct requirement *preq = fc_malloc(sizeof(*preq));

      *preq = *req;
      effect_req_append(peffect, TRUE, preq);
    } requirement_vector_iterate_end;
  }
  free(sec);

  section_file_check_unused(file, filename);
  section_file_free(file);
}

/**************************************************************************
  Load ruleset file.
**************************************************************************/
static void load_ruleset_game(void)
{
  struct section_file file;
  char *sval, **svec;
  const char *filename;
  int *food_ini;
  int i;
  char *text;

  openload_ruleset_file(&file, "game");
  filename = secfile_filename(&file);

  /* section: datafile */
  (void) check_ruleset_capabilities(&file, "+1.11.1", filename);
  (void) section_file_lookup(&file, "datafile.description");	/* unused */

  /* section: tileset */
  text = secfile_lookup_str_default(&file, "", "tileset.prefered");
  if (text[0] != '\0') {
    /* There was tileset suggestion */
    sz_strlcpy(game.control.prefered_tileset, text);
  } else {
    /* No tileset suggestions */
    game.control.prefered_tileset[0] = '\0';
  }

  /* section: about */
  text = secfile_lookup_str(&file, "about.name");
  /* Ruleset/modpack name found */
  sz_strlcpy(game.control.name, text);

  text = secfile_lookup_str_default(&file, "", "about.description");
  if (text[0] != '\0') {
    /* Ruleset/modpack description found */
    sz_strlcpy(game.control.description, text);
  } else {
    /* No description */
    game.control.description[0] = '\0';
  }

  /* section: options */
  lookup_tech_list(&file, "options", "global_init_techs",
                   game.server.rgame.global_init_techs, filename);
  lookup_building_list(&file, "options", "global_init_buildings",
                       game.server.rgame.global_init_buildings, filename);

  /* section: civstyle */
  game.info.base_pollution
    = secfile_lookup_int_default(&file, RS_DEFAULT_BASE_POLLUTION,
                                 "civstyle.base_pollution");
  game.info.happy_cost
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_HAPPY_COST,
                                         RS_MIN_HAPPY_COST,
                                         RS_MAX_HAPPY_COST,
                                         "civstyle.happy_cost");
  game.info.food_cost
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_FOOD_COST,
                                         RS_MIN_FOOD_COST,
                                         RS_MAX_FOOD_COST,
                                         "civstyle.food_cost");
  /* TODO: move to global_unit_options */
  game.info.base_bribe_cost
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_BASE_BRIBE_COST,
                                         RS_MIN_BASE_BRIBE_COST,
                                         RS_MAX_BASE_BRIBE_COST,
                                         "civstyle.base_bribe_cost");
  /* TODO: move to global_unit_options */
  game.info.ransom_gold
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_RANSOM_GOLD,
                                         RS_MIN_RANSOM_GOLD,
                                         RS_MAX_RANSOM_GOLD,
                                         "civstyle.ransom_gold");
  /* TODO: move to global_unit_options */
  game.info.pillage_select
    = secfile_lookup_bool_default(&file, RS_DEFAULT_PILLAGE_SELECT,
                                         "civstyle.pillage_select");
  /* TODO: move to global_unit_options */
  game.info.upgrade_veteran_loss
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_UPGRADE_VETERAN_LOSS,
                                         RS_MIN_UPGRADE_VETERAN_LOSS,
                                         RS_MAX_UPGRADE_VETERAN_LOSS,
                                         "civstyle.upgrade_veteran_loss");
  /* TODO: move to global_unit_options */
  game.info.autoupgrade_veteran_loss
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_UPGRADE_VETERAN_LOSS,
                                         RS_MIN_UPGRADE_VETERAN_LOSS,
                                         RS_MAX_UPGRADE_VETERAN_LOSS,
                                         "civstyle.autoupgrade_veteran_loss");
  /* TODO: move to new section research */
  game.info.base_tech_cost
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_BASE_TECH_COST,
                                         RS_MIN_BASE_TECH_COST,
                                         RS_MAX_BASE_TECH_COST,
                                         "civstyle.base_tech_cost");

  food_ini = secfile_lookup_int_vec(&file, &game.info.granary_num_inis,
                                    "civstyle.granary_food_ini");
  if (game.info.granary_num_inis > MAX_GRANARY_INIS) {
    ruleset_error(LOG_FATAL,
                  "Too many granary_food_ini entries (%d, max %d)",
                  game.info.granary_num_inis, MAX_GRANARY_INIS);
  } else if (game.info.granary_num_inis == 0) {
    freelog(LOG_ERROR, "No values for granary_food_ini. Using default "
                       "value %d.", RS_DEFAULT_GRANARY_FOOD_INI);
    game.info.granary_num_inis = 1;
    game.info.granary_food_ini[0] = RS_DEFAULT_GRANARY_FOOD_INI;
  } else {
    int i;

    /* check for <= 0 entries */
    for (i = 0; i < game.info.granary_num_inis; i++) {
      if (food_ini[i] <= 0) {
        if (i == 0) {
          food_ini[i] = RS_DEFAULT_GRANARY_FOOD_INI;
        } else {
          food_ini[i] = food_ini[i - 1];
        }
        freelog(LOG_ERROR, "Bad value for granary_food_ini[%i]. Using %i.",
                i, food_ini[i]);
      }
      game.info.granary_food_ini[i] = food_ini[i];
    }
  }
  free(food_ini);

  game.info.granary_food_inc
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_GRANARY_FOOD_INC,
                                         RS_MIN_GRANARY_FOOD_INC,
                                         RS_MAX_GRANARY_FOOD_INC,
                                         "civstyle.granary_food_inc");

  output_type_iterate(o) {
    game.info.min_city_center_output[o]
      = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                           RS_DEFAULT_CITY_CENTER_OUTPUT,
                                           RS_MIN_CITY_CENTER_OUTPUT,
                                           RS_MAX_CITY_CENTER_OUTPUT,
                                           "civstyle.min_city_center_%s",
                                           get_output_identifier(o));
  } output_type_iterate_end;

  sval = secfile_lookup_str(&file, "civstyle.nuke_contamination" );
  if (mystrcasecmp(sval, "Pollution") == 0) {
    game.info.nuke_contamination = CONTAMINATION_POLLUTION;
  } else if (mystrcasecmp(sval, "Fallout") == 0) {
    game.info.nuke_contamination = CONTAMINATION_FALLOUT;
  } else {
    freelog(LOG_ERROR, "Bad value %s for nuke_contamination. Using "
            "\"Pollution\".", sval);
    game.info.nuke_contamination = CONTAMINATION_POLLUTION;
  }

  /* This only takes effect if citymindist is set to 0. */
  game.info.min_dist_bw_cities
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_CITIES_MIN_DIST,
                                         RS_MIN_CITIES_MIN_DIST,
                                         RS_MAX_CITIES_MIN_DIST,
                                         "civstyle.min_dist_bw_cities");
  game.info.init_vis_radius_sq
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_VIS_RADIUS_SQ,
                                         RS_MIN_VIS_RADIUS_SQ,
                                         RS_MAX_VIS_RADIUS_SQ,
                                         "civstyle.init_vis_radius_sq");

  game.info.gold_upkeep_style
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_GOLD_UPKEEP_STYLE,
                                         RS_MIN_GOLD_UPKEEP_STYLE,
                                         RS_MAX_GOLD_UPKEEP_STYLE,
                                         "civstyle.gold_upkeep_style");

  /* TODO: move to new section research */
  game.info.tech_cost_style
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_TECH_COST_STYLE,
                                         RS_MIN_TECH_COST_STYLE,
                                         RS_MAX_TECH_COST_STYLE,
                                         "civstyle.tech_cost_style");
  /* TODO: move to new section research */
  game.info.tech_leakage
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_TECH_LEAKAGE,
                                         RS_MIN_TECH_LEAKAGE,
                                         RS_MAX_TECH_LEAKAGE,
                                         "civstyle.tech_leakage");
  if (game.info.tech_cost_style == 0 && game.info.tech_leakage != 0) {
    freelog(LOG_ERROR,
            "Only tech_leakage 0 supported with tech_cost_style 0.");
    freelog(LOG_ERROR, "Switching to tech_leakage 0.");
    game.info.tech_leakage = 0;
  }

  /* section: illness */
  game.info.illness_on
    = secfile_lookup_bool_default(&file, RS_DEFAULT_ILLNESS_ON,
                                  "illness.illness_on");
  game.info.illness_base_factor
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_ILLNESS_BASE_FACTOR,
                                         RS_MIN_ILLNESS_BASE_FACTOR,
                                         RS_MAX_ILLNESS_BASE_FACTOR,
                                         "illness.illness_base_factor");
  game.info.illness_min_size
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_ILLNESS_MIN_SIZE,
                                         RS_MIN_ILLNESS_MIN_SIZE,
                                         RS_MAX_ILLNESS_MIN_SIZE,
                                         "illness.illness_min_size");
  game.info.illness_trade_infection
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_ILLNESS_TRADE_INFECTION_PCT,
                                         RS_MIN_ILLNESS_TRADE_INFECTION_PCT,
                                         RS_MAX_ILLNESS_TRADE_INFECTION_PCT,
                                         "illness.illness_trade_infection");
  game.info.illness_pollution_factor
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_ILLNESS_POLLUTION_PCT,
                                         RS_MIN_ILLNESS_POLLUTION_PCT,
                                         RS_MAX_ILLNESS_POLLUTION_PCT,
                                         "illness.illness_pollution_factor");

  /* section: incite_cost */
  game.info.base_incite_cost
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_INCITE_BASE_COST,
                                         RS_MIN_INCITE_BASE_COST,
                                         RS_MAX_INCITE_BASE_COST,
                                         "incite_cost.base_incite_cost");
  game.info.incite_improvement_factor
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_INCITE_IMPROVEMENT_FCT,
                                         RS_MIN_INCITE_IMPROVEMENT_FCT,
                                         RS_MAX_INCITE_IMPROVEMENT_FCT,
                                         "incite_cost.improvement_factor");
  game.info.incite_unit_factor
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_INCITE_UNIT_FCT,
                                         RS_MIN_INCITE_UNIT_FCT,
                                         RS_MAX_INCITE_UNIT_FCT,
                                         "incite_cost.unit_factor");
  game.info.incite_total_factor
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_INCITE_TOTAL_FCT,
                                         RS_MIN_INCITE_TOTAL_FCT,
                                         RS_MAX_INCITE_TOTAL_FCT,
                                         "incite_cost.total_factor");

  /* section: global_unit_options */
  game.info.slow_invasions
    = secfile_lookup_bool_default(&file, RS_DEFAULT_SLOW_INVASIONS,
                                  "global_unit_options.slow_invasions");

  /* section: combat_rules */
  game.info.killstack
    = secfile_lookup_bool_default(&file, RS_DEFAULT_KILLSTACK,
                                  "combat_rules.killstack");
  game.info.tired_attack
    = secfile_lookup_bool_default(&file, RS_DEFAULT_TIRED_ATTACK,
                                  "combat_rules.tired_attack");

  /* section: borders */
  game.info.border_city_radius_sq
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_BORDER_RADIUS_SQ_CITY,
                                         RS_MIN_BORDER_RADIUS_SQ_CITY,
                                         RS_MAX_BORDER_RADIUS_SQ_CITY,
                                         "borders.radius_sq_city");
  game.info.border_size_effect
    = secfile_lookup_int_default_min_max(ruleset_error, &file,
                                         RS_DEFAULT_BORDER_SIZE_EFFECT,
                                         RS_MIN_BORDER_SIZE_EFFECT,
                                         RS_MAX_BORDER_SIZE_EFFECT,
                                         "borders.size_effect");

  /* section: calendar */
  game.info.calendar_skip_0
    = secfile_lookup_bool_default(&file, RS_DEFAULT_CALENDAR_SKIP_0,
                                  "calendar.skip_year_0");
  game.info.start_year
    = secfile_lookup_int_default(&file, GAME_START_YEAR,
                                 "calendar.start_year");
  sz_strlcpy(game.info.positive_year_label,
             _(secfile_lookup_str_default(&file,
                                          RS_DEFAULT_POS_YEAR_LABEL,
                                          "calendar.positive_label")));
  sz_strlcpy(game.info.negative_year_label,
             _(secfile_lookup_str_default(&file,
                                          RS_DEFAULT_NEG_YEAR_LABEL,
                                          "calendar.negative_label")));

  /* section: teams */
  svec = secfile_lookup_str_vec(&file, &game.info.num_teams, "teams.names");
  game.info.num_teams = MIN(MAX_NUM_TEAMS, game.info.num_teams);
  if (game.info.num_teams <= 0) {
    ruleset_error(LOG_FATAL, "Missing team names in game.ruleset.");
  }
  for (i = 0; i < game.info.num_teams; i++) {
    sz_strlcpy(game.info.team_names_orig[i], svec[i]);
  }
  free(svec);

  section_file_check_unused(&file, filename);
  section_file_free(&file);
}

/**************************************************************************
  Send the units ruleset information (all individual unit classes) to the
  specified connections.
**************************************************************************/
static void send_ruleset_unit_classes(struct conn_list *dest)
{
  struct packet_ruleset_unit_class packet;

  unit_class_iterate(c) {
    packet.id = uclass_number(c);
    sz_strlcpy(packet.name, c->name.vernacular);
    packet.move_type = c->move_type;
    packet.min_speed = c->min_speed;
    packet.hp_loss_pct = c->hp_loss_pct;
    packet.hut_behavior = c->hut_behavior;
    packet.flags = c->flags;

    lsend_packet_ruleset_unit_class(dest, &packet);
  } unit_class_iterate_end;
}

/**************************************************************************
  Send the units ruleset information (all individual units) to the
  specified connections.
**************************************************************************/
static void send_ruleset_units(struct conn_list *dest)
{
  struct packet_ruleset_unit packet;
  int i;

  unit_type_iterate(u) {
    packet.id = utype_number(u);
    sz_strlcpy(packet.name, u->name.vernacular);
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
                              ? advance_number(u->require_advance) : -1;
    packet.impr_requirement = u->need_improvement
                              ? improvement_number(u->need_improvement) : -1;
    packet.gov_requirement = u->need_government
                             ? government_number(u->need_government) : -1;
    packet.vision_radius_sq = u->vision_radius_sq;
    packet.transport_capacity = u->transport_capacity;
    packet.hp = u->hp;
    packet.firepower = u->firepower;
    packet.obsoleted_by = u->obsoleted_by
                          ? utype_number(u->obsoleted_by) : -1;
    packet.transformed_to = u->transformed_to
                            ? utype_number(u->transformed_to) : -1;
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
    packet.cargo = u->cargo;
    packet.targets = u->targets;
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      sz_strlcpy(packet.veteran_name[i], u->veteran[i].name);
      packet.power_fact[i] = u->veteran[i].power_fact;
      packet.move_bonus[i] = u->veteran[i].move_bonus;
    }
    if (u->helptext) {
      sz_strlcpy(packet.helptext, u->helptext);
    } else {
      packet.helptext[0] = '\0';
    }

    lsend_packet_ruleset_unit(dest, &packet);
  } unit_type_iterate_end;
}

/**************************************************************************
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
    sz_strlcpy(packet.name, s->name.vernacular);
    sz_strlcpy(packet.short_name, s->abbreviation.vernacular);
    j = 0;
    requirement_vector_iterate(&s->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;

    lsend_packet_ruleset_specialist(dest, &packet);
  } specialist_type_iterate_end;
}

/**************************************************************************
  Send the techs ruleset information (all individual advances) to the
  specified connections.
**************************************************************************/
static void send_ruleset_techs(struct conn_list *dest)
{
  struct packet_ruleset_tech packet;

  advance_iterate(A_NONE, a) {
    packet.id = advance_number(a);
    sz_strlcpy(packet.name, a->name.vernacular);
    sz_strlcpy(packet.graphic_str, a->graphic_str);
    sz_strlcpy(packet.graphic_alt, a->graphic_alt);

    packet.req[AR_ONE] = a->require[AR_ONE]
                         ? advance_number(a->require[AR_ONE]) : -1;
    packet.req[AR_TWO] = a->require[AR_TWO]
                         ? advance_number(a->require[AR_TWO]) : -1;
    packet.root_req = a->require[AR_ROOT]
                      ? advance_number(a->require[AR_ROOT]) : -1;

    packet.flags = a->flags;
    packet.preset_cost = a->preset_cost;
    packet.num_reqs = a->num_reqs;
    if (a->helptext) {
      sz_strlcpy(packet.helptext, a->helptext);
    } else {
      packet.helptext[0] = '\0';
    }

    lsend_packet_ruleset_tech(dest, &packet);
  } advance_iterate_end;
}

/**************************************************************************
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
    sz_strlcpy(packet.name, b->name.vernacular);
    sz_strlcpy(packet.graphic_str, b->graphic_str);
    sz_strlcpy(packet.graphic_alt, b->graphic_alt);
    j = 0;
    requirement_vector_iterate(&b->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;
    packet.obsolete_by = b->obsolete_by
                         ? advance_number(b->obsolete_by) : -1;
    packet.replaced_by = b->replaced_by
                         ? improvement_number(b->replaced_by) : -1;
    packet.build_cost = b->build_cost;
    packet.upkeep = b->upkeep;
    packet.sabotage = b->sabotage;
    packet.flags = b->flags;
    sz_strlcpy(packet.soundtag, b->soundtag);
    sz_strlcpy(packet.soundtag_alt, b->soundtag_alt);

    if (b->helptext) {
      sz_strlcpy(packet.helptext, b->helptext);
    } else {
      packet.helptext[0] = '\0';
    }

    lsend_packet_ruleset_building(dest, &packet);
  } improvement_iterate_end;
}

/**************************************************************************
  Send the terrain ruleset information (terrain_control, and the individual
  terrain types) to the specified connections.
**************************************************************************/
static void send_ruleset_terrain(struct conn_list *dest)
{
  struct packet_ruleset_terrain packet;

  lsend_packet_ruleset_terrain_control(dest, &terrain_control);

  terrain_type_iterate(pterrain) {
    struct resource **r;

    packet.id = terrain_number(pterrain);
    packet.native_to = pterrain->native_to;

    sz_strlcpy(packet.name_orig, pterrain->name.vernacular);
    sz_strlcpy(packet.graphic_str, pterrain->graphic_str);
    sz_strlcpy(packet.graphic_alt, pterrain->graphic_alt);

    packet.movement_cost = pterrain->movement_cost;
    packet.defense_bonus = pterrain->defense_bonus;

    output_type_iterate(o) {
      packet.output[o] = pterrain->output[o];
    } output_type_iterate_end;

    packet.num_resources = 0;
    for (r = pterrain->resources; *r; r++) {
      packet.resources[packet.num_resources++] = resource_number(*r);
    }

    packet.road_trade_incr = pterrain->road_trade_incr;
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

    packet.transform_result = (pterrain->transform_result
			       ? terrain_number(pterrain->transform_result)
			       : terrain_count());
    packet.transform_time = pterrain->transform_time;
    packet.rail_time = pterrain->rail_time;
    packet.clean_pollution_time = pterrain->clean_pollution_time;
    packet.clean_fallout_time = pterrain->clean_fallout_time;

    packet.flags = pterrain->flags;

    if (pterrain->helptext) {
      sz_strlcpy(packet.helptext, pterrain->helptext);
    } else {
      packet.helptext[0] = '\0';
    }

    lsend_packet_ruleset_terrain(dest, &packet);
  } terrain_type_iterate_end;
}

/****************************************************************************
  Send the resource ruleset information to the specified connections.
****************************************************************************/
static void send_ruleset_resources(struct conn_list *dest)
{
  struct packet_ruleset_resource packet;

  resource_type_iterate (presource) {
    packet.id = resource_number(presource);

    sz_strlcpy(packet.name_orig, presource->name.vernacular);
    sz_strlcpy(packet.graphic_str, presource->graphic_str);
    sz_strlcpy(packet.graphic_alt, presource->graphic_alt);

    output_type_iterate(o) {
      packet.output[o] = presource->output[o];
    } output_type_iterate_end;

    lsend_packet_ruleset_resource(dest, &packet);
  } resource_type_iterate_end;
}

/**************************************************************************
  Send the base ruleset information (all individual base types) to the
  specified connections.
**************************************************************************/
static void send_ruleset_bases(struct conn_list *dest)
{
  struct packet_ruleset_base packet;

  base_type_iterate(b) {
    int j;

    packet.id = base_number(b);
    sz_strlcpy(packet.name, b->name.vernacular);
    sz_strlcpy(packet.graphic_str, b->graphic_str);
    sz_strlcpy(packet.graphic_alt, b->graphic_alt);
    sz_strlcpy(packet.activity_gfx, b->activity_gfx);
    packet.buildable = b->buildable;
    packet.pillageable = b->pillageable;

    j = 0;
    requirement_vector_iterate(&b->reqs, preq) {
      packet.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    packet.reqs_count = j;
    packet.native_to = b->native_to;

    packet.gui_type = b->gui_type;
    packet.build_time = b->build_time;
    packet.defense_bonus = b->defense_bonus;
    packet.border_sq = b->border_sq;
    packet.vision_main_sq = b->vision_main_sq;
    packet.vision_invis_sq = b->vision_invis_sq;

    packet.flags = b->flags;

    lsend_packet_ruleset_base(dest, &packet);
  } base_type_iterate_end;
}

/**************************************************************************
  Send the government ruleset information to the specified connections.
  One packet per government type, and for each type one per ruler title.
**************************************************************************/
static void send_ruleset_governments(struct conn_list *dest)
{
  struct packet_ruleset_government gov;
  struct packet_ruleset_government_ruler_title title;
  struct ruler_title *p_title;
  int j;

  government_iterate(g) {
    /* send one packet_government */
    gov.id = government_number(g);

    j = 0;
    /*requirement_vector_iterate(&g->reqs, preq) {
      gov.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    gov.reqs_count = j;*/

    gov.num_ruler_titles = g->num_ruler_titles;

    sz_strlcpy(gov.name, g->name.vernacular);
    sz_strlcpy(gov.graphic_str, g->graphic_str);
    sz_strlcpy(gov.graphic_alt, g->graphic_alt);
    
    if (g->helptext) {
      sz_strlcpy(gov.helptext, g->helptext);
    } else {
      gov.helptext[0] = '\0';
    }
      
    lsend_packet_ruleset_government(dest, &gov);
    
    /* send one packet_government_ruler_title per ruler title */
    for(j=0; j<g->num_ruler_titles; j++) {
      p_title = &g->ruler_titles[j];

      title.gov = government_number(g);
      title.id = j;
      title.nation = p_title->nation ? nation_number(p_title->nation) : -1;
      sz_strlcpy(title.male_title, p_title->male.vernacular);
      sz_strlcpy(title.female_title, p_title->female.vernacular);
    
      lsend_packet_ruleset_government_ruler_title(dest, &title);
    }
  } government_iterate_end;
}

/**************************************************************************
  Send the nations ruleset information (info on each nation) to the
  specified connections.
**************************************************************************/
static void send_ruleset_nations(struct conn_list *dest)
{
  struct packet_ruleset_nation packet;
  struct packet_ruleset_nation_groups groups_packet;
  //struct nation_type *n;
  int i;

  groups_packet.ngroups = nation_group_count();
  nation_groups_iterate(pgroup) {
    sz_strlcpy(groups_packet.groups[nation_group_index(pgroup)], pgroup->name);
  } nation_groups_iterate_end;
  lsend_packet_ruleset_nation_groups(dest, &groups_packet);

  //assert(sizeof(packet.init_techs) == sizeof(n->init_techs));
  //assert(ARRAY_SIZE(packet.init_techs) == ARRAY_SIZE(n->init_techs));

  nations_iterate(n) {
    packet.id = nation_number(n);
    sz_strlcpy(packet.adjective, n->adjective.vernacular);
    sz_strlcpy(packet.noun_plural, n->noun_plural.vernacular);
    sz_strlcpy(packet.graphic_str, n->flag_graphic_str);
    sz_strlcpy(packet.graphic_alt, n->flag_graphic_alt);
    packet.leader_count = n->leader_count;
    for(i=0; i < n->leader_count; i++) {
      sz_strlcpy(packet.leader_name[i], n->leaders[i].name);
      packet.leader_sex[i] = n->leaders[i].is_male;
    }
    packet.city_style = n->city_style;
    packet.is_playable = n->is_playable;
    packet.is_available = n->is_available;
    packet.barbarian_type = n->barb_type;
    //memcpy(packet.init_techs, n->init_techs, sizeof(packet.init_techs));
    memcpy(packet.init_buildings, n->init_buildings, 
           sizeof(packet.init_buildings));
    memcpy(packet.init_units, n->init_units, 
           sizeof(packet.init_units));
    packet.init_government = government_number(n->init_government);

    sz_strlcpy(packet.legend, n->legend);

     /* client needs only the names */
     packet.ngroups = n->num_groups;
     for (i = 0; i < n->num_groups; i++) {
       packet.groups[i] = nation_group_number(n->groups[i]);
     }

    lsend_packet_ruleset_nation(dest, &packet);
  } nations_iterate_end;
}

/**************************************************************************
  Send the city-style ruleset information (each style) to the specified
  connections.
**************************************************************************/
static void send_ruleset_cities(struct conn_list *dest)
{
  struct packet_ruleset_city city_p;
  int k, j;

  for (k = 0; k < game.control.styles_count; k++) {
    city_p.style_id = k;
    city_p.replaced_by = city_styles[k].replaced_by;

    j = 0;
    requirement_vector_iterate(&city_styles[k].reqs, preq) {
      city_p.reqs[j++] = *preq;
    } requirement_vector_iterate_end;
    city_p.reqs_count = j;

    sz_strlcpy(city_p.name, city_styles[k].name.vernacular);
    sz_strlcpy(city_p.graphic, city_styles[k].graphic);
    sz_strlcpy(city_p.graphic_alt, city_styles[k].graphic_alt);
    sz_strlcpy(city_p.oceanic_graphic, city_styles[k].oceanic_graphic);
    sz_strlcpy(city_p.oceanic_graphic_alt, city_styles[k].oceanic_graphic_alt);
    sz_strlcpy(city_p.citizens_graphic, city_styles[k].citizens_graphic);
    sz_strlcpy(city_p.citizens_graphic_alt,
			 city_styles[k].citizens_graphic_alt);

    lsend_packet_ruleset_city(dest, &city_p);
  }
}

/**************************************************************************
  Send information in packet_ruleset_game (miscellaneous rules) to the
  specified connections.
**************************************************************************/
static void send_ruleset_game(struct conn_list *dest)
{
  struct packet_ruleset_game misc_p;

  memcpy(misc_p.work_veteran_chance, game.work_veteran_chance, 
         sizeof(game.work_veteran_chance));
  memcpy(misc_p.veteran_chance, game.veteran_chance, 
         sizeof(game.veteran_chance));
    
  // FIXME: removed for webclient. - ANdreas.
  //assert(sizeof(misc_p.global_init_techs) ==
  //	 sizeof(game.server.rgame.global_init_techs));
  //assert(ARRAY_SIZE(misc_p.global_init_techs) ==
  //	 ARRAY_SIZE(game.server.rgame.global_init_techs));
  //memcpy(misc_p.global_init_techs, game.server.rgame.global_init_techs,
  //	 sizeof(misc_p.global_init_techs));

  misc_p.default_specialist = DEFAULT_SPECIALIST;

  lsend_packet_ruleset_game(dest, &misc_p);
}

/****************************************************************************
  HACK: reset any nations that have been set so far.

  FIXME: this should be moved into nationhand.c.
****************************************************************************/
static void reset_player_nations(void)
{
  players_iterate(pplayer) {
    /* We cannot use player_set_nation() here since this is
     * called before nations are loaded. Player pointers
     * from nations will be initialized to NULL when nations are
     * loaded. */
    pplayer->nation = NO_NATION_SELECTED;
    pplayer->city_style = 0;
  } players_iterate_end;
  send_player_info_c(NULL, game.est_connections);
}

/**************************************************************************
  Loads the ruleset currently given in game.rulesetdir.

  This may be called more than once and it will free any stale data.
**************************************************************************/
void load_rulesets(void)
{
  struct section_file techfile, unitfile, buildfile, govfile, terrfile;
  struct section_file cityfile, nationfile, effectfile;

  freelog(LOG_NORMAL, _("Loading rulesets"));

  game_ruleset_free();
  game_ruleset_init();

  reset_player_nations();
  server.playable_nations = 0;

  openload_ruleset_file(&techfile, "techs");
  load_tech_names(&techfile);

  openload_ruleset_file(&buildfile, "buildings");
  load_building_names(&buildfile);

  openload_ruleset_file(&govfile, "governments");
  load_government_names(&govfile);

  openload_ruleset_file(&unitfile, "units");
  load_unit_names(&unitfile);

  openload_ruleset_file(&terrfile, "terrain");
  load_terrain_names(&terrfile);

  openload_ruleset_file(&cityfile, "cities");
  load_citystyle_names(&cityfile);

  openload_ruleset_file(&nationfile, "nations");
  load_nation_names(&nationfile);

  openload_ruleset_file(&effectfile, "effects");

  load_ruleset_techs(&techfile);
  load_ruleset_cities(&cityfile);
  load_ruleset_governments(&govfile);
  load_ruleset_units(&unitfile);
  load_ruleset_terrain(&terrfile);    /* terrain must precede nations */
  load_ruleset_buildings(&buildfile);
  load_ruleset_nations(&nationfile);
  load_ruleset_effects(&effectfile);
  load_ruleset_game();

  /* Init nations we just loaded. */
  init_available_nations();

  sanity_check_ruleset_data();

  precalc_tech_data();

  if (base_sections) {
    free(base_sections);
    base_sections = NULL;
  }
  if (resource_sections) {
    free(resource_sections);
    resource_sections = NULL;
  }
  if (terrain_sections) {
    free(terrain_sections);
    terrain_sections = NULL;
  }

  script_free();

  script_init();
  openload_script_file("default");
  openload_script_file("script");

  /* Build AI unit class cache corresponding to loaded rulesets */
  unit_class_ai_init();

  /* We may need to adjust the number of AI players
   * if the number of available nations changed. */
  if (game.info.aifill > server.playable_nations) {
    freelog(LOG_NORMAL, _("Reducing aifill because there "
                          "are not enough playable nations."));
    game.info.aifill = server.playable_nations;
    aifill(game.info.aifill);
  }
}

/**************************************************************************
  Send all ruleset information to the specified connections.
**************************************************************************/
void send_rulesets(struct conn_list *dest)
{
  conn_list_do_buffer(dest);
  lsend_packet_freeze_hint(dest);

  send_ruleset_control(dest);
  send_ruleset_game(dest);
  send_ruleset_techs(dest);
  send_ruleset_governments(dest);
  send_ruleset_unit_classes(dest);
  send_ruleset_units(dest);
  send_ruleset_specialists(dest);
  send_ruleset_resources(dest);
  send_ruleset_terrain(dest);
  send_ruleset_bases(dest);
  send_ruleset_buildings(dest);
  send_ruleset_nations(dest);
  send_ruleset_cities(dest);
  send_ruleset_cache(dest);

  lsend_packet_thaw_hint(dest);
  conn_list_do_unbuffer(dest);
}

/**************************************************************************
  Does nation have tech initially?
**************************************************************************/
static bool nation_has_initial_tech(struct nation_type *pnation,
                                    struct advance *tech)
{
  int i;

  /* See if it's given as global init tech */
  for (i = 0; i < MAX_NUM_TECH_LIST
       && game.server.rgame.global_init_techs[i] != A_LAST; i++) {
    if (game.server.rgame.global_init_techs[i] == advance_number(tech)) {
      return TRUE;
    }
  }

  /* See if it's given as national init tech */
  for (i = 0;
       i < MAX_NUM_TECH_LIST && pnation->init_techs[i] != A_LAST;
       i++) {
    if (pnation->init_techs[i] == advance_number(tech)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**************************************************************************
  Helper function for sanity_check_req_list() and sanity_check_req_vec()
**************************************************************************/
static bool sanity_check_req_set(int reqs_of_type[], struct requirement *preq,
                                 int max_tiles, const char *list_for)
{
  int rc;

  assert(preq->source.kind >= 0 && preq->source.kind < VUT_LAST);

  /* Add to counter */
  reqs_of_type[preq->source.kind]++;
  rc = reqs_of_type[preq->source.kind];

  if (rc > 1) {
    /* Multiple requirements of the same type */
    switch (preq->source.kind) {
     case VUT_GOVERNMENT:
     case VUT_NATION:
     case VUT_UTYPE:
     case VUT_UCLASS:
     case VUT_OTYPE:
     case VUT_SPECIALIST:
     case VUT_MINSIZE: /* Breaks nothing, but has no sense either */
     case VUT_MINYEAR:
     case VUT_AI_LEVEL:
     case VUT_TERRAINCLASS:
     case VUT_TERRAINALTER: /* Local range only */
     case VUT_CITYTILE:
       /* There can be only one requirement of these types (with current
        * range limitations)
        * Requirements might be identical, but we consider multiple
        * declarations error anyway. */

       freelog(LOG_ERROR,
               "%s: Requirement list has multiple %s requirements",
               list_for,
               universal_type_rule_name(&preq->source));
       return FALSE;
       break;

     case VUT_SPECIAL:
     case VUT_TERRAIN:
     case VUT_BASE:
       /* There can be only up to max_tiles requirements of these types */
       if (max_tiles != 1 && rc > max_tiles) {
         freelog(LOG_ERROR,
                 "%s: Requirement list has more %s requirements than "
                 "can ever be fullfilled.",
                 list_for,
                 universal_type_rule_name(&preq->source));
         return FALSE;
       }
       break;

     case VUT_NONE:
     case VUT_ADVANCE:
     case VUT_IMPROVEMENT:
     case VUT_UTFLAG:
     case VUT_UCFLAG:
       /* Can have multiple requirements of these types */
       break;
     case VUT_LAST:
       /* Should never be in requirement vector */
       assert(FALSE);
       return FALSE;
       break;
       /* No default handling here, as we want compiler warning
        * if new requirement type is added to enum and it's not handled
        * here. */
    }
  }

  return TRUE;
}

/**************************************************************************
  Check if requirement list is free of conflicting requirements.
  max_tiles is number of tiles that can provide requirement. Value -1
  disables checking based on number of tiles.

  Returns TRUE iff everything ok.

  TODO: This is based on current hardcoded range limitations.
        - There should be method of automatically determining these
          limitations for each requirement type
        - This function should check also problems caused by defining
          range to less than hardcoded max for requirement type
**************************************************************************/
static bool sanity_check_req_list(const struct requirement_list *preqs,
                                  int max_tiles,
                                  const char *list_for)
{
  int reqs_of_type[VUT_LAST];

  /* Initialize requirement counters */
  memset(reqs_of_type, 0, sizeof(reqs_of_type));

  requirement_list_iterate(preqs, preq) {
    if (!sanity_check_req_set(reqs_of_type, preq, max_tiles, list_for)) {
      return FALSE;
    }
  } requirement_list_iterate_end;

  return TRUE;
}

/**************************************************************************
  Requirement vector version of requirement sanity checking. See
  retuirement list version for comments.
**************************************************************************/
static bool sanity_check_req_vec(const struct requirement_vector *preqs,
                                 int max_tiles,
                                 const char *list_for)
{
  int reqs_of_type[VUT_LAST];

  /* Initialize requirement counters */
  memset(reqs_of_type, 0, sizeof(reqs_of_type));

  requirement_vector_iterate(preqs, preq) {
    if (!sanity_check_req_set(reqs_of_type, preq, max_tiles, list_for)) {
      return FALSE;
    }
  } requirement_vector_iterate_end;

  return TRUE;
}

/**************************************************************************
  Check that requirement list and negated requirements list do not have
  confliciting requirements.

  Returns TRUE iff everything ok.
**************************************************************************/
static bool sanity_check_req_nreq_list(const struct requirement_list *preqs,
                                       const struct requirement_list *pnreqs,
                                       int one_tile,
                                       const char *list_for)
{
  /* Check internal sanity of requirement list */
  if (!sanity_check_req_list(preqs, one_tile, list_for)) {
    return FALSE;
  }

  /* There is no pnreqs in all cases */
  if (pnreqs != NULL) {
    /* Check sanity between reqs and nreqs */
    requirement_list_iterate(preqs, preq) {
      requirement_list_iterate(pnreqs, pnreq) {
        if (are_requirements_equal(preq, pnreq)) {
          freelog(LOG_ERROR,
                  "%s: Identical %s requirement in requirements and negated requirements.",
                  list_for,
                  universal_type_rule_name(&preq->source));
          return FALSE;
        }
      } requirement_list_iterate_end;
    } requirement_list_iterate_end;
  }

  return TRUE;
}

/**************************************************************************
  Sanity check callback for iterating effects cache.
**************************************************************************/
static bool effect_list_sanity_cb(const struct effect *peffect)
{
  int one_tile = -1; /* TODO: Determine correct value from effect.
                      *       -1 disables checking */

  return sanity_check_req_nreq_list(peffect->reqs, peffect->nreqs, one_tile,
                                    effect_type_name(peffect->type));
}

/**************************************************************************
  Some more sanity checking once all rulesets are loaded. These check
  for some cross-referencing which was impossible to do while only one
  party was loaded in load_ruleset_xxx()

  Returns TRUE iff everything ok.
**************************************************************************/
static bool sanity_check_ruleset_data(void)
{
  int num_utypes;
  int i;
  bool ok = TRUE; /* Store failures to variable instead of returning
                   * immediately so all errors get printed, not just first
                   * one. */

  /* Check that all players can have their initial techs */
  nations_iterate(pnation) {
    int i;

    /* Check global initial techs */
    for (i = 0; i < MAX_NUM_TECH_LIST
         && game.server.rgame.global_init_techs[i] != A_LAST; i++) {
      Tech_type_id tech = game.server.rgame.global_init_techs[i];
      struct advance *a = valid_advance_by_number(tech);

      if (!a) {
        ruleset_error(LOG_FATAL,
                      "Tech %s does not exist, but is initial "
                      "tech for everyone.",
                      advance_rule_name(advance_by_number(tech)));
      }
      if (advance_by_number(A_NONE) != a->require[AR_ROOT]
          && !nation_has_initial_tech(pnation, a->require[AR_ROOT])) {
        /* Nation has no root_req for tech */
        ruleset_error(LOG_FATAL,
                      "Tech %s is initial for everyone, but %s has "
                      "no root_req for it.",
                      advance_rule_name(a),
                      nation_rule_name(pnation));
      }
    }

    /* Check national initial techs */
    for (i = 0;
         i < MAX_NUM_TECH_LIST && pnation->init_techs[i] != A_LAST;
         i++) {
      Tech_type_id tech = pnation->init_techs[i];
      struct advance *a = valid_advance_by_number(tech);

      if (!a) {
        ruleset_error(LOG_FATAL,
                      "Tech %s does not exist, but is tech for %s.",
                      advance_rule_name(advance_by_number(tech)),
                      nation_rule_name(pnation));
        ok = FALSE;
      }
      if (advance_by_number(A_NONE) != a->require[AR_ROOT]
          && !nation_has_initial_tech(pnation, a->require[AR_ROOT])) {
        /* Nation has no root_req for tech */
        ruleset_error(LOG_FATAL,
                      "Tech %s is initial for %s, but they have "
                      "no root_req for it.",
                      advance_rule_name(a),
                      nation_rule_name(pnation));
        ok = FALSE;
      }
    }
  } nations_iterate_end;

  /* Check against unit upgrade loops */
  num_utypes = game.control.num_unit_types;
  unit_type_iterate(putype) {
    int chain_length = 0;
    struct unit_type *upgraded = putype;

    while(upgraded != NULL) {
      upgraded = upgraded->obsoleted_by;
      chain_length++;
      if (chain_length > num_utypes) {
        ruleset_error(LOG_FATAL,
                      "There seems to be obsoleted_by loop in update "
                      "chain that starts from %s", utype_rule_name(putype));
        ok = FALSE;
      }
    }
  } unit_type_iterate_end;

  /* Check requirement sets against conflicting requirements.
   * Effects use requirement lists */
  if (!iterate_effect_cache(effect_list_sanity_cb)) {
    ruleset_error(LOG_FATAL, "Effects have conflicting requirements!");
    ok = FALSE;
  }

  /* Others use requirement vectors
   * Buildings */
  improvement_iterate(pimprove) {
    if (!sanity_check_req_vec(&pimprove->reqs, -1,
                              improvement_rule_name(pimprove))) {
      ruleset_error(LOG_FATAL, "Buildings have conflicting requirements!");
      ok = FALSE;
    }
  } improvement_iterate_end;

  /* Governments */
  government_iterate(pgov) {
    if (!sanity_check_req_vec(&pgov->reqs, -1,
                              government_rule_name(pgov))) {
      ruleset_error(LOG_FATAL, "Governments have conflicting requirements!");
      ok = FALSE;
    }
  } government_iterate_end;

  /* Specialists */
  specialist_type_iterate(sp) {
    struct specialist *psp = specialist_by_number(sp);

    if (!sanity_check_req_vec(&psp->reqs, -1,
                              specialist_rule_name(psp))) {
      ruleset_error(LOG_FATAL, "Specialists have conflicting requirements!");
      ok = FALSE;
    }
  } specialist_type_iterate_end;

  /* City styles */
  for (i = 0; i < game.control.styles_count; i++) {
    if (!sanity_check_req_vec(&city_styles[i].reqs, -1,
                              city_style_rule_name(i))) {
      ruleset_error(LOG_FATAL, "City styles have conflicting requirements!");
      ok = FALSE;
    }
  }

  return ok;
}
