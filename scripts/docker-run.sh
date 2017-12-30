#!/bin/bash

sudo service mysql start
sudo service nginx start

sudo -u freeciv mkdir /home/freeciv/freeciv
sudo -u freeciv ln -s /docker/freeciv/freeciv/server/ /home/freeciv/freeciv/bin

cd /docker/scripts/
sudo -u freeciv ./start-freeciv-web.sh

while [ 1 ]
do
  sleep 60 &
  wait $!
done
