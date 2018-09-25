#!/bin/bash
set -e
unset CDPATH
SHOW_LIST=0
basedir=/docker
export DEBIAN_FRONTEND=noninteractive
export TERM=linux
echo 'debconf debconf/frontend select Noninteractive' | sudo debconf-set-selections

handle_error () {
  local status=${1:-1}
  local msg=${2:-Unknown Error}
  local stars=$(printf '%*s' ${#msg} '')
  >&2 echo
  >&2 echo "${stars// /*}"
  >&2 echo "${msg}"
  >&2 echo "${stars// /*}"
  >&2 echo
  exit $status
}

echo "==== Building freeciv-web sync ===="
  mkdir -p "${basedir}/freeciv-web/src/derived/webapp" && \
  "${basedir}"/scripts/sync-js-hand.sh \
    -f "${basedir}/freeciv/freeciv" \
    -o "${basedir}/freeciv-web/src/derived/webapp" \
    -d "${TOMCAT_HOME}/webapps/data" || \
    handle_error 6 "Failed to synchronize freeciv project"