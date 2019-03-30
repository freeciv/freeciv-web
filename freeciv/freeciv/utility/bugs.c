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
#endif /* HAVE_CONFIG_H */

#include "fc_prehdrs.h"

#include <stdarg.h>

/* utility */
#include "fcintl.h"
#include "log.h"

#include "bugs.h"

/********************************************************************//**
  Print request for bugreport
************************************************************************/
void bugreport_request(const char *reason_format, ...)
{
  va_list args;
  char buf[1024];

  va_start(args, reason_format);
  vdo_log(__FILE__, __FUNCTION__, __FC_LINE__, FALSE, LOG_ERROR,
          buf, sizeof(buf), reason_format, args);
  va_end(args);

  /* TRANS: No full stop after the URL, could cause confusion. */
  log_error(_("Please report this message at %s"), BUG_URL);
}
