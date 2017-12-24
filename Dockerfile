# Freeciv-web docker file 
# Dockerfile update based on debian/tomcat package

FROM tomcat:8.0-jre8

MAINTAINER The Freeciv Project version: 2.5

## Create relevant users and ensure nopasswd questions during scripts

RUN useradd -ms /bin/bash freeciv
RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

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

## Install relevant packages

RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get -y upgrade && apt-get install -y locales locales-all
ENV LC_ALL en_us.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y maven mysql-server openjdk-8-jdk  libcurl4-openssl-dev nginx libjansson-dev subversion pngcrush python3-pillow libtool automake autoconf autotools-dev python3.5-dev python3-setuptools libbz2-dev imagemagick python3-pip dos2unix liblzma-dev xvfb libicu-dev pkg-config zlib1g-dev wget curl libsdl1.2-dev ca-certificates-java libsqlite3-dev sudo git python-dev webp acl python3-mysql.connector libmagickwand-dev nano htop

## Prepare mysql, tomcat and nginx

RUN chmod +x /docker/scripts/docker-prep_serv.sh
RUN /bin/bash /docker/scripts/docker-prep_serv.sh

EXPOSE 80 8080 4002 6000 6001 6002 7000 7001 7002

USER docker

WORKDIR /docker/scripts/

RUN sudo ./docker-build.sh 

CMD ["/bin/bash"]
