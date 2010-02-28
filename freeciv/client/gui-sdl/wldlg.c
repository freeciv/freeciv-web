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
/**********************************************************************
                          wldlg.c  -  description
                             -------------------
    begin                : Wed Sep 18 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "SDL.h"

/* utility */
#include "fcintl.h"

/* common */
#include "game.h"
#include "movement.h"
#include "unit.h"

/* client */
#include "client_main.h"
#include "climisc.h"

/* gui-sdl */
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

#define TARGETS_COL		4
#define TARGETS_ROW		4

struct EDITOR {
  struct widget *pBeginWidgetList;
  struct widget *pEndWidgetList;/* window */
    
  struct ADVANCED_DLG *pTargets;
  struct ADVANCED_DLG *pWork;
  struct ADVANCED_DLG *pGlobal;
    
  struct city *pCity;
  struct worklist *pOrginal_WorkList;
  struct worklist *pCopy_WorkList;
  
  /* shortcuts  */
  struct widget *pDock;
  struct widget *pWorkList_Counter;
  
  struct widget *pProduction_Name;
  struct widget *pProduction_Progres;
    
  int stock;
  struct universal currently_building;
  
} *pEditor = NULL;


static int worklist_editor_item_callback(struct widget *pWidget);
static SDL_Surface * get_progress_icon(int stock, int cost, int *progress);
static const char * get_production_name(struct city *pCity,
					struct universal prod, int *cost);
static void refresh_worklist_count_label(void);
static void refresh_production_label(int stock);

/* =========================================================== */

/* Worklist Editor Window Callback */
static int window_worklist_editor_callback(struct widget *pWidget)
{
  return -1;
}

/* Popdwon Worklist Editor */
static int popdown_worklist_editor_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_worklist_editor();
  }
  return -1;
}

/*
 * Commit changes to city/global worklist
 * In City Mode Remove Double entry of Imprv/Woder Targets from list.
 */
static int ok_worklist_editor_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int i, j;
    struct city *pCity = pEditor->pCity;
    bool same_prod = TRUE;
    
    /* remove duplicate entry of impv./wonder target from worklist */
    for(i = 0; i < worklist_length(pEditor->pCopy_WorkList); i++) {
  
      if(VUT_IMPROVEMENT == pEditor->pCopy_WorkList->entries[i].kind) {
        for(j = i + 1; j < worklist_length(pEditor->pCopy_WorkList); j++) {
          if (VUT_IMPROVEMENT == pEditor->pCopy_WorkList->entries[j].kind
           && pEditor->pCopy_WorkList->entries[i].value.building ==
              pEditor->pCopy_WorkList->entries[j].value.building) {
            worklist_remove(pEditor->pCopy_WorkList, j);
          }
        }
      }
    }
    
    if(pCity) {
      /* remove duplicate entry of currently building impv./wonder from worklist */
      if(VUT_IMPROVEMENT == pEditor->currently_building.kind) {
        for(i = 0; i < worklist_length(pEditor->pCopy_WorkList); i++) {
          if (VUT_IMPROVEMENT == pEditor->pCopy_WorkList->entries[i].kind
           && pEditor->pCopy_WorkList->entries[i].value.building ==
              pEditor->currently_building.value.building) {
              worklist_remove(pEditor->pCopy_WorkList, i);
          }
        }
      }
      
      /* change production */
      if(!are_universals_equal(&pCity->production, &pEditor->currently_building)) {
        city_change_production(pCity, pEditor->currently_building);
        same_prod = FALSE;
      }
      
      /* commit new city worklist */
      city_set_worklist(pCity, pEditor->pCopy_WorkList);
    } else {
      /* commit global worklist */
      worklist_copy(pEditor->pOrginal_WorkList, pEditor->pCopy_WorkList);
      update_worklist_report_dialog();
    }  
    
    /* popdown dialog */
    popdown_worklist_editor();
  }
  
  return -1;
}

/*
 * Rename Global Worklist
 */
static int rename_worklist_editor_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if(pWidget->string16->text) {
      char *pText = convert_to_chars(pWidget->string16->text);
      my_snprintf(pEditor->pCopy_WorkList->name, MAX_LEN_NAME, "%s", pText);
      FC_FREE(pText);
    } else {
      /* empty input -> restore previous content */
      copy_chars_to_string16(pWidget->string16, pEditor->pCopy_WorkList->name);
      widget_redraw(pWidget);
      widget_mark_dirty(pWidget);
      flush_dirty();
    }
  }  
  return -1;
}

/* ====================================================================== */

/*
 * Add target to worklist.
 */
static void add_target_to_worklist(struct widget *pTarget)
{
  struct widget *pBuf = NULL, *pDock = NULL;
  SDL_String16 *pStr = NULL;
  int i;
  struct universal prod = cid_decode(MAX_ID - pTarget->ID);
  
  set_wstate(pTarget, FC_WS_SELLECTED);
  widget_redraw(pTarget);
  widget_flush(pTarget);
  
  /* Deny adding currently building Impr/Wonder Target */ 
  if (pEditor->pCity
      && VUT_IMPROVEMENT == prod.kind
      && are_universals_equal(&prod, &pEditor->currently_building)) {
    return;
  }
  
  if (worklist_length(pEditor->pCopy_WorkList) >= MAX_LEN_WORKLIST - 1) {
    return;   
    }
    
  for(i = 0; i < worklist_length(pEditor->pCopy_WorkList); i++) {
    if (VUT_IMPROVEMENT == prod.kind
     && are_universals_equal(&prod, &pEditor->pCopy_WorkList->entries[i])) {
    return;
    }
  }
    
  worklist_append(pEditor->pCopy_WorkList, prod);
  
  /* create widget entry */
  if(VUT_UTYPE == prod.kind) {
    pStr = create_str16_from_char(utype_name_translation(prod.value.utype), adj_font(10));
  } else {
    pStr = create_str16_from_char(city_improvement_name_translation(pEditor->pCity, prod.value.building),
                                                                adj_font(10));
  }
  
  pStr->style |= SF_CENTER;
  pBuf = create_iconlabel(NULL, pTarget->dst, pStr,
				(WF_RESTORE_BACKGROUND|WF_FREE_DATA));
    
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->action = worklist_editor_item_callback;
        
  pBuf->data.ptr = fc_calloc(1, sizeof(int));
  *((int *)pBuf->data.ptr) = worklist_length(pEditor->pCopy_WorkList) - 1;
    
  pBuf->ID = MAX_ID - cid_encode(prod);
  
  if(pEditor->pWork->pBeginActiveWidgetList) {
    pDock = pEditor->pWork->pBeginActiveWidgetList;
  } else {
    pDock = pEditor->pDock;
  }
  
/* FIXME */  
#if 0
  if (worklist_length(pEditor->pCopy_WorkList) > pEditor->pWork->pScroll->active + 1) {

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
#endif
  
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

/*
 * Find if two targets are the same class (unit, imprv. , wonder).
 * This is needed by calculation of change production shields penalty.
 * [similar to are_universals_equal()]
 */
static bool are_the_same_class(const struct universal one,
			       const struct universal two)
{
  if (one.kind != two.kind) {
    return FALSE;
  }
  if (VUT_UTYPE == one.kind) {
    return one.value.utype == two.value.utype;
  }
  if (VUT_IMPROVEMENT == one.kind) {
    if (is_wonder(one.value.building)) {
      return is_wonder(two.value.building);
    }
    return (one.value.building == two.value.building);
  }
  return FALSE;
}

/*
 * Change production in editor shell, callculate production shields penalty and
 * refresh production progress label
 */
static void change_production(struct universal prod)
{
    
  if(!are_the_same_class(pEditor->currently_building, prod)) {
    if(pEditor->stock != pEditor->pCity->shield_stock) {
      if(are_the_same_class(pEditor->pCity->production, prod)) {
	pEditor->stock = pEditor->pCity->shield_stock;
      }
    } else {
      pEditor->stock =
	  city_change_production_penalty(pEditor->pCity, prod);
    }	  	  
  }
      
  pEditor->currently_building = prod;
  refresh_production_label(pEditor->stock);

  return;
}

/*
 * Change production of city but only in Editor.
 * You must commit this changes by exit editor via OK button (commit function).
 *
 * This function don't remove double imprv./wonder target entry from worklist
 * and allow more entry of such target (In Production and worklist - this is
 * fixed by commit function).
 */
static void add_target_to_production(struct widget *pTarget)
{
  int dummy;
  struct universal prod;
  assert(pTarget != NULL);
  
  /* redraw Target Icon */
  set_wstate(pTarget, FC_WS_SELLECTED);
  widget_redraw(pTarget);
  widget_flush(pTarget);

  prod = cid_decode(MAX_ID - pTarget->ID);
  
  /* check if we change to the same target */
  if(are_universals_equal(&prod, &pEditor->currently_building)) {
    /* comit changes and exit - double click detection */
    ok_worklist_editor_callback(NULL);
    return;
  }
  
  change_production(prod);
  
  /* change Production Text Label in Worklist Widget list */
  copy_chars_to_string16(pEditor->pWork->pEndActiveWidgetList->string16,
  	get_production_name(pEditor->pCity, prod, &dummy));
  
  pEditor->pWork->pEndActiveWidgetList->ID = MAX_ID - cid_encode(prod);
    
  widget_redraw(pEditor->pWork->pEndActiveWidgetList);
  widget_mark_dirty(pEditor->pWork->pEndActiveWidgetList);
  
  flush_dirty();    
  
}

/* Get Help Info about target */
static void get_target_help_data(struct widget *pTarget)
{
  struct universal prod;

  assert(pTarget != NULL);
  
  /* redraw Target Icon */
  set_wstate(pTarget, FC_WS_SELLECTED);
  widget_redraw(pTarget);
  /*widget_flush(pTarget);*/
  
  prod = cid_decode(MAX_ID - pTarget->ID);

  if (VUT_UTYPE == prod.kind)
  {
    popup_unit_info(utype_number(prod.value.utype));
  } else {
    popup_impr_info(improvement_number(prod.value.building));
  }
  
}


/*
 * Targets callback
 * left mouse button -> In city mode change production to target.
			In global mode add target to worklist.
 * middle mouse button -> get target "help"
 * right mouse button -> add target to worklist.
 */
static int worklist_editor_targets_callback(struct widget *pWidget)
{
  switch(Main.event.button.button) {
    case SDL_BUTTON_LEFT:
      if(pEditor->pCity) {
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
    	;/* do nothing */
    break;
  }
  return -1;
}

/* ====================================================================== */

/*
 * Remove element from worklist or
 * remove currently building imprv/unit and change production to first worklist
 * element (or Capitalizations if worklist is empty)
 */
static void remove_item_from_worklist(struct widget *pItem)
{
  /* only one item (production) is left */
  if (worklist_is_empty(pEditor->pCopy_WorkList))
    return;
        
  if(pItem->data.ptr) {
    /* correct "data" widget fiels */
    struct widget *pBuf = pItem;
    if(pBuf != pEditor->pWork->pBeginActiveWidgetList) {
      do{
	pBuf = pBuf->prev;
	*((int *)pBuf->data.ptr) = *((int *)pBuf->data.ptr) - 1;
      } while(pBuf != pEditor->pWork->pBeginActiveWidgetList);
    }
    
    /* remove element from worklist */
    worklist_remove(pEditor->pCopy_WorkList, *((int *)pItem->data.ptr));
    
    /* remove widget from widget list */
    del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pItem);
  } else {
    /* change productions to first worklist element */
    struct widget *pBuf = pItem->prev;
    change_production(pEditor->pCopy_WorkList->entries[0]);
    worklist_advance(pEditor->pCopy_WorkList);
    del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pItem);
    FC_FREE(pBuf->data.ptr);
    if(pBuf != pEditor->pWork->pBeginActiveWidgetList) {
      do{
        pBuf = pBuf->prev;
        *((int *)pBuf->data.ptr) = *((int *)pBuf->data.ptr) - 1;
      } while(pBuf != pEditor->pWork->pBeginActiveWidgetList);
    }
  }

/* FIXME: fix scrollbar code */
#if 0    
  /* worklist_length(pEditor->pCopy_WorkList): without production */
  if (worklist_length(pEditor->pCopy_WorkList) <= pEditor->pWork->pScroll->active + 1) {

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
#endif    
    hide_scrollbar(pEditor->pWork->pScroll);    
  }
#endif
    
  refresh_worklist_count_label();
  redraw_group(pEditor->pWork->pBeginWidgetList,
			  pEditor->pWork->pEndWidgetList, TRUE);
  flush_dirty();
}

/*
 * Swap worklist entries DOWN.
 * Fuction swap current element with next element of worklist.
 *
 * If pItem is last widget or there is only one widget on widgets list
 * fuction remove this widget from widget list and target from worklist
 *
 * In City mode, when pItem is first worklist element, function make
 * change production (currently building is moved to first element of worklist
 * and first element of worklist is build).
 */
static void swap_item_down_from_worklist(struct widget *pItem)
{
  Uint16 *pText, ID;
  bool changed = FALSE;
  struct universal tmp;
  
  if(pItem == pEditor->pWork->pBeginActiveWidgetList) {
    remove_item_from_worklist(pItem);
    return;
  }
  
  pText = pItem->string16->text;
  ID = pItem->ID;
  
  /* second item or higher was clicked */
  if(pItem->data.ptr) {
    /* worklist operations -> swap down */
    int row = *((int *)pItem->data.ptr);
       
    tmp = pEditor->pCopy_WorkList->entries[row];
    pEditor->pCopy_WorkList->entries[row] = pEditor->pCopy_WorkList->entries[row + 1];
    pEditor->pCopy_WorkList->entries[row + 1] = tmp;
      
    changed = TRUE;  
  } else {
    /* first item was clicked -> change production */
    change_production(pEditor->pCopy_WorkList->entries[0]);
    pEditor->pCopy_WorkList->entries[0] = cid_decode(MAX_ID - ID);
      changed = TRUE;
    }
  
  if(changed) {
    pItem->string16->text = pItem->prev->string16->text;
    pItem->ID = pItem->prev->ID;
  
    pItem->prev->string16->text = pText;
    pItem->prev->ID = ID;

    redraw_group(pEditor->pWork->pBeginWidgetList,
			  pEditor->pWork->pEndWidgetList, TRUE);
    flush_dirty();
  }
  
}

/*
 * Swap worklist entries UP.
 * Fuction swap current element with prev. element of worklist.
 *
 * If pItem is first widget on widgets list fuction remove this widget 
 * from widget list and target from worklist (global mode)
 * or from production (city mode)
 *
 * In City mode, when pItem is first worklist element, function make
 * change production (currently building is moved to first element of worklist
 * and first element of worklist is build).
 */
static void swap_item_up_from_worklist(struct widget *pItem)
{
  Uint16 *pText = pItem->string16->text;
  Uint16 ID = pItem->ID;
  bool changed = FALSE;
  struct universal tmp;  

  /* first item was clicked -> remove */
  if(pItem == pEditor->pWork->pEndActiveWidgetList) {
    remove_item_from_worklist(pItem);
    return;
  }

  /* third item or higher was clicked */  
  if(pItem->data.ptr && *((int *)pItem->data.ptr) > 0) {
    /* worklist operations -> swap up*/
    int row = *((int *)pItem->data.ptr);
    
    tmp = pEditor->pCopy_WorkList->entries[row];
    pEditor->pCopy_WorkList->entries[row] = pEditor->pCopy_WorkList->entries[row - 1];
    pEditor->pCopy_WorkList->entries[row - 1] = tmp;
      
    changed = TRUE;    
  } else {
    /* second item was clicked -> change production ... */
    tmp = pEditor->currently_building;
    change_production(pEditor->pCopy_WorkList->entries[0]);
    pEditor->pCopy_WorkList->entries[0] = tmp;    
      
    changed = TRUE;
  }

  if(changed) {
    pItem->string16->text = pItem->next->string16->text;
    pItem->ID = pItem->next->ID;
    
    pItem->next->string16->text = pText;
    pItem->next->ID = ID;
    
    redraw_group(pEditor->pWork->pBeginWidgetList,
			  pEditor->pWork->pEndWidgetList, TRUE);
    flush_dirty();
  }
}

/*
 * worklist callback
 * left mouse button -> swap entries up.
 * middle mouse button -> remove element from list
 * right mouse button -> swap entries down.
 */
static int worklist_editor_item_callback(struct widget *pWidget)
{
  switch(Main.event.button.button) {
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

/*
 * Add global worklist to city worklist starting from last free entry.
 * Add only avilable targets in current game state.
 * If global worklist have more targets that city worklist have free
 * entries then we adding only first part of global worklist.
 */
static void add_global_worklist(struct widget *pWidget)
{
  if(!worklist_is_empty(&client.worklists[MAX_ID - pWidget->ID])) {
    int count, firstfree;
    struct widget *pBuf = pEditor->pWork->pEndActiveWidgetList;
    struct worklist *pWorkList = &client.worklists[MAX_ID - pWidget->ID];
      
    if(worklist_length(pEditor->pCopy_WorkList) >= MAX_LEN_WORKLIST - 1) {
      /* worklist is full */
      return;
    }
    
    firstfree = worklist_length(pEditor->pCopy_WorkList);
    /* copy global worklist to city worklist */
    for(count = 0 ; count < worklist_length(pWorkList); count++) {
      
      /* global worklist can have targets unavilable in current state of game
         then we must remove those targets from new city worklist */
      if(!can_city_build_later(pEditor->pCity, pWorkList->entries[count])) {
	continue;  
      }
      
      worklist_append(pEditor->pCopy_WorkList, pWorkList->entries[count]);
      
      /* create widget */      
      if(VUT_UTYPE == pWorkList->entries[count].kind) {
	pBuf = create_iconlabel(NULL, pWidget->dst,
		create_str16_from_char(
			utype_name_translation(pWorkList->entries[count].value.utype),
			adj_font(10)),
		(WF_RESTORE_BACKGROUND|WF_FREE_DATA));
	pBuf->ID = MAX_ID - cid_encode_unit(pWorkList->entries[count].value.utype);
      } else {
	pBuf = create_iconlabel(NULL, pWidget->dst,
		create_str16_from_char(
			city_improvement_name_translation(pEditor->pCity,
				pWorkList->entries[count].value.building),
			adj_font(10)),
		(WF_RESTORE_BACKGROUND|WF_FREE_DATA));
	pBuf->ID = MAX_ID - cid_encode_building(pWorkList->entries[count].value.building);
      }
      
      pBuf->string16->style |= SF_CENTER;
      set_wstate(pBuf, FC_WS_NORMAL);
      pBuf->action = worklist_editor_item_callback;
      pBuf->size.w = adj_size(126);  
      pBuf->data.ptr = fc_calloc(1, sizeof(int));
      *((int *)pBuf->data.ptr) = firstfree;
        
      add_widget_to_vertical_scroll_widget_list(pEditor->pWork,
        pBuf, pEditor->pWork->pBeginActiveWidgetList, FALSE,
        pEditor->pEndWidgetList->area.x + adj_size(2),
        pEditor->pEndWidgetList->area.y + adj_size(152));
      
      firstfree++;
      if(firstfree == MAX_LEN_WORKLIST - 1) {
	break;
      }
    }
    
    refresh_worklist_count_label();
    redraw_group(pEditor->pWork->pBeginWidgetList,
    			pEditor->pWork->pEndWidgetList, TRUE);
    
    flush_dirty();
  }
}

/*
 * Clear city worklist and copy here global worklist.
 * Copy only avilable targets in current game state.
 * If all targets are unavilable then leave city worklist untouched.
 */
static void set_global_worklist(struct widget *pWidget)
{
  if(!worklist_is_empty(&client.worklists[MAX_ID - pWidget->ID])) {
    int count, wl_count;
    struct universal target;
    struct widget *pBuf = pEditor->pWork->pEndActiveWidgetList;
    struct worklist wl ,
	      *pWorkList = &client.worklists[MAX_ID - pWidget->ID];
    
    /* clear tmp worklist */
    worklist_init(&wl);
    
    wl_count = 0;
    /* copy global worklist to city worklist */
    for(count = 0; count < worklist_length(pWorkList); count++) {
      
      /* global worklist can have targets unavilable in current state of game
         then we must remove those targets from new city worklist */
      if(!can_city_build_later(pEditor->pCity, pWorkList->entries[count])) {
	continue;  
      }
      
      wl.entries[wl_count] = pWorkList->entries[count];
      wl_count++;
    }
    /* --------------------------------- */
    
    if(!worklist_is_empty(&wl)) {
      /* free old widget list */
      if(pBuf != pEditor->pWork->pBeginActiveWidgetList) {
        pBuf = pBuf->prev;
        if(pBuf != pEditor->pWork->pBeginActiveWidgetList) {
          do {
            pBuf = pBuf->prev;
            del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pBuf->next);
          } while(pBuf != pEditor->pWork->pBeginActiveWidgetList);
        }
        del_widget_from_vertical_scroll_widget_list(pEditor->pWork, pBuf);
      }  
      /* --------------------------------- */
      
      worklist_copy(pEditor->pCopy_WorkList, &wl);
    
      /* --------------------------------- */
      /* create new widget list */
      for(count = 0; count < MAX_LEN_WORKLIST; count++) {
        /* end of list */
        if(!worklist_peek_ith(pEditor->pCopy_WorkList, &target, count)) {
          break;
        }
    
        if(VUT_UTYPE == target.kind) {
	  pBuf = create_iconlabel(NULL, pWidget->dst,
	    create_str16_from_char(utype_name_translation(target.value.utype),
				   adj_font(10)),
				   (WF_RESTORE_BACKGROUND|WF_FREE_DATA));
	  pBuf->ID = MAX_ID - B_LAST - utype_number(target.value.utype);
        } else {
	  pBuf = create_iconlabel(NULL, pWidget->dst,
	    create_str16_from_char(city_improvement_name_translation(pEditor->pCity, target.value.building),
				   adj_font(10)),
				   (WF_RESTORE_BACKGROUND|WF_FREE_DATA));
	  pBuf->ID = MAX_ID - improvement_number(target.value.building);
        }
        pBuf->string16->style |= SF_CENTER;
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
}

/*
 * global worklist callback
 * left mouse button -> add global worklist to current city list
 * right mouse button -> clear city worklist and copy here global worklist.
 *
 * There are problems with impv./wonder tagets becouse those can't be doubled
 * on worklist and adding/seting can give you situation that global worklist
 * have imprv./wonder entry that exist on city worklist or in building state.
 * I don't make such check here and allow this "functionality" becouse doubled
 * impov./wonder entry are removed from city worklist during "commit" phase.
 */
static int global_worklist_callback(struct widget *pWidget)
{
  switch(Main.event.button.button) {
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
    	;/* do nothing */
    break;
  }
  return -1;
}

/* ======================================================= */

/* return full unit/imprv. name and build cost in "cost" pointer */
static const char * get_production_name(struct city *pCity,
					struct universal prod, int *cost)
{
  assert(cost != NULL);
        
  *cost = universal_build_shield_cost(&prod);
  if(VUT_UTYPE == prod.kind) {
    return utype_name_translation(prod.value.utype);
  } else {
    return city_improvement_name_translation(pCity, prod.value.building);
  }
}

/*
 * return progress icon (bar) of current production state
 * stock - current shielsd stocks
 * cost - unit/imprv. cost
 * function return in "proggres" pointer (0 - 100 %) progress in numbers
*/
static SDL_Surface * get_progress_icon(int stock, int cost, int *progress)
{
  SDL_Surface *pIcon = NULL;
  int width;
  assert(progress != NULL);
  
  if(stock < cost) {
    width = ((float)stock / cost) * adj_size(116.0);
    *progress = ((float)stock / cost) * 100.0;
    if(!width && stock) {
      *progress = 1;
      width = 1;
    }
  } else {
    width = adj_size(116);
    *progress = 100;
  }
    
  pIcon = create_bcgnd_surf(pTheme->Edit, 0, adj_size(120), adj_size(30));
    
  if(width) {
    SDL_Rect dst = {2,1,0,0};
    SDL_Surface *pBuf = create_bcgnd_surf(pTheme->Button, 3, width, adj_size(28));
    alphablit(pBuf, NULL, pIcon, &dst);
    FREESURFACE(pBuf);
  }
    
  return pIcon;  
}

/*
 * update and redraw production name label in worklist editor
 * stock - pCity->shields_stock or current stock after chnge production lost.
 */
static void refresh_production_label(int stock)
{
  int cost, turns;
  char cBuf[64];
  SDL_Rect area;
  const char *name = get_production_name(pEditor->pCity,
    				pEditor->currently_building, &cost);

  if (VUT_IMPROVEMENT == pEditor->currently_building.kind
     && improvement_has_flag(pEditor->currently_building.value.building, IF_GOLD))
  {
     my_snprintf(cBuf, sizeof(cBuf),
      	_("%s\n%d gold per turn"), name, MAX(0, pEditor->pCity->surplus[O_SHIELD]));
  } else {
    if(stock < cost) {
      turns = city_turns_to_build(pEditor->pCity,
				  pEditor->currently_building, TRUE);
      if(turns == 999)
      {
        my_snprintf(cBuf, sizeof(cBuf), _("%s\nblocked!"), name);
      } else {
        my_snprintf(cBuf, sizeof(cBuf), _("%s\n%d %s"),
		    name, turns, PL_("turn", "turns", turns));
      }
    } else {
      my_snprintf(cBuf, sizeof(cBuf), _("%s\nfinished!"), name);
    }
  }
  copy_chars_to_string16(pEditor->pProduction_Name->string16, cBuf);

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
  sdl_dirty_rect(area);
  
  FREESURFACE(pEditor->pProduction_Progres->theme);
  pEditor->pProduction_Progres->theme =
		  get_progress_icon(stock, cost, &cost);
    
  my_snprintf(cBuf, sizeof(cBuf), "%d%%" , cost);
  copy_chars_to_string16(pEditor->pProduction_Progres->string16, cBuf);
  widget_redraw(pEditor->pProduction_Progres);
  widget_mark_dirty(pEditor->pProduction_Progres);
}


/* update and redraw worklist length counter in worklist editor */
static void refresh_worklist_count_label(void)
{
  char cBuf[64];
  SDL_Rect area;

  my_snprintf(cBuf, sizeof(cBuf), _("( %d entries )"),
  				worklist_length(pEditor->pCopy_WorkList));
  copy_chars_to_string16(pEditor->pWorkList_Counter->string16, cBuf);

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
  sdl_dirty_rect(area);
}


/* ====================================================================== */


/*
 * Global/City worklist editor.
 * if pCity == NULL then fucnction take pWorklist as global worklist.
 * pWorklist must be not NULL.
 */
void popup_worklist_editor(struct city *pCity, struct worklist *pWorkList)
{
  SDL_Color bg_color = {255,255,255,128};
  SDL_Color bg_color2 = {255,255,255,136};

  int count = 0, turns;
  int i, widget_w = 0, widget_h = 0;
  SDL_String16 *pStr = NULL;
  struct widget *pBuf = NULL, *pWindow, *pLast;
  SDL_Surface *pText = NULL, *pText_Name = NULL, *pZoom = NULL;
  SDL_Surface *pMain;
  SDL_Surface *pIcon;
  SDL_Rect dst;
  char cBuf[128];
  struct unit_type *pUnit = NULL;
  char *state = NULL;
  bool advanced_tech;
  bool can_build, can_eventually_build;
  SDL_Rect area;
  struct worklist *worklist;
  
  if(pEditor) {
    return;
  }
  
  assert(pWorkList != NULL);
  
  advanced_tech = (pCity == NULL);
  
  pEditor = fc_calloc(1, sizeof(struct EDITOR));
  
  pEditor->pCity = pCity;
  pEditor->pOrginal_WorkList = pWorkList;
  pEditor->pCopy_WorkList = fc_calloc(1, sizeof(struct worklist));
  worklist_copy(pEditor->pCopy_WorkList, pWorkList);
  
  if(pCity) {
    pEditor->currently_building = pCity->production;
    pEditor->stock = pCity->shield_stock;
  }
  
  /* --------------- */
  /* create Target Background Icon */
  pMain = create_surf_alpha(adj_size(116), adj_size(116), SDL_SWSURFACE);
  SDL_FillRect(pMain, NULL, map_rgba(pMain->format, bg_color));
  putframe(pMain, 0, 0, pMain->w - 1, pMain->h - 1,
    map_rgba(pMain->format, *get_game_colorRGB(COLOR_THEME_WLDLG_FRAME)));
    
  /* ---------------- */
  /* Create Main Window */
  pWindow = create_window_skeleton(NULL, NULL, 0);
  pWindow->action = window_worklist_editor_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  
  add_to_gui_list(ID_WINDOW, pWindow);
  pEditor->pEndWidgetList = pWindow;

  area = pWindow->area;
  
  /* ---------------- */
  if(pCity) {
    my_snprintf(cBuf, sizeof(cBuf), _("Worklist of\n%s"), city_name(pCity));
  } else {
    my_snprintf(cBuf, sizeof(cBuf), "%s", pWorkList->name);
  }
    
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
  
  add_to_gui_list(ID_LABEL, pBuf);
  /* --------------------------- */
  
  my_snprintf(cBuf, sizeof(cBuf), _("( %d entries )"), worklist_length(pWorkList));
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->bgcol = (SDL_Color) {0, 0, 0, 0};
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
  pEditor->pWorkList_Counter = pBuf;
  add_to_gui_list(ID_LABEL, pBuf);
  /* --------------------------- */
  
  /* create production proggres label or rename worklist edit */
  if(pCity) {
    /* count == cost */
    /* turns == progress */
    const char *name = city_production_name_translation(pCity);
    count = city_production_build_shield_cost(pCity);
    
    if (city_production_has_flag(pCity, IF_GOLD))
    {
      my_snprintf(cBuf, sizeof(cBuf),
      	_("%s\n%d gold per turn"), name, MAX(0, pCity->surplus[O_SHIELD]));
    } else {
      if(pCity->shield_stock < count) {
        turns = city_production_turns_to_build(pCity, TRUE);
        if(turns == 999)
        {
          my_snprintf(cBuf, sizeof(cBuf), _("%s\nblocked!"), name);
        } else {
          my_snprintf(cBuf, sizeof(cBuf), _("%s\n%d %s"),
		    name, turns, PL_("turn", "turns", turns));
        }
      } else {
        my_snprintf(cBuf, sizeof(cBuf), _("%s\nfinished!"), name);
      }
    }
    pStr = create_str16_from_char(cBuf, adj_font(10));
    pStr->style |= SF_CENTER;
    pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
    
    pEditor->pProduction_Name = pBuf;
    add_to_gui_list(ID_LABEL, pBuf);
    
    pIcon = get_progress_icon(pCity->shield_stock, count, &turns);
    
    my_snprintf(cBuf, sizeof(cBuf), "%d%%" , turns);
    pStr = create_str16_from_char(cBuf, adj_font(12));
    pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
    
    pBuf = create_iconlabel(pIcon, pWindow->dst, pStr,
    		(WF_RESTORE_BACKGROUND|WF_ICON_CENTER|WF_FREE_THEME));
    
    pIcon = NULL;
    turns = 0;
    pEditor->pProduction_Progres = pBuf;
    add_to_gui_list(ID_LABEL, pBuf);
  } else {
    pBuf = create_edit_from_chars(NULL, pWindow->dst,  pWorkList->name, adj_font(10),
                                    adj_size(120), WF_RESTORE_BACKGROUND);
    pBuf->action = rename_worklist_editor_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    
    add_to_gui_list(ID_EDIT, pBuf);
  }
  
  /* --------------------------- */
  /* Commit Widget */
  pBuf = create_themeicon(pTheme->OK_Icon, pWindow->dst, WF_RESTORE_BACKGROUND);
  
  pBuf->action = ok_worklist_editor_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_RETURN;
    
  add_to_gui_list(ID_BUTTON, pBuf);
  /* --------------------------- */
  /* Cancel Widget */
  pBuf = create_themeicon(pTheme->CANCEL_Icon, pWindow->dst,
				  WF_RESTORE_BACKGROUND);
  
  pBuf->action = popdown_worklist_editor_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
    
  add_to_gui_list(ID_BUTTON, pBuf);
  /* --------------------------- */
  /* work list */
  
  /*
     pWidget->data filed will contains postion of target in worklist all
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
#endif
  
  if(pCity) {
   /* Production Widget Label */ 
    pStr = create_str16_from_char(city_production_name_translation(pCity), adj_font(10));
    turns = city_production_build_shield_cost(pCity);
    pStr->style |= SF_CENTER;
    pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
    
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
  
  worklist = pWorkList;
    
  worklist_iterate(worklist, prod) {

    if(VUT_UTYPE == prod.kind) {
      pStr = create_str16_from_char(utype_name_translation(prod.value.utype),
                                    adj_font(10));
    } else {
      pStr = create_str16_from_char(city_improvement_name_translation(pCity,
                                                           prod.value.building),
                                    adj_font(10));
    }
    pStr->style |= SF_CENTER;
    pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
				(WF_RESTORE_BACKGROUND|WF_FREE_DATA));
    
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = worklist_editor_item_callback;
        
    pBuf->data.ptr = fc_calloc(1, sizeof(int));
    *((int *)pBuf->data.ptr) = count;
    
    add_to_gui_list(MAX_ID - cid_encode(prod), pBuf);
    
    count++;
    
    if(count > pEditor->pWork->pScroll->active - 1) {
      set_wflag(pBuf, WF_HIDDEN);
  }
  
  } worklist_iterate_end;
  
  if(count) {
    if(!pCity) {
      pEditor->pWork->pEndWidgetList = pLast->prev;
      pEditor->pWork->pEndActiveWidgetList = pEditor->pWork->pEndWidgetList;
    }
    pEditor->pWork->pBeginWidgetList = pBuf;
    pEditor->pWork->pBeginActiveWidgetList = pEditor->pWork->pBeginWidgetList;
  } else {
    if(!pCity) {
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
  if(count <= pEditor->pWork->pScroll->active + 1) {
    if(count > 0) {
      struct widget *pTmp = pLast;
      do {
        pTmp = pTmp->prev;
        clear_wflag(pTmp, WF_HIDDEN);
      } while (pTmp != pBuf);
    }
    hide_scrollbar(pEditor->pWork->pScroll);
  }
#endif
  
  pEditor->pWork->pScroll->count += count;
  pLast = pEditor->pWork->pBeginWidgetList;
  
  /* --------------------------- */
  /* global worklists */
  if(pCity) {
    count = 0;
    for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
      if (client.worklists[i].is_valid) {
        pBuf = create_iconlabel_from_chars(NULL, pWindow->dst, 
      		client.worklists[i].name, adj_font(10),
					      WF_RESTORE_BACKGROUND);
        set_wstate(pBuf, FC_WS_NORMAL);
        add_to_gui_list(MAX_ID - i, pBuf);
        pBuf->string16->style |= SF_CENTER;
        pBuf->action = global_worklist_callback;
        pBuf->string16->fgcol = bg_color;
	
        count++;
    
        if(count>4) {
	  set_wflag(pBuf, WF_HIDDEN);
        }
      }
    }
    if(count) {
      pEditor->pGlobal = fc_calloc(1, sizeof(struct ADVANCED_DLG));
      pEditor->pGlobal->pEndWidgetList = pLast->prev;
      pEditor->pGlobal->pEndActiveWidgetList = pEditor->pGlobal->pEndWidgetList;
      pEditor->pGlobal->pBeginWidgetList = pBuf;
      pEditor->pGlobal->pBeginActiveWidgetList = pEditor->pGlobal->pBeginWidgetList;
    
      if(count > 6) {
        pEditor->pGlobal->pActiveWidgetList = pEditor->pGlobal->pEndActiveWidgetList;

        create_vertical_scrollbar(pEditor->pGlobal, 1,
                             pEditor->pGlobal->pScroll->active, FALSE, TRUE);
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
  pStr = create_string16(NULL, 0, adj_font(10));
  pStr->style |= (SF_CENTER|TTF_STYLE_BOLD);
  pStr->bgcol = (SDL_Color) {0, 0, 0, 0};
    
  improvement_iterate(pImprove) {
    can_build = can_player_build_improvement_now(client.conn.playing, pImprove);
    can_eventually_build =
	can_player_build_improvement_later(client.conn.playing, pImprove);
    
    /* If there's a city, can the city build the improvement? */
    if (pCity) {
      can_build = can_build && can_city_build_improvement_now(pCity, pImprove);
      can_eventually_build = can_eventually_build &&
	  can_city_build_improvement_later(pCity, pImprove);
    }
    
    if ((advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {

      pIcon = crop_rect_from_surface(pMain, NULL);
      
      my_snprintf(cBuf, sizeof(cBuf), "%s", improvement_name_translation(pImprove));
      copy_chars_to_string16(pStr, cBuf);
      pStr->style |= TTF_STYLE_BOLD;
      pText_Name = create_text_surf_smaller_that_w(pStr, pIcon->w - 4);
  
      if (is_wonder(pImprove)) {
        if (improvement_obsolete(client.conn.playing, pImprove)) {
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
  
      if(pCity) {
        if(!improvement_has_flag(pImprove, IF_GOLD)) {
          turns = city_turns_to_build(pCity, cid_production(cid_encode_building(pImprove)), TRUE);
            
          if (turns == FC_INFINITY) {
	    if(state) {
              my_snprintf(cBuf, sizeof(cBuf), _("(%s)\n%d/%d %s\n%s"),
			  state, pCity->shield_stock,
			  impr_build_shield_cost(pImprove),
			  PL_("shield", "shields",
			      impr_build_shield_cost(pImprove)),
			  _("never"));
	    } else {
	      my_snprintf(cBuf, sizeof(cBuf), _("%d/%d %s\n%s"),
			  pCity->shield_stock, impr_build_shield_cost(pImprove),
			  PL_("shield","shields",
			      impr_build_shield_cost(pImprove)), _("never"));
	    }	  
          } else {
            if (state) {
	      my_snprintf(cBuf, sizeof(cBuf), _("(%s)\n%d/%d %s\n%d %s"),
			  state, pCity->shield_stock,
			  impr_build_shield_cost(pImprove),
			  PL_("shield","shields",
			      impr_build_shield_cost(pImprove)),
			  turns, PL_("turn", "turns", turns));
            } else {
	      my_snprintf(cBuf, sizeof(cBuf), _("%d/%d %s\n%d %s"),
			  pCity->shield_stock, impr_build_shield_cost(pImprove),
			  PL_("shield","shields",
			      impr_build_shield_cost(pImprove)),
			  turns, PL_("turn", "turns", turns));
            }
          }
        } else {
          /* capitalization */
          my_snprintf(cBuf, sizeof(cBuf), _("%d gold per turn"),
		    MAX(0, pCity->surplus[O_SHIELD]));
        }
      } else {
        /* non city mode */
        if(!improvement_has_flag(pImprove, IF_GOLD)) {
          if(state) {
            my_snprintf(cBuf, sizeof(cBuf), _("(%s)\n%d %s"),
			state, impr_build_shield_cost(pImprove),
			PL_("shield","shields",
			    impr_build_shield_cost(pImprove)));
          } else {
	    my_snprintf(cBuf, sizeof(cBuf), _("%d %s"),
			impr_build_shield_cost(pImprove),
			PL_("shield","shields",
			    impr_build_shield_cost(pImprove)));
          }
        } else {
          my_snprintf(cBuf, sizeof(cBuf), _("shields into gold"));
        }
      }
  
      copy_chars_to_string16(pStr, cBuf);
      pStr->style &= ~TTF_STYLE_BOLD;
  
      pText = create_text_surf_from_str16(pStr);

      /*-----------------*/
      
      pZoom = get_building_surface(pImprove);
      pZoom = zoomSurface(pZoom, DEFAULT_ZOOM * ((float)54 / pZoom->w), DEFAULT_ZOOM * ((float)54 / pZoom->w), 1);
      dst.x = (pIcon->w - pZoom->w)/2;
      dst.y = (pIcon->h/2 - pZoom->h)/2;
      alphablit(pZoom, NULL, pIcon, &dst);
      dst.y += pZoom->h;
      FREESURFACE(pZoom);
  
      dst.x = (pIcon->w - pText_Name->w)/2;
      dst.y += ((pIcon->h - dst.y) - (pText_Name->h + pText->h))/2;
      alphablit(pText_Name, NULL, pIcon, &dst);

      dst.x = (pIcon->w - pText->w)/2;
      dst.y += pText_Name->h;
      alphablit(pText, NULL, pIcon, &dst);
  
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
      
      if(count > (TARGETS_ROW * TARGETS_COL - 1)) {
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
      can_eventually_build = can_eventually_build &&
	  		can_city_build_unit_later(pCity, un);
    }

    if ((advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {
	  
      pUnit = un;
	
      pIcon = crop_rect_from_surface(pMain, NULL);
      
      my_snprintf(cBuf, sizeof(cBuf), "%s", utype_name_translation(un));
  
      copy_chars_to_string16(pStr, cBuf);
      pStr->style |= TTF_STYLE_BOLD;
      pText_Name = create_text_surf_smaller_that_w(pStr, pIcon->w - 4);
  
      if (pCity) {
        turns = city_turns_to_build(pCity, cid_production(cid_encode_unit(un)), TRUE);
        if (turns == FC_INFINITY) {
          my_snprintf(cBuf, sizeof(cBuf),
		    _("(%d/%d/%d)\n%d/%d %s\nnever"),
		    pUnit->attack_strength,
		    pUnit->defense_strength, pUnit->move_rate / SINGLE_MOVE,
		    pCity->shield_stock, utype_build_shield_cost(un),
	  	    PL_("shield","shields", utype_build_shield_cost(un)));
        } else {
          my_snprintf(cBuf, sizeof(cBuf),
		    _("(%d/%d/%d)\n%d/%d %s\n%d %s"),
		    pUnit->attack_strength,
		    pUnit->defense_strength, pUnit->move_rate / SINGLE_MOVE,
		    pCity->shield_stock, utype_build_shield_cost(un), 
	  	    PL_("shield","shields", utype_build_shield_cost(un)),
		    turns, PL_("turn", "turns", turns));
        }
      } else {
        my_snprintf(cBuf, sizeof(cBuf),
		    _("(%d/%d/%d)\n%d %s"),
		    pUnit->attack_strength,
		    pUnit->defense_strength, pUnit->move_rate / SINGLE_MOVE,
		    utype_build_shield_cost(un),
		    PL_("shield","shields", utype_build_shield_cost(un)));
      }

      copy_chars_to_string16(pStr, cBuf);
      pStr->style &= ~TTF_STYLE_BOLD;
  
      pText = create_text_surf_from_str16(pStr);
  
      pZoom = adj_surf(get_unittype_surface(un));
      dst.x = (pIcon->w - pZoom->w)/2;
      dst.y = (pIcon->h/2 - pZoom->h)/2;
      alphablit(pZoom, NULL, pIcon, &dst);
      FREESURFACE(pZoom);
  
      dst.x = (pIcon->w - pText_Name->w)/2;
      dst.y = pIcon->h/2 + (pIcon->h/2 - (pText_Name->h + pText->h))/2;
      alphablit(pText_Name, NULL, pIcon, &dst);

      dst.x = (pIcon->w - pText->w)/2;
      dst.y += pText_Name->h;
      alphablit(pText, NULL, pIcon, &dst);
  
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
      
      if(count > (TARGETS_ROW * TARGETS_COL - 1)) {
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
  if(count > (TARGETS_ROW * TARGETS_COL - 1)) {
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
  if(resize_window(pWindow, pIcon, NULL,
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
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_BACKGROUND)));
  putframe(pWindow->theme, dst.x, dst.y, dst.x + dst.w - 1, dst.y + dst.h - 1,
     map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_WLDLG_FRAME)));
  putframe(pWindow->theme, dst.x + 2, dst.y + 2, dst.x + dst.w - 3, dst.y + dst.h - 3,
     map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_WLDLG_FRAME)));
  
  dst.x = area.x;
  dst.y += dst.h + adj_size(2);
  dst.w = adj_size(130);
  dst.h = adj_size(228);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color2);
  putframe(pWindow->theme, dst.x, dst.y, dst.x + dst.w - 1, dst.y + dst.h - 1,
     map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_WLDLG_FRAME)));
  
  if(pEditor->pGlobal) {
    dst.x = area.x;
    dst.y += dst.h + adj_size(2);
    dst.w = adj_size(130);
    dst.h = pWindow->size.h - dst.y - adj_size(4);

    SDL_FillRect(pWindow->theme, &dst,
      map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_BACKGROUND)));
    putframe(pWindow->theme, dst.x, dst.y, dst.x + dst.w - 1, dst.y + dst.h - 1,
      map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_WLDLG_FRAME)));
    putframe(pWindow->theme, dst.x + adj_size(2), dst.y + adj_size(2),
      dst.x + dst.w - adj_size(3), dst.y + dst.h - adj_size(3),
      map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_WLDLG_FRAME)));
  }

  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);
  
  /* name */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w)/2;
  pBuf->size.y = area.y + adj_size(4);
  
  /* size of worklist (without production) */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w)/2;
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;
  
  if(pCity) {
    /* current build and proggrse bar */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w)/2;
    pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h + adj_size(5);
    
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w)/2;
    pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;
  } else {
    /* rename worklist */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + (adj_size(130) - pBuf->size.w)/2;
    pBuf->size.y = area.y + 1 + (adj_size(145) - pBuf->size.h)/2;
  }
  
  /* ok button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + (adj_size(65) - pBuf->size.w)/2;
  pBuf->size.y = area.y + adj_size(135) - pBuf->size.h;
  
  /* exit button */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + adj_size(65) + (adj_size(65) - pBuf->size.w)/2;
  pBuf->size.y = area.y + adj_size(135) - pBuf->size.h;
  
  /* worklist */
  /* pEditor->pWork->pScroll->count: including production */
  if(pCity || (worklist_length(pWorkList) > 0)) {
    /* FIXME */
    setup_vertical_widgets_position(1,
      area.x + adj_size(2), area.y + adj_size(152)/* +
	((pEditor->pWork->pScroll->count > pEditor->pWork->pScroll->active + 2) ?
	    pEditor->pWork->pScroll->pUp_Left_Button->size.h + 1 : 0)*/,
	adj_size(126), 0, pEditor->pWork->pBeginWidgetList,
		  pEditor->pWork->pEndWidgetList);

    setup_vertical_scrollbar_area(pEditor->pWork->pScroll,
	area.x + adj_size(2),
    	area.y + adj_size(152),
    	adj_size(225), FALSE);
  }
  
  /* global worklists */
  if(pEditor->pGlobal) {
    setup_vertical_widgets_position(1,
      area.x + adj_size(4),
      area.y + adj_size(384) +
	(pEditor->pGlobal->pScroll ?
	    pEditor->pGlobal->pScroll->pUp_Left_Button->size.h + 1 : 0),
		adj_size(122), 0, pEditor->pGlobal->pBeginWidgetList,
		  	pEditor->pGlobal->pEndWidgetList);
    
    if(pEditor->pGlobal->pScroll) {
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
    
  if(pEditor->pTargets->pScroll) {
    setup_vertical_scrollbar_area(pEditor->pTargets->pScroll,
	area.x + area.w,
    	area.y + 1,
    	area.h - 1, TRUE);
    
  }
    
  /* ----------------------------------- */
  FREESTRING16(pStr);
  FREESURFACE(pMain);
  
  redraw_group(pEditor->pBeginWidgetList, pWindow, 0);
  widget_flush(pWindow);
}
  
void popdown_worklist_editor(void)
{
  if(pEditor) {
    popdown_window_group_dialog(pEditor->pBeginWidgetList,
                                            pEditor->pEndWidgetList);
    FC_FREE(pEditor->pTargets->pScroll);
    FC_FREE(pEditor->pTargets);
    
    FC_FREE(pEditor->pWork->pScroll);
    FC_FREE(pEditor->pWork);
    
    if(pEditor->pGlobal) {
      FC_FREE(pEditor->pGlobal->pScroll);
      FC_FREE(pEditor->pGlobal);
    }

    FC_FREE(pEditor->pCopy_WorkList);
        
    if(city_dialog_is_open(pEditor->pCity)) {
      enable_city_dlg_widgets();
    }
  
    FC_FREE(pEditor);
    
    flush_dirty();
  }
}
