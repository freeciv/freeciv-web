#   Copyright (C) 2018  The Freeciv-web project
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Installation of out-of-package-manager components

declare -a ext_installed

ext_install_tomcat8 () {
  local TOMCAT_URL
  local TMPFILE

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

  if ! getent group tomcat8 > /dev/null 2>&1 ; then
    sudo groupadd --system tomcat8
  fi
  if ! id tomcat8 > /dev/null 2>&1 ; then
    sudo useradd --system --home /var/lib/tomcat8 -g tomcat8 --shell /bin/false tomcat8
  fi

  sudo chgrp -R tomcat8 /var/lib/tomcat8
  sudo chmod -R g+r /var/lib/tomcat8/conf
  sudo chmod g+x /var/lib/tomcat8/conf
  sudo chown -R tomcat8 /var/lib/tomcat8/{webapps,work,temp,logs}
  sudo setfacl -m d:g:tomcat8:rwX /var/lib/tomcat8/webapps

  echo "export CATALINA_HOME=\"/var/lib/tomcat8\"" >> ~/.bashrc
  ext_installed[${#ext_installed[@]}]="tomcat8"
}

ext_install_casperjs () {
  echo "==== Installing CasperJS for testing ===="
  cd "${basedir}/tests"
  curl -LOsS 'https://github.com/casperjs/casperjs/archive/1.1.4.zip'
  unzip -qo 1.1.4.zip
  rm 1.1.4.zip
  sudo ln -sf "${basedir}/casperjs-1.1.4/bin/casperjs" /usr/local/bin/casperjs
  ext_installed[${#ext_installed[@]}]="casperjs"
}

