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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/List.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "government.h"
#include "packets.h"
#include "player.h"

/* client */
#include "client_main.h"

/* gui-xaw */
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "optiondlg.h"

#include "inteldlg.h"

/******************************************************************/
struct intel_dialog {
  struct player *pplayer;

  Widget intel_dialog_shell;
  Widget intel_form;
  Widget intel_label;
  Widget intel_close_command;
  Widget intel_diplo_command;
  bool intel_dialog_shell_is_raised;

  Widget intel_diplo_dialog_shell;
  Widget intel_diplo_form;
  Widget intel_diplo_label;
  Widget intel_diplo_list;
  Widget intel_diplo_close_command;
  bool intel_diplo_dialog_shell_is_raised;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct intel_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct intel_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list = NULL;
static bool dialog_list_has_been_initialised = FALSE;

/******************************************************************/

struct intel_dialog *get_intel_dialog(struct player *pplayer);
void popdown_intel_dialog(struct intel_dialog *pdialog);
void create_intel_dialog(struct intel_dialog *pdialog, bool raise);

void intel_close_callback(Widget w, XtPointer client_data,
			  XtPointer call_data);
void intel_diplo_callback(Widget w, XtPointer client_data,
			  XtPointer call_data);

void popup_intel_diplo_dialog(struct player *pplayer, bool raise);
void popdown_intel_diplo_dialog(struct intel_dialog *pdialog);
void create_intel_diplo_dialog(struct intel_dialog *pdialog, bool raise);
void update_intel_diplo_dialog(struct intel_dialog *pdialog);
void intel_diplo_close_callback(Widget w, XtPointer client_data,
				XtPointer call_data);

/****************************************************************
  Get an intelligence dialog for the given player.
*****************************************************************/
struct intel_dialog *get_intel_dialog(struct player *pplayer)
{
  if (!dialog_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_has_been_initialised = TRUE;
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pplayer == pplayer) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/****************************************************************
  Popup an intelligence dialog for the given player.
*****************************************************************/
void popup_intel_dialog(struct player *pplayer)
{
  struct intel_dialog *pdialog;
  bool raise = FALSE; /* Drop when we'll have raise-parametered popup */

  if (!(pdialog = get_intel_dialog(pplayer))) {
    pdialog = fc_malloc(sizeof(struct intel_dialog));
    pdialog->pplayer = pplayer;
    pdialog->intel_dialog_shell = 0;
    pdialog->intel_diplo_dialog_shell = 0;
    dialog_list_prepend(dialog_list, pdialog);
  }

  if (!(pdialog->intel_dialog_shell)) {
    create_intel_dialog(pdialog, raise);
  }

  xaw_set_relative_position(toplevel, pdialog->intel_dialog_shell, 25, 25);
  XtPopup(pdialog->intel_dialog_shell, XtGrabNone);
  if (pdialog->intel_dialog_shell_is_raised) {
    XtSetSensitive(main_form, FALSE);
  }
}

/****************************************************************
  Close an intelligence dialog.
*****************************************************************/
void popdown_intel_dialog(struct intel_dialog *pdialog)
{
  if (pdialog->intel_dialog_shell_is_raised) {
    XtSetSensitive(main_form, TRUE);
  }
  XtDestroyWidget(pdialog->intel_dialog_shell);
  pdialog->intel_dialog_shell = 0;

  if (!(pdialog->intel_diplo_dialog_shell)) {
    dialog_list_unlink(dialog_list, pdialog);
    free(pdialog);
  }
}

/****************************************************************
...
*****************************************************************/
void create_intel_dialog(struct intel_dialog *pdialog, bool raise)
{
  char buf[64];
  struct city *pcity;

  static char *tech_list_names_ptrs[A_LAST+1];
  static char tech_list_names[A_LAST+1][200];
  int j = 0;

  pdialog->intel_dialog_shell_is_raised = raise;

  I_T(pdialog->intel_dialog_shell =
    XtCreatePopupShell("intelpopup",
		       raise ? transientShellWidgetClass
		       : topLevelShellWidgetClass,
		       toplevel, NULL, 0));

  pdialog->intel_form = XtVaCreateManagedWidget("intelform",
						formWidgetClass,
						pdialog->intel_dialog_shell,
						NULL);

  my_snprintf(buf, sizeof(buf),
	      _("Intelligence Information for the %s Empire"),
	      nation_adjective_for_player(pdialog->pplayer));

  pdialog->intel_label = I_L(XtVaCreateManagedWidget("inteltitlelabel",
						     labelWidgetClass,
						     pdialog->intel_form,
						     XtNlabel, buf,
						     NULL));

  my_snprintf(buf, sizeof(buf), _("Ruler: %s %s"),
	      ruler_title_translation(pdialog->pplayer),
	      player_name(pdialog->pplayer));
  XtVaCreateManagedWidget("intelnamelabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  my_snprintf(buf, sizeof(buf), _("Government: %s"),
	      government_name_for_player(pdialog->pplayer));
  XtVaCreateManagedWidget("intelgovlabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  my_snprintf(buf, sizeof(buf), _("Gold: %d"),
	      pdialog->pplayer->economic.gold);
  XtVaCreateManagedWidget("intelgoldlabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  my_snprintf(buf, sizeof(buf), _("Tax: %d%%"),
	      pdialog->pplayer->economic.tax);
  XtVaCreateManagedWidget("inteltaxlabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  my_snprintf(buf, sizeof(buf), _("Science: %d%%"),
	      pdialog->pplayer->economic.science);
  XtVaCreateManagedWidget("intelscilabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  my_snprintf(buf, sizeof(buf), _("Luxury: %d%%"),
	      pdialog->pplayer->economic.luxury);
  XtVaCreateManagedWidget("intelluxlabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  switch (get_player_research(pdialog->pplayer)->researching) {
  case A_UNKNOWN:
    my_snprintf(buf, sizeof(buf), _("Researching: (Unknown)"));
    break;
  case A_UNSET:
    my_snprintf(buf, sizeof(buf), _("Researching: Unknown(%d/-)"),
		get_player_research(pdialog->pplayer)->bulbs_researched);
    break;
  default:
    my_snprintf(buf, sizeof(buf), _("Researching: %s(%d/%d)"),
		advance_name_researching(pdialog->pplayer),
		get_player_research(pdialog->pplayer)->bulbs_researched,
		total_bulbs_required(pdialog->pplayer));
    break;
  };

  XtVaCreateManagedWidget("intelreslabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  pcity = find_palace(pdialog->pplayer);
  my_snprintf(buf, sizeof(buf), _("Capital: %s"),
	      /* TRANS: "unknown" location */
	      (!pcity) ? _("(unknown)") : city_name(pcity));
  XtVaCreateManagedWidget("intelcapitallabel",
			  labelWidgetClass,
			  pdialog->intel_form,
			  XtNlabel, buf,
			  NULL);

  advance_index_iterate(A_FIRST, i) {
    if (player_invention_state(pdialog->pplayer, i) == TECH_KNOWN) {
      if (TECH_KNOWN == player_invention_state(client.conn.playing, i)) {
	sz_strlcpy(tech_list_names[j], advance_name_translation(advance_by_number(i)));
      } else {
	my_snprintf(tech_list_names[j], sizeof(tech_list_names[j]),
		    "%s*", advance_name_translation(advance_by_number(i)));
      }
      tech_list_names_ptrs[j]=tech_list_names[j];
      j++;
    }
  } advance_index_iterate_end;
  tech_list_names_ptrs[j]=0;

  XtVaCreateManagedWidget("inteltechlist",
			  listWidgetClass,
			  pdialog->intel_form,
			  XtNlist, tech_list_names_ptrs,
			  NULL);

  pdialog->intel_close_command =
    I_L(XtVaCreateManagedWidget("intelclosecommand",
				commandWidgetClass,
				pdialog->intel_form, NULL));

  pdialog->intel_diplo_command =
    I_L(XtVaCreateManagedWidget("inteldiplocommand",
				commandWidgetClass,
				pdialog->intel_form, NULL));

  XtAddCallback(pdialog->intel_close_command, XtNcallback,
		intel_close_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->intel_diplo_command, XtNcallback,
		intel_diplo_callback,
		INT_TO_XTPOINTER(player_number(pdialog->pplayer)));
  XtRealizeWidget(pdialog->intel_dialog_shell);

  xaw_horiz_center(pdialog->intel_label);

  XSetWMProtocols(display, XtWindow(pdialog->intel_dialog_shell), 
		  &wm_delete_window, 1);
  XtOverrideTranslations(pdialog->intel_dialog_shell,
    XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-intel()"));
}

/****************************************************************************
  Update the intelligence dialog for the given player.  This is called by
  the core client code when that player's information changes.
****************************************************************************/
void update_intel_dialog(struct player *p)
{
  /* PORTME */
}

/**************************************************************************
...
**************************************************************************/
void intel_close_callback(Widget w, XtPointer client_data,
			  XtPointer call_data)
{
  popdown_intel_dialog((struct intel_dialog *)client_data);
}

/**************************************************************************
...
**************************************************************************/
void intel_diplo_callback(Widget w, XtPointer client_data,
			  XtPointer call_data)
{
  popup_intel_diplo_dialog(player_by_number(XTPOINTER_TO_INT(client_data)),
			   FALSE);
}

/**************************************************************************
...
**************************************************************************/
void intel_dialog_msg_close(Widget w)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->intel_dialog_shell == w) {
      popdown_intel_dialog(pdialog);
      return;
    }
  } dialog_list_iterate_end;
}

/****************************************************************
  Popup a player's relations dialog for the given player.
*****************************************************************/
void popup_intel_diplo_dialog(struct player *pplayer, bool raise)
{
  struct intel_dialog *pdialog;
  
  if (!(pdialog = get_intel_dialog(pplayer))) {
    pdialog = fc_malloc(sizeof(struct intel_dialog));
    pdialog->pplayer = pplayer;
    pdialog->intel_dialog_shell = 0;
    pdialog->intel_diplo_dialog_shell = 0;
    dialog_list_prepend(dialog_list, pdialog);
  }

  if (!(pdialog->intel_diplo_dialog_shell)) {
    create_intel_diplo_dialog(pdialog, raise);
  }

  xaw_set_relative_position(toplevel,
			    pdialog->intel_diplo_dialog_shell, 25, 25);
  XtPopup(pdialog->intel_diplo_dialog_shell, XtGrabNone);
  if (pdialog->intel_diplo_dialog_shell_is_raised) {
    XtSetSensitive(main_form, FALSE);
  }
}

/****************************************************************
  Close a player's relations dialog.
*****************************************************************/
void popdown_intel_diplo_dialog(struct intel_dialog *pdialog)
{
  if (pdialog->intel_diplo_dialog_shell_is_raised) {
    XtSetSensitive(main_form, TRUE);
  }
  XtDestroyWidget(pdialog->intel_diplo_dialog_shell);
  pdialog->intel_diplo_dialog_shell = 0;

  if (!(pdialog->intel_dialog_shell)) {
    dialog_list_unlink(dialog_list, pdialog);
    free(pdialog);
  }
}

/****************************************************************
  Create a player's relations dialog for the given player.
*****************************************************************/
void create_intel_diplo_dialog(struct intel_dialog *pdialog, bool raise)
{
  char buf[64];

  pdialog->intel_diplo_dialog_shell_is_raised = raise;

  pdialog->intel_diplo_dialog_shell =
    I_IN(I_T(XtCreatePopupShell("inteldiplopopup",
				raise ? transientShellWidgetClass
				: topLevelShellWidgetClass,
				toplevel, NULL, 0)));

  pdialog->intel_diplo_form =
    XtVaCreateManagedWidget("inteldiploform",
			    formWidgetClass,
			    pdialog->intel_diplo_dialog_shell,
			    NULL);

  my_snprintf(buf, sizeof(buf),
	      _("Intelligence Diplomacy Information for the %s Empire"),
	      nation_adjective_for_player(pdialog->pplayer));

  pdialog->intel_diplo_label =
    I_L(XtVaCreateManagedWidget("inteldiplolabel",
				labelWidgetClass,
				pdialog->intel_diplo_form,
				XtNlabel, buf,
				NULL));
   
  my_snprintf(buf, sizeof(buf), _("Ruler: %s %s"), 
	      ruler_title_translation(pdialog->pplayer),
	      player_name(pdialog->pplayer));
  XtVaCreateManagedWidget("inteldiplonamelabel", 
			  labelWidgetClass, 
			  pdialog->intel_diplo_form, 
			  XtNlabel, buf,
			  NULL);   

  pdialog->intel_diplo_list =
    XtVaCreateManagedWidget("inteldiplolist",
			    listWidgetClass,
			    pdialog->intel_diplo_form,
			    NULL);

  pdialog->intel_diplo_close_command =
    I_L(XtVaCreateManagedWidget("inteldiploclosecommand",
				commandWidgetClass,
				pdialog->intel_diplo_form,
				NULL));

  XtAddCallback(pdialog->intel_diplo_close_command, XtNcallback,
		intel_diplo_close_callback, (XtPointer)pdialog);

  update_intel_diplo_dialog(pdialog);

  XtRealizeWidget(pdialog->intel_diplo_dialog_shell);
  
  XSetWMProtocols(display, XtWindow(pdialog->intel_diplo_dialog_shell), 
		  &wm_delete_window, 1);
  XtOverrideTranslations(pdialog->intel_diplo_dialog_shell,
    XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-intel-diplo()"));
}

/****************************************************************
  Update a player's relations dialog for the given player.
*****************************************************************/
void update_intel_diplo_dialog(struct intel_dialog *pdialog)
{
  int i;
  Dimension width;
  static char *namelist_ptrs[MAX_NUM_PLAYERS];
  static char namelist_text[MAX_NUM_PLAYERS][72];
  const struct player_diplstate *state;

  if (pdialog->intel_diplo_dialog_shell) {
    i = 0;
    players_iterate(other) {
      if (other == pdialog->pplayer) {
	continue;
      }
      state = pplayer_get_diplstate(pdialog->pplayer, other);
      my_snprintf(namelist_text[i], sizeof(namelist_text[i]),
		  "%-32s %-16s %-16s",
		  player_name(other),
		  nation_adjective_for_player(other),
		  diplstate_text(state->type));
      namelist_ptrs[i] = namelist_text[i];
      i++;
    } players_iterate_end;

    XawListChange(pdialog->intel_diplo_list, namelist_ptrs, i, 0, True);

    XtVaGetValues(pdialog->intel_diplo_list, XtNwidth, &width, NULL);
    XtVaSetValues(pdialog->intel_diplo_label, XtNwidth, width, NULL); 
  }
}

/**************************************************************************
...
**************************************************************************/
void intel_diplo_close_callback(Widget w, XtPointer client_data,
			  XtPointer call_data)
{
  popdown_intel_diplo_dialog((struct intel_dialog *)client_data);
}

/**************************************************************************
...
**************************************************************************/
void intel_diplo_dialog_msg_close(Widget w)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->intel_diplo_dialog_shell == w) {
      popdown_intel_diplo_dialog(pdialog);
      return;
    }
  } dialog_list_iterate_end;
}
