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
                          mapctrl.c  -  description
                             -------------------
    begin                : Thu Sep 05 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 **********************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SDL.h"

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "unit.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "overview_common.h"

/* gui-sdl */
#include "citydlg.h"
#include "cityrep.h"
#include "colors.h"
#include "dialogs.h"
#include "finddlg.h"
#include "graphics.h"
#include "gui_iconv.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_mouse.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "menu.h"
#include "messagewin.h"
#include "optiondlg.h"
#include "plrdlg.h"
#include "repodlgs.h"
#include "sprite.h"
#include "themespec.h"
#include "widget.h"
#include "wldlg.h"

#include "mapctrl.h"

#undef SCALE_MINIMAP
#undef SCALE_UNITINFO

extern int overview_start_x;
extern int overview_start_y;
extern bool is_unit_move_blocked;

static char *pSuggestedCityName = NULL;
static struct SMALL_DLG *pNewCity_Dlg = NULL;
  
#ifdef SCALE_MINIMAP
static struct SMALL_DLG *pScale_MiniMap_Dlg = NULL;
static int popdown_scale_minmap_dlg_callback(struct widget *pWidget);
#endif

#ifdef SCALE_UNITINFO
static struct SMALL_DLG *pScale_UnitInfo_Dlg = NULL;
static int INFO_WIDTH, INFO_HEIGHT = 0, INFO_WIDTH_MIN, INFO_HEIGHT_MIN;
static int popdown_scale_unitinfo_dlg_callback(struct widget *pWidget);
static void Remake_UnitInfo(int w, int h);
#endif

static struct ADVANCED_DLG *pMiniMap_Dlg = NULL;
static struct ADVANCED_DLG *pUnitInfo_Dlg = NULL;

int overview_w = 0;
int overview_h = 0;
int unitinfo_w = 0;
int unitinfo_h = 0;

bool draw_goto_patrol_lines = FALSE;
static struct widget *pNew_Turn_Button = NULL;
static struct widget *pUnits_Info_Window = NULL;
static struct widget *pMiniMap_Window = NULL;
static struct widget *pFind_City_Button = NULL;
static struct widget *pRevolution_Button = NULL;
static struct widget *pTax_Button = NULL;
static struct widget *pResearch_Button = NULL;

static void enable_minimap_widgets(void);
static void disable_minimap_widgets(void);
static void enable_unitinfo_widgets(void);
static void disable_unitinfo_widgets(void);

/* ================================================================ */

static int players_action_callback(struct widget *pWidget)
{
  set_wstate(pWidget, FC_WS_NORMAL);
  widget_redraw(pWidget);
  widget_mark_dirty(pWidget);
  if (Main.event.type == SDL_MOUSEBUTTONDOWN) {
    switch(Main.event.button.button) {
#if 0    
      case SDL_BUTTON_LEFT:
      
      break;
      case SDL_BUTTON_MIDDLE:
  
      break;
#endif    
      case SDL_BUTTON_RIGHT:
        popup_players_nations_dialog();
      break;
      default:
        popup_players_dialog(true);
      break;
    }
  } else {
    popup_players_dialog(true);
  }
  return -1;
}


static int units_action_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_wstate(pWidget, FC_WS_NORMAL);
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    popup_activeunits_report_dialog(FALSE);
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int cities_action_callback(struct widget *pButton)
{
  set_wstate(pButton, FC_WS_DISABLED);
  widget_redraw(pButton);
  widget_mark_dirty(pButton);
  if (Main.event.type == SDL_MOUSEBUTTONDOWN) {
    switch(Main.event.button.button) {
#if 0      
      case SDL_BUTTON_LEFT:
        
      break;
      case SDL_BUTTON_MIDDLE:
  
      break;
#endif      
      case SDL_BUTTON_RIGHT:
        popup_find_dialog();
      break;
      default:
        popup_city_report_dialog(FALSE);
      break;
    }
  } else {
    popup_find_dialog();
  }
    
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int end_turn_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pButton);
    widget_flush(pButton);
    disable_focus_animation();
    key_end_turn();
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int revolution_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_wstate(pButton, FC_WS_DISABLED);
    widget_redraw(pButton);
    widget_mark_dirty(pButton);
    popup_revolution_dialog();
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int research_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_science_dialog(TRUE);
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int economy_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_economy_report_dialog(FALSE);
  }
  return -1;
}

/* ====================================== */

/**************************************************************************
  Show/Hide Units Info Window
**************************************************************************/
static int toggle_unit_info_window_callback(struct widget *pIcon_Widget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct widget *pBuf = NULL;
  
    clear_surface(pIcon_Widget->theme, NULL);
    alphablit(pTheme->MAP_Icon, NULL, pIcon_Widget->theme, NULL);
  
    if (get_num_units_in_focus() > 0) {
      undraw_order_widgets();
    }
    
    if (SDL_Client_Flags & CF_UNITINFO_SHOWN) {
      /* HIDE */
      SDL_Surface *pBuf_Surf;
      SDL_Rect src, window_area;
  
      set_wstate(pIcon_Widget, FC_WS_NORMAL);
      pSellected_Widget = NULL;
    
      if (pUnits_Info_Window->private_data.adv_dlg->pEndActiveWidgetList) {
        del_group(pUnits_Info_Window->private_data.adv_dlg->pBeginActiveWidgetList,
                  pUnits_Info_Window->private_data.adv_dlg->pEndActiveWidgetList);
      }
      if (pUnits_Info_Window->private_data.adv_dlg->pScroll) {
        hide_scrollbar(pUnits_Info_Window->private_data.adv_dlg->pScroll);
      }
      
      /* clear area under old unit info window */
      widget_undraw(pUnits_Info_Window);
      widget_mark_dirty(pUnits_Info_Window);
  
      /* new button direction */
      alphablit(pTheme->L_ARROW_Icon, NULL, pIcon_Widget->theme, NULL);
  
      copy_chars_to_string16(pIcon_Widget->string16, _("Show Unit Info Window"));
          
      SDL_Client_Flags &= ~CF_UNITINFO_SHOWN;
  
      set_new_unitinfo_window_pos();
  
      /* blit part of map window */
      src.x = 0;
      src.y = 0;
      src.w = (pUnits_Info_Window->size.w - pUnits_Info_Window->area.w) + BLOCKU_W;
      src.h = pUnits_Info_Window->theme->h;
  
      window_area = pUnits_Info_Window->size;    
      alphablit(pUnits_Info_Window->theme, &src, pUnits_Info_Window->dst->surface, &window_area);
  
      /* blit right vertical frame */
      pBuf_Surf = ResizeSurface(pTheme->FR_Right, pTheme->FR_Right->w,
                                pUnits_Info_Window->area.h, 1);
  
      window_area.y = pUnits_Info_Window->area.y;
      window_area.x = pUnits_Info_Window->area.x + pUnits_Info_Window->area.w;
      alphablit(pBuf_Surf, NULL , pUnits_Info_Window->dst->surface, &window_area);
      FREESURFACE(pBuf_Surf);
  
      /* redraw widgets */
      
      /* ID_ECONOMY */
      pBuf = pUnits_Info_Window->prev;
      widget_redraw(pBuf);
  
      /* ===== */
      /* ID_RESEARCH */
      pBuf = pBuf->prev;
      widget_redraw(pBuf);
      
      /* ===== */
      /* ID_REVOLUTION */
      pBuf = pBuf->prev;
      widget_redraw(pBuf);
  
      /* ===== */
      /* ID_TOGGLE_UNITS_WINDOW_BUTTON */
      pBuf = pBuf->prev;
      widget_redraw(pBuf);

#ifdef SCALE_UNITINFO
      popdown_scale_unitinfo_dlg_callback(NULL);
#endif
    } else {
      if (Main.screen->w - pUnits_Info_Window->size.w >=
                  pMiniMap_Window->dst->dest_rect.x + pMiniMap_Window->size.w) {
  
        set_wstate(pIcon_Widget, FC_WS_NORMAL);
        pSellected_Widget = NULL;
                    
        /* SHOW */
        copy_chars_to_string16(pIcon_Widget->string16, _("Hide Unit Info Window"));
         
        alphablit(pTheme->R_ARROW_Icon, NULL, pIcon_Widget->theme, NULL);
  
        SDL_Client_Flags |= CF_UNITINFO_SHOWN;
  
        set_new_unitinfo_window_pos();
  
        widget_mark_dirty(pUnits_Info_Window);
      
        redraw_unit_info_label(get_units_in_focus());
      } else {
        alphablit(pTheme->L_ARROW_Icon, NULL, pIcon_Widget->theme, NULL);
        widget_redraw(pIcon_Widget);
        widget_mark_dirty(pIcon_Widget);
      }
    }
    
    if (get_num_units_in_focus() > 0) {
      update_order_widgets();
    }
  
    flush_dirty();
  }  
  return -1;
}

/**************************************************************************
  Show/Hide Mini Map
**************************************************************************/
static int toggle_map_window_callback(struct widget *pMap_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pFocus = head_of_units_in_focus();
    struct widget *pWidget;
      
    /* make new map icon */
    clear_surface(pMap_Button->theme, NULL);
    alphablit(pTheme->MAP_Icon, NULL, pMap_Button->theme, NULL);
  
    set_wstate(pMiniMap_Window, FC_WS_NORMAL);
  
    if (pFocus) {
      undraw_order_widgets();
    }
    
    if (SDL_Client_Flags & CF_OVERVIEW_SHOWN) {
      /* Hide MiniMap */
      SDL_Surface *pBuf_Surf;
      SDL_Rect src, map_area = pMiniMap_Window->size;
  
      set_wstate(pMap_Button, FC_WS_NORMAL);
      pSellected_Widget = NULL;
      
      copy_chars_to_string16(pMap_Button->string16, _("Show Mini Map"));
          
      /* make new map icon */
      alphablit(pTheme->R_ARROW_Icon, NULL, pMap_Button->theme, NULL);
  
      SDL_Client_Flags &= ~CF_OVERVIEW_SHOWN;
      
      /* clear area under old map window */
      widget_undraw(pMiniMap_Window);
      widget_mark_dirty(pMiniMap_Window);
  
      pMiniMap_Window->size.w = (pMiniMap_Window->size.w - pMiniMap_Window->area.w) + BLOCKM_W;
      pMiniMap_Window->area.w = BLOCKM_W;
      
      set_new_minimap_window_pos();
      
      /* blit part of map window */
      src.x = pMiniMap_Window->theme->w - BLOCKM_W - pTheme->FR_Right->w;
      src.y = 0;
      src.w = BLOCKM_W + pTheme->FR_Right->w;
      src.h = pMiniMap_Window->theme->h;
        
      alphablit(pMiniMap_Window->theme, &src , pMiniMap_Window->dst->surface, &map_area);
    
      /* blit left vertical frame theme */
      pBuf_Surf = ResizeSurface(pTheme->FR_Left, pTheme->FR_Left->w,
                                pMiniMap_Window->area.h, 1);
  
      map_area.y += adj_size(2);
      alphablit(pBuf_Surf, NULL, pMiniMap_Window->dst->surface, &map_area);
      FREESURFACE(pBuf_Surf);
      
      /* redraw widgets */  
      /* ID_NEW_TURN */
      pWidget = pMiniMap_Window->prev;
      widget_redraw(pWidget);
  
      /* ID_PLAYERS */
      pWidget = pWidget->prev;
      widget_redraw(pWidget);
  
      /* ID_CITIES */
      pWidget = pWidget->prev;
      widget_redraw(pWidget);
  
      /* ID_UNITS */
      pWidget = pWidget->prev;
      widget_redraw(pWidget);
      
      /* ID_CHATLINE_TOGGLE_LOG_WINDOW_BUTTON */
      pWidget = pWidget->prev;
      widget_redraw(pWidget);
  
      /* Toggle Minimap mode */
      pWidget = pWidget->prev;
      widget_redraw(pWidget);
      
      #ifdef SMALL_SCREEN
      /* options */
      pWidget = pWidget->prev;
      widget_redraw(pWidget);
      #endif
      
      /* ID_TOGGLE_MAP_WINDOW_BUTTON */
      pWidget = pWidget->prev;
      widget_redraw(pWidget);
  
#ifdef SCALE_MINIMAP
      popdown_scale_minmap_dlg_callback(NULL);
#endif
    } else {
      if (((pMiniMap_Window->size.w - pMiniMap_Window->area.w) +
            overview_w + BLOCKM_W) <= pUnits_Info_Window->dst->dest_rect.x) {
        
        set_wstate(pMap_Button, FC_WS_NORMAL);
        pSellected_Widget = NULL;
        
        /* show MiniMap */
        copy_chars_to_string16(pMap_Button->string16, _("Hide Mini Map"));
          
        alphablit(pTheme->L_ARROW_Icon, NULL, pMap_Button->theme, NULL);
        SDL_Client_Flags |= CF_OVERVIEW_SHOWN;
              
        pMiniMap_Window->size.w =
          (pMiniMap_Window->size.w - pMiniMap_Window->area.w) + overview_w + BLOCKM_W;
        pMiniMap_Window->area.w = overview_w + BLOCKM_W;
              
        set_new_minimap_window_pos();
      
        widget_redraw(pMiniMap_Window);
        redraw_minimap_window_buttons();
        refresh_overview();
      } else {
        alphablit(pTheme->R_ARROW_Icon, NULL, pMap_Button->theme, NULL);
        widget_redraw(pMap_Button);
        widget_mark_dirty(pMap_Button);
      }
    }
  
    if (pFocus) {
      update_order_widgets();
    }
  
    flush_dirty();
  }
  return -1;
}

/* ====================================================================== */


static int toggle_minimap_mode_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pWidget) {
      pSellected_Widget = pWidget;
      set_wstate(pWidget, FC_WS_SELLECTED);
    }
    toggle_overview_mode();
    refresh_overview();
    flush_dirty();
  }
  return -1;
}

static int toggle_msg_window_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) { 
    if (is_meswin_open()) {
      popdown_meswin_dialog();
      copy_chars_to_string16(pWidget->string16, _("Show Messages (F9)"));
    } else {
      popup_meswin_dialog(true);
      copy_chars_to_string16(pWidget->string16, _("Hide Messages (F9)"));
    }
  
    pSellected_Widget = pWidget;
    set_wstate(pWidget, FC_WS_SELLECTED);
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    
    flush_dirty();
  }
  return -1;
}

int resize_minimap(void)
{

  overview_w = overview.width;
  overview_h = overview.height;
  overview_start_x = (overview_w - overview.width)/2;
  overview_start_y = (overview_h - overview.height)/2;

  if (C_S_RUNNING == client_state()) {
    popdown_minimap_window();
    popup_minimap_window();
    refresh_overview();
    update_menus();
  }
  
  return 0;
}

#ifdef SCALE_MINIMAP
/* ============================================================== */
static int move_scale_minmap_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pScale_MiniMap_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int popdown_scale_minmap_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pScale_MiniMap_Dlg) {
      popdown_window_group_dialog(pScale_MiniMap_Dlg->pBeginWidgetList,
                                  pScale_MiniMap_Dlg->pEndWidgetList);
      FC_FREE(pScale_MiniMap_Dlg);
      if (pWidget) {
        flush_dirty();
      }
    }
  }
  return -1;
}

static int up_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if ((((OVERVIEW_TILE_WIDTH + 1) * map.xsize) +
         (pMiniMap_Window->size.w - pMiniMap_Window->area.w) + BLOCKM_W) <= 
         pUnits_Info_Window->dst->dest_rect.x) {
      char cBuf[4];
      my_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_WIDTH);
      copy_chars_to_string16(pWidget->next->string16, cBuf);
      widget_redraw(pWidget->next);
      widget_mark_dirty(pWidget->next);
      
      calculate_overview_dimensions();
      resize_minimap();
    }
    flush_dirty();
  }
  return -1;
}

static int down_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (OVERVIEW_TILE_WIDTH > 1) {
      char cBuf[4];
      my_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_WIDTH);
      copy_chars_to_string16(pWidget->prev->string16, cBuf);
      widget_redraw(pWidget->prev);
      widget_mark_dirty(pWidget->prev);
      
      resize_minimap();
    }
    flush_dirty();
  }
  return -1;
}

static int up_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (Main.screen->h -
      ((OVERVIEW_TILE_HEIGHT + 1) * map.ysize + (pTheme->FR_Bottom->h * 2)) >= 40) {
      char cBuf[4];
        
      OVERVIEW_TILE_HEIGHT++;
      my_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_HEIGHT);
      copy_chars_to_string16(pWidget->next->string16, cBuf);
      widget_redraw(pWidget->next);
      widget_mark_dirty(pWidget->next);
      resize_minimap();
    }
    flush_dirty();
  }
  return -1;
}

static int down_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (OVERVIEW_TILE_HEIGHT > 1) {
      char cBuf[4];
      
      OVERVIEW_TILE_HEIGHT--;
      my_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_HEIGHT);
      copy_chars_to_string16(pWidget->prev->string16, cBuf);
      widget_redraw(pWidget->prev);
      widget_mark_dirty(pWidget->prev);
      
      resize_minimap();
    }
    flush_dirty();
  }
  return -1;
}

static void popup_minimap_scale_dialog(void)
{
  SDL_Surface *pText1, *pText2;
  SDL_String16 *pStr = NULL;
  struct widget *pWindow = NULL;
  struct widget *pBuf = NULL;
  char cBuf[4];
  int window_x = 0, window_y = 0;
  SDL_Rect area;
  
  if (pScale_MiniMap_Dlg || !(SDL_Client_Flags & CF_OVERVIEW_SHOWN)) {
    return;
  }
  
  pScale_MiniMap_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));
    
  /* create window */
  pStr = create_str16_from_char(_("Scale Mini Map"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  pWindow = create_window_skeleton(NULL, pStr, 0);
  pWindow->action = move_scale_minmap_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  add_to_gui_list(ID_WINDOW, pWindow);
  pScale_MiniMap_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ----------------- */  
  pStr = create_str16_from_char(_("Single Tile Width"), adj_font(12));
  pText1 = create_text_surf_from_str16(pStr);
  area.w = MAX(area.w, pText1->w + adj_size(30));
    
  copy_chars_to_string16(pStr, _("Single Tile Height"));
  pText2 = create_text_surf_from_str16(pStr);
  area.w = MAX(area.w, pText2->w + adj_size(30));
  FREESTRING16(pStr);
  
  pBuf = create_themeicon_button(pTheme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  
  my_snprintf(cBuf, sizeof(cBuf), "%d" , OVERVIEW_TILE_WIDTH);
  pStr = create_str16_from_char(cBuf, adj_font(24));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
  pBuf->size.w = MAX(adj_size(50), pBuf->size.w);
  area.h += pBuf->size.h + adj_size(5);
  add_to_gui_list(ID_LABEL, pBuf);
  
  pBuf = create_themeicon_button(pTheme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  
  
  /* ------------ */
  pBuf = create_themeicon_button(pTheme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  
  my_snprintf(cBuf, sizeof(cBuf), "%d" , OVERVIEW_TILE_HEIGHT);
  pStr = create_str16_from_char(cBuf, adj_font(24));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
  pBuf->size.w = MAX(adj_size(50), pBuf->size.w);
  area.h += pBuf->size.h + adj_size(20);
  add_to_gui_list(ID_LABEL, pBuf);
  
  pBuf = create_themeicon_button(pTheme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.w = MAX(area.w , pBuf->size.w * 2 + pBuf->next->size.w + adj_size(20));
  
  /* ------------ */
  pStr = create_str16_from_char(_("Exit"), adj_font(12));
  pBuf = create_themeicon_button(pTheme->CANCEL_Icon, pWindow->dst, pStr, 0);
  pBuf->action = popdown_scale_minmap_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pScale_MiniMap_Dlg->pBeginWidgetList = pBuf;
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h + adj_size(10);
  area.w = MAX(area.w, pBuf->size.w + adj_size(20));
  /* ------------ */
  
  area.h += adj_size(20); 

  resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  if (Main.event.motion.x + pWindow->size.w > Main.screen->w) {
    if (Main.event.motion.x - pWindow->size.w >= 0) {
      window_x = Main.event.motion.x - pWindow->size.w;
    } else {
      window_x = (Main.screen->w -pWindow->size. w) / 2;
    }
  } else {
    window_x = Main.event.motion.x;
  }
    
  if (Main.event.motion.y + pWindow->size.h >= Main.screen->h) {
    if (Main.event.motion.y - pWindow->size.h >= 0) {
      window_y = Main.event.motion.y - pWindow->size.h;
    } else {
      window_y = (Main.screen->h - pWindow->size.h) / 2;
    }
  } else {
    window_y = Main.event.motion.y;
  }

  widget_set_position(pWindow, window_x, window_y);

  blit_entire_src(pText1, pWindow->theme, 15, area.y + 1);
  FREESURFACE(pText1);

  /* width label */
  pBuf = pWindow->prev->prev;
  pBuf->size.y = area.y + adj_size(16);
  pBuf->size.x = area.x + (area.w - pBuf->size.w) / 2;
  
  /* width left button */
  pBuf->next->size.y = pBuf->size.y + pBuf->size.h - pBuf->next->size.h;
  pBuf->next->size.x = pBuf->size.x - pBuf->next->size.w;
  
  /* width right button */
  pBuf->prev->size.y = pBuf->size.y + pBuf->size.h - pBuf->prev->size.h;
  pBuf->prev->size.x = pBuf->size.x + pBuf->size.w;
  
  /* height label */
  pBuf = pBuf->prev->prev->prev;
  pBuf->size.y = pBuf->next->next->next->size.y + pBuf->next->next->next->size.h + adj_size(20);
  pBuf->size.x = area.x + (area.w - pBuf->size.w) / 2;
  
  blit_entire_src(pText2, pWindow->theme, adj_size(15), pBuf->size.y - pText2->h - adj_size(2));
  FREESURFACE(pText2);
    
  /* height left button */
  pBuf->next->size.y = pBuf->size.y + pBuf->size.h - pBuf->next->size.h;
  pBuf->next->size.x = pBuf->size.x - pBuf->next->size.w;
  
  /* height right button */
  pBuf->prev->size.y = pBuf->size.y + pBuf->size.h - pBuf->prev->size.h;
  pBuf->prev->size.x = pBuf->size.x + pBuf->size.w;
  
  /* exit button */
  pBuf = pBuf->prev->prev;
  pBuf->size.x = area.x + (area.w - pBuf->size.w) / 2;
  pBuf->size.y = area.y + area.h - pBuf->size.h - adj_size(7);
  
  /* -------------------- */
  redraw_group(pScale_MiniMap_Dlg->pBeginWidgetList, pWindow, 0);
  widget_flush(pWindow);
  
}
#endif

/* ==================================================================== */
#ifdef SCALE_UNITINFO
static int move_scale_unitinfo_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pScale_UnitInfo_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int popdown_scale_unitinfo_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if(pScale_UnitInfo_Dlg) {
      popdown_window_group_dialog(pScale_UnitInfo_Dlg->pBeginWidgetList,
                                  pScale_UnitInfo_Dlg->pEndWidgetList);
      FC_FREE(pScale_UnitInfo_Dlg);
      if(pWidget) {
        flush_dirty();
      }
    }
  }
  return -1;
}

static void Remake_UnitInfo(int w, int h)
{
  SDL_Color bg_color = {255, 255, 255, 128};
  
  SDL_Surface *pSurf;
  SDL_Rect area = {pUnits_Info_Window->area.x + BLOCKU_W,
                   pUnits_Info_Window->area.y, 0, 0};

  struct widget *pWidget = pUnits_Info_Window;

  if(w < DEFAULT_UNITS_W - BLOCKU_W) {
    w = (pUnits_Info_Window->size.w - pUnits_Info_Window->area.w) + DEFAULT_UNITS_W;
  } else {
    w += (pUnits_Info_Window->size.w - pUnits_Info_Window->area.w) + BLOCKU_W;
  }
  
  if(h < DEFAULT_UNITS_H) {
    h = (pUnits_Info_Window->size.h - pUnits_Info_Window->area.h) + DEFAULT_UNITS_H;
  } else {
    h += (pUnits_Info_Window->size.h - pUnits_Info_Window->area.h);
  }
  
  /* clear area under old map window */
  clear_surface(pWidget->dst->surface, &pWidget->size);
  widget_mark_dirty(pWidget);
    
  pWidget->size.w = w;
  pWidget->size.h = h;
  
  pWidget->size.x = Main.screen->w - w;
  pWidget->size.y = Main.screen->h - h;
  
  FREESURFACE(pWidget->theme);
  pWidget->theme = create_surf_alpha(w, h, SDL_SWSURFACE);
     
  draw_frame(pWidget->theme, 0, 0, pWidget->size.w, pWidget->size.h);
  
  pSurf = ResizeSurface(pTheme->Block, BLOCKU_W,
    pWidget->size.h - ((pUnits_Info_Window->size.h - pUnits_Info_Window->area.h)), 1);
  
  blit_entire_src(pSurf, pWidget->theme, pUnits_Info_Window->area.x,
                                         pUnits_Info_Window->area.y);
  FREESURFACE(pSurf);
  
  area.w = w - BLOCKU_W - (pUnits_Info_Window->size.w - pUnits_Info_Window->area.w);
  area.h = h - (pUnits_Info_Window->size.h - pUnits_Info_Window->area.h);
  SDL_FillRect(pWidget->theme, &area, map_rgba(pWidget->theme->format, bg_color));
  
  /* economy button */
  pWidget = pTax_Button;
  FREESURFACE(pWidget->gfx);
  pWidget->size.x = pWidget->dst->surface->w - w + pUnits_Info_Window->area.x
                                             + (BLOCKU_W - pWidget->size.w)/2;  
  pWidget->size.y = pWidget->dst->surface->h - h + pWidget->area.y + 2;
  
  /* research button */
  pWidget = pWidget->prev;
  FREESURFACE(pWidget->gfx);
  pWidget->size.x = pWidget->dst->surface->w - w + pUnits_Info_Window->area.x
                                             + (BLOCKU_W - pWidget->size.w)/2;  
  pWidget->size.y = pWidget->dst->surface->h - h + pUnits_Info_Window->area.y +
                    pWidget->size.h + 2;
  
  /* revolution button */
  pWidget = pWidget->prev;
  FREESURFACE(pWidget->gfx);
  pWidget->size.x = pWidget->dst->surface->w - w + pUnits_Info_Window->area.x
                                             + (BLOCKU_W - pWidget->size.w)/2;
  pWidget->size.y = pWidget->dst->surface->h - h + pUnits_Info_Window->area.y +
                    pWidget->size.h * 2 + 2;
  
  /* show/hide unit's window button */
  pWidget = pWidget->prev;
  FREESURFACE(pWidget->gfx);
  pWidget->size.x = pWidget->dst->surface->w - w + pUnits_Info_Window->area.x
                                             + (BLOCKU_W - pWidget->size.w)/2;  
  pWidget->size.y = pUnits_Info_Window->area.y + pUnits_Info_Window->area.h -
                    pWidget->size.h - 2;
  
  unitinfo_w = w;
  unitinfo_h = h;
  
}

int resize_unit_info(void)
{
  struct widget *pInfo_Window = get_unit_info_window_widget();
  
  int w = INFO_WIDTH * map.xsize;
  int h = INFO_HEIGHT * map.ysize;
  int current_w = pUnits_Info_Window->size.w - BLOCKU_W - 
                  (pInfo_Window->size.w - pInfo_Window->area.w);
  int current_h = pUnits_Info_Window->size.h -
                  (pInfo_Window->size.h - pInfo_Window->area.h);
  
  if ((((current_w > DEFAULT_UNITS_W - BLOCKU_W)
   || (w > DEFAULT_UNITS_W - BLOCKU_W)) && (current_w != w)) ||
    (((current_h > DEFAULT_UNITS_H) || (h > DEFAULT_UNITS_H)) && (current_h != h))) {
    Remake_UnitInfo(w, h);
  }
  
  if (C_S_RUNNING == client_state()) {
    update_menus();
  }
  update_unit_info_label(get_units_in_focus());
      
  return 0;
}

static int up_info_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (Main.screen->w - ((INFO_WIDTH + 1) * map.xsize + BLOCKU_W +
         (pMiniMap_Window->size.w - pMiniMap_Window->area.w)) >=
         pMiniMap_Window->size.x + pMiniMap_Window->size.w) {
      INFO_WIDTH++;
      resize_unit_info();
    }
    flush_dirty();
  }
  return -1;
}

static int down_info_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if(INFO_WIDTH > INFO_WIDTH_MIN) {
      INFO_WIDTH--;
      resize_unit_info();
    }
    flush_dirty();
  }
  return -1;
}

static int up_info_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if(Main.screen->h - ((INFO_HEIGHT + 1) * map.ysize +
        (pUnits_Info_Window->size.h - pUnits_Info_Window->area.h)) >= adj_size(40)) {
      INFO_HEIGHT++;
      resize_unit_info();
    }
    flush_dirty();
  }
  return -1;
}

static int down_info_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if(INFO_HEIGHT > INFO_HEIGHT_MIN) {
      INFO_HEIGHT--;    
      resize_unit_info();
    }
    flush_dirty();
  }
  return -1;
}

static void popup_unitinfo_scale_dialog(void)
{

#ifndef SCALE_UNITINFO
  return;
#endif

  SDL_Surface *pText1, *pText2;
  SDL_String16 *pStr = NULL;
  struct widget *pWindow = NULL;
  struct widget *pBuf = NULL;
  int window_x = 0, window_y = 0;
  SDL_Rect area;
  
  if(pScale_UnitInfo_Dlg || !(SDL_Client_Flags & CF_UNITINFO_SHOWN)) {
    return;
  }
  
  pScale_UnitInfo_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));
    
  /* create window */
  pStr = create_str16_from_char(_("Scale Unit Info"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  pWindow = create_window_skeleton(NULL, pStr, 0);
  pWindow->action = move_scale_unitinfo_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  add_to_gui_list(ID_WINDOW, pWindow);
  pScale_UnitInfo_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;
  
  pStr = create_str16_from_char(_("Width"), adj_font(12));
  pText1 = create_text_surf_from_str16(pStr);
  area.w = MAX(area.w, pText1->w + adj_size(30));
  area.h += MAX(adj_size(20), pText1->h + adj_size(4));
  copy_chars_to_string16(pStr, _("Height"));
  pText2 = create_text_surf_from_str16(pStr);
  area.w = MAX(area.w, pText2->w + adj_size(30));
  area.h += MAX(adj_size(20), pText2->h + adj_size(4));
  FREESTRING16(pStr);
  
  /* ----------------- */
  pBuf = create_themeicon_button(pTheme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_info_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h;  
  
  pBuf = create_themeicon_button(pTheme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_info_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
    
  /* ------------ */
  pBuf = create_themeicon_button(pTheme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_info_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h + adj_size(10);
  
  pBuf = create_themeicon_button(pTheme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_info_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.w = MAX(area.w , pBuf->size.w * 2 + adj_size(20));
    
  /* ------------ */
  pStr = create_str16_from_char(_("Exit"), adj_font(12));
  pBuf = create_themeicon_button(pTheme->CANCEL_Icon,
						  pWindow->dst, pStr, 0);
  pBuf->action = popdown_scale_unitinfo_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pScale_UnitInfo_Dlg->pBeginWidgetList = pBuf;
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h + adj_size(10);
  area.w = MAX(area.w, pBuf->size.w + adj_size(20));
  
  resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  /* ------------ */
  
  if (Main.event.motion.x + pWindow->size.w > Main.screen->w) {
    if (Main.event.motion.x - pWindow->size.w >= 0) {
      window_x = Main.event.motion.x - pWindow->size.w;
    } else {
      window_x = (Main.screen->w - pWindow->size.w) / 2;
    }
  } else {
    window_x = Main.event.motion.x;
  }
    
  if (Main.event.motion.y + pWindow->size.h >= Main.screen->h) {
    if (Main.event.motion.y - pWindow->size.h >= 0) {
      window_y = Main.event.motion.y - pWindow->size.h;
    } else {
      window_y = (pWindow->dst->surface->h - pWindow->size.h) / 2;
    }
  } else {
    window_y = Main.event.motion.y;
  }
  
  widget_set_position(pWindow, window_x, window_y);
  
  /* width left button */
  pBuf = pWindow->prev;
  pBuf->size.y = area.y + MAX(adj_size(20), pText1->h + adj_size(4));
  pBuf->size.x = area.x + (area.w - pBuf->size.w * 2) / 2;
  blit_entire_src(pText1, pWindow->theme, adj_size(15), pBuf->size.y
					  - area.y - pText1->h - adj_size(2));
  FREESURFACE(pText1);
  
  /* width right button */
  pBuf->prev->size.y = pBuf->size.y;
  pBuf->prev->size.x = pBuf->size.x + pBuf->size.w;
  
  /* height left button */
  pBuf = pBuf->prev->prev;
  pBuf->size.y = pBuf->next->next->size.y +
  			pBuf->next->next->size.h + MAX(adj_size(20), pText2->h + adj_size(4));
  pBuf->size.x = area.x + (area.w - pBuf->size.w * 2) / 2;
  
  blit_entire_src(pText2, pWindow->theme, adj_size(15), pBuf->size.y - area.y - pText2->h - adj_size(2));
  FREESURFACE(pText2);
    
  /* height right button */
  pBuf->prev->size.y = pBuf->size.y;
  pBuf->prev->size.x = pBuf->size.x + pBuf->size.w;
  
  /* exit button */
  pBuf = pBuf->prev->prev;
  pBuf->size.x = area.x + (area.w - pBuf->size.w) / 2;
  pBuf->size.y = area.y + area.h - pBuf->size.h - adj_size(7);
    
  if (!INFO_HEIGHT) {
    INFO_WIDTH_MIN = (DEFAULT_UNITS_W - BLOCKU_W) / map.xsize;
    if (INFO_WIDTH_MIN == 1) {
      INFO_WIDTH_MIN = 0;
    }
    INFO_HEIGHT_MIN = DEFAULT_UNITS_H / map.ysize;
    if (!INFO_HEIGHT_MIN) {
      INFO_HEIGHT_MIN = 1;
    }  
    INFO_WIDTH = INFO_WIDTH_MIN;
    INFO_HEIGHT = INFO_HEIGHT_MIN;
  }
  
  /* -------------------- */
  redraw_group(pScale_UnitInfo_Dlg->pBeginWidgetList, pWindow, 0);
  widget_flush(pWindow);
  
}
#endif

/* ==================================================================== */
static int minimap_window_callback(struct widget *pWidget)
{
  int mouse_x, mouse_y;
  
  switch(Main.event.button.button) {
    case SDL_BUTTON_RIGHT:
      mouse_x = Main.event.motion.x - pMiniMap_Window->dst->dest_rect.x - 
                pMiniMap_Window->area.x - overview_start_x;
      mouse_y = Main.event.motion.y - pMiniMap_Window->dst->dest_rect.y - 
                pMiniMap_Window->area.y - overview_start_y;
      if ((SDL_Client_Flags & CF_OVERVIEW_SHOWN) &&  
          (mouse_x >= 0) && (mouse_x < overview_w) &&
          (mouse_y >= 0) && (mouse_y < overview_h)) {
                              
        int map_x, map_y;
                            
        overview_to_map_pos(&map_x, &map_y, mouse_x, mouse_y);
        center_tile_mapcanvas(map_pos_to_tile(map_x, map_y));
      }

      break;
    case SDL_BUTTON_MIDDLE:
    /* FIXME: scaling needs to be fixed */
#ifdef SCALE_MINIMAP
      popup_minimap_scale_dialog();
#endif    
      break;
    default:
      break;
  }
  return -1;
}

static int unit_info_window_callback(struct widget *pWidget)
{
  switch(Main.event.button.button) {
#if 0    
    case SDL_BUTTON_LEFT:
      
    break;
#endif
    case SDL_BUTTON_MIDDLE:
      request_center_focus_unit();
    break;
    case SDL_BUTTON_RIGHT:
#ifdef SCALE_UNITINFO
      popup_unitinfo_scale_dialog();
#endif
    break;
    default:
      key_unit_wait();
    break;
  }
  
  return -1;
}

/* ============================== Public =============================== */

/**************************************************************************
  This Function is used when resize Main.screen.
  We must set new Units Info Win. start position.
**************************************************************************/
void set_new_unitinfo_window_pos(void)
{
  struct widget *pUnit_Window = pUnits_Info_Window;
  struct widget *pWidget;
  SDL_Rect area;

  if (SDL_Client_Flags & CF_UNITINFO_SHOWN) {
    widget_set_position(pUnits_Info_Window,
                        Main.screen->w - pUnits_Info_Window->size.w, 
                        Main.screen->h - pUnits_Info_Window->size.h);
  } else {
    widget_set_position(pUnit_Window,
                        Main.screen->w - BLOCKU_W - pTheme->FR_Right->w, 
                        Main.screen->h - pUnits_Info_Window->size.h);
  }

  area.x = pUnits_Info_Window->area.x;
  area.y = pUnits_Info_Window->area.y;
  area.w = BLOCKU_W;
  area.h = DEFAULT_UNITS_H;
  
  /* ID_ECONOMY */
  pWidget = pTax_Button;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w)/2,
                      area.y + 2);

  /* ID_RESEARCH */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w)/2,
                      area.y + 2 + pWidget->size.h);

  /* ID_REVOLUTION */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w)/2,
                      area.y + 2 + (pWidget->size.h * 2));
  
  /* ID_TOGGLE_UNITS_WINDOW_BUTTON */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w)/2,
                      area.y + area.h - pWidget->size.h - 2);
}

/**************************************************************************
  This Function is used when resize Main.screen.
  We must set new MiniMap start position.
**************************************************************************/
void set_new_minimap_window_pos(void)
{
  struct widget *pWidget;
  SDL_Rect area;

  widget_set_position(pMiniMap_Window,
                      0, 
                      Main.screen->h - pMiniMap_Window->size.h);

  area.x = pMiniMap_Window->size.w - pTheme->FR_Right->w - BLOCKM_W;
  area.y = pMiniMap_Window->area.y;
  area.w = BLOCKM_W;
  area.h = pMiniMap_Window->area.h;

  
  /* ID_NEW_TURN */
  pWidget = pMiniMap_Window->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2) + pWidget->size.w,
                      area.y + 2);
  
  /* PLAYERS BUTTON */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2) + pWidget->size.w,
                      area.y + pWidget->size.h + 2);

  /* ID_FIND_CITY */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2) + pWidget->size.w,
                      area.y + pWidget->size.h * 2 + 2);

  /* UNITS BUTTON */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2),
                      area.y + 2);
  
  /* ID_CHATLINE_TOGGLE_LOG_WINDOW_BUTTON */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2),
                      area.y + pWidget->size.h + 2);
 
  /* Toggle minimap mode */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2),
                      area.y + pWidget->size.h * 2 + 2);
						
  #ifdef SMALL_SCREEN
  /* ID_TOGGLE_MAP_WINDOW_BUTTON */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2),
                      area.y + area.h - pWidget->size.h - 2);
  #endif
						
  /* ID_TOGGLE_MAP_WINDOW_BUTTON */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + adj_size(2) + pWidget->size.w,
                      area.y + area.h - pWidget->size.h - 2);
}

void popup_unitinfo_window() {
  struct widget *pWidget, *pWindow;
  SDL_Surface *pIcon_theme = NULL;
  char buf[256];
  
  if (pUnitInfo_Dlg) {
    return;
  }    
  
  pUnitInfo_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  /* pUnits_Info_Window */
  pWindow = create_window_skeleton(NULL, NULL, 0);

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + DEFAULT_UNITS_W,
                (pWindow->size.h - pWindow->area.h) + DEFAULT_UNITS_H);

  draw_frame(pWindow->theme, 0, 0, pWindow->size.w, pWindow->size.h);
  
  unitinfo_w = pWindow->size.w;
  unitinfo_h = pWindow->size.h;
  
  pIcon_theme = ResizeSurface(pTheme->Block, pWindow->area.w, pWindow->area.h, 1);
  
  blit_entire_src(pIcon_theme, pWindow->theme, pWindow->area.x, pWindow->area.y);
  FREESURFACE(pIcon_theme);
 
  pWindow->action = unit_info_window_callback;

  add_to_gui_list(ID_UNITS_WINDOW, pWindow);

  pUnits_Info_Window = pWindow;
  
  pUnitInfo_Dlg->pEndWidgetList = pUnits_Info_Window;
  pUnits_Info_Window->private_data.adv_dlg = pUnitInfo_Dlg;

  /* economy button */
  pWidget = create_icon2(get_tax_surface(O_GOLD), pUnits_Info_Window->dst, WF_FREE_GFX
                      | WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND | WF_FREE_THEME);

  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Economy"), "F5");
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = economy_callback;
  pWidget->key = SDLK_F5;

  add_to_gui_list(ID_ECONOMY, pWidget);
  
  pTax_Button = pWidget; 

  /* research button */
  pWidget = create_icon2(adj_surf(GET_SURF(client_research_sprite())), pUnits_Info_Window->dst, WF_FREE_GFX
		       | WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND | WF_FREE_THEME);
  /* TRANS: Research report action */
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Research"), "F6");                       
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = research_callback;
  pWidget->key = SDLK_F6;

  add_to_gui_list(ID_RESEARCH, pWidget);

  pResearch_Button = pWidget;

  /* revolution button */
  pWidget = create_icon2(adj_surf(GET_SURF(client_government_sprite())), pUnits_Info_Window->dst, (WF_FREE_GFX
			| WF_WIDGET_HAS_INFO_LABEL| WF_RESTORE_BACKGROUND | WF_FREE_THEME));
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Revolution"), "Ctrl+Shift+R");
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = revolution_callback;
  pWidget->key = SDLK_r;
  pWidget->mod = KMOD_CTRL | KMOD_SHIFT;

  add_to_gui_list(ID_REVOLUTION, pWidget);

  pRevolution_Button = pWidget;
  
  /* show/hide unit's window button */

  /* make UNITS Icon */
  pIcon_theme = create_surf_alpha(pTheme->MAP_Icon->w,
			    pTheme->MAP_Icon->h, SDL_SWSURFACE);
  alphablit(pTheme->MAP_Icon, NULL, pIcon_theme, NULL);
  alphablit(pTheme->R_ARROW_Icon, NULL, pIcon_theme, NULL);

  pWidget = create_themeicon(pIcon_theme, pUnits_Info_Window->dst,
			  WF_FREE_GFX | WF_FREE_THEME |
		WF_RESTORE_BACKGROUND | WF_WIDGET_HAS_INFO_LABEL);

  pWidget->string16 = create_str16_from_char(_("Hide Unit Info Window"), adj_font(12));
  
  pWidget->action = toggle_unit_info_window_callback;

  add_to_gui_list(ID_TOGGLE_UNITS_WINDOW_BUTTON, pWidget);
  
  pUnitInfo_Dlg->pBeginWidgetList = pWidget;

  SDL_Client_Flags |= CF_UNITINFO_SHOWN;

  set_new_unitinfo_window_pos();

  widget_redraw(pUnits_Info_Window);
}

void show_unitinfo_window_buttons()
{
  struct widget *pWidget = get_unit_info_window_widget();
    
  /* economy button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);      
  
  /* research button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
        
  /* revolution button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  
  /* show/hide unit's window button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
}

void hide_unitinfo_window_buttons()
{
  struct widget *pWidget = get_unit_info_window_widget();
    
  /* economy button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  
  /* research button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
        
  /* revolution button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  
  /* show/hide unit's window button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
}
  
void disable_unitinfo_window_buttons()
{
  struct widget *pWidget = get_unit_info_window_widget();
    
  /* economy button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
  
  /* research button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
        
  /* revolution button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
}

void popdown_unitinfo_window()
{
  if (pUnitInfo_Dlg) {
    popdown_window_group_dialog(pUnitInfo_Dlg->pBeginWidgetList, pUnitInfo_Dlg->pEndWidgetList);
    FC_FREE(pUnitInfo_Dlg);
    SDL_Client_Flags &= ~CF_UNITINFO_SHOWN;
  }
}

void popup_minimap_window() {
  struct widget *pWidget, *pWindow;
  SDL_Surface *pIcon_theme = NULL;
  SDL_Color black = {0, 0, 0, 255};
  char buf[256];

  if (pMiniMap_Dlg) {
    return;
  }
  
  pMiniMap_Dlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  
  /* pMiniMap_Window */
  pWindow = create_window_skeleton(NULL, NULL, 0);

  resize_window(pWindow, NULL, &black,
                (pWindow->size.w - pWindow->area.w) + overview_w + BLOCKM_W,
                (pWindow->size.h - pWindow->area.h) + overview_h);
  
  draw_frame(pWindow->theme, 0, 0, pWindow->size.w, pWindow->size.h);
  
  pIcon_theme = ResizeSurface(pTheme->Block, BLOCKM_W, pWindow->area.h, 1);
  blit_entire_src(pIcon_theme, pWindow->theme,
    pWindow->area.x + pWindow->area.w - pIcon_theme->w, pWindow->area.y);
  FREESURFACE(pIcon_theme);
  
  pWindow->action = minimap_window_callback;

  add_to_gui_list(ID_MINI_MAP_WINDOW, pWindow);

  pMiniMap_Window = pWindow;
  pMiniMap_Dlg->pEndWidgetList = pMiniMap_Window;  

  /* new turn button */
  pWidget = create_themeicon(pTheme->NEW_TURN_Icon, pMiniMap_Window->dst,
			  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Turn Done"), _("Shift+Return"));
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = end_turn_callback;
  pWidget->key = SDLK_RETURN;
  pWidget->mod = KMOD_SHIFT;

  add_to_gui_list(ID_NEW_TURN, pWidget);

  pNew_Turn_Button = pWidget;

  /* players button */
  pWidget = create_themeicon(pTheme->PLAYERS_Icon, pMiniMap_Window->dst,
			     WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  /* TRANS: Nations report action */
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Nations"), "F3");
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = players_action_callback;
  pWidget->key = SDLK_F3;

  add_to_gui_list(ID_PLAYERS, pWidget);

  /* find city button */
  pWidget = create_themeicon(pTheme->FindCity_Icon, pMiniMap_Window->dst,
   			     WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  my_snprintf(buf, sizeof(buf), "%s (%s)\n%s\n%s (%s)", _("Cities Report"),
                                "F4", _("or"), _("Find City"), "Ctrl+F");
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->string16->style |= SF_CENTER;
  pWidget->action = cities_action_callback;
  pWidget->key = SDLK_f;
  pWidget->mod = KMOD_CTRL;

  add_to_gui_list(ID_CITIES, pWidget);
  
  pFind_City_Button = pWidget;

  /* units button */
  pWidget = create_themeicon(pTheme->UNITS2_Icon, pMiniMap_Window->dst,
		             WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Units"), "F2");
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = units_action_callback;
  pWidget->key = SDLK_F2;

  add_to_gui_list(ID_UNITS, pWidget);

  /* show/hide log window button */
  pWidget = create_themeicon(pTheme->LOG_Icon, pMiniMap_Window->dst,
 			     WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Hide Messages"), "F9");
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = toggle_msg_window_callback;
  pWidget->key = SDLK_F9;

  add_to_gui_list(ID_CHATLINE_TOGGLE_LOG_WINDOW_BUTTON, pWidget);

  /* toggle minimap mode button */
  pWidget = create_themeicon(pTheme->BORDERS_Icon, pMiniMap_Window->dst,
 			     WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Toggle Mini Map Mode"), "Shift+\\");
  pWidget->string16 = create_str16_from_char(buf, adj_font(12));
  pWidget->action = toggle_minimap_mode_callback;
  pWidget->key = SDLK_BACKSLASH;
  pWidget->mod = KMOD_SHIFT;

  add_to_gui_list(ID_TOGGLE_MINIMAP_MODE, pWidget);

  #ifdef SMALL_SCREEN
  /* options button */
  pOptions_Button = create_themeicon(pTheme->Options_Icon, pMiniMap_Window->dst,
 			             WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  my_snprintf(buf, sizeof(buf), "%s (%s)", _("Options"), "Esc");
  pOptions_Button->string16 = create_str16_from_char(buf, adj_font(12));
  
  pOptions_Button->action = optiondlg_callback;  
  pOptions_Button->key = SDLK_ESCAPE;

  add_to_gui_list(ID_CLIENT_OPTIONS, pOptions_Button);
  #endif

  /* show/hide minimap button */

  /* make Map Icon */
  pIcon_theme =
      create_surf_alpha(pTheme->MAP_Icon->w, pTheme->MAP_Icon->h, SDL_SWSURFACE);
  alphablit(pTheme->MAP_Icon, NULL, pIcon_theme, NULL);
  alphablit(pTheme->L_ARROW_Icon, NULL, pIcon_theme, NULL);

  pWidget = create_themeicon(pIcon_theme, pMiniMap_Window->dst,
			  WF_FREE_GFX | WF_FREE_THEME |
		          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);

  pWidget->string16 = create_str16_from_char(_("Hide Mini Map"), adj_font(12));
  pWidget->action = toggle_map_window_callback;

  add_to_gui_list(ID_TOGGLE_MAP_WINDOW_BUTTON, pWidget);

  pMiniMap_Dlg->pBeginWidgetList = pWidget;

  SDL_Client_Flags |= CF_OVERVIEW_SHOWN;
  
  set_new_minimap_window_pos();
  
  widget_redraw(pMiniMap_Window);
}

void show_minimap_window_buttons()
{
  struct widget *pWidget = get_minimap_window_widget();
  
  /* new turn button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  
  /* players button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  
  /* find city button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  
  /* units button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  
  /* show/hide log window button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  
  /* toggle minimap mode button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  
  #ifdef SMALL_SCREEN
  /* options button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
  #endif
  
  /* show/hide minimap button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
}

void hide_minimap_window_buttons()
{
  struct widget *pWidget = get_minimap_window_widget();
  
  /* new turn button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  
  /* players button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  
  /* find city button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  
  /* units button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  
  /* show/hide log window button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  
  /* toggle minimap mode button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);

  #ifdef SMALL_SCREEN
  /* options button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
  #endif

  /* show/hide minimap button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
}

void redraw_minimap_window_buttons()
{
  struct widget *pWidget = get_minimap_window_widget();
  
  /* new turn button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);
  
  /* players button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);
  
  /* find city button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);
  
  /* units button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);  
  /* show/hide log window button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);
  
  /* toggle minimap mode button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);

  #ifdef SMALL_SCREEN
  /* options button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);
  #endif

  /* show/hide minimap button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);
}

void disable_minimap_window_buttons()
{
  struct widget *pWidget = get_minimap_window_widget();
  
  /* new turn button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
  
  /* players button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
  
  /* find city button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
  
  /* units button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
  
  /* show/hide log window button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
  
  #ifdef SMALL_SCREEN
  /* options button */
  pWidget = pWidget->prev;
  set_wstate(pWidget, FC_WS_DISABLED);
  #endif
}

void popdown_minimap_window()
{
  if (pMiniMap_Dlg) {
    popdown_window_group_dialog(pMiniMap_Dlg->pBeginWidgetList, pMiniMap_Dlg->pEndWidgetList);
    FC_FREE(pMiniMap_Dlg);
    SDL_Client_Flags &= ~CF_OVERVIEW_SHOWN;
  }
}

void show_game_page()
{
  struct widget *pWidget;
  SDL_Surface *pIcon_theme = NULL;

  if (SDL_Client_Flags & CF_MAP_UNIT_W_CREATED) {
    return;
  }

  popup_minimap_window();
  popup_unitinfo_window();
  SDL_Client_Flags |= CF_MAP_UNIT_W_CREATED;

  #ifndef SMALL_SCREEN
  init_options_button();
  #endif
  
  /* cooling icon */
  pIcon_theme = adj_surf(GET_SURF(client_cooling_sprite()));
  assert(pIcon_theme != NULL);
  pWidget = create_iconlabel(pIcon_theme, Main.gui, NULL, WF_FREE_THEME);

#ifdef SMALL_SCREEN
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w - adj_size(10),
                      0);
#else
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w - adj_size(10),
                      adj_size(10));
#endif

  add_to_gui_list(ID_COOLING_ICON, pWidget);

  /* warming icon */
  pIcon_theme = adj_surf(GET_SURF(client_warming_sprite()));
  assert(pIcon_theme != NULL);

  pWidget = create_iconlabel(pIcon_theme, Main.gui, NULL, WF_FREE_THEME);

#ifdef SMALL_SCREEN
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w * 2 - adj_size(10),
                      0);
#else
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w * 2 - adj_size(10),
                      adj_size(10));
#endif

  add_to_gui_list(ID_WARMING_ICON, pWidget);

  /* create order buttons */
  create_units_order_widgets();

  /* enable options button and order widgets */
  enable_options_button();
  enable_order_buttons();
}

void close_game_page()
{
  struct widget *pWidget;

  del_widget_from_gui_list(pOptions_Button);
  
  pWidget = get_widget_pointer_form_main_list(ID_COOLING_ICON);
  del_widget_from_gui_list(pWidget);
  
  pWidget = get_widget_pointer_form_main_list(ID_WARMING_ICON);
  del_widget_from_gui_list(pWidget);
  
  delete_units_order_widgets();
  
  popdown_minimap_window();
  popdown_unitinfo_window();
  SDL_Client_Flags &= ~CF_MAP_UNIT_W_CREATED;
}

static void disable_minimap_widgets()
{
  struct widget *pBuf, *pEnd;

  pBuf = pMiniMap_Window;
  set_wstate(pBuf, FC_WS_DISABLED);

  /* new turn button */
  pBuf = pBuf->prev;
  pEnd = pBuf;
  set_wstate(pBuf, FC_WS_DISABLED);

  /* players button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);

  /* find city button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);

  /* units button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);

  /* show/hide log window button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);

  /* toggle minimap mode button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);

  #ifdef SMALL_SCREEN
  /* options button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);
  #endif

  /* show/hide minimap button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);

  redraw_group(pBuf, pEnd, TRUE);
}

static void disable_unitinfo_widgets()
{
  struct widget *pBuf = pUnits_Info_Window->private_data.adv_dlg->pBeginWidgetList;
  struct widget *pEnd = pUnits_Info_Window->private_data.adv_dlg->pEndWidgetList;
      
  set_group_state(pBuf, pEnd, FC_WS_DISABLED);    
  pEnd = pEnd->prev;
  redraw_group(pBuf, pEnd, TRUE);
}

void disable_main_widgets(void)
{
  if (C_S_RUNNING == client_state()) {
    disable_minimap_widgets();
    disable_unitinfo_widgets();
    
    disable_options_button();

    disable_order_buttons();
  }
}

static void enable_minimap_widgets()
{
  struct widget *pBuf, *pEnd;

  if (can_client_issue_orders()) {
  
    pBuf = pMiniMap_Window;
    set_wstate(pBuf, FC_WS_NORMAL);

    /* new turn button */
    pBuf = pBuf->prev;
    pEnd = pBuf;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    /* players button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    /* find city button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    /* units button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);

    /* show/hide log window button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);

    /* toggle minimap mode button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);

    #ifdef SMALL_SCREEN
    /* options button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);
    #endif

    /* show/hide minimap button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);

    redraw_group(pBuf, pEnd, TRUE);
  }
}

static void enable_unitinfo_widgets()
{
  struct widget *pBuf, *pEnd;

  if (can_client_issue_orders()) {
    pBuf = pUnits_Info_Window->private_data.adv_dlg->pBeginWidgetList;    
    pEnd = pUnits_Info_Window->private_data.adv_dlg->pEndWidgetList;
        
    set_group_state(pBuf, pEnd, FC_WS_NORMAL);
    pEnd = pEnd->prev;
    redraw_group(pBuf, pEnd, TRUE);
  }
}

void enable_main_widgets(void)
{
  if (C_S_RUNNING == client_state()) {

    enable_minimap_widgets();
    enable_unitinfo_widgets();
    
    enable_options_button();
    
    enable_order_buttons();
  }
}

struct widget * get_unit_info_window_widget(void)
{
  return pUnits_Info_Window;
}

struct widget * get_minimap_window_widget(void)
{
  return pMiniMap_Window;
}

struct widget * get_tax_rates_widget(void)
{
  return pTax_Button;
}

struct widget * get_research_widget(void)
{
  return pResearch_Button;
}

struct widget * get_revolution_widget(void)
{
  return pRevolution_Button;
}

void enable_and_redraw_find_city_button(void)
{
  set_wstate(pFind_City_Button, FC_WS_NORMAL);
  widget_redraw(pFind_City_Button);
  widget_mark_dirty(pFind_City_Button);
}

void enable_and_redraw_revolution_button(void)
{
  set_wstate(pRevolution_Button, FC_WS_NORMAL);
  widget_redraw(pRevolution_Button);
  widget_mark_dirty(pRevolution_Button);
}

/**************************************************************************
  mouse click handler
**************************************************************************/
void button_down_on_map(struct mouse_button_behavior *button_behavior)
{
  struct tile *ptile;
  
  if (C_S_RUNNING != client_state()) {
    return;
  }
  
  if (button_behavior->event->button == SDL_BUTTON_LEFT) {
    switch(button_behavior->hold_state) {
      case MB_HOLD_SHORT:
        break;
      case MB_HOLD_MEDIUM:
        /* switch to goto mode */
        key_unit_goto();
        update_mouse_cursor(CURSOR_GOTO);
        break;
      case MB_HOLD_LONG:
#ifdef UNDER_CE
        /* cancel goto mode and open context menu on Pocket PC since we have
         * only one 'mouse button' */
        key_cancel_action();
        draw_goto_patrol_lines = FALSE;
        update_mouse_cursor(CURSOR_DEFAULT);      
        /* popup context menu */
        if ((ptile = canvas_pos_to_tile((int) button_behavior->event->x,
                                        (int) button_behavior->event->y))) {
          popup_advanced_terrain_dialog(ptile, button_behavior->event->x, 
                                               button_behavior->event->y);
        }
#endif
        break;
      default:
        break;
    }
  } else if (button_behavior->event->button == SDL_BUTTON_MIDDLE) {
    switch(button_behavior->hold_state) {
      case MB_HOLD_SHORT:
        break;      
      case MB_HOLD_MEDIUM:
        break;
      case MB_HOLD_LONG:
        break;
      default:
        break;
    }
  } else if (button_behavior->event->button == SDL_BUTTON_RIGHT) {
    switch (button_behavior->hold_state) {
      case MB_HOLD_SHORT:
        break;
      case MB_HOLD_MEDIUM:      
        /* popup context menu */
        if ((ptile = canvas_pos_to_tile((int) button_behavior->event->x,
                                        (int) button_behavior->event->y))) {
          popup_advanced_terrain_dialog(ptile, button_behavior->event->x,
                                               button_behavior->event->y);
        }
        break;
      case MB_HOLD_LONG:
        break;
      default:
        break;
    }
  }
    
}

void button_up_on_map(struct mouse_button_behavior *button_behavior)
{
  struct tile *ptile;
  struct city *pCity;
    
  if (C_S_RUNNING != client_state()) {
    return;
  }
  
  draw_goto_patrol_lines = FALSE;
  
  if (button_behavior->event->button == SDL_BUTTON_LEFT) {
    switch(button_behavior->hold_state) {
      case MB_HOLD_SHORT:
        if(LSHIFT || LALT || LCTRL) {
          if ((ptile = canvas_pos_to_tile((int) button_behavior->event->x,
                                          (int) button_behavior->event->y))) {
            if(LSHIFT) {
              popup_advanced_terrain_dialog(ptile, button_behavior->event->x,
                                                   button_behavior->event->y);
            } else {
              if(((pCity = tile_city(ptile)) != NULL) &&
                (city_owner(pCity) == client.conn.playing)) {
                if(LCTRL) {
                  popup_worklist_editor(pCity, &(pCity->worklist));
                } else {
                  /* LALT - this work only with fullscreen mode */
                  popup_hurry_production_dialog(pCity, NULL);
                }
              }
            }		      
          }
        } else {
          update_mouse_cursor(CURSOR_DEFAULT);
          action_button_pressed(button_behavior->event->x,
                                     button_behavior->event->y, SELECT_POPUP);
        }
        break;
      case MB_HOLD_MEDIUM:
        /* finish goto */
        update_mouse_cursor(CURSOR_DEFAULT);
        action_button_pressed(button_behavior->event->x,
                                     button_behavior->event->y, SELECT_POPUP);
        break;
      case MB_HOLD_LONG:
#ifndef UNDER_CE
        /* finish goto */
        update_mouse_cursor(CURSOR_DEFAULT);
        action_button_pressed(button_behavior->event->x,
                                     button_behavior->event->y, SELECT_POPUP);
#endif
        break;
      default:
        break;
    }
  } else if (button_behavior->event->button == SDL_BUTTON_MIDDLE) {
    switch(button_behavior->hold_state) {
      case MB_HOLD_SHORT:
/*        break;*/
      case MB_HOLD_MEDIUM:
/*        break;*/
      case MB_HOLD_LONG:
/*        break;*/
      default:
        /* popup context menu */
        if ((ptile = canvas_pos_to_tile((int) button_behavior->event->x,
                                        (int) button_behavior->event->y))) {
          popup_advanced_terrain_dialog(ptile, button_behavior->event->x,
                                               button_behavior->event->y);
        }
        break;
    }
  } else if (button_behavior->event->button == SDL_BUTTON_RIGHT) {
    switch (button_behavior->hold_state) {
      case MB_HOLD_SHORT:
        /* recenter map */
        recenter_button_pressed(button_behavior->event->x, button_behavior->event->y);
        flush_dirty();
        break;
      case MB_HOLD_MEDIUM:      
        break;
      case MB_HOLD_LONG:
        break;
      default:
        break;
    }
  }
  
}
  
/**************************************************************************
  Toggle map drawing stuff.
**************************************************************************/
bool map_event_handler(SDL_keysym Key)
{
  if (C_S_RUNNING == client_state()) {
    switch (Key.sym) {
    
      /* cancel action */
      case SDLK_ESCAPE:
        key_cancel_action();
        draw_goto_patrol_lines = FALSE;
        update_mouse_cursor(CURSOR_DEFAULT);
        return FALSE;
  
      /* *** unit movement *** */
      
      /* move north */
      case SDLK_UP:
      case SDLK_KP8:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_NORTH);
        }
        return FALSE;
  
      /* move northeast */
      case SDLK_PAGEUP:
      case SDLK_KP9:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_NORTHEAST);
        }
        return FALSE;
  
      /* move east */
      case SDLK_RIGHT:
      case SDLK_KP6:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_EAST);
        }
        return FALSE;
  
      /* move southeast */
      case SDLK_PAGEDOWN:
      case SDLK_KP3:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_SOUTHEAST);
        }
        return FALSE;
  
      /* move south */
      case SDLK_DOWN:
      case SDLK_KP2:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_SOUTH);
        }
        return FALSE;
  
      /* move southwest */    
      case SDLK_END:
      case SDLK_KP1:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_SOUTHWEST);
        }
        return FALSE;
  
      /* move west */
      case SDLK_LEFT:
      case SDLK_KP4:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_WEST);
        }
        return FALSE;
  
      /* move northwest */
      case SDLK_HOME:
      case SDLK_KP7:
        if(!is_unit_move_blocked) {
          key_unit_move(DIR8_NORTHWEST);
        }
        return FALSE;
  
      case SDLK_KP5:
        key_recall_previous_focus_unit();
        return FALSE;
  
      /* *** map view settings *** */
  
      /* show city outlines - Ctrl+y */
      case SDLK_y:
        if(LCTRL || RCTRL) {
          key_city_outlines_toggle();
        }
        return FALSE;
      
      /* show map grid - Ctrl+g */
      case SDLK_g:
        if(LCTRL || RCTRL) {
          key_map_grid_toggle();
        }
        return FALSE;
  
      /* show national borders - Ctrl+b */
      case SDLK_b:
        if(LCTRL || RCTRL) {
          key_map_borders_toggle();
        }
        return FALSE;
  
      /* show city names - Ctrl+n */
      case SDLK_n:
        if (LCTRL || RCTRL) {
          key_city_names_toggle();
        }
        return FALSE;
  
      /* show city growth Ctrl+r */
      case SDLK_r:
        if (LCTRL || RCTRL) {
          key_city_growth_toggle();
        }
        return FALSE;
  
      /* show city productions - Ctrl+p */
      case SDLK_p:
        if (LCTRL || RCTRL) {
          key_city_productions_toggle();
        }
        return FALSE;

      /* show city traderoutes - Ctrl+d */
      case SDLK_d:
        if (LCTRL || RCTRL) {
          key_city_traderoutes_toggle();
        }
        return FALSE;

      /* *** some additional shortcuts that work in the SDL client only *** */
        
      /* show terrain - Ctrl+Shift+t */
      case SDLK_t:
        if ((LCTRL || RCTRL) && (LSHIFT || RSHIFT)) {
          key_terrain_toggle();
        }
        return FALSE;
  
      /* (show coastline) */
        
      /* (show roads and rails) */
        
      /* show irrigation - Ctrl+i */
      case SDLK_i:
        if (LCTRL || RCTRL) {
          key_irrigation_toggle();
        }
        return FALSE;
  
      /* show mines - Ctrl+m */
      case SDLK_m:
        if (LCTRL || RCTRL) {
          key_mines_toggle();
        }
        return FALSE;
  
      /* show fortresses and airbases - Ctrl+Shift+f */
      case SDLK_f:
        if ((LCTRL || RCTRL) && (LSHIFT || RSHIFT)) {
          request_toggle_fortress_airbase();
        }
        return FALSE;
  
      /* show specials - Ctrl+s */
      case SDLK_s:
        if (LCTRL || RCTRL) {
          key_specials_toggle();
        }
        return FALSE;
  
      /* show pollution - Ctrl+o */
      case SDLK_o:
        if (LCTRL || RCTRL) {
          key_pollution_toggle();
        }
        return FALSE;
  
      /* show cities - Ctrl+c */
      case SDLK_c:
        if (LCTRL || RCTRL) {
          request_toggle_cities();
        } else {
  	 request_center_focus_unit();
        }
        return FALSE;

      /* show units - Ctrl+u */
      case SDLK_u:
        if (LCTRL || RCTRL) {
          key_units_toggle();
        }
        return FALSE;
  
      /* (show focus unit) */
      
      /* show city output - Ctrl+w
       * show fog of war - Ctrl+Shift+w */
      case SDLK_w:
        if (LCTRL || RCTRL) {
          if (LSHIFT || RSHIFT) {
            key_fog_of_war_toggle();
          } else {
            key_city_output_toggle();
          }
        }
        return FALSE;
  
      /* toggle minimap mode - currently without effect */
      case SDLK_BACKSLASH:
        if (LSHIFT || RSHIFT) {
          toggle_minimap_mode_callback(NULL);
        }
        return FALSE;
              
      default:
        break;
    }
  }

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static int newcity_ok_edit_callback(struct widget *pEdit) {
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *input =
            convert_to_chars(pNewCity_Dlg->pBeginWidgetList->string16->text);
  
    if (input) {
      FC_FREE(input);
    } else {
      /* empty input -> restore previous content */
      copy_chars_to_string16(pEdit->string16, pSuggestedCityName);
      widget_redraw(pEdit);
      widget_mark_dirty(pEdit);
      flush_dirty();
    }
  }  
  return -1;
}
/**************************************************************************
  ...
**************************************************************************/
static int newcity_ok_callback(struct widget *pOk_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    char *input =
            convert_to_chars(pNewCity_Dlg->pBeginWidgetList->string16->text);
    
    dsend_packet_unit_build_city(&client.conn, pOk_Button->data.unit->id,
                                 input);
    FC_FREE(input);
  
    popdown_window_group_dialog(pNewCity_Dlg->pBeginWidgetList,
                                pNewCity_Dlg->pEndWidgetList);
    FC_FREE(pNewCity_Dlg);
    
    FC_FREE(pSuggestedCityName);
    
    flush_dirty();
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int newcity_cancel_callback(struct widget *pCancel_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_window_group_dialog(pNewCity_Dlg->pBeginWidgetList,
                                pNewCity_Dlg->pEndWidgetList);
    FC_FREE(pNewCity_Dlg);
    
    FC_FREE(pSuggestedCityName);  
    
    flush_dirty();
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int move_new_city_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pNewCity_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/* ============================== Native =============================== */

  
/**************************************************************************
  Popup a dialog to ask for the name of a new city.  The given string
  should be used as a suggestion.
**************************************************************************/
void popup_newcity_dialog(struct unit *pUnit, char *pSuggestname)
{
  SDL_Surface *pBackground;
  struct SDL_String16 *pStr = NULL;
  struct widget *pLabel = NULL;
  struct widget *pWindow = NULL;
  struct widget *pCancel_Button = NULL;
  struct widget *pOK_Button;
  struct widget *pEdit;
  SDL_Rect area;

  if(pNewCity_Dlg) {
    return;
  }

  pSuggestedCityName = fc_calloc(1, strlen(pSuggestname) + 1);
  mystrlcpy(pSuggestedCityName, pSuggestname, strlen(pSuggestname) + 1);
  
  pNewCity_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  /* create window */
  pStr = create_str16_from_char(_("Build City"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  pWindow = create_window_skeleton(NULL, pStr, 0);
  pWindow->action = move_new_city_dlg_callback;

  area = pWindow->area;
  
  /* create ok button */
  pOK_Button =
    create_themeicon_button_from_chars(pTheme->Small_OK_Icon, pWindow->dst,
					  _("OK"), adj_font(10), 0);
  pOK_Button->action = newcity_ok_callback;
  pOK_Button->key = SDLK_RETURN;  
  pOK_Button->data.unit = pUnit;  

  area.h += pOK_Button->size.h;
  
  /* create cancel button */
  pCancel_Button =
      create_themeicon_button_from_chars(pTheme->Small_CANCEL_Icon,
  			pWindow->dst, _("Cancel"), adj_font(10), 0);
  pCancel_Button->action = newcity_cancel_callback;
  pCancel_Button->key = SDLK_ESCAPE;  

  /* correct sizes */
  pCancel_Button->size.w += adj_size(5);
  pOK_Button->size.w = pCancel_Button->size.w;
  
  /* create text label */
  pStr = create_str16_from_char(_("What should we call our new city?"), adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pStr->fgcol = *get_game_colorRGB(COLOR_THEME_NEWCITYDLG_TEXT);
  pLabel = create_iconlabel(NULL, pWindow->dst, pStr, WF_DRAW_TEXT_LABEL_WITH_SPACE);
  
  area.h += pLabel->size.h;
  
  pEdit = create_edit(NULL, pWindow->dst, create_str16_from_char(pSuggestname, adj_font(12)),
     (pOK_Button->size.w + pCancel_Button->size.w + adj_size(15)), WF_RESTORE_BACKGROUND);
  pEdit->action = newcity_ok_edit_callback;

  area.w = MAX(area.w, pEdit->size.w + adj_size(20));
  area.h += pEdit->size.h + adj_size(25);

  /* I make this hack to center label on window */
  if (pLabel->size.w < area.w)
  {
    pLabel->size.w = area.w;
  } else { 
    area.w = MAX(pWindow->area.w, pLabel->size.w + adj_size(10));
  }
  
  pEdit->size.w = area.w - adj_size(20);

  /* create window background */
  pBackground = theme_get_background(theme, BACKGROUND_NEWCITYDLG);
  if (resize_window(pWindow, pBackground, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pBackground);
  }
  
  area = pWindow->area;
  
  /* set start positions */
  widget_set_position(pWindow,
    (Main.screen->w - pWindow->size.w) / 2,
    (Main.screen->h - pWindow->size.h) / 2);

  pOK_Button->size.x = area.x + adj_size(10);
  pOK_Button->size.y = area.y + area.h - pOK_Button->size.h - adj_size(10);

  pCancel_Button->size.y = pOK_Button->size.y;
  pCancel_Button->size.x = area.x + area.w - pCancel_Button->size.w - adj_size(10);

  pEdit->size.x = area.x + adj_size(10);
  pEdit->size.y = area.y + adj_size(4) + pLabel->size.h + adj_size(3);

  pLabel->size.x = area.x + adj_size(3);
  pLabel->size.y = area.y + adj_size(4);

  /* enable widgets */
  set_wstate(pCancel_Button, FC_WS_NORMAL);
  set_wstate(pOK_Button, FC_WS_NORMAL);
  set_wstate(pEdit, FC_WS_NORMAL);
  set_wstate(pWindow, FC_WS_NORMAL);

  /* add widgets to main list */
  pNewCity_Dlg->pEndWidgetList = pWindow;
  add_to_gui_list(ID_NEWCITY_NAME_WINDOW, pWindow);
  add_to_gui_list(ID_NEWCITY_NAME_LABEL, pLabel);
  add_to_gui_list(ID_NEWCITY_NAME_CANCEL_BUTTON, pCancel_Button);
  add_to_gui_list(ID_NEWCITY_NAME_OK_BUTTON, pOK_Button);
  add_to_gui_list(ID_NEWCITY_NAME_EDIT, pEdit);
  pNewCity_Dlg->pBeginWidgetList = pEdit;

  /* redraw */
  redraw_group(pEdit, pWindow, 0);

  widget_flush(pWindow);
}

/**************************************************************************
  ...
**************************************************************************/
void popdown_newcity_dialog(void)
{
  if(pNewCity_Dlg) {
    popdown_window_group_dialog(pNewCity_Dlg->pBeginWidgetList,
			      pNewCity_Dlg->pEndWidgetList);
    FC_FREE(pNewCity_Dlg);
    flush_dirty();
  }
}

/**************************************************************************
  A turn done button should be provided for the player.  This function
  is called to toggle it between active/inactive.
**************************************************************************/
void set_turn_done_button_state(bool state)
{
  if (C_S_RUNNING == client_state()) {
    if (state) {
      set_wstate(pNew_Turn_Button, FC_WS_NORMAL);
    } else {
      set_wstate(pNew_Turn_Button, FC_WS_DISABLED);
    }
    widget_redraw(pNew_Turn_Button);
    widget_flush(pNew_Turn_Button);
  }
}

/**************************************************************************
  Draw a goto or patrol line at the current mouse position.
**************************************************************************/
void create_line_at_mouse_pos(void)
{
  int pos_x, pos_y;
    
  SDL_GetMouseState(&pos_x, &pos_y);
  update_line(pos_x, pos_y);
  draw_goto_patrol_lines = TRUE;
}

/**************************************************************************
 The Area Selection rectangle. Called by center_tile_mapcanvas() and
 when the mouse pointer moves.
**************************************************************************/
void update_rect_at_mouse_pos(void)
{
  /* PORTME */
}
