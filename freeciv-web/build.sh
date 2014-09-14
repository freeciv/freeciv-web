#!/bin/bash
# builds Freeciv-web and copies the war file to resin.

ROOTDIR="$(pwd)/.."
cp -R ${ROOTDIR}/freeciv/freeciv/data/flags/*.svg ${ROOTDIR}/freeciv-web/src/main/webapp/images/flags/
( cd src/main/webapp/meta/private
  ./build_flagspec.sh ${ROOTDIR}/freeciv/freeciv/data/misc/flags.spec )
mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin/webapps/ROOT.war"
