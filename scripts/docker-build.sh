#!/bin/bash

# Freeciv-web Docker Bootstrap Script - play.freeciv.org 
# 2015-09-12 - Andreas Rosdal
#
basedir="/docker"
logfile="/docker/freeciv-web-docker.log"

export LC_ALL="en_US.UTF-8"

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
mysql_pass="vagrant"

resin_version="4.0.44"
resin_url="http://www.caucho.com/download/resin-${resin_version}.tar.gz"
tornado_url="https://pypi.python.org/packages/source/t/tornado/tornado-4.2.1.tar.gz"
slimerjs_url="https://download.slimerjs.org/nightlies/0.10.0pre/slimerjs-0.10.0pre.zip"  
casperjs_url="https://github.com/n1k0/casperjs/archive/1.1-beta3.zip"


## Setup
mkdir -p ${basedir}
chmod -R 777 ${basedir}
cd ${basedir}

## dependencies
echo "==== Installing Updates and Dependencies ===="
export DEBIAN_FRONTEND=noninteractive

echo "==== Installing Handlebars ===="
curl -sL https://deb.nodesource.com/setup_8.x | bash -
apt-get -y install nodejs
npm install handlebars -g

echo "==== Fetching/Installing Tornado Web Server ===="
cd /tmp
wget ${tornado_url}
tar xvfz tornado-4.2.1.tar.gz
cd tornado-4.2.1
python3 setup.py install

pip3 install wikipedia

## mysql setup
echo "==== Setting up MySQL ===="
service mysql start || cat /var/log/mysql/*.*

# configuration files
dos2unix ${basedir}/scripts/configuration.sh.dist
sed -e "s/MYSQL_USER=root/MYSQL_USER=${mysql_user}/" -e "s/MYSQL_PASSWORD=vagrant/MYSQL_PASSWORD=${mysql_pass}/" ${basedir}/scripts/configuration.sh.dist > ${basedir}/scripts/configuration.sh
cp ${basedir}/publite2/settings.ini.dist ${basedir}/publite2/settings.ini

echo "==== Building freeciv ===="
dos2unix ${basedir}/freeciv/freeciv-web.project
cd ${basedir}/freeciv && ./prepare_freeciv.sh
cd freeciv && make install

echo "==== Building freeciv-web ===="
cd /usr/local/tomcat && sudo chmod -R 777 webapps logs && setfacl -d -m g::rwx webapps && sudo chown -R www-data:www-data webapps/
cd ${basedir}/scripts/freeciv-img-extract/ && ./setup_links.sh && ./sync.sh
cd ${basedir}/scripts && ./sync-js-hand.sh
cd ${basedir}/freeciv-web && sudo ./setup.sh

echo "=============================="

export PATH=$PATH:/docker/scripts

if [ -d "/docker/" ]; then
  echo "Starting Freeciv-web..."
  sudo service nginx start
  mkdir -p ${basedir}/logs
  chmod 777 ${basedir}/logs
  #cd ${basedir}/scripts/ && sudo -u freeciv ./start-freeciv-web.sh
else
  echo "Freeciv-web installed. Please start it manually."
fi
