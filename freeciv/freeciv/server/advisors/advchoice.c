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
#include "support.h"

/* common */
#include "improvement.h"
#include "requirements.h"
#include "unittype.h"

#include "advchoice.h"

/**********************************************************************//**
  Sets the values of the choice to initial values.
**************************************************************************/
void adv_init_choice(struct adv_choice *choice)
{
  choice->value.utype = NULL;
  choice->want = 0;
  choice->type = CT_NONE;
  choice->need_boat = FALSE;
#ifdef ADV_CHOICE_TRACK
  choice->use = NULL;
#endif /* ADV_CHOICE_TRACK */
}

/**********************************************************************//**
  Clear choice without freeing it itself
**************************************************************************/
void adv_deinit_choice(struct adv_choice *choice)
{
#ifdef ADV_CHOICE_TRACK
  if (choice->use != NULL) {
    free(choice->use);
    choice->use = NULL;
  }
#endif /* ADV_CHOICE_TRACK */
}

/**********************************************************************//**
  Dynamically allocate a new choice.
**************************************************************************/
struct adv_choice *adv_new_choice(void)
{
  struct adv_choice *choice = fc_malloc(sizeof(*choice));

  adv_init_choice(choice);

  return choice;
}

/**********************************************************************//**
  Free dynamically allocated choice.
**************************************************************************/
void adv_free_choice(struct adv_choice *choice)
{
#ifdef ADV_CHOICE_TRACK
  if (choice->use != NULL) {
    free(choice->use);
  }
#endif /* ADV_CHOICE_TRACK */
  free(choice);
}

/**********************************************************************//**
  Return better one of the choices given. In case of a draw, first one
  is preferred.
**************************************************************************/
struct adv_choice *adv_better_choice(struct adv_choice *first,
                                     struct adv_choice *second)
{
  if (second->want > first->want) {
    return second;
  } else {
    return first;
  }
}

/**********************************************************************//**
  Return better one of the choices given, and free the other.
**************************************************************************/
struct adv_choice *adv_better_choice_free(struct adv_choice *first,
                                          struct adv_choice *second)
{
  if (second->want > first->want) {
    adv_free_choice(first);

    return second;
  } else {
    adv_free_choice(second);

    return first;
  }
}

/**********************************************************************//**
  Does choice type refer to unit
**************************************************************************/
bool is_unit_choice_type(enum choice_type type)
{
   return type == CT_CIVILIAN || type == CT_ATTACKER || type == CT_DEFENDER;
}

#ifdef ADV_CHOICE_TRACK
/**********************************************************************//**
  Copy contents of one choice structure to the other
**************************************************************************/
void adv_choice_copy(struct adv_choice *dest, struct adv_choice *src)
{
  if (dest != src) {
    dest->type = src->type;
    dest->value = src->value;
    dest->want = src->want;
    dest->need_boat = src->need_boat;
    if (dest->use != NULL) {
      free(dest->use);
    }
    if (src->use != NULL) {
      dest->use = fc_strdup(src->use);
    } else {
      dest->use = NULL;
    }
  }
}

/**********************************************************************//**
  Set the use the choice is meant for.
**************************************************************************/
void adv_choice_set_use(struct adv_choice *choice, const char *use)
{
  if (choice->use != NULL) {
    free(choice->use);
  }
  choice->use = fc_strdup(use);
}

/**********************************************************************//**
  Log the choice information.
**************************************************************************/
void adv_choice_log_info(struct adv_choice *choice, const char *loc1,
                         const char *loc2)
{
  const char *use;
  const char *name;

  if (choice->use != NULL) {
    use = choice->use;
  } else {
    use = "<unknown>";
  }

  if (choice->type == CT_BUILDING) {
    name = improvement_rule_name(choice->value.building);
  } else if (choice->type == CT_NONE) {
    name = "None";
  } else {
    name = utype_rule_name(choice->value.utype);
  }

  if (loc2 != NULL) {
    log_base(ADV_CHOICE_LOG_LEVEL, "Choice at \"%s:%s\": %s, "
                                   "want " ADV_WANT_PRINTF " as %s (%d)",
             loc1, loc2, name, choice->want, use, choice->type);
  } else {
    log_base(ADV_CHOICE_LOG_LEVEL, "Choice at \"%s\": %s, "
                                   "want " ADV_WANT_PRINTF " as %s (%d)",
             loc1, name, choice->want, use, choice->type);
  }
}

#endif /* ADV_CHOICE_TRACK */
