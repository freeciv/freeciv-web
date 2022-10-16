#! /bin/bash
# starts freeciv-proxy and freeciv-web.
# This script is started by civlauncher.py in publite2.

if [ "$#" -ne 6 ]; then
  echo "init-freeciv-web.sh error: incorrect number of parameters." >&2
  exit 1
fi

declare -a args

addArgs() {
  local i=${#args[*]}
  for v in "$@"; do
    args[i]=${v}
    let i++
  done
}

echo "init-freeciv-web.sh port ${2}"

addArgs --debug 1
addArgs --port "${2}"
addArgs --Announce none
addArgs --exit-on-end
addArgs --meta --keep --Metaserver "http://${4}"
addArgs --type "${5}"
addArgs --read "pubscript_${6}.serv"
addArgs --log "../logs/freeciv-web-log-${2}.log"

if [ "$5" = "pbem" ]; then
  addArgs --Ranklog "/var/lib/tomcat10/webapps/data/ranklogs/rank_${2}.log"
fi

if [ -f "${6}.ruleset" ] ; then
  addArgs --ruleset $(cat "${6}.ruleset")
fi

savesdir=${1}
if [ "$5" = "longturn" ]; then
  savesdir="${savesdir}/lt/${6}"
  mkdir -p "${savesdir}"

  grep -q '^#\s*autoreload\s*$' "pubscript_${6}.serv"
  if [ $? -eq 0 ]; then
    lastsave=$(ls -t "${savesdir}" | head -n 1)
    if [ -n "${lastsave}" ]; then
      addArgs --file "${lastsave%.*}"
    fi
  fi
else
  addArgs --quitidle 20
fi
addArgs --saves "${savesdir}" --scenarios "${savesdir}"

export FREECIV_SAVE_PATH=${savesdir}
export FREECIV_SCENARIO_PATH=${savesdir}
rm -f "/var/lib/tomcat10/webapps/data/scorelogs/score-${2}.log"

python3 ../freeciv-proxy/freeciv-proxy.py "${3}" > "../logs/freeciv-proxy-${3}.log" 2>&1 &
proxy_pid=$! && 
${HOME}/freeciv/bin/freeciv-web "${args[@]}" > /dev/null 2> "../logs/freeciv-web-stderr-${2}.log"

rc=$?;
kill -9 $proxy_pid;
exit $rc
