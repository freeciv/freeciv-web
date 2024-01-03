#!/usr/bin/env bash

# Use systemd to start the services Freeciv-web depends on.
# Need to start the dependency services in a different way to work with
# your set up? Create a script that starts them and put it in
# scripts/dependency-services-start.sh.

if command -v pkexec > /dev/null; then
  ACCESS_MANAGER=
else
  ACCESS_MANAGER=sudo
fi

${ACCESS_MANAGER} systemctl reload nginx.service || ${ACCESS_MANAGER} systemctl start nginx.service

systemctl is-active --quiet mariadb.service || systemctl is-active --quiet mysql.service ||
    ${ACCESS_MANAGER} systemctl start mariadb.service ||
    ${ACCESS_MANAGER} systemctl start mysql.service

for unit in tomcat10; do
  systemctl is-active --quiet ${unit}.service || ${ACCESS_MANAGER} systemctl start ${unit}.service
done
