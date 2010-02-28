# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
'''


import sys
import traceback

 
def get_debug_info(civcoms):
    code = "<html><head><meta http-equiv=\"refresh\" content=\"20\"></head><body><h2>Freeciv Web Proxy Status</h2>"
    
    code += ("<h3>Logged in users  (%i) :</h3>" % len(civcoms));
    for key in civcoms.keys():
        code += ("username: <b>%s</b> (%s:%d)<br>" % (civcoms[key].username, civcoms[key].civserverhost, civcoms[key].civserverport));
        
    for threadId, stack in sys._current_frames().items():
        code += ("<br><br><h3># ThreadID: %s</h3>" % threadId)
        for filename, lineno, name, line in traceback.extract_stack(stack):
            code += ('File: "%s", line %d, in %s: ' % (filename, lineno, name))
            if line:
                code += (" <b>%s</b> <br>" % (line.strip()))
 
    return code;



