FROM python:3

# Install Freeciv server dependencies
RUN apt-get update && apt-get install -y \
  git \
  sudo \
  autoconf automake autotools-dev \
  libtool \
  libsqlite3-dev \
  libjansson-dev \
  libicu-dev
  
RUN useradd freeciv

WORKDIR /usr/src/freeciv

# Fetch Freeciv
RUN git clone git://repo.or.cz/freeciv.git --depth=1
# ADD http://files.freeciv.org/stable/freeciv-2.5.7.tar.bz2 freeciv

# Prepare Freeciv for Freeciv Web
COPY . .
RUN . ./version.txt
RUN ./apply_patches.sh


# Build Freeciv
WORKDIR /usr/src/freeciv/freeciv
RUN ./autogen.sh CFLAGS="-O3" --with-project-definition=../freeciv-web.project --enable-fcweb --enable-json --disable-delta-protocol --disable-nls --disable-fcmp --enable-freeciv-manual=html --disable-ruledit --enable-fcdb=no --enable-ai-static=classic,threaded --prefix=/usr/local && make -s -j$(nproc)
RUN make install

# Freeciv Server
EXPOSE 6556
# Metaserver
EXPOSE 8080
# Start Freeciv server
CMD ["sudo", "-u", "freeciv", "freeciv-web"]
