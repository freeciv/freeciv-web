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

resin_version="4.0.44"
resin_url="http://www.caucho.com/download/resin-${resin_version}.tar.gz"
tornado_url="https://pypi.python.org/packages/source/t/tornado/tornado-4.2.1.tar.gz"
jansson_url="http://www.digip.org/jansson/releases/jansson-2.7.tar.bz2"
slimerjs_url="https://download.slimerjs.org/nightlies/0.10.0pre/slimerjs-0.10.0pre.zip"   
casperjs_url="https://github.com/n1k0/casperjs/archive/1.1-beta3.zip"
nginx_url="http://nginx.org/download/nginx-1.8.0.tar.gz"
icu_url="http://download.icu-project.org/files/icu4c/55.1/icu4c-55_1-src.tgz"

# Based on fresh install of Ubuntu 12.04
dependencies="maven mysql-server-5.5 openjdk-7-jdk libcurl4-openssl-dev subversion pngcrush libtool automake autoconf autotools-dev language-pack-en python3-setuptools python3.2 python3.2-dev imagemagick liblzma-dev firefox xvfb libicu-dev"

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
tar xfz resin-${resin_version}.tar.gz
rm -Rf resin
mv resin-${resin_version} resin
cd resin
./configure --prefix=`pwd`; make; make install
cd ..
chmod -R 777 resin


echo "==== Fetching/Installing Tornado Web Server ===="
wget ${tornado_url}
tar xfz tornado-4.2.1.tar.gz
cd tornado-4.2.1
python3.4 setup.py install

echo "==== Fetching/Installing Jansson ===="
wget ${jansson_url}
tar xjf jansson-2.7.tar.bz2
cd jansson-2.7
./configure
make > jansson-log-file 2>&1  && make install > jansson-log-file 2>&1

## mysql setup
echo "==== Setting up MySQL ===="
mysqladmin -u ${mysql_user} -p${mysql_pass} create freeciv_web
mysql -u ${mysql_user} -p${mysql_pass} freeciv_web < ${basedir}/freeciv-web/src/main/webapp/meta/private/metaserver.sql

sed -e "s/10/2/" ${basedir}/publite2/settings.ini.dist > ${basedir}/publite2/settings.ini

echo "==== Download and install ICU.  ===="
cd ${basedir}/
wget ${icu_url}
tar xzf icu4c-55_1-src.tgz 
cd icu/source/
./configure
make > icu-log-file 2>&1  && make install > icu-log-file 2>&1
ldconfig

echo "==== Checking out Freeciv from SVN and patching... ===="
cd ${basedir}/freeciv && ./prepare_freeciv.sh
echo "==== Building freeciv ===="
cd freeciv && make install

echo "==== Building freeciv-web ===="
sed -e "s/user>root/user>${mysql_user}/" -e "s/password>changeme/password>${mysql_pass}/" ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml.dist > ${basedir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml
cd ${basedir}/scripts/freeciv-img-extract/ && ./setup_links.sh && ./sync.sh
cd ${basedir}/scripts && ./sync-js-hand.sh
cd ${basedir}/freeciv-web && sudo -u travis ./setup.sh

echo "==== Building nginx ===="
cd ${basedir}
wget ${nginx_url}
tar xzf nginx-1.8.0.tar.gz
cd nginx-1.8.0
./configure
make > nginx-log-file 2>&1
make install

cp ${basedir}/publite2/nginx.conf /usr/local/nginx/conf/

echo "Starting Freeciv-web..."
/usr/local/nginx/sbin/nginx
cd ${basedir}/scripts/ && sudo -u travis ./start-freeciv-web.sh

cat ${basedir}/logs/*.log 

echo "============================================"
echo "Installing SlimerJS and CasperJS for testing"
export SLIMERJSLAUNCHER=/usr/bin/firefox
export SLIMERJS_EXECUTABLE=${basedir}/tests/slimerjs-0.10.0pre/slimerjs
cd ${basedir}/tests
wget --no-check-certificate ${slimerjs_url}
unzip -q slimerjs-0.10.0pre.zip
wget ${casperjs_url}
unzip -q 1.1-beta3.zip
cd casperjs-1.1-beta3
ln -sf `pwd`/bin/casperjs /usr/local/bin/casperjs

echo "Start testing of Freeciv-web using CasperJS:"
cd ${basedir}/tests/
xvfb-run casperjs --engine=slimerjs test freeciv-web-tests.js || (>&2 echo "Freeciv-web CasperJS tests failed!" && exit 1)

echo "=============================="
echo "Freeciv-web built, tested and started correctly: Build successful!"
