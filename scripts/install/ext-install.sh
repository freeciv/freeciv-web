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

ext_install_tomcat9 () {
  local TOMCAT_URL
  local TMPFILE

  echo "==== Installing tomcat9 ===="

  TOMCAT_URL=$(curl -LsS 'https://tomcat.apache.org/download-90.cgi' | sed -n 's/^.*href="\([^"]*bin\/apache-tomcat-[0-9.]*tar\.gz\)".*/\1/p' | head -n 1)
  if [ -z "${TOMCAT_URL}" ]; then
    echo >&2 "Couldn't fetch download URL"
    exit 1
  fi

  echo "Downloading tomcat9 from ${TOMCAT_URL}"
  TMPFILE=$(mktemp -t tomcat9.XXXX.tar.gz)
  curl -LsS -o "${TMPFILE}" "${TOMCAT_URL}"

  cd /var/lib
  sudo tar -xzf "${TMPFILE}"
  sudo mv apache-tomcat-9.* tomcat9
  rm "${TMPFILE}"

  if ! getent group tomcat > /dev/null 2>&1 ; then
    sudo groupadd --system tomcat
  fi
  if ! id tomcat > /dev/null 2>&1 ; then
    sudo useradd --system --home /var/lib/tomcat9 -g tomcat --shell /bin/false tomcat
  fi

  sudo chgrp -R tomcat /var/lib/tomcat9
  sudo chmod -R g+r /var/lib/tomcat9/conf
  sudo chmod g+x /var/lib/tomcat9/conf
  sudo chown -R tomcat /var/lib/tomcat9/{webapps,work,temp,logs}
  sudo setfacl -m d:g:tomcat:rwX /var/lib/tomcat9/webapps

  echo "export CATALINA_HOME=\"/var/lib/tomcat9\"" >> ~/.bashrc
  ext_installed[${#ext_installed[@]}]="tomcat9"
}

ext_install_casperjs () {
  version=1.1.4-2
  echo "==== Installing CasperJS ${version} for testing ===="
  cd "${basedir}/tests"
  curl -LOsS "https://github.com/casperjs/casperjs/archive/${version}.zip"
  unzip -qo "${version}".zip
  rm "${version}".zip
  sudo ln -sf "${basedir}/tests/casperjs-${version}/bin/casperjs" /usr/local/bin/casperjs
  ext_installed[${#ext_installed[@]}]="casperjs"
}

