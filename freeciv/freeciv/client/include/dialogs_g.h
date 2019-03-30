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
#ifndef FC__DIALOGS_G_H
#define FC__DIALOGS_G_H

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "actions.h"
#include "fc_types.h"
#include "featured_text.h"      /* struct text_tag_list */
#include "nation.h"		/* Nation_type_id */
#include "terrain.h"		/* enum tile_special_type */
#include "unitlist.h"

/* client */
#include "gui_proto_constructor.h"

struct packet_nations_selected_info;

GUI_FUNC_PROTO(void, popup_notify_goto_dialog, const char *headline,
               const char *lines,
               const struct text_tag_list *tags,
               struct tile *ptile)
GUI_FUNC_PROTO(void, popup_notify_dialog, const char *caption,
               const char *headline, const char *lines)
GUI_FUNC_PROTO(void, popup_connect_msg, const char *headline, const char *message)

GUI_FUNC_PROTO(void, popup_races_dialog, struct player *pplayer)
GUI_FUNC_PROTO(void, popdown_races_dialog, void)

GUI_FUNC_PROTO(void, unit_select_dialog_popup, struct tile *ptile)
void unit_select_dialog_update(void); /* Defined in update_queue.c. */
GUI_FUNC_PROTO(void, unit_select_dialog_update_real, void *unused)

GUI_FUNC_PROTO(void, races_toggles_set_sensitive, void)
GUI_FUNC_PROTO(void, races_update_pickable, bool nationset_change)

GUI_FUNC_PROTO(void, popup_combat_info, int attacker_unit_id,
               int defender_unit_id, int attacker_hp, int defender_hp,
               bool make_att_veteran, bool make_def_veteran)
GUI_FUNC_PROTO(void, show_img_play_snd, const char *img_path,
               const char *snd_path, const char *desc, bool fullsize)
GUI_FUNC_PROTO(void, popup_action_selection, struct unit *actor_unit,
               struct city *target_city, struct unit *target_unit,
               struct tile *target_tile, struct extra_type *target_extra,
               const struct act_prob *act_probs)
GUI_FUNC_PROTO(int, action_selection_actor_unit, void)
GUI_FUNC_PROTO(int, action_selection_target_city, void)
GUI_FUNC_PROTO(int, action_selection_target_unit, void)
GUI_FUNC_PROTO(int, action_selection_target_tile, void)
GUI_FUNC_PROTO(int, action_selection_target_extra, void)
GUI_FUNC_PROTO(void, action_selection_close, void)
GUI_FUNC_PROTO(void, action_selection_refresh, struct unit *actor_unit,
               struct city *target_city, struct unit *target_unit,
               struct tile *target_tile, struct extra_type *target_extra,
               const struct act_prob *act_probs)
GUI_FUNC_PROTO(void, action_selection_no_longer_in_progress_gui_specific,
               int actor_unit_id)
GUI_FUNC_PROTO(void, popup_incite_dialog, struct unit *actor,
               struct city *pcity, int cost, const struct action *paction)
GUI_FUNC_PROTO(void, popup_bribe_dialog, struct unit *actor,
               struct unit *punit, int cost, const struct action *paction)
GUI_FUNC_PROTO(void, popup_sabotage_dialog, struct unit *actor,
               struct city *pcity, const struct action *paction)
GUI_FUNC_PROTO(void, popup_pillage_dialog, struct unit *punit, bv_extras extras)
GUI_FUNC_PROTO(void, popup_upgrade_dialog, struct unit_list *punits)
GUI_FUNC_PROTO(void, popup_disband_dialog, struct unit_list *punits)
GUI_FUNC_PROTO(void, popup_tileset_suggestion_dialog, void)
GUI_FUNC_PROTO(void, popup_soundset_suggestion_dialog, void)
GUI_FUNC_PROTO(void, popup_musicset_suggestion_dialog, void)
GUI_FUNC_PROTO(bool, popup_theme_suggestion_dialog, const char *theme_name)
GUI_FUNC_PROTO(void, show_tech_gained_dialog, Tech_type_id tech)
GUI_FUNC_PROTO(void, show_tileset_error, const char *msg)
GUI_FUNC_PROTO(bool, handmade_scenario_warning, void)

GUI_FUNC_PROTO(void, popdown_all_game_dialogs, void)

GUI_FUNC_PROTO(bool, request_transport, struct unit *pcargo, struct tile *ptile)

#endif  /* FC__DIALOGS_G_H */
