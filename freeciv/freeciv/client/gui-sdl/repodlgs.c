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
                          repodlgs.c  -  description
                             -------------------
    begin                : Nov 15 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "government.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "text.h"

/* gui-sdl */
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_main.h"
#include "gui_tilespec.h"
#include "helpdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "sprite.h"
#include "themespec.h"
#include "widget.h"

#include "repodlgs.h"


/* ===================================================================== */
/* ======================== Active Units Report ======================== */
/* ===================================================================== */
static struct ADVANCED_DLG *pUnitsDlg = NULL;
static struct SMALL_DLG *pUnits_Upg_Dlg = NULL;

struct units_entry {
  int active_count;
  int upkeep[O_LAST];
  int building_count;
  int soonest_completions;
};


static void get_units_report_data(struct units_entry *entries, 
                                  struct units_entry *total)
{
  int time_to_build;

  memset(entries, '\0', U_LAST * sizeof(struct units_entry));
  memset(total, '\0', sizeof(struct units_entry));
  for(time_to_build = 0; time_to_build < U_LAST; time_to_build++) {
    entries[time_to_build].soonest_completions = FC_INFINITY;
  }

  unit_list_iterate(client.conn.playing->units, punit) {
    Unit_type_id uti = utype_index(unit_type(punit));
    (entries[uti].active_count)++;
    (total->active_count)++;
    if (punit->homecity) {
      output_type_iterate(o) {
        entries[uti].upkeep[o] += punit->upkeep[o];
      } output_type_iterate_end;
    }
  } unit_list_iterate_end;

  city_list_iterate(client.conn.playing->cities, pCity) {
    if (VUT_UTYPE == pCity->production.kind) {
      struct unit_type *pUnitType = pCity->production.value.utype;
      (total->building_count)++;
      entries[utype_index(pUnitType)].soonest_completions =
	MIN(entries[utype_index(pUnitType)].soonest_completions,
	    city_production_turns_to_build(pCity, TRUE));
    }
  } city_list_iterate_end;
}


static int units_dialog_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pUnitsDlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

/* --------------------------------------------------------------- */
static int ok_upgrade_unit_window_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int ut1 = MAX_ID - pWidget->ID;
    
    /* popdown sell dlg */
    popdown_window_group_dialog(pUnits_Upg_Dlg->pBeginWidgetList,
                                pUnits_Upg_Dlg->pEndWidgetList);
    FC_FREE(pUnits_Upg_Dlg);
     
    dsend_packet_unit_type_upgrade(&client.conn, ut1);
  }    
  return -1;
}

static int upgrade_unit_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pUnits_Upg_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int cancel_upgrade_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pUnits_Upg_Dlg) {
      popdown_window_group_dialog(pUnits_Upg_Dlg->pBeginWidgetList,
                                pUnits_Upg_Dlg->pEndWidgetList);
      FC_FREE(pUnits_Upg_Dlg);
      flush_dirty();
    }
  }
  return -1;
}

static int popup_upgrade_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit_type *ut1;
    struct unit_type *ut2;
    int value;
    char cBuf[128];
    struct widget *pBuf = NULL, *pWindow;
    SDL_String16 *pStr;
    SDL_Surface *pText;
    SDL_Rect dst;
    SDL_Rect area;

    ut1 = utype_by_number(MAX_ID - pWidget->ID);

    if (pUnits_Upg_Dlg) {
      return 1;
    }
    CHECK_UNIT_TYPE(ut1);
    
    set_wstate(pWidget, FC_WS_NORMAL);
    pSellected_Widget = NULL;
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    
    pUnits_Upg_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));
  
    ut2 = can_upgrade_unittype(client.conn.playing, ut1);
    value = unit_upgrade_price(client.conn.playing, ut1, ut2);
    
    my_snprintf(cBuf, sizeof(cBuf),
          _("Upgrade as many %s to %s as possible for %d gold each?\n"
            "Treasury contains %d gold."),
          utype_name_translation(ut1),
          utype_name_translation(ut2),
          value, client.conn.playing->economic.gold);
   
    
    pStr = create_str16_from_char(_("Upgrade Obsolete Units"), adj_font(12));
    pStr->style |= TTF_STYLE_BOLD;
  
    pWindow = create_window_skeleton(NULL, pStr, 0);
  
    pWindow->action = upgrade_unit_window_callback;
    set_wstate(pWindow, FC_WS_NORMAL);
  
    add_to_gui_list(ID_WINDOW, pWindow);
    
    pUnits_Upg_Dlg->pEndWidgetList = pWindow;

    area = pWindow->area;

    /* ============================================================= */
    
    /* create text label */
    pStr = create_str16_from_char(cBuf, adj_font(10));
    pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_UNITUPGRADE_TEXT);
    
    pText = create_text_surf_from_str16(pStr);
    FREESTRING16(pStr);
    
    area.h += (pText->h + adj_size(10));
    area.w = MAX(area.w, pText->w + adj_size(20));
    
    /* cancel button */
    pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon,
                              pWindow->dst, _("No"), adj_font(12), 0);
  
    pBuf->action = cancel_upgrade_unit_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    area.h += (pBuf->size.h + adj_size(20));
    
    add_to_gui_list(ID_BUTTON, pBuf);
    
    if (value <= client.conn.playing->economic.gold) {
      pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
                                                _("Yes"), adj_font(12), 0);
          
      pBuf->action = ok_upgrade_unit_window_callback;
      set_wstate(pBuf, FC_WS_NORMAL);
          
      add_to_gui_list(pWidget->ID, pBuf);
      pBuf->size.w = MAX(pBuf->size.w, pBuf->next->size.w);
      pBuf->next->size.w = pBuf->size.w;
      area.w = MAX(area.w, adj_size(30) + pBuf->size.w * 2);
    } else {
      area.w = MAX(area.w, pBuf->size.w + adj_size(20));
    }
    /* ============================================ */
    
    pUnits_Upg_Dlg->pBeginWidgetList = pBuf;

    resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                  (pWindow->size.w - pWindow->area.w) + area.w,
                  (pWindow->size.h - pWindow->area.h) + area.h);

    widget_set_position(pWindow, 
                        pUnitsDlg->pEndWidgetList->size.x +
                          (pUnitsDlg->pEndWidgetList->size.w - pWindow->size.w) / 2,
                        pUnitsDlg->pEndWidgetList->size.y +
                          (pUnitsDlg->pEndWidgetList->size.h - pWindow->size.h) / 2);
    
    /* setup rest of widgets */
    /* label */
    dst.x = area.x + (area.w - pText->w) / 2;
    dst.y = area.y + adj_size(10);
    alphablit(pText, NULL, pWindow->theme, &dst);
    FREESURFACE(pText);
     
    /* cancel button */
    pBuf = pWindow->prev;
    pBuf->size.y = area.y + area.h - pBuf->size.h - adj_size(10);
    
    if (value <= client.conn.playing->economic.gold) {
      /* sell button */
      pBuf = pBuf->prev;
      pBuf->size.x = area.x + (area.w - (2 * pBuf->size.w + adj_size(10))) / 2;
      pBuf->size.y = pBuf->next->size.y;
      
      /* cancel button */
      pBuf->next->size.x = pBuf->size.x + pBuf->size.w + adj_size(10);
    } else {
      /* x position of cancel button */
      pBuf->size.x = area.x + area.w - pBuf->size.w - adj_size(10);
    }
    
    
    /* ================================================== */
    /* redraw */
    redraw_group(pUnits_Upg_Dlg->pBeginWidgetList, pWindow, 0);
      
    widget_mark_dirty(pWindow);
    flush_dirty();
  }
  return -1;
}

static int exit_units_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pUnitsDlg) {
      if (pUnits_Upg_Dlg) {
         del_group_of_widgets_from_gui_list(pUnits_Upg_Dlg->pBeginWidgetList,
                                pUnits_Upg_Dlg->pEndWidgetList);
         FC_FREE(pUnits_Upg_Dlg); 
      }
      popdown_window_group_dialog(pUnitsDlg->pBeginWidgetList,
                                        pUnitsDlg->pEndWidgetList);
      FC_FREE(pUnitsDlg->pScroll);
      FC_FREE(pUnitsDlg);
      flush_dirty();
    }
  }
  return -1;
}


/**************************************************************************
  rebuild the units report.
**************************************************************************/
static void real_activeunits_report_dialog_update(struct units_entry *units, 
  						struct units_entry *total)
{
  SDL_Color bg_color = {255, 255, 255, 136};
  
  struct widget *pBuf = NULL;
  struct widget *pWindow , *pLast;
  SDL_String16 *pStr;
  SDL_Surface *pText1, *pText2, *pText3 , *pText4, *pText5, *pLogo;
  int w = 0 , count, ww, hh = 0, name_w = 0;
  char cBuf[64];
  SDL_Rect dst;
  bool upgrade = FALSE;
  struct unit_type *pUnit;
  SDL_Rect area;
    
  if(pUnitsDlg) {
    popdown_window_group_dialog(pUnitsDlg->pBeginWidgetList,
			      			pUnitsDlg->pEndWidgetList);
  } else {
    pUnitsDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));  
  }
  
  my_snprintf(cBuf, sizeof(cBuf), _("active"));
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= SF_CENTER;
  pText1 = create_text_surf_from_str16(pStr);
    
  my_snprintf(cBuf, sizeof(cBuf), _("under\nconstruction"));
  copy_chars_to_string16(pStr, cBuf);
  pText2 = create_text_surf_from_str16(pStr);
    
  my_snprintf(cBuf, sizeof(cBuf), _("soonest\ncompletion"));
  copy_chars_to_string16(pStr, cBuf);
  pText5 = create_text_surf_from_str16(pStr);
    
  my_snprintf(cBuf, sizeof(cBuf), _("Total"));
  copy_chars_to_string16(pStr, cBuf);
  pText3 = create_text_surf_from_str16(pStr);
    
  my_snprintf(cBuf, sizeof(cBuf), _("Units"));
  copy_chars_to_string16(pStr, cBuf);
  pText4 = create_text_surf_from_str16(pStr);
  name_w = pText4->w;
  FREESTRING16(pStr);
  
  /* --------------- */
  pStr = create_str16_from_char(_("Units Report"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);
  set_wstate(pWindow, FC_WS_NORMAL);
  pWindow->action = units_dialog_callback;
  
  add_to_gui_list(ID_UNITS_DIALOG_WINDOW, pWindow);

  pUnitsDlg->pEndWidgetList = pWindow;

  area = pWindow->area;
  
  /* ------------------------- */
  /* exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  pBuf->action = exit_units_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_BUTTON, pBuf);
  /* ------------------------- */
  /* totals */
  my_snprintf(cBuf, sizeof(cBuf), "%d", total->active_count);
	
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
	
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
					WF_RESTORE_BACKGROUND);
 	
  area.h += pBuf->size.h;
  pBuf->size.w = pText1->w + adj_size(6);
  add_to_gui_list(ID_LABEL, pBuf);
  /* ---------------------------------------------- */
  my_snprintf(cBuf, sizeof(cBuf), "%d", total->upkeep[O_SHIELD]);
	
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
	
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
	
  pBuf->size.w = pText1->w;
  add_to_gui_list(ID_LABEL, pBuf);
  /* ---------------------------------------------- */	
  my_snprintf(cBuf, sizeof(cBuf), "%d", total->upkeep[O_FOOD]);
	
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
	
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
	
  pBuf->size.w = pText1->w;
  add_to_gui_list(ID_LABEL, pBuf);
  /* ---------------------------------------------- */	
  my_snprintf(cBuf, sizeof(cBuf), "%d", total->upkeep[O_GOLD]);
	
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
	
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
	
  pBuf->size.w = pText1->w;
  add_to_gui_list(ID_LABEL, pBuf);
  /* ---------------------------------------------- */	
  my_snprintf(cBuf, sizeof(cBuf), "%d", total->building_count);
	
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);

  pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
					WF_RESTORE_BACKGROUND);
	
  pBuf->size.w = pText2->w + adj_size(6);
  add_to_gui_list(ID_LABEL, pBuf);
  
  /* ------------------------- */
  pLast = pBuf;
  count = 0; 
  unit_type_iterate(i) {
    if ((units[utype_index(i)].active_count > 0)
     || (units[utype_index(i)].building_count > 0)) {
      upgrade = (can_upgrade_unittype(client.conn.playing, i) != NULL);
      pUnit = i;
	
      /* unit type icon */
      pBuf = create_iconlabel(adj_surf(get_unittype_surface(i)), pWindow->dst, NULL,
			WF_RESTORE_BACKGROUND | WF_FREE_THEME);
      if(count > adj_size(72)) {
	set_wflag(pBuf, WF_HIDDEN);
      }
      hh = pBuf->size.h;
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);
      
      /* unit type name */
      pStr = create_str16_from_char(utype_name_translation(i), adj_font(12));
      pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
      pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
			(WF_RESTORE_BACKGROUND|WF_SELLECT_WITHOUT_BAR));
      if(upgrade) {
	pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_UNITUPGRADE_TEXT);
	pBuf->action = popup_upgrade_unit_callback;
	set_wstate(pBuf, FC_WS_NORMAL);
      } else {
        pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_UNITSREP_TEXT);
      }
      pBuf->string16->style &= ~SF_CENTER;
      if(count > adj_size(72)) {
	set_wflag(pBuf , WF_HIDDEN);
      }
      hh = MAX(hh, pBuf->size.h);
      name_w = MAX(pBuf->size.w, name_w);
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);
      
      /* active */	
      my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].active_count);
      pStr = create_str16_from_char(cBuf, adj_font(10));
      pStr->style |= SF_CENTER;
      pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
					WF_RESTORE_BACKGROUND);
      if(count > adj_size(72)) {
	set_wflag(pBuf, WF_HIDDEN);
      }
      hh = MAX(hh, pBuf->size.h);
      pBuf->size.w = pText1->w + adj_size(6);
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);
      
      /* shield upkeep */	
      my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].upkeep[O_SHIELD]);
      pStr = create_str16_from_char(cBuf, adj_font(10));
      pStr->style |= SF_CENTER;
      pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
      						WF_RESTORE_BACKGROUND);
      if(count > adj_size(72)) {
	set_wflag(pBuf, WF_HIDDEN);
      }
      hh = MAX(hh, pBuf->size.h);
      pBuf->size.w = pText1->w;
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);
	
      /* food upkeep */
      my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].upkeep[O_FOOD]);
      pStr = create_str16_from_char(cBuf, adj_font(10));
      pStr->style |= SF_CENTER;
      pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
						WF_RESTORE_BACKGROUND);
      if(count > adj_size(72)) {
	set_wflag(pBuf, WF_HIDDEN);
      }
	
      hh = MAX(hh, pBuf->size.h);
      pBuf->size.w = pText1->w;
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);

      /* gold upkeep */
      my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].upkeep[O_GOLD]);
      pStr = create_str16_from_char(cBuf, adj_font(10));
      pStr->style |= SF_CENTER;
      pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
						WF_RESTORE_BACKGROUND);
      if(count > adj_size(72)) {
	set_wflag(pBuf, WF_HIDDEN);
      }
	
      hh = MAX(hh, pBuf->size.h);
      pBuf->size.w = pText1->w;
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);      
      
      /* building */
      if(units[utype_index(i)].building_count > 0) {
	my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].building_count);
      } else {
	my_snprintf(cBuf, sizeof(cBuf), "--");
      }
      pStr = create_str16_from_char(cBuf, adj_font(10));
      pStr->style |= SF_CENTER;
      pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
					WF_RESTORE_BACKGROUND);
      if(count > adj_size(72)) {
	set_wflag(pBuf, WF_HIDDEN);
      }
      hh = MAX(hh, pBuf->size.h);
      pBuf->size.w = pText2->w + adj_size(6);
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);
      
      /* soonest completion */
      if(units[utype_index(i)].building_count > 0) {
	my_snprintf(cBuf, sizeof(cBuf), "%d %s", units[utype_index(i)].soonest_completions,
			PL_("turn", "turns", units[utype_index(i)].soonest_completions));
      } else {
	my_snprintf(cBuf, sizeof(cBuf), "--");
      }
	
      pStr = create_str16_from_char(cBuf, adj_font(10));
      pStr->style |= SF_CENTER;
      pBuf = create_iconlabel(NULL, pWindow->dst, pStr,
					WF_RESTORE_BACKGROUND);
	
      if(count > adj_size(72)) {
	set_wflag(pBuf, WF_HIDDEN);
      }
      hh = MAX(hh, pBuf->size.h);
      pBuf->size.w = pText5->w + adj_size(6);
      add_to_gui_list(MAX_ID - utype_number(i), pBuf);

      
      count += adj_size(8);
      area.h += (hh/2);
    }
  } unit_type_iterate_end;
    
  pUnitsDlg->pBeginWidgetList = pBuf;
  area.w = MAX(area.w, (tileset_full_tile_width(tileset) * 2 + name_w + adj_size(15)) +
		(adj_size(4) * pText1->w + adj_size(46)) + (pText2->w + adj_size(16))
                                 + (pText5->w + adj_size(6)) + adj_size(2));
  if (count > 0) {
    pUnitsDlg->pEndActiveWidgetList = pLast->prev;
    pUnitsDlg->pBeginActiveWidgetList = pUnitsDlg->pBeginWidgetList;
    if(count > adj_size(80)) {
      pUnitsDlg->pActiveWidgetList = pUnitsDlg->pEndActiveWidgetList;
      if(pUnitsDlg->pScroll) {
	pUnitsDlg->pScroll->count = count;
      }
      ww = create_vertical_scrollbar(pUnitsDlg, 8, 10, TRUE, TRUE);
      area.w += ww;
      area.h = (hh + 9 * (hh/2) + adj_size(10));
    } else {
      area.h += hh/2;
    }
  } else {
    area.h = adj_size(50);
  }
  
  area.h += pText1->h + adj_size(10);
  area.w += adj_size(2);
  
  pLogo = theme_get_background(theme, BACKGROUND_UNITSREP);
  resize_window(pWindow, pLogo,	NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  FREESURFACE(pLogo);

  pLogo = SDL_DisplayFormat(pWindow->theme);
  FREESURFACE(pWindow->theme);
  pWindow->theme = pLogo;
  pLogo = NULL;

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);
  
  ww -= (pWindow->size.w - pWindow->area.w);
  
  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  /* totals background and label */
  dst.x = area.x + adj_size(2);
  dst.y = area.y + area.h - (pText3->h + adj_size(2)) - adj_size(2);
  dst.w = name_w + tileset_full_tile_width(tileset) * 2 + adj_size(5);
  dst.h = pText3->h + adj_size(2);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
  
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
  
  dst.y += 1;
  dst.x += ((name_w + tileset_full_tile_width(tileset) * 2 + adj_size(5)) - pText3->w) / 2;
  alphablit(pText3, NULL, pWindow->theme, &dst);
  FREESURFACE(pText3);
  
  /* total active widget */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + name_w +
                 tileset_full_tile_width(tileset) * 2 + adj_size(17);
  pBuf->size.y = area.y + dst.y;
  
  /* total shields cost widget */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
  pBuf->size.y = area.y + dst.y;
  
  /* total food cost widget */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
  pBuf->size.y = area.y + dst.y;
  
  /* total gold cost widget */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
  pBuf->size.y = area.y + dst.y;
  
  /* total building count widget */
  pBuf = pBuf->prev;
  pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
  pBuf->size.y = area.y + dst.y;
  
  /* units background and labels */
  dst.x = area.x + adj_size(2);
  dst.y = area.y + 1;
  dst.w = name_w + tileset_full_tile_width(tileset) * 2 + adj_size(5);
  dst.h = pText4->h + adj_size(2);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
  
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
  
  dst.y += 1;
  dst.x += ((name_w + tileset_full_tile_width(tileset) * 2 + adj_size(5))- pText4->w) / 2;
  alphablit(pText4, NULL, pWindow->theme, &dst);
  FREESURFACE(pText4);
  
  /* active count background and label */  
  dst.x = area.x + adj_size(2) + name_w + tileset_full_tile_width(tileset) * 2 + adj_size(15);
  dst.y = area.y + 1;
  dst.w = pText1->w + adj_size(6);
  dst.h = area.h - adj_size(3);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
    
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
    
  dst.x += adj_size(3);
  alphablit(pText1, NULL, pWindow->theme, &dst);
  ww = pText1->w;
  hh = pText1->h;
  FREESURFACE(pText1);
  
  /* shields cost background and label */
  dst.x += (ww + adj_size(13));
  w = dst.x;
  dst.w = ww;
  dst.h = area.h - adj_size(3);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
  
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
  
  dst.y = area.y + adj_size(3);
  dst.x += ((ww - pIcons->pBIG_Shield->w) / 2);
  alphablit(pIcons->pBIG_Shield, NULL, pWindow->theme, &dst);
  
  /* food cost background and label */
  dst.x = w + ww + adj_size(10);
  w = dst.x;
  dst.y = area.y + 1;
  dst.w = ww;
  dst.h = area.h - adj_size(3);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
  
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
  
  dst.y = area.y + adj_size(3);
  dst.x += ((ww - pIcons->pBIG_Food->w) / 2);
  alphablit(pIcons->pBIG_Food, NULL, pWindow->theme, &dst);
  
  /* gold cost background and label */
  dst.x = w + ww + adj_size(10);
  w = dst.x;
  dst.y = area.y + 1;
  dst.w = ww;
  dst.h = area.h - adj_size(3);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
  
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
  
  dst.y = area.y + adj_size(3);
  dst.x += ((ww - pIcons->pBIG_Coin->w) / 2);
  alphablit(pIcons->pBIG_Coin, NULL, pWindow->theme, &dst);
  
  /* building count background and label */
  dst.x = w + ww + adj_size(10);
  dst.y = area.y + 1;
  dst.w = pText2->w + adj_size(6);
  ww = pText2->w + adj_size(6);
  w = dst.x;
  dst.h = area.h - adj_size(3);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
  
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
			  
  dst.x += adj_size(3);
  alphablit(pText2, NULL, pWindow->theme, &dst);
  FREESURFACE(pText2);
   
  /* building count background and label */
  dst.x = w + ww + adj_size(10);
  dst.y = area.y + 1;
  dst.w = pText5->w + adj_size(6);
  dst.h = area.h - adj_size(3);
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color);
  
  putframe(pWindow->theme, dst.x , dst.y, dst.x + dst.w, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_UNITSREP_FRAME)));
			  
  dst.x += adj_size(3);
  alphablit(pText5, NULL, pWindow->theme, &dst);
  FREESURFACE(pText5);
  
  if(count) {
    int start_x = area.x + adj_size(2);
    int start_y = area.y + hh + adj_size(3);
    int mod = 0;
    
    pBuf = pBuf->prev;
    while(TRUE)
    {
      pBuf->size.x = start_x + (mod ? tileset_full_tile_width(tileset) : 0);
      pBuf->size.y = start_y;
      hh = pBuf->size.h;
      mod ^= 1;
      
      pBuf = pBuf->prev;
      pBuf->size.w = name_w;
      pBuf->size.x = start_x + tileset_full_tile_width(tileset) * 2 + adj_size(5);
      pBuf->size.y = start_y + (hh - pBuf->size.h) / 2;
      
      pBuf = pBuf->prev;
      pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
      pBuf->size.y = start_y + (hh - pBuf->size.h) / 2;
      
      pBuf = pBuf->prev;
      pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
      pBuf->size.y = start_y + (hh - pBuf->size.h) / 2;
      
      pBuf = pBuf->prev;
      pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
      pBuf->size.y = start_y + (hh - pBuf->size.h) / 2;
      
      pBuf = pBuf->prev;
      pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
      pBuf->size.y = start_y + (hh - pBuf->size.h) / 2;
      
      pBuf = pBuf->prev;
      pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
      pBuf->size.y = start_y + (hh - pBuf->size.h) / 2;
      
      pBuf = pBuf->prev;
      pBuf->size.x = pBuf->next->size.x + pBuf->next->size.w + adj_size(10);
      pBuf->size.y = start_y + (hh - pBuf->size.h) / 2;

      start_y += (hh>>1);
      if(pBuf == pUnitsDlg->pBeginActiveWidgetList) {
	break;
      }
      pBuf = pBuf->prev;
    }
    
    if (pUnitsDlg->pScroll)
    {
      setup_vertical_scrollbar_area(pUnitsDlg->pScroll,
	  area.x + area.w, area.y,
    	  area.h, TRUE);      
    }
    
  }
  /* ----------------------------------- */
  redraw_group(pUnitsDlg->pBeginWidgetList, pWindow, 0);
  widget_mark_dirty(pWindow);
    
  flush_dirty();  
}

/**************************************************************************
  update the units report.
**************************************************************************/
void activeunits_report_dialog_update(void)
{
  if(pUnitsDlg && !is_report_dialogs_frozen()) {
    struct units_entry units[U_LAST];
    struct units_entry units_total;
    struct widget *pWidget, *pBuf;
    bool is_in_list = FALSE;
    char cBuf[32];
    bool upgrade;
    bool search_finished;
    
    get_units_report_data(units, &units_total);

    /* find if there are new units entry (if not then rebuild all) */
    pWidget = pUnitsDlg->pEndActiveWidgetList; /* icon of first list entry */
    unit_type_iterate(i) {
      if ((units[utype_index(i)].active_count > 0) || (units[utype_index(i)].building_count > 0)) {
        is_in_list = FALSE;
        
        pBuf = pWidget; /* unit type icon */
        while(pBuf) {
	  if((MAX_ID - pBuf->ID) == utype_number(i)) {
	    is_in_list = TRUE;
	    pWidget = pBuf;
	    break;
	  }
	  if(pBuf->prev->prev->prev->prev->prev->prev->prev ==
			  pUnitsDlg->pBeginActiveWidgetList) {
	    break;
	  }

          /* first widget of next list entry */
	  pBuf = pBuf->prev->prev->prev->prev->prev->prev->prev->prev;
        }
        
        if(!is_in_list) {
	  real_activeunits_report_dialog_update(units, &units_total);
          return;
        }
      }
    } unit_type_iterate_end;
  
  
    /* update list */
    pWidget = pUnitsDlg->pEndActiveWidgetList;
    unit_type_iterate(i) {
      pBuf = pWidget; /* first widget (icon) of the first list entry */

      if ((units[utype_index(i)].active_count > 0) || (units[utype_index(i)].building_count > 0)) {
        /* the player has at least one unit of this type */

        search_finished = FALSE;
        while (!search_finished) {
          if ((MAX_ID - pBuf->ID) == utype_number(i)) { /* list entry for this unit type found */

            upgrade = (can_upgrade_unittype(client.conn.playing, i) != NULL);

            pBuf = pBuf->prev; /* unit type name */
            if(upgrade) {
              pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_UNITUPGRADE_TEXT);
              pBuf->action = popup_upgrade_unit_callback;
              set_wstate(pBuf, FC_WS_NORMAL);
            }

            pBuf = pBuf->prev; /* active */
            my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].active_count);
            copy_chars_to_string16(pBuf->string16, cBuf);

            pBuf = pBuf->prev; /* shield upkeep */
            my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].upkeep[O_SHIELD]);
            copy_chars_to_string16(pBuf->string16, cBuf);

            pBuf = pBuf->prev; /* food upkeep */
            my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].upkeep[O_FOOD]);
            copy_chars_to_string16(pBuf->string16, cBuf);

            pBuf = pBuf->prev; /* gold upkeep */
            my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].upkeep[O_GOLD]);
            copy_chars_to_string16(pBuf->string16, cBuf);

            pBuf = pBuf->prev; /* building */
            if(units[utype_index(i)].building_count > 0) {
              my_snprintf(cBuf, sizeof(cBuf), "%d", units[utype_index(i)].building_count);
            } else {
              my_snprintf(cBuf, sizeof(cBuf), "--");
            }
            copy_chars_to_string16(pBuf->string16, cBuf);

            pBuf = pBuf->prev; /* soonest completion */
            if(units[utype_index(i)].building_count > 0) {
              my_snprintf(cBuf, sizeof(cBuf), "%d %s", units[utype_index(i)].soonest_completions,
                          PL_("turn", "turns", units[utype_index(i)].soonest_completions));
            } else {
              my_snprintf(cBuf, sizeof(cBuf), "--");
            }
            copy_chars_to_string16(pBuf->string16, cBuf);

            pWidget = pBuf->prev; /* icon of next unit type */

            search_finished = TRUE;

          } else { /* list entry for this unit type not found yet */
            
            /* search it */ 
            pBuf = pWidget->next;
            do {
	      del_widget_from_vertical_scroll_widget_list(pUnitsDlg, pBuf->prev);
	    } while(((MAX_ID - pBuf->prev->ID) != utype_number(i)) &&
	            (pBuf->prev != pUnitsDlg->pBeginActiveWidgetList));

	    if(pBuf->prev == pUnitsDlg->pBeginActiveWidgetList) {
	      /* list entry not found - can this really happen? */
	      del_widget_from_vertical_scroll_widget_list(pUnitsDlg, pBuf->prev);
	      pWidget = pBuf->prev; /* pUnitsDlg->pBeginActiveWidgetList */
	      search_finished = TRUE;
	    } else {
              /* found it */
	      pBuf = pBuf->prev; /* first widget (icon) of list entry */
	    }
          }
        }
      } else { /* player has no unit of this type */
        if(pBuf && pBuf->next != pUnitsDlg->pBeginActiveWidgetList) {
          if (utype_number(i) < (MAX_ID - pBuf->ID)) {
            continue;
          } else {
            pBuf = pBuf->next;
            do {
              del_widget_from_vertical_scroll_widget_list(pUnitsDlg,
                                                          pBuf->prev);
            } while(((MAX_ID - pBuf->prev->ID) == utype_number(i)) &&
                    (pBuf->prev != pUnitsDlg->pBeginActiveWidgetList));
            if(pBuf->prev == pUnitsDlg->pBeginActiveWidgetList) {
              del_widget_from_vertical_scroll_widget_list(pUnitsDlg,
                                                          pBuf->prev);
            }
            pWidget = pBuf->prev;
          }
        }
      }
    } unit_type_iterate_end;

    /* -------------------------------------- */

    /* total active */
    pBuf = pUnitsDlg->pEndWidgetList->prev->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", units_total.active_count);
    copy_chars_to_string16(pBuf->string16, cBuf);
  
    /* total shields cost */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", units_total.upkeep[O_SHIELD]);
    copy_chars_to_string16(pBuf->string16, cBuf);
  
    /* total food cost widget */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", units_total.upkeep[O_FOOD]);
    copy_chars_to_string16(pBuf->string16, cBuf);
  
    /* total building count */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", units_total.building_count);
    copy_chars_to_string16(pBuf->string16, cBuf);

    /* -------------------------------------- */
    redraw_group(pUnitsDlg->pBeginWidgetList, pUnitsDlg->pEndWidgetList, 0);
    widget_mark_dirty(pUnitsDlg->pEndWidgetList);
    
    flush_dirty();
  }
}

/**************************************************************************
  Popup (or raise) the units report (F2).  It may or may not be modal.
**************************************************************************/
void popup_activeunits_report_dialog(bool make_modal)
{
  struct units_entry units[U_LAST];
  struct units_entry units_total;
    
  if(pUnitsDlg) {
    return;
  }

  get_units_report_data(units, &units_total);
  real_activeunits_report_dialog_update(units, &units_total);
}

/**************************************************************************
  Popdown the units report.
**************************************************************************/
void popdown_activeunits_report_dialog(void)
{
  if (pUnitsDlg) {
    if (pUnits_Upg_Dlg) {
       del_group_of_widgets_from_gui_list(pUnits_Upg_Dlg->pBeginWidgetList,
			      pUnits_Upg_Dlg->pEndWidgetList);
       FC_FREE(pUnits_Upg_Dlg); 
    }
    popdown_window_group_dialog(pUnitsDlg->pBeginWidgetList,
				      pUnitsDlg->pEndWidgetList);
    FC_FREE(pUnitsDlg->pScroll);
    FC_FREE(pUnitsDlg);
  }
}
/* ===================================================================== */
/* ======================== Economy Report ============================= */
/* ===================================================================== */
static struct ADVANCED_DLG *pEconomyDlg = NULL;
static struct SMALL_DLG *pEconomy_Sell_Dlg = NULL;
  
struct rates_move {
  int min, max, tax, x, gov_max;
  int *src_rate, *dst_rate;
  struct widget *pHoriz_Src, *pHoriz_Dst;
  struct widget *pLabel_Src, *pLabel_Dst;
};

static int economy_dialog_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pEconomyDlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int exit_economy_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if(pEconomyDlg) {
      if (pEconomy_Sell_Dlg) {
         del_group_of_widgets_from_gui_list(pEconomy_Sell_Dlg->pBeginWidgetList,
                                pEconomy_Sell_Dlg->pEndWidgetList);
         FC_FREE(pEconomy_Sell_Dlg); 
      }
      popdown_window_group_dialog(pEconomyDlg->pBeginWidgetList,
                                              pEconomyDlg->pEndWidgetList);
      FC_FREE(pEconomyDlg->pScroll);
      FC_FREE(pEconomyDlg);
      set_wstate(get_tax_rates_widget(), FC_WS_NORMAL);
      widget_redraw(get_tax_rates_widget());
      widget_mark_dirty(get_tax_rates_widget());
      flush_dirty();
    }
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int toggle_block_callback(struct widget *pCheckBox)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    switch (pCheckBox->ID) {
    case ID_CHANGE_TAXRATE_DLG_LUX_BLOCK_CHECKBOX:
      SDL_Client_Flags ^= CF_CHANGE_TAXRATE_LUX_BLOCK;
      return -1;
  
    case ID_CHANGE_TAXRATE_DLG_SCI_BLOCK_CHECKBOX:
      SDL_Client_Flags ^= CF_CHANGE_TAXRATE_SCI_BLOCK;
      return -1;
  
    default:
      return -1;
    }
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static Uint16 report_scroll_mouse_button_up(SDL_MouseButtonEvent *pButtonEvent,
						void *pData)
{
  return (Uint16)ID_SCROLLBAR;
}

/**************************************************************************
  ...
**************************************************************************/
static Uint16 report_scroll_mouse_motion_handler(
			SDL_MouseMotionEvent *pMotionEvent, void *pData)
{
  struct rates_move *pMotion = (struct rates_move *)pData;
  struct widget *pTax_Label = pEconomyDlg->pEndWidgetList->prev->prev;
  struct widget *pBuf = NULL;
  char cBuf[8];
  int dir, inc, x, *buf_rate = NULL;
  
  pMotionEvent->x -= pMotion->pHoriz_Src->dst->dest_rect.x;
  
  if ((abs(pMotionEvent->x - pMotion->x) > 7) &&
     (pMotionEvent->x >= pMotion->min) && (pMotionEvent->x <= pMotion->max)) {
	
	/* set up directions */
    if (pMotionEvent->xrel > 0) {
      dir = 15;
      inc = 10;
    } else {
      dir = -15;
      inc = -10;
    }
	
    /* make checks */
    x = pMotion->pHoriz_Src->size.x;
    if (((x + dir) <= pMotion->max) && ((x + dir) >= pMotion->min)) {
      /* src in range */  
      if(pMotion->pHoriz_Dst) {
        x = pMotion->pHoriz_Dst->size.x;
	if (((x + (-1 * dir)) > pMotion->max) || ((x + (-1 * dir)) < pMotion->min)) {
	  /* dst out of range */
	  if(pMotion->tax + (-1 * inc) <= pMotion->gov_max &&
	    pMotion->tax + (-1 * inc) >= 0) {
	    /* tax in range */
	    pBuf = pMotion->pHoriz_Dst;
	    pMotion->pHoriz_Dst = NULL;
	    buf_rate = pMotion->dst_rate;
	    pMotion->dst_rate = &pMotion->tax;
	    pMotion->pLabel_Dst = pTax_Label;
	  } else {
	    pMotion->x = pMotion->pHoriz_Src->size.x;
	    return ID_ERROR;
	  }		  
	}
      } else {
        if(pMotion->tax + (-1 * inc) > pMotion->gov_max ||
	  pMotion->tax + (-1 * inc) < 0) {
	  pMotion->x = pMotion->pHoriz_Src->size.x;
	  return ID_ERROR;
	}
      }

      /* undraw scrollbars */
      widget_undraw(pMotion->pHoriz_Src);
      widget_mark_dirty(pMotion->pHoriz_Src);
	    
      if(pMotion->pHoriz_Dst) {
        widget_undraw(pMotion->pHoriz_Dst);
        widget_mark_dirty(pMotion->pHoriz_Dst);
      }
	  
      pMotion->pHoriz_Src->size.x += dir;
      if(pMotion->pHoriz_Dst) {
        pMotion->pHoriz_Dst->size.x -= dir;
      }

      *pMotion->src_rate += inc;
      *pMotion->dst_rate -= inc;
	  	  
      my_snprintf(cBuf, sizeof(cBuf), "%d%%", *pMotion->src_rate);
      copy_chars_to_string16(pMotion->pLabel_Src->string16, cBuf);
      my_snprintf(cBuf, sizeof(cBuf), "%d%%", *pMotion->dst_rate);
      copy_chars_to_string16(pMotion->pLabel_Dst->string16, cBuf);
      		      
      /* redraw label */
      widget_redraw(pMotion->pLabel_Src);
      widget_mark_dirty(pMotion->pLabel_Src);

      widget_redraw(pMotion->pLabel_Dst);
      widget_mark_dirty(pMotion->pLabel_Dst);

      /* redraw scroolbar */
      if (get_wflags(pMotion->pHoriz_Src) & WF_RESTORE_BACKGROUND) {      
        refresh_widget_background(pMotion->pHoriz_Src);
      }
      widget_redraw(pMotion->pHoriz_Src);
      widget_mark_dirty(pMotion->pHoriz_Src);
	  
      if(pMotion->pHoriz_Dst) {
        if (get_wflags(pMotion->pHoriz_Dst) & WF_RESTORE_BACKGROUND) {
          refresh_widget_background(pMotion->pHoriz_Dst);
        }
        widget_redraw(pMotion->pHoriz_Dst);
        widget_mark_dirty(pMotion->pHoriz_Dst);
      }

      flush_dirty();

      if (pBuf) {
        pMotion->pHoriz_Dst = pBuf;
        pMotion->pLabel_Dst = pMotion->pHoriz_Dst->prev;
        pMotion->dst_rate = buf_rate;
        pBuf = NULL;
      }

      pMotion->x = pMotion->pHoriz_Src->size.x;
    }
  }				/* if */
  
  return ID_ERROR;
}


/**************************************************************************
  ...
**************************************************************************/
static int horiz_taxrate_callback(struct widget *pHoriz_Src)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct rates_move pMotion;
    
    pMotion.pHoriz_Src = pHoriz_Src;
    pMotion.pLabel_Src = pHoriz_Src->prev;
    
    switch (pHoriz_Src->ID) {
      case ID_CHANGE_TAXRATE_DLG_LUX_SCROLLBAR:
        if (SDL_Client_Flags & CF_CHANGE_TAXRATE_LUX_BLOCK) {
          goto END;
        }
        pMotion.src_rate = (int *)pHoriz_Src->data.ptr;
        pMotion.pHoriz_Dst = pHoriz_Src->prev->prev->prev;	/* sci */
        pMotion.dst_rate = (int *)pMotion.pHoriz_Dst->data.ptr;
        pMotion.tax = 100 - *pMotion.src_rate - *pMotion.dst_rate;
        if ((SDL_Client_Flags & CF_CHANGE_TAXRATE_SCI_BLOCK)) {
          if (pMotion.tax <= get_player_bonus(client.conn.playing, EFT_MAX_RATES)) {
            pMotion.pHoriz_Dst = NULL;	/* tax */
            pMotion.dst_rate = &pMotion.tax;
          } else {
            goto END;	/* all blocked */
          }
        }
  
      break;
  
      case ID_CHANGE_TAXRATE_DLG_SCI_SCROLLBAR:
        if ((SDL_Client_Flags & CF_CHANGE_TAXRATE_SCI_BLOCK)) {
          goto END;
        }
        pMotion.src_rate = (int *)pHoriz_Src->data.ptr;
        pMotion.pHoriz_Dst = pHoriz_Src->next->next->next;	/* lux */
        pMotion.dst_rate = (int *)pMotion.pHoriz_Dst->data.ptr;
        pMotion.tax = 100 - *pMotion.src_rate - *pMotion.dst_rate;
        if (SDL_Client_Flags & CF_CHANGE_TAXRATE_LUX_BLOCK) {
          if (pMotion.tax <= get_player_bonus(client.conn.playing, EFT_MAX_RATES)) {
            /* tax */
            pMotion.pHoriz_Dst = NULL;
            pMotion.dst_rate = &pMotion.tax;
          } else {
            goto END;	/* all blocked */
          }
        }
  
      break;
  
      default:
        return -1;
    }
  
    if(pMotion.pHoriz_Dst) {
      pMotion.pLabel_Dst = pMotion.pHoriz_Dst->prev;
    } else {
      /* tax label */
      pMotion.pLabel_Dst = pEconomyDlg->pEndWidgetList->prev->prev;
    }
  
    pMotion.min = pHoriz_Src->next->size.x + pHoriz_Src->next->size.w + adj_size(2);
    pMotion.gov_max = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
    pMotion.max = pMotion.min + pMotion.gov_max * 1.5;
    pMotion.x = pHoriz_Src->size.x;
    
    MOVE_STEP_Y = 0;
    /* Filter mouse motion events */
    SDL_SetEventFilter(FilterMouseMotionEvents);
    gui_event_loop((void *)(&pMotion), NULL, NULL, NULL, NULL,
                    report_scroll_mouse_button_up,
                          report_scroll_mouse_motion_handler);
    /* Turn off Filter mouse motion events */
    SDL_SetEventFilter(NULL);
    MOVE_STEP_Y = DEFAULT_MOVE_STEP;
    
END:
    unsellect_widget_action();
    pSellected_Widget = pHoriz_Src;
    set_wstate(pHoriz_Src, FC_WS_SELLECTED);
    widget_redraw(pHoriz_Src);
    widget_flush(pHoriz_Src);
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int apply_taxrates_callback(struct widget *pButton)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct widget *pBuf;
    int science, luxury, tax;
  
    if (C_S_RUNNING != client_state()) {
      return -1;
    }
  
    /* Science Scrollbar */
    pBuf = pButton->next->next;
    science = *(int *)pBuf->data.ptr;
      
    /* Luxuries Scrollbar */
    pBuf = pBuf->next->next->next;
    luxury = *(int *)pBuf->data.ptr;
    
    /* Tax */
    tax = 100 - luxury - science;

    if (tax != client.conn.playing->economic.tax
     || science != client.conn.playing->economic.science
     || luxury != client.conn.playing->economic.luxury) {
      dsend_packet_player_rates(&client.conn, tax, luxury, science);
    }
  
    widget_redraw(pButton);
    widget_flush(pButton);
  }
  return -1;
}

static void enable_economy_dlg(void)
{
  /* lux lock */
  struct widget *pBuf = pEconomyDlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;
  set_wstate(pBuf, FC_WS_NORMAL);
  
  /* lux scrollbar */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_NORMAL);
  
  /* sci lock */
  pBuf = pBuf->prev->prev;
  set_wstate(pBuf, FC_WS_NORMAL);
  
  /* sci scrollbar */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_NORMAL);
  
  /* update button */
  pBuf = pBuf->prev->prev;
  set_wstate(pBuf, FC_WS_NORMAL);
  
  /* cancel button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_NORMAL);
  
  set_group_state(pEconomyDlg->pBeginActiveWidgetList,
			pEconomyDlg->pEndActiveWidgetList, FC_WS_NORMAL);
  if(pEconomyDlg->pScroll && pEconomyDlg->pActiveWidgetList) {
    set_wstate(pEconomyDlg->pScroll->pUp_Left_Button, FC_WS_NORMAL);
    set_wstate(pEconomyDlg->pScroll->pDown_Right_Button, FC_WS_NORMAL);
    set_wstate(pEconomyDlg->pScroll->pScrollBar, FC_WS_NORMAL);
  }
}

static void disable_economy_dlg(void)
{
  /* lux lock */
  struct widget *pBuf = pEconomyDlg->pEndWidgetList->prev->prev->prev->prev->prev->prev;
  set_wstate(pBuf, FC_WS_DISABLED);
  
  /* lux scrollbar */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);
  
  /* sci lock */
  pBuf = pBuf->prev->prev;
  set_wstate(pBuf, FC_WS_DISABLED);
  
  /* sci scrollbar */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);
  
  /* update button */
  pBuf = pBuf->prev->prev;
  set_wstate(pBuf, FC_WS_DISABLED);
  
  /* cancel button */
  pBuf = pBuf->prev;
  set_wstate(pBuf, FC_WS_DISABLED);
  
  set_group_state(pEconomyDlg->pBeginActiveWidgetList,
			pEconomyDlg->pEndActiveWidgetList, FC_WS_DISABLED);
  if(pEconomyDlg->pScroll && pEconomyDlg->pActiveWidgetList) {
    set_wstate(pEconomyDlg->pScroll->pUp_Left_Button, FC_WS_DISABLED);
    set_wstate(pEconomyDlg->pScroll->pDown_Right_Button, FC_WS_DISABLED);
    set_wstate(pEconomyDlg->pScroll->pScrollBar, FC_WS_DISABLED);
  }
  
}

/* --------------------------------------------------------------- */
static int ok_sell_impv_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int imp, total_count, count = 0;
    struct widget *pImpr = (struct widget *)pWidget->data.ptr;
      
    imp = pImpr->data.cont->id0;
    total_count = pImpr->data.cont->id1;
    
    /* popdown sell dlg */
    popdown_window_group_dialog(pEconomy_Sell_Dlg->pBeginWidgetList,
                                pEconomy_Sell_Dlg->pEndWidgetList);
    FC_FREE(pEconomy_Sell_Dlg);
    enable_economy_dlg();
    
    /* send sell */
    city_list_iterate(client.conn.playing->cities, pCity) {
      if(!pCity->did_sell && city_has_building(pCity, improvement_by_number(imp))){
          count++;
  
          city_sell_improvement(pCity, imp);
        
      }
    } city_list_iterate_end;
    
    if(count == total_count) {
      del_widget_from_vertical_scroll_widget_list(pEconomyDlg, pImpr);
    }
  }  
  return -1;
}

static int sell_impv_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pEconomy_Sell_Dlg->pBeginWidgetList, pWindow);
  }
  return -1;
}

static int cancel_sell_impv_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pEconomy_Sell_Dlg) {
      popdown_window_group_dialog(pEconomy_Sell_Dlg->pBeginWidgetList,
                                pEconomy_Sell_Dlg->pEndWidgetList);
      FC_FREE(pEconomy_Sell_Dlg);
      enable_economy_dlg();
      flush_dirty();
    }
  }
  return -1;
}


static int popup_sell_impv_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int imp, total_count ,count = 0, gold = 0;
    int value;
    char cBuf[128];
    struct widget *pBuf = NULL, *pWindow;
    SDL_String16 *pStr;
    SDL_Surface *pText;
    SDL_Rect dst;
    SDL_Rect area;
    
    if (pEconomy_Sell_Dlg) {
      return 1;
    }
    
    set_wstate(pWidget, FC_WS_NORMAL);
    pSellected_Widget = NULL;
    widget_redraw(pWidget);
    widget_mark_dirty(pWidget);
    
    pEconomy_Sell_Dlg = fc_calloc(1, sizeof(struct SMALL_DLG));
  
    imp = pWidget->data.cont->id0;
    total_count = pWidget->data.cont->id1;
    value = impr_sell_gold(improvement_by_number(imp));

    city_list_iterate(client.conn.playing->cities, pCity) {
      if(!pCity->did_sell && city_has_building(pCity, improvement_by_number(imp))) {
          count++;
          gold += value;
      }
    } city_list_iterate_end;
    
    if(count > 0) {
      my_snprintf(cBuf, sizeof(cBuf),
                  _("We have %d of %s\n(total value is : %d)\n"
                    "We can sell %d of them for %d gold."),
                  total_count,
                  improvement_name_translation(improvement_by_number(imp)),
                  total_count * value, count, gold); 
    } else {
      my_snprintf(cBuf, sizeof(cBuf),
                  _("We can't sell any %s in this turn."),
                  improvement_name_translation(improvement_by_number(imp))); 
    }
    
    
    pStr = create_str16_from_char(_("Sell It?"), adj_font(12));
    pStr->style |= TTF_STYLE_BOLD;
  
    pWindow = create_window_skeleton(NULL, pStr, 0);
  
    pWindow->action = sell_impv_window_callback;
    set_wstate(pWindow, FC_WS_NORMAL);
  
    pEconomy_Sell_Dlg->pEndWidgetList = pWindow;
  
    add_to_gui_list(ID_WINDOW, pWindow);

    area = pWindow->area;
    
    /* ============================================================= */
    
    /* create text label */
    pStr = create_str16_from_char(cBuf, adj_font(10));
    pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_SELLIMPR_TEXT);
    
    pText = create_text_surf_from_str16(pStr);
    FREESTRING16(pStr);
    
    area.w = MAX(area.w, pText->w + adj_size(20));
    area.h += (pText->h + adj_size(10));
    
    /* cancel button */
    pBuf = create_themeicon_button_from_chars(pTheme->CANCEL_Icon,
                              pWindow->dst, _("No"), adj_font(12), 0);
  
    pBuf->action = cancel_sell_impv_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
  
    area.h += (pBuf->size.h + adj_size(20));
    
    add_to_gui_list(ID_BUTTON, pBuf);
    
    if (count > 0) {
      pBuf = create_themeicon_button_from_chars(pTheme->OK_Icon, pWindow->dst,
                                                "Sell", adj_font(12), 0);
          
      pBuf->action = ok_sell_impv_callback;
      set_wstate(pBuf, FC_WS_NORMAL);
      pBuf->data.ptr = (void *)pWidget;
      
      add_to_gui_list(ID_BUTTON, pBuf);
      pBuf->size.w = MAX(pBuf->size.w, pBuf->next->size.w);
      pBuf->next->size.w = pBuf->size.w;
      area.w = MAX(area.w, adj_size(30) + pBuf->size.w * 2);
    } else {
      area.w = MAX(area.w, pBuf->size.w + adj_size(20));
    }
    /* ============================================ */
    
    pEconomy_Sell_Dlg->pBeginWidgetList = pBuf;
    
    resize_window(pWindow, NULL, get_game_colorRGB(COLOR_THEME_BACKGROUND),
                  (pWindow->size.w - pWindow->area.w) + area.w,
                  (pWindow->size.h - pWindow->area.h) + area.h);
    
    area = pWindow->area;

    widget_set_position(pWindow,
                        pEconomyDlg->pEndWidgetList->size.x +
                          (pEconomyDlg->pEndWidgetList->size.w - pWindow->size.w) / 2,
                        pEconomyDlg->pEndWidgetList->size.y +
                          (pEconomyDlg->pEndWidgetList->size.h - pWindow->size.h) / 2);
    
    /* setup rest of widgets */
    /* label */
    dst.x = area.x + (area.w - pText->w) / 2;
    dst.y = area.y + adj_size(10);
    alphablit(pText, NULL, pWindow->theme, &dst);
    FREESURFACE(pText);
     
    /* cancel button */
    pBuf = pWindow->prev;
    pBuf->size.y = area.y + area.h - pBuf->size.h - adj_size(10);
    
    if (count > 0) {
      /* sell button */
      pBuf = pBuf->prev;
      pBuf->size.x = area.x + (area.w - (2 * pBuf->size.w + adj_size(10))) / 2;
      pBuf->size.y = pBuf->next->size.y;
      
      /* cancel button */
      pBuf->next->size.x = pBuf->size.x + pBuf->size.w + adj_size(10);
    } else {
      /* x position of cancel button */
      pBuf->size.x = area.x + area.w - adj_size(10) - pBuf->size.w;
    }
    
    
    /* ================================================== */
    /* redraw */
    redraw_group(pEconomy_Sell_Dlg->pBeginWidgetList, pWindow, 0);
    disable_economy_dlg();
    
    widget_mark_dirty(pWindow);
    flush_dirty();
  }
  return -1;
}


/**************************************************************************
  Update the economy report.
**************************************************************************/
void economy_report_dialog_update(void)
{
  if(pEconomyDlg && !is_report_dialogs_frozen()) {
    struct widget *pBuf = pEconomyDlg->pEndWidgetList;
    int tax, total, entries_used = 0;
    char cBuf[128];
    struct improvement_entry entries[B_LAST];
    
    get_economy_report_data(entries, &entries_used, &total, &tax);
  
    /* tresure */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", client.conn.playing->economic.gold);
    copy_chars_to_string16(pBuf->string16, cBuf);
    remake_label_size(pBuf);
  
    /* Icome */
    pBuf = pBuf->prev->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", tax);
    copy_chars_to_string16(pBuf->string16, cBuf);
    remake_label_size(pBuf);
  
    /* Cost */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", total);
    copy_chars_to_string16(pBuf->string16, cBuf);
    remake_label_size(pBuf);
  
    /* Netto */
    pBuf = pBuf->prev;
    my_snprintf(cBuf, sizeof(cBuf), "%d", tax - total);
    copy_chars_to_string16(pBuf->string16, cBuf);
    remake_label_size(pBuf);
    if(tax - total < 0) {
      pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_ECONOMYDLG_NEG_TEXT);
    } else {
      pBuf->string16->fgcol = *get_game_colorRGB(COLOR_THEME_ECONOMYDLG_TEXT);
    }
  
  
    /* ---------------- */
    redraw_group(pEconomyDlg->pBeginWidgetList, pEconomyDlg->pEndWidgetList, 0);
    widget_flush(pEconomyDlg->pEndWidgetList);
  }
}

/**************************************************************************
  Popdown the economy report.
**************************************************************************/
void popdown_economy_report_dialog(void)
{
  if(pEconomyDlg) {
    if (pEconomy_Sell_Dlg) {
       del_group_of_widgets_from_gui_list(pEconomy_Sell_Dlg->pBeginWidgetList,
			      pEconomy_Sell_Dlg->pEndWidgetList);
       FC_FREE(pEconomy_Sell_Dlg); 
    }    
    popdown_window_group_dialog(pEconomyDlg->pBeginWidgetList,
					    pEconomyDlg->pEndWidgetList);
    FC_FREE(pEconomyDlg->pScroll);
    FC_FREE(pEconomyDlg);
    set_wstate(get_tax_rates_widget(), FC_WS_NORMAL);
    widget_redraw(get_tax_rates_widget());
    widget_mark_dirty(get_tax_rates_widget());
  }
}

#define TARGETS_ROW	2
#define TARGETS_COL	4

/**************************************************************************
  Popup (or raise) the economy report (F5).  It may or may not be modal.
**************************************************************************/
void popup_economy_report_dialog(bool make_modal)
{
  SDL_Color bg_color = {255,255,255,128};
  SDL_Color bg_color2 = {255,255,255,136};
  SDL_Color bg_color3 = {255,255,255,64};

  struct widget *pBuf;
  struct widget *pWindow , *pLast;
  SDL_String16 *pStr, *pStr2;
  SDL_Surface *pSurf, *pText_Name, *pText, *pZoom;
  SDL_Surface *pBackground;
  int i, count , h = 0;
  int w = 0; /* left column values */
  int w2 = 0; /* right column: lock + scrollbar + ... */
  int w3 = 0; /* left column text without values */
  int tax, total, entries_used = 0;
  char cBuf[128];
  struct improvement_entry entries[B_LAST];
  SDL_Rect dst;
  SDL_Rect area;
  struct government *pGov = government_of_player(client.conn.playing);

  SDL_Surface *pTreasuryText;
  struct widget *pTreasuryValue;
  SDL_Surface *pTaxRateText;
  struct widget *pTaxRateValue;
  SDL_Surface *pTotalIncomeText;
  struct widget *pTotalIncomeValue;
  SDL_Surface *pTotalCostText;
  struct widget *pTotalCostValue;
  SDL_Surface *pNetIncomeText;
  struct widget *pNetIncomeValue;
  SDL_Surface *pMaxRateText;
  
  if(pEconomyDlg) {
    return;
  }
  
  /* disable "Economy" button */
  pBuf = get_tax_rates_widget();
  set_wstate(pBuf, FC_WS_DISABLED);
  widget_redraw(pBuf);
  widget_mark_dirty(pBuf);
  
  pEconomyDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  
  get_economy_report_data(entries, &entries_used, &total, &tax);
  
  /* --------------- */
  pStr = create_str16_from_char(_("Economy Report"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);
  pEconomyDlg->pEndWidgetList = pWindow;
  set_wstate(pWindow, FC_WS_NORMAL);
  pWindow->action = economy_dialog_callback;
  
  add_to_gui_list(ID_ECONOMY_DIALOG_WINDOW, pWindow);

  area = pWindow->area;
  
  /* ------------------------- */
  
  /* "Treasury" text surface */
  my_snprintf(cBuf, sizeof(cBuf), _("Treasury: "));
  pStr2 = create_str16_from_char(cBuf, adj_font(12));
  pStr2->style |= TTF_STYLE_BOLD;
  pTreasuryText = create_text_surf_from_str16(pStr2);
  w3 = MAX(w3, pTreasuryText->w);
  
  /* "Treasury" value label*/
  my_snprintf(cBuf, sizeof(cBuf), "%d", client.conn.playing->economic.gold);
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);

  pBuf = create_iconlabel(pIcons->pBIG_Coin, pWindow->dst, pStr,
  			(WF_RESTORE_BACKGROUND|WF_ICON_CENTER_RIGHT));
  
  add_to_gui_list(ID_LABEL, pBuf);

  pTreasuryValue = pBuf;

  w = MAX(w, pBuf->size.w);
  h += pBuf->size.h;
  
  /* "Tax Rate" text surface */
  my_snprintf(cBuf, sizeof(cBuf), _("Tax Rate: "));
  copy_chars_to_string16(pStr2, cBuf);
  pTaxRateText = create_text_surf_from_str16(pStr2);
  w3 = MAX(w3, pTaxRateText->w);
  
  /* "Tax Rate" value label */
  /* it is important to leave 1 space at ending of this string */
  my_snprintf(cBuf, sizeof(cBuf), "%d%% " , client.conn.playing->economic.tax);
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
    
  add_to_gui_list(ID_LABEL, pBuf);

  pTaxRateValue = pBuf;

  w = MAX(w, pBuf->size.w + pBuf->next->size.w);
  h += pBuf->size.h;

  /* "Total Income" text surface */
  my_snprintf(cBuf, sizeof(cBuf), _("Total Income: "));
  copy_chars_to_string16(pStr2, cBuf);
  pTotalIncomeText = create_text_surf_from_str16(pStr2);
  w3 = MAX(w3, pTotalIncomeText->w);

  /* "Total Icome" value label */
  my_snprintf(cBuf, sizeof(cBuf), "%d", tax);
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
    
  add_to_gui_list(ID_LABEL, pBuf);

  pTotalIncomeValue = pBuf;

  w = MAX(w, pBuf->size.w);
  h += pBuf->size.h;

  /* "Total Cost" text surface */
  my_snprintf(cBuf, sizeof(cBuf), _("Total Cost: "));
  copy_chars_to_string16(pStr2, cBuf);
  pTotalCostText = create_text_surf_from_str16(pStr2);
  
  /* "Total Cost" value label */
  my_snprintf(cBuf, sizeof(cBuf), "%d", total);
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
    
  add_to_gui_list(ID_LABEL, pBuf);
  
  pTotalCostValue = pBuf;
  
  w = MAX(w, pBuf->size.w);
  h += pBuf->size.h;
  
  /* "Net Income" text surface */
  my_snprintf(cBuf, sizeof(cBuf), _("Net Income: "));
  copy_chars_to_string16(pStr2, cBuf);
  pNetIncomeText = create_text_surf_from_str16(pStr2);
  w3 = MAX(w3, pNetIncomeText->w);
  
  /* "Net Icome" value label */
  my_snprintf(cBuf, sizeof(cBuf), "%d", tax - total);
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pStr->style |= (TTF_STYLE_BOLD|SF_CENTER);
  
  if(tax - total < 0) {
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_ECONOMYDLG_NEG_TEXT);
  }
  
  pBuf = create_iconlabel(NULL, pWindow->dst, pStr, WF_RESTORE_BACKGROUND);
			  
  add_to_gui_list(ID_LABEL, pBuf);
  
  pNetIncomeValue = pBuf;
  
  w = MAX(w, pBuf->size.w);
  h += pBuf->size.h;

  /* gov and taxrate */
  my_snprintf(cBuf, sizeof(cBuf), _("%s max rate : %d%%"),
    government_name_translation(pGov),
    get_player_bonus(client.conn.playing, EFT_MAX_RATES));
  copy_chars_to_string16(pStr2, cBuf);
  pMaxRateText = create_text_surf_from_str16(pStr2);
  
  FREESTRING16(pStr2);
  
  /* ------------------------- */
  /* lux rate */
  
  /* lux rate lock */
  my_snprintf(cBuf, sizeof(cBuf), _("Lock"));
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= TTF_STYLE_BOLD;

  pBuf = create_checkbox(pWindow->dst, 
      		(SDL_Client_Flags & CF_CHANGE_TAXRATE_LUX_BLOCK),
      		(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL));
  set_new_checkbox_theme(pBuf, pTheme->LOCK_Icon, pTheme->UNLOCK_Icon);
  pBuf->string16 = pStr;
  pBuf->action = toggle_block_callback;
  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_LUX_BLOCK_CHECKBOX, pBuf);

  w2 = adj_size(10) + pBuf->size.w;  

  /* lux rate slider */
  pBuf = create_horizontal(pTheme->Horiz, pWindow->dst, adj_size(30),
			(WF_FREE_DATA | WF_RESTORE_BACKGROUND));

  pBuf->action = horiz_taxrate_callback;
  pBuf->data.ptr = fc_calloc(1, sizeof(int));
  *(int *)pBuf->data.ptr = client.conn.playing->economic.luxury;
  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_LUX_SCROLLBAR, pBuf);
  
  w2 += adj_size(184);  
  
  /* lux rate iconlabel */
  
  /* it is important to leave 1 space at ending of this string */
  my_snprintf(cBuf, sizeof(cBuf), "%d%% ", client.conn.playing->economic.luxury);
  pStr = create_str16_from_char(cBuf, adj_font(11));
  pStr->style |= TTF_STYLE_BOLD;

  pBuf = create_iconlabel(pIcons->pBIG_Luxury, pWindow->dst, pStr,
					      WF_RESTORE_BACKGROUND);
  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_LUX_LABEL, pBuf);
  
  w2 += (adj_size(5) + pBuf->size.w + adj_size(10));
  
  /* ------------------------- */
  /* science rate */

  /* science rate lock */
  my_snprintf(cBuf, sizeof(cBuf), _("Lock"));
  pStr = create_str16_from_char(cBuf, adj_font(10));
  pStr->style |= TTF_STYLE_BOLD;

  pBuf = create_checkbox(pWindow->dst,
	      (SDL_Client_Flags & CF_CHANGE_TAXRATE_SCI_BLOCK),
      		(WF_RESTORE_BACKGROUND|WF_WIDGET_HAS_INFO_LABEL));

  set_new_checkbox_theme(pBuf, pTheme->LOCK_Icon, pTheme->UNLOCK_Icon);
    
  pBuf->string16 = pStr;
  pBuf->action = toggle_block_callback;
  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_SCI_BLOCK_CHECKBOX, pBuf);
  
  /* science rate slider */
  pBuf = create_horizontal(pTheme->Horiz, pWindow->dst, adj_size(30),
				(WF_FREE_DATA | WF_RESTORE_BACKGROUND));

  pBuf->action = horiz_taxrate_callback;
  pBuf->data.ptr = fc_calloc(1, sizeof(int));
  *(int *)pBuf->data.ptr = client.conn.playing->economic.science;
  
  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_SCI_SCROLLBAR, pBuf);
  
  /* science rate iconlabel */
  /* it is important to leave 1 space at ending of this string */
  my_snprintf(cBuf, sizeof(cBuf), "%d%% ", client.conn.playing->economic.science);
  pStr = create_str16_from_char(cBuf, adj_font(11));
  pStr->style |= TTF_STYLE_BOLD;

  pBuf = create_iconlabel(pIcons->pBIG_Colb, pWindow->dst, pStr,
					      WF_RESTORE_BACKGROUND);

  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_SCI_LABEL, pBuf);
  
  /* ---- */
  
  my_snprintf(cBuf, sizeof(cBuf), _("Update"));
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pBuf = create_themeicon_button(pTheme->Small_OK_Icon, pWindow->dst, pStr, 0);
  pBuf->action = apply_taxrates_callback;
  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_OK_BUTTON, pBuf);
  
  /* ---- */
  
  my_snprintf(cBuf, sizeof(cBuf), _("Close Dialog (Esc)"));
  pStr = create_str16_from_char(cBuf, adj_font(12));
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
                                  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = pStr;
  pBuf->action = exit_economy_dialog_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_CHANGE_TAXRATE_DLG_CANCEL_BUTTON, pBuf);
  
  h += adj_size(5);
  
  /* ------------------------- */
  pLast = pBuf;
  if(entries_used > 0) {
    
    /* Create Imprv Background Icon */
    pBackground = create_surf_alpha(adj_size(116), adj_size(116), SDL_SWSURFACE);
    
    SDL_FillRect(pBackground, NULL, map_rgba(pBackground->format, bg_color));
    putframe(pBackground, 0, 0, pBackground->w - 1, pBackground->h - 1,
        map_rgba(pBackground->format, *get_game_colorRGB(COLOR_THEME_ECONOMYDLG_FRAME)));
    
    pStr = create_string16(NULL, 0, adj_font(10));
    pStr->style |= (SF_CENTER|TTF_STYLE_BOLD);
    pStr->bgcol = (SDL_Color) {0, 0, 0, 0};
  
    for (i = 0; i < entries_used; i++) {
      struct improvement_entry *p = &entries[i];
      struct impr_type *pImprove = p->type;

      pSurf = crop_rect_from_surface(pBackground, NULL);
      
      my_snprintf(cBuf, sizeof(cBuf), "%s", improvement_name_translation(pImprove));
      
      copy_chars_to_string16(pStr, cBuf);
      pStr->style |= TTF_STYLE_BOLD;
      pText_Name = create_text_surf_smaller_that_w(pStr, pSurf->w - adj_size(4));
            
      my_snprintf(cBuf, sizeof(cBuf), "%s %d\n%s %d",
			_("Built"), p->count, _("U Total"),p->total_cost);
      copy_chars_to_string16(pStr, cBuf);
      pStr->style &= ~TTF_STYLE_BOLD;
  
      pText = create_text_surf_from_str16(pStr);
      
      /*-----------------*/
  
      pZoom = get_building_surface(pImprove);
      pZoom = zoomSurface(pZoom, DEFAULT_ZOOM * ((float)54 / pZoom->w), DEFAULT_ZOOM * ((float)54 / pZoom->w), 1);

      dst.x = (pSurf->w - pZoom->w)/2;
      dst.y = (pSurf->h/2 - pZoom->h)/2;
      alphablit(pZoom, NULL, pSurf, &dst);
      dst.y += pZoom->h;
      FREESURFACE(pZoom);
  
      dst.x = (pSurf->w - pText_Name->w)/2;
      dst.y += ((pSurf->h - dst.y) -
	      (pText_Name->h + (pIcons->pBIG_Coin->h + 2) + pText->h))/2;
      alphablit(pText_Name, NULL, pSurf, &dst);
      
      dst.y += pText_Name->h;
      if(p->cost) {
	dst.x = (pSurf->w - p->cost * (pIcons->pBIG_Coin->w + 1))/2;
        for(count = 0; count < p->cost; count++) {
	  alphablit(pIcons->pBIG_Coin, NULL, pSurf, &dst);
	  dst.x += pIcons->pBIG_Coin->w + 1;
        }
      } else {
        
        if(!is_wonder(pImprove)) {
	  copy_chars_to_string16(pStr, _("Nation"));
	} else {
	  copy_chars_to_string16(pStr, _("Wonder"));
	}
        /*pStr->style &= ~TTF_STYLE_BOLD;*/
  
        pZoom = create_text_surf_from_str16(pStr);
	
	dst.x = (pSurf->w - pZoom->w)/2;
	alphablit(pZoom, NULL, pSurf, &dst);
	FREESURFACE(pZoom);
      }
      
      dst.y += (pIcons->pBIG_Coin->h + adj_size(2));
      dst.x = (pSurf->w - pText->w)/2;
      alphablit(pText, NULL, pSurf, &dst);
  
      FREESURFACE(pText);
      FREESURFACE(pText_Name);
            
      pBuf = create_icon2(pSurf, pWindow->dst,
    		(WF_RESTORE_BACKGROUND|WF_FREE_THEME|WF_FREE_DATA));
      
      set_wstate(pBuf, FC_WS_NORMAL);

      pBuf->data.cont = fc_calloc(1, sizeof(struct CONTAINER));
      pBuf->data.cont->id0 = improvement_number(p->type);
      pBuf->data.cont->id1 = p->count;
      pBuf->action = popup_sell_impv_callback;
      
      add_to_gui_list(MAX_ID - i, pBuf);
      
      if(i > (TARGETS_ROW * TARGETS_COL - 1)) {
        set_wflag(pBuf, WF_HIDDEN);
      }
      
    }
  
    FREESTRING16(pStr);
    FREESURFACE(pBackground);
    
    pEconomyDlg->pEndActiveWidgetList = pLast->prev;
    pEconomyDlg->pBeginWidgetList = pBuf;
    pEconomyDlg->pBeginActiveWidgetList = pEconomyDlg->pBeginWidgetList;
    
    if(entries_used > (TARGETS_ROW * TARGETS_COL)) {
      pEconomyDlg->pActiveWidgetList = pEconomyDlg->pEndActiveWidgetList;
      count = create_vertical_scrollbar(pEconomyDlg,
		    		TARGETS_COL, TARGETS_ROW, TRUE, TRUE);
      h += (TARGETS_ROW * pBuf->size.h + adj_size(10));
    } else {
      count = 0;
      if(entries_used > TARGETS_COL) {
	h += pBuf->size.h;
      }
      h += (adj_size(10) + pBuf->size.h);
    }
    count = TARGETS_COL * pBuf->size.w + count;  
  } else {
    pEconomyDlg->pBeginWidgetList = pBuf;
    h += adj_size(10);
    count = 0;
  }
  
  area.w = MAX(area.w, MAX(adj_size(10) + w3 + w + w2, count));
  area.h = h;

  pBackground = theme_get_background(theme, BACKGROUND_ECONOMYDLG);
  if(resize_window(pWindow, pBackground, NULL,
                   (pWindow->size.w - pWindow->area.w) + area.w,
                   (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pBackground);
  }

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);

  /* "Treasury" value label */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + adj_size(10) + pTreasuryText->w;
  pBuf->size.y = area.y + adj_size(5);
  
  w = pTreasuryText->w + pBuf->size.w;
  h = pBuf->size.h;
  
  /* "Tax Rate" value label */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + adj_size(10) + pTaxRateText->w;
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;
  
  w = MAX(w, pTaxRateText->w + pBuf->size.w);
  h += pBuf->size.h;

  /* "Total Income" value label */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + adj_size(10) + pTotalIncomeText->w;
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;

  w = MAX(w, pTotalIncomeText->w + pBuf->size.w);
  h += pBuf->size.h;

 /* "Total Cost" value label */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + adj_size(10) + pTotalCostText->w;
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;

  w = MAX(w, pTotalCostText->w + pBuf->size.w);
  h += pBuf->size.h;

  /* "Net Income" value label */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + adj_size(10) + pNetIncomeText->w;
  pBuf->size.y = pBuf->next->size.y + pBuf->next->size.h;

  w = MAX(w, pNetIncomeText->w + pBuf->size.w);
  h += pBuf->size.h;
  
  /* Backgrounds */
  dst.x = area.x;
  dst.y = area.y;
  dst.w = area.w;
  dst.h = h + adj_size(15);
  h = dst.h;
  
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color2);
  
  putframe(pWindow->theme, dst.x, dst.y, dst.x + dst.w - 1, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_ECONOMYDLG_FRAME)));
  
  /* draw statical strings */
  dst.x = area.x + adj_size(10);
  dst.y = area.y + adj_size(5);
  
  /* "Treasury */
  alphablit(pTreasuryText, NULL, pWindow->theme, &dst);
  dst.y += pTreasuryText->h;  
  FREESURFACE(pTreasuryText);

  /* Tax Rate */
  alphablit(pTaxRateText, NULL, pWindow->theme, &dst);
  dst.y += pTaxRateText->h;
  FREESURFACE(pTaxRateText);

  /* Total Income */
  alphablit(pTotalIncomeText, NULL, pWindow->theme, &dst);
  dst.y += pTotalIncomeText->h;
  FREESURFACE(pTotalIncomeText);

  /* Total Cost */
  alphablit(pTotalCostText, NULL, pWindow->theme, &dst);
  dst.y += pTotalCostText->h;
  FREESURFACE(pTotalCostText);
  
  /* Net Income */
  alphablit(pNetIncomeText, NULL, pWindow->theme, &dst);
  dst.y += pNetIncomeText->h;
  FREESURFACE(pNetIncomeText);

  /* gov and taxrate */
  dst.x = area.x + adj_size(10) + w + ((area.w - (w + adj_size(10)) - pMaxRateText->w) / 2);
  dst.y = area.y + adj_size(5);
	
  alphablit(pMaxRateText, NULL, pWindow->theme, &dst);
  dst.y += (pMaxRateText->h + 1);
  FREESURFACE(pMaxRateText);
  
  /* Luxuries Horizontal Scrollbar Background */
  dst.x = area.x + adj_size(10) + w + (area.w - (w + adj_size(10)) - adj_size(184)) / 2;
  dst.w = adj_size(184);
  dst.h = pTheme->Horiz->h - adj_size(2);
  
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color3);
  
  putframe(pWindow->theme, dst.x, dst.y, dst.x + dst.w - 1, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_ECONOMYDLG_FRAME)));
  
  /* lock icon */
  pBuf = pBuf->prev;
  pBuf->size.x = dst.x - pBuf->size.w;
  pBuf->size.y = dst.y - adj_size(2);
  
  /* lux scrollbar */
  pBuf = pBuf->prev;
  pBuf->size.x = dst.x + adj_size(2) + (client.conn.playing->economic.luxury * 3) / 2;
  pBuf->size.y = dst.y -1;
  
  /* lux rate */
  pBuf = pBuf->prev;
  pBuf->size.x = dst.x + dst.w + adj_size(5);
  pBuf->size.y = dst.y + 1;
  
  
  /* Science Horizontal Scrollbar Background */
  dst.y += pTheme->Horiz->h + 1;
  SDL_FillRectAlpha(pWindow->theme, &dst, &bg_color3);
  
  putframe(pWindow->theme, dst.x, dst.y, dst.x + dst.w - 1, dst.y + dst.h - 1,
    map_rgba(pWindow->theme->format, *get_game_colorRGB(COLOR_THEME_ECONOMYDLG_FRAME)));
  
  /* science lock icon */
  pBuf = pBuf->prev;
  pBuf->size.x = dst.x - pBuf->size.w;
  pBuf->size.y = dst.y - adj_size(2);
  
  /* science scrollbar */
  pBuf = pBuf->prev;
  pBuf->size.x = dst.x + adj_size(2) + (client.conn.playing->economic.science * 3) / 2;
  pBuf->size.y = dst.y -1;
  
  /* science rate */
  pBuf = pBuf->prev;
  pBuf->size.x = dst.x + dst.w + adj_size(5);
  pBuf->size.y = dst.y + 1;

  /* update */
  pBuf = pBuf->prev;
  pBuf->size.x = dst.x + (dst.w - pBuf->size.w) / 2;
  pBuf->size.y = dst.y + dst.h + adj_size(3);
    
  /* cancel */
  pBuf = pBuf->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2); 
  /* ------------------------------- */
  
  if (entries_used > 0) {
    setup_vertical_widgets_position(TARGETS_COL,
	area.x,
	area.y + h,
	  0, 0, pEconomyDlg->pBeginActiveWidgetList,
			  pEconomyDlg->pEndActiveWidgetList);
    if (pEconomyDlg->pScroll) {
      setup_vertical_scrollbar_area(pEconomyDlg->pScroll,
	area.x + area.w - 1,
    	area.y + h,
    	area.h - h - 1, TRUE);
    }
  }
  
  /* ------------------------ */
  redraw_group(pEconomyDlg->pBeginWidgetList, pWindow, 0);
  widget_mark_dirty(pWindow);
  flush_dirty();
}

/* ===================================================================== */
/* ======================== Science Report ============================= */
/* ===================================================================== */
static struct SMALL_DLG *pScienceDlg = NULL;

static struct ADVANCED_DLG *pChangeTechDlg = NULL;

SDL_Surface * create_sellect_tech_icon(SDL_String16 *pStr, Tech_type_id tech_id, enum tech_info_mode mode)
{
  struct unit_type *pUnit = NULL;
  SDL_Surface *pSurf, *pText, *pTmp, *pTmp2;
  SDL_Surface *Surf_Array[10], **pBuf_Array;
  SDL_Rect dst;
  SDL_Color color;
  int w, h;

  color = *get_tech_color(tech_id);
  switch (mode)
  {
    case SMALL_MODE:
      h = adj_size(40);
      w = adj_size(135);
      break;
    case MED_MODE:
      color = *get_game_colorRGB(COLOR_THEME_SCIENCEDLG_MED_TECHICON_BG);
    default:
      h = adj_size(200);
      w = adj_size(100);
    break;
  }

  pText = create_text_surf_smaller_that_w(pStr, adj_size(100 - 4));
  
  /* create label surface */
  pSurf = create_surf_alpha(w, h, SDL_SWSURFACE);
  
  if (tech_id == get_player_research(client.conn.playing)->researching)
  {
    color.unused = 180;
  } else {
    color.unused = 128;
  }

  SDL_FillRect(pSurf, NULL, map_rgba(pSurf->format, color));
  putframe(pSurf, 0,0, pSurf->w - 1, pSurf->h - 1,
    map_rgba(pSurf->format, *get_game_colorRGB(COLOR_THEME_SCIENCEDLG_FRAME)));
  
  pTmp = get_tech_icon(tech_id);
  
  if (mode == SMALL_MODE)
  {
    /* draw name tech text */ 
    dst.x = adj_size(35) + (pSurf->w - pText->w - adj_size(35)) / 2;
    dst.y = (pSurf->h - pText->h) / 2;
    alphablit(pText, NULL, pSurf, &dst);
    FREESURFACE(pText);
    
    /* draw tech icon */
    pText = ResizeSurface(pTmp, adj_size(25), adj_size(25), 1);
    dst.x = (adj_size(35) - pText->w) / 2;
    dst.y = (pSurf->h - pText->h) / 2;
    alphablit(pText, NULL, pSurf, &dst);
    FREESURFACE(pText);
    
  } else {
  
    /* draw name tech text */ 
    dst.x = (pSurf->w - pText->w) / 2;
    dst.y = adj_size(20);
    alphablit(pText, NULL, pSurf, &dst);
    dst.y += pText->h + adj_size(10);
    FREESURFACE(pText);
    
    /* draw tech icon */
    dst.x = (pSurf->w - pTmp->w) / 2;
    alphablit(pTmp, NULL, pSurf, &dst);
    dst.y += pTmp->w + adj_size(10);

    /* fill array with iprvm. icons */
    w = 0;
    improvement_iterate(pImprove) {
      requirement_vector_iterate(&pImprove->reqs, preq) {
        if (VUT_ADVANCE == preq->source.kind
         && advance_number(preq->source.value.advance) == tech_id) {
          pTmp2 = get_building_surface(pImprove);
          Surf_Array[w++] = zoomSurface(pTmp2, DEFAULT_ZOOM * ((float)36 / pTmp2->w), DEFAULT_ZOOM * ((float)36 / pTmp2->w), 1);
        }
      } requirement_vector_iterate_end;
    } improvement_iterate_end;

    if (w) {
      if (w >= 2) {
        dst.x = (pSurf->w - 2 * Surf_Array[0]->w) / 2;
      } else {
        dst.x = (pSurf->w - Surf_Array[0]->w) / 2;
      }

      /* draw iprvm. icons */
      pBuf_Array = Surf_Array;
      h = 0;
      while (w) {
        alphablit(*pBuf_Array, NULL, pSurf, &dst);
        dst.x += (*pBuf_Array)->w;
        w--;
        h++;
        if (!(h % 2)) {
          if (w >= 2) {
            dst.x = (pSurf->w - 2 * (*pBuf_Array)->w) / 2;
          } else {
            dst.x = (pSurf->w - (*pBuf_Array)->w) / 2;
          }
          dst.y += (*pBuf_Array)->h;
          h = 0;
        }	/* h == 2 */
        pBuf_Array++;
      }	/* while */
      dst.y += Surf_Array[0]->h + adj_size(5);
    } /* if (w) */
  /* -------------------------------------------------------- */
    w = 0;
    unit_type_iterate(un) {
      pUnit = un;
      if (advance_number(pUnit->require_advance) == tech_id) {
        Surf_Array[w++] = adj_surf(get_unittype_surface(un));
      }
    } unit_type_iterate_end;

    if (w) {
      if (w < 2) {
        /* w == 1 */
        if (Surf_Array[0]->w > 64) {
	  float zoom = DEFAULT_ZOOM * (64.0 / Surf_Array[0]->w);
	  SDL_Surface *pZoomed = zoomSurface(Surf_Array[0], zoom, zoom, 1);
	
	  dst.x = (pSurf->w - pZoomed->w) / 2;
	  alphablit(pZoomed, NULL, pSurf, &dst);
	  FREESURFACE(pZoomed);
        } else {
          dst.x = (pSurf->w - Surf_Array[0]->w) / 2;
          alphablit(Surf_Array[0], NULL, pSurf, &dst);
        }
      } else {
        float zoom;
      
        if (w > 2) {
	  zoom = DEFAULT_ZOOM * (38.0 / Surf_Array[0]->w);
        } else {
	  zoom = DEFAULT_ZOOM * (45.0 / Surf_Array[0]->w);
        }
        dst.x = (pSurf->w - (Surf_Array[0]->w * 2) * zoom - 2) / 2;
        pBuf_Array = Surf_Array;
        h = 0;
        while (w) {
	  SDL_Surface *pZoomed = zoomSurface((*pBuf_Array), zoom, zoom, 1);
          alphablit(pZoomed, NULL, pSurf, &dst);
          dst.x += pZoomed->w + 2;
          w--;
          h++;
          if (!(h % 2)) {
            if (w >= 2) {
	      dst.x = (pSurf->w - 2 * pZoomed->w - 2 ) / 2;
            } else {
	      dst.x = (pSurf->w - pZoomed->w) / 2;
            }
	    dst.y += pZoomed->h + 2;
            h = 0;
          }	/* h == 2 */
          pBuf_Array++;
	  FREESURFACE(pZoomed);
        }	/* while */
      } /* w > 1 */
    }/* if (w) */
  }
  
  FREESURFACE(pTmp);
  
  return pSurf;
}

/**************************************************************************
  enable science dialog group ( without window )
**************************************************************************/
static void enable_science_dialog(void)
{
  set_group_state(pScienceDlg->pBeginWidgetList,
		     pScienceDlg->pEndWidgetList->prev, FC_WS_NORMAL);
}

/**************************************************************************
  disable science dialog group ( without window )
**************************************************************************/
static void disable_science_dialog(void)
{
  set_group_state(pScienceDlg->pBeginWidgetList,
		     pScienceDlg->pEndWidgetList->prev, FC_WS_DISABLED);
}

/**************************************************************************
  Update the science report.
**************************************************************************/
void science_dialog_update(void)
{
  SDL_Color bg_color = {255, 255, 255, 136};
  
  if(pScienceDlg && !is_report_dialogs_frozen()) {
    char cBuf[128];
    SDL_String16 *pStr;
    SDL_Surface *pSurf;
    SDL_Surface *pColb_Surface = pIcons->pBIG_Colb;
    int step, i, cost;
    SDL_Rect dest;
    struct unit_type *pUnit;
    
    struct widget *pChangeResearchButton;
    struct widget *pChangeResearchGoalButton;
    SDL_Rect area;
    
    struct widget *pWindow = pScienceDlg->pEndWidgetList;

    area = pWindow->area;
    pChangeResearchButton = pWindow->prev;
    pChangeResearchGoalButton = pWindow->prev->prev;
    
    if (A_UNSET != get_player_research(client.conn.playing)->researching) {
      cost = total_bulbs_required(client.conn.playing);
    } else {
      cost = 0;
    }        
    
    /* update current research icons */
    FREESURFACE(pChangeResearchButton->theme);
    pChangeResearchButton->theme = get_tech_icon(get_player_research(client.conn.playing)->researching);
    FREESURFACE(pChangeResearchGoalButton->theme);
    pChangeResearchGoalButton->theme = get_tech_icon(get_player_research(client.conn.playing)->tech_goal);
    
    /* redraw Window */
    widget_redraw(pWindow);
  
    /* ------------------------------------- */

    /* research progress text */
    pStr = create_str16_from_char(science_dialog_text(), adj_font(12));
    pStr->style |= SF_CENTER;
    pStr->fgcol = *get_game_colorRGB(COLOR_THEME_SCIENCEDLG_TEXT);
  
    pSurf = create_text_surf_from_str16(pStr);
      
    dest.x = area.x + (area.w - pSurf->w) / 2;
    dest.y = area.y + adj_size(2);
    alphablit(pSurf, NULL, pWindow->dst->surface, &dest);

    dest.y += pSurf->h + adj_size(4);

    FREESURFACE(pSurf);

    dest.x = area.x + adj_size(16);
    
    /* separator */
    putline(pWindow->dst->surface,
      dest.x, dest.y,
      (area.x + area.w - adj_size(16)), dest.y,
      map_rgba(pWindow->dst->surface->format, *get_game_colorRGB(COLOR_THEME_SCIENCEDLG_FRAME)));

    dest.y += adj_size(6);
    
    widget_set_position(pChangeResearchButton, dest.x, dest.y + adj_size(18));
    
    /* current research text */
    my_snprintf(cBuf, sizeof(cBuf), "%s: %s",
      advance_name_researching(client.conn.playing),
      get_science_target_text(NULL));

    copy_chars_to_string16(pStr, cBuf);
    
    pSurf = create_text_surf_from_str16(pStr);
    
    dest.x = pChangeResearchButton->size.x + pChangeResearchButton->size.w + adj_size(10);
    
    alphablit(pSurf, NULL, pWindow->dst->surface, &dest);

    dest.y += pSurf->h + adj_size(4);
    
    FREESURFACE(pSurf);

    /* progress bar */
    dest.w = cost * pColb_Surface->w;
    step = pColb_Surface->w;
    if (dest.w > (area.w - dest.x - adj_size(16))) {
      dest.w = (area.w - dest.x - adj_size(16));
      step = ((area.w - dest.x - adj_size(16)) - pColb_Surface->w) / (cost - 1);

      if (step == 0) {
        step = 1;
      }
    }

    dest.h = pColb_Surface->h + adj_size(4);
    SDL_FillRectAlpha(pWindow->dst->surface, &dest, &bg_color);
  
    putframe(pWindow->dst->surface, dest.x - 1, dest.y - 1, dest.x + dest.w,
      dest.y + dest.h, map_rgba(pWindow->dst->surface->format, *get_game_colorRGB(COLOR_THEME_SCIENCEDLG_FRAME)));
  
    if (cost > adj_size(286))
    {
      cost =
        adj_size(286) * ((float) get_player_research(client.conn.playing)->bulbs_researched / cost);
    }
    else
    {
      cost =
        (float)cost * ((float)get_player_research(client.conn.playing)->bulbs_researched/cost);
    }
  
    dest.y += adj_size(2);
    for (i = 0; i < cost; i++) {
      alphablit(pColb_Surface, NULL, pWindow->dst->surface, &dest);
      dest.x += step;
    }

    /* improvement icons */
    
    dest.y += dest.h + adj_size(4);
    dest.x = pChangeResearchButton->size.x + pChangeResearchButton->size.w + adj_size(10);

    /* buildings */
    improvement_iterate(pImprove) {
      requirement_vector_iterate(&pImprove->reqs, preq) {
        if (VUT_ADVANCE == preq->source.kind
         && advance_number(preq->source.value.advance) == get_player_research(client.conn.playing)->researching) {
          pSurf = adj_surf(get_building_surface(pImprove));
          alphablit(pSurf, NULL, pWindow->dst->surface, &dest);
          dest.x += pSurf->w + 1;
        }
      } requirement_vector_iterate_end;
    } improvement_iterate_end;

    dest.x += adj_size(5);

    /* units */
    unit_type_iterate(un) {
      pUnit = un;
      if (advance_number(pUnit->require_advance) == get_player_research(client.conn.playing)->researching) {
	if (get_unittype_surface(un)->w > 64) {
	  float zoom = DEFAULT_ZOOM * (64.0 / get_unittype_surface(un)->w);
	  pSurf = zoomSurface(get_unittype_surface(un), zoom, zoom, 1);
	  alphablit(pSurf, NULL, pWindow->dst->surface, &dest);
          dest.x += pSurf->w + adj_size(2);          
	  FREESURFACE(pSurf);
	} else {
          pSurf = adj_surf(get_unittype_surface(un));
          alphablit(pSurf, NULL, pWindow->dst->surface, &dest);
          dest.x += pSurf->w + adj_size(2);
	}
      }
    } unit_type_iterate_end;
  
    /* -------------------------------- */
    /* draw separator line */
    dest.x = area.x + adj_size(16);
    dest.y += adj_size(48) + adj_size(6);

    putline(pWindow->dst->surface,
      dest.x, dest.y,
      (area.x + area.w - adj_size(16)), dest.y,
      map_rgba(pWindow->dst->surface->format, *get_game_colorRGB(COLOR_THEME_SCIENCEDLG_FRAME)));
    
    dest.x = pChangeResearchButton->size.x;
    dest.y += adj_size(6);

    widget_set_position(pChangeResearchGoalButton, dest.x, dest.y + adj_size(16));    
    
    /* -------------------------------- */
    
    /* Goals */
    if (A_UNSET != get_player_research(client.conn.playing)->tech_goal)
    {
      /* current goal text */
      my_snprintf(cBuf, sizeof(cBuf), "%s",
        advance_name_for_player(client.conn.playing,
          get_player_research(client.conn.playing)->tech_goal));
      
      copy_chars_to_string16(pStr, cBuf);
      pSurf = create_text_surf_from_str16(pStr);
      
      dest.x = pChangeResearchGoalButton->size.x + pChangeResearchGoalButton->size.w + adj_size(10);
      alphablit(pSurf, NULL, pWindow->dst->surface, &dest);

      dest.y += pSurf->h;
      
      FREESURFACE(pSurf);
      
      copy_chars_to_string16(pStr, get_science_goal_text(get_player_research(client.conn.playing)->tech_goal));
      pSurf = create_text_surf_from_str16(pStr);
      
      dest.x = pChangeResearchGoalButton->size.x + pChangeResearchGoalButton->size.w + adj_size(10);
      alphablit(pSurf, NULL, pWindow->dst->surface, &dest);

      dest.y += pSurf->h + adj_size(6);
      
      FREESURFACE(pSurf);

      /* buildings */
      improvement_iterate(pImprove) {
	requirement_vector_iterate(&pImprove->reqs, preq) {  
          if (VUT_ADVANCE == preq->source.kind
           && advance_number(preq->source.value.advance) == get_player_research(client.conn.playing)->tech_goal) {
            pSurf = adj_surf(get_building_surface(pImprove));
            alphablit(pSurf, NULL, pWindow->dst->surface, &dest);
            dest.x += pSurf->w + 1;
        }
        } requirement_vector_iterate_end;
      } improvement_iterate_end;

      dest.x += adj_size(6);

      /* units */
      unit_type_iterate(un) {
        pUnit = un;
        if (advance_number(pUnit->require_advance) == get_player_research(client.conn.playing)->tech_goal) {
	  if (get_unittype_surface(un)->w > 64) {
	    float zoom = DEFAULT_ZOOM * (64.0 / get_unittype_surface(un)->w);
	    pSurf = zoomSurface(get_unittype_surface(un), zoom, zoom, 1);
	    alphablit(pSurf, NULL, pWindow->dst->surface, &dest);
            dest.x += pSurf->w + adj_size(2);
	    FREESURFACE(pSurf);
	  } else {
            pSurf = adj_surf(get_unittype_surface(un));
            alphablit(pSurf, NULL, pWindow->dst->surface, &dest);
            dest.x += pSurf->w + adj_size(2);
	  }
        }
      } unit_type_iterate_end;
    }
  
    /* -------------------------------- */
    widget_mark_dirty(pWindow);    
    redraw_group(pScienceDlg->pBeginWidgetList, pWindow->prev, 1);  
    flush_dirty();
  
    FREESTRING16(pStr);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void popdown_science_dialog()
{
  if(pScienceDlg) {
    popdown_window_group_dialog(pScienceDlg->pBeginWidgetList,
				  pScienceDlg->pEndWidgetList);
    FC_FREE(pScienceDlg);
    set_wstate(get_research_widget(), FC_WS_NORMAL);
    widget_redraw(get_research_widget());
    widget_mark_dirty(get_research_widget());
    flush_dirty();
  }
}

/**************************************************************************
  ...
**************************************************************************/
static int exit_change_tech_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (pChangeTechDlg) {
      popdown_window_group_dialog(pChangeTechDlg->pBeginWidgetList, 
                                  pChangeTechDlg->pEndWidgetList);
      FC_FREE(pChangeTechDlg->pScroll);
      FC_FREE(pChangeTechDlg);
      enable_science_dialog();
      if (pWidget) {
        flush_dirty();
      }
    }
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static int change_research_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    dsend_packet_player_research(&client.conn, (MAX_ID - pWidget->ID));
    exit_change_tech_dlg_callback(NULL);
  } else if (Main.event.button.button == SDL_BUTTON_MIDDLE) {
    popup_tech_info((MAX_ID - pWidget->ID));
  }
  return -1;
}

/**************************************************************************
  This function is used by change research and change goals dlgs.
**************************************************************************/
static int change_research_goal_dialog_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if(sellect_window_group_dialog(pChangeTechDlg->pBeginWidgetList, pWindow)) {
        widget_flush(pWindow);
    }
  }
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static void popup_change_research_dialog()
{
  struct widget *pBuf = NULL;
  struct widget *pWindow;
  SDL_String16 *pStr;
  SDL_Surface *pSurf;
  int max_col, max_row, col, i, count = 0, h;
  SDL_Rect area;

  if (is_future_tech(get_player_research(client.conn.playing)->researching)) {
    return;
  }
    
  advance_index_iterate(A_FIRST, i) {
    if (!player_invention_reachable(client.conn.playing, i)
     || TECH_PREREQS_KNOWN != player_invention_state(client.conn.playing, i)) {
      continue;
    }
    count++;
  } advance_index_iterate_end;
  
  if (count < 2) {
    return;
  }
  
  pChangeTechDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  
  pStr = create_str16_from_char(_("What should we focus on now?"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);
  pChangeTechDlg->pEndWidgetList = pWindow;
  set_wstate(pWindow, FC_WS_NORMAL);
  pWindow->action = change_research_goal_dialog_callback;
  
  add_to_gui_list(ID_SCIENCE_DLG_CHANGE_REASARCH_WINDOW, pWindow);
  
  area = pWindow->area;
  
  /* ------------------------- */
    /* exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = exit_change_tech_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_TERRAIN_ADV_DLG_EXIT_BUTTON, pBuf);

  /* ------------------------- */
  /* max col - 104 is sellect tech widget width */
  max_col = (Main.screen->w - (pWindow->size.w - pWindow->area.w) - adj_size(2)) / adj_size(104);
  /* max row - 204 is sellect tech widget height */
  max_row = (Main.screen->h - (pWindow->size.h - pWindow->area.h) - adj_size(2)) / adj_size(204);
  
  /* make space on screen for scrollbar */
  if (max_col * max_row < count) {
    max_col--;
  }

  if (count < max_col + 1) {
    col = count;
  } else {
    if (count < max_col + 3) {
      col = max_col - 2;
    } else {
      if (count < max_col + 5) {
        col = max_col - 1;
      } else {
        col = max_col;
      }
    }
  }
  
  pStr = create_string16(NULL, 0, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD | SF_CENTER);
  
  count = 0;
  h = col * max_row;
  advance_index_iterate(A_FIRST, i) {
    if (!player_invention_reachable(client.conn.playing, i)
     || TECH_PREREQS_KNOWN != player_invention_state(client.conn.playing, i)) {
      continue;
    }
    
    count++;
    
    copy_chars_to_string16(pStr, advance_name_translation(advance_by_number(i)));
    pSurf = create_sellect_tech_icon(pStr, i, MED_MODE);
    pBuf = create_icon2(pSurf, pWindow->dst,
      		WF_FREE_THEME | WF_RESTORE_BACKGROUND);

    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = change_research_callback;

    add_to_gui_list(MAX_ID - i, pBuf);
    
    if (count > h) {
      set_wflag(pBuf, WF_HIDDEN);
    }
    
  } advance_index_iterate_end;
  
  FREESTRING16(pStr);
  
  pChangeTechDlg->pBeginWidgetList = pBuf;
  pChangeTechDlg->pBeginActiveWidgetList = pChangeTechDlg->pBeginWidgetList;
  pChangeTechDlg->pEndActiveWidgetList = pChangeTechDlg->pEndWidgetList->prev->prev;
  
  /* -------------------------------------------------------------- */
  
  i = 0;
  if (count > col) {
    count = (count + (col - 1)) / col;
    if (count > max_row) {
      pChangeTechDlg->pActiveWidgetList = pChangeTechDlg->pEndActiveWidgetList;
      count = max_row;
      i = create_vertical_scrollbar(pChangeTechDlg, col, count, TRUE, TRUE);  
    }
  } else {
    count = 1;
  }

  disable_science_dialog();
  
  area.w = MAX(area.w, (col * pBuf->size.w + adj_size(2) + i));
  area.h = MAX(area.h, count * pBuf->size.h + adj_size(2));
  
  /* alloca window theme and win background buffer */
  pSurf = theme_get_background(theme, BACKGROUND_CHANGERESEARCHDLG);
  resize_window(pWindow, pSurf, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  FREESURFACE(pSurf);

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);

    /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  setup_vertical_widgets_position(col, area.x + 1,
		  area.y, 0, 0,
		  pChangeTechDlg->pBeginActiveWidgetList,
  		  pChangeTechDlg->pEndActiveWidgetList);
    
  if(pChangeTechDlg->pScroll) {
    setup_vertical_scrollbar_area(pChangeTechDlg->pScroll,
	area.x + area.w, area.y,
    	area.h, TRUE);
  }

  redraw_group(pChangeTechDlg->pBeginWidgetList, pWindow, FALSE);

  widget_flush(pWindow);
}

/**************************************************************************
  ...
**************************************************************************/
static int change_research_goal_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    dsend_packet_player_tech_goal(&client.conn, (MAX_ID - pWidget->ID));

    exit_change_tech_dlg_callback(NULL);
    
   /* Following is to make the menu go back to the current goal;
   * there may be a better way to do this?  --dwp */
    science_dialog_update();
  } else if (Main.event.button.button == SDL_BUTTON_MIDDLE) {
    popup_tech_info((MAX_ID - pWidget->ID));
  } 
  return -1;
}

/**************************************************************************
  ...
**************************************************************************/
static void popup_change_research_goal_dialog()
{
  struct widget *pBuf = NULL;
  struct widget *pWindow;
  SDL_String16 *pStr;
  SDL_Surface *pSurf;
  char cBuf[128];
  int max_col, max_row, col, i, count = 0, h , num;
  SDL_Rect area;

  /* collect all techs which are reachable in under 11 steps
   * hist will hold afterwards the techid of the current choice
   */
  advance_index_iterate(A_FIRST, i) {
    if (player_invention_reachable(client.conn.playing, i)
        && TECH_KNOWN != player_invention_state(client.conn.playing, i)
	&& (11 > num_unknown_techs_for_goal(client.conn.playing, i)
	    || i == get_player_research(client.conn.playing)->tech_goal)) {
      count++;
    }
  } advance_index_iterate_end;
  
  if (count < 1) {
    return;
  }
  
  pChangeTechDlg = fc_calloc(1, sizeof(struct ADVANCED_DLG));
  
  pStr = create_str16_from_char(_("Select target :"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pStr, 0);
  pChangeTechDlg->pEndWidgetList = pWindow;
  set_wstate(pWindow, FC_WS_NORMAL);
  pWindow->action = change_research_goal_dialog_callback;
  
  add_to_gui_list(ID_SCIENCE_DLG_CHANGE_GOAL_WINDOW, pWindow);
  
  area = pWindow->area;
  
  /* ------------------------- */
    /* exit button */
  pBuf = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
  			  WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = exit_change_tech_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_SCIENCE_DLG_CHANGE_GOAL_CANCEL_BUTTON, pBuf);

  /* ------------------------- */
  /* max col - 104 is goal tech widget width */
  max_col = (Main.screen->w - (pWindow->size.w - pWindow->area.w) - adj_size(2)) / adj_size(104);
  
  /* max row - 204 is goal tech widget height */
  max_row = (Main.screen->h - (pWindow->size.h - pWindow->area.h) - adj_size(2)) / adj_size(204);

  /* make space on screen for scrollbar */
  if (max_col * max_row < count) {
    max_col--;
  }
  
  if (count < max_col + 1) {
    col = count;
  } else {
    if (count < max_col + 3) {
      col = max_col - 2;
    } else {
      if (count < max_col + 5) {
        col = max_col - 1;
      } else {
        col = max_col;
      }
    }
  }
  
  pStr = create_string16(NULL, 0, adj_font(10));
  pStr->style |= (TTF_STYLE_BOLD | SF_CENTER);
  
  /* collect all techs which are reachable in under 11 steps
   * hist will hold afterwards the techid of the current choice
   */
  count = 0;
  h = col * max_row;
  advance_index_iterate(A_FIRST, i) {
    if (player_invention_reachable(client.conn.playing, i)
        && TECH_KNOWN != player_invention_state(client.conn.playing, i)
	&& (11 > (num = num_unknown_techs_for_goal(client.conn.playing, i))
	    || i == get_player_research(client.conn.playing)->tech_goal)) {

      count++;
      my_snprintf(cBuf, sizeof(cBuf), "%s\n%d %s",
                  advance_name_translation(advance_by_number(i)),
                  num,
                  PL_("step", "steps", num));
      copy_chars_to_string16(pStr, cBuf);
      pSurf = create_sellect_tech_icon(pStr, i, FULL_MODE);
      pBuf = create_icon2(pSurf, pWindow->dst,
      		WF_FREE_THEME | WF_RESTORE_BACKGROUND);

      set_wstate(pBuf, FC_WS_NORMAL);
      pBuf->action = change_research_goal_callback;

      add_to_gui_list(MAX_ID - i, pBuf);
    
      if (count > h) {
        set_wflag(pBuf, WF_HIDDEN);
      }
    }
  } advance_index_iterate_end;
  
  FREESTRING16(pStr);
  
  pChangeTechDlg->pBeginWidgetList = pBuf;
  pChangeTechDlg->pBeginActiveWidgetList = pChangeTechDlg->pBeginWidgetList;
  pChangeTechDlg->pEndActiveWidgetList = pChangeTechDlg->pEndWidgetList->prev->prev;
  
  /* -------------------------------------------------------------- */
  
  i = 0;
  if (count > col) {
    count = (count + (col-1)) / col;
    if (count > max_row) {
      pChangeTechDlg->pActiveWidgetList = pChangeTechDlg->pEndActiveWidgetList;
      count = max_row;
      i = create_vertical_scrollbar(pChangeTechDlg, col, count, TRUE, TRUE);  
    }
  } else {
    count = 1;
  }

  disable_science_dialog();
  
  area.w = MAX(area.w, (col * pBuf->size.w + adj_size(2) + i));
  area.h = MAX(area.h, count * pBuf->size.h + adj_size(2));

  /* alloca window theme and win background buffer */
  pSurf = theme_get_background(theme, BACKGROUND_CHANGERESEARCHDLG);
  resize_window(pWindow, pSurf, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);
  FREESURFACE(pSurf);

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);

  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);
  
  setup_vertical_widgets_position(col, area.x + 1,
		  area.y, 0, 0,
		  pChangeTechDlg->pBeginActiveWidgetList,
  		  pChangeTechDlg->pEndActiveWidgetList);
    
  if(pChangeTechDlg->pScroll) {
    setup_vertical_scrollbar_area(pChangeTechDlg->pScroll,
	area.x + area.w, area.y,
    	area.h, TRUE);
  }

  redraw_group(pChangeTechDlg->pBeginWidgetList, pWindow, FALSE);

  widget_flush(pWindow);
}

static int science_dialog_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (!pChangeTechDlg) {
      if (sellect_window_group_dialog(pScienceDlg->pBeginWidgetList, pWindow)) {
        widget_flush(pWindow);
      }
      if (move_window_group_dialog(pScienceDlg->pBeginWidgetList, pWindow)) {
        science_dialog_update();
      }
    }
  }  
  return -1;
}

static int popup_change_research_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_wstate(pWidget, FC_WS_NORMAL);
    pSellected_Widget = NULL;
    widget_redraw(pWidget);
    widget_flush(pWidget);
    
    popup_change_research_dialog();
  }  
  return -1;
}

static int popup_change_research_goal_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    set_wstate(pWidget, FC_WS_NORMAL);
    pSellected_Widget = NULL;
    widget_redraw(pWidget);
    widget_flush(pWidget);
    
    popup_change_research_goal_dialog();
  }  
  return -1;
}

static int popdown_science_dialog_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_science_dialog();
  }
  return -1;
}

/**************************************************************************
  Popup (or raise) the science report(F6).  It may or may not be modal.
**************************************************************************/
void popup_science_dialog(bool raise)
{
  struct widget *pWidget, *pWindow;
  struct widget *pChangeResearchButton;
  struct widget *pChangeResearchGoalButton;
  struct widget *pExitButton;
  
  SDL_String16 *pStr;
  SDL_Surface *pBackground, *pTechIcon;
  int count;
  SDL_Rect area;
  
  if (pScienceDlg) {
    return;
  }

  /* disable research button */
  pWidget = get_research_widget();
  set_wstate(pWidget, FC_WS_DISABLED);
  widget_redraw(pWidget);
  widget_mark_dirty(pWidget);
  
  pScienceDlg = fc_calloc(1, sizeof(struct SMALL_DLG));
    
  /* TRANS: Research report title */
  pStr = create_str16_from_char(_("Research"), adj_font(12));
  pStr->style |= TTF_STYLE_BOLD;
  
#ifdef SMALL_SCREEN
  pWindow = create_window(NULL, pStr, 200, 132, 0);
#else
  pWindow = create_window(NULL, pStr, adj_size(400), adj_size(246), 0);
#endif
  set_wstate(pWindow, FC_WS_NORMAL);
  pWindow->action = science_dialog_callback;

  pScienceDlg->pEndWidgetList = pWindow;

  pBackground = theme_get_background(theme, BACKGROUND_SCIENCEDLG);
  pWindow->theme = ResizeSurface(pBackground, pWindow->size.w, pWindow->size.h, 1);
  FREESURFACE(pBackground);

  area = pWindow->area;
  
  widget_set_position(pWindow,
                      (Main.screen->w - pWindow->size.w) / 2,
                      (Main.screen->h - pWindow->size.h) / 2);

  add_to_gui_list(ID_SCIENCE_DLG_WINDOW, pWindow);
  
  /* count number of researchable techs */
  count = 0;
  advance_index_iterate(A_FIRST, i) {
    if (player_invention_reachable(client.conn.playing, i)
     && TECH_KNOWN != player_invention_state(client.conn.playing, i)) {
	count++;	  
    }
  }  advance_index_iterate_end;

  /* current research icon */
  pTechIcon = get_tech_icon(get_player_research(client.conn.playing)->researching);
  pChangeResearchButton = create_icon2(pTechIcon, pWindow->dst, WF_RESTORE_BACKGROUND | WF_FREE_THEME);

  pChangeResearchButton->action = popup_change_research_dialog_callback;
  if (count > 0) {
    set_wstate(pChangeResearchButton, FC_WS_NORMAL);
  }
  
  add_to_gui_list(ID_SCIENCE_DLG_CHANGE_REASARCH_BUTTON, pChangeResearchButton);

  /* current research goal icon */
  pTechIcon = get_tech_icon(get_player_research(client.conn.playing)->tech_goal);
  pChangeResearchGoalButton = create_icon2(pTechIcon, pWindow->dst, WF_RESTORE_BACKGROUND | WF_FREE_THEME);
  
  pChangeResearchGoalButton->action = popup_change_research_goal_dialog_callback;
  if (count > 0) {
    set_wstate(pChangeResearchGoalButton, FC_WS_NORMAL);
  }

  add_to_gui_list(ID_SCIENCE_DLG_CHANGE_GOAL_BUTTON, pChangeResearchGoalButton);

  /* ------ */
  /* exit button */
  pExitButton = create_themeicon(pTheme->Small_CANCEL_Icon, pWindow->dst,
                                 WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pExitButton->string16 = create_str16_from_char(_("Close Dialog (Esc)"), adj_font(12));
  pExitButton->action = popdown_science_dialog_callback;
  set_wstate(pExitButton, FC_WS_NORMAL);
  pExitButton->key = SDLK_ESCAPE;
  
  add_to_gui_list(ID_SCIENCE_CANCEL_DLG_BUTTON, pExitButton);

  widget_set_position(pExitButton, 
    area.x + area.w - pExitButton->size.w - adj_size(1),
    pWindow->size.y + adj_size(2));
    
  /* ======================== */
  pScienceDlg->pBeginWidgetList = pExitButton;

  science_dialog_update();
}

/**************************************************************************
  Popdow all the science reports (report, chnge tech, change goals).
**************************************************************************/
void popdown_all_science_dialogs(void)
{
  if(pChangeTechDlg) {
    popdown_window_group_dialog(pChangeTechDlg->pBeginWidgetList,
				  pChangeTechDlg->pEndWidgetList);
    FC_FREE(pChangeTechDlg->pScroll);
    FC_FREE(pChangeTechDlg);
  }
  if(pScienceDlg) {
    popdown_window_group_dialog(pScienceDlg->pBeginWidgetList,
				  pScienceDlg->pEndWidgetList);
    FC_FREE(pScienceDlg);
    set_wstate(get_research_widget(), FC_WS_NORMAL);
    widget_redraw(get_research_widget());
    widget_mark_dirty(get_research_widget());
  }  
}
  
/**************************************************************************
  Update all report dialogs.
**************************************************************************/
void update_report_dialogs(void)
{
  economy_report_dialog_update();
  activeunits_report_dialog_update();
  if(pScienceDlg) {
    science_dialog_update();
  }
}

/* ===================================================================== */
/* ======================== Endgame Report ============================= */
/* ===================================================================== */

/****************************************************************
  Show a dialog with player statistics at endgame.
  TODO: Display all statistics in packet_endgame_report.
*****************************************************************/
void popup_endgame_report_dialog(struct packet_endgame_report *packet)
{
  char buffer[150 * MAX_NUM_PLAYERS];
  int i;
 
  buffer[0] = '\0';
  for (i = 0; i < packet->nscores; i++) {
    cat_snprintf(buffer, sizeof(buffer),
                 PL_("%2d: The %s ruler %s scored %d point\n",
                     "%2d: The %s ruler %s scored %d points\n",
                     packet->score[i]),
                 i + 1,
                 nation_adjective_for_player(player_by_number(packet->id[i])),
                 player_name(player_by_number(packet->id[i])),
                 packet->score[i]);
  }
  popup_notify_dialog(_("Final Report:"),
                      _("The Greatest Civilizations in the world."),
                      buffer);
}

/*************************************************************************
  Server options dialog
*************************************************************************/
void popup_settable_options_dialog(void)
{
  /* PORT ME */
}
