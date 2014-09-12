#!/bin/bash
# setups freeciv-web

ROOTDIR="$(pwd)/.."
cp src/main/webapp/meta/php_code/local.php.dist src/main/webapp/meta/php_code/local.php
./build.sh
