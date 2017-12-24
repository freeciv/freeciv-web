#!/bin/bash

sudo service mysql start
sudo service nginx start
cd /docker/scripts/
sudo -u freeciv ./start-freeciv-web.sh

while [ 1 ]
do
  sleep 60 &
  wait $!
done
