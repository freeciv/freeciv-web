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
                          mapctrl.c  -  description
                             -------------------
    begin                : Thu Sep 05 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

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
#include "unit.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "overview_common.h"
#include "update_queue.h"

/* client/gui-sdl2 */
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
#include "pages.h"
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
#endif /* SCALE_MINIMAP */

#ifdef SCALE_UNITINFO
static struct SMALL_DLG *pScale_UnitInfo_Dlg = NULL;
static int INFO_WIDTH, INFO_HEIGHT = 0, INFO_WIDTH_MIN, INFO_HEIGHT_MIN;
static int popdown_scale_unitinfo_dlg_callback(struct widget *pWidget);
static void remake_unitinfo(int w, int h);
#endif /* SCALE_UNITINFO */

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

/**********************************************************************//**
  User interacted with nations button.
**************************************************************************/
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
#endif /* 0 */
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

/**********************************************************************//**
  User interacted with units button.
**************************************************************************/
static int units_action_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_wstate(pWidget, FC_WS_NORMAL);
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    units_report_dialog_popup(FALSE);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with cities button.
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
#endif /* 0 */
      case SDL_BUTTON_RIGHT:
        popup_find_dialog();
      break;
      default:
        city_report_dialog_popup(FALSE);
      break;
    }
  } else {
    popup_find_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with Turn Done button.
**************************************************************************/
static int end_turn_callback(struct widget *pButton)
{
  if (Main.event.type == SDL_KEYDOWN
      || (Main.event.type == SDL_MOUSEBUTTONDOWN
          && Main.event.button.button == SDL_BUTTON_LEFT)) {
    widget_redraw(pButton);
    widget_flush(pButton);
    disable_focus_animation();
    key_end_turn();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with Revolution button.
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

/**********************************************************************//**
  User interacted with Research button.
**************************************************************************/
static int research_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    science_report_dialog_popup(TRUE);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with Economy button.
**************************************************************************/
static int economy_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    economy_report_dialog_popup(FALSE);
  }

  return -1;
}

/* ====================================== */

/**********************************************************************//**
  Show/Hide Units Info Window
**************************************************************************/
static int toggle_unit_info_window_callback(struct widget *pIcon_Widget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct widget *pBuf = NULL;

    clear_surface(pIcon_Widget->theme, NULL);
    alphablit(current_theme->MAP_Icon, NULL, pIcon_Widget->theme, NULL, 255);

    if (get_num_units_in_focus() > 0) {
      undraw_order_widgets();
    }

    if (SDL_Client_Flags & CF_UNITINFO_SHOWN) {
      /* HIDE */
      SDL_Surface *pBuf_Surf;
      SDL_Rect src, window_area;

      set_wstate(pIcon_Widget, FC_WS_NORMAL);
      selected_widget = NULL;

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
      alphablit(current_theme->L_ARROW_Icon, NULL, pIcon_Widget->theme, NULL, 255);

      copy_chars_to_utf8_str(pIcon_Widget->info_label,
                             _("Show Unit Info Window"));

      SDL_Client_Flags &= ~CF_UNITINFO_SHOWN;

      set_new_unitinfo_window_pos();

      /* blit part of map window */
      src.x = 0;
      src.y = 0;
      src.w = (pUnits_Info_Window->size.w - pUnits_Info_Window->area.w) + BLOCKU_W;
      src.h = pUnits_Info_Window->theme->h;

      window_area = pUnits_Info_Window->size;
      alphablit(pUnits_Info_Window->theme, &src,
                pUnits_Info_Window->dst->surface, &window_area, 255);

      /* blit right vertical frame */
      pBuf_Surf = ResizeSurface(current_theme->FR_Right, current_theme->FR_Right->w,
                                pUnits_Info_Window->area.h, 1);

      window_area.y = pUnits_Info_Window->area.y;
      window_area.x = pUnits_Info_Window->area.x + pUnits_Info_Window->area.w;
      alphablit(pBuf_Surf, NULL,
                pUnits_Info_Window->dst->surface, &window_area, 255);
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
      if (main_window_width() - pUnits_Info_Window->size.w >=
                  pMiniMap_Window->dst->dest_rect.x + pMiniMap_Window->size.w) {

        set_wstate(pIcon_Widget, FC_WS_NORMAL);
        selected_widget = NULL;

        /* SHOW */
        copy_chars_to_utf8_str(pIcon_Widget->info_label,
                               _("Hide Unit Info Window"));

        alphablit(current_theme->R_ARROW_Icon, NULL, pIcon_Widget->theme, NULL, 255);

        SDL_Client_Flags |= CF_UNITINFO_SHOWN;

        set_new_unitinfo_window_pos();

        widget_mark_dirty(pUnits_Info_Window);

        redraw_unit_info_label(get_units_in_focus());
      } else {
        alphablit(current_theme->L_ARROW_Icon, NULL, pIcon_Widget->theme, NULL, 255);
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

/**********************************************************************//**
  Show/Hide Mini Map
**************************************************************************/
static int toggle_map_window_callback(struct widget *pMap_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *pFocus = head_of_units_in_focus();
    struct widget *pWidget;

    /* make new map icon */
    clear_surface(pMap_Button->theme, NULL);
    alphablit(current_theme->MAP_Icon, NULL, pMap_Button->theme, NULL, 255);

    set_wstate(pMiniMap_Window, FC_WS_NORMAL);

    if (pFocus) {
      undraw_order_widgets();
    }

    if (SDL_Client_Flags & CF_OVERVIEW_SHOWN) {
      /* Hide MiniMap */
      SDL_Surface *pBuf_Surf;
      SDL_Rect src, map_area = pMiniMap_Window->size;

      set_wstate(pMap_Button, FC_WS_NORMAL);
      selected_widget = NULL;

      copy_chars_to_utf8_str(pMap_Button->info_label, _("Show Mini Map"));

      /* make new map icon */
      alphablit(current_theme->R_ARROW_Icon, NULL, pMap_Button->theme, NULL, 255);

      SDL_Client_Flags &= ~CF_OVERVIEW_SHOWN;

      /* clear area under old map window */
      widget_undraw(pMiniMap_Window);
      widget_mark_dirty(pMiniMap_Window);

      pMiniMap_Window->size.w = (pMiniMap_Window->size.w - pMiniMap_Window->area.w) + BLOCKM_W;
      pMiniMap_Window->area.w = BLOCKM_W;

      set_new_minimap_window_pos();

      /* blit part of map window */
      src.x = pMiniMap_Window->theme->w - BLOCKM_W - current_theme->FR_Right->w;
      src.y = 0;
      src.w = BLOCKM_W + current_theme->FR_Right->w;
      src.h = pMiniMap_Window->theme->h;

      alphablit(pMiniMap_Window->theme, &src,
                pMiniMap_Window->dst->surface, &map_area, 255);

      /* blit left vertical frame theme */
      pBuf_Surf = ResizeSurface(current_theme->FR_Left, current_theme->FR_Left->w,
                                pMiniMap_Window->area.h, 1);

      map_area.y += adj_size(2);
      alphablit(pBuf_Surf, NULL, pMiniMap_Window->dst->surface, &map_area, 255);
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
#endif /* SMALL_SCREEN */

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
        selected_widget = NULL;

        /* show MiniMap */
        copy_chars_to_utf8_str(pMap_Button->info_label, _("Hide Mini Map"));

        alphablit(current_theme->L_ARROW_Icon, NULL, pMap_Button->theme, NULL, 255);
        SDL_Client_Flags |= CF_OVERVIEW_SHOWN;

        pMiniMap_Window->size.w =
          (pMiniMap_Window->size.w - pMiniMap_Window->area.w) + overview_w + BLOCKM_W;
        pMiniMap_Window->area.w = overview_w + BLOCKM_W;

        set_new_minimap_window_pos();

        widget_redraw(pMiniMap_Window);
        redraw_minimap_window_buttons();
        refresh_overview();
      } else {
        alphablit(current_theme->R_ARROW_Icon, NULL, pMap_Button->theme, NULL, 255);
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

/**********************************************************************//**
  User interacted with minimap toggling button.
**************************************************************************/
static int toggle_minimap_mode_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pWidget != NULL) {
      selected_widget = pWidget;
      set_wstate(pWidget, FC_WS_SELECTED);
    }
    toggle_overview_mode();
    refresh_overview();
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with messages toggling button.
**************************************************************************/
static int toggle_msg_window_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (meswin_dialog_is_open()) {
      meswin_dialog_popdown();
      copy_chars_to_utf8_str(pWidget->info_label, _("Show Messages (F9)"));
    } else {
      meswin_dialog_popup(TRUE);
      copy_chars_to_utf8_str(pWidget->info_label, _("Hide Messages (F9)"));
    }

    selected_widget = pWidget;
    set_wstate(pWidget, FC_WS_SELECTED);
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);

    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  Update the size of the minimap
**************************************************************************/
int resize_minimap(void)
{
  overview_w = gui_options.overview.width;
  overview_h = gui_options.overview.height;
  overview_start_x = (overview_w - gui_options.overview.width) / 2;
  overview_start_y = (overview_h - gui_options.overview.height) / 2;

  if (C_S_RUNNING == client_state()) {
    popdown_minimap_window();
    popup_minimap_window();
    refresh_overview();
    menus_update();
  }

  return 0;
}

#ifdef SCALE_MINIMAP
/* ============================================================== */

/**********************************************************************//**
  User interacted with minimap scaling dialog
**************************************************************************/
static int move_scale_minimap_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pScale_MiniMap_Dlg->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with minimap scaling dialog closing button.
**************************************************************************/
static int popdown_scale_minimap_dlg_callback(struct widget *pWidget)
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

/**********************************************************************//**
  User interacted with minimap width increase button.
**************************************************************************/
static int up_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if ((((OVERVIEW_TILE_WIDTH + 1) * map.xsize) +
         (pMiniMap_Window->size.w - pMiniMap_Window->area.w) + BLOCKM_W) <=
         pUnits_Info_Window->dst->dest_rect.x) {
      char cBuf[4];

      fc_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_WIDTH);
      copy_chars_to_utf8_str(pWidget->next->string_utf8, cBuf);
      widget_redraw(pWidget->next);
      widget_mark_dirty(pWidget->next);

      calculate_overview_dimensions();
      resize_minimap();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with minimap width decrease button.
**************************************************************************/
static int down_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (OVERVIEW_TILE_WIDTH > 1) {
      char cBuf[4];

      fc_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_WIDTH);
      copy_chars_to_utf8_str(pWidget->prev->string_utf8, cBuf);
      widget_redraw(pWidget->prev);
      widget_mark_dirty(pWidget->prev);

      resize_minimap();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with minimap height increase button.
**************************************************************************/
static int up_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (Main.screen->h -
      ((OVERVIEW_TILE_HEIGHT + 1) * map.ysize + (current_theme->FR_Bottom->h * 2)) >= 40) {
      char cBuf[4];

      OVERVIEW_TILE_HEIGHT++;
      fc_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_HEIGHT);
      copy_chars_to_utf8_str(pWidget->next->string_utf8, cBuf);
      widget_redraw(pWidget->next);
      widget_mark_dirty(pWidget->next);
      resize_minimap();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with minimap height decrease button.
**************************************************************************/
static int down_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (OVERVIEW_TILE_HEIGHT > 1) {
      char cBuf[4];

      OVERVIEW_TILE_HEIGHT--;
      fc_snprintf(cBuf, sizeof(cBuf), "%d", OVERVIEW_TILE_HEIGHT);
      copy_chars_to_utf8_str(pWidget->prev->string_utf8, cBuf);
      widget_redraw(pWidget->prev);
      widget_mark_dirty(pWidget->prev);

      resize_minimap();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  Open minimap scaling dialog.
**************************************************************************/
static void popup_minimap_scale_dialog(void)
{
  SDL_Surface *pText1, *pText2;
  utf8_str *pstr = NULL;
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
  pstr = create_utf8_from_char(_("Scale Mini Map"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;
  pWindow = create_window_skeleton(NULL, pstr, 0);
  pWindow->action = move_scale_minimap_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  add_to_gui_list(ID_WINDOW, pWindow);
  pScale_MiniMap_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  /* ----------------- */
  pstr = create_utf8_from_char(_("Single Tile Width"), adj_font(12));
  pText1 = create_text_surf_from_utf8(pstr);
  area.w = MAX(area.w, pText1->w + adj_size(30));

  copy_chars_to_utf8_str(pstr, _("Single Tile Height"));
  pText2 = create_text_surf_from_utf8(pstr);
  area.w = MAX(area.w, pText2->w + adj_size(30));
  FREEUTF8_STR(pstr);

  pBuf = create_themeicon_button(current_theme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);

  fc_snprintf(cBuf, sizeof(cBuf), "%d" , OVERVIEW_TILE_WIDTH);
  pstr = create_utf8_from_char(cBuf, adj_font(24));
  pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pBuf = create_iconlabel(NULL, pWindow->dst, pstr, WF_RESTORE_BACKGROUND);
  pBuf->size.w = MAX(adj_size(50), pBuf->size.w);
  area.h += pBuf->size.h + adj_size(5);
  add_to_gui_list(ID_LABEL, pBuf);

  pBuf = create_themeicon_button(current_theme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);

  /* ------------ */
  pBuf = create_themeicon_button(current_theme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);

  fc_snprintf(cBuf, sizeof(cBuf), "%d" , OVERVIEW_TILE_HEIGHT);
  pstr = create_utf8_from_char(cBuf, adj_font(24));
  pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pBuf = create_iconlabel(NULL, pWindow->dst, pstr, WF_RESTORE_BACKGROUND);
  pBuf->size.w = MAX(adj_size(50), pBuf->size.w);
  area.h += pBuf->size.h + adj_size(20);
  add_to_gui_list(ID_LABEL, pBuf);

  pBuf = create_themeicon_button(current_theme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.w = MAX(area.w , pBuf->size.w * 2 + pBuf->next->size.w + adj_size(20));

  /* ------------ */
  pstr = create_utf8_from_char(_("Exit"), adj_font(12));
  pBuf = create_themeicon_button(current_theme->CANCEL_Icon, pWindow->dst, pstr, 0);
  pBuf->action = popdown_scale_minimap_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pScale_MiniMap_Dlg->pBeginWidgetList = pBuf;
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h + adj_size(10);
  area.w = MAX(area.w, pBuf->size.w + adj_size(20));
  /* ------------ */

  area.h += adj_size(20);

  resize_window(pWindow, NULL, get_theme_color(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  if (Main.event.motion.x + pWindow->size.w > main_window_width()) {
    if (Main.event.motion.x - pWindow->size.w >= 0) {
      window_x = Main.event.motion.x - pWindow->size.w;
    } else {
      window_x = (main_window_width() - pWindow->size. w) / 2;
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
#endif /* SCALE_MINIMAP */

/* ==================================================================== */
#ifdef SCALE_UNITINFO

/**********************************************************************//**
  User interacted with unitinfo scaling dialog.
**************************************************************************/
static int move_scale_unitinfo_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pScale_UnitInfo_Dlg->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  Close unitinfo scaling dialog.
**************************************************************************/
static int popdown_scale_unitinfo_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pScale_UnitInfo_Dlg) {
      popdown_window_group_dialog(pScale_UnitInfo_Dlg->pBeginWidgetList,
                                  pScale_UnitInfo_Dlg->pEndWidgetList);
      FC_FREE(pScale_UnitInfo_Dlg);
      if (pWidget) {
        flush_dirty();
      }
    }
  }

  return -1;
}

/**********************************************************************//**
  Rebuild unitinfo widget.
**************************************************************************/
static void remake_unitinfo(int w, int h)
{
  SDL_Color bg_color = {255, 255, 255, 128};
  SDL_Surface *pSurf;
  SDL_Rect area = {pUnits_Info_Window->area.x + BLOCKU_W,
                   pUnits_Info_Window->area.y, 0, 0};
  struct widget *pWidget = pUnits_Info_Window;

  if (w < DEFAULT_UNITS_W - BLOCKU_W) {
    w = (pUnits_Info_Window->size.w - pUnits_Info_Window->area.w) + DEFAULT_UNITS_W;
  } else {
    w += (pUnits_Info_Window->size.w - pUnits_Info_Window->area.w) + BLOCKU_W;
  }

  if (h < DEFAULT_UNITS_H) {
    h = (pUnits_Info_Window->size.h - pUnits_Info_Window->area.h) + DEFAULT_UNITS_H;
  } else {
    h += (pUnits_Info_Window->size.h - pUnits_Info_Window->area.h);
  }

  /* clear area under old map window */
  clear_surface(pWidget->dst->surface, &pWidget->size);
  widget_mark_dirty(pWidget);

  pWidget->size.w = w;
  pWidget->size.h = h;

  pWidget->size.x = main_window_width() - w;
  pWidget->size.y = main_window_height() - h;

  FREESURFACE(pWidget->theme);
  pWidget->theme = create_surf(w, h, SDL_SWSURFACE);

  draw_frame(pWidget->theme, 0, 0, pWidget->size.w, pWidget->size.h);

  pSurf = ResizeSurface(current_theme->Block, BLOCKU_W,
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
                                             + (BLOCKU_W - pWidget->size.w) / 2;
  pWidget->size.y = pWidget->dst->surface->h - h + pWidget->area.y + 2;

  /* research button */
  pWidget = pWidget->prev;
  FREESURFACE(pWidget->gfx);
  pWidget->size.x = pWidget->dst->surface->w - w + pUnits_Info_Window->area.x
                                             + (BLOCKU_W - pWidget->size.w) / 2;
  pWidget->size.y = pWidget->dst->surface->h - h + pUnits_Info_Window->area.y +
                    pWidget->size.h + 2;

  /* revolution button */
  pWidget = pWidget->prev;
  FREESURFACE(pWidget->gfx);
  pWidget->size.x = pWidget->dst->surface->w - w + pUnits_Info_Window->area.x
                                             + (BLOCKU_W - pWidget->size.w) / 2;
  pWidget->size.y = pWidget->dst->surface->h - h + pUnits_Info_Window->area.y +
                    pWidget->size.h * 2 + 2;

  /* show/hide unit's window button */
  pWidget = pWidget->prev;
  FREESURFACE(pWidget->gfx);
  pWidget->size.x = pWidget->dst->surface->w - w + pUnits_Info_Window->area.x
                                             + (BLOCKU_W - pWidget->size.w) / 2;
  pWidget->size.y = pUnits_Info_Window->area.y + pUnits_Info_Window->area.h -
                    pWidget->size.h - 2;

  unitinfo_w = w;
  unitinfo_h = h;
}

/**********************************************************************//**
  Resize unitinfo widget.
**************************************************************************/
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
        || (w > DEFAULT_UNITS_W - BLOCKU_W)) && (current_w != w))
      || (((current_h > DEFAULT_UNITS_H) || (h > DEFAULT_UNITS_H)) && (current_h != h))) {
    remake_unitinfo(w, h);
  }

  if (C_S_RUNNING == client_state()) {
    menus_update();
  }
  update_unit_info_label(get_units_in_focus());

  return 0;
}

/**********************************************************************//**
  User interacted with unitinfo width increase button.
**************************************************************************/
static int up_info_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (main_window_width() - ((INFO_WIDTH + 1) * map.xsize + BLOCKU_W +
         (pMiniMap_Window->size.w - pMiniMap_Window->area.w)) >=
         pMiniMap_Window->size.x + pMiniMap_Window->size.w) {
      INFO_WIDTH++;
      resize_unit_info();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with unitinfo width decrease button.
**************************************************************************/
static int down_info_width_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (INFO_WIDTH > INFO_WIDTH_MIN) {
      INFO_WIDTH--;
      resize_unit_info();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with unitinfo height increase button.
**************************************************************************/
static int up_info_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (Main.screen->h - ((INFO_HEIGHT + 1) * map.ysize +
        (pUnits_Info_Window->size.h - pUnits_Info_Window->area.h)) >= adj_size(40)) {
      INFO_HEIGHT++;
      resize_unit_info();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with unitinfo height decrease button.
**************************************************************************/
static int down_info_height_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    if (INFO_HEIGHT > INFO_HEIGHT_MIN) {
      INFO_HEIGHT--;
      resize_unit_info();
    }
    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  Open unitinfo scaling dialog.
**************************************************************************/
static void popup_unitinfo_scale_dialog(void)
{

#ifndef SCALE_UNITINFO
  return;
#endif  /* SCALE_UNITINFO */

  SDL_Surface *pText1, *pText2;
  utf8_str *pstr = NULL;
  struct widget *pWindow = NULL;
  struct widget *pBuf = NULL;
  int window_x = 0, window_y = 0;
  SDL_Rect area;

  if (pScale_UnitInfo_Dlg || !(SDL_Client_Flags & CF_UNITINFO_SHOWN)) {
    return;
  }

  pScale_UnitInfo_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  /* create window */
  pstr = create_utf8_from_char(_("Scale Unit Info"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;
  pWindow = create_window_skeleton(NULL, pstr, 0);
  pWindow->action = move_scale_unitinfo_dlg_callback;
  set_wstate(pWindow, FC_WS_NORMAL);
  add_to_gui_list(ID_WINDOW, pWindow);
  pScale_UnitInfo_Dlg->pEndWidgetList = pWindow;

  area = pWindow->area;

  pstr = create_utf8_from_char(_("Width"), adj_font(12));
  pText1 = create_text_surf_from_utf8(pstr);
  area.w = MAX(area.w, pText1->w + adj_size(30));
  area.h += MAX(adj_size(20), pText1->h + adj_size(4));
  copy_chars_to_utf8_str(pstr, _("Height"));
  pText2 = create_text_surf_from_utf8(pstr);
  area.w = MAX(area.w, pText2->w + adj_size(30));
  area.h += MAX(adj_size(20), pText2->h + adj_size(4));
  FREEUTF8STR(pstr);

  /* ----------------- */
  pBuf = create_themeicon_button(current_theme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_info_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h;

  pBuf = create_themeicon_button(current_theme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_info_width_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);

  /* ------------ */
  pBuf = create_themeicon_button(current_theme->L_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = down_info_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h + adj_size(10);

  pBuf = create_themeicon_button(current_theme->R_ARROW_Icon, pWindow->dst, NULL, 0);
  pBuf->action = up_info_height_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  add_to_gui_list(ID_BUTTON, pBuf);
  area.w = MAX(area.w , pBuf->size.w * 2 + adj_size(20));

  /* ------------ */
  pstr = create_utf8_from_char(_("Exit"), adj_font(12));
  pBuf = create_themeicon_button(current_theme->CANCEL_Icon,
                                 pWindow->dst, pstr, 0);
  pBuf->action = popdown_scale_unitinfo_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pScale_UnitInfo_Dlg->pBeginWidgetList = pBuf;
  add_to_gui_list(ID_BUTTON, pBuf);
  area.h += pBuf->size.h + adj_size(10);
  area.w = MAX(area.w, pBuf->size.w + adj_size(20));

  resize_window(pWindow, NULL, get_theme_color(COLOR_THEME_BACKGROUND),
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  /* ------------ */

  if (Main.event.motion.x + pWindow->size.w > main_window_width()) {
    if (Main.event.motion.x - pWindow->size.w >= 0) {
      window_x = Main.event.motion.x - pWindow->size.w;
    } else {
      window_x = (main_window_width() - pWindow->size.w) / 2;
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
#endif /* SCALE_UNITINFO */

/* ==================================================================== */

/**********************************************************************//**
  User interacted with minimap window.
**************************************************************************/
static int minimap_window_callback(struct widget *pWidget)
{
  int mouse_x, mouse_y;

  switch(Main.event.button.button) {
    case SDL_BUTTON_RIGHT:
      mouse_x = Main.event.motion.x - pMiniMap_Window->dst->dest_rect.x -
                pMiniMap_Window->area.x - overview_start_x;
      mouse_y = Main.event.motion.y - pMiniMap_Window->dst->dest_rect.y -
                pMiniMap_Window->area.y - overview_start_y;
      if ((SDL_Client_Flags & CF_OVERVIEW_SHOWN)
          && (mouse_x >= 0) && (mouse_x < overview_w)
          && (mouse_y >= 0) && (mouse_y < overview_h)) {
        int map_x, map_y;

        overview_to_map_pos(&map_x, &map_y, mouse_x, mouse_y);
        center_tile_mapcanvas(map_pos_to_tile(&(wld.map), map_x, map_y));
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

/**********************************************************************//**
  User interacted with unitinfo window.
**************************************************************************/
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

/**********************************************************************//**
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
                        main_window_width() - pUnits_Info_Window->size.w,
                        main_window_height() - pUnits_Info_Window->size.h);
  } else {
    widget_set_position(pUnit_Window,
                        main_window_width() - BLOCKU_W - current_theme->FR_Right->w,
                        main_window_height() - pUnits_Info_Window->size.h);
  }

  area.x = pUnits_Info_Window->area.x;
  area.y = pUnits_Info_Window->area.y;
  area.w = BLOCKU_W;
  area.h = DEFAULT_UNITS_H;

  /* ID_ECONOMY */
  pWidget = pTax_Button;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w) / 2,
                      area.y + 2);

  /* ID_RESEARCH */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w) / 2,
                      area.y + 2 + pWidget->size.h);

  /* ID_REVOLUTION */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w) / 2,
                      area.y + 2 + (pWidget->size.h * 2));

  /* ID_TOGGLE_UNITS_WINDOW_BUTTON */
  pWidget = pWidget->prev;
  widget_set_area(pWidget, area);
  widget_set_position(pWidget,
                      area.x + (area.w - pWidget->size.w) / 2,
                      area.y + area.h - pWidget->size.h - 2);
}

/**********************************************************************//**
  This Function is used when resize Main.screen.
  We must set new MiniMap start position.
**************************************************************************/
void set_new_minimap_window_pos(void)
{
  struct widget *pWidget;
  SDL_Rect area;

  widget_set_position(pMiniMap_Window, 0,
                      main_window_height() - pMiniMap_Window->size.h);

  area.x = pMiniMap_Window->size.w - current_theme->FR_Right->w - BLOCKM_W;
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

/**********************************************************************//**
  Open unitinfo window.
**************************************************************************/
void popup_unitinfo_window(void)
{
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

  pIcon_theme = ResizeSurface(current_theme->Block, pWindow->area.w,
                              pWindow->area.h, 1);

  blit_entire_src(pIcon_theme, pWindow->theme, pWindow->area.x, pWindow->area.y);
  FREESURFACE(pIcon_theme);

  pWindow->action = unit_info_window_callback;

  add_to_gui_list(ID_UNITS_WINDOW, pWindow);

  pUnits_Info_Window = pWindow;

  pUnitInfo_Dlg->pEndWidgetList = pUnits_Info_Window;
  pUnits_Info_Window->private_data.adv_dlg = pUnitInfo_Dlg;

  /* economy button */
  pWidget = create_icon2(get_tax_surface(O_GOLD), pUnits_Info_Window->dst,
                         WF_FREE_GFX | WF_WIDGET_HAS_INFO_LABEL
                         | WF_RESTORE_BACKGROUND | WF_FREE_THEME);

  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Economy"), "F5");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = economy_callback;
  pWidget->key = SDLK_F5;

  add_to_gui_list(ID_ECONOMY, pWidget);

  pTax_Button = pWidget;

  /* research button */
  pWidget = create_icon2(adj_surf(GET_SURF(client_research_sprite())),
                         pUnits_Info_Window->dst,
                         WF_FREE_GFX | WF_WIDGET_HAS_INFO_LABEL
                         | WF_RESTORE_BACKGROUND | WF_FREE_THEME);
  /* TRANS: Research report action */
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Research"), "F6");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = research_callback;
  pWidget->key = SDLK_F6;

  add_to_gui_list(ID_RESEARCH, pWidget);

  pResearch_Button = pWidget;

  /* revolution button */
  pWidget = create_icon2(adj_surf(GET_SURF(client_government_sprite())),
                         pUnits_Info_Window->dst,
                         WF_FREE_GFX | WF_WIDGET_HAS_INFO_LABEL
                         | WF_RESTORE_BACKGROUND | WF_FREE_THEME);
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Revolution"), "Ctrl+Shift+R");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = revolution_callback;
  pWidget->key = SDLK_r;
  pWidget->mod = KMOD_CTRL | KMOD_SHIFT;

  add_to_gui_list(ID_REVOLUTION, pWidget);

  pRevolution_Button = pWidget;

  /* show/hide unit's window button */

  /* make UNITS Icon */
  pIcon_theme = create_surf(current_theme->MAP_Icon->w,
                            current_theme->MAP_Icon->h, SDL_SWSURFACE);
  alphablit(current_theme->MAP_Icon, NULL, pIcon_theme, NULL, 255);
  alphablit(current_theme->R_ARROW_Icon, NULL, pIcon_theme, NULL, 255);

  pWidget = create_themeicon(pIcon_theme, pUnits_Info_Window->dst,
                             WF_FREE_GFX | WF_FREE_THEME
                             | WF_RESTORE_BACKGROUND
                             | WF_WIDGET_HAS_INFO_LABEL);
  pWidget->info_label = create_utf8_from_char(_("Hide Unit Info Window"),
                                              adj_font(12));

  pWidget->action = toggle_unit_info_window_callback;

  add_to_gui_list(ID_TOGGLE_UNITS_WINDOW_BUTTON, pWidget);

  pUnitInfo_Dlg->pBeginWidgetList = pWidget;

  SDL_Client_Flags |= CF_UNITINFO_SHOWN;

  set_new_unitinfo_window_pos();

  widget_redraw(pUnits_Info_Window);
}

/**********************************************************************//**
  Make unitinfo buttons visible.
**************************************************************************/
void show_unitinfo_window_buttons(void)
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

/**********************************************************************//**
  Make unitinfo buttons hidden.
**************************************************************************/
void hide_unitinfo_window_buttons(void)
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

/**********************************************************************//**
  Make unitinfo buttons disabled.
**************************************************************************/
void disable_unitinfo_window_buttons(void)
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

/**********************************************************************//**
  Close unitinfo window.
**************************************************************************/
void popdown_unitinfo_window(void)
{
  if (pUnitInfo_Dlg) {
    popdown_window_group_dialog(pUnitInfo_Dlg->pBeginWidgetList, pUnitInfo_Dlg->pEndWidgetList);
    FC_FREE(pUnitInfo_Dlg);
    SDL_Client_Flags &= ~CF_UNITINFO_SHOWN;
  }
}

/**********************************************************************//**
  Open minimap area.
**************************************************************************/
void popup_minimap_window(void)
{
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

  pIcon_theme = ResizeSurface(current_theme->Block, BLOCKM_W, pWindow->area.h, 1);
  blit_entire_src(pIcon_theme, pWindow->theme,
    pWindow->area.x + pWindow->area.w - pIcon_theme->w, pWindow->area.y);
  FREESURFACE(pIcon_theme);

  pWindow->action = minimap_window_callback;

  add_to_gui_list(ID_MINI_MAP_WINDOW, pWindow);

  pMiniMap_Window = pWindow;
  pMiniMap_Dlg->pEndWidgetList = pMiniMap_Window;

  /* new turn button */
  pWidget = create_themeicon(current_theme->NEW_TURN_Icon, pMiniMap_Window->dst,
                             WF_WIDGET_HAS_INFO_LABEL
                             | WF_RESTORE_BACKGROUND);
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Turn Done"), _("Shift+Return"));
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = end_turn_callback;
  pWidget->key = SDLK_RETURN;
  pWidget->mod = KMOD_SHIFT;

  add_to_gui_list(ID_NEW_TURN, pWidget);

  pNew_Turn_Button = pWidget;

  /* players button */
  pWidget = create_themeicon(current_theme->PLAYERS_Icon, pMiniMap_Window->dst,
                             WF_WIDGET_HAS_INFO_LABEL
                             | WF_RESTORE_BACKGROUND);
  /* TRANS: Nations report action */
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Nations"), "F3");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = players_action_callback;
  pWidget->key = SDLK_F3;

  add_to_gui_list(ID_PLAYERS, pWidget);

  /* find city button */
  pWidget = create_themeicon(current_theme->FindCity_Icon, pMiniMap_Window->dst,
                             WF_WIDGET_HAS_INFO_LABEL
                             | WF_RESTORE_BACKGROUND);
  fc_snprintf(buf, sizeof(buf), "%s (%s)\n%s\n%s (%s)", _("Cities Report"),
                                "F4", _("or"), _("Find City"), "Ctrl+F");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->info_label->style |= SF_CENTER;
  pWidget->action = cities_action_callback;
  pWidget->key = SDLK_f;
  pWidget->mod = KMOD_CTRL;

  add_to_gui_list(ID_CITIES, pWidget);

  pFind_City_Button = pWidget;

  /* units button */
  pWidget = create_themeicon(current_theme->UNITS2_Icon, pMiniMap_Window->dst,
                             WF_WIDGET_HAS_INFO_LABEL
                             | WF_RESTORE_BACKGROUND);
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Units"), "F2");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = units_action_callback;
  pWidget->key = SDLK_F2;

  add_to_gui_list(ID_UNITS, pWidget);

  /* show/hide log window button */
  pWidget = create_themeicon(current_theme->LOG_Icon, pMiniMap_Window->dst,
                             WF_WIDGET_HAS_INFO_LABEL
                             | WF_RESTORE_BACKGROUND);
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Hide Messages"), "F9");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = toggle_msg_window_callback;
  pWidget->key = SDLK_F9;

  add_to_gui_list(ID_CHATLINE_TOGGLE_LOG_WINDOW_BUTTON, pWidget);

  /* toggle minimap mode button */
  pWidget = create_themeicon(current_theme->BORDERS_Icon, pMiniMap_Window->dst,
                             WF_WIDGET_HAS_INFO_LABEL
                             | WF_RESTORE_BACKGROUND);
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Toggle Mini Map Mode"), "Shift+\\");
  pWidget->info_label = create_utf8_from_char(buf, adj_font(12));
  pWidget->action = toggle_minimap_mode_callback;
  pWidget->key = SDLK_BACKSLASH;
  pWidget->mod = KMOD_SHIFT;

  add_to_gui_list(ID_TOGGLE_MINIMAP_MODE, pWidget);

#ifdef SMALL_SCREEN
  /* options button */
  pOptions_Button = create_themeicon(current_theme->Options_Icon,
                                     pMiniMap_Window->dst,
                                     WF_WIDGET_HAS_INFO_LABEL
                                     | WF_RESTORE_BACKGROUND);
  fc_snprintf(buf, sizeof(buf), "%s (%s)", _("Options"), "Esc");
  pOptions_Button->info_label = create_str16_from_char(buf, adj_font(12));

  pOptions_Button->action = optiondlg_callback;
  pOptions_Button->key = SDLK_ESCAPE;

  add_to_gui_list(ID_CLIENT_OPTIONS, pOptions_Button);
#endif /* SMALL_SCREEN */

  /* show/hide minimap button */

  /* make Map Icon */
  pIcon_theme = create_surf(current_theme->MAP_Icon->w, current_theme->MAP_Icon->h,
                            SDL_SWSURFACE);
  alphablit(current_theme->MAP_Icon, NULL, pIcon_theme, NULL, 255);
  alphablit(current_theme->L_ARROW_Icon, NULL, pIcon_theme, NULL, 255);

  pWidget = create_themeicon(pIcon_theme, pMiniMap_Window->dst,
                             WF_FREE_GFX | WF_FREE_THEME |
                             WF_WIDGET_HAS_INFO_LABEL
                             | WF_RESTORE_BACKGROUND);

  pWidget->info_label = create_utf8_from_char(_("Hide Mini Map"),
                                               adj_font(12));
  pWidget->action = toggle_map_window_callback;

  add_to_gui_list(ID_TOGGLE_MAP_WINDOW_BUTTON, pWidget);

  pMiniMap_Dlg->pBeginWidgetList = pWidget;

  SDL_Client_Flags |= CF_OVERVIEW_SHOWN;

  set_new_minimap_window_pos();

  widget_redraw(pMiniMap_Window);
}

/**********************************************************************//**
  Make minimap window buttons visible.
**************************************************************************/
void show_minimap_window_buttons(void)
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
#endif /* SMALL_SCREEN */

  /* show/hide minimap button */
  pWidget = pWidget->prev;
  clear_wflag(pWidget, WF_HIDDEN);
}

/**********************************************************************//**
  Make minimap window buttons hidden.
**************************************************************************/
void hide_minimap_window_buttons(void)
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
#endif /* SMALL_SCREEN */

  /* show/hide minimap button */
  pWidget = pWidget->prev;
  set_wflag(pWidget, WF_HIDDEN);
}

/**********************************************************************//**
  Redraw minimap window buttons.
**************************************************************************/
void redraw_minimap_window_buttons(void)
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
#endif /* SMALL_SCREEN */

  /* show/hide minimap button */
  pWidget = pWidget->prev;
  widget_redraw(pWidget);
}

/**********************************************************************//**
  Make minimap window buttons disabled.
**************************************************************************/
void disable_minimap_window_buttons(void)
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
#endif /* SMALL_SCREEN */
}

/**********************************************************************//**
  Close minimap window.
**************************************************************************/
void popdown_minimap_window(void)
{
  if (pMiniMap_Dlg) {
    popdown_window_group_dialog(pMiniMap_Dlg->pBeginWidgetList, pMiniMap_Dlg->pEndWidgetList);
    FC_FREE(pMiniMap_Dlg);
    SDL_Client_Flags &= ~CF_OVERVIEW_SHOWN;
  }
}

/**********************************************************************//**
  Create and show game page.
**************************************************************************/
void show_game_page(void)
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
  fc_assert(pIcon_theme != NULL);
  pWidget = create_iconlabel(pIcon_theme, Main.gui, NULL, WF_FREE_THEME);

#ifdef SMALL_SCREEN
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w - adj_size(10),
                      0);
#else  /* SMALL_SCREEN */
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w - adj_size(10),
                      adj_size(10));
#endif /* SMALL_SCREEN */

  add_to_gui_list(ID_COOLING_ICON, pWidget);

  /* warming icon */
  pIcon_theme = adj_surf(GET_SURF(client_warming_sprite()));
  fc_assert(pIcon_theme != NULL);

  pWidget = create_iconlabel(pIcon_theme, Main.gui, NULL, WF_FREE_THEME);

#ifdef SMALL_SCREEN
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w * 2 - adj_size(10),
                      0);
#else  /* SMALL_SCREEN */
  widget_set_position(pWidget,
                      pWidget->dst->surface->w - pWidget->size.w * 2 - adj_size(10),
                      adj_size(10));
#endif /* SMALL_SCREEN */

  add_to_gui_list(ID_WARMING_ICON, pWidget);

  /* create order buttons */
  create_units_order_widgets();

  /* enable options button and order widgets */
  enable_options_button();
  enable_order_buttons();
}

/**********************************************************************//**
  Close game page.
**************************************************************************/
void close_game_page(void)
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

/**********************************************************************//**
  Make minimap window buttons disabled and redraw.
  TODO: Use disable_minimap_window_buttons() for disabling buttons.
**************************************************************************/
static void disable_minimap_widgets(void)
{
  struct widget *pBuf, *pEnd;

  pBuf = get_minimap_window_widget();
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
#endif /* SMALL_SCREEN */

  /* show/hide minimap button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);

  redraw_group(pBuf, pEnd, TRUE);
}

/**********************************************************************//**
  Make unitinfo window buttons disabled and redraw.
  TODO: Use disable_unitinfo_window_buttons() for disabling buttons.
**************************************************************************/
static void disable_unitinfo_widgets(void)
{
  struct widget *pBuf = pUnits_Info_Window->private_data.adv_dlg->pBeginWidgetList;
  struct widget *pEnd = pUnits_Info_Window->private_data.adv_dlg->pEndWidgetList;

  set_group_state(pBuf, pEnd, FC_WS_DISABLED);
  pEnd = pEnd->prev;
  redraw_group(pBuf, pEnd, TRUE);
}

/**********************************************************************//**
  Disable game page widgets.
**************************************************************************/
void disable_main_widgets(void)
{
  if (C_S_RUNNING == client_state()) {
    disable_minimap_widgets();
    disable_unitinfo_widgets();

    disable_options_button();

    disable_order_buttons();
  }
}

/**********************************************************************//**
  Make minimap window buttons enabled and redraw.
**************************************************************************/
static void enable_minimap_widgets(void)
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
#endif /* SMALL_SCREEN */

    /* show/hide minimap button */
    pBuf = pBuf->prev;
    set_wstate(pBuf, FC_WS_NORMAL);

    redraw_group(pBuf, pEnd, TRUE);
  }
}

/**********************************************************************//**
  Make unitinfo window buttons enabled and redraw.
**************************************************************************/
static void enable_unitinfo_widgets(void)
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

/**********************************************************************//**
  Enable game page widgets.
**************************************************************************/
void enable_main_widgets(void)
{
  if (C_S_RUNNING == client_state()) {
    enable_minimap_widgets();
    enable_unitinfo_widgets();

    enable_options_button();

    enable_order_buttons();
  }
}

/**********************************************************************//**
  Get main unitinfo widget.
**************************************************************************/
struct widget *get_unit_info_window_widget(void)
{
  return pUnits_Info_Window;
}

/**********************************************************************//**
  Get main minimap widget.
**************************************************************************/
struct widget *get_minimap_window_widget(void)
{
  return pMiniMap_Window;
}

/**********************************************************************//**
  Get main tax rates widget.
**************************************************************************/
struct widget *get_tax_rates_widget(void)
{
  return pTax_Button;
}

/**********************************************************************//**
  Get main research widget.
**************************************************************************/
struct widget *get_research_widget(void)
{
  return pResearch_Button;
}

/**********************************************************************//**
  Get main revolution widget.
**************************************************************************/
struct widget *get_revolution_widget(void)
{
  return pRevolution_Button;
}

/**********************************************************************//**
  Make Find City button available.
**************************************************************************/
void enable_and_redraw_find_city_button(void)
{
  set_wstate(pFind_City_Button, FC_WS_NORMAL);
  widget_redraw(pFind_City_Button);
  widget_mark_dirty(pFind_City_Button);
}

/**********************************************************************//**
  Make Revolution button available.
**************************************************************************/
void enable_and_redraw_revolution_button(void)
{
  set_wstate(pRevolution_Button, FC_WS_NORMAL);
  widget_redraw(pRevolution_Button);
  widget_mark_dirty(pRevolution_Button);
}

/**********************************************************************//**
  Mouse click handler
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
#endif /* UNDER_CE */
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

/**********************************************************************//**
  Use released mouse button over map.
**************************************************************************/
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
        if (LSHIFT || LALT || LCTRL) {
          if ((ptile = canvas_pos_to_tile((int) button_behavior->event->x,
                                          (int) button_behavior->event->y))) {
            if (LSHIFT) {
              popup_advanced_terrain_dialog(ptile, button_behavior->event->x,
                                            button_behavior->event->y);
            } else {
              if (((pCity = tile_city(ptile)) != NULL)
                  && (city_owner(pCity) == client.conn.playing)) {
                if (LCTRL) {
                  popup_worklist_editor(pCity, NULL);
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
#endif /* UNDER_CE */
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

/**********************************************************************//**
  Toggle map drawing stuff.
**************************************************************************/
bool map_event_handler(SDL_Keysym key)
{
  if (C_S_RUNNING == client_state()) {
    enum direction8 movedir = DIR8_MAGIC_MAX;

    switch (key.scancode) {

    case SDL_SCANCODE_KP_8:
      movedir = DIR8_NORTH;
      break;

    case SDL_SCANCODE_KP_9:
      movedir = DIR8_NORTHEAST;
      break;

    case SDL_SCANCODE_KP_6:
      movedir = DIR8_EAST;
      break;

    case SDL_SCANCODE_KP_3:
      movedir = DIR8_SOUTHEAST;
      break;

    case SDL_SCANCODE_KP_2:
      movedir = DIR8_SOUTH;
      break;

    case SDL_SCANCODE_KP_1:
      movedir = DIR8_SOUTHWEST;
      break;

    case SDL_SCANCODE_KP_4:
      movedir = DIR8_WEST;
      break;

    case SDL_SCANCODE_KP_7:
      movedir = DIR8_NORTHWEST;
      break;

    case SDL_SCANCODE_KP_5:
      key_recall_previous_focus_unit();
      return FALSE;

    default:
      switch (key.sym) {

        /* cancel action */
      case SDLK_ESCAPE:
        key_cancel_action();
        draw_goto_patrol_lines = FALSE;
        update_mouse_cursor(CURSOR_DEFAULT);
        return FALSE;

      case SDLK_UP:
        movedir = DIR8_NORTH;
        break;

      case SDLK_PAGEUP:
        movedir = DIR8_NORTHEAST;
        break;

      case SDLK_RIGHT:
        movedir = DIR8_EAST;
        break;

      case SDLK_PAGEDOWN:
        movedir = DIR8_SOUTHEAST;
        break;

      case SDLK_DOWN:
        movedir = DIR8_SOUTH;
        break;

      case SDLK_END:
        movedir = DIR8_SOUTHWEST;
        break;

      case SDLK_LEFT:
        movedir = DIR8_WEST;
        break;

      case SDLK_HOME:
        movedir = DIR8_NORTHWEST;
        break;

        /* *** map view settings *** */

        /* show city outlines - Ctrl+y */
      case SDLK_y:
        if (LCTRL || RCTRL) {
          key_city_outlines_toggle();
        }
        return FALSE;

        /* show map grid - Ctrl+g */
      case SDLK_g:
        if (LCTRL || RCTRL) {
          key_map_grid_toggle();
        }
        return FALSE;

        /* show national borders - Ctrl+b */
      case SDLK_b:
        if (LCTRL || RCTRL) {
          key_map_borders_toggle();
        }
        return FALSE;

      case SDLK_n:
        /* show native tiles - Ctrl+Shift+n */ 
        if ((LCTRL || RCTRL) && (LSHIFT || RSHIFT)) {
          key_map_native_toggle();
        } else if (LCTRL || RCTRL) {
          /* show city names - Ctrl+n */
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

        /* show city trade routes - Ctrl+d */
      case SDLK_d:
        if (LCTRL || RCTRL) {
          key_city_trade_routes_toggle();
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

        /* show bases - Ctrl+Shift+f */
      case SDLK_f:
        if ((LCTRL || RCTRL) && (LSHIFT || RSHIFT)) {
          request_toggle_bases();
        }
        return FALSE;

        /* show resources - Ctrl+s */
      case SDLK_s:
        if (LCTRL || RCTRL) {
          key_resources_toggle();
        }
        return FALSE;

        /* show huts - Ctrl+h */
      case SDLK_h:
        if (LCTRL || RCTRL) {
          key_huts_toggle();
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

    if (movedir != DIR8_MAGIC_MAX) {
      if (!is_unit_move_blocked) {
        key_unit_move(movedir);
      }
      return FALSE;
    }
  }

  return TRUE;
}

/**********************************************************************//**
  User interacted with the edit button of the new city dialog.
**************************************************************************/
static int newcity_name_edit_callback(struct widget *pEdit)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pNewCity_Dlg->pBeginWidgetList->string_utf8->text == NULL) {
      /* empty input -> restore previous content */
      copy_chars_to_utf8_str(pEdit->string_utf8, pSuggestedCityName);
      widget_redraw(pEdit);
      widget_mark_dirty(pEdit);
      flush_dirty();
    }
  }

  return -1;
}

/**********************************************************************//**
  User interacted with the Ok button of the new city dialog.
**************************************************************************/
static int newcity_ok_callback(struct widget *ok_button)
{
  if (Main.event.type == SDL_KEYDOWN
      || (Main.event.type == SDL_MOUSEBUTTONDOWN
          && Main.event.button.button == SDL_BUTTON_LEFT)) {

    finish_city(ok_button->data.tile, pNewCity_Dlg->pBeginWidgetList->string_utf8->text);

    popdown_window_group_dialog(pNewCity_Dlg->pBeginWidgetList,
                                pNewCity_Dlg->pEndWidgetList);
    FC_FREE(pNewCity_Dlg);

    FC_FREE(pSuggestedCityName);

    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with the Cancel button of the new city dialog.
**************************************************************************/
static int newcity_cancel_callback(struct widget *pCancel_Button)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_window_group_dialog(pNewCity_Dlg->pBeginWidgetList,
                                pNewCity_Dlg->pEndWidgetList);

    cancel_city(pCancel_Button->data.tile);

    FC_FREE(pNewCity_Dlg);

    FC_FREE(pSuggestedCityName);

    flush_dirty();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with the new city dialog.
**************************************************************************/
static int move_new_city_dlg_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pNewCity_Dlg->pBeginWidgetList, pWindow);
  }

  return -1;
}

/* ============================== Native =============================== */

  
/**********************************************************************//**
  Popup a dialog to ask for the name of a new city.  The given string
  should be used as a suggestion.
**************************************************************************/
void popup_newcity_dialog(struct unit *pUnit, const char *pSuggestname)
{
  SDL_Surface *pBackground;
  utf8_str *pstr = NULL;
  struct widget *pLabel = NULL;
  struct widget *pWindow = NULL;
  struct widget *pCancel_Button = NULL;
  struct widget *pOK_Button;
  struct widget *pEdit;
  SDL_Rect area;
  int suggestlen;

  if (pNewCity_Dlg) {
    return;
  }

  suggestlen = strlen(pSuggestname) + 1;
  pSuggestedCityName = fc_calloc(1, suggestlen);
  fc_strlcpy(pSuggestedCityName, pSuggestname, suggestlen);

  pNewCity_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));

  /* create window */
  pstr = create_utf8_from_char(action_id_name_translation(ACTION_FOUND_CITY),
                               adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;
  pWindow = create_window_skeleton(NULL, pstr, 0);
  pWindow->action = move_new_city_dlg_callback;

  area = pWindow->area;

  /* create ok button */
  pOK_Button =
    create_themeicon_button_from_chars(current_theme->Small_OK_Icon, pWindow->dst,
                                       _("OK"), adj_font(10), 0);
  pOK_Button->action = newcity_ok_callback;
  pOK_Button->key = SDLK_RETURN;
  pOK_Button->data.tile = unit_tile(pUnit);

  area.h += pOK_Button->size.h;

  /* create cancel button */
  pCancel_Button =
      create_themeicon_button_from_chars(current_theme->Small_CANCEL_Icon,
                                         pWindow->dst, _("Cancel"), adj_font(10), 0);
  pCancel_Button->action = newcity_cancel_callback;
  pCancel_Button->key = SDLK_ESCAPE; 
  pCancel_Button->data.tile = unit_tile(pUnit);

  /* correct sizes */
  pCancel_Button->size.w += adj_size(5);
  pOK_Button->size.w = pCancel_Button->size.w;

  /* create text label */
  pstr = create_utf8_from_char(_("What should we call our new city?"), adj_font(10));
  pstr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  pstr->fgcol = *get_theme_color(COLOR_THEME_NEWCITYDLG_TEXT);
  pLabel = create_iconlabel(NULL, pWindow->dst, pstr, WF_DRAW_TEXT_LABEL_WITH_SPACE);

  area.h += pLabel->size.h;

  pEdit = create_edit(NULL, pWindow->dst, create_utf8_from_char(pSuggestname, adj_font(12)),
     (pOK_Button->size.w + pCancel_Button->size.w + adj_size(15)), WF_RESTORE_BACKGROUND);
  pEdit->action = newcity_name_edit_callback;

  area.w = MAX(area.w, pEdit->size.w + adj_size(20));
  area.h += pEdit->size.h + adj_size(25);

  /* I make this hack to center label on window */
  if (pLabel->size.w < area.w) {
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
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

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

/**********************************************************************//**
  Close new city dialog.
**************************************************************************/
void popdown_newcity_dialog(void)
{
  if (pNewCity_Dlg) {
    popdown_window_group_dialog(pNewCity_Dlg->pBeginWidgetList,
                                pNewCity_Dlg->pEndWidgetList);
    FC_FREE(pNewCity_Dlg);
    flush_dirty();
  }
}

/**********************************************************************//**
  A turn done button should be provided for the player. This function
  is called to toggle it between active/inactive.
**************************************************************************/
void set_turn_done_button_state(bool state)
{
  if (PAGE_GAME == get_current_client_page()
      && !update_queue_is_switching_page()) {
    if (state) {
      set_wstate(pNew_Turn_Button, FC_WS_NORMAL);
    } else {
      set_wstate(pNew_Turn_Button, FC_WS_DISABLED);
    }
    widget_redraw(pNew_Turn_Button);
    widget_flush(pNew_Turn_Button);
    redraw_unit_info_label(get_units_in_focus());
  }
}

/**********************************************************************//**
  Draw a goto or patrol line at the current mouse position.
**************************************************************************/
void create_line_at_mouse_pos(void)
{
  int pos_x, pos_y;

  SDL_GetMouseState(&pos_x, &pos_y);
  update_line(pos_x, pos_y);
  draw_goto_patrol_lines = TRUE;
}

/**********************************************************************//**
  The Area Selection rectangle. Called by center_tile_mapcanvas() and
  when the mouse pointer moves.
**************************************************************************/
void update_rect_at_mouse_pos(void)
{
  /* PORTME */
}
