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
#ifndef FC__RULESET_H
#define FC__RULESET_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define RULESET_CAPABILITIES_BASE "+Freeciv-ruleset-Devel-2017.Jan.02"
#ifdef FREECIV_WEB
#define RULESET_CAPABILITIES RULESET_CAPABILITIES_BASE " +web-compatible"
#else  /* FREECIV_WEB */
#define RULESET_CAPABILITIES RULESET_CAPABILITIES_BASE
#endif /* FREECIV_WEB */
/*
 * Ruleset capabilities acceptable to this program:
 *
 * +Freeciv-3.0-ruleset
 *    - basic ruleset format for Freeciv versions 3.0.x; required
 *
 * +Freeciv-tilespec-Devel-YYYY.MMM.DD
 *    - ruleset of the development version at the given data
 */

struct conn_list;

typedef void (*rs_conversion_logger)(const char *msg);

/* functions */
bool load_rulesets(const char *restore, bool compat_mode,
                   rs_conversion_logger logger,
                   bool act, bool buffer_script);
bool reload_rulesets_settings(void);
void send_rulesets(struct conn_list *dest);

void rulesets_deinit(void);

void ruleset_error_real(const char *file, const char *function,
                        int line, enum log_level level,
                        const char *format, ...)
  fc__attribute((__format__ (__printf__, 5, 6)));

#define ruleset_error(level, format, ...)                               \
  if (log_do_output_for_level(level)) {                                 \
    ruleset_error_real(__FILE__, __FUNCTION__, __FC_LINE__,             \
                       level, format, ## __VA_ARGS__);                  \
  }

char *get_script_buffer(void);
char *get_parser_buffer(void);

/* Default ruleset values that are not settings (in game.h) */

#define GAME_DEFAULT_ADDTOSIZE           9
#define GAME_DEFAULT_CHANGABLE_TAX       TRUE
#define GAME_DEFAULT_VISION_REVEAL_TILES FALSE
#define GAME_DEFAULT_NATIONALITY         FALSE
#define GAME_DEFAULT_CONVERT_SPEED       50
#define GAME_DEFAULT_DISASTER_FREQ       10
#define GAME_DEFAULT_ACH_UNIQUE          TRUE
#define GAME_DEFAULT_ACH_VALUE           1
#define RS_DEFAULT_MUUK_FOOD_WIPE        TRUE
#define RS_DEFAULT_MUUK_GOLD_WIPE        TRUE
#define RS_DEFAULT_MUUK_SHIELD_WIPE      FALSE
#define RS_DEFAULT_TECH_STEAL_HOLES      TRUE
#define RS_DEFAULT_TECH_TRADE_HOLES      TRUE
#define RS_DEFAULT_TECH_TRADE_LOSS_HOLES TRUE
#define RS_DEFAULT_TECH_PARASITE_HOLES   TRUE
#define RS_DEFAULT_TECH_LOSS_HOLES       TRUE
#define RS_DEFAULT_PYTHAGOREAN_DIAGONAL  FALSE

#define RS_DEFAULT_GOLD_UPKEEP_STYLE     "City"
#define RS_DEFAULT_TECH_COST_STYLE       "Civ I|II"
#define RS_DEFAULT_TECH_LEAKAGE          "None"
#define RS_DEFAULT_TECH_UPKEEP_STYLE     "None"

#define RS_DEFAULT_CULTURE_VIC_POINTS    1000
#define RS_DEFAULT_CULTURE_VIC_LEAD      300
#define RS_DEFAULT_CULTURE_MIGRATION_PML 50

#define RS_DEFAULT_EXTRA_APPEARANCE      15
#define RS_DEFAULT_EXTRA_DISAPPEARANCE   15

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__RULESET_H */
