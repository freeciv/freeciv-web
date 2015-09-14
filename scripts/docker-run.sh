#!/bin/bash

service mysql start
service nginx start
cd /docker/scripts/
sudo -u freeciv ./start-freeciv-web.sh
