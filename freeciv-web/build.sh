#!/bin/bash
# builds Freeciv-web and copies the war file to resin.

ROOTDIR="$(pwd)/.."
cp src/main/webapp/meta/php_code/local.php.dist src/main/webapp/meta/php_code/local.php
mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin/webapps/ROOT.war"
