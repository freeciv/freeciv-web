/***********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

/* utility */
#include "capability.h"
#include "fc_cmdline.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "support.h"

/* common */
#include "capstr.h"
#include "connection.h"
#include "events.h"
#include "fc_cmdhelp.h"
#include "fc_interface.h"
#include "fc_types.h" /* LINE_BREAK */
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "map.h"
#include "movement.h"
#include "player.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "helpdata.h"
#include "helpdlg_g.h"
#include "tilespec.h"

/* server */
#include "citytools.h"
#include "commands.h"
#include "connecthand.h"
#include "console.h"
#include "diplhand.h"
#include "gamehand.h"
#include "plrhand.h"
#include "report.h"
#include "ruleset.h"
#include "settings.h"
#include "sernet.h"
#include "srv_main.h"
#include "stdinhand.h"

/* tools/shared */
#include "tools_fc_interface.h"

enum manuals {
  MANUAL_SETTINGS,
  MANUAL_COMMANDS,
  MANUAL_TERRAIN,
  MANUAL_BUILDINGS,
  MANUAL_WONDERS,
  MANUAL_GOVS,
  MANUAL_UNITS,
  MANUAL_TECHS,
  MANUAL_COUNT
};

struct tag_types {
  const char *file_ext;
  const char *header;
  const char *title_begin;
  const char *title_end;
  const char *sect_title_begin;
  const char *sect_title_end;
  const char *image_begin;
  const char *image_end;
  const char *item_begin;
  const char *item_end;
  const char *subitem_begin;
  const char *subitem_end;
  const char *tail;
};

struct tag_types html_tags = {
  /* file extension */
  "html",

  /* header */
  "<html><head><link rel=\"stylesheet\" type=\"text/css\" "
  "href=\"manual.css\"/><meta http-equiv=\"Content-Type\" "
  "content=\"text/html; charset=UTF-8\"/></head><body>\n\n",

  /* title begin */
  "<h1>",

  /* title end */
  "</h1>",

  /* section title begin */
  "<h3 class='section'>",

  /* section title end */
  "</h3>",

  /* image begin */
  "<img src=\"",

  /* image end */
  ".png\">",

  /* item begin */
  "<div class='item' id='%s%d'>\n",

  /* item end */
  "</div>\n",

  /* subitem begin */
  "<pre class='%s'>",

  /* subitem end */
  "</pre>\n",

  /* tail */
  "</body></html>"
};

struct tag_types wiki_tags = {
  /* file extension */
  "mediawiki",

  /* header */
  " ",

  /* title begin */
  "=",

  /* title end */
  "=",

  /* section title begin */
  "===",

  /* section title end */
  "===",

  /* image begin */
  "[[Image:",

  /* image end */
  ".png]]",

  /* item begin */
  "----\n<!-- %s %d -->\n",

  /* item end */
  "\n",

  /* subitem begin */
  "<!-- %s -->\n",

  /* subitem end */
  "\n",

  /* tail */
  " "
};


void insert_client_build_info(char *outbuf, size_t outlen);

/* Needed for "About Freeciv" help */
const char *client_string = "freeciv-manual";

static char *ruleset = NULL;

/**********************************************************************//**
  Replace html special characters ('&', '<' and '>').
**************************************************************************/
static char *html_special_chars(char *str, size_t *len)
{
  char *buf;

  buf = fc_strrep_resize(str, len, "&", "&amp;");
  buf = fc_strrep_resize(buf, len, "<", "&lt;");
  buf = fc_strrep_resize(buf, len, ">", "&gt;");

  return buf;
}


/*******************************************
  Useless stubs for compiling client code.
*/

/**********************************************************************//**
  Client stub
**************************************************************************/
void popup_help_dialog_string(const char *item)
{
  /* Empty stub. */
}

/**********************************************************************//**
  Client stub
**************************************************************************/
void popdown_help_dialog(void)
{
  /* Empty stub. */
}

struct tileset *tileset;

/**********************************************************************//**
  Client stub
**************************************************************************/
const char *tileset_name_get(struct tileset *t)
{
  return NULL;
}

/**********************************************************************//**
  Client stub
**************************************************************************/
const char *tileset_version(struct tileset *t)
{
  return NULL;
}

/**********************************************************************//**
  Client stub
**************************************************************************/
const char *tileset_summary(struct tileset *t)
{
  return NULL;
}

/**********************************************************************//**
  Client stub
**************************************************************************/
const char *tileset_description(struct tileset *t)
{
  return NULL;
}

enum client_states client_state(void)
{
  return C_S_INITIAL;
}

bool client_nation_is_in_current_set(const struct nation_type *pnation)
{
  /* Currently, there is no way to select a nation set for freeciv-manual.
   * Then, let's assume we want to print help for all nations. */
  return TRUE;
}

/**********************************************************************//**
  Write a server manual, then quit.
**************************************************************************/
static bool manual_command(struct tag_types *tag_info)
{
  FILE *doc;
  char filename[40];
  enum manuals manuals;
  struct connection my_conn;

  /* Default client access. */
  connection_common_init(&my_conn);
  my_conn.access_level = ALLOW_CTRL;

  /* Reset aifill to zero */
  game.info.aifill = 0;

  if (!load_rulesets(NULL, FALSE, NULL, FALSE, FALSE)) {
    /* Failed to load correct ruleset */
    return FALSE;
  }

  for (manuals = 0; manuals < MANUAL_COUNT; manuals++) {
    int i;
    int ri;

    fc_snprintf(filename, sizeof(filename), "%s%d.%s",
                game.server.rulesetdir, manuals + 1, tag_info->file_ext);

    if (!is_reg_file_for_access(filename, TRUE)
        || !(doc = fc_fopen(filename, "w"))) {
      log_error(_("Could not write manual file %s."), filename);
      return FALSE;
    }

    fprintf(doc, "%s", tag_info->header);
    fprintf(doc, "<!-- Generated by freeciv-manual version %s -->\n\n",
            freeciv_datafile_version());

    switch (manuals) {
    case MANUAL_SETTINGS:
      /* TRANS: markup ... Freeciv version ... ruleset name ... markup */
      fprintf(doc, _("%sFreeciv %s server options (%s)%s\n\n"), tag_info->title_begin,
              VERSION_STRING, game.control.name, tag_info->title_end);
      settings_iterate(SSET_ALL, pset) {
        char buf[256];
        const char *sethelp;

        fprintf(doc, tag_info->item_begin, "setting", setting_number(pset));
        fprintf(doc, "%s%s - %s%s\n\n", tag_info->sect_title_begin,
                setting_name(pset), _(setting_short_help(pset)),
                tag_info->sect_title_end);
        sethelp = _(setting_extra_help(pset, TRUE));
        if (strlen(sethelp) > 0) {
          char *help = fc_strdup(sethelp);
          size_t help_len = strlen(help) + 1;

          fc_break_lines(help, LINE_BREAK);
          help = html_special_chars(help, &help_len);
          fprintf(doc, "<pre>%s</pre>\n\n", help);
          FC_FREE(help);
        }
        fprintf(doc, "<p class=\"misc\">");
        fprintf(doc, _("Level: %s.<br>"),
                _(sset_level_name(setting_level(pset))));
        fprintf(doc, _("Category: %s.<br>"),
                _(sset_category_name(setting_category(pset))));

        /* first check if the setting is locked because this is included in
         * the function setting_is_changeable() */
        if (setting_locked(pset)) {
          fprintf(doc, _("Is locked by the ruleset."));
        } else if (!setting_is_changeable(pset, &my_conn, NULL, 0)) {
          fprintf(doc, _("Can only be used in server console."));
        }

        fprintf(doc, "</p>\n");
        setting_default_name(pset, TRUE, buf, sizeof(buf));
        switch (setting_type(pset)) {
        case SST_INT:
          fprintf(doc, "\n<p class=\"bounds\">%s %d, %s %s, %s %d</p>\n",
                  _("Minimum:"), setting_int_min(pset),
                  _("Default:"), buf,
                  _("Maximum:"), setting_int_max(pset));
          break;
        case SST_ENUM:
          {
            const char *value;

            fprintf(doc, "\n<p class=\"bounds\">%s</p>",
                    _("Possible values:"));
            for (i = 0; (value = setting_enum_val(pset, i, FALSE)); i++) {
              fprintf(doc, "\n<p class=\"bounds\"><li/> %s: \"%s\"</p>",
                      value, setting_enum_val(pset, i, TRUE));
            }
          }
          break;
        case SST_BITWISE:
          {
            const char *value;

            fprintf(doc, "\n<p class=\"bounds\">%s</p>",
                    _("Possible values (option can take any number of these):"));
            for (i = 0; (value = setting_bitwise_bit(pset, i, FALSE)); i++) {
              fprintf(doc, "\n<p class=\"bounds\"><li/> %s: \"%s\"</p>",
                      value, setting_bitwise_bit(pset, i, TRUE));
            }
          }
          break;
        case SST_BOOL:
        case SST_STRING:
          break;
        case SST_COUNT:
          fc_assert(setting_type(pset) != SST_COUNT);
          break;
        }
        if (SST_INT != setting_type(pset)) {
          fprintf(doc, "\n<p class=\"bounds\">%s %s</p>\n",
                  _("Default:"), buf);
        }
        if (setting_non_default(pset)) {
          fprintf(doc, _("\n<p class=\"changed\">Value set to %s</p>\n"),
                  setting_value_name(pset, TRUE, buf, sizeof(buf)));
        }

        fprintf(doc, "%s", tag_info->item_end);
      } settings_iterate_end;
      break;

    case MANUAL_COMMANDS:
      /* TRANS: markup ... Freeciv version ... markup */
      fprintf(doc, _("%sFreeciv %s server commands%s\n\n"), tag_info->title_begin,
              VERSION_STRING, tag_info->title_end);
      for (i = 0; i < CMD_NUM; i++) {
        const struct command *cmd = command_by_number(i);

        fprintf(doc, tag_info->item_begin, "cmd", i);
        fprintf(doc, "%s%s  -  %s%s\n\n", tag_info->sect_title_begin,
                command_name(cmd), command_short_help(cmd),
                tag_info->sect_title_end);
        if (command_synopsis(cmd)) {
          char *cmdstr = fc_strdup(command_synopsis(cmd));
          size_t cmdstr_len = strlen(cmdstr) + 1;

          cmdstr = html_special_chars(cmdstr, &cmdstr_len);
          fprintf(doc, _("<table>\n<tr>\n<td valign=\"top\">"
                         "<pre>Synopsis:</pre></td>\n<td>"));
          fprintf(doc, "<pre>%s</pre></td></tr></table>", cmdstr);
          FC_FREE(cmdstr);
        }
        fprintf(doc, _("<p class=\"level\">Level: %s</p>\n"),
                cmdlevel_name(command_level(cmd)));
        {
          char *help = command_extra_help(cmd);
          if (help) {
            size_t help_len = strlen(help) + 1;

            fc_break_lines(help, LINE_BREAK);
            help = html_special_chars(help, &help_len);
            fprintf(doc, "\n");
            fprintf(doc, _("<p>Description:</p>\n\n"));
            fprintf(doc, "<pre>%s</pre>\n", help);
            FC_FREE(help);
          }
        }

        fprintf(doc, "%s", tag_info->item_end);
      }
      break;

    case MANUAL_TERRAIN:
      /* TRANS: markup ... Freeciv version ... ruleset name ... markup */
      fprintf(doc, _("%sFreeciv %s terrain help (%s)%s\n\n"), tag_info->title_begin,
              VERSION_STRING, game.control.name, tag_info->title_end);
      fprintf(doc, "<table><tr bgcolor=#9bc3d1><th colspan=2>%s</th>", _("Terrain"));
      fprintf(doc, "<th>F/P/T</th><th>%s</th>", _("Resources"));
      fprintf(doc, "<th>%s<br/>%s</th>", _("Move cost"), _("Defense bonus"));
      fprintf(doc, "<th>%s<br/>%s<br/>%s<br/>%s<br/>(%s)</th>",
              _("Irrigation"), _("Mining"), _("Transform"),
              /* xgettext:no-c-format */
              _("% of Road bonus"), _("turns"));
      fprintf(doc, "<th>%s<br/>%s</th>",
              _("Clean pollution"), _("Clean fallout"));
      ri = 0;
      if (game.control.num_road_types > 0) {
        fprintf(doc, "<th>");
      }
      extra_type_by_cause_iterate(EC_ROAD, pextra) {
        if (++ri < game.control.num_road_types) {
          fprintf(doc, "%s<br/>", extra_name_translation(pextra));
        } else {
          /* Last one */
          fprintf(doc, "%s</th>", extra_name_translation(pextra));
        }
      } extra_type_by_cause_iterate_end;
      fprintf(doc, "</tr>\n\n");
      terrain_type_iterate(pterrain) {
        struct extra_type **r;

        if (0 == strlen(terrain_rule_name(pterrain))) {
          /* Must be a disabled piece of terrain */
          continue;
        }

        fprintf(doc, "<tr><td>%s%s%s</td><td>%s</td>",
                tag_info->image_begin, pterrain->graphic_str, tag_info->image_end,
                terrain_name_translation(pterrain));
        fprintf(doc, "<td>%d/%d/%d</td>\n",
                pterrain->output[O_FOOD], pterrain->output[O_SHIELD],
                pterrain->output[O_TRADE]);

        fprintf(doc, "<td><table width=\"100%%\">\n");
        for (r = pterrain->resources; *r; r++) {
          fprintf(doc, "<tr><td>%s%s%s</td><td>%s</td>"
                  "<td align=\"right\">%d/%d/%d</td></tr>\n",
                  tag_info->image_begin, (*r)->graphic_str, tag_info->image_end,
                  extra_name_translation(*r),
                  (*r)->data.resource->output[O_FOOD],
                  (*r)->data.resource->output[O_SHIELD],
                  (*r)->data.resource->output[O_TRADE]);
        }
        fprintf(doc, "</table></td>\n");

        fprintf(doc, "<td align=\"center\">%d<br/>+%d%%</td>\n",
                pterrain->movement_cost, pterrain->defense_bonus);

        fprintf(doc, "<td><table width=\"100%%\">\n");
        if (pterrain->irrigation_result == pterrain) {
          fprintf(doc, "<tr><td>+%d F</td><td align=\"right\">(%d)</td></tr>\n",
                  pterrain->irrigation_food_incr, pterrain->irrigation_time);
        } else if (pterrain->irrigation_result == T_NONE) {
          fprintf(doc, "<tr><td>%s</td></tr>\n", _("impossible"));
        } else {
          fprintf(doc, "<tr><td>%s</td><td align=\"right\">(%d)</td></tr>\n",
                  terrain_name_translation(pterrain->irrigation_result),
                  pterrain->irrigation_time);
        }
        if (pterrain->mining_result == pterrain) {
          fprintf(doc, "<tr><td>+%d P</td><td align=\"right\">(%d)</td></tr>\n",
                  pterrain->mining_shield_incr, pterrain->mining_time);
        } else if (pterrain->mining_result == T_NONE) {
          fprintf(doc, "<tr><td>%s</td></tr>\n", _("impossible"));
        } else {
          fprintf(doc, "<tr><td>%s</td><td align=\"right\">(%d)</td></tr>\n",
                  terrain_name_translation(pterrain->mining_result),
                  pterrain->mining_time);
        }

        if (pterrain->transform_result) {
          fprintf(doc, "<tr><td>%s</td><td align=\"right\">(%d)</td></tr>\n",
                  terrain_name_translation(pterrain->transform_result),
                  pterrain->transform_time);
        } else {
          fprintf(doc, "<tr><td>-</td><td align=\"right\">(-)</td></tr>\n");
        }
        fprintf(doc, "<tr><td>%d / %d / %d</td></tr>\n</table></td>\n",
                pterrain->road_output_incr_pct[O_FOOD],
                pterrain->road_output_incr_pct[O_SHIELD],
                pterrain->road_output_incr_pct[O_TRADE]);

        fprintf(doc, "<td align=\"center\">%d / %d</td>",
                pterrain->clean_pollution_time, pterrain->clean_fallout_time);

        ri = 0;
        if (game.control.num_road_types > 0) {
          fprintf(doc, "<td>");
        }
        extra_type_by_cause_iterate(EC_ROAD, pextra) {
          if (++ri < game.control.num_road_types) {
            fprintf(doc, "%d / ", terrain_extra_build_time(pterrain, ACTIVITY_GEN_ROAD,
                                                           pextra));
          } else {
            fprintf(doc, "%d</td>", terrain_extra_build_time(pterrain, ACTIVITY_GEN_ROAD,
                                                             pextra));
          }
        } extra_type_by_cause_iterate_end;
        fprintf(doc, "</tr>\n\n");
      } terrain_type_iterate_end;

      fprintf(doc, "</table>\n");

      break;

    case MANUAL_BUILDINGS:
    case MANUAL_WONDERS:
      if (manuals == MANUAL_BUILDINGS) {
        /* TRANS: markup ... Freeciv version ... ruleset name ... markup */
        fprintf(doc, _("%sFreeciv %s buildings help (%s)%s\n\n"), tag_info->title_begin,
                VERSION_STRING, game.control.name, tag_info->title_end);
      } else {
        /* TRANS: markup ... Freeciv version ... ruleset name ... markup */
        fprintf(doc, _("%sFreeciv %s wonders help (%s)%s\n\n"), tag_info->title_begin,
                VERSION_STRING, game.control.name, tag_info->title_end);
      }

      fprintf(doc, "<table>\n<tr bgcolor=#9bc3d1><th colspan=2>%s</th>"
                   "<th>%s<br/>%s</th><th>%s<br/>%s</th><th>%s</th></tr>\n\n",
              _("Name"), _("Cost"), _("Upkeep"),
              _("Requirement"), _("Obsolete by"), _("More info"));

      improvement_iterate(pimprove) {
        char buf[64000];
        struct advance *obs_tech = NULL;

        if (!valid_improvement(pimprove)
            || is_great_wonder(pimprove) == (manuals == MANUAL_BUILDINGS)) {
          continue;
        }

        helptext_building(buf, sizeof(buf), NULL, NULL, pimprove);

        fprintf(doc, "<tr><td>%s%s%s</td><td>%s</td>\n"
                     "<td align=\"center\"><b>%d</b><br/>%d</td>\n<td>",
                tag_info->image_begin, pimprove->graphic_str, tag_info->image_end,
                improvement_name_translation(pimprove),
                pimprove->build_cost,
                pimprove->upkeep);

        requirement_vector_iterate(&pimprove->reqs, req) {
          char text[512], text2[512];
          fc_snprintf(text2, sizeof(text2),
                      /* TRANS: improvement requires a feature to be absent. */
                      req->present ? "%s" : _("no %s"),
                      VUT_NONE != req->source.kind
                      ? universal_name_translation(&req->source,
                                                   text, sizeof(text))
                      : Q_("?req:None"));
          fprintf(doc, "%s<br/>", text2);
        } requirement_vector_iterate_end;

        requirement_vector_iterate(&pimprove->obsolete_by, pobs) {
          if (pobs->source.kind == VUT_ADVANCE) {
            obs_tech = pobs->source.value.advance;
            break;
          }
        } requirement_vector_iterate_end;

        fprintf(doc, "<em>%s</em></td>\n",
                obs_tech != NULL
                ? advance_name_translation(obs_tech)
                : Q_("?tech:None"));
        fprintf(doc, "<td>%s</td>\n</tr>\n\n", buf);
      } improvement_iterate_end;
      fprintf(doc, "</table>");
      break;

    case MANUAL_GOVS:
      /* Freeciv-web uses (parts of) the government HTML output in its own
       * manual pages. */
      /* FIXME: this doesn't resemble the wiki manual at all. */
      /* TRANS: markup ... Freeciv version ... ruleset name ... markup */
      fprintf(doc, _("%sFreeciv %s governments help (%s)%s\n\n"), tag_info->title_begin,
              VERSION_STRING, game.control.name, tag_info->title_end);
      governments_iterate(pgov) {
        char buf[64000];
        fprintf(doc, tag_info->item_begin, "gov", pgov->item_number);
        fprintf(doc, "%s%s%s\n\n", tag_info->sect_title_begin,
                government_name_translation(pgov), tag_info->sect_title_end);
        fprintf(doc, tag_info->subitem_begin, "helptext");
        helptext_government(buf, sizeof(buf), NULL, NULL, pgov);
        fprintf(doc, "%s\n\n", buf);
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, "%s", tag_info->item_end);
      } governments_iterate_end;
      break;

    case MANUAL_UNITS:
      /* Freeciv-web uses (parts of) the unit type HTML output in its own
       * manual pages. */
      /* FIXME: this doesn't resemble the wiki manual at all. */
      /* TRANS: markup ... Freeciv version ... ruleset name ... markup */
      fprintf(doc, _("%sFreeciv %s unit types help (%s)%s\n\n"),
              tag_info->title_begin, VERSION_STRING, game.control.name,
              tag_info->title_end);
      unit_type_iterate(putype) {
        char buf[64000];

        fprintf(doc, tag_info->item_begin, "utype", putype->item_number);
        fprintf(doc, "%s%s%s\n\n", tag_info->sect_title_begin,
                utype_name_translation(putype), tag_info->sect_title_end);
        fprintf(doc, tag_info->subitem_begin, "cost");
        fprintf(doc,
                PL_("Cost: %d shield",
                    "Cost: %d shields",
                    utype_build_shield_cost_base(putype)),
                utype_build_shield_cost_base(putype));
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "upkeep");
        fprintf(doc, _("Upkeep: %s"),
                helptext_unit_upkeep_str(putype));
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "moves");
        fprintf(doc, _("Moves: %s"),
                move_points_text(putype->move_rate, TRUE));
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "vision");
        fprintf(doc, _("Vision: %d"),
                (int)sqrt((double)putype->vision_radius_sq));
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "attack");
        fprintf(doc, _("Attack: %d"),
                putype->attack_strength);
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "defense");
        fprintf(doc, _("Defense: %d"),
                putype->defense_strength);
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "firepower");
        fprintf(doc, _("Firepower: %d"),
                putype->firepower);
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "hitpoints");
        fprintf(doc, _("Hitpoints: %d"),
                putype->hp);
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "obsolete");
        fprintf(doc, _("Obsolete by: %s"),
                U_NOT_OBSOLETED == putype->obsoleted_by ?
                  Q_("?utype:None") :
                  utype_name_translation(putype->obsoleted_by));
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, tag_info->subitem_begin, "helptext");
        helptext_unit(buf, sizeof(buf), NULL, "", putype);
        fprintf(doc, "%s", buf);
        fprintf(doc, "%s", tag_info->subitem_end);
        fprintf(doc, "%s", tag_info->item_end);
      } unit_type_iterate_end;
      break;

    case MANUAL_TECHS:
      /* FIXME: this doesn't resemble the wiki manual at all. */
      /* TRANS: markup ... Freeciv version ... ruleset name ... markup */
      fprintf(doc, _("%sFreeciv %s tech help (%s)%s\n\n"),
              tag_info->title_begin, VERSION_STRING, game.control.name,
              tag_info->title_end);
      advance_iterate(A_FIRST, ptech) {
        if (valid_advance(ptech)) {
          char buf[64000];

          fprintf(doc, tag_info->item_begin, "tech", ptech->item_number);
          fprintf(doc, "%s%s%s\n\n", tag_info->sect_title_begin,
                  advance_name_translation(ptech), tag_info->sect_title_end);

          fprintf(doc, tag_info->subitem_begin, "helptext");
          helptext_advance(buf, sizeof(buf), NULL, "", ptech->item_number);
          fprintf(doc, "%s", buf);
          fprintf(doc, "%s", tag_info->subitem_end);

          fprintf(doc, "%s", tag_info->item_end);
        }
      } advance_iterate_end;
      break;

    case MANUAL_COUNT:
      break;

    } /* switch */

    fprintf(doc, "%s", tag_info->tail);
    fclose(doc);
    log_normal(_("Manual file %s successfully written."), filename);
  } /* manuals */

  return TRUE;
}

/**********************************************************************//**
  Entry point of whole freeciv-manual program
**************************************************************************/
int main(int argc, char **argv)
{
  int inx;
  bool showhelp = FALSE;
  bool showvers = FALSE;
  char *option = NULL;
  int retval = EXIT_SUCCESS;
  struct tag_types *tag_info = &html_tags;

  init_nls();
  registry_module_init();
  init_character_encodings(FC_DEFAULT_DATA_ENCODING, FALSE);

  /* Set the default log level. */
  srvarg.loglevel = LOG_NORMAL;

  /* parse command-line arguments... */
  inx = 1;
  while (inx < argc) {
    if ((option = get_option_malloc("--ruleset", argv, &inx, argc, TRUE))) {
      if (ruleset != NULL) {
        fc_fprintf(stderr, _("Multiple rulesets requested. Only one "
                             "ruleset at a time is supported.\n"));
      } else {
        ruleset = option;
      }
    } else if (is_option("--help", argv[inx])) {
      showhelp = TRUE;
      break;
    } else if (is_option("--version", argv[inx])) {
      showvers = TRUE;
    } else if ((option = get_option_malloc("--log", argv, &inx, argc, TRUE))) {
      srvarg.log_filename = option;
    } else if (is_option("--wiki", argv[inx])) {
      tag_info = &wiki_tags;
#ifndef FREECIV_NDEBUG
    } else if (is_option("--Fatal", argv[inx])) {
      if (inx + 1 >= argc || '-' == argv[inx + 1][0]) {
        srvarg.fatal_assertions = SIGABRT;
      } else if (str_to_int(argv[inx + 1], &srvarg.fatal_assertions)) {
        inx++;
      } else {
        fc_fprintf(stderr, _("Invalid signal number \"%s\".\n"),
                   argv[inx + 1]);
        inx++;
        showhelp = TRUE;
      }
#endif /* FREECIV_NDEBUG */
    } else if ((option = get_option_malloc("--debug", argv, &inx, argc, FALSE))) {
      if (!log_parse_level_str(option, &srvarg.loglevel)) {
        showhelp = TRUE;
        break;
      }
      free(option);
    } else {
      fc_fprintf(stderr, _("Unrecognized option: \"%s\"\n"), argv[inx]);
      exit(EXIT_FAILURE);
    }
    inx++;
  }

  init_our_capability();

  /* must be before con_log_init() */
  init_connections();
  con_log_init(srvarg.log_filename, srvarg.loglevel,
               srvarg.fatal_assertions);
  /* logging available after this point */

  /* Get common code to treat us as a tool. */
  i_am_tool();

  /* Initialize the fc_interface functions needed to generate the help
   * text. */
  fc_interface_init_tool();

  /* Initialize game with default values */
  game_init(FALSE);

  /* Set ruleset user requested in to use */
  if (ruleset != NULL) {
    sz_strlcpy(game.server.rulesetdir, ruleset);
  }

  settings_init(FALSE);

  if (showvers && !showhelp) {
    fc_fprintf(stderr, "%s \n", freeciv_name_version());
    exit(EXIT_SUCCESS);
  } else if (showhelp) {
    struct cmdhelp *help = cmdhelp_new(argv[0]);

#ifdef FREECIV_DEBUG
    cmdhelp_add(help, "d",
                  /* TRANS: "debug" is exactly what user must type, do not translate. */
                _("debug NUM"),
                _("Set debug log level (%d to %d, or %d:file1,min,max:...)"),
                LOG_FATAL, LOG_DEBUG, LOG_DEBUG);
#else  /* FREECIV_DEBUG */
    cmdhelp_add(help, "d",
                  /* TRANS: "debug" is exactly what user must type, do not translate. */
                _("debug NUM"),
                _("Set debug log level (%d to %d)"),
                LOG_FATAL, LOG_VERBOSE);
#endif /* FREECIV_DEBUG */
#ifndef FREECIV_NDEBUG
    cmdhelp_add(help, "F",
                  /* TRANS: "Fatal" is exactly what user must type, do not translate. */
                _("Fatal [SIGNAL]"),
                _("Raise a signal on failed assertion"));
#endif /* FREECIV_NDEBUG */
    cmdhelp_add(help, "h", "help",
                _("Print a summary of the options"));
    cmdhelp_add(help, "l",
                  /* TRANS: "log" is exactly what user must type, do not translate. */
                _("log FILE"),
                _("Use FILE as logfile"));
    cmdhelp_add(help, "r",
                  /* TRANS: "ruleset" is exactly what user must type, do not translate. */
                _("ruleset RULESET"),
                _("Make manual for RULESET"));
    cmdhelp_add(help, "v", "version",
                _("Print the version number"));
    cmdhelp_add(help, "w", "wiki",
                _("Write manual in wiki format"));

    /* The function below prints a header and footer for the options.
     * Furthermore, the options are sorted. */
    cmdhelp_display(help, TRUE, FALSE, TRUE);
    cmdhelp_destroy(help);

    exit(EXIT_SUCCESS);
  }

  if (!manual_command(tag_info)) {
    retval = EXIT_FAILURE;
  }

  con_log_close();
  registry_module_close();
  free_libfreeciv();
  free_nls();
  cmdline_option_values_free();

  return retval;
}

/**********************************************************************//**
  Empty function required by helpdata
**************************************************************************/
void insert_client_build_info(char *outbuf, size_t outlen)
{
  /* Nothing here */
}
