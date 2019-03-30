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

/*
  This file includes the definition of a new savegame format introduced with
  2.3.0. It is defined by the mandatory option '+version2'. The main load
  function checks if this option is present. If not, the old (pre-2.3.0)
  loading routines are used.
  The format version is also saved in the settings section of the savefile, as an
  integer (savefile.version). The integer is used to determine the version
  of the savefile.

  Structure of this file:

  - The real work is done by savegame2_load().
    This function call all submodules (settings, players, etc.)

  - The remaining part of this file is split into several sections:
     * helper functions
     * load functions for all submodules (and their subsubmodules)

  - If possible, all functions for load submodules should exit in
    pairs named sg_load_<submodule>. If one is not
    needed please add a comment why.

  - The submodules can be further divided as:
    sg_load_<submodule>_<subsubmodule>

  - If needed (due to static variables in the *.c files) these functions
    can be located in the corresponding source files (as done for the settings
    and the event_cache).

  Loading a savegame:

  - The status of the process is saved within the static variable
    'sg_success'. This variable is set to TRUE within savegame2_load().
    If you encounter an error use sg_failure_*() to set it to FALSE and
    return an error message. Furthermore, sg_check_* should be used at the
    start of each (submodule) function to return if previous functions failed.

  - While the loading process dependencies between different modules exits.
    They can be handled within the struct loaddata *loading which is used as
    first argument for all sg_load_*() function. Please indicate the
    dependencies within the definition of this struct.

*/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "bitvector.h"
#include "fcintl.h"
#include "idex.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"            /* bool type */
#include "timing.h"

/* common */
#include "achievements.h"
#include "ai.h"
#include "bitvector.h"
#include "capability.h"
#include "citizens.h"
#include "city.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "mapimg.h"
#include "movement.h"
#include "multipliers.h"
#include "packets.h"
#include "research.h"
#include "rgbcolor.h"
#include "specialist.h"
#include "unit.h"
#include "unitlist.h"
#include "version.h"

/* server */
#include "barbarian.h"
#include "citizenshand.h"
#include "citytools.h"
#include "cityturn.h"
#include "diplhand.h"
#include "maphand.h"
#include "meta.h"
#include "notify.h"
#include "plrhand.h"
#include "report.h"
#include "ruleset.h"
#include "sanitycheck.h"
#include "savecompat.h"
#include "score.h"
#include "settings.h"
#include "spacerace.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "techtools.h"
#include "unittools.h"

/* server/advisors */
#include "advdata.h"
#include "advbuilding.h"
#include "infracache.h"

/* server/generator */
#include "mapgen.h"
#include "mapgen_utils.h"

/* server/scripting */
#include "script_server.h"

/* ai */
#include "aitraits.h"
#include "difficulty.h"

#include "savegame2.h"

extern bool sg_success;

/*
 * This loops over the entire map to save data. It collects all the data of
 * a line using GET_XY_CHAR and then executes the macro SECFILE_INSERT_LINE.
 *
 * Parameters:
 *   ptile:         current tile within the line (used by GET_XY_CHAR)
 *   GET_XY_CHAR:   macro returning the map character for each position
 *   secfile:       a secfile struct
 *   secpath, ...:  path as used for sprintf() with arguments; the last item
 *                  will be the y coordinate
 * Example:
 *   SAVE_MAP_CHAR(ptile, terrain2char(ptile->terrain), file, "map.t%04d");
 */
#define SAVE_MAP_CHAR(ptile, GET_XY_CHAR, secfile, secpath, ...)            \
{                                                                           \
  char _line[wld.map.xsize + 1];                                            \
  int _nat_x, _nat_y;                                                       \
                                                                            \
  for (_nat_y = 0; _nat_y < wld.map.ysize; _nat_y++) {                      \
    for (_nat_x = 0; _nat_x < wld.map.xsize; _nat_x++) {                    \
      struct tile *ptile = native_pos_to_tile(&(wld.map), _nat_x, _nat_y);  \
      fc_assert_action(ptile != NULL, continue);                            \
      _line[_nat_x] = (GET_XY_CHAR);                                        \
      sg_failure_ret(fc_isprint(_line[_nat_x] & 0x7f),                      \
                     "Trying to write invalid map data at position "        \
                     "(%d, %d) for path %s: '%c' (%d)", _nat_x, _nat_y,     \
                     secpath, _line[_nat_x], _line[_nat_x]);                \
    }                                                                       \
    _line[wld.map.xsize] = '\0';                                            \
    secfile_insert_str(secfile, _line, secpath, ## __VA_ARGS__, _nat_y);    \
  }                                                                         \
}

/*
 * This loops over the entire map to load data. It inputs a line of data
 * using the macro SECFILE_LOOKUP_LINE and then loops using the macro
 * SET_XY_CHAR to load each char into the map at (map_x, map_y). Internal
 * variables ch, map_x, map_y, nat_x, and nat_y are allocated within the
 * macro but definable by the caller.
 *
 * Parameters:
 *   ch:            a variable to hold a char (data for a single position,
 *                  used by SET_XY_CHAR)
 *   ptile:         current tile within the line (used by SET_XY_CHAR)
 *   SET_XY_CHAR:   macro to load the map character at each (map_x, map_y)
 *   secfile:       a secfile struct
 *   secpath, ...:  path as used for sprintf() with arguments; the last item
 *                  will be the y coordinate
 * Example:
 *   LOAD_MAP_CHAR(ch, ptile,
 *                 map_get_player_tile(ptile, plr)->terrain
 *                   = char2terrain(ch), file, "player%d.map_t%04d", plrno);
 *
 * Note: some (but not all) of the code this is replacing used to skip over
 *       lines that did not exist. This allowed for backward-compatibility.
 *       We could add another parameter that specified whether it was OK to
 *       skip the data, but there's not really much advantage to exiting
 *       early in this case. Instead, we let any map data type to be empty,
 *       and just print an informative warning message about it.
 */
#define LOAD_MAP_CHAR(ch, ptile, SET_XY_CHAR, secfile, secpath, ...)        \
{                                                                           \
  int _nat_x, _nat_y;                                                       \
  bool _printed_warning = FALSE;                                            \
  for (_nat_y = 0; _nat_y < wld.map.ysize; _nat_y++) {                      \
    const char *_line = secfile_lookup_str(secfile, secpath,                \
                                           ## __VA_ARGS__, _nat_y);         \
    if (NULL == _line) {                                                    \
      char buf[64];                                                         \
      fc_snprintf(buf, sizeof(buf), secpath, ## __VA_ARGS__, _nat_y);       \
      log_verbose("Line not found='%s'", buf);                              \
      _printed_warning = TRUE;                                              \
      continue;                                                             \
    } else if (strlen(_line) != wld.map.xsize) {                            \
      char buf[64];                                                         \
      fc_snprintf(buf, sizeof(buf), secpath, ## __VA_ARGS__, _nat_y);       \
      log_verbose("Line too short (expected %d got %lu)='%s'",              \
                  wld.map.xsize, (unsigned long) strlen(_line), buf);       \
      _printed_warning = TRUE;                                              \
      continue;                                                             \
    }                                                                       \
    for (_nat_x = 0; _nat_x < wld.map.xsize; _nat_x++) {                    \
      const char ch = _line[_nat_x];                                        \
      struct tile *ptile = native_pos_to_tile(&(wld.map), _nat_x, _nat_y);  \
      (SET_XY_CHAR);                                                        \
    }                                                                       \
  }                                                                         \
  if (_printed_warning) {                                                   \
    /* TRANS: Minor error message. */                                       \
    log_sg(_("Saved game contains incomplete map data. This can"            \
             " happen with old saved games, or it may indicate an"          \
             " invalid saved game file. Proceed at your own risk."));       \
  }                                                                         \
}

/* Iterate on the extras half-bytes */
#define halfbyte_iterate_extras(e, num_extras_types)                        \
{                                                                           \
  int e;                                                                    \
  for (e = 0; 4 * e < (num_extras_types); e++) {

#define halfbyte_iterate_extras_end                                         \
  }                                                                         \
}

/* Iterate on the specials half-bytes */
#define halfbyte_iterate_special(s, num_specials_types)                     \
{                                                                           \
  enum tile_special_type s;                                                 \
  for (s = 0; 4 * s < (num_specials_types); s++) {

#define halfbyte_iterate_special_end                                        \
  }                                                                         \
}

/* Iterate on the bases half-bytes */
#define halfbyte_iterate_bases(b, num_bases_types)                          \
{                                                                           \
  int b;                                                                    \
  for (b = 0; 4 * b < (num_bases_types); b++) {

#define halfbyte_iterate_bases_end                                          \
  }                                                                         \
}

/* Iterate on the roads half-bytes */
#define halfbyte_iterate_roads(r, num_roads_types)                          \
{                                                                           \
  int r;                                                                    \
  for (r = 0; 4 * r < (num_roads_types); r++) {

#define halfbyte_iterate_roads_end                                          \
  }                                                                         \
}

#define TOKEN_SIZE 10

static struct loaddata *loaddata_new(struct section_file *file);
static void loaddata_destroy(struct loaddata *loading);

static enum unit_orders char2order(char order);
static enum direction8 char2dir(char dir);
static char activity2char(enum unit_activity activity);
static enum unit_activity char2activity(char activity);
static int unquote_block(const char *const quoted_, void *dest,
                         int dest_length);
static void worklist_load(struct section_file *file, struct worklist *pwl,
                          const char *path, ...);
static void unit_ordering_apply(void);
static void sg_extras_set(bv_extras *extras, char ch, struct extra_type **idx);
static void sg_special_set(struct tile *ptile, bv_extras *extras, char ch,
                           const enum tile_special_type *idx,
                           bool rivers_overlay);
static void sg_bases_set(bv_extras *extras, char ch, struct base_type **idx);
static void sg_roads_set(bv_extras *extras, char ch, struct road_type **idx);
static struct extra_type *char2resource(char c);
static struct terrain *char2terrain(char ch);
static Tech_type_id technology_load(struct section_file *file,
                                    const char* path, int plrno);

static void sg_load_ruleset(struct loaddata *loading);
static void sg_load_savefile(struct loaddata *loading);

static void sg_load_game(struct loaddata *loading);

static void sg_load_ruledata(struct loaddata *loading);

static void sg_load_random(struct loaddata *loading);

static void sg_load_script(struct loaddata *loading);

static void sg_load_scenario(struct loaddata *loading);

static void sg_load_settings(struct loaddata *loading);

static void sg_load_map(struct loaddata *loading);
static void sg_load_map_tiles(struct loaddata *loading);
static void sg_load_map_tiles_extras(struct loaddata *loading);
static void sg_load_map_tiles_bases(struct loaddata *loading);
static void sg_load_map_tiles_roads(struct loaddata *loading);
static void sg_load_map_tiles_specials(struct loaddata *loading,
                                       bool rivers_overlay);
static void sg_load_map_tiles_resources(struct loaddata *loading);

static void sg_load_map_startpos(struct loaddata *loading);
static void sg_load_map_owner(struct loaddata *loading);
static void sg_load_map_worked(struct loaddata *loading);
static void sg_load_map_known(struct loaddata *loading);

static void sg_load_players_basic(struct loaddata *loading);
static void sg_load_players(struct loaddata *loading);
static void sg_load_player_main(struct loaddata *loading,
                                struct player *plr);
static void sg_load_player_cities(struct loaddata *loading,
                                  struct player *plr);
static bool sg_load_player_city(struct loaddata *loading, struct player *plr,
                                struct city *pcity, const char *citystr);
static void sg_load_player_city_citizens(struct loaddata *loading,
                                         struct player *plr,
                                         struct city *pcity,
                                         const char *citystr);
static void sg_load_player_units(struct loaddata *loading,
                                 struct player *plr);
static bool sg_load_player_unit(struct loaddata *loading,
                                struct player *plr, struct unit *punit,
                                const char *unitstr);
static void sg_load_player_units_transport(struct loaddata *loading,
                                           struct player *plr);
static void sg_load_player_attributes(struct loaddata *loading,
                                      struct player *plr);
static void sg_load_player_vision(struct loaddata *loading,
                                  struct player *plr);
static bool sg_load_player_vision_city(struct loaddata *loading,
                                       struct player *plr,
                                       struct vision_site *pdcity,
                                       const char *citystr);

static void sg_load_researches(struct loaddata *loading);

static void sg_load_event_cache(struct loaddata *loading);

static void sg_load_treaties(struct loaddata *loading);

static void sg_load_history(struct loaddata *loading);

static void sg_load_mapimg(struct loaddata *loading);

static void sg_load_sanitycheck(struct loaddata *loading);


/* =======================================================================
 * Basic load / save functions.
 * ======================================================================= */

/************************************************************************//**
  Really loading the savegame.
****************************************************************************/
void savegame2_load(struct section_file *file)
{
  struct loaddata *loading;
  bool was_send_city_suppressed, was_send_tile_suppressed;

  /* initialise loading */
  was_send_city_suppressed = send_city_suppression(TRUE);
  was_send_tile_suppressed = send_tile_suppression(TRUE);
  loading = loaddata_new(file);
  sg_success = TRUE;

  /* Load the savegame data. */
  /* Set up correct ruleset */
  sg_load_ruleset(loading);
  /* [compat] */
  sg_load_compat(loading, SAVEGAME_2);
  /* [savefile] */
  sg_load_savefile(loading);
  /* [game] */
  sg_load_game(loading);
  /* [scenario] */
  sg_load_scenario(loading);
  /* [random] */
  sg_load_random(loading);
  /* [settings] */
  sg_load_settings(loading);
  /* [ruldata] */
  sg_load_ruledata(loading);
  /* [players] (basic data) */
  sg_load_players_basic(loading);
  /* [map]; needs width and height loaded by [settings]  */
  sg_load_map(loading);
  /* [research] */
  sg_load_researches(loading);
  /* [player<i>] */
  sg_load_players(loading);
  /* [event_cache] */
  sg_load_event_cache(loading);
  /* [treaties] */
  sg_load_treaties(loading);
  /* [history] */
  sg_load_history(loading);
  /* [mapimg] */
  sg_load_mapimg(loading);
  /* [script] -- must come last as may reference game objects */
  sg_load_script(loading);

  /* Sanity checks for the loaded game. */
  sg_load_sanitycheck(loading);

  /* deinitialise loading */
  loaddata_destroy(loading);
  send_tile_suppression(was_send_tile_suppressed);
  send_city_suppression(was_send_city_suppressed);

  if (!sg_success) {
    log_error("Failure loading savegame!");
    game_reset();
  }
}

/************************************************************************//**
  Create new loaddata item for given section file.
****************************************************************************/
static struct loaddata *loaddata_new(struct section_file *file)
{
  struct loaddata *loading = calloc(1, sizeof(*loading));
  loading->file = file;
  loading->secfile_options = NULL;

  loading->improvement.order = NULL;
  loading->improvement.size = -1;
  loading->technology.order = NULL;
  loading->technology.size = -1;
  loading->activities.order = NULL;
  loading->activities.size = -1;
  loading->trait.order = NULL;
  loading->trait.size = -1;
  loading->extra.order = NULL;
  loading->extra.size = -1;
  loading->multiplier.order = NULL;
  loading->multiplier.size = -1;
  loading->special.order = NULL;
  loading->special.size = -1;
  loading->base.order = NULL;
  loading->base.size = -1;
  loading->road.order = NULL;
  loading->road.size = -1;
  loading->specialist.order = NULL;
  loading->specialist.size = -1;
  loading->ds_t.order = NULL;
  loading->ds_t.size = -1;

  loading->server_state = S_S_INITIAL;
  loading->rstate = fc_rand_state();
  loading->worked_tiles = NULL;

  return loading;
}

/************************************************************************//**
  Free resources allocated for loaddata item.
****************************************************************************/
static void loaddata_destroy(struct loaddata *loading)
{
  if (loading->improvement.order != NULL) {
    free(loading->improvement.order);
  }

  if (loading->technology.order != NULL) {
    free(loading->technology.order);
  }

  if (loading->activities.order != NULL) {
    free(loading->activities.order);
  }

  if (loading->trait.order != NULL) {
    free(loading->trait.order);
  }

  if (loading->extra.order != NULL) {
    free(loading->extra.order);
  }

  if (loading->multiplier.order != NULL) {
    free(loading->multiplier.order);
  }

  if (loading->special.order != NULL) {
    free(loading->special.order);
  }

  if (loading->base.order != NULL) {
    free(loading->base.order);
  }

  if (loading->road.order != NULL) {
    free(loading->road.order);
  }

  if (loading->specialist.order != NULL) {
    free(loading->specialist.order);
  }

  if (loading->ds_t.order != NULL) {
    free(loading->ds_t.order);
  }

  if (loading->worked_tiles != NULL) {
    free(loading->worked_tiles);
  }

  free(loading);
}

/* =======================================================================
 * Helper functions.
 * ======================================================================= */

/************************************************************************//**
  Returns an order for a character identifier.
****************************************************************************/
static enum unit_orders char2order(char order)
{
  switch (order) {
  case 'm':
  case 'M':
    return ORDER_MOVE;
  case 'w':
  case 'W':
    return ORDER_FULL_MP;
  case 'b':
  case 'B':
    return ORDER_OLD_BUILD_CITY;
  case 'a':
  case 'A':
    return ORDER_ACTIVITY;
  case 'd':
  case 'D':
    return ORDER_OLD_DISBAND;
  case 'u':
  case 'U':
    return ORDER_OLD_BUILD_WONDER;
  case 't':
  case 'T':
    return ORDER_OLD_TRADE_ROUTE;
  case 'h':
  case 'H':
    return ORDER_OLD_HOMECITY;
  case 'x':
  case 'X':
    return ORDER_ACTION_MOVE;
  }

  /* This can happen if the savegame is invalid. */
  return ORDER_LAST;
}

/************************************************************************//**
  Returns a direction for a character identifier.
****************************************************************************/
static enum direction8 char2dir(char dir)
{
  /* Numberpad values for the directions. */
  switch (dir) {
  case '1':
    return DIR8_SOUTHWEST;
  case '2':
    return DIR8_SOUTH;
  case '3':
    return DIR8_SOUTHEAST;
  case '4':
    return DIR8_WEST;
  case '6':
    return DIR8_EAST;
  case '7':
    return DIR8_NORTHWEST;
  case '8':
    return DIR8_NORTH;
  case '9':
    return DIR8_NORTHEAST;
  }

  /* This can happen if the savegame is invalid. */
  return direction8_invalid();
}

/************************************************************************//**
  Returns a character identifier for an activity.  See also char2activity.
****************************************************************************/
static char activity2char(enum unit_activity activity)
{
  switch (activity) {
  case ACTIVITY_IDLE:
    return 'w';
  case ACTIVITY_POLLUTION:
    return 'p';
  case ACTIVITY_OLD_ROAD:
    return 'r';
  case ACTIVITY_MINE:
    return 'm';
  case ACTIVITY_IRRIGATE:
    return 'i';
  case ACTIVITY_FORTIFIED:
    return 'f';
  case ACTIVITY_FORTRESS:
    return 't';
  case ACTIVITY_SENTRY:
    return 's';
  case ACTIVITY_OLD_RAILROAD:
    return 'l';
  case ACTIVITY_PILLAGE:
    return 'e';
  case ACTIVITY_GOTO:
    return 'g';
  case ACTIVITY_EXPLORE:
    return 'x';
  case ACTIVITY_TRANSFORM:
    return 'o';
  case ACTIVITY_AIRBASE:
    return 'a';
  case ACTIVITY_FORTIFYING:
    return 'y';
  case ACTIVITY_FALLOUT:
    return 'u';
  case ACTIVITY_BASE:
    return 'b';
  case ACTIVITY_GEN_ROAD:
    return 'R';
  case ACTIVITY_CONVERT:
    return 'c';
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_PATROL_UNUSED:
    return '?';
  case ACTIVITY_LAST:
    break;
  }

  fc_assert(FALSE);
  return '?';
}

/************************************************************************//**
  Returns an activity for a character identifier.  See also activity2char.
****************************************************************************/
static enum unit_activity char2activity(char activity)
{
  enum unit_activity a;

  for (a = 0; a < ACTIVITY_LAST; a++) {
    char achar = activity2char(a);

    if (activity == achar) {
      return a;
    }
  }

  /* This can happen if the savegame is invalid. */
  return ACTIVITY_LAST;
}

/************************************************************************//**
  Unquote a string. The unquoted data is written into dest. If the unquoted
  data will be larger than dest_length the function aborts. It returns the
  actual length of the unquoted block.
****************************************************************************/
static int unquote_block(const char *const quoted_, void *dest,
                         int dest_length)
{
  int i, length, parsed, tmp;
  char *endptr;
  const char *quoted = quoted_;

  parsed = sscanf(quoted, "%d", &length);
  fc_assert_ret_val(1 == parsed, 0);

  if (length > dest_length) {
    return 0;
  }
  quoted = strchr(quoted, ':');
  fc_assert_ret_val(quoted != NULL, 0);
  quoted++;

  for (i = 0; i < length; i++) {
    tmp = strtol(quoted, &endptr, 16);
    fc_assert_ret_val((endptr - quoted) == 2, 0);
    fc_assert_ret_val(*endptr == ' ', 0);
    fc_assert_ret_val((tmp & 0xff) == tmp, 0);
    ((unsigned char *) dest)[i] = tmp;
    quoted += 3;
  }
  return length;
}

/************************************************************************//**
  Load the worklist elements specified by path to the worklist pointed to
  by 'pwl'. 'pwl' should be a pointer to an existing worklist.
****************************************************************************/
static void worklist_load(struct section_file *file, struct worklist *pwl,
                          const char *path, ...)
{
  int i;
  const char *kind;
  const char *name;
  char path_str[1024];
  va_list ap;

  /* The first part of the registry path is taken from the varargs to the
   * function. */
  va_start(ap, path);
  fc_vsnprintf(path_str, sizeof(path_str), path, ap);
  va_end(ap);

  worklist_init(pwl);
  pwl->length = secfile_lookup_int_default(file, 0,
                                           "%s.wl_length", path_str);

  for (i = 0; i < pwl->length; i++) {
    kind = secfile_lookup_str(file, "%s.wl_kind%d", path_str, i);

    /* We lookup the production value by name. An invalid entry isn't a
     * fatal error; we just truncate the worklist. */
    name = secfile_lookup_str_default(file, "-", "%s.wl_value%d",
                                      path_str, i);
    pwl->entries[i] = universal_by_rule_name(kind, name);
    if (pwl->entries[i].kind == universals_n_invalid()) {
      log_sg("%s.wl_value%d: unknown \"%s\" \"%s\".", path_str, i, kind,
             name);
      pwl->length = i;
      break;
    }
  }
}

/************************************************************************//**
  For each city and tile, sort unit lists according to ord_city and ord_map
  values.
****************************************************************************/
static void unit_ordering_apply(void)
{
  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      unit_list_sort_ord_city(pcity->units_supported);
    }
    city_list_iterate_end;
  } players_iterate_end;

  whole_map_iterate(&(wld.map), ptile) {
    unit_list_sort_ord_map(ptile->units);
  } whole_map_iterate_end;
}

/************************************************************************//**
  Helper function for loading extras from a savegame.

  'ch' gives the character loaded from the savegame. Extras are packed
  in four to a character in hex notation. 'index' is a mapping of
  savegame bit -> base bit.
****************************************************************************/
static void sg_extras_set(bv_extras *extras, char ch, struct extra_type **idx)
{
  int i, bin;
  const char *pch = strchr(hex_chars, ch);

  if (!pch || ch == '\0') {
    log_sg("Unknown hex value: '%c' (%d)", ch, ch);
    bin = 0;
  } else {
    bin = pch - hex_chars;
  }

  for (i = 0; i < 4; i++) {
    struct extra_type *pextra = idx[i];

    if (pextra == NULL) {
      continue;
    }
    if ((bin & (1 << i))
        && (wld.map.server.have_huts || !is_extra_caused_by(pextra, EC_HUT))) {
      BV_SET(*extras, extra_index(pextra));
    }
  }
}

/************************************************************************//**
  Complicated helper function for loading specials from a savegame.

  'ch' gives the character loaded from the savegame. Specials are packed
  in four to a character in hex notation. 'index' is a mapping of
  savegame bit -> special bit. S_LAST is used to mark unused savegame bits.
****************************************************************************/
static void sg_special_set(struct tile *ptile, bv_extras *extras, char ch,
                           const enum tile_special_type *idx,
                           bool rivers_overlay)
{
  int i, bin;
  const char *pch = strchr(hex_chars, ch);

  if (!pch || ch == '\0') {
    log_sg("Unknown hex value: '%c' (%d)", ch, ch);
    bin = 0;
  } else {
    bin = pch - hex_chars;
  }

  for (i = 0; i < 4; i++) {
    enum tile_special_type sp = idx[i];

    if (sp == S_LAST) {
      continue;
    }
    if (rivers_overlay && sp != S_OLD_RIVER) {
      continue;
    }

    if (sp == S_HUT && !wld.map.server.have_huts) {
      /* It would be logical to have this in the saving side -
       * really not saving the huts in the first place, BUT
       * 1) They have been saved by older versions, so we
       *    have to deal with such savegames.
       * 2) This makes scenario author less likely to lose
       *    one's work completely after carefully placing huts
       *    and then saving with 'have_huts' disabled. */
      continue;
    }

    if (bin & (1 << i)) {
      if (sp == S_OLD_ROAD) {
        struct road_type *proad;

        proad = road_by_compat_special(ROCO_ROAD);
        if (proad) {
          BV_SET(*extras, extra_index(road_extra_get(proad)));
        }
      } else if (sp == S_OLD_RAILROAD) {
        struct road_type *proad;

        proad = road_by_compat_special(ROCO_RAILROAD);
        if (proad) {
          BV_SET(*extras, extra_index(road_extra_get(proad)));
        }
      } else if (sp == S_OLD_RIVER) {
        struct road_type *proad;

        proad = road_by_compat_special(ROCO_RIVER);
        if (proad) {
          BV_SET(*extras, extra_index(road_extra_get(proad)));
        }
      } else {
        struct extra_type *pextra = NULL;
        enum extra_cause cause = EC_COUNT;

        /* Converting from old hardcoded specials to as sensible extra as we can */
        switch (sp) {
        case S_IRRIGATION:
        case S_FARMLAND:
          /* If old savegame has both irrigation and farmland, EC_IRRIGATION
           * gets applied twice, which hopefully has the correct result. */
          cause = EC_IRRIGATION;
          break;
        case S_MINE:
          cause = EC_MINE;
          break;
        case S_POLLUTION:
          cause = EC_POLLUTION;
          break;
        case S_HUT:
          cause = EC_HUT;
          break;
        case S_FALLOUT:
          cause = EC_FALLOUT;
          break;
        default:
          pextra = extra_type_by_rule_name(special_rule_name(sp));
          break;
        }

        if (cause != EC_COUNT) {
          struct tile *vtile = tile_virtual_new(ptile);
          struct terrain *pterr = tile_terrain(vtile);

          /* Do not let the extras already set to the real tile mess with setup
           * of the player tiles if that's what we're doing. */
          vtile->extras = *extras;

          /* It's ok not to know which player or which unit originally built the extra -
           * in the rules used when specials were saved these could not have made any
           * difference. */
          /* Can't use next_extra_for_tile() as it works for buildable extras only. */

          if ((cause != EC_IRRIGATION || pterr->irrigation_result == pterr)
              && (cause != EC_MINE || pterr->mining_result == pterr)
              && (cause != EC_BASE || pterr->base_time != 0)
              && (cause != EC_ROAD || pterr->road_time != 0)) {
            extra_type_by_cause_iterate(cause, candidate) {
              if (!tile_has_extra(vtile, candidate)) {
                if ((!is_extra_caused_by(candidate, EC_BASE)
                     || tile_city(vtile) != NULL
                     || extra_base_get(candidate)->border_sq <= 0)
                    && are_reqs_active(NULL, tile_owner(vtile), NULL, NULL, vtile,
                                       NULL, NULL, NULL, NULL, NULL, &candidate->reqs,
                                       RPT_POSSIBLE)) {
                  pextra = candidate;
                  break;
                }
              }
            } extra_type_by_cause_iterate_end;
          }

          tile_virtual_destroy(vtile);
        }

        if (pextra) {
          BV_SET(*extras, extra_index(pextra));
        }
      }
    }
  }
}

/************************************************************************//**
  Helper function for loading bases from a savegame.

  'ch' gives the character loaded from the savegame. Bases are packed
  in four to a character in hex notation. 'index' is a mapping of
  savegame bit -> base bit.
****************************************************************************/
static void sg_bases_set(bv_extras *extras, char ch, struct base_type **idx)
{
  int i, bin;
  const char *pch = strchr(hex_chars, ch);

  if (!pch || ch == '\0') {
    log_sg("Unknown hex value: '%c' (%d)", ch, ch);
    bin = 0;
  } else {
    bin = pch - hex_chars;
  }

  for (i = 0; i < 4; i++) {
    struct base_type *pbase = idx[i];

    if (pbase == NULL) {
      continue;
    }
    if (bin & (1 << i)) {
      BV_SET(*extras, extra_index(base_extra_get(pbase)));
    }
  }
}

/************************************************************************//**
  Helper function for loading roads from a savegame.

  'ch' gives the character loaded from the savegame. Roads are packed
  in four to a character in hex notation. 'index' is a mapping of
  savegame bit -> road bit.
****************************************************************************/
static void sg_roads_set(bv_extras *extras, char ch, struct road_type **idx)
{
  int i, bin;
  const char *pch = strchr(hex_chars, ch);

  if (!pch || ch == '\0') {
    log_sg("Unknown hex value: '%c' (%d)", ch, ch);
    bin = 0;
  } else {
    bin = pch - hex_chars;
  }

  for (i = 0; i < 4; i++) {
    struct road_type *proad = idx[i];

    if (proad == NULL) {
      continue;
    }
    if (bin & (1 << i)) {
      BV_SET(*extras, extra_index(road_extra_get(proad)));
    }
  }
}

/************************************************************************//**
  Return the resource for the given identifier.
****************************************************************************/
static struct extra_type *char2resource(char c)
{
  /* speed common values */
  if (c == RESOURCE_NULL_IDENTIFIER
      || c == RESOURCE_NONE_IDENTIFIER) {
    return NULL;
  }

  return resource_by_identifier(c);
}

/************************************************************************//**
  Dereferences the terrain character.  See terrains[].identifier
    example: char2terrain('a') => T_ARCTIC
****************************************************************************/
static struct terrain *char2terrain(char ch)
{
  /* terrain_by_identifier plus fatal error */
  if (ch == TERRAIN_UNKNOWN_IDENTIFIER) {
    return T_UNKNOWN;
  }
  terrain_type_iterate(pterrain) {
    if (pterrain->identifier == ch) {
      return pterrain;
    }
  } terrain_type_iterate_end;

  log_fatal("Unknown terrain identifier '%c' in savegame.", ch);
  exit(EXIT_FAILURE);
}

/************************************************************************//**
  Load technology from path_name and if doesn't exist (because savegame
  is too old) load from path.
****************************************************************************/
static Tech_type_id technology_load(struct section_file *file,
                                    const char* path, int plrno)
{
  char path_with_name[128];
  const char* name;
  struct advance *padvance;

  fc_snprintf(path_with_name, sizeof(path_with_name),
              "%s_name", path);

  name = secfile_lookup_str(file, path_with_name, plrno);

  if (!name || name[0] == '\0') {
    /* used by researching_saved */
    return A_UNKNOWN;
  }
  if (fc_strcasecmp(name, "A_FUTURE") == 0) {
    return A_FUTURE;
  }
  if (fc_strcasecmp(name, "A_NONE") == 0) {
    return A_NONE;
  }
  if (fc_strcasecmp(name, "A_UNSET") == 0) {
    return A_UNSET;
  }

  padvance = advance_by_rule_name(name);
  sg_failure_ret_val(NULL != padvance, A_NONE,
                     "%s: unknown technology \"%s\".", path_with_name, name);

  return advance_number(padvance);
}

/* =======================================================================
 * Load savefile data.
 * ======================================================================= */

/************************************************************************//**
  Set up correct ruleset for the savegame
****************************************************************************/
static void sg_load_ruleset(struct loaddata *loading)
{
  /* Load ruleset. */
  sz_strlcpy(game.server.rulesetdir,
             secfile_lookup_str_default(loading->file, GAME_DEFAULT_RULESETDIR,
                                        "savefile.rulesetdir"));
  if (!strcmp("default", game.server.rulesetdir)) {
    int version;

    version = secfile_lookup_int_default(loading->file, -1, "savefile.version");
    if (version >= 30) {
      /* Here 'default' really means current default.
       * Saving happens with real ruleset name, so savegames containing this
       * are special scenarios. */
      sz_strlcpy(game.server.rulesetdir, GAME_DEFAULT_RULESETDIR);
    } else {
      /* 'default' is the old name of the classic ruleset */
      sz_strlcpy(game.server.rulesetdir, "classic");
    }
  }
  if (!load_rulesets(NULL, FALSE, NULL, TRUE, FALSE)) {
    /* Failed to load correct ruleset */
    sg_failure_ret(FALSE, _("Failed to load ruleset"));
  }
}

/************************************************************************//**
  Load '[savefile]'.
****************************************************************************/
static void sg_load_savefile(struct loaddata *loading)
{
  int i;
  const char *terr_name;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Load savefile options. */
  loading->secfile_options
    = secfile_lookup_str(loading->file, "savefile.options");

  /* We don't need these entries, but read them anyway to avoid
   * warnings about unread secfile entries. */
  (void) secfile_entry_by_path(loading->file, "savefile.reason");
  (void) secfile_entry_by_path(loading->file, "savefile.revision");

  /* In case of savegame2.c saves, missing entry means savegame older than support
   * for saving last_updated by turn. So this must default to TRUE. */
  game.server.last_updated_year = secfile_lookup_bool_default(loading->file, TRUE,
                                                              "savefile.last_updated_as_year");

  /* Load improvements. */
  loading->improvement.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.improvement_size");
  if (loading->improvement.size) {
    loading->improvement.order
      = secfile_lookup_str_vec(loading->file, &loading->improvement.size,
                               "savefile.improvement_vector");
    sg_failure_ret(loading->improvement.size != 0,
                   "Failed to load improvement order: %s",
                   secfile_error());
  }

  /* Load technologies. */
  loading->technology.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.technology_size");
  if (loading->technology.size) {
    loading->technology.order
      = secfile_lookup_str_vec(loading->file, &loading->technology.size,
                               "savefile.technology_vector");
    sg_failure_ret(loading->technology.size != 0,
                   "Failed to load technology order: %s",
                   secfile_error());
  }

  /* Load Activities. */
  loading->activities.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.activities_size");
  if (loading->activities.size) {
    loading->activities.order
      = secfile_lookup_str_vec(loading->file, &loading->activities.size,
                               "savefile.activities_vector");
    sg_failure_ret(loading->activities.size != 0,
                   "Failed to load activity order: %s",
                   secfile_error());
  }

  /* Load traits. */
  loading->trait.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.trait_size");
  if (loading->trait.size) {
    loading->trait.order
      = secfile_lookup_str_vec(loading->file, &loading->trait.size,
                               "savefile.trait_vector");
    sg_failure_ret(loading->trait.size != 0,
                   "Failed to load trait order: %s",
                   secfile_error());
  }

  /* Load extras. */
  loading->extra.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.extras_size");
  if (loading->extra.size) {
    const char **modname;
    size_t nmod;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->extra.size,
                                     "savefile.extras_vector");
    sg_failure_ret(loading->extra.size != 0,
                   "Failed to load extras order: %s",
                   secfile_error());
    sg_failure_ret(!(game.control.num_extra_types < loading->extra.size),
                   "Number of extras defined by the ruleset (= %d) are "
                   "lower than the number in the savefile (= %d).",
                   game.control.num_extra_types, (int)loading->extra.size);
    /* make sure that the size of the array is divisible by 4 */
    nmod = 4 * ((loading->extra.size + 3) / 4);
    loading->extra.order = fc_calloc(nmod, sizeof(*loading->extra.order));
    for (j = 0; j < loading->extra.size; j++) {
      loading->extra.order[j] = extra_type_by_rule_name(modname[j]);
    }
    free(modname);
    for (; j < nmod; j++) {
      loading->extra.order[j] = NULL;
    }
  }

  /* Load multipliers. */
  loading->multiplier.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.multipliers_size");
  if (loading->multiplier.size) {
    const char **modname;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->multiplier.size,
                                     "savefile.multipliers_vector");
    sg_failure_ret(loading->multiplier.size != 0,
                   "Failed to load multipliers order: %s",
                   secfile_error());
    /* It's OK for the set of multipliers in the savefile to differ
     * from those in the ruleset. */
    loading->multiplier.order = fc_calloc(loading->multiplier.size,
                                          sizeof(*loading->multiplier.order));
    for (j = 0; j < loading->multiplier.size; j++) {
      loading->multiplier.order[j] = multiplier_by_rule_name(modname[j]);
      if (!loading->multiplier.order[j]) {
        log_verbose("Multiplier \"%s\" in savegame but not in ruleset, "
                    "discarding", modname[j]);
      }
    }
    free(modname);
  }

  /* Load specials. */
  loading->special.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.specials_size");
  if (loading->special.size) {
    const char **modname;
    size_t nmod;
    enum tile_special_type j;

    modname = secfile_lookup_str_vec(loading->file, &loading->special.size,
                                     "savefile.specials_vector");
    sg_failure_ret(loading->special.size != 0,
                   "Failed to load specials order: %s",
                   secfile_error());
    /* make sure that the size of the array is divisible by 4 */
    /* Allocating extra 4 slots, just a couple of bytes,
     * in case of special.size being divisible by 4 already is intentional.
     * Added complexity would cost those couple of bytes in code size alone,
     * and we actually need at least one slot immediately after last valid
     * one. That's where S_LAST is (or was in version that saved the game)
     * and in some cases S_LAST gets written to savegame, at least as
     * activity target special when activity targets some base or road
     * instead. By having current S_LAST in that index allows us to map
     * that old S_LAST to current S_LAST, just like any real special within
     * special.size gets mapped. */
    nmod = loading->special.size + (4 - (loading->special.size % 4));
    loading->special.order = fc_calloc(nmod,
                                       sizeof(*loading->special.order));
    for (j = 0; j < loading->special.size; j++) {
      if (!strcasecmp("Road", modname[j])) {
        loading->special.order[j] = S_OLD_ROAD;
      } else if (!strcasecmp("Railroad", modname[j])) {
        loading->special.order[j] = S_OLD_RAILROAD;
      } else if (!strcasecmp("River", modname[j])) {
        loading->special.order[j] = S_OLD_RIVER;
      } else {
        loading->special.order[j] = special_by_rule_name(modname[j]);
      }
    }
    free(modname);
    for (; j < nmod; j++) {
      loading->special.order[j] = S_LAST;
    }
  }

  /* Load bases. */
  loading->base.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.bases_size");
  if (loading->base.size) {
    const char **modname;
    size_t nmod;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->base.size,
                                     "savefile.bases_vector");
    sg_failure_ret(loading->base.size != 0,
                   "Failed to load bases order: %s",
                   secfile_error());
    /* make sure that the size of the array is divisible by 4 */
    nmod = 4 * ((loading->base.size + 3) / 4);
    loading->base.order = fc_calloc(nmod, sizeof(*loading->base.order));
    for (j = 0; j < loading->base.size; j++) {
      struct extra_type *pextra = extra_type_by_rule_name(modname[j]);

      sg_failure_ret(pextra != NULL
                     || game.control.num_base_types >= loading->base.size,
                     "Unknown base type %s in savefile.",
                     modname[j]);

      if (pextra != NULL) {
        loading->base.order[j] = extra_base_get(pextra);
      } else {
        loading->base.order[j] = NULL;
      }
    }
    free(modname);
    for (; j < nmod; j++) {
      loading->base.order[j] = NULL;
    }
  }

  /* Load roads. */
  loading->road.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.roads_size");
  if (loading->road.size) {
    const char **modname;
    size_t nmod;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->road.size,
                                     "savefile.roads_vector");
    sg_failure_ret(loading->road.size != 0,
                   "Failed to load roads order: %s",
                   secfile_error());
    sg_failure_ret(!(game.control.num_road_types < loading->road.size),
                   "Number of roads defined by the ruleset (= %d) are "
                   "lower than the number in the savefile (= %d).",
                   game.control.num_road_types, (int)loading->road.size);
    /* make sure that the size of the array is divisible by 4 */
    nmod = 4 * ((loading->road.size + 3) / 4);
    loading->road.order = fc_calloc(nmod, sizeof(*loading->road.order));
    for (j = 0; j < loading->road.size; j++) {
      struct extra_type *pextra = extra_type_by_rule_name(modname[j]);

      if (pextra != NULL) {
        loading->road.order[j] = extra_road_get(pextra);
      } else {
        loading->road.order[j] = NULL;
      }
    }
    free(modname);
    for (; j < nmod; j++) {
      loading->road.order[j] = NULL;
    }
  }

  /* Load specialists. */
  loading->specialist.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.specialists_size");
  if (loading->specialist.size) {
    const char **modname;
    size_t nmod;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->specialist.size,
                                     "savefile.specialists_vector");
    sg_failure_ret(loading->specialist.size != 0,
                   "Failed to load specialists order: %s",
                   secfile_error());
    sg_failure_ret(!(game.control.num_specialist_types < loading->specialist.size),
                   "Number of specialists defined by the ruleset (= %d) are "
                   "lower than the number in the savefile (= %d).",
                   game.control.num_specialist_types, (int)loading->specialist.size);
    /* make sure that the size of the array is divisible by 4 */
    /* That's not really needed with specialists at the moment, but done this way
     * for consistency with other types, and to be prepared for the time it needs
     * to be this way. */
    nmod = 4 * ((loading->specialist.size + 3) / 4);
    loading->specialist.order = fc_calloc(nmod, sizeof(*loading->specialist.order));
    for (j = 0; j < loading->specialist.size; j++) {
      loading->specialist.order[j] = specialist_by_rule_name(modname[j]);
    }
    free(modname);
    for (; j < nmod; j++) {
      loading->specialist.order[j] = NULL;
    }
  }

  /* Load diplomatic state type order. */
  loading->ds_t.size
    = secfile_lookup_int_default(loading->file, 0,
                                 "savefile.diplstate_type_size");

  sg_failure_ret(loading->ds_t.size > 0,
                 "Failed to load diplomatic state type order: %s",
                 secfile_error());

  if (loading->ds_t.size) {
    const char **modname;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->ds_t.size,
                                     "savefile.diplstate_type_vector");

    loading->ds_t.order = fc_calloc(loading->ds_t.size,
                                  sizeof(*loading->ds_t.order));

    for (j = 0; j < loading->ds_t.size; j++) {
      loading->ds_t.order[j] = diplstate_type_by_name(modname[j],
                                                    fc_strcasecmp);
    }

    free(modname);
  }

  terrain_type_iterate(pterr) {
    pterr->identifier_load = '\0';
  } terrain_type_iterate_end;

  i = 0;
  while ((terr_name = secfile_lookup_str_default(loading->file, NULL,
                                                 "savefile.terrident%d.name", i)) != NULL) {
    struct terrain *pterr = terrain_by_rule_name(terr_name);

    if (pterr != NULL) {
      const char *iptr =  secfile_lookup_str_default(loading->file, NULL,
                                                     "savefile.terrident%d.identifier", i);

      pterr->identifier_load = *iptr;
    } else {
      log_error("Identifier for unknown terrain type %s.", terr_name);
    }
    i++;
  }

  terrain_type_iterate(pterr) {
    terrain_type_iterate(pterr2) {
      if (pterr != pterr2  && pterr->identifier_load != '\0') {
        sg_failure_ret((pterr->identifier_load != pterr2->identifier_load),
                       "%s and %s share a saved identifier",
                       terrain_rule_name(pterr), terrain_rule_name(pterr2));
      }
    } terrain_type_iterate_end;
  } terrain_type_iterate_end;
}

/* =======================================================================
 * Load game status.
 * ======================================================================= */

/************************************************************************//**
  Load '[ruledata]'.
****************************************************************************/
static void sg_load_ruledata(struct loaddata *loading)
{
  int i;
  const char *name;

  for (i = 0;
       (name = secfile_lookup_str_default(loading->file, NULL,
                                          "ruledata.government%d.name", i));
       i++) {
    struct government *gov = government_by_rule_name(name);

    if (gov != NULL) {
      gov->changed_to_times = secfile_lookup_int_default(loading->file, 0,
                                                         "ruledata.government%d.changes", i);
    }
  }
}

/************************************************************************//**
  Load '[game]'.
****************************************************************************/
static void sg_load_game(struct loaddata *loading)
{
  int game_version;
  const char *string;
  const char *level;
  int i;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Load version. */
  game_version
    = secfile_lookup_int_default(loading->file, 0, "game.version");
  /* We require at least version 2.2.99 */
  sg_failure_ret(20299 <= game_version, "Saved game is too old, at least "
                                        "version 2.2.99 required.");

  /* Load server state. */
  string = secfile_lookup_str_default(loading->file, "S_S_INITIAL",
                                      "game.server_state");
  loading->server_state = server_states_by_name(string, strcmp);
  if (!server_states_is_valid(loading->server_state)) {
    /* Don't take any risk! */
    loading->server_state = S_S_INITIAL;
  }

  string = secfile_lookup_str_default(loading->file,
                                      default_meta_patches_string(),
                                      "game.meta_patches");
  set_meta_patches_string(string);

  if (0 == strcmp(DEFAULT_META_SERVER_ADDR, srvarg.metaserver_addr)) {
    /* Do not overwrite this if the user requested a specific metaserver
     * from the command line (option --Metaserver). */
    sz_strlcpy(srvarg.metaserver_addr,
               secfile_lookup_str_default(loading->file,
                                          DEFAULT_META_SERVER_ADDR,
                                          "game.meta_server"));
  }

  if ('\0' == srvarg.serverid[0]) {
    /* Do not overwrite this if the user requested a specific metaserver
     * from the command line (option --serverid). */
    sz_strlcpy(srvarg.serverid,
               secfile_lookup_str_default(loading->file, "",
                                          "game.serverid"));
  }
  sz_strlcpy(server.game_identifier,
             secfile_lookup_str_default(loading->file, "", "game.id"));
  /* We are not checking game_identifier legality just yet.
   * That's done when we are sure that rand seed has been initialized,
   * so that we can generate new game_identifier, if needed.
   * See sq_load_sanitycheck(). */

  level = secfile_lookup_str_default(loading->file, NULL,
                                     "game.level");
  if (level != NULL) {
    game.info.skill_level = ai_level_by_name(level, fc_strcasecmp);
  } else {
    game.info.skill_level = ai_level_invalid();
  }

  if (!ai_level_is_valid(game.info.skill_level)) {
    game.info.skill_level
      = ai_level_convert(secfile_lookup_int_default(loading->file,
                                                    GAME_HARDCODED_DEFAULT_SKILL_LEVEL,
                                                    "game.skill_level"));
  }
  game.info.phase_mode
    = secfile_lookup_int_default(loading->file, GAME_DEFAULT_PHASE_MODE,
                                 "game.phase_mode");
  game.server.phase_mode_stored
    = secfile_lookup_int_default(loading->file, GAME_DEFAULT_PHASE_MODE,
                                 "game.phase_mode_stored");
  game.info.phase
    = secfile_lookup_int_default(loading->file, 0,
                                 "game.phase");
  game.server.scoreturn
    = secfile_lookup_int_default(loading->file,
                                 game.info.turn + GAME_DEFAULT_SCORETURN,
                                 "game.scoreturn");

  game.server.timeoutint
    = secfile_lookup_int_default(loading->file, GAME_DEFAULT_TIMEOUTINT,
                                 "game.timeoutint");
  game.server.timeoutintinc
    = secfile_lookup_int_default(loading->file, GAME_DEFAULT_TIMEOUTINTINC,
                                 "game.timeoutintinc");
  game.server.timeoutinc
    = secfile_lookup_int_default(loading->file, GAME_DEFAULT_TIMEOUTINC,
                                 "game.timeoutinc");
  game.server.timeoutincmult
    = secfile_lookup_int_default(loading->file, GAME_DEFAULT_TIMEOUTINCMULT,
                                 "game.timeoutincmult");
  game.server.timeoutcounter
    = secfile_lookup_int_default(loading->file, GAME_DEFAULT_TIMEOUTCOUNTER,
                                 "game.timeoutcounter");

  game.info.turn
    = secfile_lookup_int_default(loading->file, 0, "game.turn");
  sg_failure_ret(secfile_lookup_int(loading->file, &game.info.year,
                                    "game.year"), "%s", secfile_error());
  game.info.year_0_hack
    = secfile_lookup_bool_default(loading->file, FALSE, "game.year_0_hack");

  game.info.globalwarming
    = secfile_lookup_int_default(loading->file, 0, "game.globalwarming");
  game.info.heating
    = secfile_lookup_int_default(loading->file, 0, "game.heating");
  game.info.warminglevel
    = secfile_lookup_int_default(loading->file, 0, "game.warminglevel");

  game.info.nuclearwinter
    = secfile_lookup_int_default(loading->file, 0, "game.nuclearwinter");
  game.info.cooling
    = secfile_lookup_int_default(loading->file, 0, "game.cooling");
  game.info.coolinglevel
    = secfile_lookup_int_default(loading->file, 0, "game.coolinglevel");

  /* Global advances. */
  string = secfile_lookup_str_default(loading->file, NULL,
                                      "game.global_advances");
  if (string != NULL) {
    sg_failure_ret(strlen(string) == loading->technology.size,
                   "Invalid length of 'game.global_advances' (%lu ~= %lu).",
                   (unsigned long) strlen(string),
                   (unsigned long) loading->technology.size);
    for (i = 0; i < loading->technology.size; i++) {
      sg_failure_ret(string[i] == '1' || string[i] == '0',
                     "Undefined value '%c' within 'game.global_advances'.",
                     string[i]);
      if (string[i] == '1') {
        struct advance *padvance =
            advance_by_rule_name(loading->technology.order[i]);

        if (padvance != NULL) {
          game.info.global_advances[advance_number(padvance)] = TRUE;
        }
      }
    }
  }

  game.info.is_new_game
    = !secfile_lookup_bool_default(loading->file, TRUE, "game.save_players");

  game.server.turn_change_time
    = secfile_lookup_int_default(loading->file, 0, "game.last_turn_change_time") / 100;
}

/* =======================================================================
 * Load random status.
 * ======================================================================= */

/************************************************************************//**
  Load '[random]'.
****************************************************************************/
static void sg_load_random(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  if (secfile_lookup_bool_default(loading->file, FALSE, "random.saved")) {
    const char *string;
    int i;

    /* Since random state was previously saved, save it also when resaving.
     * This affects only pre-2.6 scenarios where scenario.save_random
     * is not defined.
     * - If this is 2.6 or later scenario -> it would have saved random.saved = TRUE
     *   only if scenario.save_random is already TRUE
     *
     * Do NOT touch this in case of regular savegame. They always have random.saved
     * set, but if one starts to make scenario based on a savegame, we want
     * default scenario settings in the beginning (default save_random = FALSE).
     */
    if (game.scenario.is_scenario) {
      game.scenario.save_random = TRUE;
    }

    sg_failure_ret(secfile_lookup_int(loading->file, &loading->rstate.j,
                                      "random.index_J"), "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file, &loading->rstate.k,
                                      "random.index_K"), "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file, &loading->rstate.x,
                                      "random.index_X"), "%s", secfile_error());

    for (i = 0; i < 8; i++) {
      string = secfile_lookup_str(loading->file, "random.table%d",i);
      sg_failure_ret(NULL != string, "%s", secfile_error());
      sscanf(string, "%8x %8x %8x %8x %8x %8x %8x", &loading->rstate.v[7*i],
             &loading->rstate.v[7*i+1], &loading->rstate.v[7*i+2],
             &loading->rstate.v[7*i+3], &loading->rstate.v[7*i+4],
             &loading->rstate.v[7*i+5], &loading->rstate.v[7*i+6]);
    }
    loading->rstate.is_init = TRUE;
    fc_rand_set_state(loading->rstate);
  } else {
    /* No random values - mark the setting. */
    (void) secfile_entry_by_path(loading->file, "random.saved");

    /* We're loading a game without a seed (which is okay, if it's a scenario).
     * We need to generate the game seed now because it will be needed later
     * during the load. */
    init_game_seed();
    loading->rstate = fc_rand_state();
  }
}

/* =======================================================================
 * Load lua script data.
 * ======================================================================= */

/************************************************************************//**
  Load '[script]'.
****************************************************************************/
static void sg_load_script(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  script_server_state_load(loading->file);
}

/* =======================================================================
 * Load scenario data.
 * ======================================================================= */

/************************************************************************//**
  Load '[scenario]'.
****************************************************************************/
static void sg_load_scenario(struct loaddata *loading)
{
  const char *buf;
  bool lake_flood_default;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  if (NULL == secfile_section_lookup(loading->file, "scenario")) {
    game.scenario.is_scenario = FALSE;

    return;
  }

  /* Default is that when there's scenario section (which we already checked)
   * this is a scenario. Only if it explicitly says that it's not, we consider
   * this regular savegame */
  game.scenario.is_scenario = secfile_lookup_bool_default(loading->file, TRUE, "scenario.is_scenario");

  if (!game.scenario.is_scenario) {
    return;
  }

  buf = secfile_lookup_str_default(loading->file, "", "scenario.name");
  if (buf[0] != '\0') {
    sz_strlcpy(game.scenario.name, buf);
  }

  buf = secfile_lookup_str_default(loading->file, "",
                                   "scenario.authors");
  if (buf[0] != '\0') {
    sz_strlcpy(game.scenario.authors, buf);
  } else {
    game.scenario.authors[0] = '\0';
  }

  buf = secfile_lookup_str_default(loading->file, "",
                                   "scenario.description");
  if (buf[0] != '\0') {
    sz_strlcpy(game.scenario_desc.description, buf);
  } else {
    game.scenario_desc.description[0] = '\0';
  }

  game.scenario.save_random
    = secfile_lookup_bool_default(loading->file, FALSE, "scenario.save_random");
  game.scenario.players
    = secfile_lookup_bool_default(loading->file, TRUE, "scenario.players");
  game.scenario.startpos_nations
    = secfile_lookup_bool_default(loading->file, FALSE,
                                  "scenario.startpos_nations");

  game.scenario.prevent_new_cities
    = secfile_lookup_bool_default(loading->file, FALSE,
                                  "scenario.prevent_new_cities");
  if (loading->version < 20599) {
    /* Lake flooding may break some old scenarios where rivers made out of
     * lake terrains, so play safe there */
    lake_flood_default = FALSE;
  } else {
    /* If lake flooding is a problem for a newer scenario, it could explicitly
     * disable it. */
    lake_flood_default = TRUE;
  }
  game.scenario.lake_flooding
    = secfile_lookup_bool_default(loading->file, lake_flood_default,
                                  "scenario.lake_flooding");
  game.scenario.handmade
    = secfile_lookup_bool_default(loading->file, FALSE,
                                  "scenario.handmade");
  game.scenario.allow_ai_type_fallback
    = secfile_lookup_bool_default(loading->file, FALSE,
                                  "scenario.allow_ai_type_fallback");
  game.scenario.ruleset_locked = TRUE;

  sg_failure_ret(loading->server_state == S_S_INITIAL
                 || (loading->server_state == S_S_RUNNING
                     && game.scenario.players == TRUE),
                 "Invalid scenario definition (server state '%s' and "
                 "players are %s).",
                 server_states_name(loading->server_state),
                 game.scenario.players ? "saved" : "not saved");

  /* Remove all defined players. They are recreated with the skill level
   * defined by the scenario. */
  (void) aifill(0);
}

/* =======================================================================
 * Load game settings.
 * ======================================================================= */

/************************************************************************//**
  Load '[settings]'.
****************************************************************************/
static void sg_load_settings(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  settings_game_load(loading->file, "settings");

  /* Save current status of fogofwar. */
  game.server.fogofwar_old = game.info.fogofwar;

  /* Add all compatibility settings here. */
}

/* =======================================================================
 * Load the main map.
 * ======================================================================= */

/************************************************************************//**
  Load '[map'].
****************************************************************************/
static void sg_load_map(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* This defaults to TRUE even if map has not been generated. Also,
   * old versions have also explicitly saved TRUE even in pre-game.
   * We rely on that
   *   1) scenario maps have it explicitly right.
   *   2) when map is actually generated, it re-initialize this to FALSE. */
  wld.map.server.have_huts
    = secfile_lookup_bool_default(loading->file, TRUE, "map.have_huts");

  if (S_S_INITIAL == loading->server_state
      && MAPGEN_SCENARIO == wld.map.server.generator) {
    /* Generator MAPGEN_SCENARIO is used;
     * this map was done with the map editor. */

    /* Load tiles. */
    sg_load_map_tiles(loading);
    sg_load_map_startpos(loading);

    if (loading->version >= 30) {
      /* 2.6.0 or newer */
      sg_load_map_tiles_extras(loading);
    } else {
      sg_load_map_tiles_bases(loading);
      if (loading->version >= 20) {
        /* 2.5.0 or newer */
        sg_load_map_tiles_roads(loading);
      }
      if (has_capability("specials", loading->secfile_options)) {
        /* Load specials. */
        sg_load_map_tiles_specials(loading, FALSE);
      }
    }

    /* have_resources TRUE only if set so by sg_load_map_tiles_resources() */
    game.scenario.have_resources = FALSE;
    if (has_capability("specials", loading->secfile_options)) {
      /* Load resources. */
      sg_load_map_tiles_resources(loading);
    } else if (has_capability("riversoverlay", loading->secfile_options)) {
      /* Load only rivers overlay. */
      sg_load_map_tiles_specials(loading, TRUE);
    }

    /* Nothing more needed for a scenario. */
    return;
  }

  if (S_S_INITIAL == loading->server_state) {
    /* Nothing more to do if it is not a scenario but in initial state. */
    return;
  }

  sg_load_map_tiles(loading);
  sg_load_map_startpos(loading);
  if (loading->version >= 30) {
    /* 2.6.0 or newer */
    sg_load_map_tiles_extras(loading);
  } else {
    sg_load_map_tiles_bases(loading);
    if (loading->version >= 20) {
      /* 2.5.0 or newer */
      sg_load_map_tiles_roads(loading);
    }
    sg_load_map_tiles_specials(loading, FALSE);
  }
  sg_load_map_tiles_resources(loading);
  sg_load_map_known(loading);
  sg_load_map_owner(loading);
  sg_load_map_worked(loading);
}

/************************************************************************//**
  Load tiles of the map.
****************************************************************************/
static void sg_load_map_tiles(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Initialize the map for the current topology. 'map.xsize' and
   * 'map.ysize' must be set. */
  map_init_topology();

  /* Allocate map. */
  main_map_allocate();

  /* get the terrain type */
  LOAD_MAP_CHAR(ch, ptile, ptile->terrain = char2terrain(ch), loading->file,
                "map.t%04d");
  assign_continent_numbers();

  /* Check for special tile sprites. */
  whole_map_iterate(&(wld.map), ptile) {
    const char *spec_sprite;
    const char *label;
    int nat_x, nat_y;

    index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));
    spec_sprite = secfile_lookup_str(loading->file, "map.spec_sprite_%d_%d",
                                     nat_x, nat_y);
    label = secfile_lookup_str_default(loading->file, NULL, "map.label_%d_%d",
                                       nat_x, nat_y);
    if (NULL != ptile->spec_sprite) {
      ptile->spec_sprite = fc_strdup(spec_sprite);
    }
    if (label != NULL) {
      tile_set_label(ptile, label);
    }
  } whole_map_iterate_end;
}

/************************************************************************//**
  Load extras to map
****************************************************************************/
static void sg_load_map_tiles_extras(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Load extras. */
  halfbyte_iterate_extras(j, loading->extra.size) {
    LOAD_MAP_CHAR(ch, ptile, sg_extras_set(&ptile->extras, ch, loading->extra.order + 4 * j),
                  loading->file, "map.e%02d_%04d", j);
  } halfbyte_iterate_extras_end;
}

/************************************************************************//**
  Load bases to map
****************************************************************************/
static void sg_load_map_tiles_bases(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Load bases. */
  halfbyte_iterate_bases(j, loading->base.size) {
    LOAD_MAP_CHAR(ch, ptile, sg_bases_set(&ptile->extras, ch,
                                          loading->base.order + 4 * j),
                  loading->file, "map.b%02d_%04d", j);
  } halfbyte_iterate_bases_end;
}

/************************************************************************//**
  Load roads to map
****************************************************************************/
static void sg_load_map_tiles_roads(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Load roads. */
  halfbyte_iterate_roads(j, loading->road.size) {
    LOAD_MAP_CHAR(ch, ptile, sg_roads_set(&ptile->extras, ch,
                                          loading->road.order + 4 * j),
                  loading->file, "map.r%02d_%04d", j);
  } halfbyte_iterate_roads_end;
}

/************************************************************************//**
  Load information about specials on map
****************************************************************************/
static void sg_load_map_tiles_specials(struct loaddata *loading,
                                       bool rivers_overlay)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* If 'rivers_overlay' is set to TRUE, load only the rivers overlay map
   * from the savegame file.
   *
   * A scenario may define the terrain of the map but not list the specials
   * on it (thus allowing users to control the placement of specials).
   * However rivers are a special case and must be included in the map along
   * with the scenario. Thus in those cases this function should be called
   * to load the river information separate from any other special data.
   *
   * This does not need to be called from map_load(), because map_load()
   * loads the rivers overlay along with the rest of the specials.  Call this
   * only if you've already called map_load_tiles(), and want to load only
   * the rivers overlay but no other specials. Scenarios that encode things
   * this way should have the "riversoverlay" capability. */
  halfbyte_iterate_special(j, loading->special.size) {
    LOAD_MAP_CHAR(ch, ptile, sg_special_set(ptile, &ptile->extras, ch,
                                            loading->special.order + 4 * j,
                                            rivers_overlay),
                  loading->file, "map.spe%02d_%04d", j);
  } halfbyte_iterate_special_end;
}

/************************************************************************//**
  Load information about resources on map.
****************************************************************************/
static void sg_load_map_tiles_resources(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  LOAD_MAP_CHAR(ch, ptile, tile_set_resource(ptile, char2resource(ch)),
                loading->file, "map.res%04d");

  /* After the resources are loaded, indicate those currently valid. */
  whole_map_iterate(&(wld.map), ptile) {
    if (NULL == ptile->resource) {
      continue;
    }

    if (ptile->terrain == NULL || !terrain_has_resource(ptile->terrain, ptile->resource)) {
      BV_CLR(ptile->extras, extra_index(ptile->resource));
    }
  } whole_map_iterate_end;

  wld.map.server.have_resources = TRUE;
  game.scenario.have_resources = TRUE;
}

/************************************************************************//**
  Load starting positions for the players from a savegame file. There should
  be at least enough for every player.
****************************************************************************/
static void sg_load_map_startpos(struct loaddata *loading)
{
  struct nation_type *pnation;
  struct startpos *psp;
  struct tile *ptile;
  const char SEPARATOR = '#';
  const char *nation_names;
  int nat_x, nat_y;
  bool exclude;
  int i, startpos_count;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  startpos_count
    = secfile_lookup_int_default(loading->file, 0, "map.startpos_count");

  if (0 == startpos_count) {
    /* Nothing to do. */
    return;
  }

  for (i = 0; i < startpos_count; i++) {
    if (!secfile_lookup_int(loading->file, &nat_x, "map.startpos%d.x", i)
        || !secfile_lookup_int(loading->file, &nat_y,
                               "map.startpos%d.y", i)) {
      log_sg("Warning: Undefined coordinates for startpos %d", i);
      continue;
    }

    ptile = native_pos_to_tile(&(wld.map), nat_x, nat_y);
    if (NULL == ptile) {
      log_error("Start position native coordinates (%d, %d) do not exist "
                "in this map. Skipping...", nat_x, nat_y);
      continue;
    }

    exclude = secfile_lookup_bool_default(loading->file, FALSE,
                                          "map.startpos%d.exclude", i);

    psp = map_startpos_new(ptile);

    nation_names = secfile_lookup_str(loading->file,
                                      "map.startpos%d.nations", i);
    if (NULL != nation_names && '\0' != nation_names[0]) {
      const size_t size = strlen(nation_names) + 1;
      char buf[size], *start, *end;

      memcpy(buf, nation_names, size);
      for (start = buf - 1; NULL != start; start = end) {
        start++;
        if ((end = strchr(start, SEPARATOR))) {
          *end = '\0';
        }

        pnation = nation_by_rule_name(start);
        if (NO_NATION_SELECTED != pnation) {
          if (exclude) {
            startpos_disallow(psp, pnation);
          } else {
            startpos_allow(psp, pnation);
          }
        } else {
          log_verbose("Missing nation \"%s\".", start);
        }
      }
    }
  }

  if (0 < map_startpos_count()
      && loading->server_state == S_S_INITIAL
      && map_startpos_count() < game.server.max_players) {
    log_verbose("Number of starts (%d) are lower than rules.max_players "
                "(%d), lowering rules.max_players.",
                map_startpos_count(), game.server.max_players);
    game.server.max_players = map_startpos_count();
  }

  /* Re-initialize nation availability in light of start positions.
   * This has to be after loading [scenario] and [map].startpos and
   * before we seek nations for players. */
  update_nations_with_startpos();
}

/************************************************************************//**
  Load tile owner information
****************************************************************************/
static void sg_load_map_owner(struct loaddata *loading)
{
  int x, y;
  struct player *owner = NULL;
  struct tile *claimer = NULL;
  struct player *eowner = NULL;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  if (game.info.is_new_game) {
    /* No owner/source information for a new game / scenario. */
    return;
  }

  /* Owner and ownership source are stored as plain numbers */
  for (y = 0; y < wld.map.ysize; y++) {
    const char *buffer1 = secfile_lookup_str(loading->file,
                                             "map.owner%04d", y);
    const char *buffer2 = secfile_lookup_str(loading->file,
                                             "map.source%04d", y);
    const char *buffer3 = secfile_lookup_str(loading->file,
                                             "map.eowner%04d", y);
    const char *ptr1 = buffer1;
    const char *ptr2 = buffer2;
    const char *ptr3 = buffer3;

    sg_failure_ret(buffer1 != NULL, "%s", secfile_error());
    sg_failure_ret(buffer2 != NULL, "%s", secfile_error());
    if (loading->version >= 30) {
      sg_failure_ret(buffer3 != NULL, "%s", secfile_error());
    }

    for (x = 0; x < wld.map.xsize; x++) {
      char token1[TOKEN_SIZE];
      char token2[TOKEN_SIZE];
      char token3[TOKEN_SIZE];
      int number;
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

      scanin(&ptr1, ",", token1, sizeof(token1));
      sg_failure_ret(token1[0] != '\0',
                     "Map size not correct (map.owner%d).", y);
      if (strcmp(token1, "-") == 0) {
        owner = NULL;
      } else {
        sg_failure_ret(str_to_int(token1, &number),
                       "Got map owner %s in (%d, %d).", token1, x, y);
        owner = player_by_number(number);
      }

      scanin(&ptr2, ",", token2, sizeof(token2));
      sg_failure_ret(token2[0] != '\0',
                     "Map size not correct (map.source%d).", y);
      if (strcmp(token2, "-") == 0) {
        claimer = NULL;
      } else {
        sg_failure_ret(str_to_int(token2, &number),
                       "Got map source %s in (%d, %d).", token2, x, y);
        claimer = index_to_tile(&(wld.map), number);
      }

      if (loading->version >= 30) {
        scanin(&ptr3, ",", token3, sizeof(token3));
        sg_failure_ret(token3[0] != '\0',
                       "Map size not correct (map.eowner%d).", y);
        if (strcmp(token3, "-") == 0) {
          eowner = NULL;
        } else {
          sg_failure_ret(str_to_int(token3, &number),
                         "Got base owner %s in (%d, %d).", token3, x, y);
          eowner = player_by_number(number);
        }
      } else {
        eowner = owner;
      }

      map_claim_ownership(ptile, owner, claimer, FALSE);
      tile_claim_bases(ptile, eowner);
      log_debug("extras_owner(%d, %d) = %s", TILE_XY(ptile), player_name(eowner));
    }
  }
}

/************************************************************************//**
  Load worked tiles information
****************************************************************************/
static void sg_load_map_worked(struct loaddata *loading)
{
  int x, y;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  sg_failure_ret(loading->worked_tiles == NULL,
                 "City worked map not loaded!");

  loading->worked_tiles = fc_malloc(MAP_INDEX_SIZE *
                                    sizeof(*loading->worked_tiles));

  for (y = 0; y < wld.map.ysize; y++) {
    const char *buffer = secfile_lookup_str(loading->file, "map.worked%04d",
                                            y);
    const char *ptr = buffer;

    sg_failure_ret(NULL != buffer,
                   "Savegame corrupt - map line %d not found.", y);
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      int number;
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

      scanin(&ptr, ",", token, sizeof(token));
      sg_failure_ret('\0' != token[0],
                     "Savegame corrupt - map size not correct.");
      if (strcmp(token, "-") == 0) {
        number = -1;
      } else {
        sg_failure_ret(str_to_int(token, &number) && 0 < number,
                       "Savegame corrupt - got tile worked by city "
                       "id=%s in (%d, %d).", token, x, y);
      }

      loading->worked_tiles[ptile->index] = number;
    }
  }
}

/************************************************************************//**
  Load tile known status
****************************************************************************/
static void sg_load_map_known(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  players_iterate(pplayer) {
    /* Allocate player private map here; it is needed in different modules
     * besides this one ((i.e. sg_load_player_*()). */
    player_map_init(pplayer);
  } players_iterate_end;

  if (secfile_lookup_bool_default(loading->file, TRUE,
                                  "game.save_known")) {
    int lines = player_slot_max_used_number()/32 + 1, j, p, l, i;
    unsigned int *known = fc_calloc(lines * MAP_INDEX_SIZE, sizeof(*known));

    for (l = 0; l < lines; l++) {
      for (j = 0; j < 8; j++) {
        for (i = 0; i < 4; i++) {
          /* Only bother trying to load the map for this halfbyte if at least
           * one of the corresponding player slots is in use. */
          if (player_slot_is_used(player_slot_by_number(l*32 + j*4 + i))) {
            LOAD_MAP_CHAR(ch, ptile,
                          known[l * MAP_INDEX_SIZE + tile_index(ptile)]
                            |= ascii_hex2bin(ch, j),
                          loading->file, "map.k%02d_%04d", l * 8 + j);
            break;
          }
        }
      }
    }

    players_iterate(pplayer) {
      dbv_clr_all(&pplayer->tile_known);
    } players_iterate_end;

    /* HACK: we read the known data from hex into 32-bit integers, and
     * now we convert it to the known tile data of each player. */
    whole_map_iterate(&(wld.map), ptile) {
      players_iterate(pplayer) {
        p = player_index(pplayer);
        l = player_index(pplayer) / 32;

        if (known[l * MAP_INDEX_SIZE + tile_index(ptile)] & (1u << (p % 32))) {
          map_set_known(ptile, pplayer);
        }
      } players_iterate_end;
    } whole_map_iterate_end;

    FC_FREE(known);
  }
}

/* =======================================================================
 * Load player data.
 *
 * This is splitted into two parts as some data can only be loaded if the
 * number of players is known and the corresponding player slots are
 * defined.
 * ======================================================================= */

/************************************************************************//**
  Load '[player]' (basic data).
****************************************************************************/
static void sg_load_players_basic(struct loaddata *loading)
{
  int i, k, nplayers;
  const char *string;
  bool shuffle_loaded = TRUE;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  if (S_S_INITIAL == loading->server_state
      || game.info.is_new_game) {
    /* Nothing more to do. */
    return;
  }

  /* Load destroyed wonders: */
  string = secfile_lookup_str(loading->file,
                              "players.destroyed_wonders");
  sg_failure_ret(string != NULL, "%s", secfile_error());
  sg_failure_ret(strlen(string) == loading->improvement.size,
                 "Invalid length for 'players.destroyed_wonders' "
                 "(%lu ~= %lu)", (unsigned long) strlen(string),
                 (unsigned long) loading->improvement.size);
  for (k = 0; k < loading->improvement.size; k++) {
    sg_failure_ret(string[k] == '1' || string[k] == '0',
                   "Undefined value '%c' within "
                   "'players.destroyed_wonders'.", string[k]);

    if (string[k] == '1') {
      struct impr_type *pimprove =
          improvement_by_rule_name(loading->improvement.order[k]);
      if (pimprove) {
        game.info.great_wonder_owners[improvement_index(pimprove)]
          = WONDER_DESTROYED;
      }
    }
  }

  server.identity_number
    = secfile_lookup_int_default(loading->file, server.identity_number,
                                 "players.identity_number_used");

  /* First remove all defined players. */
  players_iterate(pplayer) {
    server_remove_player(pplayer);
  } players_iterate_end;

  /* Now, load the players from the savefile. */
  player_slots_iterate(pslot) {
    struct player *pplayer;
    struct rgbcolor *prgbcolor = NULL;
    int pslot_id = player_slot_index(pslot);

    if (NULL == secfile_section_lookup(loading->file, "player%d",
                                       pslot_id)) {
      continue;
    }

    /* Get player AI type. */
    string = secfile_lookup_str(loading->file, "player%d.ai_type",
                                player_slot_index(pslot));
    sg_failure_ret(string != NULL, "%s", secfile_error());

    /* Get player color */
    if (!rgbcolor_load(loading->file, &prgbcolor, "player%d.color",
                       pslot_id)) {
      if (loading->version >= 10 && game_was_started()) {
        /* 2.4.0 or later savegame. This is not an error in 2.3 savefiles,
         * as they predate the introduction of configurable player colors. */
        log_sg("Game has started, yet player %d has no color defined.",
               pslot_id);
        /* This will be fixed up later */
      } else {
        log_verbose("No color defined for player %d.", pslot_id);
        /* Colors will be assigned on game start, or at end of savefile
         * loading if game has already started */
      }
    }

    /* Create player. */
    pplayer = server_create_player(player_slot_index(pslot), string,
                                   prgbcolor, game.scenario.allow_ai_type_fallback);
    sg_failure_ret(pplayer != NULL, "Invalid AI type: '%s'!", string);

    server_player_init(pplayer, FALSE, FALSE);

    /* Free the color definition. */
    rgbcolor_destroy(prgbcolor);

    /* Multipliers (policies) */

    /* First initialise player values with ruleset defaults; this will
     * cover any in the ruleset not known when the savefile was created. */
    multipliers_iterate(pmul) {
      pplayer->multipliers[multiplier_index(pmul)]
        = pplayer->multipliers_target[multiplier_index(pmul)] = pmul->def;
    } multipliers_iterate_end;

    /* Now override with any values from the savefile. */
    for (k = 0; k < loading->multiplier.size; k++) {
      const struct multiplier *pmul = loading->multiplier.order[k];

      if (pmul) {
        Multiplier_type_id idx = multiplier_index(pmul);
        int val =
          secfile_lookup_int_default(loading->file, pmul->def,
                                     "player%d.multiplier%d.val",
                                     player_slot_index(pslot), k);
        int rval = (((CLIP(pmul->start, val, pmul->stop)
                      - pmul->start) / pmul->step) * pmul->step) + pmul->start;

        if (rval != val) {
          log_verbose("Player %d had illegal value for multiplier \"%s\": "
                      "was %d, clamped to %d", pslot_id,
                      multiplier_rule_name(pmul), val, rval);
        }
        pplayer->multipliers[idx] = rval;

        val =
          secfile_lookup_int_default(loading->file,
                                     pplayer->multipliers[idx],
                                     "player%d.multiplier%d.target",
                                     player_slot_index(pslot), k);
        rval = (((CLIP(pmul->start, val, pmul->stop)
                  - pmul->start) / pmul->step) * pmul->step) + pmul->start;

        if (rval != val) {
          log_verbose("Player %d had illegal value for multiplier_target "
                      "\"%s\": was %d, clamped to %d", pslot_id,
                      multiplier_rule_name(pmul), val, rval);
        }
        pplayer->multipliers_target[idx] = rval;
      } /* else silently discard multiplier not in current ruleset */
    }

    /* Just in case savecompat starts adding it in the future. */
    pplayer->server.border_vision =
        secfile_lookup_bool_default(loading->file, FALSE,
                                    "player%d.border_vision",
                                    player_slot_index(pslot));
  } player_slots_iterate_end;

  /* check number of players */
  nplayers = secfile_lookup_int_default(loading->file, 0, "players.nplayers");
  sg_failure_ret(player_count() == nplayers, "The value of players.nplayers "
                 "(%d) from the loaded game does not match the number of "
                 "players present (%d).", nplayers, player_count());

  /* Load team informations. */
  players_iterate(pplayer) {
    int team;
    struct team_slot *tslot = NULL;

    sg_failure_ret(secfile_lookup_int(loading->file, &team,
                                      "player%d.team_no",
                                      player_number(pplayer))
                   && (tslot = team_slot_by_number(team)),
                   "Invalid team definition for player %s (nb %d).",
                   player_name(pplayer), player_number(pplayer));
    team_add_player(pplayer, team_new(tslot));
  } players_iterate_end;

  /* Loading the shuffle list is quite complex. At the time of saving the
   * shuffle data is saved as
   *   shuffled_player_<number> = player_slot_id
   * where number is an increasing number and player_slot_id is a number
   * between 0 and the maximum number of player slots. Now we have to create
   * a list
   *   shuffler_players[number] = player_slot_id
   * where all player slot IDs are used exactly one time. The code below
   * handles this ... */
  if (secfile_lookup_int_default(loading->file, -1,
                                 "players.shuffled_player_%d", 0) >= 0) {
    int shuffled_players[player_slot_count()];
    bool shuffled_player_set[player_slot_count()];

    player_slots_iterate(pslot) {
      int plrid = player_slot_index(pslot);

      /* Array to save used numbers. */
      shuffled_player_set[plrid] = FALSE;
      /* List of all player IDs (needed for set_shuffled_players()). It is
       * initialised with the value -1 to indicate that no value is set. */
      shuffled_players[plrid] = -1;
    } player_slots_iterate_end;

    /* Load shuffled player list. */
    for (i = 0; i < player_count(); i++){
      int shuffle
        = secfile_lookup_int_default(loading->file, -1,
                                     "players.shuffled_player_%d", i);

      if (shuffle == -1) {
        log_sg("Missing player shuffle information (index %d) "
               "- reshuffle player list!", i);
        shuffle_loaded = FALSE;
        break;
      } else if (shuffled_player_set[shuffle]) {
        log_sg("Player shuffle %d used two times "
               "- reshuffle player list!", shuffle);
        shuffle_loaded = FALSE;
        break;
      }
      /* Set this ID as used. */
      shuffled_player_set[shuffle] = TRUE;

      /* Save the player ID in the shuffle list. */
      shuffled_players[i] = shuffle;
    }

    if (shuffle_loaded) {
      /* Insert missing numbers. */
      int shuffle_index = player_count();
      for (i = 0; i < player_slot_count(); i++){
        if (!shuffled_player_set[i]) {
          shuffled_players[shuffle_index] = i;
          shuffle_index++;
        }
        /* shuffle_index must not grow behind the size of shuffled_players. */
        sg_failure_ret(shuffle_index <= player_slot_count(),
                       "Invalid player shuffle data!");
      }

#ifdef FREECIV_DEBUG
      log_debug("[load shuffle] player_count() = %d", player_count());
      player_slots_iterate(pslot) {
        int plrid = player_slot_index(pslot);
        log_debug("[load shuffle] id: %3d => slot: %3d | slot %3d: %s",
                  plrid, shuffled_players[plrid], plrid,
                  shuffled_player_set[plrid] ? "is used" : "-");
      } player_slots_iterate_end;
#endif /* FREECIV_DEBUG */

      /* Set shuffle list from savegame. */
      set_shuffled_players(shuffled_players);
    }
  }

  if (!shuffle_loaded) {
    /* No shuffled players included or error loading them, so shuffle them
     * (this may include scenarios). */
    shuffle_players();
  }
}

/************************************************************************//**
  Load '[player]'.
****************************************************************************/
static void sg_load_players(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  if (game.info.is_new_game) {
    /* Nothing to do. */
    return;
  }

  players_iterate(pplayer) {
    sg_load_player_main(loading, pplayer);
    sg_load_player_cities(loading, pplayer);
    sg_load_player_units(loading, pplayer);
    sg_load_player_attributes(loading, pplayer);

    /* Check the success of the functions above. */
    sg_check_ret();

    /* print out some informations */
    if (is_ai(pplayer)) {
      log_normal(_("%s has been added as %s level AI-controlled player "
                   "(%s)."), player_name(pplayer),
                 ai_level_translated_name(pplayer->ai_common.skill_level),
                 ai_name(pplayer->ai));
    } else {
      log_normal(_("%s has been added as human player."),
                 player_name(pplayer));
    }
  } players_iterate_end;

  /* Also load the transport status of the units here. It must be a special
   * case as all units must be known (unit on an allied transporter). */
  players_iterate(pplayer) {
    /* Load unit transport status. */
    sg_load_player_units_transport(loading, pplayer);
  } players_iterate_end;

  /* Savegame may contain nation assignments that are incompatible with the
   * current nationset -- for instance, if it predates the introduction of
   * nationsets. Ensure they are compatible, one way or another. */
  fit_nationset_to_players();

  /* Some players may have invalid nations in the ruleset. Once all players
   * are loaded, pick one of the remaining nations for them. */
  players_iterate(pplayer) {
    if (pplayer->nation == NO_NATION_SELECTED) {
      player_set_nation(pplayer, pick_a_nation(NULL, FALSE, TRUE,
                                               NOT_A_BARBARIAN));
      /* TRANS: Minor error message: <Leader> ... <Poles>. */
      log_sg(_("%s had invalid nation; changing to %s."),
             player_name(pplayer), nation_plural_for_player(pplayer));

      ai_traits_init(pplayer);
    }
  } players_iterate_end;

  /* Sanity check alliances, prevent allied-with-ally-of-enemy. */
  players_iterate_alive(plr) {
    players_iterate_alive(aplayer) {
      if (pplayers_allied(plr, aplayer)) {
        enum dipl_reason can_ally = pplayer_can_make_treaty(plr, aplayer,
                                                            DS_ALLIANCE);
        if (can_ally == DIPL_ALLIANCE_PROBLEM_US
            || can_ally == DIPL_ALLIANCE_PROBLEM_THEM) {
          log_sg("Illegal alliance structure detected: "
                 "%s alliance to %s reduced to peace treaty.",
                 nation_rule_name(nation_of_player(plr)),
                 nation_rule_name(nation_of_player(aplayer)));
          player_diplstate_get(plr, aplayer)->type = DS_PEACE;
          player_diplstate_get(aplayer, plr)->type = DS_PEACE;
        }
      }
    } players_iterate_alive_end;
  } players_iterate_alive_end;

  /* Update cached city illness. This can depend on trade routes,
   * so can't be calculated until all players have been loaded. */
  if (game.info.illness_on) {
    cities_iterate(pcity) {
      pcity->server.illness
        = city_illness_calc(pcity, NULL, NULL,
                            &(pcity->illness_trade), NULL);
    } cities_iterate_end;
  }

  /* Update all city information.  This must come after all cities are
   * loaded (in player_load) but before player (dumb) cities are loaded
   * in player_load_vision(). */
  players_iterate(plr) {
    city_list_iterate(plr->cities, pcity) {
      city_refresh(pcity);
      sanity_check_city(pcity);
      CALL_PLR_AI_FUNC(city_got, plr, plr, pcity);
    } city_list_iterate_end;
  } players_iterate_end;

  /* Since the cities must be placed on the map to put them on the
     player map we do this afterwards */
  players_iterate(pplayer) {
    sg_load_player_vision(loading, pplayer);
    /* Check the success of the function above. */
    sg_check_ret();
  } players_iterate_end;

  /* Check shared vision. */
  players_iterate(pplayer) {
    BV_CLR_ALL(pplayer->gives_shared_vision);
    BV_CLR_ALL(pplayer->server.really_gives_vision);
  } players_iterate_end;

  players_iterate(pplayer) {
    int plr1 = player_index(pplayer);

    players_iterate(pplayer2) {
      int plr2 = player_index(pplayer2);
      if (secfile_lookup_bool_default(loading->file, FALSE,
              "player%d.diplstate%d.gives_shared_vision", plr1, plr2)) {
        give_shared_vision(pplayer, pplayer2);
      }
    } players_iterate_end;
  } players_iterate_end;

  initialize_globals();
  unit_ordering_apply();

  /* All vision is ready; this calls city_thaw_workers_queue(). */
  map_calculate_borders();

  /* Make sure everything is consistent. */
  players_iterate(pplayer) {
    unit_list_iterate(pplayer->units, punit) {
      if (!can_unit_continue_current_activity(punit)) {
        log_sg("Unit doing illegal activity in savegame!");
        log_sg("Activity: %s, Target: %s",
               unit_activity_name(punit->activity),
               punit->activity_target ? extra_rule_name(
                                          punit->activity_target)
                                      : "missing");
        punit->activity = ACTIVITY_IDLE;
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  cities_iterate(pcity) {
    city_refresh(pcity);
    city_thaw_workers(pcity); /* may auto_arrange_workers() */
  } cities_iterate_end;

  /* Player colors are always needed once game has started. Pre-2.4 savegames
   * lack them. This cannot be in compatibility conversion layer as we need
   * all the player data available to be able to assign best colors. */
  if (game_was_started()) {
    assign_player_colors();
  }
}

/************************************************************************//**
  Main player data loading function
****************************************************************************/
static void sg_load_player_main(struct loaddata *loading,
                                struct player *plr)
{
  const char **slist;
  int i, plrno = player_number(plr);
  const char *string;
  struct government *gov;
  const char *level;
  const char *barb_str;
  size_t nval;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Basic player data. */
  string = secfile_lookup_str(loading->file, "player%d.name", plrno);
  sg_failure_ret(string != NULL, "%s", secfile_error());
  server_player_set_name(plr, string);
  sz_strlcpy(plr->username,
             secfile_lookup_str_default(loading->file, "",
                                        "player%d.username", plrno));
  sg_failure_ret(secfile_lookup_bool(loading->file, &plr->unassigned_user,
                                     "player%d.unassigned_user", plrno),
                 "%s", secfile_error());
  sz_strlcpy(plr->server.orig_username,
             secfile_lookup_str_default(loading->file, "",
                                        "player%d.orig_username",
                                        plrno));
  sz_strlcpy(plr->ranked_username,
             secfile_lookup_str_default(loading->file, "",
                                        "player%d.ranked_username",
                                        plrno));
  sg_failure_ret(secfile_lookup_bool(loading->file, &plr->unassigned_ranked,
                                     "player%d.unassigned_ranked", plrno),
                 "%s", secfile_error());
  string = secfile_lookup_str_default(loading->file, "",
                                      "player%d.delegation_username",
                                      plrno);
  /* Defaults to no delegation. */
  if (strlen(string)) {
    player_delegation_set(plr, string);
  }

  /* Player flags */
  BV_CLR_ALL(plr->flags);
  slist = secfile_lookup_str_vec(loading->file, &nval, "player%d.flags", plrno);
  for (i = 0; i < nval; i++) {
    const char *sval = slist[i];
    enum plr_flag_id fid = plr_flag_id_by_name(sval, fc_strcasecmp);

    BV_SET(plr->flags, fid);
  }
  free(slist);

  /* Nation */
  string = secfile_lookup_str(loading->file, "player%d.nation", plrno);
  player_set_nation(plr, nation_by_rule_name(string));
  if (plr->nation != NULL) {
    ai_traits_init(plr);
  }

  /* Government */
  string = secfile_lookup_str(loading->file, "player%d.government_name",
                              plrno);
  gov = government_by_rule_name(string);
  sg_failure_ret(gov != NULL, "Player%d: unsupported government \"%s\".",
                 plrno, string);
  plr->government = gov;

  /* Target government */
  string = secfile_lookup_str(loading->file,
                              "player%d.target_government_name", plrno);
  if (string) {
    plr->target_government = government_by_rule_name(string);
  } else {
    plr->target_government = NULL;
  }
  plr->revolution_finishes
    = secfile_lookup_int_default(loading->file, -1,
                                 "player%d.revolution_finishes", plrno);

  sg_failure_ret(secfile_lookup_bool(loading->file,
                                     &plr->server.got_first_city,
                                     "player%d.got_first_city", plrno),
                 "%s", secfile_error());

  /* Load diplomatic data (diplstate + embassy + vision).
   * Shared vision is loaded in sg_load_players(). */
  BV_CLR_ALL(plr->real_embassy);
  players_iterate(pplayer) {
    char buf[32];
    int unconverted;
    struct player_diplstate *ds = player_diplstate_get(plr, pplayer);
    i = player_index(pplayer);

    /* load diplomatic status */
    fc_snprintf(buf, sizeof(buf), "player%d.diplstate%d", plrno, i);

    unconverted =
      secfile_lookup_int_default(loading->file, -1, "%s.type", buf);
    if (unconverted >= 0 && unconverted < loading->ds_t.size) {
      /* Look up what state the unconverted number represents. */
      ds->type = loading->ds_t.order[unconverted];
    } else {
      log_sg("No valid diplomatic state type between players %d and %d",
             plrno, i);

      ds->type = DS_WAR;
    }

    unconverted =
      secfile_lookup_int_default(loading->file, -1, "%s.max_state", buf);
    if (unconverted >= 0 && unconverted < loading->ds_t.size) {
      /* Look up what state the unconverted number represents. */
      ds->max_state = loading->ds_t.order[unconverted];
    } else {
      log_sg("No valid diplomatic max_state between players %d and %d",
             plrno, i);

      ds->max_state = DS_WAR;
    }

    ds->first_contact_turn =
      secfile_lookup_int_default(loading->file, 0,
                                 "%s.first_contact_turn", buf);
    ds->turns_left =
      secfile_lookup_int_default(loading->file, -2, "%s.turns_left", buf);
    ds->has_reason_to_cancel =
      secfile_lookup_int_default(loading->file, 0,
                                 "%s.has_reason_to_cancel", buf);
    ds->contact_turns_left =
      secfile_lookup_int_default(loading->file, 0,
                                 "%s.contact_turns_left", buf);

    if (secfile_lookup_bool_default(loading->file, FALSE, "%s.embassy",
                                    buf)) {
      BV_SET(plr->real_embassy, i);
    }
    /* 'gives_shared_vision' is loaded in sg_load_players() as all cities
     * must be known. */
  } players_iterate_end;

  /* load ai data */
  players_iterate(aplayer) {
    char buf[32];

    fc_snprintf(buf, sizeof(buf), "player%d.ai%d", plrno,
                player_index(aplayer));

    plr->ai_common.love[player_index(aplayer)] =
        secfile_lookup_int_default(loading->file, 1, "%s.love", buf);
    CALL_FUNC_EACH_AI(player_load_relations, plr, aplayer, loading->file, plrno);
  } players_iterate_end;

  CALL_FUNC_EACH_AI(player_load, plr, loading->file, plrno);

  /* Some sane defaults */
  plr->ai_common.fuzzy = 0;
  plr->ai_common.expand = 100;
  plr->ai_common.science_cost = 100;


  level = secfile_lookup_str_default(loading->file, NULL,
                                     "player%d.ai.level", plrno);
  if (level != NULL) {
    plr->ai_common.skill_level = ai_level_by_name(level, fc_strcasecmp);

    /* In builds where level "Experimental" is not supported, convert it to "Hard" */
    if (!ai_level_is_valid(plr->ai_common.skill_level)
        && !fc_strcasecmp(level, "Experimental")) {
      plr->ai_common.skill_level = AI_LEVEL_HARD;
    }
  } else {
    plr->ai_common.skill_level = ai_level_invalid();
  }

  if (!ai_level_is_valid(plr->ai_common.skill_level)) {
    plr->ai_common.skill_level
      = ai_level_convert(secfile_lookup_int_default(loading->file,
                                                    game.info.skill_level,
                                                    "player%d.ai.skill_level",
                                                    plrno));
  }

  barb_str = secfile_lookup_str_default(loading->file, "None",
                                        "player%d.ai.barb_type", plrno);
  plr->ai_common.barbarian_type = barbarian_type_by_name(barb_str, fc_strcasecmp);

  if (!barbarian_type_is_valid(plr->ai_common.barbarian_type)) {
    log_sg("Player%d: Invalid barbarian type \"%s\". "
           "Changed to \"None\".", plrno, barb_str);
    plr->ai_common.barbarian_type = NOT_A_BARBARIAN;
  }

  if (is_barbarian(plr)) {
    server.nbarbarians++;
  }

  if (is_ai(plr)) {
    set_ai_level_directer(plr, plr->ai_common.skill_level);
    CALL_PLR_AI_FUNC(gained_control, plr, plr);
  }

  /* Load nation style. */
  {
    struct nation_style *style;

    string = secfile_lookup_str(loading->file, "player%d.style_by_name", plrno);

    /* Handle pre-2.6 savegames */
    if (string == NULL) {
      string = secfile_lookup_str(loading->file, "player%d.city_style_by_name",
                                  plrno);
    }

    sg_failure_ret(string != NULL, "%s", secfile_error());
    style = style_by_rule_name(string);
    if (style == NULL) {
      style = style_by_number(0);
      log_sg("Player%d: unsupported city_style_name \"%s\". "
             "Changed to \"%s\".", plrno, string, style_rule_name(style));
    }
    plr->style = style;
  }

  sg_failure_ret(secfile_lookup_int(loading->file, &plr->nturns_idle,
                                    "player%d.idle_turns", plrno),
                 "%s", secfile_error());
  plr->is_male = secfile_lookup_bool_default(loading->file, TRUE,
                                             "player%d.is_male", plrno);
  sg_failure_ret(secfile_lookup_bool(loading->file, &plr->is_alive,
                                     "player%d.is_alive", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->turns_alive,
                                    "player%d.turns_alive", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->last_war_action,
                                    "player%d.last_war", plrno),
                 "%s", secfile_error());
  plr->phase_done = secfile_lookup_bool_default(loading->file, FALSE,
                                                "player%d.phase_done", plrno);
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.gold,
                                    "player%d.gold", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.tax,
                                    "player%d.rates.tax", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.science,
                                    "player%d.rates.science", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.luxury,
                                    "player%d.rates.luxury", plrno),
                 "%s", secfile_error());
  plr->server.bulbs_last_turn =
    secfile_lookup_int_default(loading->file, 0,
                               "player%d.research.bulbs_last_turn", plrno);

  /* Traits */
  if (plr->nation) {
    for (i = 0; i < loading->trait.size; i++) {
      enum trait tr = trait_by_name(loading->trait.order[i], fc_strcasecmp);

      if (trait_is_valid(tr)) {
        int val = secfile_lookup_int_default(loading->file, -1, "player%d.trait%d.val",
                                             plrno, i);

        if (val != -1) {
          plr->ai_common.traits[tr].val = val;
        }

        sg_failure_ret(secfile_lookup_int(loading->file, &val,
                                          "player%d.trait%d.mod", plrno, i),
                       "%s", secfile_error());
        plr->ai_common.traits[tr].mod = val;
      }
    }
  }

  /* Achievements */
  {
    int count;

    count = secfile_lookup_int_default(loading->file, -1,
                                       "player%d.achievement_count", plrno);

    if (count > 0) {
      for (i = 0; i < count; i++) {
        const char *name;
        struct achievement *pach;
        bool first;

        name = secfile_lookup_str(loading->file,
                                  "player%d.achievement%d.name", plrno, i);
        pach = achievement_by_rule_name(name);

        sg_failure_ret(pach != NULL,
                       "Unknown achievement \"%s\".", name);

        sg_failure_ret(secfile_lookup_bool(loading->file, &first,
                                           "player%d.achievement%d.first",
                                           plrno, i),
                       "achievement error: %s", secfile_error());

        sg_failure_ret(pach->first == NULL || !first,
                       "Multiple players listed as first to get achievement \"%s\".",
                       name);

        BV_SET(pach->achievers, player_index(plr));

        if (first) {
          pach->first = plr;
        }
      }
    }
  }

  /* Player score. */
  plr->score.happy =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.happy", plrno);
  plr->score.content =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.content", plrno);
  plr->score.unhappy =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.unhappy", plrno);
  plr->score.angry =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.angry", plrno);

  /* Make sure that the score about specialists in current ruleset that
   * were not present at saving time are set to zero. */
  specialist_type_iterate(sp) {
    plr->score.specialists[sp] = 0;
  } specialist_type_iterate_end;

  for (i = 0; i < loading->specialist.size; i++) {
    plr->score.specialists[specialist_index(loading->specialist.order[i])]
      = secfile_lookup_int_default(loading->file, 0,
                                   "score%d.specialists%d", plrno, i);
  }

  plr->score.wonders =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.wonders", plrno);
  plr->score.techs =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.techs", plrno);
  plr->score.techout =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.techout", plrno);
  plr->score.landarea =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.landarea", plrno);
  plr->score.settledarea =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.settledarea", plrno);
  plr->score.population =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.population", plrno);
  plr->score.cities =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.cities", plrno);
  plr->score.units =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.units", plrno);
  plr->score.pollution =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.pollution", plrno);
  plr->score.literacy =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.literacy", plrno);
  plr->score.bnp =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.bnp", plrno);
  plr->score.mfg =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.mfg", plrno);
  plr->score.spaceship =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.spaceship", plrno);
  plr->score.units_built =
      secfile_lookup_int_default(loading->file, 0,
                                 "score%d.units_built", plrno);
  plr->score.units_killed =
      secfile_lookup_int_default(loading->file, 0,
                                 "score%d.units_killed", plrno);
  plr->score.units_lost =
      secfile_lookup_int_default(loading->file, 0,
                                 "score%d.units_lost", plrno);
  plr->score.culture =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.culture", plrno);
  plr->score.game =
    secfile_lookup_int_default(loading->file, 0,
                               "score%d.total", plrno);

  /* Load space ship data. */
  {
    struct player_spaceship *ship = &plr->spaceship;
    char prefix[32];
    const char *st;
    int ei;

    fc_snprintf(prefix, sizeof(prefix), "player%d.spaceship", plrno);
    spaceship_init(ship);
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &ei,
                                      "%s.state", prefix),
                   "%s", secfile_error());
    ship->state = ei;

    if (ship->state != SSHIP_NONE) {
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->structurals,
                                        "%s.structurals", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->components,
                                 "%s.components", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->modules,
                                 "%s.modules", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->fuel,
                                 "%s.fuel", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->propulsion,
                                 "%s.propulsion", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->habitation,
                                 "%s.habitation", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->life_support,
                                 "%s.life_support", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->solar_panels,
                                 "%s.solar_panels", prefix),
                     "%s", secfile_error());

      st = secfile_lookup_str(loading->file, "%s.structure", prefix);
      sg_failure_ret(st != NULL, "%s", secfile_error())
      for (i = 0; i < NUM_SS_STRUCTURALS && st[i]; i++) {
        sg_failure_ret(st[i] == '1' || st[i] == '0',
                       "Undefined value '%c' within '%s.structure'.", st[i],
                       prefix)

        if (!(st[i] == '0')) {
          BV_SET(ship->structure, i);
        }
      }
      if (ship->state >= SSHIP_LAUNCHED) {
        sg_failure_ret(secfile_lookup_int(loading->file, &ship->launch_year,
                                          "%s.launch_year", prefix),
                       "%s", secfile_error());
      }
      spaceship_calc_derived(ship);
    }
  }

  /* Load lost wonder data. */
  string = secfile_lookup_str(loading->file, "player%d.lost_wonders", plrno);
  /* If not present, probably an old savegame; nothing to be done */
  if (string) {
    int k;
    sg_failure_ret(strlen(string) == loading->improvement.size,
                   "Invalid length for 'player%d.lost_wonders' "
                   "(%lu ~= %lu)", plrno, (unsigned long) strlen(string),
                   (unsigned long) loading->improvement.size);
    for (k = 0; k < loading->improvement.size; k++) {
      sg_failure_ret(string[k] == '1' || string[k] == '0',
                     "Undefined value '%c' within "
                     "'player%d.lost_wonders'.", plrno, string[k]);

      if (string[k] == '1') {
        struct impr_type *pimprove =
            improvement_by_rule_name(loading->improvement.order[k]);
        if (pimprove) {
          plr->wonders[improvement_index(pimprove)] = WONDER_LOST;
        }
      }
    }
  }

  plr->culture =
    secfile_lookup_int_default(loading->file, 0, "player%d.culture", plrno);
  plr->server.huts =
    secfile_lookup_int_default(loading->file, 0, "player%d.hut_count", plrno);
}

/************************************************************************//**
  Load city data
****************************************************************************/
static void sg_load_player_cities(struct loaddata *loading,
                                  struct player *plr)
{
  int ncities, i, plrno = player_number(plr);
  bool tasks_handled;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  sg_failure_ret(secfile_lookup_int(loading->file, &ncities,
                                    "player%d.ncities", plrno),
                 "%s", secfile_error());

  if (!plr->is_alive && ncities > 0) {
    log_sg("'player%d.ncities' = %d for dead player!", plrno, ncities);
    ncities = 0;
  }

  if (!plr->server.got_first_city && ncities > 0) {
    /* Probably barbarians in an old savegame; fix up */
    plr->server.got_first_city = TRUE;
  }

  /* Load all cities of the player. */
  for (i = 0; i < ncities; i++) {
    char buf[32];
    struct city *pcity;

    fc_snprintf(buf, sizeof(buf), "player%d.c%d", plrno, i);

    /* Create a dummy city. */
    pcity = create_city_virtual(plr, NULL, buf);
    adv_city_alloc(pcity);
    if (!sg_load_player_city(loading, plr, pcity, buf)) {
      adv_city_free(pcity);
      destroy_city_virtual(pcity);
      sg_failure_ret(FALSE, "Error loading city %d of player %d.", i, plrno);
    }

    identity_number_reserve(pcity->id);
    idex_register_city(&wld, pcity);

    /* Load the information about the nationality of citizens. This is done
     * here because the city sanity check called by citizens_update() requires
     * that the city is registered. */
    sg_load_player_city_citizens(loading, plr, pcity, buf);

    /* After everything is loaded, but before vision. */
    map_claim_ownership(city_tile(pcity), plr, city_tile(pcity), TRUE);

    /* adding the city contribution to fog-of-war */
    pcity->server.vision = vision_new(plr, city_tile(pcity));
    vision_reveal_tiles(pcity->server.vision, game.server.vision_reveal_tiles);
    city_refresh_vision(pcity);

    city_list_append(plr->cities, pcity);
  }

  tasks_handled = FALSE;
  for (i = 0; !tasks_handled; i++) {
    int city_id;
    struct city *pcity = NULL;

    city_id = secfile_lookup_int_default(loading->file, -1, "player%d.task%d.city",
                                         plrno, i);

    if (city_id != -1) {
      pcity = player_city_by_number(plr, city_id);
    }

    if (pcity != NULL) {
      const char *str;
      int nat_x, nat_y;
      struct worker_task *ptask = fc_malloc(sizeof(struct worker_task));

      nat_x = secfile_lookup_int_default(loading->file, -1, "player%d.task%d.x", plrno, i);
      nat_y = secfile_lookup_int_default(loading->file, -1, "player%d.task%d.y", plrno, i);

      ptask->ptile = native_pos_to_tile(&(wld.map), nat_x, nat_y);

      str = secfile_lookup_str(loading->file, "player%d.task%d.activity", plrno, i);
      ptask->act = unit_activity_by_name(str, fc_strcasecmp);

      sg_failure_ret(unit_activity_is_valid(ptask->act),
                     "Unknown workertask activity %s", str);

      str = secfile_lookup_str(loading->file, "player%d.task%d.target", plrno, i);

      if (strcmp("-", str)) {
        ptask->tgt = extra_type_by_rule_name(str);

        sg_failure_ret(ptask->tgt != NULL,
                       "Unknown workertask target %s", str);
      } else {
        ptask->tgt = NULL;
      }

      ptask->want = secfile_lookup_int_default(loading->file, 1,
                                               "player%d.task%d.want", plrno, i);

      worker_task_list_append(pcity->task_reqs, ptask);
    } else {
      tasks_handled = TRUE;
    }
  }
}

/************************************************************************//**
  Load data for one city.
****************************************************************************/
static bool sg_load_player_city(struct loaddata *loading, struct player *plr,
                                struct city *pcity, const char *citystr)
{
  struct player *past;
  const char *kind, *name, *string;
  int id, i, repair, sp_count = 0, workers = 0, value;
  int nat_x, nat_y;
  citizens size;
  const char *stylename;

  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_x, "%s.x", citystr),
                  FALSE, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_y, "%s.y", citystr),
                  FALSE, "%s", secfile_error());
  pcity->tile = native_pos_to_tile(&(wld.map), nat_x, nat_y);
  sg_warn_ret_val(NULL != pcity->tile, FALSE,
                  "%s has invalid center tile (%d, %d)",
                  citystr, nat_x, nat_y);
  sg_warn_ret_val(NULL == tile_city(pcity->tile), FALSE,
                  "%s duplicates city (%d, %d)", citystr, nat_x, nat_y);

  /* Instead of dying, use 'citystr' string for damaged name. */
  sz_strlcpy(pcity->name, secfile_lookup_str_default(loading->file, citystr,
                                                     "%s.name", citystr));

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->id, "%s.id",
                                     citystr), FALSE, "%s", secfile_error());

  id = secfile_lookup_int_default(loading->file, player_number(plr),
                                  "%s.original", citystr);
  past = player_by_number(id);
  if (NULL != past) {
    pcity->original = past;
  }

  sg_warn_ret_val(secfile_lookup_int(loading->file, &value, "%s.size",
                                     citystr), FALSE, "%s", secfile_error());
  size = (citizens)value; /* set the correct type */
  sg_warn_ret_val(value == (int)size, FALSE,
                  "Invalid city size: %d, set to %d", value, size);
  city_size_set(pcity, size);

  for (i = 0; i < loading->specialist.size; i++) {
    sg_warn_ret_val(secfile_lookup_int(loading->file, &value, "%s.nspe%d",
                                       citystr, i),
                    FALSE, "%s", secfile_error());
    pcity->specialists[specialist_index(loading->specialist.order[i])]
      = (citizens)value;
    sp_count += value;
  }

  for (i = 0; i < MAX_TRADE_ROUTES; i++) {
    int partner = secfile_lookup_int_default(loading->file, 0,
                                             "%s.traderoute%d", citystr, i);

    if (partner != 0) {
      struct trade_route *proute = fc_malloc(sizeof(struct trade_route));

      proute->partner = partner;
      proute->dir = RDIR_BIDIRECTIONAL;
      proute->goods = goods_by_number(0); /* First good */

      trade_route_list_append(pcity->routes, proute);
    }
  }

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->food_stock,
                                    "%s.food_stock", citystr),
                  FALSE, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->shield_stock,
                                    "%s.shield_stock", citystr),
                  FALSE, "%s", secfile_error());
  pcity->history =
    secfile_lookup_int_default(loading->file, 0, "%s.history", citystr);

  pcity->airlift =
    secfile_lookup_int_default(loading->file, 0, "%s.airlift", citystr);
  pcity->was_happy =
    secfile_lookup_bool_default(loading->file, FALSE, "%s.was_happy",
                                citystr);

  pcity->turn_plague =
    secfile_lookup_int_default(loading->file, 0, "%s.turn_plague", citystr);

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->anarchy,
                                    "%s.anarchy", citystr),
                  FALSE, "%s", secfile_error());
  pcity->rapture =
    secfile_lookup_int_default(loading->file, 0, "%s.rapture", citystr);
  pcity->server.steal =
    secfile_lookup_int_default(loading->file, 0, "%s.steal", citystr);

  /* before did_buy for undocumented hack */
  pcity->turn_founded =
    secfile_lookup_int_default(loading->file, -2, "%s.turn_founded",
                               citystr);
  sg_warn_ret_val(secfile_lookup_int(loading->file, &i, "%s.did_buy",
                                     citystr), FALSE, "%s", secfile_error());
  pcity->did_buy = (i != 0);
  if (i == -1 && pcity->turn_founded == -2) {
    /* undocumented hack */
    pcity->turn_founded = game.info.turn;
  }

  pcity->did_sell =
    secfile_lookup_bool_default(loading->file, FALSE, "%s.did_sell", citystr);

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->turn_last_built,
                                    "%s.turn_last_built", citystr),
                  FALSE, "%s", secfile_error());

  kind = secfile_lookup_str(loading->file, "%s.currently_building_kind",
                            citystr);
  name = secfile_lookup_str(loading->file, "%s.currently_building_name",
                            citystr);
  pcity->production = universal_by_rule_name(kind, name);
  sg_warn_ret_val(pcity->production.kind != universals_n_invalid(), FALSE,
                  "%s.currently_building: unknown \"%s\" \"%s\".",
                  citystr, kind, name);

  kind = secfile_lookup_str(loading->file, "%s.changed_from_kind",
                            citystr);
  name = secfile_lookup_str(loading->file, "%s.changed_from_name",
                            citystr);
  pcity->changed_from = universal_by_rule_name(kind, name);
  sg_warn_ret_val(pcity->changed_from.kind != universals_n_invalid(), FALSE,
                 "%s.changed_from: unknown \"%s\" \"%s\".",
                 citystr, kind, name);

  pcity->before_change_shields =
    secfile_lookup_int_default(loading->file, pcity->shield_stock,
                               "%s.before_change_shields", citystr);
  pcity->caravan_shields =
    secfile_lookup_int_default(loading->file, 0,
                               "%s.caravan_shields", citystr);
  pcity->disbanded_shields =
    secfile_lookup_int_default(loading->file, 0,
                               "%s.disbanded_shields", citystr);
  pcity->last_turns_shield_surplus =
    secfile_lookup_int_default(loading->file, 0,
                               "%s.last_turns_shield_surplus",
                               citystr);

  stylename = secfile_lookup_str_default(loading->file, NULL,
                                         "%s.style", citystr);
  if (stylename != NULL) {
    pcity->style = city_style_by_rule_name(stylename);
  } else {
    pcity->style = 0;
  }
  if (pcity->style < 0) {
    pcity->style = city_style(pcity);
  }

  pcity->server.synced = FALSE; /* must re-sync with clients */

  /* Initialise list of city improvements. */
  for (i = 0; i < ARRAY_SIZE(pcity->built); i++) {
    pcity->built[i].turn = I_NEVER;
  }

  /* Load city improvements. */
  string = secfile_lookup_str(loading->file, "%s.improvements", citystr);
  sg_warn_ret_val(string != NULL, FALSE, "%s", secfile_error());
  sg_warn_ret_val(strlen(string) == loading->improvement.size, FALSE,
                  "Invalid length of '%s.improvements' (%lu ~= %lu).",
                  citystr, (unsigned long) strlen(string),
                  (unsigned long) loading->improvement.size);
  for (i = 0; i < loading->improvement.size; i++) {
    sg_warn_ret_val(string[i] == '1' || string[i] == '0', FALSE,
                   "Undefined value '%c' within '%s.improvements'.",
                   string[i], citystr)

    if (string[i] == '1') {
      struct impr_type *pimprove =
          improvement_by_rule_name(loading->improvement.order[i]);
      if (pimprove) {
        city_add_improvement(pcity, pimprove);
      }
    }
  }

  sg_failure_ret_val(loading->worked_tiles != NULL, FALSE,
                     "No worked tiles map defined.");

  city_freeze_workers(pcity);

  /* load new savegame with variable (squared) city radius and worked
   * tiles map */

  int radius_sq
    = secfile_lookup_int_default(loading->file, -1, "%s.city_radius_sq",
                                 citystr);
  city_map_radius_sq_set(pcity, radius_sq);

  city_tile_iterate(radius_sq, city_tile(pcity), ptile) {
    if (loading->worked_tiles[ptile->index] == pcity->id) {
      tile_set_worked(ptile, pcity);
      workers++;

#ifdef FREECIV_DEBUG
      /* set this tile to unused; a check for not resetted tiles is
       * included in game_load_internal() */
      loading->worked_tiles[ptile->index] = -1;
#endif /* FREECIV_DEBUG */
    }
  } city_tile_iterate_end;

  if (tile_worked(city_tile(pcity)) != pcity) {
    struct city *pwork = tile_worked(city_tile(pcity));

    if (NULL != pwork) {
      log_sg("[%s] city center of '%s' (%d,%d) [%d] is worked by '%s' "
             "(%d,%d) [%d]; repairing ", citystr, city_name_get(pcity),
             TILE_XY(city_tile(pcity)), city_size_get(pcity), city_name_get(pwork),
             TILE_XY(city_tile(pwork)), city_size_get(pwork));

      tile_set_worked(city_tile(pcity), NULL); /* remove tile from pwork */
      pwork->specialists[DEFAULT_SPECIALIST]++;
      auto_arrange_workers(pwork);
    } else {
      log_sg("[%s] city center of '%s' (%d,%d) [%d] is empty; repairing ",
             citystr, city_name_get(pcity), TILE_XY(city_tile(pcity)),
             city_size_get(pcity));
    }

    /* repair pcity */
    tile_set_worked(city_tile(pcity), pcity);
    city_repair_size(pcity, -1);
  }

  repair = city_size_get(pcity) - sp_count - (workers - FREE_WORKED_TILES);
  if (0 != repair) {
    log_sg("[%s] size mismatch for '%s' (%d,%d): size [%d] != "
           "(workers [%d] - free worked tiles [%d]) + specialists [%d]",
           citystr, city_name_get(pcity), TILE_XY(city_tile(pcity)), city_size_get(pcity),
           workers, FREE_WORKED_TILES, sp_count);

    /* repair pcity */
    city_repair_size(pcity, repair);
  }

  /* worklist_init() done in create_city_virtual() */
  worklist_load(loading->file, &pcity->worklist, "%s", citystr);

  /* Load city options. */
  BV_CLR_ALL(pcity->city_options);
  for (i = 0; i < CITYO_LAST; i++) {
    if (secfile_lookup_bool_default(loading->file, FALSE, "%s.option%d",
                                    citystr, i)) {
      BV_SET(pcity->city_options, i);
    }
  }

  CALL_FUNC_EACH_AI(city_load, loading->file, pcity, citystr);

  return TRUE;
}

/************************************************************************//**
  Load nationality data for one city.
****************************************************************************/
static void sg_load_player_city_citizens(struct loaddata *loading,
                                         struct player *plr,
                                         struct city *pcity,
                                         const char *citystr)
{
  if (game.info.citizen_nationality) {
    citizens size;

    citizens_init(pcity);
    player_slots_iterate(pslot) {
      int nationality;

      nationality = secfile_lookup_int_default(loading->file, -1,
                                               "%s.citizen%d", citystr,
                                               player_slot_index(pslot));
      if (nationality > 0 && !player_slot_is_used(pslot)) {
        log_sg("Citizens of an invalid nation for %s (player slot %d)!",
               city_name_get(pcity), player_slot_index(pslot));
        continue;
      }

      if (nationality != -1 && player_slot_is_used(pslot)) {
        sg_warn(nationality >= 0 && nationality <= MAX_CITY_SIZE,
                "Invalid value for citizens of player %d in %s: %d.",
                player_slot_index(pslot), city_name_get(pcity), nationality);
        citizens_nation_set(pcity, pslot, nationality);
      }
    } player_slots_iterate_end;
    /* Sanity check. */
    size = citizens_count(pcity);
    if (size != city_size_get(pcity)) {
      if (size != 0) {
        /* size == 0 can be result from the fact that ruleset had no
         * nationality enabled at saving time, so no citizens at all
         * were saved. But something more serious must be going on if
         * citizens have been saved partially - if some of them are there. */
        log_sg("City size and number of citizens does not match in %s "
               "(%d != %d)! Repairing ...", city_name_get(pcity),
               city_size_get(pcity), size);
      }
      citizens_update(pcity, NULL);
    }
  }
}

/************************************************************************//**
  Load unit data
****************************************************************************/
static void sg_load_player_units(struct loaddata *loading,
                                 struct player *plr)
{
  int nunits, i, plrno = player_number(plr);

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  sg_failure_ret(secfile_lookup_int(loading->file, &nunits,
                                    "player%d.nunits", plrno),
                 "%s", secfile_error());
  if (!plr->is_alive && nunits > 0) {
    log_sg("'player%d.nunits' = %d for dead player!", plrno, nunits);
    nunits = 0; /* Some old savegames may be buggy. */
  }

  for (i = 0; i < nunits; i++) {
    struct unit *punit;
    struct city *pcity;
    const char *name;
    char buf[32];
    struct unit_type *type;
    struct tile *ptile;

    fc_snprintf(buf, sizeof(buf), "player%d.u%d", plrno, i);

    name = secfile_lookup_str(loading->file, "%s.type_by_name", buf);
    type = unit_type_by_rule_name(name);
    sg_failure_ret(type != NULL, "%s: unknown unit type \"%s\".", buf, name);

    /* Create a dummy unit. */
    punit = unit_virtual_create(plr, NULL, type, 0);
    if (!sg_load_player_unit(loading, plr, punit, buf)) {
      unit_virtual_destroy(punit);
      sg_failure_ret(FALSE, "Error loading unit %d of player %d.", i, plrno);
    }

    identity_number_reserve(punit->id);
    idex_register_unit(&wld, punit);

    if ((pcity = game_city_by_number(punit->homecity))) {
      unit_list_prepend(pcity->units_supported, punit);
    } else if (punit->homecity > IDENTITY_NUMBER_ZERO) {
      log_sg("%s: bad home city %d.", buf, punit->homecity);
      punit->homecity = IDENTITY_NUMBER_ZERO;
    }

    ptile = unit_tile(punit);

    /* allocate the unit's contribution to fog of war */
    punit->server.vision = vision_new(unit_owner(punit), ptile);
    unit_refresh_vision(punit);
    /* NOTE: There used to be some map_set_known calls here.  These were
     * unneeded since unfogging the tile when the unit sees it will
     * automatically reveal that tile. */

    unit_list_append(plr->units, punit);
    unit_list_prepend(unit_tile(punit)->units, punit);

    /* Claim ownership of fortress? */
    if ((extra_owner(ptile) == NULL
         || pplayers_at_war(extra_owner(ptile), plr))
        && tile_has_claimable_base(ptile, unit_type_get(punit))) {
      tile_claim_bases(ptile, plr);
    }
  }
}

/************************************************************************//**
  Load one unit.
****************************************************************************/
static bool sg_load_player_unit(struct loaddata *loading,
                                struct player *plr, struct unit *punit,
                                const char *unitstr)
{
  int j;
  enum unit_activity activity;
  int nat_x, nat_y;
  enum tile_special_type target;
  struct extra_type *pextra = NULL;
  struct base_type *pbase = NULL;
  struct road_type *proad = NULL;
  struct tile *ptile;
  int extra_id;
  int base_id;
  int road_id;
  int ei;
  const char *facing_str;
  enum tile_special_type cfspe;
  int natnbr;

  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->id, "%s.id",
                                     unitstr), FALSE, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_x, "%s.x", unitstr),
                  FALSE, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_y, "%s.y", unitstr),
                  FALSE, "%s", secfile_error());

  ptile = native_pos_to_tile(&(wld.map), nat_x, nat_y);
  sg_warn_ret_val(NULL != ptile, FALSE, "%s invalid tile (%d, %d)",
                  unitstr, nat_x, nat_y);
  unit_tile_set(punit, ptile);

  facing_str
    = secfile_lookup_str_default(loading->file, "x",
                                 "%s.facing", unitstr);
  if (facing_str[0] != 'x') {
    /* We don't touch punit->facing if savegame does not contain that
     * information. Initial orientation set by unit_virtual_create()
     * is as good as any. */
    enum direction8 facing = char2dir(facing_str[0]);

    if (direction8_is_valid(facing)) {
      punit->facing = facing;
    } else {
      log_error("Illegal unit orientation '%s'", facing_str);
    }
  }

  /* If savegame has unit nationality, it doesn't hurt to
   * internally set it even if nationality rules are disabled. */
  natnbr = secfile_lookup_int_default(loading->file,
                                      player_number(plr),
                                      "%s.nationality", unitstr);

  punit->nationality = player_by_number(natnbr);
  if (punit->nationality == NULL) {
    punit->nationality = plr;
  }

  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->homecity,
                                     "%s.homecity", unitstr), FALSE,
                  "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->moves_left,
                                     "%s.moves", unitstr), FALSE,
                  "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->fuel,
                                     "%s.fuel", unitstr), FALSE,
                  "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &ei,
                                     "%s.activity", unitstr), FALSE,
                  "%s", secfile_error());
  activity = unit_activity_by_name(loading->activities.order[ei],
                                   fc_strcasecmp);

  punit->server.birth_turn
    = secfile_lookup_int_default(loading->file, game.info.turn,
                                 "%s.born", unitstr);

  if (activity == ACTIVITY_PATROL_UNUSED) {
    /* Previously ACTIVITY_PATROL and ACTIVITY_GOTO were used for
     * client-side goto. Now client-side goto is handled by setting
     * a special flag, and units with orders generally have ACTIVITY_IDLE.
     * Old orders are lost. Old client-side goto units will still have
     * ACTIVITY_GOTO and will goto the correct position via server goto.
     * Old client-side patrol units lose their patrol routes and are put
     * into idle mode. */
    activity = ACTIVITY_IDLE;
  }

  extra_id = secfile_lookup_int_default(loading->file, -2,
                                        "%s.activity_tgt", unitstr);

  if (extra_id != -2) {
    if (extra_id >= 0 && extra_id < loading->extra.size) {
      pextra = loading->extra.order[extra_id];
      set_unit_activity_targeted(punit, activity, pextra);
    } else if (activity == ACTIVITY_IRRIGATE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                   EC_IRRIGATION,
                                                   unit_owner(punit),
                                                   punit);
      if (tgt != NULL) {
        set_unit_activity_targeted(punit, ACTIVITY_IRRIGATE, tgt);
      } else {
        set_unit_activity_targeted(punit, ACTIVITY_IRRIGATE, NULL);
      }
    } else if (activity == ACTIVITY_MINE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                   EC_MINE,
                                                   unit_owner(punit),
                                                   punit);
      if (tgt != NULL) {
        set_unit_activity_targeted(punit, ACTIVITY_MINE, tgt);
      } else {
        set_unit_activity_targeted(punit, ACTIVITY_MINE, NULL);
      }
    } else {
      set_unit_activity(punit, activity);
    }
  } else {
    /* extra_id == -2 -> activity_tgt not set */
    base_id = secfile_lookup_int_default(loading->file, -1,
                                      "%s.activity_base", unitstr);
    if (base_id >= 0 && base_id < loading->base.size) {
      pbase = loading->base.order[base_id];
    }
    road_id = secfile_lookup_int_default(loading->file, -1,
                                      "%s.activity_road", unitstr);
    if (road_id >= 0 && road_id < loading->road.size) {
      proad = loading->road.order[road_id];
    }

    {
      int tgt_no = secfile_lookup_int_default(loading->file,
                                              loading->special.size /* S_LAST */,
                                              "%s.activity_target", unitstr);
      if (tgt_no >= 0 && tgt_no < loading->special.size) {
        target = loading->special.order[tgt_no];
      } else {
        target = S_LAST;
      }
    }

    if (target == S_OLD_ROAD) {
      target = S_LAST;
      proad = road_by_compat_special(ROCO_ROAD);
    } else if (target == S_OLD_RAILROAD) {
      target = S_LAST;
      proad = road_by_compat_special(ROCO_RAILROAD);
    }

    if (activity == ACTIVITY_OLD_ROAD) {
      activity = ACTIVITY_GEN_ROAD;
      proad = road_by_compat_special(ROCO_ROAD);
    } else if (activity == ACTIVITY_OLD_RAILROAD) {
      activity = ACTIVITY_GEN_ROAD;
      proad = road_by_compat_special(ROCO_RAILROAD);
    }

    /* We need changed_from == ACTIVITY_IDLE by now so that
     * set_unit_activity() and friends don't spuriously restore activity
     * points -- unit should have been created this way */
    fc_assert(punit->changed_from == ACTIVITY_IDLE);

    if (activity == ACTIVITY_BASE) {
      if (pbase) {
        set_unit_activity_base(punit, base_number(pbase));
      } else {
        log_sg("Cannot find base %d for %s to build",
               base_id, unit_rule_name(punit));
        set_unit_activity(punit, ACTIVITY_IDLE);
      }
    } else if (activity == ACTIVITY_GEN_ROAD) {
      if (proad) {
        set_unit_activity_road(punit, road_number(proad));
      } else {
        log_sg("Cannot find road %d for %s to build",
               road_id, unit_rule_name(punit));
        set_unit_activity(punit, ACTIVITY_IDLE);
      }
    } else if (activity == ACTIVITY_PILLAGE) {
      struct extra_type *a_target;

      if (target != S_LAST) {
        a_target = special_extra_get(target);
      } else if (pbase != NULL) {
        a_target = base_extra_get(pbase);
      } else if (proad != NULL) {
        a_target = road_extra_get(proad);
      } else {
        a_target = NULL;
      }
      /* An out-of-range base number is seen with old savegames. We take
       * it as indicating undirected pillaging. We will assign pillage
       * targets before play starts. */
      set_unit_activity_targeted(punit, activity, a_target);
    } else if (activity == ACTIVITY_IRRIGATE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                   EC_IRRIGATION,
                                                   unit_owner(punit),
                                                   punit);
      if (tgt != NULL) {
        set_unit_activity_targeted(punit, ACTIVITY_IRRIGATE, tgt);
      } else {
        set_unit_activity_targeted(punit, ACTIVITY_IRRIGATE, NULL);
      }
    } else if (activity == ACTIVITY_MINE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                   EC_MINE,
                                                   unit_owner(punit),
                                                   punit);
      if (tgt != NULL) {
        set_unit_activity_targeted(punit, ACTIVITY_MINE, tgt);
      } else {
        set_unit_activity_targeted(punit, ACTIVITY_MINE, NULL);
      }
    } else if (activity == ACTIVITY_POLLUTION) {
      struct extra_type *tgt = prev_extra_in_tile(unit_tile(punit),
                                                  ERM_CLEANPOLLUTION,
                                                  unit_owner(punit),
                                                  punit);
      if (tgt != NULL) {
        set_unit_activity_targeted(punit, ACTIVITY_POLLUTION, tgt);
      } else {
        set_unit_activity_targeted(punit, ACTIVITY_POLLUTION, NULL);
      }
    } else if (activity == ACTIVITY_FALLOUT) {
      struct extra_type *tgt = prev_extra_in_tile(unit_tile(punit),
                                                  ERM_CLEANFALLOUT,
                                                  unit_owner(punit),
                                                  punit);
      if (tgt != NULL) {
        set_unit_activity_targeted(punit, ACTIVITY_FALLOUT, tgt);
      } else {
        set_unit_activity_targeted(punit, ACTIVITY_FALLOUT, NULL);
      }
    } else {
      set_unit_activity_targeted(punit, activity, NULL);
    }
  } /* activity_tgt == NULL */

  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->activity_count,
                                     "%s.activity_count", unitstr), FALSE,
                  "%s", secfile_error());

  punit->changed_from =
    secfile_lookup_int_default(loading->file, ACTIVITY_IDLE,
                               "%s.changed_from", unitstr);

  extra_id = secfile_lookup_int_default(loading->file, -2,
                                        "%s.changed_from_tgt", unitstr);

  if (extra_id != -2) {
    if (extra_id >= 0 && extra_id < loading->extra.size) {
      punit->changed_from_target = loading->extra.order[extra_id];
    } else {
      punit->changed_from_target = NULL;
    }
  } else {
    /* extra_id == -2 -> changed_from_tgt not set */

    cfspe =
      secfile_lookup_int_default(loading->file, S_LAST,
                                 "%s.changed_from_target", unitstr);
    base_id =
      secfile_lookup_int_default(loading->file, -1,
                                 "%s.changed_from_base", unitstr);
    road_id =
      secfile_lookup_int_default(loading->file, -1,
                                 "%s.changed_from_road", unitstr);

    if (road_id == -1) {
      if (cfspe == S_OLD_ROAD) {
        proad = road_by_compat_special(ROCO_ROAD);
        if (proad) {
          road_id = road_number(proad);
        }
      } else if (cfspe == S_OLD_RAILROAD) {
        proad = road_by_compat_special(ROCO_RAILROAD);
        if (proad) {
          road_id = road_number(proad);
        }
      }
    }

    if (base_id >= 0 && base_id < loading->base.size) {
      punit->changed_from_target = base_extra_get(loading->base.order[base_id]);
    } else if (road_id >= 0 && road_id < loading->road.size) {
      punit->changed_from_target = road_extra_get(loading->road.order[road_id]);
    } else if (cfspe != S_LAST) {
      punit->changed_from_target = special_extra_get(cfspe);
    } else {
      punit->changed_from_target = NULL;
    }

    if (punit->changed_from == ACTIVITY_IRRIGATE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                   EC_IRRIGATION,
                                                   unit_owner(punit),
                                                   punit);
      if (tgt != NULL) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = NULL;
      }
    } else if (punit->changed_from == ACTIVITY_MINE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                   EC_MINE,
                                                   unit_owner(punit),
                                                   punit);
      if (tgt != NULL) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = NULL;
      }
    } else if (punit->changed_from == ACTIVITY_POLLUTION) {
      struct extra_type *tgt = prev_extra_in_tile(unit_tile(punit),
                                                  ERM_CLEANPOLLUTION,
                                                  unit_owner(punit),
                                                  punit);
      if (tgt != NULL) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = NULL;
      }
    } else if (punit->changed_from == ACTIVITY_FALLOUT) {
      struct extra_type *tgt = prev_extra_in_tile(unit_tile(punit),
                                                  ERM_CLEANFALLOUT,
                                                  unit_owner(punit),
                                                  punit);
      if (tgt != NULL) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = NULL;
      }
    }
  }

  punit->changed_from_count =
    secfile_lookup_int_default(loading->file, 0,
                               "%s.changed_from_count", unitstr);

  /* Special case: for a long time, we accidentally incremented
   * activity_count while a unit was sentried, so it could increase
   * without bound (bug #20641) and be saved in old savefiles.
   * We zero it to prevent potential trouble overflowing the range
   * in network packets, etc. */
  if (activity == ACTIVITY_SENTRY) {
    punit->activity_count = 0;
  }
  if (punit->changed_from == ACTIVITY_SENTRY) {
    punit->changed_from_count = 0;
  }

  punit->veteran
    = secfile_lookup_int_default(loading->file, 0, "%s.veteran", unitstr);
  {
    /* Protect against change in veteran system in ruleset */
    const int levels = utype_veteran_levels(unit_type_get(punit));
    if (punit->veteran >= levels) {
      fc_assert(levels >= 1);
      punit->veteran = levels - 1;
    }
  }
  punit->done_moving
    = secfile_lookup_bool_default(loading->file, (punit->moves_left == 0),
                                  "%s.done_moving", unitstr);
  punit->battlegroup
    = secfile_lookup_int_default(loading->file, BATTLEGROUP_NONE,
                                 "%s.battlegroup", unitstr);

  if (secfile_lookup_bool_default(loading->file, FALSE,
                                  "%s.go", unitstr)) {
    int gnat_x, gnat_y;

    sg_warn_ret_val(secfile_lookup_int(loading->file, &gnat_x,
                                       "%s.goto_x", unitstr), FALSE,
                    "%s", secfile_error());
    sg_warn_ret_val(secfile_lookup_int(loading->file, &gnat_y,
                                       "%s.goto_y", unitstr), FALSE,
                    "%s", secfile_error());

    punit->goto_tile = native_pos_to_tile(&(wld.map), gnat_x, gnat_y);
  } else {
    punit->goto_tile = NULL;

    /* These variables are not used but needed for saving the unit table.
     * Load them to prevent unused variables errors. */
    (void) secfile_entry_lookup(loading->file, "%s.goto_x", unitstr);
    (void) secfile_entry_lookup(loading->file, "%s.goto_y", unitstr);
  }

  /* Load AI data of the unit. */
  CALL_FUNC_EACH_AI(unit_load, loading->file, punit, unitstr);

  sg_warn_ret_val(secfile_lookup_bool(loading->file,
                                      &punit->ai_controlled,
                                      "%s.ai", unitstr), FALSE,
                  "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->hp,
                                     "%s.hp", unitstr), FALSE,
                  "%s", secfile_error());

  punit->server.ord_map
    = secfile_lookup_int_default(loading->file, 0, "%s.ord_map", unitstr);
  punit->server.ord_city
    = secfile_lookup_int_default(loading->file, 0, "%s.ord_city", unitstr);
  punit->moved
    = secfile_lookup_bool_default(loading->file, FALSE, "%s.moved", unitstr);
  punit->paradropped
    = secfile_lookup_bool_default(loading->file, FALSE,
                                  "%s.paradropped", unitstr);

  /* The transport status (punit->transported_by) is loaded in
   * sg_player_units_transport(). */

  /* Initialize upkeep values: these are hopefully initialized
   * elsewhere before use (specifically, in city_support(); but
   * fixme: check whether always correctly initialized?).
   * Below is mainly for units which don't have homecity --
   * otherwise these don't get initialized (and AI calculations
   * etc may use junk values). */
  output_type_iterate(o) {
    punit->upkeep[o] = utype_upkeep_cost(unit_type_get(punit), plr, o);
  } output_type_iterate_end;

  punit->action_decision_want
      = secfile_lookup_enum_default(loading->file,
                                    ACT_DEC_NOTHING, action_decision,
                                    "%s.action_decision_want", unitstr);

  if (punit->action_decision_want != ACT_DEC_NOTHING) {
    /* Load the tile to act against. */
    int adwt_x, adwt_y;

    if (secfile_lookup_int(loading->file, &adwt_x,
                           "%s.action_decision_tile_x", unitstr)
        && secfile_lookup_int(loading->file, &adwt_y,
                              "%s.action_decision_tile_y", unitstr)) {
      punit->action_decision_tile = native_pos_to_tile(&(wld.map),
                                                       adwt_x, adwt_y);
    } else {
      punit->action_decision_want = ACT_DEC_NOTHING;
      punit->action_decision_tile = NULL;
      log_sg("Bad action_decision_tile for unit %d", punit->id);
    }
  } else {
    (void) secfile_entry_lookup(loading->file, "%s.action_decision_tile_x", unitstr);
    (void) secfile_entry_lookup(loading->file, "%s.action_decision_tile_y", unitstr);
    punit->action_decision_tile = NULL;
  }

  /* load the unit orders */
  {
    int len = secfile_lookup_int_default(loading->file, 0,
                                         "%s.orders_length", unitstr);
    if (len > 0) {
      const char *orders_unitstr, *dir_unitstr, *act_unitstr;
      const char *tgt_unitstr;
      const char *base_unitstr = NULL;
      const char *road_unitstr = NULL;
      int road_idx = road_number(road_by_compat_special(ROCO_ROAD));
      int rail_idx = road_number(road_by_compat_special(ROCO_RAILROAD));

      punit->orders.list = fc_malloc(len * sizeof(*(punit->orders.list)));
      punit->orders.length = len;
      punit->orders.index
        = secfile_lookup_int_default(loading->file, 0,
                                     "%s.orders_index", unitstr);
      punit->orders.repeat
        = secfile_lookup_bool_default(loading->file, FALSE,
                                      "%s.orders_repeat", unitstr);
      punit->orders.vigilant
        = secfile_lookup_bool_default(loading->file, FALSE,
                                      "%s.orders_vigilant", unitstr);

      orders_unitstr
        = secfile_lookup_str_default(loading->file, "",
                                     "%s.orders_list", unitstr);
      dir_unitstr
        = secfile_lookup_str_default(loading->file, "",
                                     "%s.dir_list", unitstr);
      act_unitstr
        = secfile_lookup_str_default(loading->file, "",
                                     "%s.activity_list", unitstr);
      tgt_unitstr
        = secfile_lookup_str_default(loading->file, NULL, "%s.tgt_list", unitstr);

      if (tgt_unitstr == NULL) {
        base_unitstr
          = secfile_lookup_str(loading->file, "%s.base_list", unitstr);
        road_unitstr
          = secfile_lookup_str_default(loading->file, NULL, "%s.road_list", unitstr);
      }

      punit->has_orders = TRUE;
      for (j = 0; j < len; j++) {
        struct unit_order *order = &punit->orders.list[j];

        if (orders_unitstr[j] == '\0' || dir_unitstr[j] == '\0'
            || act_unitstr[j] == '\0') {
          log_sg("Invalid unit orders.");
          free_unit_orders(punit);
          break;
        }
        order->order = char2order(orders_unitstr[j]);
        order->dir = char2dir(dir_unitstr[j]);
        order->activity = char2activity(act_unitstr[j]);
        order->sub_target = -1;

        if (order->order == ORDER_LAST
            || (order->order == ORDER_MOVE && !direction8_is_valid(order->dir))
            || (order->order == ORDER_ACTION_MOVE
                && !direction8_is_valid(order->dir))
            || (order->order == ORDER_ACTIVITY
                && order->activity == ACTIVITY_LAST)) {
          /* An invalid order. Just drop the orders for this unit. */
          free(punit->orders.list);
          punit->orders.list = NULL;
          punit->has_orders = FALSE;
          break;
        }

        /* The order may have been replaced by the perform action order */
        order->action = sg_order_to_action(order->order, punit,
                                           punit->goto_tile);
        if (order->action != ACTION_NONE) {
          /* The order was converted by order_to_action */
          order->order = ORDER_PERFORM_ACTION;
        }

        if (tgt_unitstr) {
          if (tgt_unitstr[j] != '?') {
            extra_id = char2num(tgt_unitstr[j]);

            if (extra_id < 0 || extra_id >= loading->extra.size) {
              log_sg("Cannot find extra %d for %s to build",
                     extra_id, unit_rule_name(punit));
              order->sub_target = EXTRA_NONE;
            } else {
              order->sub_target = extra_id;
            }
          } else {
            order->sub_target = EXTRA_NONE;
          }
        } else {
          /* In pre-2.6 savegames, base_list and road_list were only saved
           * for those activities (and not e.g. pillaging) */
          if (base_unitstr && base_unitstr[j] != '?'
              && order->activity == ACTIVITY_BASE) {
            base_id = char2num(base_unitstr[j]);

            if (base_id < 0 || base_id >= loading->base.size) {
              log_sg("Cannot find base %d for %s to build",
                     base_id, unit_rule_name(punit));
              base_id = base_number(get_base_by_gui_type(BASE_GUI_FORTRESS,
                                                         NULL, NULL));
            }

            order->sub_target
              = extra_number(base_extra_get(base_by_number(base_id)));
          } else if (road_unitstr && road_unitstr[j] != '?'
                     && order->activity == ACTIVITY_GEN_ROAD) {
            road_id = char2num(road_unitstr[j]);

            if (road_id < 0 || road_id >= loading->road.size) {
              log_sg("Cannot find road %d for %s to build",
                     road_id, unit_rule_name(punit));
              road_id = 0;
            }

            order->sub_target
              = extra_number(road_extra_get(road_by_number(road_id)));
          } else {
            order->sub_target = EXTRA_NONE;
          }

          if (order->activity == ACTIVITY_OLD_ROAD) {
            order->activity = ACTIVITY_GEN_ROAD;
            order->sub_target
              = extra_number(road_extra_get(road_by_number(road_idx)));
          } else if (order->activity == ACTIVITY_OLD_RAILROAD) {
            order->activity = ACTIVITY_GEN_ROAD;
            order->sub_target
              = extra_number(road_extra_get(road_by_number(rail_idx)));
          }
        }
      }
    } else {
      punit->has_orders = FALSE;
      punit->orders.list = NULL;

      (void) secfile_entry_lookup(loading->file, "%s.orders_index", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.orders_repeat", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.orders_vigilant", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.orders_list", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.dir_list", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.activity_list", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.tgt_list", unitstr);
    }
  }

  return TRUE;
}

/************************************************************************//**
  Load the transport status of all units. This is seperated from the other
  code as all units must be known.
****************************************************************************/
static void sg_load_player_units_transport(struct loaddata *loading,
                                           struct player *plr)
{
  int nunits, i, plrno = player_number(plr);

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Recheck the number of units for the player. This is a copied from
   * sg_load_player_units(). */
  sg_failure_ret(secfile_lookup_int(loading->file, &nunits,
                                    "player%d.nunits", plrno),
                 "%s", secfile_error());
  if (!plr->is_alive && nunits > 0) {
    log_sg("'player%d.nunits' = %d for dead player!", plrno, nunits);
    nunits = 0; /* Some old savegames may be buggy. */
  }

  for (i = 0; i < nunits; i++) {
    int id_unit, id_trans;
    struct unit *punit, *ptrans;

    id_unit = secfile_lookup_int_default(loading->file, -1,
                                         "player%d.u%d.id",
                                         plrno, i);
    punit = player_unit_by_number(plr, id_unit);
    fc_assert_action(punit != NULL, continue);

    id_trans = secfile_lookup_int_default(loading->file, -1,
                                          "player%d.u%d.transported_by",
                                          plrno, i);
    if (id_trans == -1) {
      /* Not transported. */
      continue;
    }

    ptrans = game_unit_by_number(id_trans);
    fc_assert_action(id_trans == -1 || ptrans != NULL, continue);

    if (ptrans) {
      bool load_success = unit_transport_load(punit, ptrans, TRUE);

      fc_assert_action(load_success == TRUE, continue);
    }
  }
}

/************************************************************************//**
  Load player (client) attributes data
****************************************************************************/
static void sg_load_player_attributes(struct loaddata *loading,
                                      struct player *plr)
{
  int plrno = player_number(plr);

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Toss any existing attribute_block (should not exist) */
  if (plr->attribute_block.data) {
    free(plr->attribute_block.data);
    plr->attribute_block.data = NULL;
  }

  /* This is a big heap of opaque data for the client, check everything! */
  plr->attribute_block.length = secfile_lookup_int_default(
      loading->file, 0, "player%d.attribute_v2_block_length", plrno);

  if (0 > plr->attribute_block.length) {
    log_sg("player%d.attribute_v2_block_length=%d too small", plrno,
           plr->attribute_block.length);
    plr->attribute_block.length = 0;
  } else if (MAX_ATTRIBUTE_BLOCK < plr->attribute_block.length) {
    log_sg("player%d.attribute_v2_block_length=%d too big (max %d)",
           plrno, plr->attribute_block.length, MAX_ATTRIBUTE_BLOCK);
    plr->attribute_block.length = 0;
  } else if (0 < plr->attribute_block.length) {
    int part_nr, parts;
    size_t actual_length;
    int quoted_length;
    char *quoted;

    sg_failure_ret(
        secfile_lookup_int(loading->file, &quoted_length,
                           "player%d.attribute_v2_block_length_quoted",
                           plrno), "%s", secfile_error());
    sg_failure_ret(
        secfile_lookup_int(loading->file, &parts,
                           "player%d.attribute_v2_block_parts", plrno),
        "%s", secfile_error());

    quoted = fc_malloc(quoted_length + 1);
    quoted[0] = '\0';
    plr->attribute_block.data = fc_malloc(plr->attribute_block.length);
    for (part_nr = 0; part_nr < parts; part_nr++) {
      const char *current =
          secfile_lookup_str(loading->file,
                             "player%d.attribute_v2_block_data.part%d",
                             plrno, part_nr);
      if (!current) {
        log_sg("attribute_v2_block_parts=%d actual=%d", parts, part_nr);
        break;
      }
      log_debug("attribute_v2_block_length_quoted=%lu have=%lu part=%lu",
                (unsigned long) quoted_length,
                (unsigned long) strlen(quoted),
                (unsigned long) strlen(current));
      fc_assert(strlen(quoted) + strlen(current) <= quoted_length);
      strcat(quoted, current);
    }
    fc_assert_msg(quoted_length == strlen(quoted),
                  "attribute_v2_block_length_quoted=%lu actual=%lu",
                  (unsigned long) quoted_length,
                  (unsigned long) strlen(quoted));

    actual_length =
        unquote_block(quoted,
                      plr->attribute_block.data,
                      plr->attribute_block.length);
    fc_assert(actual_length == plr->attribute_block.length);
    free(quoted);
  }
}

/************************************************************************//**
  Load vision data
****************************************************************************/
static void sg_load_player_vision(struct loaddata *loading,
                                  struct player *plr)
{
  int plrno = player_number(plr);
  int total_ncities =
      secfile_lookup_int_default(loading->file, -1,
                                 "player%d.dc_total", plrno);
  int i;
  bool someone_alive = FALSE;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  if (game.server.revealmap & REVEAL_MAP_DEAD) {
    player_list_iterate(team_members(plr->team), pteam_member) {
      if (pteam_member->is_alive) {
        someone_alive = TRUE;
        break;
      }
    } player_list_iterate_end;

    if (!someone_alive) {
      /* Reveal all for completely dead teams. */
      map_know_and_see_all(plr);
    }
  }

  if (!plr->is_alive
      || -1 == total_ncities
      || FALSE == game.info.fogofwar
      || !secfile_lookup_bool_default(loading->file, TRUE,
                                      "game.save_private_map")) {
    /* We have:
     * - a dead player;
     * - fogged cities are not saved for any reason;
     * - a savegame with fog of war turned off;
     * - or game.save_private_map is not set to FALSE in the scenario /
     * savegame. The players private knowledge is set to be what he could
     * see without fog of war. */
    whole_map_iterate(&(wld.map), ptile) {
      if (map_is_known(ptile, plr)) {
        struct city *pcity = tile_city(ptile);

        update_player_tile_last_seen(plr, ptile);
        update_player_tile_knowledge(plr, ptile);

        if (NULL != pcity) {
          update_dumb_city(plr, pcity);
        }
      }
    } whole_map_iterate_end;

    /* Nothing more to do; */
    return;
  }

  /* Load player map (terrain). */
  LOAD_MAP_CHAR(ch, ptile,
                map_get_player_tile(ptile, plr)->terrain
                  = char2terrain(ch), loading->file,
                "player%d.map_t%04d", plrno);

  /* Load player map (resources). */
  LOAD_MAP_CHAR(ch, ptile,
                map_get_player_tile(ptile, plr)->resource
                  = char2resource(ch), loading->file,
                "player%d.map_res%04d", plrno);

  if (loading->version >= 30) {
    /* 2.6.0 or newer */

    /* Load player map (extras). */
    halfbyte_iterate_extras(j, loading->extra.size) {
      LOAD_MAP_CHAR(ch, ptile,
                    sg_extras_set(&map_get_player_tile(ptile, plr)->extras,
                                  ch, loading->extra.order + 4 * j),
                    loading->file, "player%d.map_e%02d_%04d", plrno, j);
    } halfbyte_iterate_extras_end;
  } else {
    /* Load player map (specials). */
    halfbyte_iterate_special(j, loading->special.size) {
      LOAD_MAP_CHAR(ch, ptile,
                    sg_special_set(ptile, &map_get_player_tile(ptile, plr)->extras,
                                   ch, loading->special.order + 4 * j, FALSE),
                    loading->file, "player%d.map_spe%02d_%04d", plrno, j);
    } halfbyte_iterate_special_end;

    /* Load player map (bases). */
    halfbyte_iterate_bases(j, loading->base.size) {
      LOAD_MAP_CHAR(ch, ptile,
                    sg_bases_set(&map_get_player_tile(ptile, plr)->extras,
                                 ch, loading->base.order + 4 * j),
                    loading->file, "player%d.map_b%02d_%04d", plrno, j);
    } halfbyte_iterate_bases_end;

    /* Load player map (roads). */
    if (loading->version >= 20) {
      /* 2.5.0 or newer */
      halfbyte_iterate_roads(j, loading->road.size) {
        LOAD_MAP_CHAR(ch, ptile,
                      sg_roads_set(&map_get_player_tile(ptile, plr)->extras,
                                   ch, loading->road.order + 4 * j),
                      loading->file, "player%d.map_r%02d_%04d", plrno, j);
      } halfbyte_iterate_roads_end;
    }
  }

  if (game.server.foggedborders) {
    /* Load player map (border). */
    int x, y;

    for (y = 0; y < wld.map.ysize; y++) {
      const char *buffer
        = secfile_lookup_str(loading->file, "player%d.map_owner%04d",
                             plrno, y);
      const char *buffer2
        = secfile_lookup_str(loading->file, "player%d.extras_owner%04d",
                             plrno, y);
      const char *ptr = buffer;
      const char *ptr2 = buffer2;

      sg_failure_ret(NULL != buffer,
                    "Savegame corrupt - map line %d not found.", y);
      for (x = 0; x < wld.map.xsize; x++) {
        char token[TOKEN_SIZE];
        char token2[TOKEN_SIZE];
        int number;
        struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

        scanin(&ptr, ",", token, sizeof(token));
        sg_failure_ret('\0' != token[0],
                       "Savegame corrupt - map size not correct.");
        if (strcmp(token, "-") == 0) {
          map_get_player_tile(ptile, plr)->owner = NULL;
        } else  {
          sg_failure_ret(str_to_int(token, &number),
                         "Savegame corrupt - got tile owner=%s in (%d, %d).",
                         token, x, y);
          map_get_player_tile(ptile, plr)->owner = player_by_number(number);
        }

        if (loading->version >= 30) {
          scanin(&ptr2, ",", token2, sizeof(token2));
          sg_failure_ret('\0' != token2[0],
                         "Savegame corrupt - map size not correct.");
          if (strcmp(token2, "-") == 0) {
            map_get_player_tile(ptile, plr)->extras_owner = NULL;
          } else  {
            sg_failure_ret(str_to_int(token2, &number),
                           "Savegame corrupt - got extras owner=%s in (%d, %d).",
                           token, x, y);
            map_get_player_tile(ptile, plr)->extras_owner = player_by_number(number);
          }
        } else {
          map_get_player_tile(ptile, plr)->extras_owner
            = map_get_player_tile(ptile, plr)->owner;
        }
      }
    }
  }

  /* Load player map (update time). */
  for (i = 0; i < 4; i++) {
    /* put 4-bit segments of 16-bit "updated" field */
    if (i == 0) {
      LOAD_MAP_CHAR(ch, ptile,
                    map_get_player_tile(ptile, plr)->last_updated
                      = ascii_hex2bin(ch, i),
                    loading->file, "player%d.map_u%02d_%04d", plrno, i);
    } else {
      LOAD_MAP_CHAR(ch, ptile,
                    map_get_player_tile(ptile, plr)->last_updated
                      |= ascii_hex2bin(ch, i),
                    loading->file, "player%d.map_u%02d_%04d", plrno, i);
    }
  }

  /* Load player map known cities. */
  for (i = 0; i < total_ncities; i++) {
    struct vision_site *pdcity;
    char buf[32];
    fc_snprintf(buf, sizeof(buf), "player%d.dc%d", plrno, i);

    pdcity = vision_site_new(0, NULL, NULL);
    if (sg_load_player_vision_city(loading, plr, pdcity, buf)) {
      change_playertile_site(map_get_player_tile(pdcity->location, plr),
                             pdcity);
      identity_number_reserve(pdcity->identity);
    } else {
      /* Error loading the data. */
      log_sg("Skipping seen city %d for player %d.", i, plrno);
      if (pdcity != NULL) {
        vision_site_destroy(pdcity);
      }
    }
  }

  /* Repair inconsistent player maps. */
  whole_map_iterate(&(wld.map), ptile) {
    if (map_is_known_and_seen(ptile, plr, V_MAIN)) {
      struct city *pcity = tile_city(ptile);

      update_player_tile_knowledge(plr, ptile);
      reality_check_city(plr, ptile);

      if (NULL != pcity) {
        update_dumb_city(plr, pcity);
      }
    }
  } whole_map_iterate_end;
}

/************************************************************************//**
  Load data for one seen city.
****************************************************************************/
static bool sg_load_player_vision_city(struct loaddata *loading,
                                       struct player *plr,
                                       struct vision_site *pdcity,
                                       const char *citystr)
{
  const char *string;
  int i, id, size;
  citizens city_size;
  int nat_x, nat_y;
  const char *stylename;

  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_x, "%s.x",
                                     citystr),
                  FALSE, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_y, "%s.y",
                                     citystr),
                  FALSE, "%s", secfile_error());
  pdcity->location = native_pos_to_tile(&(wld.map), nat_x, nat_y);
  sg_warn_ret_val(NULL != pdcity->location, FALSE,
                  "%s invalid tile (%d,%d)", citystr, nat_x, nat_y);

  sg_warn_ret_val(secfile_lookup_int(loading->file, &id, "%s.owner",
                                     citystr),
                  FALSE, "%s", secfile_error());
  pdcity->owner = player_by_number(id);
  sg_warn_ret_val(NULL != pdcity->owner, FALSE,
                  "%s has invalid owner (%d); skipping.", citystr, id);

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pdcity->identity,
                                     "%s.id", citystr),
                  FALSE, "%s", secfile_error());
  sg_warn_ret_val(IDENTITY_NUMBER_ZERO < pdcity->identity, FALSE,
                  "%s has invalid id (%d); skipping.", citystr, id);

  sg_warn_ret_val(secfile_lookup_int(loading->file, &size,
                                     "%s.size", citystr),
                  FALSE, "%s", secfile_error());
  city_size = (citizens)size; /* set the correct type */
  sg_warn_ret_val(size == (int)city_size, FALSE,
                  "Invalid city size: %d; set to %d.", size, city_size);
  vision_site_size_set(pdcity, city_size);

  /* Initialise list of improvements */
  BV_CLR_ALL(pdcity->improvements);
  string = secfile_lookup_str(loading->file, "%s.improvements", citystr);
  sg_warn_ret_val(string != NULL, FALSE, "%s", secfile_error());
  sg_warn_ret_val(strlen(string) == loading->improvement.size, FALSE,
                  "Invalid length of '%s.improvements' (%lu ~= %lu).",
                  citystr, (unsigned long) strlen(string),
                  (unsigned long) loading->improvement.size);
  for (i = 0; i < loading->improvement.size; i++) {
    sg_warn_ret_val(string[i] == '1' || string[i] == '0', FALSE,
                    "Undefined value '%c' within '%s.improvements'.",
                    string[i], citystr)

    if (string[i] == '1') {
      struct impr_type *pimprove =
          improvement_by_rule_name(loading->improvement.order[i]);
      if (pimprove) {
        BV_SET(pdcity->improvements, improvement_index(pimprove));
      }
    }
  }

  /* Use the section as backup name. */
  sz_strlcpy(pdcity->name, secfile_lookup_str_default(loading->file, citystr,
                                                      "%s.name", citystr));

  pdcity->occupied = secfile_lookup_bool_default(loading->file, FALSE,
                                                 "%s.occupied", citystr);
  pdcity->walls = secfile_lookup_bool_default(loading->file, FALSE,
                                              "%s.walls", citystr);
  pdcity->happy = secfile_lookup_bool_default(loading->file, FALSE,
                                              "%s.happy", citystr);
  pdcity->unhappy = secfile_lookup_bool_default(loading->file, FALSE,
                                                "%s.unhappy", citystr);
  stylename = secfile_lookup_str_default(loading->file, NULL,
                                             "%s.style", citystr);
  if (stylename != NULL) {
    pdcity->style = city_style_by_rule_name(stylename);
  } else {
    pdcity->style = 0;
  }
  if (pdcity->style < 0) {
    pdcity->style = 0;
  }

  pdcity->city_image = secfile_lookup_int_default(loading->file, -100,
                                                  "%s.city_image", citystr);

  return TRUE;
}

/* =======================================================================
 * Load the researches.
 * ======================================================================= */

/************************************************************************//**
  Load '[research]'.
****************************************************************************/
static void sg_load_researches(struct loaddata *loading)
{
  struct research *presearch;
  int count;
  int number;
  const char *string;
  int i, j;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Initialize all researches. */
  researches_iterate(pinitres) {
    init_tech(pinitres, FALSE);
  } researches_iterate_end;

  /* May be unsaved (e.g. scenario case). */
  count = secfile_lookup_int_default(loading->file, 0, "research.count");
  for (i = 0; i < count; i++) {
    sg_failure_ret(secfile_lookup_int(loading->file, &number,
                                      "research.r%d.number", i),
                   "%s", secfile_error());
    presearch = research_by_number(number);
    sg_failure_ret(presearch != NULL,
                   "Invalid research number %d in 'research.r%d.number'",
                   number, i);

    presearch->tech_goal = technology_load(loading->file,
                                           "research.r%d.goal", i);
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &presearch->techs_researched,
                                      "research.r%d.techs", i),
                 "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &presearch->future_tech,
                                      "research.r%d.futuretech", i),
                   "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &presearch->bulbs_researched,
                                      "research.r%d.bulbs", i),
                   "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &presearch->bulbs_researching_saved,
                                      "research.r%d.bulbs_before", i),
                   "%s", secfile_error());
    presearch->researching_saved = technology_load(loading->file,
                                                   "research.r%d.saved", i);
    presearch->researching = technology_load(loading->file,
                                             "research.r%d.now", i);
    sg_failure_ret(secfile_lookup_bool(loading->file,
                                       &presearch->got_tech,
                                       "research.r%d.got_tech", i),
                   "%s", secfile_error());

    string = secfile_lookup_str(loading->file, "research.r%d.done",
                                i);
    sg_failure_ret(string != NULL, "%s", secfile_error());
    sg_failure_ret(strlen(string) == loading->technology.size,
                   "Invalid length of 'research.r%d.done' (%lu ~= %lu).",
                   i, (unsigned long) strlen(string),
                   (unsigned long) loading->technology.size);
    for (j = 0; j < loading->technology.size; j++) {
      sg_failure_ret(string[j] == '1' || string[j] == '0',
                     "Undefined value '%c' within 'research.r%d.done'.",
                     string[j], i);

      if (string[j] == '1') {
        struct advance *padvance =
            advance_by_rule_name(loading->technology.order[j]);

        if (padvance) {
          research_invention_set(presearch, advance_number(padvance),
                                 TECH_KNOWN);
        }
      }
    }
  }

  /* In case of tech_leakage, we can update research only after all the
   * researches have been loaded */
  researches_iterate(pupres) {
    research_update(pupres);
  } researches_iterate_end;
}

/* =======================================================================
 * Load the event cache. Should be the last thing to do.
 * ======================================================================= */

/************************************************************************//**
  Load '[event_cache]'.
****************************************************************************/
static void sg_load_event_cache(struct loaddata *loading)
{
  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  event_cache_load(loading->file, "event_cache");
}

/* =======================================================================
 * Load  the open treaties
 * ======================================================================= */

/************************************************************************//**
  Load '[treaty_xxx]'.
****************************************************************************/
static void sg_load_treaties(struct loaddata *loading)
{
  int tidx;
  const char *plr0;
  struct treaty_list *treaties = get_all_treaties();

  for (tidx = 0; (plr0 = secfile_lookup_str_default(loading->file, NULL,
                                                    "treaty%d.plr0", tidx)) != NULL ;
       tidx++) {
    const char *plr1;
    const char *ct;
    int cidx;
    struct player *p0, *p1;

    plr1 = secfile_lookup_str(loading->file, "treaty%d.plr1", tidx);

    p0 = player_by_name(plr0);
    p1 = player_by_name(plr1);

    if (p0 == NULL || p1 == NULL) {
      log_error("Treaty between unknown players %s and %s", plr0, plr1);
    } else {
      struct Treaty *ptreaty = fc_malloc(sizeof(*ptreaty));

      init_treaty(ptreaty, p0, p1);
      treaty_list_prepend(treaties, ptreaty);

      for (cidx = 0; (ct = secfile_lookup_str_default(loading->file, NULL,
                                                      "treaty%d.clause%d.type",
                                                      tidx, cidx)) != NULL ;
           cidx++ ) {
        enum clause_type type = clause_type_by_name(ct, fc_strcasecmp);
        const char *plrx;

        if (!clause_type_is_valid(type)) {
          log_error("Invalid clause type \"%s\"", ct);
        } else {
          struct player *pgiver = NULL;

          plrx = secfile_lookup_str(loading->file, "treaty%d.clause%d.from",
                                    tidx, cidx);

          if (!fc_strcasecmp(plrx, plr0)) {
            pgiver = p0;
          } else if (!fc_strcasecmp(plrx, plr1)) {
            pgiver = p1;
          } else {
            log_error("Clause giver %s is not participant of the treaty"
                      "between %s and %s", plrx, plr0, plr1);
          }

          if (pgiver != NULL) {
            int value;

            value = secfile_lookup_int_default(loading->file, 0,
                                               "treaty%d.clause%d.value",
                                               tidx, cidx);

            add_clause(ptreaty, pgiver, type, value);
          }
        }

        /* These must be after clauses have been added so that acceptance
         * does not get cleared by what seems like changes to the treaty. */
        ptreaty->accept0 = secfile_lookup_bool_default(loading->file, FALSE,
                                                       "treaty%d.accept0", tidx);
        ptreaty->accept1 = secfile_lookup_bool_default(loading->file, FALSE,
                                                       "treaty%d.accept1", tidx);
      }
    }
  }
}

/* =======================================================================
 * Load the history report
 * ======================================================================= */

/************************************************************************//**
  Load '[history]'.
****************************************************************************/
static void sg_load_history(struct loaddata *loading)
{
  struct history_report *hist = history_report_get();
  int turn;

  turn = secfile_lookup_int_default(loading->file, -2, "history.turn");

  if (turn + 1 >= game.info.turn) {
    const char *str;

    hist->turn = turn;
    str = secfile_lookup_str(loading->file, "history.title");
    sg_failure_ret(str != NULL, "%s", secfile_error());
    sz_strlcpy(hist->title, str);
    str = secfile_lookup_str(loading->file, "history.body");
    sg_failure_ret(str != NULL, "%s", secfile_error());
    sz_strlcpy(hist->body, str);
  }
}

/* =======================================================================
 * Load the mapimg definitions.
 * ======================================================================= */

/************************************************************************//**
  Load '[mapimg]'.
****************************************************************************/
static void sg_load_mapimg(struct loaddata *loading)
{
  int mapdef_count, i;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  /* Clear all defined map images. */
  while (mapimg_count() > 0) {
    mapimg_delete(0);
  }

  mapdef_count = secfile_lookup_int_default(loading->file, 0,
                                            "mapimg.count");
  log_verbose("Saved map image definitions: %d.", mapdef_count);

  if (0 >= mapdef_count) {
    return;
  }

  for (i = 0; i < mapdef_count; i++) {
    const char *p;

    p = secfile_lookup_str(loading->file, "mapimg.mapdef%d", i);
    if (NULL == p) {
      log_verbose("[Mapimg %4d] Missing definition.", i);
      continue;
    }

    if (!mapimg_define(p, FALSE)) {
      log_error("Invalid map image definition %4d: %s.", i, p);
    }

    log_verbose("Mapimg %4d loaded.", i);
  }
}

/* =======================================================================
 * Sanity checks for loading a game.
 * ======================================================================= */

/************************************************************************//**
  Sanity check for loaded game.
****************************************************************************/
static void sg_load_sanitycheck(struct loaddata *loading)
{
  int players;

  /* Check status and return if not OK (sg_success != TRUE). */
  sg_check_ret();

  if (game.info.is_new_game) {
    /* Nothing to do for new games (or not started scenarios). */
    return;
  }

  /* Old savegames may have maxplayers lower than current player count,
   * fix. */
  players = normal_player_count();
  if (game.server.max_players < players) {
    log_verbose("Max players lower than current players, fixing");
    game.server.max_players = players;
  }

  /* Fix ferrying sanity */
  players_iterate(pplayer) {
    unit_list_iterate_safe(pplayer->units, punit) {
      if (!unit_transport_get(punit)
          && !can_unit_exist_at_tile(&(wld.map), punit, unit_tile(punit))) {
        log_sg("Removing %s unferried %s in %s at (%d, %d)",
               nation_rule_name(nation_of_player(pplayer)),
               unit_rule_name(punit),
               terrain_rule_name(unit_tile(punit)->terrain),
               TILE_XY(unit_tile(punit)));
        bounce_unit(punit, TRUE);
      }
    } unit_list_iterate_safe_end;
  } players_iterate_end;

  /* Fix stacking issues.  We don't rely on the savegame preserving
   * alliance invariants (old savegames often did not) so if there are any
   * unallied units on the same tile we just bounce them. */
  players_iterate(pplayer) {
    players_iterate(aplayer) {
      resolve_unit_stacks(pplayer, aplayer, TRUE);
    } players_iterate_end;

    /* Backward compatibility: if we had any open-ended orders (pillage)
     * in the savegame, assign specific targets now */
    unit_list_iterate(pplayer->units, punit) {
      unit_assign_specific_activity_target(punit,
                                           &punit->activity,
                                           &punit->activity_target);
    } unit_list_iterate_end;
  } players_iterate_end;

  /* Recalculate the potential buildings for each city. Has caused some
   * problems with game random state.
   * This also changes the game state if you save the game directly after
   * loading it and compare the results. */
  players_iterate(pplayer) {
    bool saved_ai_control = is_ai(pplayer);

    /* Recalculate for all players. */
    set_as_human(pplayer);

    /* Building advisor needs data phase open in order to work */
    adv_data_phase_init(pplayer, FALSE);
    building_advisor(pplayer);
    /* Close data phase again so it can be opened again when game starts. */
    adv_data_phase_done(pplayer);

    if (saved_ai_control) {
      set_as_ai(pplayer);
    }
  } players_iterate_end;

  /* Check worked tiles map */
#ifdef FREECIV_DEBUG
  if (loading->worked_tiles != NULL) {
    /* check the entire map for unused worked tiles */
    whole_map_iterate(&(wld.map), ptile) {
      if (loading->worked_tiles[ptile->index] != -1) {
        log_error("[city id: %d] Unused worked tile at (%d, %d).",
                  loading->worked_tiles[ptile->index], TILE_XY(ptile));
      }
    } whole_map_iterate_end;
  }
#endif /* FREECIV_DEBUG */

  /* Check researching technologies and goals. */
  researches_iterate(presearch) {
    if (presearch->researching != A_UNSET
        && !is_future_tech(presearch->researching)
        && (valid_advance_by_number(presearch->researching) == NULL
            || (research_invention_state(presearch, presearch->researching)
                != TECH_PREREQS_KNOWN))) {
      log_sg(_("%s had invalid researching technology."),
             research_name_translation(presearch));
      presearch->researching = A_UNSET;
    }
    if (presearch->tech_goal != A_UNSET
        && !is_future_tech(presearch->tech_goal)
        && (valid_advance_by_number(presearch->tech_goal) == NULL
            || !research_invention_reachable(presearch, presearch->tech_goal)
            || (research_invention_state(presearch, presearch->tech_goal)
                == TECH_KNOWN))) {
      log_sg(_("%s had invalid technology goal."),
             research_name_translation(presearch));
      presearch->tech_goal = A_UNSET;
    }
  } researches_iterate_end;

  if (0 == strlen(server.game_identifier)
      || !is_base64url(server.game_identifier)) {
    /* This uses fc_rand(), so random state has to be initialized before. */
    randomize_base64url_string(server.game_identifier,
                               sizeof(server.game_identifier));
  }

  /* Check if some player has more than one of some UTYF_UNIQUE unit type */
  players_iterate(pplayer) {
    int unique_count[U_LAST];

    memset(unique_count, 0, sizeof(unique_count));

    unit_list_iterate(pplayer->units, punit) {
      unique_count[utype_index(unit_type_get(punit))]++;
    } unit_list_iterate_end;

    unit_type_iterate(ut) {
      if (unique_count[utype_index(ut)] > 1 && utype_has_flag(ut, UTYF_UNIQUE)) {
        log_sg(_("%s has multiple units of type %s though it should be possible "
                 "to have only one."),
               player_name(pplayer), utype_name_translation(ut));
      }
    } unit_type_iterate_end;
  } players_iterate_end;

  /* Restore game random state, just in case various initialization code
   * inexplicably altered the previously existing state. */
  if (!game.info.is_new_game) {
    fc_rand_set_state(loading->rstate);

    if (loading->version < 30) {
      /* For older savegames we have to recalculate the score with current data,
       * instead of using beginning-of-turn saved scores. */
      players_iterate(pplayer) {
        calc_civ_score(pplayer);
      } players_iterate_end;
    }
  }

  /* At the end do the default sanity checks. */
  sanity_check();
}
