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

echo "==== Installing dependencies ===="
  dependencies="nginx maven acl openjdk-8-jdk-headless procps curl sed tar unzip git gnupg libcurl4-openssl-dev libjansson-dev pngcrush python3-pil python3-dev python3-setuptools python3-wheel libtool patch make automake autoconf autotools-dev python3-minimal libbz2-dev imagemagick python3-pip liblzma-dev libicu-dev pkg-config zlib1g-dev libsqlite3-dev webp libmagickcore.*extra libmagickwand-dev python3-mysql.connector"
  APT_GET='DEBIAN_FRONTEND=noninteractive apt-get -y -qq'

  sudo ${APT_GET} install --no-install-recommends python3-software-properties software-properties-common
  sudo add-apt-repository -y ppa:openjdk-r/ppa
  sudo ${APT_GET} update
  sudo ${APT_GET} upgrade
  sudo ${APT_GET} install --no-install-recommends ${dependencies}

  # Where default-jdk is v7, it may be installed with a higher priority than v8
  for n in java javac; do
    sudo update-alternatives --set $n $(update-alternatives --query $n | sed -n 's/Alternative: \(.*java-8.*\)/\1/p' | head -n 1)
  done