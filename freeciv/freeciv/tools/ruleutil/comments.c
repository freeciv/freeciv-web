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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "mem.h"
#include "registry.h"
#include "section_file.h"

#include "comments.h"

static struct {
  char *file_header;
  char *buildings;
  char *tech_classes;
  char *techs;
  char *govs;
  char *policies;
  char *uclasses;
  char *utypes;
  char *terrains;
  char *resources;
  char *extras;
  char *bases;
  char *roads;
  char *styles;
  char *citystyles;
  char *musicstyles;
  char *effects;
  char *disasters;
  char *achievements;
  char *goods;
  char *enablers;
  char *specialists;
  char *nations;
  char *nationgroups;
  char *nationsets;
  char *clauses;
} comments_storage;

/**********************************************************************//**
  Load comments to add to the saved rulesets.
**************************************************************************/
bool comments_load(void)
{
  struct section_file *comment_file;
  const char *fullpath;

  fullpath = fileinfoname(get_data_dirs(), "ruledit/comments.txt");

  if (fullpath == NULL) {
    return FALSE;
  }

  comment_file = secfile_load(fullpath, FALSE);
  if (comment_file == NULL) {
    return FALSE;
  }

#define comment_load(target, comment_file, comment_path)                  \
{                                                                         \
  const char *comment;                                                    \
                                                                          \
  if ((comment = secfile_lookup_str(comment_file, comment_path))) {       \
    target = fc_strdup(comment);                                          \
  } else {                                                                \
    return FALSE;                                                         \
  }                                                                       \
}

  comment_load(comments_storage.file_header, comment_file, "common.header");
  comment_load(comments_storage.buildings,
               comment_file, "typedoc.buildings");
  comment_load(comments_storage.tech_classes,
               comment_file, "typedoc.tech_classes");
  comment_load(comments_storage.techs, comment_file, "typedoc.techs");
  comment_load(comments_storage.govs, comment_file, "typedoc.governments");
  comment_load(comments_storage.policies, comment_file, "typedoc.policies");
  comment_load(comments_storage.uclasses, comment_file, "typedoc.uclasses");
  comment_load(comments_storage.utypes, comment_file, "typedoc.utypes");
  comment_load(comments_storage.terrains, comment_file, "typedoc.terrains");
  comment_load(comments_storage.resources,
               comment_file, "typedoc.resources");
  comment_load(comments_storage.extras, comment_file, "typedoc.extras");
  comment_load(comments_storage.bases, comment_file, "typedoc.bases");
  comment_load(comments_storage.roads, comment_file, "typedoc.roads");
  comment_load(comments_storage.styles, comment_file, "typedoc.styles");
  comment_load(comments_storage.citystyles,
               comment_file, "typedoc.citystyles");
  comment_load(comments_storage.musicstyles,
               comment_file, "typedoc.musicstyles");
  comment_load(comments_storage.effects, comment_file, "typedoc.effects");
  comment_load(comments_storage.disasters,
               comment_file, "typedoc.disasters");
  comment_load(comments_storage.achievements,
               comment_file, "typedoc.achievements");
  comment_load(comments_storage.goods, comment_file, "typedoc.goods");
  comment_load(comments_storage.enablers, comment_file, "typedoc.enablers");
  comment_load(comments_storage.specialists,
               comment_file, "typedoc.specialists");
  comment_load(comments_storage.nations, comment_file, "typedoc.nations");
  comment_load(comments_storage.nationgroups,
               comment_file, "typedoc.nationgroups");
  comment_load(comments_storage.nationsets,
               comment_file, "typedoc.nationsets");
  comment_load(comments_storage.clauses, comment_file, "typedoc.clauses");

  secfile_check_unused(comment_file);
  secfile_destroy(comment_file);

  return TRUE;
}

/**********************************************************************//**
  Free comments.
**************************************************************************/
void comments_free(void)
{
  free(comments_storage.file_header);
}

/**********************************************************************//**
  Generic comment writing function with some error checking.
**************************************************************************/
static void comment_write(struct section_file *sfile, const char *comment,
                          const char *name)
{
  if (comment == NULL) {
    log_error("Comment for %s missing.", name);
    return;
  }

  secfile_insert_long_comment(sfile, comment);
}

/**********************************************************************//**
  Write file header.
**************************************************************************/
void comment_file_header(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.file_header, "File header");
}

/**********************************************************************//**
  Write buildings header.
**************************************************************************/
void comment_buildings(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.buildings, "Buildings");
}

/**********************************************************************//**
  Write tech classess header.
**************************************************************************/
void comment_tech_classes(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.tech_classes, "Tech Classes");
}

/**********************************************************************//**
  Write techs header.
**************************************************************************/
void comment_techs(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.techs, "Techs");
}

/**********************************************************************//**
  Write governments header.
**************************************************************************/
void comment_govs(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.govs, "Governments");
}

/**********************************************************************//**
  Write policies header.
**************************************************************************/
void comment_policies(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.policies, "Policies");
}

/**********************************************************************//**
  Write unit classes header.
**************************************************************************/
void comment_uclasses(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.uclasses, "Unit classes");
}

/**********************************************************************//**
  Write unit types header.
**************************************************************************/
void comment_utypes(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.utypes, "Unit types");
}

/**********************************************************************//**
  Write terrains header.
**************************************************************************/
void comment_terrains(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.terrains, "Terrains");
}

/**********************************************************************//**
  Write resources header.
**************************************************************************/
void comment_resources(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.resources, "Resources");
}

/**********************************************************************//**
  Write extras header.
**************************************************************************/
void comment_extras(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.extras, "Extras");
}

/**********************************************************************//**
  Write bases header.
**************************************************************************/
void comment_bases(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.bases, "Bases");
}

/**********************************************************************//**
  Write roads header.
**************************************************************************/
void comment_roads(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.roads, "Roads");
}

/**********************************************************************//**
  Write styles header.
**************************************************************************/
void comment_styles(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.styles, "Styles");
}

/**********************************************************************//**
  Write city styles header.
**************************************************************************/
void comment_citystyles(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.citystyles, "City Styles");
}

/**********************************************************************//**
  Write music styles header.
**************************************************************************/
void comment_musicstyles(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.musicstyles, "Music Styles");
}

/**********************************************************************//**
  Write effects header.
**************************************************************************/
void comment_effects(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.effects, "Effects");
}

/**********************************************************************//**
  Write disasters header.
**************************************************************************/
void comment_disasters(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.disasters, "Disasters");
}

/**********************************************************************//**
  Write achievements header.
**************************************************************************/
void comment_achievements(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.achievements, "Achievements");
}

/**********************************************************************//**
  Write goods header.
**************************************************************************/
void comment_goods(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.goods, "Goods");
}

/**********************************************************************//**
  Write action enablers header.
**************************************************************************/
void comment_enablers(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.enablers, "Action Enablers");
}

/**********************************************************************//**
  Write specialists header.
**************************************************************************/
void comment_specialists(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.specialists, "Specialists");
}

/**********************************************************************//**
  Write nations header.
**************************************************************************/
void comment_nations(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.nations, "Nations");
}

/**********************************************************************//**
  Write nationgroups header.
**************************************************************************/
void comment_nationgroups(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.nationgroups, "Nationgroups");
}

/**********************************************************************//**
  Write nationsets header.
**************************************************************************/
void comment_nationsets(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.nationsets, "Nationsets");
}

/**********************************************************************//**
  Write clauses header.
**************************************************************************/
void comment_clauses(struct section_file *sfile)
{
  comment_write(sfile, comments_storage.clauses, "Clauses");
}
