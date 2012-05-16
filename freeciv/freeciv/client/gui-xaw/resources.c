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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "resources.h"

/**************************************************************************
Fallback resources
**************************************************************************/
String fallback_resources[] = {
#include <Freeciv.h>
#if defined(HAVE_LIBXAW3D)
"Freeciv*international: False",
#else
"Freeciv*international: True",
"Freeciv*fontSet: -*-*-*-*-*-*-14-*",
#endif
  /* Deliberate use of angle-brackets instead of double quotes, to
     support compilation from another dir.  Then we "-I." (see Makefile.am)
     to include the locally generated Freeciv.h in the compilation dir,
     in preference to the one in the source dir (which is what double-quote
     include would use).  --dwp
  */
NULL,
};
