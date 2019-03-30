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

#include <stdlib.h>

/* utility */
#include "fc_cmdline.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"

/* common */
#include "version.h"

/* modinst */
#include "download.h"
#include "mpcmdline.h"
#include "mpdb.h"

#include "modinst.h"

struct fcmp_params fcmp = {
  .list_url = MODPACK_LIST_URL,
  .inst_prefix = NULL,
  .autoinstall = NULL
};

/**********************************************************************//**
  Progress indications from downloader
**************************************************************************/
static void msg_callback(const char *msg)
{
  log_normal("%s", msg);
}

/**********************************************************************//**
  Build main modpack list view
**************************************************************************/
static void setup_modpack_list(const char *name, const char *URL,
                               const char *version, const char *license,
                               enum modpack_type type, const char *subtype,
                               const char *notes)
{
  const char *type_str;
  const char *lic_str;
  const char *inst_str;

  if (modpack_type_is_valid(type)) {
    type_str = _(modpack_type_name(type));
  } else {
    /* TRANS: Unknown modpack type */
    type_str = _("?");
  }

  if (license != NULL) {
    lic_str = license;
  } else {
    /* TRANS: License of modpack is not known */
    lic_str = Q_("?license:Unknown");
  }

  inst_str = mpdb_installed_version(name, type);
  if (inst_str == NULL) {
    inst_str = _("Not installed");
  }

  log_normal("%s", "");
  log_normal(_("Name=\"%s\""), name);
  log_normal(_("Version=\"%s\""), version);
  log_normal(_("Installed=\"%s\""), inst_str);
  log_normal(_("Type=\"%s\" / \"%s\""), type_str, subtype);
  log_normal(_("License=\"%s\""), lic_str);
  log_normal(_("URL=\"%s\""), URL);
  if (notes != NULL) {
    log_normal(_("Comment=\"%s\""), notes);
  }
}

/**********************************************************************//**
  Entry point of the freeciv-modpack program
**************************************************************************/
int main(int argc, char *argv[])
{
  int ui_options;

  fcmp_init();

  /* This modifies argv! */
  ui_options = fcmp_parse_cmdline(argc, argv);

  if (ui_options != -1) {
    int i;

    for (i = 1; i <= ui_options; i++) {
      if (is_option("--help", argv[i])) {
        fc_fprintf(stderr,
                   _("This modpack installer does not support any specific options\n\n"));

        /* TRANS: No full stop after the URL, could cause confusion. */
        fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);

        ui_options = -1;
      } else {
        log_error(_("Unknown option '--' '%s'"), argv[i]);
        ui_options = -1;
      }
    }
  }

  if (ui_options != -1) {
    const char *rev_ver;

    load_install_info_lists(&fcmp);

    log_normal(_("Freeciv modpack installer (command line version)"));

    log_normal("%s%s", word_version(), VERSION_STRING);

    rev_ver = fc_git_revision();
    if (rev_ver != NULL) {
      log_normal(_("commit: %s"), rev_ver);
    }

    log_normal("%s", "");

    if (fcmp.autoinstall == NULL) {
      download_modpack_list(&fcmp, setup_modpack_list, msg_callback);
    } else {
      const char *errmsg;

      errmsg = download_modpack(fcmp.autoinstall, &fcmp, msg_callback, NULL);

      if (errmsg == NULL) {
        log_normal(_("Modpack installed successfully"));
      } else {
        log_error(_("Modpack install failed: %s"), errmsg);
      }
    }

    close_mpdbs();
  }

  fcmp_deinit();
  cmdline_option_values_free();

  return EXIT_SUCCESS;
}
