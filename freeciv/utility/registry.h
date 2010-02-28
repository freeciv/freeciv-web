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
#ifndef FC__REGISTRY_H
#define FC__REGISTRY_H

#include "ioz.h"
#include "shared.h"		/* bool type and fc__attribute */

struct sbuffer;
struct hash_data;
struct section_list;

struct section_file {
  char *filename;
  int num_entries;
  struct section_list *sections;
  struct hash_table *hash_sections;
  struct hash_data *hashd;
  struct sbuffer *sb;
};

typedef void (*error_func_t)(int loglevel, const char *format, ...)
                             fc__attribute((__format__ (__printf__, 2, 3)));

void section_file_init(struct section_file *file);
bool section_file_load(struct section_file *my_section_file,
		       const char *filename);
bool section_file_load_section(struct section_file *my_section_file,
			       const char *filename, const char *section);
bool section_file_load_nodup(struct section_file *my_section_file,
			    const char *filename);
bool section_file_load_from_stream(struct section_file *my_section_file,
				   fz_FILE * stream);
bool section_file_save(struct section_file *my_section_file,
                       const char *filename, int compression_level,
                       enum fz_method compression_method);
void section_file_free(struct section_file *file);
void section_file_check_unused(struct section_file *file,
			       const char *filename);
const char *secfile_filename(const struct section_file *file);

void secfile_insert_int(struct section_file *my_section_file, 
			int val, const char *path, ...)
                        fc__attribute((__format__ (__printf__, 3, 4)));
void secfile_insert_int_comment(struct section_file *my_section_file,
				int val, const char *const comment, 
				const char *path, ...)
                                fc__attribute((__format__(__printf__, 4, 5)));
void secfile_insert_bool(struct section_file *my_section_file,
			 bool val, const char *path, ...)
                         fc__attribute((__format__(__printf__, 3, 4)));

void secfile_insert_str(struct section_file *my_section_file, 
			const char *sval, const char *path, ...)
                        fc__attribute((__format__ (__printf__, 3, 4)));

void secfile_insert_str_noescape(struct section_file *my_section_file, 
			const char *sval, const char *path, ...)
                        fc__attribute((__format__ (__printf__, 3, 4)));

void secfile_insert_str_comment(struct section_file *my_section_file,
				char *sval, const char *const comment,
				const char *path, ...)
                                fc__attribute((__format__ (__printf__, 4, 5)));
void secfile_insert_str_vec(struct section_file *my_section_file, 
			    const char **values, int dim,
			    const char *path, ...)
                            fc__attribute((__format__ (__printf__, 4, 5)));

bool section_file_lookup(struct section_file *my_section_file, 
			const char *path, ...)
                        fc__attribute((__format__ (__printf__, 2, 3)));

int secfile_lookup_int(struct section_file *my_section_file, 
		       const char *path, ...)
                       fc__attribute((__format__ (__printf__, 2, 3)));

bool secfile_lookup_bool(struct section_file *my_section_file, 
		        const char *path, ...)
                        fc__attribute((__format__ (__printf__, 2, 3)));
		       
char *secfile_lookup_str(struct section_file *my_section_file, 
			 const char *path, ...)
                         fc__attribute((__format__ (__printf__, 2, 3)));

char *secfile_lookup_str_int(struct section_file *my_section_file, 
			     int *ival, const char *path, ...)
                             fc__attribute((__format__ (__printf__, 3, 4)));

int secfile_lookup_int_default(struct section_file *my_section_file,
                               int def, const char *path, ...)
                               fc__attribute((__format__ (__printf__, 3, 4)));

int secfile_lookup_int_default_min_max(error_func_t error_handle,
                                       struct section_file *my_section_file,
                                       int def, int minval, int maxval,
                                       const char *path, ...)
                                       fc__attribute((__format__ (__printf__, 6, 7)));

bool secfile_lookup_bool_default(struct section_file *my_section_file,
				 bool def, const char *path, ...)
                                 fc__attribute((__format__ (__printf__, 3, 4)));

char *secfile_lookup_str_default(struct section_file *my_section_file, 
				 const char *def, const char *path, ...)
                                 fc__attribute((__format__ (__printf__, 3, 4)));

int secfile_lookup_vec_dimen(struct section_file *my_section_file, 
			     const char *path, ...)
                             fc__attribute((__format__ (__printf__, 2, 3)));
int *secfile_lookup_int_vec(struct section_file *my_section_file,
			    int *dimen, const char *path, ...)
                            fc__attribute((__format__ (__printf__, 3, 4)));
char **secfile_lookup_str_vec(struct section_file *my_section_file,
			      int *dimen, const char *path, ...)
                              fc__attribute((__format__ (__printf__, 3, 4)));

char **secfile_get_secnames_prefix(struct section_file *my_section_file,
				   const char *prefix, int *num);

bool secfile_has_section(const struct section_file *sf,
                         const char *section_name, ...)
                         fc__attribute((__format__ (__printf__, 2, 3)));
char **secfile_get_section_entries(struct section_file *my_section_file,
				   const char *section, int *num);

#endif  /* FC__REGISTRY_H */
