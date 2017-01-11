#!/bin/bash

nohup python3 -u publite2.py > ../logs/publite2.log 2>&1 || tail -5 ../logs/publite2.log && sleep 5  &
