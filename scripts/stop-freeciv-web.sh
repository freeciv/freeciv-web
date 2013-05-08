#!/bin/bash
# Shitdown script for Freeciv-web

echo "Shutting down Freeciv-web: nginx, resin, publite2, freeciv-proxy."

# 1. nginx
sudo killall nginx

# 2. Resin
~/freeciv-build/resin/bin/resin.sh stop 

#3. publite2
killall -9 freeciv-web
ps aux | grep -ie publite2 | awk '{print $2}' | xargs kill -9 


#4. freeciv-proxy

ps aux | grep -ie freeciv-websocket-proxy | awk '{print $2}' | xargs kill -9 



