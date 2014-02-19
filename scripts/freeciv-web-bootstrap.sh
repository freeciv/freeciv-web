#!/bin/bash

# Freeciv-web Vagrant Bootstrap Script - play.freeciv.org 
# 2014-02-17 - Andreas Røsdal
#
# Setup script for Freeciv-web to be used on a Vagrant local developer image.
# This script assumes that the source code in git has been checked out from
# https://github.com/freeciv/freeciv-web to /vagrant 

# Redirect copy of output to a log file.
exec > >(tee /tmp/vagrant-logfile.txt)
exec 2>&1
set -e

echo "================================="
echo "Running Freeciv-web setup script."
echo "================================="


basedir="/vagrant"

# User will need permissions to create a database
mysql_user="root"
mysql_pass="vagrant"

resin_version="4.0.38"
resin_url="http://www.caucho.com/download/resin-${resin_version}.tar.gz"
tornado_url="https://pypi.python.org/packages/source/t/tornado/tornado-3.2.tar.gz"

# Based on fresh install of Ubuntu 12.04
dependencies="maven2 mysql-server-5.5 openjdk-7-jdk libcurl4-openssl-dev nginx libjansson-dev subversion pngcrush python-imaging libtool automake autoconf autotools-dev language-pack-en python3.3-dev python3-setuptools libglib2.0-dev"

## Setup
mkdir -p ${basedir}
cd ${basedir}

## dependencies
echo "==== Installing Updates and Dependencies ===="
echo "apt-get update"
apt-get -y update
echo "apt-get upgrade"
apt-get -y upgrade
echo "mysql setup..."
sudo debconf-set-selections <<< 'mysql-server-5.5 mysql-server/root_password password vagrant'
sudo debconf-set-selections <<< 'mysql-server-5.5 mysql-server/root_password_again password vagrant'
echo "apt-get install dependencies"
apt-get -y install ${dependencies}

## build/install resin
echo "==== Fetching/Installing Resin ${resin_version} ===="
wget ${resin_url}
tar xvfz resin-${resin_version}.tar.gz
rm -Rf resin
mv resin-${resin_version} resin

echo "==== Fetching/Installing Tornado Web Server ===="
wget ${tornado_url}
tar xvfz tornado-3.2.tar.gz
cd tornado-3.2
python3.3 setup.py install


## mysql setup
echo "==== Setting up MySQL ===="
mysqladmin -u ${mysql_user} -p${mysql_pass} create freeciv_web
mysql -u ${mysql_user} -p${mysql_pass} freeciv_web < ${basedir}/freeciv-web/src/main/webapp/meta/private/metaserver.sql

echo "==== Building freeciv ===="
cd ${basedir}/freeciv && ./prepare_freeciv.sh
cd freeciv && make install
cd ${basedir}/freeciv/data/ && cp -rf fcweb /usr/local/share/freeciv

echo "==== Building freeciv-web ===="
sed -e "s/user>root/user>${mysql_user}/" -e "s/password>changeme/password>${mysql_pass}/" ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml.dist > ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml
cd ${basedir}/freeciv-img-extract/ && ./setup_links.sh && ./sync.sh
cd ${basedir}/scripts && ./sync-js-hand.sh
cd ${basedir}/freeciv-web && ./build.sh

echo "=============================="

rm /etc/nginx/sites-enabled/default
cp ${basedir}/publite2/nginx.conf /etc/nginx/
cp ${basedir}/publite2/nginx/freeciv-web /etc/nginx/sites-enabled/

echo "Starting Freeciv-web..."
service nginx start
cd ${basedir}/scripts/ && sudo -u vagrant ./start-freeciv-web.sh
echo "Freeciv-web started! Now try http://localhost/ on your host operating system."
