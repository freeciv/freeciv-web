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

#include "fc_prehdrs.h"

#include <string.h>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "mem.h"
#include "support.h"

#include "fc_cmdline.h"

/* get 'struct cmdline_value_list' and related functions: */
#define SPECLIST_TAG cmdline_value
#define SPECLIST_TYPE char
#include "speclist.h"

#define cmdline_value_list_iterate(vallist, pvalue) \
    TYPED_LIST_ITERATE(char *, vallist, pvalue)
#define cmdline_value_list_iterate_end LIST_ITERATE_END

static struct cmdline_value_list *cmdline_values = NULL;

/**********************************************************************//**
  Return a char* to the parameter of the option or NULL.
  *i can be increased to get next string in the array argv[].
  It is an error for the option to exist but be an empty string.
  This doesn't use log_*() because it is used before logging is set up.

  The argv strings are assumed to be in the local encoding; the returned
  string is in the internal encoding.
**************************************************************************/
char *get_option_malloc(const char *option_name,
                        char **argv, int *i, int argc,
                        bool gc)
{
  int len = strlen(option_name);

  if (gc && cmdline_values == NULL) {
    cmdline_values = cmdline_value_list_new();
  }

  if (strcmp(option_name, argv[*i]) == 0
      || (strncmp(option_name, argv[*i], len) == 0 && argv[*i][len] == '=')
      || strncmp(option_name + 1, argv[*i], 2) == 0) {
    char *opt = argv[*i] + (argv[*i][1] != '-' ? 0 : len);
    char *ret;

    if (*opt == '=') {
      opt++;
    } else {
      if (*i < argc - 1) {
        (*i)++;
        opt = argv[*i];
        if (strlen(opt) == 0) {
          fc_fprintf(stderr, _("Empty argument for \"%s\".\n"), option_name);
          exit(EXIT_FAILURE);
        }
      } else {
        fc_fprintf(stderr, _("Missing argument for \"%s\".\n"), option_name);
        exit(EXIT_FAILURE);
     }
   }

    ret = local_to_internal_string_malloc(opt);

    if (gc) {
      cmdline_value_list_append(cmdline_values, ret);
    }

    return ret;
 }

  return NULL;
}

/**********************************************************************//**
  Free memory allocated for commandline option values.
**************************************************************************/
void cmdline_option_values_free(void)
{
  if (cmdline_values != NULL) {
    cmdline_value_list_iterate(cmdline_values, pval) {
      free(pval);
    } cmdline_value_list_iterate_end;

    cmdline_value_list_destroy(cmdline_values);
  }
}

/**********************************************************************//**
  Is option some form of option_name. option_name must be
  full length long version such as "--help"
**************************************************************************/
bool is_option(const char *option_name,char *option)
{
  return (strcmp(option_name, option) == 0
          || strncmp(option_name + 1, option, 2) == 0);
}

/**********************************************************************//**
  Like strcspn but also handles quotes, i.e. *reject chars are
  ignored if they are inside single or double quotes.
**************************************************************************/
static size_t fc_strcspn(const char *s, const char *reject)
{
  bool in_single_quotes = FALSE, in_double_quotes = FALSE;
  size_t i, len = strlen(s);

  for (i = 0; i < len; i++) {
    if (s[i] == '"' && !in_single_quotes) {
      in_double_quotes = !in_double_quotes;
    } else if (s[i] == '\'' && !in_double_quotes) {
      in_single_quotes = !in_single_quotes;
    }

    if (in_single_quotes || in_double_quotes) {
      continue;
    }

    if (strchr(reject, s[i])) {
      break;
    }
  }

  return i;
}

/**********************************************************************//**
  Splits the string into tokens. The individual tokens are
  returned. The delimiterset can freely be chosen.

  i.e. "34 abc 54 87" with a delimiterset of " " will yield
       tokens={"34", "abc", "54", "87"}

  Part of the input string can be quoted (single or double) to embedded
  delimiter into tokens. For example,
    command 'a name' hard "1,2,3,4,5" 99
    create 'Mack "The Knife"'
  will yield 5 and 2 tokens respectively using the delimiterset " ,".

  Tokens which aren't used aren't modified (and memory is not
  allocated). If the string would yield more tokens only the first
  num_tokens are extracted.

  The user has the responsiblity to free the memory allocated by
  **tokens using free_tokens().
**************************************************************************/
int get_tokens(const char *str, char **tokens, size_t num_tokens,
               const char *delimiterset)
{
  int token;

  fc_assert_ret_val(NULL != str, -1);

  for (token = 0; token < num_tokens && *str != '\0'; token++) {
    size_t len, padlength = 0;

    /* skip leading delimiters */
    str += strspn(str, delimiterset);

    len = fc_strcspn(str, delimiterset);

    /* strip start/end quotes if they exist */
    if (len >= 2) {
      if ((str[0] == '"' && str[len - 1] == '"')
          || (str[0] == '\'' && str[len - 1] == '\'')) {
        len -= 2;
        padlength = 1; /* to set the string past the end quote */
        str++;
     }
   }

   tokens[token] = fc_malloc(len + 1);
   (void) fc_strlcpy(tokens[token], str, len + 1); /* adds the '\0' */

   str += len + padlength;
  }

  return token;
}

/**********************************************************************//**
  Frees a set of tokens created by get_tokens().
**************************************************************************/
void free_tokens(char **tokens, size_t ntokens)
{
  size_t i;

  for (i = 0; i < ntokens; i++) {
    if (tokens[i]) {
      free(tokens[i]);
    }
  }
}
