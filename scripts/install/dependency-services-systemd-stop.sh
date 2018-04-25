#!/bin/sh

# Use systemd to stop the services Freeciv-web depends on.
# Need to start the dependency services in a different way to work with
# your set up? Create a script that starts them and put it in
# scripts/dependency-services-stop.sh.

if systemctl is-active --quiet nginx.service ; then
  sudo systemctl stop nginx.service
fi

if systemctl is-active --quiet tomcat8.service ; then
  sudo systemctl stop tomcat8.service
fi
