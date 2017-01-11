#!/bin/bash

# Freeciv-web Travis CI Bootstrap Script - play.freeciv.org 
#
# https://travis-ci.org/freeciv/freeciv-web
#
# script is run to install Freeciv-web on Travis CI continuous integration.
# Travis-CI currenctly uses Ubuntu 14.04 as the base Ubuntu version.
#
echo "Installing Freeciv-web on Travis CI."
basedir=$(pwd)
logfile="${basedir}/freeciv-web-travis.log"


# Redirect copy of output to a log file.
exec > >(tee ${logfile})
exec 2>&1
set -e

echo "================================="
echo "Running Freeciv-web setup script."
echo "================================="

uname -a
echo basedir  $basedir
echo logfile $logfile

# User will need permissions to create a database
mysql_user="root"

tornado_url="https://github.com/tornadoweb/tornado/archive/v4.4.1.tar.gz"
nginx_url="http://nginx.org/download/nginx-1.9.11.tar.gz"
casperjs_url="https://github.com/casperjs/casperjs/zipball/1.1.3"
tomcat_url="https://bitbucket.org/andreasrosdal/fcweb/downloads/apache-tomcat-8.0.33.tar.gz"

# Based on fresh install of Ubuntu 14.04
dependencies="mysql-server-5.6 mysql-client-core-5.6 mysql-client-5.6 maven openjdk-7-jdk libcurl4-openssl-dev subversion pngcrush libtool automake autoconf autotools-dev language-pack-en python3-setuptools python3.4 python3.4-dev imagemagick liblzma-dev xvfb libicu-dev libsdl1.2-dev libjansson-dev php5-common php5-cli php5-fpm php5-mysql dos2unix zip"

## dependencies
echo "==== Installing Updates and Dependencies ===="
echo "apt-get update"
apt-get -y update
echo "apt-get install dependencies"
apt-get -y install ${dependencies}

python3 -m easy_install Pillow

echo "===== Install Tomcat 8 ======="
echo "if you get a download error 404 here, then there could be a new Tomcat version released, so update the URL below."
cd /var/lib
wget --quiet ${tomcat_url} || (>&2 echo "Unable to download Tomcat. Check download url in travis-build.sh." && exit 1)
tar -xvzf apache-tomcat-8.0.33.tar.gz
mv apache-tomcat-8.0.33 tomcat8
echo "export CATALINA_HOME=\"/var/lib/tomcat8\"" >> ~/.bashrc
cd tomcat8/bin
./startup.sh


echo "==== Fetching/Installing Tornado Web Server ===="
wget ${tornado_url}
tar xfz v4.4.1.tar.gz
cd tornado-4.4.1
python3 setup.py install

## build and install mysql-connector-python
cd /tmp
wget --quiet https://github.com/mysql/mysql-connector-python/archive/2.1.3.zip
unzip 2.1.3.zip
cd mysql-connector-python-2.1.3
python3 setup.py install

## mysql setup
echo "==== Setting up MySQL ===="
mysqladmin -u ${mysql_user} --password=""  create freeciv_web
mysql -u ${mysql_user} --password=""  freeciv_web < ${basedir}/freeciv-web/src/main/webapp/meta/private/metaserver.sql

sed -e "s/10/2/" ${basedir}/publite2/settings.ini.dist > ${basedir}/publite2/settings.ini
dos2unix ${basedir}/scripts/configuration.sh.dist
sed -e "s/MYSQL_USER=root/MYSQL_USER=${mysql_user}/" -e "s/MYSQL_PASSWORD=changeme/MYSQL_PASSWORD=/" ${basedir}/scripts/configuration.sh.dist > ${basedir}/scripts/configuration.sh

echo "==== Checking out Freeciv from SVN and patching... ===="
cd ${basedir}/freeciv && sudo -u travis ./prepare_freeciv.sh
echo "==== Building freeciv ===="
cd freeciv && sudo -u travis make install

echo "==== Building freeciv-web ===="
cd /var/lib/tomcat8 && sudo chmod -R 777 webapps logs && setfacl -d -m g::rwx webapps && sudo chown -R www-data:www-data webapps/
sed -e "s/vagrant//" ${basedir}/freeciv-web/src/main/webapp/meta/php_code/local.php.dist > ${basedir}/freeciv-web/src/main/webapp/meta/php_code/local.php
sed -e "s/vagrant//" ${basedir}/freeciv-web/src/main/webapp/META-INF/context.xml.dist > ${basedir}/freeciv-web/src/main/webapp/META-INF/context.xml
cp ${basedir}/freeciv-web/src/main/webapp/WEB-INF/config.properties.dist ${basedir}/freeciv-web/src/main/webapp/WEB-INF/config.properties
sed -i.bak -e "s/vagrant//" ${basedir}/freeciv-web/flyway.properties.dist
cd ${basedir}/scripts/freeciv-img-extract/ && ./setup_links.sh && ./sync.sh
cd ${basedir}/scripts && ./sync-js-hand.sh
cd ${basedir}/freeciv-web && sudo -u travis ./build.sh
cp flyway.properties.dist flyway.properties
sudo -u travis mvn compile flyway:migrate

echo "==== Building nginx ===="
cd ${basedir}
wget --quiet ${nginx_url}
tar xzf nginx-1.9.11.tar.gz
cd nginx-1.9.11
./configure
make > nginx-log-file 2>&1
make install


sed -i.bak -e "s/php7/php5/" ${basedir}/publite2/nginx.conf 
cp ${basedir}/publite2/nginx.conf /usr/local/nginx/conf/
sed -e "s/vagrant//" ${basedir}/pbem/settings.ini.dist > ${basedir}/pbem/settings.ini

echo "Starting Freeciv-web..."
/usr/local/nginx/sbin/nginx
cd ${basedir}/scripts/ && sudo -u travis ./start-freeciv-web.sh

cat ${basedir}/logs/*.log 

echo "============================================"
echo "Installing CasperJS for testing"
cd ${basedir}/tests

wget --quiet ${casperjs_url}
unzip -qo 1.1.3
cd casperjs-casperjs-cd78443
ln -sf `pwd`/bin/casperjs /usr/local/bin/casperjs

sleep 10

echo "Start testing of Freeciv-web using CasperJS:"
cd ${basedir}/tests/
xvfb-run casperjs --engine=phantomjs test freeciv-web-tests.js || (>&2 echo "Freeciv-web CasperJS tests failed!" && exit 1)

#echo "Running Freeciv-web server in autogame mode."
#cp ${basedir}/publite2/pubscript_autogame.serv ${basedir}/publite2/pubscript_singleplayer.serv
#killall freeciv-web
#sleep 20
#xvfb-run casperjs --engine=phantomjs test freeciv-web-autogame.js || (>&2 echo "Freeciv-web CasperJS autogame tests failed!" && exit 1)

echo "running pbem unit tests."
cd ${basedir}/pbem
python3 test_pbem.py

#echo "=============================="
echo "Freeciv-web built, tested and started correctly: Build successful!"
