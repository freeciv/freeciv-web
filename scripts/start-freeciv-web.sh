#!/bin/sh
# Startup script for running all processes of Freeciv-web

SCRIPT_DIR="$(dirname "$0")"
cd "$(dirname "$0")"
export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8

if [ ! -f ${SCRIPT_DIR}/configuration.sh ]; then
    echo "ERROR: configuration.sh not found. copy configuration.sh.dist to configuration.sh and update it with your settings."
fi
. ${SCRIPT_DIR}/configuration.sh

if [ "x$DEPENDENCY_SERVICES_START" = x ] ; then
  DEPENDENCY_SERVICES_START="./dependency-services-default-start.sh"
fi

echo "Starting up Freeciv-web: nginx, tomcat, publite2, freeciv-proxy."

mkdir -p ${FREECIV_WEB_DIR}/logs

# Start Freeciv-web's dependency services according to the users
# configuration.
$DEPENDENCY_SERVICES_START

#3. publite2
echo "Starting publite2" && \
(cd ${FREECIV_WEB_DIR}/publite2/ && \
sh run.sh) && \
echo "Publite2 started" && \
echo "Starting Freeciv-PBEM" && \
cd ${FREECIV_WEB_DIR}/pbem/ && nohup python3 -u pbem.py > ../logs/pbem.log 2>&1 || echo "unable to start pbem" &

echo "starting meta-stats.py" && \
cd ${FREECIV_WEB_DIR}/scripts/meta-stats && nohup python3 -u meta-stats.py > ../../logs/meta-stats.log 2>&1 || echo "unable to start meta-stats" &

echo "Starting Freeciv-Earth-mapgen." && \
cd ${FREECIV_WEB_DIR}/freeciv-earth/ && nohup python3 -u freeciv-earth-mapgen.py > ../logs/freeciv-earth.log 2>&1 || echo "unable to start freeciv-earth-mapgen" &

echo "Will sleep for 8 seconds, then do a status test..." && \
sleep 8 && \
sh ${FREECIV_WEB_DIR}/scripts/status-freeciv-web.sh
