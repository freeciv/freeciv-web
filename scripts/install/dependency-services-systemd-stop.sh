#!/bin/sh

# Use systemd to stop the services Freeciv-web depends on.
# Need to start the dependency services in a different way to work with
# your set up? Create a script that starts them and put it in
# scripts/dependency-services-stop.sh.

if which pkexec > /dev/null; then
  ACCESS_MANAGER=
else
  ACCESS_MANAGER=sudo
fi

for unit in nginx tomcat8; do
  systemctl is-active --quiet ${unit}.service && ${ACCESS_MANAGER} systemctl stop ${unit}.service
done

