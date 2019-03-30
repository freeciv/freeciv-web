/***********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
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

/* common */
#include "fc_types.h"
#include "game.h"
#include "name_translation.h"


#include "style.h"

static struct nation_style *styles = NULL;

static struct music_style *music_styles = NULL;

/**********************************************************************//**
  Initialise styles structures.
**************************************************************************/
void styles_alloc(int count)
{
  int i;

  styles = fc_malloc(count * sizeof(struct nation_style));

  for (i = 0; i < count; i++) {
    styles[i].id = i;
    styles[i].ruledit_disabled = FALSE;
  }
}

/**********************************************************************//**
  Free the memory associated with styles
**************************************************************************/
void styles_free(void)
{
  FC_FREE(styles);
  styles = NULL;
}

/**********************************************************************//**
  Return the number of styles.
**************************************************************************/
int style_count(void)
{
  return game.control.num_styles;
}

/**********************************************************************//**
  Return the style id.
**************************************************************************/
int style_number(const struct nation_style *pstyle)
{
  fc_assert_ret_val(NULL != pstyle, -1);

  return pstyle->id;
}

/**********************************************************************//**
  Return the style index.
**************************************************************************/
int style_index(const struct nation_style *pstyle)
{
  fc_assert_ret_val(NULL != pstyle, -1);

  return pstyle - styles;
}

/**********************************************************************//**
  Return style of given id.
**************************************************************************/
struct nation_style *style_by_number(int id)
{
  fc_assert_ret_val(id >= 0 && id < game.control.num_styles, NULL);

  return &styles[id];
}

/**********************************************************************//**
  Return the (translated) name of the style.
  You don't have to free the return pointer.
**************************************************************************/
const char *style_name_translation(const struct nation_style *pstyle)
{
  return name_translation_get(&pstyle->name);
}

/**********************************************************************//**
  Return the (untranslated) rule name of the style.
  You don't have to free the return pointer.
**************************************************************************/
const char *style_rule_name(const struct nation_style *pstyle)
{
  return rule_name_get(&pstyle->name);
}

/**********************************************************************//**
  Returns style matching rule name or NULL if there is no style
  with such name.
**************************************************************************/
struct nation_style *style_by_rule_name(const char *name)
{
  const char *qs = Qn_(name);

  styles_iterate(pstyle) {
    if (!fc_strcasecmp(style_rule_name(pstyle), qs)) {
      return pstyle;
    }
  } styles_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Initialise music styles structures.
**************************************************************************/
void music_styles_alloc(int count)
{
  int i;

  music_styles = fc_malloc(count * sizeof(struct music_style));

  for (i = 0; i < count; i++) {
    music_styles[i].id = i;
    requirement_vector_init(&(music_styles[i].reqs));
  }
}

/**********************************************************************//**
  Free the memory associated with music styles
**************************************************************************/
void music_styles_free(void)
{
  music_styles_iterate(pmus) {
    requirement_vector_free(&(pmus->reqs));
  } music_styles_iterate_end;

  FC_FREE(music_styles);
  music_styles = NULL;
}

/**********************************************************************//**
  Return the music style id.
**************************************************************************/
int music_style_number(const struct music_style *pms)
{
  fc_assert_ret_val(NULL != pms, -1);

  return pms->id;
}

/**********************************************************************//**
  Return music style of given id.
**************************************************************************/
struct music_style *music_style_by_number(int id)
{
  fc_assert_ret_val(id >= 0 && id < game.control.num_music_styles, NULL);

  if (music_styles == NULL) {
    return NULL;
  }

  return &music_styles[id];
}

/**********************************************************************//**
  Return music style for player
**************************************************************************/
struct music_style *player_music_style(struct player *plr)
{
  struct music_style *best = NULL;

  music_styles_iterate(pms) {
    if (are_reqs_active(plr, NULL, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL, &pms->reqs,
                        RPT_CERTAIN)) {
      best = pms;
    }
  } music_styles_iterate_end;

  return best;
}

/**********************************************************************//**
  Evaluate which style should be used to draw a city.
**************************************************************************/
int style_of_city(const struct city *pcity)
{
  return pcity->style;
}

/**********************************************************************//**
  Return basic city style representing nation style.
**************************************************************************/
int basic_city_style_for_style(struct nation_style *pstyle)
{
  enum fc_tristate style_style;
  int i;

  for (i = game.control.styles_count - 1; i >= 0; i--) {
    style_style = TRI_MAYBE;

    requirement_vector_iterate(&city_styles[i].reqs, preq) {
      if (preq->source.kind == VUT_STYLE
          && preq->source.value.style == pstyle
          && style_style != TRI_NO) {
        style_style = TRI_YES;
      } else {
        /* No any other requirements allowed at the moment.
         * TODO: Allow some other reqs */
        style_style = TRI_NO;
        break;
      }
    } requirement_vector_iterate_end;

    if (style_style == TRI_YES) {
      break;
    }
  }

  if (style_style == TRI_YES) {
    return i;
  }

  return -1;
}

/**********************************************************************//**
  Return citystyle of the city.
**************************************************************************/
int city_style(struct city *pcity)
{
  int i;
  struct player *plr = city_owner(pcity);

  for (i = game.control.styles_count - 1; i >= 0; i--) {
    if (are_reqs_active(plr, NULL, pcity, NULL, city_tile(pcity),
                        NULL, NULL, NULL, NULL, NULL,
                        &city_styles[i].reqs, RPT_CERTAIN)) {
      return i;
    }
  }

  return 0;
}
