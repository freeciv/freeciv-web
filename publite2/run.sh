#!/bin/bash

export LANG=C
FREECIV_SAVE_PATH="~/freeciv-build/resin/webapps/ROOT/savegames/"
export FREECIV_SAVE_PATH

nohup python3.3 publite2.py || tail -5 nohup.out && sleep 5  &
