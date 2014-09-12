#!/bin/bash
# builds Freeciv-web and copies the war file to resin.

ROOTDIR="$(pwd)/.."
mvn install && cp target/freeciv-web.war "${ROOTDIR}/resin/webapps/ROOT.war"
