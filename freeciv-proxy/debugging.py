# -*- coding: utf-8 -*-

'''
 Freeciv - Copyright (C) 2009-2014 - Andreas Røsdal   andrearo@pvv.ntnu.no
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
from time import gmtime, strftime
import os
import platform
import time
from tornado import version as tornado_version
import gc


startTime = time.time()


def get_debug_info(civcoms):
    code = "<html><head><meta http-equiv=\"refresh\" content=\"20\">" \
       + "<link href='/css/bootstrap.min.css' rel='stylesheet'></head>" \
       + "<body><div class='container'>" \
       + "<h2>Freeciv WebSocket Proxy Status</h2>" \
       + "<font color=\"green\">Process status: OK</font><br>"

    code += "<b>Process Uptime: " + \
        str(int(time.time() - startTime)) + " s.</b><br>"

    code += ("Python version: %s %s (%s)<br>" % (
        platform.python_implementation(),
        platform.python_version(),
        platform.python_build()[0],
    ))

    cpu = ' '.join(platform.processor().split())
    code += ("Platform: %s %s on '%s' <br>" % (
        platform.machine(),
        platform.system(),
        cpu))

    code += ("Tornado version %s <br>" % (tornado_version))

    try:
        f = open("/proc/loadavg")
        contents = f.read()
        code += "Load average: " + contents
        f.close()
    except:
        print("Cannot open uptime file: /proc/uptime")

    try:
        code += ("<h3>Logged in users  (count %i) :</h3>" % len(civcoms))
        for key in list(civcoms.keys()):
            code += (
                "username: <b>%s</b> <br>IP:%s <br>Civserver: %d<br>Connect time: %d<br><br>" %
                (civcoms[key].username,
                 civcoms[key].civwebserver.ip,
                 civcoms[key].civserverport,
                 time.time() - civcoms[key].connect_time))
    except:
        print(("Unexpected error:" + str(sys.exc_info()[0])))
        raise

    code += "</div></body></html>"

    return code


