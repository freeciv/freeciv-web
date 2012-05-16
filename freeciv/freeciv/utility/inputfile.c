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

/********************************************************************** 
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
  be used immediately, or mystrdup-ed etc.)
  
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
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "astring.h"
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
  struct astring cur_line;	/* data from current line, or .n==0 if
				   have not yet read in the current line */
  struct astring copy_line;	/* original cur_line (sometimes insert nulls
				   in cur_line for processing) */
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
static void inf_warn(struct inputfile *inf, const char *message);

/********************************************************************** 
  Return true if c is a 'comment' character: '#' or ';'
***********************************************************************/
static bool is_comment(int c)
{
  return (c == '#' || c == ';');
}

/********************************************************************** 
  Set values to zeros; should have free'd/closed everything before
  this if appropriate.
***********************************************************************/
static void init_zeros(struct inputfile *inf)
{
  assert(inf != NULL);
  inf->magic = INF_MAGIC;
  inf->filename = NULL;
  inf->fp = NULL;
  inf->datafn = NULL;
  inf->included_from = NULL;
  inf->line_num = inf->cur_line_pos = 0;
  inf->at_eof = inf->in_string = FALSE;
  inf->string_start_line = 0;
  astr_init(&inf->cur_line);
  astr_init(&inf->copy_line);
  astr_init(&inf->token);
  astr_init(&inf->partial);
}

/********************************************************************** 
  Check sensible values for an opened inputfile.
***********************************************************************/
static void assert_sanity(struct inputfile *inf)
{
  assert(inf != NULL);
  assert(inf->magic==INF_MAGIC);
  assert(inf->fp != NULL);
  assert(inf->line_num >= 0);
  assert(inf->cur_line_pos >= 0);
  assert(inf->at_eof == FALSE || inf->at_eof == TRUE);
  assert(inf->in_string == FALSE || inf->in_string == TRUE);
#ifdef DEBUG
  assert(inf->string_start_line >= 0);
  assert(inf->cur_line.n >= 0);
  assert(inf->copy_line.n >= 0);
  assert(inf->token.n >= 0);
  assert(inf->partial.n >= 0);
  assert(inf->cur_line.n_alloc >= 0);
  assert(inf->copy_line.n_alloc >= 0);
  assert(inf->token.n_alloc >= 0);
  assert(inf->partial.n_alloc >= 0);
  if(inf->included_from) {
    assert_sanity(inf->included_from);
  }
#endif
}

/**************************************************************************
  Return the filename the inputfile was loaded as, or "(anonymous)"
  if this inputfile was loaded from a stream rather than from a file.
**************************************************************************/
static const char *inf_filename(struct inputfile *inf)
{
  if (inf->filename) {
    return inf->filename;
  } else {
    return "(anonymous)";
  }
}

/********************************************************************** 
  Open the file, and return an allocated, initialized structure.
  Returns NULL if the file could not be opened.
***********************************************************************/
struct inputfile *inf_from_file(const char *filename,
				datafilename_fn_t datafn)
{
  struct inputfile *inf;
  fz_FILE *fp;

  assert(filename != NULL);
  assert(strlen(filename) > 0);
  fp = fz_from_file(filename, "r", FZ_NOT_USED, FZ_NOT_USED);
  if (!fp) {
    return NULL;
  }
  freelog(LOG_DEBUG, "inputfile: opened \"%s\" ok", filename);
  inf = inf_from_stream(fp, datafn);
  inf->filename = mystrdup(filename);
  return inf;
}

/********************************************************************** 
  Open the stream, and return an allocated, initialized structure.
  Returns NULL if the file could not be opened.
***********************************************************************/
struct inputfile *inf_from_stream(fz_FILE * stream, datafilename_fn_t datafn)
{
  struct inputfile *inf;

  assert(stream != NULL);
  inf = fc_malloc(sizeof(*inf));
  init_zeros(inf);
  
  inf->filename = NULL;
  inf->fp = stream;
  inf->datafn = datafn;

  freelog(LOG_DEBUG, "inputfile: opened \"%s\" ok", inf_filename(inf));
  return inf;
}


/********************************************************************** 
  Close the file and free associated memory, but don't recurse
  included_from files, and don't free the actual memory where
  the inf record is stored (ie, the memory where the users pointer
  points to).  This is used when closing an included file.
***********************************************************************/
static void inf_close_partial(struct inputfile *inf)
{
  assert_sanity(inf);

  freelog(LOG_DEBUG, "inputfile: sub-closing \"%s\"", inf_filename(inf));

  if (fz_ferror(inf->fp) != 0) {
    freelog(LOG_ERROR, "Error before closing %s: %s", inf_filename(inf),
	    fz_strerror(inf->fp));
    fz_fclose(inf->fp);
    inf->fp = NULL;
  }
  else if (fz_fclose(inf->fp) != 0) {
    freelog(LOG_ERROR, "Error closing %s", inf_filename(inf));
  }
  if (inf->filename) {
    free(inf->filename);
  }
  inf->filename = NULL;
  astr_free(&inf->cur_line);
  astr_free(&inf->copy_line);
  astr_free(&inf->token);
  astr_free(&inf->partial);

  /* assign zeros for safety if accidently re-use etc: */
  init_zeros(inf);
  inf->magic = ~INF_MAGIC;

  freelog(LOG_DEBUG, "inputfile: sub-closed ok");
}

/********************************************************************** 
  Close the file and free associated memory, included any partially
  recursed included files, and the memory allocated for 'inf' itself.
  Should only be used on an actually open inputfile.
  After this, the pointer should not be used.
***********************************************************************/
void inf_close(struct inputfile *inf)
{
  assert_sanity(inf);

  freelog(LOG_DEBUG, "inputfile: closing \"%s\"", inf_filename(inf));
  if (inf->included_from) {
    inf_close(inf->included_from);
  }
  inf_close_partial(inf);
  free(inf);
  freelog(LOG_DEBUG, "inputfile: closed ok");
}

/********************************************************************** 
  Return 1 if have data for current line.
***********************************************************************/
static bool have_line(struct inputfile *inf)
{
  assert_sanity(inf);
  return (inf->cur_line.n > 0);
}

/********************************************************************** 
  Return 1 if current pos is at end of current line.
***********************************************************************/
static bool at_eol(struct inputfile *inf)
{
  assert_sanity(inf);
  assert(inf->cur_line_pos <= inf->cur_line.n);
  return (inf->cur_line_pos >= inf->cur_line.n - 1);
}

/********************************************************************** 
  ...
***********************************************************************/
bool inf_at_eof(struct inputfile *inf)
{
  assert_sanity(inf);
  return inf->at_eof;
}

/********************************************************************** 
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
  char *bare_name, *full_name, *c;
  struct inputfile *new_inf, temp;

  if (len==0) {
    len = strlen(include_prefix);
  }
  assert_sanity(inf);
  if (inf->at_eof || inf->in_string || inf->cur_line.n <= len
      || inf->cur_line_pos > 0) {
    return FALSE;
  }
  if (strncmp(inf->cur_line.str, include_prefix, len)!=0) {
    return FALSE;
  }
  /* from here, the include-line must be well formed or we die */
  /* keep inf->cur_line_pos accurate just so error messages are useful */

  /* skip any whitespace: */
  inf->cur_line_pos = len;
  c = inf->cur_line.str + len;
  while (*c != '\0' && my_isspace(*c)) c++;

  if (*c != '\"') {
    inf_log(inf, LOG_ERROR, 
            "Did not find opening doublequote for '*include' line");
    return FALSE;
  }
  c++;
  inf->cur_line_pos = c - inf->cur_line.str;

  bare_name = c;
  while (*c != '\0' && *c != '\"') c++;
  if (*c != '\"') {
    inf_log(inf, LOG_ERROR, 
            "Did not find closing doublequote for '*include' line");
    return FALSE;
  }
  *c++ = '\0';
  inf->cur_line_pos = c - inf->cur_line.str;

  /* check rest of line is well-formed: */
  while (*c != '\0' && my_isspace(*c) && !is_comment(*c)) c++;
  if (!(*c=='\0' || is_comment(*c))) {
    inf_log(inf, LOG_ERROR, "Junk after filename for '*include' line");
    return FALSE;
  }
  inf->cur_line_pos = inf->cur_line.n-1;

  full_name = inf->datafn(bare_name);
  if (!full_name) {
    freelog(LOG_ERROR, "Could not find included file \"%s\"", bare_name);
    return FALSE;
  }

  /* avoid recursion: (first filename may not have the same path,
     but will at least stop infinite recursion) */
  {
    struct inputfile *inc = inf;
    do {
      if (inc->filename && strcmp(full_name, inc->filename)==0) {
        freelog(LOG_ERROR, 
                "Recursion trap on '*include' for \"%s\"", full_name);
        return FALSE;
      }
    } while((inc=inc->included_from));
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

/********************************************************************** 
  Read a new line into cur_line; also copy to copy_line.
  Increments line_num and cur_line_pos.
  Returns 0 if didn't read or other problem: treat as EOF.
  Strips newline from input.
***********************************************************************/
static bool read_a_line(struct inputfile *inf)
{
  struct astring *line;
  char *ret;
  int pos;
  
  assert_sanity(inf);

  if (inf->at_eof)
    return FALSE;
  
  /* abbreviation: */
  line = &inf->cur_line;
  
  /* minimum initial line length: */
  astr_minsize(line, 80);
  pos = 0;

  /* don't print "orig line" in warnings until we have it: */
  inf->copy_line.n = 0;
  
  /* Read until we get a full line:
   * At start of this loop, pos is index to trailing null
   * (or first position) in line.
   */
  for(;;) {
    ret = fz_fgets(line->str + pos, line->n_alloc - pos, inf->fp);
    
    if (!ret) {
      /* fgets failed */
      inf->at_eof = TRUE;
      if (pos != 0) {
	inf_warn(inf, "missing newline at EOF, or failed read");
	/* treat as simple EOF, ignoring last line: */
	pos = 0;
      }
      line->str[0] = '\0';
      line->n = 0;
      if (inf->in_string) {
	/* Note: Don't allow multi-line strings to cross "include"
	   boundaries */
	inf_log(inf, LOG_ERROR, "Multi-line string went to end-of-file");
        return FALSE;
      }
      if (inf->included_from) {
	/* Pop the include, and get next line from file above instead. */
	struct inputfile *inc = inf->included_from;
	inf_close_partial(inf);
	*inf = *inc;		/* so the user pointer in still valid
				   (and inf pointers in calling functions) */
	free(inc);
	return read_a_line(inf);
      }
      break;
    }
    
    pos += strlen(line->str + pos);
    line->n = pos + 1;
    
    if (line->str[pos-1] == '\n') {
      line->str[pos-1] = '\0';
      line->n--;
      break;
    }
    if (line->n != line->n_alloc) {
      freelog(LOG_VERBOSE, "inputfile: expect missing newline at EOF");
    }
    astr_minsize(line, line->n*2);
  }
  inf->line_num++;
  inf->cur_line_pos = 0;

  astr_minsize(&inf->copy_line, inf->cur_line.n + ((inf->cur_line.n == 0) ? 1 : 0));
  strcpy(inf->copy_line.str, inf->cur_line.str);

  if (check_include(inf)) {
    return read_a_line(inf);
  }
  return (!inf->at_eof);
}

/********************************************************************** 
  Set "flag" token when we don't really want to return anything,
  except non-null.
***********************************************************************/
static void assign_flag_token(struct astring *astr, char val)
{
  static char flag_token[2];

  assert(astr != NULL);
  flag_token[0] = val;
  flag_token[1] = '\0';
  astr_minsize(astr, 2);
  strcpy(astr->str, flag_token);
}

/********************************************************************** 
  Give a detailed log message, including information on
  current line number etc.  Message can be NULL: then just logs
  information on where we are in the file.
***********************************************************************/
void inf_log(struct inputfile *inf, int loglevel, const char *message)
{
  assert_sanity(inf);

  if (message) {
    freelog(loglevel, "%s", message);
  }
  freelog(loglevel, "  file \"%s\", line %d, pos %d%s",
	  inf_filename(inf), inf->line_num, inf->cur_line_pos,
	  (inf->at_eof ? ", EOF" : ""));
  if (inf->cur_line.str && inf->cur_line.n > 0) {
    freelog(loglevel, "  looking at: '%s'",
	    inf->cur_line.str+inf->cur_line_pos);
  }
  if (inf->copy_line.str && inf->copy_line.n > 0) {
    freelog(loglevel, "  original line: '%s'", inf->copy_line.str);
  }
  if (inf->in_string) {
    freelog(loglevel, "  processing string starting at line %d",
	    inf->string_start_line);
  }
  while ((inf=inf->included_from)) {    /* local pointer assignment */
    freelog(loglevel, "  included from file \"%s\", line %d",
	    inf_filename(inf), inf->line_num);
  }
}

/********************************************************************** 
  ...
***********************************************************************/
static void inf_warn(struct inputfile *inf, const char *message)
{
  inf_log(inf, LOG_NORMAL, message);
}

/********************************************************************** 
  ...
***********************************************************************/
static const char *get_token(struct inputfile *inf,
			     enum inf_token_type type,
			     bool required)
{
  const char *c;
  const char *name;
  get_token_fn_t func;
  
  assert_sanity(inf);
  assert(type>=INF_TOK_FIRST && type<INF_TOK_LAST);

  name = tok_tab[type].name ? tok_tab[type].name : "(unnamed)";
  func = tok_tab[type].func;
  
  if (!func) {
    freelog(LOG_ERROR, "token type %d (%s) not supported yet", type, name);
    c = NULL;
  } else {
    if (!have_line(inf))
      (void) read_a_line(inf);
    if (!have_line(inf)) {
      c = NULL;
    } else {
      c = func(inf);
    }
  }
  if (c) {
    if (INF_DEBUG_FOUND) {
      freelog(LOG_DEBUG, "inputfile: found %s '%s'", name, inf->token.str);
    }
  } else if (required) {
    freelog(LOG_FATAL, "Did not find token %s in %s line %d", 
            name, inf->filename, inf->line_num);
    exit(EXIT_FAILURE);
  }
  return c;
}
  
const char *inf_token(struct inputfile *inf, enum inf_token_type type)
{
  return get_token(inf, type, FALSE);
}

const char *inf_token_required(struct inputfile *inf, enum inf_token_type type)
{
  return get_token(inf, type, TRUE);
}

/********************************************************************** 
  Read as many tokens of specified type as possible, discarding
  the results; returns number of such tokens read and discarded.
***********************************************************************/
int inf_discard_tokens(struct inputfile *inf, enum inf_token_type type)
{
  int count = 0;
  
  while(inf_token(inf, type))
    count++;
  return count;
}

/********************************************************************** 
  ...
***********************************************************************/
static const char *get_token_section_name(struct inputfile *inf)
{
  char *c, *start;
  
  assert(have_line(inf));

  c = inf->cur_line.str + inf->cur_line_pos;
  if (*c++ != '[')
    return NULL;
  start = c;
  while (*c != '\0' && *c != ']') {
    c++;
  }
  if (*c != ']')
    return NULL;
  *c++ = '\0';
  inf->cur_line_pos = c - inf->cur_line.str;
  astr_minsize(&inf->token, strlen(start)+1);
  strcpy(inf->token.str, start);
  return inf->token.str;
}

/********************************************************************** 
  ...
***********************************************************************/
static const char *get_token_entry_name(struct inputfile *inf)
{
  char *c, *start, *end;
  
  assert(have_line(inf));

  c = inf->cur_line.str + inf->cur_line_pos;
  while(*c != '\0' && my_isspace(*c)) {
    c++;
  }
  if (*c == '\0')
    return NULL;
  start = c;
  while (*c != '\0' && !my_isspace(*c) && *c != '=' && !is_comment(*c)) {
    c++;
  }
  if (!(*c != '\0' && (my_isspace(*c) || *c == '='))) 
    return NULL;
  end = c;
  while (*c != '\0' && *c != '=' && !is_comment(*c)) {
    c++;
  }
  if (*c != '=') {
    return NULL;
  }
  *end = '\0';
  inf->cur_line_pos = c + 1 - inf->cur_line.str;
  astr_minsize(&inf->token, strlen(start)+1);
  strcpy(inf->token.str, start);
  return inf->token.str;
}

/********************************************************************** 
  ...
***********************************************************************/
static const char *get_token_eol(struct inputfile *inf)
{
  char *c;
  
  assert(have_line(inf));

  if (!at_eol(inf)) {
    c = inf->cur_line.str + inf->cur_line_pos;
    while(*c != '\0' && my_isspace(*c)) {
      c++;
    }
    if (*c != '\0' && !is_comment(*c))
      return NULL;
  }

  /* finished with this line: say that we don't have it any more: */
  inf->cur_line.n = 0;
  inf->cur_line_pos = 0;
  
  assign_flag_token(&inf->token, ' ');
  return inf->token.str;
}

/********************************************************************** 
  Get a flag token of a single character, with optional
  preceeding whitespace.
***********************************************************************/
static const char *get_token_white_char(struct inputfile *inf,
					char target)
{
  char *c;
  
  assert(have_line(inf));

  c = inf->cur_line.str + inf->cur_line_pos;
  while(*c != '\0' && my_isspace(*c)) {
    c++;
  }
  if (*c != target)
    return NULL;
  inf->cur_line_pos = c + 1 - inf->cur_line.str;
  assign_flag_token(&inf->token, target);
  return inf->token.str;
}

/********************************************************************** 
  ...
***********************************************************************/
static const char *get_token_table_start(struct inputfile *inf)
{
  return get_token_white_char(inf, '{');
}

/********************************************************************** 
  ...
***********************************************************************/
static const char *get_token_table_end(struct inputfile *inf)
{
  return get_token_white_char(inf, '}');
}

/********************************************************************** 
  ...
***********************************************************************/
static const char *get_token_comma(struct inputfile *inf)
{
  return get_token_white_char(inf, ',');
}

/********************************************************************** 
  This one is more complicated; note that it may read in multiple lines.
***********************************************************************/
static const char *get_token_value(struct inputfile *inf)
{
  struct astring *partial;
  char *c, *start;
  char trailing;
  bool has_i18n_marking = FALSE;
  char border_character = '\"';
  
  assert(have_line(inf));

  c = inf->cur_line.str + inf->cur_line_pos;
  while(*c != '\0' && my_isspace(*c)) {
    c++;
  }
  if (*c == '\0')
    return NULL;

  if (*c == '-' || my_isdigit(*c)) {
    /* a number: */
    start = c++;
    while(*c != '\0' && my_isdigit(*c)) {
      c++;
    }
    /* check that the trailing stuff is ok: */
    if (!(*c == '\0' || *c == ',' || my_isspace(*c) || is_comment(*c))) {
      return NULL;
    }
    /* If its a comma, we don't want to obliterate it permanently,
     * so rememeber it: */
    trailing = *c;
    *c = '\0';
    
    inf->cur_line_pos = c - inf->cur_line.str;
    astr_minsize(&inf->token, strlen(start)+1);
    strcpy(inf->token.str, start);
    
    *c = trailing;
    return inf->token.str;
  }

  /* allow gettext marker: */
  if (*c == '_' && *(c+1) == '(') {
    has_i18n_marking = TRUE;
    c += 2;
    while(*c != '\0' && my_isspace(*c)) {
      c++;
    }
    if (*c == '\0')
      return NULL;
  }

  border_character = *c;

  if (border_character != '\"'
      && border_character != '\''
      && border_character != '$') {
    return NULL;
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

  partial = &inf->partial;	/* abbreviation */
  astr_minsize(partial, 1);
  partial->str[0] = '\0';
  
  start = c++;			/* start includes the initial \", to
				   distinguish from a number */
  for(;;) {
    int pos;
    
    while(*c != '\0' && *c != border_character) {
      /* skip over escaped chars, including backslash-doublequote,
	 and backslash-backslash: */
      if (*c == '\\' && *(c+1) != '\0') {  
	c++;
      }
      c++;
    }

    if (*c == border_character) {
      /* Found end of string */
      break;
    }

    /* Accumulate to partial string and try more lines;
     * note partial->n must be _exactly_ the right size, so we
     * can strcpy instead of strcat-ing all the way to the end
     * each time. */
    pos = partial->n - 1;
    astr_minsize(partial, partial->n + c - start + 1);
    strcpy(partial->str + pos, start);
    strcpy(partial->str + partial->n - 2, "\n");
    
    if (!read_a_line(inf)) {
      /* shouldn't happen */
      inf_log(inf, LOG_ERROR, 
              "Bad return for multi-line string from read_a_line");
      return NULL;
    }
    c = start = inf->cur_line.str;
  }

  /* found end of string */
  *c = '\0';
  inf->cur_line_pos = c + 1 - inf->cur_line.str;
  astr_minsize(&inf->token, partial->n + strlen(start));
  strcpy(inf->token.str, partial->str);
  strcpy(inf->token.str + partial->n - 1, start);

  /* check gettext tag at end: */
  if (has_i18n_marking) {
    if (*++c == ')') {
      inf->cur_line_pos++;
    } else {
      inf_warn(inf, "Missing end of i18n string marking");
    }
  }
  inf->in_string = FALSE;
  return inf->token.str;
}
