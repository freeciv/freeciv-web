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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/AsciiText.h>  
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Viewport.h>	/* for racesdlg */

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"

/* common */
#include "game.h"
#include "government.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "unitlist.h"

/* client */
#include "chatline.h"
#include "cityrep.h"	/* for popdown_city_report_dialog */
#include "client_main.h"
#include "climisc.h"
#include "control.h" /* request_xxx and set_unit_focus */
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapctrl.h"
#include "mapctrl_common.h"
#include "mapview.h"
#include "messagewin.h"	/* for popdown_meswin_dialog */
#include "options.h"
#include "packhand.h"
#include "plrdlg.h"	/* for popdown_players_dialog */
#include "repodlgs.h"	/* for popdown_xxx_dialog */
#include "tilespec.h"

#include "dialogs.h"

/******************************************************************/
void popdown_notify_dialog(void);
static Widget notify_dialog_shell;

/******************************************************************/
static Widget races_dialog_shell=NULL;
static Widget races_form, races_toggles_form, races_label;
static Widget races_toggles_viewport;
static Widget *races_toggles=NULL;
static struct nation_type **races_toggles_to_nations = NULL;
static int *nation_idx_to_race_toggle = NULL;
struct player *races_player;
static Widget races_leader_form, races_leader;
static Widget races_leader_pick_popupmenu, races_leader_pick_menubutton;
static Widget races_sex_toggles[2], races_sex_form, races_sex_label;
static Widget races_style_form, races_style_label;
static Widget *races_style_toggles=NULL;
static Widget races_action_form;
static Widget races_ok_command, races_random_command, races_quit_command;


/******************************************************************/
#define MAX_SELECT_UNITS 100
static Widget unit_select_dialog_shell;
static Widget unit_select_form;
static Widget unit_select_commands[MAX_SELECT_UNITS];
static Widget unit_select_labels[MAX_SELECT_UNITS];
static Pixmap unit_select_pixmaps[MAX_SELECT_UNITS];
static int unit_select_ids[MAX_SELECT_UNITS];
static int unit_select_no;

void about_button_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data);

void help_button_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data);

void create_rates_dialog(void);
void create_about_dialog(void);
void create_help_dialog(Widget *shell);


/******************************************************************/
static void create_races_dialog(struct player *pplayer);
static void races_leader_set_values(struct nation_type *race, int lead);
static int races_buttons_get_current(void);
static int races_sex_buttons_get_current(void);
static int races_style_buttons_get_current(void);
static void races_sex_buttons_set_current(int i);

static int races_indirect_compare(const void *first, const void *second);

static void races_toggles_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
static void races_leader_pick_callback(Widget w, XtPointer client_data,
				       XtPointer call_data);
static void races_ok_command_callback(Widget w, XtPointer client_data, 
				      XtPointer call_data);
static void races_random_command_callback(Widget w, XtPointer client_data, 
					      XtPointer call_data);
static void races_quit_command_callback(Widget w, XtPointer client_data, 
					XtPointer call_data);

/******************************************************************/
void unit_select_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data);
void unit_select_all_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data);


/******************************************************************/
static int city_style_idx[64];     /* translation table basic style->city_style  */
static int city_style_ridx[64];    /* translation table the other way            */
                                   /* they in fact limit the num of styles to 64 */
static int b_s_num; /* num basic city styles, i.e. those that you can start with */


/******************************************************************/

int is_showing_pillage_dialog = FALSE;
int unit_to_use_to_pillage;

int caravan_city_id;
int caravan_unit_id;

struct city *pcity_caravan_dest;
struct unit *punit_caravan;

static Widget caravan_dialog;

/****************************************************************
...
*****************************************************************/
static void notify_command_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data)
{
  popdown_notify_dialog();
}

/****************************************************************
 ...
*****************************************************************/
static void select_random_race(void)
{
  /* try to find a free nation */
  /* FIXME: this code should be done another way. -ev */
  while (1) {
    unsigned int race_toggle_index = myrand(nation_count());

    if (!is_nation_playable(nation_by_number(race_toggle_index))
	|| !nation_by_number(race_toggle_index)->is_available
	|| nation_by_number(race_toggle_index)->player) {
      continue;
    }
    if (XtIsSensitive(races_toggles[race_toggle_index])) {
      x_simulate_button_click(races_toggles[race_toggle_index]);
      break;
    }
  }
}

/**************************************************************************
  Popup a generic dialog to display some generic information.
**************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
			 const char *lines)
{
  Widget notify_form, notify_command;
  Widget notify_headline, notify_label;
  Dimension width, width2;
  
  notify_dialog_shell = XtCreatePopupShell(caption,
					   transientShellWidgetClass,
					   toplevel, NULL, 0);

  notify_form = XtVaCreateManagedWidget("notifyform", 
					 formWidgetClass, 
					 notify_dialog_shell, NULL);

  notify_headline=XtVaCreateManagedWidget("notifyheadline", 
			  labelWidgetClass, notify_form, 
			  XtNlabel, headline,
			  NULL);

  
  notify_label=XtVaCreateManagedWidget("notifylabel", 
			  labelWidgetClass, notify_form, 
			  XtNlabel, lines,
			  NULL);   

  notify_command =
    I_L(XtVaCreateManagedWidget("notifycommand", 
				commandWidgetClass,
				notify_form,
				NULL));

  XtVaGetValues(notify_label, XtNwidth, &width, NULL);
  XtVaGetValues(notify_headline, XtNwidth, &width2, NULL);
  if(width>width2)
    XtVaSetValues(notify_headline, XtNwidth, width, NULL); 
  
  XtAddCallback(notify_command, XtNcallback, notify_command_callback, NULL);
  
  xaw_set_relative_position(toplevel, notify_dialog_shell, 25, 5);
  XtPopup(notify_dialog_shell, XtGrabNone);
  XtSetSensitive(toplevel, FALSE);
}

/****************************************************************
  Closes the notification dialog.
*****************************************************************/
void popdown_notify_dialog(void)
{
  if (notify_dialog_shell) {
    XtDestroyWidget(notify_dialog_shell);
    XtSetSensitive(toplevel, TRUE);
    notify_dialog_shell = 0;
  }
}

/****************************************************************
...
*****************************************************************/

/* surely this should use genlists??  --dwp */
struct widget_list {
  Widget w;
  struct tile *tile;
  struct widget_list *next;
};
static struct widget_list *notify_goto_widget_list = NULL;

static void notify_goto_widget_remove(Widget w)
{
  struct widget_list *cur, *tmp;
  cur=notify_goto_widget_list;
  if (!cur)
    return;
  if (cur && cur->w == w) {
    cur = cur->next;
    free(notify_goto_widget_list);
    notify_goto_widget_list = cur;
    return;
  }
  for (; cur->next && cur->next->w!= w; cur=cur->next);
  if (cur->next) {
    tmp = cur->next;
    cur->next = cur->next->next;
    free(tmp);
  }
}

static struct tile *notify_goto_find_widget(Widget w)
{
  struct widget_list *cur;

  for (cur = notify_goto_widget_list; cur && cur->w !=w; cur = cur->next) {
    /* Nothing. */
  }

  if (cur) {
    return cur->tile;
  } else {
    return NULL;
  }
}

static void notify_goto_add_widget_tile(Widget w, struct tile *ptile)
{
  struct widget_list *newwidget;

  newwidget = fc_malloc(sizeof(*newwidget));
  newwidget->w = w;
  newwidget->tile = ptile;
  newwidget->next = notify_goto_widget_list;
  notify_goto_widget_list = newwidget;
}

static void notify_goto_command_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data)
{
  struct tile *ptile =  notify_goto_find_widget(w);

  center_tile_mapcanvas(ptile);
  notify_goto_widget_remove(w);

  XtDestroyWidget(XtParent(XtParent(w)));
  XtSetSensitive(toplevel, TRUE);
}

static void notify_no_goto_command_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data)
{
  notify_goto_widget_remove(w);
  XtDestroyWidget(XtParent(XtParent(w)));
  XtSetSensitive(toplevel, TRUE);
}


/**************************************************************************
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
**************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  Widget notify_dialog_shell, notify_form, notify_command, notify_goto_command;
  Widget notify_headline, notify_label;
  Dimension width, width2, width_1, width_2;
  
  if (!ptile) {
    popup_notify_dialog("Message:", headline, lines);
    return;
  }
  notify_dialog_shell = XtCreatePopupShell("Message:",
					   transientShellWidgetClass,
					   toplevel, NULL, 0);

  notify_form = XtVaCreateManagedWidget("notifyform", 
					 formWidgetClass, 
					 notify_dialog_shell, NULL);

  notify_headline=XtVaCreateManagedWidget("notifyheadline", 
			  labelWidgetClass, notify_form, 
			  XtNlabel, headline,
			  NULL);

  
  notify_label=XtVaCreateManagedWidget("notifylabel", 
			  labelWidgetClass, notify_form, 
			  XtNlabel, lines,
			  NULL);   

  notify_command =
    I_L(XtVaCreateManagedWidget("notifycommand", 
				commandWidgetClass,
				notify_form,
				NULL));

  notify_goto_command =
    I_L(XtVaCreateManagedWidget("notifygotocommand", 
				commandWidgetClass,
				notify_form,
				NULL));
  
  XtVaGetValues(notify_label, XtNwidth, &width, NULL);
  XtVaGetValues(notify_headline, XtNwidth, &width2, NULL);
  XtVaGetValues(notify_command, XtNwidth, &width_1, NULL);
  XtVaGetValues(notify_goto_command, XtNwidth, &width_2, NULL);
  if (width_1 + width_2 > width) width = width_1 + width_2;
  if(width>width2)
    XtVaSetValues(notify_headline, XtNwidth, width, NULL); 
  
  XtAddCallback(notify_command, XtNcallback, notify_no_goto_command_callback, NULL);
  XtAddCallback(notify_goto_command, XtNcallback, notify_goto_command_callback, NULL);
  notify_goto_add_widget_tile(notify_goto_command, ptile);
  xaw_set_relative_position(toplevel, notify_dialog_shell, 25, 5);
  XtPopup(notify_dialog_shell, XtGrabNone);
  /*  XtSetSensitive(toplevel, FALSE); */
}

/**************************************************************************
  Popup a dialog to display connection message from server.
**************************************************************************/
void popup_connect_msg(const char *headline, const char *message)
{
  /* FIXME: Needs proper implementation.
   *        Now just puts to chat window so message is not completely lost. */

  output_window_append(FTC_SERVER_INFO, NULL, message);
}

/****************************************************************
...
*****************************************************************/
static void caravan_establish_trade_callback(Widget w, XtPointer client_data,
					     XtPointer call_data)
{
  dsend_packet_unit_establish_trade(&client.conn, caravan_unit_id);
  destroy_message_dialog(w);
  caravan_dialog = 0;
  process_caravan_arrival(NULL);
}


/****************************************************************
...
*****************************************************************/
static void caravan_help_build_wonder_callback(Widget w,
					       XtPointer client_data,
					       XtPointer call_data)
{
  dsend_packet_unit_help_build_wonder(&client.conn, caravan_unit_id);

  destroy_message_dialog(w);
  caravan_dialog = 0;
  process_caravan_arrival(NULL);
}


/****************************************************************
...
*****************************************************************/
static void caravan_keep_moving_callback(Widget w, XtPointer client_data, 
					 XtPointer call_data)
{
  destroy_message_dialog(w);
  caravan_dialog = 0;
  process_caravan_arrival(NULL);
}


/****************************************************************
...
*****************************************************************/
void popup_caravan_dialog(struct unit *punit,
			  struct city *phomecity, struct city *pdestcity)
{
  char buf[128];
  
  my_snprintf(buf, sizeof(buf),
	      _("Your caravan from %s reaches the city of %s.\nWhat now?"),
	      city_name(phomecity), city_name(pdestcity));
  
  caravan_city_id=pdestcity->id; /* callbacks need these */
  caravan_unit_id=punit->id;
  
  caravan_dialog=popup_message_dialog(toplevel, "caravandialog", 
			   buf,
			   caravan_establish_trade_callback, 0, 0,
			   caravan_help_build_wonder_callback, 0, 0,
			   caravan_keep_moving_callback, 0, 0,
			   NULL);
  
  if (!can_cities_trade(phomecity, pdestcity))
    XtSetSensitive(XtNameToWidget(caravan_dialog, "*button0"), FALSE);
  
  if(!unit_can_help_build_wonder(punit, pdestcity))
    XtSetSensitive(XtNameToWidget(caravan_dialog, "*button1"), FALSE);
}

/****************************************************************
...
*****************************************************************/
bool caravan_dialog_is_open(int* unit_id, int* city_id)
{
  return BOOL_VAL(caravan_dialog);
}

/**************************************************************************
  Updates caravan dialog
**************************************************************************/
void caravan_dialog_update(void)
{
/* PORT ME */
}

/****************************************************************
...
*****************************************************************/
static void revolution_callback_yes(Widget w, XtPointer client_data, 
				    XtPointer call_data)
{
  struct government *pgovernment = client_data;

  if (!pgovernment) {
    start_revolution();
  } else {
    /* Player have choosed government */
    set_government_choice(pgovernment);
  }
  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void revolution_callback_no(Widget w, XtPointer client_data, 
				   XtPointer call_data)
{
  destroy_message_dialog(w);
}



/****************************************************************
...
*****************************************************************/
void popup_revolution_dialog(struct government *pgovernment)
{
  popup_message_dialog(toplevel, "revolutiondialog",
		       _("You say you wanna revolution?"),
		       revolution_callback_yes, pgovernment, 0,
		       revolution_callback_no, 0, 0,
		       NULL);
}


/****************************************************************
...
*****************************************************************/
static void pillage_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data)
{
  if (!is_showing_pillage_dialog) {
    destroy_message_dialog (w);
    return;
  }

  if (client_data) {
    struct unit *punit = game_find_unit_by_number (unit_to_use_to_pillage);
    if (punit) {
      Base_type_id pillage_base = -1;
      int what = XTPOINTER_TO_INT(client_data);

      if (what > S_LAST) {
        pillage_base = what - S_LAST - 1;
        what = S_LAST;
      }

      request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE,
					 what, pillage_base);
    }
  }

  destroy_message_dialog (w);
  is_showing_pillage_dialog = FALSE;
}

/****************************************************************
...
*****************************************************************/
void popup_pillage_dialog(struct unit *punit,
			  bv_special may_pillage,
                          bv_bases bases)
{
  Widget shell, form, dlabel, button, prev;
  int what;
  enum tile_special_type prereq;

  if (is_showing_pillage_dialog) {
    return;
  }
  is_showing_pillage_dialog = TRUE;
  unit_to_use_to_pillage = punit->id;

  XtSetSensitive (toplevel, FALSE);

  shell = I_T(XtCreatePopupShell("pillagedialog", transientShellWidgetClass,
				 toplevel, NULL, 0));
  form = XtVaCreateManagedWidget ("form", formWidgetClass, shell, NULL);
  dlabel = I_L(XtVaCreateManagedWidget("dlabel", labelWidgetClass, form, NULL));

  prev = dlabel;
  while ((what = get_preferred_pillage(may_pillage, bases)) != S_LAST) {
    bv_special what_bv;
    bv_bases what_base;

    BV_CLR_ALL(what_bv);
    BV_CLR_ALL(what_base);

    if (what > S_LAST) {
      BV_SET(what_base, what - S_LAST - 1);
    } else {
      BV_SET(what_bv, what);
    }

    button =
      XtVaCreateManagedWidget ("button", commandWidgetClass, form,
                               XtNfromVert, prev,
                               XtNlabel,
                               (XtArgVal)(get_infrastructure_text(what_bv,
                                                                  what_base)),
                               NULL);
    XtAddCallback(button, XtNcallback, pillage_callback,
                  INT_TO_XTPOINTER(what));

    if (what > S_LAST) {
      BV_CLR(bases, what - S_LAST - 1);
    } else {
      clear_special(&may_pillage, what);
      prereq = get_infrastructure_prereq(what);
      if (prereq != S_LAST) {
        clear_special(&may_pillage, prereq);
      }
    }
    prev = button;
  }
  button =
    I_L(XtVaCreateManagedWidget("closebutton", commandWidgetClass, form,
				XtNfromVert, prev,
				NULL));
  XtAddCallback (button, XtNcallback, pillage_callback, NULL);

  xaw_set_relative_position (toplevel, shell, 10, 0);
  XtPopup (shell, XtGrabNone);
}

/****************************************************************
  Parameters after named parameters should be in triplets:
  - callback, callback_data, fixed_width 
*****************************************************************/
Widget popup_message_dialog(Widget parent, const char *dialogname,
			    const char *text, ...)
{
  va_list args;
  Widget dshell, dform, button;
  Position x, y;
  Dimension width, height;
  void (*fcb)(Widget, XtPointer, XtPointer);
  XtPointer client_data;
  char button_name[512];
  int i, fixed_width;

  XtSetSensitive(parent, FALSE);
  
  I_T(dshell=XtCreatePopupShell(dialogname, transientShellWidgetClass,
				parent, NULL, 0));
  
  dform=XtVaCreateManagedWidget("dform", formWidgetClass, dshell, NULL);

  /* caller should i18n text as desired */
  XtVaCreateManagedWidget("dlabel", labelWidgetClass, dform, 
			  XtNlabel, (XtArgVal)text,
			  NULL);   

  i=0;
  va_start(args, text);

  while((fcb=((void(*)(Widget, XtPointer, XtPointer))(va_arg(args, void *))))) {
    client_data=va_arg(args, XtPointer);
    fixed_width=va_arg(args, int);
    my_snprintf(button_name, sizeof(button_name), "button%d", i++);
    
    button=XtVaCreateManagedWidget(button_name, commandWidgetClass, 
				   dform, NULL);
    if (fixed_width) {
      I_LW(button);
    } else {
      I_L(button);
    }
    XtAddCallback(button, XtNcallback, fcb, client_data);
  }
  
  va_end(args);

  XtVaGetValues(parent, XtNwidth, &width, XtNheight, &height, NULL);
  XtTranslateCoords(parent, (Position) width/10, (Position) height/10,
		    &x, &y);
  XtVaSetValues(dshell, XtNx, x, XtNy, y, NULL);
  
  XtPopup(dshell, XtGrabNone);

  return dshell;
}

/****************************************************************
...
*****************************************************************/
void destroy_message_dialog(Widget button)
{
  XtSetSensitive(XtParent(XtParent(XtParent(button))), TRUE);

  XtDestroyWidget(XtParent(XtParent(button)));
}

static int number_of_columns(int n)
{
#if 0
  /* This would require libm, which isn't worth it for this one little
   * function.  Since MAX_SELECT_UNITS is 100 already, the ifs
   * work fine.  */
  double sqrt(); double ceil();
  return ceil(sqrt((double)n/5.0));
#else
  assert(MAX_SELECT_UNITS == 100);
  if(n<=5) return 1;
  else if(n<=20) return 2;
  else if(n<=45) return 3;
  else if(n<=80) return 4;
  else return 5;
#endif
}
static int number_of_rows(int n)
{
  int c=number_of_columns(n);
  return (n+c-1)/c;
}

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_unit_select_dialog(struct tile *ptile)
{
  int i,n,r;
  char buffer[512];
  Arg args[4];
  int nargs;
  Widget unit_select_all_command, unit_select_close_command;
  Widget firstcolumn=0,column=0;
  Pixel bg;
  struct unit *unit_list[unit_list_size(ptile->units)];

  XtSetSensitive(main_form, FALSE);

  unit_select_dialog_shell =
    I_T(XtCreatePopupShell("unitselectdialogshell", 
			   transientShellWidgetClass,
			   toplevel, NULL, 0));

  unit_select_form = XtVaCreateManagedWidget("unitselectform", 
					     formWidgetClass, 
					     unit_select_dialog_shell, NULL);

  XtVaGetValues(unit_select_form, XtNbackground, &bg, NULL);
  XSetForeground(display, fill_bg_gc, bg);

  n = MIN(MAX_SELECT_UNITS, unit_list_size(ptile->units));
  r = number_of_rows(n);

  fill_tile_unit_list(ptile, unit_list);

  for(i=0; i<n; i++) {
    struct unit *punit = unit_list[i];
    struct unit_type *punittemp=unit_type(punit);
    struct city *pcity;
    struct canvas store;
    
    if(!(i%r))  {
      nargs=0;
      if(i)  { XtSetArg(args[nargs], XtNfromHoriz, column); nargs++;}
      column = XtCreateManagedWidget("column", formWidgetClass,
				     unit_select_form,
				     args, nargs);
      if(!i) firstcolumn=column;
    }

    unit_select_ids[i]=punit->id;

    pcity = player_find_city_by_id(client.conn.playing, punit->homecity);
    
    my_snprintf(buffer, sizeof(buffer), "%s(%s)\n%s", 
	    utype_name_translation(punittemp), 
	    pcity ? city_name(pcity) : "",
	    unit_activity_text(punit));

    unit_select_pixmaps[i]=XCreatePixmap(display, XtWindow(map_canvas), 
					 tileset_full_tile_width(tileset), tileset_full_tile_height(tileset),
					 display_depth);

    XFillRectangle(display, unit_select_pixmaps[i], fill_bg_gc,
		   0, 0, tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));
    store.pixmap = unit_select_pixmaps[i];
    put_unit(punit, &store, 0, 0);

    nargs=0;
    XtSetArg(args[nargs], XtNbitmap, (XtArgVal)unit_select_pixmaps[i]);nargs++;
    XtSetArg(args[nargs], XtNsensitive, 
             can_unit_do_activity(punit, ACTIVITY_IDLE));nargs++;
    if(i%r)  {
      XtSetArg(args[nargs], XtNfromVert, unit_select_commands[i-1]); nargs++;
    }
    unit_select_commands[i]=XtCreateManagedWidget("unitselectcommands", 
						  commandWidgetClass,
						  column, args, nargs);

    nargs=0;
    XtSetArg(args[nargs], XtNlabel, (XtArgVal)buffer); nargs++;
    XtSetArg(args[nargs], XtNfromHoriz, unit_select_commands[i]); nargs++;
    if(i%r) {
      XtSetArg(args[nargs], XtNfromVert, unit_select_commands[i-1]); nargs++;
    }
    unit_select_labels[i]=XtCreateManagedWidget("unitselectlabels", 
						labelWidgetClass, 
						column, args, nargs);

    XtAddCallback(unit_select_commands[i],
		  XtNdestroyCallback,free_bitmap_destroy_callback, NULL);
    XtAddCallback(unit_select_commands[i],
                  XtNcallback, unit_select_callback, NULL);
  }

  unit_select_no=i;

  unit_select_close_command =
    I_L(XtVaCreateManagedWidget("unitselectclosecommand", 
				commandWidgetClass,
				unit_select_form,
				XtNfromVert, firstcolumn,
				NULL));

  unit_select_all_command =
    I_L(XtVaCreateManagedWidget("unitselectallcommand", 
				commandWidgetClass,
				unit_select_form,
				XtNfromVert, firstcolumn,
				NULL));

  XtAddCallback(unit_select_close_command, XtNcallback, unit_select_callback, NULL);
  XtAddCallback(unit_select_all_command, XtNcallback, unit_select_all_callback, NULL);

  xaw_set_relative_position(toplevel, unit_select_dialog_shell, 15, 10);
  XtPopup(unit_select_dialog_shell, XtGrabNone);
}

/**************************************************************************
...
**************************************************************************/
void unit_select_all_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  int i;

  XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(unit_select_dialog_shell);
  
  for(i=0; i<unit_select_no; i++) {
    struct unit *punit = player_find_unit_by_id(client.conn.playing,
						unit_select_ids[i]);
    if(punit) {
      set_unit_focus(punit);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void unit_select_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  int i;

  XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(unit_select_dialog_shell);

  for(i=0; i<unit_select_no; i++) {

    if(unit_select_commands[i]==w) {
      struct unit *punit = player_find_unit_by_id(client.conn.playing,
						  unit_select_ids[i]);
      if(punit) {
	set_unit_focus(punit);
      }
      return;
    }
  }
}


/****************************************************************
popup the dialog 5% inside the main-window 
*****************************************************************/
void popup_races_dialog(struct player *pplayer)
{
  Position x, y;
  Dimension width, height;

  XtSetSensitive(main_form, FALSE);

  if (!races_dialog_shell) {
    create_races_dialog(pplayer);
  }

  XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);

  XtTranslateCoords(toplevel, (Position) width/20, (Position) height/20,
		    &x, &y);
  XtVaSetValues(races_dialog_shell, XtNx, x, XtNy, y, NULL);

  XtPopup(races_dialog_shell, XtGrabNone);
  XtSetKeyboardFocus(toplevel, races_dialog_shell);
}

/****************************************************************
...
*****************************************************************/
void popdown_races_dialog(void)
{
  if (races_dialog_shell) {
  XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(races_dialog_shell);
  races_dialog_shell = NULL;
  } /* else there is no dialog shell to destroy */
}

/****************************************************************
...
*****************************************************************/
void create_races_dialog(struct player *pplayer)
{
  int per_row = 5;
  int i, j, len, maxracelen, index, nat_count;
  char maxracename[MAX_LEN_NAME];
  char namebuf[64];
  int space;
  XtWidgetGeometry geom;

  races_player = pplayer;
  maxracelen = 0;
  nations_iterate(pnation) {
    if (is_nation_playable(pnation)) {
      len = strlen(nation_adjective_translation(pnation));
      maxracelen = MAX(maxracelen, len);
    }
  } nations_iterate_end;
  maxracelen = MIN(maxracelen, MAX_LEN_NAME-1);
  my_snprintf(maxracename, sizeof(maxracename), "%*s", maxracelen+2, "W");

  races_dialog_shell = I_T(XtCreatePopupShell("racespopup", 
					  transientShellWidgetClass,
					  toplevel, NULL, 0));

  races_form = XtVaCreateManagedWidget("racesform", 
				       formWidgetClass, 
				       races_dialog_shell, NULL);   

  races_label = I_L(XtVaCreateManagedWidget("raceslabel", 
				       labelWidgetClass, 
				       races_form, NULL));  

  races_toggles_viewport =
    XtVaCreateManagedWidget("racestogglesviewport",
			    viewportWidgetClass,
			    races_form,
			    XtNfromVert, races_label,
			    NULL);

  races_toggles_form =
    XtVaCreateManagedWidget("racestogglesform",
			    formWidgetClass,
			    races_toggles_viewport,
			    NULL);

  free(races_toggles);
  races_toggles = fc_calloc(nation_count(), sizeof(Widget));
  free(races_toggles_to_nations);
  races_toggles_to_nations = fc_calloc(nation_count(),
				       sizeof(struct nation_type *));

  i = 0;
  j = 0;
  index = 0;
  nations_iterate(pnation) {
    if (!is_nation_playable(pnation)) {
      continue;
    }

    if (j == 0) {
      index = i * per_row;
      my_snprintf(namebuf, sizeof(namebuf), "racestoggle%d", index);
      if (i == 0) {
	races_toggles[index] =
	  XtVaCreateManagedWidget(namebuf,
				  toggleWidgetClass,
				  races_toggles_form,
				  XtNlabel, maxracename,
				  NULL);
      } else {
	races_toggles[index] =
	  XtVaCreateManagedWidget(namebuf,
				  toggleWidgetClass,
				  races_toggles_form,
				  XtNradioGroup,
				  races_toggles[index - 1],
				  XtNfromVert,
				  races_toggles[index - per_row],
				  XtNlabel, maxracename,
				  NULL);
      }
    } else {
      index = i * per_row + j;
      my_snprintf(namebuf, sizeof(namebuf), "racestoggle%d", index);
      if (i == 0) {
	races_toggles[index] =
	  XtVaCreateManagedWidget(namebuf,
				  toggleWidgetClass,
				  races_toggles_form,
				  XtNradioGroup,
				  races_toggles[index - 1],
				  XtNfromHoriz,
				  races_toggles[index - 1],
				  XtNlabel, maxracename,
				  NULL);
      } else {
	races_toggles[index] =
	  XtVaCreateManagedWidget(namebuf,
				  toggleWidgetClass,
				  races_toggles_form,
				  XtNradioGroup,
				  races_toggles[index - 1],
				  XtNfromVert,
				  races_toggles[index - per_row],
				  XtNfromHoriz,
				  races_toggles[index - 1],
				  XtNlabel, maxracename,
				  NULL);
      }
    }

    races_toggles_to_nations[index] = pnation;

    j++;
    if (j >= per_row) {
      j = 0;
      i++;
    }
  } nations_iterate_end;
  nat_count = index + 1;

  races_leader_form =
    XtVaCreateManagedWidget("racesleaderform",
			    formWidgetClass,
			    races_form,
			    XtNfromVert, races_toggles_viewport,
/*			    XtNfromHoriz, races_toggles_viewport,*/
			    NULL);

  XtVaGetValues(races_leader_form, XtNdefaultDistance, &space, NULL);
  XtQueryGeometry(races_toggles[0], NULL, &geom);
  races_leader =
    XtVaCreateManagedWidget("racesleader",
			    asciiTextWidgetClass,
			    races_leader_form,
			    XtNeditType, XawtextEdit,
			    XtNwidth,
			      space + 2*(geom.width + geom.border_width),
			    XtNstring, "",
			    NULL);

  races_leader_pick_popupmenu = 0;

  races_leader_pick_menubutton =
    I_L(XtVaCreateManagedWidget("racesleaderpickmenubutton",
				menuButtonWidgetClass,
				races_leader_form,
/*				XtNfromVert, races_leader,*/
				XtNfromHoriz, races_leader,
				NULL));

  races_sex_label = I_L(XtVaCreateManagedWidget("racessexlabel",
				            labelWidgetClass,
				            races_form,
					    XtNfromVert, races_leader_form,
					    NULL));

  races_sex_form = XtVaCreateManagedWidget("racessexform",
					   formWidgetClass,
					   races_form,
					   XtNfromVert, races_sex_label,
					   NULL);

  races_sex_toggles[0] =
    I_L(XtVaCreateManagedWidget("racessextoggle0",
				toggleWidgetClass,
				races_sex_form,
				NULL));

  races_sex_toggles[1] =
    I_L(XtVaCreateManagedWidget("racessextoggle1",
				toggleWidgetClass,
				races_sex_form,
				XtNfromHoriz,
				(XtArgVal)races_sex_toggles[0],
				XtNradioGroup,
				races_sex_toggles[0],
				NULL));

  /* find out styles that can be used at the game beginning */
  /* Limit of 64 city_styles should be deleted. -ev */
  for (i = 0, b_s_num = 0; i < game.control.styles_count && i < 64; i++) {
    if (!city_style_has_requirements(&city_styles[i])) {
      city_style_idx[b_s_num] = i;
      city_style_ridx[i] = b_s_num;
      b_s_num++;
    }
  }

  races_style_label =
    I_L(XtVaCreateManagedWidget("racesstylelabel", 
				labelWidgetClass, 
				races_form,
				XtNfromVert, races_sex_form,
/*				XtNfromHoriz, races_toggles_viewport,*/
				NULL));  

  races_style_form =
    XtVaCreateManagedWidget("racesstyleform", 
			    formWidgetClass, 
			    races_form, 
			    XtNfromVert, races_style_label,
/*			    XtNfromHoriz, races_toggles_viewport,*/
			    NULL);   

  free(races_style_toggles);
  races_style_toggles = fc_calloc(b_s_num,sizeof(Widget));

  for( i = 0; i < ((b_s_num-1)/per_row)+1; i++) {
    index = i * per_row;
    my_snprintf(namebuf, sizeof(namebuf), "racesstyle%d", index);
    if( i == 0 ) {
      races_style_toggles[index] =
	XtVaCreateManagedWidget(namebuf,
				toggleWidgetClass,
				races_style_form,
				XtNlabel, maxracename,
				NULL);
    } else {
      races_style_toggles[index] =
	XtVaCreateManagedWidget(namebuf,
				toggleWidgetClass,
				races_style_form,
				XtNradioGroup,
				races_style_toggles[index-1],
				XtNfromVert,
				races_style_toggles[index-per_row],
				XtNlabel, maxracename,
				NULL);
    }

    for( j = 1; j < per_row; j++) {
      index = i * per_row + j;
      if( index >= b_s_num ) break;
      my_snprintf(namebuf, sizeof(namebuf), "racesstyle%d", index);
      if( i == 0 ) {
	races_style_toggles[index] =
	  XtVaCreateManagedWidget(namebuf,
				  toggleWidgetClass,
				  races_style_form,
				  XtNradioGroup,
				  races_style_toggles[index-1],
				  XtNfromHoriz,
				  races_style_toggles[index-1],
				  XtNlabel, maxracename,
				  NULL);
      } else {
	races_style_toggles[index] =
	  XtVaCreateManagedWidget(namebuf,
				  toggleWidgetClass,
				  races_style_form,
				  XtNradioGroup,
				  races_style_toggles[index-1],
				  XtNfromVert,
				  races_style_toggles[index-per_row],
				  XtNfromHoriz,
				  races_style_toggles[index-1],
				  XtNlabel, maxracename,
				  NULL);
      }
    }
  }

  races_action_form = XtVaCreateManagedWidget("racesactionform",
					      formWidgetClass,
					      races_form,
					      XtNfromVert, races_style_form,
					      NULL);

  races_ok_command =
    I_L(XtVaCreateManagedWidget("racesokcommand",
				commandWidgetClass,
				races_action_form,
				NULL));

  races_random_command =
    I_L(XtVaCreateManagedWidget("racesdisconnectcommand",
				commandWidgetClass,
				races_action_form,
				XtNfromHoriz, races_ok_command,
				NULL));

  races_quit_command =
    I_L(XtVaCreateManagedWidget("racesquitcommand",
				commandWidgetClass,
				races_action_form,
				XtNfromHoriz, races_random_command,
				NULL));

  XtAddCallback(races_random_command, XtNcallback,
		races_random_command_callback, NULL);
  XtAddCallback(races_quit_command, XtNcallback,
		races_quit_command_callback, NULL);


  for (i = 0; i < nation_count(); i++) {
    if (races_toggles[i]) {
      XtAddCallback(races_toggles[i], XtNcallback,
		    races_toggles_callback, INT_TO_XTPOINTER(i));
    }
  }


  XtAddCallback(races_ok_command, XtNcallback,
		races_ok_command_callback, NULL);


  XtSetKeyboardFocus(races_form, races_leader);

  XtRealizeWidget(races_dialog_shell);

/*  for(i=0; i<game.control.playable_nation_count; i++) {
    races_toggles_to_nations[i] = i;
  }
*/
  qsort(races_toggles_to_nations, nat_count,
	sizeof(struct nation_type *), races_indirect_compare);

  /* Build nation_to_race_toggle */
  free(nation_idx_to_race_toggle);
  nation_idx_to_race_toggle =
      fc_calloc(nation_count(), sizeof(int));
  for (i = 0; i < nation_count(); i++) {
    nation_idx_to_race_toggle[i] = -1;
  }
  for (i = 0; i < nation_count(); i++) {
    if (races_toggles_to_nations[i]) {
      nation_idx_to_race_toggle[nation_index(races_toggles_to_nations[i])] = i;
    }
  }

  for (i = 0; i < nation_count(); i++) {
    if (races_toggles[i]) {
      XtVaSetValues(races_toggles[i],
		    XtNlabel,
		      (XtArgVal)nation_adjective_translation(races_toggles_to_nations[i]),
		    NULL);
    }
  }

  for(i=0; i<b_s_num; i++) {
    XtVaSetValues(races_style_toggles[i], XtNlabel,
		  (XtArgVal)city_style_name_translation(city_style_idx[i]), NULL);
  }

  select_random_race();
}

/****************************************************************
...
*****************************************************************/
void racesdlg_key_ok(Widget w)
{
  Widget ok = XtNameToWidget(XtParent(XtParent(w)), "*racesokcommand");
  if (ok)
    x_simulate_button_click(ok);
}

/**************************************************************************
...
**************************************************************************/
void races_toggles_set_sensitive(void)
{
  int i;

  if (!races_dialog_shell) {
    return;
  }
  for (i = 0; i < nation_count(); i++) {
    if (nation_idx_to_race_toggle[i] > -1) {
      XtSetSensitive(races_toggles[nation_idx_to_race_toggle[i]], TRUE);
    }
  }

  nations_iterate(nation) {
    int selected_nation = -1;
    int this_index = nation_index(nation);

    if (!is_nation_playable(nation)) {
      continue;
    }

    if (nation->is_available && !nation->player) {
      continue;
    }

    if (races_buttons_get_current() != -1) {
      selected_nation =
        nation_index(races_toggles_to_nations[races_buttons_get_current()]);
    }

    freelog(LOG_DEBUG, "  [%d]: %d = %s",
	    selected_nation,
	    nation_number(nation),
	    nation_rule_name(nation));

    if (this_index == selected_nation) {
      XawToggleUnsetCurrent(races_toggles[0]);
      XtSetSensitive(races_toggles[nation_idx_to_race_toggle[this_index]], FALSE);
      select_random_race();
    } else {
      XtSetSensitive(races_toggles[nation_idx_to_race_toggle[this_index]], FALSE);
    }
  } nations_iterate_end;
}

/* We store this value locally in case it changes globally. */
static int local_nation_count;

/**************************************************************************
...
**************************************************************************/
void races_toggles_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  int index = XTPOINTER_TO_INT(client_data);
  struct nation_type *race = races_toggles_to_nations[index];
  int j;
  int leader_count;
  struct nation_leader *leaders = get_nation_leaders(race, &leader_count);
  Widget entry;

  if(races_leader_pick_popupmenu)
    XtDestroyWidget(races_leader_pick_popupmenu);

  races_leader_pick_popupmenu =
    XtVaCreatePopupShell("menu",
			 simpleMenuWidgetClass,
			 races_leader_pick_menubutton,
			 NULL);

  local_nation_count = nation_count();

  for(j=0; j<leader_count; j++) {
    entry =
      XtVaCreateManagedWidget(leaders[j].name,
			      smeBSBObjectClass,
			      races_leader_pick_popupmenu,
			      NULL);
    XtAddCallback(entry, XtNcallback, races_leader_pick_callback,
		  INT_TO_XTPOINTER(local_nation_count * j + nation_index(race)));
  }

  races_leader_set_values(race, myrand(leader_count));

  x_simulate_button_click
  (
   races_style_toggles[city_style_ridx[city_style_of_nation(race)]]
  );
}

/**************************************************************************
...
**************************************************************************/
void races_leader_pick_callback(Widget w, XtPointer client_data,
				XtPointer call_data)
{
  int lead = XTPOINTER_TO_INT(client_data) / local_nation_count;
  int race = XTPOINTER_TO_INT(client_data) - (local_nation_count * lead);

  races_leader_set_values(nation_by_number(race), lead);
}

/**************************************************************************
...
**************************************************************************/
void races_leader_set_values(struct nation_type *race, int lead)
{
  int leader_count;
  struct nation_leader *leaders = get_nation_leaders(race, &leader_count);

  XtVaSetValues(races_leader, XtNstring, leaders[lead].name, NULL);
  XawTextSetInsertionPoint(races_leader, strlen(leaders[lead].name));

  races_sex_buttons_set_current(!leaders[lead].is_male);
}

/**************************************************************************
...
**************************************************************************/
int races_buttons_get_current(void)
{
  int i;
  XtPointer dp, yadp;

  if (nation_count() == 1) {
    return 0;
  }

  if(!(dp=XawToggleGetCurrent(races_toggles[0])))
    return -1;

  for(i = 0; i < nation_count(); i++) {
    if (races_toggles[i]) {
      XtVaGetValues(races_toggles[i], XtNradioData, &yadp, NULL);
      if(dp==yadp)
	return i;
    }
  }

  return -1;
}

/**************************************************************************
...
**************************************************************************/
int races_sex_buttons_get_current(void)
{
  int i;
  XtPointer dp, yadp;

  if(!(dp=XawToggleGetCurrent(races_sex_toggles[0])))
    return -1;

  for(i=0; i<2; i++) {
    XtVaGetValues(races_sex_toggles[i], XtNradioData, &yadp, NULL);
    if(dp==yadp)
      return i;
  }

  return -1;
}

/**************************************************************************
...
**************************************************************************/
int races_style_buttons_get_current(void)
{
  int i;
  XtPointer dp, yadp;

  if(b_s_num==1)
    return 0;

  if(!(dp=XawToggleGetCurrent(races_style_toggles[0])))
    return -1;

  for(i=0; i<b_s_num; i++) {
    XtVaGetValues(races_style_toggles[i], XtNradioData, &yadp, NULL);
    if(dp==yadp)
      return i;
  }

  return -1;
}

/**************************************************************************
...
**************************************************************************/
void races_sex_buttons_set_current(int i)
{
  XtPointer dp;

  XtVaGetValues(races_sex_toggles[i], XtNradioData, &dp, NULL);

  XawToggleSetCurrent(races_sex_toggles[0], dp);
}

/**************************************************************************
...
**************************************************************************/
int races_indirect_compare(const void *first, const void *second)
{
  const char *first_string;
  const char *second_string;

  struct nation_type *first_nation = *(struct nation_type **)first;
  struct nation_type *second_nation = *(struct nation_type **)second;

  first_string = nation_adjective_translation(first_nation);
  second_string = nation_adjective_translation(second_nation);

  return mystrcasecmp(first_string, second_string);
}

/**************************************************************************
...
**************************************************************************/
void races_ok_command_callback(Widget w, XtPointer client_data, 
			       XtPointer call_data)
{
  int selected_index, selected_sex, selected_style;
  XtPointer dp;

  if((selected_index=races_buttons_get_current())==-1) {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("You must select a nation."));
    return;
  }

  if((selected_sex=races_sex_buttons_get_current())==-1) {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("You must select your sex."));
    return;
  }

  if((selected_style=races_style_buttons_get_current())==-1) {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("You must select your city style."));
    return;
  }

  XtVaGetValues(races_leader, XtNstring, &dp, NULL);

  /* perform a minimum of sanity test on the name */
  if (strlen(dp) == 0) {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("You must type a legal name."));
    return;
  }

  dsend_packet_nation_select_req(&client.conn,
				 player_number(races_player),
				 nation_index(races_toggles_to_nations[selected_index]),
				 selected_sex ? FALSE : TRUE,
				 dp, city_style_idx[selected_style]);
  popdown_races_dialog();
}

/**************************************************************************
...
**************************************************************************/
void races_random_command_callback(Widget w, XtPointer client_data, 
				       XtPointer call_data)
{
  popdown_races_dialog();
/*  disconnect_from_server();*/
}

/**************************************************************************
...
**************************************************************************/
void races_quit_command_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data)
{
  client_exit();
}

/**************************************************************************
  Frees a bitmap associated with a Widget when it is destroyed
**************************************************************************/
void free_bitmap_destroy_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  Pixmap pm;

  XtVaGetValues(w,XtNbitmap,&pm,NULL);
  if(pm) XFreePixmap(XtDisplay(w),pm);
}

/**************************************************************************
  Destroys its widget.  Usefull for a popdown callback on pop-ups that
  won't get resused.
**************************************************************************/
void destroy_me_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data)
{
  XtDestroyWidget(w);
}

/**************************************************************************
  Adjust tax rates from main window
**************************************************************************/
void taxrates_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  common_taxrates_callback((size_t) client_data);
}

/**************************************************************************
  Ruleset (modpack) has suggested loading certain tileset. Confirm from
  user and load.
**************************************************************************/
void popup_tileset_suggestion_dialog(void)
{
}

/**************************************************************************
  Tileset (modpack) has suggested loading certain theme. Confirm from
  user and load.
**************************************************************************/
bool popup_theme_suggestion_dialog(const char *theme_name)
{
  /* Don't load */
  return FALSE;
}

/********************************************************************** 
  This function is called when the client disconnects or the game is
  over.  It should close all dialog windows for that game.
***********************************************************************/
void popdown_all_game_dialogs(void)
{
  popdown_city_report_dialog();
  popdown_meswin_dialog();
  popdown_science_dialog();
  popdown_economy_report_dialog();
  popdown_activeunits_report_dialog();
  popdown_players_dialog();
  popdown_notify_dialog();
}
