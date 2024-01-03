#!/usr/bin/env bash

export JAVA_OPTS="-Djava.security.egd=file:/dev/urandom"
export CATALINA_HOME=/var/lib/tomcat10
export JAVA_HOME=/opt/jdk/jdk1.8.0_131/

/var/lib/tomcat10/bin/catalina.sh stop
sleep 2
killall java
/var/lib/tomcat10/bin/catalina.sh start

