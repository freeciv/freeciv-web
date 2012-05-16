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
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/AsciiText.h>  
#include <X11/Xaw/Toggle.h>     
#include <X11/IntrinsicP.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "genlist.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "connection.h"	/* can_conn_edit */
#include "game.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "specialist.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "control.h"	/* request_xxx and set_unit_focus */
#include "options.h"
#include "text.h"
#include "tilespec.h"

#include "cma_fec.h"

#include "canvas.h"
#include "cityrep.h"
#include "cma_fe.h"
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "optiondlg.h"	/* for toggle_callback */
#include "pixcomm.h"
#include "repodlgs.h"
#include "wldlg.h"

#include "citydlg_common.h"
#include "citydlg.h"

#define MIN_NUM_CITIZENS	22
#define MAX_NUM_CITIZENS	50
#define DEFAULT_NUM_CITIZENS	38
#define MIN_NUM_UNITS		8
#define MAX_NUM_UNITS		20
#define DEFAULT_NUM_UNITS	11

struct city_dialog {
  struct city *pcity;

  int num_citizens_shown;
  int num_units_shown;

  Widget shell;
  Widget main_form;
  Widget left_form;
  Widget cityname_label;
  Widget *citizen_labels;
  Widget production_label;
  Widget output_label;
  Widget storage_label;
  Widget pollution_label;
  Widget sub_form;
  Widget map_canvas;
  Widget sell_command;
  Widget close_command, rename_command, trade_command, activate_command;
  Widget show_units_command, cityopt_command, cma_command;
  Widget building_label, progress_label, buy_command, change_command,
    worklist_command, worklist_label;
  Widget improvement_viewport, improvement_list;
  Widget support_unit_label;
  Widget *support_unit_pixcomms;
  Widget support_unit_next_command;
  Widget support_unit_prev_command;
  Widget present_unit_label;
  Widget *present_unit_pixcomms;
  Widget present_unit_next_command;
  Widget present_unit_prev_command;
  Widget change_list;
  Widget rename_input;
  Widget worklist_shell;
  
  Impr_type_id sell_id;
  
  int support_unit_base;
  int present_unit_base;
  char improvlist_names[B_LAST+1][64];
  char *improvlist_names_ptrs[B_LAST+1];
  
  char *change_list_names_ptrs[B_LAST+1+U_LAST+1+1];
  char change_list_names[B_LAST+1+U_LAST+1][200];
  int change_list_ids[B_LAST+1+U_LAST+1];
  int change_list_num_improvements;

  /*int is_modal;*/
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct city_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct city_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list = NULL;
static bool dialog_list_has_been_initialised = FALSE;

static struct city_dialog *get_city_dialog(struct city *pcity);
static struct city_dialog *create_city_dialog(struct city *pcity);
static void close_city_dialog(struct city_dialog *pdialog);

static void city_dialog_update_improvement_list(struct city_dialog *pdialog);
static void city_dialog_update_title(struct city_dialog *pdialog);
static void city_dialog_update_supported_units(struct city_dialog *pdialog, int id);
static void city_dialog_update_present_units(struct city_dialog *pdialog, int id);
static void city_dialog_update_citizens(struct city_dialog *pdialog);
static void city_dialog_update_production(struct city_dialog *pdialog);
static void city_dialog_update_output(struct city_dialog *pdialog);
static void city_dialog_update_building(struct city_dialog *pdialog);
static void city_dialog_update_storage(struct city_dialog *pdialog);
static void city_dialog_update_pollution(struct city_dialog *pdialog);

static void sell_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void buy_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void change_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void worklist_callback(Widget w, XtPointer client_data, XtPointer call_data);
void commit_city_worklist(struct worklist *pwl, void *data);
void cancel_city_worklist(void *data);
static void close_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void rename_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void trade_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void activate_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void show_units_callback(Widget W, XtPointer client_data, XtPointer call_data);
static void units_next_prev_callback(Widget W, XtPointer client_data,
				     XtPointer call_data);
static void unitupgrade_callback_yes(Widget w, XtPointer client_data,
				     XtPointer call_data);
static void unitupgrade_callback_no(Widget w, XtPointer client_data,
				    XtPointer call_data);
static void upgrade_callback(Widget w, XtPointer client_data, XtPointer call_data);

static void present_units_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
static void cityopt_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data);
static void cma_callback(Widget w, XtPointer client_data,
                         XtPointer call_data);
static void popdown_cityopt_dialog(void);

/****************************************************************
...
*****************************************************************/
static void get_contents_of_pollution(struct city_dialog *pdialog,
				      char *retbuf, int n)
{
  struct city *pcity;
  int pollution=0;

  if (pdialog) {
    pcity=pdialog->pcity;
    pollution=pcity->pollution;
  }

  my_snprintf(retbuf, n, _("Pollution:    %3d"), pollution);
}

/****************************************************************
...
*****************************************************************/
static void get_contents_of_storage(struct city_dialog *pdialog,
				    char *retbuf, int n)
{
  struct city *pcity;
  int foodstock=0;
  int foodbox=0;

  if (pdialog) {
    pcity=pdialog->pcity;
    foodstock=pcity->food_stock;
    foodbox=city_granary_size(pcity->size);
  }

  /* We used to mark cities with a granary with a "*" here. */
  my_snprintf(retbuf, n, _("Granary:  %3d/%-3d"),
	      foodstock, foodbox);
}

/****************************************************************
...
*****************************************************************/
static void get_contents_of_production(struct city_dialog *pdialog,
				       char *retbuf, int n)
{
  struct city *pcity;
  int foodprod=0;
  int foodsurplus=0;
  int shieldprod=0;
  int shieldsurplus=0;
  int tradeprod=0;
  int tradesurplus=0;

  if (pdialog) {
    pcity=pdialog->pcity;
    foodprod=pcity->prod[O_FOOD];
    foodsurplus = pcity->surplus[O_FOOD];
    shieldprod=pcity->prod[O_SHIELD] + pcity->waste[O_SHIELD];
    shieldsurplus = pcity->surplus[O_SHIELD];
    tradeprod = pcity->surplus[O_TRADE] + pcity->waste[O_TRADE];
    tradesurplus = pcity->surplus[O_TRADE];
  }

  my_snprintf(retbuf, n,
	  _("Food:  %3d (%+-4d)\n"
	    "Prod:  %3d (%+-4d)\n"
	    "Trade: %3d (%+-4d)"),
	  foodprod, foodsurplus,
	  shieldprod, shieldsurplus,
	  tradeprod, tradesurplus);
}

/****************************************************************
...
*****************************************************************/
static void get_contents_of_output(struct city_dialog *pdialog,
				   char *retbuf, int n)
{
  struct city *pcity;
  int goldtotal=0;
  int goldsurplus=0;
  int luxtotal=0;
  int scitotal=0;

  if (pdialog) {
    pcity=pdialog->pcity;
    goldtotal=pcity->prod[O_GOLD];
    goldsurplus = pcity->surplus[O_GOLD];
    luxtotal=pcity->prod[O_LUXURY];
    scitotal=pcity->prod[O_SCIENCE];
  }

  my_snprintf(retbuf, n, 
	  _("Gold:  %3d (%+-4d)\n"
	    "Lux:   %3d\n"
	    "Sci:   %3d"),
	  goldtotal, goldsurplus,
	  luxtotal,
	  scitotal);
}

/****************************************************************
...
*****************************************************************/
static void get_contents_of_progress(struct city_dialog *pdialog,
				     char *retbuf, int n)
{
  get_city_dialog_production(pdialog ? pdialog->pcity : NULL, retbuf, n);
}

/****************************************************************
...
*****************************************************************/
static void get_contents_of_worklist(struct city_dialog *pdialog,
				     char *retbuf, int n)
{
  struct city *pcity = pdialog ? pdialog->pcity : NULL;

  if (pcity && worklist_is_empty(&pcity->worklist)) {
    mystrlcpy(retbuf, _("(is empty)"), n);
  } else {
    mystrlcpy(retbuf, _("(in prog.)"), n);
  }
}

/****************************************************************
...
*****************************************************************/
struct city_dialog *get_city_dialog(struct city *pcity)
{
  if (!dialog_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_has_been_initialised = TRUE;
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pcity == pcity) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/****************************************************************
...
*****************************************************************/
bool city_dialog_is_open(struct city *pcity)
{
  return get_city_dialog(pcity) != NULL;
}

/****************************************************************
...
*****************************************************************/
void refresh_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;
  
  if((pdialog=get_city_dialog(pcity))) {
    struct canvas store = {XtWindow(pdialog->map_canvas)};

    city_dialog_update_improvement_list(pdialog);
    city_dialog_update_title(pdialog);
    city_dialog_update_supported_units(pdialog, 0);
    city_dialog_update_present_units(pdialog, 0);
    city_dialog_update_citizens(pdialog);
    city_dialog_redraw_map(pdialog->pcity, &store);
    city_dialog_update_production(pdialog);
    city_dialog_update_output(pdialog);
    city_dialog_update_building(pdialog);
    city_dialog_update_storage(pdialog);
    city_dialog_update_pollution(pdialog);

    XtSetSensitive(pdialog->trade_command,
    		   city_num_trade_routes(pcity)?True:False);
    XtSetSensitive(pdialog->activate_command,
		   unit_list_size(pcity->tile->units)
		   ?True:False);
    XtSetSensitive(pdialog->show_units_command,
                   unit_list_size(pcity->tile->units)
		   ?True:False);
    XtSetSensitive(pdialog->cma_command, True);
    XtSetSensitive(pdialog->cityopt_command, True);
  }

  if (NULL == client.conn.playing
      || city_owner(pcity) == client.conn.playing) {
    city_report_dialog_update_city(pcity);
    economy_report_dialog_update();
  } else {
    if (pdialog) {
      /* Set the buttons we do not want live while a Diplomat investigates */
      XtSetSensitive(pdialog->buy_command, FALSE);
      XtSetSensitive(pdialog->change_command, FALSE);
      XtSetSensitive(pdialog->worklist_command, FALSE);
      XtSetSensitive(pdialog->sell_command, FALSE);
      XtSetSensitive(pdialog->rename_command, FALSE);
      XtSetSensitive(pdialog->activate_command, FALSE);
      XtSetSensitive(pdialog->show_units_command, FALSE);
      XtSetSensitive(pdialog->cma_command, FALSE);
      XtSetSensitive(pdialog->cityopt_command, FALSE);
    }
  }
}

/**************************************************************************
  Updates supported and present units views in city dialogs for given unit
**************************************************************************/
void refresh_unit_city_dialogs(struct unit *punit)
{
  struct city *pcity_sup, *pcity_pre;
  struct city_dialog *pdialog;

  pcity_sup = player_find_city_by_id(client.conn.playing, punit->homecity);
  pcity_pre=tile_city(punit->tile);
  
  if(pcity_sup && (pdialog=get_city_dialog(pcity_sup)))
    city_dialog_update_supported_units(pdialog, 0);
  
  if(pcity_pre && (pdialog=get_city_dialog(pcity_pre)))
    city_dialog_update_present_units(pdialog, 0);
}

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;
  
  if(!(pdialog=get_city_dialog(pcity)))
    pdialog=create_city_dialog(pcity);

  xaw_set_relative_position(toplevel, pdialog->shell, 10, 10);
  XtPopup(pdialog->shell, XtGrabNone);
}

/****************************************************************
popdown the dialog 
*****************************************************************/
void popdown_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;
  
  if((pdialog=get_city_dialog(pcity)))
    close_city_dialog(pdialog);
}

/****************************************************************
popdown all dialogs
*****************************************************************/
void popdown_all_city_dialogs(void)
{
  if(!dialog_list_has_been_initialised) {
    return;
  }
  while (dialog_list_size(dialog_list) > 0) {
    close_city_dialog(dialog_list_get(dialog_list, 0));
  }
  popdown_cityopt_dialog();
  popdown_cma_dialog();
}


/****************************************************************
...
*****************************************************************/
static void city_map_canvas_expose(Widget w, XEvent *event, Region exposed, 
				   void *client_data)
{
  struct city_dialog *pdialog = client_data;
  struct canvas store = {XtWindow(pdialog->map_canvas)};
  
  city_dialog_redraw_map(pdialog->pcity, &store);
}


/****************************************************************
...
*****************************************************************/

#define LAYOUT_DEBUG 0

struct city_dialog *create_city_dialog(struct city *pcity)
{
  char *dummy_improvement_list[]={ 
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    0
  };

  int i, itemWidth;
  struct city_dialog *pdialog;
  char lblbuf[512];
  Widget first_citizen, first_support, first_present;
  XtWidgetGeometry geom;
  Dimension widthTotal;
  Dimension widthCitizen, borderCitizen, internalCitizen, spaceCitizen;
  Dimension widthUnit, borderUnit, internalUnit, spaceUnit;
  Dimension widthNext, borderNext, internalNext, spaceNext;
  Dimension widthPrev, borderPrev, internalPrev, spacePrev;
  Widget relative;
  enum citizen_category c = CITIZEN_SPECIALIST + DEFAULT_SPECIALIST;

  if (tileset_tile_height(tileset)<45) dummy_improvement_list[5]=0;

  if (concise_city_production) {
    dummy_improvement_list[0] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXX";
  }

  pdialog=fc_malloc(sizeof(struct city_dialog));
  pdialog->pcity=pcity;
  pdialog->support_unit_base=0;
  pdialog->present_unit_base=0;
  pdialog->worklist_shell = NULL;

  pdialog->shell=
    XtVaCreatePopupShell(city_name(pcity),
/*			 make_modal ? transientShellWidgetClass :*/
			 topLevelShellWidgetClass,
			 toplevel, 
			 XtNallowShellResize, True, 
			 NULL);

  pdialog->main_form=
    XtVaCreateManagedWidget("citymainform", 
			    formWidgetClass, 
			    pdialog->shell, 
			    NULL);

  pdialog->cityname_label=
    XtVaCreateManagedWidget("citynamelabel", 
			    labelWidgetClass,
			    pdialog->main_form,
			    NULL);


  first_citizen=
    XtVaCreateManagedWidget("citizenlabels",
			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, 
			    pdialog->cityname_label,
			    XtNbitmap,
			    get_citizen_pixmap(c, 0, pcity),
			    NULL);


  pdialog->sub_form=
    XtVaCreateManagedWidget("citysubform", 
			    formWidgetClass, 
			    pdialog->main_form, 
			    XtNfromVert, 
			    (XtArgVal)first_citizen,
			    NULL);


  pdialog->left_form=
    XtVaCreateManagedWidget("cityleftform", 
			    formWidgetClass, 
			    pdialog->sub_form, 
			    NULL);

  get_contents_of_production(NULL, lblbuf, sizeof(lblbuf));
  pdialog->production_label=
    XtVaCreateManagedWidget("cityprodlabel", 
			    labelWidgetClass,
			    pdialog->left_form,
			    XtNlabel, lblbuf,
			    NULL);

  get_contents_of_output(NULL, lblbuf, sizeof(lblbuf));
  pdialog->output_label=
    XtVaCreateManagedWidget("cityoutputlabel", 
			    labelWidgetClass,
			    pdialog->left_form,
			    XtNlabel, lblbuf,
			    XtNfromVert, 
			    (XtArgVal)pdialog->production_label,
			    NULL);

  get_contents_of_storage(NULL, lblbuf, sizeof(lblbuf));
  pdialog->storage_label=
    XtVaCreateManagedWidget("citystoragelabel", 
			    labelWidgetClass,
			    pdialog->left_form,
			    XtNlabel, lblbuf,
			    XtNfromVert, 
			    (XtArgVal)pdialog->output_label,
			    NULL);

  get_contents_of_pollution(NULL, lblbuf, sizeof(lblbuf));
  pdialog->pollution_label=
    XtVaCreateManagedWidget("citypollutionlabel", 
			    labelWidgetClass,
			    pdialog->left_form,
			    XtNlabel, lblbuf,
			    XtNfromVert, 
			    (XtArgVal)pdialog->storage_label,
			    NULL);


  pdialog->map_canvas=
    XtVaCreateManagedWidget("citymapcanvas", 
			    xfwfcanvasWidgetClass,
			    pdialog->sub_form,
			    "exposeProc", (XtArgVal)city_map_canvas_expose,
			    "exposeProcData", (XtArgVal)pdialog,
			    XtNfromHoriz, (XtArgVal)pdialog->left_form,
			    XtNwidth, get_citydlg_canvas_width(),
			    XtNheight, get_citydlg_canvas_height(),
			    NULL);


  pdialog->building_label=
    XtVaCreateManagedWidget("citybuildinglabel",
			    labelWidgetClass,
			    pdialog->sub_form,
			    XtNfromHoriz, 
			    (XtArgVal)pdialog->map_canvas,
			    XtNlabel,
			    concise_city_production
				? "XXXXXXXXXXXXXXXXXXXXXXXXXXXX"
				: "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
			    NULL);

  get_contents_of_progress(NULL, lblbuf, sizeof(lblbuf));
  pdialog->progress_label=
    XtVaCreateManagedWidget("cityprogresslabel",
			    labelWidgetClass,
			    pdialog->sub_form,
			    XtNfromHoriz, 
			    (XtArgVal)pdialog->map_canvas,
			    XtNfromVert, 
			    pdialog->building_label,
			    XtNlabel, lblbuf,
			    NULL);

  pdialog->buy_command=
    I_L(XtVaCreateManagedWidget("citybuycommand", 
				commandWidgetClass,
				pdialog->sub_form,
				XtNfromVert, 
				pdialog->building_label,
				XtNfromHoriz, 
				pdialog->progress_label,
				NULL));

  pdialog->change_command=
    I_L(XtVaCreateManagedWidget("citychangecommand", 
			    commandWidgetClass,
			    pdialog->sub_form,
			    XtNfromVert, 
			    pdialog->building_label,
			    XtNfromHoriz, 
			    pdialog->buy_command,
			    NULL));
 
  pdialog->improvement_viewport=
    XtVaCreateManagedWidget("cityimprovview", 
			    viewportWidgetClass,
			    pdialog->sub_form,
			    XtNfromHoriz, 
			    (XtArgVal)pdialog->map_canvas,
			    XtNfromVert, 
			    pdialog->change_command,
			    NULL);

  pdialog->improvement_list=
    XtVaCreateManagedWidget("cityimprovlist", 
			    listWidgetClass,
			    pdialog->improvement_viewport,
			    XtNforceColumns, 1,
			    XtNdefaultColumns,1, 
			    XtNlist,
			      (XtArgVal)dummy_improvement_list,
			    XtNverticalList, False,
			    NULL);

  pdialog->sell_command=
    I_L(XtVaCreateManagedWidget("citysellcommand", 
			    commandWidgetClass,
			    pdialog->sub_form,
			    XtNfromVert, 
			    pdialog->improvement_viewport,
			    XtNfromHoriz, 
			    (XtArgVal)pdialog->map_canvas,
			    NULL));

  pdialog->worklist_command=
    I_L(XtVaCreateManagedWidget("cityworklistcommand", 
			    commandWidgetClass,
			    pdialog->sub_form,
			    XtNfromVert, 
			    pdialog->improvement_viewport,
			    XtNfromHoriz, 
			    pdialog->sell_command,
			    NULL));

  get_contents_of_worklist(NULL, lblbuf, sizeof(lblbuf));
  pdialog->worklist_label=
    XtVaCreateManagedWidget("cityworklistlabel",
			    labelWidgetClass,
			    pdialog->sub_form,
			    XtNfromVert,
			    pdialog->improvement_viewport,
			    XtNfromHoriz,
			    pdialog->worklist_command,
			    XtNlabel, lblbuf,
			    NULL);


  pdialog->support_unit_label=
    I_L(XtVaCreateManagedWidget("supportunitlabel",
			    labelWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, 
			    pdialog->sub_form,
			    NULL));

  first_support=
    XtVaCreateManagedWidget("supportunitcanvas",
			    pixcommWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, pdialog->support_unit_label,
			    XtNwidth, tileset_full_tile_width(tileset),
			    XtNheight, 3 * tileset_tile_height(tileset) / 2,
			    NULL);

  pdialog->present_unit_label=
    I_L(XtVaCreateManagedWidget("presentunitlabel",
			    labelWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, 
			    first_support,
			    NULL));

  first_present=
    XtVaCreateManagedWidget("presentunitcanvas",
    			    pixcommWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, pdialog->present_unit_label,
			    XtNwidth, tileset_full_tile_width(tileset),
			    XtNheight, tileset_full_tile_height(tileset),
			    NULL);


  pdialog->support_unit_next_command=
    XtVaCreateManagedWidget("supportunitnextcommand",
			    commandWidgetClass,
			    pdialog->main_form,
			    NULL);
  pdialog->support_unit_prev_command=
    XtVaCreateManagedWidget("supportunitprevcommand",
			    commandWidgetClass,
			    pdialog->main_form,
			    NULL);

  pdialog->present_unit_next_command=
    XtVaCreateManagedWidget("presentunitnextcommand",
			    commandWidgetClass,
			    pdialog->main_form,
			    NULL);
  pdialog->present_unit_prev_command=
    XtVaCreateManagedWidget("presentunitprevcommand",
			    commandWidgetClass,
			    pdialog->main_form,
			    NULL);


  pdialog->close_command=
    I_L(XtVaCreateManagedWidget("cityclosecommand", 
			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, first_present,
			    NULL));

  pdialog->rename_command=
    I_L(XtVaCreateManagedWidget("cityrenamecommand", 
			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, first_present,
			    XtNfromHoriz, pdialog->close_command,
			    NULL));

  pdialog->trade_command=
    I_L(XtVaCreateManagedWidget("citytradecommand", 
			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, first_present,
			    XtNfromHoriz, pdialog->rename_command,
			    NULL));

  pdialog->activate_command=
    I_L(XtVaCreateManagedWidget("cityactivatecommand",
    			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, first_present,
			    XtNfromHoriz, pdialog->trade_command,
			    NULL));

  pdialog->show_units_command=
    I_L(XtVaCreateManagedWidget("cityshowunitscommand",
			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, first_present,
			    XtNfromHoriz, pdialog->activate_command,
			    NULL));

  pdialog->cma_command =
    I_L(XtVaCreateManagedWidget("cmacommand",
                            commandWidgetClass,
                            pdialog->main_form,
                            XtNfromVert, first_present,
                            XtNfromHoriz, pdialog->show_units_command,
                            NULL));

  pdialog->cityopt_command=
    I_L(XtVaCreateManagedWidget("cityoptionscommand",
			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, first_present,
			    XtNfromHoriz, pdialog->cma_command,
			    NULL));


  XtRealizeWidget(pdialog->shell);
  XtQueryGeometry (pdialog->sub_form, NULL, &geom);
  widthTotal=geom.width;
  if (widthTotal>0) {
    XtQueryGeometry (first_citizen, NULL, &geom);
    widthCitizen=geom.width;
    borderCitizen=geom.border_width;
    XtVaGetValues(first_citizen,
		  XtNinternalWidth, &internalCitizen,
		  XtNhorizDistance, &spaceCitizen,
		  NULL);
    XtQueryGeometry (first_support, NULL, &geom);
    widthUnit=geom.width;
    borderUnit=geom.border_width;
    XtVaGetValues(first_support,
		  XtNinternalWidth, &internalUnit,
		  XtNhorizDistance, &spaceUnit,
		  NULL);
    XtQueryGeometry (pdialog->support_unit_next_command, NULL, &geom);
    widthNext=geom.width;
    borderNext=geom.border_width;
    XtVaGetValues(pdialog->support_unit_next_command,
		  XtNinternalWidth, &internalNext,
		  XtNhorizDistance, &spaceNext,
		  NULL);
    XtQueryGeometry (pdialog->support_unit_prev_command, NULL, &geom);
    widthPrev=geom.width;
    borderPrev=geom.border_width;
    XtVaGetValues(pdialog->support_unit_prev_command,
		  XtNinternalWidth, &internalPrev,
		  XtNhorizDistance, &spacePrev,
		  NULL);
#if LAYOUT_DEBUG >= 3
    printf
    (
     "T: w: %d\n"
     "C: wbis: %d %d %d %d\n"
     "U: wbis: %d %d %d %d\n"
     "N: wbis: %d %d %d %d\n"
     "P: wbis: %d %d %d %d\n"
     ,
     widthTotal,
     widthCitizen, borderCitizen, internalCitizen, spaceCitizen,
     widthUnit, borderUnit, internalUnit, spaceUnit,
     widthNext, borderNext, internalNext, spaceNext,
     widthPrev, borderPrev, internalPrev, spacePrev
    );
#endif
    itemWidth=widthCitizen+2*borderCitizen+2*internalCitizen+spaceCitizen;
    if (itemWidth>0) {
      pdialog->num_citizens_shown=widthTotal/itemWidth;
      if (pdialog->num_citizens_shown<MIN_NUM_CITIZENS)
	pdialog->num_citizens_shown=MIN_NUM_CITIZENS;
      else if (pdialog->num_citizens_shown>MAX_NUM_CITIZENS)
	pdialog->num_citizens_shown=MAX_NUM_CITIZENS;
    } else {
      pdialog->num_citizens_shown=MIN_NUM_CITIZENS;
    }
#if LAYOUT_DEBUG >= 2
    printf
    (
     "C: wT iW nC: %d %d %d\n"
     ,
     widthTotal,
     itemWidth,
     pdialog->num_citizens_shown
    );
#endif
    if (widthNext<widthPrev) widthNext=widthPrev;
    if (borderNext<borderPrev) borderNext=borderPrev;
    if (internalNext<internalPrev) internalNext=internalPrev;
    if (spaceNext<spacePrev) spaceNext=spacePrev;
    widthTotal-=(widthNext+2*borderNext+2*internalNext+spaceNext);
    itemWidth=widthUnit+2*borderUnit+2*internalUnit+spaceUnit;
    if (itemWidth>0) {
      pdialog->num_units_shown=widthTotal/itemWidth;
      if (pdialog->num_units_shown<MIN_NUM_UNITS)
	pdialog->num_units_shown=MIN_NUM_UNITS;
      else if (pdialog->num_units_shown>MAX_NUM_UNITS)
	pdialog->num_units_shown=MAX_NUM_UNITS;
    } else {
      pdialog->num_units_shown=MIN_NUM_UNITS;
    }
#if LAYOUT_DEBUG >= 2
    printf
    (
     "U: wT iW nU: %d %d %d\n"
     ,
     widthTotal,
     itemWidth,
     pdialog->num_units_shown
    );
#endif
  } else {
    pdialog->num_citizens_shown=DEFAULT_NUM_CITIZENS;
    pdialog->num_units_shown=DEFAULT_NUM_UNITS;
    if (tileset_tile_height(tileset)<45) {
      pdialog->num_citizens_shown-=5;
      pdialog->num_units_shown+=3;
    }
  }
#if LAYOUT_DEBUG >= 1
    printf
    (
     "nC nU: %d %d\n"
     ,
     pdialog->num_citizens_shown, pdialog->num_units_shown
    );
#endif

  pdialog->citizen_labels=
    fc_malloc(pdialog->num_citizens_shown * sizeof(Widget));

  pdialog->support_unit_pixcomms=
    fc_malloc(pdialog->num_units_shown * sizeof(Widget));
  pdialog->present_unit_pixcomms=
    fc_malloc(pdialog->num_units_shown * sizeof(Widget));


  pdialog->citizen_labels[0]=first_citizen;
  for(i=1; i<pdialog->num_citizens_shown; i++)
    pdialog->citizen_labels[i]=
    XtVaCreateManagedWidget("citizenlabels",
			    commandWidgetClass,
			    pdialog->main_form,
			    XtNfromVert, pdialog->cityname_label,
			    XtNfromHoriz, 
			      (XtArgVal)pdialog->citizen_labels[i-1],
			    XtNbitmap,
			    get_citizen_pixmap(c, 0, pcity),
			    NULL);


  pdialog->support_unit_pixcomms[0]=first_support;
  for(i=1; i<pdialog->num_units_shown; i++) {
    pdialog->support_unit_pixcomms[i]=
      XtVaCreateManagedWidget("supportunitcanvas",
			      pixcommWidgetClass,
			      pdialog->main_form,
			      XtNfromVert, pdialog->support_unit_label,
			      XtNfromHoriz,
			        (XtArgVal)pdialog->support_unit_pixcomms[i-1],
			      XtNwidth, tileset_full_tile_width(tileset),
			      XtNheight, 3 * tileset_tile_height(tileset) / 2,
			      NULL);
  }

  relative=pdialog->support_unit_pixcomms[pdialog->num_units_shown-1];
  XtVaSetValues(pdialog->support_unit_next_command,
		XtNfromVert, pdialog->support_unit_label,
		XtNfromHoriz, (XtArgVal)relative,
		NULL);
  XtVaSetValues(pdialog->support_unit_prev_command,
		XtNfromVert, pdialog->support_unit_next_command,
		XtNfromHoriz, (XtArgVal)relative,
		NULL);

  pdialog->present_unit_pixcomms[0]=first_present;
  for(i=1; i<pdialog->num_units_shown; i++) {
    pdialog->present_unit_pixcomms[i]=
      XtVaCreateManagedWidget("presentunitcanvas",
			      pixcommWidgetClass,
			      pdialog->main_form,
			      XtNfromVert, pdialog->present_unit_label,
			      XtNfromHoriz, 
			        (XtArgVal)pdialog->support_unit_pixcomms[i-1],
			      XtNwidth, tileset_full_tile_width(tileset),
			      XtNheight, tileset_full_tile_height(tileset),
			      NULL);
  }

  relative=pdialog->present_unit_pixcomms[pdialog->num_units_shown-1];
  XtVaSetValues(pdialog->present_unit_next_command,
		XtNfromVert, pdialog->present_unit_label,
		XtNfromHoriz, (XtArgVal)relative,
		NULL);
  XtVaSetValues(pdialog->present_unit_prev_command,
		XtNfromVert, pdialog->present_unit_next_command,
		XtNfromHoriz, (XtArgVal)relative,
		NULL);

  /* FIXME: this ignores the mask. */
  XtVaSetValues(pdialog->shell, XtNiconPixmap,
		get_icon_sprite(tileset, ICON_CITYDLG), NULL);


  XtAddCallback(pdialog->sell_command, XtNcallback, sell_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->buy_command, XtNcallback, buy_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->change_command, XtNcallback, change_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->worklist_command, XtNcallback, worklist_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->close_command, XtNcallback, close_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->rename_command, XtNcallback, rename_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->trade_command, XtNcallback, trade_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->activate_command, XtNcallback, activate_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->show_units_command, XtNcallback, show_units_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->support_unit_next_command, XtNcallback,
		units_next_prev_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->support_unit_prev_command, XtNcallback,
		units_next_prev_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->present_unit_next_command, XtNcallback,
		units_next_prev_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->present_unit_prev_command, XtNcallback,
		units_next_prev_callback, (XtPointer)pdialog);

  XtAddCallback(pdialog->cityopt_command, XtNcallback, cityopt_callback,
		(XtPointer)pdialog);

  XtAddCallback(pdialog->cma_command, XtNcallback, cma_callback,
                (XtPointer)pdialog);

  dialog_list_prepend(dialog_list, pdialog);

  for(i=0; i<B_LAST+1; i++)
    pdialog->improvlist_names_ptrs[i]=0;

  XtRealizeWidget(pdialog->shell);

  refresh_city_dialog(pdialog->pcity);
/*
  if(make_modal)
    XtSetSensitive(toplevel, FALSE);
  
  pdialog->is_modal=make_modal;
*/

  XSetWMProtocols(display, XtWindow(pdialog->shell), &wm_delete_window, 1);
  XtOverrideTranslations(pdialog->shell, 
    XtParseTranslationTable ("<Message>WM_PROTOCOLS: msg-close-city()"));

  XtSetKeyboardFocus(pdialog->shell, pdialog->close_command);


  return pdialog;
}

/****************************************************************
...
*****************************************************************/
void activate_callback(Widget w, XtPointer client_data,
		       XtPointer call_data)
{
  struct city_dialog *pdialog = (struct city_dialog *)client_data;

  activate_all_units(pdialog->pcity->tile);
}


/****************************************************************
...
*****************************************************************/
void show_units_callback(Widget w, XtPointer client_data,
                        XtPointer call_data)
{
  struct city_dialog *pdialog = (struct city_dialog *)client_data;
  struct tile *ptile = pdialog->pcity->tile;

  if( unit_list_size(ptile->units) )
    popup_unit_select_dialog(ptile);
}


/****************************************************************
...
*****************************************************************/
void units_next_prev_callback(Widget w, XtPointer client_data,
			      XtPointer call_data)
{
  struct city_dialog *pdialog = (struct city_dialog *)client_data;

  if (w==pdialog->support_unit_next_command) {
    (pdialog->support_unit_base)++;
    city_dialog_update_supported_units(pdialog, 0);
  } else if (w==pdialog->support_unit_prev_command) {
    (pdialog->support_unit_base)--;
    city_dialog_update_supported_units(pdialog, 0);
  } else if (w==pdialog->present_unit_next_command) {
    (pdialog->present_unit_base)++;
    city_dialog_update_present_units(pdialog, 0);
  } else if (w==pdialog->present_unit_prev_command) {
    (pdialog->present_unit_base)--;
    city_dialog_update_present_units(pdialog, 0);
  }
}


#ifdef UNUSED
/****************************************************************
...
*****************************************************************/
static void present_units_ok_callback(Widget w, XtPointer client_data, 
				      XtPointer call_data)
{
  destroy_message_dialog(w);
}
#endif


/****************************************************************
...
*****************************************************************/
static void present_units_activate_callback(Widget w, XtPointer client_data, 
					    XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  if (NULL != punit) {
    struct city *pcity = tile_city(punit->tile);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	city_dialog_update_present_units(pdialog, 0);
      }
    }
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void supported_units_activate_callback(Widget w, XtPointer client_data, 
					      XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  if (NULL != punit) {
    struct city *pcity = tile_city(punit->tile);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	city_dialog_update_supported_units(pdialog, 0);
      }
    }
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void present_units_activate_close_callback(Widget w,
						  XtPointer client_data, 
						  XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  destroy_message_dialog(w);

  if (NULL != punit) {
    struct city *pcity = tile_city(punit->tile);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	close_city_dialog(pdialog);
      }
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void supported_units_activate_close_callback(Widget w,
						    XtPointer client_data, 
						    XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  destroy_message_dialog(w);

  if (NULL != punit) {
    struct city *pcity =
      player_find_city_by_id(client.conn.playing, punit->homecity);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
	close_city_dialog(pdialog);
      }
    }
  }
}


/****************************************************************
...
*****************************************************************/
static void present_units_sentry_callback(Widget w, XtPointer client_data, 
					   XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  if (NULL != punit) {
    request_unit_sentry(punit);
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void present_units_fortify_callback(Widget w, XtPointer client_data, 
					   XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  if (NULL != punit) {
    request_unit_fortify(punit);
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void present_units_disband_callback(Widget w, XtPointer client_data, 
					   XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  if (NULL != punit) {
    request_unit_disband(punit);
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void present_units_homecity_callback(Widget w, XtPointer client_data, 
					    XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  if (NULL != punit) {
    request_unit_change_homecity(punit);
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void present_units_cancel_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data)
{
  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
void present_units_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  Widget wd;
  struct city_dialog *pdialog;
  struct city *pcity;
  XEvent *e = (XEvent*)call_data;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);
  
  if ((NULL != punit
       || (can_conn_edit(&client.conn)
           && NULL == client.conn.playing
           && (punit = game_find_unit_by_number((size_t)client_data))))
      && (pcity = tile_city(punit->tile))
      && (pdialog = get_city_dialog(pcity))) {
    
    if(e->type==ButtonRelease && e->xbutton.button==Button2)  {
      set_unit_focus(punit);
      close_city_dialog(pdialog);
      return;
    }
    if(e->type==ButtonRelease && e->xbutton.button==Button3)  {
      set_unit_focus(punit);
      return;
    }

    wd=popup_message_dialog(pdialog->shell, 
			    "presentunitsdialog", 
			    unit_description(punit),
			    present_units_activate_callback, punit->id, 1,
			    present_units_activate_close_callback, punit->id, 1,
			    present_units_sentry_callback, punit->id, 1,
			    present_units_fortify_callback, punit->id, 1,
			    present_units_disband_callback, punit->id, 1,
			    present_units_homecity_callback, punit->id, 1,
			    upgrade_callback, punit->id, 1,
			    present_units_cancel_callback, 0, 0, 
			    NULL);

    if (punit->activity == ACTIVITY_SENTRY
	|| !can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
      XtSetSensitive(XtNameToWidget(wd, "*button2"), FALSE);
    }
    if (punit->activity == ACTIVITY_FORTIFYING
	|| !can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      XtSetSensitive(XtNameToWidget(wd, "*button3"), FALSE);
    }
    if (unit_has_type_flag(punit, F_UNDISBANDABLE)) {
      XtSetSensitive(XtNameToWidget(wd, "*button4"), FALSE);
    }
    if (punit->homecity == pcity->id) {
      XtSetSensitive(XtNameToWidget(wd, "*button5"), FALSE);
    }
    if (NULL == client.conn.playing
	|| (NULL == can_upgrade_unittype(client.conn.playing, unit_type(punit)))) {
      XtSetSensitive(XtNameToWidget(wd, "*button6"), FALSE);
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void rename_city_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data)
{
  struct city_dialog *pdialog = client_data;

  if (pdialog) {
    city_rename(pdialog->pcity, input_dialog_get_input(w));
  }
  input_dialog_destroy(w);
}





/****************************************************************
...
*****************************************************************/
void rename_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct city_dialog *pdialog;

  pdialog=(struct city_dialog *)client_data;
  
  input_dialog_create(pdialog->shell, 
		      "shellrenamecity", 
		      _("What should we rename the city to?"),
		      city_name(pdialog->pcity),
		      rename_city_callback, (XtPointer)pdialog,
		      rename_city_callback, (XtPointer)0);
}

/****************************************************************
...
*****************************************************************/
static void trade_message_dialog_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data)
{
  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
void trade_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  int i;
  int x=0,total=0;
  char buf[512], *bptr=buf;
  int nleft = sizeof(buf);
  struct city_dialog *pdialog;

  pdialog=(struct city_dialog *)client_data;

  my_snprintf(buf, sizeof(buf),
	      _("These trade routes have been established with %s:\n"),
	      city_name(pdialog->pcity));
  bptr = end_of_strn(bptr, &nleft);
  
  for (i = 0; i < NUM_TRADEROUTES; i++)
    if(pdialog->pcity->trade[i]) {
      struct city *pcity;
      x=1;
      total+=pdialog->pcity->trade_value[i];
      if((pcity=game_find_city_by_number(pdialog->pcity->trade[i]))) {
	my_snprintf(bptr, nleft, _("%32s: %2d Trade/Year\n"),
		    city_name(pcity), pdialog->pcity->trade_value[i]);
	bptr = end_of_strn(bptr, &nleft);
      } else {	
	my_snprintf(bptr, nleft, _("%32s: %2d Trade/Year\n"), _("Unknown"),
		    pdialog->pcity->trade_value[i]);
	bptr = end_of_strn(bptr, &nleft);
      }
    }
  if (!x) {
    mystrlcpy(bptr, _("No trade routes exist.\n"), nleft);
  } else {
    my_snprintf(bptr, nleft, _("\nTotal trade %d Trade/Year\n"), total);
  }
  
  popup_message_dialog(pdialog->shell, 
		       "citytradedialog", 
		       buf, 
		       trade_message_dialog_callback, 0, 0,
		       NULL);
}


/****************************************************************
...
*****************************************************************/
void city_dialog_update_pollution(struct city_dialog *pdialog)
{
  char buf[512];

  get_contents_of_pollution(pdialog, buf, sizeof(buf));
  xaw_set_label(pdialog->pollution_label, buf);
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_storage(struct city_dialog *pdialog)
{
  char buf[512];

  get_contents_of_storage(pdialog, buf, sizeof(buf));
  xaw_set_label(pdialog->storage_label, buf);
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_building(struct city_dialog *pdialog)
{
  char buf[32];
  struct city *pcity=pdialog->pcity;

  XtSetSensitive(pdialog->buy_command, city_can_buy(pcity));
  XtSetSensitive(pdialog->sell_command, !pcity->did_sell);

  xaw_set_label(pdialog->building_label,
		city_production_name_translation(pcity));

  get_contents_of_progress(pdialog, buf, sizeof(buf));
  xaw_set_label(pdialog->progress_label, buf);

  get_contents_of_worklist(pdialog, buf, sizeof(buf));
  xaw_set_label(pdialog->worklist_label, buf);
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_production(struct city_dialog *pdialog)
{
  char buf[512];

  get_contents_of_production(pdialog, buf, sizeof(buf));
  xaw_set_label(pdialog->production_label, buf);
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_output(struct city_dialog *pdialog)
{
  char buf[512];

  get_contents_of_output(pdialog, buf, sizeof(buf));
  xaw_set_label(pdialog->output_label, buf);
}

/****************************************************************************
  Handle the callback when a citizen sprite widget is clicked.
****************************************************************************/
static void citizen_callback(Widget w, XtPointer client_data,
			     XtPointer call_data)
{
  struct city_dialog *pdialog = client_data;
  int i;

  /* HACK: figure out which figure was clicked. */
  for (i = 0; i < pdialog->pcity->size; i++) {
    if (pdialog->citizen_labels[i] == w) {
      break;
    }
  }
  assert(i < pdialog->pcity->size);

  city_rotate_specialist(pdialog->pcity, i);
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_citizens(struct city_dialog *pdialog)
{
  enum citizen_category citizens[MAX_CITY_SIZE];
  int i;
  struct city *pcity=pdialog->pcity;
  int num_citizens = get_city_citizen_types(pcity, FEELING_FINAL, citizens);

  for (i = 0; i < num_citizens && i < pdialog->num_citizens_shown; i++) {
    xaw_set_bitmap(pdialog->citizen_labels[i],
		   get_citizen_pixmap(citizens[i], i, pcity));

    /* HACK: set sensitivity/callbacks on the widget */
    XtRemoveAllCallbacks(pdialog->citizen_labels[i], XtNcallback);
    XtAddCallback(pdialog->citizen_labels[i], XtNcallback,
		  citizen_callback, (XtPointer)pdialog);
    XtSetSensitive(pdialog->citizen_labels[i], TRUE);
  }

  if (i >= pdialog->num_citizens_shown && i < num_citizens) {
    i = pdialog->num_citizens_shown - 1;
    /* FIXME: what about the mask? */
    xaw_set_bitmap(pdialog->citizen_labels[i],
		   get_arrow_sprite(tileset, ARROW_RIGHT)->pixmap);
    XtSetSensitive(pdialog->citizen_labels[i], FALSE);
    XtRemoveAllCallbacks(pdialog->citizen_labels[i], XtNcallback);
    return;
  }

  for(; i<pdialog->num_citizens_shown; i++) {
    xaw_set_bitmap(pdialog->citizen_labels[i], None);
    XtSetSensitive(pdialog->citizen_labels[i], FALSE);
    XtRemoveAllCallbacks(pdialog->citizen_labels[i], XtNcallback);
  }
}

/****************************************************************
...
*****************************************************************/
static void support_units_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data)
{
  Widget wd;
  XEvent *e = (XEvent*)call_data;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  if (NULL != punit) {
    struct city *pcity = game_find_city_by_number(punit->homecity);

    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if ( NULL != pdialog) {
	if(e->type==ButtonRelease && e->xbutton.button==Button2)  {
	  set_unit_focus(punit);
	  close_city_dialog(pdialog);
	  return;
	}
	if(e->type==ButtonRelease && e->xbutton.button==Button3)  {
	  set_unit_focus(punit);
	  return;
	}
	wd = popup_message_dialog(pdialog->shell,
			     "supportunitsdialog", 
			     unit_description(punit),
			     supported_units_activate_callback, punit->id, 1,
			     supported_units_activate_close_callback,
			                                     punit->id, 1,
			     present_units_disband_callback, punit->id, 1,
			     present_units_cancel_callback, 0, 0,
			     NULL);
        if (unit_has_type_flag(punit, F_UNDISBANDABLE)) {
          XtSetSensitive(XtNameToWidget(wd, "*button3"), FALSE);
        }
      }
    }
  }
}


/****************************************************************
...
*****************************************************************/
static int units_scroll_maintenance(int nunits, int nshow, int *base,
				    Widget next, Widget prev)
{
  int adj_base=FALSE;
  int nextra;

  nextra=nunits-nshow;
  if (nextra<0) nextra=0;

  if (*base>nextra) {
    *base=nextra;
    adj_base=TRUE;
  }
  if (*base<0) {
    *base=0;
    adj_base=TRUE;
  }

  if (nextra<=0) {
    XtUnmapWidget(next);
    XtUnmapWidget(prev);
  } else {
    XtMapWidget(next);
    XtMapWidget(prev);
    XtSetSensitive(next, *base<nextra);
    XtSetSensitive(prev, *base>0);
  }

  return (adj_base);
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_supported_units(struct city_dialog *pdialog, 
					int unitid)
{
  struct unit_list *plist;
  int i, j, adj_base;
  Widget pixcomm;
  int free_unhappy = get_city_bonus(pdialog->pcity, EFT_MAKE_CONTENT_MIL);

  if (NULL != client.conn.playing
      && city_owner(pdialog->pcity) != client.conn.playing) {
    plist = pdialog->pcity->info_units_supported;
  } else {
    plist = pdialog->pcity->units_supported;
  }

  adj_base = units_scroll_maintenance(unit_list_size(plist),
                                      pdialog->num_units_shown,
                                      &(pdialog->support_unit_base),
                                      pdialog->support_unit_next_command,
                                      pdialog->support_unit_prev_command);

  i = 0; /* number of displayed units */
  j = 0; /* index into list */
  unit_list_iterate(plist, punit) {
    struct canvas store;
    int happy_cost = city_unit_unhappiness(punit, &free_unhappy);

    if (j++ < pdialog->support_unit_base) {
      continue;
    }
    if (i >= pdialog->num_units_shown) {
      break;
    }

    pixcomm=pdialog->support_unit_pixcomms[i];
    store.pixmap = XawPixcommPixmap(pixcomm);

    if(!adj_base && unitid && punit->id!=unitid)
      continue;

    XawPixcommClear(pixcomm); /* STG */
    put_unit(punit, &store, 0, 0);
    put_unit_pixmap_city_overlays(punit,
                                  XawPixcommPixmap(pixcomm),
                                  punit->upkeep, happy_cost);

    xaw_expose_now(pixcomm);

    XtRemoveAllCallbacks(pixcomm, XtNcallback);
    XtAddCallback(pixcomm, XtNcallback,
		  support_units_callback, INT_TO_XTPOINTER(punit->id));
    XtSetSensitive(pixcomm, TRUE);
    i++;
  } unit_list_iterate_end;

  /* Disable any empty slots */
  for(; i<pdialog->num_units_shown; i++) {
    XawPixcommClear(pdialog->support_unit_pixcomms[i]);
    XtSetSensitive(pdialog->support_unit_pixcomms[i], FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_present_units(struct city_dialog *pdialog, int unitid)
{
  struct unit_list *plist;
  int i, j, adj_base;
  Widget pixcomm;

  if (NULL != client.conn.playing
      && city_owner(pdialog->pcity) != client.conn.playing) {
    plist = pdialog->pcity->info_units_present;
  } else {
    plist = pdialog->pcity->tile->units;
  }

  adj_base = units_scroll_maintenance(unit_list_size(plist),
                                      pdialog->num_units_shown,
                                      &(pdialog->present_unit_base),
                                      pdialog->present_unit_next_command,
                                      pdialog->present_unit_prev_command);

  i = 0; /* number of displayed units */
  j = 0; /* index into list */
  unit_list_iterate(plist, punit) {
    struct canvas store;

    if (j++ < pdialog->present_unit_base) {
      continue;
    }
    if (i >= pdialog->num_units_shown) {
      break;
    }

    pixcomm=pdialog->present_unit_pixcomms[i];
    store.pixmap = XawPixcommPixmap(pixcomm);

    if(!adj_base && unitid && punit->id!=unitid)
      continue;

    XawPixcommClear(pixcomm); /* STG */
    put_unit(punit, &store, 0, 0);

    xaw_expose_now(pixcomm);

    XtRemoveAllCallbacks(pixcomm, XtNcallback);
    XtAddCallback(pixcomm, XtNcallback, 
		  present_units_callback, INT_TO_XTPOINTER(punit->id));
    XtSetSensitive(pixcomm, TRUE);
    i++;
  } unit_list_iterate_end;

  for(; i<pdialog->num_units_shown; i++) {
    XawPixcommClear(pdialog->present_unit_pixcomms[i]);
    XtSetSensitive(pdialog->present_unit_pixcomms[i], FALSE);
  }
}


/****************************************************************
...
*****************************************************************/
void city_dialog_update_title(struct city_dialog *pdialog)
{
  char buf[512];
  String now;
  
  my_snprintf(buf, sizeof(buf), _("%s - %s citizens  Governor: %s"),
	      city_name(pdialog->pcity),
	      population_to_text(city_population(pdialog->pcity)),
                   cmafec_get_short_descr_of_city(pdialog->pcity));

  XtVaGetValues(pdialog->cityname_label, XtNlabel, &now, NULL);
  if(strcmp(now, buf) != 0) {
    XtVaSetValues(pdialog->cityname_label, XtNlabel, (XtArgVal)buf, NULL);
    xaw_horiz_center(pdialog->cityname_label);
    XtVaSetValues(pdialog->shell, XtNtitle, (XtArgVal)city_name(pdialog->pcity), NULL);
  }
}

/****************************************************************
...
*****************************************************************/
void city_dialog_update_improvement_list(struct city_dialog *pdialog)
{
  int n = 0, flag = 0;

  city_built_iterate(pdialog->pcity, pimprove) {
    if (!pdialog->improvlist_names_ptrs[n] ||
	strcmp(pdialog->improvlist_names_ptrs[n],
	       city_improvement_name_translation(pdialog->pcity, pimprove)) != 0)
      flag = 1;
    sz_strlcpy(pdialog->improvlist_names[n],
	       city_improvement_name_translation(pdialog->pcity, pimprove));
    pdialog->improvlist_names_ptrs[n] = pdialog->improvlist_names[n];
    n++;
  } city_built_iterate_end;
  
  if(pdialog->improvlist_names_ptrs[n]!=0) {
    pdialog->improvlist_names_ptrs[n]=0;
    flag=1;
  }
  
  if(flag || n==0) {
    XawListChange(pdialog->improvement_list, pdialog->improvlist_names_ptrs, 
		  n, 0, False);  
    /* force refresh of viewport so the scrollbar is added.
     * Buggy sun athena requires this */
    XtVaSetValues(pdialog->improvement_viewport, XtNforceBars, False, NULL);
    XtVaSetValues(pdialog->improvement_viewport, XtNforceBars, True, NULL);
  }
}


/**************************************************************************
...
**************************************************************************/
void citydlg_btn_select_citymap(Widget w, XEvent *event)
{
  XButtonEvent *ev=&event->xbutton;
  struct city *pcity = NULL;

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->map_canvas == w) {
      pcity = pdialog->pcity;
      break;
    }
  } dialog_list_iterate_end;

  if (pcity) {
    if (!cma_is_city_under_agent(pcity, NULL)) {
      int xtile, ytile;

      if (canvas_to_city_pos(&xtile, &ytile, ev->x, ev->y)) {
	city_toggle_worker(pcity, xtile, ytile);
      }
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void buy_callback_yes(Widget w, XtPointer client_data,
			     XtPointer call_data)
{
  struct city_dialog *pdialog = client_data;

  city_buy_production(pdialog->pcity);

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void buy_callback_no(Widget w, XtPointer client_data,
			    XtPointer call_data)
{
  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
void buy_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  char buf[512];
  struct city_dialog *pdialog = (struct city_dialog *)client_data;;
  const char *name = city_production_name_translation(pdialog->pcity);
  int value = city_production_buy_gold_cost(pdialog->pcity);
  
  if (!can_client_issue_orders()) {
    return;
  }

  if (value <= client.conn.playing->economic.gold) {
    my_snprintf(buf, sizeof(buf),
		_("Buy %s for %d gold?\nTreasury contains %d gold."), 
		name, value, client.conn.playing->economic.gold);
    popup_message_dialog(pdialog->shell, "buydialog", buf,
			 buy_callback_yes, pdialog, 0,
			 buy_callback_no, 0, 0,
			 NULL);
  }
  else {
    my_snprintf(buf, sizeof(buf),
		_("%s costs %d gold.\nTreasury contains %d gold."), 
		name, value, client.conn.playing->economic.gold);
    popup_message_dialog(pdialog->shell, "buynodialog", buf,
			 buy_callback_no, 0, 0,
			 NULL);
  }
  
}


/****************************************************************
...
*****************************************************************/
void unitupgrade_callback_yes(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)client_data);

  /* Is it right place for breaking? -ev */
  if (!can_client_issue_orders()) {
    return;
  }

  if (NULL != punit) {
    request_unit_upgrade(punit);
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
void unitupgrade_callback_no(Widget w, XtPointer client_data, XtPointer call_data)
{
  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
void upgrade_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  char buf[512];
  struct unit *punit = player_find_unit_by_id(client.conn.playing,
					      (size_t)client_data);

  if (!punit) {
    return;
  }

  if (get_unit_upgrade_info(buf, sizeof(buf), punit) == UR_OK) {
    popup_message_dialog(toplevel, "upgradedialog", buf,
			 unitupgrade_callback_yes,
			 INT_TO_XTPOINTER(punit->id), 0,
			 unitupgrade_callback_no, 0, 0, NULL);
  } else {
    popup_message_dialog(toplevel, "upgradenodialog", buf,
			 unitupgrade_callback_no, 0, 0,
			 NULL);
  }

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void change_to_callback(Widget w, XtPointer client_data,
			       XtPointer call_data)
{
  struct city_dialog *pdialog;
  XawListReturnStruct *ret;

  pdialog=(struct city_dialog *)client_data;

  ret=XawListShowCurrent(pdialog->change_list);

  if (ret->list_index != XAW_LIST_NONE) {
    struct universal target =
      universal_by_number((ret->list_index >= pdialog->change_list_num_improvements)
			     ? VUT_UTYPE : VUT_IMPROVEMENT,
			     pdialog->change_list_ids[ret->list_index]);

    city_change_production(pdialog->pcity, target);
  }
  
  XtDestroyWidget(XtParent(XtParent(w)));
  XtSetSensitive(pdialog->shell, TRUE);
}

/****************************************************************
...
*****************************************************************/
static void change_no_callback(Widget w, XtPointer client_data,
			       XtPointer call_data)
{
  struct city_dialog *pdialog;
  
  pdialog=(struct city_dialog *)client_data;
  
  XtDestroyWidget(XtParent(XtParent(w)));
  XtSetSensitive(pdialog->shell, TRUE);
}

/****************************************************************
...
*****************************************************************/
static void change_help_callback(Widget w, XtPointer client_data,
				 XtPointer call_data)
{
  struct city_dialog *pdialog;
  XawListReturnStruct *ret;

  pdialog=(struct city_dialog *)client_data;

  ret=XawListShowCurrent(pdialog->change_list);
  if(ret->list_index!=XAW_LIST_NONE) {
    int idx = pdialog->change_list_ids[ret->list_index];
    bool is_unit = (ret->list_index >= pdialog->change_list_num_improvements);

    if (is_unit) {
      popup_help_dialog_typed(utype_name_translation(utype_by_number(idx)), HELP_UNIT);
    } else if (is_great_wonder(improvement_by_number(idx))) {
      popup_help_dialog_typed(improvement_name_translation(improvement_by_number(idx)), HELP_WONDER);
    } else {
      popup_help_dialog_typed(improvement_name_translation(improvement_by_number(idx)), HELP_IMPROVEMENT);
    }
  }
  else
    popup_help_dialog_string(HELP_IMPROVEMENTS_ITEM);
}


/****************************************************************
...
*****************************************************************/
void change_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  static char *dummy_change_list[]={ 
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  888 turns",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    0
  };

  Widget cshell, cform, clabel, cview, button_change, button_cancel, button_help;
  Position x, y;
  Dimension width, height;
  struct city_dialog *pdialog;
  struct universal production;
  int n;
  
  pdialog=(struct city_dialog *)client_data;
  
  I_T(cshell=XtCreatePopupShell("changedialog", transientShellWidgetClass,
				pdialog->shell, NULL, 0));
  
  cform=XtVaCreateManagedWidget("dform", formWidgetClass, cshell, NULL);
  
  I_L(clabel=XtVaCreateManagedWidget("dlabel", labelWidgetClass, cform,
				     NULL));

  cview=XtVaCreateManagedWidget("dview", viewportWidgetClass,
				cform,
				XtNfromVert, 
				clabel,
				NULL);

  pdialog->change_list=XtVaCreateManagedWidget("dlist", listWidgetClass, 
					       cview, 
					       XtNforceColumns, 1,
					       XtNdefaultColumns,1, 
					       XtNlist, 
					       (XtArgVal)dummy_change_list,
					       XtNverticalList, False,
					       NULL);

  
  button_change = I_L(XtVaCreateManagedWidget("buttonchange",
					commandWidgetClass,
					cform,
					XtNfromVert, 
					cview,
					NULL));

  button_cancel = I_L(XtVaCreateManagedWidget("buttoncancel",
				    commandWidgetClass,
				    cform,
				    XtNfromVert, 
				    cview,
				    XtNfromHoriz,
				    button_change,
				    NULL));

  button_help = I_L(XtVaCreateManagedWidget("buttonhelp",
				    commandWidgetClass,
				    cform,
				    XtNfromVert, 
				    cview,
				    XtNfromHoriz,
				    button_cancel,
				    NULL));

  XtAddCallback(button_change, XtNcallback, 
		change_to_callback, (XtPointer)pdialog);
  XtAddCallback(button_cancel, XtNcallback, 
		change_no_callback, (XtPointer)pdialog);
  XtAddCallback(button_help, XtNcallback, 
		change_help_callback, (XtPointer)pdialog);

  

  XtVaGetValues(pdialog->shell, XtNwidth, &width, XtNheight, &height, NULL);
  XtTranslateCoords(pdialog->shell, (Position) width/6, (Position) height/10,
		    &x, &y);
  XtVaSetValues(cshell, XtNx, x, XtNy, y, NULL);
  
  XtPopup(cshell, XtGrabNone);
  
  XtSetSensitive(pdialog->shell, FALSE);

  n = 0;
  improvement_iterate(pimprove) {
    if(can_city_build_improvement_now(pdialog->pcity, pimprove)) {
      production.kind = VUT_IMPROVEMENT;
      production.value.building = pimprove;
      get_city_dialog_production_full(pdialog->change_list_names[n],
                                      sizeof(pdialog->change_list_names[n]),
                                      production, pdialog->pcity);
      pdialog->change_list_names_ptrs[n]=pdialog->change_list_names[n];
      pdialog->change_list_ids[n++] = improvement_number(pimprove);
    }
  } improvement_iterate_end;
  
  pdialog->change_list_num_improvements=n;


  unit_type_iterate(punittype) {
    if (can_city_build_unit_now(pdialog->pcity, punittype)) {
      production.kind = VUT_UTYPE;
      production.value.utype = punittype;
      get_city_dialog_production_full(pdialog->change_list_names[n],
                                      sizeof(pdialog->change_list_names[n]),
                                      production, pdialog->pcity);
      pdialog->change_list_names_ptrs[n]=pdialog->change_list_names[n];
      pdialog->change_list_ids[n++] = utype_number(punittype);
    }
  } unit_type_iterate_end;
  
  pdialog->change_list_names_ptrs[n]=0;

  XawListChange(pdialog->change_list, pdialog->change_list_names_ptrs, 
		0, 0, False);
  /* force refresh of viewport so the scrollbar is added.
   * Buggy sun athena requires this */
  XtVaSetValues(cview, XtNforceBars, True, NULL);
}


/****************************************************************
  Display the city's worklist.
*****************************************************************/
void worklist_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct city_dialog *pdialog;
  
  pdialog = (struct city_dialog *)client_data;

  if (pdialog->worklist_shell) {
    XtPopup(pdialog->worklist_shell, XtGrabNone);
  } else {
    pdialog->worklist_shell = 
      popup_worklist(&pdialog->pcity->worklist, pdialog->pcity,
		     pdialog->shell, (void *) pdialog, commit_city_worklist,
		     cancel_city_worklist);
  }
}

/****************************************************************
  Commit the changes to the worklist for the city.
*****************************************************************/
void commit_city_worklist(struct worklist *pwl, void *data)
{
  struct city_dialog *pdialog = data;

  city_worklist_commit(pdialog->pcity, pwl);
}

void cancel_city_worklist(void *data) {
  struct city_dialog *pdialog = (struct city_dialog *)data;
  pdialog->worklist_shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void sell_callback_yes(Widget w, XtPointer client_data,
			      XtPointer call_data)
{
  struct city_dialog *pdialog = client_data;

  city_sell_improvement(pdialog->pcity, pdialog->sell_id);

  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
static void sell_callback_no(Widget w, XtPointer client_data,
			     XtPointer call_data)
{
  destroy_message_dialog(w);
}


/****************************************************************
...
*****************************************************************/
void sell_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct city_dialog *pdialog;
  XawListReturnStruct *ret;
  
  pdialog=(struct city_dialog *)client_data;

  ret=XawListShowCurrent(pdialog->improvement_list);

  if(ret->list_index!=XAW_LIST_NONE) {
    int n = 0;
    city_built_iterate(pdialog->pcity, pimprove) {
      if (n == ret->list_index) {
	char buf[512];

	if (!can_city_sell_building(pdialog->pcity, pimprove)) {
	  return;
	}

	pdialog->sell_id = improvement_number(pimprove);
	my_snprintf(buf, sizeof(buf), _("Sell %s for %d gold?"),
		    city_improvement_name_translation(pdialog->pcity, pimprove),
		    impr_sell_gold(pimprove));

	popup_message_dialog(pdialog->shell, "selldialog", buf,
			     sell_callback_yes, pdialog, 0,
			     sell_callback_no, pdialog, 0, NULL);

	return;
      }
      n++;
    } city_built_iterate_end;
  }
}

/****************************************************************
...
*****************************************************************/
void close_city_dialog(struct city_dialog *pdialog)
{
  if (pdialog->worklist_shell)
    XtDestroyWidget(pdialog->worklist_shell);

  XtDestroyWidget(pdialog->shell);
  dialog_list_unlink(dialog_list, pdialog);

  free(pdialog->citizen_labels);

  free(pdialog->support_unit_pixcomms);
  free(pdialog->present_unit_pixcomms);

  unit_list_iterate(pdialog->pcity->info_units_supported, psunit) {
    free(psunit);
  } unit_list_iterate_end;
  unit_list_clear(pdialog->pcity->info_units_supported);
  unit_list_iterate(pdialog->pcity->info_units_present, psunit) {
    free(psunit);
  } unit_list_iterate_end;
  unit_list_clear(pdialog->pcity->info_units_present);

/*
  if(pdialog->is_modal)
    XtSetSensitive(toplevel, TRUE);
*/
  free(pdialog);
  popdown_cma_dialog();
}

/****************************************************************
...
*****************************************************************/
void citydlg_key_close(Widget w)
{
  citydlg_msg_close(XtParent(XtParent(w)));
}

/****************************************************************
...
*****************************************************************/
void citydlg_msg_close(Widget w)
{
  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->shell == w) {
      close_city_dialog(pdialog);
      return;
    }
  } dialog_list_iterate_end;
}

/****************************************************************
...
*****************************************************************/
void close_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  close_city_dialog((struct city_dialog *)client_data);
}


/****************************************************************
								 
 City Options dialog:  (current only auto-attack options)
 
Note, there can only be one such dialog at a time, because
I'm lazy.  That could be fixed, similar to way you can have
multiple city dialogs.

triggle = tri_toggle (three way toggle button)

*****************************************************************/
								  
#define NUM_CITYOPT_TOGGLES 5

Widget create_cityopt_dialog(const char *cityname);
void cityopt_ok_command_callback(Widget w, XtPointer client_data, 
				XtPointer call_data);
void cityopt_cancel_command_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data);
void cityopt_newcit_triggle_callback(Widget w, XtPointer client_data,
					XtPointer call_data);

char *newcitizen_labels[] = {
  N_("?city:Workers"),
  N_("Scientists"),
  N_("Taxmen")
};

static Widget cityopt_shell = 0;
static Widget cityopt_triggle;
static Widget cityopt_toggles[NUM_CITYOPT_TOGGLES];
static int cityopt_city_id = 0;
static int newcitizen_index;

/****************************************************************
...
*****************************************************************/
void cityopt_callback(Widget w, XtPointer client_data,
                        XtPointer call_data)
{
  struct city_dialog *pdialog = (struct city_dialog *)client_data;
  struct city *pcity = pdialog->pcity;
  int i;

  if(cityopt_shell) {
    XtDestroyWidget(cityopt_shell);
  }
  cityopt_shell=create_cityopt_dialog(city_name(pcity));
  /* Doing this here makes the "No"'s centered consistently */
  for(i=0; i<NUM_CITYOPT_TOGGLES; i++) {
    bool state = is_city_option_set(pcity, i);
    XtVaSetValues(cityopt_toggles[i], XtNstate, state,
		  XtNlabel, state?_("Yes"):_("No"), NULL);
  }
  if (is_city_option_set(pcity, CITYO_NEW_EINSTEIN)) {
    newcitizen_index = 1;
  } else if (is_city_option_set(pcity, CITYO_NEW_TAXMAN)) {
    newcitizen_index = 2;
  } else {
    newcitizen_index = 0;
  }
  XtVaSetValues(cityopt_triggle, XtNstate, 1,
		XtNlabel, _(newcitizen_labels[newcitizen_index]),
		NULL);
  
  cityopt_city_id = pcity->id;

  xaw_set_relative_position(pdialog->shell, cityopt_shell, 15, 15);
  XtPopup(cityopt_shell, XtGrabNone);
}


/**************************************************************************
...
**************************************************************************/
Widget create_cityopt_dialog(const char *cityname)
{
  Widget shell, form, label, ok, cancel;
  int i;

  shell = I_T(XtCreatePopupShell("cityoptpopup",
				 transientShellWidgetClass,
				 toplevel, NULL, 0));
  form = XtVaCreateManagedWidget("cityoptform", 
				 formWidgetClass, 
				 shell, NULL);
  label = XtVaCreateManagedWidget("cityoptlabel", labelWidgetClass,
				  form, XtNlabel, cityname, NULL);


  I_L(XtVaCreateManagedWidget("cityoptnewcitlabel", labelWidgetClass, 
			      form, NULL));
  
  cityopt_triggle = XtVaCreateManagedWidget("cityoptnewcittriggle", 
					    toggleWidgetClass, 
					    form, NULL);

  /* NOTE: the ordering here is deliberately out of order;
     want toggles[] to be in enum city_options order, but
     want display in different order. --dwp
     - disband and workers options at top
     - helicopters (special case air) at bottom
  */

  I_L(XtVaCreateManagedWidget("cityoptdisbandlabel", labelWidgetClass, 
			      form, NULL));
  
  cityopt_toggles[4] = XtVaCreateManagedWidget("cityoptdisbandtoggle", 
					      toggleWidgetClass, 
					      form, NULL);

  I_L(XtVaCreateManagedWidget("cityoptvlandlabel", labelWidgetClass, 
			      form, NULL));
  
  cityopt_toggles[0] = XtVaCreateManagedWidget("cityoptvlandtoggle", 
					      toggleWidgetClass, 
					      form, NULL);
  
  I_L(XtVaCreateManagedWidget("cityoptvsealabel", labelWidgetClass, 
			      form, NULL));
  
  cityopt_toggles[1] = XtVaCreateManagedWidget("cityoptvseatoggle", 
					      toggleWidgetClass, 
					      form, NULL);
  
  I_L(XtVaCreateManagedWidget("cityoptvairlabel", labelWidgetClass, 
			      form, NULL));
  
  cityopt_toggles[3] = XtVaCreateManagedWidget("cityoptvairtoggle", 
					      toggleWidgetClass, 
					      form, NULL);
  
  I_L(XtVaCreateManagedWidget("cityoptvhelilabel", labelWidgetClass, 
			      form, NULL));
  
  cityopt_toggles[2] = XtVaCreateManagedWidget("cityoptvhelitoggle", 
					      toggleWidgetClass, 
					      form, NULL);

  ok = I_L(XtVaCreateManagedWidget("cityoptokcommand",
				   commandWidgetClass,
				   form, NULL));
  
  cancel = I_L(XtVaCreateManagedWidget("cityoptcancelcommand",
				       commandWidgetClass,
				       form, NULL));

  XtAddCallback(ok, XtNcallback, cityopt_ok_command_callback, 
                (XtPointer)shell);
  XtAddCallback(cancel, XtNcallback, cityopt_cancel_command_callback, 
                (XtPointer)shell);
  for(i=0; i<NUM_CITYOPT_TOGGLES; i++) {
    XtAddCallback(cityopt_toggles[i], XtNcallback, toggle_callback, NULL);
  }
  XtAddCallback(cityopt_triggle, XtNcallback,
		cityopt_newcit_triggle_callback, NULL);

  XtRealizeWidget(shell);

  xaw_horiz_center(label);
  return shell;
}
  
/**************************************************************************
...
**************************************************************************/
void cityopt_cancel_command_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data)
{
  XtDestroyWidget(cityopt_shell);
  cityopt_shell = 0;
}

/**************************************************************************
...
**************************************************************************/
void cityopt_ok_command_callback(Widget w, XtPointer client_data, 
				XtPointer call_data)
{
  struct city *pcity = game_find_city_by_number(cityopt_city_id);

  if (pcity) {
/*    int i; */
    bv_city_options new_options;
    Boolean b;

    assert(CITYO_LAST == 3);

    BV_CLR_ALL(new_options);
/*    for(i=0; i<NUM_CITYOPT_TOGGLES; i++)  {
      XtVaGetValues(cityopt_toggles[i], XtNstate, &b, NULL);
      if (b) new_options |= (1<<i);
    }
*/
    XtVaGetValues(cityopt_toggles[4], XtNstate, &b, NULL);
    if (b) {
      BV_SET(new_options, CITYO_DISBAND);
    }
    if (newcitizen_index == 1) {
      BV_SET(new_options, CITYO_NEW_EINSTEIN);
    } else if (newcitizen_index == 2) {
      BV_SET(new_options, CITYO_NEW_TAXMAN);
    }
    dsend_packet_city_options_req(&client.conn, cityopt_city_id,new_options);
  }
  XtDestroyWidget(cityopt_shell);
  cityopt_shell = 0;
}

/**************************************************************************
 Changes the label of the toggle widget to between newcitizen_labels
 and increments (mod 3) newcitizen_index.
**************************************************************************/
void cityopt_newcit_triggle_callback(Widget w, XtPointer client_data,
					XtPointer call_data)
{
  newcitizen_index++;
  if (newcitizen_index>=3) {
    newcitizen_index = 0;
  }
  XtVaSetValues(cityopt_triggle, XtNstate, 1,
		XtNlabel, _(newcitizen_labels[newcitizen_index]),
		NULL);
}

/**************************************************************************
...
**************************************************************************/
void popdown_cityopt_dialog(void)
{
  if(cityopt_shell) {
    XtDestroyWidget(cityopt_shell);
    cityopt_shell = 0;
  }
}

/****************************************************************
  The user clicked on the "CMA..." button in the citydialog.
*****************************************************************/
void cma_callback(Widget w, XtPointer client_data,
                  XtPointer call_data)
{
  struct city_dialog *pdialog = (struct city_dialog *)client_data;
  popdown_cma_dialog();
  show_cma_dialog(pdialog->pcity, pdialog->shell);
}


