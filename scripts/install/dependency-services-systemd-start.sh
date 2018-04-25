#!/bin/sh

# Use systemd to start the services Freeciv-web depends on.
# Need to start the dependency services in a different way to work with
# your set up? Create a script that starts them and put it in
# scripts/dependency-services-start.sh.

# Just in case it didn't start at boot time
systemctl is-active --quiet mysql.service || sudo systemctl start mysql.service

# Is stopped and (re)started with Freeciv-web
systemctl is-active --quiet nginx.service || sudo systemctl start nginx.service
systemctl is-active --quiet tomcat8.service || sudo systemctl start tomcat8.service
