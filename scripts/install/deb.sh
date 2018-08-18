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

dependencies="nginx maven mysql-server openjdk-8-jdk-headless acl procps curl sed tar unzip git gnupg libcurl4-openssl-dev libjansson-dev pngcrush python3-pil python3-dev python3-setuptools python3-wheel libtool patch make automake autoconf autotools-dev python3-minimal libbz2-dev imagemagick python3-pip liblzma-dev libicu-dev pkg-config zlib1g-dev libsqlite3-dev webp libmagickcore.*extra libmagickwand-dev"

export DEBIAN_FRONTEND=noninteractive
INSTALLED_TOMCAT=N
INSTALLED_MYSQLPYCON=N
APT_GET='apt-get -y -qq -o=Dpkg::Use-Pty=0'

sudo ${APT_GET} update

if ! apt-cache -qq show openjdk-8-jdk-headless > /dev/null; then
  echo "==== Adding openjdk-8 repo ===="
  if [ "${FCW_INSTALL_VND}" = Ubuntu ]; then
    sudo ${APT_GET} install --no-install-recommends python3-software-properties software-properties-common
    sudo add-apt-repository -y ppa:openjdk-r/ppa
  else
    echo "deb http://http.debian.net/debian ${FCW_INSTALL_REL}-backports main" | sudo tee --append /etc/apt/sources.list.d/${FCW_INSTALL_REL}-backports.list > /dev/null
  fi
  sudo ${APT_GET} update
fi

if apt-cache -qq show tomcat8 > /dev/null; then
  dependencies="${dependencies} tomcat8 tomcat8-admin"
  INSTALLED_TOMCAT=Y
else
  INSTALLED_TOMCAT=N
fi

if [ "${FCW_INSTALL_MODE}" = TEST ]; then
  dependencies="${dependencies} xvfb"
fi

apt-cache policy python3-mysql.connector | while IFS= read -r line; do
  if [ "${line::13}" = "  Candidate: " ]; then
    IFS=. v=(${line#  Candidate: })
    if [ ${v[0]} -lt 2 ]; then
      INSTALLED_MYSQLPYCON=N
    elif [ ${v[0]} -eq 2 ] && [ ${v[1]} -lt 1 ]; then
      INSTALLED_MYSQLPYCON=N
    else
      dependencies="${dependencies} python3-mysql.connector"
      INSTALLED_MYSQLPYCON=Y
    fi
  fi
done

echo "==== Installing Updates and Dependencies ===="
echo "apt-get upgrade"
sudo ${APT_GET} upgrade
echo "mysql setup..."
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password password ${DB_ROOT_PASSWORD}"
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password_again password ${DB_ROOT_PASSWORD}"
echo "apt-get install dependencies"
sudo ${APT_GET} install --no-install-recommends ${dependencies}

# Where default-jdk is v7, it may be installed with a higher priority than v8
for n in java javac; do
  sudo update-alternatives --set $n $(update-alternatives --query $n | sed -n 's/Alternative: \(.*java-8.*\)/\1/p' | head -n 1)
done

if [ "${INSTALLED_TOMCAT}" = N ]; then
  ext_install_tomcat8
fi

if [ "${INSTALLED_MYSQLPYCON}" = N ]; then
  ext_install_mysql_connector_python
fi

ext_install_tornado

TMPINSTDIR=$(mktemp -d)

echo "==== Installing Wikipedia helper ===="
cd "${TMPINSTDIR}"
sudo -H pip3 install wikipedia

echo "==== Installing Node.js ===="
cd "${TMPINSTDIR}"
curl -LOsS 'https://deb.nodesource.com/setup_8.x'
sudo bash setup_8.x
sudo ${APT_GET} install --no-install-recommends nodejs
# Populate ~/.config with current user
npm help > /dev/null

echo "==== Installing Handlebars ===="
cd "${TMPINSTDIR}"
sudo -H npm install handlebars -g

if [ "${FCW_INSTALL_MODE}" = TEST ]; then
  ext_install_casperjs
fi

