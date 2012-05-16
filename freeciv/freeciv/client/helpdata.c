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

/****************************************************************
 This module is for generic handling of help data, independent
 of gui considerations.
*****************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "genlist.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "support.h"

/* common */
#include "city.h"
#include "effects.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "requirements.h"
#include "specialist.h"
#include "unit.h"

/* client */
#include "client_main.h"

#include "helpdata.h"

/* helper macro for easy conversion from snprintf and cat_snprintf */
#define CATLSTR(_b, _s, _t) mystrlcat(_b, _t, _s)

/* This must be in same order as enum in helpdlg_g.h */
static const char * const help_type_names[] = {
  "(Any)", "(Text)", "Units", "Improvements", "Wonders",
  "Techs", "Terrain", "Governments", "Ruleset", NULL
};

/*define MAX_LAST (MAX(MAX(MAX(A_LAST,B_LAST),U_LAST),terrain_count()))*/

#define SPECLIST_TAG help
#define SPECLIST_TYPE struct help_item
#include "speclist.h"

#define help_list_iterate(helplist, phelp) \
    TYPED_LIST_ITERATE(struct help_item, helplist, phelp)
#define help_list_iterate_end  LIST_ITERATE_END

static const struct genlist_link *help_nodes_iterator;
static struct help_list *help_nodes;
static bool help_nodes_init = FALSE;
/* helpnodes_init is not quite the same as booted in boot_help_texts();
   latter can be 0 even after call, eg if couldn't find helpdata.txt.
*/

/****************************************************************
  Initialize.
*****************************************************************/
void helpdata_init(void)
{
  help_nodes = help_list_new();
}

/****************************************************************
  Clean up.
*****************************************************************/
void helpdata_done(void)
{
  help_list_free(help_nodes);
}

/****************************************************************
  Make sure help_nodes is initialised.
  Should call this just about everywhere which uses help_nodes,
  to be careful...  or at least where called by external
  (non-static) functions.
*****************************************************************/
static void check_help_nodes_init(void)
{
  if (!help_nodes_init) {
    help_nodes_init = TRUE;    /* before help_iter_start to avoid recursion! */
    help_iter_start();
  }
}

/****************************************************************
  Free all allocations associated with help_nodes.
*****************************************************************/
void free_help_texts(void)
{
  check_help_nodes_init();
  help_list_iterate(help_nodes, ptmp) {
    free(ptmp->topic);
    free(ptmp->text);
    free(ptmp);
  } help_list_iterate_end;
  help_list_clear(help_nodes);
}

/****************************************************************************
  Insert generated data for the helpdate name.

  Currently only for terrain ("TerrainAlterations") is such a table created.
****************************************************************************/
static void insert_generated_table(char *outbuf, size_t outlen, const char *name)
{
  if (0 == strcmp (name, "TerrainAlterations")) {
    CATLSTR(outbuf, outlen,
            _("Terrain     Road   Irrigation     Mining         Transform\n"));
    CATLSTR(outbuf, outlen,
            "---------------------------------------------------------------\n");
    terrain_type_iterate(pterrain) {
      if (0 != strlen(terrain_rule_name(pterrain))) {
	cat_snprintf(outbuf, outlen,
		"%-10s %3d    %3d %-10s %3d %-10s %3d %-10s\n",
		terrain_name_translation(pterrain),
		pterrain->road_time,
		pterrain->irrigation_time,
		((pterrain->irrigation_result == pterrain
		  || pterrain->irrigation_result == T_NONE) ? ""
		 : terrain_name_translation(pterrain->irrigation_result)),
		pterrain->mining_time,
		((pterrain->mining_result == pterrain
		  || pterrain->mining_result == T_NONE) ? ""
		 : terrain_name_translation(pterrain->mining_result)),
		pterrain->transform_time,
		((pterrain->transform_result == pterrain
		 || pterrain->transform_result == T_NONE) ? ""
		 : terrain_name_translation(pterrain->transform_result)));
      }
    } terrain_type_iterate_end;
    CATLSTR(outbuf, outlen, "\n");
    CATLSTR(outbuf, outlen,
            _("(Railroads and fortresses require 3 turns, regardless of terrain.)"));
  }
  return;
}

/****************************************************************
  Append text for the requirement.  Something like

    "Requires the Communism technology.\n\n"

  pplayer may be NULL.
*****************************************************************/
static void insert_requirement(char *buf, size_t bufsz,
			      struct player *pplayer,
			      struct requirement *req)
{
  switch (req->source.kind) {
  case VUT_NONE:
    return;
  case VUT_ADVANCE:
    cat_snprintf(buf, bufsz, _("Requires the %s technology.\n\n"),
		 advance_name_for_player(pplayer,
					 advance_number(req->source.value.advance)));
    return;
  case VUT_GOVERNMENT:
    cat_snprintf(buf, bufsz, _("Requires the %s government.\n\n"),
		 government_name_translation(req->source.value.govern));
    return;
  case VUT_IMPROVEMENT:
    cat_snprintf(buf, bufsz, _("Requires the %s building.\n\n"),
		 improvement_name_translation(req->source.value.building));
    return;
  case VUT_SPECIAL:
    cat_snprintf(buf, bufsz, _("Requires the %s terrain special.\n\n"),
		 special_name_translation(req->source.value.special));
    return;
  case VUT_TERRAIN:
    cat_snprintf(buf, bufsz, _("Requires the %s terrain.\n\n"),
		 terrain_name_translation(req->source.value.terrain));
    return;
  case VUT_NATION:
    cat_snprintf(buf, bufsz, _("Requires the %s nation.\n\n"),
		 nation_adjective_translation(req->source.value.nation));
    return;
  case VUT_UTYPE:
    cat_snprintf(buf, bufsz, _("Only applies to %s units.\n\n"),
		 utype_name_translation(req->source.value.utype));
    return;
  case VUT_UTFLAG:
    cat_snprintf(buf, bufsz, _("Only applies to \"%s\" units.\n\n"),
                /* flag names are never translated */
                unit_flag_rule_name(req->source.value.unitflag));
    return;
  case VUT_UCLASS:
    cat_snprintf(buf, bufsz, _("Only applies to %s units.\n\n"),
		 uclass_name_translation(req->source.value.uclass));
    return;
  case VUT_UCFLAG:
    cat_snprintf(buf, bufsz, _("Only applies to \"%s\" units.\n\n"),
                /* flag names are never translated */
                unit_class_flag_rule_name(req->source.value.unitclassflag));
    return;
  case VUT_OTYPE:
    cat_snprintf(buf, bufsz, _("Applies only to %s.\n\n"),
		 get_output_name(req->source.value.outputtype));
    return;
  case VUT_SPECIALIST:
    cat_snprintf(buf, bufsz, _("Applies only to %s.\n\n"),
		 specialist_name_translation(req->source.value.specialist));
    return;
  case VUT_MINSIZE:
    cat_snprintf(buf, bufsz, _("Requires a minimum size of %d.\n\n"),
		 req->source.value.minsize);
    return;
  case VUT_AI_LEVEL:
    cat_snprintf(buf, bufsz, _("Requires AI player of level %s.\n\n"),
                 ai_level_name(req->source.value.ai_level));
    return;
  case VUT_TERRAINCLASS:
     cat_snprintf(buf, bufsz, _("Requires %s terrain.\n\n"),
                  terrain_class_name_translation(req->source.value.terrainclass));
     return;
  case VUT_TERRAINALTER:
     cat_snprintf(buf, bufsz, _("Requires terrain on which %s can be built.\n\n"),
                  terrain_alteration_name_translation(req->source.value.terrainalter));
     return;
  case VUT_CITYTILE:
     cat_snprintf(buf, bufsz, _("Applies only to city centers.\n\n"));
     return;
  case VUT_LAST:
  default:
    break;
  };
  assert(0);
}

/****************************************************************************
  Generate text for what this requirement source allows.  Something like

    "Allows Communism government (with University technology).\n\n"
    "Allows Mfg. Plant building (with Factory building).\n\n"

  This should be called to generate helptext for every possible source
  type.  Note this doesn't handle effects but rather production
  requirements (currently only building reqs).

  NB: This function overwrites any existing buffer contents by writing the
  generated text to the start of the given 'buf' pointer (i.e. it does
  NOT append like cat_snprintf).
****************************************************************************/
static void insert_allows(struct universal *psource,
			  char *buf, size_t bufsz)
{
  buf[0] = '\0';

  /* FIXME: show other data like range and survives. */

  improvement_iterate(pimprove) {
    requirement_vector_iterate(&pimprove->reqs, req) {
      if (are_universals_equal(psource, &req->source)) {
        char coreq_buf[512] = "";

        requirement_vector_iterate(&pimprove->reqs, coreq) {
          if (!are_universals_equal(psource, &coreq->source)) {
            char buf2[512] = "";

            universal_name_translation(&coreq->source, buf2,
                                       sizeof(buf2));
            if (coreq_buf[0] == '\0') {
              sz_strlcpy(coreq_buf, buf2);
            } else {
              cat_snprintf(coreq_buf, sizeof(coreq_buf),
                           Q_("?clistmore:, %s"), buf2);
            }
          }
        } requirement_vector_iterate_end;

        if (coreq_buf[0] == '\0') {
          cat_snprintf(buf, bufsz, _("Allows %s."),
                       improvement_name_translation(pimprove));
        } else {
          cat_snprintf(buf, bufsz, _("Allows %s (with %s)."),
                       improvement_name_translation(pimprove),
                       coreq_buf);
        }
        cat_snprintf(buf, bufsz, "\n");
      }
    } requirement_vector_iterate_end;
  } improvement_iterate_end;
}

/****************************************************************
...
*****************************************************************/
static struct help_item *new_help_item(int type)
{
  struct help_item *pitem;
  
  pitem = fc_malloc(sizeof(struct help_item));
  pitem->topic = NULL;
  pitem->text = NULL;
  pitem->type = type;
  return pitem;
}

/****************************************************************
 for help_list_sort(); sort by topic via compare_strings()
 (sort topics with more leading spaces after those with fewer)
*****************************************************************/
static int help_item_compar(const struct help_item *const *ppa,
                            const struct help_item *const *ppb)
{
  const struct help_item *ha, *hb;
  char *ta, *tb;
  ha = *ppa;
  hb = *ppb;
  for (ta = ha->topic, tb = hb->topic; *ta != '\0' && *tb != '\0'; ta++, tb++) {
    if (*ta != ' ') {
      if (*tb == ' ') return -1;
      break;
    } else if (*tb != ' ') {
      if (*ta == ' ') return 1;
      break;
    }
  }
  return compare_strings(ta, tb);
}

/****************************************************************
  pplayer may be NULL.
*****************************************************************/
void boot_help_texts(struct player *pplayer)
{
  static bool booted = FALSE;

  struct section_file file, *sf = &file;
  char *filename;
  struct help_item *pitem;
  int i, isec;
  char **sec, **paras;
  int nsec, npara;
  char long_buffer[64000]; /* HACK: this may be overrun. */

  check_help_nodes_init();

  /* need to do something like this or bad things happen */
  popdown_help_dialog();

  if(!booted) {
    freelog(LOG_VERBOSE, "Booting help texts");
  } else {
    /* free memory allocated last time booted */
    free_help_texts();
    freelog(LOG_VERBOSE, "Rebooting help texts");
  }    

  filename = datafilename("helpdata.txt");
  if (!filename) {
    freelog(LOG_ERROR, "Did not read help texts");
    return;
  }
  /* after following call filename may be clobbered; use sf->filename instead */
  if (!section_file_load(sf, filename)) {
    /* this is now unlikely to happen */
    freelog(LOG_ERROR, "failed reading help-texts");
    return;
  }

  sec = secfile_get_secnames_prefix(sf, "help_", &nsec);

  for(isec=0; isec<nsec; isec++) {
    const char *gen_str =
      secfile_lookup_str_default(sf, NULL, "%s.generate", sec[isec]);
    
    if (gen_str) {
      enum help_page_type current_type = HELP_ANY;
      if (!booted) {
	continue; /* on initial boot data tables are empty */
      }
      for(i=2; help_type_names[i]; i++) {
	if(strcmp(gen_str, help_type_names[i])==0) {
	  current_type = i;
	  break;
	}
      }
      if (current_type == HELP_ANY) {
	freelog(LOG_ERROR, "bad help-generate category \"%s\"", gen_str);
	continue;
      }
      {
	/* Note these should really fill in pitem->text from auto-gen
	   data instead of doing it later on the fly, but I don't want
	   to change that now.  --dwp
	*/
	char name[2048];
	struct help_list *category_nodes = help_list_new();

        switch (current_type) {
	 case HELP_UNIT:
           unit_type_iterate(punittype) {
             pitem = new_help_item(current_type);
             my_snprintf(name, sizeof(name), " %s",
                         utype_name_translation(punittype));
             pitem->topic = mystrdup(name);
             pitem->text = mystrdup("");
             help_list_append(category_nodes, pitem);
           } unit_type_iterate_end;
           break;
         case HELP_TECH:
           advance_index_iterate(A_FIRST, i) {
             if (valid_advance_by_number(i)) {
               pitem = new_help_item(current_type);
               my_snprintf(name, sizeof(name), " %s",
                           advance_name_for_player(pplayer, i));
               pitem->topic = mystrdup(name);
               pitem->text = mystrdup("");
               help_list_append(category_nodes, pitem);
             }
           } advance_index_iterate_end;
           break;
         case HELP_TERRAIN:
           terrain_type_iterate(pterrain) {
             if (0 != strlen(terrain_rule_name(pterrain))) {
               pitem = new_help_item(current_type);
               my_snprintf(name, sizeof(name), " %s",
                           terrain_name_translation(pterrain));
               pitem->topic = mystrdup(name);
               pitem->text = mystrdup("");
               help_list_append(category_nodes, pitem);
             }
           } terrain_type_iterate_end;
           /* Add special Civ2-style river help text if it's supplied. */
           if (terrain_control.river_help_text) {
             pitem = new_help_item(HELP_TEXT);
             /* TRANS: preserve single space at beginning */
             pitem->topic = mystrdup(_(" Rivers"));
             sz_strlcpy(long_buffer, _(terrain_control.river_help_text));
             pitem->text = mystrdup(long_buffer);
             help_list_append(category_nodes, pitem);
           }
           break;
         case HELP_GOVERNMENT:
           government_iterate(gov) {
             pitem = new_help_item(current_type);
             my_snprintf(name, sizeof(name), " %s",
                         government_name_translation(gov));
             pitem->topic = mystrdup(name);
             pitem->text = mystrdup("");
             help_list_append(category_nodes, pitem);
           } government_iterate_end;
           break;
         case HELP_IMPROVEMENT:
           improvement_iterate(pimprove) {
             if (valid_improvement(pimprove) && !is_great_wonder(pimprove)) {
               pitem = new_help_item(current_type);
               my_snprintf(name, sizeof(name), " %s",
                           improvement_name_translation(pimprove));
               pitem->topic = mystrdup(name);
               pitem->text = mystrdup("");
               help_list_append(category_nodes, pitem);
             }
           } improvement_iterate_end;
           break;
         case HELP_WONDER:
           improvement_iterate(pimprove) {
             if (valid_improvement(pimprove) && is_great_wonder(pimprove)) {
               pitem = new_help_item(current_type);
               my_snprintf(name, sizeof(name), " %s",
                           improvement_name_translation(pimprove));
               pitem->topic = mystrdup(name);
               pitem->text = mystrdup("");
               help_list_append(category_nodes, pitem);
             }
           } improvement_iterate_end;
           break;
         case HELP_RULESET:
           pitem = new_help_item(HELP_RULESET);
           /*           pitem->topic = mystrdup(game.control.name); */
           pitem->topic = mystrdup(HELP_RULESET_ITEM);
           if (game.control.description[0] != '\0') {
             pitem->text = mystrdup(game.control.description);
           } else {
             pitem->text = mystrdup(_("Current ruleset contains no description."));
           }
           help_list_append(help_nodes, pitem);
           break;
         default:
           die("Bad current_type %d", current_type);
           break;
        }
	help_list_sort(category_nodes, help_item_compar);
	help_list_iterate(category_nodes, ptmp) {
	  help_list_append(help_nodes, ptmp);
	} help_list_iterate_end;
        help_list_free(category_nodes);
	continue;
      }
    }
    
    /* It wasn't a "generate" node: */
    
    pitem = new_help_item(HELP_TEXT);
    pitem->topic = mystrdup(_(secfile_lookup_str(sf, "%s.name", sec[isec])));

    paras = secfile_lookup_str_vec(sf, &npara, "%s.text", sec[isec]);

    long_buffer[0] = '\0';
    for (i=0; i<npara; i++) {
      char *para = paras[i];
      if(strncmp(para, "$", 1)==0) {
        insert_generated_table(long_buffer, sizeof(long_buffer), para+1);
      } else {
        sz_strlcat(long_buffer, _(para));
      }
      if (i!=npara-1) {
        sz_strlcat(long_buffer, "\n\n");
      }
    }
    free(paras);
    paras = NULL;
    pitem->text=mystrdup(long_buffer);
    help_list_append(help_nodes, pitem);
  }

  free(sec);
  sec = NULL;
  section_file_check_unused(sf, sf->filename);
  section_file_free(sf);
  booted = TRUE;
  freelog(LOG_VERBOSE, "Booted help texts ok");
}

/****************************************************************
  The following few functions are essentially wrappers for the
  help_nodes genlist.  This allows us to avoid exporting the
  genlist, and instead only access it through a controlled
  interface.
*****************************************************************/

/****************************************************************
  Number of help items.
*****************************************************************/
int num_help_items(void)
{
  check_help_nodes_init();
  return help_list_size(help_nodes);
}

/****************************************************************
  Return pointer to given help_item.
  Returns NULL for 1 past end.
  Returns NULL and prints error message for other out-of bounds.
*****************************************************************/
const struct help_item *get_help_item(int pos)
{
  int size;
  
  check_help_nodes_init();
  size = help_list_size(help_nodes);
  if (pos < 0 || pos > size) {
    freelog(LOG_ERROR, "Bad index %d to get_help_item (size %d)", pos, size);
    return NULL;
  }
  if (pos == size) {
    return NULL;
  }
  return help_list_get(help_nodes, pos);
}

/****************************************************************
  Find help item by name and type.
  Returns help item, and sets (*pos) to position in list.
  If no item, returns pointer to static internal item with
  some faked data, and sets (*pos) to -1.
*****************************************************************/
const struct help_item*
get_help_item_spec(const char *name, enum help_page_type htype, int *pos)
{
  int idx;
  const struct help_item *pitem = NULL;
  static struct help_item vitem; /* v = virtual */
  static char vtopic[128];
  static char vtext[256];

  check_help_nodes_init();
  idx = 0;
  help_list_iterate(help_nodes, ptmp) {
    char *p=ptmp->topic;
    while (*p == ' ') {
      p++;
    }
    if(strcmp(name, p)==0 && (htype==HELP_ANY || htype==ptmp->type)) {
      pitem = ptmp;
      break;
    }
    idx++;
  }
  help_list_iterate_end;
  
  if(!pitem) {
    idx = -1;
    vitem.topic = vtopic;
    sz_strlcpy(vtopic, name);
    vitem.text = vtext;
    if(htype==HELP_ANY || htype==HELP_TEXT) {
      my_snprintf(vtext, sizeof(vtext),
		  _("Sorry, no help topic for %s.\n"), vitem.topic);
      vitem.type = HELP_TEXT;
    } else {
      my_snprintf(vtext, sizeof(vtext),
		  _("Sorry, no help topic for %s.\n"
		    "This page was auto-generated.\n\n"),
		  vitem.topic);
      vitem.type = htype;
    }
    pitem = &vitem;
  }
  *pos = idx;
  return pitem;
}

/****************************************************************
  Start iterating through help items;
  that is, reset iterator to start position.
  (Could iterate using get_help_item(), but that would be
  less efficient due to scanning to find pos.)
*****************************************************************/
void help_iter_start(void)
{
  check_help_nodes_init();
  help_nodes_iterator = genlist_head(help_list_base(help_nodes));
}

/****************************************************************
  Returns next help item; after help_iter_start(), this is
  the first item.  At end, returns NULL.
*****************************************************************/
const struct help_item *help_iter_next(void)
{
  const struct help_item *pitem;
  
  check_help_nodes_init();
  pitem = genlist_link_data(help_nodes_iterator);
  if (pitem) {
    help_nodes_iterator = genlist_link_next(help_nodes_iterator);
  }

  return pitem;
}


/****************************************************************
  FIXME:
  Also, in principle these could be auto-generated once, inserted
  into pitem->text, and then don't need to keep re-generating them.
  Only thing to be careful of would be changeable data, but don't
  have that here (for ruleset change or spacerace change must
  re-boot helptexts anyway).  Eg, genuinely dynamic information
  which could be useful would be if help system said which wonders
  have been built (or are being built and by who/where?)
*****************************************************************/

/**************************************************************************
  Write dynamic text for buildings (including wonders).  This includes
  the ruleset helptext as well as any automatically generated text.

  pplayer may be NULL.
  user_text, if non-NULL, will be appended to the text.
**************************************************************************/
char *helptext_building(char *buf, size_t bufsz, struct player *pplayer,
			const char *user_text, struct impr_type *pimprove)
{
  struct universal source = {
    .kind = VUT_IMPROVEMENT,
    .value = {.building = pimprove}
  };

  assert(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (NULL == pimprove) {
    return buf;
  }

  if (pimprove->helptext && pimprove->helptext[0] != '\0') {
    cat_snprintf(buf, bufsz, "%s\n\n", _(pimprove->helptext));
  }

  if (valid_advance(pimprove->obsolete_by)) {
    cat_snprintf(buf, bufsz,
		 _("* The discovery of %s will make %s obsolete.\n"),
		 advance_name_for_player(pplayer,
					 advance_number(pimprove->obsolete_by)),
		 improvement_name_translation(pimprove));
  }

  if (building_has_effect(pimprove, EFT_ENABLE_NUKE)
      && num_role_units(F_NUCLEAR) > 0) {
    struct unit_type *u = get_role_unit(F_NUCLEAR, 0);
    CHECK_UNIT_TYPE(u);

    /* TRANS: 'Allows all players with knowledge of atomic power to
     * build nuclear units.' */
    cat_snprintf(buf, bufsz,
		 _("* Allows all players with knowledge of %s "
		   "to build %s units.\n"),
		 advance_name_for_player(pplayer,
					 advance_number(u->require_advance)),
		 utype_name_translation(u));
    cat_snprintf(buf, bufsz, "  ");
  }

  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));

  unit_type_iterate(u) {
    if (u->need_improvement == pimprove) {
      if (A_NEVER != u->require_advance) {
	cat_snprintf(buf, bufsz, _("* Allows %s (with %s).\n"),
		     utype_name_translation(u),
		     advance_name_for_player(pplayer,
					     advance_number(u->require_advance)));
      } else {
	cat_snprintf(buf, bufsz, _("* Allows %s.\n"),
		     utype_name_translation(u));
      }
    }
  } unit_type_iterate_end;

  if (user_text && user_text[0] != '\0') {
    cat_snprintf(buf, bufsz, "\n\n%s", user_text);
  }
  return buf;
}

#define techs_with_flag_iterate(flag, tech_id)				    \
{									    \
  Tech_type_id tech_id = 0;						    \
									    \
  while ((tech_id = find_advance_by_flag(tech_id, (flag))) != A_LAST) {

#define techs_with_flag_iterate_end		\
    tech_id++;					\
  }						\
}

/****************************************************************************
  Return a string containing the techs that have the flag.  Returns the
  number of techs found.

  pplayer may be NULL.
****************************************************************************/
static int techs_with_flag_string(char *buf, size_t bufsz,
				  struct player *pplayer,
				  enum tech_flag_id flag)
{
  int count = 0;

  assert(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  techs_with_flag_iterate(flag, tech_id) {
    const char *name = advance_name_for_player(pplayer, tech_id);

    if (buf[0] == '\0') {
      CATLSTR(buf, bufsz, name);
    } else {
      /* TRANS: continue list, in case comma is not the separator of choice. */
      cat_snprintf(buf, bufsz, Q_("?clistmore:, %s"), name);
    }
    count++;
  } techs_with_flag_iterate_end;

  return count;
}

/****************************************************************
  Append misc dynamic text for units.
  Transport capacity, unit flags, fuel.

  pplayer may be NULL.
*****************************************************************/
char *helptext_unit(char *buf, size_t bufsz, struct player *pplayer,
		    const char *user_text, struct unit_type *utype)
{
  assert(NULL != buf && 0 < bufsz && NULL != user_text);

  if (!utype) {
    freelog(LOG_ERROR, "Unknown unit!");
    mystrlcpy(buf, user_text, bufsz);
    return buf;
  }
  buf[0] = '\0';

  cat_snprintf(buf, bufsz,
               _("* Belongs to %s units class.\n"),
               uclass_name_translation(utype_class(utype)));
  if (uclass_has_flag(utype_class(utype), UCF_CAN_OCCUPY_CITY)
      && !utype_has_flag(utype, F_CIVILIAN)) {
    CATLSTR(buf, bufsz, _("  * Can occupy empty enemy cities.\n"));
  }
  if (!uclass_has_flag(utype_class(utype), UCF_TERRAIN_SPEED)) {
    CATLSTR(buf, bufsz, _("  * Speed is not affected by terrain.\n"));
  }
  if (!uclass_has_flag(utype_class(utype), UCF_TERRAIN_DEFENSE)) {
    CATLSTR(buf, bufsz, _("  * Does not get defense bonuses from terrain.\n"));
  }
  if (uclass_has_flag(utype_class(utype), UCF_DAMAGE_SLOWS)) {
    CATLSTR(buf, bufsz, _("  * Slowed down while damaged\n"));
  }
  if (uclass_has_flag(utype_class(utype), UCF_MISSILE)) {
    CATLSTR(buf, bufsz, _("  * Gets used up in making an attack.\n"));
  }
  if (uclass_has_flag(utype_class(utype), UCF_UNREACHABLE)) {
    CATLSTR(buf, bufsz,
	    _("  * Is unreachable. Most units cannot attack this one.\n"));
  }
  if (uclass_has_flag(utype_class(utype), UCF_CAN_PILLAGE)) {
    CATLSTR(buf, bufsz,
	    _("  * Can pillage tile improvements.\n"));
  }
  if (uclass_has_flag(utype_class(utype), UCF_DOESNT_OCCUPY_TILE)) {
    CATLSTR(buf, bufsz,
	    _("  * Doesn't prevent enemy cities from using tile.\n"));
  }

  if (utype->need_improvement) {
    cat_snprintf(buf, bufsz,
                 _("* Can only be built if there is %s in the city.\n"),
                 improvement_name_translation(utype->need_improvement));
  }

  if (utype->need_government) {
    cat_snprintf(buf, bufsz,
                 _("* Can only be built with %s as government.\n"),
                 government_name_translation(utype->need_government));
  }
  
  if (utype_has_flag(utype, F_NOBUILD)) {
    CATLSTR(buf, bufsz, _("* May not be built in cities.\n"));
  }
  if (utype_has_flag(utype, F_BARBARIAN_ONLY)) {
    CATLSTR(buf, bufsz, _("* Only barbarians may build this.\n"));
  }
  if (utype_has_flag(utype, F_NOHOME)) {
    CATLSTR(buf, bufsz, _("* Never has a home city.\n"));
  }
  if (utype_has_flag(utype, F_GAMELOSS)) {
    CATLSTR(buf, bufsz, _("* Losing this unit will lose you the game!\n"));
  }
  if (utype_has_flag(utype, F_UNIQUE)) {
    CATLSTR(buf, bufsz,
	    _("* Each player may only have one of this type of unit.\n"));
  }
  if (utype->pop_cost > 0) {
    cat_snprintf(buf, bufsz,
                 _("* Requires %d population to build.\n"),
                 utype->pop_cost);
  }
  if (utype->transport_capacity > 0) {
    cat_snprintf(buf, bufsz,
                 PL_("* Can carry and refuel %d unit from classes:\n",
                     "* Can carry and refuel up to %d units from classes:\n",
                     utype->transport_capacity),
                 utype->transport_capacity);
    unit_class_iterate(uclass) {
      if (can_unit_type_transport(utype, uclass)) {
        cat_snprintf(buf, bufsz,
                     _("  * %s units\n"),
                     uclass_name_translation(uclass));
      }
    } unit_class_iterate_end
  }
  if (utype_has_flag(utype, F_TRADE_ROUTE)) {
    /* TRANS: "Manhattan" distance is the distance along gridlines, with
     * no diagonals allowed. */
    cat_snprintf(buf, bufsz,
                 _("* Can establish trade routes (must travel to target"
                   " city and must be at least %d tiles [in Manhattan"
                   " distance] from this unit's home city).\n"),
                 game.info.trademindist);
  }
  if (utype_has_flag(utype, F_HELP_WONDER)) {
    cat_snprintf(buf, bufsz,
		 _("* Can help build wonders (adds %d production).\n"),
		 utype_build_shield_cost(utype));
  }
  if (utype_has_flag(utype, F_UNDISBANDABLE)) {
    CATLSTR(buf, bufsz, _("* May not be disbanded.\n"));
  } else {
    CATLSTR(buf, bufsz,
	    /* xgettext:no-c-format */
	    _("* May be disbanded in a city to recover 50% of the"
	      " production cost.\n"));
  }
  if (utype_has_flag(utype, F_CITIES)) {
    CATLSTR(buf, bufsz, _("* Can build new cities.\n"));
  }
  if (utype_has_flag(utype, F_ADD_TO_CITY)) {
    cat_snprintf(buf, bufsz,
		 _("* Can add on %d population to cities of no more than"
		   " size %d.\n"),
		 utype_pop_value(utype),
		 game.info.add_to_size_limit - utype_pop_value(utype));
  }
  if (utype_has_flag(utype, F_SETTLERS)) {
    char buf2[1024];

    /* Roads, rail, mines, irrigation. */
    CATLSTR(buf, bufsz, _("* Can build roads and railroads.\n"));
    CATLSTR(buf, bufsz, _("* Can build mines on tiles.\n"));
    CATLSTR(buf, bufsz, _("* Can build irrigation on tiles.\n"));

    /* Farmland. */
    switch (techs_with_flag_string(buf2, sizeof(buf2), pplayer, TF_FARMLAND)) {
    case 0:
      CATLSTR(buf, bufsz, _("* Can build farmland.\n"));
      break;
    case 1:
      cat_snprintf(buf, bufsz,
		   _("* Can build farmland (if %s is known).\n"), buf2);
      break;
    default:
      cat_snprintf(buf, bufsz,
		   _("* Can build farmland (if any of the following are"
		     " known: %s).\n"), buf2);
      break;
    }

    /* Fortress. */
    CATLSTR(buf, bufsz, _("* Can build fortresses.\n"));
 
    /* Pollution, fallout. */
    CATLSTR(buf, bufsz, _("* Can clean pollution from tiles.\n"));
    CATLSTR(buf, bufsz, _("* Can clean nuclear fallout from tiles.\n"));
  }
  if (utype_has_flag(utype, F_TRANSFORM)) {
    CATLSTR(buf, bufsz, _("* Can transform tiles.\n"));
  }
  if (is_ground_unittype(utype) && !utype_has_flag(utype, F_SETTLERS)) {
    CATLSTR(buf, bufsz,
            /* xgettext:no-c-format */
            _("* May fortify, granting a 50% defensive bonus.\n"));
  }
  if (is_ground_unittype(utype)) {
    CATLSTR(buf, bufsz, _("* May pillage to destroy infrastructure from tiles.\n"));
  }
  if (utype_has_flag(utype, F_DIPLOMAT)) {
    if (utype_has_flag(utype, F_SPY)) {
      CATLSTR(buf, bufsz, _("* Can perform diplomatic actions,"
			    " plus special spy abilities.\n"));
    } else {
      CATLSTR(buf, bufsz, _("* Can perform diplomatic actions.\n"));
    }
  }
  if (utype_has_flag(utype, F_SUPERSPY)) {
    CATLSTR(buf, bufsz, _("* Will never lose a diplomat-versus-diplomat fight.\n"));
  }
  if (utype_has_flag(utype, F_UNBRIBABLE)) {
    CATLSTR(buf, bufsz, _("* May not be bribed.\n"));
  }
  if (utype_has_flag(utype, F_PARTIAL_INVIS)) {
    CATLSTR(buf, bufsz,
            _("* Is invisible except when next to an enemy unit or city.\n"));
  }
  if (utype_has_flag(utype, F_NO_LAND_ATTACK)) {
    CATLSTR(buf, bufsz,
            _("* Can only attack units on ocean squares (no land attacks).\n"));
  }
  if (utype_has_flag(utype, F_MARINES)) {
    CATLSTR(buf, bufsz,
	    _("* Can attack from aboard sea units: against"
	      " enemy cities and onto land squares.\n"));
  }
  if (utype_has_flag(utype, F_PARATROOPERS)) {
    cat_snprintf(buf, bufsz,
		 _("* Can be paradropped from a friendly city"
		   " (Range: %d).\n"),
		 utype->paratroopers_range);
  }
  if (utype_has_flag(utype, F_PIKEMEN)) {
    CATLSTR(buf, bufsz,
            _("* Gets double defense against units specified as 'mounted'.\n"));
  }
  if (utype_has_flag(utype, F_HORSE)) {
    CATLSTR(buf, bufsz,
	    _("* Counts as 'mounted' against certain defenders.\n"));
  }
  if (utype_has_flag(utype, F_HELICOPTER)) {
    CATLSTR(buf, bufsz,
            _("* Counts as 'helicopter' against certain attackers.\n"));
  }
  if (utype_has_flag(utype, F_FIGHTER)) {
    CATLSTR(buf, bufsz,
            _("* Very good at attacking 'helicopter' units.\n"));
  }
  if (utype_has_flag(utype, F_AIRUNIT)) {
    CATLSTR(buf, bufsz,
            _("* Very bad at attacking AEGIS units.\n"));
  }
  if (!uclass_has_flag(utype_class(utype), UCF_MISSILE)
      && utype_has_flag(utype, F_ONEATTACK)) {
    CATLSTR(buf, bufsz,
	    _("* Making an attack ends this unit's turn.\n"));
  }
  if (utype_has_flag(utype, F_NUCLEAR)) {
    CATLSTR(buf, bufsz,
	    _("* This unit's attack causes a nuclear explosion!\n"));
  }
  if (utype_has_flag(utype, F_CITYBUSTER)) {
    CATLSTR(buf, bufsz,
	    _("* Gets double firepower when attacking cities.\n"));
  }
  if (utype_has_flag(utype, F_IGWALL)) {
    CATLSTR(buf, bufsz, _("* Ignores the effects of city walls.\n"));
  }
  if (utype_has_flag(utype, F_BOMBARDER)) {
    cat_snprintf(buf, bufsz,
		 _("* Does bombard attacks (%d per turn).  These attacks will"
		   " only damage (never kill) the defender, but have no risk"
		   " for the attacker.\n"),
		 utype->bombard_rate);
  }
  if (utype_has_flag(utype, F_AEGIS)) {
    CATLSTR(buf, bufsz,
	    _("* Gets quintuple defense against missiles and aircraft.\n"));
  }
  if (utype_has_flag(utype, F_IGTER)) {
    CATLSTR(buf, bufsz,
	    _("* Ignores terrain effects (treats all squares as roads).\n"));
  }
  if (utype_has_flag(utype, F_IGZOC)) {
    CATLSTR(buf, bufsz, _("* Ignores zones of control.\n"));
  }
  if (utype_has_flag(utype, F_CIVILIAN)) {
    CATLSTR(buf, bufsz,
            _("* A non-military unit (cannot attack; no martial law).\n"));
  }
  if (utype_has_flag(utype, F_FIELDUNIT)) {
    CATLSTR(buf, bufsz,
            _("* A field unit: one unhappiness applies even when non-aggressive.\n"));
  }
  if (utype_has_flag(utype, F_NO_VETERAN)) {
    CATLSTR(buf, bufsz, _("* Will never achieve veteran status.\n"));
  } else {
    switch(utype_move_type(utype)) {
      case BOTH_MOVING:
        CATLSTR(buf, bufsz,
                _("* Will be built as a veteran in cities with appropriate"
                  " training facilities (see Airport).\n"));
        CATLSTR(buf, bufsz,
                _("* May be promoted after defeating an enemy unit.\n"));
        break;
      case LAND_MOVING:
        if (utype_has_flag(utype, F_DIPLOMAT)||utype_has_flag(utype, F_SPY)) {
          CATLSTR(buf, bufsz,
                  _("* Will be built as a veteran under communist governments.\n"));
          CATLSTR(buf, bufsz,
                  _("* May be promoted after a successful mission.\n"));
        } else {
          CATLSTR(buf, bufsz,
                  _("* Will be built as a veteran in cities with appropriate"
                    " training facilities (see Barracks).\n"));
          CATLSTR(buf, bufsz,
                  _("* May be promoted after defeating an enemy unit.\n"));
        }
        break;
      case SEA_MOVING:
        CATLSTR(buf, bufsz,
                _("* Will be built as a veteran in cities with appropriate"
                  " training facilities (see Port Facility).\n"));
        CATLSTR(buf, bufsz,
                _("* May be promoted after defeating an enemy unit.\n"));
        break;
      default:          /* should never happen in default rulesets */
        CATLSTR(buf, bufsz,
                _("* May be promoted through combat or training\n"));
        break;
    };
  }
  if (utype_has_flag(utype, F_SHIELD2GOLD)) {
    /* FIXME: the conversion shield => gold is activated if
     *        EFT_SHIELD2GOLD_FACTOR is not equal null; how to determine
     *        possible sources? */
    CATLSTR(buf, bufsz,
            _("* Under certain conditions the shield upkeep of this unit can "
              " be converted to gold upkeep.\n"));
  }

  unit_class_iterate(pclass) {
    if (uclass_has_flag(pclass, UCF_UNREACHABLE)
        && BV_ISSET(utype->targets, uclass_index(pclass))) {
      cat_snprintf(buf, bufsz, "* Can attack against %s units, which are usually not reachable.\n",
                   uclass_name_translation(pclass));
    }
  } unit_class_iterate_end;
  if (utype_fuel(utype)) {
    char allowed_units[10][64];
    int num_allowed_units = 0;
    int j;
    struct astring astr;

    astr_init(&astr);
    astr_minsize(&astr,1);
    astr.str[0] = '\0';

    unit_type_iterate(transport) {
      if (can_unit_type_transport(transport, utype_class(utype))) {
        mystrlcpy(allowed_units[num_allowed_units],
                  utype_name_translation(transport),
                  sizeof(allowed_units[num_allowed_units]));
        num_allowed_units++;
      }
    } unit_type_iterate_end;

    for (j = 0; j < num_allowed_units; j++) {
      const char *deli_str = NULL;

      /* there should be something like astr_append() */
      astr_minsize(&astr, astr.n + strlen(allowed_units[j]));
      strcat(astr.str, allowed_units[j]);

      if (j == num_allowed_units - 2) {
        /* TRANS: List of possible unit types has this between
         *        last two elements */
	deli_str = Q_(" or ");
      } else if (j < num_allowed_units - 1) {
	deli_str = Q_("?or:, ");
      }

      if (deli_str) {
	astr_minsize(&astr, astr.n + strlen(deli_str));
	strcat(astr.str, deli_str);
      }
    }

    if (num_allowed_units == 0) {
     cat_snprintf(buf, bufsz,
                   PL_("* Unit has to be in a city, or a base"
                       " after %d turn.\n",
                       "* Unit has to be in a city, or a base"
                       " after %d turns.\n",
                       utype_fuel(utype)),
                  utype_fuel(utype));
    } else {
      cat_snprintf(buf, bufsz,
                   PL_("* Unit has to be in a city, a base, or on a %s"
                       " after %d turn.\n",
                       "* Unit has to be in a city, a base, or on a %s"
                       " after %d turns.\n",
                       utype_fuel(utype)),
                   astr.str,
                   utype_fuel(utype));
    }
    astr_free(&astr);
  }
  if (strlen(buf) > 0) {
    CATLSTR(buf, bufsz, "\n");
  } 
  if (utype->helptext && utype->helptext[0] != '\0') {
    cat_snprintf(buf, bufsz, "%s\n\n", _(utype->helptext));
  }
  CATLSTR(buf, bufsz, user_text);
  return buf;
}

/****************************************************************
  Append misc dynamic text for advance/technology.

  pplayer may be NULL.
*****************************************************************/
void helptext_advance(char *buf, size_t bufsz, struct player *pplayer,
		      const char *user_text, int i)
{
  struct advance *vap = valid_advance_by_number(i);
  struct universal source = {
    .kind = VUT_ADVANCE,
    .value = {.advance = vap}
  };

  assert(NULL != buf && 0 < bufsz && NULL != user_text);
  mystrlcpy(buf, user_text, bufsz);

  if (NULL == vap) {
    freelog(LOG_ERROR, "Unknown tech %d.", i);
    return;
  }

  if (player_invention_state(pplayer, i) != TECH_KNOWN) {
    if (player_invention_state(pplayer, i) == TECH_PREREQS_KNOWN) {
      cat_snprintf(buf, bufsz,
		   _("If we would now start with %s we would need %d bulbs."),
		   advance_name_for_player(pplayer, i),
		   base_total_bulbs_required(pplayer, i));
    } else if (player_invention_reachable(pplayer, i)) {
      cat_snprintf(buf, bufsz,
		   _("To reach %s we need to obtain %d other"
		     " technologies first. The whole project"
		     " will require %d bulbs to complete."),
		   advance_name_for_player(pplayer, i),
		   num_unknown_techs_for_goal(pplayer, i) - 1,
		   total_bulbs_required_for_goal(pplayer, i));
    } else {
      CATLSTR(buf, bufsz,
	      _("You cannot research this technology."));
    }
    if (!techs_have_fixed_costs()
     && player_invention_reachable(pplayer, i)) {
      CATLSTR(buf, bufsz,
	      _(" This number may vary depending on what "
		"other players will research.\n"));
    } else {
      CATLSTR(buf, bufsz, "\n");
    }
  }

  CATLSTR(buf, bufsz, "\n");
  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));

  if (advance_has_flag(i, TF_BONUS_TECH)) {
    cat_snprintf(buf, bufsz,
		 _("* The first player to research %s gets"
		   " an immediate advance.\n"),
		 advance_name_for_player(pplayer, i));
  }
  if (advance_has_flag(i, TF_POPULATION_POLLUTION_INC))
    CATLSTR(buf, bufsz,
            _("* Increases the pollution generated by the population.\n"));

  if (advance_has_flag(i, TF_BRIDGE)) {
    const char *units_str = role_units_translations(F_SETTLERS);
    cat_snprintf(buf, bufsz,
		 _("* Allows %s to build roads on river squares.\n"),
		 units_str);
    free((void *) units_str);
  }

  if (advance_has_flag(i, TF_RAILROAD)) {
    const char *units_str = role_units_translations(F_SETTLERS);
    cat_snprintf(buf, bufsz,
		 _("* Allows %s to upgrade roads to railroads.\n"),
		 units_str);
    free((void *) units_str);
  }

  if (advance_has_flag(i, TF_FARMLAND)) {
    const char *units_str = role_units_translations(F_SETTLERS);
    cat_snprintf(buf, bufsz,
		 _("* Allows %s to upgrade irrigation to farmland.\n"),
		 units_str);
    free((void *) units_str);
  }
  if (vap->helptext && vap->helptext[0] != '\0') {
    if (strlen(buf) > 0) {
      CATLSTR(buf, bufsz, "\n");
    }
    cat_snprintf(buf, bufsz, "%s\n", _(vap->helptext));
  }
}

/****************************************************************
  Append text for terrain.
*****************************************************************/
void helptext_terrain(char *buf, size_t bufsz, struct player *pplayer,
		      const char *user_text, struct terrain *pterrain)
{
  struct universal source = {
    .kind = VUT_TERRAIN,
    .value = {.terrain = pterrain}
  };

  assert(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (!pterrain) {
    freelog(LOG_ERROR, "Unknown terrain!");
    return;
  }

  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));
  if (terrain_has_flag(pterrain, TER_NO_POLLUTION)) {
    CATLSTR(buf, bufsz,
	    _("* Pollution cannot be generated on this terrain."));
    CATLSTR(buf, bufsz, "\n");
  }
  if (terrain_has_flag(pterrain, TER_NO_CITIES)) {
    CATLSTR(buf, bufsz,
	    _("* You cannot build cities on this terrain."));
    CATLSTR(buf, bufsz, "\n");
  }
  if (terrain_has_flag(pterrain, TER_UNSAFE_COAST)
      && !terrain_has_flag(pterrain, TER_OCEANIC)) {
    CATLSTR(buf, bufsz,
	    _("* The coastline of this terrain is unsafe."));
    CATLSTR(buf, bufsz, "\n");
  }
  if (terrain_has_flag(pterrain, TER_OCEANIC)) {
    CATLSTR(buf, bufsz,
	    _("* Land units cannot travel on oceanic terrains."));
    CATLSTR(buf, bufsz, "\n");
  }

  if (pterrain->helptext[0] != '\0') {
    if (buf[0] != '\0') {
      CATLSTR(buf, bufsz, "\n");
    }
    CATLSTR(buf, bufsz, _(pterrain->helptext));
  }
  if (user_text && user_text[0] != '\0') {
    CATLSTR(buf, bufsz, "\n\n");
    CATLSTR(buf, bufsz, user_text);
  }
}

/****************************************************************
  Append text for government.

  pplayer may be NULL.

  TODO: Generalize the effects code for use elsewhere. Add
  other requirements.
*****************************************************************/
void helptext_government(char *buf, size_t bufsz, struct player *pplayer,
			 const char *user_text, struct government *gov)
{
  struct universal source = {
    .kind = VUT_GOVERNMENT,
    .value = {.govern = gov}
  };

  assert(NULL != buf && 0 < bufsz);
  buf[0] = '\0';

  if (gov->helptext[0] != '\0') {
    cat_snprintf(buf, bufsz, "%s\n\n", _(gov->helptext));
  }

  /* Add requirement text for government itself */
  requirement_vector_iterate(&gov->reqs, preq) {
    insert_requirement(buf, bufsz, pplayer, preq);
  } requirement_vector_iterate_end;

  /* Effects */
  CATLSTR(buf, bufsz, _("Features:\n"));
  insert_allows(&source, buf + strlen(buf), bufsz - strlen(buf));
  effect_list_iterate(get_req_source_effects(&source), peffect) {
    Output_type_id output_type = O_LAST;
    struct unit_class *unitclass = NULL;
    struct unit_type *unittype = NULL;
    enum unit_flag_id unitflag = F_LAST;
    struct astring outputs_or = ASTRING_INIT;
    struct astring outputs_and = ASTRING_INIT;
    bool extra_reqs = FALSE;

    astr_clear(&outputs_or);
    astr_clear(&outputs_and);

    /* Grab output type, if there is one */
    requirement_list_iterate(peffect->reqs, preq) {
      switch (preq->source.kind) {
       case VUT_OTYPE:
         if (output_type == O_LAST) {
           /* We should never have multiple outputtype requirements
            * in one list in the first place (it simply makes no sense,
            * output cannot be of multiple types)
            * Ruleset loading code should check against that. */
           const char *oname;

           output_type = preq->source.value.outputtype;
           oname = get_output_name(output_type);
           astr_add(&outputs_or, "%s", oname);
           astr_add(&outputs_and, "%s", oname);
         }
         break;
       case VUT_UCLASS:
         if (unitclass == NULL) {
           unitclass = preq->source.value.uclass;
         }
         break;
       case VUT_UTYPE:
         if (unittype == NULL) {
           unittype = preq->source.value.utype;
         }
         break;
       case VUT_UTFLAG:
         if (unitflag == F_LAST) {
           /* FIXME: We should list all the unit flag requirements,
            *        not only first one. */
           unitflag = preq->source.value.unitflag;
         }
         break;
       case VUT_GOVERNMENT:
         /* This is government we are generating helptext for.
          * ...or if not, it's ruleset bug that should never make it
          * this far. Fix ruleset loading code. */
         break;
       default:
         extra_reqs = TRUE;
         break;
      };
    } requirement_list_iterate_end;

    if (!extra_reqs) {
      /* Only list effects that have no special requirements. */

      if (output_type == O_LAST) {
        /* There was no outputtype requirement. Effect is active for all
         * output types. Generate lists for that. */
        const char *prev  = NULL;
        const char *prev2 = NULL;
        bool harvested_only = TRUE; /* Consider only output types from fields */

        if (peffect->type == EFT_UPKEEP_FACTOR
            || peffect->type == EFT_UNIT_UPKEEP_FREE_PER_CITY
            || peffect->type == EFT_OUTPUT_BONUS
            || peffect->type == EFT_OUTPUT_BONUS_2) {
          /* Effect can use or require any kind of output */
          harvested_only = FALSE;
        }

        output_type_iterate(ot) {
          struct output_type *pot = get_output_type(ot);

          if (!harvested_only || pot->harvested) {
            if (prev2 != NULL) {
              astr_add(&outputs_or,  "%s", prev2);
              astr_add(&outputs_or,  "%s", Q_("?or:, "));
              astr_add(&outputs_and, "%s", prev2);
              astr_add(&outputs_and, "%s", Q_("?and:, "));
            }
            prev2 = prev;
            prev = _(pot->name);
          }
        } output_type_iterate_end;
        if (prev2 != NULL) {
          astr_add(&outputs_or, "%s", prev2);
          /* TRANS: List of possible output types has this between
           *        last two elements */
          astr_add(&outputs_or,  "%s", Q_(" or "));
          astr_add(&outputs_and, "%s", prev2);
          /* TRANS: List of possible output types has this between
           *        last two elements */
          astr_add(&outputs_and, "%s", Q_(" and "));
        }
        if (prev != NULL) {
          astr_add(&outputs_or,  "%s", prev);
          astr_add(&outputs_and, "%s", prev);
        } else {
          /* TRANS: Empty output type list, should never happen. */
          astr_add(&outputs_or,  "%s", Q_("?outputlist: Nothing "));
          astr_add(&outputs_and, "%s", Q_("?outputlist: Nothing "));
        }
      }

      switch (peffect->type) {
      case EFT_UNHAPPY_FACTOR:
        cat_snprintf(buf, bufsz,
                     PL_("* Military units away from home and field units"
                         " will cause %d citizen to become unhappy.\n",
                         "* Military units away from home and field units"
                         " will cause %d citizens to become unhappy.\n",
                         peffect->value),
                     peffect->value);
        break;
      case EFT_MAKE_CONTENT:
      case EFT_FORCE_CONTENT:
        cat_snprintf(buf, bufsz,
                     _("* Each of your cities will avoid %d unhappiness"
                       " that would otherwise be caused by units.\n"),
                     peffect->value);
        break;
      case EFT_UPKEEP_FACTOR:
        if (peffect->value > 1 && output_type != O_LAST) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is the output type, like 'shield' or 'gold'. */
                       _("* You pay %d times normal %s upkeep for your units.\n"),
                       peffect->value,
                       outputs_and.str);
        } else if (peffect->value > 1) {
          cat_snprintf(buf, bufsz,
                       _("* You pay %d times normal upkeep for your units.\n"),
                       peffect->value);
        } else if (peffect->value == 0 && output_type != O_LAST) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is the output type, like 'shield' or 'gold'. */
                       _("* You pay no %s upkeep for your units.\n"),
                       outputs_and.str);
        } else if (peffect->value == 0) {
          CATLSTR(buf, bufsz,
                  _("* You pay no upkeep for your units.\n"));
        }
        break;
      case EFT_UNIT_UPKEEP_FREE_PER_CITY:
        if (output_type != O_LAST) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is the output type, like 'shield' or 'gold'.
                        * There is currently no way to control the
                        * singular/plural version of these. */
                       _("* Each of your cities will avoid paying %d %s"
                         " upkeep for your units.\n"),
                       peffect->value,
                       outputs_and.str);
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: Amount is subtracted from upkeep cost
                        * for each upkeep type. */
                       _("* Each of your cities will avoid paying %d"
                         " upkeep for your units.\n"),
                       peffect->value);
        }
        break;
      case EFT_CIVIL_WAR_CHANCE:
        cat_snprintf(buf, bufsz,
                     _("* If you lose your capital,"
                       " chance of civil war is %d%%.\n"),
                     peffect->value);
        break;
      case EFT_EMPIRE_SIZE_BASE:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %d should always be greater than 2. */
                     _("* When you have %d cities, the first unhappy citizen"
                       " will appear in each city due to civilization size.\n"),
                     peffect->value);
        break;
      case EFT_EMPIRE_SIZE_STEP:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %d should always be greater than 2. */
                     _("* After the first unhappy citizen due to"
                       " civilization size, for each %d additional cities"
                       " another unhappy citizen will appear.\n"),
                     peffect->value);
        break;
      case EFT_MAX_RATES:
        if (peffect->value < 100 && game.info.changable_tax) {
          cat_snprintf(buf, bufsz,
                       _("* The maximum rate you can set for science,"
                          " gold, or luxuries is %d%%.\n"),
                       peffect->value);
        } else if (game.info.changable_tax) {
          CATLSTR(buf, bufsz,
                  _("* Has unlimited science/gold/luxuries rates.\n"));
        }
        break;
      case EFT_MARTIAL_LAW_EACH:
        cat_snprintf(buf, bufsz,
                     PL_("* Your units may impose martial law."
                         " Each military unit inside a city will force %d"
                         " unhappy citizen to become content.\n",
                         "* Your units may impose martial law."
                         " Each military unit inside a city will force %d"
                         " unhappy citizens to become content.\n",
                         peffect->value),
                     peffect->value);
        break;
      case EFT_MARTIAL_LAW_MAX:
        if (peffect->value < 100) {
          cat_snprintf(buf, bufsz,
                       PL_("* A maximum of %d unit in each city can enforce"
                           " martial law.\n",
                           "* A maximum of %d units in each city can enforce"
                           " martial law.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_RAPTURE_GROW:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %d should always be greater than 2. */
                     _("* You may grow your cities by means of celebrations."
                       " Your cities must be at least size %d.\n"),
                     peffect->value);
        break;
      case EFT_UNBRIBABLE_UNITS:
        CATLSTR(buf, bufsz, _("* Your units cannot be bribed.\n"));
        break;
      case EFT_NO_INCITE:
        CATLSTR(buf, bufsz, _("* Your cities cannot be incited.\n"));
        break;
      case EFT_REVOLUTION_WHEN_UNHAPPY:
        CATLSTR(buf, bufsz,
                _("* If any city is in disorder for more than two turns in a row,"
                  " government will fall into anarchy.\n"));
        break;
      case EFT_HAS_SENATE:
        CATLSTR(buf, bufsz,
                _("* Has a senate that may prevent declaration of war.\n"));
        break;
      case EFT_INSPIRE_PARTISANS:
        CATLSTR(buf, bufsz,
                _("* Allows partisans when cities are taken by the enemy.\n"));
        break;
      case EFT_HAPPINESS_TO_GOLD:
        CATLSTR(buf, bufsz,
                _("* Buildings that normally confer bonuses against"
                  " unhappiness will instead give gold.\n"));
        break;
      case EFT_FANATICS:
        CATLSTR(buf, bufsz, _("* Pays no upkeep for fanatics.\n"));
        break;
      case EFT_NO_UNHAPPY:
        CATLSTR(buf, bufsz, _("* Has no unhappy citizens.\n"));
        break;
      case EFT_VETERAN_BUILD:
        /* FIXME: There could be both class and flag requirement.
         *        meaning that only some units from class are affected.
         *          Should class related string, type related strings and
         *        flag related string to be at least qualified to allow
         *        different translations? */
        if (unitclass) {
          cat_snprintf(buf, bufsz,
                       _("* Veteran %s units.\n"),
                       uclass_name_translation(unitclass));
        } else if (unittype != NULL) {
          cat_snprintf(buf, bufsz,
                       _("* Veteran %s units.\n"),
                       utype_name_translation(unittype));
        } else if (unitflag != F_LAST) {
          cat_snprintf(buf, bufsz,
                       _("* Veteran %s units.\n"),
                       unit_flag_rule_name(unitflag));
        } else {
          CATLSTR(buf, bufsz, _("* Veteran units.\n"));
        }
        break;
      case EFT_OUTPUT_PENALTY_TILE:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is list of output types, with 'or' */
                     _("* Each worked tile that gives more than %d %s will"
                       " suffer a -1 penalty unless celebrating.\n"),
                     peffect->value,
                     outputs_or.str);
        break;
      case EFT_OUTPUT_INC_TILE_CELEBRATE:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is list of output types, with 'or' */
                     _("* Each worked tile with at least 1 %s will yield"
                       " %d more of it while celebrating.\n"),
                     outputs_or.str,
                     peffect->value);
        break;
      case EFT_OUTPUT_INC_TILE:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is list of output types, with 'or' */
                     _("* Each worked tile with at least 1 %s will yield"
                       " %d more of it.\n"),
                     outputs_or.str,
                     peffect->value);
        break;
      case EFT_OUTPUT_BONUS:
      case EFT_OUTPUT_BONUS_2:
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is list of output types, with 'and' */
                     _("* %s production is increased %d%%.\n"),
                     outputs_and.str,
                     peffect->value);
        break;
      case EFT_OUTPUT_WASTE:
        if (peffect->value > 30) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of output types, with 'and' */
                       _("* %s production will suffer massive waste.\n"),
                       outputs_and.str);
        } else if (peffect->value >= 15) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of output types, with 'and' */
                       _("* %s production will suffer some waste.\n"),
                       outputs_and.str);
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of output types, with 'and' */
                       _("* %s production will suffer a small amount of waste.\n"),
                       outputs_and.str);
        }
        break;
      case EFT_HEALTH_PCT:
        if (peffect->value > 0) {
          CATLSTR(buf, bufsz, _("* Increases the possibility of plague"
                                " within Your cities.\n"));
        } else if (peffect->value < 0) {
          CATLSTR(buf, bufsz, _("* Decreases the possibility of plague"
                                " within Your cities.\n"));
        }
        break;
      case EFT_OUTPUT_WASTE_BY_DISTANCE:
        if (peffect->value >= 3) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of output types, with 'and' */
                       _("* %s waste will increase quickly"
                         " with distance from capital.\n"),
                       outputs_and.str);
        } else if (peffect->value == 2) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of output types, with 'and' */
                       _("* %s waste will increase"
                         " with distance from capital.\n"),
                       outputs_and.str);
        } else {
          cat_snprintf(buf, bufsz,
                       /* TRANS: %s is list of output types, with 'and' */
                       _("* %s waste will increase slowly"
                         " with distance from capital.\n"),
                       outputs_and.str);
        }
      case EFT_MIGRATION_PCT:
        if (peffect->value > 0) {
          CATLSTR(buf, bufsz, _("* Increases the possibility of migration"
                                " into Your cities.\n"));
        } else if (peffect->value < 0) {
          CATLSTR(buf, bufsz, _("* Decreases the possibility of migration"
                                " into Your cities.\n"));
        }
        break;
      default:
        break;
      };
    }

    astr_clear(&outputs_or);
    astr_clear(&outputs_and);

  } effect_list_iterate_end;

  unit_type_iterate(utype) {
    if (utype->need_government == gov) {
      cat_snprintf(buf, bufsz,
                   _("* Allows you to build %s.\n"),
                   utype_name_translation(utype));
    }
  } unit_type_iterate_end;

  CATLSTR(buf, bufsz, user_text);
}

/****************************************************************
  Returns pointer to static string with eg: "1 shield, 1 unhappy"
*****************************************************************/
char *helptext_unit_upkeep_str(struct unit_type *utype)
{
  static char buf[128];
  int any = 0;

  if (!utype) {
    freelog(LOG_ERROR, "Unknown unit!");
    return "";
  }


  buf[0] = '\0';
  output_type_iterate(o) {
    if (utype->upkeep[o] > 0) {
      /* TRANS: "2 Food" or ", 1 shield" */
      cat_snprintf(buf, sizeof(buf), _("%s%d %s"),
	      (any > 0 ? Q_("?blistmore:, ") : ""), utype->upkeep[o],
	      get_output_name(o));
      any++;
    }
  } output_type_iterate_end;
  if (utype->happy_cost > 0) {
    /* TRANS: "2 unhappy" or ", 1 unhappy" */
    cat_snprintf(buf, sizeof(buf), _("%s%d unhappy"),
	    (any > 0 ? Q_("?blistmore:, ") : ""), utype->happy_cost);
    any++;
  }

  if (any == 0) {
    /* strcpy(buf, _("None")); */
    my_snprintf(buf, sizeof(buf), "%d", 0);
  }
  return buf;
}
