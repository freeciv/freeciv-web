#!/bin/bash

# Freeciv-web Travis CI test script

error=0

casper_test () {
  xvfb-run --server-args="-screen 0 800x600x24" casperjs --engine=phantomjs test "$@"
  err=$?
  if [ $err -ne 0 ]; then
    >&2 echo "Freeciv-web CasperJS test $@ failed with exit code ${err}"
    error=1
  fi
}

basedir=$(pwd)

cd ${basedir}/scripts/ && ./start-freeciv-web.sh

cat ${basedir}/logs/*.log 

echo "============================================"
echo "Start testing of Freeciv-web using CasperJS:"
cd ${basedir}/tests/

casper_test freeciv-web-tests.js

echo "Running Freeciv-web server in autogame mode."
mv "${basedir}"/publite2/pubscript_singleplayer.serv{,.bak}
cp ${basedir}/publite2/pubscript_autogame.serv ${basedir}/publite2/pubscript_singleplayer.serv
killall freeciv-web
sleep 20
casper_test freeciv-web-autogame.js
mv "${basedir}"/publite2/pubscript_singleplayer.serv{.bak,}

echo "running pbem unit tests."
cd ${basedir}/pbem
if ! python3 test_pbem.py; then
  error=1
fi

#echo "=============================="
if [ ${error} -eq 0 ]; then
  echo "Freeciv-web built, tested and started correctly: Build successful!"
else
  exit 1
fi
