#!/bin/sh
# Startup script for running all processes of Freeciv-web

SCRIPT_DIR="$(dirname "$0")"
export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8

echo "Starting up Freeciv-web: nginx, resin, publite2, freeciv-proxy."

# 1. nginx
echo "Starting nginx first. Please enter root password:"

if [ "$(pidof nginx)" ] 
then
  echo "nginx running!"
else
  sudo service nginx start && \
  echo "nginx started!" && \
  sleep 1 
fi


# 2. Resin
echo "Starting up Resin" && \
${FREECIV_WEB_DIR}/resin/bin/resin.sh start && \
echo "Resin starting.." && \

# waiting for Resin to start, since it will take some time.
until `curl --output /dev/null --silent --head --fail "http://localhost:8080/meta/metaserver.php"`; do
    printf ".."
    sleep 3
done
sleep 4

#3. publite2
echo "Starting publite2" && \
(cd ${FREECIV_WEB_DIR}/publite2/ && \
sh run.sh) && \
echo "Publite2 started" && \
echo "Will sleep for 5 seconds, then do a status test..." && \
sleep 5 && \
sh ./status-freeciv-web.sh
