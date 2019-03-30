/***********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__PF_TOOLS_H
#define FC__PF_TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* common/aicore */
#include "path_finding.h"

/* 
 * Use to create 'amphibious' paths. An amphibious path starts on a sea tile,
 * perhaps goes over some other sea tiles, then perhaps goes over some land
 * tiles. This is suitable for a land unit riding on a ferry.
 * If you want, you can use different call-backs for the land and sea legs,
 * by changing the 'land' and 'sea' fields.
 * Initialise the 'land' and 'sea' fields as you like, then initialise the
 * other fields using using pft_fill_amphibious_parameter().
 * Give the 'combined' field to pf_create_map to create a map
 * for finding the path.
 */
struct pft_amphibious
{
  /* The caller must initialize these. */
  struct pf_parameter land, sea;

  /* Initialized in pft_fill_amphibious_parameter; do not touch. */
  int land_scale, sea_scale;
  struct pf_parameter combined;
};


void pft_fill_unit_parameter(struct pf_parameter *parameter,
                             const struct unit *punit);
void pft_fill_unit_overlap_param(struct pf_parameter *parameter,
                                 const struct unit *punit);
void pft_fill_unit_attack_param(struct pf_parameter *parameter,
                                const struct unit *punit);

void pft_fill_utype_parameter(struct pf_parameter *parameter,
                              const struct unit_type *punittype,
                              struct tile *pstart_tile,
                              struct player *pplayer);
void pft_fill_utype_overlap_param(struct pf_parameter *parameter,
                                  const struct unit_type *punittype,
                                  struct tile *pstart_tile,
                                  struct player *pplayer);
void pft_fill_utype_attack_param(struct pf_parameter *parameter,
                                 const struct unit_type *punittype,
                                 struct tile *pstart_tile,
                                 struct player *pplayer);
void pft_fill_reverse_parameter(struct pf_parameter *parameter,
                                struct tile *target_tile);

void pft_fill_amphibious_parameter(struct pft_amphibious *parameter);
enum tile_behavior no_fights_or_unknown(const struct tile *ptile,
                                        enum known_type known,
                                        const struct pf_parameter *param);
enum tile_behavior no_fights(const struct tile *ptile, enum known_type known,
                             const struct pf_parameter *param);
enum tile_behavior no_intermediate_fights(const struct tile *ptile,
                                          enum known_type known,
                                          const struct pf_parameter *param);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__PF_TOOLS_H */
