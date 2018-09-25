#! /bin/bash
# starts freeciv-proxy and freeciv-web.
# This script is started by civlauncher.py in publite2.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

if [ "$#" -ne 6 ]; then
  echo "init-freeciv-web.sh error: incorrect number of parameters." >&2
  exit 1
fi

echo "init-freeciv-web.sh port ${2}"

if [ $5 = "pbem" ]; then
   pbemcmd="--Ranklog /var/lib/tomcat8/webapps/data/ranklogs/rank_${2}.log "
fi

quitidle=" -q 20"
if [[ $6 == *"longturn"* ]]; then
  quitidle=" "
fi

export FREECIV_SAVE_PATH=${1};
# TODO: Needs to read location from environment variable
rm -f /var/lib/tomcat8/webapps/data/scorelogs/score-${2}.log; 
mkdir -p "${DIR}"/../logs
# This allows civcom to be found and imported
cd "${DIR}"/../freeciv-proxy/
python3 freeciv-proxy.py ${3} && \
proxy_pid=$! && \
${HOME}/freeciv/bin/freeciv-web --debug=1 --port ${2} --keep ${quitidle} --Announce none -e  -m \
-M http://${4} --type ${5} --read pubscript_${6}.serv --log ${DIR}/../logs/freeciv-web-log-${2}.log \
--saves ${1} ${pbemcmd:- }

rc=$?; 
kill -9 $proxy_pid; 
exit $rc
