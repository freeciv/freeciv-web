# Freeciv-web docker file 
# Dockerfile update based on debian/tomcat package

FROM tomcat:8.0-jre8

MAINTAINER The Freeciv Project version: 2.5

## Install relevant packages

RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get -y upgrade && apt-get install -y locales locales-all
ENV LC_ALL en_us.UTF-8
ENV LANG en_US.UTF-8
ENV DEBIAN_FRONTEND=noninteractive
ENV LANGUAGE en_US:en

RUN apt-get update
RUN apt-get install -y maven mysql-server openjdk-8-jdk  libcurl4-openssl-dev nginx libjansson-dev subversion pngcrush python3-pillow libtool automake autoconf autotools-dev python3.5-dev python3-setuptools libbz2-dev imagemagick python3-pip dos2unix liblzma-dev xvfb libicu-dev pkg-config zlib1g-dev wget curl libsdl1.2-dev ca-certificates-java libsqlite3-dev sudo git python-dev webp acl python3-mysql.connector

## Create relevant users and ensure nopasswd questions during scripts

RUN useradd -ms /bin/bash freeciv
RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

RUN echo "docker ALL = (root) NOPASSWD: /docker/scripts/docker-build*.sh, /docker/scripts/start-freeciv-web.sh, /usr/sbin/service nginx *, /usr/sbin/service tomcat8 *, /usr/sbin/service *, /etc/init.d/tomcat8 *, /bin/chmod, /bin/ls, /bin/cat, /usr/bin/mysql *  /usr/share/tomcat8/bin/catalina.sh *" >> /etc/sudoers
RUN echo "freeciv ALL = (root) NOPASSWD: /docker/scripts/docker-build*.sh, /docker/scripts/start-freeciv-web.sh, /usr/sbin/service nginx *, /usr/sbin/service tomcat8 *, /usr/sbin/service *, /etc/init.d/tomcat8 *, /bin/chmod, /bin/ls, /bin/cat,  /usr/local/tomcat/bin/catalina.sh *" >> /etc/sudoers

## Add relevant content - to be pruned in the future

ADD .git /docker/.git
ADD freeciv /docker/freeciv
ADD freeciv-earth /docker/freeciv-earth
ADD freeciv-proxy /docker/freeciv-proxy
ADD freeciv-web /docker/freeciv-web
ADD pbem /docker/pbem
ADD publite2 /docker/publite2
ADD LICENSE.txt /docker/LICENSE.txt

ADD scripts /docker/scripts
ADD tests /docker/tests
ADD music /docker/music
ADD blender /docker/blender

## Prepare mysql, tomcat and nginx

RUN mkdir -p /data /var/lib/mysql
RUN chmod -R 777 /data /var/lib/mysql
RUN chown -R root:root /data /var/lib/mysql

ENV CATALINA_HOME /usr/local/tomcat
ENV PATH $PATH:$CATALINA_HOME/bin

RUN cp /docker/publite2/nginx.conf /etc/nginx/nginx.conf
RUN ln -s /usr/local/tomcat/ /var/lib/tomcat8
RUN mv /var/lib/tomcat8/webapps/ROOT/ /var/lib/tomcat8/webapps/root-old/

EXPOSE 80
EXPOSE 4002

EXPOSE 6000
EXPOSE 6001
EXPOSE 6002

EXPOSE 7000
EXPOSE 7001
EXPOSE 7002

EXPOSE 8080

## Prepare and conduct installation

RUN chmod +x /docker/scripts/docker-build.sh
RUN chmod +x /docker/scripts/init_mysql_db.sh
RUN /bin/bash /docker/scripts/init_mysql_db.sh

RUN apt-get install -y libmagickwand-dev
USER docker

WORKDIR /docker/scripts/

RUN sudo ./docker-build.sh 

#CMD ["./docker-run.sh"]
CMD ["/bin/bash"]
