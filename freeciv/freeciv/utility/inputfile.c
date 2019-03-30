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

/***********************************************************************
  A low-level object for reading a registry-format file.
  original author: David Pfitzner <dwp@mso.anu.edu.au>

  This module implements an object which is useful for reading/parsing
  a file in the registry format of registry.c.  It takes care of the
  low-level file-reading details, and provides functions to return
  specific "tokens" from the file.  Probably this should really use
  higher-level tools... (flex/lex bison/yacc?)

  When the user tries to read a token, we return a (const char*)
  pointing to some data if the token was found, or NULL otherwise.
  The data pointed to should not be modified.  The retuned pointer
  is valid _only_ until another inputfile is performed.  (So should
  be used immediately, or fc_strdup-ed etc.)
  
  The tokens recognised are as follows:
  (Single quotes are delimiters used here, but are not part of the
  actual tokens/strings.)
  Most tokens can be preceeded by optional whitespace; exceptions
  are section_name and entry_name.

  section_name:  '[foo]'
  returned token: 'foo'
  
  entry_name:  'foo =' (optional whitespace allowed before '=')
  returned token: 'foo'
  
  end_of_line: newline, or optional '#' or ';' (comment characters)
               followed by any other chars, then newline.
  returned token: should not be used except to check non-NULL.
  
  table_start: '{'  
  returned token: should not be used except to check non-NULL.
  
  table_end: '}'  
  returned token: should not be used except to check non-NULL.

  comma:  literal ','  
  returned token: should not be used except to check non-NULL.
  
  value:  a signed integer, or a double-quoted string, or a
          gettext-marked double quoted string.  Strings _may_ contain
	  raw embedded newlines, and escaped doublequotes, or \.
	  eg:  '123', '-999', '"foo"', '_("foo")'
  returned token: string containing number, for numeric, or string
          starting at first doublequote for strings, but ommiting
	  trailing double-quote.  Note this does _not_ translate
	  escaped doublequotes etc back to normal.

***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "ioz.h"
#include "log.h"
#include "mem.h"
#include "shared.h"		/* TRUE, FALSE */
#include "support.h"

#include "inputfile.h"

#define INF_DEBUG_FOUND     FALSE
#define INF_DEBUG_NOT_FOUND FALSE

#define INF_MAGIC (0xabdc0132)	/* arbitrary */

struct inputfile {
  unsigned int magic;		/* memory check */
  char *filename;		/* filename as passed to fopen */
  fz_FILE *fp;			/* read from this */
  bool at_eof;			/* flag for end-of-file */
  struct astring cur_line;	/* data from current line */
  int cur_line_pos;		/* position in current line */
  int line_num;			/* line number from file in cur_line */
  struct astring token;		/* data returned to user */
  struct astring partial;	/* used in accumulating multi-line strings;
				   used only in get_token_value, but put
				   here so it gets freed when file closed */
  datafilename_fn_t datafn;	/* function like datafilename(); use a
				   function pointer just to keep this
				   inputfile module "generic" */
  bool in_string;		/* set when reading multi-line strings,
				   to know not to handle *include at start
				   of line as include mechanism */
  int string_start_line;	/* when in_string is true, this is the
				   start line of current string */
  struct inputfile *included_from; /* NULL for toplevel file, otherwise
				      points back to files which this one
				      has been included from */
};

/* A function to get a specific token type: */
typedef const char *(*get_token_fn_t)(struct inputfile *inf);

static const char *get_token_section_name(struct inputfile *inf);
static const char *get_token_entry_name(struct inputfile *inf);
static const char *get_token_eol(struct inputfile *inf);
static const char *get_token_table_start(struct inputfile *inf);
static const char *get_token_table_end(struct inputfile *inf);
static const char *get_token_comma(struct inputfile *inf);
static const char *get_token_value(struct inputfile *inf);

static struct {
  const char *name;
  get_token_fn_t func;
}
tok_tab[INF_TOK_LAST] =
{
  { "section_name", get_token_section_name },
  { "entry_name",   get_token_entry_name },
  { "end_of_line",  get_token_eol },
  { "table_start",  get_token_table_start },
  { "table_end",    get_token_table_end },
  { "comma",        get_token_comma },
  { "value",        get_token_value },
};

static bool read_a_line(struct inputfile *inf);

#define inf_log(inf, level, message, ...)                                   \
  if (log_do_output_for_level(level)) {                                     \
    do_log(__FILE__, __FUNCTION__, __FC_LINE__, FALSE, level, "%s",         \
           inf_log_str(inf, message, ## __VA_ARGS__));                      \
  }
#define inf_warn(inf, message)                                              \
  inf_log(inf, LOG_NORMAL, "%s", message);

/*******************************************************************//**
  Return true if c is a 'comment' character: '#' or ';'
***********************************************************************/
static bool is_comment(int c)
{
  return (c == '#' || c == ';');
}

/*******************************************************************//**
  Set values to zeros; should have free'd/closed everything before
  this if appropriate.
***********************************************************************/
static void init_zeros(struct inputfile *inf)
{
  fc_assert_ret(NULL != inf);
  inf->magic = INF_MAGIC;
  inf->filename = NULL;
  inf->fp = NULL;
  inf->datafn = NULL;
  inf->included_from = NULL;
  inf->line_num = inf->cur_line_pos = 0;
  inf->at_eof = inf->in_string = FALSE;
  inf->string_start_line = 0;
  astr_init(&inf->cur_line);
  astr_init(&inf->token);
  astr_init(&inf->partial);
}

/*******************************************************************//**
  Check sensible values for an opened inputfile.
***********************************************************************/
static bool inf_sanity_check(struct inputfile *inf)
{
  fc_assert_ret_val(NULL != inf, FALSE);
  fc_assert_ret_val(INF_MAGIC == inf->magic, FALSE);
  fc_assert_ret_val(NULL != inf->fp, FALSE);
  fc_assert_ret_val(0 <= inf->line_num, FALSE);
  fc_assert_ret_val(0 <= inf->cur_line_pos, FALSE);
  fc_assert_ret_val(FALSE == inf->at_eof
                    || TRUE == inf->at_eof, FALSE);
  fc_assert_ret_val(FALSE == inf->in_string
                    || TRUE == inf->in_string, FALSE);

#ifdef FREECIV_DEBUG
  fc_assert_ret_val(0 <= inf->string_start_line, FALSE);
  if (inf->included_from && !inf_sanity_check(inf->included_from)) {
    return FALSE;
  }
#endif /* FREECIV_DEBUG */

  return TRUE;
}

/*******************************************************************//**
  Return the filename the inputfile was loaded as, or "(anonymous)"
  if this inputfile was loaded from a stream rather than from a file.
***********************************************************************/
static const char *inf_filename(struct inputfile *inf)
{
  if (inf->filename) {
    return inf->filename;
  } else {
    return "(anonymous)";
  }
}

/*******************************************************************//**
  Open the file, and return an allocated, initialized structure.
  Returns NULL if the file could not be opened.
***********************************************************************/
struct inputfile *inf_from_file(const char *filename,
                                datafilename_fn_t datafn)
{
  struct inputfile *inf;
  fz_FILE *fp;

  fc_assert_ret_val(NULL != filename, NULL);
  fc_assert_ret_val(0 < strlen(filename), NULL);
  fp = fz_from_file(filename, "r", -1, 0);
  if (!fp) {
    return NULL;
  }
  log_debug("inputfile: opened \"%s\" ok", filename);
  inf = inf_from_stream(fp, datafn);
  inf->filename = fc_strdup(filename);
  return inf;
}

/*******************************************************************//**
  Open the stream, and return an allocated, initialized structure.
  Returns NULL if the file could not be opened.
***********************************************************************/
struct inputfile *inf_from_stream(fz_FILE *stream, datafilename_fn_t datafn)
{
  struct inputfile *inf;

  fc_assert_ret_val(NULL != stream, NULL);
  inf = fc_malloc(sizeof(*inf));
  init_zeros(inf);

  inf->filename = NULL;
  inf->fp = stream;
  inf->datafn = datafn;

  log_debug("inputfile: opened \"%s\" ok", inf_filename(inf));
  return inf;
}


/*******************************************************************//**
  Close the file and free associated memory, but don't recurse
  included_from files, and don't free the actual memory where
  the inf record is stored (ie, the memory where the users pointer
  points to).  This is used when closing an included file.
***********************************************************************/
static void inf_close_partial(struct inputfile *inf)
{
  fc_assert_ret(inf_sanity_check(inf));

  log_debug("inputfile: sub-closing \"%s\"", inf_filename(inf));

  if (fz_ferror(inf->fp) != 0) {
    log_error("Error before closing %s: %s", inf_filename(inf),
              fz_strerror(inf->fp));
    fz_fclose(inf->fp);
    inf->fp = NULL;
  }
  else if (fz_fclose(inf->fp) != 0) {
    log_error("Error closing %s", inf_filename(inf));
  }
  if (inf->filename) {
    free(inf->filename);
  }
  inf->filename = NULL;
  astr_free(&inf->cur_line);
  astr_free(&inf->token);
  astr_free(&inf->partial);

  /* assign zeros for safety if accidently re-use etc: */
  init_zeros(inf);
  inf->magic = ~INF_MAGIC;

  log_debug("inputfile: sub-closed ok");
}

/*******************************************************************//**
  Close the file and free associated memory, included any partially
  recursed included files, and the memory allocated for 'inf' itself.
  Should only be used on an actually open inputfile.
  After this, the pointer should not be used.
***********************************************************************/
void inf_close(struct inputfile *inf)
{
  fc_assert_ret(inf_sanity_check(inf));

  log_debug("inputfile: closing \"%s\"", inf_filename(inf));
  if (inf->included_from) {
    inf_close(inf->included_from);
  }
  inf_close_partial(inf);
  free(inf);
  log_debug("inputfile: closed ok");
}

/*******************************************************************//**
  Return TRUE if have data for current line.
***********************************************************************/
static bool have_line(struct inputfile *inf)
{
  fc_assert_ret_val(inf_sanity_check(inf), FALSE);
  return !astr_empty(&inf->cur_line);
}

/*******************************************************************//**
  Return TRUE if current pos is at end of current line.
***********************************************************************/
static bool at_eol(struct inputfile *inf)
{
  fc_assert_ret_val(inf_sanity_check(inf), TRUE);
  fc_assert_ret_val(inf->cur_line_pos <= astr_len(&inf->cur_line), TRUE);
  return (inf->cur_line_pos >= astr_len(&inf->cur_line));
}

/*******************************************************************//**
  Return TRUE if current pos is at end of file.
***********************************************************************/
bool inf_at_eof(struct inputfile *inf)
{
  fc_assert_ret_val(inf_sanity_check(inf), TRUE);
  return inf->at_eof;
}

/*******************************************************************//**
  Check for an include command, which is an isolated line with:
     *include "filename"
  If a file is included via this mechanism, returns 1, and sets up
  data appropriately: (*inf) will now correspond to the new file,
  which is opened but no data read, and inf->included_from is set
  to newly malloced memory which corresponds to the old file.
***********************************************************************/
static bool check_include(struct inputfile *inf)
{
  const char *include_prefix = "*include";
  static size_t len = 0;
  size_t bare_name_len;
  char *bare_name;
  const char *c, *bare_name_start, *full_name;
  struct inputfile *new_inf, temp;

  if (len == 0) {
    len = strlen(include_prefix);
  }
  fc_assert_ret_val(inf_sanity_check(inf), FALSE);
  if (inf->in_string || astr_len(&inf->cur_line) <= len
      || inf->cur_line_pos > 0) {
    return FALSE;
  }
  if (strncmp(astr_str(&inf->cur_line), include_prefix, len) != 0) {
    return FALSE;
  }
  /* from here, the include-line must be well formed */
  /* keep inf->cur_line_pos accurate just so error messages are useful */

  /* skip any whitespace: */
  inf->cur_line_pos = len;
  c = astr_str(&inf->cur_line) + len;
  while (*c != '\0' && fc_isspace(*c)){
    c++;
  }

  if (*c != '\"') {
    inf_log(inf, LOG_ERROR, 
            "Did not find opening doublequote for '*include' line");
    return FALSE;
  }
  c++;
  inf->cur_line_pos = c - astr_str(&inf->cur_line);

  bare_name_start = c;
  while (*c != '\0' && *c != '\"') c++;
  if (*c != '\"') {
    inf_log(inf, LOG_ERROR, 
            "Did not find closing doublequote for '*include' line");
    return FALSE;
  }
  c++;
  bare_name_len = c - bare_name_start;
  bare_name = fc_malloc(bare_name_len);
  strncpy(bare_name, bare_name_start, bare_name_len - 1);
  bare_name[bare_name_len - 1] = '\0';
  inf->cur_line_pos = c - astr_str(&inf->cur_line);

  /* check rest of line is well-formed: */
  while (*c != '\0' && fc_isspace(*c) && !is_comment(*c)) {
    c++;
  }
  if (!(*c == '\0' || is_comment(*c))) {
    inf_log(inf, LOG_ERROR, "Junk after filename for '*include' line");
    return FALSE;
  }
  inf->cur_line_pos = astr_len(&inf->cur_line) - 1;

  full_name = inf->datafn(bare_name);
  if (!full_name) {
    log_error("Could not find included file \"%s\"", bare_name);
    free(bare_name);
    return FALSE;
  }
  free(bare_name);

  /* avoid recursion: (first filename may not have the same path,
   * but will at least stop infinite recursion) */
  {
    struct inputfile *inc = inf;
    do {
      if (inc->filename && strcmp(full_name, inc->filename) == 0) {
        log_error("Recursion trap on '*include' for \"%s\"", full_name);
        return FALSE;
      }
    } while ((inc = inc->included_from));
  }

  new_inf = inf_from_file(full_name, inf->datafn);

  /* Swap things around so that memory pointed to by inf (user pointer,
     and pointer in calling functions) contains the new inputfile,
     and newly allocated memory for new_inf contains the old inputfile.
     This is pretty scary, lets hope it works...
  */
  temp = *new_inf;
  *new_inf = *inf;
  *inf = temp;
  inf->included_from = new_inf;
  return TRUE;
}

/*******************************************************************//**
  Read a new line into cur_line.
  Increments line_num and cur_line_pos.
  Returns 0 if didn't read or other problem: treat as EOF.
  Strips newline from input.
***********************************************************************/
static bool read_a_line(struct inputfile *inf)
{
  struct astring *line;
  char *ret;
  int pos;

  fc_assert_ret_val(inf_sanity_check(inf), FALSE);

  if (inf->at_eof) {
    return FALSE;
  }

  /* abbreviation: */
  line = &inf->cur_line;

  /* minimum initial line length: */
  astr_reserve(line, 80);
  astr_clear(line);
  pos = 0;

  /* Read until we get a full line:
   * At start of this loop, pos is index to trailing null
   * (or first position) in line.
   */
  for (;;) {
    ret = fz_fgets((char *) astr_str(line) + pos,
                   astr_capacity(line) - pos, inf->fp);

    if (!ret) {
      /* fgets failed */
      if (pos > 0) {
        inf_log(inf, LOG_ERROR, _("End-of-file not in line of its own"));
      }
      inf->at_eof = TRUE;
      if (inf->in_string) {
        /* Note: Don't allow multi-line strings to cross "include"
         * boundaries */
        inf_log(inf, LOG_ERROR, "Multi-line string went to end-of-file");
        return FALSE;
      }
      break;
    }

    /* Cope with \n\r line endings if not caught by library:
     * strip off any leading \r */
    if (0 == pos && 0 < astr_len(line) && astr_str(line)[0] == '\r') {
      memmove((char *)astr_str(line), astr_str(line)+1, astr_len(line));
    }

    pos = astr_len(line);

    if (0 < pos && astr_str(line)[pos - 1] == '\n') {
      int end;
      /* Cope with \r\n line endings if not caught by library:
       * strip off any trailing \r */
      if (1 < pos && astr_str(line)[pos - 2] == '\r') {
        end = pos - 2;
      } else {
        end = pos - 1;
      }
      *((char *) astr_str(line) + end) = '\0';
      break;
    }
    astr_reserve(line, pos * 2);
  }

  if (!inf->at_eof) {
    inf->line_num++;
    inf->cur_line_pos = 0;

    if (check_include(inf)) {
      return read_a_line(inf);
    }
    return TRUE;
  } else {
    astr_clear(line);
    if (inf->included_from) {
      /* Pop the include, and get next line from file above instead. */
      struct inputfile *inc = inf->included_from;
      inf_close_partial(inf);
      *inf = *inc;    /* so the user pointer in still valid
                       * (and inf pointers in calling functions) */
      free(inc);
      return read_a_line(inf);
    }
    return FALSE;
  }
}

/*******************************************************************//**
  Return a detailed log message, including information on current line
  number etc. Message can be NULL: then just logs information on where
  we are in the file.
***********************************************************************/
char *inf_log_str(struct inputfile *inf, const char *message, ...)
{
  va_list args;
  static char str[512];

  fc_assert_ret_val(inf_sanity_check(inf), NULL);

  if (message) {
    va_start(args, message);
    fc_vsnprintf(str, sizeof(str), message, args);
    va_end(args);
    sz_strlcat(str, "\n");
  } else {
    str[0] = '\0';
  }

  cat_snprintf(str, sizeof(str), "  file \"%s\", line %d, pos %d%s",
               inf_filename(inf), inf->line_num, inf->cur_line_pos,
               (inf->at_eof ? ", EOF" : ""));

  if (!astr_empty(&inf->cur_line)) {
    cat_snprintf(str, sizeof(str), "\n  looking at: '%s'",
                 astr_str(&inf->cur_line) + inf->cur_line_pos);
  }
  if (inf->in_string) {
    cat_snprintf(str, sizeof(str),
                 "\n  processing string starting at line %d",
                 inf->string_start_line);
  }
  while ((inf = inf->included_from)) {  /* local pointer assignment */
    cat_snprintf(str, sizeof(str), "\n  included from file \"%s\", line %d",
                 inf_filename(inf), inf->line_num);
  }

  return str;
}

/*******************************************************************//**
  Returns token of given type from given inputfile.
***********************************************************************/
const char *inf_token(struct inputfile *inf, enum inf_token_type type)
{
  const char *c;
  const char *name;
  get_token_fn_t func;

  fc_assert_ret_val(inf_sanity_check(inf), NULL);
  fc_assert_ret_val(INF_TOK_FIRST <= type && INF_TOK_LAST > type, NULL);

  name = tok_tab[type].name ? tok_tab[type].name : "(unnamed)";
  func = tok_tab[type].func;

  if (!func) {
    log_error("token type %d (%s) not supported yet", type, name);
    c = NULL;
  } else {
    while (!have_line(inf) && read_a_line(inf)) {
      /* Nothing. */
    }
    if (!have_line(inf)) {
      c = NULL;
    } else {
      c = func(inf);
    }
  }
  if (c && INF_DEBUG_FOUND) {
    log_debug("inputfile: found %s '%s'", name, astr_str(&inf->token));
  }
  return c;
}

/*******************************************************************//**
  Read as many tokens of specified type as possible, discarding
  the results; returns number of such tokens read and discarded.
***********************************************************************/
int inf_discard_tokens(struct inputfile *inf, enum inf_token_type type)
{
  int count = 0;

  while (inf_token(inf, type)) {
    count++;
  }

  return count;
}

/*******************************************************************//**
  Returns section name in current position of inputfile. Returns NULL
  if there is no section name on that position. Sets inputfile position
  after section name.
***********************************************************************/
static const char *get_token_section_name(struct inputfile *inf)
{
  const char *c, *start;

  fc_assert_ret_val(have_line(inf), NULL);

  c = astr_str(&inf->cur_line) + inf->cur_line_pos;
  if (*c++ != '[') {
    return NULL;
  }
  start = c;
  while (*c != '\0' && *c != ']') {
    c++;
  }
  if (*c != ']') {
    return NULL;
  }
  *((char *) c) = '\0'; /* Tricky. */
  astr_set(&inf->token, "%s", start);
  *((char *) c) = ']';  /* Revert. */
  inf->cur_line_pos = c + 1 - astr_str(&inf->cur_line);
  return astr_str(&inf->token);
}

/*******************************************************************//**
  Returns next entry name from inputfile. Skips white spaces and
  comments. Sets inputfile position after entry name.
***********************************************************************/
static const char *get_token_entry_name(struct inputfile *inf)
{
  const char *c, *start, *end;
  char trailing;

  fc_assert_ret_val(have_line(inf), NULL);

  c = astr_str(&inf->cur_line) + inf->cur_line_pos;
  while (*c != '\0' && fc_isspace(*c)) {
    c++;
  }
  if (*c == '\0') {
    return NULL;
  }
  start = c;
  while (*c != '\0' && !fc_isspace(*c) && *c != '=' && !is_comment(*c)) {
    c++;
  }
  if (!(*c != '\0' && (fc_isspace(*c) || *c == '='))) {
    return NULL;
  }
  end = c;
  while (*c != '\0' && *c != '=' && !is_comment(*c)) {
    c++;
  }
  if (*c != '=') {
    return NULL;
  }
  trailing = *end;
  *((char *) end) = '\0';       /* Tricky. */
  astr_set(&inf->token, "%s", start);
  *((char *) end) = trailing;   /* Revert. */
  inf->cur_line_pos = c + 1 - astr_str(&inf->cur_line);
  return astr_str(&inf->token);
}

/*******************************************************************//**
  If inputfile is at end-of-line, frees current line, and returns " ".
  If there is still something on that line, returns NULL. 
***********************************************************************/
static const char *get_token_eol(struct inputfile *inf)
{
  const char *c;

  fc_assert_ret_val(have_line(inf), NULL);

  if (!at_eol(inf)) {
    c = astr_str(&inf->cur_line) + inf->cur_line_pos;
    while (*c != '\0' && fc_isspace(*c)) {
      c++;
    }
    if (*c != '\0' && !is_comment(*c)) {
      return NULL;
    }
  }

  /* finished with this line: say that we don't have it any more: */
  astr_clear(&inf->cur_line);
  inf->cur_line_pos = 0;

  astr_set(&inf->token, " ");
  return astr_str(&inf->token);
}

/*******************************************************************//**
  Get a flag token of a single character, with optional
  preceeding whitespace.
***********************************************************************/
static const char *get_token_white_char(struct inputfile *inf,
                                        char target)
{
  const char *c;

  fc_assert_ret_val(have_line(inf), NULL);

  c = astr_str(&inf->cur_line) + inf->cur_line_pos;
  while (*c != '\0' && fc_isspace(*c)) {
    c++;
  }
  if (*c != target) {
    return NULL;
  }
  inf->cur_line_pos = c + 1 - astr_str(&inf->cur_line);
  astr_set(&inf->token, "%c", target);
  return astr_str(&inf->token);
}

/*******************************************************************//**
  Get flag token for table start, or NULL if that is not next token.
***********************************************************************/
static const char *get_token_table_start(struct inputfile *inf)
{
  return get_token_white_char(inf, '{');
}

/*******************************************************************//**
  Get flag token for table end, or NULL if that is not next token.
***********************************************************************/
static const char *get_token_table_end(struct inputfile *inf)
{
  return get_token_white_char(inf, '}');
}

/*******************************************************************//**
  Get flag token comma, or NULL if that is not next token.
***********************************************************************/
static const char *get_token_comma(struct inputfile *inf)
{
  return get_token_white_char(inf, ',');
}

/*******************************************************************//**
  This one is more complicated; note that it may read in multiple lines.
***********************************************************************/
static const char *get_token_value(struct inputfile *inf)
{
  struct astring *partial;
  const char *c, *start;
  char trailing;
  bool has_i18n_marking = FALSE;
  char border_character = '\"';

  fc_assert_ret_val(have_line(inf), NULL);

  c = astr_str(&inf->cur_line) + inf->cur_line_pos;
  while (*c != '\0' && fc_isspace(*c)) {
    c++;
  }
  if (*c == '\0') {
    return NULL;
  }

  if (*c == '-' || *c == '+' || fc_isdigit(*c)) {
    /* a number: */
    start = c++;
    while (*c != '\0' && fc_isdigit(*c)) {
      c++;
    }
    if (*c == '.') {
      /* Float maybe */
      c++;
      while (*c != '\0' && fc_isdigit(*c)) {
        c++;
      }
    }
    /* check that the trailing stuff is ok: */
    if (!(*c == '\0' || *c == ',' || fc_isspace(*c) || is_comment(*c))) {
      return NULL;
    }
    /* If its a comma, we don't want to obliterate it permanently,
     * so remember it: */
    trailing = *c;
    *((char *) c) = '\0';       /* Tricky. */

    inf->cur_line_pos = c - astr_str(&inf->cur_line);
    astr_set(&inf->token, "%s", start);

    *((char *) c) = trailing;   /* Revert. */
    return astr_str(&inf->token);
  }

  /* allow gettext marker: */
  if (*c == '_' && *(c + 1) == '(') {
    has_i18n_marking = TRUE;
    c += 2;
    while (*c != '\0' && fc_isspace(*c)) {
      c++;
    }
    if (*c == '\0') {
      return NULL;
    }
  }

  border_character = *c;

  if (border_character == '*') {
    const char *rfname;
    fz_FILE *fp;
    bool eof;
    int pos;

    c++;

    start = c;
    while (*c != '*') {
      if (*c == '\0' || *c == '\n') {
        return NULL;
      }
      c++;
    }
    c++;
    /* check that the trailing stuff is ok: */
    if (!(*c == '\0' || *c == ',' || fc_isspace(*c) || is_comment(*c))) {
      return NULL;
    }
    /* We don't want to obliterate ending '*' permanently,
     * so remember it: */
    trailing = *(c - 1);
    *((char *) (c - 1)) = '\0';     /* Tricky. */

    rfname = fileinfoname(get_data_dirs(), start);
    if (rfname == NULL) {
      inf_log(inf, LOG_ERROR, 
              _("Cannot find stringfile \"%s\"."), start);
      *((char *) c) = trailing; /* Revert. */
      return NULL;
    }
    *((char *) c) = trailing; /* Revert. */
    fp = fz_from_file(rfname, "r", -1, 0);
    if (!fp) {
      inf_log(inf, LOG_ERROR,
              _("Cannot open stringfile \"%s\"."), rfname);
      return NULL;
    }
    log_debug("Stringfile \"%s\" opened ok", start);
    *((char *) (c - 1)) = trailing; /* Revert. */
    astr_set(&inf->token, "*"); /* Mark as a string read from a file */

    eof = FALSE;
    pos = 1; /* Past 'filestring' marker */
    while (!eof) {
      char *ret;

      ret = fz_fgets((char *) astr_str(&inf->token) + pos,
                     astr_capacity(&inf->token) - pos, fp);
      if (ret == NULL) {
        eof = TRUE;
      } else {
        pos = astr_len(&inf->token);
        astr_reserve(&inf->token, pos + 200);
      }
    }

    fz_fclose(fp);

    inf->cur_line_pos = c + 1 - astr_str(&inf->cur_line);

    return astr_str(&inf->token);
  } else if (border_character != '\"'
      && border_character != '\''
      && border_character != '$') {
    /* A one-word string: maybe FALSE or TRUE. */
    start = c;
    while (fc_isalnum(*c)) {
      c++;
    }
    /* check that the trailing stuff is ok: */
    if (!(*c == '\0' || *c == ',' || fc_isspace(*c) || is_comment(*c))) {
      return NULL;
    }
    /* If its a comma, we don't want to obliterate it permanently,
     * so remember it: */
    trailing = *c;
    *((char *) c) = '\0';       /* Tricky. */

    inf->cur_line_pos = c - astr_str(&inf->cur_line);
    astr_set(&inf->token, "%s", start);

    *((char *) c) = trailing;   /* Revert. */
    return astr_str(&inf->token);
  }

  /* From here, we know we have a string, we just have to find the
     trailing (un-escaped) double-quote.  We read in extra lines if
     necessary to find it.  If we _don't_ find the end-of-string
     (that is, we come to end-of-file), we return NULL, but we
     leave the file in at_eof, and don't try to back-up to the
     current point.  (That would be more difficult, and probably
     not necessary: at that point we probably have a malformed
     string/file.)

     As we read extra lines, the string value from previous
     lines is placed in partial.
  */

  /* prepare for possibly multi-line string: */
  inf->string_start_line = inf->line_num;
  inf->in_string = TRUE;

  partial = &inf->partial;      /* abbreviation */
  astr_clear(partial);

  start = c++;                  /* start includes the initial \", to
                                 * distinguish from a number */
  for (;;) {
    while (*c != '\0' && *c != border_character) {
      /* skip over escaped chars, including backslash-doublequote,
       * and backslash-backslash: */
      if (*c == '\\' && *(c + 1) != '\0') {
        c++;
      }
      c++;
    }

    if (*c == border_character) {
      /* Found end of string */
      break;
    }

    astr_add(partial, "%s\n", start);

    if (!read_a_line(inf)) {
      /* shouldn't happen */
      inf_log(inf, LOG_ERROR, 
              "Bad return for multi-line string from read_a_line");
      return NULL;
    }
    c = start = astr_str(&inf->cur_line);
  }

  /* found end of string */
  trailing = *c;
  *((char *) c) = '\0';         /* Tricky. */

  inf->cur_line_pos = c + 1 - astr_str(&inf->cur_line);
  astr_set(&inf->token, "%s%s", astr_str(partial), start);

  *((char *) c) = trailing;     /* Revert. */

  /* check gettext tag at end: */
  if (has_i18n_marking) {
    if (*++c == ')') {
      inf->cur_line_pos++;
    } else {
      inf_warn(inf, "Missing end of i18n string marking");
    }
  }
  inf->in_string = FALSE;
  return astr_str(&inf->token);
}
