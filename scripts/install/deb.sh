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

dependencies="\
  acl \
  autoconf \
  automake \
  autotools-dev \
  build-essential \
  curl \
  git \
  gnupg \
  imagemagick \
  libbz2-dev \
  libcurl4-openssl-dev \
  libicu-dev \
  libjansson-dev \
  liblzma-dev \
  libmagickcore.*extra \
  libmagickwand-dev \
  libsqlite3-dev \
  libtool \
  make \
  maven \
  default-mysql-server \
  nginx \
  openjdk-11-jdk-headless \
  patch \
  pkg-config \
  pngcrush \
  procps \
  python \
  python3-minimal \
  python3-pip \
  python3-setuptools \
  python3-wheel \
  sed \
  tar \
  unzip \
  webp \
  zlib1g-dev \
"

INSTALLED_TOMCAT=N
INSTALLED_NODEJS=N
APT_GET='DEBIAN_FRONTEND=noninteractive apt-get -y -qq -o=Dpkg::Use-Pty=0'

sudo ${APT_GET} update

if ! apt-cache -qq show openjdk-11-jdk-headless > /dev/null; then
  echo "==== Adding openjdk-11 repo ===="
  if [ "${FCW_INSTALL_VND}" = Ubuntu ]; then
    sudo ${APT_GET} install --no-install-recommends python3-software-properties software-properties-common
    sudo add-apt-repository -y ppa:openjdk-r/ppa
  else
    echo "deb http://http.debian.net/debian ${FCW_INSTALL_REL}-backports main" | sudo tee --append /etc/apt/sources.list.d/${FCW_INSTALL_REL}-backports.list > /dev/null
  fi
  sudo ${APT_GET} update
fi

if [ "$DEB_NO_TOMCAT" != "Y" ] && apt-get --simulate install tomcat9 &> /dev/null; then
  dependencies="${dependencies} tomcat9 tomcat9-admin"
  INSTALLED_TOMCAT=Y
else
  INSTALLED_TOMCAT=N
fi

debian_nodejs_packages="nodejs npm handlebars"
if [ $(lsb_release -rs) = "testing" ] && [ $(lsb_release -is) = "Debian" ] \
   && apt-get --simulate install ${debian_nodejs_packages} &> /dev/null; then
  dependencies="${dependencies} ${debian_nodejs_packages}"
  INSTALLED_NODEJS=Y
else
  INSTALLED_NODEJS=N
fi

echo "==== Installing Dependencies ===="
echo "mysql setup..."
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password password ${DB_ROOT_PASSWORD}"
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password_again password ${DB_ROOT_PASSWORD}"
echo "apt-get install dependencies"
sudo ${APT_GET} install --no-install-recommends ${dependencies}

# Where default-jdk is older version, it may be installed with a higher priority than v11
for n in java javac; do
  sudo update-alternatives --set $n $(update-alternatives --query $n | sed -n 's/Alternative: \(.*java-11.*\)/\1/p' | head -n 1)
done

if [ "${INSTALLED_TOMCAT}" = N ]; then
  ext_install_tomcat9
fi

TMPINSTDIR=$(mktemp -d)

echo "==== Installing Node.js ===="
if [ "${INSTALLED_NODEJS}" = N ]; then
  cd "${TMPINSTDIR}"
  curl -LOsS 'https://deb.nodesource.com/setup_14.x'
  sudo bash setup_14.x
  sudo ${APT_GET} install --no-install-recommends nodejs
fi
# Populate ~/.config with current user
npm help > /dev/null


