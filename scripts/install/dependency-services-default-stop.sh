#!/bin/bash

# Shutdown Freeciv-web's dependency services.
#
# Need to stop the dependency services in a different way to work with
# your set up? Create a script that stops them and put it in
# scripts/dependency-services-stop.sh.

# 1. nginx
if [ "$(pidof nginx)" ] ; then
  sudo killall nginx
fi

# 2. Tomcat
if [ -z "${TOMCATMANAGER_USER}" ]; then
  if service --status-all | grep -Fq 'tomcat8'; then
    sudo service tomcat8 stop || echo "unable to stop tomcat8 service"
  else
    sudo -u tomcat8 /var/lib/tomcat8/bin/catalina.sh stop
  fi
fi
