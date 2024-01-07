#!/usr/bin/env bash

# Use systemd to stop the services Freeciv-web depends on.
# Need to start the dependency services in a different way to work with
# your set up? Create a script that starts them and put it in
# scripts/dependency-services-stop.sh.

if command -v pkexec > /dev/null; then
  ACCESS_MANAGER=
else
  ACCESS_MANAGER=sudo
fi

systemctl is-active --quiet nginx.service && ${ACCESS_MANAGER} systemctl reload nginx.service
if [ "${TOMCATMANAGER}" != "Y" ]; then
  systemctl is-active --quiet tomcat10.service && ${ACCESS_MANAGER} systemctl stop tomcat10.service
fi
