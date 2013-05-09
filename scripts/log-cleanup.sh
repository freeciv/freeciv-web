#!/bin/bash
# cleans up logs

cp /dev/null /home/freeciv/freeciv-build/resin/log/jvm-app-0.log
cp /dev/null /home/freeciv/freeciv-build/freeciv-web/publite2/nohup.out
cp /dev/null /home/freeciv/freeciv-build/freeciv-web/freeciv-proxy/nohup.out
cp /dev/null /usr/local/nginx/logs/access.log
cp /dev/null /usr/local/nginx/logs/error.log

