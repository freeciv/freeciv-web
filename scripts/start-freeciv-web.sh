#!/usr/bin/env bash

# Startup script for running all processes of Freeciv-web

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd ${SCRIPT_DIR}

export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."
export FREECIV_DATA_PATH="${HOME}/freeciv/share/freeciv/"
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8

if [ ! -f ${SCRIPT_DIR}/configuration.sh ]; then
    echo "ERROR: configuration.sh not found. Copy configuration.sh.dist to configuration.sh and update it with your settings."
    exit 2
fi
. ./configuration.sh

echo "Starting up Freeciv-web: nginx, tomcat, publite2, freeciv-proxy."

mkdir -p ${FREECIV_WEB_DIR}/logs
if [ ! -f /etc/nginx/sites-enabled/freeciv-web ]; then
    # Not enabled. Try to enable Freeciv-web.
    sudo ln -f /etc/nginx/sites-available/freeciv-web /etc/nginx/sites-enabled/freeciv-web
fi

# Start Freeciv-web's dependency services according to the users
# configuration.
./dependency-services-start.sh
if [ "${TOMCATMANAGER}" = "Y" ]; then
    if [ -z "${TOMCATMANAGER_PASSWORD}" ]; then
        echo "Please enter tomcat-manager password for ${TOMCATMANAGER_USER}"
        read TOMCATMANAGER_PASSWORD
    fi
    curl -LsSg -K - << EOF
url="http://${TOMCATMANAGER_USER}:${TOMCATMANAGER_PASSWORD}@localhost:8080/manager/text/start?path=/freeciv-web"
EOF
fi

#3. publite2
echo "Starting publite2" && \
(cd ${FREECIV_WEB_DIR}/publite2/ && \
sh run.sh) && \
echo "Publite2 started" && \
echo "Starting Freeciv-PBEM" && \
(cd ${FREECIV_WEB_DIR}/pbem/ && nohup python3 -u pbem.py > ../logs/pbem.log 2>&1) || echo "unable to start pbem" &

echo "Will sleep for 8 seconds, then do a status test..." && \
sleep 8 && \
bash ${FREECIV_WEB_DIR}/scripts/status-freeciv-web.sh
