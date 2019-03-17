#! /bin/bash
# Starts freeciv-proxy.

until python3 ../freeciv-proxy/freeciv-proxy.py "${1}" > "../logs/freeciv-proxy-${3}.log" 2>&1 ; do
  echo "Freeciv-proxy crashed with exit code $?. Restarting" >&2
  sleep 10
done
