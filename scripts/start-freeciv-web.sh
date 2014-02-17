#!/bin/sh
# Startup script for running all processes of Freeciv-web

SCRIPT_DIR="$(dirname "$0")"
export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."

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
echo "Resin started" && \
sleep 20 && \

#3. publite2
echo "Starting publite2" && \
(cd ${FREECIV_WEB_DIR}/publite2/ && \
sh run.sh) && \
echo "Publite2 started" && \

#4. freeciv-proxy
echo "Starting freeciv-proxy" && \
(cd  ${FREECIV_WEB_DIR}/freeciv-proxy/ && \
sh run.sh) && \
echo "freeciv-proxy started" &&\
echo "Will sleep for 30 seconds, then do a status test..." && \
sleep 30 && \
sh ./status-freeciv-web.sh
