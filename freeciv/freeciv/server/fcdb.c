/****************************************************************************
 Freeciv - Copyright (C) 2005  - M.C. Kaufman
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "md5.h"
#include "shared.h"
#include "support.h"

/* common */
#include "connection.h"
#include "packets.h"

/* server */
#include "connecthand.h"
#include "notify.h"
#include "sernet.h"
#include "srv_main.h"

/* server/scripting */
#ifdef HAVE_FCDB
#include "script_fcdb.h"
#endif /* HAVE_FCDB */

#include "fcdb.h"

/* If HAVE_FCDB is set, the freeciv database module is compiled. Else only
 * some dummy functions are defiend. */
#ifdef HAVE_FCDB

enum fcdb_option_source {
  AOS_DEFAULT,  /* Internal default, currently not used */
  AOS_FILE,     /* Read from config file */
  AOS_SET       /* Set, currently not used */
};

struct fcdb_option {
  enum fcdb_option_source source;
  char *value;
};

#define SPECHASH_TAG fcdb_option
#define SPECHASH_ASTR_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct fcdb_option *
#include "spechash.h"
#define fcdb_option_hash_data_iterate(phash, data)                          \
  TYPED_HASH_DATA_ITERATE(struct fcdb_option *, phash, data)
#define fcdb_option_hash_data_iterate_end HASH_DATA_ITERATE_END
#define fcdb_option_hash_keys_iterate(phash, key)                           \
  TYPED_HASH_KEYS_ITERATE(char *, phash, key)
#define fcdb_option_hash_keys_iterate_end HASH_KEYS_ITERATE_END
#define fcdb_option_hash_iterate(phash, key, data)                          \
  TYPED_HASH_ITERATE(char *, struct fcdb_option *, phash, key, data)
#define fcdb_option_hash_iterate_end HASH_ITERATE_END

struct fcdb_option_hash *fcdb_config = NULL;

static bool fcdb_set_option(const char *key, const char *value,
                            enum fcdb_option_source source);
static bool fcdb_load_config(const char *filename);


/************************************************************************//**
  Set one fcdb option (or delete it if value == NULL).
  Replaces any previous setting.
****************************************************************************/
static bool fcdb_set_option(const char *key, const char *value,
                            enum fcdb_option_source source)
{
  struct fcdb_option *oldopt = NULL;
  bool removed;

  if (value != NULL) {
    struct fcdb_option *newopt = fc_malloc(sizeof(*newopt));

    newopt->value = fc_strdup(value);
    newopt->source = source;
    removed = fcdb_option_hash_replace_full(fcdb_config, key, newopt,
                                            NULL, &oldopt);
  } else {
    removed = fcdb_option_hash_remove_full(fcdb_config, key, NULL, &oldopt);
  }

  if (removed) {
    /* Overwritten/removed an existing value */
    fc_assert_ret_val(oldopt != NULL, FALSE);
    FC_FREE(oldopt->value);
    FC_FREE(oldopt);
  }

  return TRUE;
}

/************************************************************************//**
  Load fcdb configuration from file.
  We deliberately don't search datadirs for filename, as we don't want this
  overridden by modpacks etc.
****************************************************************************/
static bool fcdb_load_config(const char *filename)
{
  struct section_file *secfile;

  fc_assert_ret_val(NULL != filename, FALSE);

  if (!(secfile = secfile_load(filename, FALSE))) {
    log_error(_("Cannot load fcdb config file '%s':\n%s"), filename,
              secfile_error());
    return FALSE;
  }

  entry_list_iterate(section_entries(secfile_section_by_name(secfile,
                                                             "fcdb")),
                     pentry) {
    if (entry_type(pentry) == ENTRY_STR) {
      const char *value;
#ifndef FREECIV_NDEBUG
      bool entry_str_get_success =
#endif
        entry_str_get(pentry, &value);

      fc_assert(entry_str_get_success);
      fcdb_set_option(entry_name(pentry), value, AOS_FILE);
    } else {
      log_error("Value for '%s' in '%s' is not of string type, ignoring",
                entry_name(pentry), filename);
    }
  } entry_list_iterate_end;

  /* FIXME: we could arrange to call secfile_check_unused() and have it
   * complain about unused entries (e.g. those not in [fcdb]). */
  secfile_destroy(secfile);

  return TRUE;
}

/************************************************************************//**
  Initialize freeciv database system
****************************************************************************/
bool fcdb_init(const char *conf_file)
{
  fc_assert(fcdb_config == NULL);
  fcdb_config = fcdb_option_hash_new();

  if (conf_file && strcmp(conf_file, "-")) {
    if (!fcdb_load_config(conf_file)) {
      return FALSE;
    }
  } else {
    log_debug("No fcdb config file.");
  }

  return script_fcdb_init(NULL);
}

/************************************************************************//**
  Return the selected fcdb config value.
****************************************************************************/
const char *fcdb_option_get(const char *type)
{
  struct fcdb_option *opt;

  if (fcdb_option_hash_lookup(fcdb_config, type, &opt)) {
    return opt->value;
  } else {
    return NULL;
  }
}

/************************************************************************//**
  Free resources allocated by fcdb system.
****************************************************************************/
void fcdb_free(void)
{
  script_fcdb_free();

  fcdb_option_hash_data_iterate(fcdb_config, popt) {
    FC_FREE(popt->value);
    FC_FREE(popt);
  } fcdb_option_hash_data_iterate_end;

  fcdb_option_hash_destroy(fcdb_config);
  fcdb_config = NULL;
}

#else  /* HAVE_FCDB */

/************************************************************************//**
  Dummy function - Initialize freeciv database system
****************************************************************************/
bool fcdb_init(const char *conf_file)
{
  return TRUE;
}

/************************************************************************//**
  Dummy function - Return the selected fcdb config value.
****************************************************************************/
const char *fcdb_option_get(const char *type)
{
  return NULL;
}

/************************************************************************//**
  Dummy function - Free resources allocated by fcdb system.
****************************************************************************/
void fcdb_free(void)
{
  return;
}
#endif /* HAVE_FCDB */
