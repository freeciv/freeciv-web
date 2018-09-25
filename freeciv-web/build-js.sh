#!/bin/bash
# builds javascript files Freeciv-web and copies the resulting file to tomcat.

FCW_DEST=${TOMCAT_HOME}/webapps/freeciv-web

mvn compile && \
echo "Copying target/javascript/webclient.* to ${FCW_DEST}/javascript" && \
  cp target/freeciv-web/javascript/webclient.* "${FCW_DEST}"/javascript/ && \
echo target/freeciv-web/javascript/webgl/libs/webgl-client* "${FCW_DEST}"/javascript/webgl/libs && \
  cp target/freeciv-web/javascript/webgl/libs/webgl-client* "${FCW_DEST}"/javascript/webgl/libs/

# update timestamp to clear browser cache.
sed -i.bak -e "s/ts=\"/ts=\"1/" -e "s/\?ts=/\?ts=1/" "${FCW_DEST}"/webclient/index.jsp

cp src/main/webapp/javascript/webgl/shaders/*.* "${FCW_DEST}"/javascript/webgl/shaders/
