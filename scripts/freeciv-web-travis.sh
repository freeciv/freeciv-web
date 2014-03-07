#!/bin/bash

# Freeciv-web Travis CI Bootstrap Script - play.freeciv.org 
#
# https://travis-ci.org/freeciv/freeciv-web
#
# script is run to install Freeciv-web on Travis CI continuous integration.
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
mysql_pass="vagrant"

resin_version="4.0.38"
resin_url="http://www.caucho.com/download/resin-${resin_version}.tar.gz"
tornado_url="https://pypi.python.org/packages/source/t/tornado/tornado-3.2.tar.gz"

# Based on fresh install of Ubuntu 12.04
dependencies="maven mysql-server-5.5 openjdk-7-jdk libcurl4-openssl-dev nginx libjansson-dev subversion pngcrush libtool automake autoconf autotools-dev language-pack-en python3-setuptools libglib2.0-dev python3.2 python3.2-dev"

## dependencies
echo "==== Installing Updates and Dependencies ===="
echo "apt-get update"
apt-get -y update
echo "mysql setup..."
sudo debconf-set-selections <<< "mysql-server-5.5 mysql-server/root_password password ${mysql_pass}"
sudo debconf-set-selections <<< "mysql-server-5.5 mysql-server/root_password_again password ${mysql_pass}"
echo "apt-get install dependencies"
apt-get -y install ${dependencies}

#Travis doesn't support Python 3.3 at the moment.
ln -s /usr/bin/python3.2 /usr/bin/python3.3
python3.3 --version

python3.2 -m easy_install Pillow

java -version
javac -version

## build/install resin
echo "==== Fetching/Installing Resin ${resin_version} ===="
wget ${resin_url}
tar xvfz resin-${resin_version}.tar.gz
rm -Rf resin
mv resin-${resin_version} resin
chmod -R 777 resin


echo "==== Fetching/Installing Tornado Web Server ===="
wget ${tornado_url}
tar xvfz tornado-3.2.tar.gz
cd tornado-3.2
python3.3 setup.py install


## mysql setup
echo "==== Setting up MySQL ===="
mysqladmin -u ${mysql_user} -p${mysql_pass} create freeciv_web
mysql -u ${mysql_user} -p${mysql_pass} freeciv_web < ${basedir}/freeciv-web/src/main/webapp/meta/private/metaserver.sql

echo "==== Checking out Freeciv from SVN and patching... ===="
cd ${basedir}/freeciv && ./prepare_freeciv.sh
echo "==== Building freeciv ===="
cd freeciv && make install
cd ${basedir}/freeciv/data/ && cp -rf fcweb /usr/local/share/freeciv

echo "==== Building freeciv-web ===="
sed -e "s/user>root/user>${mysql_user}/" -e "s/password>changeme/password>${mysql_pass}/" ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml.dist > ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml
cd ${basedir}/freeciv-img-extract/ && ./setup_links.sh && ./sync.sh
cd ${basedir}/scripts && ./sync-js-hand.sh
cd ${basedir}/freeciv && rm -rf freeciv
cd ${basedir}/freeciv-web && ./build.sh

df -h
top -n 1

echo "Starting Freeciv-web..."
service nginx start
cd ${basedir}/scripts/ && sudo -u travis ./start-freeciv-web.sh

echo "=============================="
echo "Freeciv-web built and started correctly: Build successful!"
