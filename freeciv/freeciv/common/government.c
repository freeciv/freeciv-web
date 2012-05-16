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

#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "mem.h"
#include "player.h"
#include "shared.h"
#include "support.h"
#include "tech.h"

#include "government.h"

struct government *governments = NULL;

#define CHECK_GOVERNMENT(gp) assert((NULL != gp) && ((gp) == &governments[(gp)->item_number]))

/****************************************************************************
  Returns the government that has the given (translated) name.
  Returns NULL if none match.
****************************************************************************/
struct government *find_government_by_translated_name(const char *name)
{
  government_iterate(gov) {
    if (0 == strcmp(government_name_translation(gov), name)) {
      return gov;
    }
  } government_iterate_end;

  return NULL;
}

/****************************************************************************
  Returns the government that has the given (untranslated) rule name.
  Returns NULL if none match.
****************************************************************************/
struct government *find_government_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  government_iterate(gov) {
    if (0 == mystrcasecmp(government_rule_name(gov), qname)) {
      return gov;
    }
  } government_iterate_end;

  return NULL;
}

/**************************************************************************
  Return the number of governments.
**************************************************************************/
int government_count(void)
{
  return game.control.government_count;
}

/**************************************************************************
  Return the government index.

  Currently same as government_number(), paired with government_count()
  indicates use as an array index.
**************************************************************************/
int government_index(const struct government *pgovern)
{
  assert(pgovern);
  return pgovern - governments;
}

/**************************************************************************
  Return the government index.
**************************************************************************/
int government_number(const struct government *pgovern)
{
  assert(pgovern);
  return pgovern->item_number;
}

/****************************************************************************
  Return the government with the given index.

  This function returns NULL for an out-of-range index (some callers
  rely on this).
****************************************************************************/
struct government *government_by_number(const int gov)
{
  if (gov < 0 || gov >= game.control.government_count) {
    return NULL;
  }
  return &governments[gov];
}

/****************************************************************************
  Return the government of a player.
****************************************************************************/
struct government *government_of_player(const struct player *pplayer)
{
  assert(pplayer != NULL);
  return pplayer->government;
}

/****************************************************************************
  Return the government of the player who owns the city.
****************************************************************************/
struct government *government_of_city(const struct city *pcity)
{
  assert(pcity != NULL);
  return government_of_player(city_owner(pcity));
}

/****************************************************************************
  Return the (untranslated) rule name of the government.
  You don't have to free the return pointer.
****************************************************************************/
const char *government_rule_name(const struct government *pgovern)
{
  CHECK_GOVERNMENT(pgovern);
  return Qn_(pgovern->name.vernacular);
}

/****************************************************************************
  Return the (translated) name of the given government. 
  You don't have to free the return pointer.
****************************************************************************/
const char *government_name_translation(struct government *pgovern)
{
  CHECK_GOVERNMENT(pgovern);
  if (NULL == pgovern->name.translated) {
    /* delayed (unified) translation */
    pgovern->name.translated = ('\0' == pgovern->name.vernacular[0])
				? pgovern->name.vernacular
				: Q_(pgovern->name.vernacular);
  }
  return pgovern->name.translated;
}

/****************************************************************************
  Return the (translated) name of the given government of a player. 
  You don't have to free the return pointer.
****************************************************************************/
const char *government_name_for_player(const struct player *pplayer)
{
  return government_name_translation(government_of_player(pplayer));
}


/***************************************************************
...
***************************************************************/
const char *ruler_title_translation(const struct player *pp)
{
  struct government *gp = government_of_player(pp);
  struct nation_type *np = nation_of_player(pp);
  struct ruler_title *best_match = NULL;
  struct name_translation *sex;
  int i;

  CHECK_GOVERNMENT(gp);

  for(i=0; i<gp->num_ruler_titles; i++) {
    struct ruler_title *title = &gp->ruler_titles[i];

    if (title->nation == DEFAULT_TITLE && !best_match) {
      best_match = title;
    } else if (title->nation == np) {
      best_match = title;
      break;
    }
  }

  if (NULL == best_match) {
    freelog(LOG_ERROR,
	    "Missing title for government \"%s\" (%d) nation \"%s\" (%d).",
	    government_rule_name(gp),
	    government_number(gp),
	    nation_rule_name(np),
	    nation_number(np));
    return pp->is_male ? "Mr." : "Ms.";
  }

  sex = pp->is_male ? &best_match->male : &best_match->female;

  if (NULL == sex->translated) {
    /* delayed (unified) translation */
    sex->translated = ('\0' == sex->vernacular[0])
                      ? sex->vernacular
                      : Q_(sex->vernacular);
  }
  return sex->translated;
}

/***************************************************************
  Can change to government if appropriate tech exists, and one of:
   - no required tech (required is A_NONE)
   - player has required tech
   - we have an appropriate wonder
  Returns FALSE if pplayer is NULL (used for observers).
***************************************************************/
bool can_change_to_government(struct player *pplayer,
			      const struct government *gov)
{
  CHECK_GOVERNMENT(gov);

  if (!pplayer) {
    return FALSE;
  }

  if (get_player_bonus(pplayer, EFT_ANY_GOVERNMENT) > 0) {
    /* Note, this may allow govs that are on someone else's "tech tree". */
    return TRUE;
  }

  return are_reqs_active(pplayer, NULL, NULL, NULL, NULL, NULL, NULL,
			 &gov->reqs, RPT_CERTAIN);
}

/**************************************************************************
  Return the first item of governments.
**************************************************************************/
struct government *government_array_first(void)
{
  if (game.control.government_count > 0) {
    return governments;
  }
  return NULL;
}

/**************************************************************************
  Return the last item of governments.
**************************************************************************/
const struct government *government_array_last(void)
{
  if (game.control.government_count > 0) {
    return &governments[game.control.government_count - 1];
  }
  return NULL;
}

/***************************************************************
 Allocate space for the given number of governments.
***************************************************************/
void governments_alloc(int num)
{
  int index;

  governments = fc_calloc(num, sizeof(*governments));
  game.control.government_count = num;

  for (index = 0; index < num; index++) {
    struct government *gov = &governments[index];

    gov->item_number = index;
    requirement_vector_init(&gov->reqs);
  }
}

/***************************************************************
 De-allocate resources associated with the given government.
***************************************************************/
static void government_free(struct government *gov)
{
  free(gov->ruler_titles);
  gov->ruler_titles = NULL;

  free(gov->helptext);
  gov->helptext = NULL;

  requirement_vector_free(&gov->reqs);
}

/***************************************************************
 De-allocate the currently allocated governments.
***************************************************************/
void governments_free(void)
{
  government_iterate(gov) {
    government_free(gov);
  } government_iterate_end;
  free(governments);
  governments = NULL;
  game.control.government_count = 0;
}
