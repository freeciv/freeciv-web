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
#ifndef FC__EFFECTS_H
#define FC__EFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "connection.h"
#include "fc_types.h"
#include "multipliers.h"

#include "requirements.h"

/* Type of effects. Add new values via SPECENUM_VALUE%d and
 * SPECENUM_VALUE%dNAME at the end of the list.
 * Used in the network protocol.
 *
 */
#define SPECENUM_NAME effect_type
#define SPECENUM_VALUE0 EFT_TECH_PARASITE
#define SPECENUM_VALUE0NAME "Tech_Parasite"
#define SPECENUM_VALUE1 EFT_AIRLIFT
#define SPECENUM_VALUE1NAME "Airlift"
#define SPECENUM_VALUE2 EFT_ANY_GOVERNMENT
#define SPECENUM_VALUE2NAME "Any_Government"
#define SPECENUM_VALUE3 EFT_CAPITAL_CITY
#define SPECENUM_VALUE3NAME "Capital_City"
#define SPECENUM_VALUE4 EFT_ENABLE_NUKE
#define SPECENUM_VALUE4NAME "Enable_Nuke"
#define SPECENUM_VALUE5 EFT_ENABLE_SPACE
#define SPECENUM_VALUE5NAME "Enable_Space"
#define SPECENUM_VALUE6 EFT_SPECIALIST_OUTPUT
#define SPECENUM_VALUE6NAME "Specialist_Output"
#define SPECENUM_VALUE7 EFT_OUTPUT_BONUS
#define SPECENUM_VALUE7NAME "Output_Bonus"
#define SPECENUM_VALUE8 EFT_OUTPUT_BONUS_2
#define SPECENUM_VALUE8NAME "Output_Bonus_2"
/* add to each worked tile */
#define SPECENUM_VALUE9 EFT_OUTPUT_ADD_TILE
#define SPECENUM_VALUE9NAME "Output_Add_Tile"
/* add to each worked tile that already has output */
#define SPECENUM_VALUE10 EFT_OUTPUT_INC_TILE
#define SPECENUM_VALUE10NAME "Output_Inc_Tile"
/* increase tile output by given % */
#define SPECENUM_VALUE11 EFT_OUTPUT_PER_TILE
#define SPECENUM_VALUE11NAME "Output_Per_Tile"
#define SPECENUM_VALUE12 EFT_OUTPUT_WASTE_PCT
#define SPECENUM_VALUE12NAME "Output_Waste_Pct"
#define SPECENUM_VALUE13 EFT_FORCE_CONTENT
#define SPECENUM_VALUE13NAME "Force_Content"
/* TODO: EFT_FORCE_CONTENT_PCT */
#define SPECENUM_VALUE14 EFT_GIVE_IMM_TECH
#define SPECENUM_VALUE14NAME "Give_Imm_Tech"
#define SPECENUM_VALUE15 EFT_GROWTH_FOOD
#define SPECENUM_VALUE15NAME "Growth_Food"
/* reduced illness due to buildings ... */
#define SPECENUM_VALUE16 EFT_HEALTH_PCT
#define SPECENUM_VALUE16NAME "Health_Pct"
#define SPECENUM_VALUE17 EFT_HAVE_EMBASSIES
#define SPECENUM_VALUE17NAME "Have_Embassies"
#define SPECENUM_VALUE18 EFT_MAKE_CONTENT
#define SPECENUM_VALUE18NAME "Make_Content"
#define SPECENUM_VALUE19 EFT_MAKE_CONTENT_MIL
#define SPECENUM_VALUE19NAME "Make_Content_Mil"
#define SPECENUM_VALUE20 EFT_MAKE_CONTENT_MIL_PER
#define SPECENUM_VALUE20NAME "Make_Content_Mil_Per"
/* TODO: EFT_MAKE_CONTENT_PCT */
#define SPECENUM_VALUE21 EFT_MAKE_HAPPY
#define SPECENUM_VALUE21NAME "Make_Happy"
#define SPECENUM_VALUE22 EFT_NO_ANARCHY
#define SPECENUM_VALUE22NAME "No_Anarchy"
#define SPECENUM_VALUE23 EFT_NUKE_PROOF
#define SPECENUM_VALUE23NAME "Nuke_Proof"
/* TODO: EFT_POLLU_ADJ */
/* TODO: EFT_POLLU_PCT */
/* TODO: EFT_POLLU_POP_ADJ */
#define SPECENUM_VALUE24 EFT_POLLU_POP_PCT
#define SPECENUM_VALUE24NAME "Pollu_Pop_Pct"
#define SPECENUM_VALUE25 EFT_POLLU_POP_PCT_2
#define SPECENUM_VALUE25NAME "Pollu_Pop_Pct_2"
/* TODO: EFT_POLLU_PROD_ADJ */
#define SPECENUM_VALUE26 EFT_POLLU_PROD_PCT
#define SPECENUM_VALUE26NAME "Pollu_Prod_Pct"
/* TODO: EFT_PROD_PCT */
#define SPECENUM_VALUE27 EFT_REVEAL_CITIES
#define SPECENUM_VALUE27NAME "Reveal_Cities"
#define SPECENUM_VALUE28 EFT_REVEAL_MAP
#define SPECENUM_VALUE28NAME "Reveal_Map"
/* TODO: EFT_INCITE_DIST_ADJ */
#define SPECENUM_VALUE29 EFT_INCITE_COST_PCT
#define SPECENUM_VALUE29NAME "Incite_Cost_Pct"
#define SPECENUM_VALUE30 EFT_SIZE_ADJ
#define SPECENUM_VALUE30NAME "Size_Adj"
#define SPECENUM_VALUE31 EFT_SIZE_UNLIMIT
#define SPECENUM_VALUE31NAME "Size_Unlimit"
#define SPECENUM_VALUE32 EFT_SS_STRUCTURAL
#define SPECENUM_VALUE32NAME "SS_Structural"
#define SPECENUM_VALUE33 EFT_SS_COMPONENT
#define SPECENUM_VALUE33NAME "SS_Component"
#define SPECENUM_VALUE34 EFT_SS_MODULE
#define SPECENUM_VALUE34NAME "SS_Module"
#define SPECENUM_VALUE35 EFT_SPY_RESISTANT
#define SPECENUM_VALUE35NAME "Spy_Resistant"
#define SPECENUM_VALUE36 EFT_MOVE_BONUS
#define SPECENUM_VALUE36NAME "Move_Bonus"
#define SPECENUM_VALUE37 EFT_UNIT_NO_LOSE_POP
#define SPECENUM_VALUE37NAME "Unit_No_Lose_Pop"
#define SPECENUM_VALUE38 EFT_UNIT_RECOVER
#define SPECENUM_VALUE38NAME "Unit_Recover"
#define SPECENUM_VALUE39 EFT_UPGRADE_UNIT
#define SPECENUM_VALUE39NAME "Upgrade_Unit"
#define SPECENUM_VALUE40 EFT_UPKEEP_FREE
#define SPECENUM_VALUE40NAME "Upkeep_Free"
#define SPECENUM_VALUE41 EFT_TECH_UPKEEP_FREE
#define SPECENUM_VALUE41NAME "Tech_Upkeep_Free"
#define SPECENUM_VALUE42 EFT_NO_UNHAPPY
#define SPECENUM_VALUE42NAME "No_Unhappy"
#define SPECENUM_VALUE43 EFT_VETERAN_BUILD
#define SPECENUM_VALUE43NAME "Veteran_Build"
#define SPECENUM_VALUE44 EFT_VETERAN_COMBAT
#define SPECENUM_VALUE44NAME "Veteran_Combat"
#define SPECENUM_VALUE45 EFT_HP_REGEN
#define SPECENUM_VALUE45NAME "HP_Regen"
#define SPECENUM_VALUE46 EFT_CITY_VISION_RADIUS_SQ
#define SPECENUM_VALUE46NAME "City_Vision_Radius_Sq"
#define SPECENUM_VALUE47 EFT_UNIT_VISION_RADIUS_SQ
#define SPECENUM_VALUE47NAME "Unit_Vision_Radius_Sq"
/* Interacts with UTYF_BADWALLATTACKER */
#define SPECENUM_VALUE48 EFT_DEFEND_BONUS
#define SPECENUM_VALUE48NAME "Defend_Bonus"
#define SPECENUM_VALUE49 EFT_TRADEROUTE_PCT
#define SPECENUM_VALUE49NAME "Traderoute_Pct"
#define SPECENUM_VALUE50 EFT_GAIN_AI_LOVE
#define SPECENUM_VALUE50NAME "Gain_AI_Love"
#define SPECENUM_VALUE51 EFT_TURN_YEARS
#define SPECENUM_VALUE51NAME "Turn_Years"
#define SPECENUM_VALUE52 EFT_SLOW_DOWN_TIMELINE
#define SPECENUM_VALUE52NAME "Slow_Down_Timeline"
#define SPECENUM_VALUE53 EFT_CIVIL_WAR_CHANCE
#define SPECENUM_VALUE53NAME "Civil_War_Chance"
/* change of the migration score */
#define SPECENUM_VALUE54 EFT_MIGRATION_PCT
#define SPECENUM_VALUE54NAME "Migration_Pct"
/* +1 unhappy when more than this cities */
#define SPECENUM_VALUE55 EFT_EMPIRE_SIZE_BASE
#define SPECENUM_VALUE55NAME "Empire_Size_Base"
/* adds additional +1 unhappy steps to above */
#define SPECENUM_VALUE56 EFT_EMPIRE_SIZE_STEP
#define SPECENUM_VALUE56NAME "Empire_Size_Step"
#define SPECENUM_VALUE57 EFT_MAX_RATES
#define SPECENUM_VALUE57NAME "Max_Rates"
#define SPECENUM_VALUE58 EFT_MARTIAL_LAW_EACH
#define SPECENUM_VALUE58NAME "Martial_Law_Each"
#define SPECENUM_VALUE59 EFT_MARTIAL_LAW_MAX
#define SPECENUM_VALUE59NAME "Martial_Law_Max"
#define SPECENUM_VALUE60 EFT_RAPTURE_GROW
#define SPECENUM_VALUE60NAME "Rapture_Grow"
#define SPECENUM_VALUE61 EFT_REVOLUTION_UNHAPPINESS
#define SPECENUM_VALUE61NAME "Revolution_Unhappiness"
#define SPECENUM_VALUE62 EFT_HAS_SENATE
#define SPECENUM_VALUE62NAME "Has_Senate"
#define SPECENUM_VALUE63 EFT_INSPIRE_PARTISANS
#define SPECENUM_VALUE63NAME "Inspire_Partisans"
#define SPECENUM_VALUE64 EFT_HAPPINESS_TO_GOLD
#define SPECENUM_VALUE64NAME "Happiness_To_Gold"
/* stupid special case; we hate it */
#define SPECENUM_VALUE65 EFT_FANATICS
#define SPECENUM_VALUE65NAME "Fanatics"
#define SPECENUM_VALUE66 EFT_NO_DIPLOMACY
#define SPECENUM_VALUE66NAME "No_Diplomacy"
#define SPECENUM_VALUE67 EFT_TRADE_REVENUE_BONUS
#define SPECENUM_VALUE67NAME "Trade_Revenue_Bonus"
/* multiply unhappy upkeep by this effect */
#define SPECENUM_VALUE68 EFT_UNHAPPY_FACTOR
#define SPECENUM_VALUE68NAME "Unhappy_Factor"
/* multiply upkeep by this effect */
#define SPECENUM_VALUE69 EFT_UPKEEP_FACTOR
#define SPECENUM_VALUE69NAME "Upkeep_Factor"
/* this many units are free from upkeep */
#define SPECENUM_VALUE70 EFT_UNIT_UPKEEP_FREE_PER_CITY
#define SPECENUM_VALUE70NAME "Unit_Upkeep_Free_Per_City"
#define SPECENUM_VALUE71 EFT_OUTPUT_WASTE
#define SPECENUM_VALUE71NAME "Output_Waste"
#define SPECENUM_VALUE72 EFT_OUTPUT_WASTE_BY_DISTANCE
#define SPECENUM_VALUE72NAME "Output_Waste_By_Distance"
/* -1 penalty to tiles producing more than this */
#define SPECENUM_VALUE73 EFT_OUTPUT_PENALTY_TILE
#define SPECENUM_VALUE73NAME "Output_Penalty_Tile"
#define SPECENUM_VALUE74 EFT_OUTPUT_INC_TILE_CELEBRATE
#define SPECENUM_VALUE74NAME "Output_Inc_Tile_Celebrate"
/* all citizens after this are unhappy */
#define SPECENUM_VALUE75 EFT_CITY_UNHAPPY_SIZE
#define SPECENUM_VALUE75NAME "City_Unhappy_Size"
/* add to default squared city radius */
#define SPECENUM_VALUE76 EFT_CITY_RADIUS_SQ
#define SPECENUM_VALUE76NAME "City_Radius_Sq"
/* number of build slots for units */
#define SPECENUM_VALUE77 EFT_CITY_BUILD_SLOTS
#define SPECENUM_VALUE77NAME "City_Build_Slots"
#define SPECENUM_VALUE78 EFT_UPGRADE_PRICE_PCT
#define SPECENUM_VALUE78NAME "Upgrade_Price_Pct"
/* City should use walls gfx */
#define SPECENUM_VALUE79 EFT_VISIBLE_WALLS
#define SPECENUM_VALUE79NAME "Visible_Walls"
#define SPECENUM_VALUE80 EFT_TECH_COST_FACTOR
#define SPECENUM_VALUE80NAME "Tech_Cost_Factor"
/* [x%] gold upkeep instead of [1] shield upkeep for units */
#define SPECENUM_VALUE81 EFT_SHIELD2GOLD_FACTOR
#define SPECENUM_VALUE81NAME "Shield2Gold_Factor"
#define SPECENUM_VALUE82 EFT_TILE_WORKABLE
#define SPECENUM_VALUE82NAME "Tile_Workable"
/* The index for the city image of the given city style. */
#define SPECENUM_VALUE83 EFT_CITY_IMAGE
#define SPECENUM_VALUE83NAME "City_Image"
#define SPECENUM_VALUE84 EFT_IMPR_BUILD_COST_PCT
#define SPECENUM_VALUE84NAME "Building_Build_Cost_Pct"
#define SPECENUM_VALUE85 EFT_MAX_TRADE_ROUTES
#define SPECENUM_VALUE85NAME "Max_Trade_Routes"
#define SPECENUM_VALUE86 EFT_GOV_CENTER
#define SPECENUM_VALUE86NAME "Gov_Center"
#define SPECENUM_VALUE87 EFT_COMBAT_ROUNDS
#define SPECENUM_VALUE87NAME "Combat_Rounds"
#define SPECENUM_VALUE88 EFT_IMPR_BUY_COST_PCT
#define SPECENUM_VALUE88NAME "Building_Buy_Cost_Pct"
#define SPECENUM_VALUE89 EFT_UNIT_BUILD_COST_PCT
#define SPECENUM_VALUE89NAME "Unit_Build_Cost_Pct"
#define SPECENUM_VALUE90 EFT_UNIT_BUY_COST_PCT
#define SPECENUM_VALUE90NAME "Unit_Buy_Cost_Pct"
#define SPECENUM_VALUE91 EFT_NOT_TECH_SOURCE
#define SPECENUM_VALUE91NAME "Not_Tech_Source"
#define SPECENUM_VALUE92 EFT_ENEMY_CITIZEN_UNHAPPY_PCT
#define SPECENUM_VALUE92NAME "Enemy_Citizen_Unhappy_Pct"
#define SPECENUM_VALUE93 EFT_IRRIGATION_PCT
#define SPECENUM_VALUE93NAME "Irrigation_Pct"
#define SPECENUM_VALUE94 EFT_MINING_PCT
#define SPECENUM_VALUE94NAME "Mining_Pct"
#define SPECENUM_VALUE95 EFT_OUTPUT_TILE_PUNISH_PCT
#define SPECENUM_VALUE95NAME "Output_Tile_Punish_Pct"
#define SPECENUM_VALUE96 EFT_UNIT_BRIBE_COST_PCT
#define SPECENUM_VALUE96NAME "Unit_Bribe_Cost_Pct"
#define SPECENUM_VALUE97 EFT_VICTORY
#define SPECENUM_VALUE97NAME "Victory"
#define SPECENUM_VALUE98 EFT_PERFORMANCE
#define SPECENUM_VALUE98NAME "Performance"
#define SPECENUM_VALUE99 EFT_HISTORY
#define SPECENUM_VALUE99NAME "History"
#define SPECENUM_VALUE100 EFT_NATION_PERFORMANCE
#define SPECENUM_VALUE100NAME "National_Performance"
#define SPECENUM_VALUE101 EFT_NATION_HISTORY
#define SPECENUM_VALUE101NAME "National_History"
#define SPECENUM_VALUE102 EFT_TURN_FRAGMENTS
#define SPECENUM_VALUE102NAME "Turn_Fragments"
#define SPECENUM_VALUE103 EFT_MAX_STOLEN_GOLD_PM
#define SPECENUM_VALUE103NAME "Max_Stolen_Gold_Pm"
#define SPECENUM_VALUE104 EFT_THIEFS_SHARE_PM
#define SPECENUM_VALUE104NAME "Thiefs_Share_Pm"
#define SPECENUM_VALUE105 EFT_RETIRE_PCT
#define SPECENUM_VALUE105NAME "Retire_Pct"
#define SPECENUM_VALUE106 EFT_ILLEGAL_ACTION_MOVE_COST
#define SPECENUM_VALUE106NAME "Illegal_Action_Move_Cost"
#define SPECENUM_VALUE107 EFT_HAVE_CONTACTS
#define SPECENUM_VALUE107NAME "Have_Contacts"
#define SPECENUM_VALUE108 EFT_CASUS_BELLI_CAUGHT
#define SPECENUM_VALUE108NAME "Casus_Belli_Caught"
#define SPECENUM_VALUE109 EFT_CASUS_BELLI_SUCCESS
#define SPECENUM_VALUE109NAME "Casus_Belli_Success"
#define SPECENUM_VALUE110 EFT_ACTION_ODDS_PCT
#define SPECENUM_VALUE110NAME "Action_Odds_Pct"
#define SPECENUM_VALUE111 EFT_BORDER_VISION
#define SPECENUM_VALUE111NAME "Border_Vision"
#define SPECENUM_VALUE112 EFT_STEALINGS_IGNORE
#define SPECENUM_VALUE112NAME "Stealings_Ignore"
#define SPECENUM_VALUE113 EFT_OUTPUT_WASTE_BY_REL_DISTANCE
#define SPECENUM_VALUE113NAME "Output_Waste_By_Rel_Distance"
#define SPECENUM_VALUE114 EFT_SABOTEUR_RESISTANT
#define SPECENUM_VALUE114NAME "Building_Saboteur_Resistant"
#define SPECENUM_VALUE115 EFT_UNIT_SLOTS
#define SPECENUM_VALUE115NAME "Unit_Slots"
#define SPECENUM_VALUE116 EFT_ATTACK_BONUS
#define SPECENUM_VALUE116NAME "Attack_Bonus"
#define SPECENUM_VALUE117 EFT_CONQUEST_TECH_PCT
#define SPECENUM_VALUE117NAME "Conquest_Tech_Pct"
/* keep this last */
#define SPECENUM_COUNT EFT_COUNT
#include "specenum_gen.h"

/* An effect is provided by a source.  If the source is present, and the
 * other conditions (described below) are met, the effect will be active.
 * Note the difference between effect and effect_type. */
struct effect {
  enum effect_type type;

  /* pointer to multipliers (NULL means that this effect has no multiplier  */
  struct  multiplier *multiplier;

  /* The "value" of the effect.  The meaning of this varies between
   * effects.  When get_xxx_bonus() is called the value of all applicable
   * effects will be summed up. */
  int value;

  /* An effect can have multiple requirements.  The effect will only be
   * active if all of these requirement are met. */
  struct requirement_vector reqs;
};

/* An effect_list is a list of effects. */
#define SPECLIST_TAG effect
#define SPECLIST_TYPE struct effect
#include "speclist.h"
#define effect_list_iterate(effect_list, peffect) \
  TYPED_LIST_ITERATE(struct effect, effect_list, peffect)
#define effect_list_iterate_end LIST_ITERATE_END

struct effect *effect_new(enum effect_type type, int value,
                          struct multiplier *pmul);
struct effect *effect_copy(struct effect *old);
void effect_req_append(struct effect *peffect, struct requirement req);

struct astring;
void get_effect_req_text(const struct effect *peffect,
                         char *buf, size_t buf_len);
void get_effect_list_req_text(const struct effect_list *plist,
                              struct astring *astr);

/* ruleset cache creation and communication functions */
struct packet_ruleset_effect;

void ruleset_cache_init(void);
void ruleset_cache_free(void);
void recv_ruleset_effect(const struct packet_ruleset_effect *packet);
void send_ruleset_cache(struct conn_list *dest);

int effect_cumulative_max(enum effect_type type, struct universal *for_uni);
int effect_cumulative_min(enum effect_type type, struct universal *for_uni);

int effect_value_from_universals(enum effect_type type,
                                 struct universal *unis, size_t n_unis);

bool is_building_replaced(const struct city *pcity,
			  struct impr_type *pimprove,
                          const enum req_problem_type prob_type);

/* functions to know the bonuses a certain effect is granting */
int get_world_bonus(enum effect_type effect_type);
int get_player_bonus(const struct player *plr, enum effect_type effect_type);
int get_city_bonus(const struct city *pcity, enum effect_type effect_type);
int get_city_specialist_output_bonus(const struct city *pcity,
				     const struct specialist *pspecialist,
				     const struct output_type *poutput,
				     enum effect_type effect_type);
int get_city_tile_output_bonus(const struct city *pcity,
			       const struct tile *ptile,
			       const struct output_type *poutput,
			       enum effect_type effect_type);
int get_tile_output_bonus(const struct city *pcity,
                          const struct tile *ptile,
                          const struct output_type *poutput,
                          enum effect_type effect_type);
int get_player_output_bonus(const struct player *pplayer,
                            const struct output_type *poutput,
                            enum effect_type effect_type);
int get_city_output_bonus(const struct city *pcity,
                          const struct output_type *poutput,
                          enum effect_type effect_type);
int get_building_bonus(const struct city *pcity,
		       const struct impr_type *building,
		       enum effect_type effect_type);
int get_unittype_bonus(const struct player *pplayer,
		       const struct tile *ptile, /* pcity is implied */
		       const struct unit_type *punittype,
		       enum effect_type effect_type);
int get_unit_bonus(const struct unit *punit, enum effect_type effect_type);
int get_tile_bonus(const struct tile *ptile, const struct unit *punit,
                   enum effect_type etype);

/* miscellaneous auxiliary effects functions */
struct effect_list *get_req_source_effects(struct universal *psource);

int get_player_bonus_effects(struct effect_list *plist,
                             const struct player *pplayer,
                             enum effect_type effect_type);
int get_city_bonus_effects(struct effect_list *plist,
			   const struct city *pcity,
			   const struct output_type *poutput,
			   enum effect_type effect_type);

int get_target_bonus_effects(struct effect_list *plist,
                             const struct player *target_player,
                             const struct player *other_player,
                             const struct city *target_city,
                             const struct impr_type *target_building,
                             const struct tile *target_tile,
                             const struct unit *target_unit,
                             const struct unit_type *target_unittype,
                             const struct output_type *target_output,
                             const struct specialist *target_specialist,
                             const struct action *target_action,
                             enum effect_type effect_type);

bool building_has_effect(const struct impr_type *pimprove,
			 enum effect_type effect_type);
int get_current_construction_bonus(const struct city *pcity,
                                   enum effect_type effect_type,
                                   const enum req_problem_type prob_type);
int get_potential_improvement_bonus(struct impr_type *pimprove,
                                    const struct city *pcity,
                                    enum effect_type effect_type,
                                    const enum req_problem_type prob_type);

struct effect_list *get_effects(enum effect_type effect_type);

typedef bool (*iec_cb)(struct effect*, void *data);
bool iterate_effect_cache(iec_cb cb, void *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__EFFECTS_H */
