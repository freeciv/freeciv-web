# Freeciv-web docker file
# Dockerfile update based on debian/tomcat package

FROM debian:stretch

MAINTAINER The Freeciv Project version: 2.5

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
ADD nginx /docker/nginx

## Install relevant packages

RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get -y upgrade && apt-get install -y sudo lsb-release

EXPOSE 80 8080 4002 6000 6001 6002 7000 7001 7002

## Create user and ensure no passwd questions during scripts

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo
RUN echo "docker ALL = (root) NOPASSWD: ALL" >> /etc/sudoers.d/docker
RUN chmod 0440 /etc/sudoers.d/docker

RUN chown -R docker:docker /docker

USER docker
ENV LC_ALL C.UTF-8

WORKDIR /docker/scripts/

RUN install/install.sh
RUN sudo rm /etc/sudoers.d/docker

CMD ["/bin/bash"]
