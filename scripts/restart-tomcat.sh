#!/bin/bash

export JAVA_OPTS="-Djava.security.egd=file:/dev/urandom"
export CATALINA_HOME=/var/lib/tomcat8
export JAVA_HOME=/opt/jdk/jdk1.8.0_131/

/var/lib/tomcat8/bin/catalina.sh stop
sleep 2
killall java
/var/lib/tomcat8/bin/catalina.sh start

