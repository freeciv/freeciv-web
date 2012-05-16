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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "government.h"
#include "idex.h"
#include "improvement.h"
#include "map.h"
#include "tech.h"
#include "unit.h"
#include "unitlist.h"
#include "vision.h"

#include "player.h"

/* Names of AI levels. These must correspond to enum ai_level in
 * player.h. Also commands to set AI level in server/commands.c
 * must match these. */
static const char *ai_level_names[] = {
  NULL, N_("Away"), N_("Novice"), N_("Easy"), NULL, N_("Normal"),
  NULL, N_("Hard"), N_("Cheating"), NULL, N_("Experimental")
};

/***************************************************************
  Returns true iff p1 can cancel treaty on p2.

  The senate may not allow you to break the treaty.  In this 
  case you must first dissolve the senate then you can break 
  it.  This is waived if you have statue of liberty since you 
  could easily just dissolve and then recreate it. 
***************************************************************/
enum dipl_reason pplayer_can_cancel_treaty(const struct player *p1, 
                                           const struct player *p2)
{
  enum diplstate_type ds = pplayer_get_diplstate(p1, p2)->type;

  if (p1 == p2 || ds == DS_WAR) {
    return DIPL_ERROR;
  }
  if (players_on_same_team(p1, p2)) {
    return DIPL_ERROR;
  }
  if (p1->diplstates[player_index(p2)].has_reason_to_cancel == 0
      && get_player_bonus(p1, EFT_HAS_SENATE) > 0
      && get_player_bonus(p1, EFT_ANY_GOVERNMENT) == 0) {
    return DIPL_SENATE_BLOCKING;
  }
  return DIPL_OK;
}

/***************************************************************
  Returns true iff p1 can be in alliance with p2.

  Check that we are not at war with any of p2's allies. Note 
  that for an alliance to be made, we need to check this both 
  ways.

  The reason for this is to avoid the dread 'love-love-hate' 
  triad, in which p1 is allied to p2 is allied to p3 is at
  war with p1. These lead to strange situations.
***************************************************************/
static bool is_valid_alliance(const struct player *p1, 
                              const struct player *p2)
{
  players_iterate(pplayer) {
    enum diplstate_type ds = pplayer_get_diplstate(p1, pplayer)->type;

    if (pplayer != p1
        && pplayer != p2
        && pplayers_allied(p2, pplayer)
        && ds == DS_WAR /* do not count 'never met' as war here */
        && pplayer->is_alive) {
      return FALSE;
    }
  } players_iterate_end;

  return TRUE;
}

/***************************************************************
  Returns true iff p1 can make given treaty with p2.

  We cannot regress in a treaty chain. So we cannot suggest
  'Peace' if we are in 'Alliance'. Then you have to cancel.

  For alliance there is only one condition: We are not at war 
  with any of p2's allies.
***************************************************************/
enum dipl_reason pplayer_can_make_treaty(const struct player *p1,
                                         const struct player *p2,
                                         enum diplstate_type treaty)
{
  enum diplstate_type existing = pplayer_get_diplstate(p1, p2)->type;

  if (p1 == p2) {
    return DIPL_ERROR; /* duh! */
  }
  if (get_player_bonus(p1, EFT_NO_DIPLOMACY)
      || get_player_bonus(p2, EFT_NO_DIPLOMACY)) {
    return DIPL_ERROR;
  }
  if (treaty == DS_WAR 
      || treaty == DS_NO_CONTACT 
      || treaty == DS_ARMISTICE 
      || treaty == DS_TEAM
      || treaty == DS_LAST) {
    return DIPL_ERROR; /* these are not negotiable treaties */
  }
  if (treaty == DS_CEASEFIRE && existing != DS_WAR) {
    return DIPL_ERROR; /* only available from war */
  }
  if (treaty == DS_PEACE 
      && (existing != DS_WAR && existing != DS_CEASEFIRE)) {
    return DIPL_ERROR;
  }
  if (treaty == DS_ALLIANCE
      && (!is_valid_alliance(p1, p2) || !is_valid_alliance(p2, p1))) {
    return DIPL_ALLIANCE_PROBLEM;
  }
  /* this check must be last: */
  if (treaty == existing) {
    return DIPL_ERROR;
  }
  return DIPL_OK;
}

/***************************************************************
  Check if pplayer has an embassy with pplayer2. We always have
  an embassy with ourselves.
***************************************************************/
bool player_has_embassy(const struct player *pplayer,
			const struct player *pplayer2)
{
  return (BV_ISSET(pplayer->embassy, player_index(pplayer2))
          || (pplayer == pplayer2)
          || (get_player_bonus(pplayer, EFT_HAVE_EMBASSIES) > 0
              && !is_barbarian(pplayer2)));
}

/****************************************************************************
  Return TRUE iff the given player owns the city.
****************************************************************************/
bool player_owns_city(const struct player *pplayer, const struct city *pcity)
{
  return (pcity && pplayer && city_owner(pcity) == pplayer);
}

/****************************************************************************
  Return TRUE iff the player can invade a particular tile (linked with
  borders and diplomatic states).
****************************************************************************/
bool player_can_invade_tile(const struct player *pplayer,
                            const struct tile *ptile)
{
  const struct player *ptile_owner = tile_owner(ptile);

  return (!ptile_owner
          || ptile_owner == pplayer
          || !players_non_invade(pplayer, ptile_owner));
}

/***************************************************************
  In the server you must use server_player_init.  Note that
  this function is matched by game_remove_player() in game.c,
  there is no corresponding player_free() in this file.
***************************************************************/
void player_init(struct player *plr)
{
  int i;

  sz_strlcpy(plr->name, ANON_PLAYER_NAME);
  sz_strlcpy(plr->username, ANON_USER_NAME);
  sz_strlcpy(plr->ranked_username, ANON_USER_NAME);
  plr->user_turns = 0;
  plr->is_male = TRUE;
  plr->government = NULL;
  plr->target_government = NULL;
  plr->nation = NO_NATION_SELECTED;
  plr->team = NULL;
  plr->is_ready = FALSE;

  plr->revolution_finishes = -1;
  plr->capital = FALSE;
  plr->city_style=0;            /* should be first basic style */
  plr->cities = city_list_new();
  plr->units = unit_list_new();

  plr->connections = conn_list_new();
  plr->current_conn = NULL;
  plr->is_connected = FALSE;

  plr->was_created = FALSE;
  plr->is_alive=TRUE;
  plr->is_dying = FALSE;
  plr->is_winner = FALSE;
  plr->surrendered = FALSE;
  BV_CLR_ALL(plr->embassy);
  for(i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    plr->diplstates[i].type = DS_NO_CONTACT;
    plr->diplstates[i].has_reason_to_cancel = 0;
    plr->diplstates[i].contact_turns_left = 0;
  }
  plr->ai = NULL;
  plr->ai_data.control=FALSE;
  BV_CLR_ALL(plr->ai_data.handicaps);
  plr->ai_data.skill_level = 0;
  plr->ai_data.fuzzy = 0;
  plr->ai_data.expand = 100;
  plr->ai_data.barbarian_type = NOT_A_BARBARIAN;
  plr->economic.tax=PLAYER_DEFAULT_TAX_RATE;
  plr->economic.science=PLAYER_DEFAULT_SCIENCE_RATE;
  plr->economic.luxury=PLAYER_DEFAULT_LUXURY_RATE;

  plr->economic = player_limit_to_max_rates(plr);
  spaceship_init(&plr->spaceship);

  plr->gives_shared_vision = 0;
  plr->really_gives_vision = 0;

  for (i = 0; i < B_LAST; i++) {
    plr->wonders[i] = WONDER_NOT_BUILT;
  }

  plr->attribute_block.data = NULL;
  plr->attribute_block.length = 0;
  plr->attribute_block_buffer.data = NULL;
  plr->attribute_block_buffer.length = 0;
  BV_CLR_ALL(plr->debug);
}

/**************************************************************************
  Return the number of players.
**************************************************************************/
int player_count(void)
{
  return game.nplayers;
}

/**************************************************************************
  Set the number of players.
**************************************************************************/
void set_player_count(int count)
{
  game.nplayers = count;
}

/**************************************************************************
  Return the player index.

  Currently same as player_number(), paired with player_count()
  indicates use as an array index.
**************************************************************************/
int player_index(const struct player *pplayer)
{
  return player_number(pplayer);
}

/**************************************************************************
  Return the player index/number/id.
**************************************************************************/
int player_number(const struct player *pplayer)
{
  assert(pplayer);
  return pplayer - game.players;
}

/**************************************************************************
  Return struct player pointer for the given player index.

  You can retrieve players that are not in the game (with IDs larger than
  player_count).  An out-of-range player request will return NULL.
**************************************************************************/
struct player *player_by_number(const int player_id)
{
  return player_slot_by_number(player_id);
}

/**************************************************************************
  Return pointer iff the player ID refers to an in-game player.
**************************************************************************/
struct player *valid_player_by_number(const int player_id)
{
  struct player *pslot;

  pslot = player_slot_by_number(player_id);

  if (!player_slot_is_used(pslot)) {
    return NULL;
  }
  return pslot;
}

/****************************************************************************
  Set the player's nation to the given nation (may be NULL).  Returns TRUE
  iff there was a change.
****************************************************************************/
bool player_set_nation(struct player *pplayer, struct nation_type *pnation)
{
  if (pplayer->nation != pnation) {
    if (pplayer->nation) {
      assert(pplayer->nation->player == pplayer);
      pplayer->nation->player = NULL;
    }
    if (pnation) {
      assert(pnation->player == NULL);
      pnation->player = pplayer;
    }
    pplayer->nation = pnation;
    return TRUE;
  }
  return FALSE;
}

/***************************************************************
...
***************************************************************/
void player_set_unit_focus_status(struct player *pplayer)
{
  unit_list_iterate(pplayer->units, punit) 
    punit->focus_status=FOCUS_AVAIL;
  unit_list_iterate_end;
}

/***************************************************************
...
***************************************************************/
struct player *find_player_by_name(const char *name)
{
  players_iterate(pplayer) {
    if (mystrcasecmp(name, pplayer->name) == 0) {
      return pplayer;
    }
  } players_iterate_end;

  return NULL;
}

/**************************************************************************
  Return the leader name of the player.
**************************************************************************/
const char *player_name(const struct player *pplayer)
{
  if (!pplayer) {
    return NULL;
  }
  return pplayer->name;
}

/***************************************************************
  Find player by name, allowing unambigous prefix (ie abbreviation).
  Returns NULL if could not match, or if ambiguous or other
  problem, and fills *result with characterisation of match/non-match
  (see shared.[ch])
***************************************************************/
static const char *player_slot_name_by_number(int i)
{
  struct player *pplayer;
  
  pplayer = valid_player_by_number(i);
  return player_name(pplayer);
}

/***************************************************************
Find player by its name prefix
***************************************************************/
struct player *find_player_by_name_prefix(const char *name,
					  enum m_pre_result *result)
{
  int ind;

  *result = match_prefix(player_slot_name_by_number,
                         player_slot_count(), MAX_LEN_NAME-1,
                         mystrncasequotecmp, effectivestrlenquote,
                         name, &ind);

  if (*result < M_PRE_AMBIGUOUS) {
    return valid_player_by_number(ind);
  } else {
    return NULL;
  }
}

/***************************************************************
Find player by its user name (not player/leader name)
***************************************************************/
struct player *find_player_by_user(const char *name)
{
  players_iterate(pplayer) {
    if (mystrcasecmp(name, pplayer->username) == 0) {
      return pplayer;
    }
  } players_iterate_end;
  
  return NULL;
}

/****************************************************************************
  Checks if a unit can be seen by pplayer at (x,y).
  A player can see a unit if he:
  (a) can see the tile AND
  (b) can see the unit at the tile (i.e. unit not invisible at this tile) AND
  (c) the unit is outside a city OR in an allied city AND
  (d) the unit isn't in a transporter, or we are allied AND
  (e) the unit isn't in a transporter, or we can see the transporter
****************************************************************************/
bool can_player_see_unit_at(const struct player *pplayer,
			    const struct unit *punit,
			    const struct tile *ptile)
{
  struct city *pcity;

  /* If the player can't even see the tile... */
  if (TILE_KNOWN_SEEN != tile_get_known(ptile, pplayer)) {
    return FALSE;
  }

  /* Don't show non-allied units that are in transports.  This is logical
   * because allied transports can also contain our units.  Shared vision
   * isn't taken into account. */
  if (punit->transported_by != -1 && unit_owner(punit) != pplayer
      && !pplayers_allied(pplayer, unit_owner(punit))) {
    return FALSE;
  }

  /* Units in cities may be hidden. */
  pcity = tile_city(ptile);
  if (pcity && !can_player_see_units_in_city(pplayer, pcity)) {
    return FALSE;
  }

  /* Allied or non-hiding units are always seen. */
  if (pplayers_allied(unit_owner(punit), pplayer)
      || !is_hiding_unit(punit)) {
    return TRUE;
  }

  /* Hiding units are only seen by the V_INVIS fog layer. */
  return BV_ISSET(ptile->tile_seen[V_INVIS], player_index(pplayer));

  return FALSE;
}


/****************************************************************************
  Checks if a unit can be seen by pplayer at its current location.

  See can_player_see_unit_at.
****************************************************************************/
bool can_player_see_unit(const struct player *pplayer,
			 const struct unit *punit)
{
  return can_player_see_unit_at(pplayer, punit, punit->tile);
}

/****************************************************************************
  Return TRUE iff the player can see units in the city.  Either they
  can see all units or none.

  If the player can see units in the city, then the server sends the
  unit info for units in the city to the client.  The client uses the
  tile's unitlist to determine whether to show the city occupied flag.  Of
  course the units will be visible to the player as well, if he clicks on
  them.

  If the player can't see units in the city, then the server doesn't send
  the unit info for these units.  The client therefore uses the "occupied"
  flag sent in the short city packet to determine whether to show the city
  occupied flag.

  Note that can_player_see_city_internals => can_player_see_units_in_city.
  Otherwise the player would not know anything about the city's units at
  all, since the full city packet has no "occupied" flag.

  Returns TRUE if given a NULL player.  This is used by the client when in
  observer mode.
****************************************************************************/
bool can_player_see_units_in_city(const struct player *pplayer,
				  const struct city *pcity)
{
  return (!pplayer
	  || can_player_see_city_internals(pplayer, pcity)
	  || pplayers_allied(pplayer, city_owner(pcity)));
}

/****************************************************************************
  Return TRUE iff the player can see the city's internals.  This means the
  full city packet is sent to the client, who should then be able to popup
  a dialog for it.

  Returns TRUE if given a NULL player.  This is used by the client when in
  observer mode.
****************************************************************************/
bool can_player_see_city_internals(const struct player *pplayer,
				   const struct city *pcity)
{
  return (!pplayer || pplayer == city_owner(pcity));
}

/***************************************************************
 If the specified player owns the city with the specified id,
 return pointer to the city struct.  Else return NULL.
 Now always uses fast idex_lookup_city.

 pplayer may be NULL in which case all cities registered to
 hash are considered - even those not currently owned by any
 player. Callers expect this behavior.
***************************************************************/
struct city *player_find_city_by_id(const struct player *pplayer,
				    int city_id)
{
  /* We call idex directly. Should use game_find_city_by_id()
   * instead? */
  struct city *pcity = idex_lookup_city(city_id);

  if (!pcity) {
    return NULL;
  }

  if (!pplayer || (city_owner(pcity) == pplayer)) {
    /* Correct owner */
    return pcity;
  }

  return NULL;
}

/***************************************************************
 If the specified player owns the unit with the specified id,
 return pointer to the unit struct.  Else return NULL.
 Uses fast idex_lookup_city.

 pplayer may be NULL in which case all units registered to
 hash are considered - even those not currently owned by any
 player. Callers expect this behavior.
***************************************************************/
struct unit *player_find_unit_by_id(const struct player *pplayer,
				    int unit_id)
{
  /* We call idex directly. Should use game_find_unit_by_number()
   * instead? */
  struct unit *punit = idex_lookup_unit(unit_id);

  if (!punit) {
    return NULL;
  }

  if (!pplayer || (unit_owner(punit) == pplayer)) {
    /* Correct owner */
    return punit;
  }

  return NULL;
}

/*************************************************************************
Return 1 if x,y is inside any of the player's city radii.
**************************************************************************/
bool player_in_city_radius(const struct player *pplayer,
			   const struct tile *ptile)
{
  city_tile_iterate(ptile, ptile1) {
    struct city *pcity = tile_city(ptile1);

    if (pcity && city_owner(pcity) == pplayer) {
      return TRUE;
    }
  } city_tile_iterate_end;

  return FALSE;
}

/**************************************************************************
 Returns the number of techs the player has researched which has this
 flag. Needs to be optimized later (e.g. int tech_flags[TF_LAST] in
 struct player)
**************************************************************************/
int num_known_tech_with_flag(const struct player *pplayer,
			     enum tech_flag_id flag)
{
  return get_player_research(pplayer)->num_known_tech_with_flag[flag];
}

/**************************************************************************
  Return the expected net income of the player this turn.  This includes
  tax revenue and upkeep, but not one-time purchases or found gold.

  This function depends on pcity->prod[O_GOLD] being set for all cities, so
  make sure the player's cities have been refreshed.
**************************************************************************/
int player_get_expected_income(const struct player *pplayer)
{
  int income = 0;

  /* City income/expenses. */
  city_list_iterate(pplayer->cities, pcity) {
    /* Gold suplus accounts for imcome plus building and unit upkeep. */
    income += pcity->surplus[O_GOLD];

    /* Gold upkeep for buildings and units is defined by the setting
     * 'game.info.gold_upkeep_style':
     * 0: cities pay for buildings and units (this is included in
     *    pcity->surplus[O_GOLD])
     * 1: cities pay only for buildings; the nation pays for units
     * 2: the nation pays for buildings and units */
    if (game.info.gold_upkeep_style > 0) {
      switch (game.info.gold_upkeep_style) {
        case 2:
          /* nation pays for buildings (and units) */
          income -= city_total_impr_gold_upkeep(pcity);
          /* no break */
        case 1:
          /* nation pays for units */
          income -= city_total_unit_gold_upkeep(pcity);
          break;
        default:
          /* fallthru */
          break;
      }
    }

    /* Capitalization income. */
    if (city_production_has_flag(pcity, IF_GOLD)) {
      income += pcity->shield_stock + pcity->surplus[O_SHIELD];
    }
  } city_list_iterate_end;

  return income;
}

/**************************************************************************
 Returns TRUE iff the player knows at least one tech which has the
 given flag.
**************************************************************************/
bool player_knows_techs_with_flag(const struct player *pplayer,
				  enum tech_flag_id flag)
{
  return num_known_tech_with_flag(pplayer, flag) > 0;
}

/**************************************************************************
The following limits a player's rates to those that are acceptable for the
present form of government.  If a rate exceeds maxrate for this government,
it adjusts rates automatically adding the extra to the 2nd highest rate,
preferring science to taxes and taxes to luxuries.
(It assumes that for any government maxrate>=50)
Returns actual max rate used.
**************************************************************************/
struct player_economic player_limit_to_max_rates(struct player *pplayer)
{
  int maxrate, surplus;
  struct player_economic economic;

  /* ai players allowed to cheat */
  if (pplayer->ai_data.control) {
    return pplayer->economic;
  }

  economic = pplayer->economic;

  maxrate = get_player_bonus(pplayer, EFT_MAX_RATES);
  if (maxrate == 0) {
    maxrate = 100; /* effects not initialized yet */
  }

  surplus = 0;
  if (economic.luxury > maxrate) {
    surplus += economic.luxury - maxrate;
    economic.luxury = maxrate;
  }
  if (economic.tax > maxrate) {
    surplus += economic.tax - maxrate;
    economic.tax = maxrate;
  }
  if (economic.science > maxrate) {
    surplus += economic.science - maxrate;
    economic.science = maxrate;
  }

  assert(surplus % 10 == 0);
  while (surplus > 0) {
    if (economic.science < maxrate) {
      economic.science += 10;
    } else if (economic.tax < maxrate) {
      economic.tax += 10;
    } else if (economic.luxury < maxrate) {
      economic.luxury += 10;
    } else {
      die("byebye");
    }
    surplus -= 10;
  }

  return economic;
}

/**************************************************************************
Locate the city where the players palace is located, (NULL Otherwise) 
**************************************************************************/
struct city *find_palace(const struct player *pplayer)
{
  if (!pplayer) {
    /* The client depends on this behavior in some places. */
    return NULL;
  }
  city_list_iterate(pplayer->cities, pcity) {
    if (is_capital(pcity)) {
      return pcity;
    }
  } city_list_iterate_end;
  return NULL;
}

/**************************************************************************
  AI players may have handicaps - allowing them to cheat or preventing
  them from using certain algorithms.  This function returns whether the
  player has the given handicap.  Human players are assumed to have no
  handicaps.
**************************************************************************/
bool ai_handicap(const struct player *pplayer, enum handicap_type htype)
{
  if (!pplayer->ai_data.control) {
    return TRUE;
  }
  return BV_ISSET(pplayer->ai_data.handicaps, htype);
}

/**************************************************************************
Return the value normal_decision (a boolean), except if the AI is fuzzy,
then sometimes flip the value.  The intention of this is that instead of
    if (condition) { action }
you can use
    if (ai_fuzzy(pplayer, condition)) { action }
to sometimes flip a decision, to simulate an AI with some confusion,
indecisiveness, forgetfulness etc. In practice its often safer to use
    if (condition && ai_fuzzy(pplayer,1)) { action }
for an action which only makes sense if condition holds, but which a
fuzzy AI can safely "forget".  Note that for a non-fuzzy AI, or for a
human player being helped by the AI (eg, autosettlers), you can ignore
the "ai_fuzzy(pplayer," part, and read the previous example as:
    if (condition && 1) { action }
--dwp
**************************************************************************/
bool ai_fuzzy(const struct player *pplayer, bool normal_decision)
{
  if (!pplayer->ai_data.control || pplayer->ai_data.fuzzy == 0) {
    return normal_decision;
  }
  if (myrand(1000) >= pplayer->ai_data.fuzzy) {
    return normal_decision;
  }
  return !normal_decision;
}



/**************************************************************************
  Return a text describing an AI's love for you.  (Oooh, kinky!!)
  These words should be adjectives which can fit in the sentence
  "The x are y towards us"
  "The Babylonians are respectful towards us"
**************************************************************************/
const char *love_text(const int love)
{
  if (love <= - MAX_AI_LOVE * 90 / 100) {
    return Q_("?attitude:Genocidal");
  } else if (love <= - MAX_AI_LOVE * 70 / 100) {
    return Q_("?attitude:Belligerent");
  } else if (love <= - MAX_AI_LOVE * 50 / 100) {
    return Q_("?attitude:Hostile");
  } else if (love <= - MAX_AI_LOVE * 25 / 100) {
    return Q_("?attitude:Uncooperative");
  } else if (love <= - MAX_AI_LOVE * 10 / 100) {
    return Q_("?attitude:Uneasy");
  } else if (love <= MAX_AI_LOVE * 10 / 100) {
    return Q_("?attitude:Neutral");
  } else if (love <= MAX_AI_LOVE * 25 / 100) {
    return Q_("?attitude:Respectful");
  } else if (love <= MAX_AI_LOVE * 50 / 100) {
    return Q_("?attitude:Helpful");
  } else if (love <= MAX_AI_LOVE * 70 / 100) {
    return Q_("?attitude:Enthusiastic");
  } else if (love <= MAX_AI_LOVE * 90 / 100) {
    return Q_("?attitude:Admiring");
  } else {
    assert(love > MAX_AI_LOVE * 90 / 100);
    return Q_("?attitude:Worshipful");
  }
}

/**************************************************************************
  Return a diplomatic state as a human-readable string
**************************************************************************/
const char *diplstate_text(const enum diplstate_type type)
{
  static const char *ds_names[DS_LAST] = 
  {
    N_("?diplomatic_state:Armistice"),
    N_("?diplomatic_state:War"), 
    N_("?diplomatic_state:Cease-fire"),
    N_("?diplomatic_state:Peace"),
    N_("?diplomatic_state:Alliance"),
    N_("?diplomatic_state:Never met"),
    N_("?diplomatic_state:Team")
  };

  if (type < DS_LAST) {
    return Q_(ds_names[type]);
  }
  die("Bad diplstate_type in diplstate_text: %d", type);
  return NULL;
}

/***************************************************************
returns diplomatic state type between two players
***************************************************************/
const struct player_diplstate *pplayer_get_diplstate(const struct player *pplayer,
						     const struct player *pplayer2)
{
  return &(pplayer->diplstates[player_index(pplayer2)]);
}

/***************************************************************
  Returns true iff players can attack each other.
***************************************************************/
bool pplayers_at_war(const struct player *pplayer,
                     const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;
  if (pplayer == pplayer2) {
    return FALSE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return TRUE;
  }
  return ds == DS_WAR || ds == DS_NO_CONTACT;
}

/***************************************************************
  Returns true iff players are allied.
***************************************************************/
bool pplayers_allied(const struct player *pplayer,
                     const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;
  if (pplayer == pplayer2) {
    return TRUE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return FALSE;
  }
  return (ds == DS_ALLIANCE || ds == DS_TEAM);
}

/***************************************************************
  Returns true iff players are allied or at peace.
***************************************************************/
bool pplayers_in_peace(const struct player *pplayer,
                       const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;

  if (pplayer == pplayer2) {
    return TRUE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return FALSE;
  }
  return (ds == DS_PEACE || ds == DS_ALLIANCE || ds == DS_TEAM);
}

/****************************************************************************
  Returns TRUE if players can't enter each others' territory.
****************************************************************************/
bool players_non_invade(const struct player *pplayer1,
			const struct player *pplayer2)
{
  if (pplayer1 == pplayer2 || !pplayer1 || !pplayer2) {
    return FALSE;
  }
  if (is_barbarian(pplayer1) || is_barbarian(pplayer2)) {
    /* Likely an unnecessary test. */
    return FALSE;
  }
  return pplayer_get_diplstate(pplayer1, pplayer2)->type == DS_PEACE;
}

/***************************************************************
  Returns true iff players have peace or cease-fire.
***************************************************************/
bool pplayers_non_attack(const struct player *pplayer,
                         const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;
  if (pplayer == pplayer2) {
    return FALSE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return FALSE;
  }
  return (ds == DS_PEACE || ds == DS_CEASEFIRE || ds == DS_ARMISTICE);
}

/**************************************************************************
  Return TRUE if players are in the same team
**************************************************************************/
bool players_on_same_team(const struct player *pplayer1,
                          const struct player *pplayer2)
{
  return (pplayer1->team && pplayer1->team == pplayer2->team);
}

bool is_barbarian(const struct player *pplayer)
{
  return pplayer->ai_data.barbarian_type != NOT_A_BARBARIAN;
}

/**************************************************************************
  Return TRUE iff the player me gives shared vision to player them.
**************************************************************************/
bool gives_shared_vision(const struct player *me, const struct player *them)
{
  return TEST_BIT(me->gives_shared_vision, player_index(them));
}

/**************************************************************************
  Return TRUE iff the two diplstates are equal.
**************************************************************************/
bool are_diplstates_equal(const struct player_diplstate *pds1,
			  const struct player_diplstate *pds2)
{
  return (pds1->type == pds2->type && pds1->turns_left == pds2->turns_left
	  && pds1->has_reason_to_cancel == pds2->has_reason_to_cancel
	  && pds1->contact_turns_left == pds2->contact_turns_left);
}

/***************************************************************************
  Return the number of pplayer2's visible units in pplayer's territory,
  from the point of view of pplayer.  Units that cannot be seen by pplayer
  will not be found (this function doesn't cheat).
***************************************************************************/
int player_in_territory(const struct player *pplayer,
			const struct player *pplayer2)
{
  int in_territory = 0;

  /* This algorithm should work at server or client.  It only returns the
   * number of visible units (a unit can potentially hide inside the
   * transport of a different player).
   *
   * Note this may be quite slow.  An even slower alternative is to iterate
   * over the entire map, checking all units inside the player's territory
   * to see if they're owned by the enemy. */
  unit_list_iterate(pplayer2->units, punit) {
    /* Get the owner of the tile/territory. */
    struct player *owner = tile_owner(punit->tile);

    if (owner == pplayer && can_player_see_unit(pplayer, punit)) {
      /* Found one! */
      in_territory += 1;
    }
  } unit_list_iterate_end;

  return in_territory;
}

/****************************************************************************
  Returns whether this is a valid username.  This is used by the server to
  validate usernames and should be used by the client to avoid invalid
  ones.
****************************************************************************/
bool is_valid_username(const char *name)
{
  return (strlen(name) > 0
	  && !my_isdigit(name[0])
	  && is_ascii_name(name)
	  && mystrcasecmp(name, ANON_USER_NAME) != 0);
}

/****************************************************************************
  Returns player_research struct of the given player. Note that team
  members share research
****************************************************************************/
struct player_research *get_player_research(const struct player *plr)
{
  if (!plr || !plr->team) {
    /* Some client users depend on this behavior. */
    return NULL;
  }
  return &(plr->team->research);
}

/****************************************************************************
  Marks player as winner.
****************************************************************************/
void player_set_winner(struct player *plr)
{
  plr->is_winner = TRUE;
}

/****************************************************************************
  Returns AI level associated with level name
****************************************************************************/
enum ai_level find_ai_level_by_name(const char *name)
{
  enum ai_level level;

  for (level = 0; level < AI_LEVEL_LAST; level++) {
    if (ai_level_names[level] != NULL) {
      /* Only consider levels that really have names */
      if (mystrcasecmp(ai_level_names[level], name) == 0) {
        return level;
      }
    }
  }

  /* No level matches name */
  return AI_LEVEL_LAST;
}

/***************************************************************
  Return localized name of the AI level
***************************************************************/
const char *ai_level_name(enum ai_level level)
{
  assert(level >= 0 && level < AI_LEVEL_LAST);

  if (ai_level_names[level] == NULL) {
    return NULL;
  }

  return _(ai_level_names[level]);
}

/***************************************************************
  Return cmd that sets given ai level
***************************************************************/
const char *ai_level_cmd(enum ai_level level)
{
  assert(level >= 0 && level < AI_LEVEL_LAST);

  if (ai_level_names[level] == NULL) {
    return NULL;
  }

  return ai_level_names[level];
}

/***************************************************************
  Return is AI can be set to given level
***************************************************************/
bool is_settable_ai_level(enum ai_level level)
{
  if (level == AI_LEVEL_AWAY) {
    /* Cannot set away level for AI */
    return FALSE;
  }

  /* It's usable if it has name */
  return ai_level_cmd(level) != NULL;
}

/***************************************************************
  Return number of AI levels in game
***************************************************************/
int number_of_ai_levels(void)
{
  /* We determine this runtime instead of hardcoding correct answer.
   * But as this is constant, we determine it only once. */
  static int count = 0;
  enum ai_level level;

  if (count) {
    /* Answer already known */
    return count;
  }

  /* Determine how many levels are actually usable */
  for (level = 0; level < AI_LEVEL_LAST; level++) {
    if (is_settable_ai_level(level)) {
      count++;
    }
  }

  return count;
}

/***************************************************************
  Returns the total number of player slots, i.e. the maximum
  number of players (including barbarians, etc.) that could ever
  exist at once.
***************************************************************/
int player_slot_count(void)
{
  return ARRAY_SIZE(game.players);
}

/***************************************************************
  Returns TRUE is this slot is "used" i.e. corresponds to a
  valid, initialized player that exists in the game.
***************************************************************/
bool player_slot_is_used(const struct player *pslot)
{
  if (!pslot) {
    return FALSE;
  }
  return pslot->used;
}

/***************************************************************
  Set the 'used' status of the player slot.  
***************************************************************/
void player_slot_set_used(struct player *pslot, bool used)
{
  if (!pslot) {
    return;
  }
  pslot->used = used;
}

/***************************************************************
  Return the possibly unused and uninitialized player slot.
***************************************************************/
struct player *player_slot_by_number(int player_id)
{
  if (!(0 <= player_id && player_id < player_slot_count())) {
    return NULL;
  }
  return &game.players[player_id];
}
