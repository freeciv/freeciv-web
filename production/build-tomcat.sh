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

echo "==== Installing tomcat8 ===="
  TOMCAT_URL=$(curl -LsS 'https://tomcat.apache.org/download-80.cgi' | sed -n 's/^.*href="\([^"]*bin\/apache-tomcat-[0-9.]*tar\.gz\)".*/\1/p' | head -n 1)
  if [ -z "${TOMCAT_URL}" ]; then
    echo >&2 "Couldn't fetch download URL"
    exit 1
  fi

  echo "Downloading tomcat8 from ${TOMCAT_URL}"
  TMPFILE=$(mktemp -t tomcat8.XXXX.tar.gz)
  curl -LsS -o "${TMPFILE}" "${TOMCAT_URL}"

  cd /var/lib
  sudo tar -xzf "${TMPFILE}"
  sudo mv apache-tomcat-8.* tomcat8
  rm "${TMPFILE}"

  # if ! getent group tomcat8 > /dev/null 2>&1 ; then
  #   sudo groupadd --system tomcat8
  # fi
  # if ! id tomcat8 > /dev/null 2>&1 ; then
  #   sudo useradd --system --home ${TOMCAT_HOME} -g tomcat8 --shell /bin/false tomcat8
  # fi

  # sudo chgrp -R tomcat8 ${TOMCAT_HOME}
  # sudo chmod -R g+r "${TOMCAT_HOME}"/conf
  # sudo chmod g+x "${TOMCAT_HOME}"/conf
  # sudo chown -R tomcat8 "${TOMCAT_HOME}"/{webapps,work,temp,logs}
  # sudo setfacl -m d:g:tomcat8:rwX "${TOMCAT_HOME}"/webapps

echo "==== Installing Wikipedia helper ===="
  sudo -H pip3 install wikipedia

echo "==== Installing Node.js ===="
  APT_GET='DEBIAN_FRONTEND=noninteractive apt-get -y -qq'
  curl -LOsS 'https://deb.nodesource.com/setup_8.x'
  sudo bash setup_8.x
  sudo ${APT_GET} install --no-install-recommends nodejs
  # Populate ~/.config with current user
  npm help > /dev/null
  

echo "==== Installing Handlebars ===="
  sudo -H npm install handlebars -g


echo "==== Installing Tornado Web Server ===="
  TMPINSTDIR=$(mktemp -d)
  (
    cd "${TMPINSTDIR}"
    curl -LOsS 'https://github.com/tornadoweb/tornado/archive/v4.5.3.tar.gz'
    tar -xzf v4.5.3.tar.gz
    cd tornado-4.5.3
    sudo -H python3 setup.py install
  )
  sudo rm -rf "${TMPINSTDIR}"
  
echo "==== Preparing Tomcat ===="
  cd "${TOMCAT_HOME}"
  sudo setfacl -m d:u:root:rwX,u:root:rwx webapps
  mkdir -p webapps/data/{savegames/pbem,scorelogs,ranklogs}
  # setfacl -Rm d:u:tomcat8:rwX webapps/data
  cd "${basedir}"
