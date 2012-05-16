/********************************************************************** 
 Freeciv - Copyright (C) 2001 - R. Falke
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/**************************************************************************
 This is the common file for all front-end (Front End Common) for the
 citizen management agent (CMA).
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "mem.h"
#include "specialist.h"
#include "support.h"

#include "agents.h"
#include "attribute.h"

#include "cma_fec.h"

#define RESULT_COLUMNS		10
#define BUFFER_SIZE		100
#define MAX_LEN_PRESET_NAME	80

struct cma_preset {
  char *descr;
  struct cm_parameter parameter;
};

#define SPECLIST_TAG preset
#define SPECLIST_TYPE struct cma_preset
#include "speclist.h"

#define preset_list_iterate(presetlist, ppreset) \
    TYPED_LIST_ITERATE(struct cma_preset, presetlist, ppreset)
#define preset_list_iterate_end  LIST_ITERATE_END

static struct preset_list *preset_list = NULL;

/****************************************************************************
 Is called if the game removes a city. It will clear the 
 "fe parameter" attribute to reduce the size of the savegame.   
*****************************************************************************/
static void city_remove(int city_id)
{
  attr_city_set(ATTR_CITY_CMAFE_PARAMETER, city_id, 0, NULL);
}

/**************************************************************************
 Initialize the presets if there are no presets loaded on startup.
**************************************************************************/
void cmafec_init(void)
{
  struct agent self;

  if (preset_list == NULL) {
    preset_list = preset_list_new();
  }

  memset(&self, 0, sizeof(self));
  strcpy(self.name, "CMA");
  self.level = 1;
  self.city_callbacks[CB_REMOVE] = city_remove;
  register_agent(&self);
}

/**************************************************************************
 ...
**************************************************************************/
void cmafec_free(void)
{
  while (cmafec_preset_num() > 0) {
    cmafec_preset_remove(0);
  }
  preset_list_free(preset_list);
}

/**************************************************************************
 Sets the front-end parameter.
**************************************************************************/
void cmafec_set_fe_parameter(struct city *pcity,
			     const struct cm_parameter *const parameter)
{
  cma_set_parameter(ATTR_CITY_CMAFE_PARAMETER, pcity->id, parameter);
}

/****************************************************************
 Return the front-end parameter for the given city. Returns a dummy
 parameter if no parameter was set.
*****************************************************************/
void cmafec_get_fe_parameter(struct city *pcity, struct cm_parameter *dest)
{
  struct cm_parameter parameter;

  /* our fe_parameter could be stale. our agents parameter is uptodate */
  if (cma_is_city_under_agent(pcity, &parameter)) {
    cm_copy_parameter(dest, &parameter);
    cmafec_set_fe_parameter(pcity, dest);
  } else {
    /* Create a dummy parameter to return. */
    cm_init_parameter(dest);
    if (!cma_get_parameter(ATTR_CITY_CMAFE_PARAMETER, pcity->id, dest)) {
      /* We haven't seen this city before; store the dummy. */
      cmafec_set_fe_parameter(pcity, dest);
    }
  }
}

/**************************************************************************
 Adds a preset.
**************************************************************************/
void cmafec_preset_add(const char *descr_name, struct cm_parameter *pparam)
{
  struct cma_preset *ppreset = fc_malloc(sizeof(struct cma_preset));

  if (preset_list == NULL) {
    preset_list = preset_list_new();
  }

  cm_copy_parameter(&ppreset->parameter, pparam);
  ppreset->descr = fc_malloc(MAX_LEN_PRESET_NAME);
  (void) mystrlcpy(ppreset->descr, descr_name, MAX_LEN_PRESET_NAME);
  preset_list_prepend(preset_list, ppreset);
}

/**************************************************************************
 Removes a preset.
**************************************************************************/
void cmafec_preset_remove(int index)
{
  struct cma_preset *ppreset;

  assert(index >= 0 && index < cmafec_preset_num());

  ppreset = preset_list_get(preset_list, index);
  preset_list_unlink(preset_list, ppreset);

  free(ppreset->descr);
  free(ppreset);
}

/**************************************************************************
 Returns the indexed preset's description.
**************************************************************************/
char *cmafec_preset_get_descr(int index)
{
  struct cma_preset *ppreset;

  assert(index >= 0 && index < cmafec_preset_num());

  ppreset = preset_list_get(preset_list, index);
  return ppreset->descr;
}

/**************************************************************************
 Returns the indexed preset's parameter.
**************************************************************************/
const struct cm_parameter *cmafec_preset_get_parameter(int index)
{
  struct cma_preset *ppreset;

  assert(index >= 0 && index < cmafec_preset_num());

  ppreset = preset_list_get(preset_list, index);
  return &ppreset->parameter;
}

/**************************************************************************
 Returns the index of the preset which matches the given
 parameter. Returns -1 if no preset could be found.
**************************************************************************/
int cmafec_preset_get_index_of_parameter(const struct cm_parameter
					 *const parameter)
{
  int i;

  for (i = 0; i < preset_list_size(preset_list); i++) {
    struct cma_preset *ppreset = preset_list_get(preset_list, i);
    if (cm_are_parameter_equal(&ppreset->parameter, parameter)) {
      return i;
    }
  }
  return -1;
}

/**************************************************************************
 Returns the total number of presets.
**************************************************************************/
int cmafec_preset_num(void)
{
  return preset_list_size(preset_list);
}

/**************************************************************************
...
**************************************************************************/
const char *cmafec_get_short_descr_of_city(const struct city *pcity)
{
  struct cm_parameter parameter;

  if (!cma_is_city_under_agent(pcity, &parameter)) {
    return _("none");
  } else {
    return cmafec_get_short_descr(&parameter);
  }
}

/**************************************************************************
 Returns the description of the matching preset or "custom" if no
 preset could be found.
**************************************************************************/
const char *cmafec_get_short_descr(const struct cm_parameter *const
				   parameter)
{
  int index = cmafec_preset_get_index_of_parameter(parameter);

  if (index == -1) {
    return _("custom");
  } else {
    return cmafec_preset_get_descr(index);
  }
}

/**************************************************************************
...
**************************************************************************/
static const char *get_city_growth_string(struct city *pcity, int surplus)
{
  int stock, cost, turns;
  static char buffer[50];

  if (surplus == 0) {
    my_snprintf(buffer, sizeof(buffer), _("never"));
    return buffer;
  }

  stock = pcity->food_stock;
  cost = city_granary_size(pcity->size);

  stock += surplus;

  if (stock >= cost) {
    turns = 1;
  } else if (surplus > 0) {
    turns = ((cost - stock - 1) / surplus) + 1 + 1;
  } else {
    if (stock < 0) {
      turns = -1;
    } else {
      turns = (stock / surplus);
    }
  }
  my_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns", turns),
	      turns);
  return buffer;
}

/**************************************************************************
...
**************************************************************************/
static const char *get_prod_complete_string(struct city *pcity, int surplus)
{
  int stock, cost, turns;
  static char buffer[50];

  if (surplus <= 0) {
    my_snprintf(buffer, sizeof(buffer), _("never"));
    return buffer;
  }

  if (city_production_has_flag(pcity, IF_GOLD)) {
    mystrlcpy(buffer,
	      improvement_name_translation(pcity->production.value.building),
	      sizeof(buffer));
    return buffer;
  }
  stock = pcity->shield_stock + surplus;
  cost = city_production_build_shield_cost(pcity);

  if (stock >= cost) {
    turns = 1;
  } else if (surplus > 0) {
    turns = ((cost - stock - 1) / surplus) + 1 + 1;
  } else {
    if (stock < 0) {
      turns = -1;
    } else {
      turns = (stock / surplus);
    }
  }
  my_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns", turns),
	      turns);
  return buffer;
}

/**************************************************************************
...
**************************************************************************/
const char *cmafec_get_result_descr(struct city *pcity,
				    const struct cm_result *const
				    result,
				    const struct cm_parameter *const
				    parameter)
{
  int j;
  char buf[RESULT_COLUMNS][BUFFER_SIZE];
  static char buffer[600];

  if (!result->found_a_valid) {
    for (j = 0; j < RESULT_COLUMNS; j++)
      my_snprintf(buf[j], BUFFER_SIZE, "---");
  } else {
    output_type_iterate(j) {
      my_snprintf(buf[j], BUFFER_SIZE, "%+3d", result->surplus[j]);
    } output_type_iterate_end;

    my_snprintf(buf[6], BUFFER_SIZE, "%d/%s%s",
		pcity->size - cm_result_specialists(result),
		specialists_string(result->specialists),
		result->happy ? _(" happy") : "");

    my_snprintf(buf[7], BUFFER_SIZE, "%s",
		get_city_growth_string(pcity, result->surplus[O_FOOD]));
    my_snprintf(buf[8], BUFFER_SIZE, "%s",
		get_prod_complete_string(pcity, result->surplus[O_SHIELD]));
    my_snprintf(buf[9], BUFFER_SIZE, "%s",
		cmafec_get_short_descr(parameter));
  }

  my_snprintf(buffer, sizeof(buffer),
	      _("Name: %s\n"
		"Food:       %10s Gold:    %10s\n"
		"Production: %10s Luxury:  %10s\n"
		"Trade:      %10s Science: %10s\n"
		"\n"
		"    People (W/E/S/T): %s\n"
		"          City grows: %s\n"
		"Production completed: %s"),
	      buf[9], buf[O_FOOD], buf[O_GOLD], buf[O_SHIELD], buf[O_LUXURY],
	      buf[O_TRADE], buf[O_SCIENCE], buf[6], buf[7], buf[8]);

  freelog(LOG_DEBUG, "\n%s", buffer);
  return buffer;
}


/**************************************************************************
  Create default cma presets for a new user (or without configuration file)
**************************************************************************/
void create_default_cma_presets(void)
{
 int i;
 struct cm_parameter parameters[] = {
   { /* very happy */
     .minimal_surplus = {0, 0, 0, -20, 0, 0},
     .require_happy = FALSE,
     .allow_disorder = FALSE,
     .allow_specialists = TRUE,
     .factor = {10, 5, 0, 4, 0, 4},
     .happy_factor = 25
   },
   { /* max food */
     .minimal_surplus = {-20, 0, 0, -20, 0, 0},
     .require_happy = FALSE,
     .allow_disorder = FALSE,
     .allow_specialists = TRUE,
     .factor = {25, 5, 0, 4, 0, 4},
     .happy_factor = 0
   },
   { /* max prod */
     .minimal_surplus = {0, -20, 0, -20, 0, 0},
     .require_happy = FALSE,
     .allow_disorder = FALSE,
     .allow_specialists = TRUE,
     .factor = {10, 25, 0, 4, 0, 4},
     .happy_factor = 0
   },
   { /* max gold */
     .minimal_surplus = {0, 0, 0, -20, 0, 0},
     .require_happy = FALSE,
     .allow_disorder = FALSE,
     .allow_specialists = TRUE,
     .factor = {10, 5, 0, 25, 0, 4},
     .happy_factor = 0
   },
   { /* max science */
     .minimal_surplus = {0, 0, 0, -20, 0, 0},
     .require_happy = FALSE,
     .allow_disorder = FALSE,
     .allow_specialists = TRUE,
     .factor = {10, 5, 0, 4, 0, 25},
     .happy_factor = 0
   }
 };
 const char* names[ARRAY_SIZE(parameters)] = {
   N_("?cma:Very happy"),
   N_("?cma:Max food"),
   N_("?cma:Max production"),
   N_("?cma:Max gold"),
   N_("?cma:Max science")
 };

 for (i = ARRAY_SIZE(parameters) - 1; i >= 0; i--) {
   cmafec_preset_add(Q_(names[i]), &parameters[i]);
 }
}
