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
                          gui_tilespec.h  -  description
                             -------------------
    begin                : Dec. 2 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafal Bursig <bursig@poczta.fm>
 **********************************************************************/
 
#ifndef FC__GUI_TILESPEC_H
#define FC__GUI_TILESPEC_H

#include "SDL.h"

#include "tilespec.h"

#include "graphics.h"
#include "sprite.h"

struct Theme {

  /* Frame */  
  SDL_Surface *FR_Left;
  SDL_Surface *FR_Right;
  SDL_Surface *FR_Top;
  SDL_Surface *FR_Bottom;

  /* Button */
  SDL_Surface *Button;

  /* Edit */
  SDL_Surface *Edit;

  /* Checkbox */
  SDL_Surface *CBOX_Sell_Icon;
  SDL_Surface *CBOX_Unsell_Icon;

  /* Scrollbar */
  SDL_Surface *UP_Icon;
  SDL_Surface *DOWN_Icon;
  SDL_Surface *LEFT_Icon;
  SDL_Surface *RIGHT_Icon;
  SDL_Surface *Vertic;
  SDL_Surface *Horiz;

  /* Game */
  SDL_Surface *OK_Icon;
  SDL_Surface *CANCEL_Icon;
  SDL_Surface *Small_OK_Icon;
  SDL_Surface *Small_CANCEL_Icon;
  SDL_Surface *FORWARD_Icon;
  SDL_Surface *BACK_Icon;
  SDL_Surface *INFO_Icon;
  SDL_Surface *R_ARROW_Icon;
  SDL_Surface *L_ARROW_Icon;
  SDL_Surface *LOCK_Icon;
  SDL_Surface *UNLOCK_Icon;
  
  SDL_Surface *Options_Icon;
  SDL_Surface *Block;
  SDL_Surface *UNITS_Icon;
  SDL_Surface *MAP_Icon;
  SDL_Surface *LOG_Icon;
  SDL_Surface *PLAYERS_Icon;
  SDL_Surface *UNITS2_Icon;
  SDL_Surface *FindCity_Icon;
  SDL_Surface *NEW_TURN_Icon;
  SDL_Surface *SAVE_Icon;
  SDL_Surface *LOAD_Icon;
  SDL_Surface *DELETE_Icon;
  SDL_Surface *BORDERS_Icon;

  /* help icons */
  SDL_Surface *Tech_Tree_Icon;
  
  /* city icons */
  SDL_Surface *Army_Icon;
  SDL_Surface *Support_Icon;
  SDL_Surface *Happy_Icon;
  SDL_Surface *CMA_Icon;
  SDL_Surface *PROD_Icon;
  SDL_Surface *QPROD_Icon;
  SDL_Surface *Buy_PROD_Icon;

  /* diplomacy */
  SDL_Surface *OK_PACT_Icon;
  SDL_Surface *CANCEL_PACT_Icon;
  
  /* orders icons */
  SDL_Surface *Order_Icon;
  SDL_Surface *ODisband_Icon;
  SDL_Surface *OWait_Icon;
  SDL_Surface *ODone_Icon;
  SDL_Surface *OAutoAtt_Icon;
  SDL_Surface *OAutoExp_Icon;
  SDL_Surface *OAutoSett_Icon;
  SDL_Surface *OAutoConnect_Icon;
  SDL_Surface *OUnload_Icon;
  SDL_Surface *OBuildCity_Icon;
  SDL_Surface *OGotoCity_Icon;
  SDL_Surface *OGoto_Icon;
  SDL_Surface *OHomeCity_Icon;
  SDL_Surface *OPatrol_Icon;
  SDL_Surface *OMine_Icon;
  SDL_Surface *OPlantForest_Icon;
  SDL_Surface *OCutDownForest_Icon;
  SDL_Surface *OFortify_Icon;
  SDL_Surface *OSentry_Icon;
  SDL_Surface *OIrrigation_Icon;
  SDL_Surface *ORoad_Icon;
  SDL_Surface *ORailRoad_Icon;
  SDL_Surface *OPillage_Icon;
  SDL_Surface *OParaDrop_Icon;
  SDL_Surface *ONuke_Icon;
  SDL_Surface *OFortress_Icon;
  SDL_Surface *OFallout_Icon;
  SDL_Surface *OPollution_Icon;
  SDL_Surface *OAirBase_Icon;
  SDL_Surface *OTransform_Icon;
  SDL_Surface *OAddCity_Icon;
  SDL_Surface *OWonder_Icon;
  SDL_Surface *OTrade_Icon;
  SDL_Surface *OSpy_Icon;
  SDL_Surface *OWakeUp_Icon;
  SDL_Surface *OReturn_Icon;
  SDL_Surface *OAirLift_Icon;
  SDL_Surface *OLoad_Icon;
			
};

struct City_Icon {
  
  int style;
  
  SDL_Surface *pBIG_Food_Corr;
  SDL_Surface *pBIG_Shield_Corr;
  SDL_Surface *pBIG_Trade_Corr;
  SDL_Surface *pBIG_Food;
  SDL_Surface *pBIG_Shield;
  SDL_Surface *pBIG_Trade;
  SDL_Surface *pBIG_Luxury;
  SDL_Surface *pBIG_Coin;
  SDL_Surface *pBIG_Colb;
  SDL_Surface *pBIG_Face;
  SDL_Surface *pBIG_Coin_Corr;
  SDL_Surface *pBIG_Coin_UpKeep;
  SDL_Surface *pBIG_Shield_Surplus;
  SDL_Surface *pBIG_Food_Surplus;
  
  SDL_Surface *pFood;
  SDL_Surface *pShield;
  SDL_Surface *pTrade;
  SDL_Surface *pLuxury;
  SDL_Surface *pCoin;
  SDL_Surface *pColb;
  SDL_Surface *pFace;
  /*SDL_Surface *pDark_Face;*/
	
  SDL_Surface *pPollution;
  SDL_Surface *pPolice;
  SDL_Surface *pWorklist;
  
  /* Small Citizens */
  SDL_Surface *pMale_Happy;
  SDL_Surface *pFemale_Happy;
  SDL_Surface *pMale_Content;
  SDL_Surface *pFemale_Content;
  SDL_Surface *pMale_Unhappy;
  SDL_Surface *pFemale_Unhappy;
  SDL_Surface *pMale_Angry;
  SDL_Surface *pFemale_Angry;
	
  SDL_Surface *pSpec_Lux; /* Elvis */
  SDL_Surface *pSpec_Tax; /* TaxMan */
  SDL_Surface *pSpec_Sci; /* Scientist */

};

extern struct Theme *pTheme;
extern struct City_Icon *pIcons;

void tilespec_setup_theme(void);
void tilespec_free_theme(void);

void tilespec_setup_city_gfx(void);
  
void tilespec_setup_city_icons(void);
void tilespec_free_city_icons(void);

void reload_citizens_icons(int style);
SDL_Surface * get_city_gfx(void);

void draw_intro_gfx(void);

void setup_auxiliary_tech_icons(void);
void free_auxiliary_tech_icons(void);
SDL_Surface * get_tech_icon(Tech_type_id tech);
SDL_Color * get_tech_color(Tech_type_id tech_id);

/**************************************************************************
  Return a surface for the given citizen.  The citizen's type is given,
  as well as their index (in the range [0..pcity->size)).
**************************************************************************/
static inline SDL_Surface *get_citizen_surface(enum citizen_category type,
                                               int citizen_index)
{
  return GET_SURF(get_citizen_sprite(tileset, type, 0, NULL));
}

static inline SDL_Surface *get_nation_flag_surface(const struct nation_type *pnation)
{
  return GET_SURF(get_nation_flag_sprite(tileset, pnation));
}

static inline SDL_Surface *get_government_surface(const struct government *gov)
{
  return GET_SURF(get_government_sprite(tileset, gov));
}

static inline SDL_Surface *get_sample_city_surface(int city_style)
{
  return GET_SURF(get_sample_city_sprite(tileset, city_style));
}

static inline SDL_Surface *get_building_surface(struct impr_type *pimprove)
{
  return GET_SURF(get_building_sprite(tileset, pimprove));
}

static inline SDL_Surface *get_unittype_surface(const struct unit_type *punittype)
{
  return GET_SURF(get_unittype_sprite(tileset, punittype));
}

static inline SDL_Surface *get_tax_surface(Output_type_id otype)
{
  return adj_surf(GET_SURF(get_tax_sprite(tileset, otype)));
}

#endif  /* FC__GUI_TILESPEC_H */
