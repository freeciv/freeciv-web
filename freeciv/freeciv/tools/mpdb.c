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

#include <sqlite3.h>

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "mem.h"
#include "registry.h"

/* modinst */
#include "download.h"

#include "mpdb.h"

#define MPDB_CAPSTR "+mpdb"

#define MPDB_FORMAT_VERSION "1"

static sqlite3 *main_handle = NULL;
static sqlite3 *scenario_handle = NULL;

static int mpdb_query(sqlite3 *handle, const char *query);

/**********************************************************************//**
  Construct install info list from file.
**************************************************************************/
void load_install_info_list(const char *filename)
{
  struct section_file *file;
  const char *caps;

  file = secfile_load(filename, FALSE);

  if (file == NULL) {
    /* This happens in first run - or actually all runs until something is
     * installed. Previous run has not saved database. */
    log_debug("No install info file");

    return;
  }

  caps = secfile_lookup_str(file, "info.options");

  if (caps == NULL) {
    log_error("MPDB %s missing capability information", filename);
    secfile_destroy(file);
    return;
  }

  if (!has_capabilities(MPDB_CAPSTR, caps)) {
    log_error("Incompatible mpdb file %s:", filename);
    log_error("  file options:      %s", caps);
    log_error("  supported options: %s", MPDB_CAPSTR);

    secfile_destroy(file);

    return;
  }

  if (file != NULL) {
    bool all_read = FALSE;
    int i;

    for (i = 0 ; !all_read ; i++) {
      const char *str;
      char buf[80];

      fc_snprintf(buf, sizeof(buf), "modpacks.mp%d", i);

      str = secfile_lookup_str_default(file, NULL, "%s.name", buf);

      if (str != NULL) {
        const char *type;
        const char *ver;

        type = secfile_lookup_str(file, "%s.type", buf);
        ver = secfile_lookup_str(file, "%s.version", buf);

        mpdb_update_modpack(str, modpack_type_by_name(type, fc_strcasecmp),
                            ver);
      } else {
        all_read = TRUE;
      }
    }

    secfile_destroy(file);
  }
}

/**********************************************************************//**
  SQL query to database
**************************************************************************/
static int mpdb_query(sqlite3 *handle, const char *query)
{
  int ret;
  sqlite3_stmt *stmt;

  ret = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);

  if (ret == SQLITE_OK) {
    ret = sqlite3_step(stmt);
  }

  if (ret == SQLITE_DONE) {
    ret = sqlite3_finalize(stmt);
  }

  if (ret != SQLITE_OK) {
    log_error("Query \"%s\" failed. (%d)", query, ret);
  }

  return ret;
}

/**********************************************************************//**
  Create modpack database
**************************************************************************/
void create_mpdb(const char *filename, bool scenario_db)
{
  sqlite3 **handle;
  int ret;
  int llen = strlen(filename) + 1;
  char *local_name = fc_malloc(llen);
  int i;

  strncpy(local_name, filename, llen);
  for (i = llen - 1 ; local_name[i] != DIR_SEPARATOR_CHAR ; i--) {
    /* Nothing */
  }
  local_name[i] = '\0';
  if (!make_dir(local_name)) {
    log_error(_("Can't create directory \"%s\" for modpack database."), local_name);
    return;
  }

  if (scenario_db) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  ret = sqlite3_open_v2(filename, handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        NULL);

  if (ret == SQLITE_OK) {
    ret = mpdb_query(*handle,
                     "create table meta (version INTEGER default " MPDB_FORMAT_VERSION ");");
  }

  if (ret == SQLITE_OK) {
    ret = mpdb_query(*handle,
                     "create table modpacks (name VARCHAR(60) NOT NULL, type VARCHAR(32), version VARCHAR(32) NOT NULL);");
  }

  if (ret == SQLITE_OK) {
    log_debug("Created %s", filename);
  } else {
    log_error(_("Creating \"%s\" failed: %s"), filename, sqlite3_errstr(ret));
  }
}

/**********************************************************************//**
  Open existing database
**************************************************************************/
void open_mpdb(const char *filename, bool scenario_db)
{
  sqlite3 **handle;
  int ret;

  if (scenario_db) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  ret = sqlite3_open_v2(filename, handle, SQLITE_OPEN_READWRITE, NULL);

  if (ret != SQLITE_OK) {
    log_error(_("Opening \"%s\" failed: %s"), filename, sqlite3_errstr(ret));
  }
}

/**********************************************************************//**
  Close open databases
**************************************************************************/
void close_mpdbs(void)
{
  sqlite3_close(main_handle);
  main_handle = NULL;
  sqlite3_close(scenario_handle);
  scenario_handle = NULL;
}

/**********************************************************************//**
  Update modpack information in database
**************************************************************************/
bool mpdb_update_modpack(const char *name, enum modpack_type type,
                         const char *version)
{
  sqlite3 **handle;
  int ret;
  char qbuf[2048];

  if (type == MPT_SCENARIO) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  sqlite3_snprintf(sizeof(qbuf), qbuf, "select * from modpacks where name is '%q';",
                   name);
  ret = mpdb_query(*handle, qbuf);

  if (ret == SQLITE_ROW) {
    sqlite3_snprintf(sizeof(qbuf), qbuf,
                     "update modpacks set type = '%q', version = '%q' where name is '%q';",
                     modpack_type_name(type), version, name);
    ret = mpdb_query(*handle, qbuf);
  } else {
    /* Completely new modpack */
    sqlite3_snprintf(sizeof(qbuf), qbuf,
                     "insert into modpacks values ('%q', '%q', '%q');",
                     name, modpack_type_name(type), version);
    ret = mpdb_query(*handle, qbuf);
  }

  if (ret != SQLITE_OK) {
    log_error(_("Failed to insert modpack '%s' information"), name);
  }

  return ret != SQLITE_OK;
}

/**********************************************************************//**
  Return version of modpack.
**************************************************************************/
const char *mpdb_installed_version(const char *name,
                                   enum modpack_type type)
{
  sqlite3 **handle;
  int ret;
  char qbuf[2048];
  sqlite3_stmt *stmt;

  if (type == MPT_SCENARIO) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  sqlite3_snprintf(sizeof(qbuf), qbuf,
                   "select * from modpacks where name is '%q';",
                   name);
  ret = sqlite3_prepare_v2(*handle, qbuf, -1, &stmt, NULL);

  if (ret == SQLITE_OK) {
    ret = sqlite3_step(stmt);
  }

  if (ret == SQLITE_DONE) {
    ret = sqlite3_finalize(stmt);
  }

  if (ret == SQLITE_ROW) {
    return (const char *)sqlite3_column_text(stmt, 2);
  }

  return NULL;
}
