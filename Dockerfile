# Use phusion/baseimage as base image. To make your builds reproducible, make
# sure you lock down to a specific version, not to `latest`!
# See https://github.com/phusion/baseimage-docker/blob/master/Changelog.md for
# a list of version numbers.
FROM phusion/baseimage:latest

# Use baseimage-docker's init system.
CMD ["/sbin/my_init"]

# Install the base packages and update Ubuntu
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get update && apt-get -y upgrade && apt-get install -y sudo lsb-release

# Get the basic dependencies, like Java 8, Python 3, and everything we need to compile freeciv
ADD production/build-apt-get.sh /docker/production/built-apt-get.sh
RUN /docker/production/built-apt-get.sh

# Files for tomcat and other dependencies
# Warning: These values cannot yet be modified
# -- Modifiable environment ends --
ENV TOMCAT_HOME=/var/lib/tomcat8
ENV CATALINA_HOME=${TOMCAT_HOME}
# -- Modifiable environment starts --
ADD production/build-tomcat.sh /docker/production/build-tomcat.sh
RUN /docker/production/build-tomcat.sh

# Base files necessary for freeciv
ADD LICENSE.txt /docker/LICENSE.txt
ADD tests /docker/tests
ADD freeciv /docker/freeciv
# -- Modifiable environment ends --
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US.UTF-8
# -- Modifiable environment starts --

ADD production/build-freeciv.sh /docker/production/build-freeciv.sh
RUN /docker/production/build-freeciv.sh

# Clean up APT when done (no other apt-based dependencies from here on out)
RUN sudo apt-get clean && sudo rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Install freeciv-web, which requires all these environment variables
# Warning: These values cannot yet be modified
# -- Modifiable environment ends --
ENV PUBLITE2_PORT=6000
ENV FREECIV_PROXY_PORT=8002
# TODO: This has to be retrieved in the scorelog_names.patch
ENV FREECIV_WEB_SCORE_LOGS_DIRECTORY=${TOMCAT_HOME}/webapps/data/scorelogs/
ENV FREECIV_WEB_SAVE_GAME_DIRECTORY=${TOMCAT_HOME}/webapps/data/savegames/
ENV FREECIV_WEB_RANK_LOGS_DIRECTORY=${TOMCAT_HOME}/webapps/data/ranklogs/
ENV FREECIV_WEB_DIR=/docker
ENV FREECIV_DATA_PATH=/root/freeciv/share/freeciv/
# -- Modifiable environment starts --

# Build freeciv web
ADD music /docker/music
ADD blender /docker/blender
ADD scripts /docker/scripts
ADD freeciv-web/src/derived /docker/freeciv-web/src/derived
WORKDIR /docker/scripts
ADD production/build-freeciv-web-sync.sh /docker/production/build-freeciv-web-sync.sh
RUN /docker/production/build-freeciv-web-sync.sh
ADD freeciv-web/src/main /docker/freeciv-web/src/main
ADD freeciv-web/build.sh /docker/freeciv-web/build.sh
ADD freeciv-web/pom.xml /docker/freeciv-web/pom.xml
ADD production/build-freeciv-web.sh /docker/production/build-freeciv-web.sh
RUN /docker/production/build-freeciv-web.sh

# Configure nginx
ADD nginx/conf.d/freeciv-web.conf /etc/nginx/conf.d/freeciv-web.conf
ADD nginx/sites-available/freeciv-web /etc/nginx/sites-available/freeciv-web
ADD production/build-nginx.sh /docker/production/build-nginx.sh
RUN /docker/production/build-nginx.sh

# Add the python services
ADD pbem /docker/pbem
ADD freeciv-earth /docker/freeciv-earth
ADD freeciv-proxy /docker/freeciv-proxy
ADD meta-stats /docker/meta-stats
ADD publite2 /docker/publite2

# Add the service runsv scripts for phusion
ADD production/freeciv-earth /etc/service/freeciv-earth
ADD production/meta-stats /etc/service/meta-stats
ADD production/nginx /etc/service/nginx
ADD production/pbem /etc/service/pbem
ADD production/publite2 /etc/service/publite2
ADD production/tomcat8 /etc/service/tomcat8

# Default environment variables (the full scope of the production configuration occurs here)
# The connection URL to the MySQL database.
ENV MYSQL_URL=mysql://localhost:3306/freeciv_web?useSSL=false
# The user with which to connect to MySQL
ENV MYSQL_USER=localuser
# The password with which to connect to MySQL
ENV MYSQL_PASSWORD=localpassword
# The public-facing URL of this Freeciv-Web host. For example, if you plan on accessing Freeciv-Web at
# http://web.freeciv.org/ set this to http://web.freeciv.org/
ENV FREECIV_WEB_ROOT_URL=http://localhost
# A Google reCAPTCHA secret key. Follow the instructions at https://developers.google.com/recaptcha/intro to sign up
# for the appropriate service account and use the secret key you receive here
ENV FREECIV_WEB_CAPTCHA_SECRET=empty
# A Mailgun mailing service username. Create a Mailgun account and then create SMTP credentials using the instructions
# from https://help.mailgun.com/hc/en-us/articles/203409084-How-do-I-create-additional-SMTP-credentials-
ENV FREECIV_WEB_MAILGUN_EMAIL_USERNAME=empty
# The Mailgun mailing service password you created following the instructions for the FREECIV_WEB_MAILGUN_EMAIL_USERNAME
ENV FREECIV_WEB_MAILGUN_EMAIL_PASSWORD=empty
# A Google Analytics property ID, typically starting with UA or GA.
ENV FREECIV_WEB_GOOGLE_ANALYTICS_UA_ID=empty
# A Google Sign In client ID. Follow the instructions at https://developers.google.com/identity/sign-in/web/sign-in#specify_your_apps_client_id
# to create a Google platform project and enable Google Sign In. Find the client ID and paste it here.
ENV FREECIV_WEB_GOOGLE_SIGN_IN_CLIENT_KEY=empty
# A token from TrackJS
ENV FREECIV_WEB_TRACK_JS_TOKEN=empty
# Indicates whether this server should start long turn by default
ENV FREECIV_WEB_START_LONGTURN=True
# Space-separated list of ports that will be exposed to host Freeciv long turn games
ENV FREECIV_WEB_LONGTURN_SERVER_PORTS 6003 6004 6005 6006 6007
# The maximum number of single player games this server should keep warm
ENV FREECIV_WEB_SERVER_CAPACITY_SINGLE=2
# The maximum number of multiplayer games this server should keep warm
ENV FREECIV_WEB_SERVER_CAPACITY_MULTI=1
# The maximum number of PBEM games this server should keep warm
ENV FREECIV_WEB_SERVER_CAPACITY_PBEM=1
# The number of seconds to wait until a zero-player-connected game is closed (abandoned due to idling)
ENV FREECIV_WEB_SERVER_IDLE=20
# The maximum number of games to run
ENV FREECIV_WEB_SERVER_LIMIT=999

# Runtime prefix (in case you want to run services locally, this can be changed
ENV FREECIV_RUNTIME_PREFIX=/docker

EXPOSE 80
