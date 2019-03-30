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
/***********************************************************************
                          wldlg.c  -  description
                             -------------------
    begin                : Wed Sep 18 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdlib.h>

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "movement.h"
#include "unit.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "global_worklist.h"

/* gui-sdl2 */
#include "citydlg.h"
#include "colors.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "helpdlg.h"
#include "mapview.h"
#include "sprite.h"
#include "themespec.h"
#include "widget.h"

#include "wldlg.h"

#define TARGETS_COL 4
#define TARGETS_ROW 4

struct EDITOR {
  struct widget *pBeginWidgetList;
  struct widget *pEndWidgetList; /* window */

  struct ADVANCED_DLG *pTargets;
  struct ADVANCED_DLG *pWork;
  struct ADVANCED_DLG *pGlobal;

  struct city *pCity;
  int global_worklist_id;
  struct worklist worklist_copy;
  char worklist_name[MAX_LEN_NAME];

  /* shortcuts  */
  struct widget *pDock;
  struct widget *pWorkList_Counter;

  struct widget *pProduction_Name;
  struct widget *pProduction_Progres;

  int stock;
  struct universal currently_building;
} *pEditor = NULL;


static int worklist_editor_item_callback(struct widget *pWidget);
static SDL_Surface *get_progress_icon(int stock, int cost, int *progress);
static const char *get_production_name(struct city *pCity,
                                       struct universal prod, int *cost);
static void refresh_worklist_count_label(void);
static void refresh_production_label(int stock);

/* =========================================================== */

/**********************************************************************//**
  Worklist Editor Window Callback
**************************************************************************/
static int window_worklist_editor_callback(struct widget *pWidget)
{
  return -1;
}

/**********************************************************************//**
  Popdwon Worklist Editor
**************************************************************************/
static int popdown_worklist_editor_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_worklist_editor();
  }

  return -1;
}

/**********************************************************************//**
  Commit changes to city/global worklist.
  In City Mode Remove Double entry of Imprv/Woder Targets from list.
**************************************************************************/
static int ok_worklist_editor_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int i, j;
    struct city *pCity = pEditor->pCity;

    /* remove duplicate entry of impv./wonder target from worklist */
    for (i = 0; i < worklist_length(&pEditor->worklist_copy); i++) {
      if (VUT_IMPROVEMENT == pEditor->worklist_copy.entries[i].kind) {
        for (j = i + 1; j < worklist_length(&pEditor->worklist_copy); j++) {
          if (VUT_IMPROVEMENT == pEditor->worklist_copy.entries[j].kind
              && pEditor->worklist_copy.entries[i].value.building ==
              pEditor->worklist_copy.entries[j].value.building) {
            worklist_remove(&pEditor->worklist_copy, j);
          }
        }
      }
    }

    if (pCity) {
      /* remove duplicate entry of currently building impv./wonder from worklist */
      if (VUT_IMPROVEMENT == pEditor->currently_building.kind) {
        for (i = 0; i < worklist_length(&pEditor->worklist_copy); i++) {
          if (VUT_IMPROVEMENT == pEditor->worklist_copy.entries[i].kind
              && pEditor->worklist_copy.entries[i].value.building ==
              pEditor->currently_building.value.building) {
            worklist_remove(&pEditor->worklist_copy, i);
          }
        }
      }

      /* change production */
      if (!are_universals_equal(&pCity->production, &pEditor->currently_building)) {
        city_change_production(pCity, &pEditor->currently_building);
      }

      /* commit new city worklist */
      city_set_worklist(pCity, &pEditor->worklist_copy);
    } else {
      /* commit global worklist */
      struct global_worklist *pGWL = global_worklist_by_id(pEditor->global_worklist_id);

      if (pGWL) {
        global_worklist_set(pGWL, &pEditor->worklist_copy);
        global_worklist_set_name(pGWL, pEditor->worklist_name);
        update_worklist_report_dialog();
      }
    }

    /* popdown dialog */
    popdown_worklist_editor();
  }

  return -1;
}

/**********************************************************************//**
  Rename Global Worklist
**************************************************************************/
static int rename_worklist_editor_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pWidget->string_utf8->text != NULL) {
      fc_snprintf(pEditor->worklist_name, MAX_LEN_NAME, "%s",
                  pWidget->string_utf8->text);
    } else {
      /* empty input -> restore previous content */
      copy_chars_to_utf8_str(pWidget->string_utf8, pEditor->worklist_name);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }

  return -1;
}

/* ====================================================================== */

/**********************************************************************//**
  Add target to worklist.
**************************************************************************/
static void add_target_to_worklist(struct widget *pTarget)
{
  struct widget *pBuf = NULL, *pDock = NULL;
  utf8_str *pstr = NULL;
  int i;
  struct universal prod = cid_decode(MAX_ID - pTarget->ID);

  set_wstate(pTarget, FC_WS_SELECTED);
  widget_redraw(pTarget);
  widget_flush(pTarget);

  /* Deny adding currently building Impr/Wonder Target */
  if (pEditor->pCity
      && VUT_IMPROVEMENT == prod.kind
      && are_universals_equal(&prod, &pEditor->currently_building)) {
    return;
  }

  if (worklist_length(&pEditor->worklist_copy) >= MAX_LEN_WORKLIST - 1) {
    return;
  }

  for (i = 0; i < worklist_length(&pEditor->worklist_copy); i++) {
    if (VUT_IMPROVEMENT == prod.kind
        && are_universals_equal(&prod, &pEditor->worklist_copy.entries[i])) {
      return;
    }
  }

  worklist_append(&pEditor->worklist_copy, &prod);

  /* create widget entry */
  if (VUT_UTYPE == prod.kind) {
    pstr = create_utf8_from_char(utype_name_translation(prod.value.utype), adj_font(10));
  } else {
    pstr = create_utf8_from_char(city_improvement_name_translation(pEditor->pCity,
                                                                   prod.value.building),
                                 adj_font(10));
  }

  pstr->style |= SF_CENTER;
  pBuf = create_iconlabel(NULL, pTarget->dst, pstr,
                          (WF_RESTORE_BACKGROUND|WF_FREE_DATA));

  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = worklist_editor_item_callback;

  pBuf->data.ptr = fc_calloc(1, sizeof(int));
  *((int *)pBuf->data.ptr) = worklist_length(&pEditor->worklist_copy) - 1;

  pBuf->ID = MAX_ID - cid_encode(prod);

  if (pEditor->pWork->pBeginActiveWidgetList) {
    pDock = pEditor->pWork->pBeginActiveWidgetList;
  } else {
    pDock = pEditor->pDock;
  }

/* FIXME */
#if 0
  if (worklist_length(&pEditor->worklist_copy) > pEditor->pWork->pScroll->active + 1) {

    setup_vertical_widgets_position(1,
      pEditor->pEndWidgetList->area.x + adj_size(2),
      get_widget_pointer_form_main_list(ID_WINDOW)->area.y + adj_size(152) +
        pEditor->pWork->pScroll->pUp_Left_Button->size.h + 1,
      adj_size(126), 0, pEditor->pWork->pBeginWidgetList,
      pEditor->pWork->pEndWidgetList);

    setup_vertical_scrollbar_area(pEditor->pWork->pScroll,
        pEditor->pEndWidgetList->area.x + adj_size(2),
        get_widget_pointer_form_main_list(ID_WINDOW)->area.y + adj_size(152),
        adj_size(225), FALSE);

    show_scrollbar(pEditor->pWork->pScroll);
  }
#endif /* 0 */

  add_widget_to_vertical_scroll_widget_list(pEditor->pWork, pBuf,
                                            pDock, FALSE,
                                            pEditor->pEndWidgetList->area.x + adj_size(2),
                                            pEditor->pEndWidgetList->area.y + adj_size(152));

  pBuf->size.w = adj_size(126);

  refresh_worklist_count_label();
  redraw_group(pEditor->pWork->pBeginWidgetList,
               pEditor->pWork->pEndWidgetList, TRUE);
  flush_dirty();
}

/**********************************************************************//**
  Find if two targets are the same class (unit, imprv. , wonder).
  This is needed by calculation of change production shields penalty.
  [similar to are_universals_equal()]
**************************************************************************/
static bool are_prods_same_class(const struct universal *one,
                                 const struct universal *two)
{
  if (one->kind != two->kind) {
    return FALSE;
  }

  if (VUT_IMPROVEMENT == one->kind) {
    if (is_wonder(one->value.building)) {
      return is_wonder(two->value.building);
    } else {
      return !is_wonder(two->value.building);
    }
  }

  return FALSE;
}

/**********************************************************************//**
  Change production in editor shell, calculate production shields penalty and
  refresh production progress label
**************************************************************************/
static void change_production(struct universal *prod)
{
  if (!are_prods_same_class(&pEditor->currently_building, prod)) {
    if (pEditor->stock != pEditor->pCity->shield_stock) {
      if (are_prods_same_class(&pEditor->pCity->production, prod)) {
        pEditor->stock = pEditor->pCity->shield_stock;
      }
    } else {
      pEditor->stock =
        city_change_production_penalty(pEditor->pCity, prod);
    }
  }

  pEditor->currently_building = *prod;
  refresh_production_label(pEditor->stock);
}

/**********************************************************************//**
  Change production of city but only in Editor.
  You must commit this changes by exit editor via OK button (commit function).

  This function don't remove double imprv./wonder target entry from worklist
  and allow more entry of such target (In Production and worklist - this is
  fixed by commit function).
**************************************************************************/
static void add_target_to_production(struct widget *pTarget)
{
  int dummy;
  struct universal prod;

  fc_assert_ret(pTarget != NULL);

  /* redraw Target Icon */
  set_wstate(pTarget, FC_WS_SELECTED);
  widget_redraw(pTarget);
  widget_flush(pTarget);

  prod = cid_decode(MAX_ID - pTarget->ID);

  /* check if we change to the same target */
  if (are_universals_equal(&prod, &pEditor->currently_building)) {
    /* comit changes and exit - double click detection */
    ok_worklist_editor_callback(NULL);
    return;
  }

  change_production(&prod);

  /* change Production Text Label in Worklist Widget list */
  copy_chars_to_utf8_str(pEditor->pWork->pEndActiveWidgetList->string_utf8,
                         get_production_name(pEditor->pCity, prod, &dummy));

  pEditor->pWork->pEndActiveWidgetList->ID = MAX_ID - cid_encode(prod);

  widget_redraw(pEditor->pWork->pEndActiveWidgetList);
  widget_mark_dirty(pEditor->pWork->pEndActiveWidgetList);

  flush_dirty();
}

/**********************************************************************//**
  Get Help Info about target
**************************************************************************/
static void get_target_help_data(struct widget *pTarget)
{
  struct universal prod;

  fc_assert_ret(pTarget != NULL);

  /* redraw Target Icon */
  set_wstate(pTarget, FC_WS_SELECTED);
  widget_redraw(pTarget);
  /*widget_flush(pTarget);*/

  prod = cid_decode(MAX_ID - pTarget->ID);

  if (VUT_UTYPE == prod.kind) {
    popup_unit_info(utype_number(prod.value.utype));
  } else {
    popup_impr_info(improvement_number(prod.value.building));
  }
}

/**********************************************************************//**
  Targets callback
  left mouse button -> In city mode change production to target.
                       In global mode add target to worklist.
  middle mouse button -> get target "help"
  right mouse button -> add target to worklist.
**************************************************************************/
static int worklist_editor_targets_callback(struct widget *pWidget)
{
  switch (Main.event.button.button) {
  case SDL_BUTTON_LEFT:
    if (pEditor->pCity) {
      add_target_to_production(pWidget);
    } else {
      add_target_to_worklist(pWidget);
    }
    break;
  case SDL_BUTTON_MIDDLE:
    get_target_help_data(pWidget);
    break;
  case SDL_BUTTON_RIGHT:
    add_target_to_worklist(pWidget);
    break;
  default:
    /* do nothing */
    break;
  }

  return -1;
}

/* ====================================================================== */

/**********************************************************************//**
  Remove element from worklist or
  remove currently building imprv/unit and change production to first worklist
  element (or Capitalizations if worklist is empty)
**************************************************************************/
static void remove_item_from_worklist(struct widget *pItem)
{
  /* only one item (production) is left */
  if (worklist_is_empty(&pEditor->worklist_copy)) {
    return;
  }

  if (pItem->data.ptr) {
    /* correct "data" widget fiels */
    struct widget *pBuf = pItem;

    if (pBuf != pEditor->pWork->pBeginActiveWidgetList) {
      do {
        pBuf = pBuf->prev;
        *((int *)pBuf->data.ptr) = *((int *)pBuf->data.ptr) - 1;
      } while (pBuf != pEditor->pWork->pBeginActiveWidgetList);
    }

    /* remove element from worklist */
    worklist_remove(&pEditor->worklist_copy, *((int *)pItem->data.ptr));

    /* remove widget from widget list */
    del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pItem);
  } else {
    /* change productions to first worklist element */
    struct widget *pBuf = pItem->prev;

    change_production(&pEditor->worklist_copy.entries[0]);
    worklist_advance(&pEditor->worklist_copy);
    del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pItem);
    FC_FREE(pBuf->data.ptr);
    if (pBuf != pEditor->pWork->pBeginActiveWidgetList) {
      do {
        pBuf = pBuf->prev;
        *((int *)pBuf->data.ptr) = *((int *)pBuf->data.ptr) - 1;
      } while (pBuf != pEditor->pWork->pBeginActiveWidgetList);
    }
  }

/* FIXME: fix scrollbar code */
#if 0
  /* worklist_length(&pEditor->worklist_copy): without production */
  if (worklist_length(&pEditor->worklist_copy) <= pEditor->pWork->pScroll->active + 1) {

    setup_vertical_widgets_position(1,
      pEditor->pEndWidgetList->area.x + adj_size(2),
      get_widget_pointer_form_main_list(ID_WINDOW)->area.y + adj_size(152),
	adj_size(126), 0, pEditor->pWork->pBeginWidgetList,
      pEditor->pWork->pEndWidgetList);
#if 0
    setup_vertical_scrollbar_area(pEditor->pWork->pScroll,
	pEditor->pEndWidgetList->area.x + adj_size(2),
    	get_widget_pointer_form_main_list(ID_WINDOW)->area.y + adj_size(152),
    	adj_size(225), FALSE);*/
#endif /* 0 */
    hide_scrollbar(pEditor->pWork->pScroll);
  }
#endif /* 0 */

  refresh_worklist_count_label();
  redraw_group(pEditor->pWork->pBeginWidgetList,
               pEditor->pWork->pEndWidgetList, TRUE);
  flush_dirty();
}

/**********************************************************************//**
  Swap worklist entries DOWN.
  Fuction swap current element with next element of worklist.

  If pItem is last widget or there is only one widget on widgets list
  fuction remove this widget from widget list and target from worklist

  In City mode, when pItem is first worklist element, function make
  change production (currently building is moved to first element of worklist
  and first element of worklist is build).
**************************************************************************/
static void swap_item_down_from_worklist(struct widget *pItem)
{
  char *text;
  Uint16 ID;
  bool changed = FALSE;
  struct universal tmp;

  if (pItem == pEditor->pWork->pBeginActiveWidgetList) {
    remove_item_from_worklist(pItem);
    return;
  }

  text = pItem->string_utf8->text;
  ID = pItem->ID;

  /* second item or higher was clicked */
  if (pItem->data.ptr) {
    /* worklist operations -> swap down */
    int row = *((int *)pItem->data.ptr);

    tmp = pEditor->worklist_copy.entries[row];
    pEditor->worklist_copy.entries[row] = pEditor->worklist_copy.entries[row + 1];
    pEditor->worklist_copy.entries[row + 1] = tmp;

    changed = TRUE;
  } else {
    /* first item was clicked -> change production */
    change_production(&pEditor->worklist_copy.entries[0]);
    pEditor->worklist_copy.entries[0] = cid_decode(MAX_ID - ID);
    changed = TRUE;
  }

  if (changed) {
    pItem->string_utf8->text = pItem->prev->string_utf8->text;
    pItem->ID = pItem->prev->ID;

    pItem->prev->string_utf8->text = text;
    pItem->prev->ID = ID;

    redraw_group(pEditor->pWork->pBeginWidgetList,
                 pEditor->pWork->pEndWidgetList, TRUE);
    flush_dirty();
  }
}

/**********************************************************************//**
  Swap worklist entries UP.
  Fuction swap current element with prev. element of worklist.

  If pItem is first widget on widgets list fuction remove this widget
  from widget list and target from worklist (global mode)
  or from production (city mode)

  In City mode, when pItem is first worklist element, function make
  change production (currently building is moved to first element of worklist
  and first element of worklist is build).
**************************************************************************/
static void swap_item_up_from_worklist(struct widget *pItem)
{
  char *text = pItem->string_utf8->text;
  Uint16 ID = pItem->ID;
  bool changed = FALSE;
  struct universal tmp;

  /* first item was clicked -> remove */
  if (pItem == pEditor->pWork->pEndActiveWidgetList) {
    remove_item_from_worklist(pItem);
    return;
  }

  /* third item or higher was clicked */
  if (pItem->data.ptr && *((int *)pItem->data.ptr) > 0) {
    /* worklist operations -> swap up */
    int row = *((int *)pItem->data.ptr);

    tmp = pEditor->worklist_copy.entries[row];
    pEditor->worklist_copy.entries[row] = pEditor->worklist_copy.entries[row - 1];
    pEditor->worklist_copy.entries[row - 1] = tmp;

    changed = TRUE;
  } else {
    /* second item was clicked -> change production ... */
    tmp = pEditor->currently_building;
    change_production(&pEditor->worklist_copy.entries[0]);
    pEditor->worklist_copy.entries[0] = tmp;

    changed = TRUE;
  }

  if (changed) {
    pItem->string_utf8->text = pItem->next->string_utf8->text;
    pItem->ID = pItem->next->ID;

    pItem->next->string_utf8->text = text;
    pItem->next->ID = ID;

    redraw_group(pEditor->pWork->pBeginWidgetList,
                 pEditor->pWork->pEndWidgetList, TRUE);
    flush_dirty();
  }
}

/**********************************************************************//**
  worklist callback
  left mouse button -> swap entries up.
  middle mouse button -> remove element from list
  right mouse button -> swap entries down.
**************************************************************************/
static int worklist_editor_item_callback(struct widget *pWidget)
{
  switch (Main.event.button.button) {
  case SDL_BUTTON_LEFT:
    swap_item_up_from_worklist(pWidget);
    break;
  case SDL_BUTTON_MIDDLE:
    remove_item_from_worklist(pWidget);
    break;
  case SDL_BUTTON_RIGHT:
    swap_item_down_from_worklist(pWidget);
    break;
  default:
    ;/* do nothing */
    break;
  }

  return -1;
}
/* ======================================================= */

/**********************************************************************//**
  Add global worklist to city worklist starting from last free entry.
  Add only avilable targets in current game state.
  If global worklist have more targets that city worklist have free
  entries then we adding only first part of global worklist.
**************************************************************************/
static void add_global_worklist(struct widget *pWidget)
{
  struct global_worklist *pGWL = global_worklist_by_id(MAX_ID - pWidget->ID);
  struct widget *pBuf = pEditor->pWork->pEndActiveWidgetList;
  const struct worklist *pWorkList;
  int count, firstfree;

  if (!pGWL
      || !(pWorkList = global_worklist_get(pGWL))
      || worklist_is_empty(pWorkList)) {
    return;
  }

  if (worklist_length(&pEditor->worklist_copy) >= MAX_LEN_WORKLIST - 1) {
    /* worklist is full */
    return;
  }

  firstfree = worklist_length(&pEditor->worklist_copy);
  /* copy global worklist to city worklist */
  for (count = 0 ; count < worklist_length(pWorkList); count++) {
    /* global worklist can have targets unavilable in current state of game
       then we must remove those targets from new city worklist */
    if (!can_city_build_later(pEditor->pCity, &pWorkList->entries[count])) {
      continue;
    }

    worklist_append(&pEditor->worklist_copy, &pWorkList->entries[count]);

    /* create widget */
    if (VUT_UTYPE == pWorkList->entries[count].kind) {
      pBuf = create_iconlabel(NULL, pWidget->dst,
                              create_utf8_from_char(
                      utype_name_translation(pWorkList->entries[count].value.utype),
                      adj_font(10)),
                              (WF_RESTORE_BACKGROUND|WF_FREE_DATA));
      pBuf->ID = MAX_ID - cid_encode_unit(pWorkList->entries[count].value.utype);
    } else {
      pBuf = create_iconlabel(NULL, pWidget->dst,
                              create_utf8_from_char(
                      city_improvement_name_translation(pEditor->pCity,
                                                        pWorkList->entries[count].value.building),
                      adj_font(10)),
                              (WF_RESTORE_BACKGROUND|WF_FREE_DATA));
      pBuf->ID = MAX_ID - cid_encode_building(pWorkList->entries[count].value.building);
    }

    pBuf->string_utf8->style |= SF_CENTER;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = worklist_editor_item_callback;
    pBuf->size.w = adj_size(126);
    pBuf->data.ptr = fc_calloc(1, sizeof(int));
    *((int *)pBuf->data.ptr) = firstfree;

    add_widget_to_vertical_scroll_widget_list(pEditor->pWork,
                                              pBuf, pEditor->pWork->pBeginActiveWidgetList,
                                              FALSE,
                                              pEditor->pEndWidgetList->area.x + adj_size(2),
                                              pEditor->pEndWidgetList->area.y + adj_size(152));

    firstfree++;
    if (firstfree == MAX_LEN_WORKLIST - 1) {
      break;
    }
  }

  refresh_worklist_count_label();
  redraw_group(pEditor->pWork->pBeginWidgetList,
               pEditor->pWork->pEndWidgetList, TRUE);

  flush_dirty();
}

/**********************************************************************//**
  Clear city worklist and copy here global worklist.
  Copy only available targets in current game state.
  If all targets are unavilable then leave city worklist untouched.
**************************************************************************/
static void set_global_worklist(struct widget *pWidget)
{
  struct global_worklist *pGWL = global_worklist_by_id(MAX_ID - pWidget->ID);
  struct widget *pBuf = pEditor->pWork->pEndActiveWidgetList;
  const struct worklist *pWorkList;
  struct worklist wl;
  int count, wl_count;
  struct universal target;

  if (!pGWL
      || !(pWorkList = global_worklist_get(pGWL))
      || worklist_is_empty(pWorkList)) {
    return;
  }

  /* clear tmp worklist */
  worklist_init(&wl);

  wl_count = 0;
  /* copy global worklist to city worklist */
  for (count = 0; count < worklist_length(pWorkList); count++) {
    /* global worklist can have targets unavilable in current state of game
       then we must remove those targets from new city worklist */
    if (!can_city_build_later(pEditor->pCity, &pWorkList->entries[count])) {
      continue;
    }

    wl.entries[wl_count] = pWorkList->entries[count];
    wl_count++;
  }
  /* --------------------------------- */

  if (!worklist_is_empty(&wl)) {
    /* free old widget list */
    if (pBuf != pEditor->pWork->pBeginActiveWidgetList) {
      pBuf = pBuf->prev;
      if (pBuf != pEditor->pWork->pBeginActiveWidgetList) {
        do {
          pBuf = pBuf->prev;
          del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pBuf->next);
        } while (pBuf != pEditor->pWork->pBeginActiveWidgetList);
      }
      del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pBuf);
    }
    /* --------------------------------- */

    worklist_copy(&pEditor->worklist_copy, &wl);

    /* --------------------------------- */
    /* create new widget list */
    for (count = 0; count < MAX_LEN_WORKLIST; count++) {
      /* end of list */
      if (!worklist_peek_ith(&pEditor->worklist_copy, &target, count)) {
        break;
      }

      if (VUT_UTYPE == target.kind) {
        pBuf = create_iconlabel(NULL, pWidget->dst,
          create_utf8_from_char(utype_name_translation(target.value.utype),
                                adj_font(10)),
                                (WF_RESTORE_BACKGROUND|WF_FREE_DATA));
        pBuf->ID = MAX_ID - B_LAST - utype_number(target.value.utype);
      } else {
        pBuf = create_iconlabel(NULL, pWidget->dst,
          create_utf8_from_char(city_improvement_name_translation(pEditor->pCity,
                                                                  target.value.building),
                                adj_font(10)),
                                (WF_RESTORE_BACKGROUND|WF_FREE_DATA));
        pBuf->ID = MAX_ID - improvement_number(target.value.building);
      }
      pBuf->string_utf8->style |= SF_CENTER;
      set_wstate(pBuf, FC_WS_NORMAL);
      pBuf->action = worklist_editor_item_callback;
      pBuf->size.w = adj_size(126);
      pBuf->data.ptr = fc_calloc(1, sizeof(int));
      *((int *)pBuf->data.ptr) = count;

      add_widget_to_vertical_scroll_widget_list(pEditor->pWork,
        pBuf, pEditor->pWork->pBeginActiveWidgetList, FALSE,
        pEditor->pEndWidgetList->area.x + adj_size(2),
        pEditor->pEndWidgetList->area.y + adj_size(152));
    }

    refresh_worklist_count_label();
    redraw_group(pEditor->pWork->pBeginWidgetList,
                 pEditor->pWork->pEndWidgetList, TRUE);

    flush_dirty();
  }
}

/**********************************************************************//**
  Global worklist callback
  left mouse button -> add global worklist to current city list
  right mouse button -> clear city worklist and copy here global worklist.

  There are problems with impv./wonder tagets becouse those can't be doubled
  on worklist and adding/seting can give you situation that global worklist
  have imprv./wonder entry that exist on city worklist or in building state.
  I don't make such check here and allow this "functionality" becouse doubled
  impov./wonder entry are removed from city worklist during "commit" phase.
**************************************************************************/
static int global_worklist_callback(struct widget *pWidget)
{
  switch (Main.event.button.button) {
  case SDL_BUTTON_LEFT:
    add_global_worklist(pWidget);
    break;
  case SDL_BUTTON_MIDDLE:
    /* nothing */
    break;
  case SDL_BUTTON_RIGHT:
    set_global_worklist(pWidget);
    break;
  default:
    /* do nothing */
    break;
  }

  return -1;
}

/* ======================================================= */

/**********************************************************************//**
  Return full unit/imprv. name and build cost in "cost" pointer.
**************************************************************************/
static const char *get_production_name(struct city *pCity,
                                       struct universal prod, int *cost)
{
  fc_assert_ret_val(cost != NULL, NULL);

  *cost = universal_build_shield_cost(pCity, &prod);

  if (VUT_UTYPE == prod.kind) {
    return utype_name_translation(prod.value.utype);
  } else {
    return city_improvement_name_translation(pCity, prod.value.building);
  }
}

/**********************************************************************//**
  Return progress icon (bar) of current production state
  stock - current shields stocks
  cost - unit/imprv. cost
  function return in "progress" pointer (0 - 100 %) progress in numbers
**************************************************************************/
static SDL_Surface *get_progress_icon(int stock, int cost, int *progress)
{
  SDL_Surface *pIcon = NULL;
  int width;

  fc_assert_ret_val(progress != NULL, NULL);

  if (stock < cost) {
    width = ((float)stock / cost) * adj_size(116.0);
    *progress = ((float)stock / cost) * 100.0;
    if (!width && stock) {
      *progress = 1;
      width = 1;
    }
  } else {
    width = adj_size(116);
    *progress = 100;
  }

  pIcon = create_bcgnd_surf(current_theme->Edit, 0, adj_size(120), adj_size(30));

  if (width) {
    SDL_Rect dst = {2,1,0,0};
    SDL_Surface *pBuf = create_bcgnd_surf(current_theme->Button, 3, width,
                                          adj_size(28));

    alphablit(pBuf, NULL, pIcon, &dst, 255);
    FREESURFACE(pBuf);
  }

  return pIcon;
}

/**********************************************************************//**
  Update and redraw production name label in worklist editor.
  stock - pCity->shields_stock or current stock after change production lost.
**************************************************************************/
static void refresh_production_label(int stock)
{
  int cost, turns;
  char cBuf[64];
  SDL_Rect area;
  bool gold_prod = improvement_has_flag(pEditor->currently_building.value.building, IF_GOLD);
  const char *name = get_production_name(pEditor->pCity,
                                         pEditor->currently_building, &cost);

  if (VUT_IMPROVEMENT == pEditor->currently_building.kind && gold_prod) {
    int gold = MAX(0, pEditor->pCity->surplus[O_SHIELD]);

    fc_snprintf(cBuf, sizeof(cBuf),
                PL_("%s\n%d gold per turn",
                    "%s\n%d gold per turn", gold),
                name, gold);
  } else {
    if (stock < cost) {
      turns = city_turns_to_build(pEditor->pCity,
                                  &pEditor->currently_building, TRUE);
      if (turns == 999) {
        fc_snprintf(cBuf, sizeof(cBuf), _("%s\nblocked!"), name);
      } else {
        fc_snprintf(cBuf, sizeof(cBuf), _("%s\n%d %s"),
                    name, turns, PL_("turn", "turns", turns));
      }
    } else {
      fc_snprintf(cBuf, sizeof(cBuf), _("%s\nfinished!"), name);
    }
  }
  copy_chars_to_utf8_str(pEditor->pProduction_Name->string_utf8, cBuf);

  widget_undraw(pEditor->pProduction_Name);
  remake_label_size(pEditor->pProduction_Name);

  pEditor->pProduction_Name->size.x = pEditor->pEndWidgetList->area.x +
    (adj_size(130) - pEditor->pProduction_Name->size.w)/2;

  area.x = pEditor->pEndWidgetList->area.x;
  area.y = pEditor->pProduction_Name->size.y;
  area.w = adj_size(130);
  area.h = pEditor->pProduction_Name->size.h;

  if (get_wflags(pEditor->pProduction_Name) & WF_RESTORE_BACKGROUND) {
    refresh_widget_background(pEditor->pProduction_Name);
  }

  widget_redraw(pEditor->pProduction_Name);
  dirty_sdl_rect(&area);

  FREESURFACE(pEditor->pProduction_Progres->theme);
  pEditor->pProduction_Progres->theme =
    get_progress_icon(stock, cost, &cost);

  if (!gold_prod) {
    fc_snprintf(cBuf, sizeof(cBuf), "%d%%" , cost);
  } else {
    fc_snprintf(cBuf, sizeof(cBuf), "-");
  }
  copy_chars_to_utf8_str(pEditor->pProduction_Progres->string_utf8, cBuf);
  widget_redraw(pEditor->pProduction_Progres);
  widget_mark_dirty(pEditor->pProduction_Progres);
}

/**********************************************************************//**
  Update and redraw worklist length counter in worklist editor
**************************************************************************/
static void refresh_worklist_count_label(void)
{
  char cBuf[64];
  SDL_Rect area;
  int external_entries;

  if (pEditor->pCity != NULL) {
    external_entries = 1; /* Current production */
  } else {
    external_entries = 0;
  }

  /* TRANS: length of worklist */
  fc_snprintf(cBuf, sizeof(cBuf), _("( %d entries )"),
              worklist_length(&pEditor->worklist_copy) + external_entries);
  copy_chars_to_utf8_str(pEditor->pWorkList_Counter->string_utf8, cBuf);

  widget_undraw(pEditor->pWorkList_Counter);
  remake_label_size(pEditor->pWorkList_Counter);

  pEditor->pWorkList_Counter->size.x = pEditor->pEndWidgetList->area.x +
    (adj_size(130) - pEditor->pWorkList_Counter->size.w)/2;

  if (get_wflags(pEditor->pWorkList_Counter) & WF_RESTORE_BACKGROUND) {
    refresh_widget_background(pEditor->pWorkList_Counter);
  }

  widget_redraw(pEditor->pWorkList_Counter);

  area.x = pEditor->pEndWidgetList->area.x;
  area.y = pEditor->pWorkList_Counter->size.y;
  area.w = adj_size(130);
  area.h = pEditor->pWorkList_Counter->size.h;
  dirty_sdl_rect(&area);
}

/* ====================================================================== */

/**********************************************************************//**
  Global/City worklist editor.
  if pCity == NULL then fucnction take pWorklist as global worklist.
  pWorklist must be not NULL.
**************************************************************************/
void popup_worklist_editor(struct city *pCity, struct global_worklist *gwl)
{
  SDL_Color bg_color = {255,255,255,128};
  SDL_Color bg_color2 = {255,255,255,136};
  int count = 0, turns;
  int widget_w = 0, widget_h = 0;
  utf8_str *pstr = NULL;
  struct widget *pBuf = NULL, *pWindow, *pLast;
  SDL_Surface *pText = NULL, *pText_Name = NULL, *pZoom = NULL;
  SDL_Surface *pMain;
  SDL_Surface *pIcon;
  SDL_Rect dst;
  char cbuf[128];
  struct unit_type *pUnit = NULL;
  char *state = NULL;
  bool advanced_tech;
  bool can_build, can_eventually_build;
  SDL_Rect area;
  int external_entries;

  if (pEditor) {
    return;
  }

  pEditor = fc_calloc(1, sizeof(struct EDITOR));

  if (pCity) {
    pEditor->pCity = pCity;
    pEditor->global_worklist_id = -1;
    pEditor->currently_building = pCity->production;
    pEditor->stock = pCity->shield_stock;
    worklist_copy(&pEditor->worklist_copy, &pCity->worklist);
    fc_snprintf(pEditor->worklist_name, sizeof(pEditor->worklist_name),
                "%s worklist", city_name_get(pCity));
  } else if (gwl != NULL) {
    pEditor->pCity = NULL;
    pEditor->global_worklist_id = global_worklist_id(gwl);
    worklist_copy(&pEditor->worklist_copy, global_worklist_get(gwl));
    sz_strlcpy(pEditor->worklist_name, global_worklist_name(gwl));
  } else {
    /* Not valid variant! */
    return;
  }

  advanced_tech = (pCity == NULL);

  /* --------------- */
  /* create Target Background Icon */
  pMain = create_surf(adj_size(116), adj_size(116), SDL_SWSURFACE);
  SDL_FillRect(pMain, NULL, map_rgba(pMain->format, bg_color));

  create_frame(pMain,
               0, 0, pMain->w - 1, pMain->h - 1,
               get_theme_color(COLOR_THEME_WLDLG_FRAME));

  /* ---------------- */
  /* Create Main Window */
  pWindow = create_window_skeleton(NULL, NULL, 0);
  pWindow->action = window_worklist_editor_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_WINDOW, pWindow);
  pEditor->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ---------------- */
  if (pCity) {
    fc_snprintf(cbuf, sizeof(cbuf), _("Worklist of\n%s"), city_name_get(pCity));
    external_entries = 1; /* Current production */
  } else {
    fc_snprintf(cbuf, sizeof(cbuf), "%s", global_worklist_name(gwl));
    external_entries = 0;
  }

  pstr = create_utf8_from_char(cbuf, adj_font(12));
  pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);

  pBuf = create_iconlabel(NULL, pWindow->dst, pstr, WF_RESTORE_BACKGROUND);

  add_to_gui_list(ID_LABEL, pBuf);
  /* --------------------------- */

  /* TRANS: length of worklist */
  fc_snprintf(cbuf, sizeof(cbuf), _("( %d entries )"),
              worklist_length(&pEditor->worklist_copy) + external_entries);
  pstr = create_utf8_from_char(cbuf, adj_font(10));
  pstr->bgcol = (SDL_Color) {0, 0, 0, 0};
  pBuf = create_iconlabel(NULL, pWindow->dst, pstr, WF_RESTORE_BACKGROUND);
  pEditor->pWorkList_Counter = pBuf;
  add_to_gui_list(ID_LABEL, pBuf);
  /* --------------------------- */

  /* create production progress label or rename worklist edit */
  if (pCity) {
    /* count == cost */
    /* turns == progress */
    const char *name = city_production_name_translation(pCity);
    bool gold_prod = city_production_has_flag(pCity, IF_GOLD);

    count = city_production_build_shield_cost(pCity);

    if (gold_prod) {
      int gold = MAX(0, pCity->surplus[O_SHIELD]);

      fc_snprintf(cbuf, sizeof(cbuf),
                  PL_("%s\n%d gold per turn",
                      "%s\n%d gold per turn", gold),
                  name, gold);
    } else {
      if (pCity->shield_stock < count) {
        turns = city_production_turns_to_build(pCity, TRUE);
        if (turns == 999) {
          fc_snprintf(cbuf, sizeof(cbuf), _("%s\nblocked!"), name);
        } else {
          fc_snprintf(cbuf, sizeof(cbuf), _("%s\n%d %s"),
                      name, turns, PL_("turn", "turns", turns));
        }
      } else {
        fc_snprintf(cbuf, sizeof(cbuf), _("%s\nfinished!"), name);
      }
    }
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pBuf = create_iconlabel(NULL, pWindow->dst, pstr, WF_RESTORE_BACKGROUND);

    pEditor->pProduction_Name = pBuf;
    add_to_gui_list(ID_LABEL, pBuf);

    pIcon = get_progress_icon(pCity->shield_stock, count, &turns);

    if (!gold_prod) {
      fc_snprintf(cbuf, sizeof(cbuf), "%d%%" , turns);
    } else {
      fc_snprintf(cbuf, sizeof(cbuf), "-");
    }
    pstr = create_utf8_from_char(cbuf, adj_font(12));
    pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);

    pBuf = create_iconlabel(pIcon, pWindow->dst, pstr,
                            (WF_RESTORE_BACKGROUND|WF_ICON_CENTER|WF_FREE_THEME));

    pIcon = NULL;
    turns = 0;
    pEditor->pProduction_Progres = pBuf;
    add_to_gui_list(ID_LABEL, pBuf);
  } else {
    pBuf = create_edit_from_chars(NULL, pWindow->dst,
                                  global_worklist_name(gwl), adj_font(10),
                                  adj_size(120), WF_RESTORE_BACKGROUND);
    pBuf->action = rename_worklist_editor_callback;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(ID_EDIT, pBuf);
  }

  /* --------------------------- */
  /* Commit Widget */
  pBuf = create_themeicon(current_theme->OK_Icon, pWindow->dst, WF_RESTORE_BACKGROUND);

  pBuf->action = ok_worklist_editor_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_RETURN;

  add_to_gui_list(ID_BUTTON, pBuf);
  /* --------------------------- */
  /* Cancel Widget */
  pBuf = create_themeicon(current_theme->CANCEL_Icon, pWindow->dst,
                          WF_RESTORE_BACKGROUND);

  pBuf->action = popdown_worklist_editor_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_BUTTON, pBuf);
  /* --------------------------- */
  /* work list */

  /*
     pWidget->data filed will contains position of target in worklist all
     action on worklist (swap/romove/add) must correct this fields

     Production Widget Label in worklist Widget list
     will have this field NULL
  */

  pEditor->pWork = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  pEditor->pWork->pScroll = fc_calloc(1, sizeof(struct ScrollBar));
  pEditor->pWork->pScroll->count = 0;
  pEditor->pWork->pScroll->active = MAX_LEN_WORKLIST;
  pEditor->pWork->pScroll->step = 1;

/* FIXME: this should replace the 4 lines above, but
 *        pEditor->pWork->pEndWidgetList is not set yet */
#if 0
  create_vertical_scrollbar(pEditor->pWork, 1, MAX_LEN_WORKLIST, TRUE, TRUE);
#endif /* 0 */

  if (pCity) {
   /* Production Widget Label */
    pstr = create_utf8_from_char(city_production_name_translation(pCity), adj_font(10));
    turns = city_production_build_shield_cost(pCity);
    pstr->style |= SF_CENTER;
    pBuf = create_iconlabel(NULL, pWindow->dst, pstr, WF_RESTORE_BACKGROUND);

    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = worklist_editor_item_callback;

    add_to_gui_list(MAX_ID - cid_encode(pCity->production), pBuf);

    pEditor->pWork->pEndWidgetList = pBuf;
    pEditor->pWork->pBeginWidgetList = pBuf;
    pEditor->pWork->pEndActiveWidgetList = pEditor->pWork->pEndWidgetList;
    pEditor->pWork->pBeginActiveWidgetList = pEditor->pWork->pBeginWidgetList;
    pEditor->pWork->pScroll->count++;
  }

  pLast = pBuf;
  pEditor->pDock = pBuf;

  /* create Widget Labels of worklist entries */

  count = 0;

  worklist_iterate(&pEditor->worklist_copy, prod) {
    if (VUT_UTYPE == prod.kind) {
      pstr = create_utf8_from_char(utype_name_translation(prod.value.utype),
                                   adj_font(10));
    } else {
      pstr = create_utf8_from_char(city_improvement_name_translation(pCity,
                                                                     prod.value.building),
                                   adj_font(10));
    }
    pstr->style |= SF_CENTER;
    pBuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            (WF_RESTORE_BACKGROUND|WF_FREE_DATA));

    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = worklist_editor_item_callback;

    pBuf->data.ptr = fc_calloc(1, sizeof(int));
    *((int *)pBuf->data.ptr) = count;

    add_to_gui_list(MAX_ID - cid_encode(prod), pBuf);

    count++;

    if (count > pEditor->pWork->pScroll->active - 1) {
      set_wflag(pBuf, WF_HIDDEN);
    }

  } worklist_iterate_end;

  if (count) {
    if (!pCity) {
      pEditor->pWork->pEndWidgetList = pLast->prev;
      pEditor->pWork->pEndActiveWidgetList = pEditor->pWork->pEndWidgetList;
    }
    pEditor->pWork->pBeginWidgetList = pBuf;
    pEditor->pWork->pBeginActiveWidgetList = pEditor->pWork->pBeginWidgetList;
  } else {
    if (!pCity) {
      pEditor->pWork->pEndWidgetList = pLast;
    }
    pEditor->pWork->pBeginWidgetList = pLast;
  }

/* FIXME */
#if 0
  pEditor->pWork->pActiveWidgetList = pEditor->pWork->pEndActiveWidgetList;
  create_vertical_scrollbar(pEditor->pWork, 1,
                            pEditor->pWork->pScroll->active, FALSE, TRUE);
  pEditor->pWork->pScroll->pUp_Left_Button->size.w = adj_size(122);
  pEditor->pWork->pScroll->pDown_Right_Button->size.w = adj_size(122);

  /* count: without production */
  if (count <= pEditor->pWork->pScroll->active + 1) {
    if (count > 0) {
      struct widget *pTmp = pLast;

      do {
        pTmp = pTmp->prev;
        clear_wflag(pTmp, WF_HIDDEN);
      } while (pTmp != pBuf);
    }
    hide_scrollbar(pEditor->pWork->pScroll);
  }
#endif /* 0 */

  pEditor->pWork->pScroll->count += count;
  pLast = pEditor->pWork->pBeginWidgetList;

  /* --------------------------- */
  /* global worklists */
  if (pCity) {
    count = 0;

    global_worklists_iterate(iter_gwl) {
      pBuf = create_iconlabel_from_chars(NULL, pWindow->dst,
                                         global_worklist_name(iter_gwl),
                                         adj_font(10),
                                         WF_RESTORE_BACKGROUND);
      set_wstate(pBuf, FC_WS_NORMAL);
      add_to_gui_list(MAX_ID - global_worklist_id(iter_gwl), pBuf);
      pBuf->string_utf8->style |= SF_CENTER;
      pBuf->action = global_worklist_callback;
      pBuf->string_utf8->fgcol = bg_color;

      count++;

      if (count > 4) {
        set_wflag(pBuf, WF_HIDDEN);
      }
    } global_worklists_iterate_end;

    if (count) {
      pEditor->pGlobal = fc_calloc(1, sizeof(struct ADVANCED_DLG));
      pEditor->pGlobal->pEndWidgetList = pLast->prev;
      pEditor->pGlobal->pEndActiveWidgetList = pEditor->pGlobal->pEndWidgetList;
      pEditor->pGlobal->pBeginWidgetList = pBuf;
      pEditor->pGlobal->pBeginActiveWidgetList = pEditor->pGlobal->pBeginWidgetList;

      if (count > 6) {
        pEditor->pGlobal->pActiveWidgetList = pEditor->pGlobal->pEndActiveWidgetList;

        create_vertical_scrollbar(pEditor->pGlobal, 1, 4, FALSE, TRUE);
        pEditor->pGlobal->pScroll->pUp_Left_Button->size.w = adj_size(122);
        pEditor->pGlobal->pScroll->pDown_Right_Button->size.w = adj_size(122);
      } else {
        struct widget *pTmp = pLast;

        do {
          pTmp = pTmp->prev;
          clear_wflag(pTmp, WF_HIDDEN);
        } while (pTmp != pBuf);
      }

      pLast = pEditor->pGlobal->pBeginWidgetList;
    }
  }
  /* ----------------------------- */
  count = 0;
  /* Targets units and imprv. to build */
  pstr = create_utf8_str(NULL, 0, adj_font(10));
  pstr->style |= (SF_CENTER|TTF_STYLE_BOLD);
  pstr->bgcol = (SDL_Color) {0, 0, 0, 0};

  improvement_iterate(pImprove) {
    can_build = can_player_build_improvement_now(client.conn.playing, pImprove);
    can_eventually_build =
        can_player_build_improvement_later(client.conn.playing, pImprove);

    /* If there's a city, can the city build the improvement? */
    if (pCity) {
      can_build = can_build && can_city_build_improvement_now(pCity, pImprove);
      can_eventually_build = can_eventually_build
        && can_city_build_improvement_later(pCity, pImprove);
    }

    if ((advanced_tech && can_eventually_build)
        || (!advanced_tech && can_build)) {

      pIcon = crop_rect_from_surface(pMain, NULL);

      fc_snprintf(cbuf, sizeof(cbuf), "%s", improvement_name_translation(pImprove));
      copy_chars_to_utf8_str(pstr, cbuf);
      pstr->style |= TTF_STYLE_BOLD;

      if (is_improvement_redundant(pCity, pImprove)) {
        pstr->style |= TTF_STYLE_STRIKETHROUGH;
      }

      pText_Name = create_text_surf_smaller_than_w(pstr, pIcon->w - 4);

      if (is_wonder(pImprove)) {
        if (improvement_obsolete(client.conn.playing, pImprove, pCity)) {
          state = _("Obsolete");
        } else if (is_great_wonder(pImprove)) {
          if (great_wonder_is_built(pImprove)) {
            state = _("Built");
          } else if (great_wonder_is_destroyed(pImprove)) {
            state = _("Destroyed");
          } else {
            state = _("Great Wonder");
          }
        } else if (is_small_wonder(pImprove)) {
          if (small_wonder_is_built(client.conn.playing, pImprove)) {
            state = _("Built");
          } else {
            state = _("Small Wonder");
          }
        }
      } else {
        state = NULL;
      }

      if (pCity) {
        if (!improvement_has_flag(pImprove, IF_GOLD)) {
          struct universal univ = cid_production(cid_encode_building(pImprove));
          int cost = impr_build_shield_cost(pCity, pImprove);

          turns = city_turns_to_build(pCity, &univ, TRUE);

          if (turns == FC_INFINITY) {
            if (state) {
              fc_snprintf(cbuf, sizeof(cbuf), _("(%s)\n%d/%d %s\n%s"),
                          state, pCity->shield_stock, cost,
                          PL_("shield", "shields", cost),
                          _("never"));
            } else {
              fc_snprintf(cbuf, sizeof(cbuf), _("%d/%d %s\n%s"),
                          pCity->shield_stock, cost,
                          PL_("shield", "shields", cost), _("never"));
            }
          } else {
            if (state) {
              fc_snprintf(cbuf, sizeof(cbuf), _("(%s)\n%d/%d %s\n%d %s"),
                          state, pCity->shield_stock, cost,
                          PL_("shield", "shields", cost),
                          turns, PL_("turn", "turns", turns));
            } else {
              fc_snprintf(cbuf, sizeof(cbuf), _("%d/%d %s\n%d %s"),
                          pCity->shield_stock, cost,
                          PL_("shield", "shields", cost),
                          turns, PL_("turn", "turns", turns));
            }
          }
        } else {
          /* capitalization */
          int gold = MAX(0, pCity->surplus[O_SHIELD]);

          fc_snprintf(cbuf, sizeof(cbuf), PL_("%d gold per turn",
                                              "%d gold per turn", gold),
                      gold);
        }
      } else {
        /* non city mode */
        if (!improvement_has_flag(pImprove, IF_GOLD)) {
          int cost = impr_build_shield_cost(NULL, pImprove);

          if (state) {
            fc_snprintf(cbuf, sizeof(cbuf), _("(%s)\n%d %s"),
                        state, cost,
                        PL_("shield", "shields", cost));
          } else {
            fc_snprintf(cbuf, sizeof(cbuf), _("%d %s"),
                        cost,
                        PL_("shield", "shields", cost));
          }
        } else {
          fc_snprintf(cbuf, sizeof(cbuf), _("shields into gold"));
        }
      }

      copy_chars_to_utf8_str(pstr, cbuf);
      pstr->style &= ~TTF_STYLE_BOLD;
      pstr->style &= ~TTF_STYLE_STRIKETHROUGH;

      pText = create_text_surf_from_utf8(pstr);

      /*-----------------*/

      pZoom = get_building_surface(pImprove);
      pZoom = zoomSurface(pZoom, DEFAULT_ZOOM * ((float)54 / pZoom->w), DEFAULT_ZOOM * ((float)54 / pZoom->w), 1);
      dst.x = (pIcon->w - pZoom->w) / 2;
      dst.y = (pIcon->h/2 - pZoom->h) / 2;
      alphablit(pZoom, NULL, pIcon, &dst, 255);
      dst.y += pZoom->h;
      FREESURFACE(pZoom);

      dst.x = (pIcon->w - pText_Name->w) / 2;
      dst.y += ((pIcon->h - dst.y) - (pText_Name->h + pText->h)) / 2;
      alphablit(pText_Name, NULL, pIcon, &dst, 255);

      dst.x = (pIcon->w - pText->w) / 2;
      dst.y += pText_Name->h;
      alphablit(pText, NULL, pIcon, &dst, 255);

      FREESURFACE(pText);
      FREESURFACE(pText_Name);

      pBuf = create_icon2(pIcon, pWindow->dst,
                          WF_RESTORE_BACKGROUND|WF_FREE_THEME);
      set_wstate(pBuf, FC_WS_NORMAL);

      widget_w = MAX(widget_w, pBuf->size.w);
      widget_h = MAX(widget_h, pBuf->size.h);

      pBuf->data.city = pCity;
      add_to_gui_list(MAX_ID - improvement_number(pImprove), pBuf);
      pBuf->action = worklist_editor_targets_callback;

      if (count > (TARGETS_ROW * TARGETS_COL - 1)) {
        set_wflag(pBuf, WF_HIDDEN);
      }
      count++;
    }
  } improvement_iterate_end;

  /* ------------------------------ */

  unit_type_iterate(un) {
    can_build = can_player_build_unit_now(client.conn.playing, un);
    can_eventually_build =
        can_player_build_unit_later(client.conn.playing, un);

    /* If there's a city, can the city build the unit? */
    if (pCity) {
      can_build = can_build && can_city_build_unit_now(pCity, un);
      can_eventually_build = can_eventually_build
        && can_city_build_unit_later(pCity, un);
    }

    if ((advanced_tech && can_eventually_build)
        || (!advanced_tech && can_build)) {

      pUnit = un;

      pIcon = crop_rect_from_surface(pMain, NULL);

      fc_snprintf(cbuf, sizeof(cbuf), "%s", utype_name_translation(un));

      copy_chars_to_utf8_str(pstr, cbuf);
      pstr->style |= TTF_STYLE_BOLD;
      pText_Name = create_text_surf_smaller_than_w(pstr, pIcon->w - 4);

      if (pCity) {
        struct universal univ = cid_production(cid_encode_unit(un));
        int cost = utype_build_shield_cost(pCity, un);

        turns = city_turns_to_build(pCity, &univ, TRUE);

        if (turns == FC_INFINITY) {
          fc_snprintf(cbuf, sizeof(cbuf),
                      _("(%d/%d/%s)\n%d/%d %s\nnever"),
                      pUnit->attack_strength,
                      pUnit->defense_strength,
                      move_points_text(pUnit->move_rate, TRUE),
                      pCity->shield_stock, cost,
                      PL_("shield", "shields", cost));
        } else {
          fc_snprintf(cbuf, sizeof(cbuf),
                      _("(%d/%d/%s)\n%d/%d %s\n%d %s"),
                      pUnit->attack_strength,
                      pUnit->defense_strength,
                      move_points_text(pUnit->move_rate, TRUE),
                      pCity->shield_stock, cost,
                      PL_("shield", "shields", cost),
                      turns, PL_("turn", "turns", turns));
        }
      } else {
        int cost = utype_build_shield_cost_base(un);

        fc_snprintf(cbuf, sizeof(cbuf),
                    _("(%d/%d/%s)\n%d %s"),
                    pUnit->attack_strength,
                    pUnit->defense_strength,
                    move_points_text(pUnit->move_rate, TRUE),
                    cost,
                    PL_("shield", "shields", cost));
      }

      copy_chars_to_utf8_str(pstr, cbuf);
      pstr->style &= ~TTF_STYLE_BOLD;

      pText = create_text_surf_from_utf8(pstr);

      pZoom = adj_surf(get_unittype_surface(un, direction8_invalid()));
      dst.x = (pIcon->w - pZoom->w) / 2;
      dst.y = (pIcon->h/2 - pZoom->h) / 2;
      alphablit(pZoom, NULL, pIcon, &dst, 255);
      FREESURFACE(pZoom);

      dst.x = (pIcon->w - pText_Name->w) / 2;
      dst.y = pIcon->h/2 + (pIcon->h/2 - (pText_Name->h + pText->h)) / 2;
      alphablit(pText_Name, NULL, pIcon, &dst, 255);

      dst.x = (pIcon->w - pText->w) / 2;
      dst.y += pText_Name->h;
      alphablit(pText, NULL, pIcon, &dst, 255);

      FREESURFACE(pText);
      FREESURFACE(pText_Name);

      pBuf = create_icon2(pIcon, pWindow->dst,
                          WF_RESTORE_BACKGROUND|WF_FREE_THEME);
      set_wstate(pBuf, FC_WS_NORMAL);

      widget_w = MAX(widget_w, pBuf->size.w);
      widget_h = MAX(widget_h, pBuf->size.h);

      pBuf->data.city = pCity;
      add_to_gui_list(MAX_ID - cid_encode_unit(un), pBuf);
      pBuf->action = worklist_editor_targets_callback;

      if (count > (TARGETS_ROW * TARGETS_COL - 1)) {
        set_wflag(pBuf, WF_HIDDEN);
      }
      count++;

    }
  } unit_type_iterate_end;

  pEditor->pTargets = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  pEditor->pTargets->pEndWidgetList = pLast->prev;
  pEditor->pTargets->pBeginWidgetList = pBuf;
  pEditor->pTargets->pEndActiveWidgetList = pEditor->pTargets->pEndWidgetList;
  pEditor->pTargets->pBeginActiveWidgetList = pEditor->pTargets->pBeginWidgetList;
  pEditor->pTargets->pActiveWidgetList = pEditor->pTargets->pEndActiveWidgetList;

  /* --------------- */
  if (count > (TARGETS_ROW * TARGETS_COL - 1)) {
    count = create_vertical_scrollbar(pEditor->pTargets,
                                      TARGETS_COL, TARGETS_ROW, TRUE, TRUE);
  } else {
    count = 0;
  }
  /* --------------- */

  pEditor->pBeginWidgetList = pEditor->pTargets->pBeginWidgetList;

  /* Window */
  area.w = MAX(area.w, widget_w * TARGETS_COL + count + adj_size(130));
  area.h = MAX(area.h, widget_h * TARGETS_ROW);

  pIcon = theme_get_background(theme, BACKGROUND_WLDLG);
  if (resize_window(pWindow, pIcon, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pIcon);
  }

  area = pWindow->area;

  /* Backgrounds */
  dst.x = area.x;
  dst.y = area.y;
  dst.w = adj_size(130);
  dst.h = adj_size(145);

  SDL_FillRect(pWindow->theme, &dst,
               map_rgba(pWindow->theme->format, *get_theme_color(COLOR_THEME_BACKGROUND)));

  create_frame(pWindow->theme,
               dst.x, dst.y, dst.w - 1, dst.h - 1,
               get_theme_color(COLOR_THEME_WLDLG_FRAME));
  create_frame(pWindow->theme,
               dst.x + 2, dst.y + 2, dst.w - 5, dst.h - 5,
               get_theme_color(COLOR_THEME_WLDLG_FRAME));

  dst.x = area.x;
  dst.y += dst.h + adj_size(2);
  dst.w = adj_size(130);
  dst.h = adj_size(228);
  fill_rect_alpha(pWindow->theme, &dst, &bg_color2);

  create_frame(pWindow->theme,
               dst.x, dst.y, dst.w - 1, dst.h - 1,
               get_theme_color(COLOR_THEME_WLDLG_FRAME));

  if (pEditor->pGlobal) {
    dst.x = area.x;
    dst.y += dst.h + adj_size(2);
    dst.w = adj_size(130);
    dst.h = pWindow->size.h - dst.y - adj_size(4);

    SDL_FillRect(pWindow->theme, &dst,
                 map_rgba(pWindow->theme->format, *get_theme_color(COLOR_THEME_BACKGROUND)));

    create_frame(pWindow->theme,
                 dst.x, dst.y, dst.w - 1, dst.h - 1,
                 get_theme_color(COLOR_THEME_WLDLG_FRAME));
    create_frame(pWindow->theme,
                 dst.x + adj_size(2), dst.y + adj_size(2),
                 dst.w - adj_size(5), dst.h - adj_size(5),
                 get_theme_color(COLOR_THEME_WLDLG_FRAME));
  }

  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  /* name */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w) / 2;
  pBuf->size.y = area.y + adj_size(4);

  /* size of worklist (without production) */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w) / 2;
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;

  if (pCity) {
    /* current build and progress bar */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w) / 2;
    pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h + adj_size(5);

    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w) / 2;
    pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;
  } else {
    /* rename worklist */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w) / 2;
    pBuf->size.y = area.y + 1 + (adj_size(145) - pBuf->size.h) / 2;
  }

  /* ok button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + (adj_size(65) - pBuf->size.w) / 2;
  pBuf->size.y = area.y + adj_size(135) - pBuf->size.h;

  /* exit button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + adj_size(65) + (adj_size(65) - pBuf->size.w) / 2;
  pBuf->size.y = area.y + adj_size(135) - pBuf->size.h;

  /* worklist */
  /* pEditor->pWork->pScroll->count: including production */
  if (pCity || (worklist_length(&pEditor->worklist_copy) > 0)) {
    /* FIXME */
    setup_vertical_widgets_position(1,
                                    area.x + adj_size(2), area.y + adj_size(152)
          /* + ((pEditor->pWork->pScroll->count > pEditor->pWork->pScroll->active + 2) ?
             pEditor->pWork->pScroll->pUp_Left_Button->size.h + 1 : 0)*/,
                                    adj_size(126), 0, pEditor->pWork->pBeginWidgetList,
                                    pEditor->pWork->pEndWidgetList);

    setup_vertical_scrollbar_area(pEditor->pWork->pScroll,
	area.x + adj_size(2),
    	area.y + adj_size(152),
    	adj_size(225), FALSE);
  }

  /* global worklists */
  if (pEditor->pGlobal) {
    setup_vertical_widgets_position(1,
                                    area.x + adj_size(4),
                                    area.y + adj_size(384) +
                                    (pEditor->pGlobal->pScroll ?
                                     pEditor->pGlobal->pScroll->pUp_Left_Button->size.h + 1 : 0),
                                    adj_size(122), 0, pEditor->pGlobal->pBeginWidgetList,
                                    pEditor->pGlobal->pEndWidgetList);

    if (pEditor->pGlobal->pScroll) {
      setup_vertical_scrollbar_area(pEditor->pGlobal->pScroll,
                                    area.x + adj_size(4),
                                    area.y + adj_size(384),
                                    adj_size(93), FALSE);
    }

  }

  /* Targets */
  setup_vertical_widgets_position(TARGETS_COL,
                                  area.x + adj_size(130), area.y,
                                  0, 0, pEditor->pTargets->pBeginWidgetList,
                                  pEditor->pTargets->pEndWidgetList);

  if (pEditor->pTargets->pScroll) {
    setup_vertical_scrollbar_area(pEditor->pTargets->pScroll,
                                  area.x + area.w,
                                  area.y + 1,
                                  area.h - 1, TRUE);

  }
 
  /* ----------------------------------- */
  FREEUTF8STR(pstr);
  FREESURFACE(pMain);

  redraw_group(pEditor->pBeginWidgetList, pWindow, 0);
  widget_flush(pWindow);
}

/**********************************************************************//**
  Close worklist from view.
**************************************************************************/
void popdown_worklist_editor(void)
{
  if (pEditor) {
    popdown_window_group_dialog(pEditor->pBeginWidgetList,
                                pEditor->pEndWidgetList);
    FC_FREE(pEditor->pTargets->pScroll);
    FC_FREE(pEditor->pTargets);

    FC_FREE(pEditor->pWork->pScroll);
    FC_FREE(pEditor->pWork);

    if (pEditor->pGlobal) {
      FC_FREE(pEditor->pGlobal->pScroll);
      FC_FREE(pEditor->pGlobal);
    }

    if (city_dialog_is_open(pEditor->pCity)) {
      enable_city_dlg_widgets();
    }

    FC_FREE(pEditor);

    flush_dirty();
  }
}
