/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "version.h"

#include "fc_cmdhelp.h"

struct cmdarg {
  char shortarg;
  char *longarg;
  char *helpstr;
};

/* 'struct cmdarg_list' and related functions. */
#define SPECLIST_TAG cmdarg
#define SPECLIST_TYPE struct cmdarg
#include "speclist.h"
#define cmdarg_list_iterate(cmdarg_list, pcmdarg)                            \
  TYPED_LIST_ITERATE(struct cmdarg, cmdarg_list, pcmdarg)
#define cmdarg_list_iterate_end LIST_ITERATE_END

struct cmdhelp {
  char *cmdname;
  struct cmdarg_list *cmdarglist;
};

static struct cmdarg *cmdarg_new(const char *shortarg, const char *longarg,
                                 const char *helpstr);
static void cmdarg_destroy(struct cmdarg *pcmdarg);
static int cmdarg_compare(const struct cmdarg *const *pcmdarg0,
                          const struct cmdarg *const *pcmdarg1);

/*************************************************************************//**
  Create a new command help struct.
*****************************************************************************/
struct cmdhelp *cmdhelp_new(const char *cmdname)
{
  struct cmdhelp *pcmdhelp = fc_calloc(1, sizeof(*pcmdhelp));

  pcmdhelp->cmdname = fc_strdup(fc_basename(cmdname));
  pcmdhelp->cmdarglist = cmdarg_list_new();

  return pcmdhelp;
}

/*************************************************************************//**
  Destroy a command help struct.
*****************************************************************************/
void cmdhelp_destroy(struct cmdhelp *pcmdhelp)
{
  if (pcmdhelp) {
    if (pcmdhelp->cmdname) {
      free(pcmdhelp->cmdname);
    }
    cmdarg_list_iterate(pcmdhelp->cmdarglist, pcmdarg) {
      cmdarg_destroy(pcmdarg);
    } cmdarg_list_iterate_end;
  }
  free(pcmdhelp);
}

/*************************************************************************//**
  Add a command help moption.
*****************************************************************************/
void cmdhelp_add(struct cmdhelp *pcmdhelp, const char *shortarg,
                 const char *longarg, const char *helpstr, ...)
{
  va_list args;
  char buf[512];
  struct cmdarg *pcmdarg;

  va_start(args, helpstr);
  fc_vsnprintf(buf, sizeof(buf), helpstr, args);
  va_end(args);

  pcmdarg = cmdarg_new(shortarg, longarg, buf);
  cmdarg_list_append(pcmdhelp->cmdarglist, pcmdarg);
}

/*************************************************************************//**
  Display the help for the command.
*****************************************************************************/
void cmdhelp_display(struct cmdhelp *pcmdhelp, bool sort, bool gui_options,
                     bool report_bugs)
{
  fc_fprintf(stderr, _("Usage: %s [option ...]\nValid option are:\n"),
             pcmdhelp->cmdname);

  cmdarg_list_sort(pcmdhelp->cmdarglist, cmdarg_compare);
  cmdarg_list_iterate(pcmdhelp->cmdarglist, pcmdarg) {
    if (pcmdarg->shortarg != '\0') {
      fc_fprintf(stderr, "  -%c, --%-15s %s\n", pcmdarg->shortarg,
                 pcmdarg->longarg, pcmdarg->helpstr);
    } else {
      fc_fprintf(stderr, "      --%-15s %s\n", pcmdarg->longarg,
                 pcmdarg->helpstr);
    }
  } cmdarg_list_iterate_end;

  if (gui_options) {
    char buf[128];

    fc_snprintf(buf, sizeof(buf), _("Try \"%s -- --help\" for more."),
                pcmdhelp->cmdname);

    /* The nearly empty strings in the two functions below have to be adapted
     * if the format of the command argument list above is changed.*/
    fc_fprintf(stderr, "      --                %s\n",
               _("Pass any following options to the UI."));
    fc_fprintf(stderr, "                        %s\n", buf);
  }

  if (report_bugs) {
    /* TRANS: No full stop after the URL, could cause confusion. */
    fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
  }
}

/*************************************************************************//**
  Create a new command argument struct.
*****************************************************************************/
static struct cmdarg *cmdarg_new(const char *shortarg, const char *longarg,
                                 const char *helpstr)
{
  struct cmdarg *pcmdarg = fc_calloc(1, sizeof(*pcmdarg));

  if (shortarg && strlen(shortarg) == 1) {
    pcmdarg->shortarg = shortarg[0];
  } else {
    /* '\0' means no short argument for this option. */
    pcmdarg->shortarg = '\0';
  }
  pcmdarg->longarg = fc_strdup(longarg);
  pcmdarg->helpstr = fc_strdup(helpstr);

  return pcmdarg;
}

/*************************************************************************//**
  Destroy a command argument struct.
*****************************************************************************/
static void cmdarg_destroy(struct cmdarg *pcmdarg)
{
  if (pcmdarg) {
    if (pcmdarg->longarg) {
      free(pcmdarg->longarg);
    }
    if (pcmdarg->helpstr) {
      free(pcmdarg->helpstr);
    }
  }
  free(pcmdarg);
}

/*************************************************************************//**
  Compare two command argument definitions.
*****************************************************************************/
static int cmdarg_compare(const struct cmdarg *const *pp0,
                          const struct cmdarg *const *pp1)
{
  const struct cmdarg *pcmdarg0 = *pp0;
  const struct cmdarg *pcmdarg1 = *pp1;
  int c0, c1;

  if (pcmdarg0 == NULL) {
    return -1;
  }
  if (pcmdarg1 == NULL) {
    return 1;
  }

  /* Arguments without a short option are listed at the end sorted by the
   * long option. */
  if (pcmdarg0->shortarg == '\0') {
    if (pcmdarg1->shortarg == '\0') {
      return fc_strcasecmp(pcmdarg0->longarg, pcmdarg1->longarg);
    } else {
      return 1;
    }
  }
  if (pcmdarg1->shortarg == '\0') {
    return -1;
  }

  /* All other are sorted alphabetically by the shortarg in the following
   * order: AaBbCcDd... */
  c0 = (int) (unsigned char) fc_tolower(pcmdarg0->shortarg);
  c1 = (int) (unsigned char) fc_tolower(pcmdarg1->shortarg);
  if (c0 == c1) {
    return (int) (unsigned char)pcmdarg0->shortarg
           - (int) (unsigned char)pcmdarg1->shortarg;
  } else {
    return c0 - c1;
  }
}
