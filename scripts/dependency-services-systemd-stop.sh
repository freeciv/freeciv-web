#!/bin/sh

# Use systemd to stop the services Freeciv-web depends on. Put this script
# in configuration.sh's DEPENDENCY_SERVICES_STOP variable to use it.

if systemctl is-active --quiet nginx.service ; then
  sudo systemctl stop nginx.service
fi

if systemctl is-active --quiet tomcat8.service ; then
  sudo systemctl stop tomcat8.service
fi
