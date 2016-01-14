Freeciv-web Publite2   
====================

Publite2 is a process manager which launches multiple Freeciv-web servers
depending on demand reported by the Metaserver. It requires the Freeciv-web
webapplication to be already running on Resin to work. 

Publite2 also launches one Freeciv-proxy server instance for each 
Freeciv C server.

Publite2 is started automatically by the start-freeciv-web.sh script.

Requires Python 3.5 and the Freeciv C server built for Freeciv-web
This process logs to logs/publite2.log file.

Publite2 has a HTTP status page which can be accessed at:
http://localhost/pubstatus through nginx or 
http://localhost:4002/pubstatus directly.

