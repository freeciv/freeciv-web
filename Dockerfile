# Freeciv-web docker file 
FROM ubuntu:14.04

MAINTAINER The Freeciv Project version: 2.5

RUN apt-get update && apt-get -y upgrade && apt-get install -y maven mysql-server openjdk-7-jdk libcurl4-openssl-dev nginx libjansson-dev subversion pngcrush python3-pillow libtool automake autoconf autotools-dev language-pack-en python3.4-dev python3-setuptools libbz2-dev imagemagick python3-pip dos2unix liblzma-dev firefox xvfb libicu-dev pkg-config zlib1g-dev wget curl

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

RUN sudo /docker/scripts/docker-build.sh 


EXPOSE 80

