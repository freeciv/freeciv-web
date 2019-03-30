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
#include "game.h"

/* client */
#include "client_main.h"

/* gui-sdl2 */
#include "citydlg.h"
#include "cma_fe.h"
#include "colors.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "mapctrl.h"
#include "mapview.h"
#include "repodlgs.h"
#include "sprite.h"
#include "themespec.h"
#include "widget.h"
#include "wldlg.h"

#include "cityrep.h"

static struct ADVANCED_DLG *pCityRep = NULL;

static void real_info_city_report_dialog_update(void);

/* ==================================================================== */

/**********************************************************************//**
  Close city report dialog.
**************************************************************************/
void city_report_dialog_popdown(void)
{
  if (pCityRep) {
    popdown_window_group_dialog(pCityRep->pBeginWidgetList,
                                pCityRep->pEndWidgetList);
    FC_FREE(pCityRep->pScroll);
    FC_FREE(pCityRep);

    enable_and_redraw_find_city_button();
    flush_dirty();
  }
}

/**********************************************************************//**
  User interacted with cityreport window.
**************************************************************************/
static int city_report_windows_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pCityRep->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with city report close button.
**************************************************************************/
static int exit_city_report_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    city_report_dialog_popdown();
  }

  return -1;
}

/**********************************************************************//**
  User interacted with city button on city report.
**************************************************************************/
static int popup_citydlg_from_city_report_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_city_dialog(pWidget->data.city);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with worklist button on city report.
**************************************************************************/
static int popup_worklist_from_city_report_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_worklist_editor(pWidget->data.city, NULL);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with city production button on city report.
**************************************************************************/
static int popup_buy_production_from_city_report_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popup_hurry_production_dialog(pWidget->data.city, NULL);
  }

  return -1;
}

/**********************************************************************//**
  User interacted with cma button on city report.
**************************************************************************/
static int popup_cma_from_city_report_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct city *pCity = game_city_by_number(MAX_ID - pWidget->ID);

    /* state is changed before enter this function */
    if (!get_checkbox_state(pWidget)) {
      cma_release_city(pCity);
      city_report_dialog_update_city(pCity);
    } else {
      popup_city_cma_dialog(pCity);
    }
  }

  return -1;
}

#if 0
/**********************************************************************//**
  User interacted with information report button.
**************************************************************************/
static int info_city_report_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_wstate(pWidget, FC_WS_NORMAL);
    selected_widget = NULL;
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    real_info_city_report_dialog_update();
  }

  return -1;
}
#endif /* 0 */

#define COL	17

/**********************************************************************//**
  Rebuild the city info report.
**************************************************************************/
static void real_info_city_report_dialog_update(void)
{
  SDL_Color bg_color = {255, 255, 255, 136};

  struct widget *pbuf = NULL;
  struct widget *pWindow, *pLast;
  utf8_str *pstr;
  SDL_Surface *pText1, *pText2, *pText3, *pUnits_Icon, *pCMA_Icon, *pText4;
  SDL_Surface *pLogo;
  int togrow, w = 0 , count, ww = 0, hh = 0, name_w = 0, prod_w = 0, H;
  char cbuf[128];
  const char *pName;
  SDL_Rect dst;
  SDL_Rect area;

  if (pCityRep) {
    popdown_window_group_dialog(pCityRep->pBeginWidgetList,
                                pCityRep->pEndWidgetList);
  } else {
    pCityRep = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  }

  fc_snprintf(cbuf, sizeof(cbuf), _("size"));
  pstr = create_utf8_from_char(cbuf, adj_font(10));
  pstr->style |= SF_CENTER;
  pText1 = create_text_surf_from_utf8(pstr);

  fc_snprintf(cbuf, sizeof(cbuf), _("time\nto grow"));
  copy_chars_to_utf8_str(pstr, cbuf);
  pText2 = create_text_surf_from_utf8(pstr);

  fc_snprintf(cbuf, sizeof(cbuf), _("City Name"));
  copy_chars_to_utf8_str(pstr, cbuf);
  pText3 = create_text_surf_from_utf8(pstr);
  name_w = pText3->w + adj_size(6);

  fc_snprintf(cbuf, sizeof(cbuf), _("Production"));
  copy_chars_to_utf8_str(pstr, cbuf);
  pstr->fgcol = *get_theme_color(COLOR_THEME_CITYREP_TEXT);
  pText4 = create_text_surf_from_utf8(pstr);
  prod_w = pText4->w;
  FREEUTF8STR(pstr);

  pUnits_Icon = create_icon_from_theme(current_theme->UNITS_Icon, 0);
  pCMA_Icon = create_icon_from_theme(current_theme->CMA_Icon, 0);

  /* --------------- */
  pstr = create_utf8_from_char(_("Cities Report"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);
  pCityRep->pEndWidgetList = pWindow;
  set_wstate(pWindow, FC_WS_NORMAL);
  pWindow->action = city_report_windows_callback;

  add_to_gui_list(ID_WINDOW, pWindow);

  area = pWindow->area;

  /* ------------------------- */
  /* exit button */
  pbuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pbuf->info_label = create_utf8_from_char(_("Close Dialog"), adj_font(12));
  pbuf->action = exit_city_report_callback;
  set_wstate(pbuf, FC_WS_NORMAL);
  pbuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_BUTTON, pbuf);

/* FIXME: not implemented yet */
#if 0
  /* ------------------------- */
  pbuf = create_themeicon(current_theme->INFO_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pbuf->info_label = create_str16_from_char(_("Information Report"),
                                            adj_font(12));
/*
  pbuf->action = info_city_report_callback;
  set_wstate(pbuf, FC_WS_NORMAL);
*/
  add_to_gui_list(ID_BUTTON, pbuf);
  /* -------- */
  pbuf = create_themeicon(current_theme->Happy_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pbuf->info_label = create_str16_from_char(_("Happiness Report"), adj_font(12));
/*
  pbuf->action = happy_city_report_callback;
  set_wstate(pbuf, FC_WS_NORMAL);
*/
  add_to_gui_list(ID_BUTTON, pbuf);
  /* -------- */
  pbuf = create_themeicon(current_theme->Army_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);

  pbuf->info_label = create_str16_from_char(_("Garrison Report"),
                                            adj_font(12));
/*
  pbuf->action = army_city_dlg_callback;
  set_wstate(pbuf, FC_WS_NORMAL);
*/
  add_to_gui_list(ID_BUTTON, pbuf);
  /* -------- */
  pbuf = create_themeicon(current_theme->Support_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pbuf->info_label = create_str16_from_char(_("Maintenance Report"),
                                            adj_font(12));
/*
  pbuf->action = supported_unit_city_dlg_callback;
  set_wstate(pbuf, FC_WS_NORMAL);
*/
  add_to_gui_list(ID_BUTTON, pbuf);
  /* ------------------------ */
#endif /* 0 */

  pLast = pbuf;
  count = 0;
  city_list_iterate(client.conn.playing->cities, pCity) {
    pstr = create_utf8_from_char(city_name_get(pCity), adj_font(12));
    pstr->style |= TTF_STYLE_BOLD;
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            (WF_RESTORE_BACKGROUND | WF_SELECT_WITHOUT_BAR));

    if (city_unhappy(pCity)) {
      pbuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_TRADE);
    } else {
      if (city_celebrating(pCity)) {
        pbuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_CELEB);
      } else {
        if (city_happy(pCity)) {
          pbuf->string_utf8->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_HAPPY);
        }
      }
    }

    pbuf->action = popup_citydlg_from_city_report_callback;
    set_wstate(pbuf, FC_WS_NORMAL);
    pbuf->data.city = pCity;
    if (count > 13 * COL) {
      set_wflag(pbuf , WF_HIDDEN);
    }
    hh = pbuf->size.h;
    name_w = MAX(pbuf->size.w, name_w);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", city_size_get(pCity));
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pText1->w + adj_size(8);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    pbuf = create_checkbox(pWindow->dst,
                           cma_is_city_under_agent(pCity, NULL), WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    fc_assert(MAX_ID > pCity->id);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);
    set_wstate(pbuf, FC_WS_NORMAL);
    pbuf->action = popup_cma_from_city_report_callback;

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->prod[O_FOOD] - pCity->surplus[O_FOOD]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_game_color(COLOR_OVERVIEW_LAND);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Food->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_FOOD]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_FOOD_SURPLUS);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Food_Corr->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    togrow = city_turns_to_grow(pCity);
    switch (togrow) {
    case 0:
      fc_snprintf(cbuf, sizeof(cbuf), "#");
      break;
    case FC_INFINITY:
      fc_snprintf(cbuf, sizeof(cbuf), "--");
      break;
    default:
      fc_snprintf(cbuf, sizeof(cbuf), "%d", togrow);
      break;
    }

    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    if (togrow < 0) {
      pstr->fgcol.r = 255;
    }
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pText2->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_TRADE]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_TRADE);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Trade->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->waste[O_TRADE]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Trade_Corr->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_GOLD]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_GOLD);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Coin->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->prod[O_SCIENCE]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_SCIENCE);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Colb->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->prod[O_LUXURY]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_LUX);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Luxury->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d",
                pCity->prod[O_SHIELD] + pCity->waste[O_SHIELD]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_PROD);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Shield->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->waste[O_SHIELD]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Shield_Corr->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d",
                pCity->prod[O_SHIELD] + pCity->waste[O_SHIELD] - pCity->surplus[O_SHIELD]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_SUPPORT);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pUnits_Icon->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_SHIELD]);
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_TRADE);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            WF_RESTORE_BACKGROUND);
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    pbuf->size.w = pIcons->pBIG_Shield_Surplus->w + adj_size(6);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);

    /* ----------- */
    if (VUT_UTYPE == pCity->production.kind) {
      struct unit_type *punittype = pCity->production.value.utype;

      pLogo = ResizeSurfaceBox(get_unittype_surface(punittype, direction8_invalid()),
                               adj_size(36), adj_size(24), 1,
                               TRUE, TRUE);
      togrow = utype_build_shield_cost(pCity, punittype);
      pName = utype_name_translation(punittype);
    } else {
      struct impr_type *pimprove = pCity->production.value.building;

      pLogo = ResizeSurfaceBox(get_building_surface(pCity->production.value.building),
                               adj_size(36), adj_size(24), 1,
                               TRUE, TRUE);
      togrow = impr_build_shield_cost(pCity, pimprove);
      pName = improvement_name_translation(pimprove);
    }

    if (!worklist_is_empty(&(pCity->worklist))) {
      dst.x = pLogo->w - pIcons->pWorklist->w;
      dst.y = 0;
      alphablit(pIcons->pWorklist, NULL, pLogo, &dst, 255);
      fc_snprintf(cbuf, sizeof(cbuf), "%s\n(%d/%d)\n%s",
                  pName, pCity->shield_stock, togrow, _("worklist"));
    } else {
      fc_snprintf(cbuf, sizeof(cbuf), "%s\n(%d/%d)%s",
                  pName, pCity->shield_stock, togrow,
                  pCity->shield_stock > togrow ? _("\nfinished"): "" );
    }

    /* info string */
    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;

    togrow = city_production_turns_to_build(pCity, TRUE);
    if (togrow == 999) {
      fc_snprintf(cbuf, sizeof(cbuf), "%s", _("never"));
    } else {
      fc_snprintf(cbuf, sizeof(cbuf), "%d %s",
                  togrow, PL_("turn", "turns", togrow));
    }

    pbuf = create_icon2(pLogo, pWindow->dst,
                        WF_WIDGET_HAS_INFO_LABEL |WF_RESTORE_BACKGROUND
                        | WF_FREE_THEME);
    pbuf->info_label = pstr;
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);
    set_wstate(pbuf, FC_WS_NORMAL);
    pbuf->action = popup_worklist_from_city_report_callback;
    pbuf->data.city = pCity;

    pstr = create_utf8_from_char(cbuf, adj_font(10));
    pstr->style |= SF_CENTER;
    pstr->fgcol = *get_theme_color(COLOR_THEME_CITYREP_TEXT);
    pbuf = create_iconlabel(NULL, pWindow->dst, pstr,
                            (WF_SELECT_WITHOUT_BAR | WF_RESTORE_BACKGROUND));
    if (count > 13 * COL) {
      set_wflag(pbuf, WF_HIDDEN);
    }
    hh = MAX(hh, pbuf->size.h);
    prod_w = MAX(prod_w, pbuf->size.w);
    add_to_gui_list(MAX_ID - pCity->id, pbuf);
    pbuf->data.city = pCity;
    pbuf->action = popup_buy_production_from_city_report_callback;
    if (city_can_buy(pCity)) {
      set_wstate(pbuf, FC_WS_NORMAL);    
    }

    count += COL;
  } city_list_iterate_end;

  H = hh;
  pCityRep->pBeginWidgetList = pbuf;
  /* setup window width */
  area.w = name_w + adj_size(6) + pText1->w + adj_size(8) + pCMA_Icon->w
    + (pIcons->pBIG_Food->w + adj_size(6)) * 10 + pText2->w + adj_size(6)
    + pUnits_Icon->w + adj_size(6) + prod_w + adj_size(170);

  if (count) {
    pCityRep->pEndActiveWidgetList = pLast->prev;
    pCityRep->pBeginActiveWidgetList = pCityRep->pBeginWidgetList;
    if (count > 14 * COL) {
      pCityRep->pActiveWidgetList = pCityRep->pEndActiveWidgetList;
      if (pCityRep->pScroll) {
	pCityRep->pScroll->count = count;
      }
      ww = create_vertical_scrollbar(pCityRep, COL, 14, TRUE, TRUE);
      area.w += ww;
      area.h = 14 * (hh + adj_size(2));
    } else {
      area.h = (count / COL) * (hh + adj_size(2));
    }
  }

  area.h += pText2->h + adj_size(6);
  area.w += adj_size(2);

  pLogo = theme_get_background(theme, BACKGROUND_CITYREP);
  resize_window(pWindow, pLogo,	NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  FREESURFACE(pLogo);

#if 0
  pLogo = SDL_DisplayFormat(pWindow->theme);
  FREESURFACE(pWindow->theme);
  pWindow->theme = pLogo;
  pLogo = NULL;
#endif

  area = pWindow->area;

  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  /* exit button */
  pbuf = pWindow->prev;
  pbuf->size.x = area.x + area.w - pbuf->size.w - 1;
  pbuf->size.y = pWindow->size.y + adj_size(2);

/* FIXME: not implemented yet */
#if 0
  /* info button */
  pbuf = pbuf->prev;
  pbuf->size.x = area.x + area.w - pbuf->size.w - adj_size(5);
  pbuf->size.y = area.y + area.h - pbuf->size.h - adj_size(5);

  /* happy button */
  pbuf = pbuf->prev;
  pbuf->size.x = pbuf->next->size.x - adj_size(5) - pbuf->size.w;
  pbuf->size.y = pbuf->next->size.y;

  /* army button */
  pbuf = pbuf->prev;
  pbuf->size.x = pbuf->next->size.x - adj_size(5) - pbuf->size.w;
  pbuf->size.y = pbuf->next->size.y;

  /* supported button */
  pbuf = pbuf->prev;
  pbuf->size.x = pbuf->next->size.x - adj_size(5) - pbuf->size.w;
  pbuf->size.y = pbuf->next->size.y;
#endif /* 0 */

  /* cities background and labels */
  dst.x = area.x + adj_size(2);
  dst.y = area.y + 1;
  dst.w = (name_w + adj_size(6)) + (pText1->w + adj_size(8)) + adj_size(5);
  dst.h = area.h - adj_size(2);
  fill_rect_alpha(pWindow->theme, &dst, &bg_color);

  create_frame(pWindow->theme,
               dst.x , dst.y, dst.w, dst.h - 1,
               get_theme_color(COLOR_THEME_CITYREP_FRAME));

  dst.y += (pText2->h - pText3->h) / 2;
  dst.x += ((name_w + adj_size(6)) - pText3->w) / 2;
  alphablit(pText3, NULL, pWindow->theme, &dst, 255);
  FREESURFACE(pText3);

  /* city size background and label */
  dst.x = area.x + adj_size(5) + name_w + adj_size(5 + 4);
  alphablit(pText1, NULL, pWindow->theme, &dst, 255);
  ww = pText1->w;
  FREESURFACE(pText1);

  /* cma icon */
  dst.x += (ww + adj_size(9));
  dst.y = area.y + 1 + (pText2->h - pCMA_Icon->h) / 2;
  alphablit(pCMA_Icon, NULL, pWindow->theme, &dst, 255);
  ww = pCMA_Icon->w;
  FREESURFACE(pCMA_Icon);

  /* -------------- */
  /* populations food unkeep background and label */
  dst.x += (ww + 1);
  dst.y = area.y + 1;
  w = dst.x + adj_size(2);
  dst.w = (pIcons->pBIG_Food->w + adj_size(6)) + adj_size(10)
    + (pIcons->pBIG_Food_Surplus->w + adj_size(6)) + adj_size(10)
    + pText2->w + adj_size(6 + 2);
  dst.h = area.h - adj_size(2);
  fill_rect_alpha(pWindow->theme, &dst, get_theme_color(COLOR_THEME_CITYREP_FOODSTOCK));

  create_frame(pWindow->theme,
               dst.x, dst.y, dst.w, dst.h - 1,
               get_theme_color(COLOR_THEME_CITYREP_FRAME));

  dst.y = area.y + 1 + (pText2->h - pIcons->pBIG_Food->h) / 2;
  dst.x += adj_size(5);
  alphablit(pIcons->pBIG_Food, NULL, pWindow->theme, &dst, 255);

  /* food surplus Icon */
  w += (pIcons->pBIG_Food->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  alphablit(pIcons->pBIG_Food_Surplus, NULL, pWindow->theme, &dst, 255);

  /* to grow label */
  w += (pIcons->pBIG_Food_Surplus->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  dst.y = area.y + 1;
  alphablit(pText2, NULL, pWindow->theme, &dst, 255);
  hh = pText2->h;
  ww = pText2->w;
  FREESURFACE(pText2);
  /* -------------- */

  /* trade, corruptions, gold, science, luxury income background and label */
  dst.x = w + (ww + adj_size(8));
  dst.y = area.y + 1;
  w = dst.x + adj_size(2);
  dst.w = (pIcons->pBIG_Trade->w + adj_size(6)) + adj_size(10) +
	  (pIcons->pBIG_Trade_Corr->w + adj_size(6)) + adj_size(10) +
	  (pIcons->pBIG_Coin->w + adj_size(6)) + adj_size(10) +
	  (pIcons->pBIG_Colb->w + adj_size(6)) + adj_size(10) +
	  (pIcons->pBIG_Luxury->w + adj_size(6)) + adj_size(4);
  dst.h = area.h - adj_size(2);

  fill_rect_alpha(pWindow->theme, &dst, get_theme_color(COLOR_THEME_CITYREP_TRADE));

  create_frame(pWindow->theme,
               dst.x , dst.y, dst.w, dst.h - 1,
               get_theme_color(COLOR_THEME_CITYREP_FRAME));

  dst.y = area.y + 1 + (hh - pIcons->pBIG_Trade->h) / 2;
  dst.x += adj_size(5);
  alphablit(pIcons->pBIG_Trade, NULL, pWindow->theme, &dst, 255);

  w += (pIcons->pBIG_Trade->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  alphablit(pIcons->pBIG_Trade_Corr, NULL, pWindow->theme, &dst, 255);

  w += (pIcons->pBIG_Food_Corr->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  alphablit(pIcons->pBIG_Coin, NULL, pWindow->theme, &dst, 255);

  w += (pIcons->pBIG_Coin->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  alphablit(pIcons->pBIG_Colb, NULL, pWindow->theme, &dst, 255);

  w += (pIcons->pBIG_Colb->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  alphablit(pIcons->pBIG_Luxury, NULL, pWindow->theme, &dst, 255);
  /* --------------------- */

  /* total productions, waste, support, shields surplus background and label */
  w += (pIcons->pBIG_Luxury->w + adj_size(6)) + adj_size(4);
  dst.x = w;
  w += adj_size(2);
  dst.y = area.y + 1;
  dst.w = (pIcons->pBIG_Shield->w + adj_size(6)) + adj_size(10) +
	  (pIcons->pBIG_Shield_Corr->w + adj_size(6)) + adj_size(10) +
	  (pUnits_Icon->w + adj_size(6)) + adj_size(10) +
	  (pIcons->pBIG_Shield_Surplus->w + adj_size(6)) + adj_size(4);
  dst.h = area.h - adj_size(2);

  fill_rect_alpha(pWindow->theme, &dst, get_theme_color(COLOR_THEME_CITYREP_PROD));

  create_frame(pWindow->theme,
               dst.x , dst.y, dst.w, dst.h - 1,
               get_theme_color(COLOR_THEME_CITYREP_FRAME));

  dst.y = area.y + 1 + (hh - pIcons->pBIG_Shield->h) / 2;
  dst.x += adj_size(5);
  alphablit(pIcons->pBIG_Shield, NULL, pWindow->theme, &dst, 255);

  w += (pIcons->pBIG_Shield->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  alphablit(pIcons->pBIG_Shield_Corr, NULL, pWindow->theme, &dst, 255);

  w += (pIcons->pBIG_Shield_Corr->w + adj_size(6)) + adj_size(10);
  dst.x = w + adj_size(3);
  dst.y = area.y + 1 + (hh - pUnits_Icon->h) / 2;
  alphablit(pUnits_Icon, NULL, pWindow->theme, &dst, 255);

  w += (pUnits_Icon->w + adj_size(6)) + adj_size(10);
  FREESURFACE(pUnits_Icon);
  dst.x = w + adj_size(3);
  dst.y = area.y + 1 + (hh - pIcons->pBIG_Shield_Surplus->h) / 2;
  alphablit(pIcons->pBIG_Shield_Surplus, NULL, pWindow->theme, &dst, 255);
  /* ------------------------------- */

  w += (pIcons->pBIG_Shield_Surplus->w + adj_size(6)) + adj_size(10);
  dst.x = w;
  w += adj_size(2);
  dst.y = area.y + 1;
  dst.w = adj_size(36) + adj_size(5) + prod_w;
  dst.h = hh + adj_size(2);

  fill_rect_alpha(pWindow->theme, &dst, get_theme_color(COLOR_THEME_CITYREP_PROD));

  create_frame(pWindow->theme,
               dst.x , dst.y, dst.w, dst.h - 1,
               get_theme_color(COLOR_THEME_CITYREP_FRAME));

  dst.y = area.y + 1 + (hh - pText4->h) / 2;
  dst.x += (dst.w - pText4->w) / 2;
  alphablit(pText4, NULL, pWindow->theme, &dst, 255);
  FREESURFACE(pText4);

  if (count) {
    int start_x = area.x + adj_size(5);
    int start_y = area.y + hh + adj_size(3);

    H += adj_size(2);
    pbuf = pbuf->prev;
    while (TRUE) {

      /* city name */
      pbuf->size.x = start_x;
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;
      pbuf->size.w = name_w;

      /* city size */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(5);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* cma */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(6);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* food cons. */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(6);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* food surplus */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* time to grow */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* trade */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(5);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* trade corruptions */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* net gold income */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* science income */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* luxuries income */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* total production */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(6);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* waste */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* units support */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* producrion surplus */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* currently build */
      /* icon */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(10);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;

      /* label */
      pbuf = pbuf->prev;
      pbuf->size.x = pbuf->next->size.x + pbuf->next->size.w + adj_size(5);
      pbuf->size.y = start_y + (H - pbuf->size.h) / 2;
      pbuf->size.w = prod_w;

      start_y += H;
      if (pbuf == pCityRep->pBeginActiveWidgetList) {
        break;
      }
      pbuf = pbuf->prev;
    }

    if (pCityRep->pScroll) {
      setup_vertical_scrollbar_area(pCityRep->pScroll,
                                    area.x + area.w, area.y,
                                    area.h, TRUE);
    }

  }
  /* ----------------------------------- */
  redraw_group(pCityRep->pBeginWidgetList, pWindow, 0);
  widget_mark_dirty(pWindow);
  flush_dirty();
}

/**********************************************************************//**
  Update city information in city report.
**************************************************************************/
static struct widget *real_city_report_dialog_update_city(struct widget *pWidget,
                                                          struct city *pCity)
{
  char cbuf[64];
  const char *pName;
  int togrow;
  SDL_Surface *pLogo;
  SDL_Rect dst;

  /* city name status */
  if (city_unhappy(pCity)) {
    pWidget->string_utf8->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_TRADE);
  } else {
    if (city_celebrating(pCity)) {
      pWidget->string_utf8->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_CELEB);
    } else {
      if (city_happy(pCity)) {
	pWidget->string_utf8->fgcol = *get_theme_color(COLOR_THEME_CITYDLG_HAPPY);
      }
    }
  }

  /* city size */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", city_size_get(pCity));
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* cma check box */
  pWidget = pWidget->prev;
  if (cma_is_city_under_agent(pCity, NULL) != get_checkbox_state(pWidget)) {
    toggle_checkbox(pWidget);
  }

  /* food consumptions */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->prod[O_FOOD] - pCity->surplus[O_FOOD]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* food surplus */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_FOOD]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* time to grow */
  pWidget = pWidget->prev;
  togrow = city_turns_to_grow(pCity);
  switch (togrow) {
    case 0:
      fc_snprintf(cbuf, sizeof(cbuf), "#");
    break;
    case FC_INFINITY:
      fc_snprintf(cbuf, sizeof(cbuf), "--");
    break;
    default:
      fc_snprintf(cbuf, sizeof(cbuf), "%d", togrow);
    break;
  }
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  if (togrow < 0) {
    pWidget->string_utf8->fgcol.r = 255;
  } else {
    pWidget->string_utf8->fgcol.r = 0;
  }

  /* trade production */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_TRADE]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* corruptions */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->waste[O_TRADE]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* gold surplus */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_GOLD]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* science income */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->prod[O_SCIENCE]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* lugury income */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->prod[O_LUXURY]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* total production */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d",
              pCity->prod[O_SHIELD] + pCity->waste[O_SHIELD]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* waste */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->waste[O_SHIELD]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* units support */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->prod[O_SHIELD] +
              pCity->waste[O_SHIELD] - pCity->surplus[O_SHIELD]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* production income */
  pWidget = pWidget->prev;
  fc_snprintf(cbuf, sizeof(cbuf), "%d", pCity->surplus[O_SHIELD]);
  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  /* change production */
  if (VUT_UTYPE == pCity->production.kind) {
    struct unit_type *punittype = pCity->production.value.utype;

    pLogo = ResizeSurface(get_unittype_surface(punittype, direction8_invalid()),
                          adj_size(36), adj_size(24), 1);
    togrow = utype_build_shield_cost(pCity, punittype);
    pName = utype_name_translation(punittype);
  } else {
    struct impr_type *pimprove = pCity->production.value.building;

    pLogo = ResizeSurface(get_building_surface(pCity->production.value.building),
                          adj_size(36), adj_size(24), 1);
    togrow = impr_build_shield_cost(pCity, pimprove);
    pName = improvement_name_translation(pimprove);
  }

  if (!worklist_is_empty(&(pCity->worklist))) {
    dst.x = pLogo->w - pIcons->pWorklist->w;
    dst.y = 0;
    alphablit(pIcons->pWorklist, NULL, pLogo, &dst, 255);
    fc_snprintf(cbuf, sizeof(cbuf), "%s\n(%d/%d)\n%s",
                pName, pCity->shield_stock, togrow, _("worklist"));
  } else {
    fc_snprintf(cbuf, sizeof(cbuf), "%s\n(%d/%d)",
      		pName, pCity->shield_stock, togrow);
  }

  pWidget = pWidget->prev;
  copy_chars_to_utf8_str(pWidget->info_label, cbuf);
  FREESURFACE(pWidget->theme);
  pWidget->theme = pLogo;

  /* hurry productions */
  pWidget = pWidget->prev;
  togrow = city_production_turns_to_build(pCity, TRUE);
  if (togrow == 999) {
    fc_snprintf(cbuf, sizeof(cbuf), "%s", _("never"));
  } else {
    fc_snprintf(cbuf, sizeof(cbuf), "%d %s",
                togrow, PL_("turn", "turns", togrow));
  }

  copy_chars_to_utf8_str(pWidget->string_utf8, cbuf);

  return pWidget->prev;
}

/* ======================================================================== */

/**********************************************************************//**
  Check if city report is open.
**************************************************************************/
bool is_city_report_open(void)
{
  return (pCityRep != NULL);
}

/**********************************************************************//**
  Pop up or brings forward the city report dialog.  It may or may not
  be modal.
**************************************************************************/
void city_report_dialog_popup(bool make_modal)
{
  if (!pCityRep) {
    real_info_city_report_dialog_update();
  }
}

/**********************************************************************//**
  Update (refresh) the entire city report dialog.
**************************************************************************/
void real_city_report_dialog_update(void *unused)
{
  if (pCityRep) {
    struct widget *pWidget;
    int count;

    /* find if the lists are identical (if not then rebuild all) */
    pWidget = pCityRep->pEndActiveWidgetList;/* name of first city */
    city_list_iterate(client.conn.playing->cities, pCity) {
      if (pCity->id == MAX_ID - pWidget->ID) {
        count = COL;

        while (count) {
          count--;
          pWidget = pWidget->prev;
        }
      } else {
        real_info_city_report_dialog_update();
        return;
      }
    } city_list_iterate_end;

    /* check it there are some city widgets left on list */
    if (pWidget && pWidget->next != pCityRep->pBeginActiveWidgetList) {
      real_info_city_report_dialog_update();
      return;
    }

    /* update widget city list (widget list is the same that city list) */
    pWidget = pCityRep->pEndActiveWidgetList;
    city_list_iterate(client.conn.playing->cities, pCity) {
      pWidget = real_city_report_dialog_update_city(pWidget, pCity);
    } city_list_iterate_end;

    /* -------------------------------------- */
    redraw_group(pCityRep->pBeginWidgetList, pCityRep->pEndWidgetList, 0);
    widget_mark_dirty(pCityRep->pEndWidgetList);

    flush_dirty();
  }
}

/**********************************************************************//**
  Update the city report dialog for a single city.
**************************************************************************/
void real_city_report_update_city(struct city *pCity)
{
  if (pCityRep && pCity) {
    struct widget *pBuf = pCityRep->pEndActiveWidgetList;

    while (pCity->id != MAX_ID - pBuf->ID
           && pBuf != pCityRep->pBeginActiveWidgetList) {
      pBuf = pBuf->prev;
    }

    if (pBuf == pCityRep->pBeginActiveWidgetList) {
      real_info_city_report_dialog_update();
      return;
    }
    real_city_report_dialog_update_city(pBuf, pCity);

    /* -------------------------------------- */
    redraw_group(pCityRep->pBeginWidgetList, pCityRep->pEndWidgetList, 0);
    widget_mark_dirty(pCityRep->pEndWidgetList);

    flush_dirty();
  }
}

/**********************************************************************//**
  After a selection rectangle is defined, make the cities that
  are hilited on the canvas exclusively hilited in the
  City List window.
**************************************************************************/
void hilite_cities_from_canvas(void)
{
  log_debug("hilite_cities_from_canvas : PORT ME");
}

/**********************************************************************//**
  Toggle a city's hilited status.
**************************************************************************/
void toggle_city_hilite(struct city *pCity, bool on_off)
{
  log_debug("toggle_city_hilite : PORT ME");
}
