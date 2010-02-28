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

#include "aws4c.h"
#include "netsave.h"

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


bool net_file_save(struct section_file *my_section_file,
                       const char *filename)
{
  char real_filename[1024];
  const struct genlist_link *ent_iter, *save_iter, *col_iter;
  struct entry *pentry, *col_pentry;
  int i;
  char S[256];

  aws_init ();
  aws_set_debug  ( 0 );
  int rc = aws_read_config  ( "freeciv" );
  if ( rc )
    {
      puts ( "Could not find a credential in the config file" );
      puts ( "Make sure your ~/.awsAuth file is correct" );
      exit ( 1 );
    }

  s3_set_bucket ("freeciv-net-savegames");
  s3_set_mime ("text/plain");
  s3_set_acl ("public-read");

  IOBuf * bf = aws_iobuf_new ();

  /* SAVE GAME START  */

  interpret_tilde(real_filename, sizeof(real_filename), filename);


  section_list_iterate(my_section_file->sections, psection) {
    //fz_fprintf(fs, "\n[%s]\n", psection->name);
    my_snprintf (S, sizeof(S), "\n[%s]\n", psection->name);
    aws_iobuf_append ( bf, S, strlen(S));

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
	//fz_fprintf(fs, "%s={", base);
        my_snprintf (S, sizeof(S), "%s={", base);
        aws_iobuf_append ( bf, S, strlen(S));



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
	  //fz_fprintf(fs, "%c\"%s\"", (ncol==0?' ':','), col_pentry->name+offset);
          my_snprintf (S, sizeof(S), "%c\"%s\"", (ncol==0?' ':','), col_pentry->name+offset);
          aws_iobuf_append ( bf, S, strlen(S));



	  ncol++;
	}
	//fz_fprintf(fs, "\n");
        my_snprintf (S, sizeof(S), "\n");
        aws_iobuf_append ( bf, S, strlen(S));


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
	      //fz_fprintf(fs, "\n");
              my_snprintf (S, sizeof(S), "\n");
              aws_iobuf_append ( bf, S, strlen(S));


	    }
	    //fz_fprintf(fs, "}\n");
            my_snprintf (S, sizeof(S), "}\n");
            aws_iobuf_append ( bf, S, strlen(S));


	    break;
	  }

	  if(icol>0) {
	    //fz_fprintf(fs, ",");
            my_snprintf (S, sizeof(S), ",");
            aws_iobuf_append ( bf, S, strlen(S));
           }

	  if(pentry->svalue) {
	    //fz_fprintf(fs, "%s", moutstr(pentry->svalue, pentry->escaped));
             my_snprintf (S, sizeof(S), "%s", moutstr(pentry->svalue, pentry->escaped));
             aws_iobuf_append ( bf, S, strlen(S));

	  } else {
	    //fz_fprintf(fs, "%d", pentry->ivalue);
             my_snprintf (S, sizeof(S), "%d", pentry->ivalue);
             aws_iobuf_append ( bf, S, strlen(S));

          }

          ent_iter = genlist_link_next(ent_iter);
          col_iter = genlist_link_next(col_iter);
	  
	  icol++;
	  if(icol==ncol) {
	    //fz_fprintf(fs, "\n");
            my_snprintf (S, sizeof(S), "\n");
            aws_iobuf_append ( bf, S, strlen(S));

	    irow++;
	    icol = 0;
	    col_iter = save_iter;
	  }
	}
	if(!pentry) break;
      }
      if(!pentry) break;
      
      if (pentry->vec_values) {
        //fz_fprintf(fs, "%s=%s", pentry->name,
	//           moutstr(pentry->vec_values[0], pentry->escaped));
        my_snprintf (S, sizeof(S), "%s=%s", pentry->name,
	           moutstr(pentry->vec_values[0], pentry->escaped));
        aws_iobuf_append ( bf, S, strlen(S));


	for (i = 1; i < pentry->dim; i++) {
	  //fz_fprintf(fs, ", %s", moutstr(pentry->vec_values[i], pentry->escaped));
           my_snprintf (S, sizeof(S), ", %s", moutstr(pentry->vec_values[i], pentry->escaped) );
           aws_iobuf_append ( bf, S, strlen(S));

	}
      } else if (pentry->svalue) {
	//fz_fprintf(fs, "%s=%s", pentry->name, moutstr(pentry->svalue, pentry->escaped));
           my_snprintf (S, sizeof(S), "%s=%s", pentry->name, moutstr(pentry->svalue, pentry->escaped));
           aws_iobuf_append ( bf, S, strlen(S));

      } else {
	//fz_fprintf(fs, "%s=%d", pentry->name, pentry->ivalue);
        my_snprintf (S, sizeof(S), "%s=%d", pentry->name, pentry->ivalue);
        aws_iobuf_append ( bf, S, strlen(S));


      }
      
      if (pentry->comment) {
	//fz_fprintf(fs, "  # %s\n", pentry->comment);
        my_snprintf (S, sizeof(S), "  # %s\n", pentry->comment);
        aws_iobuf_append ( bf, S, strlen(S));


      } else {
	//fz_fprintf(fs, "\n");
        my_snprintf (S, sizeof(S), "\n");
        aws_iobuf_append ( bf, S, strlen(S));


      }
    }
  }
  section_list_iterate_end;
  
  (void) moutstr(NULL, TRUE);		/* free internal buffer */

  /* SAVE GAME END*/

  int rv = s3_put ( bf, real_filename );

  printf ( "RV %d\n", rv );

  printf ( "CODE    [%d] \n", bf->code );
  printf ( "RESULT  [%s] \n", bf->result );
  printf ( "LEN     [%d] \n", bf->len );
  printf ( "LASTMOD [%s] \n", bf->lastMod );
  printf ( "ETAG    [%s] \n", bf->eTag );

  while(-1)
    {
  char Ln[1024];
  int sz = aws_iobuf_getline ( bf, Ln, sizeof(Ln));
  if ( Ln[0] == 0 ) break;
  printf ( "S[%3d] %s", sz, Ln );
    }


  return TRUE;

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
  Load a section_file, but disallow (die on) duplicate entries.
**************************************************************************/
bool net_file_load(struct section_file *my_section_file,
			           const char *filename)
{
  char real_filename[1024];
  struct inputfile *inf;
  bool success;

  char savedata[512000];


  FILE *stream;

  savedata[0] = '\0';

  aws_init ();
  aws_set_debug ( 0 );
  int rc = aws_read_config  ( "freeciv" );
  if ( rc )
    {
      puts ( "Could not find a credential in the config file" );
      puts ( "Make sure your ~/.awsAuth file is correct" );
      exit ( 1 );
    }

  s3_set_host ( "s3.amazonaws.com");
  s3_set_bucket ("freeciv-net-savegames");


  IOBuf * bf = aws_iobuf_new ();

  interpret_tilde(real_filename, sizeof(real_filename), filename);

  int rv = s3_get ( bf, real_filename );

  printf ( "RV %d\n", rv );

  printf ( "CODE    [%d] \n", bf->code );
  printf ( "RESULT  [%s] \n", bf->result );
  printf ( "LEN     [%d] \n", bf->len );
  printf ( "C-LEN   [%d] \n", bf->contentLen );
  printf ( "LASTMOD [%s] \n", bf->lastMod );
  printf ( "ETAG    [%s] \n", bf->eTag );

  while (-1) {
    char Ln[1024];
    int sz = aws_iobuf_getline ( bf, Ln, sizeof(Ln));
    if ( Ln[0] == 0 ) break;
    sz_strlcat(savedata, Ln);
  }

  stream = fmemopen (savedata, strlen (savedata), "r");

  inf = inf_from_stream(fz_from_stream(stream), datafilename);

  if (!inf) {
    return FALSE;
  }
  success = section_file_read_dup(my_section_file, real_filename,
			       inf, FALSE, NULL);

  inf_close(inf);

  return success;
}

