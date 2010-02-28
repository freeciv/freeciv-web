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

/**************************************************************************
  the idea with this file is to create something similar to the ms-windows
  .ini files functions.
  it also demonstrates how ugly code using the genlist class looks.
  however the interface is nice. ie:
  section_file_lookup_string(file, "player%d.unit%d.name", plrno, unitno); 
***************************************************************************/

/**************************************************************************
  Description of the file format:
  (This is based on a format by the original authors, with
  various incremental extensions. --dwp)
  
  - Whitespace lines are ignored, as are lines where the first
  non-whitespace character is ';' (comment lines).
  Optionally '#' can also be used for comments.

  - A line of the form:
       *include "filename"
  includes the named file at that point.  (The '*' must be the
  first character on the line.) The file is found by looking in
  FREECIV_PATH.  Non-infinite recursive includes are allowed.
  
  - A line with "[name]" labels the start of a section with
  that name; one of these must be the first non-comment line in
  the file.  Any spaces within the brackets are included in the
  name, but this feature (?) should probably not be used...

  - Within a section, lines have one of the following forms:
      subname = "stringvalue"
      subname = -digits
      subname = digits
  for a value with given name and string, negative integer, and
  positive integer values, respectively.  These entries are
  referenced in the following functions as "sectionname.subname".
  The section name should not contain any dots ('.'); the subname
  can, but they have no particular significance.  There can be
  optional whitespace before and/or after the equals sign.
  You can put a newline after (but not before) the equals sign.
  
  Backslash is an escape character in strings (double-quoted strings
  only, not names); recognised escapes are \n, \\, and \".
  (Any other \<char> is just treated as <char>.)

  - Gettext markings:  You can surround strings like so:
      foo = _("stringvalue")
  The registry just ignores these extra markings, but this is
  useful for marking strings for translations via gettext tools.

  - Multiline strings:  Strings can have embeded newlines, eg:
    foo = _("
    This is a string
    over multiple lines
    ")
  This is equivalent to:
    foo = _("\nThis is a string\nover multiple lines\n")
  Note that if you missplace the trailing doublequote you can
  easily end up with strange errors reading the file...

  - Vector format: An entry can have multiple values separated
  by commas, eg:
      foo = 10, 11, "x"
  These are accessed by names "foo", "foo,1" and "foo,2"
  (with section prefix as above).  So the above is equivalent to:
      foo   = 10
      foo,1 = 11
      foo,2 = "x"
  As in the example, in principle you can mix integers and strings,
  but the calling program will probably require elements to be the
  same type.   Note that the first element of a vector is not "foo,0",
  in order that the name of the first element is the same whether or
  not there are subsequent elements.  However as a convenience, if
  you try to lookup "foo,0" then you get back "foo".  (So you should
  never have "foo,0" as a real name in the datafile.)

  - Tabular format:  The lines:
      foo = { "bar",  "baz",   "bax"
              "wow",   10,     -5
              "cool",  "str"
              "hmm",    314,   99, 33, 11
      }
  are equivalent to the following:
      foo0.bar = "wow"
      foo0.baz = 10
      foo0.bax = -5
      foo1.bar = "cool"
      foo1.baz = "str"
      foo2.bar = "hmm"
      foo2.baz = 314
      foo2.bax = 99
      foo2.bax,1 = 33
      foo2.bax,2 = 11
  The first line specifies the base name and the column names, and the
  subsequent lines have data.  Again it is possible to mix string and
  integer values in a column, and have either more or less values
  in a row than there are column headings, but the code which uses
  this information (via the registry) may set more stringent conditions.
  If a row has more entries than column headings, the last column is
  treated as a vector (as above).  You can optionally put a newline
  after '=' and/or after '{'.

  The equivalence above between the new and old formats is fairly
  direct: internally, data is converted to the old format.
  In principle it could be a good idea to represent the data
  as a table (2-d array) internally, but the current method
  seems sufficient and relatively simple...
  
  There is a limited ability to save data in tabular:
  So long as the section_file is constructed in an expected way,
  tabular data (with no missing or extra values) can be saved
  in tabular form.  (See section_file_save().)

  - Multiline vectors: if the last non-comment non-whitespace
  character in a line is a comma, the line is considered to
  continue on to the next line.  Eg:
      foo = 10,
            11,
            "x"
  This is equivalent to the original "vector format" example above.
  Such multi-lines can occur for column headings, vectors, or
  table rows, again with some potential for strange errors...

***************************************************************************/

/**************************************************************************
  Hashing registry lookups: (by dwp)
  - Have a hash table direct to entries, bypassing sections division.
  - For convenience, store the key (the full section+entry name)
    in the hash table (some memory overhead).
  - The number of entries is fixed when the hash table is built.
  - Now uses hash.c
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "astring.h"
#include "fcintl.h"
#include "genlist.h"
#include "hash.h"
#include "inputfile.h"
#include "ioz.h"
#include "log.h"
#include "mem.h"
#include "sbuffer.h"
#include "shared.h"
#include "support.h"

#include "registry.h"

#define MAX_LEN_BUFFER 1024

#define SAVE_TABLES TRUE	/* set to 0 for old-style savefiles */
#define SECF_DEBUG_ENTRIES FALSE/* LOG_DEBUG each entry value */

#define SPECVEC_TAG astring
#include "specvec.h"

/* An 'entry' is a string, integer or string vector;
 * Whether it is string or int or string vector is determined by whether
 * svalue/vec_values is NULL.
 */
struct entry {
  char *name;			/* name, not including section prefix */
  int  ivalue;			/* value if integer */
  char *svalue;			/* value if string (in sbuffer) */
  char **vec_values;		/* string vector values */
  int dim;			/* vector's size */
  int  used;			/* number of times entry looked up */
  char *comment;                /* comment, may be NULL */
  bool escaped;                 /* " or $. Usually TRUE */
};

/* create a 'struct entry_list' and related functions: */
#define SPECLIST_TAG entry
#include "speclist.h"

#define entry_list_iterate(entlist, pentry) \
       TYPED_LIST_ITERATE(struct entry, entlist, pentry)
#define entry_list_iterate_end  LIST_ITERATE_END


struct section {
  char *name;
  struct entry_list *entries;
};

/* create a 'struct section_list' and related functions: */
#define SPECLIST_TAG section
#include "speclist.h"

#define section_list_iterate(seclist, psection) \
       TYPED_LIST_ITERATE(struct section, seclist, psection)
#define section_list_iterate_end  LIST_ITERATE_END

#define section_list_iterate_rev(seclist, psection) \
       TYPED_LIST_ITERATE_REV(struct section, seclist, psection)
#define section_list_iterate_rev_end  LIST_ITERATE_REV_END

/* The hash table and some extra data: */
struct hash_data {
  struct hash_table *htbl;
  int num_entries_hashbuild;
  bool allow_duplicates;
  int num_duplicates;
};

static void secfilehash_check(struct section_file *file);
static void secfilehash_insert(struct section_file *file,
			       char *key, struct entry *data);

static char *minstrdup(struct sbuffer *sb, const char *str,
                       bool full_escapes);
static char *moutstr(char *str, bool full_escapes);

static struct entry*
section_file_lookup_internal(struct section_file *my_section_file,  
			     char *fullpath);
static struct entry*
section_file_insert_internal(struct section_file *my_section_file, 
			     char *fullpath);

/* Only need use these explicitly on section_files constructed,
   not for ones generated by section_file_load:
*/
static bool secfilehash_hashash(struct section_file *file);
static void secfilehash_build(struct section_file *file,
			      bool allow_duplicates);
static void secfilehash_free(struct section_file *file);

/**************************************************************************
  Return the filename the sectionfile was loaded as, or "(anonymous)"
  if this sectionfile was created rather than loaded from file.
  The memory is managed internally, and should not be altered,
  nor used after section_file_free() called for the sectionfile.
**************************************************************************/
const char *secfile_filename(const struct section_file *file)
{
  if (file->filename) {
    return file->filename;
  } else {
    return "(anonymous)";
  }
}

/**************************************************************************
...
**************************************************************************/
void section_file_init(struct section_file *file)
{
  file->filename = NULL;
  file->sections = section_list_new();
  file->hash_sections = hash_new(hash_fval_string, hash_fcmp_string);
  file->num_entries = 0;
  file->hashd = NULL;
  file->sb = sbuf_new();
}

/**************************************************************************
...
**************************************************************************/
void section_file_free(struct section_file *file)
{
  /* all the real data is stored in the sbuffer;
     just free the list meta-data:
  */
  section_list_iterate(file->sections, psection) {
    entry_list_free(psection->entries);
  } section_list_iterate_end;
  section_list_free(file->sections);
  file->sections = NULL;

  /* free the sections hash data: */
  hash_free(file->hash_sections);
  file->hash_sections = NULL;

  /* free the hash data: */
  if(secfilehash_hashash(file)) {
    secfilehash_free(file);
  }
  
  /* free the real data: */
  sbuf_free(file->sb);
  file->sb = NULL;

  if (file->filename) {
    free(file->filename);
  }
  file->filename = NULL;
}

/**************************************************************************
...
**************************************************************************/
static struct section*
section_file_append_section(struct section_file *sf, const char *sec_name)
{
  struct section *psection;

  psection = sbuf_malloc(sf->sb, sizeof(*psection));
  psection->name = sbuf_strdup(sf->sb, sec_name);
  psection->entries = entry_list_new();
  section_list_append(sf->sections, psection);

  if (!hash_insert(sf->hash_sections, psection->name, psection)) {
    freelog(LOG_ERROR, "Section \"%s\" name collision", sec_name);
  }

  return psection;
}

/**************************************************************************
  Print log messages for any entries in the file which have
  not been looked up -- ie, unused or unrecognised entries.
  To mark an entry as used without actually doing anything with it,
  you could do something like:
     section_file_lookup(&file, "foo.bar");  / * unused * /
**************************************************************************/
void section_file_check_unused(struct section_file *file, const char *filename)
{
  int any = 0;

  section_list_iterate(file->sections, psection) {
    entry_list_iterate(psection->entries, pentry) {
      if (pentry->used == 0) {
	if (any == 0 && filename) {
	  freelog(LOG_VERBOSE, "Unused entries in file %s:", filename);
	  any = 1;
	}
	freelog(LOG_VERBOSE, "  unused entry: %s.%s",
		psection->name, pentry->name);
      }
    }
    entry_list_iterate_end;
  }
  section_list_iterate_end;
}

/**************************************************************************
  Initialize the entry struct and set to default (empty) values.
**************************************************************************/
static void entry_init(struct entry *pentry)
{
  pentry->name = NULL;
  pentry->svalue = NULL;
  pentry->ivalue = 0;
  pentry->used = 0;
  pentry->dim = 0;
  pentry->vec_values = NULL;
  pentry->comment = NULL;
  pentry->escaped = TRUE;
}

/**************************************************************************
  Return a new entry struct, allocated from sb, with given name,
  and where tok is a "value" return token from inputfile.
  The entry value has any escaped double-quotes etc removed.
**************************************************************************/
static struct entry *new_entry(struct sbuffer *sb, const char *name,
			       const char *tok)
{
  struct entry *pentry;

  pentry = sbuf_malloc(sb, sizeof(struct entry));
  entry_init(pentry);
  pentry->name = sbuf_strdup(sb, name);
  if (tok[0] != '-' && !my_isdigit(tok[0])) {
    /* It is not integer, but string with some border character. */
    pentry->escaped = (tok[0] != '$'); /* Border character '$' means no escapes */

    /* minstrdup() starting after border character */
    pentry->svalue = minstrdup(sb, tok+1, pentry->escaped);
    if (SECF_DEBUG_ENTRIES) {
      freelog(LOG_DEBUG, "entry %s '%s'", name, pentry->svalue);
    }
  } else {
    if (sscanf(tok, "%d", &pentry->ivalue) != 1) {
      freelog(LOG_ERROR, "'%s' isn't an integer", tok);
    }
    if (SECF_DEBUG_ENTRIES) {
      freelog(LOG_DEBUG, "entry %s %d", name, pentry->ivalue);
    }
  }
  return pentry;
}

/****************************************************************************
  Return the section with the given name, or NULL if there is none.
****************************************************************************/
static struct section *find_section_by_name(struct section_file *sf,
					    const char *name)
{
  return hash_lookup_data(sf->hash_sections, name);
}	

/**************************************************************************
...
**************************************************************************/
static bool section_file_read_dup(struct section_file *sf,
      	      	      	      	  const char *filename,
      	      	      	      	  struct inputfile *inf,
				  bool allow_duplicates,
				  const char *requested_section)
{
  struct section *psection = NULL;
  struct entry *pentry;
  bool table_state = FALSE;	/* 1 when within tabular format */
  int table_lineno = 0;		/* row number in tabular, 0=top data row */
  struct sbuffer *sb;
  const char *tok;
  int i;
  struct astring base_name = ASTRING_INIT;    /* for table or single entry */
  struct astring entry_name = ASTRING_INIT;
  struct astring_vector columns;    /* astrings for column headings */
  bool found_my_section = FALSE;

  section_file_init(sf);
  if (filename) {
    sf->filename = mystrdup(filename);
  } else {
    sf->filename = NULL;
  }
  astring_vector_init(&columns);
  sb = sf->sb;

  if (filename) {
    freelog(LOG_VERBOSE, "Reading registry from \"%s\"", filename);
  } else {
    freelog(LOG_VERBOSE, "Reading registry");
  }

  while(!inf_at_eof(inf)) {
    if (inf_token(inf, INF_TOK_EOL))
      continue;
    if (inf_at_eof(inf)) {
      /* may only realise at eof after trying to read eol above */
      break;
    }
    tok = inf_token(inf, INF_TOK_SECTION_NAME);
    if (tok) {
      if (found_my_section) {
	/* This shortcut will stop any further loading after the requested
	 * section has been loaded (i.e., at the start of a new section).
	 * This is needed to make the behavior useful, since the whole
	 * purpose is to short-cut further loading of the file.  However
	 * normally a section may be split up, and that will no longer
	 * work here because it will be short-cut. */
	inf_log(inf, LOG_DEBUG, "found requested section; finishing");
	return TRUE;
      }
      if (table_state) {
	inf_log(inf, LOG_ERROR, "new section during table");
        return FALSE;
      }
      /* Check if we already have a section with this name.
	 (Could ignore this and have a duplicate sections internally,
	 but then secfile_get_secnames_prefix would return duplicates.)
	 Duplicate section in input are likely to be useful for includes.
      */
      psection = find_section_by_name(sf, tok);
      if (!psection) {
	if (!requested_section || strcmp(tok, requested_section) == 0) {
	  psection = section_file_append_section(sf, tok);
	  if (requested_section) {
	    found_my_section = TRUE;
	  }
	}
      }
      (void) inf_token_required(inf, INF_TOK_EOL);
      continue;
    }
#if 0
    if (!psection) {
      /* This used to be an error.  However there's no reason it shouldn't
       * be allowed, and it breaks the rest of the requested_section code.
       * It's been defined out but may be of use sometime. */
      inf_log(inf, LOG_ERROR, "data before first section");
      return FALSE;
    }
#endif
    if (inf_token(inf, INF_TOK_TABLE_END)) {
      if (!table_state) {
	inf_log(inf, LOG_ERROR, "misplaced \"}\"");
        return FALSE;
      }
      (void) inf_token_required(inf, INF_TOK_EOL);
      table_state = FALSE;
      continue;
    }
    if (table_state) {
      i = -1;
      do {
	int num_columns = astring_vector_size(&columns);

	i++;
	inf_discard_tokens(inf, INF_TOK_EOL);  	/* allow newlines */
	if (!(tok = inf_token_required(inf, INF_TOK_VALUE))) {
          return FALSE;
        }

	if (i < num_columns) {
	  astr_minsize(&entry_name, base_name.n + 10 + columns.p[i].n);
	  my_snprintf(entry_name.str, entry_name.n_alloc, "%s%d.%s",
		      base_name.str, table_lineno, columns.p[i].str);
	} else {
	  astr_minsize(&entry_name,
		       base_name.n + 20 + columns.p[num_columns - 1].n);
	  my_snprintf(entry_name.str, entry_name.n_alloc, "%s%d.%s,%d",
		      base_name.str, table_lineno,
		      columns.p[num_columns - 1].str,
		      (int) (i - num_columns + 1));
	}
	if (psection) {
	  /* Load this entry (if psection == NULL the entry is silently
	   * skipped). */
	  pentry = new_entry(sb, entry_name.str, tok);
	  entry_list_append(psection->entries, pentry);
	  sf->num_entries++;
	}
      } while(inf_token(inf, INF_TOK_COMMA));
      
      (void) inf_token_required(inf, INF_TOK_EOL);
      table_lineno++;
      continue;
    }
    
    if (!(tok = inf_token_required(inf, INF_TOK_ENTRY_NAME))) {
      return FALSE;
    }

    /* need to store tok before next calls: */
    astr_minsize(&base_name, strlen(tok)+1);
    strcpy(base_name.str, tok);

    inf_discard_tokens(inf, INF_TOK_EOL);  	/* allow newlines */
    
    if (inf_token(inf, INF_TOK_TABLE_START)) {
      i = -1;
      do {
	i++;
	inf_discard_tokens(inf, INF_TOK_EOL);  	/* allow newlines */
	if (!(tok = inf_token_required(inf, INF_TOK_VALUE))) {
          return FALSE;
        }
	if( tok[0] != '\"' ) {
	  inf_log(inf, LOG_ERROR, "table column header non-string");
          return FALSE;
	}
	{ 	/* expand columns: */
	  int j, n_prev;
	  n_prev = astring_vector_size(&columns);
	  for (j = i + 1; j < n_prev; j++) {
	    astr_free(&columns.p[j]);
	  }
	  astring_vector_reserve(&columns, i + 1);
	  for (j = n_prev; j < i + 1; j++) {
	    astr_init(&columns.p[j]);
	  }
	}
	astr_minsize(&columns.p[i], strlen(tok));
	strcpy(columns.p[i].str, tok+1);
	
      } while(inf_token(inf, INF_TOK_COMMA));
      
      (void) inf_token_required(inf, INF_TOK_EOL);
      table_state = TRUE;
      table_lineno=0;
      continue;
    }
    /* ordinary value: */
    i = -1;
    do {
      i++;
      inf_discard_tokens(inf, INF_TOK_EOL);  	/* allow newlines */
      if (!(tok = inf_token_required(inf, INF_TOK_VALUE))) {
        return FALSE;
      }
      if (i==0) {
	pentry = new_entry(sb, base_name.str, tok);
      } else {
	astr_minsize(&entry_name, base_name.n + 20);
	my_snprintf(entry_name.str, entry_name.n_alloc,
		    "%s,%d", base_name.str, i);
	pentry = new_entry(sb, entry_name.str, tok);
      }
      if (psection) {
	/* Load this entry (if psection == NULL the entry is silently
	 * skipped). */
	entry_list_append(psection->entries, pentry);
	sf->num_entries++;
      }
    } while(inf_token(inf, INF_TOK_COMMA));
    (void) inf_token_required(inf, INF_TOK_EOL);
  }
  
  if (table_state) {
    if (filename) {
      freelog(LOG_FATAL, "finished registry %s before end of table\n", filename);
    } else {
      freelog(LOG_FATAL, "finished registry before end of table\n");
    }
    exit(EXIT_FAILURE);
  }
  
  astr_free(&base_name);
  astr_free(&entry_name);
  for (i = 0; i < astring_vector_size(&columns); i++) {
    astr_free(&columns.p[i]);
  }
  astring_vector_free(&columns);
  
  secfilehash_build(sf, allow_duplicates);
    
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
bool section_file_load(struct section_file *my_section_file,
		      const char *filename)
{
  return section_file_load_section(my_section_file, filename, NULL);
}

/**************************************************************************
  Like section_file_load, but this function will only load one "part" of
  the section file.  For instance if you pass in "tutorial", then it will
  only load the [tutorial] section.

  Passing in NULL will give identical results to section_file_load.
**************************************************************************/
bool section_file_load_section(struct section_file *my_section_file,
			       const char *filename, const char *part)
{
  char real_filename[1024];
  struct inputfile *inf;
  bool success;

  interpret_tilde(real_filename, sizeof(real_filename), filename);
  inf = inf_from_file(real_filename, datafilename);

  if (!inf) {
    return FALSE;
  }
  success = section_file_read_dup(my_section_file, real_filename,
                                  inf, TRUE, part);

  inf_close(inf);

  return success;
}

/**************************************************************************
  Load a section_file, but disallow (die on) duplicate entries.
**************************************************************************/
bool section_file_load_nodup(struct section_file *my_section_file,
			    const char *filename)
{
  char real_filename[1024];
  struct inputfile *inf;
  bool success;

  interpret_tilde(real_filename, sizeof(real_filename), filename);
  inf = inf_from_file(real_filename, datafilename);

  if (!inf) {
    return FALSE;
  }
  success = section_file_read_dup(my_section_file, real_filename,
			       inf, FALSE, NULL);

  inf_close(inf);

  return success;
}

/**************************************************************************
...
**************************************************************************/
bool section_file_load_from_stream(struct section_file *my_section_file,
				   fz_FILE * stream)
{
  bool success;
  struct inputfile *inf = inf_from_stream(stream, datafilename);

  if (!inf) {
    return FALSE;
  }
  success = section_file_read_dup(my_section_file, NULL, inf, TRUE, NULL);

  inf_close(inf);

  return success;
}

/**************************************************************************
 Save the previously filled in section_file to disk.
 
 There is now limited ability to save in the new tabular format
 (to give smaller savefiles).
 The start of a table is detected by an entry with name of the form:
    (alphabetical_component)(zero)(period)(alphanumeric_component)
 Eg: u0.id, or c0.id, in the freeciv savefile.
 The alphabetical component is taken as the "name" of the table,
 and the component after the period as the first column name.
 This should be followed by the other column values for u0,
 and then subsequent u1, u2, etc, in strict order with no omissions,
 and with all of the columns for all uN in the same order as for u0.

 If compression_level is non-zero, then compress using zlib.  (Should
 only supply non-zero compression_level if already know that HAVE_LIBZ.)
 Below simply specifies FZ_ZLIB method, since fz_fromFile() automatically
 changes to FZ_PLAIN method when level==0.
**************************************************************************/
bool section_file_save(struct section_file *my_section_file,
                       const char *filename,
                       int compression_level,
                       enum fz_method compression_method)
{
  char real_filename[1024];
  fz_FILE *fs;
  const struct genlist_link *ent_iter, *save_iter, *col_iter;
  struct entry *pentry, *col_pentry;
  int i;
  
  interpret_tilde(real_filename, sizeof(real_filename), filename);
  fs = fz_from_file(real_filename, "w", compression_method, compression_level);

  if (!fs) {
    return FALSE;
  }

  section_list_iterate(my_section_file->sections, psection) {
    fz_fprintf(fs, "\n[%s]\n", psection->name);

    /* Following doesn't use entry_list_iterate() because we want to do
     * tricky things with the iterators...
     */
    for (ent_iter = genlist_head(entry_list_base(psection->entries));
         ent_iter && (pentry = genlist_link_data(ent_iter));
         ent_iter = genlist_link_next(ent_iter)) {

      /* Tables: break out of this loop if this is a non-table
       * entry (pentry and ent_iter unchanged) or after table (pentry
       * and ent_iter suitably updated, pentry possibly NULL).
       * After each table, loop again in case the next entry
       * is another table.
       */
      for(;;) {
	char *c, *first, base[64];
	int offset, irow, icol, ncol;
	
	/* Example: for first table name of "xyz0.blah":
	 *  first points to the original string pentry->name
	 *  base contains "xyz";
	 *  offset=5 (so first+offset gives "blah")
	 *  note strlen(base)=offset-2
	 */

	if(!SAVE_TABLES) break;
	
	c = first = pentry->name;
	if(*c == '\0' || !my_isalpha(*c)) break;
	for (; *c != '\0' && my_isalpha(*c); c++) {
	  /* nothing */
	}
	if(strncmp(c,"0.",2) != 0) break;
	c+=2;
	if(*c == '\0' || !my_isalnum(*c)) break;

	offset = c - first;
	first[offset-2] = '\0';
	sz_strlcpy(base, first);
	first[offset-2] = '0';
	fz_fprintf(fs, "%s={", base);

	/* Save an iterator at this first entry, which we can later use
	 * to repeatedly iterate over column names:
	 */
	save_iter = ent_iter;

	/* write the column names, and calculate ncol: */
	ncol = 0;
	col_iter = save_iter;
        for(; (col_pentry = genlist_link_data(col_iter));
            col_iter = genlist_link_next(col_iter)) {
	  if(strncmp(col_pentry->name, first, offset) != 0)
	    break;
	  fz_fprintf(fs, "%c\"%s\"", (ncol==0?' ':','), col_pentry->name+offset);
	  ncol++;
	}
	fz_fprintf(fs, "\n");

	/* Iterate over rows and columns, incrementing ent_iter as we go,
	 * and writing values to the table.  Have a separate iterator
	 * to the column names to check they all match.
	 */
	irow = icol = 0;
	col_iter = save_iter;
	for(;;) {
	  char expect[128];	/* pentry->name we're expecting */

	  pentry = genlist_link_data(ent_iter);
	  col_pentry = genlist_link_data(col_iter);

	  my_snprintf(expect, sizeof(expect), "%s%d.%s",
		      base, irow, col_pentry->name+offset);

	  /* break out of tabular if doesn't match: */
	  if((!pentry) || (strcmp(pentry->name, expect) != 0)) {
	    if(icol != 0) {
	      /* If the second or later row of a table is missing some
	       * entries that the first row had, we drop out of the tabular
	       * format.  This is inefficient so we print a warning message;
	       * the calling code probably needs to be fixed so that it can
	       * use the more efficient tabular format.
	       *
	       * FIXME: If the first row is missing some entries that the
	       * second or later row has, then we'll drop out of tabular
	       * format without an error message. */
	      freelog(LOG_ERROR,
		      "In file %s, there is no entry in the registry for \n"
		      "%s.%s (or the entries are out of order. This means a \n"
		      "less efficient non-tabular format will be used. To\n"
		      "avoid this make sure all rows of a table are filled\n"
		      "out with an entry for every column.",
		      real_filename, psection->name, expect);
	      freelog(LOG_ERROR,
                      /* TRANS: No full stop after the URL, could cause confusion. */
                      _("Please report this message at %s"),
		      BUG_URL);
	      fz_fprintf(fs, "\n");
	    }
	    fz_fprintf(fs, "}\n");
	    break;
	  }

	  if(icol>0)
	    fz_fprintf(fs, ",");
	  if(pentry->svalue)
	    fz_fprintf(fs, "%s", moutstr(pentry->svalue, pentry->escaped));
	  else
	    fz_fprintf(fs, "%d", pentry->ivalue);

          ent_iter = genlist_link_next(ent_iter);
          col_iter = genlist_link_next(col_iter);
	  
	  icol++;
	  if(icol==ncol) {
	    fz_fprintf(fs, "\n");
	    irow++;
	    icol = 0;
	    col_iter = save_iter;
	  }
	}
	if(!pentry) break;
      }
      if(!pentry) break;
      
      if (pentry->vec_values) {
        fz_fprintf(fs, "%s=%s", pentry->name,
	           moutstr(pentry->vec_values[0], pentry->escaped));
	for (i = 1; i < pentry->dim; i++) {
	  fz_fprintf(fs, ", %s", moutstr(pentry->vec_values[i], pentry->escaped));
	}
      } else if (pentry->svalue) {
	fz_fprintf(fs, "%s=%s", pentry->name, moutstr(pentry->svalue, pentry->escaped));
      } else {
	fz_fprintf(fs, "%s=%d", pentry->name, pentry->ivalue);
      }
      
      if (pentry->comment) {
	fz_fprintf(fs, "  # %s\n", pentry->comment);
      } else {
	fz_fprintf(fs, "\n");
      }
    }
  }
  section_list_iterate_end;
  
  (void) moutstr(NULL, TRUE);		/* free internal buffer */

  if (fz_ferror(fs) != 0) {
    freelog(LOG_ERROR, "Error before closing %s: %s", real_filename,
	    fz_strerror(fs));
    fz_fclose(fs);
    return FALSE;
  }
  if (fz_fclose(fs) != 0) {
    freelog(LOG_ERROR, "Error closing %s", real_filename);
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
char *secfile_lookup_str(struct section_file *my_section_file, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  if(!(pentry=section_file_lookup_internal(my_section_file, buf))) {
    freelog(LOG_FATAL, "sectionfile %s doesn't contain a '%s' entry",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }

  if(!pentry->svalue) {
    freelog(LOG_FATAL, "sectionfile %s entry '%s' doesn't contain a string",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }
  
  return pentry->svalue;
}

/**************************************************************************
  Lookup string or int value; if (char*) return is NULL, int value is
  put into (*ival).
**************************************************************************/
char *secfile_lookup_str_int(struct section_file *my_section_file, 
			     int *ival, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  assert(ival != NULL);
  
  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  if(!(pentry=section_file_lookup_internal(my_section_file, buf))) {
    freelog(LOG_FATAL, "sectionfile %s doesn't contain a '%s' entry",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }

  if(pentry->svalue) {
    return pentry->svalue;
  } else {
    *ival = pentry->ivalue;
    return NULL;
  }
}
      
/**************************************************************************
...
**************************************************************************/
void secfile_insert_int(struct section_file *my_section_file,
			int val, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  pentry=section_file_insert_internal(my_section_file, buf);

  pentry->ivalue=val;
}

/**************************************************************************
...
**************************************************************************/
void secfile_insert_int_comment(struct section_file *my_section_file,
				int val, const char *const comment,
				const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  pentry = section_file_insert_internal(my_section_file, buf);

  pentry->ivalue = val;
  pentry->comment = sbuf_strdup(my_section_file->sb, comment);
}

/**************************************************************************
...
**************************************************************************/
void secfile_insert_bool(struct section_file *my_section_file,
			 bool val, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  pentry=section_file_insert_internal(my_section_file, buf);

  if (val != TRUE && val != FALSE) {
    freelog(LOG_ERROR, "Trying to insert a non-boolean (%d) at key %s",
	    (int) val, buf);
    val = TRUE;
  }

  pentry->ivalue = val ? 1 : 0;
}

/**************************************************************************
...
**************************************************************************/
void secfile_insert_str(struct section_file *my_section_file,
			const char *sval, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  pentry = section_file_insert_internal(my_section_file, buf);
  pentry->svalue = sbuf_strdup(my_section_file->sb, sval);
}

/**************************************************************************
...
**************************************************************************/
void secfile_insert_str_noescape(struct section_file *my_section_file,
			const char *sval, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  pentry = section_file_insert_internal(my_section_file, buf);
  pentry->svalue = sbuf_strdup(my_section_file->sb, sval);
  pentry->escaped = FALSE;
}



/**************************************************************************
...
**************************************************************************/
void secfile_insert_str_comment(struct section_file *my_section_file,
				char *sval, const char *const comment,
				const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  pentry = section_file_insert_internal(my_section_file, buf);
  pentry->svalue = sbuf_strdup(my_section_file->sb, sval);
  pentry->comment = sbuf_strdup(my_section_file->sb, comment);
}

/**************************************************************************
  Insert string vector into section_file. It will be writen out as:
    name = "value1", "value2", "value3"
  The vector must have at least one element in it.

  This function is little tricky, because values inserted here can't
  be immediately recovered by secfile_lookup_str_vec. Luckily we never use
  section_file for both reading and writing.
**************************************************************************/
void secfile_insert_str_vec(struct section_file *my_section_file,
			    const char **values, int dim,
			    const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  int i;
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  assert(dim > 0);
  
  pentry = section_file_insert_internal(my_section_file, buf);
  pentry->dim = dim;
  pentry->vec_values = sbuf_malloc(my_section_file->sb,
                                   sizeof(char*) * dim);
  for (i = 0; i < dim; i++) {
    pentry->vec_values[i] = sbuf_strdup(my_section_file->sb, values[i]);
  }
}

/**************************************************************************
...
**************************************************************************/
int secfile_lookup_int(struct section_file *my_section_file, 
		       const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  if(!(pentry=section_file_lookup_internal(my_section_file, buf))) {
    freelog(LOG_FATAL, "sectionfile %s doesn't contain a '%s' entry",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }

  if(pentry->svalue) {
    freelog(LOG_FATAL, "sectionfile %s entry '%s' doesn't contain an integer",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }
  
  return pentry->ivalue;
}


/**************************************************************************
  As secfile_lookup_int(), but return a specified default value if the
  entry does not exist.  If the entry exists as a string, then die.
**************************************************************************/
int secfile_lookup_int_default(struct section_file *my_section_file,
			       int def, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  if(!(pentry=section_file_lookup_internal(my_section_file, buf))) {
    return def;
  }
  if(pentry->svalue) {
    freelog(LOG_FATAL, "sectionfile %s contains a '%s', but string not integer",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }
  return pentry->ivalue;
}

/**************************************************************************
  As secfile_lookup_int_default(), but also check the range [min/max].
**************************************************************************/
int secfile_lookup_int_default_min_max(error_func_t error_handle,
                                       struct section_file *my_section_file,
                                       int def, int minval, int maxval,
                                       const char *path, ...)
{
  assert(error_handle != NULL);

  char buf[MAX_LEN_BUFFER];
  va_list ap;
  int ival;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  ival = secfile_lookup_int_default(my_section_file, def, "%s", buf);

  if(ival < minval) {
    error_handle(LOG_ERROR, "sectionfile %s: '%s' should be in the "
                            "interval [%d, %d] but is %d; using the "
                            "minimal value.",
                 secfile_filename(my_section_file), buf, minval, maxval,
                 ival);
    ival = minval;
  }

  if(ival > maxval) {
    error_handle(LOG_ERROR, "sectionfile %s: '%s' should be in the "
                            "interval [%d, %d] but is %d; using the "
                            "maximal value.",
                 secfile_filename(my_section_file), buf, minval, maxval,
                 ival);
    ival = maxval;
  }

  return ival;
}

/**************************************************************************
...
**************************************************************************/
bool secfile_lookup_bool(struct section_file *my_section_file, 
		       const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  if(!(pentry=section_file_lookup_internal(my_section_file, buf))) {
    freelog(LOG_FATAL, "sectionfile %s doesn't contain a '%s' entry",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }

  if(pentry->svalue) {
    freelog(LOG_FATAL, "sectionfile %s entry '%s' doesn't contain an integer",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }

  if (pentry->ivalue != 0 && pentry->ivalue != 1) {
    freelog(LOG_ERROR, "Value read for key %s isn't boolean: %d", buf,
	    pentry->ivalue);
    pentry->ivalue = 1;
  }
  
  return pentry->ivalue != 0;
}


/**************************************************************************
  As secfile_lookup_bool(), but return a specified default value if the
  entry does not exist.  If the entry exists as a string, then die.
**************************************************************************/
bool secfile_lookup_bool_default(struct section_file *my_section_file,
				 bool def, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  if(!(pentry=section_file_lookup_internal(my_section_file, buf))) {
    return def;
  }
  if(pentry->svalue) {
    freelog(LOG_FATAL, "sectionfile %s contains a '%s', but string not integer",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }

  if (pentry->ivalue != 0 && pentry->ivalue != 1) {
    freelog(LOG_ERROR, "Value read for key %s isn't boolean: %d", buf,
	    pentry->ivalue);
    pentry->ivalue = 1;
  }
  
  return pentry->ivalue != 0;
}

/**************************************************************************
  As secfile_lookup_str(), but return a specified default (char*) if the
  entry does not exist.  If the entry exists as an int, then die.
**************************************************************************/
char *secfile_lookup_str_default(struct section_file *my_section_file, 
				 const char *def, const char *path, ...)
{
  struct entry *pentry;
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  if(!(pentry=section_file_lookup_internal(my_section_file, buf))) {
    return (char *) def;
  }

  if(!pentry->svalue) {
    freelog(LOG_FATAL, "sectionfile %s contains a '%s', but integer not string",
	    secfile_filename(my_section_file), buf);
    exit(EXIT_FAILURE);
  }
  
  return pentry->svalue;
}

/**************************************************************************
...
**************************************************************************/
bool section_file_lookup(struct section_file *my_section_file, 
			 const char *path, ...)
{
  char buf[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  return section_file_lookup_internal(my_section_file, buf) != NULL;
}


/**************************************************************************
...
**************************************************************************/
static struct entry*
section_file_lookup_internal(struct section_file *my_section_file,  
			     char *fullpath) 
{
  char *pdelim;
  char sec_name[MAX_LEN_BUFFER];
  char ent_name[MAX_LEN_BUFFER];
  char mod_fullpath[2*MAX_LEN_BUFFER];
  int len;
  struct entry *result;
  struct section *psection;

  /* freelog(LOG_DEBUG, "looking up: %s", fullpath); */
  
  /* treat "sec.foo,0" as "sec.foo": */
  len = strlen(fullpath);
  if(len>2 && fullpath[len-2]==',' && fullpath[len-1]=='0') {
    assert(len<sizeof(mod_fullpath));
    strcpy(mod_fullpath, fullpath);
    fullpath = mod_fullpath;	/* reassign local pointer 'fullpath' */
    fullpath[len-2] = '\0';
  }
  
  if (secfilehash_hashash(my_section_file)) {
    result = hash_lookup_data(my_section_file->hashd->htbl, fullpath);
    if (result) {
      result->used++;
    }
    return result;
  }

  /* i dont like strtok */
  pdelim = strchr(fullpath, '.');
  if (!pdelim) {
    return NULL;
  }

  (void) mystrlcpy(sec_name, fullpath,
		   MIN(pdelim - fullpath + 1, sizeof(sec_name)));
  sz_strlcpy(ent_name, pdelim+1);

  psection = find_section_by_name(my_section_file, sec_name);
  if (psection) {
    entry_list_iterate(psection->entries, pentry) {
      if (strcmp(pentry->name, ent_name) == 0) {
	result = pentry;
	result->used++;
	return result;
      }
    } entry_list_iterate_end;
  }

  return NULL;
}

/**************************************************************************
 The caller should ensure that "fullpath" should not refer to an entry
 which already exists in "my_section_file".  (Actually, in some cases
 now it is ok to have duplicate entries, but be careful...)
**************************************************************************/
static struct entry*
section_file_insert_internal(struct section_file *my_section_file, 
			     char *fullpath)
{
  char *pdelim;
  char sec_name[MAX_LEN_BUFFER];
  char ent_name[MAX_LEN_BUFFER];
  struct section *psection;
  struct entry *pentry;
  struct sbuffer *sb = my_section_file->sb;

  if(!(pdelim=strchr(fullpath, '.'))) { /* d dont like strtok */
    freelog(LOG_FATAL,
	    "Insertion fullpath \"%s\" missing '.' for sectionfile %s",
	    fullpath, secfile_filename(my_section_file));
    exit(EXIT_FAILURE);
  }
  (void) mystrlcpy(sec_name, fullpath,
		   MIN(pdelim - fullpath + 1, sizeof(sec_name)));
  sz_strlcpy(ent_name, pdelim+1);
  my_section_file->num_entries++;
  
  if(strlen(sec_name)==0 || strlen(ent_name)==0) {
    freelog(LOG_FATAL,
	    "Insertion fullpath \"%s\" missing %s for sectionfile %s",
	    fullpath, (strlen(sec_name)==0 ? "section" : "entry"),
	    secfile_filename(my_section_file));
    exit(EXIT_FAILURE);
  }

  psection = find_section_by_name(my_section_file, sec_name);
  if (psection) {
    /* This DOES NOT check whether the entry already exists in
     * the section, to avoid O(N^2) behaviour.
     */
    pentry = sbuf_malloc(sb, sizeof(struct entry));
    entry_init(pentry);
    pentry->name = sbuf_strdup(sb, ent_name);
    entry_list_append(psection->entries, pentry);
    return pentry;
  }

  psection = section_file_append_section(my_section_file, sec_name);

  pentry = sbuf_malloc(sb, sizeof(struct entry));
  entry_init(pentry);
  pentry->name = sbuf_strdup(sb, ent_name);
  entry_list_append(psection->entries, pentry);

  return pentry;
}


/**************************************************************************
 Return 0 if the section_file has not been setup for hashing.
**************************************************************************/
static bool secfilehash_hashash(struct section_file *file)
{
  return (file->hashd && hash_num_buckets(file->hashd->htbl) != 0);
}

/**************************************************************************
  Basic checks for existence/integrity of hash data and fail if bad.
**************************************************************************/
static void secfilehash_check(struct section_file *file)
{
  if (!secfilehash_hashash(file)) {
    freelog(LOG_FATAL, "sectionfile %s hash operation before setup",
	    secfile_filename(file));
    exit(EXIT_FAILURE);
  }
  if (file->num_entries != file->hashd->num_entries_hashbuild) {
    freelog(LOG_FATAL, "sectionfile %s has more entries than when hash built",
	    secfile_filename(file));
    exit(EXIT_FAILURE);
  }
}

/**************************************************************************
 Insert a entry into the hash table.  The key is malloced here (using sbuf;
 malloc somewhere required by hash implementation).
**************************************************************************/
static void secfilehash_insert(struct section_file *file,
			       char *key, struct entry *data)
{
  struct entry *hentry;

  key = sbuf_strdup(file->sb, key);
  hentry = hash_replace(file->hashd->htbl, key, data);
  if (hentry) {
    if (file->hashd->allow_duplicates) {
      hentry->used = 1;
      file->hashd->num_duplicates++;
      /* Subsequent entries replace earlier ones; could do differently so
	 that first entry would be used and following ones ignored.
	 Need to mark the replaced one as used or else it will show
	 up when we iterate the sections and entries (since hash
	 lookup will never find it to mark it as used).
      */
    } else {
      freelog(LOG_FATAL, "Tried to insert same value twice: %s (sectionfile %s)",
	      key, secfile_filename(file));
      exit(EXIT_FAILURE);
    }
  }
}

/**************************************************************************
 Build a hash table for the file.  Note that the section_file should
 not be modified (except to free it) subsequently.
 If allow_duplicates is true, then relax normal condition that
 all entries must have unique names; in this case for duplicates
 the hash ref will be to the _last_ entry.
**************************************************************************/
static void secfilehash_build(struct section_file *file, bool allow_duplicates)
{
  struct hash_data *hashd;
  char buf[256];

  hashd = file->hashd = fc_malloc(sizeof(struct hash_data));
  hashd->htbl = hash_new_nentries(hash_fval_string, hash_fcmp_string,
				  file->num_entries);
  
  hashd->num_entries_hashbuild = file->num_entries;
  hashd->allow_duplicates = allow_duplicates;
  hashd->num_duplicates = 0;
  
  section_list_iterate(file->sections, psection) {
    entry_list_iterate(psection->entries, pentry) {
      my_snprintf(buf, sizeof(buf), "%s.%s", psection->name, pentry->name);
      secfilehash_insert(file, buf, pentry);
    }
    entry_list_iterate_end;
  }
  section_list_iterate_end;
  
  if (hashd->allow_duplicates) {
    freelog(LOG_DEBUG, "Hash duplicates during build: %d",
	    hashd->num_duplicates);
  }
}


/**************************************************************************
 Free the memory allocated for the hash table.
**************************************************************************/
static void secfilehash_free(struct section_file *file)
{
  secfilehash_check(file);
  hash_free(file->hashd->htbl);
  free(file->hashd);
  file->hashd = NULL;
}

/**************************************************************************
 Returns the number of elements in a "vector".
 That is, returns the number of consecutive entries in the sequence:
 "path,0" "path,1", "path,2", ...
 If none, returns 0.
**************************************************************************/
int secfile_lookup_vec_dimen(struct section_file *my_section_file, 
			     const char *path, ...)
{
  char buf[MAX_LEN_BUFFER];
  va_list ap;
  int j=0;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  while(section_file_lookup(my_section_file, "%s,%d", buf, j)) {
    j++;
  }
  return j;
}

/**************************************************************************
 Return a pointer for a list of integers for a "vector".
 The return value is malloced here, and should be freed by the user.
 The size of the returned list is returned in (*dimen).
 If the vector does not exist, returns NULL ands sets (*dimen) to 0.
**************************************************************************/
int *secfile_lookup_int_vec(struct section_file *my_section_file,
			    int *dimen, const char *path, ...)
{
  char buf[MAX_LEN_BUFFER];
  va_list ap;
  int j, *res;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  *dimen = secfile_lookup_vec_dimen(my_section_file, "%s", buf);
  if (*dimen == 0) {
    return NULL;
  }
  res = fc_malloc((*dimen)*sizeof(int));
  for(j=0; j<(*dimen); j++) {
    res[j] = secfile_lookup_int(my_section_file, "%s,%d", buf, j);
  }
  return res;
}

/**************************************************************************
 Return a pointer for a list of "strings" for a "vector".
 The return value is malloced here, and should be freed by the user,
 but the strings themselves are contained in the sectionfile
 (as when a single string is looked up) and should not be altered
 or freed by the user.
 The size of the returned list is returned in (*dimen).
 If the vector does not exist, returns NULL ands sets (*dimen) to 0.
**************************************************************************/
char **secfile_lookup_str_vec(struct section_file *my_section_file,
			      int *dimen, const char *path, ...)
{
  char buf[MAX_LEN_BUFFER];
  va_list ap;
  int j;
  char **res;

  va_start(ap, path);
  my_vsnprintf(buf, sizeof(buf), path, ap);
  va_end(ap);

  *dimen = secfile_lookup_vec_dimen(my_section_file, "%s", buf);
  if (*dimen == 0) {
    return NULL;
  }
  res = fc_malloc((*dimen)*sizeof(char*));
  for(j=0; j<(*dimen); j++) {
    res[j] = secfile_lookup_str(my_section_file, "%s,%d", buf, j);
  }
  return res;
}

/***************************************************************
  Copies a string. Backslash followed by a genuine newline always
  removes the newline.
  If full_escapes is TRUE:
    - '\n' -> newline translation.
    - Other '\c' sequences (any character 'c') are just passed
      through with the '\' removed (eg, includes '\\', '\"')
***************************************************************/
static char *minstrdup(struct sbuffer *sb, const char *str,
                       bool full_escapes)
{
  char *dest = sbuf_malloc(sb, strlen(str)+1);
  char *d2=dest;
  if(dest) {
    while (*str != '\0') {
      if (*str == '\\' && *(str+1) == '\n') {
        /* Escape followed by newline. Skip both */
        str += 2;
      } else if (full_escapes && *str=='\\') {
	str++;
        if (*str=='n') {
	  *dest++='\n';
	  str++;
	}
      } else {
	*dest++=*str++;
      }

    }

    *dest='\0';
  }
  return d2;
}

/***************************************************************
 Returns a pointer to an internal buffer (can only get one
 string at a time) with str escaped the opposite of minstrdup.
 Specifically, any newline, backslash, or double quote is
 escaped with a backslash.
 Adds appropriate delimiters: "" if escaped, $$ unescaped.
 The internal buffer is grown as necessary, and not normally
 freed (since this will be called frequently.)  A call with
 str=NULL frees the buffer and does nothing else (returns NULL).
***************************************************************/
static char *moutstr(char *str, bool full_escapes)
{
  static char *buf = NULL;
  static int nalloc = 0;

  int len;			/* required length, including terminator */
  char *c, *dest;

  if (!str) {
    freelog(LOG_DEBUG, "moutstr alloc was %d", nalloc);
    free(buf);
    buf = NULL;
    nalloc = 0;
    return NULL;
  }
  
  len = strlen(str)+3;
  if (full_escapes) {
    for(c=str; *c != '\0'; c++) {
      if (*c == '\n' || *c == '\\' || *c == '\"') {
        len++;
      }
    }
  }
  if (len > nalloc) {
    nalloc = 2 * len + 1;
    buf = fc_realloc(buf, nalloc);
  }
  
  dest = buf;
  *dest++ = (full_escapes ? '\"' : '$');
  while(*str != '\0') {
    if (full_escapes && (*str == '\n' || *str == '\\' || *str == '\"')) {
      *dest++ = '\\';
      if (*str == '\n') {
	*dest++ = 'n';
	str++;
      } else {
	*dest++ = *str++;
      }
    } else {
      *dest++ = *str++;
    }
  }
  *dest++ = (full_escapes ? '\"' : '$');
  *dest = '\0';
  return buf;
}

/***************************************************************
  Returns pointer to list of strings giving all section names
  which start with prefix, and sets the number of such sections
  in (*num).  If there are none such, returns NULL and sets
  (*num) to zero.  The returned pointer is malloced, and it is
  the responsibilty of the caller to free this pointer, but the
  actual strings pointed to are part of the section_file data,
  and should not be freed by the caller (nor used after the
  section_file has been freed or changed).  The section names
  returned are in the order they appeared in the original file.
***************************************************************/
char **secfile_get_secnames_prefix(struct section_file *my_section_file,
				   const char *prefix, int *num)
{
  char **ret;
  int len, i;

  len = strlen(prefix);

  /* count 'em: */
  i = 0;
  section_list_iterate(my_section_file->sections, psection) {
    if (strncmp(psection->name, prefix, len) == 0) {
      i++;
    }
  }
  section_list_iterate_end;
  (*num) = i;

  if (i==0) {
    return NULL;
  }
  
  ret = fc_malloc((*num)*sizeof(char*));

  i = 0;
  section_list_iterate(my_section_file->sections, psection) {
    if (strncmp(psection->name, prefix, len) == 0) {
      ret[i++] = psection->name;
    }
  }
  section_list_iterate_end;
  return ret;
}

/****************************************************************************
  Returns a pointer to a list of strings giving all keys in the given section
  and sets the number of such sections in (*num).  If there are no keys
  found, the function returns NULL and sets (*num) to zero.  The returned
  pointer is malloced, and it is the responsibilty of the caller to free this
  pointer (unless it's NULL), but the actual strings pointed to are part of
  the section_file data, and should not be freed by the caller (nor used
  after the section_file has been freed or changed, so the caller may need to
  strdup them to keep them around).  The order of the returned names is
  unspecified.
****************************************************************************/
char **secfile_get_section_entries(struct section_file *my_section_file,
				   const char *section, int *num)
{
  char **ret;
  int i;
  struct section *psection = find_section_by_name(my_section_file,section);

  if (!psection) {
    *num = 0;
    return NULL;
  }

  *num = entry_list_size(psection->entries);

  if (*num == 0) {
    return NULL;
  }

  ret = fc_malloc((*num) * sizeof(*ret));

  i = 0;  
  entry_list_iterate(psection->entries, pentry) {
    ret[i++] = pentry->name;
  } entry_list_iterate_end;

  return ret;
}

/****************************************************************************
  Returns TRUE if the given section exists in the secfile.
****************************************************************************/
bool secfile_has_section(const struct section_file *sf,
                         const char *section_name_fmt, ...)
{
  char name[MAX_LEN_BUFFER];
  va_list ap;

  va_start(ap, section_name_fmt);
  my_vsnprintf(name, sizeof(name), section_name_fmt, ap);
  va_end(ap);

  return hash_key_exists(sf->hash_sections, name);
}
