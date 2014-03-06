#!/bin/bash

if [ -d "/vagrant/" ]; then
  FREECIV_SAVE_PATH="/vagrant/resin/webapps/ROOT/savegames/"
else
  FREECIV_SAVE_PATH="~/freeciv-build/freeciv-web/resin/webapps/ROOT/savegames/"
fi
export FREECIV_SAVE_PATH


nohup python3.3 publite2.py || tail -5 nohup.out && sleep 5  &
