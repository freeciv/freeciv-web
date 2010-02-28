/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project 
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
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Toggle.h>
#include <X11/IntrinsicP.h>

#include "pixcomm.h"
#include "canvas.h"

/* common & utility */
#include "city.h"
#include "fcintl.h"
#include "game.h"
#include "genlist.h"
#include "map.h"
#include "mem.h"
#include "packets.h"
#include "player.h"
#include "shared.h"
#include "support.h"

/* client */
#include "client_main.h"
#include "cma_fec.h"

#include "cityrep.h"
#include "citydlg.h"
#include "cma_fe.h"
#include "colors.h"
#include "control.h" 
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "optiondlg.h"  
#include "repodlgs.h"
#include "wldlg.h"

Widget *stat_surplus_label, *stat_factor_label;
Widget control_button, change_button, preset_list, cma_dialog;
Widget celebrate_toggle, result_label, release_button;
Widget surplus_slider[O_LAST], factor_slider[O_LAST + 1];

int minimal_surplus[O_LAST], factors[O_LAST + 1];
struct city *current_city;
char *initial_preset_list[] = {
	N_("For information on\n"
	"the citizen governor and governor presets,\n"
	"including sample presets,\n"
	"see README.cma."),
	NULL};


static void update_cma_preset_list(void);
static void control_city(Widget w, XtPointer client_data,
                         XtPointer call_data);
static void release_city(Widget w, XtPointer client_data,
                         XtPointer call_data);

static void add_preset(Widget w, XtPointer client_data,
                       XtPointer call_data);
static void remove_preset(Widget w, XtPointer client_data,
                          XtPointer call_data);
static void remove_preset_yes(Widget w, XtPointer client_data,
                              XtPointer call_data);
static void remove_preset_no(Widget w, XtPointer client_data,
                             XtPointer call_data);
static void select_preset(Widget w, XtPointer client_data,
                          XtPointer call_data);
static void cma_help(Widget w, XtPointer client_data,
                     XtPointer call_data);
static void set_slider_values(void);
static void sliders_jump_callback(Widget w, XtPointer client_data,
                                  XtPointer position_val);
static void sliders_scroll_callback(Widget w, XtPointer client_data,
                                    XtPointer position_val);
static void update_stat_labels(bool is_valid);
static void new_preset_callback(Widget w, XtPointer client_data,
                                XtPointer call_data);
static void celebrate_callback(Widget w, XtPointer client_data,
                               XtPointer position_val);
static void close_callback(Widget w, XtPointer client_data,
                           XtPointer call_data);
static void change_callback(Widget w, XtPointer client_data,
                            XtPointer call_data);


/**************************************************************************
 Show the Citizen Management Agent Dialog for the current city.
**************************************************************************/
void show_cma_dialog(struct city *pcity, Widget citydlg)
{
  Widget add_button, remove_button, help_button;
  Widget form, preset_viewport, close_button;
  Widget surplus_header, factor_header, prev;
  struct cm_parameter parameter;
  struct cm_result result;
  int i;

  current_city = pcity;
  stat_surplus_label = fc_malloc((O_LAST + 1) * sizeof(Widget));
  stat_factor_label = fc_malloc((O_LAST + 1) * sizeof(Widget));

  cma_dialog = 
    I_T(XtCreatePopupShell("cmapopup",
                           topLevelShellWidgetClass,
                           citydlg, NULL, 0));

  form = 
    XtVaCreateManagedWidget("cmaform",
                            formWidgetClass,
                            cma_dialog, NULL);

  preset_viewport = 
    XtVaCreateManagedWidget("cmapresetviewport",
                            viewportWidgetClass, form,
                            NULL);

  preset_list = 
    XtVaCreateManagedWidget("cmapresetlist",
                            listWidgetClass, preset_viewport,
                            XtNlist, (XtArgVal)initial_preset_list,
                            NULL);

  result_label = 
    I_L(XtVaCreateManagedWidget("cmaresultlabel",
                                labelWidgetClass, form,
                                XtNfromHoriz, preset_viewport,
                                NULL));

  surplus_header = 
    I_L(XtVaCreateManagedWidget("cmasurpluslabel",
                                labelWidgetClass, form,
                                XtNfromHoriz, preset_viewport,
                                XtNfromVert, result_label,
                                NULL));

  /* Create labels in the minimal surplus column. */
  prev = surplus_header;
  for (i = 0; i < (O_LAST + 1); i++) {
    I_L(stat_surplus_label[i] = 
        XtVaCreateManagedWidget("cmastatlabel",
                                labelWidgetClass, form,
                                XtNfromHoriz, preset_viewport,
                                XtNfromVert, prev,
                                XtNvertDistance, 1,
                                XtNlabel, (i == O_LAST) ? 
                                  _("Celebrate") : get_output_name(i),
                                NULL));
    prev = stat_surplus_label[i];
  }

  /* Create scrollbars in the minimal surplus column. */
  prev = surplus_header;
  output_type_iterate(i) {
    surplus_slider[i] = 
      XtVaCreateManagedWidget("cmapresetscroll",
                                  scrollbarWidgetClass, form,
                                  XtNfromHoriz, stat_surplus_label[i],
                                  XtNfromVert, prev,
                                  NULL);
    prev = stat_surplus_label[i];
  } output_type_iterate_end;

  celebrate_toggle = 
    I_L(XtVaCreateManagedWidget("cmapresettoggle",
                                toggleWidgetClass,form,
                                XtNfromHoriz, stat_surplus_label[0],
                                XtNfromVert, stat_surplus_label[O_LAST - 1],
                                NULL));

  /* Create header label in the factor column. */
  factor_header = 
    I_L(XtVaCreateManagedWidget("cmafactorlabel",
                                labelWidgetClass, form,
                                XtNfromHoriz, surplus_slider[0],
                                XtNfromVert, result_label,
                                NULL));

  /* Create stat labels in the factor column. */
  prev = factor_header;
  for (i = 0; i < (O_LAST + 1); i++) {
    I_L(stat_factor_label[i] =
        XtVaCreateManagedWidget("cmastatlabel",
                                labelWidgetClass, form,
                                XtNfromHoriz, surplus_slider[0],
                                XtNfromVert, prev,
                                XtNvertDistance, 1,
                                XtNlabel, (i == O_LAST) ?
                                  _("Celebrate") : get_output_name(i),
                                NULL));
    prev = stat_factor_label[i];
  }

  /* Create scrollbars in the factor column. */
  prev = factor_header;
  for (i = 0; i < (O_LAST + 1); i++) {
    factor_slider[i] =
      XtVaCreateManagedWidget("cmapresetscroll",
                                  scrollbarWidgetClass, form,
                                  XtNfromHoriz, stat_factor_label[i],
                                  XtNfromVert, prev,
                                  NULL);
    prev = stat_factor_label[i];
  }

  close_button = 
    I_L(XtVaCreateManagedWidget("cmaclosebutton",
                                commandWidgetClass, form,
                                XtNfromVert, preset_viewport,
                                NULL));

  control_button = 
    I_L(XtVaCreateManagedWidget("cmacontrolbutton",
                                commandWidgetClass, form,
                                XtNfromVert, preset_viewport,
                                XtNfromHoriz, close_button,
                                NULL));

  release_button = 
    I_L(XtVaCreateManagedWidget("cmareleasebutton",
                                commandWidgetClass, form,
                                XtNfromVert, preset_viewport,
                                XtNfromHoriz, control_button,
                                NULL));

  change_button =
    I_L(XtVaCreateManagedWidget("cmachangebutton",
                                commandWidgetClass, form,
                                XtNfromVert, preset_viewport,
                                XtNfromHoriz, release_button,
                                NULL));

  add_button = 
   I_L(XtVaCreateManagedWidget("cmaaddbutton",
                               commandWidgetClass, form,
                               XtNfromVert, preset_viewport,
                               XtNfromHoriz, change_button,
                               NULL));

  remove_button = 
     I_L(XtVaCreateManagedWidget("cmaremovebutton",
                                 commandWidgetClass, form, 
                                 XtNfromVert, preset_viewport,
                                 XtNfromHoriz, add_button,
                                 NULL));

  help_button = 
     I_L(XtVaCreateManagedWidget("cmahelpbutton",
                                 commandWidgetClass, form,
                                 XtNfromVert, preset_viewport,
                                 XtNfromHoriz, remove_button,
                                 NULL));

  XtAddCallback(control_button, XtNcallback, control_city,
                (XtPointer)preset_list);
  XtAddCallback(release_button, XtNcallback, release_city,
                (XtPointer)preset_list);
  XtAddCallback(change_button, XtNcallback, change_callback,
                (XtPointer)preset_list);
  XtAddCallback(add_button, XtNcallback, add_preset,
                (XtPointer)preset_list);
  XtAddCallback(remove_button, XtNcallback, remove_preset,
                (XtPointer)preset_list);
  XtAddCallback(preset_list, XtNcallback, select_preset,
                (XtPointer)preset_list);
  XtAddCallback(help_button, XtNcallback, cma_help,
                (XtPointer)preset_list);
  XtAddCallback(celebrate_toggle, XtNcallback,
                celebrate_callback, NULL);
  XtAddCallback(close_button, XtNcallback,
                close_callback, NULL);

  output_type_iterate(i) {
    XtAddCallback(surplus_slider[i], XtNscrollProc,
                  sliders_scroll_callback, NULL);
    XtAddCallback(surplus_slider[i], XtNjumpProc,
                  sliders_jump_callback, NULL);
  } output_type_iterate_end;

  for (i = 0; i < (O_LAST + 1); i++) {
    XtAddCallback(factor_slider[i], XtNscrollProc,
                  sliders_scroll_callback, NULL);
    XtAddCallback(factor_slider[i], XtNjumpProc,
                  sliders_jump_callback, NULL);
  }

  /* Update dialog with CMA parameters from city.  */
  cmafec_get_fe_parameter(current_city, &parameter);

  output_type_iterate(i) {
    minimal_surplus[i] = parameter.minimal_surplus[i];
  } output_type_iterate_end;

  XtVaSetValues(celebrate_toggle, 
         XtNlabel, parameter.require_happy ? _("Yes") : _("No"),
         XtNstate, parameter.require_happy, NULL);

  if (parameter.happy_factor > 0) {
    factors[O_LAST] = parameter.happy_factor;
  } else {
    factors[O_LAST] = 1;
  }

  output_type_iterate(i) {
    factors[i] = parameter.factor[i];
  } output_type_iterate_end;

  set_slider_values();
  update_cma_preset_list();

  XtVaSetValues(preset_list, XtNwidth, 200, NULL);
  XtVaSetValues(preset_list, XtNheight, 300, NULL);
  XtVaSetValues(celebrate_toggle, XtNwidth, 30, NULL);
  XtVaSetValues(result_label, XtNwidth, 360, NULL);
  XtVaSetValues(result_label, XtNheight, 110, NULL);

  output_type_iterate(i) {
    XtVaSetValues(stat_surplus_label[i], XtNwidth, 90, NULL);
    XtVaSetValues(stat_factor_label[i], XtNwidth, 90, NULL);
  } output_type_iterate_end;
  /* FIXME! Now we assume that output_type_iterate ends with O_LAST. */
  XtVaSetValues(stat_factor_label[O_LAST], XtNwidth, 90, NULL);

  XtRealizeWidget(cma_dialog);

  update_stat_labels(True);
  cm_result_from_main_map(&result, pcity, TRUE);
  xaw_set_label(result_label, 
       (char *) cmafec_get_result_descr(current_city, &result, &parameter));

  XSetWMProtocols(display, XtWindow(cma_dialog),
                  &wm_delete_window, 1);
  XtVaSetValues(preset_viewport, XtNforceBars, True, NULL);
  xaw_set_relative_position(citydlg, cma_dialog, 5, 5);
  XtPopup(cma_dialog, XtGrabNone);
}

/**************************************************************************
 Fills in the preset list.
**************************************************************************/
static void update_cma_preset_list(void) 
{
  static char *preset_lines[256];
  static char preset_text[256][256];
  int i;

  if (cmafec_preset_num()) {
    /* Add all CMA presets to the list. */
    for (i = 0; i < cmafec_preset_num(); i++) {
      mystrlcpy(preset_text[i], cmafec_preset_get_descr(i),
		sizeof(preset_text[i]));
      preset_lines[i] = preset_text[i];
    }
    XawListChange(preset_list, preset_lines, cmafec_preset_num(), 0, True);
  } else {
    /* Show intro message in the preset list. */
    XawListChange(preset_list, initial_preset_list, 8, 0, True);
  }  
}

/****************************************************************
 Enable CMA control over the city.
*****************************************************************/
static void control_city(Widget w, XtPointer list,
                         XtPointer call_data)
{
  struct cm_parameter param;

  cmafec_get_fe_parameter(current_city, &param);
  cma_put_city_under_agent(current_city, &param);
  refresh_city_dialog(current_city);

  XtSetSensitive(control_button, FALSE);
  XtSetSensitive(release_button, TRUE);
  XtSetSensitive(change_button, FALSE);
}

/****************************************************************
 Disables CMA control over the city.
*****************************************************************/
static void release_city(Widget w, XtPointer list,
                         XtPointer call_data)
{
  cma_release_city(current_city);
  refresh_city_dialog(current_city);

  XtSetSensitive(control_button, TRUE);
  XtSetSensitive(release_button, FALSE);
  XtSetSensitive(change_button, TRUE);
}

/****************************************************************
 Create a new preset, first asking for the presets name.
*****************************************************************/
static void add_preset(Widget w, XtPointer client_data,
                       XtPointer call_data)
{
  input_dialog_create(cma_dialog, "cmapresetname",
                      _("What should we name the new preset?"), 
                      _("new preset"),
                      new_preset_callback, INT_TO_XTPOINTER(TRUE),
                      new_preset_callback, INT_TO_XTPOINTER(FALSE));
}

/****************************************************************
 Confirm that the user wants to remove the selected preset.
*****************************************************************/
static void remove_preset(Widget w, XtPointer list,
                          XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(preset_list);
  char buf[256]; 

  if (ret->list_index != XAW_LIST_NONE && cmafec_preset_num()) {
    my_snprintf(buf, sizeof(buf), 
                _("Do you really want to remove %s?"),
                cmafec_preset_get_descr(ret->list_index));

    popup_message_dialog(cma_dialog, "cmaremovepreset",
                         buf, remove_preset_yes, 
                         INT_TO_XTPOINTER(ret->list_index), 0,
                         remove_preset_no, 0, 0,
                         NULL);
  }
}

/****************************************************************
 Remove the preset selected in the preset list.
*****************************************************************/
static void remove_preset_yes(Widget w, XtPointer row,
                              XtPointer call_data)
{
  cmafec_preset_remove(XTPOINTER_TO_INT(row));
  update_cma_preset_list();
  destroy_message_dialog(w);
}

/****************************************************************
 Close the remove preset dialog, without removing a preset. 
*****************************************************************/
static void remove_preset_no(Widget w, XtPointer client_data,
                             XtPointer call_data)
{
  destroy_message_dialog(w);
}

/****************************************************************
 Callback for CMA the preset list. 
*****************************************************************/
static void select_preset(Widget w, XtPointer list,
                          XtPointer call_data)
{
  XawListReturnStruct *ret;
  const struct cm_parameter *param;
  struct cm_parameter parameter;
  struct cm_result result;

  ret = XawListShowCurrent(list);

  if (ret->list_index != XAW_LIST_NONE && cmafec_preset_num()) {
    param = cmafec_preset_get_parameter(ret->list_index);
    cmafec_set_fe_parameter(current_city, param);

    if (cma_is_city_under_agent(current_city, NULL)) {
      cma_release_city(current_city);
      cma_put_city_under_agent(current_city, param);
    }

    cmafec_get_fe_parameter(current_city, &parameter);
    cm_result_from_main_map(&result, current_city, TRUE);
    xaw_set_label(result_label,
        (char *) cmafec_get_result_descr(current_city, &result, &parameter));

    output_type_iterate(i) {
      minimal_surplus[i] = param->minimal_surplus[i];
    } output_type_iterate_end;

    XtVaSetValues(celebrate_toggle, XtNlabel,
            param->require_happy ? _("Yes") : _("No"),
            XtNstate, parameter.require_happy, NULL);

    factors[O_LAST] = param->happy_factor;

    output_type_iterate(i) {
      factors[i] = param->factor[i];
    } output_type_iterate_end;
  }

  update_stat_labels(result.found_a_valid);
}

/****************************************************************
 Show the CMA help page.
*****************************************************************/
static void cma_help(Widget w, XtPointer list,
                     XtPointer call_data)
{
  popup_help_dialog_string(HELP_CMA_ITEM);
}

/****************************************************************
 Sets the position for all sliders.
*****************************************************************/
static void set_slider_values(void) 
{
  int i;
  
  output_type_iterate(i) {
    XawScrollbarSetThumb(surplus_slider[i],
                     (((20 + minimal_surplus[i])))*1/41.0f, 1/41.0f);
  } output_type_iterate_end;

  for (i = 0; i < (O_LAST + 1); i++) { 
    XawScrollbarSetThumb(factor_slider[i],
                     (((factors[i])))*1/25.0f, 1/25.0f);
  }
}

/**************************************************************************
 Callback to update parameters if slider was scrolled. (Xaw)
**************************************************************************/
static void sliders_scroll_callback(Widget w, XtPointer client_data,
                                    XtPointer position_val)
{
  int pos = XTPOINTER_TO_INT(position_val);
  struct cm_parameter parameter;
  struct cm_result result;
  int i, preset_index;

  output_type_iterate(i) {
    if (w == surplus_slider[i]) {
      if (pos > 0 ) {
        minimal_surplus[i]++;
        if (minimal_surplus[i] == 21) {
          minimal_surplus[i]--;
        }
      } else {
        minimal_surplus[i]--;
        if (minimal_surplus[i] == -21) {
          minimal_surplus[i]++;
        }
      }
    }
  } output_type_iterate_end;

  for (i = 0; i < (O_LAST + 1); i++) {
    if (w == factor_slider[i]) {
      if (pos > 0 ) {
        factors[i]++;
        if (factors[i] == 26) {
          factors[i]--;
        }
      } else {
        factors[i]--;
        if (factors[i] == 0) {
          factors[i]++;
        }
      }
    }
  }

  cmafec_get_fe_parameter(current_city, &parameter);

  output_type_iterate(i) {
    parameter.minimal_surplus[i] = minimal_surplus[i];
    parameter.factor[i] = factors[i];
  } output_type_iterate_end;

  XtVaGetValues(celebrate_toggle, XtNstate, &parameter.require_happy, NULL);
  parameter.happy_factor = factors[O_LAST];

  cmafec_set_fe_parameter(current_city, &parameter);

  if (cma_is_city_under_agent(current_city, NULL)) {
    cma_release_city(current_city);
    cma_put_city_under_agent(current_city, &parameter);
  }

  cmafec_get_fe_parameter(current_city, &parameter);
  cm_result_from_main_map(&result, current_city, TRUE);
  xaw_set_label(result_label,
        (char *) cmafec_get_result_descr(current_city, &result, &parameter));

  update_stat_labels(result.found_a_valid);
  update_cma_preset_list();

  /* highlight preset if parameter matches */
  preset_index = cmafec_preset_get_index_of_parameter(&parameter);
  if (preset_index != -1) {
    XawListHighlight(preset_list, preset_index);
  }
}

/**************************************************************************
 Callback to update parameters if slider was jumped. (Xaw3D)
**************************************************************************/
void sliders_jump_callback(Widget w, XtPointer client_data,
                           XtPointer percent_ptr)
{
  float percent=*(float*)percent_ptr;
  struct cm_parameter parameter;
  struct cm_result result;
  int i, preset_index;

  output_type_iterate(i) {
    if (w == surplus_slider[i]) {
      /* convert from percent to [-20..20] */
      minimal_surplus[i] = (int)(percent * 41) - 20;
    }
  } output_type_iterate_end;

  for (i = 0; i < (O_LAST + 1); i++) {
    if (w == factor_slider[i]) {
      /* convert from percent to [1..25] */
      factors[i] = (int)(percent * 25) + 1;  
    }
  }

  cmafec_get_fe_parameter(current_city, &parameter);

  output_type_iterate(i) {
    parameter.minimal_surplus[i] = minimal_surplus[i];
    parameter.factor[i] = factors[i];
  } output_type_iterate_end;

  XtVaGetValues(celebrate_toggle, XtNstate, &parameter.require_happy, NULL);
  parameter.happy_factor = factors[O_LAST];

  cmafec_set_fe_parameter(current_city, &parameter);

  if (cma_is_city_under_agent(current_city, NULL)) {
    cma_release_city(current_city);
    cma_put_city_under_agent(current_city, &parameter);
  }

  cmafec_get_fe_parameter(current_city, &parameter);
  cm_result_from_main_map(&result, current_city, TRUE);
  xaw_set_label(result_label,
        (char *) cmafec_get_result_descr(current_city, &result, &parameter));

  update_cma_preset_list();
  update_stat_labels(result.found_a_valid);

  /* highlight preset if parameter matches */
  preset_index = cmafec_preset_get_index_of_parameter(&parameter);
  if (preset_index != -1) {
    XawListHighlight(preset_list, preset_index);
  }
}

/**************************************************************************
 Update CMA stat labels, set sensitivity on buttons, refresh city dialog.
**************************************************************************/
static void update_stat_labels(bool is_valid)
{
  char buf[256]; 

  output_type_iterate(i) {
    my_snprintf(buf, sizeof(buf), "%-9s%3d",
                get_output_name(i),
                minimal_surplus[i]);
    xaw_set_label(stat_surplus_label[i], buf);
  } output_type_iterate_end;

  output_type_iterate(i) {
    my_snprintf(buf, sizeof(buf), "%-9s%3d",
                get_output_name(i),
                factors[i]);
    xaw_set_label(stat_factor_label[i], buf);
  } output_type_iterate_end;
  my_snprintf(buf, sizeof(buf), "%-9s%3d",
              "Celebrate",
              factors[O_LAST]);
  xaw_set_label(stat_factor_label[O_LAST], buf);

  XtSetSensitive(release_button, 
       (cma_is_city_under_agent(current_city, NULL)
        && can_client_issue_orders()));

  XtSetSensitive(control_button, 
       (!cma_is_city_under_agent(current_city, NULL)
        && is_valid
        && can_client_issue_orders()));

  XtSetSensitive(change_button,
       (!cma_is_city_under_agent(current_city, NULL)
        && is_valid
        && can_client_issue_orders()));
  set_slider_values();
  refresh_city_dialog(current_city);
}

/**************************************************************************
 Callback to save the new CMA preset after a name has been typed.
**************************************************************************/
static void new_preset_callback(Widget w, XtPointer save_preset,
                                     XtPointer call_data)
{
  Boolean celebrate_setting;
  struct cm_parameter parameter;
  
  if (save_preset) {
    /* The user entered a preset name and clicked OK  */
    XtVaGetValues(celebrate_toggle, XtNstate, &celebrate_setting, NULL);
    cmafec_get_fe_parameter(current_city, &parameter);

    output_type_iterate(i) {
      parameter.minimal_surplus[i] = minimal_surplus[i];
      parameter.factor[i] = factors[i];
    } output_type_iterate_end;

    parameter.happy_factor = factors[O_LAST];
    parameter.require_happy = celebrate_setting;

    cmafec_preset_add(input_dialog_get_input(w), &parameter);

    update_cma_preset_list();
  }
  input_dialog_destroy(w);
}

/**************************************************************************
 Callback to handle changes to the celebrate toggle.
**************************************************************************/
void celebrate_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Boolean celebrate;
  struct cm_parameter parameter;
  struct cm_result result;

  /* Change label on celebrate toggle. */
  XtVaGetValues(w, XtNstate, &celebrate, NULL);
  XtVaSetValues(w, XtNlabel, celebrate ? _("Yes") : _("No"), NULL);

  /* Update parameters with new celebrate setting. */
  cmafec_get_fe_parameter(current_city, &parameter);
  output_type_iterate(i) {
    parameter.minimal_surplus[i] = minimal_surplus[i];
    parameter.factor[i] = factors[i];
  } output_type_iterate_end;

  XtVaGetValues(celebrate_toggle, XtNstate, &parameter.require_happy, NULL);
  parameter.happy_factor = factors[O_LAST];

  cmafec_set_fe_parameter(current_city, &parameter);

  if (cma_is_city_under_agent(current_city, NULL)) {
    cma_release_city(current_city);
    cma_put_city_under_agent(current_city, &parameter);
  }

  cmafec_get_fe_parameter(current_city, &parameter);
  cm_result_from_main_map(&result, current_city, TRUE);
  xaw_set_label(result_label,
        (char *) cmafec_get_result_descr(current_city, &result, &parameter));

  update_stat_labels(result.found_a_valid);
  update_cma_preset_list();
}

/**************************************************************************
 Close the CMA dialog. Also called when the citydialog closes.
**************************************************************************/
void popdown_cma_dialog(void)
{
  city_report_dialog_update_city(current_city);
  if (cma_dialog) {
    XtDestroyWidget(cma_dialog);
    cma_dialog = 0;
  }
}

/**************************************************************************
  Callback to close the CMA dialog when the user clicks on Close.
**************************************************************************/
static void close_callback(Widget w, XtPointer client_data,
                           XtPointer call_data)
{
  popdown_cma_dialog();
}

/**************************************************************************
  Callback to apply CMA preset to city once. 
**************************************************************************/
static void change_callback(Widget w, XtPointer client_data,
                            XtPointer call_data)
{
  struct cm_result result;
  struct cm_parameter param;

  cmafec_get_fe_parameter(current_city, &param);
  cm_query_result(current_city, &param, &result);
  cma_apply_result(current_city, &result);
  refresh_city_dialog(current_city);
}

