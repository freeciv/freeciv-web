#!/bin/bash
# Checks status of various Freeciv-web processes

printf "Checking that Freeciv-web is running correctly... \n\n\n"
printf "checking nginx on http://localhost\n"
wget -O /tmp/status1.log --quiet --spider --no-check-certificate http://localhost 
if [ "$?" != 0 ]; then
  wget -O /tmp/status1.log --spider --no-check-certificate http://localhost 
  echo "nginx not running!"
  echo "\n\n\n"
else
  echo "nginx OK!"
fi

printf "\n--------------------------------\n";

printf "checking Tomcat on http://localhost:8080/\n"
wget -O /tmp/status2.log --quiet --spider --no-check-certificate http://localhost:8080/
if [ "$?" != 0 ]; then
  wget -O /tmp/status2.log --spider --no-check-certificate http://localhost:8080/
  echo "Tomcat not running!"
  echo "\n\n\n"
else
  echo "Tomcat OK!"
fi

printf "\n--------------------------------\n";

echo "checking freeciv-proxy directly on http://localhost:7001/status"
wget -O /tmp/status3.log --quiet --no-check-certificate http://localhost:7001/status
if [ "$?" != 0 ]; then
  wget -O /tmp/status3.log --no-check-certificate http://localhost:7001/status
  echo "freeciv-proxy not running!"
  echo "\n\n\n"

else
  echo "freeciv-proxy OK!"
fi

printf "\n--------------------------------\n";
echo "checking freeciv-proxy through nginx on http://localhost/civsocket/7001/status"
wget -O /tmp/status4.log --quiet --no-check-certificate http://localhost/civsocket/7001/status
if [ "$?" != 0 ]; then
  wget -O /tmp/status4.log --no-check-certificate http://localhost/civsocket/7001/status
  echo "freeciv-proxy not running correctly through nginx!"
  echo "\n\n\n"

else
  echo "freeciv-proxy OK!"
fi

printf "\n--------------------------------\n";

echo "testing WebSocket connection directly to Tornado"
echo "This should show \"HTTP/1.1 101 Switching Protocols\" and then timeout after 2 seconds."
curl -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Host: localhost" -H "Origin: http://localhost" -H "Sec-WebSocket-Version: 13" -H 'Sec-WebSocket-Key: +onQ3ZxjWlkNa0na6ydhNg==' --max-time 2  http://localhost:7002/civsocket/7002

echo "testing WebSocket connection through nginx"
echo "This should show \"HTTP/1.1 101 Switching Protocols\" and then timeout after 2 seconds."
curl -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Host: localhost" -H "Origin: http://localhost" -H "Sec-WebSocket-Version: 13" -H 'Sec-WebSocket-Key: +onQ3ZxjWlkNa0na6ydhNg==' --max-time 2  http://localhost/civsocket/7002
printf "\n--------------------------------\n ";

echo "checking freeciv-web / publite2"
if [ "$(pidof freeciv-web)" ] 
then
  echo "freeciv-web OK!"
else
  echo "freeciv-web not running"
fi

printf "\n--------------------------------\n";

echo "checking that webclient.min.js has been generated..."
wget -O /tmp/status5.log --quiet --spider --no-check-certificate http://localhost/javascript/webclient.min.js
if [ "$?" != 0 ]; then
  wget -O /tmp/status5.log --spider --no-check-certificate http://localhost/javascript/webclient.min.js 
  echo "webclient.min.js is not OK"
  echo "\n\n\n"
else
  echo "webclient.min.js is OK!"
fi

printf "\n--------------------------------\n";

echo "checking that tileset has been generated..."
wget -O /tmp/status6.log --quiet --spider --no-check-certificate http://localhost/tileset/freeciv-web-tileset-amplio2-0.png
if [ "$?" != 0 ]; then
  wget -O /tmp/status6.log --spider --no-check-certificate http://localhost/tileset/freeciv-web-tileset-amplio2-0.png 
  echo "tileset is not OK"
  echo "\n\n\n"
else
  echo "tileset is OK!"
fi

printf "\n--------------------------------\n";

echo "checking metaserver on PHP server..."
wget -O /tmp/status7.log --quiet --spider --no-check-certificate http://localhost/meta/metaserver.php
if [ "$?" != 0 ]; then
  wget -O /tmp/status7.log --spider --no-check-certificate http://localhost/meta/metaserver.php 
  echo "metaserver is not OK"
  echo "\n\n\n"
else
  echo "metaserver is OK!"
fi

printf "\n--------------------------------\n";
echo "Check of Freeciv-web is done!"
