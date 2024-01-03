#!/usr/bin/env bash

# Checks status of various Freeciv-web processes

alias curl='stdbuf -i0 -o0 -e0 curl'
verbose=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -v) verbose=true; shift; shift;;
    *) shift;;
  esac
done

# Some versions of Curl will by default change the formatting of headers
# when printing them to the terminal. A bug will then cause Curl to not turn
# of the custom formatting after the headers are printed. This causes all
# subsequent console output to have the custom formatting.
# This can't be worked around with the --no-styled-output option because
# older versions of curl, that don't have --no-styled-output, fails on the
# unknown option.
work_around_curl_console_output_bug () {
  # reset the terminal formatting
  echo -ne "\e[0m"
}

checkURL () {
  local URL=$1
  shift 1
  curl --no-buffer --silent --show-error --fail --insecure "$@" "${URL}"
}
checkWS () {
  local URL=$1
  shift 1
  curl --include --no-buffer --silent --show-error \
    -H "Connection: Upgrade" \
    -H "Upgrade: websocket" \
    -H "Host: localhost" \
    -H "Origin: http://localhost" \
    -H "Sec-WebSocket-Version: 13" \
    -H 'Sec-WebSocket-Key: +onQ3ZxjWlkNa0na6ydhNg==' \
    --max-time 2 "$@" "${URL}"
  local status=$?
  work_around_curl_console_output_bug

  if [[ ${status} -eq 28 ]]; then # timeout - expected for websockets
    return 0
  elif [[ ${status} -eq 0 ]]; then
    return -1
  else
    return ${status}
  fi
}
checkWebURL () { # description, URL, curl-params
  local description="$1"
  local URL=$2
  shift 2

  checkWebStatus "${description}" "" checkURL "${URL}" "$@"
}
checkWebSocket () { # description, URL, curl-params
  local description="$1"
  local URL=$2
  shift 2
  local err_msg="Expected \"HTTP/1.1 101 Switching Protocols\" followed by timeout after 2 seconds."

  checkWebStatus "${description}" "${err_msg}" checkWS "${URL}" "$@"
}
checkWebStatus () { # description, err_msg, testFn, URL, curl-params
  local description="$1"
  local err_msg="$2"
  local testFn="$3"
  local URL="$4"
  shift 4

  if [[ "${verbose}" = true ]]; then
    description="${description} (${URL})"
  fi
  checkStatus "${description}" "${err_msg}" "${testFn}" "${URL}" "$@"
}
checkPID () { # description, program
  local description="$1"
  local program="$2"
  checkStatus "${description}" "" pidof "${program}"
}
checkService () { # service
  checkStatus "$1" "" /usr/sbin/service "$1" status
}
checkStatus () { # description, err_msg, <function and args...>
  local description="$1"
  local err_msg="$2"
  shift 2
  local logfile=$(mktemp /tmp/status.XXXXXX.log)

  printf "%-50s " "Checking ${description}"
  if "$@" > "${logfile}" 2>&1; then
    printf "OK!\n"
  else
    printf "Down\n"
    if [[ "${verbose}" = true ]]; then
      [[ -n "${err_msg}" ]] && echo "  ${err_msg}"
      cat "${logfile}" | sed 's/^/  /'; echo
    fi
  fi
  rm "${logfile}"
}


printf "Checking that Freeciv-web is running correctly... (-v for verbose)\n\n"

checkService "nginx"

checkWebURL "Tomcat" "http://localhost:8080/" --head
checkWebURL "freeciv-web on Tomcat" "http://localhost:8080/freeciv-web" --head
checkWebURL "Tomcat DB connection" "http://localhost/game/list" --head

checkWebURL "Pubstatus" "http://localhost:4002/pubstatus"
checkWebURL "Mailstatus" "http://localhost:4003/mailstatus"

checkWebURL "freeciv-proxy directly" "http://localhost:7001/status"
checkWebURL "freeciv-proxy through nginx" "http://localhost/civsocket/7001/status"

checkWebSocket "WebSocket directly (freeciv-proxy)" "http://localhost:7002/civsocket/7002"
checkWebSocket "WebSocket through nginx" "http://localhost/civsocket/7002"

checkPID "freeciv-web (spawned by publite2)" "freeciv-web"

checkWebURL "webclient.min.js generation" "http://localhost/javascript/webclient.min.js" --head
checkWebURL "tileset generation" "http://localhost/tileset/freeciv-web-tileset-amplio2-0.png" --head

printf "\n--------------------------------\n";
echo "Check of Freeciv-web is done!"
