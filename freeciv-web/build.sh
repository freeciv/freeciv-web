#!/usr/bin/env bash

# Builds Freeciv-web and copies the war file to Tomcat.

BATCH_MODE=""

while [[ $# -gt 0 ]]; do
  case $1 in
    -B) BATCH_MODE="-B"; shift;;
    *) echo "Unrecognized argument: $1"; shift;;
  esac
done

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

TOMCATDIR="/var/lib/tomcat10"
WEBAPP_DIR="${DIR}/target/freeciv-web"

# Creating build.txt info file
REVTMP="$(git rev-parse HEAD 2>/dev/null)"

if test "$REVTMP" != "" ; then
  # This is build from git repository.
  mkdir -p "${WEBAPP_DIR}"
  echo "This build is from freeciv-web commit: $REVTMP" > "${WEBAPP_DIR}/build.txt"
  if ! test $(git diff | wc -l) -eq 0 ; then
    echo "It had local modifications." >> "${WEBAPP_DIR}/build.txt"
  fi
  date >> "${WEBAPP_DIR}/build.txt"
else
  rm -f "${WEBAPP_DIR}/build.txt"
fi

echo "maven package"
mvn ${BATCH_MODE} -Dflyway.configFiles=flyway.properties flyway:migrate package && \
echo "Copying target/freeciv-web.war to ${TOMCATDIR}/webapps" && \
  cp target/freeciv-web.war "${TOMCATDIR}/webapps/"
