#!/bin/bash
# nightly restart of PBEM process.

kill $(ps -ef | grep python| grep pbem | awk '{print $2}')
cd ~/freeciv-web/freeciv-web/pbem
nohup python3 -u pbem.py > ../logs/pbem.log 2>&1 &
echo "restart of PBEM process completed."

