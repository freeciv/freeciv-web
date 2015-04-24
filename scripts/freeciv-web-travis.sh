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

resin_version="4.0.40"
resin_url="http://www.caucho.com/download/resin-${resin_version}.tar.gz"
tornado_url="https://pypi.python.org/packages/source/t/tornado/tornado-4.1.tar.gz"
jansson_url="http://www.digip.org/jansson/releases/jansson-2.7.tar.bz2"
slimerjs_url="https://github.com/laurentj/slimerjs/archive/master.zip"
casperjs_url="https://github.com/n1k0/casperjs/archive/1.1-beta3.zip"


# Based on fresh install of Ubuntu 12.04
dependencies="maven mysql-server-5.5 openjdk-7-jdk libcurl4-openssl-dev nginx subversion pngcrush libtool automake autoconf autotools-dev language-pack-en python3-setuptools libglib2.0-dev python3.2 python3.2-dev imagemagick liblzma-dev firefox xvfb"

## dependencies
echo "==== Installing Updates and Dependencies ===="
echo "apt-get update"
apt-get -y update
echo "mysql setup..."
sudo debconf-set-selections <<< "mysql-server-5.5 mysql-server/root_password password ${mysql_pass}"
sudo debconf-set-selections <<< "mysql-server-5.5 mysql-server/root_password_again password ${mysql_pass}"
echo "apt-get install dependencies"
apt-get -y install ${dependencies}

#Travis doesn't support Python 3.4 at the moment.
ln -s /usr/bin/python3.2 /usr/bin/python3.4
python3.4 --version

python3.2 -m easy_install Pillow

java -version
javac -version

## build/install resin
echo "==== Fetching/Installing Resin ${resin_version} ===="
wget ${resin_url}
tar xvfz resin-${resin_version}.tar.gz
rm -Rf resin
mv resin-${resin_version} resin
cd resin
./configure --prefix=`pwd`; make; make install
cd ..
chmod -R 777 resin


echo "==== Fetching/Installing Tornado Web Server ===="
wget ${tornado_url}
tar xvfz tornado-4.1.tar.gz
cd tornado-4.1
python3.4 setup.py install

echo "==== Fetching/Installing Jansson ===="
wget ${jansson_url}
tar xvjf jansson-2.7.tar.bz2
cd jansson-2.7
./configure
make && make install


## mysql setup
echo "==== Setting up MySQL ===="
mysqladmin -u ${mysql_user} -p${mysql_pass} create freeciv_web
mysql -u ${mysql_user} -p${mysql_pass} freeciv_web < ${basedir}/freeciv-web/src/main/webapp/meta/private/metaserver.sql

sed -e "s/10/2/" ${basedir}/publite2/settings.ini.dist > ${basedir}/publite2/settings.ini

echo "==== Checking out Freeciv from SVN and patching... ===="
cd ${basedir}/freeciv && ./prepare_freeciv.sh
echo "==== Building freeciv ===="
cd freeciv && make install

echo "==== Building freeciv-web ===="
sed -e "s/user>root/user>${mysql_user}/" -e "s/password>changeme/password>${mysql_pass}/" ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml.dist > ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml
cd ${basedir}/scripts/freeciv-img-extract/ && ./setup_links.sh && ./sync.sh
cd ${basedir}/scripts && ./sync-js-hand.sh
cd ${basedir}/freeciv && rm -rf freeciv
cd ${basedir}/freeciv-web && ./build.sh

echo "============================================"
echo "Installing SlimerJS and CasperJS for testing"
export SLIMERJSLAUNCHER=/usr/bin/firefox
export SLIMERJS_EXECUTABLE=${basedir}/tests/slimerjs-master/src/slimerjs
cd ${basedir}/tests
wget ${slimerjs_url}
unzip master.zip
wget ${casperjs_url}
unzip 1.1-beta3.zip


echo "Starting Freeciv-web..."
service nginx start
cd ${basedir}/scripts/ && sudo -u travis ./start-freeciv-web.sh

sleep 20
cat ${basedir}/logs/*.log 

echo "Start testing of Freeciv-web using CasperJS:"
cd ${basedir}/tests/casperjs-1.1-beta3/bin
xvfb-run ./casperjs --engine=slimerjs test ${basedir}/tests/freeciv-web-tests.js || (>&2 echo "Freeciv-web CasperJS tests failed!" && exit 1)

echo "=============================="
echo "Freeciv-web built and started correctly: Build successful!"
