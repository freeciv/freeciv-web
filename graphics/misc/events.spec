
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2015-Mar-25"

[info]

artists = "
    GriffonSpade
    Hogne Håskjold <haskjold@gmail.com> (gold coin)
"

[file]
gfx = "misc/events"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 15
dy = 20

tiles = { "row", "column", "tag"
  0,  0, "e_city_cantbuild"
  0,  1, "e_city_lost"
  0,  2, "e_city_love"
  0,  3, "e_city_disorder"
  0,  4, "e_city_famine"
  0,  5, "e_city_famine_feared"
  0,  5, "e_unit_built_pop_cost"
  0,  6, "e_city_growth"
  0,  7, "e_city_may_soon_grow"
  0,  8, "e_city_aqueduct"
  0,  9, "e_city_aq_building"
  0, 10, "e_city_normal"
  0, 11, "e_city_nuked"
  0, 12, "e_city_cma_release"
  0, 13, "e_city_gran_throttle"
  0, 14, "e_city_transfer"
  0, 15, "e_city_build"
  0, 16, "e_city_production_changed"
  0, 17, "e_disaster"
  0, 18, "e_city_plague"
  0, 12, "e_worklist" ; (0, 19) unused
  0, 12, "e_city_radius_sq" ; (0, 20) unused

  1,  0,  "e_treaty_shared_vision"
  1,  1,  "e_treaty_alliance"
  1,  2,  "e_treaty_peace"
  1,  3,  "e_treaty_ceasefire"
  1,  4,  "e_treaty_broken"
  1,  5,  "e_treaty_embassy"

  1,  6, "e_imp_buy"
  1,  7, "e_imp_build"
  1,  8, "e_imp_auctioned"
  1,  9, "e_imp_sold"
  1,  10, "e_imp_auto"
;  1,  11, "e_imp_destroyed"

  1, 12,  "e_wonder_build"
  1, 13,  "e_wonder_started"
  1, 14,  "e_wonder_stopped"
  1, 15,  "e_wonder_will_be_built"
  1, 16,  "e_wonder_obsolete"

  2,  0, "e_hut_barb"
  2,  1, "e_hut_city"
  2,  2, "e_hut_gold"
  2,  3, "e_hut_barb_killed"
  2,  4, "e_hut_merc"
  2,  5, "e_hut_settler"
  2,  6, "e_hut_tech"
  2,  7, "e_hut_barb_city_near"

  2,  8, "e_tech_gain"
  2,  9, "e_tech_learned"
  2, 10, "e_tech_lost"
  2, 11, "e_tech_embassy"
  2, 11, "e_tech_goal"
 
  3,  0,  "e_unit_lost_att"
  3,  1,  "e_unit_win_att"
  3,  2,  "e_unit_buy"
  3,  3,  "e_unit_built"
  3,  4,  "e_unit_lost_def"
  3,  5,  "e_unit_lost_misc"
  3,  6,  "e_unit_became_vet"
  3,  7,  "e_unit_upgraded"
  3,  8,  "e_unit_relocated"
  3,  9,  "e_unit_orders"
  3, 10,  "e_unit_illegal_action"
  3, 11,  "e_caravan_action"
  3, 12,  "e_unit_win_def"
  3, 13,  "e_unit_escaped"
  3, 14,  "e_unit_was_expelled"
  3, 15,  "e_unit_did_expel"
  3, 16,  "e_unit_action_failed"
  3, 17,  "e_my_unit_did_heal"
  3, 18,  "e_my_unit_was_healed"

  4,  0, "e_my_diplomat_escape" ; base sprite for my diplomats
  4,  1, "e_my_diplomat_bribe"
  4,  1, "e_my_spy_steal_gold"
  4,  2, "e_my_diplomat_incite"
  4,  3, "e_my_diplomat_embassy"
  4,  4, "e_my_diplomat_failed"
  4,  5, "e_my_diplomat_sabotage"
  4,  6, "e_my_diplomat_theft"
  4,  7, "e_diplomatic_incident"
  4,  8, "e_my_diplomat_poison"
  4,  9, "e_my_spy_nuke"
  4, 10, "e_my_spy_steal_map"
;  4, 11, "e_embassy_was_expelled"

;  5,  0, "e_enemy_diplomat_escape" ; base sprite for enemy diplomats
  5,  1, "e_enemy_diplomat_bribe"
  5,  1, "e_enemy_spy_steal_gold"
  3,  2, "e_enemy_diplomat_incite"
  3,  3, "e_enemy_diplomat_embassy"
  3,  4, "e_enemy_diplomat_failed"
  3,  5, "e_enemy_diplomat_sabotage"
  3,  6, "e_enemy_diplomat_theft"
;  3,  7, "e_enemy_diplomatic_incident"
  5,  8, "e_enemy_diplomat_poison"
  5,  9, "e_enemy_spy_nuke"
  5, 10, "e_enemy_spy_steal_map"
;  5, 11, "e_embassy_did_expel"


  6,  0, "e_achievement"
  6,  1, "e_uprising"
  6,  2, "e_civil_war"
  6,  3, "e_anarchy"
  6,  4, "e_first_contact"
  6,  4, "e_diplomacy"
  6,  4, "e_new_government" ; (4, 5) unused
  6,  4, "e_nation_selected"
  6,  4, "e_setting"
  6,  4, "e_message_wall"
  6,  4, "e_connection"
  6,  5, "e_destroyed"
  6,  6, "e_low_on_funds"
  6,  7, "e_pollution" ; copy from small.png
  6,  8, "e_revolt_done"
  6,  9, "e_revolt_start"
  6, 10, "e_spaceship"
  6, 11, "e_spontaneous_extra"

  6,  4, "e_vote_new"
  6,  5, "e_vote_aborted"
  6, 11, "e_vote_resolved"

  6, 12, "e_nuke"
;  6, 13, "e_chem"
  6, 14, "e_ai_debug"
  6, 14, "e_script"
  6, 14, "e_broadcast_report"
  6, 14, "e_report"
  6, 14, "e_chat_msg"
  6, 14, "e_log_error"
  6, 14, "e_deprecation_warning"
  6, 14, "e_beginner_help"
  6, 15, "e_global_eco"
  6, 15, "e_log_fatal"
  6, 15, "e_chat_error"
  6, 16, "e_bad_command"

  6, 17,  "e_game_start"
  6, 18,  "e_next_year"
  6, 19,  "e_game_end"
  6, 20,  "e_turn_bell"

}
