# Freeciv-web docker file
# Dockerfile update based on debian/tomcat package

FROM debian:stretch

MAINTAINER The Freeciv Project version: 2.5

RUN DEBIAN_FRONTEND=noninteractive apt-get update --yes --quiet && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes \
        sudo \
        lsb-release \
        locales && \
    DEBIAN_FRONTEND=noninteractive apt-get clean --yes && \
    rm --recursive --force /var/lib/apt/lists/*

RUN DEBIAN_FRONTEND=noninteractive locale-gen en_US.UTF-8 && \
    localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

## Create user and ensure no passwd questions during scripts
RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo && \
    echo "docker ALL = (root) NOPASSWD: ALL\n" > /etc/sudoers.d/docker && \
    chmod 0440 /etc/sudoers.d/docker

## Add relevant content - to be pruned in the future
COPY .git /docker/.git
COPY freeciv /docker/freeciv
COPY freeciv-earth /docker/freeciv-earth
COPY freeciv-proxy /docker/freeciv-proxy
COPY freeciv-web /docker/freeciv-web
COPY pbem /docker/pbem
COPY publite2 /docker/publite2
COPY LICENSE.txt /docker/LICENSE.txt

COPY scripts /docker/scripts
COPY tests /docker/tests
COPY music /docker/music
COPY blender /docker/blender
COPY nginx /docker/nginx
COPY config /docker/config

RUN chown -R docker:docker /docker

USER docker

WORKDIR /docker/scripts/

RUN DEBIAN_FRONTEND=noninteractive sudo apt-get update --yes --quiet && \
    DEBIAN_FRONTEND=noninteractive install/install.sh --mode=TEST && \
    DEBIAN_FRONTEND=noninteractive sudo apt-get clean --yes && \
    sudo rm --recursive --force /var/lib/apt/lists/*

COPY docker-entrypoint.sh /docker/docker-entrypoint.sh

EXPOSE 80 8080 4002 6000 6001 6002 7000 7001 7002

ENTRYPOINT ["/docker/docker-entrypoint.sh"]

CMD ["/bin/bash"]
