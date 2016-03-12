#!/bin/bash
# cleans up logs

find /home/freeciv/freeciv-web/freeciv-web/logs/*.log -exec cp /dev/null {} \;
cp /dev/null /usr/local/nginx/logs/access.log
cp /dev/null /usr/local/nginx/logs/error.log

