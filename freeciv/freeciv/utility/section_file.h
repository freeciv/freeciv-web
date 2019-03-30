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
#ifndef FC__SECTION_FILE_H
#define FC__SECTION_FILE_H

/* This header contains internals of section_file that its users should
 * not care about. This header should be included by soruce files implementing
 * registry itself. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"

/* Section structure. */
struct section {
  struct section_file *secfile; /* Parent structure. */
  enum entry_special_type special;
  char *name;                   /* Name of the section. */
  struct entry_list *entries;   /* The list of the children. */
};

/* The section file struct itself. */
struct section_file {
  char *name;                           /* Can be NULL. */
  size_t num_entries;
  /* num_includes should be size_t, but as there's no truly portable
   * printf format for size_t and we need to construct string containing
   * num_includes with fc_snprintf(), we set for unsigned int. */
  unsigned int num_includes;
  unsigned int num_long_comments;
  struct section_list *sections;
  bool allow_duplicates;
  bool allow_digital_boolean;
  struct {
    struct section_hash *sections;
    struct entry_hash *entries;
  } hash;
};

void secfile_log(const struct section_file *secfile,
                 const struct section *psection,
                 const char *file, const char *function, int line,
                 const char *format, ...)
  fc__attribute((__format__(__printf__, 6, 7)));

#define SECFILE_LOG(secfile, psection, format, ...)                         \
  secfile_log(secfile, psection, __FILE__, __FUNCTION__, __FC_LINE__,       \
              format, ## __VA_ARGS__)
#define SECFILE_RETURN_IF_FAIL(secfile, psection, condition)                \
  if (!(condition)) {                                                       \
    SECFILE_LOG(secfile, psection, "Assertion '%s' failed.", #condition);   \
    return;                                                                 \
  }
#define SECFILE_RETURN_VAL_IF_FAIL(secfile, psection, condition, value)     \
  if (!(condition)) {                                                       \
    SECFILE_LOG(secfile, psection, "Assertion '%s' failed.", #condition);   \
    return value;                                                           \
  }

#define SPECHASH_TAG section
#define SPECHASH_CSTR_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct section *
#include "spechash.h"

#define SPECHASH_TAG entry
#define SPECHASH_ASTR_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct entry *
#include "spechash.h"

bool entry_from_token(struct section *psection, const char *name,
                      const char *tok);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__SECTION_FILE_H */
