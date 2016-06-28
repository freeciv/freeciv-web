#!/bin/bash
# builds javascript files Freeciv-web and copies the resulting file to tomcat.

mvn compile && cp target/freeciv-web/javascript/webclient.min.* /var/lib/tomcat8/webapps/ROOT/javascript/
