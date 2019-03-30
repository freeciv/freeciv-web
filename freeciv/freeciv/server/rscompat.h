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
#ifndef FC__RSCOMPAT_H
#define FC__RSCOMPAT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"

/* server */
#include "ruleset.h"

#define RULESET_COMPAT_CAP "+Freeciv-ruleset-Devel-2015.January.14"

struct rscompat_info
{
  bool compat_mode;
  rs_conversion_logger log_cb;
  int ver_buildings;
  int ver_cities;
  int ver_effects;
  int ver_game;
  int ver_governments;
  int ver_nations;
  int ver_styles;
  int ver_techs;
  int ver_terrain;
  int ver_units;
};

void rscompat_init_info(struct rscompat_info *info);

int rscompat_check_capabilities(struct section_file *file, const char *filename,
                                struct rscompat_info *info);

bool rscompat_names(struct rscompat_info *info);

void rscompat_postprocess(struct rscompat_info *info);

/* Functions from ruleset.c made visible to rscompat.c */
struct requirement_vector *lookup_req_list(struct section_file *file,
                                           struct rscompat_info *compat,
                                           const char *sec,
                                           const char *sub,
                                           const char *rfor);

/* Functions specific to 3.0 -> 3.1 transition */
const char *rscompat_req_type_name_3_1(const char *type, const char *range,
                                       bool survives, bool present,
                                       bool quiet, const char *value);
const char *rscompat_req_name_3_1(const char *type,
                                  const char *old_name);
const char *rscompat_utype_flag_name_3_1(struct rscompat_info *info,
                                         const char *old_type);
bool rscompat_old_effect_3_1(const char *type, struct section_file *file,
                             const char *sec_name, struct rscompat_info *compat);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__RSCOMPAT_H */
