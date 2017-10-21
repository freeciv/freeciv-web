#!/bin/bash
# nightly restart of PBEM process.

kill $(ps -ef | grep python| grep pbem | awk '{print $2}')
sleep 4
cd /home/freeciv/freeciv-web/freeciv-web/pbem/ && /usr/local/bin/python3 -u /home/freeciv/freeciv-web/freeciv-web/pbem/pbem.py > /home/freeciv/freeciv-web/freeciv-web/logs/pbem.log 2>&1 &
echo "restart of PBEM process completed."

