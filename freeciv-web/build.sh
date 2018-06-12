#!/bin/bash
# builds Freeciv-web and copies the war file to Tomcat.

TOMCATDIR="/var/lib/tomcat8"
ROOTDIR="$(pwd)/.."

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

echo "maven package"
mvn flyway:migrate package && cp target/freeciv-web.war "${TOMCATDIR}/webapps/"
