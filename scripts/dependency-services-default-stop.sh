#!/bin/bash

# Shutdown Freeciv-web's dependency services.
#
# Need to stop the dependency services in a different way to work with
# your set up? Create a script that stops them and put it in
# configuration.sh's DEPENDENCY_SERVICES_STOP variable.

# 1. nginx
if [ "$(pidof nginx)" ] ; then
  sudo killall nginx
fi

# 1.1 PHP7-FPM
sudo service php7-fpm stop || sudo service php5-fpm stop || echo "unable to stop php"

# 2. Tomcat
if service --status-all | grep -Fq 'tomcat8'; then
 sudo service tomcat8 stop || echo "unable to stop tomcat8 service"
else
 /var/lib/tomcat8/bin/catalina.sh stop

fi

# 2. Resin
#${FREECIV_WEB_DIR}/resin/bin/resin.sh stop
