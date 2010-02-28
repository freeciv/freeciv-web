/**********************************************************************
 Freeciv - Copyright (C) 2004 A. Kemnade
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

#include <windows.h>
#include <windowsx.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "events.h"
#include "game.h"

/* client */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "client_main.h"
#include "cma_fec.h"
#include "messagewin_g.h"

#include "cityrep.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "inputdlg.h"

#include "cma_fe.h"

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct cma_dialog
#include "speclist.h"

struct cma_dialog {
  struct city *pcity;
  HWND mainwin;
  HWND preset_list;
  HWND result_label;
  HWND add_preset;
  HWND del_preset;
  HWND change;
  HWND perm;
  HWND release;
  HWND minimal_surplus[O_LAST];
  HWND happy;
  HWND factor[O_LAST + 1];
  int id;
};

enum cmagui_ids {
  CMAGUI_PRESET_LIST = 100,
  CMAGUI_PRESET_ADD,
  CMAGUI_PRESET_DEL,
  CMAGUI_CHANGE,
  CMAGUI_PERM,
  CMAGUI_HAPPY,
  CMAGUI_RELEASE,
  CMAGUI_SURPLUS = 200,
  CMAGUI_FACTOR = 300
} ;

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct cma_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static void handle_hscroll(HWND win, HWND winctl, UINT code, int pos);

static struct dialog_list *dialog_list;
static bool dialog_list_has_been_initialised = FALSE;
static int allow_refreshes = 1;


/**************************************************************************
...
**************************************************************************/
static void ensure_initialised_dialog_list(void)
{
  if (!dialog_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_has_been_initialised = TRUE;
  }
}

/****************************************************************
 return the cma_dialog for a given city.
*****************************************************************/
struct cma_dialog *get_cma_dialog(struct city *pcity)
{
  ensure_initialised_dialog_list();

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pcity == pcity) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}




/**************************************************************************

**************************************************************************/
static void cmaguisp_minsize(POINT *rcmin, void *data)
{ 
  rcmin->y=15;
  rcmin->x=80;
}

/**************************************************************************

**************************************************************************/
static void cmaguifa_minsize(POINT *rcmin, void *data)
{
  rcmin->y=15;
  rcmin->x=60;
}

/**************************************************************************

**************************************************************************/
static void cmaguisl_setsize(RECT *rc,void *data)
{ 
  MoveWindow((HWND)data,rc->left,rc->top,
             rc->right-rc->left,
             rc->bottom-rc->top,TRUE);
}

/**************************************************************************

**************************************************************************/
static void cmaguisl_delete(void *data)
{ 
  DestroyWindow((HWND)data);
}

/**************************************************************************
  create a single CMA slider row
**************************************************************************/
static void cmagui_add_slider(struct cma_dialog *pdialog,
			      HWND win, struct fcwin_box **vbox,
			      bool checkbox, const char *name,
			      int id)
{
  struct fcwin_box *hbox;
  fcwin_box_add_static(vbox[0], name, 0, SS_LEFT, TRUE, TRUE, 0);
  if (checkbox) {
    pdialog->happy = fcwin_box_add_checkbox(vbox[1], " ", CMAGUI_HAPPY, 0,
					    TRUE, TRUE, 0);
  } else {
    hbox = fcwin_hbox_new(win, FALSE);
    pdialog->minimal_surplus[id] = CreateWindow("SCROLLBAR",NULL,
					    WS_CHILD | WS_VISIBLE | SBS_HORZ,
					    0,0,0,0,
					    win,
					    (HMENU)(id + CMAGUI_SURPLUS),
					    freecivhinst,NULL);

    fcwin_box_add_generic(hbox, cmaguisp_minsize, cmaguisl_setsize,
			  cmaguisl_delete, pdialog->minimal_surplus[id],
			  TRUE, TRUE, 10);
    fcwin_box_add_static(hbox, " 0", 0, SS_RIGHT, FALSE, FALSE, 0);
    fcwin_box_add_box(vbox[1], hbox, TRUE, TRUE, 0);
    ScrollBar_SetRange(pdialog->minimal_surplus[id], -20, 20, TRUE);
  }

  hbox = fcwin_hbox_new(win, FALSE);

  pdialog->factor[id] = CreateWindow("SCROLLBAR",NULL,
				     WS_CHILD | WS_VISIBLE | SBS_HORZ,
				     0,0,0,0,
				     win,
				     (HMENU)(id + CMAGUI_FACTOR),
				     freecivhinst,NULL);
  fcwin_box_add_generic(hbox, cmaguifa_minsize, cmaguisl_setsize,
			cmaguisl_delete, pdialog->factor[id],
			TRUE, TRUE, 10);
  fcwin_box_add_static(hbox, " 0", 0, SS_RIGHT, FALSE, FALSE, 0);
  fcwin_box_add_box(vbox[2], hbox, TRUE, TRUE, 0);

  ScrollBar_SetRange(pdialog->factor[id], 0, 50, TRUE);
 

}

/**************************************************************************
  create CMA sliders
**************************************************************************/
static void create_cma_sliders(struct cma_dialog *pdialog,
			       HWND win, struct fcwin_box *box)
{
  struct fcwin_box *hbox = fcwin_hbox_new(win, FALSE);
  struct fcwin_box *vbox[3];
  int i;

  fcwin_box_add_box(box, hbox, FALSE,FALSE, 0);
  for (i = 0; i < 3; i++) {
    vbox[i] = fcwin_vbox_new(win, TRUE);
    fcwin_box_add_box(hbox, vbox[i], FALSE, FALSE,5);
  }
  fcwin_box_add_static(vbox[0], " ",0, SS_LEFT,TRUE, TRUE, 0);
  fcwin_box_add_static(vbox[1], _("Minimal Surplus"), 0, SS_LEFT, TRUE, TRUE, 0);
  fcwin_box_add_static(vbox[2], _("Factor"), 0, SS_LEFT, TRUE, TRUE, 0);
  output_type_iterate(i) {
    cmagui_add_slider(pdialog, win, vbox, FALSE, get_output_name(i), i);
  } output_type_iterate_end;
  cmagui_add_slider(pdialog, win, vbox, TRUE, _("Celebrate"), i);
}

/****************************************************************
 called to adjust the sliders when a preset is selected
 notice that we don't want to call update_result here. 
*****************************************************************/
static void set_hscales(const struct cm_parameter *const parameter,
                        struct cma_dialog *pdialog)
{
  allow_refreshes = 0;
  output_type_iterate(i) {
    handle_hscroll(pdialog->mainwin, pdialog->minimal_surplus[i],
		   SB_THUMBTRACK, parameter->minimal_surplus[i]);
    handle_hscroll(pdialog->mainwin, pdialog->factor[i],
		   SB_THUMBTRACK, parameter->factor[i]);
  } output_type_iterate_end;
  Button_SetCheck(pdialog->happy,
		  parameter->require_happy ? BST_CHECKED : BST_UNCHECKED);
  handle_hscroll(pdialog->mainwin, pdialog->factor[O_LAST],
		 SB_THUMBTRACK, parameter->happy_factor);
  allow_refreshes = 1;
}

/**************************************************************************
 refreshes the cma dialog
**************************************************************************/
void refresh_cma_dialog(struct city *pcity, enum cma_refresh refresh)
{
  struct cm_result result;
  struct cm_parameter param;
  struct cma_dialog *pdialog = get_cma_dialog(pcity);
 
  int controlled = cma_is_city_under_agent(pcity, NULL);
  int preset_index;
  cmafec_get_fe_parameter(pcity, &param);
  if (!pdialog)
    return;
  /* fill in result label */
  cm_result_from_main_map(&result, pcity, TRUE);
  SetWindowText(pdialog->result_label,
		(char *) cmafec_get_result_descr(pcity, &result, &param));
  /* if called from a hscale, we _don't_ want to do this */
  if (refresh != DONT_REFRESH_HSCALES) {
    set_hscales(&param, pdialog);
  }
  if (refresh != DONT_REFRESH_SELECT) {
    /* highlight preset if parameter matches */
    preset_index = cmafec_preset_get_index_of_parameter(&param);
    if (preset_index != -1) {
      allow_refreshes = 0;
      ListBox_SetCurSel(pdialog->preset_list,
			preset_index);
      allow_refreshes = 1;
    } else {
      int s = ListBox_GetCurSel(pdialog->preset_list);
      if (s != LB_ERR)
	ListBox_SetSel(pdialog->preset_list, FALSE, s);
    }
  }
  EnableWindow(pdialog->change, 
	       can_client_issue_orders() &&
	       result.found_a_valid && !controlled);
  EnableWindow(pdialog->perm,
	       can_client_issue_orders() &&
	       result.found_a_valid && !controlled);
  EnableWindow(pdialog->release,
	       can_client_issue_orders() &&
	       controlled);
}

/**************************************************************************
 fills in the preset list
**************************************************************************/
static void update_cma_preset_list(struct cma_dialog *pdialog)
{
  int i;

  /* Fill preset list */
  ListBox_ResetContent(pdialog->preset_list);
  
  /* if there are no presets preset, print informational message else 
   * append the presets */
  if (cmafec_preset_num()) {
    for (i = 0; i < cmafec_preset_num(); i++) {
      ListBox_AddString(pdialog->preset_list, cmafec_preset_get_descr(i));
    }
  } else {
    ListBox_AddString(pdialog->preset_list, _("For information on:"));
    ListBox_AddString(pdialog->preset_list, _("CMA and presets"));
    ListBox_AddString(pdialog->preset_list, _("including sample presets,"));
    ListBox_AddString(pdialog->preset_list, _("see README.cma."));
  }
}

/****************************************************************
 callback for selecting a preset from the preset clist
*****************************************************************/
static void cma_select_preset_callback(struct cma_dialog *pdialog)
{
  int preset_index = ListBox_GetCurSel(pdialog->preset_list);
  const struct cm_parameter *pparam;
  
  if (preset_index == LB_ERR)
    return;
  if (preset_index >= cmafec_preset_num())
    return;
  pparam = cmafec_preset_get_parameter(preset_index);
  
  /* save the change */
  cmafec_set_fe_parameter(pdialog->pcity, pparam);
  
  if (allow_refreshes) {
    if (cma_is_city_under_agent(pdialog->pcity, NULL)) {
      cma_release_city(pdialog->pcity);
      cma_put_city_under_agent(pdialog->pcity, pparam);

      /* unfog the city map if we were unable to put back under */
      if (!cma_is_city_under_agent(pdialog->pcity, NULL)) {
        refresh_city_dialog(pdialog->pcity);
        return;                 /* refreshing the city, refreshes cma */
      } else {
        city_report_dialog_update_city(pdialog->pcity);
      }
    }
    refresh_cma_dialog(pdialog->pcity, DONT_REFRESH_SELECT);
  }
  
}

/****************************************************************
 callback for the add_preset popup (don't add it)
*****************************************************************/
static void cma_preset_add_callback_no(HWND w, void *data)
{
  DestroyWindow(w);
}

/****************************************************************
 callback for the add_preset popup (add it)
*****************************************************************/
static void cma_preset_add_callback_yes(HWND w, void *data)
{
  struct cma_dialog *pdialog = (struct cma_dialog *) data;
  struct cm_parameter param;

  cmafec_get_fe_parameter(pdialog->pcity, &param);
  cmafec_preset_add(input_dialog_get_input(w), &param);
  update_cma_preset_list(pdialog);
  refresh_cma_dialog(pdialog->pcity, DONT_REFRESH_HSCALES);
  /* if this or other cities have this set as "custom" */
  city_report_dialog_update();
  DestroyWindow(w);
}


/**************************************************************************
 pops up a dialog to allow to name your new preset
**************************************************************************/
static void cma_add_preset_callback(struct cma_dialog *pdialog)
{
  
  input_dialog_create(pdialog->mainwin,
		      _("Name new preset"),
		      _("What should we name the preset?"),
		      _("new preset"),
		      cma_preset_add_callback_yes,
		      (void *) pdialog,
		      cma_preset_add_callback_no,
		      (void *) pdialog);
}


/**************************************************************************
 callback for del_preset 
**************************************************************************/
static void cma_del_preset_callback(struct cma_dialog *pdialog)
{
  int sel = ListBox_GetCurSel(pdialog->preset_list);
  if (sel == LB_ERR)
    return;
  cmafec_preset_remove(sel);
  update_cma_preset_list(pdialog);
  refresh_cma_dialog(pdialog->pcity, DONT_REFRESH_HSCALES);
  /* if this or other cities have this set, reset to "custom" */
  city_report_dialog_update();

}

/**************************************************************************
 changes the workers of the city to the cma parameters
**************************************************************************/
static void cma_change_to_callback(struct cma_dialog *pdialog)
{
  struct cm_result result;
  struct cm_parameter param;

  cmafec_get_fe_parameter(pdialog->pcity, &param);
  cm_query_result(pdialog->pcity, &param, &result);
  cma_apply_result(pdialog->pcity, &result);
}


/**************************************************************************
 changes the workers of the city to the cma parameters and puts the
 city under agent control
**************************************************************************/
static void cma_change_permanent_to_callback(struct cma_dialog *pdialog)
{
  struct cm_parameter param;

  cmafec_get_fe_parameter(pdialog->pcity, &param);
  cma_put_city_under_agent(pdialog->pcity, &param);
  refresh_city_dialog(pdialog->pcity);
}

/**************************************************************************
 releases the city from agent control
**************************************************************************/
static void cma_release_callback(struct cma_dialog *pdialog)
{
  cma_release_city(pdialog->pcity);
  refresh_city_dialog(pdialog->pcity);
}



/**************************************************************************
  create CMA gui
**************************************************************************/
static struct cma_dialog * create_cma_gui(HWND win)
{
  struct cma_dialog *pdialog = fc_malloc(sizeof(struct cma_dialog));
  struct fcwin_box *vbox;
  struct fcwin_box *vbox2;
  struct fcwin_box *hbox;
  struct fcwin_box *hbox2;
  vbox = fcwin_vbox_new(win, FALSE);
  fcwin_box_add_static(vbox, _("Presets"), 0, SS_LEFT, FALSE, FALSE, 0);
  hbox = fcwin_hbox_new(win, FALSE);
  fcwin_box_add_box(vbox, hbox, TRUE, TRUE, 0);
  pdialog->mainwin = win;
  vbox2 = fcwin_vbox_new(win, FALSE);
  fcwin_box_add_box(hbox, vbox2, TRUE, TRUE, 0);
  pdialog->preset_list = fcwin_box_add_list(vbox2, 8, CMAGUI_PRESET_LIST,
					    0, TRUE, TRUE, 0);
  hbox2 = fcwin_hbox_new(win, TRUE);
  pdialog->add_preset = fcwin_box_add_button(hbox2, _("New"),
					     CMAGUI_PRESET_ADD,
					     0, TRUE, TRUE, 5);
  pdialog->del_preset = fcwin_box_add_button(hbox2, _("Delete"),
					     CMAGUI_PRESET_DEL,
					     0, TRUE ,TRUE, 5);
  fcwin_box_add_box(vbox2, hbox2, FALSE, FALSE, 0);

  vbox2 = fcwin_vbox_new(win, FALSE);
  fcwin_box_add_box(hbox, vbox2, TRUE, TRUE, 0);
  hbox2 = fcwin_hbox_new(win, FALSE);
  pdialog->result_label = fcwin_box_add_static(hbox2, "x\nx\nx\nx\nx\nx\nx\nx",
					       0, SS_LEFT, TRUE, TRUE, 0);
  SendMessage(pdialog->result_label,
	      WM_SETFONT,(WPARAM) font_12courier, MAKELPARAM(TRUE, 0)); 
  fcwin_box_add_groupbox(vbox2, _("Results"), hbox2, 0, FALSE, FALSE, 5);

  create_cma_sliders(pdialog, win, vbox2);

  hbox2 = fcwin_hbox_new(win, TRUE);
  fcwin_box_add_box(vbox2, hbox2, FALSE, FALSE, 0);
  pdialog->change = fcwin_box_add_button(hbox2, _("Apply once"),
					 CMAGUI_CHANGE, 0, TRUE, TRUE, 0);
  pdialog->perm = fcwin_box_add_button(hbox2, _("Control city"),
				       CMAGUI_PERM, 0, TRUE, TRUE, 0);
  pdialog->release = fcwin_box_add_button(hbox2, _("Release city"),
					  CMAGUI_RELEASE, 0, TRUE, TRUE, 0);

  fcwin_set_box(win, vbox);
  ensure_initialised_dialog_list();
  
  dialog_list_prepend(dialog_list, pdialog);
  
  
  return pdialog;
}


/************************************************************************
 callback if we moved the sliders.
*************************************************************************/
static void hscale_changed(struct cma_dialog *pdialog)
{
  struct cm_parameter param;

  if (!allow_refreshes) {
    return;
  }
  
  cmafec_get_fe_parameter(pdialog->pcity, &param);
  output_type_iterate(i) {
    param.minimal_surplus[i] = ScrollBar_GetPos(pdialog->minimal_surplus[i]);
    param.factor[i] = ScrollBar_GetPos(pdialog->factor[i]);
  } output_type_iterate_end;
  
  param.require_happy = (Button_GetCheck(pdialog->happy) == BST_CHECKED) ? 1 : 0;
  param.happy_factor = ScrollBar_GetPos(pdialog->factor[O_LAST]);
  
  /* save the change */
  cmafec_set_fe_parameter(pdialog->pcity, &param);

  /* refreshes the cma */
  if (cma_is_city_under_agent(pdialog->pcity, NULL)) {
    cma_release_city(pdialog->pcity);
    cma_put_city_under_agent(pdialog->pcity, &param);

    /* unfog the city map if we were unable to put back under */
    if (!cma_is_city_under_agent(pdialog->pcity, NULL)) {
      refresh_city_dialog(pdialog->pcity);
      return;                   /* refreshing city refreshes cma */
    } else {
      city_report_dialog_update_city(pdialog->pcity);
    }
  }

  refresh_cma_dialog(pdialog->pcity, DONT_REFRESH_HSCALES);

}

/**************************************************************************

**************************************************************************/
static void handle_hscroll(HWND win, HWND winctl, UINT code, int pos) 
{
  char buf[16];
  int poscur, posmax, posmin, id;
  poscur = ScrollBar_GetPos(winctl);
  ScrollBar_GetRange(winctl,&posmin,&posmax);
  switch(code)
    {
    case SB_LINELEFT: poscur--; break;
    case SB_LINERIGHT: poscur++; break;
    case SB_PAGELEFT: poscur -= (posmax - posmin + 1) / 10; break;
    case SB_PAGERIGHT: poscur += (posmax - posmin + 1) / 10; break;
    case SB_LEFT: poscur = posmin; break;
    case SB_RIGHT: poscur = posmax; break;
    case SB_THUMBTRACK: poscur = pos; break; 
    default:
      return;
    }
  id = GetDlgCtrlID(winctl);
  if (poscur < posmin) poscur = posmin;
  if (poscur > posmax) poscur = posmax;
  ScrollBar_SetPos(winctl, poscur, TRUE);
  my_snprintf(buf, sizeof(buf), "%d", poscur);
  SetWindowText(GetNextSibling(winctl), buf);

}

/**************************************************************************
  CMA message proc
**************************************************************************/
LONG CALLBACK cma_proc(HWND win, UINT message,
		       WPARAM wParam, LPARAM lParam)
{
  struct cma_gui_initdata *guidata;
  guidata = fcwin_get_user_data(win);
  switch(message) 
    {
    case WM_CREATE:
      guidata->pdialog = create_cma_gui(win);
      guidata->pdialog->pcity = guidata->pcity;
      update_cma_preset_list(guidata->pdialog);
      break;
    case WM_DESTROY:
      break;
      if (guidata) {
	dialog_list_unlink(dialog_list, guidata->pdialog);
	free(guidata->pdialog);
	free(guidata);
      }
      fcwin_set_user_data(win, NULL);
      break;
    case WM_SIZE:
    case WM_GETMINMAXINFO:
      break; 
    case WM_HSCROLL:
      if (!guidata->pdialog)
	break;
      HANDLE_WM_HSCROLL(win,wParam,lParam,handle_hscroll);      
      hscale_changed(guidata->pdialog);
      break;
    case WM_COMMAND:
      if (!guidata->pdialog)
	break;
      switch((enum cmagui_ids)LOWORD(wParam)) 
	{
	case CMAGUI_PRESET_LIST:
	  cma_select_preset_callback(guidata->pdialog);
	  break;
	case CMAGUI_PRESET_ADD:
	  cma_add_preset_callback(guidata->pdialog);
	  break;
	case CMAGUI_PRESET_DEL:
	  cma_del_preset_callback(guidata->pdialog);
	  break;
	case CMAGUI_CHANGE:
	  cma_change_to_callback(guidata->pdialog);
	  break;
	case CMAGUI_PERM:
	  cma_change_permanent_to_callback(guidata->pdialog);
	  break;
	case CMAGUI_RELEASE:
	  cma_release_callback(guidata->pdialog);
	  break;
	case CMAGUI_HAPPY:
	  hscale_changed(guidata->pdialog);
	  break;
	default:
	  break;
	}
      break;
    default:
      return DefWindowProc(win, message, wParam, lParam);
    }
  return 0;
}
