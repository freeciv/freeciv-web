#!/bin/bash
# setups freeciv-web

ROOTDIR="$(pwd)/.."
cp src/main/webapp/META-INF/context.xml.dist src/main/webapp/META-INF/context.xml
./build.sh
