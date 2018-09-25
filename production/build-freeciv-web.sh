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

echo "==== Building freeciv-web ===="
  cd "${basedir}"/freeciv-web && \
    ./build.sh -B || \
    handle_error 7 "Failed to build freeciv-web server"
