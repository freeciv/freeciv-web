# Freeciv-web docker file 
# FIXME! Freeciv-web does not work with Docker at the moment. This file needs fixing.

FROM ubuntu:15.10

MAINTAINER The Freeciv Project version: 2.5

RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get -y upgrade && apt-get install -y maven mysql-server openjdk-8-jdk libcurl4-openssl-dev nginx libjansson-dev subversion pngcrush python3-pillow libtool automake autoconf autotools-dev language-pack-en python3.5-dev python3-setuptools libbz2-dev imagemagick python3-pip dos2unix liblzma-dev firefox xvfb libicu-dev pkg-config zlib1g-dev wget curl libsdl1.2-dev sudo ca-certificates-java tomcat8 tomcat8-admin 

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en

RUN useradd -ms /bin/bash freeciv

ADD freeciv /docker/freeciv
ADD freeciv-proxy /docker/freeciv-proxy
ADD freeciv-web /docker/freeciv-web
ADD publite2 /docker/publite2
ADD scripts /docker/scripts
ADD tests /docker/tests
ADD LICENSE.txt /docker/LICENSE.txt

RUN mkdir -p /data /var/lib/mysql
RUN chmod -R 777 /data /var/lib/mysql
RUN chown -R root:root /data /var/lib/mysql

RUN sudo /docker/scripts/docker-build.sh 


EXPOSE 80

