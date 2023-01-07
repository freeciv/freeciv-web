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

ext_install_tomcat10 () {
  local TOMCAT_URL
  local TMPFILE

  echo "==== Installing tomcat10 ===="

  TOMCAT_URL=$(curl -LsS 'https://tomcat.apache.org/download-10.cgi' | sed -n 's/^.*href="\([^"]*bin\/apache-tomcat-[0-9.]*tar\.gz\)".*/\1/p' | head -n 1)
  if [ -z "${TOMCAT_URL}" ]; then
    echo >&2 "Couldn't fetch download URL"
    exit 1
  fi

  echo "Downloading tomcat10 from ${TOMCAT_URL}"
  TMPFILE=$(mktemp -t tomcat10.XXXX.tar.gz)
  curl -LsS -o "${TMPFILE}" "${TOMCAT_URL}"

  cd /var/lib
  sudo tar -xzf "${TMPFILE}"
  sudo mv apache-tomcat-10.* tomcat10
  rm "${TMPFILE}"

  if ! getent group tomcat > /dev/null 2>&1 ; then
    sudo groupadd --system tomcat
  fi
  if ! id tomcat > /dev/null 2>&1 ; then
    sudo useradd --system --home /var/lib/tomcat10 -g tomcat --shell /bin/false tomcat
  fi

  sudo chgrp -R tomcat /var/lib/tomcat10
  sudo chmod -R g+r /var/lib/tomcat10/conf
  sudo chmod g+x /var/lib/tomcat10/conf
  sudo chown -R tomcat /var/lib/tomcat10/{webapps,work,temp,logs}
  sudo chown tomcat /var/lib/tomcat10/bin/catalina.sh
  sudo chmod u+s /var/lib/tomcat10/bin/catalina.sh
  sudo setfacl -m d:g:tomcat:rwX /var/lib/tomcat10/webapps

  echo "export CATALINA_HOME=\"/var/lib/tomcat10\"" >> ~/.bashrc
  ext_installed[${#ext_installed[@]}]="tomcat10"
}

ext_install_meson () {
  mkdir -p ${HOME}/freeciv/meson-install

  ( cd ${HOME}/freeciv/meson-install

    if ! wget "https://github.com/mesonbuild/meson/releases/download/${MESON_VER}/meson-${MESON_VER}.tar.gz" ; then
      echo "Meson download failed!" >&2
      exit 1
    fi

    tar xzf meson-${MESON_VER}.tar.gz
    ln -s "meson-${MESON_VER}/meson.py" meson
  )

  ext_installed[${#ext_installed[@]}]="meson"
}
