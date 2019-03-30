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

#include <stdlib.h>             /* exit */
#include <string.h>
#include <math.h>

/* utility */
#include "fcintl.h"
#include "iterator.h"
#include "log.h"
#include "mem.h"                /* free */
#include "shared.h"             /* ARRAY_SIZE */
#include "string_vector.h"
#include "support.h"

/* common */
#include "game.h"
#include "research.h"

#include "tech.h"


struct advance_req_iter {
  struct iterator base;
  bv_techs done;
  const struct advance *array[A_LAST];
  const struct advance **current, **end;
};
#define ADVANCE_REQ_ITER(it) ((struct advance_req_iter *) it)

struct advance_root_req_iter {
  struct iterator base;
  bv_techs done, rootdone;
  const struct advance *array[A_LAST];
  const struct advance **current, **end;
};
#define ADVANCE_ROOT_REQ_ITER(it) ((struct advance_root_req_iter *) it)

/* the advances array is now setup in:
 * server/ruleset.c (for the server)
 * client/packhand.c (for the client)
 */
struct advance advances[A_ARRAY_SIZE];

struct tech_class tech_classes[MAX_NUM_TECH_CLASSES];

static struct user_flag user_tech_flags[MAX_NUM_USER_TECH_FLAGS];

/**********************************************************************//**
  Return the last item of advances/technologies.
**************************************************************************/
const struct advance *advance_array_last(void)
{
  if (game.control.num_tech_types > 0) {
    return &advances[game.control.num_tech_types - 1];
  }
  return NULL;
}

/**********************************************************************//**
  Return the number of advances/technologies.
**************************************************************************/
Tech_type_id advance_count(void)
{
  return game.control.num_tech_types;
}

/**********************************************************************//**
  Return the advance index.

  Currently same as advance_number(), paired with advance_count()
  indicates use as an array index.
**************************************************************************/
Tech_type_id advance_index(const struct advance *padvance)
{
  fc_assert_ret_val(NULL != padvance, -1);
  return padvance - advances;
}

/**********************************************************************//**
  Return the advance index.
**************************************************************************/
Tech_type_id advance_number(const struct advance *padvance)
{
  fc_assert_ret_val(NULL != padvance, -1);
  return padvance->item_number;
}

/**********************************************************************//**
  Return the advance for the given advance index.
**************************************************************************/
struct advance *advance_by_number(const Tech_type_id atype)
{
  if (atype != A_FUTURE
      && (atype < 0 || atype >= game.control.num_tech_types)) {
    /* This isn't an error; some callers depend on it. */
    return NULL;
  }

  return &advances[atype];
}

/**********************************************************************//**
  Accessor for requirements.
**************************************************************************/
Tech_type_id advance_required(const Tech_type_id tech,
			      enum tech_req require)
{
  fc_assert_ret_val(require >= 0 && require < AR_SIZE, -1);
  fc_assert_ret_val(tech >= A_NONE && tech < A_LAST, -1);
  if (A_NEVER == advances[tech].require[require]) {
    /* out of range */
    return A_LAST;
  }
  return advance_number(advances[tech].require[require]);
}

/**********************************************************************//**
  Accessor for requirements.
**************************************************************************/
struct advance *advance_requires(const struct advance *padvance,
				 enum tech_req require)
{
  fc_assert_ret_val(require >= 0 && require < AR_SIZE, NULL);
  fc_assert_ret_val(NULL != padvance, NULL);
  return padvance->require[require];
}

/**********************************************************************//**
  Returns pointer when the advance "exists" in this game, returns NULL
  otherwise.

  A tech doesn't exist if it has been flagged as removed by setting its
  require values to A_NEVER. Note that this function returns NULL if either
  of req values is A_NEVER, rather than both, to be on the safe side.
**************************************************************************/
struct advance *valid_advance(struct advance *padvance)
{
  if (NULL == padvance
      || A_NEVER == padvance->require[AR_ONE]
      || A_NEVER == padvance->require[AR_TWO]) {
    return NULL;
  }

  return padvance;
}

/**********************************************************************//**
  Returns pointer when the advance "exists" in this game,
  returns NULL otherwise.

  In addition to valid_advance(), tests for id is out of range.
**************************************************************************/
struct advance *valid_advance_by_number(const Tech_type_id id)
{
  return valid_advance(advance_by_number(id));
}

/**********************************************************************//**
  Does a linear search of advances[].name.translated
  Returns NULL when none match.
**************************************************************************/
struct advance *advance_by_translated_name(const char *name)
{
  advance_iterate(A_NONE, padvance) {
    if (0 == strcmp(advance_name_translation(padvance), name)) {
      return padvance;
    }
  } advance_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Does a linear search of advances[].name.vernacular
  Returns NULL when none match.
**************************************************************************/
struct advance *advance_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  advance_iterate(A_NONE, padvance) {
    if (0 == fc_strcasecmp(advance_rule_name(padvance), qname)) {
      return padvance;
    }
  } advance_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Return TRUE if the tech has this flag otherwise FALSE
**************************************************************************/
bool advance_has_flag(Tech_type_id tech, enum tech_flag_id flag)
{
  fc_assert_ret_val(tech_flag_id_is_valid(flag), FALSE);
  return BV_ISSET(advance_by_number(tech)->flags, flag);
}

/**********************************************************************//**
  Function to precalculate needed data for technologies.
**************************************************************************/
void techs_precalc_data(void)
{
  fc_assert_msg(tech_cost_style_is_valid(game.info.tech_cost_style),
                "Invalid tech_cost_style %d", game.info.tech_cost_style);

  advance_iterate(A_FIRST, padvance) {
    int num_reqs = 0;
    bool min_req = TRUE;

    advance_req_iterate(padvance, preq) {
      (void) preq; /* Compiler wants us to do something with 'preq'. */
      num_reqs++;
    } advance_req_iterate_end;
    padvance->num_reqs = num_reqs;

    switch (game.info.tech_cost_style) {
    case TECH_COST_CIV1CIV2:
    case TECH_COST_LINEAR:
      padvance->cost = game.info.base_tech_cost * num_reqs;
      break;
    case TECH_COST_CLASSIC_PRESET:
      if (-1 != padvance->cost) {
        min_req = FALSE;
        break;
      }
      /* No break. */
    case TECH_COST_CLASSIC:
      padvance->cost = game.info.base_tech_cost * (1.0 + num_reqs)
                       * sqrt(1.0 + num_reqs) / 2;
      break;
    case TECH_COST_EXPERIMENTAL_PRESET:
      if (-1 != padvance->cost) {
        min_req = FALSE;
        break;
      }
      /* No break. */
    case TECH_COST_EXPERIMENTAL:
      padvance->cost = game.info.base_tech_cost * ((num_reqs) * (num_reqs)
                           / (1 + sqrt(sqrt(num_reqs + 1))) - 0.5);
      break;
    }

    if (min_req && padvance->cost < game.info.base_tech_cost) {
      padvance->cost = game.info.base_tech_cost;
    }

    /* Class cost */
    if (padvance->tclass != NULL) {
      padvance->cost = padvance->cost * padvance->tclass->cost_pct / 100;
    }
  } advance_iterate_end;
}

/**********************************************************************//**
  Is the given tech a future tech.
**************************************************************************/
bool is_future_tech(Tech_type_id tech)
{
  return tech == A_FUTURE;
}

/**********************************************************************//**
  Return the (translated) name of the given advance/technology.
  You don't have to free the return pointer.
**************************************************************************/
const char *advance_name_translation(const struct advance *padvance)
{
  return name_translation_get(&padvance->name);
}

/**********************************************************************//**
  Return the (untranslated) rule name of the advance/technology.
  You don't have to free the return pointer.
**************************************************************************/
const char *advance_rule_name(const struct advance *padvance)
{
  return rule_name_get(&padvance->name);
}

/**********************************************************************//**
  Initialize tech classes
**************************************************************************/
void tech_classes_init(void)
{
  int i;

  for (i = 0; i < MAX_NUM_TECH_CLASSES; i++) {
    tech_classes[i].idx = i;
    tech_classes[i].ruledit_disabled = FALSE;
  }
}

/**********************************************************************//**
  Return the tech_class for the given index.
**************************************************************************/
struct tech_class *tech_class_by_number(const int idx)
{
  if (idx < 0 || idx >= game.control.num_tech_classes) {
    return NULL;
  }

  return &tech_classes[idx];
}

/**********************************************************************//**
  Return the (translated) name of the given tech_class
  You must not free the return pointer.
**************************************************************************/
const char *tech_class_name_translation(const struct tech_class *ptclass)
{
  return name_translation_get(&ptclass->name);
}

/**********************************************************************//**
  Return the (untranslated) rule name of tech_class
  You must not free the return pointer.
**************************************************************************/
const char *tech_class_rule_name(const struct tech_class *ptclass)
{
  return rule_name_get(&ptclass->name);
}

/**********************************************************************//**
  Does a linear search of tech_classes[].name.vernacular
  Returns NULL when none match.
**************************************************************************/
struct tech_class *tech_class_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);
  int i;

  for (i = 0; i < game.control.num_tech_classes; i++) {
    struct tech_class *ptclass = tech_class_by_number(i);

    if (!fc_strcasecmp(tech_class_rule_name(ptclass), qname)) {
      return ptclass;
    }
  }

  return NULL;
}

/**********************************************************************//**
  Initialize user tech flags.
**************************************************************************/
void user_tech_flags_init(void)
{
  int i;

  for (i = 0; i < MAX_NUM_USER_TECH_FLAGS; i++) {
    user_flag_init(&user_tech_flags[i]);
  }
}

/**********************************************************************//**
  Frees the memory associated with all user tech flags
**************************************************************************/
void user_tech_flags_free(void)
{
  int i;

  for (i = 0; i < MAX_NUM_USER_TECH_FLAGS; i++) {
    user_flag_free(&user_tech_flags[i]);
  }
}

/**********************************************************************//**
  Sets user defined name for tech flag.
**************************************************************************/
void set_user_tech_flag_name(enum tech_flag_id id, const char *name,
                             const char *helptxt)
{
  int tfid = id - TECH_USER_1;

  fc_assert_ret(id >= TECH_USER_1 && id <= TECH_USER_LAST);

  if (user_tech_flags[tfid].name != NULL) {
    FC_FREE(user_tech_flags[tfid].name);
    user_tech_flags[tfid].name = NULL;
  }

  if (name && name[0] != '\0') {
    user_tech_flags[tfid].name = fc_strdup(name);
  }

  if (user_tech_flags[tfid].helptxt != NULL) {
    FC_FREE(user_tech_flags[tfid].helptxt);
    user_tech_flags[tfid].helptxt = NULL;
  }

  if (helptxt && helptxt[0] != '\0') {
    user_tech_flags[tfid].helptxt = fc_strdup(helptxt);
  }
}

/**********************************************************************//**
  Tech flag name callback, called from specenum code.
**************************************************************************/
const char *tech_flag_id_name_cb(enum tech_flag_id flag)
{
  if (flag < TECH_USER_1 || flag > TECH_USER_LAST) {
    return NULL;
  }

  return user_tech_flags[flag-TECH_USER_1].name;
}

/**********************************************************************//**
  Return the (untranslated) helptxt of the user tech flag.
**************************************************************************/
const char *tech_flag_helptxt(enum tech_flag_id id)
{
  fc_assert(id >= TECH_USER_1 && id <= TECH_USER_LAST);

  return user_tech_flags[id - TECH_USER_1].helptxt;
}

/**********************************************************************//**
 Returns true if the costs for the given technology will stay constant
 during the game. False otherwise.

 Checking every tech_cost_style with fixed costs seems a waste of system
 resources, when we can check that it is not the one style without fixed
 costs.
**************************************************************************/
bool techs_have_fixed_costs()
{
  return (game.info.tech_leakage == TECH_LEAKAGE_NONE
          && game.info.tech_cost_style != TECH_COST_CIV1CIV2);
}

/**********************************************************************//**
  Initialize tech structures.
**************************************************************************/
void techs_init(void)
{
  struct advance *a_none = &advances[A_NONE];
  struct advance *a_future = &advances[A_FUTURE];
  int i;

  memset(advances, 0, sizeof(advances));
  for (i = 0; i < ARRAY_SIZE(advances); i++) {
    advances[i].item_number = i;
    advances[i].cost = -1;
    advances[i].inherited_root_req = FALSE;
    advances[i].tclass = 0;

    requirement_vector_init(&(advances[i].research_reqs));
  }

  /* Initialize dummy tech A_NONE */
  /* TRANS: "None" tech */
  name_set(&a_none->name, NULL, N_("?tech:None"));
  a_none->require[AR_ONE] = a_none;
  a_none->require[AR_TWO] = a_none;
  a_none->require[AR_ROOT] = A_NEVER;

  name_set(&a_future->name, NULL, "Future");
  a_future->require[AR_ONE] = A_NEVER;
  a_future->require[AR_TWO] = A_NEVER;
  a_future->require[AR_ROOT] = A_NEVER;
}

/**********************************************************************//**
  De-allocate resources associated with the given tech.
**************************************************************************/
static void tech_free(Tech_type_id tech)
{
  struct advance *p = &advances[tech];

  if (NULL != p->helptext) {
    strvec_destroy(p->helptext);
    p->helptext = NULL;
  }

  if (p->bonus_message) {
    free(p->bonus_message);
    p->bonus_message = NULL;
  }
}

/**********************************************************************//**
  De-allocate resources of all techs.
**************************************************************************/
void techs_free(void)
{
  int i;

  advance_index_iterate(A_FIRST, adv_idx) {
    tech_free(adv_idx);
  } advance_index_iterate_end;

  for (i = 0; i < ARRAY_SIZE(advances); i++) {
    requirement_vector_free(&(advances[i].research_reqs));
  }
}

/**********************************************************************//**
  Return the size of the advance requirements iterator.
**************************************************************************/
size_t advance_req_iter_sizeof(void)
{
  return sizeof(struct advance_req_iter);
}

/**********************************************************************//**
  Return the current advance.
**************************************************************************/
static void *advance_req_iter_get(const struct iterator *it)
{
  return (void *) *ADVANCE_REQ_ITER(it)->current;
}

/**********************************************************************//**
  Jump to next advance requirement.
**************************************************************************/
static void advance_req_iter_next(struct iterator *it)
{
  struct advance_req_iter *iter = ADVANCE_REQ_ITER(it);
  const struct advance *padvance = *iter->current, *preq;
  enum tech_req req;
  bool new = FALSE;

  for (req = AR_ONE; req < AR_SIZE; req++) {
    preq = valid_advance(advance_requires(padvance, req));
    if (NULL != preq
        && A_NONE != advance_number(preq)
        && !BV_ISSET(iter->done, advance_number(preq))) {
      BV_SET(iter->done, advance_number(preq));
      if (new) {
        *iter->end++ = preq;
      } else {
        *iter->current = preq;
        new = TRUE;
      }
    }
  }

  if (!new) {
    iter->current++;
  }
}

/**********************************************************************//**
  Return whether we finished to iterate or not.
**************************************************************************/
static bool advance_req_iter_valid(const struct iterator *it)
{
  const struct advance_req_iter *iter = ADVANCE_REQ_ITER(it);

  return iter->current < iter->end;
}

/**********************************************************************//**
  Initialize an advance requirements iterator.
**************************************************************************/
struct iterator *advance_req_iter_init(struct advance_req_iter *it,
                                       const struct advance *goal)
{
  struct iterator *base = ITERATOR(it);

  base->get = advance_req_iter_get;
  base->next = advance_req_iter_next;
  base->valid = advance_req_iter_valid;

  BV_CLR_ALL(it->done);
  it->current = it->array;
  *it->current = goal;
  it->end = it->current + 1;

  return base;
}

/************************************************************************//**
  Return the size of the advance root requirements iterator.
****************************************************************************/
size_t advance_root_req_iter_sizeof(void)
{
  return sizeof(struct advance_root_req_iter);
}

/************************************************************************//**
  Return the current root_req.
****************************************************************************/
static void *advance_root_req_iter_get(const struct iterator *it)
{
  return
    (void *) advance_requires(*ADVANCE_ROOT_REQ_ITER(it)->current, AR_ROOT);
}

/************************************************************************//**
  Return whether we finished to iterate or not.
****************************************************************************/
static bool advance_root_req_iter_valid(const struct iterator *it)
{
  const struct advance_root_req_iter *iter = ADVANCE_ROOT_REQ_ITER(it);

  return iter->current < iter->end;
}

/************************************************************************//**
  Jump to next advance which has a previously unseen root_req.
****************************************************************************/
static void advance_root_req_iter_next(struct iterator *it)
{
  struct advance_root_req_iter *iter = ADVANCE_ROOT_REQ_ITER(it);

  /* Precondition: either iteration has already finished, or iter->current
   * points at a tech with an interesting root_req (which means its
   * requirements may have more). */
  while (advance_root_req_iter_valid(it)) {
    const struct advance *padvance = *iter->current;
    enum tech_req req;
    bool new = FALSE;

    for (req = AR_ONE; req < AR_SIZE; req++) {
      const struct advance *preq
        = valid_advance(advance_requires(padvance, req));

      if (NULL != preq
          && A_NONE != advance_number(preq)
          && !BV_ISSET(iter->done, advance_number(preq))) {

        BV_SET(iter->done, advance_number(preq));
        /* Do we need to look at this subtree at all? If it has A_NONE as
         * root_req, further root_reqs can't propagate through it, so no. */
        if (advance_required(advance_number(preq), AR_ROOT) != A_NONE) {
          /* Yes, this subtree needs iterating over at some point, starting
           * with preq (whose own root_req we'll consider in a bit) */
          if (!new) {
            *iter->current = preq;
            new = TRUE;
          } else {
            *iter->end++ = preq; /* make a note for later */
          }
        }
      }
    }
    if (!new) {
      /* Didn't find an interesting new subtree. */
      iter->current++;
    }
    /* Precondition: *current has been moved on from where we started, and
     * it has an interesting root_req or it wouldn't be on the list; but
     * it may be one that we've already yielded. */
    if (advance_root_req_iter_valid(it)) {
      Tech_type_id root = advance_required(advance_number(*iter->current),
                                           AR_ROOT);
      if (!BV_ISSET(iter->rootdone, root)) {
        /* A previously unseen root_req. Stop and yield it. */
        break;
      } /* else keep looking */
    }
  }
}

/************************************************************************//**
  Initialize a root requirements iterator.
****************************************************************************/
struct iterator *advance_root_req_iter_init(struct advance_root_req_iter *it,
                                            const struct advance *goal)
{
  struct iterator *base = ITERATOR(it);

  base->get = advance_root_req_iter_get;
  base->next = advance_root_req_iter_next;
  base->valid = advance_root_req_iter_valid;

  BV_CLR_ALL(it->done);
  BV_CLR_ALL(it->rootdone);
  it->current = it->array;
  if (advance_required(advance_number(goal), AR_ROOT) != A_NONE) {
    /* First root_req to return is goal's own, and there may be more
     * for next() to find. */
    *it->current = goal;
    it->end = it->current + 1;
  } else {
    /* No root_reqs -- go straight to invalid state */
    it->end = it->current;
  }

  return base;
}
