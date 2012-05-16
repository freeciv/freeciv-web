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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/AsciiText.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "packets.h"
#include "shared.h"		/* wordwrap_string() */
#include "support.h"

#include "climisc.h"
#include "gui_main.h"
#include "gui_stuff.h"

#include "chatline.h"

/**************************************************************************
...
**************************************************************************/
void chatline_key_send(Widget w)
{
  String theinput;
  String empty="";

  XtVaGetValues(w, XtNstring, &theinput, NULL);
  
  if(*theinput) {
    send_chat(theinput);
  }

  XtVaSetValues(w, XtNstring, empty, NULL);
}

/**************************************************************************
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
**************************************************************************/
void real_output_window_append(const char *astring,
                               const struct text_tag_list *tags,
                               int conn_id)
{
  /* this is properly a bad way to append to a text widget. Using the 
   * "useStringInPlace" resource and doubling mem alloc'ing would be better.  
   * Nope - tried it and many other variations and it wasn't any better. 
   * I'll replace this widget with a widget supportting hyperlinks later, so
   * leth's forget the problem.
   *
   * There seems to be an Xaw problem with doing both wrap and scroll:
   * its supposed to automatically scroll to end when we change the insertion
   * point, but if a line is wrapped the scroll lags behind a line, and
   * stays behind on subsequent additions, until the too-long line scrolls
   * off the top.  (I tried setting the insert position to the last char,
   * instead of the start of line as below, but that didn't help.)  So we
   * split the line ourselves.  I'm just using a fixed length split; should
   * perhaps check and use width of output window (but font size?)  -dwp
   *
   * Now uses window's font size and width.  Assumes fixed-width font.  --jjm
   */
  String theoutput;
  char *newout, *rmcr, *astring2 = mystrdup(astring);

  XtVaGetValues(outputwindow_text, XtNstring, &theoutput, NULL);
  newout=fc_malloc(strlen(astring2)+strlen(theoutput)+2);
  sprintf(newout, "%s\n%s", theoutput, astring2);

  /* calc carret position - last line, first pos */ 
  for(rmcr=newout+strlen(newout); rmcr>newout; rmcr--)
    if(*rmcr=='\n')
      break;

  /* shit happens when setting both values at the same time */
  XawTextDisableRedisplay(outputwindow_text);
  XtVaSetValues(outputwindow_text, XtNstring, newout, NULL);
  XtVaSetValues(outputwindow_text, XtNinsertPosition, rmcr-newout+1, NULL);
  XawTextEnableRedisplay(outputwindow_text);
  
  free(newout);
  free(astring2);
}

/**************************************************************************
 I have no idea what module this belongs in -- Syela
 I've decided to put output_window routines in chatline.c, because
 the are somewhat related and append_output_window is already here.  --dwp
**************************************************************************/
void log_output_window(void)
{
  String theoutput;

  XtVaGetValues(outputwindow_text, XtNstring, &theoutput, NULL);
  write_chatline_content(theoutput);
}

/**************************************************************************
...
**************************************************************************/
void clear_output_window(void)
{
  XtVaSetValues(outputwindow_text, XtNstring, _("Cleared output window."), NULL);
}
