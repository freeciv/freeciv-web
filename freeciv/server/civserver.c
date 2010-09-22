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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef GENERATING_MAC  /* mac header(s) */
#include <Controls.h>
#include <Dialogs.h>
#endif

#ifdef WIN32_NATIVE
#include <windows.h>
#endif

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

/* common */
#include "capstr.h"
#include "game.h"
#include "version.h"

/* server */
#include "console.h"
#include "ggzserver.h"
#include "meta.h"
#include "sernet.h"
#include "srv_main.h"

#include "civserver.h"

#ifdef GENERATING_MAC
static void Mac_options(int argc);  /* don't need argv */
#endif

#ifdef HAVE_SIGNAL_H
#  define USE_INTERRUPT_HANDLERS
#endif

#ifdef USE_INTERRUPT_HANDLERS
/**************************************************************************
  This function is called when a SIGINT (ctrl-c) is received.  It will exit
  only if two SIGINTs are received within a second.

  TODO: SIGHUP and SIGTERM should be handled too.  At a minimum we should
  save the game before exiting.
**************************************************************************/
static void sigint_handler(int sig)
{
  static struct timer *timer = NULL;

  if (with_ggz) {
    exit(EXIT_SUCCESS);
  }
  if (timer && read_timer_seconds(timer) <= 1.0) {
    exit(EXIT_SUCCESS);
  } else {
    if (game.info.timeout == -1) {
      freelog(LOG_NORMAL, _("Setting timeout to 0. Autogame will stop.\n"));
      game.info.timeout = 0;
    }
    if (!timer) {
      freelog(LOG_NORMAL, _("You must interrupt Freeciv twice"
                            " within one second to make it exit.\n"));
    }
  }
  timer = renew_timer_start(timer, TIMER_USER, TIMER_ACTIVE);
}
#endif

/**************************************************************************
 Entry point for Freeciv server.  Basically, does two things:
  1. Parses command-line arguments (possibly dialog, on mac).
  2. Calls the main server-loop routine.
**************************************************************************/
int main(int argc, char *argv[])
{
  int inx;
  bool showhelp = FALSE;
  bool showvers = FALSE;
  char *option = NULL;

  /* Load win32 post-crash debugger */
#ifdef WIN32_NATIVE
# ifndef NDEBUG
  if (LoadLibrary("exchndl.dll") == NULL) {
#  ifdef DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#  endif
  }
# endif
#endif

#ifdef USE_INTERRUPT_HANDLERS
  signal(SIGINT, sigint_handler);
#endif

  /* initialize server */
  srv_init();

  /* parse command-line arguments... */

#ifdef GENERATING_MAC
  Mac_options(argc);
#endif
  srvarg.announce = ANNOUNCE_DEFAULT;

  /* no  we don't use GNU's getopt or even the "standard" getopt */
  /* yes we do have reasons ;)                                   */
  inx = 1;
  while (inx < argc) {
    if ((option = get_option_malloc("--file", argv, &inx, argc))) {
      sz_strlcpy(srvarg.load_filename, option);
      free(option);
    } else if (is_option("--help", argv[inx])) {
      showhelp = TRUE;
      break;
    } else if ((option = get_option_malloc("--log", argv, &inx, argc))) {
      srvarg.log_filename = option; /* Never freed. */
    } else if ((option = get_option_malloc("--Ranklog", argv, &inx, argc))) {
      srvarg.ranklog_filename = option; /* Never freed. */
    } else if (is_option("--nometa", argv[inx])) {
      fc_fprintf(stderr, _("Warning: the %s option is obsolete.  "
			   "Use -m to enable the metaserver.\n"), argv[inx]);
      showhelp = TRUE;
    } else if (is_option("--meta", argv[inx])) {
      srvarg.metaserver_no_send = FALSE;
    } else if ((option = get_option_malloc("--Metaserver",
					 argv, &inx, argc))) {
      sz_strlcpy(srvarg.metaserver_addr, option);
      free(option);
      srvarg.metaserver_no_send = FALSE;      /* --Metaserver implies --meta */
    } else if ((option = get_option_malloc("--identity",
					   argv, &inx, argc))) {
      sz_strlcpy(srvarg.metaserver_name, option);
      free(option);
    } else if ((option = get_option_malloc("--port", argv, &inx, argc))) {
      if (sscanf(option, "%d", &srvarg.port) != 1) {
	showhelp = TRUE;
	break;
      }
      free(option);
    } else if ((option = get_option_malloc("--bind", argv, &inx, argc))) {
      srvarg.bind_addr = option; /* Never freed. */
    } else if ((option = get_option_malloc("--read", argv, &inx, argc)))
      srvarg.script_filename = option; /* Never freed. */
    else if ((option = get_option_malloc("--quitidle", argv, &inx, argc))) {
      if (sscanf(option, "%d", &srvarg.quitidle) != 1) {
	showhelp = TRUE;
	break;
      }
      free(option);
    } else if (is_option("--exit-on-end", argv[inx])) {
      srvarg.exit_on_end = TRUE;
    } else if ((option = get_option_malloc("--debug", argv, &inx, argc))) {
      srvarg.loglevel = log_parse_level_str(option);
      if (srvarg.loglevel == -1) {
	srvarg.loglevel = LOG_NORMAL;
	showhelp = TRUE;
	break;
      }
      free(option);
#ifdef HAVE_AUTH
    } else if ((option = get_option_malloc("--auth", argv, &inx, argc))) {
      srvarg.auth_enabled = TRUE;
      srvarg.auth_conf = option;
    } else if (is_option("--Guests", argv[inx])) {
      srvarg.auth_allow_guests = TRUE;
    } else if (is_option("--Newusers", argv[inx])) {
      srvarg.auth_allow_newusers = TRUE;
#endif
    } else if (is_option("--Ppm", argv[inx])) {
      srvarg.save_ppm = TRUE;
    } else if ((option = get_option_malloc("--Serverid", argv, &inx, argc))) {
      sz_strlcpy(srvarg.serverid, option);
      free(option);
    } else if ((option = get_option_malloc("--saves", argv, &inx, argc))) {
      srvarg.saves_pathname = option; /* Never freed. */
    } else if ((option = get_option_malloc("--scenarios", argv, &inx, argc))) {
      srvarg.scenarios_pathname = option; /* Never freed */
    } else if (is_option("--version", argv[inx])) {
      showvers = TRUE;
    } else if ((option = get_option_malloc("--Announce", argv, &inx, argc))) {
      if (!strcasecmp(option, "ipv4")) {
        srvarg.announce = ANNOUNCE_IPV4;
      } else if(!strcasecmp(option, "none")) {
        srvarg.announce= ANNOUNCE_NONE;
#ifdef IPV6_SUPPORT
      } else if (!strcasecmp(option, "ipv6")) {
        srvarg.announce = ANNOUNCE_IPV6;
#endif /* IPv6 support */
      } else {
        freelog(LOG_ERROR, _("Illegal value \"%s\" for --Announce"), option);
      }
      free(option);
    } else {
      fc_fprintf(stderr, _("Error: unknown option '%s'\n"), argv[inx]);
      showhelp = TRUE;
      break;
    }
    inx++;
  }

  if (showvers && !showhelp) {
    fc_fprintf(stderr, "%s \n", freeciv_name_version());
    exit(EXIT_SUCCESS);
  }
  con_write(C_VERSION, _("This is the server for %s"), freeciv_name_version());
  /* TRANS: No full stop after the URL, could cause confusion. */
  con_write(C_COMMENT, _("You can learn a lot about Freeciv at %s"),
	    WIKI_URL);

  if (showhelp) {
    fc_fprintf(stderr,
	       _("Usage: %s [option ...]\nValid options are:\n"), argv[0]);
    fc_fprintf(stderr, _("  -A  --Announce PROTO\tAnnounce game in LAN using protocol PROTO (IPv4/IPv6/none)\n"));
#ifdef HAVE_AUTH
    fc_fprintf(stderr, _("  -a  --auth FILE\tEnable server authentication "
                         "with configuration from FILE.\n"));
    fc_fprintf(stderr, _("  -G  --Guests\t\tAllow guests to "
			 "login if auth is enabled.\n"));
    fc_fprintf(stderr, _("  -N  --Newusers\tAllow new users to "
			 "login if auth is enabled.\n"));
#endif
    fc_fprintf(stderr, _("  -b  --bind ADDR\tListen for clients on ADDR\n"));
#ifdef DEBUG
    fc_fprintf(stderr, _("  -d, --debug NUM\tSet debug log level (0 to 4,"
		      " or 4:file1,min,max:...)\n"));
#else
    fc_fprintf(stderr,
	       _("  -d, --debug NUM\tSet debug log level (0 to 3)\n"));
#endif
    fc_fprintf(stderr, _("  -f, --file FILE\tLoad saved game FILE\n"));
    fc_fprintf(stderr,
	       _("  -h, --help\t\tPrint a summary of the options\n"));
   fc_fprintf(stderr, _("  -i, --identity ADDR\tBe known as ADDR at metaserver\n"));
    fc_fprintf(stderr, _("  -l, --log FILE\tUse FILE as logfile\n"));
    fc_fprintf(stderr, _("  -m, --meta\t\tNotify metaserver and "
			 "send server's info\n"));
    fc_fprintf(stderr, _("  -M, --Metaserver ADDR\tSet ADDR "
			 "as metaserver address\n"));

    fc_fprintf(stderr, _("  -p, --port PORT\tListen for clients on "
			 "port PORT\n"));
    fc_fprintf(stderr, _("  -q, --quitidle TIME\tQuit if no players "
			 "for TIME seconds\n"));
    fc_fprintf(stderr, _("  -e, --exit-on-end\t"
		      "When a game ends, exit instead of restarting\n"));
    fc_fprintf(stderr,
	       _("  -s, --saves DIR\tSave games to directory DIR\n"));
    fc_fprintf(stderr,
	       _("  -S, --Serverid ID\tSets the server id to ID\n"));
    fc_fprintf(stderr,
	     _("  -P, --Ppm\t\tSave ppms of the map when saving the game.\n"));
    fc_fprintf(stderr, _("  -r, --read FILE\tRead startup script FILE\n"));
    fc_fprintf(stderr,
	       _("  -R, --Ranklog FILE\tUse FILE as ranking logfile\n"));
    fc_fprintf(stderr, _("  -v, --version\t\tPrint the version number\n"));
    /* TRANS: No full stop after the URL, could cause confusion. */
    fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
    exit(EXIT_SUCCESS);
  }

  /* disallow running as root -- too dangerous */
  dont_run_as_root(argv[0], "freeciv_server");

  ggz_initialize();
  init_our_capability();

  /* have arguments, call the main server loop... */
  srv_main();

  /* Technically, we won't ever get here. We exit via server_quit. */

  /* done */
  exit(EXIT_SUCCESS);
}

#ifdef GENERATING_MAC
/*************************************************************************
  generate an option dialog if no options have been passed in
*************************************************************************/
static void Mac_options(int argc)
{
#define HARDCODED_OPT
  /*temporary hack since GetNewDialog() doesn't want to work*/
#ifdef HARDCODED_OPT
  srvarg.log_filename="log.out";
  srvarg.loglevel=LOG_DEBUG;
#else  /* HARDCODED_OPT */
  if (argc == 0)
  {
    OSErr err;
    DialogPtr  optptr;
    Ptr storage;
    Handle ditl;
    Handle dlog;
    short the_type;
    Handle the_handle;
    Rect the_rect;
    short the_item, old_item=16;
    int done = false;
    Str255 the_string;
    /*load/init the stuff for the dialog*/
    storage =NewPtr(sizeof(DialogRecord));
    if (storage == 0)
    {
      exit(EXIT_FAILURE);
    }
    ditl=Get1Resource('DITL',200);
    if ((ditl == 0)||(ResError()))
    {
      exit(EXIT_FAILURE);
    }
    dlog=Get1Resource('DLOG',200);
    if ((dlog == 0)||(ResError()))
    {
      exit(EXIT_FAILURE);
    }
    /*make the dialog*/
    optptr=GetNewDialog(200, storage, (WindowPtr)-1L);
    /*setup the dialog*/
    err=SetDialogDefaultItem(optptr, 1);
    if (err != 0)
    {
      exit(EXIT_FAILURE);
    }
    /*insert default highlight draw code?*/
    err=SetDialogCancelItem(optptr, 2);
    if (err != 0)
    {
      exit(EXIT_FAILURE);
    }
    err=SetDialogTracksCursor(optptr, true);
    if (err != 0)
    {
      exit(EXIT_FAILURE);
    }
    GetDItem(optptr, 16/*normal radio button*/, &the_type, &the_handle, &the_rect);
    SetCtlValue((ControlHandle)the_handle, true);

    while(!done)/*loop*/
    {
      ModalDialog(0L, &the_item);/* don't feed 0 where a upp is expected? */
      	/* old book suggests using OL(NIL) as the first argument, but
           It doesn't include anything about UPPs either, so... */
      switch (the_item)
      {
        case 1:
          done = true;
        break;
        case 2:
          exit(EXIT_SUCCESS);
        break;
        case 13:
          GetDItem(optptr, 13, &the_type, &the_handle, &the_rect);
          srvarg.metaserver_no_send=GetCtlValue((ControlHandle)the_handle);
          SetCtlValue((ControlHandle)the_handle, !srvarg.metaserver_no_send);
        break;
        case 15:
        case 16:
        case 17:
          GetDItem(optptr, old_item, &the_type, &the_handle, &the_rect);
          SetCtlValue((ControlHandle)the_handle, false);
          old_item=the_item;
          GetDItem(optptr, the_item, &the_type, &the_handle, &the_rect);
          SetCtlValue((ControlHandle)the_handle, true);
        break;
      }
    }
    /* now, load the dialog items into the correct variables interpritation */
    GetDItem( optptr, 4, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, (unsigned char *)srvarg.load_filename);
    GetDItem( optptr, 8, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, (unsigned char *)srvarg.log_filename);
    GetDItem( optptr, 12, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, the_string);
    sscanf(the_string, "%d", srvarg.port);
    GetDItem( optptr, 10, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, (unsigned char *)srvarg.script_filename);
    GetDItem(optptr, 15, &the_type, &the_handle, &the_rect);
    if(GetControlValue((ControlHandle)the_handle))
    {
      srvarg.loglevel=LOG_FATAL;
    }
    GetDItem(optptr, 16, &the_type, &the_handle, &the_rect);
    if(GetControlValue((ControlHandle)the_handle))
    {
      srvarg.loglevel=LOG_NORMAL;
    }
    GetDItem(optptr, 17, &the_type, &the_handle, &the_rect);
    if(GetControlValue((ControlHandle)the_handle))
    {
      srvarg.loglevel=LOG_VERBOSE;
    }
    DisposeDialog(optptr);/*get rid of the dialog after sorting out the options*/
    DisposePtr(storage);/*clean up the allocated memory*/
  }
#endif /* HARDCODED_OPT */
#undef  HARDCODED_OPT
}
#endif /* GENERATING_MAC */
