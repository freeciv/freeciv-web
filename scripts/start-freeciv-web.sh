#!/bin/sh
# Startup script for running all processes of Freeciv-web

SCRIPT_DIR="$(dirname "$0")"
export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8

echo "Starting up Freeciv-web: nginx, tomcat, publite2, freeciv-proxy."

mkdir -p ${FREECIV_WEB_DIR}/logs

# 1. nginx
echo "Starting nginx first."

if [ "$(pidof nginx)" ] 
then
  echo "nginx already running!"
else
  echo "Please enter root password:"
  sudo service nginx start && \
  echo "nginx started!" && \
  sleep 1 
fi

#1.1 PHP-FPM
echo "starting php5-fpm"
sudo service php5-fpm start

# 2. Tomcat
echo "Starting up Tomcat" && \
sudo service tomcat8 start
# 2. Resin
#echo "Starting up Resin" && \
#${FREECIV_WEB_DIR}/resin/bin/resin.sh start && \
#echo "Resin starting.." && \

# waiting for Tomcat to start, since it will take some time.
until `curl --output /dev/null --silent --head --fail "http://localhost:8080/"`; do
    printf ".."
    sleep 3
done
sleep 8

#3. publite2
echo "Starting publite2" && \
(cd ${FREECIV_WEB_DIR}/publite2/ && \
sh run.sh) && \
echo "Publite2 started" && \
echo "Starting Freeciv-PBEM" && \
cd ${FREECIV_WEB_DIR}/pbem/ && nohup python3.5 -u freeciv-pbem.py > ../logs/freeciv-pbem.log 2>&1 || echo "unable to start freeciv-pbem" & 

echo "starting meta-stats.py" && \
cd ${FREECIV_WEB_DIR}/scripts/meta-stats && nohup python3.5 -u meta-stats.py > ../../logs/meta-stats.log 2>&1 || echo "unable to start meta-stats" & 

echo "Starting Freeciv-Earth-mapgen." && \
cd ${FREECIV_WEB_DIR}/freeciv-earth/ && nohup python3.5 -u freeciv-earth-mapgen.py > ../logs/freeciv-earth.log 2>&1 || echo "unable to start freeciv-earth-mapgen" & 

echo "Will sleep for 8 seconds, then do a status test..." && \
sleep 8 && \
sh ${FREECIV_WEB_DIR}/scripts/status-freeciv-web.sh
