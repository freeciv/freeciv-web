#!/bin/bash
# builds javascript files Freeciv-web and copies the resulting file to tomcat.

mvn compile && cp target/freeciv-web/javascript/webclient.min.* /var/lib/tomcat8/webapps/ROOT/javascript/

# update timestamp to clear browser cache.
sed -i.bak -e "s/ts=\"/ts=\"1/" -e "s/\?ts=/\?ts=1/" /var/lib/tomcat8/webapps/ROOT/webclient/index.jsp 


