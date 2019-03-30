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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "shared.h"

struct section;

void registry_module_init(void);
void registry_module_close(void);

struct section_file *secfile_new(bool allow_duplicates);
void secfile_destroy(struct section_file *secfile);
struct section_file *secfile_load(const char *filename,
                                  bool allow_duplicates);

void secfile_allow_digital_boolean(struct section_file *secfile,
                                   bool allow_digital_boolean);

const char *secfile_error(void);
const char *section_name(const struct section *psection);

#include "registry_ini.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__REGISTRY_H */
