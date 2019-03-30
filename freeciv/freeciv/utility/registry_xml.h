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
#ifndef FC__REGISTRY_XML_H
#define FC__REGISTRY_XML_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef FREECIV_HAVE_XML_REGISTRY

#define FCXML_CAPSTR "+xml"

struct section_file *xmlfile_load(xmlDoc *sec_doc, const char *filename);

#endif /* FREECIV_HAVE_XML_REGISTRY */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__REGISTRY_XML_H */
