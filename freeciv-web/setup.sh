#!/bin/bash
# setups freeciv-web

ROOTDIR="$(pwd)/.."
cp src/main/webapp/meta/php_code/local.php.dist src/main/webapp/meta/php_code/local.php
cp src/main/webapp/META-INF/context.xml.dist src/main/webapp/META-INF/context.xml
./build.sh
