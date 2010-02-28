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
                          gui_stuff.c  -  description
                             -------------------
    begin                : June 30 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SDL.h"

#include "log.h"

/* gui-sdl */
#include "colors.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "themespec.h"
#include "unistring.h"

#include "widget.h"
#include "widget_p.h"

struct widget *pSellected_Widget;
SDL_Rect *pInfo_Area = NULL;

extern Uint32 widget_info_counter;

/* ================================ Private ============================ */

static struct widget *pBeginMainWidgetList;
/* static struct widget *pEndMainWidgetList; */

static SDL_Surface *pInfo_Label = NULL;

/**************************************************************************
  Correct backgroud size ( set min size ). Used in create widget
  functions.
**************************************************************************/
void correct_size_bcgnd_surf(SDL_Surface * pTheme,
				    Uint16 * pWidth, Uint16 * pHigh)
{
  *pWidth = MAX(*pWidth, 2 * (pTheme->w / adj_size(16)));
  *pHigh = MAX(*pHigh, 2 * (pTheme->h / adj_size(16)));
}

/**************************************************************************
  Create background image for buttons, iconbuttons and edit fields
  then return  pointer to this image.

  Graphic is taken from pTheme surface and blit to new created image.

  Length and height depend of iText_with, iText_high parameters.

  Type of image depend of "state" parameter.
    state = 0 - normal
    state = 1 - selected
    state = 2 - pressed
    state = 3 - disabled
**************************************************************************/
SDL_Surface *create_bcgnd_surf(SDL_Surface * pTheme, Uint8 state,
                               Uint16 Width, Uint16 High)
{
  bool zoom;
  int iTile_width_len_end, iTile_width_len_mid, iTile_count_len_mid;
  int iTile_width_high_end, iTile_width_high_mid, iTile_count_high_mid;
  int i, j;

  SDL_Rect src, des;
  SDL_Surface *pBackground = NULL;

  int iStart_y = (pTheme->h / 4) * state;

  iTile_width_len_end = pTheme->w / 16;
  iTile_width_len_mid = pTheme->w - (iTile_width_len_end * 2);

  iTile_count_len_mid =
      (Width - (iTile_width_len_end * 2)) / iTile_width_len_mid;

  /* corrections I */
  if (((iTile_count_len_mid *
	iTile_width_len_mid) + (iTile_width_len_end * 2)) < Width) {
    iTile_count_len_mid++;
  }

  iTile_width_high_end = pTheme->h / 16;
  iTile_width_high_mid = (pTheme->h / 4) - (iTile_width_high_end * 2);
  iTile_count_high_mid =
      (High - (iTile_width_high_end * 2)) / iTile_width_high_mid;

  /* corrections II */
  if (((iTile_count_high_mid *
	iTile_width_high_mid) + (iTile_width_high_end * 2)) < High) {
    iTile_count_high_mid++;
  }

  i = MAX(iTile_width_len_end * 2, Width);
  j = MAX(iTile_width_high_end * 2, High);
  zoom = ((i != Width) ||  (j != High));
  
  /* now allocate memory */
  pBackground = create_surf_alpha(i, j, SDL_SWSURFACE);

  /* copy left end */

  /* copy left top end */
  src.x = 0;
  src.y = iStart_y;
  src.w = iTile_width_len_end;
  src.h = iTile_width_high_end;

  des.x = 0;
  des.y = 0;
  alphablit(pTheme, &src, pBackground, &des);

  /* copy left middels parts */
  src.y = iStart_y + iTile_width_high_end;
  src.h = iTile_width_high_mid;
  for (i = 0; i < iTile_count_high_mid; i++) {
    des.y = iTile_width_high_end + i * iTile_width_high_mid;
    alphablit(pTheme, &src, pBackground, &des);
  }

  /* copy left boton end */
  src.y = iStart_y + ((pTheme->h / 4) - iTile_width_high_end);
  src.h = iTile_width_high_end;
  des.y = pBackground->h - iTile_width_high_end;
  clear_surface(pBackground, &des);
  alphablit(pTheme, &src, pBackground, &des);

  /* copy middle parts without right end part */

  src.x = iTile_width_len_end;
  src.y = iStart_y;
  src.w = iTile_width_len_mid;

  for (i = 0; i < iTile_count_len_mid; i++) {

    /* top */
    des.x = iTile_width_len_end + i * iTile_width_len_mid;
    des.y = 0;
    src.y = iStart_y;
    alphablit(pTheme, &src, pBackground, &des);

    /*  middels */
    src.y = iStart_y + iTile_width_high_end;
    src.h = iTile_width_high_mid;
    for (j = 0; j < iTile_count_high_mid; j++) {
      des.y = iTile_width_high_end + j * iTile_width_high_mid;
      alphablit(pTheme, &src, pBackground, &des);
    }

    /* bottom */
    src.y = iStart_y + ((pTheme->h / 4) - iTile_width_high_end);
    src.h = iTile_width_high_end;
    des.y = pBackground->h - iTile_width_high_end;
    clear_surface(pBackground, &des);    
    alphablit(pTheme, &src, pBackground, &des);
  }

  /* copy right end */
  src.x = pTheme->w - iTile_width_len_end;
  src.y = iStart_y;
  src.w = iTile_width_len_end;

  des.x = pBackground->w - iTile_width_len_end;
  des.y = 0;

  alphablit(pTheme, &src, pBackground, &des);

  /*  middels */
  src.y = iStart_y + iTile_width_high_end;
  src.h = iTile_width_high_mid;
  for (i = 0; i < iTile_count_high_mid; i++) {
    des.y = iTile_width_high_end + i * iTile_width_high_mid;
    alphablit(pTheme, &src, pBackground, &des);
  }

  /*boton */
  src.y = iStart_y + ((pTheme->h / 4) - iTile_width_high_end);
  src.h = iTile_width_high_end;
  des.y = pBackground->h - iTile_width_high_end;
  clear_surface(pBackground, &des);  
  alphablit(pTheme, &src, pBackground, &des);
  
  if (zoom)
  {
    SDL_Surface *pZoom = ResizeSurface(pBackground, Width, High, 1);
    FREESURFACE(pBackground);
    pBackground = pZoom;
  }
  
  return pBackground;
}

/* =================================================== */
/* ===================== WIDGET LIST ==================== */
/* =================================================== */

/**************************************************************************
  Find the next visible widget in the widget list starting with
  pStartWidget that is drawn at position (x, y). If pStartWidget is NULL,
  the search starts with the first entry of the main widget list. 
**************************************************************************/
struct widget *find_next_widget_at_pos(struct widget *pStartWidget, int x, int y)
{
  SDL_Rect area = {0, 0, 0, 0};
  struct widget *pWidget;
  
  pWidget = pStartWidget ? pStartWidget : pBeginMainWidgetList;
  
  while (pWidget) {
    area.x = pWidget->dst->dest_rect.x + pWidget->size.x;
    area.y = pWidget->dst->dest_rect.y + pWidget->size.y;
    area.w = pWidget->size.w;
    area.h = pWidget->size.h;
    if (is_in_rect_area(x, y, area)
       && !((get_wflags(pWidget) & WF_HIDDEN) == WF_HIDDEN)) {
      return (struct widget *) pWidget;
    }
    pWidget = pWidget->next;
  }
  return NULL;
}

/**************************************************************************
  Find the next enabled and visible widget in the widget list starting
  with pStartWidget that handles the given key. If pStartWidget is NULL,
  the search starts with the first entry of the main widget list.
  NOTE: This function ignores CapsLock and NumLock Keys.
**************************************************************************/
struct widget *find_next_widget_for_key(struct widget *pStartWidget,
                                        SDL_keysym key)
{
  struct widget *pWidget;
  
  pWidget = pStartWidget ? pStartWidget : pBeginMainWidgetList;
  
  key.mod &= ~(KMOD_NUM | KMOD_CAPS);
  while (pWidget) {
    if ((pWidget->key == key.sym ||
      (pWidget->key == SDLK_RETURN && key.sym == SDLK_KP_ENTER) ||
      (pWidget->key == SDLK_KP_ENTER && key.sym == SDLK_RETURN)) &&
	((pWidget->mod & key.mod) || (pWidget->mod == key.mod))) {
      if (!((get_wstate(pWidget) == FC_WS_DISABLED) ||
	    ((get_wflags(pWidget) & WF_HIDDEN) == WF_HIDDEN))) {
	return (struct widget *) pWidget;
      }
    }
    pWidget = pWidget->next;
  }
  return NULL;
}

/**************************************************************************
  Do default Widget action when pressed, and then call widget callback
  function.

  example for Button:
    set flags FW_Pressed
    redraw button ( pressed )
    refresh screen ( to see result )
    wait 300 ms	( to see result :)
    If exist (button callback function) then
      call (button callback function)

    Function normal return Widget ID.
    NOTE: NOZERO return of this function deterninate exit of
        MAIN_SDL_GAME_LOOP
    if ( pWidget->action )
      if ( pWidget->action(pWidget)  ) ID = 0;
    if widget callback function return = 0 then return NOZERO
    I default return (-1) from Widget callback functions.
**************************************************************************/
Uint16 widget_pressed_action(struct widget * pWidget)
{
  Uint16 ID = 0;

  if (!pWidget) {
    return 0;
  }
  
  widget_info_counter = 0;
  if (pInfo_Area) {
    sdl_dirty_rect(*pInfo_Area);
    FC_FREE(pInfo_Area);
    FREESURFACE(pInfo_Label);
  }
  
  switch (get_wtype(pWidget)) {
    case WT_TI_BUTTON:
    case WT_I_BUTTON:    
    case WT_ICON:
    case WT_ICON2:
      if (Main.event.button.button == SDL_BUTTON_LEFT) {
        set_wstate(pWidget, FC_WS_PRESSED);
        widget_redraw(pWidget);
        widget_mark_dirty(pWidget);
        flush_dirty();
        set_wstate(pWidget, FC_WS_SELLECTED);
        SDL_Delay(300);
      }
      ID = pWidget->ID;
      if (pWidget->action) {
        if (pWidget->action(pWidget)) {
          ID = 0;
        }
      }
      break;

    case WT_EDIT:
    {
      if (Main.event.button.button == SDL_BUTTON_LEFT) {
        bool ret, loop = ((get_wflags(pWidget) & WF_EDIT_LOOP) == WF_EDIT_LOOP);
        enum Edit_Return_Codes change;
        do {
          ret = FALSE;
          change = edit_field(pWidget);
          if (change != ED_FORCE_EXIT && (!loop || change != ED_RETURN)) {
            widget_redraw(pWidget);
            widget_mark_dirty(pWidget);
            flush_dirty();
          }
          if (change != ED_FORCE_EXIT && change != ED_ESC && pWidget->action) {
            if (pWidget->action(pWidget)) {
              ID = 0;
            }
          }
          if (loop && change == ED_RETURN) {
            ret = TRUE;
          }
        } while(ret);
        ID = 0;
      }
      break;
    }
    case WT_VSCROLLBAR:
    case WT_HSCROLLBAR:
      if (Main.event.button.button == SDL_BUTTON_LEFT) {
        set_wstate(pWidget, FC_WS_PRESSED);
        widget_redraw(pWidget);
        widget_mark_dirty(pWidget);
        flush_dirty();
      }
      ID = pWidget->ID;
      if (pWidget->action) {
        if (pWidget->action(pWidget)) {
          ID = 0;
        }
      }
      break;
    case WT_CHECKBOX:
    case WT_TCHECKBOX:
      if (Main.event.button.button == SDL_BUTTON_LEFT) {
        set_wstate(pWidget, FC_WS_PRESSED);
        widget_redraw(pWidget);
        widget_mark_dirty(pWidget);
        flush_dirty();
        set_wstate(pWidget, FC_WS_SELLECTED);
        togle_checkbox(pWidget);
        SDL_Delay(300);
      }
      ID = pWidget->ID;  
      if (pWidget->action) {
        if (pWidget->action(pWidget)) {
          ID = 0;
        }
      }
      break;
    default:
      ID = pWidget->ID;
      if (pWidget->action) {
        if (pWidget->action(pWidget) != 0) {
          ID = 0;
        }
      }
      break;
  }

  return ID;
}

/**************************************************************************
  Unsellect (sellected) widget and redraw this widget;
**************************************************************************/
void unsellect_widget_action(void)
{
  if (pSellected_Widget && (get_wstate(pSellected_Widget) != FC_WS_DISABLED)) {
    
      set_wstate(pSellected_Widget, FC_WS_NORMAL);
  
      if (!(get_wflags(pSellected_Widget) & WF_HIDDEN)) {
        pSellected_Widget->unselect(pSellected_Widget);
        
        /* turn off quick info timer/counter */ 
        widget_info_counter = 0;
      }
  }

  if (pInfo_Area) {
    flush_rect(*pInfo_Area, FALSE);
    FC_FREE(pInfo_Area);
    FREESURFACE(pInfo_Label);    
  }

  pSellected_Widget = NULL;
}

/**************************************************************************
  Sellect widget.  Redraw this widget;
**************************************************************************/
void widget_sellected_action(struct widget *pWidget)
{
  if (!pWidget || pWidget == pSellected_Widget) {
    return;
  }

  if (pSellected_Widget) {
    unsellect_widget_action();
  }

  set_wstate(pWidget, FC_WS_SELLECTED);  
  
  pWidget->select(pWidget);
  
  pSellected_Widget = pWidget;
  
  if (get_wflags(pWidget) & WF_WIDGET_HAS_INFO_LABEL) {
    widget_info_counter = 1;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void redraw_widget_info_label(SDL_Rect *rect)
{
  SDL_Surface *pText, *pBcgd;
  SDL_Rect srcrect, dstrect;
  SDL_Color color;

  struct widget *pWidget = pSellected_Widget;

  if (!pWidget) {
    return;
  }

  if (!pInfo_Label) {
  
    pInfo_Area = fc_calloc(1, sizeof(SDL_Rect));
  
    color = pWidget->string16->fgcol;
    pWidget->string16->style |= TTF_STYLE_BOLD;
    pWidget->string16->fgcol = *get_game_colorRGB(COLOR_THEME_QUICK_INFO_TEXT);
    
    /* create string and bcgd theme */
    pText = create_text_surf_from_str16(pWidget->string16);

    pWidget->string16->fgcol = color;
    
    pBcgd = create_filled_surface(pText->w + adj_size(10), pText->h + adj_size(6),
              SDL_SWSURFACE, get_game_colorRGB(COLOR_THEME_QUICK_INFO_BG), TRUE);
    
    /* calculate start position */
    if ((pWidget->dst->dest_rect.y + pWidget->size.y) - pBcgd->h - adj_size(6) < 0) {
      pInfo_Area->y = (pWidget->dst->dest_rect.y + pWidget->size.y) + pWidget->size.h + adj_size(3);
    } else {
      pInfo_Area->y = (pWidget->dst->dest_rect.y + pWidget->size.y) - pBcgd->h - adj_size(5);
    }
  
    if ((pWidget->dst->dest_rect.x + pWidget->size.x) + pBcgd->w + adj_size(5) > Main.screen->w) {
      pInfo_Area->x = (pWidget->dst->dest_rect.x + pWidget->size.x) - pBcgd->w - adj_size(5);
    } else {
      pInfo_Area->x = (pWidget->dst->dest_rect.x + pWidget->size.x) + adj_size(3);
    }
  
    pInfo_Area->w = pBcgd->w + adj_size(2);
    pInfo_Area->h = pBcgd->h + adj_size(3);

    pInfo_Label = SDL_DisplayFormatAlpha(pBcgd);

    FREESURFACE(pBcgd);
    
    /* draw text */
    dstrect.x = adj_size(6);
    dstrect.y = adj_size(3);
    
    alphablit(pText, NULL, pInfo_Label, &dstrect);
    
    FREESURFACE(pText);    
    
    /* draw frame */
    putframe(pInfo_Label, 0, 0,
         pInfo_Label->w - 1, pInfo_Label->h - 1,
         map_rgba(pInfo_Label->format, *get_game_colorRGB(COLOR_THEME_QUICK_INFO_FRAME)));
    
  }

  if (rect) {
    dstrect.x = MAX(rect->x, pInfo_Area->x);
    dstrect.y = MAX(rect->y, pInfo_Area->y);
    
    srcrect.x = dstrect.x - pInfo_Area->x;
    srcrect.y = dstrect.y - pInfo_Area->y;
    srcrect.w = MIN((pInfo_Area->x + pInfo_Area->w), (rect->x + rect->w)) - dstrect.x;
    srcrect.h = MIN((pInfo_Area->y + pInfo_Area->h), (rect->y + rect->h)) - dstrect.y;

    alphablit(pInfo_Label, &srcrect, Main.screen, &dstrect);
  } else {
    alphablit(pInfo_Label, NULL, Main.screen, pInfo_Area);
  }

  if (correct_rect_region(pInfo_Area)) {
    SDL_UpdateRect(Main.screen, pInfo_Area->x, pInfo_Area->y,
				    pInfo_Area->w, pInfo_Area->h);
  }
  
  return;
}

/**************************************************************************
  Find ID in Widget's List ('pGUI_List') and return pointer to this
  Widgets.
**************************************************************************/
struct widget *get_widget_pointer_form_ID(const struct widget *pGUI_List,
				       Uint16 ID, enum scan_direction direction)
{
  while (pGUI_List) {
    if (pGUI_List->ID == ID) {
      return (struct widget *) pGUI_List;
    }
    if (direction == SCAN_FORWARD)
    pGUI_List = pGUI_List->next;
    else
      pGUI_List = pGUI_List->prev;  
  }
  return NULL;
}

/**************************************************************************
  Find ID in MAIN Widget's List ( pBeginWidgetList ) and return pointer to
  this Widgets.
**************************************************************************/
struct widget *get_widget_pointer_form_main_list(Uint16 ID)
{
  return get_widget_pointer_form_ID(pBeginMainWidgetList, ID, SCAN_FORWARD);
}

/**************************************************************************
  Add Widget to Main Widget's List ( pBeginWidgetList )
**************************************************************************/
void add_to_gui_list(Uint16 ID, struct widget *pGUI)
{
  if (pBeginMainWidgetList != NULL) {
    pGUI->next = pBeginMainWidgetList;
    pGUI->ID = ID;
    pBeginMainWidgetList->prev = pGUI;
    pBeginMainWidgetList = pGUI;
  } else {
    pBeginMainWidgetList = pGUI;
    pBeginMainWidgetList->ID = ID;
  }
}

/**************************************************************************
  Add Widget to Widget's List at pAdd_Dock position on 'prev' slot.
**************************************************************************/
void DownAdd(struct widget *pNew_Widget, struct widget *pAdd_Dock)
{
  pNew_Widget->next = pAdd_Dock;
  pNew_Widget->prev = pAdd_Dock->prev;
  if (pAdd_Dock->prev) {
    pAdd_Dock->prev->next = pNew_Widget;
  }
  pAdd_Dock->prev = pNew_Widget;
  if (pAdd_Dock == pBeginMainWidgetList) {
    pBeginMainWidgetList = pNew_Widget;
  }
}

/**************************************************************************
  Delete Widget from Main Widget's List ( pBeginWidgetList )

  NOTE: This function does not destroy Widget, only remove his pointer from
  list. To destroy this Widget totaly ( free mem... ) call macro:
  del_widget_from_gui_list( pWidget ).  This macro call this function.
**************************************************************************/
void del_widget_pointer_from_gui_list(struct widget *pGUI)
{
  if (!pGUI) {
    return;
  }

  if (pGUI == pBeginMainWidgetList) {
    pBeginMainWidgetList = pBeginMainWidgetList->next;
  }
  
  if (pGUI->prev && pGUI->next) {
    pGUI->prev->next = pGUI->next;
    pGUI->next->prev = pGUI->prev;
  } else {
    if (pGUI->prev) {
      pGUI->prev->next = NULL;
    }

    if (pGUI->next) {
      pGUI->next->prev = NULL;
    }

  }

  if (pSellected_Widget == pGUI) {
    pSellected_Widget = NULL;
  }
}

/**************************************************************************
  Determinate if 'pGui' is first on WidgetList

  NOTE: This is used by My (move) GUI Window mechanism.  Return TRUE if is
  first.
**************************************************************************/
bool is_this_widget_first_on_list(struct widget *pGUI)
{
  return (pBeginMainWidgetList == pGUI);
}

/**************************************************************************
  Move pointer to Widget to list begin.

  NOTE: This is used by My GUI Window mechanism.
**************************************************************************/
void move_widget_to_front_of_gui_list(struct widget *pGUI)
{
  if (!pGUI || pGUI == pBeginMainWidgetList) {
    return;
  }

  /* pGUI->prev always exists because
     we don't do this to pBeginMainWidgetList */
  if (pGUI->next) {
    pGUI->prev->next = pGUI->next;
    pGUI->next->prev = pGUI->prev;
  } else {
    pGUI->prev->next = NULL;
  }

  pGUI->next = pBeginMainWidgetList;
  pBeginMainWidgetList->prev = pGUI;
  pBeginMainWidgetList = pGUI;
  pGUI->prev = NULL;
}

/**************************************************************************
  Delete Main Widget's List.
**************************************************************************/
void del_main_list(void)
{
  del_gui_list(pBeginMainWidgetList);
}

/**************************************************************************
   Delete Wideget's List ('pGUI_List').
**************************************************************************/
void del_gui_list(struct widget *pGUI_List)
{
  while (pGUI_List) {
    if (pGUI_List->next) {
      pGUI_List = pGUI_List->next;
      FREEWIDGET(pGUI_List->prev);
    } else {
      FREEWIDGET(pGUI_List);
    }
  }
}

/* ===================================================================== */
/* ================================ Universal ========================== */
/* ===================================================================== */

/**************************************************************************
  Universal redraw Group of Widget function.  Function is optimized to
  WindowGroup: start draw from 'pEnd' and stop on 'pBegin', in theory
  'pEnd' is window widget;
**************************************************************************/
Uint16 redraw_group(const struct widget *pBeginGroupWidgetList,
		    const struct widget *pEndGroupWidgetList,
		      int add_to_update)
{
  Uint16 count = 0;
  struct widget *pTmpWidget = (struct widget *) pEndGroupWidgetList;

  while (pTmpWidget) {

    if ((get_wflags(pTmpWidget) & WF_HIDDEN) != WF_HIDDEN) {
      
      widget_redraw(pTmpWidget);

      if (add_to_update) {
	widget_mark_dirty(pTmpWidget);
      }

      count++;
    }

    if (pTmpWidget == pBeginGroupWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->prev;

  }

  return count;
}

/**************************************************************************
  ...
**************************************************************************/
void undraw_group(struct widget *pBeginGroupWidgetList,
	          struct widget *pEndGroupWidgetList)
{
  struct widget *pTmpWidget = pEndGroupWidgetList;

  while (pTmpWidget) {
    widget_undraw(pTmpWidget);
    widget_mark_dirty(pTmpWidget);

    if (pTmpWidget == pBeginGroupWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->prev;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void set_new_group_start_pos(const struct widget *pBeginGroupWidgetList,
			     const struct widget *pEndGroupWidgetList,
			     Sint16 Xrel, Sint16 Yrel)
{
  struct widget *pTmpWidget = (struct widget *) pEndGroupWidgetList;

  while (pTmpWidget) {

    widget_set_position(pTmpWidget, pTmpWidget->size.x + Xrel,
                                    pTmpWidget->size.y + Yrel);
    
    if (get_wtype(pTmpWidget) == WT_VSCROLLBAR
      && pTmpWidget->private_data.adv_dlg
      && pTmpWidget->private_data.adv_dlg->pScroll) {
      pTmpWidget->private_data.adv_dlg->pScroll->max += Yrel;
      pTmpWidget->private_data.adv_dlg->pScroll->min += Yrel;
    }
    
    if (get_wtype(pTmpWidget) == WT_HSCROLLBAR
      && pTmpWidget->private_data.adv_dlg
      && pTmpWidget->private_data.adv_dlg->pScroll) {
      pTmpWidget->private_data.adv_dlg->pScroll->max += Xrel;
      pTmpWidget->private_data.adv_dlg->pScroll->min += Xrel;
    }
    
    if (pTmpWidget == pBeginGroupWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->prev;
  }
}

/**************************************************************************
  Move group of Widget's pointers to begin of main list.
  Move group destination buffer to end of buffer array.
  NOTE: This is used by My GUI Window(group) mechanism.
**************************************************************************/
void move_group_to_front_of_gui_list(struct widget *pBeginGroupWidgetList,
				     struct widget *pEndGroupWidgetList)
{
  struct widget *pTmpWidget = pEndGroupWidgetList , *pPrev = NULL;
  struct gui_layer *gui_layer = get_gui_layer(pEndGroupWidgetList->dst->surface);
  
  /* Widget Pointer Menagment */
  while (pTmpWidget) {
    
    pPrev = pTmpWidget->prev;
    
    /* pTmpWidget->prev always exists because we
       don't do this to pBeginMainWidgetList */
    if (pTmpWidget->next) {
      pTmpWidget->prev->next = pTmpWidget->next;
      pTmpWidget->next->prev = pTmpWidget->prev;
    } else {
      pTmpWidget->prev->next = NULL;
    }

    pTmpWidget->next = pBeginMainWidgetList;
    pBeginMainWidgetList->prev = pTmpWidget;
    pBeginMainWidgetList = pTmpWidget;
    pBeginMainWidgetList->prev = NULL;
    
    if (pTmpWidget == pBeginGroupWidgetList) {
      break;
    }

    pTmpWidget = pPrev;
  }

  /* Window Buffer Menagment */  
  if (gui_layer) {
    int i = 0;
    while ((i < Main.guis_count - 1) && Main.guis[i]) {
      if (Main.guis[i] && Main.guis[i + 1] && (Main.guis[i] == gui_layer)) {
        Main.guis[i] = Main.guis[i + 1];
	Main.guis[i + 1] = gui_layer;
      }
      i++;
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
void del_group_of_widgets_from_gui_list(struct widget *pBeginGroupWidgetList,
					struct widget *pEndGroupWidgetList)
{
  struct widget *pBufWidget = NULL;
  struct widget *pTmpWidget = pEndGroupWidgetList;

  if (!pEndGroupWidgetList)
	return;
  
  if (pBeginGroupWidgetList == pEndGroupWidgetList) {
    del_widget_from_gui_list(pTmpWidget);
    return;
  }

  pTmpWidget = pTmpWidget->prev;

  while (pTmpWidget) {

    pBufWidget = pTmpWidget->next;
    del_widget_from_gui_list(pBufWidget);

    if (pTmpWidget == pBeginGroupWidgetList) {
      del_widget_from_gui_list(pTmpWidget);
      break;
    }

    pTmpWidget = pTmpWidget->prev;
  }

}

/**************************************************************************
  ...
**************************************************************************/
void set_group_state(struct widget *pBeginGroupWidgetList,
		     struct widget *pEndGroupWidgetList, enum widget_state state)
{
  struct widget *pTmpWidget = pEndGroupWidgetList;
  while (pTmpWidget) {
    set_wstate(pTmpWidget, state);
    if (pTmpWidget == pBeginGroupWidgetList) {
      break;
    }
    pTmpWidget = pTmpWidget->prev;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void hide_group(struct widget *pBeginGroupWidgetList,
		struct widget *pEndGroupWidgetList)
{
  struct widget *pTmpWidget = pEndGroupWidgetList;

  while (pTmpWidget) {
    set_wflag(pTmpWidget, WF_HIDDEN);

    if (pTmpWidget == pBeginGroupWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->prev;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void show_group(struct widget *pBeginGroupWidgetList,
		struct widget *pEndGroupWidgetList)
{
  struct widget *pTmpWidget = pEndGroupWidgetList;

  while (pTmpWidget) {
    clear_wflag(pTmpWidget, WF_HIDDEN);

    if (pTmpWidget == pBeginGroupWidgetList) {
      break;
    }

    pTmpWidget = pTmpWidget->prev;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void group_set_area(struct widget *pBeginGroupWidgetList,
		    struct widget *pEndGroupWidgetList,
                    SDL_Rect area)
{
  struct widget *pWidget = pEndGroupWidgetList;
    
  while (pWidget) {
    widget_set_area(pWidget, area);
    
    if(pWidget == pBeginGroupWidgetList) {
      break;
    }

    pWidget = pWidget->prev;
  }
}

/* ===================================================================== *
 * =========================== Window Group ============================ *
 * ===================================================================== */

/*
 *	Window Group  -	group with 'pBegin' and 'pEnd' where
 *	windowed type widget is last on list ( 'pEnd' ).
 */

/**************************************************************************
  Undraw and destroy Window Group  The Trick is simple. We undraw only
  last member of group: the window.
**************************************************************************/
void popdown_window_group_dialog(struct widget *pBeginGroupWidgetList,
				 struct widget *pEndGroupWidgetList)
{
  if ((pBeginGroupWidgetList) && (pEndGroupWidgetList)) {
    widget_mark_dirty(pEndGroupWidgetList);
    remove_gui_layer(pEndGroupWidgetList->dst);

    del_group(pBeginGroupWidgetList, pEndGroupWidgetList);
  }
}

/**************************************************************************
  Sellect Window Group. (move widget group up the widgets list)
  Function return TRUE when group was sellected.
**************************************************************************/
bool sellect_window_group_dialog(struct widget *pBeginWidgetList,
							 struct widget *pWindow)
{
  if (!is_this_widget_first_on_list(pBeginWidgetList)) {
    move_group_to_front_of_gui_list(pBeginWidgetList, pWindow);
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
  Move Window Group.  The Trick is simple.  We move only last member of
  group: the window , and then set new position to all members of group.

  Function return 1 when group was moved.
**************************************************************************/
bool move_window_group_dialog(struct widget *pBeginGroupWidgetList,
			     struct widget *pEndGroupWidgetList)
{
  bool ret = FALSE;
  Sint16 oldX = pEndGroupWidgetList->size.x,
      oldY = pEndGroupWidgetList->size.y;

  if (move_window(pEndGroupWidgetList)) {
    set_new_group_start_pos(pBeginGroupWidgetList,
			    pEndGroupWidgetList->prev,
			    pEndGroupWidgetList->size.x - oldX,
			    pEndGroupWidgetList->size.y - oldY);
    ret = TRUE;
  }

  return ret;
}

/**************************************************************************
  Standart Window Group Widget Callback (window)
  When Pressed check mouse move;
  if move then move window and redraw else
  if not on fron then move window up to list and redraw;  
**************************************************************************/
void move_window_group(struct widget *pBeginWidgetList, struct widget *pWindow)
{
  if (sellect_window_group_dialog(pBeginWidgetList, pWindow)) {
    widget_flush(pWindow);
  }
  
  move_window_group_dialog(pBeginWidgetList, pWindow);
}

/**************************************************************************
  ...
**************************************************************************/
int setup_vertical_widgets_position(int step,
	Sint16 start_x, Sint16 start_y, Uint16 w, Uint16 h,
				struct widget *pBegin, struct widget *pEnd)
{
  struct widget *pBuf = pEnd;
  register int count = 0;
  register int real_start_x = start_x;
  int ret = 0;
  
  while(pBuf)
  {
    pBuf->size.x = real_start_x;
    pBuf->size.y = start_y;
     
    if(w) {
      pBuf->size.w = w;
    }
    
    if(h) {
      pBuf->size.h = h;
    }
    
    if(!((count + 1) % step)) {
      real_start_x = start_x;
      start_y += pBuf->size.h;
      if (((get_wflags(pBuf) & WF_HIDDEN) != WF_HIDDEN))
      {
        ret += pBuf->size.h;
      }
    } else {
      real_start_x += pBuf->size.w;
    }
    
    if(pBuf == pBegin) {
      
      break;
    }
    count++;
    pBuf = pBuf->prev;
  }
  
  return ret;
}

/* =================================================== */
/* ======================= WINDOWs ==================== */
/* =================================================== */

/**************************************************************************
  Window Menager Mechanism.
  Idea is simple each window/dialog has own buffer layer which is draw
  on screen during flush operations.
  This consume lots of memory but is extremly effecive.

  Each widget has own "destination" parm == where ( on what buffer )
  will be draw.
**************************************************************************/

/* =================================================== */
/* ======================== MISC ===================== */
/* =================================================== */

/**************************************************************************
  Draw Themed Frame.
**************************************************************************/
void draw_frame(SDL_Surface * pDest, Sint16 start_x, Sint16 start_y,
		Uint16 w, Uint16 h)
{
  SDL_Surface *pTmpLeft = 
    ResizeSurface(pTheme->FR_Left, pTheme->FR_Left->w, h, 1);
  SDL_Surface *pTmpRight = 
    ResizeSurface(pTheme->FR_Right, pTheme->FR_Right->w, h, 1);
  SDL_Surface *pTmpTop =
    ResizeSurface(pTheme->FR_Top, w, pTheme->FR_Top->h, 1);
  SDL_Surface *pTmpBottom =
    ResizeSurface(pTheme->FR_Bottom, w, pTheme->FR_Bottom->h, 1);
  
  SDL_Rect tmp,dst = {start_x, start_y, 0, 0};

  tmp = dst;
  alphablit(pTmpLeft, NULL, pDest, &tmp);
  
  dst.x += w - pTmpRight->w;
  tmp = dst;
  alphablit(pTmpRight, NULL, pDest, &tmp);

  dst.x = start_x;
  tmp = dst;
  alphablit(pTmpTop, NULL, pDest, &tmp);
  
  dst.y += h - pTmpBottom->h;
  tmp = dst;
  alphablit(pTmpBottom, NULL, pDest, &tmp);

  FREESURFACE(pTmpLeft);
  FREESURFACE(pTmpRight);
  FREESURFACE(pTmpTop);
  FREESURFACE(pTmpBottom);
}

/**************************************************************************
  ...
**************************************************************************/
void refresh_widget_background(struct widget *pWidget)
{
  if (pWidget) {
    if (pWidget->gfx && pWidget->gfx->w == pWidget->size.w &&
      				pWidget->gfx->h == pWidget->size.h) {
      clear_surface(pWidget->gfx, NULL);
      alphablit(pWidget->dst->surface, &pWidget->size, pWidget->gfx, NULL);
    } else {
      FREESURFACE(pWidget->gfx);
      pWidget->gfx = crop_rect_from_surface(pWidget->dst->surface, &pWidget->size);
    }
  }
}
