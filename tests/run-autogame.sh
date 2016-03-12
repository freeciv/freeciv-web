#!/bin/bash

echo "Running Freeciv-web server in autogame mode."
stop-freeciv-web.sh 
sleep 5

cp ../publite2/pubscript_autogame.serv ../publite2/pubscript_singleplayer.serv

start-freeciv-web.sh
sleep 5

xvfb-run casperjs --engine=phantomjs test freeciv-web-autogame.js || (>&2 echo "Freeciv-web CasperJS tests failed!" && exit 1)


