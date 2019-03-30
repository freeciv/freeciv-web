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

/* ANSI */
#include <stdlib.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

/* utility */
#include "fc_cmdline.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "registry.h"

/* common */
#include "fc_cmdhelp.h"
#include "fc_interface.h"
#include "version.h"

/* server */
#include "sernet.h"
#include "settings.h"

/* tools/shared */
#include "tools_fc_interface.h"

/* ruledit */
#include "comments.h"
#include "ruledit_qt.h"

#include "ruledit.h"

static int re_parse_cmdline(int argc, char *argv[]);

struct ruledit_arguments reargs;

/**********************************************************************//**
  Main entry point for freeciv-ruledit
**************************************************************************/
int main(int argc, char **argv)
{
  enum log_level loglevel = LOG_NORMAL;
  int ui_options;

  /* Load win32 post-crash debugger */
#ifdef FREECIV_MSWINDOWS
# ifndef FREECIV_NDEBUG
  if (LoadLibrary("exchndl.dll") == NULL) {
#  ifdef FREECIV_DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#  endif /* FREECIV_DEBUG */
  }
# endif /* FREECIV_NDEBUG */
#endif /* FREECIV_MSWINDOWS */

  init_nls();

#ifdef ENABLE_NLS
  (void) bindtextdomain("freeciv-ruledit", get_locale_dir());
#endif

  init_character_encodings(FC_DEFAULT_DATA_ENCODING, FALSE);
#ifdef ENABLE_NLS
  bind_textdomain_codeset("freeciv-ruledit", get_internal_encoding());
#endif

  registry_module_init();

  log_init(NULL, loglevel, NULL, NULL, -1);

  /* Initialize command line arguments. */
  reargs.ruleset = NULL;

  ui_options = re_parse_cmdline(argc, argv);

  if (ui_options != -1) {
    init_connections();

    settings_init(FALSE);

    /* Reset aifill to zero */
    game.info.aifill = 0;

    game_init(FALSE);
    i_am_tool();

    /* Initialize the fc_interface functions needed to understand rules. */
    fc_interface_init_tool();

    if (comments_load()) {
      ruledit_qt_run(ui_options, argv);

      comments_free();
    } else {
      log_error(R__("Failed to load comments.txt"));
    }
  }

  registry_module_close();
  log_close();
  free_libfreeciv();
  free_nls();

  /* Clean up command line arguments. */
  cmdline_option_values_free();

  return EXIT_SUCCESS;
}

/**********************************************************************//**
  Parse freeciv-ruledit commandline.
**************************************************************************/
static int re_parse_cmdline(int argc, char *argv[])
{
  int i = 1;
  bool ui_separator = FALSE;
  int ui_options = 0;

  while (i < argc) {
    char *option;

    if (ui_separator) {
      argv[1 + ui_options] = argv[i];
      ui_options++;
    } else if (is_option("--help", argv[i])) {
      struct cmdhelp *help = cmdhelp_new(argv[0]);

      cmdhelp_add(help, "h", "help",
                  R__("Print a summary of the options"));
      cmdhelp_add(help, "v", "version",
                  R__("Print the version number"));
      cmdhelp_add(help, "r",
                  /* TRANS: argument (don't translate) VALUE (translate) */
                  R__("ruleset RULESET"),
                  R__("Ruleset to use as the starting point."));
      /* The function below prints a header and footer for the options.
       * Furthermore, the options are sorted. */
      cmdhelp_display(help, TRUE, TRUE, TRUE);
      cmdhelp_destroy(help);

      exit(EXIT_SUCCESS);
    } else if (is_option("--version", argv[i])) {
      fc_fprintf(stderr, "%s \n", freeciv_name_version());

      exit(EXIT_SUCCESS);
    } else if ((option = get_option_malloc("--ruleset", argv, &i, argc, true))) {
      if (reargs.ruleset) {
        fc_fprintf(stderr, R__("Can only edit one ruleset at a time.\n"));
      } else {
        reargs.ruleset = option;
      }
    } else if (is_option("--", argv[i])) {
      ui_separator = TRUE;
    } else {
      fc_fprintf(stderr, R__("Unrecognized option: \"%s\"\n"), argv[i]);
      exit(EXIT_FAILURE);
    }

    i++;
  }

  return ui_options;
}

/**********************************************************************//**
  Show widget if experimental features enabled, hide otherwise
**************************************************************************/
void show_experimental(QWidget *wdg)
{
#ifdef RULEDIT_EXPERIMENTAL
  wdg->show();
#else
  wdg->hide();
#endif
}
