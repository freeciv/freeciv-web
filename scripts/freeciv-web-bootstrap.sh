#!/bin/bash

# FIXME: This is not in working condition
# It doesn't get freeciv sources and patch them automatically
# It doesn't create javascript packet definitions 
# It doesn't create javascript gfx definitions

basedir="${HOME}/freeciv-build"
srcdir="${basedir}/freeciv-web"
destdir="${basedir}/build"

# User will need permissions to create a database
mysql_user="root"
mysql_pass="changeme"

freeciv_repo="https://github.com/cazfi/freeciv-web.git"
resin_version="4.0.35"
resin_url="http://www.caucho.com/download/resin-${resin_version}.tar.gz"

# Based on fresh install of Ubuntu 10.10
dependencies="maven2 mysql-server openjdk-6-jdk libcurl4-openssl-dev nginx git-core libjansson-dev"


## Setup
mkdir -p ${basedir}
cd ${basedir}

## dependencies
echo "==== Installing Dependencies ===="
sudo aptitude build-dep freeciv-server
sudo aptitude install ${dependencies}

## freeciv source
echo "==== Fetching Source ===="
git clone ${freeciv_repo} ${basedir}/freeciv-web

## build/install resin
echo "==== Fetching/Installing Resin ${resin_version} ===="
wget ${resin_url}
tar xvfz resin-${resin_version}.tar.gz
cd resin-${resin_version}
./configure --prefix=${basedir}/resin --with-resin-init.d=${basedir}/resin/bin/initscript --with-resin-log=${basedir}/resin/log/ && make && make install

## mysql setup
echo "==== Setting up MySQL ===="
mysqladmin -u ${mysql_user} -p${mysql_pass} create freeciv_web
mysql -u ${mysql_user} -p${mysql_pass} freeciv_web < ${srcdir}/freeciv-web/src/main/webapp/meta/private/metaserver.sql

echo "==== Configuring Build ===="

sed -e "s/user>root/user>${mysql_user}/" -e "s/password>changeme/password>${mysql_pass}/" \
        ${srcdir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml.dist \
        > ${srcdir}/freeciv-web/src/main/webapp/WEB-INF/resin-web.xml

echo "==== Building freeciv-web ===="
cd ${srcdir}/freeciv-web/src/main/webapp/ && bash compress-js.sh
cd ${srcdir}/freeciv-web && mvn install
mv ${destdir}/resin/webapps/ROOT ${destdir}/resin/webapps/old-root 
cp target/freeciv-web.war ${destdir}/resin/webapps/

echo "==== Building freeciv ===="
cd ${basedir}/freeciv-web/freeciv
./autogen.sh --enable-fcweb
make && sudo make install

echo "=============================="
echo "Refer to the freeciv-web README for instructions on how to proceed after the build"
echo "=============================="
