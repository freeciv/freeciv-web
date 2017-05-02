#!/bin/bash
# builds javascript files Freeciv-web and copies the resulting file to tomcat.

# workaround for https://github.com/samaxes/minify-maven-plugin/issues/142
rm -rf target/freeciv-web/javascript/webclient.min.js.map
rm -rf target/freeciv-web/javascript/webgl/libs/webgl-client.min.js.map

mvn compile && cp target/freeciv-web/javascript/webclient.* /var/lib/tomcat8/webapps/ROOT/javascript/ && cp target/freeciv-web/javascript/webgl/libs/webgl-client* /var/lib/tomcat8/webapps/ROOT/javascript/webgl/libs/

# update timestamp to clear browser cache.
sed -i.bak -e "s/ts=\"/ts=\"1/" -e "s/\?ts=/\?ts=1/" /var/lib/tomcat8/webapps/ROOT/webclient/index.jsp 

cp src/main/webapp/javascript/webgl/shaders/*.* /var/lib/tomcat8/webapps/ROOT/javascript/webgl/shaders/
