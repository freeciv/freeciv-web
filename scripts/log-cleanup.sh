#!/bin/bash
# cleans up logs

cp /dev/null /home/freeciv/freeciv-web/resin/log/jvm-app-0.log
cp /dev/null /home/freeciv/freeciv-web/resin/log/access.log
cp /dev/null /home/freeciv/freeciv-web/freeciv-web/logs/*.log
cp /dev/null /usr/local/nginx/logs/access.log
cp /dev/null /usr/local/nginx/logs/error.log

