#!/bin/bash
# builds Freeciv-web and copies the war file to Tomcat.

TOMCATDIR="/var/lib/tomcat8"
DATADIR="/var/lib/tomcat8/webapps/data/"
ROOTDIR="$(pwd)/.."

echo "compressing 3d-models into zip file"
cd src/main/webapp/3d-models
zip -9 models.zip *.dae
cp models.zip /var/lib/tomcat8/webapps/ROOT/3d-models/

