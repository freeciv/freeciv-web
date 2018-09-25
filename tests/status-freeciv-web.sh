#!/bin/bash
# Checks status of various Freeciv-web processes

alias curl='stdbuf -i0 -o0 -e0 curl'

checkURL () {
  local URL=$1
  local logfile=$2
  shift 2
  curl --no-buffer --silent --show-error --fail --insecure "$@" -o "${logfile}" "${URL}" >> "${logfile}" 2>&1
}

rm -f /tmp/status[1-7].log
printf "Checking that Freeciv-web is running correctly... \n\n\n"
printf "checking nginx on http://localhost\n"
if checkURL http://localhost /tmp/status1.log --head; then
  echo "nginx OK!"
else
  cat /tmp/status1.log
  echo "nginx not running!"
  exit 1
fi

printf "\n--------------------------------\n";

printf "checking Tomcat on http://localhost/\n"
if checkURL http://localhost/ /tmp/status2.log --head; then
  echo "Tomcat OK!"
else
  cat /tmp/status2.log
  echo "Tomcat not running!"
  exit 1
fi

printf "checking freeciv-web on Tomcat on http://localhost/webclient\n"
if checkURL http://localhost/webclient /tmp/status2.log --head; then
  echo "freeciv-web webapp OK!"
else
  cat /tmp/status2.log
  echo "freeciv-web webapp not running!"
  exit 1
fi


printf "\n--------------------------------\n";
echo "checking freeciv-proxy through nginx on http://localhost/civsocket/7001/status"
if checkURL http://localhost/civsocket/7001/status /tmp/status4.log; then
  echo "freeciv-proxy OK!"
else
  cat /tmp/status4.log
  echo "freeciv-proxy not running correctly through nginx!"
  exit 1
fi

printf "\n--------------------------------\n";

echo "testing WebSocket connection through nginx"
echo "This should show \"HTTP/1.1 101 Switching Protocols\" and then timeout after 2 seconds."
curl -isSN -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Host: localhost" -H "Origin: http://localhost" -H "Sec-WebSocket-Version: 13" -H 'Sec-WebSocket-Key: +onQ3ZxjWlkNa0na6ydhNg==' --max-time 2  http://localhost/civsocket/7002
printf "\n--------------------------------\n ";


printf "\n--------------------------------\n";

echo "checking that webclient.min.js has been generated..."
if checkURL http://localhost/javascript/webclient.min.js /tmp/status5.log --head; then
  echo "webclient.min.js is OK!"
else
  cat /tmp/status5.log
  echo "webclient.min.js is not OK"
  exit 1
fi

printf "\n--------------------------------\n";

echo "checking that tileset has been generated..."
if checkURL http://localhost/tileset/freeciv-web-tileset-amplio2-0.png /tmp/status6.log --head; then
  echo "tileset is OK!"
else
  cat /tmp/status6.log
  echo "tileset is not OK"
  exit 1
fi

printf "\n--------------------------------\n";

echo "checking metaserver on PHP server..."
if checkURL http://localhost/game/list /tmp/status7.log --head; then
  echo "metaserver is OK!"
else
  cat /tmp/status7.log
  echo "metaserver is not OK"
  exit 1
fi

printf "\n--------------------------------\n";
echo "Check of Freeciv-web is done!"
