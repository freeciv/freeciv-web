#!/bin/sh

# Start Freeciv-web's dependency services.
#
# Need to start the dependency services in a different way to work with
# your set up? Create a script that starts them and put it in
# configuration.sh's DEPENDENCY_SERVICES_START variable.


export JAVA_OPTS="-Djava.security.egd=file:/dev/urandom"
export CATALINA_HOME=/var/lib/tomcat8

# 1. nginx
echo "Starting nginx first."

if [ "$(pidof nginx)" ]
then
  echo "nginx already running!"
else
  echo "Please enter root password:"
  sudo service nginx start && \
  echo "nginx started!" && \
  sleep 1
fi

if  type php7.0 > /dev/null; then
  echo "Starting php7.0-fpm"
  if [ "$(pidof php7.0-fpm)" ]
  then
    echo "php7.0-fpm already running!"
  else
    echo "starting php7.0-fpm"
    sudo service php7.0-fpm start
  fi
else
  echo "Starting php5.0-fpm"
  if [ "$(pidof php5-fpm)" ]
  then
    echo "php5.0-fpm already running!"
  else
    echo "starting php5-fpm"
    sudo service php5-fpm start
  fi
fi


# 2. Tomcat
echo "Starting up Tomcat" && \
if service --status-all | grep -Fq 'tomcat8'; then
 sudo service tomcat8 start || echo "unable to start tomcat8 service"
else
  /var/lib/tomcat8/bin/catalina.sh start

fi

# waiting for Tomcat to start, since it will take some time.
until `curl --output /dev/null --silent --head --fail "http://localhost:8080/"`; do
    printf ".."
    sleep 3
done
sleep 8
