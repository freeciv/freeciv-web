Configuration of One Turn per Day (LongTurn) games in Freeciv-web

LongTurn games require some manual configuration to work. 

File: publite2\settings.ini
Config: Add LongTurn game server ports to the 'longturn_server_ports' variable.

File: freeciv-web\src\main\webapp\javascript\civclient.js 
Add LongTurn game server ports to the 'longturn_server_port_list' variable.
(Changing this requires recompiling Freeciv-web using freeciv-web/build-js.sh)

LongTurn game specific server settings:
publite2\pubscript_longturn_6003.serv
publite2\pubscript_longturn_6004.serv


freeciv\patches\longturn.patch is a patch which adds LongTurn support
to the Freeciv C server, and it is automatically applied to the source code.
