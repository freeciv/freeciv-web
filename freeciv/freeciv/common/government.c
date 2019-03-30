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
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "string_vector.h"
#include "support.h"

/* common */
#include "game.h"
#include "player.h"
#include "tech.h"

#include "government.h"

struct government *governments = NULL;

/**********************************************************************//**
  Returns the government that has the given (translated) name.
  Returns NULL if none match.
**************************************************************************/
struct government *government_by_translated_name(const char *name)
{
  governments_iterate(gov) {
    if (0 == strcmp(government_name_translation(gov), name)) {
      return gov;
    }
  } governments_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Returns the government that has the given (untranslated) rule name.
  Returns NULL if none match.
**************************************************************************/
struct government *government_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  governments_iterate(gov) {
    if (0 == fc_strcasecmp(government_rule_name(gov), qname)) {
      return gov;
    }
  } governments_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Return the number of governments.
**************************************************************************/
Government_type_id government_count(void)
{
  return game.control.government_count;
}

/**********************************************************************//**
  Return the government index.

  Currently same as government_number(), paired with government_count()
  indicates use as an array index.
**************************************************************************/
Government_type_id government_index(const struct government *pgovern)
{
  fc_assert_ret_val(NULL != pgovern, -1);
  return pgovern - governments;
}

/**********************************************************************//**
  Return the government index.
**************************************************************************/
Government_type_id government_number(const struct government *pgovern)
{
  fc_assert_ret_val(NULL != pgovern, -1);
  return pgovern->item_number;
}

/**********************************************************************//**
  Return the government with the given index.

  This function returns NULL for an out-of-range index (some callers
  rely on this).
**************************************************************************/
struct government *government_by_number(const Government_type_id gov)
{
  if (gov < 0 || gov >= game.control.government_count) {
    return NULL;
  }
  return &governments[gov];
}

/**********************************************************************//**
  Return the government of a player.
**************************************************************************/
struct government *government_of_player(const struct player *pplayer)
{
  fc_assert_ret_val(NULL != pplayer, NULL);
  return pplayer->government;
}

/**********************************************************************//**
  Return the government of the player who owns the city.
**************************************************************************/
struct government *government_of_city(const struct city *pcity)
{
  fc_assert_ret_val(NULL != pcity, NULL);
  return government_of_player(city_owner(pcity));
}

/**********************************************************************//**
  Return the (untranslated) rule name of the government.
  You don't have to free the return pointer.
**************************************************************************/
const char *government_rule_name(const struct government *pgovern)
{
  fc_assert_ret_val(NULL != pgovern, NULL);
  return rule_name_get(&pgovern->name);
}

/**********************************************************************//**
  Return the (translated) name of the given government. 
  You don't have to free the return pointer.
**************************************************************************/
const char *government_name_translation(const struct government *pgovern)
{
  fc_assert_ret_val(NULL != pgovern, NULL);

  return name_translation_get(&pgovern->name);
}

/**********************************************************************//**
  Return the (translated) name of the given government of a player. 
  You don't have to free the return pointer.
**************************************************************************/
const char *government_name_for_player(const struct player *pplayer)
{
  return government_name_translation(government_of_player(pplayer));
}

/**********************************************************************//**
  Can change to government if appropriate tech exists, and one of:
   - no required tech (required is A_NONE)
   - player has required tech
   - we have an appropriate wonder
  Returns FALSE if pplayer is NULL (used for observers).
**************************************************************************/
bool can_change_to_government(struct player *pplayer,
                              const struct government *gov)
{
  fc_assert_ret_val(NULL != gov, FALSE);

  if (!pplayer) {
    return FALSE;
  }

  if (get_player_bonus(pplayer, EFT_ANY_GOVERNMENT) > 0) {
    /* Note, this may allow govs that are on someone else's "tech tree". */
    return TRUE;
  }

  return are_reqs_active(pplayer, NULL, NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, &gov->reqs, RPT_CERTAIN);
}


/**************************************************************************
  Ruler titles.
**************************************************************************/
struct ruler_title {
  const struct nation_type *pnation;
  struct name_translation male;
  struct name_translation female;
};

/**********************************************************************//**
  Hash function.
**************************************************************************/
static genhash_val_t nation_hash_val(const struct nation_type *pnation)
{
  return NULL != pnation ? nation_number(pnation) : nation_count();
}

/**********************************************************************//**
  Hash function.
**************************************************************************/
static bool nation_hash_comp(const struct nation_type *pnation1,
                             const struct nation_type *pnation2)
{
  return pnation1 == pnation2;
}

/**********************************************************************//**
  Create a new ruler title.
**************************************************************************/
static struct ruler_title *ruler_title_new(const struct nation_type *pnation,
                                           const char *domain,
                                           const char *ruler_male_title,
                                           const char *ruler_female_title)
{
  struct ruler_title *pruler_title = fc_malloc(sizeof(*pruler_title));

  pruler_title->pnation = pnation;
  name_set(&pruler_title->male, domain, ruler_male_title);
  name_set(&pruler_title->female, domain, ruler_female_title);

  return pruler_title;
}

/**********************************************************************//**
  Free a ruler title.
**************************************************************************/
static void ruler_title_destroy(struct ruler_title *pruler_title)
{
  free(pruler_title);
}

/**********************************************************************//**
  Return TRUE if the ruler title is valid.
**************************************************************************/
static bool ruler_title_check(const struct ruler_title *pruler_title)
{
  bool ret = TRUE;

  if (!formats_match(rule_name_get(&pruler_title->male), "%s")) {
    if (NULL != pruler_title->pnation) {
      log_error("\"%s\" male ruler title for nation \"%s\" (nb %d) "
                "is not a format. It should match \"%%s\"",
                rule_name_get(&pruler_title->male),
                nation_rule_name(pruler_title->pnation),
                nation_number(pruler_title->pnation));
    } else {
      log_error("\"%s\" male ruler title is not a format. "
                "It should match \"%%s\"",
                rule_name_get(&pruler_title->male));
    }
    ret = FALSE;
  }

  if (!formats_match(rule_name_get(&pruler_title->female), "%s")) {
    if (NULL != pruler_title->pnation) {
      log_error("\"%s\" female ruler title for nation \"%s\" (nb %d) "
                "is not a format. It should match \"%%s\"",
                rule_name_get(&pruler_title->female),
                nation_rule_name(pruler_title->pnation),
                nation_number(pruler_title->pnation));
    } else {
      log_error("\"%s\" female ruler title is not a format. "
                "It should match \"%%s\"",
                rule_name_get(&pruler_title->female));
    }
    ret = FALSE;
  }

  if (!formats_match(name_translation_get(&pruler_title->male), "%s")) {
    if (NULL != pruler_title->pnation) {
      log_error("Translation of \"%s\" male ruler title for nation \"%s\" "
                "(nb %d) is not a format (\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->male),
                nation_rule_name(pruler_title->pnation),
                nation_number(pruler_title->pnation),
                name_translation_get(&pruler_title->male));
    } else {
      log_error("Translation of \"%s\" male ruler title is not a format "
                "(\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->male),
                name_translation_get(&pruler_title->male));
    }
    ret = FALSE;
  }

  if (!formats_match(name_translation_get(&pruler_title->female), "%s")) {
    if (NULL != pruler_title->pnation) {
      log_error("Translation of \"%s\" female ruler title for nation \"%s\" "
                "(nb %d) is not a format (\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->female),
                nation_rule_name(pruler_title->pnation),
                nation_number(pruler_title->pnation),
                name_translation_get(&pruler_title->female));
    } else {
      log_error("Translation of \"%s\" female ruler title is not a format "
                "(\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->female),
                name_translation_get(&pruler_title->female));
    }
    ret = FALSE;
  }

  return ret;
}

/**********************************************************************//**
  Returns all ruler titles for a government type.
**************************************************************************/
const struct ruler_title_hash *
government_ruler_titles(const struct government *pgovern)
{
  fc_assert_ret_val(NULL != pgovern, NULL);
  return pgovern->ruler_titles;
}

/**********************************************************************//**
  Add a new ruler title for the nation. Pass NULL for pnation for defining
  the default title.
**************************************************************************/
struct ruler_title *
government_ruler_title_new(struct government *pgovern,
                           const struct nation_type *pnation,
                           const char *ruler_male_title,
                           const char *ruler_female_title)
{
  const char *domain = NULL;
  struct ruler_title *pruler_title;

  if (pnation != NULL) {
    domain = pnation->translation_domain;
  }
  pruler_title =
    ruler_title_new(pnation, domain, ruler_male_title, ruler_female_title);

  if (!ruler_title_check(pruler_title)) {
    ruler_title_destroy(pruler_title);
    return NULL;
  }

  if (ruler_title_hash_replace(pgovern->ruler_titles,
                               pnation, pruler_title)) {
    if (NULL != pnation) {
      log_error("Ruler title for government \"%s\" (nb %d) and "
                "nation \"%s\" (nb %d) was set twice.",
                government_rule_name(pgovern), government_number(pgovern),
                nation_rule_name(pnation), nation_number(pnation));
    } else {
      log_error("Default ruler title for government \"%s\" (nb %d) "
                "was set twice.", government_rule_name(pgovern),
                government_number(pgovern));
    }
  }

  return pruler_title;
}

/**********************************************************************//**
  Return the nation of the rule title. Returns NULL if this is default.
**************************************************************************/
const struct nation_type *
ruler_title_nation(const struct ruler_title *pruler_title)
{
  return pruler_title->pnation;
}

/**********************************************************************//**
  Return the male rule title name.
**************************************************************************/
const char *
ruler_title_male_untranslated_name(const struct ruler_title *pruler_title)
{
  return untranslated_name(&pruler_title->male);
}

/**********************************************************************//**
  Return the female rule title name.
**************************************************************************/
const char *
ruler_title_female_untranslated_name(const struct ruler_title *pruler_title)
{
  return untranslated_name(&pruler_title->female);
}

/**********************************************************************//**
  Return the ruler title of the player (translated).
**************************************************************************/
const char *ruler_title_for_player(const struct player *pplayer,
                                   char *buf, size_t buf_len)
{
  const struct government *pgovern = government_of_player(pplayer);
  const struct nation_type *pnation = nation_of_player(pplayer);
  struct ruler_title *pruler_title;

  fc_assert_ret_val(NULL != buf, NULL);
  fc_assert_ret_val(0 < buf_len, NULL);

  /* Try specific nation rule title. */
  if (!ruler_title_hash_lookup(pgovern->ruler_titles,
                               pnation, &pruler_title)
      /* Try default rule title. */
      && !ruler_title_hash_lookup(pgovern->ruler_titles,
                                  NULL, &pruler_title)) {
    log_error("Missing title for government \"%s\" (nb %d) "
              "nation \"%s\" (nb %d).",
              government_rule_name(pgovern), government_number(pgovern),
              nation_rule_name(pnation), nation_number(pnation));
    if (pplayer->is_male) {
      fc_snprintf(buf, buf_len, _("Mr. %s"), player_name(pplayer));
    } else {
      fc_snprintf(buf, buf_len, _("Ms. %s"), player_name(pplayer));
    }
  } else {
    fc_snprintf(buf, buf_len,
                name_translation_get(pplayer->is_male
                                     ? &pruler_title->male
                                     : &pruler_title->female),
                player_name(pplayer));
  }

  return buf;
}


/**************************************************************************
  Government iterator.
**************************************************************************/
struct government_iter {
  struct iterator vtable;
  struct government *p, *end;
};
#define GOVERNMENT_ITER(p) ((struct government_iter *) (p))

/**********************************************************************//**
  Implementation of iterator 'sizeof' function.
**************************************************************************/
size_t government_iter_sizeof(void)
{
  return sizeof(struct government_iter);
}

/**********************************************************************//**
  Implementation of iterator 'next' function.
**************************************************************************/
static void government_iter_next(struct iterator *iter)
{
  GOVERNMENT_ITER(iter)->p++;
}

/**********************************************************************//**
  Implementation of iterator 'get' function.
**************************************************************************/
static void *government_iter_get(const struct iterator *iter)
{
  return GOVERNMENT_ITER(iter)->p;
}

/**********************************************************************//**
  Implementation of iterator 'valid' function.
**************************************************************************/
static bool government_iter_valid(const struct iterator *iter)
{
  struct government_iter *it = GOVERNMENT_ITER(iter);
  return it->p < it->end;
}

/**********************************************************************//**
  Implementation of iterator 'init' function.
**************************************************************************/
struct iterator *government_iter_init(struct government_iter *it)
{
  it->vtable.next = government_iter_next;
  it->vtable.get = government_iter_get;
  it->vtable.valid = government_iter_valid;
  it->p = governments;
  it->end = governments + government_count();
  return ITERATOR(it);
}

/**********************************************************************//**
  Allocate resources associated with the given government.
**************************************************************************/
static inline void government_init(struct government *pgovern)
{
  memset(pgovern, 0, sizeof(*pgovern));

  pgovern->item_number = pgovern - governments;
  pgovern->ruler_titles =
      ruler_title_hash_new_full(nation_hash_val, nation_hash_comp,
                                NULL, NULL, NULL, ruler_title_destroy);
  requirement_vector_init(&pgovern->reqs);
  pgovern->changed_to_times = 0;
  pgovern->ruledit_disabled = FALSE;
}

/**********************************************************************//**
  De-allocate resources associated with the given government.
**************************************************************************/
static inline void government_free(struct government *pgovern)
{
  ruler_title_hash_destroy(pgovern->ruler_titles);
  pgovern->ruler_titles = NULL;

  if (NULL != pgovern->helptext) {
    strvec_destroy(pgovern->helptext);
    pgovern->helptext = NULL;
  }

  requirement_vector_free(&pgovern->reqs);
}

/**********************************************************************//**
  Allocate the governments.
**************************************************************************/
void governments_alloc(int num)
{
  int i;

  fc_assert(NULL == governments);
  governments = fc_malloc(sizeof(*governments) * num);
  game.control.government_count = num;

  for (i = 0; i < game.control.government_count; i++) {
    government_init(governments + i);
  }
}

/**********************************************************************//**
  De-allocate the currently allocated governments.
**************************************************************************/
void governments_free(void)
{
  int i;

  if (NULL == governments) {
    return;
  }

  for (i = 0; i < game.control.government_count; i++) {
    government_free(governments + i);
  }

  free(governments);
  governments = NULL;
  game.control.government_count = 0;
}

/**********************************************************************//**
  Is it possible to start a revolution without specifying the target
  government in the current game?
**************************************************************************/
bool untargeted_revolution_allowed(void)
{
  if (game.info.revolentype == REVOLEN_QUICKENING
      || game.info.revolentype == REVOLEN_RANDQUICK) {
    /* We need to know the target government at the onset of the revolution
     * in order to know how long anarchy will last. */
    return FALSE;
  }
  return TRUE;
}
