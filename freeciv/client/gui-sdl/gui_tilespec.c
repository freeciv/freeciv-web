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
                          gui_tilespec.c  -  description
                             -------------------
    begin                : Dec. 2 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafal Bursig <bursig@poczta.fm>
 **********************************************************************/
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SDL.h"

/* utility */
#include "fcintl.h"
#include "log.h"

/* client */
#include "client_main.h"

/* gui-sdl */
#include "colors.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_string.h"
#include "sprite.h"
#include "themespec.h"

#include "gui_tilespec.h"

struct Theme *pTheme;
struct City_Icon *pIcons;

static SDL_Surface *pCity_Surf;

static SDL_Surface *pNeutral_Tech_Icon;
static SDL_Surface *pNone_Tech_Icon;
static SDL_Surface *pFuture_Tech_Icon;

#define load_GUI_surface(pSpr, pStruct, pSurf, tag)		  \
do {								  \
  pSpr = theme_lookup_sprite_tag_alt(theme, LOG_FATAL, tag, "", "", ""); \
  assert(pSpr != NULL);				                  \
  pStruct->pSurf = adj_surf(GET_SURF(pSpr));            	  \
  FREESURFACE(GET_SURF(pSpr));                                    \
} while(0)

#define load_theme_surface(pSpr, pSurf, tag)		\
	load_GUI_surface(pSpr, pTheme, pSurf, tag)

#define load_city_icon_surface(pSpr, pSurf, tag)        \
        load_GUI_surface(pSpr, pIcons, pSurf, tag)

#define load_order_theme_surface(pSpr, pSurf, tag)	\
do {							\
  load_GUI_surface(pSpr, pTheme, pSurf, tag);		\
} while(0)

/*******************************************************************************
 * reload small citizens "style" icons.
 *******************************************************************************/
static void reload_small_citizens_icons(int style)
{
  /* free info icons */
  FREESURFACE(pIcons->pMale_Content);
  FREESURFACE(pIcons->pFemale_Content);
  FREESURFACE(pIcons->pMale_Happy);
  FREESURFACE(pIcons->pFemale_Happy);
  FREESURFACE(pIcons->pMale_Unhappy);
  FREESURFACE(pIcons->pFemale_Unhappy);
  FREESURFACE(pIcons->pMale_Angry);
  FREESURFACE(pIcons->pFemale_Angry);
  
  FREESURFACE(pIcons->pSpec_Lux); /* Elvis */
  FREESURFACE(pIcons->pSpec_Tax); /* TaxMan */
  FREESURFACE(pIcons->pSpec_Sci); /* Scientist */
  
  /* allocate icons */
  pIcons->pMale_Happy = adj_surf(get_citizen_surface(CITIZEN_HAPPY, 0));
  pIcons->pFemale_Happy = adj_surf(get_citizen_surface(CITIZEN_HAPPY, 1));
  pIcons->pMale_Content = adj_surf(get_citizen_surface(CITIZEN_CONTENT, 0));
  pIcons->pFemale_Content = adj_surf(get_citizen_surface(CITIZEN_CONTENT, 1));
  pIcons->pMale_Unhappy = adj_surf(get_citizen_surface(CITIZEN_UNHAPPY, 0));
  pIcons->pFemale_Unhappy = adj_surf(get_citizen_surface(CITIZEN_UNHAPPY, 1));
  pIcons->pMale_Angry = adj_surf(get_citizen_surface(CITIZEN_ANGRY, 0));
  pIcons->pFemale_Angry = adj_surf(get_citizen_surface(CITIZEN_ANGRY, 1));
  pIcons->pSpec_Lux = get_tax_surface(O_LUXURY);
  pIcons->pSpec_Tax = get_tax_surface(O_GOLD);
  pIcons->pSpec_Sci = get_tax_surface(O_SCIENCE);
}

/* ================================================================================= */
/* ===================================== Public ==================================== */
/* ================================================================================= */

/**********************************************************************
  Set city citizens icons sprite value; should only happen after
  start of game (city style struct was filled ).
***********************************************************************/
void reload_citizens_icons(int style)
{
  reload_small_citizens_icons(style);
  pIcons->style = style;
}

/**************************************************************************
  ...
**************************************************************************/
void tilespec_setup_city_gfx(void) {
  struct sprite *pSpr =
    theme_lookup_sprite_tag_alt(theme, LOG_FATAL, "theme.city", "", "", "");    

  pCity_Surf = (pSpr ? adj_surf(GET_SURF(pSpr)) : NULL);
  
  assert(pCity_Surf != NULL);
}

/**********************************************************************
  Set city icons sprite value; should only happen after
  tileset_load_tiles(tileset).
***********************************************************************/
void tilespec_setup_city_icons(void)
{

  struct sprite *pSpr = NULL;
  
  pIcons = ( struct City_Icon *)fc_calloc(1,  sizeof( struct City_Icon ));
  
  load_city_icon_surface(pSpr, pBIG_Food_Corr, "city.food_waste");
  load_city_icon_surface(pSpr, pBIG_Shield_Corr, "city.shield_waste");
  load_city_icon_surface(pSpr, pBIG_Trade_Corr, "city.trade_waste");
  load_city_icon_surface(pSpr, pBIG_Food, "city.food");
      
  pIcons->pBIG_Food_Surplus = crop_rect_from_surface(pIcons->pBIG_Food, NULL);
  SDL_SetAlpha(pIcons->pBIG_Food_Surplus, SDL_SRCALPHA, 128);
    
  load_city_icon_surface(pSpr, pBIG_Shield, "city.shield");
  
  pIcons->pBIG_Shield_Surplus = crop_rect_from_surface(pIcons->pBIG_Shield, NULL);
  SDL_SetAlpha(pIcons->pBIG_Shield_Surplus, SDL_SRCALPHA, 128);
  
  load_city_icon_surface(pSpr, pBIG_Trade, "city.trade");
  load_city_icon_surface(pSpr, pBIG_Luxury, "city.lux");
  load_city_icon_surface(pSpr, pBIG_Coin, "city.coin");
  load_city_icon_surface(pSpr, pBIG_Colb, "city.colb");
  load_city_icon_surface(pSpr, pBIG_Face, "city.red_face");
  load_city_icon_surface(pSpr, pBIG_Coin_Corr, "city.dark_coin");
  load_city_icon_surface(pSpr, pBIG_Coin_UpKeep, "city.unkeep_coin");
  		  
  /* small icon */
  load_city_icon_surface(pSpr, pFood, "city.small_food");
  load_city_icon_surface(pSpr, pShield, "city.small_shield");
  load_city_icon_surface(pSpr, pTrade, "city.small_trade");
  load_city_icon_surface(pSpr, pFace, "city.small_red_face");
  load_city_icon_surface(pSpr, pLuxury, "city.small_lux");
  load_city_icon_surface(pSpr, pCoin, "city.small_coin");		  
  load_city_icon_surface(pSpr, pColb, "city.small_colb");		  
    
  load_city_icon_surface(pSpr, pPollution, "city.pollution");
  /* ================================================================= */
  
  load_city_icon_surface(pSpr, pPolice, "city.police");
  /* ================================================================= */
  pIcons->pWorklist = create_surf_alpha(9,9, SDL_SWSURFACE);
  SDL_FillRect(pIcons->pWorklist, NULL,
		  SDL_MapRGB(pIcons->pWorklist->format, 255, 255,255));
  putframe(pIcons->pWorklist, 0,0, pIcons->pWorklist->w - 1, pIcons->pWorklist->h - 1,
    map_rgba(pIcons->pWorklist->format, *get_game_colorRGB(COLOR_THEME_CITYREP_FRAME)));
  putline(pIcons->pWorklist, 3, 2, 5, 2,
    map_rgba(pIcons->pWorklist->format, *get_game_colorRGB(COLOR_THEME_CITYREP_FRAME)));
  putline(pIcons->pWorklist, 3, 4, 7, 4, 
    map_rgba(pIcons->pWorklist->format, *get_game_colorRGB(COLOR_THEME_CITYREP_FRAME)));
  putline(pIcons->pWorklist, 3, 6, 6, 6,
    map_rgba(pIcons->pWorklist->format, *get_game_colorRGB(COLOR_THEME_CITYREP_FRAME)));
  
  /* ================================================================= */
  
  /* force reload citizens icons */
  pIcons->style = 999;
}

void tilespec_free_city_icons(void)
{
  if (!pIcons) {
    return;
  }
  
  FREESURFACE(pIcons->pBIG_Food_Corr);
  FREESURFACE(pIcons->pBIG_Shield_Corr);
  FREESURFACE(pIcons->pBIG_Trade_Corr);
  FREESURFACE(pIcons->pBIG_Food);
  FREESURFACE(pIcons->pBIG_Food_Surplus);
  FREESURFACE(pIcons->pBIG_Shield);
  FREESURFACE(pIcons->pBIG_Shield_Surplus);
  FREESURFACE(pIcons->pBIG_Trade);
  FREESURFACE(pIcons->pBIG_Luxury);
  FREESURFACE(pIcons->pBIG_Coin);
  FREESURFACE(pIcons->pBIG_Colb);
  FREESURFACE(pIcons->pBIG_Face);
  FREESURFACE(pIcons->pBIG_Coin_Corr);
  FREESURFACE(pIcons->pBIG_Coin_UpKeep);

  FREESURFACE(pIcons->pFood);
  FREESURFACE(pIcons->pShield);
  FREESURFACE(pIcons->pTrade);
  FREESURFACE(pIcons->pFace);
  FREESURFACE(pIcons->pLuxury);
  FREESURFACE(pIcons->pCoin);		  
  FREESURFACE(pIcons->pColb);		  

  FREESURFACE(pIcons->pPollution);
  FREESURFACE(pIcons->pPolice);
  FREESURFACE(pIcons->pWorklist);

  /* small citizens */
  FREESURFACE(pIcons->pMale_Content);
  FREESURFACE(pIcons->pFemale_Content);
  FREESURFACE(pIcons->pMale_Happy);
  FREESURFACE(pIcons->pFemale_Happy);
  FREESURFACE(pIcons->pMale_Unhappy);
  FREESURFACE(pIcons->pFemale_Unhappy);
  FREESURFACE(pIcons->pMale_Angry);
  FREESURFACE(pIcons->pFemale_Angry);

  FREESURFACE(pIcons->pSpec_Lux); /* Elvis */
  FREESURFACE(pIcons->pSpec_Tax); /* TaxMan */
  FREESURFACE(pIcons->pSpec_Sci); /* Scientist */

  FC_FREE(pIcons);
  
}


/* =================================================== */
/* ===================== THEME ======================= */
/* =================================================== */

/*
 *	Alloc and fill Theme struct
 */
void tilespec_setup_theme(void)
{
  struct sprite *pBuf = NULL;
  
  pTheme = fc_calloc(1, sizeof(struct Theme));

  load_theme_surface(pBuf, FR_Left, "theme.left_frame");
  load_theme_surface(pBuf, FR_Right, "theme.right_frame");
  load_theme_surface(pBuf, FR_Top, "theme.top_frame");
  load_theme_surface(pBuf, FR_Bottom, "theme.bottom_frame");
  load_theme_surface(pBuf, Button, "theme.button");
  load_theme_surface(pBuf, Edit, "theme.edit");
  load_theme_surface(pBuf, CBOX_Sell_Icon, "theme.sbox");
  load_theme_surface(pBuf, CBOX_Unsell_Icon, "theme.ubox");
  load_theme_surface(pBuf, UP_Icon, "theme.UP_scroll");
  load_theme_surface(pBuf, DOWN_Icon, "theme.DOWN_scroll");
#if 0
  load_theme_surface(pBuf, LEFT_Icon, "theme.LEFT_scroll");
  load_theme_surface(pBuf, RIGHT_Icon, "theme.RIGHT_scroll");
#endif
  load_theme_surface(pBuf, Vertic, "theme.vertic_scrollbar");
  load_theme_surface(pBuf, Horiz, "theme.horiz_scrollbar");
  
  /* ------------------- */
  load_theme_surface(pBuf, OK_PACT_Icon, "theme.pact_ok");
  load_theme_surface(pBuf, CANCEL_PACT_Icon, "theme.pact_cancel");
  /* ------------------- */
  load_theme_surface(pBuf, Small_OK_Icon, "theme.SMALL_OK_button");
  load_theme_surface(pBuf, Small_CANCEL_Icon, "theme.SMALL_FAIL_button");
  /* ------------------- */
  load_theme_surface(pBuf, OK_Icon, "theme.OK_button");
  load_theme_surface(pBuf, CANCEL_Icon, "theme.FAIL_button");
  load_theme_surface(pBuf, FORWARD_Icon, "theme.NEXT_button");
  load_theme_surface(pBuf, BACK_Icon, "theme.BACK_button");
  load_theme_surface(pBuf, L_ARROW_Icon, "theme.LEFT_ARROW_button");
  load_theme_surface(pBuf, R_ARROW_Icon, "theme.RIGHT_ARROW_button");
  load_theme_surface(pBuf, MAP_Icon, "theme.MAP_button");
  load_theme_surface(pBuf, FindCity_Icon, "theme.FIND_CITY_button");
  load_theme_surface(pBuf, NEW_TURN_Icon, "theme.NEW_TURN_button");
  load_theme_surface(pBuf, LOG_Icon, "theme.LOG_button");
  load_theme_surface(pBuf, UNITS_Icon, "theme.UNITS_INFO_button");
  load_theme_surface(pBuf, Options_Icon, "theme.OPTIONS_button");
  load_theme_surface(pBuf, Block, "theme.block");
  load_theme_surface(pBuf, INFO_Icon, "theme.INFO_button");
  load_theme_surface(pBuf, Army_Icon, "theme.ARMY_button");
  load_theme_surface(pBuf, Happy_Icon, "theme.HAPPY_button");
  load_theme_surface(pBuf, Support_Icon, "theme.HOME_button");
  load_theme_surface(pBuf, Buy_PROD_Icon, "theme.BUY_button");
  load_theme_surface(pBuf, PROD_Icon, "theme.PROD_button");
  load_theme_surface(pBuf, QPROD_Icon, "theme.WORK_LIST_button");
  load_theme_surface(pBuf, CMA_Icon, "theme.CMA_button");
  load_theme_surface(pBuf, LOCK_Icon, "theme.LOCK_button");
  load_theme_surface(pBuf, UNLOCK_Icon, "theme.UNLOCK_button");
  load_theme_surface(pBuf, PLAYERS_Icon, "theme.PLAYERS_button");
  load_theme_surface(pBuf, UNITS2_Icon, "theme.UNITS_button");
  load_theme_surface(pBuf, SAVE_Icon, "theme.SAVE_button");
  load_theme_surface(pBuf, LOAD_Icon, "theme.LOAD_button");
  load_theme_surface(pBuf, DELETE_Icon, "theme.DELETE_button");
  load_theme_surface(pBuf, BORDERS_Icon, "theme.BORDERS_button");
  /* ------------------------------ */
  load_theme_surface(pBuf, Tech_Tree_Icon, "theme.tech_tree");
  /* ------------------------------ */

  load_order_theme_surface(pBuf, Order_Icon, "theme.order_empty");  
  load_order_theme_surface(pBuf, OAutoAtt_Icon, "theme.order_auto_attack");
  load_order_theme_surface(pBuf, OAutoConnect_Icon, "theme.order_auto_connect");  
  load_order_theme_surface(pBuf, OAutoExp_Icon, "theme.order_auto_explorer");
  load_order_theme_surface(pBuf, OAutoSett_Icon, "theme.order_auto_settler");
  load_order_theme_surface(pBuf, OBuildCity_Icon, "theme.order_build_city");
  load_order_theme_surface(pBuf, OCutDownForest_Icon, "theme.order_cutdown_forest");
  load_order_theme_surface(pBuf, OPlantForest_Icon, "theme.order_plant_forest");
  load_order_theme_surface(pBuf, OMine_Icon, "theme.order_build_mining");
  load_order_theme_surface(pBuf, OIrrigation_Icon, "theme.order_irrigation");
  load_order_theme_surface(pBuf, ODone_Icon, "theme.order_done");
  load_order_theme_surface(pBuf, ODisband_Icon, "theme.order_disband");
  load_order_theme_surface(pBuf, OFortify_Icon, "theme.order_fortify");
  load_order_theme_surface(pBuf, OGoto_Icon, "theme.order_goto");
  load_order_theme_surface(pBuf, OGotoCity_Icon, "theme.order_goto_city");
  load_order_theme_surface(pBuf, OHomeCity_Icon, "theme.order_home");  
  load_order_theme_surface(pBuf, ONuke_Icon, "theme.order_nuke");
  load_order_theme_surface(pBuf, OParaDrop_Icon, "theme.order_paradrop");
  load_order_theme_surface(pBuf, OPatrol_Icon, "theme.order_patrol");
  load_order_theme_surface(pBuf, OPillage_Icon, "theme.order_pillage");
  load_order_theme_surface(pBuf, ORailRoad_Icon, "theme.order_build_railroad");
  load_order_theme_surface(pBuf, ORoad_Icon, "theme.order_build_road");
  load_order_theme_surface(pBuf, OSentry_Icon, "theme.order_sentry");
  load_order_theme_surface(pBuf, OUnload_Icon, "theme.order_unload");
  load_order_theme_surface(pBuf, OWait_Icon, "theme.order_wait");
  load_order_theme_surface(pBuf, OFortress_Icon, "theme.order_build_fortress");
  load_order_theme_surface(pBuf, OFallout_Icon, "theme.order_clean_fallout");
  load_order_theme_surface(pBuf, OPollution_Icon, "theme.order_clean_pollution");
  load_order_theme_surface(pBuf, OAirBase_Icon, "theme.order_build_airbase");
  load_order_theme_surface(pBuf, OTransform_Icon, "theme.order_transform");
  load_order_theme_surface(pBuf, OAddCity_Icon, "theme.order_add_to_city");
  load_order_theme_surface(pBuf, OWonder_Icon, "theme.order_carravan_wonder");
  load_order_theme_surface(pBuf, OTrade_Icon, "theme.order_carravan_traderoute");
  load_order_theme_surface(pBuf, OSpy_Icon, "theme.order_spying");
  load_order_theme_surface(pBuf, OWakeUp_Icon, "theme.order_wakeup");
  load_order_theme_surface(pBuf, OReturn_Icon, "theme.order_return");
  load_order_theme_surface(pBuf, OAirLift_Icon, "theme.order_airlift");
  load_order_theme_surface(pBuf, OLoad_Icon, "theme.order_load");
  
  /* ------------------------------ */
    
  return;
}

/*
 *	Free memmory
 */
void tilespec_free_theme(void)
{
  if (!pTheme) {
    return;
  }

  FREESURFACE(pTheme->FR_Left);
  FREESURFACE(pTheme->FR_Right);
  FREESURFACE(pTheme->FR_Top);
  FREESURFACE(pTheme->FR_Bottom);
  
  FREESURFACE(pTheme->Button);
  
  FREESURFACE(pTheme->Edit);

  FREESURFACE(pTheme->CBOX_Sell_Icon);
  FREESURFACE(pTheme->CBOX_Unsell_Icon);

  FREESURFACE(pTheme->UP_Icon);
  FREESURFACE(pTheme->DOWN_Icon);
#if 0
  FREESURFACE(pTheme->LEFT_Icon);
  FREESURFACE(pTheme->RIGHT_Icon);
#endif
  FREESURFACE(pTheme->Vertic);
  FREESURFACE(pTheme->Horiz);
  
  /* ------------------- */
  
  FREESURFACE(pTheme->OK_Icon);
  FREESURFACE(pTheme->CANCEL_Icon);
  FREESURFACE(pTheme->Small_OK_Icon);
  FREESURFACE(pTheme->Small_CANCEL_Icon);
  FREESURFACE(pTheme->FORWARD_Icon);
  FREESURFACE(pTheme->BACK_Icon);
  FREESURFACE(pTheme->L_ARROW_Icon);
  FREESURFACE(pTheme->R_ARROW_Icon);
  FREESURFACE(pTheme->MAP_Icon);
  FREESURFACE(pTheme->FindCity_Icon);
  FREESURFACE(pTheme->NEW_TURN_Icon);
  FREESURFACE(pTheme->LOG_Icon);
  FREESURFACE(pTheme->UNITS_Icon);
  FREESURFACE(pTheme->UNITS2_Icon);
  FREESURFACE(pTheme->PLAYERS_Icon);
  FREESURFACE(pTheme->Options_Icon);
  FREESURFACE(pTheme->Block);
  FREESURFACE(pTheme->INFO_Icon);
  FREESURFACE(pTheme->Army_Icon);
  FREESURFACE(pTheme->Happy_Icon);
  FREESURFACE(pTheme->Support_Icon);
  FREESURFACE(pTheme->Buy_PROD_Icon);
  FREESURFACE(pTheme->PROD_Icon);
  FREESURFACE(pTheme->QPROD_Icon);
  FREESURFACE(pTheme->CMA_Icon);
  FREESURFACE(pTheme->LOCK_Icon);
  FREESURFACE(pTheme->UNLOCK_Icon);
  FREESURFACE(pTheme->OK_PACT_Icon);
  FREESURFACE(pTheme->CANCEL_PACT_Icon);
  FREESURFACE(pTheme->SAVE_Icon);
  FREESURFACE(pTheme->LOAD_Icon);
  FREESURFACE(pTheme->DELETE_Icon);
  FREESURFACE(pTheme->BORDERS_Icon);
  /* ------------------------------ */
  FREESURFACE(pTheme->Tech_Tree_Icon);
  /* ------------------------------ */

  FREESURFACE(pTheme->Order_Icon);
  FREESURFACE(pTheme->OAutoAtt_Icon);
  FREESURFACE(pTheme->OAutoConnect_Icon);
  FREESURFACE(pTheme->OAutoExp_Icon);
  FREESURFACE(pTheme->OAutoSett_Icon);
  FREESURFACE(pTheme->OBuildCity_Icon);
  FREESURFACE(pTheme->OCutDownForest_Icon);
  FREESURFACE(pTheme->OPlantForest_Icon);
  FREESURFACE(pTheme->OMine_Icon);
  FREESURFACE(pTheme->OIrrigation_Icon);
  FREESURFACE(pTheme->ODone_Icon);
  FREESURFACE(pTheme->ODisband_Icon);
  FREESURFACE(pTheme->OFortify_Icon);
  FREESURFACE(pTheme->OGoto_Icon);
  FREESURFACE(pTheme->OGotoCity_Icon);
  FREESURFACE(pTheme->OHomeCity_Icon);
  FREESURFACE(pTheme->ONuke_Icon);
  FREESURFACE(pTheme->OParaDrop_Icon);
  FREESURFACE(pTheme->OPatrol_Icon);
  FREESURFACE(pTheme->OPillage_Icon);
  FREESURFACE(pTheme->ORailRoad_Icon);
  FREESURFACE(pTheme->ORoad_Icon);
  FREESURFACE(pTheme->OSentry_Icon);
  FREESURFACE(pTheme->OUnload_Icon);
  FREESURFACE(pTheme->OWait_Icon);
  FREESURFACE(pTheme->OFortress_Icon);
  FREESURFACE(pTheme->OFallout_Icon);
  FREESURFACE(pTheme->OPollution_Icon);
  FREESURFACE(pTheme->OAirBase_Icon);
  FREESURFACE(pTheme->OTransform_Icon);
  FREESURFACE(pTheme->OAddCity_Icon);
  FREESURFACE(pTheme->OWonder_Icon);
  FREESURFACE(pTheme->OTrade_Icon);
  FREESURFACE(pTheme->OSpy_Icon);
  FREESURFACE(pTheme->OWakeUp_Icon);
  FREESURFACE(pTheme->OReturn_Icon);
  FREESURFACE(pTheme->OAirLift_Icon);
  FREESURFACE(pTheme->OLoad_Icon);

  FC_FREE(pTheme);
  return;
}

/**************************************************************************
  ...
**************************************************************************/
void setup_auxiliary_tech_icons(void)
{
  SDL_Color bg_color = {255, 255, 255, 136};

  SDL_Surface *pSurf;
  SDL_String16 *pStr = create_str16_from_char(_("None"), adj_font(10));
  
  pStr->style |= (TTF_STYLE_BOLD | SF_CENTER);
    
  /* create icons */
  pSurf = create_surf_alpha(adj_size(50), adj_size(50), SDL_SWSURFACE);
  SDL_FillRect(pSurf, NULL, map_rgba(pSurf->format, bg_color));
  putframe(pSurf, 0 , 0, pSurf->w - 1, pSurf->h - 1,
         map_rgba(pSurf->format, *get_game_colorRGB(COLOR_THEME_SCIENCEDLG_FRAME)));

  pNeutral_Tech_Icon = SDL_DisplayFormatAlpha(pSurf);
  pNone_Tech_Icon = SDL_DisplayFormatAlpha(pSurf);    
  pFuture_Tech_Icon = SDL_DisplayFormatAlpha(pSurf);
  
  FREESURFACE(pSurf);
    
  /* None */
  pSurf = create_text_surf_from_str16(pStr);
  blit_entire_src(pSurf, pNone_Tech_Icon ,
	  (adj_size(50) - pSurf->w) / 2 , (adj_size(50) - pSurf->h) / 2);
  
  FREESURFACE(pSurf);
  
  /* TRANS: Future Technology */ 
  copy_chars_to_string16(pStr, _("FT"));
  pSurf = create_text_surf_from_str16(pStr);
  blit_entire_src(pSurf, pFuture_Tech_Icon,
	  (adj_size(50) - pSurf->w) / 2 , (adj_size(50) - pSurf->h) / 2);
  
  FREESURFACE(pSurf);
  
  FREESTRING16(pStr);
    
}

/**************************************************************************
  ...
**************************************************************************/
void free_auxiliary_tech_icons(void)
{
  FREESURFACE(pNeutral_Tech_Icon);
  FREESURFACE(pNone_Tech_Icon);
  FREESURFACE(pFuture_Tech_Icon);
}

/**************************************************************************
  ...
**************************************************************************/
SDL_Surface * get_tech_icon(Tech_type_id tech)
{
  switch(tech)
  {
    case A_NONE:
    case A_UNSET:
    case A_UNKNOWN:
    case A_LAST:
      return SDL_DisplayFormatAlpha(pNone_Tech_Icon);
    case A_FUTURE:
      return SDL_DisplayFormatAlpha(pFuture_Tech_Icon);
    default:
      if (get_tech_sprite(tileset, tech)) {
        return adj_surf(GET_SURF(get_tech_sprite(tileset, tech)));
      } else {
        return SDL_DisplayFormatAlpha(pNeutral_Tech_Icon);
      }
  }
  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
SDL_Color * get_tech_color(Tech_type_id tech_id)
{
  if (player_invention_reachable(client.conn.playing, tech_id))
  {
    switch (player_invention_state(client.conn.playing, tech_id))
    {
      case TECH_UNKNOWN:
        return get_game_colorRGB(COLOR_REQTREE_UNKNOWN);
      case TECH_KNOWN:
        return get_game_colorRGB(COLOR_REQTREE_KNOWN);
      case TECH_PREREQS_KNOWN:
        return get_game_colorRGB(COLOR_REQTREE_PREREQS_KNOWN);
      default:
        return get_game_colorRGB(COLOR_REQTREE_BACKGROUND);
    }
  }
  return get_game_colorRGB(COLOR_REQTREE_UNREACHABLE);
}

/**************************************************************************
  ...
**************************************************************************/
SDL_Surface * get_city_gfx(void)
{
  return pCity_Surf;
}

/**************************************************************************
  ...
**************************************************************************/
void draw_intro_gfx(void)
{
  SDL_Surface *pIntro = theme_get_background(theme, BACKGROUND_MAINPAGE);

  if(pIntro->w != Main.screen->w)
  {
    SDL_Surface *pTmp = ResizeSurface(pIntro, Main.screen->w, Main.screen->h,1);
    FREESURFACE(pIntro);
    pIntro = pTmp;
  }
  
  /* draw intro gfx center in screen */
  alphablit(pIntro, NULL, Main.map, NULL);
  
  FREESURFACE(pIntro);
}
