#!/bin/bash
# Shutdown script for Freeciv-web

SCRIPT_DIR="$(dirname "$0")"
cd "$(dirname "$0")"
export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."

if [ ! -f ${SCRIPT_DIR}/configuration.sh ]; then
    echo "ERROR: configuration.sh not found. copy configuration.sh.dist to configuration.sh and update it with your settings."
fi
. configuration.sh

echo "Shutting down Freeciv-web: nginx, tomcat, publite2, freeciv-proxy, pbem."

# Shutdown Freeciv-web's dependency services according to the users
# configuration.
./dependency-services-stop.sh

#3. publite2
ps aux | grep -ie publite2 | awk '{print $2}' | xargs kill -9 
killall -9 freeciv-web


#4. freeciv-proxy

ps aux | grep -ie freeciv-proxy | awk '{print $2}' | xargs kill -9 

#5.1 Freeciv-PBEM
ps aux | grep -ie pbem | awk '{print $2}' | xargs kill -9 

#5.2 meta-stats
ps aux | grep -ie meta-stats | awk '{print $2}' | xargs kill -9 

#5.3 Freeciv-Earth
ps aux | grep -ie freeciv-earth | awk '{print $2}' | xargs kill -9 

# Clean up server list in metaserver database.
echo "delete from servers" | mysql -u "${DB_USER}" -p"${DB_PASSWORD}" "${DB_NAME}"
