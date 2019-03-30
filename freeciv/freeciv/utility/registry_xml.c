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

#ifdef FREECIV_HAVE_XML_REGISTRY

#include <libxml/parser.h>

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "mem.h"
#include "registry.h"
#include "section_file.h"

#include "registry_xml.h"

/**********************************************************************//**
  Load section file from xml-file
**************************************************************************/
struct section_file *xmlfile_load(xmlDoc *sec_doc, const char *filename)
{
  struct section_file *secfile;
  xmlNodePtr xmlroot;
  xmlNodePtr current;
  char *cap;

  secfile = secfile_new(TRUE);

  xmlroot = xmlDocGetRootElement(sec_doc);
  if (xmlroot == NULL || strcmp((const char *)xmlroot->name, "Freeciv")) {
    /* TRANS: do not translate <Freeciv> */
    log_error(_("XML-file has no root node <Freeciv>"));
    secfile_destroy(secfile);
    return NULL;
  }

  cap = (char *)xmlGetNsProp(xmlroot, (xmlChar *)"options", NULL);

  if (cap == NULL) {
    log_error(_("XML-file has no capabilities defined!"));
    secfile_destroy(secfile);

    return NULL;
  }
  if (!has_capabilities(FCXML_CAPSTR, cap)) {
    log_error(_("XML-file has incompatible capabilities."));
    log_normal(_("Freeciv capabilities: %s"), FCXML_CAPSTR);
    log_normal(_("File capabilities: %s"), cap);
    secfile_destroy(secfile);

    return NULL;
  }

  if (filename) {
    secfile->name = fc_strdup(filename);
  } else {
    secfile->name = NULL;
  }

  current = xmlroot->children;

  while (current != NULL) {
    if (current->type == XML_ELEMENT_NODE) {
      struct section *psection;
      xmlNodePtr sec_node;

      psection = secfile_section_new(secfile, (const char *)current->name);
      sec_node = current->children;

      while (sec_node != NULL) {
        if (sec_node->type == XML_ELEMENT_NODE) {
          xmlNodePtr value_node = sec_node->children;

          while (value_node != NULL) {
            if (value_node->type == XML_TEXT_NODE) {
              const char *content = (const char *) xmlNodeGetContent(value_node);
              int len = strlen(content);
              char buf[len + 1];

              strncpy(buf, content, sizeof(buf));
              if (buf[0] == '"' && buf[len - 1] == '"') {
                buf[len - 1] = '\0';
              }

              if (!entry_from_token(psection, (const char *) sec_node->name,
                                    buf)) {
                log_error("Cannot parse token \"%s\"", content);
                secfile_destroy(secfile);
                return NULL;
              }
            }

            value_node = value_node->next;
          }
        }

        sec_node = sec_node->next;
      }
    }

    current = current->next;
  }

  return secfile;
}

#endif /* FREECIV_HAVE_XML_REGISTRY */
