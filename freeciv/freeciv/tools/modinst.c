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

#include "fc_prehdrs.h"

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "net_types.h"
#include "rand.h"
#include "registry.h"
#include "support.h"

/* common */
#include "fc_interface.h"

/* modinst */
#include "mpdb.h"

#include "modinst.h"

static char main_ii_filename[500];
static char scenario_ii_filename[500];

/**********************************************************************//**
  Load all required install info lists.
**************************************************************************/
void load_install_info_lists(struct fcmp_params *fcmp)
{
  char main_db_filename[500];
  char scenario_db_filename[500];
  struct stat buf;

  fc_snprintf(main_db_filename, sizeof(main_db_filename),
              "%s" DIR_SEPARATOR DATASUBDIR DIR_SEPARATOR FCMP_CONTROLD DIR_SEPARATOR "mp.db",
              fcmp->inst_prefix);
  fc_snprintf(scenario_db_filename, sizeof(scenario_db_filename),
              "%s" DIR_SEPARATOR "scenarios" DIR_SEPARATOR FCMP_CONTROLD DIR_SEPARATOR "mp.db",
              fcmp->inst_prefix);

  fc_snprintf(main_ii_filename, sizeof(main_ii_filename),
              "%s" DIR_SEPARATOR DATASUBDIR DIR_SEPARATOR FCMP_CONTROLD DIR_SEPARATOR "modpacks.db",
              fcmp->inst_prefix);
  fc_snprintf(scenario_ii_filename, sizeof(scenario_ii_filename),
              "%s" DIR_SEPARATOR "scenarios" DIR_SEPARATOR FCMP_CONTROLD DIR_SEPARATOR "modpacks.db",
              fcmp->inst_prefix);

  if (fc_stat(main_db_filename, &buf)) {
    create_mpdb(main_db_filename, FALSE);
    load_install_info_list(main_ii_filename);
  } else {
    open_mpdb(main_db_filename, FALSE);
  }

  if (fc_stat(scenario_db_filename, &buf)) {
    create_mpdb(scenario_db_filename, TRUE);
    load_install_info_list(scenario_ii_filename);
  } else {
    open_mpdb(scenario_db_filename, TRUE);
  }
}

/**********************************************************************//**
  Initialize modpack installer
**************************************************************************/
void fcmp_init(void)
{
  init_nls();
  init_character_encodings(FC_DEFAULT_DATA_ENCODING, FALSE);
  registry_module_init();

  fc_init_network();

  fc_srand(time(NULL)); /* Needed at least for Windows version of netfile_get_section_file() */
}

/**********************************************************************//**
  Deinitialize modpack installer
**************************************************************************/
void fcmp_deinit(void)
{
  registry_module_close();
  fc_shutdown_network();
  /* log_init() was not done by fcmp_init(); we assume the caller called
   * fcmp_parse_cmdline() (which sets up logging) in between */
  log_close();
  free_libfreeciv();
  free_nls();
}
