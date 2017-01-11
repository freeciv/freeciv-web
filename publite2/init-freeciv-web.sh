#! /bin/bash
# starts freeciv-proxy and freeciv-web.
# This script is started by civlauncher.py in publite2.

if [ "$#" -ne 5 ]; then
  echo "init-freeciv-web.sh error: incorrect number of parameters." >&2
  exit 1
fi

echo "init-freeciv-web.sh port ${2}"

if [ $5 = "pbem" ]; then
   pbemcmd="--Ranklog /var/lib/tomcat8/webapps/data/ranklogs/rank_${2}.log "
fi

ulimit -t 10000 && ulimit -Sv 500000 && \
export FREECIV_SAVE_PATH=${1};
rm -f /var/lib/tomcat8/webapps/data/scorelogs/score-${2}.log; 

python3 ../freeciv-proxy/freeciv-proxy.py ${3} > ../logs/freeciv-proxy-${3}.log 2>&1 &
proxy_pid=$! && 
${HOME}/freeciv/bin/freeciv-web --debug=1 --port ${2} -q 20 --Announce none -e  -m \
-M http://${4} --type ${5} --read pubscript_${5}.serv --log ../logs/freeciv-web-log-${2}.log \
--saves ${1} ${pbemcmd:- } > /dev/null 2> ../logs/freeciv-web-stderr-${2}.log;

rc=$?; 
kill -9 $proxy_pid; 
exit $rc
