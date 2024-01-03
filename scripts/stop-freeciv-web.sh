#!/usr/bin/env bash

# Shutdown script for Freeciv-web

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd ${SCRIPT_DIR}

export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."

if [ ! -f ${SCRIPT_DIR}/configuration.sh ]; then
    echo "ERROR: configuration.sh not found. Copy configuration.sh.dist to configuration.sh and update it with your settings."
    exit 2
fi
. ./configuration.sh

echo "Shutting down Freeciv-web: nginx, tomcat, publite2, freeciv-proxy, pbem."

if [ "${TOMCATMANAGER}" = "Y" ]; then
    if [ -z "${TOMCATMANAGER_PASSWORD}" ]; then
        echo "Please enter tomcat-manager password for ${TOMCATMANAGER_USER}"
        read TOMCATMANAGER_PASSWORD
    fi
    curl -LsSg -K - << EOF
url="http://${TOMCATMANAGER_USER}:${TOMCATMANAGER_PASSWORD}@localhost:8080/manager/text/stop?path=/freeciv-web"
EOF
fi

if [ ! "${NGINX_DISABLE_ON_SHUTDOW}" = "N" ]; then
    sudo rm -f /etc/nginx/sites-enabled/freeciv-web
fi

# Shutdown Freeciv-web's dependency services according to the users
# configuration.
. ./dependency-services-stop.sh

#3. publite2
ps aux | grep -ie publite2 | awk '{print $2}' | xargs kill -9
killall -9 freeciv-web


#4. freeciv-proxy
ps aux | grep -ie freeciv-proxy | awk '{print $2}' | xargs kill -9

#5.1 Freeciv-PBEM
ps aux | grep -ie pbem | awk '{print $2}' | xargs kill -9

# Clean up server list in metaserver database.
echo "delete from servers" | mysql -u "${DB_USER}" -p"${DB_PASSWORD}" "${DB_NAME}"
