/********************************************************************** 
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__SETTINGS_H
#define FC__SETTINGS_H

#include "shared.h"

#include "game.h"

/* Whether settings are sent to the client when the client lists
 * server options; also determines whether clients can set them in principle.
 * Eg, not sent: seeds, saveturns, etc.
 */
#define SSET_TO_CLIENT TRUE
#define SSET_SERVER_ONLY FALSE

/* Categories allow options to be usefully organized when presented to the user
 */
enum sset_category {
  SSET_GEOLOGY,
  SSET_SOCIOLOGY,
  SSET_ECONOMICS,
  SSET_MILITARY,
  SSET_SCIENCE,
  SSET_INTERNAL,
  SSET_NETWORK,
  SSET_NUM_CATEGORIES
};

extern const char *sset_category_names[];

/* Levels allow options to be subdivided and thus easier to navigate */
enum sset_level {
  SSET_NONE,
  SSET_ALL,
  SSET_VITAL,
  SSET_SITUATIONAL,
  SSET_RARE,
  SSET_CHANGED
};

extern const char *sset_level_names[];
extern const int OLEVELS_NUM;

typedef bool (*bool_validate_func_t)(bool value, struct connection *pconn,
                                     const char **reject_message);
typedef bool (*int_validate_func_t)(int value, struct connection *pconn,
                                    const char **reject_message);
typedef bool (*string_validate_func_t)(const char * value,
                                       struct connection *pconn,
                                       const char **reject_message);

struct settings_s {
  const char *name;
  enum sset_class sclass;
  bool to_client;

  /*
   * Sould be less than 42 chars (?), or shorter if the values may
   * have more than about 4 digits.  Don't put "." on the end.
   */
  const char *short_help;

  /*
   * May be empty string, if short_help is sufficient.  Need not
   * include embedded newlines (but may, for formatting); lines will
   * be wrapped (and indented) automatically.  Should have punctuation
   * etc, and should end with a "."
   */
  const char *extra_help;
  enum sset_type stype;
  enum sset_category scategory;
  enum sset_level slevel;

  /* 
   * About the *_validate functions: If the function is non-NULL, it
   * is called with the new value, and returns whether the change is
   * legal. The char * is an error message in the case of reject. 
   */

  /*** bool part ***/
  bool *bool_value;
  bool bool_default_value;
  bool_validate_func_t bool_validate;

  /*** int part ***/
  int *int_value;
  int int_default_value;
  int_validate_func_t int_validate;
  int int_min_value, int_max_value;

  /*** string part ***/
  char *string_value;
  const char *string_default_value;
  string_validate_func_t string_validate;
  size_t string_value_size;	/* max size we can write into string_value */
};

extern struct settings_s settings[];
extern const int SETTINGS_NUM;

bool setting_is_changeable(int setting_id);

void settings_init(void);
void settings_reset(void);
void settings_turn(void);
void settings_free(void);

#endif				/* FC__SETTINGS_H */
