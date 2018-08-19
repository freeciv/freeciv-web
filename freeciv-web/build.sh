#!/bin/bash
# builds Freeciv-web and copies the war file to Tomcat.

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

TOMCATDIR="/var/lib/tomcat8"
WEBAPP_DIR="${DIR}/target/freeciv-web"

# Creating build.txt info file
REVTMP="$(git rev-parse HEAD 2>/dev/null)"
if test "x$REVTMP" != "x" ; then
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
mvn -B flyway:migrate package && \
echo "Copying target/freeciv-web.war to ${TOMCATDIR}/webapps" && \
  cp target/freeciv-web.war "${TOMCATDIR}/webapps/"
