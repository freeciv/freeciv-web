#!/bin/bash
# builds javascript files Freeciv-web and copies the resulting file to resin.

mvn compile && cp target/freeciv-web/javascript/webclient.min.js ../resin/webapps/ROOT/javascript/
