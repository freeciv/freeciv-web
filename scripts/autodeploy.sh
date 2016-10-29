#!/bin/bash
# Automatic deploy of Freeciv-web.
#
# Checks if Travis CI build is sucessful, then pulls from git,
# and builds, installs and restarts Freeciv-web.

#Requires: https://github.com/travis-ci/travis.rb

set -e

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_USER="freeciv"
cd "$(dirname "$0")"
export FREECIV_WEB_DIR="${SCRIPT_DIR}/.."
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/home/freeciv/freeciv-web/freeciv-web/scripts:/usr/local/apache-maven-3.2.3/bin
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8
export CATALINA_HOME=/var/lib/tomcat8

. configuration.sh

rm -rf /var/lib/tomcat8/webapps/data/autodeploy.log 
exec >> /var/lib/tomcat8/webapps/data/autodeploy.log 
exec 2>&1

echo "Auto-deploy of Freeciv-web from master branch."
date

if travis status -qpx ; then
  echo "Travis CI build passed!"
else
  echo "Travis CI build failed! (or build is in progress)";
  exit 1;
fi 

cd ${FREECIV_WEB_DIR} && \
git pull origin master | grep -q "up-to-date" && \
echo "Freeciv-web is already updated, nothing to build." && exit 1

echo "Freeciv-web updated. Start to rebuild." && \
echo "Building Freeciv..." && \
cd freeciv && \
./prepare_freeciv.sh && cd freeciv && make install && \
cd .. && chmod -R 777 freeciv && \
echo "Freeciv installed!" && \

echo "Running sync scripts." && \
cd ../scripts/ && ./sync-js-hand.sh && \
cd freeciv-img-extract && ./sync.sh && \

echo "Building Freeciv-web." && \
cd ../../freeciv-web && sh build.sh && \
mvn compile flyway:migrate && \

set +e
echo "Restarting Freeciv C servers." && \
echo "Killing freeciv-web processes." && ps aux | grep -ie "[f]reeciv/bin/freeciv-web" | awk '{print $2}' | xargs kill -9 || echo "unable to stop process" 
echo "Killing init-freeciv-web.sh processes." && ps aux | grep -ie "[i]nit-freeciv-web.sh" | awk '{print $2}' | xargs kill -9 || echo "unable to stop process" 
echo "Killing freeciv-proxy processes." && ps aux | grep -ie "[f]reeciv-proxy" | awk '{print $2}' | xargs kill -9  || echo "unable to stop process" 
echo "Clearing games in DB." && echo "delete from servers" | mysql -u ${MYSQL_USER} -p${MYSQL_PASSWORD} freeciv_web
echo "Killing publite2 processes." && ps aux | grep -ie "[p]ublite2" | awk '{print $2}' | xargs kill -9 
echo "Starting publite2" && cd ../publite2/ && ./run.sh && \


echo "Autodeploy of Freeciv-web is complete."

