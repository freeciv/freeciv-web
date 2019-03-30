/***********************************************************************
 Freeciv - Copyright (C) 1996-2006 - Freeciv Development Team
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

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"

/* common */
#include "actions.h"
#include "game.h"
#include "movement.h"
#include "research.h"
#include "traderoutes.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "control.h"

/* client/gui-sdl2 */
#include "citydlg.h"
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_id.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "repodlgs.h"
#include "themespec.h"
#include "widget.h"

#include "dialogs_g.h"

typedef int (*act_func)(struct widget *);

struct diplomat_dialog {
  int actor_unit_id;
  int target_ids[ATK_COUNT];
  int target_extra_id;
  action_id act_id;
  struct ADVANCED_DLG *pdialog;
};

struct small_diplomat_dialog {
  int actor_unit_id;
  int target_id;
  action_id act_id;
  struct SMALL_DLG *pdialog;
};
 
extern bool is_unit_move_blocked;

void popdown_diplomat_dialog(void);
void popdown_incite_dialog(void);
void popdown_bribe_dialog(void);


static struct diplomat_dialog *pDiplomat_Dlg = NULL;
static bool is_more_user_input_needed = FALSE;
static bool did_not_decide = FALSE;

/**********************************************************************//**
  The action selection is done.
**************************************************************************/
static void act_sel_done_primary(int actor_unit_id)
{
  if (!is_more_user_input_needed) {
    /* The client isn't waiting for information for any unanswered follow
     * up questions. */

    struct unit *actor_unit;

    if ((actor_unit = game_unit_by_number(actor_unit_id))) {
      /* The action selection dialog wasn't closed because the actor unit
       * was lost. */

      /* The probabilities didn't just disappear, right? */
      fc_assert_action(actor_unit->client.act_prob_cache,
                       client_unit_init_act_prob_cache(actor_unit));

      FC_FREE(actor_unit->client.act_prob_cache);
    }

    /* The action selection process is over, at least for now. */
    action_selection_no_longer_in_progress(actor_unit_id);

    if (did_not_decide) {
      /* The action selection dialog was closed but the player didn't
       * decide what the unit should do. */

      /* Reset so the next action selection dialog does the right thing. */
      did_not_decide = FALSE;
    } else {
      /* An action, or no action at all, was selected. */
      action_decision_clear_want(actor_unit_id);
      action_selection_next_in_focus(actor_unit_id);
    }
  }
}

/**********************************************************************//**
  A follow up question about the selected action is done.
**************************************************************************/
static void act_sel_done_secondary(int actor_unit_id)
{
  /* Stop blocking. */
  is_more_user_input_needed = FALSE;
  act_sel_done_primary(actor_unit_id);
}

/**********************************************************************//**
  Let the non shared client code know that the action selection process
  no longer is in progress for the specified unit.

  This allows the client to clean up any client specific assumptions.
**************************************************************************/
void action_selection_no_longer_in_progress_gui_specific(int actor_id)
{
  /* Stop assuming the answer to a follow up question will arrive. */
  is_more_user_input_needed = FALSE;
}

/**********************************************************************//**
  Get the non targeted version of an action so it, if enabled, can appear
  in the target selection dialog.
**************************************************************************/
static action_id get_non_targeted_action_id(action_id tgt_action_id)
{
  /* Don't add an action mapping here unless the non targeted version is
   * selectable in the targeted version's target selection dialog. */
  switch ((enum gen_action)tgt_action_id) {
  case ACTION_SPY_TARGETED_SABOTAGE_CITY:
    return ACTION_SPY_SABOTAGE_CITY;
  case ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC:
    return ACTION_SPY_SABOTAGE_CITY_ESC;
  case ACTION_SPY_TARGETED_STEAL_TECH:
    return ACTION_SPY_STEAL_TECH;
  case ACTION_SPY_TARGETED_STEAL_TECH_ESC:
    return ACTION_SPY_STEAL_TECH_ESC;
  default:
    /* No non targeted version found. */
    return ACTION_NONE;
  }
}

/**********************************************************************//**
  Get the targeted version of an action so it, if enabled, can hide the
  non targeted action in the action selection dialog.
**************************************************************************/
static action_id get_targeted_action_id(action_id non_tgt_action_id)
{
  /* Don't add an action mapping here unless the non targeted version is
   * selectable in the targeted version's target selection dialog. */
  switch ((enum gen_action)non_tgt_action_id) {
  case ACTION_SPY_SABOTAGE_CITY:
    return ACTION_SPY_TARGETED_SABOTAGE_CITY;
  case ACTION_SPY_SABOTAGE_CITY_ESC:
    return ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC;
  case ACTION_SPY_STEAL_TECH:
    return ACTION_SPY_TARGETED_STEAL_TECH;
  case ACTION_SPY_STEAL_TECH_ESC:
    return ACTION_SPY_TARGETED_STEAL_TECH_ESC;
  default:
    /* No targeted version found. */
    return ACTION_NONE;
  }
}

/* ====================================================================== */
/* ============================ CARAVAN DIALOG ========================== */
/* ====================================================================== */

/**********************************************************************//**
  User selected that caravan should enter marketplace.
**************************************************************************/
static int caravan_marketplace_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_MARKETPLACE,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User selected that caravan should establish traderoute.
**************************************************************************/
static int caravan_establish_trade_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_TRADE_ROUTE,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User selected that caravan should help in wonder building.
**************************************************************************/
static int caravan_help_build_wonder_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_HELP_WONDER,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User selected to Recycle Unit to help city production.
**************************************************************************/
static int unit_recycle_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_RECYCLE_UNIT,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/* ====================================================================== */
/* ============================ DIPLOMAT DIALOG ========================= */
/* ====================================================================== */

/**********************************************************************//**
  User interacted with diplomat dialog.
**************************************************************************/
static int diplomat_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Establish Embassy"
**************************************************************************/
static int spy_embassy_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_ESTABLISH_EMBASSY,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Establish Embassy"
**************************************************************************/
static int diplomat_embassy_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_ESTABLISH_EMBASSY_STAY,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Investigate City"
**************************************************************************/
static int spy_investigate_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_INVESTIGATE_CITY,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    /* FIXME: Wait for the city display in stead? */
    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Investigate City (spends the unit)"
**************************************************************************/
static int diplomat_investigate_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_INV_CITY_SPEND,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    /* FIXME: Wait for the city display in stead? */
    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Poison City"
**************************************************************************/
static int spy_poison_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_POISON, pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Poison City Escape"
**************************************************************************/
static int spy_poison_esc_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_POISON_ESC, pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Suitcase Nuke"
**************************************************************************/
static int spy_nuke_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_NUKE, pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Suitcase Nuke Escape"
**************************************************************************/
static int spy_nuke_city_esc_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_NUKE_ESC, pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Destroy City"
**************************************************************************/
static int destroy_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_DESTROY_CITY, pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Steal Gold"
**************************************************************************/
static int spy_steal_gold_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_STEAL_GOLD,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Steal Gold Escape"
**************************************************************************/
static int spy_steal_gold_esc_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_STEAL_GOLD_ESC,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Steal Maps"
**************************************************************************/
static int spy_steal_maps_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_STEAL_MAPS,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Steal Maps and Escape"
**************************************************************************/
static int spy_steal_maps_esc_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_STEAL_MAPS_ESC,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Requests up-to-date list of improvements, the return of
  which will trigger the popup_sabotage_dialog() function.
**************************************************************************/
static int spy_sabotage_request(struct widget *pWidget)
{
  if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
      && NULL != game_city_by_number(
              pDiplomat_Dlg->target_ids[ATK_CITY])) {
    request_action_details(ACTION_SPY_TARGETED_SABOTAGE_CITY,
                           pDiplomat_Dlg->actor_unit_id,
                           pDiplomat_Dlg->target_ids[ATK_CITY]);
    is_more_user_input_needed = TRUE;
    popdown_diplomat_dialog();
  } else {
    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Requests up-to-date list of improvements, the return of
  which will trigger the popup_sabotage_dialog() function.
  (Escape version)
**************************************************************************/
static int spy_sabotage_esc_request(struct widget *pWidget)
{
  if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
      && NULL != game_city_by_number(
              pDiplomat_Dlg->target_ids[ATK_CITY])) {
    request_action_details(ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC,
                           pDiplomat_Dlg->actor_unit_id,
                           pDiplomat_Dlg->target_ids[ATK_CITY]);
    is_more_user_input_needed = TRUE;
    popdown_diplomat_dialog();
  } else {
    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Sabotage City" for diplomat (not spy)
**************************************************************************/
static int diplomat_sabotage_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
                pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_SABOTAGE_CITY,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        B_LAST + 1, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Sabotage City Escape" for diplomat (not spy)
**************************************************************************/
static int diplomat_sabotage_esc_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
                pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_SABOTAGE_CITY_ESC,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        B_LAST + 1, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/* --------------------------------------------------------- */

/**********************************************************************//**
  User interacted with spy's steal dialog window.
**************************************************************************/
static int spy_steal_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  Exit spy's steal or sabotage dialog.
**************************************************************************/
static int exit_spy_tgt_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = pDiplomat_Dlg->actor_unit_id;

    fc_assert(is_more_user_input_needed);
    popdown_diplomat_dialog();
    act_sel_done_secondary(actor_id);
  }

  return -1;
}

/**********************************************************************//**
  User selected which tech spy steals.
**************************************************************************/
static int spy_steal_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int steal_advance = MAX_ID - pWidget->ID;
    int actor_id = pDiplomat_Dlg->actor_unit_id;

    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
                pDiplomat_Dlg->target_ids[ATK_CITY])) {
      if (steal_advance == A_UNSET) {
        /* This is the untargeted version. */
        request_do_action(get_non_targeted_action_id(
                            pDiplomat_Dlg->act_id),
                          pDiplomat_Dlg->actor_unit_id,
                          pDiplomat_Dlg->target_ids[ATK_CITY],
                          steal_advance, "");
      } else {
        /* This is the targeted version. */
        request_do_action(pDiplomat_Dlg->act_id,
                          pDiplomat_Dlg->actor_unit_id,
                          pDiplomat_Dlg->target_ids[ATK_CITY],
                          steal_advance, "");
      }
    }

    fc_assert(is_more_user_input_needed);
    popdown_diplomat_dialog();
    act_sel_done_secondary(actor_id);
  }

  return -1;
}

/**********************************************************************//**
  Popup spy tech stealing dialog.
**************************************************************************/
static int spy_steal_popup_shared(struct widget *pWidget)
{
  const struct research *presearch, *vresearch;
  struct city *pVcity = pWidget->data.city;
  int id = MAX_ID - pWidget->ID;
  struct player *pVictim = NULL;
  struct CONTAINER *pCont;
  struct widget *pBuf = NULL;
  struct widget *pWindow;
  utf8_str *pstr;
  SDL_Surface *pSurf;
  int max_col, max_row, col, count = 0;
  int tech, idx;
  SDL_Rect area;

  struct unit *actor_unit = game_unit_by_number(id);
  action_id act_id = pDiplomat_Dlg->act_id;

  is_more_user_input_needed = TRUE;
  popdown_diplomat_dialog();

  if (pVcity) {
    pVictim = city_owner(pVcity);
  }

  fc_assert_ret_val_msg(!pDiplomat_Dlg, 1, "Diplomat dialog already open");

  if (!pVictim) {
    act_sel_done_secondary(id);
    return 1;
  }

  count = 0;
  presearch = research_get(client_player());
  vresearch = research_get(pVictim);
  advance_index_iterate(A_FIRST, i) {
    if (research_invention_gettable(presearch, i,
                                    game.info.tech_steal_allow_holes)
        && TECH_KNOWN == research_invention_state(vresearch, i)
        && TECH_KNOWN != research_invention_state(presearch, i)) {
      count++;
    }
  } advance_index_iterate_end;

  if (!count) {
    /* if there is no known tech to steal then
     * send steal order at Spy's Discretion */
    int target_id = pVcity->id;

    request_do_action(get_non_targeted_action_id(act_id),
                      id, target_id, A_UNSET, "");

    act_sel_done_secondary(id);

    return -1;
  }

  pCont = fc_calloc(1, sizeof(struct CONTAINER));
  pCont->id0 = pVcity->id;
  pCont->id1 = id;/* spy id */

  pDiplomat_Dlg = fc_calloc(1, sizeof(struct diplomat_dialog));
  pDiplomat_Dlg->act_id = act_id;
  pDiplomat_Dlg->actor_unit_id = id;
  pDiplomat_Dlg->target_ids[ATK_CITY] = pVcity->id;
  pDiplomat_Dlg->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  pstr = create_utf8_from_char(_("Select Advance to Steal"), adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = spy_steal_dlg_window_callback;
  set_wstate(pWindow , FC_WS_NORMAL);

  add_to_gui_list(ID_DIPLOMAT_DLG_WINDOW, pWindow);
  pDiplomat_Dlg->pdialog->pEndWidgetList = pWindow;

  area = pWindow->area;
  area.w = MAX(area.w, adj_size(8));

  /* ------------------ */
  /* exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = exit_spy_tgt_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_TERRAIN_ADV_DLG_EXIT_BUTTON, pBuf);
  /* ------------------------- */

  if (action_prob_possible(actor_unit->client.act_prob_cache[
                           get_non_targeted_action_id(act_id)])) {
     /* count + at Spy's Discretion */
    count++;
  }

  /* max col - 104 is steal tech widget width */
  max_col = (main_window_width() - (pWindow->size.w - pWindow->area.w) - adj_size(2)) / adj_size(104);
  /* max row - 204 is steal tech widget height */
  max_row = (main_window_height() - (pWindow->size.h - pWindow->area.h)) / adj_size(204);

  /* make space on screen for scrollbar */
  if (max_col * max_row < count) {
    max_col--;
  }

  if (count < max_col + 1) {
    col = count;
  } else {
    if (count < max_col + adj_size(3)) {
      col = max_col - adj_size(2);
    } else {
      if (count < max_col + adj_size(5)) {
        col = max_col - 1;
      } else {
        col = max_col;
      }
    }
  }

  pstr = create_utf8_str(NULL, 0, adj_font(10));
  pstr->style |= (TTF_STYLE_BOLD | SF_CENTER);

  count = 0;
  advance_index_iterate(A_FIRST, i) {
    if (research_invention_gettable(presearch, i,
                                    game.info.tech_steal_allow_holes)
        && TECH_KNOWN == research_invention_state(vresearch, i)
        && TECH_KNOWN != research_invention_state(presearch, i)) {
      count++;

      copy_chars_to_utf8_str(pstr, advance_name_translation(advance_by_number(i)));
      pSurf = create_select_tech_icon(pstr, i, FULL_MODE);
      pBuf = create_icon2(pSurf, pWindow->dst,
      		WF_FREE_THEME | WF_RESTORE_BACKGROUND);

      set_wstate(pBuf, FC_WS_NORMAL);
      pBuf->action = spy_steal_callback;
      pBuf->data.cont = pCont;

      add_to_gui_list(MAX_ID - i, pBuf);

      if (count > (col * max_row)) {
        set_wflag(pBuf, WF_HIDDEN);
      }
    }
  } advance_index_iterate_end;

  /* Get Spy tech to use for "At Spy's Discretion" -- this will have the
   * side effect of displaying the unit's icon */
  tech = advance_number(unit_type_get(actor_unit)->require_advance);

  if (action_prob_possible(actor_unit->client.act_prob_cache[
                           get_non_targeted_action_id(act_id)])) {
    struct astring str = ASTRING_INIT;

    /* TRANS: %s is a unit name, e.g., Spy */
    astr_set(&str, _("At %s's Discretion"),
             unit_name_translation(actor_unit));
    copy_chars_to_utf8_str(pstr, astr_str(&str));
    astr_free(&str);

    pSurf = create_select_tech_icon(pstr, tech, FULL_MODE);

    pBuf = create_icon2(pSurf, pWindow->dst,
                        (WF_FREE_THEME | WF_RESTORE_BACKGROUND
                         | WF_FREE_DATA));
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->action = spy_steal_callback;
    pBuf->data.cont = pCont;

    add_to_gui_list(MAX_ID - A_UNSET, pBuf);
    count++;
  }

  /* --------------------------------------------------------- */
  FREEUTF8STR(pstr);
  pDiplomat_Dlg->pdialog->pBeginWidgetList = pBuf;
  pDiplomat_Dlg->pdialog->pBeginActiveWidgetList = pDiplomat_Dlg->pdialog->pBeginWidgetList;
  pDiplomat_Dlg->pdialog->pEndActiveWidgetList = pDiplomat_Dlg->pdialog->pEndWidgetList->prev->prev;

  /* -------------------------------------------------------------- */

  idx = 0;
  if (count > col) {
    count = (count + (col - 1)) / col;
    if (count > max_row) {
      pDiplomat_Dlg->pdialog->pActiveWidgetList = pDiplomat_Dlg->pdialog->pEndActiveWidgetList;
      count = max_row;
      idx = create_vertical_scrollbar(pDiplomat_Dlg->pdialog, col, count, TRUE, TRUE);  
    }
  } else {
    count = 1;
  }

  area.w = MAX(area.w, (col * pBuf->size.w + adj_size(2) + idx));
  area.h = count * pBuf->size.h + adj_size(2);

  /* alloca window theme and win background buffer */
  pSurf = theme_get_background(theme, BACKGROUND_SPYSTEALDLG);
  if (resize_window(pWindow, pSurf, NULL,
                    (pWindow->size.w - pWindow->area.w) + area.w,
                    (pWindow->size.h - pWindow->area.h) + area.h)) {
    FREESURFACE(pSurf);
  }

  area = pWindow->area;

  widget_set_position(pWindow,
                      (main_window_width() - pWindow->size.w) / 2,
                      (main_window_height() - pWindow->size.h) / 2);

  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  setup_vertical_widgets_position(col, area.x + 1,
		  area.y, 0, 0,
		  pDiplomat_Dlg->pdialog->pBeginActiveWidgetList,
  		  pDiplomat_Dlg->pdialog->pEndActiveWidgetList);

  if (pDiplomat_Dlg->pdialog->pScroll) {
    setup_vertical_scrollbar_area(pDiplomat_Dlg->pdialog->pScroll,
	area.x + area.w, area.y,
    	area.h, TRUE);
  }

  redraw_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow, FALSE);
  widget_mark_dirty(pWindow);

  return -1;
}

/**********************************************************************//**
  Popup spy tech stealing dialog for "Targeted Steal Tech".
**************************************************************************/
static int spy_steal_popup(struct widget *pWidget)
{
  pDiplomat_Dlg->act_id = ACTION_SPY_TARGETED_STEAL_TECH;
  return spy_steal_popup_shared(pWidget);
}

/**********************************************************************//**
  Popup spy tech stealing dialog for "Targeted Steal Tech Escape Expected".
**************************************************************************/
static int spy_steal_esc_popup(struct widget *pWidget)
{
  pDiplomat_Dlg->act_id = ACTION_SPY_TARGETED_STEAL_TECH_ESC;
  return spy_steal_popup_shared(pWidget);
}

/**********************************************************************//**
  Technology stealing dialog, diplomat (not spy) version
**************************************************************************/
static int diplomat_steal_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
                pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_STEAL_TECH,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        A_UNSET, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Steal Tech Escape Expected"
**************************************************************************/
static int diplomat_steal_esc_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
                pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_do_action(ACTION_SPY_STEAL_TECH_ESC,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        A_UNSET, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Ask the server how much the revolt is going to cost us
**************************************************************************/
static int diplomat_incite_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
                pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_action_details(ACTION_SPY_INCITE_CITY,
                             pDiplomat_Dlg->actor_unit_id,
                             pDiplomat_Dlg->target_ids[ATK_CITY]);
      is_more_user_input_needed = TRUE;
      popdown_diplomat_dialog();
    } else {
      popdown_diplomat_dialog();
    }
  }

  return -1;
}

/**********************************************************************//**
  Ask the server how much the revolt is going to cost us
**************************************************************************/
static int spy_incite_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL != game_city_by_number(
                pDiplomat_Dlg->target_ids[ATK_CITY])) {
      request_action_details(ACTION_SPY_INCITE_CITY_ESC,
                             pDiplomat_Dlg->actor_unit_id,
                             pDiplomat_Dlg->target_ids[ATK_CITY]);
      is_more_user_input_needed = TRUE;
      popdown_diplomat_dialog();
    } else {
      popdown_diplomat_dialog();
    }
  }

  return -1;
}

/**********************************************************************//**
  Callback from action selection dialog for "keep moving".
  (This should only occur when entering a tile with an allied city
  or an allied unit.)
**************************************************************************/
static int act_sel_keep_moving_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *punit;

    if ((punit = game_unit_by_number(pDiplomat_Dlg->actor_unit_id))
        && !same_pos(unit_tile(punit), pWidget->data.tile)) {
      request_unit_non_action_move(punit, pWidget->data.tile);
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Delay selection of what action to take.
**************************************************************************/
static int act_sel_wait_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    key_unit_wait();

    /* The dialog was popped down when key_unit_wait() resulted in
     * action_selection_close() being called. */
  }

  return -1;
}

/**********************************************************************//**
  Ask the server how much the bribe costs
**************************************************************************/
static int diplomat_bribe_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)
        && NULL !=
         game_unit_by_number(pDiplomat_Dlg->target_ids[ATK_UNIT])) {
      request_action_details(ACTION_SPY_BRIBE_UNIT,
                             pDiplomat_Dlg->actor_unit_id,
                             pDiplomat_Dlg->target_ids[ATK_UNIT]);
      is_more_user_input_needed = TRUE;
      popdown_diplomat_dialog();
    } else {
      popdown_diplomat_dialog();
    }
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Sabotage Unit"
**************************************************************************/
static int spy_sabotage_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int diplomat_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_SPY_SABOTAGE_UNIT,
                      diplomat_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Sabotage Unit Escape"
**************************************************************************/
static int spy_sabotage_unit_esc_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int diplomat_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_SPY_SABOTAGE_UNIT_ESC,
                      diplomat_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Heal Unit"
**************************************************************************/
static int heal_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_HEAL_UNIT,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Capture Units"
**************************************************************************/
static int capture_units_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = tile_index(pWidget->data.tile);

    popdown_diplomat_dialog();
    request_do_action(ACTION_CAPTURE_UNITS,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Expel Unit"
**************************************************************************/
static int expel_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_EXPEL_UNIT,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Bombard"
**************************************************************************/
static int bombard_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = tile_index(pWidget->data.tile);

    popdown_diplomat_dialog();
    request_do_action(ACTION_BOMBARD,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Join City"
**************************************************************************/
static int join_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_JOIN_CITY,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Found City"
**************************************************************************/
static int found_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;

    popdown_diplomat_dialog();
    dsend_packet_city_name_suggestion_req(&client.conn,
                                          actor_id);
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Transform Terrain"
**************************************************************************/
static int transform_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;

    popdown_diplomat_dialog();
    request_do_action(ACTION_TRANSFORM_TERRAIN,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Irrigate TF"
**************************************************************************/
static int irrig_tf_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;

    popdown_diplomat_dialog();
    request_do_action(ACTION_IRRIGATE_TF,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Mine TF"
**************************************************************************/
static int mine_tf_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;

    popdown_diplomat_dialog();
    request_do_action(ACTION_MINE_TF,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Pillage"
**************************************************************************/
static int pillage_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;
    int sub_target_id = pDiplomat_Dlg->target_extra_id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_PILLAGE, actor_id,
                      target_id, sub_target_id, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Road"
**************************************************************************/
static int road_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;
    int sub_target_id = pDiplomat_Dlg->target_extra_id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_ROAD, actor_id,
                      target_id, sub_target_id, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Build Base"
**************************************************************************/
static int base_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;
    int sub_target_id = pDiplomat_Dlg->target_extra_id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_BASE, actor_id,
                      target_id, sub_target_id, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Build Mine"
**************************************************************************/
static int mine_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;
    int sub_target_id = pDiplomat_Dlg->target_extra_id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_MINE, actor_id,
                      target_id, sub_target_id, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Build Irrigation"
**************************************************************************/
static int irrigate_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;
    int sub_target_id = pDiplomat_Dlg->target_extra_id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_IRRIGATE, actor_id,
                      target_id, sub_target_id, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Explode Nuclear"
**************************************************************************/
static int nuke_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;

    popdown_diplomat_dialog();
    request_do_action(ACTION_NUKE,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Attack"
**************************************************************************/
static int attack_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;

    popdown_diplomat_dialog();
    request_do_action(ACTION_ATTACK,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Suicide Attack"
**************************************************************************/
static int suicide_attack_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;

    popdown_diplomat_dialog();
    request_do_action(ACTION_SUICIDE_ATTACK,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Paradrop Unit"
**************************************************************************/
static int paradrop_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.tile->index;

    popdown_diplomat_dialog();
    request_do_action(ACTION_PARADROP,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Disband Unit"
**************************************************************************/
static int disband_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_DISBAND_UNIT,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Fortify"
**************************************************************************/
static int fortify_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_FORTIFY,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Convert Unit"
**************************************************************************/
static int convert_unit_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int actor_id = MAX_ID - pWidget->ID;
    int target_id = pWidget->data.unit->id;

    popdown_diplomat_dialog();
    request_do_action(ACTION_CONVERT,
                      actor_id, target_id, 0, "");
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Set Home City"
**************************************************************************/
static int home_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_HOME_CITY,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Upgrade Unit"
**************************************************************************/
static int upgrade_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    struct unit *punit;
    struct city *pcity;

    if ((pcity = game_city_by_number(pDiplomat_Dlg->target_ids[ATK_CITY]))
        && (punit = game_unit_by_number(pDiplomat_Dlg->actor_unit_id))) {
      popup_unit_upgrade_dlg(punit, FALSE);
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Airlift Unit"
**************************************************************************/
static int airlift_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_AIRLIFT,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  User clicked "Conquer City"
**************************************************************************/
static int conquer_city_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_city_by_number(
          pDiplomat_Dlg->target_ids[ATK_CITY])
        && NULL != game_unit_by_number(pDiplomat_Dlg->actor_unit_id)) {
      request_do_action(ACTION_CONQUER_CITY,
                        pDiplomat_Dlg->actor_unit_id,
                        pDiplomat_Dlg->target_ids[ATK_CITY],
                        0, "");
    }

    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Close diplomat dialog.
**************************************************************************/
static int diplomat_close_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_diplomat_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Popdown a dialog giving a diplomatic unit some options when moving into
  the target tile.
**************************************************************************/
void popdown_diplomat_dialog(void)
{
  if (pDiplomat_Dlg) {
    act_sel_done_primary(pDiplomat_Dlg->actor_unit_id);

    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pDiplomat_Dlg->pdialog->pBeginWidgetList,
				pDiplomat_Dlg->pdialog->pEndWidgetList);
    FC_FREE(pDiplomat_Dlg->pdialog->pScroll);
    FC_FREE(pDiplomat_Dlg->pdialog);
    FC_FREE(pDiplomat_Dlg);
    queue_flush();
  }
}

/* Mapping from an action to the function to call when its button is
 * pushed. */
static const act_func af_map[ACTION_COUNT] = {
  /* Unit acting against a city target. */
  [ACTION_ESTABLISH_EMBASSY] = spy_embassy_callback,
  [ACTION_ESTABLISH_EMBASSY_STAY] = diplomat_embassy_callback,
  [ACTION_SPY_INVESTIGATE_CITY] = spy_investigate_callback,
  [ACTION_INV_CITY_SPEND] = diplomat_investigate_callback,
  [ACTION_SPY_POISON] = spy_poison_callback,
  [ACTION_SPY_POISON_ESC] = spy_poison_esc_callback,
  [ACTION_SPY_STEAL_GOLD] = spy_steal_gold_callback,
  [ACTION_SPY_STEAL_GOLD_ESC] = spy_steal_gold_esc_callback,
  [ACTION_STEAL_MAPS] = spy_steal_maps_callback,
  [ACTION_STEAL_MAPS_ESC] = spy_steal_maps_esc_callback,
  [ACTION_SPY_SABOTAGE_CITY] = diplomat_sabotage_callback,
  [ACTION_SPY_SABOTAGE_CITY_ESC] = diplomat_sabotage_esc_callback,
  [ACTION_SPY_TARGETED_SABOTAGE_CITY] = spy_sabotage_request,
  [ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC] = spy_sabotage_esc_request,
  [ACTION_SPY_STEAL_TECH] = diplomat_steal_callback,
  [ACTION_SPY_STEAL_TECH_ESC] = diplomat_steal_esc_callback,
  [ACTION_SPY_TARGETED_STEAL_TECH] = spy_steal_popup,
  [ACTION_SPY_TARGETED_STEAL_TECH_ESC] = spy_steal_esc_popup,
  [ACTION_SPY_INCITE_CITY] = diplomat_incite_callback,
  [ACTION_SPY_INCITE_CITY_ESC] = spy_incite_callback,
  [ACTION_TRADE_ROUTE] = caravan_establish_trade_callback,
  [ACTION_MARKETPLACE] = caravan_marketplace_callback,
  [ACTION_HELP_WONDER] = caravan_help_build_wonder_callback,
  [ACTION_JOIN_CITY] = join_city_callback,
  [ACTION_SPY_NUKE] = spy_nuke_city_callback,
  [ACTION_SPY_NUKE_ESC] = spy_nuke_city_esc_callback,
  [ACTION_DESTROY_CITY] = destroy_city_callback,
  [ACTION_RECYCLE_UNIT] = unit_recycle_callback,
  [ACTION_HOME_CITY] = home_city_callback,
  [ACTION_UPGRADE_UNIT] = upgrade_callback,
  [ACTION_AIRLIFT] = airlift_callback,
  [ACTION_CONQUER_CITY] = conquer_city_callback,

  /* Unit acting against a unit target. */
  [ACTION_SPY_BRIBE_UNIT] = diplomat_bribe_callback,
  [ACTION_SPY_SABOTAGE_UNIT] = spy_sabotage_unit_callback,
  [ACTION_SPY_SABOTAGE_UNIT_ESC] = spy_sabotage_unit_esc_callback,
  [ACTION_HEAL_UNIT] = heal_unit_callback,
  [ACTION_EXPEL_UNIT] = expel_unit_callback,

  /* Unit acting against all units at a tile. */
  [ACTION_CAPTURE_UNITS] = capture_units_callback,
  [ACTION_BOMBARD] = bombard_callback,

  /* Unit acting against a tile. */
  [ACTION_FOUND_CITY] = found_city_callback,
  [ACTION_NUKE] = nuke_callback,
  [ACTION_PARADROP] = paradrop_callback,
  [ACTION_ATTACK] = attack_callback,
  [ACTION_SUICIDE_ATTACK] = suicide_attack_callback,
  [ACTION_TRANSFORM_TERRAIN] = transform_callback,
  [ACTION_IRRIGATE_TF] = irrig_tf_callback,
  [ACTION_MINE_TF] = mine_tf_callback,
  [ACTION_PILLAGE] = pillage_callback,
  [ACTION_ROAD] = road_callback,
  [ACTION_BASE] = base_callback,
  [ACTION_MINE] = mine_callback,
  [ACTION_IRRIGATE] = irrigate_callback,

  /* Unit acting with no target except itself. */
  [ACTION_DISBAND_UNIT] = disband_unit_callback,
  [ACTION_FORTIFY] = fortify_callback,
  [ACTION_CONVERT] = convert_unit_callback,
};

/**********************************************************************//**
  Add an entry for an action in the action choice dialog.
**************************************************************************/
static void action_entry(const action_id act,
                         const struct act_prob *act_probs,
                         const char *custom,
                         struct unit *act_unit,
                         struct tile *tgt_tile,
                         struct city *tgt_city,
                         struct unit *tgt_unit,
                         struct widget *pWindow,
                         SDL_Rect *area)
{
  struct widget *pBuf;
  utf8_str *pstr;
  const char *ui_name;

  if (get_targeted_action_id(act) != ACTION_NONE
      && action_prob_possible(act_probs[get_targeted_action_id(act)])) {
    /* The player can select the untargeted version from the target
     * selection dialog. */
    return;
  }

  if (af_map[act] == NULL) {
    /* This client doesn't support ordering this action from the
     * action selection dialog. */
    return;
  }

  /* Don't show disabled actions */
  if (!action_prob_possible(act_probs[act])) {
    return;
  }

  ui_name = action_prepare_ui_name(act, "",
                                   act_probs[act], custom);

  create_active_iconlabel(pBuf, pWindow->dst, pstr,
                          ui_name, af_map[act]);

  switch(action_id_get_target_kind(act)) {
  case ATK_CITY:
    pBuf->data.city = tgt_city;
    break;
  case ATK_UNIT:
    pBuf->data.unit = tgt_unit;
    break;
  case ATK_TILE:
  case ATK_UNITS:
    pBuf->data.tile = tgt_tile;
    break;
  case ATK_SELF:
    pBuf->data.unit = act_unit;
    break;
  case ATK_COUNT:
    fc_assert_msg(FALSE, "Unsupported target kind");
  }

  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(MAX_ID - act_unit->id, pBuf);

  area->w = MAX(area->w, pBuf->size.w);
  area->h += pBuf->size.h;
}

/**********************************************************************//**
  Return custom text for the specified action (given that the aciton is
  possible).
**************************************************************************/
static const char *action_custom_text(const action_id act_id,
                                      const struct act_prob prob,
                                      const struct unit *actor_unit,
                                      const struct city *actor_homecity,
                                      const struct city *target_city)
{
  static struct astring custom = ASTRING_INIT;

  if (!action_prob_possible(prob)) {
    /* No info since impossible. */
    return NULL;
  }

  switch ((enum gen_action)act_id) {
  case ACTION_TRADE_ROUTE:
    {
      int revenue = get_caravan_enter_city_trade_bonus(actor_homecity,
                                                       target_city,
                                                       actor_unit->carrying,
                                                       TRUE);

      astr_set(&custom,
               /* TRANS: Estimated one time bonus and recurring revenue for
                * the Establish Trade _Route action. */
               _("%d one time bonus + %d trade"),
               revenue,
               trade_base_between_cities(actor_homecity, target_city));
      break;
    }
  case ACTION_MARKETPLACE:
    {
      int revenue = get_caravan_enter_city_trade_bonus(actor_homecity,
                                                       target_city,
                                                       actor_unit->carrying,
                                                       FALSE);

      astr_set(&custom,
               /* TRANS: Estimated one time bonus for the Enter Marketplace
                * action. */
               _("%d one time bonus"), revenue);
      break;
    }
  default:
    /* No info to add. */
    return NULL;
  }

  return astr_str(&custom);
}

/**********************************************************************//**
  Popup a dialog that allows the player to select what action a unit
  should take.
**************************************************************************/
void popup_action_selection(struct unit *actor_unit,
                            struct city *target_city,
                            struct unit *target_unit,
                            struct tile *target_tile,
                            struct extra_type *target_extra,
                            const struct act_prob *act_probs)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  utf8_str *pstr;
  SDL_Rect area;
  struct city *actor_homecity;

  fc_assert_ret_msg(!pDiplomat_Dlg, "Diplomat dialog already open");

  /* Could be caused by the server failing to reply to a request for more
   * information or a bug in the client code. */
  fc_assert_msg(!is_more_user_input_needed,
                "Diplomat queue problem. Is another diplomat window open?");

  /* No extra input is required as no action has been chosen yet. */
  is_more_user_input_needed = FALSE;

  is_unit_move_blocked = TRUE;

  actor_homecity = game_city_by_number(actor_unit->homecity);

  pDiplomat_Dlg = fc_calloc(1, sizeof(struct diplomat_dialog));
  pDiplomat_Dlg->actor_unit_id = actor_unit->id;
  pDiplomat_Dlg->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  /* window */
  if (target_city) {
    struct astring title = ASTRING_INIT;

    /* TRANS: %s is a unit name, e.g., Spy */
    astr_set(&title, _("Choose Your %s's Strategy"),
             unit_name_translation(actor_unit));
    pstr = create_utf8_from_char(astr_str(&title), adj_font(12));
    astr_free(&title);
  } else {
    pstr = create_utf8_from_char(_("Subvert Enemy Unit"), adj_font(12));
  }

  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);
  
  pWindow->action = diplomat_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_CARAVAN_DLG_WINDOW, pWindow);
  pDiplomat_Dlg->pdialog->pEndWidgetList = pWindow;

  area = pWindow->area;
  area.w = MAX(area.w, adj_size(8));
  area.h = MAX(area.h, adj_size(2));

  if (target_city) {
    pDiplomat_Dlg->target_ids[ATK_CITY] = target_city->id;
  } else {
    pDiplomat_Dlg->target_ids[ATK_CITY] = IDENTITY_NUMBER_ZERO;
  }

  if (target_unit) {
    pDiplomat_Dlg->target_ids[ATK_UNIT] = target_unit->id;
  } else {
    pDiplomat_Dlg->target_ids[ATK_UNIT] = IDENTITY_NUMBER_ZERO;
  }

  pDiplomat_Dlg->target_ids[ATK_TILE] = tile_index(target_tile);

  if (target_extra) {
    pDiplomat_Dlg->target_extra_id = extra_number(target_extra);
  } else {
    pDiplomat_Dlg->target_extra_id = EXTRA_NONE;
  }

  pDiplomat_Dlg->target_ids[ATK_SELF] = actor_unit->id;

  /* ---------- */
  /* Unit acting against a city */

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_CITY) {
      action_entry(act, act_probs,
                   action_custom_text(act, act_probs[act], actor_unit,
                                      actor_homecity, target_city),
                   actor_unit, NULL, target_city, NULL,
                   pWindow, &area);
    }
  } action_iterate_end;

  /* Unit acting against another unit */

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_UNIT) {
      action_entry(act, act_probs,
                   NULL,
                   actor_unit, NULL, NULL, target_unit,
                   pWindow, &area);
    }
  } action_iterate_end;

  /* Unit acting against all units at a tile */

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_UNITS) {
      action_entry(act, act_probs,
                   NULL,
                   actor_unit, target_tile, NULL, NULL,
                   pWindow, &area);
    }
  } action_iterate_end;

  /* Unit acting against a tile. */

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_TILE) {
      action_entry(act, act_probs,
                   NULL,
                   actor_unit, target_tile, NULL, NULL,
                   pWindow, &area);
    }
  } action_iterate_end;

  /* Unit acting against itself. */

  action_iterate(act) {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_SELF) {
      action_entry(act, act_probs,
                   NULL,
                   actor_unit, NULL, NULL, target_unit,
                   pWindow, &area);
    }
  } action_iterate_end;

  /* ---------- */
  if (unit_can_move_to_tile(&(wld.map), actor_unit, target_tile,
                            FALSE, FALSE)) {
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Keep moving"),
                            act_sel_keep_moving_callback);

    pBuf->data.tile = target_tile;

    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(MAX_ID - actor_unit->id, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }

  /* ---------- */
  create_active_iconlabel(pBuf, pWindow->dst, pstr,
                          _("Wait"), act_sel_wait_callback);

  pBuf->data.tile = target_tile;

  set_wstate(pBuf, FC_WS_NORMAL);

  add_to_gui_list(MAX_ID - actor_unit->id, pBuf);

  area.w = MAX(area.w, pBuf->size.w);
  area.h += pBuf->size.h;

  /* ---------- */
  create_active_iconlabel(pBuf, pWindow->dst, pstr,
                          _("Cancel"), diplomat_close_callback);

  set_wstate(pBuf , FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_LABEL , pBuf);

  area.w = MAX(area.w, pBuf->size.w);
  area.h += pBuf->size.h;
  /* ---------- */
  pDiplomat_Dlg->pdialog->pBeginWidgetList = pBuf;

  /* setup window size and start position */

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           unit_tile(actor_unit));

  /* setup widget size and start position */

  pBuf = pWindow->prev;
  setup_vertical_widgets_position(1,
	area.x,
  	area.y + 1, area.w, 0,
	pDiplomat_Dlg->pdialog->pBeginWidgetList, pBuf);

  /* --------------------- */
  /* redraw */
  redraw_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);

  /* Give follow up questions access to action probabilities. */
  client_unit_init_act_prob_cache(actor_unit);
  action_iterate(act) {
    actor_unit->client.act_prob_cache[act] = act_probs[act];
  } action_iterate_end;
}

/**********************************************************************//**
  Returns the id of the actor unit currently handled in action selection
  dialog when the action selection dialog is open.
  Returns IDENTITY_NUMBER_ZERO if no action selection dialog is open.
**************************************************************************/
int action_selection_actor_unit(void)
{
  if (!pDiplomat_Dlg) {
    return IDENTITY_NUMBER_ZERO;
  }

  return pDiplomat_Dlg->actor_unit_id;
}

/**********************************************************************//**
  Returns id of the target city of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has a
  city target. Returns IDENTITY_NUMBER_ZERO if no action selection dialog
  is open or no city target is present in the action selection dialog.
**************************************************************************/
int action_selection_target_city(void)
{
  if (!pDiplomat_Dlg) {
    return IDENTITY_NUMBER_ZERO;
  }

  return pDiplomat_Dlg->target_ids[ATK_CITY];
}

/**********************************************************************//**
  Returns id of the target unit of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has a
  unit target. Returns IDENTITY_NUMBER_ZERO if no action selection dialog
  is open or no unit target is present in the action selection dialog.
**************************************************************************/
int action_selection_target_unit(void)
{
  if (!pDiplomat_Dlg) {
    return IDENTITY_NUMBER_ZERO;
  }

  return pDiplomat_Dlg->target_ids[ATK_UNIT];
}

/**********************************************************************//**
  Returns id of the target tile of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has a
  tile target. Returns TILE_INDEX_NONE if no action selection dialog is
  open.
**************************************************************************/
int action_selection_target_tile(void)
{
  if (!pDiplomat_Dlg) {
    return TILE_INDEX_NONE;
  }

  return pDiplomat_Dlg->target_ids[ATK_TILE];
}

/**********************************************************************//**
  Returns id of the target extra of the actions currently handled in action
  selection dialog when the action selection dialog is open and it has an
  extra target. Returns EXTRA_NONE if no action selection dialog is open
  or no extra target is present in the action selection dialog.
**************************************************************************/
int action_selection_target_extra(void)
{
  if (!pDiplomat_Dlg) {
    return EXTRA_NONE;
  }

  return pDiplomat_Dlg->target_extra_id;
}

/**********************************************************************//**
  Updates the action selection dialog with new information.
**************************************************************************/
void action_selection_refresh(struct unit *actor_unit,
                              struct city *target_city,
                              struct unit *target_unit,
                              struct tile *target_tile,
                              struct extra_type *target_extra,
                              const struct act_prob *act_probs)
{
  /* TODO: port me. */
}

/**********************************************************************//**
  Closes the action selection dialog
**************************************************************************/
void action_selection_close(void)
{
  did_not_decide = TRUE;
  popdown_diplomat_dialog();
}

/* ====================================================================== */
/* ============================ SABOTAGE DIALOG ========================= */
/* ====================================================================== */

/**********************************************************************//**
  User selected what to sabotage.
**************************************************************************/
static int sabotage_impr_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    int sabotage_improvement = MAX_ID - pWidget->ID;
    int diplomat_target_id = pWidget->data.cont->id0;
    int diplomat_id = pWidget->data.cont->id1;
    action_id act_id = pWidget->data.cont->value;

    fc_assert(is_more_user_input_needed);
    popdown_diplomat_dialog();

    if (sabotage_improvement == 1000) {
      sabotage_improvement = -1;
    }

    if (NULL != game_unit_by_number(diplomat_id)
        && NULL != game_city_by_number(diplomat_target_id)) {
      if (sabotage_improvement == B_LAST) {
        /* This is the untargeted version. */
        request_do_action(get_non_targeted_action_id(act_id),
                          diplomat_id, diplomat_target_id,
                          sabotage_improvement + 1, "");
      } else {
        /* This is the targeted version. */
        request_do_action(act_id,
                          diplomat_id, diplomat_target_id,
                          sabotage_improvement + 1, "");
      }
    }

    act_sel_done_secondary(diplomat_id);
  }

  return -1;
}

/**********************************************************************//**
  Pops-up the Spy sabotage dialog, upon return of list of
  available improvements requested by the above function.
**************************************************************************/
void popup_sabotage_dialog(struct unit *actor, struct city *pCity,
                           const struct action *paction)
{
  struct widget *pWindow = NULL, *pBuf = NULL , *pLast = NULL;
  struct CONTAINER *pCont;
  utf8_str *pstr;
  SDL_Rect area, area2;
  int n, w = 0, h, imp_h = 0, y;

  fc_assert_ret_msg(!pDiplomat_Dlg, "Diplomat dialog already open");

  /* Should be set before sending request to the server. */
  fc_assert(is_more_user_input_needed);

  if (!actor) {
    act_sel_done_secondary(IDENTITY_NUMBER_ZERO);
    return;
  }

  is_unit_move_blocked = TRUE;
  
  pDiplomat_Dlg = fc_calloc(1, sizeof(struct diplomat_dialog));
  pDiplomat_Dlg->actor_unit_id = actor->id;
  pDiplomat_Dlg->target_ids[ATK_CITY] = pCity->id;
  pDiplomat_Dlg->pdialog = fc_calloc(1, sizeof(struct ADVANCED_DLG));

  pCont = fc_calloc(1, sizeof(struct CONTAINER));
  pCont->id0 = pCity->id;
  pCont->id1 = actor->id; /* spy id */
  pCont->value = paction->id;

  pstr = create_utf8_from_char(_("Select Improvement to Sabotage") , adj_font(12));
  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = diplomat_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_TERRAIN_ADV_DLG_WINDOW, pWindow);
  pDiplomat_Dlg->pdialog->pEndWidgetList = pWindow;

  area = pWindow->area;
  area.h = MAX(area.h, adj_size(2));

  /* ---------- */
  /* exit button */
  pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                          WF_WIDGET_HAS_INFO_LABEL | WF_RESTORE_BACKGROUND);
  pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                           adj_font(12));
  area.w += pBuf->size.w + adj_size(10);
  pBuf->action = exit_spy_tgt_dlg_callback;
  set_wstate(pBuf, FC_WS_NORMAL);
  pBuf->key = SDLK_ESCAPE;

  add_to_gui_list(ID_TERRAIN_ADV_DLG_EXIT_BUTTON, pBuf);
  /* ---------- */

  create_active_iconlabel(pBuf, pWindow->dst, pstr,
                          _("City Production"), sabotage_impr_callback);
  pBuf->data.cont = pCont;
  set_wstate(pBuf, FC_WS_NORMAL);
  set_wflag(pBuf, WF_FREE_DATA);
  add_to_gui_list(MAX_ID - 1000, pBuf);

  area.w = MAX(area.w, pBuf->size.w);
  area.h += pBuf->size.h;

  /* separator */
  pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);

  add_to_gui_list(ID_SEPARATOR, pBuf);
  area.h += pBuf->next->size.h;

  pDiplomat_Dlg->pdialog->pEndActiveWidgetList = pBuf;

  /* ------------------ */
  n = 0;
  city_built_iterate(pCity, pImprove) {
    if (pImprove->sabotage > 0) {
      create_active_iconlabel(pBuf, pWindow->dst, pstr,
	      (char *) city_improvement_name_translation(pCity, pImprove),
				      sabotage_impr_callback);
      pBuf->data.cont = pCont;
      set_wstate(pBuf, FC_WS_NORMAL);

      add_to_gui_list(MAX_ID - improvement_number(pImprove), pBuf);

      area.w = MAX(area.w , pBuf->size.w);
      imp_h += pBuf->size.h;

      if (n > 9) {
        set_wflag(pBuf, WF_HIDDEN);
      }

      n++;
      /* ----------- */
    }
  } city_built_iterate_end;

  pDiplomat_Dlg->pdialog->pBeginActiveWidgetList = pBuf;

  if (n > 0
      && action_prob_possible(actor->client.act_prob_cache[
                              get_non_targeted_action_id(paction->id)])) {
    /* separator */
    pBuf = create_iconlabel(NULL, pWindow->dst, NULL, WF_FREE_THEME);

    add_to_gui_list(ID_SEPARATOR, pBuf);
    area.h += pBuf->next->size.h;
  /* ------------------ */
  }

  if (action_prob_possible(actor->client.act_prob_cache[
                           get_non_targeted_action_id(paction->id)])) {
    struct astring str = ASTRING_INIT;

    /* TRANS: %s is a unit name, e.g., Spy */
    astr_set(&str, _("At %s's Discretion"), unit_name_translation(actor));
    create_active_iconlabel(pBuf, pWindow->dst, pstr, astr_str(&str),
                            sabotage_impr_callback);
    astr_free(&str);

    pBuf->data.cont = pCont;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(MAX_ID - B_LAST, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }

  /* ----------- */

  pLast = pBuf;
  pDiplomat_Dlg->pdialog->pBeginWidgetList = pLast;
  pDiplomat_Dlg->pdialog->pActiveWidgetList = pDiplomat_Dlg->pdialog->pEndActiveWidgetList;

  /* ---------- */
  if (n > 10) {
    imp_h = 10 * pBuf->size.h;

    n = create_vertical_scrollbar(pDiplomat_Dlg->pdialog,
                                  1, 10, TRUE, TRUE);
    area.w += n;
  } else {
    n = 0;
  }
  /* ---------- */

  area.h += imp_h;

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           unit_tile(actor));

  w = area.w;

  /* exit button */
  pBuf = pWindow->prev;
  pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
  pBuf->size.y = pWindow->size.y + adj_size(2);

  /* Production sabotage */
  pBuf = pBuf->prev;

  pBuf->size.x = area.x;
  pBuf->size.y = y = area.y + 1;
  pBuf->size.w = w;
  h = pBuf->size.h;

  area2.x = adj_size(10);
  area2.h = adj_size(2);

  pBuf = pBuf->prev;
  while (pBuf) {
    if (pBuf == pDiplomat_Dlg->pdialog->pEndActiveWidgetList) {
      w -= n;
    }

    pBuf->size.w = w;
    pBuf->size.x = pBuf->next->size.x;
    pBuf->size.y = y = y + pBuf->next->size.h;

    if (pBuf->ID == ID_SEPARATOR) {
      FREESURFACE(pBuf->theme);
      pBuf->size.h = h;
      pBuf->theme = create_surf(w, h, SDL_SWSURFACE);

      area2.y = pBuf->size.h / 2 - 1;
      area2.w = pBuf->size.w - adj_size(20);

      SDL_FillRect(pBuf->theme , &area2, map_rgba(pBuf->theme->format, *get_theme_color(COLOR_THEME_SABOTAGEDLG_SEPARATOR)));
    }

    if (pBuf == pLast) {
      break;
    }

    if (pBuf == pDiplomat_Dlg->pdialog->pBeginActiveWidgetList) {
      /* Reset to end of scrolling area */
      y = MIN(y, pDiplomat_Dlg->pdialog->pEndActiveWidgetList->size.y
              + 9 * pBuf->size.h);
      w += n;
    }
    pBuf = pBuf->prev;
  }

  if (pDiplomat_Dlg->pdialog->pScroll) {
    setup_vertical_scrollbar_area(pDiplomat_Dlg->pdialog->pScroll,
        area.x + area.w,
        pDiplomat_Dlg->pdialog->pEndActiveWidgetList->size.y,
        pDiplomat_Dlg->pdialog->pBeginActiveWidgetList->prev->size.y
          - pDiplomat_Dlg->pdialog->pEndActiveWidgetList->size.y,
        TRUE);
  }

  /* -------------------- */
  /* redraw */
  redraw_group(pDiplomat_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

/* ====================================================================== */
/* ============================== INCITE DIALOG ========================= */
/* ====================================================================== */
static struct small_diplomat_dialog *pIncite_Dlg = NULL;

/**********************************************************************//**
  User interacted with Incite Revolt dialog window.
**************************************************************************/
static int incite_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pIncite_Dlg->pdialog->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User confirmed incite
**************************************************************************/
static int diplomat_incite_yes_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pIncite_Dlg->actor_unit_id)
        && NULL != game_city_by_number(pIncite_Dlg->target_id)) {
      request_do_action(pIncite_Dlg->act_id, pIncite_Dlg->actor_unit_id,
                        pIncite_Dlg->target_id, 0, "");
    }

    popdown_incite_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Close incite dialog.
**************************************************************************/
static int exit_incite_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_incite_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Popdown a window asking a diplomatic unit if it wishes to incite the
  given enemy city.
**************************************************************************/
void popdown_incite_dialog(void)
{
  if (pIncite_Dlg) {
    act_sel_done_secondary(pIncite_Dlg->actor_unit_id);

    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pIncite_Dlg->pdialog->pBeginWidgetList,
				pIncite_Dlg->pdialog->pEndWidgetList);
    FC_FREE(pIncite_Dlg->pdialog);
    FC_FREE(pIncite_Dlg);
    flush_dirty();
  }
}

/**********************************************************************//**
  Popup a window asking a diplomatic unit if it wishes to incite the
  given enemy city.
**************************************************************************/
void popup_incite_dialog(struct unit *actor, struct city *pCity, int cost,
                         const struct action *paction)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  utf8_str *pstr;
  char tBuf[255], cBuf[255];
  bool exit = FALSE;
  SDL_Rect area;

  if (pIncite_Dlg) {
    return;
  }

  /* Should be set before sending request to the server. */
  fc_assert(is_more_user_input_needed);

  if (!actor || !unit_can_do_action(actor, paction->id)) {
    act_sel_done_secondary(actor ? actor->id : IDENTITY_NUMBER_ZERO);
    return;
  }

  is_unit_move_blocked = TRUE;

  pIncite_Dlg = fc_calloc(1, sizeof(struct small_diplomat_dialog));
  pIncite_Dlg->actor_unit_id = actor->id;
  pIncite_Dlg->target_id = pCity->id;
  pIncite_Dlg->act_id = paction->id;
  pIncite_Dlg->pdialog = fc_calloc(1, sizeof(struct SMALL_DLG));

  fc_snprintf(tBuf, ARRAY_SIZE(tBuf), PL_("Treasury contains %d gold.",
                                          "Treasury contains %d gold.",
                                          client_player()->economic.gold),
              client_player()->economic.gold);

  /* window */
  pstr = create_utf8_from_char(_("Incite a Revolt!"), adj_font(12));

  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = incite_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_INCITE_DLG_WINDOW, pWindow);
  pIncite_Dlg->pdialog->pEndWidgetList = pWindow;

  area = pWindow->area;
  area.w  =MAX(area.w, adj_size(8));
  area.h = MAX(area.h, adj_size(2));

  if (INCITE_IMPOSSIBLE_COST == cost) {
    /* exit button */
    pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                            WF_WIDGET_HAS_INFO_LABEL
                            | WF_RESTORE_BACKGROUND);
    pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                             adj_font(12));
    area.w += pBuf->size.w + adj_size(10);
    pBuf->action = exit_incite_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;

    add_to_gui_list(ID_INCITE_DLG_EXIT_BUTTON, pBuf);
    exit = TRUE;
    /* --------------- */

    fc_snprintf(cBuf, sizeof(cBuf), _("You can't incite a revolt in %s."),
                city_name_get(pCity));

    create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf, NULL);

    add_to_gui_list(ID_LABEL , pBuf);

    area.w = MAX(area.w , pBuf->size.w);
    area.h += pBuf->size.h;
    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("City can't be incited!"), NULL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;

  } else if (cost <= client_player()->economic.gold) {
    fc_snprintf(cBuf, sizeof(cBuf),
                /* TRANS: %s is pre-pluralised "Treasury contains %d gold." */
                PL_("Incite a revolt for %d gold?\n%s",
                    "Incite a revolt for %d gold?\n%s", cost), cost, tBuf);

    create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf, NULL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;

    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Yes") , diplomat_incite_yes_callback);

    pBuf->data.city = pCity;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(MAX_ID - actor->id, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ------- */
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("No") , exit_incite_dlg_callback);

    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;

  } else {
    /* exit button */
    pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                            WF_WIDGET_HAS_INFO_LABEL
                            | WF_RESTORE_BACKGROUND);
    pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                             adj_font(12));
    area.w += pBuf->size.w + adj_size(10);
    pBuf->action = exit_incite_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;

    add_to_gui_list(ID_INCITE_DLG_EXIT_BUTTON, pBuf);
    exit = TRUE;
    /* --------------- */

    fc_snprintf(cBuf, sizeof(cBuf),
                /* TRANS: %s is pre-pluralised "Treasury contains %d gold." */
                PL_("Inciting a revolt costs %d gold.\n%s",
                    "Inciting a revolt costs %d gold.\n%s", cost), cost, tBuf);

    create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf, NULL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;

    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Traitors Demand Too Much!"), NULL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }
  pIncite_Dlg->pdialog->pBeginWidgetList = pBuf;

  /* setup window size and start position */

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           pCity->tile);

  /* setup widget size and start position */
  pBuf = pWindow;

  if (exit) {
    /* exit button */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
    pBuf->size.y = pWindow->size.y + adj_size(2);
  }

  pBuf = pBuf->prev;
  setup_vertical_widgets_position(1,
	area.x,
  	area.y + 1, area.w, 0,
	pIncite_Dlg->pdialog->pBeginWidgetList, pBuf);

  /* --------------------- */
  /* redraw */
  redraw_group(pIncite_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}

/* ====================================================================== */
/* ============================ BRIBE DIALOG ========================== */
/* ====================================================================== */
static struct small_diplomat_dialog *pBribe_Dlg = NULL;

/**********************************************************************//**
  User interacted with bribe dialog window.
**************************************************************************/
static int bribe_dlg_window_callback(struct widget *pWindow)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    move_window_group(pBribe_Dlg->pdialog->pBeginWidgetList, pWindow);
  }

  return -1;
}

/**********************************************************************//**
  User confirmed bribe.
**************************************************************************/
static int diplomat_bribe_yes_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    if (NULL != game_unit_by_number(pBribe_Dlg->actor_unit_id)
        && NULL != game_unit_by_number(pBribe_Dlg->target_id)) {
      request_do_action(pBribe_Dlg->act_id, pBribe_Dlg->actor_unit_id,
                        pBribe_Dlg->target_id, 0, "");
    }
    popdown_bribe_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Close bribe dialog.
**************************************************************************/
static int exit_bribe_dlg_callback(struct widget *pWidget)
{
  if (Main.event.button.button == SDL_BUTTON_LEFT) {
    popdown_bribe_dialog();
  }

  return -1;
}

/**********************************************************************//**
  Popdown a dialog asking a diplomatic unit if it wishes to bribe the
  given enemy unit.
**************************************************************************/
void popdown_bribe_dialog(void)
{
  if (pBribe_Dlg) {
    act_sel_done_secondary(pBribe_Dlg->actor_unit_id);

    is_unit_move_blocked = FALSE;
    popdown_window_group_dialog(pBribe_Dlg->pdialog->pBeginWidgetList,
                                pBribe_Dlg->pdialog->pEndWidgetList);
    FC_FREE(pBribe_Dlg->pdialog);
    FC_FREE(pBribe_Dlg);
    flush_dirty();
  }
}

/**********************************************************************//**
  Popup a dialog asking a diplomatic unit if it wishes to bribe the
  given enemy unit.
**************************************************************************/
void popup_bribe_dialog(struct unit *actor, struct unit *pUnit, int cost,
                        const struct action *paction)
{
  struct widget *pWindow = NULL, *pBuf = NULL;
  utf8_str *pstr;
  char tBuf[255], cBuf[255];
  bool exit = FALSE;
  SDL_Rect area;

  if (pBribe_Dlg) {
    return;
  }

  /* Should be set before sending request to the server. */
  fc_assert(is_more_user_input_needed);

  if (!actor || !unit_can_do_action(actor, paction->id)) {
    act_sel_done_secondary(actor ? actor->id : IDENTITY_NUMBER_ZERO);
    return;
  }

  is_unit_move_blocked = TRUE;

  pBribe_Dlg = fc_calloc(1, sizeof(struct small_diplomat_dialog));
  pBribe_Dlg->act_id = paction->id;
  pBribe_Dlg->actor_unit_id = actor->id;
  pBribe_Dlg->target_id = pUnit->id;
  pBribe_Dlg->pdialog = fc_calloc(1, sizeof(struct SMALL_DLG));

  fc_snprintf(tBuf, ARRAY_SIZE(tBuf), PL_("Treasury contains %d gold.",
                                          "Treasury contains %d gold.",
                                          client_player()->economic.gold),
              client_player()->economic.gold);

  /* window */
  pstr = create_utf8_from_char(_("Bribe Enemy Unit"), adj_font(12));

  pstr->style |= TTF_STYLE_BOLD;

  pWindow = create_window_skeleton(NULL, pstr, 0);

  pWindow->action = bribe_dlg_window_callback;
  set_wstate(pWindow, FC_WS_NORMAL);

  add_to_gui_list(ID_BRIBE_DLG_WINDOW, pWindow);
  pBribe_Dlg->pdialog->pEndWidgetList = pWindow;

  area = pWindow->area;
  area.w = MAX(area.w, adj_size(8));
  area.h = MAX(area.h, adj_size(2));

  if (cost <= client_player()->economic.gold) {
    fc_snprintf(cBuf, sizeof(cBuf),
                /* TRANS: %s is pre-pluralised "Treasury contains %d gold." */
                PL_("Bribe unit for %d gold?\n%s",
                    "Bribe unit for %d gold?\n%s", cost), cost, tBuf);

    create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf, NULL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;

    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Yes"), diplomat_bribe_yes_callback);
    pBuf->data.unit = pUnit;
    set_wstate(pBuf, FC_WS_NORMAL);

    add_to_gui_list(MAX_ID - actor->id, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
    /* ------- */
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("No") , exit_bribe_dlg_callback);

    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;

  } else {
    /* exit button */
    pBuf = create_themeicon(current_theme->Small_CANCEL_Icon, pWindow->dst,
                            WF_WIDGET_HAS_INFO_LABEL
                            | WF_RESTORE_BACKGROUND);
    pBuf->info_label = create_utf8_from_char(_("Close Dialog (Esc)"),
                                             adj_font(12));
    area.w += pBuf->size.w + adj_size(10);
    pBuf->action = exit_bribe_dlg_callback;
    set_wstate(pBuf, FC_WS_NORMAL);
    pBuf->key = SDLK_ESCAPE;

    add_to_gui_list(ID_BRIBE_DLG_EXIT_BUTTON, pBuf);
    exit = TRUE;
    /* --------------- */

    fc_snprintf(cBuf, sizeof(cBuf),
                /* TRANS: %s is pre-pluralised "Treasury contains %d gold." */
                PL_("Bribing the unit costs %d gold.\n%s",
                    "Bribing the unit costs %d gold.\n%s", cost), cost, tBuf);

    create_active_iconlabel(pBuf, pWindow->dst, pstr, cBuf, NULL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;

    /*------------*/
    create_active_iconlabel(pBuf, pWindow->dst, pstr,
                            _("Traitors Demand Too Much!"), NULL);

    add_to_gui_list(ID_LABEL, pBuf);

    area.w = MAX(area.w, pBuf->size.w);
    area.h += pBuf->size.h;
  }
  pBribe_Dlg->pdialog->pBeginWidgetList = pBuf;

  /* setup window size and start position */

  resize_window(pWindow, NULL, NULL,
                (pWindow->size.w - pWindow->area.w) + area.w,
                (pWindow->size.h - pWindow->area.h) + area.h);

  area = pWindow->area;

  auto_center_on_focus_unit();
  put_window_near_map_tile(pWindow, pWindow->size.w, pWindow->size.h,
                           unit_tile(actor));

  /* setup widget size and start position */
  pBuf = pWindow;

  if (exit) {
    /* exit button */
    pBuf = pBuf->prev;
    pBuf->size.x = area.x + area.w - pBuf->size.w - 1;
    pBuf->size.y = pWindow->size.y + adj_size(2);
  }

  pBuf = pBuf->prev;
  setup_vertical_widgets_position(1,
	area.x,
  	area.y + 1, area.w, 0,
	pBribe_Dlg->pdialog->pBeginWidgetList, pBuf);

  /* --------------------- */
  /* redraw */
  redraw_group(pBribe_Dlg->pdialog->pBeginWidgetList, pWindow, 0);

  widget_flush(pWindow);
}
