#!/bin/sh

# Start Freeciv-web's dependency services.
#
# Need to start the dependency services in a different way to work with
# your set up? Create a script that starts them and put it in
# scripts/dependency-services-start.sh.


export JAVA_OPTS="-Djava.security.egd=file:/dev/urandom"
export CATALINA_HOME=/var/lib/tomcat8

# 0. mysql
pidof mysqld > /dev/null || sudo service mysql start

# 1. nginx
echo "Starting nginx first."

if [ "$(pidof nginx)" ]
then
  echo "nginx already running!"
  sudo nginx -s reload
else
  echo "Please enter root password:"
  sudo service nginx start && \
  echo "nginx started!" && \
  sleep 1
fi

# 2. Tomcat
echo "Starting up Tomcat" && \
if service --status-all | grep -Fq 'tomcat8'; then
   sudo /usr/sbin/service tomcat8 start || echo "unable to start tomcat8 service"
else
   sudo -u tomcat8 $CATALINA_HOME/bin/catalina.sh start
fi

# waiting for Tomcat to start, since it will take some time.
until `curl --output /dev/null --silent --head --fail "http://localhost:8080/"`; do
    printf ".."
    sleep 3
done
sleep 8
