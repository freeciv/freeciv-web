#!/bin/bash
# builds Freeciv-web and copies the war file to resin.

ROOTDIR="$(pwd)/.."
cp -R ${ROOTDIR}/freeciv/freeciv/data/flags/*.svg ${ROOTDIR}/freeciv-web/src/main/webapp/images/flags/
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
else
  rm -f ${ROOTDIR}/freeciv-web/src/main/webapp/build.txt
fi

mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin/webapps/ROOT.war"
