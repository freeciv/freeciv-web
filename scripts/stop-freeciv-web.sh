#!/bin/bash
# Shutdown script for Freeciv-web

SCRIPT_DIR="$(dirname "$0")"
export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."

echo "Shutting down Freeciv-web: nginx, resin, publite2, freeciv-proxy."

# 1. nginx
if [ "$(pidof nginx)" ] ; then
  sudo killall nginx
fi

# 2. Resin
${FREECIV_WEB_DIR}/resin/bin/resin.sh stop 

#3. publite2
ps aux | grep -ie publite2 | awk '{print $2}' | xargs kill -9 
killall -9 freeciv-web


#4. freeciv-proxy

ps aux | grep -ie freeciv-proxy | awk '{print $2}' | xargs kill -9 

# Clean up server list in metaserver database.
echo "delete from servers" | mysql -u root -pvagrant freeciv_web
