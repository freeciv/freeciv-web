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

#include <windows.h>
#include <windowsx.h>

/* common & utility */
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "shared.h"
#include "support.h"
#include "version.h"

/* client */
#include "chatline.h"
#include "client_main.h"
#include "climisc.h"
#include "colors.h"
#include "connectdlg.h"
#include "control.h"
#include "dialogs.h"
#include "gotodlg.h"
#include "graphics.h"
#include "gui_stuff.h"
#include "helpdata.h"           /* boot_help_texts() */
#include "mapctrl.h"
#include "mapview.h"
#include "menu.h"
#include "optiondlg.h"
#include "options.h"
#include "spaceshipdlg.h"
#include "tilespec.h"

#include <stdio.h>

#include "gui_main.h"

/**************************************************************************
 Handles WM_COMMAND messages from the chatline
 To find out when the return key is pressed, it checks for newlines
 in the string
 A backup of the string (without the newline) is sent to the server
 Is there a nicer way to handle this?
**************************************************************************/
void handle_chatline(void)
{
  static char msg_buf[MAX_LEN_MSG-MAX_LEN_USERNAME+1];
  char msg_buf2[MAX_LEN_MSG-MAX_LEN_USERNAME+1];

  GetWindowText(hchatline,msg_buf2,sizeof(msg_buf2));
  if (strchr(msg_buf2,'\n')) {
    send_chat(msg_buf);
    SetWindowText(hchatline,"");
  } else {
    sz_strlcpy(msg_buf, msg_buf2);
  }
}

/**************************************************************************

**************************************************************************/
static void append_output_window_real(const char *astring)
{
  int len;
  len=Edit_GetTextLength(logoutput_win);
  Edit_SetSel(logoutput_win,len,len);
  Edit_ReplaceSel(logoutput_win,astring);
  Edit_ScrollCaret(logoutput_win);
}

/**************************************************************************
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
**************************************************************************/
void real_output_window_append(const char *astring,
                               const struct text_tag_list *tags,
                               int conn_id)
{
  const char *str;
  char *str2;
  char buf[512];

  str=astring;
  while((str2=strchr(str,'\n')))
    {
      /* HACK: We need to add \r to lineends. */
      strncpy(buf,str,str2-str);
      buf[str2-str]=0;
      strcat(buf,"\r\n");
      append_output_window_real(buf);
      str=str2+1;
    }
  append_output_window_real(str);
  append_output_window_real("\r\n");
}

/**************************************************************************
...
**************************************************************************/
void
log_output_window(void)
{
  int len;
  char *theoutput;
 
  len=GetWindowTextLength(logoutput_win)+1;
  theoutput=fc_malloc(len);
  GetWindowText(logoutput_win,theoutput,len);
  write_chatline_content(theoutput);
  free(theoutput);
}

/**************************************************************************
...
**************************************************************************/
void
clear_output_window(void)
{
  SetWindowText(logoutput_win,"");
  output_window_append(FTC_CLIENT_INFO, NULL, _("Cleared output window."));
}
