#!/bin/bash
# builds Freeciv-web and copies the war file to Tomcat.

TOMCATDIR="/var/lib/tomcat8"
DATADIR="/var/lib/tomcat8/webapps/data/"
ROOTDIR="$(pwd)/.."

( cd src/main/webapp/meta/private
  ./build_flagspec.sh ${ROOTDIR}/freeciv/freeciv/data/misc/flags.spec )

# Creating build.txt info file
REVTMP="$(git rev-parse HEAD 2>/dev/null)"
if test "x$REVTMP" != "x" ; then
  # This is build from git repository.
  echo "This build is from freeciv-web commit: $REVTMP" > ${ROOTDIR}/freeciv-web/src/main/webapp/build.txt
  if ! test $(git diff | wc -l) -eq 0 ; then
    echo "It had local modifications." >> ${ROOTDIR}/freeciv-web/src/main/webapp/build.txt
  fi
  date >> ${ROOTDIR}/freeciv-web/src/main/webapp/build.txt
else
  rm -f ${ROOTDIR}/freeciv-web/src/main/webapp/build.txt
fi

#create data webapp for savegames.
mkdir -p $DATADIR/savegames
mkdir -p $DATADIR/scorelogs
mkdir -p $DATADIR/ranklogs

echo "maven package"
mvn package && cp target/freeciv-web.war "${TOMCATDIR}/webapps/ROOT.war"
