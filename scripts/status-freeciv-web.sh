#!/bin/bash
# Checks status of various Freeciv-web processes

echo "checking nginx"
wget --quiet --spider http://localhost 
if [ "$?" != 0 ]; then
  wget --spider http://localhost 
  echo "nginx not running!"
  echo "\n\n\n"
else
  echo "nginx OK!"
fi

echo "checking resin"
wget --quiet --spider http://localhost:8080/
if [ "$?" != 0 ]; then
  wget --spider http://localhost:8080/
  echo "resin not running!"
  echo "\n\n\n"
else
  echo "resin OK!"
fi


echo "checking freeciv-proxy"
wget --quiet http://localhost:8002/status
if [ "$?" != 0 ]; then
  wget http://localhost:8002/status
  echo "freeciv-proxy not running!"
  echo "\n\n\n"

else
  echo "freeciv-proxy OK!"
  rm status
fi

echo "checking freeciv-web / publite2"
if [ "$(pidof freeciv-web)" ] 
then
  echo "freeciv-web OK!"
else
  echo "freeciv-web not running"
fi


echo "Check done!"
