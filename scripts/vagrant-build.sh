#!/bin/bash

# Freeciv-web Vagrant Bootstrap Script - play.freeciv.org 
# 2014-02-17 - Andreas Røsdal
#
# Setup script for Freeciv-web to be used on a Vagrant local developer image.
# This script assumes that the source code in git has been checked out from
# https://github.com/freeciv/freeciv-web to /vagrant 

if [ -d "/vagrant/" ]; then
  # script is run to install Freeciv-web under vagrant
  basedir="/vagrant"
  logfile="/vagrant/freeciv-web-vagrant.log"
else
  # script is run to install Freeciv-web on current system without vagrant
  echo "Installing Freeciv-web on current system. Please run this script as root user."
  basedir=$(pwd)"/.."
  logfile="${basedir}/freeciv-web-vagrant.log"
fi


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
tornado_url="https://pypi.python.org/packages/source/t/tornado/tornado-4.3.tar.gz"
slimerjs_url="https://download.slimerjs.org/nightlies/0.10.0pre/slimerjs-0.10.0pre.zip"  
casperjs_url="https://github.com/n1k0/casperjs/archive/1.1-beta3.zip"

# Based on fresh install of Ubuntu 12.04
dependencies="maven mysql-server openjdk-7-jdk libcurl4-openssl-dev nginx libjansson-dev subversion pngcrush python3-pillow libtool automake autoconf autotools-dev language-pack-en python3.5-dev python3-setuptools libbz2-dev imagemagick python3-pip dos2unix liblzma-dev firefox xvfb libicu-dev pkg-config zlib1g-dev libsdl1.2-dev"

## Setup
mkdir -p ${basedir}
cd ${basedir}

## dependencies
echo "==== Installing Updates and Dependencies ===="
export DEBIAN_FRONTEND=noninteractive
apt-mark hold firefox
echo "apt-get update"
apt-get -y update
echo "apt-get upgrade"
apt-get -y upgrade
echo "mysql setup..."
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password password ${mysql_pass}"
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password_again password ${mysql_pass}"
echo "apt-get install dependencies"
apt-get -y install ${dependencies}

## build/install resin
echo "==== Fetching/Installing Resin ${resin_version} ===="
wget ${resin_url}
tar xvfz resin-${resin_version}.tar.gz
rm -Rf resin
mv resin-${resin_version} resin

echo "==== Fetching/Installing Tornado Web Server ===="
cd /tmp
wget ${tornado_url}
tar xvfz tornado-4.3.tar.gz
cd tornado-4.3
python3.5 setup.py install

pip3 install wikipedia

## mysql setup
echo "==== Setting up MySQL ===="
mysqladmin -u ${mysql_user} -p${mysql_pass} create freeciv_web
mysql -u ${mysql_user} -p${mysql_pass} freeciv_web < ${basedir}/freeciv-web/src/main/webapp/meta/private/metaserver.sql

# configuration files
dos2unix ${basedir}/scripts/configuration.sh.dist
sed -e "s/MYSQL_USER=root/MYSQL_USER=${mysql_user}/" -e "s/MYSQL_PASSWORD=changeme/MYSQL_PASSWORD=${mysql_pass}/" ${basedir}/scripts/configuration.sh.dist > ${basedir}/scripts/configuration.sh
cp ${basedir}/publite2/settings.ini.dist ${basedir}/publite2/settings.ini

echo "==== Building freeciv ===="
dos2unix ${basedir}/freeciv/freeciv-web.project
cd ${basedir}/freeciv && ./prepare_freeciv.sh
cd freeciv && make install

echo "==== Building freeciv-web ===="
sed -e "s/user>root/user>${mysql_user}/" -e "s/password>changeme/password>${mysql_pass}/" ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml.dist > ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml
cd ${basedir}/scripts/freeciv-img-extract/ && ./setup_links.sh && ./sync.sh
cd ${basedir}/scripts && ./sync-js-hand.sh
cd ${basedir}/freeciv-web && sudo -u vagrant ./setup.sh

echo "=============================="

# FIXME: this command fails.
#pip3 install --allow-external mysql-connector-python mysql-connector-python
cp ${basedir}/pbem/settings.ini.dist ${basedir}/pbem/settings.ini

service nginx stop
rm /etc/nginx/sites-enabled/default
cp ${basedir}/publite2/nginx.conf /etc/nginx/

# add Freeciv-web scripts to path
export PATH=$PATH:/vagrant/scripts
echo 'export PATH=$PATH:/vagrant/scripts' >> /home/vagrant/.bashrc

if [ -d "/vagrant/" ]; then
  echo "Starting Freeciv-web..."
  service nginx start
  cd ${basedir}/scripts/ && sudo -u vagrant ./start-freeciv-web.sh
else
  echo "Freeciv-web installed. Please start it manually."
fi

echo "============================================"
echo "Installing SlimerJS and CasperJS for testing"
export SLIMERJSLAUNCHER=/usr/bin/firefox
export SLIMERJS_EXECUTABLE=${basedir}/tests/slimerjs-0.10.0pre/slimerjs
cd ${basedir}/tests
wget --no-check-certificate ${slimerjs_url}
unzip -qo slimerjs-0.10.0pre.zip

wget ${casperjs_url}
unzip -qo 1.1-beta3.zip
cd casperjs-1.1-beta3
ln -sf `pwd`/bin/casperjs /usr/local/bin/casperjs

echo "Start testing of Freeciv-web using CasperJS:"
cd ${basedir}/tests/
xvfb-run casperjs --engine=slimerjs test freeciv-web-tests.js || (>&2 echo "Freeciv-web CasperJS tests failed!" && exit 1)

echo "Freeciv-web started! Now try http://localhost/ on your host operating system."

