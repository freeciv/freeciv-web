﻿THE FREECIV-WEB PROJECT
-----------------------

[![Build Status](https://api.travis-ci.org/freeciv/freeciv-web.png)](https://travis-ci.org/freeciv/freeciv-web)
[![Code Quality: Javascript](https://img.shields.io/lgtm/grade/javascript/g/freeciv/freeciv-web.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/freeciv/freeciv-web/context:javascript)
[![Total Alerts](https://img.shields.io/lgtm/alerts/g/freeciv/freeciv-web.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/freeciv/freeciv-web/alerts)

Quick Start
-----------

```
docker build . -t freeciv-web
docker-compose up 
```

Overview
--------

Freeciv-Web consists of these components:

* [Freeciv-web](freeciv-web) - a Java web application for the Freeciv-web client.
  This application is a Java web application which make up the application
  viewed in each user's web browser. The Metaserver is also a part of this module.
  Implemented in Javascript, Java, JSP, HTML and CSS. Built with maven and runs 
  on Tomcat 8 and nginx.

* [Freeciv](freeciv) - the Freeciv C server, which is checked out from the official
  Git repository, and patched to work with a WebSocket/JSON protocol. Implemented in C.

* [Freeciv-proxy](freeciv-proxy) - a WebSocket proxy which allows WebSocket clients in Freeciv-web
  to send socket requests to Freeciv servers. WebSocket requests are sent from Javascript 
  in Freeciv-web to nginx, which then proxies the WebSocket messages to freeciv-proxy, 
  which finally sends Freeciv socket requests to the Freeciv servers. Implemented in Python.

* [Publite2](publite2) - a process launcher for Freeciv C servers, which manages
  multiple Freeciv server processes and checks capacity through the Metaserver. 
  Implemented in Python.

* [pbem](pbem) is play-by-email support. 

* [freeciv-earth](freeciv-earth) is code to generate Freeciv savegames from a map captured from mapbox.

Freeciv WebGL
-------------
Freeciv WebGL is the 3D version, which uses the Three.js 3D engine. More info about the WebGL 3D version can be found for [developers](https://github.com/freeciv/freeciv-web/tree/develop/freeciv-web/src/main/webapp/javascript/webgl) and [3D artists](https://github.com/freeciv/freeciv-web/wiki/Contributing-Blender-models-for-Freeciv-WebGL).
Developer: Andreas Røsdal [@andreasrosdal](http://www.twitter.com/andreasrosdal)  

Running Freeciv-web on your computer
------------------------------------
The recommended and probably easiest way is to use Vagrant on VirtualBox.

Whatever the method you choose, you'll have to check out Freeciv-web to a
directory on your computer, by installing [Git](http://git-scm.com/) and
running this command:
 ```bash
  git clone https://github.com/freeciv/freeciv-web.git --depth=10
 ```

You may also want to change some parameters before installing, although
it's not needed in most cases. If you have special requirements, have a look
at [configuration.sh.dist](scripts/configuration.sh.dist)
and [WEB-INF/config.properties.dist](freeciv-web/src/main/webapp/WEB-INF/config.properties.dist),
copy them without the `.dist` extension and edit to your liking.
Don't change the `.dist` files, they are the ones from the repo.

#### :warning: Notice for Windows users

Please keep in mind that the files are to be used in a Unix-like system
(some Ubuntu version with the provided Vagrant file).
Line endings for text files are different in Windows, and some editors
"correct" them, making the files unusable in the VM.
There's some provision to recode the main configuration files when installing,
but not afterwards. If you touch shared files after installation, please use
an editor that respect Unix line endings or transform them with a utility
like dos2unix after saving them.

### Running Freeciv-web with Vagrant on VirtualBox

Freeciv-web can be setup using Vagrant on VirtualBox to quickly create a 
local developer image running Freeciv-web on latest Ubuntu on your host
operating system such as Windows, OSX or Linux. 
This is the recommended way to build Freeciv-web on your computer.

1. Install VirtualBox: https://www.virtualbox.org/ - Install manually on Windows, and with the following command on Linux:
 ```bash
sudo apt-get install virtualbox
 ```

2. Install Vagrant: http://www.vagrantup.com/ - Install manually on Windows
, and with the following command on Linux:
 ```bash
sudo apt-get install vagrant
 ```

3. Run Vagrant with the following commands in your Freeciv-web directory:
 ```bash
 vagrant up
 ```

  This will build, compile, install and run Freeciv-web on the virtual server image. Wait for the installation process to complete, watching for any error messages in the logs.

4. Test Freeciv-web by pointing your browser to http://localhost if you run Windows or http://localhost:8080 if you run Linux or macOS.

To log in to your Vagrant server, run the command: 
 ```bash
 vagrant ssh
 ```

The Vagrant guest machine will mount the Freeciv-web source repository in the /vagrant directory.
Note that running Freeciv-web using Vagrant requires about 4Gb of memory
and 3 Gb of harddisk space.

### System Requirements for manual install

Install this software if you are not running Freeciv-web with Vagrant:

- Tomcat 8 - https://tomcat.apache.org/ 

- Java 8 JDK - http://www.oracle.com/technetwork/java/javase/downloads/ 

- Python 3.6 - http://www.python.org/

- Pillow v2.3.0 (PIL fork) - http://pillow.readthedocs.org/
  (required for freeciv-img-extract)

- Mysql 5.5.x - http://www.mysql.com/

- Maven 3 - http://maven.apache.org/download.html

- Firebug for debugging - http://getfirebug.com/

- curl-7.19.7 - http://curl.haxx.se/

- OpenSSL - http://www.openssl.org/

- nginx 1.11.x or later - http://nginx.org/

- MySQL Connector/Python - https://github.com/mysql/mysql-connector-python

- pngcrush, required for freeciv-img-extract.  http://pmt.sourceforge.net/pngcrush/

- Tornado 4.4 or later - http://www.tornadoweb.org/

- Jansson 2.6 - http://www.digip.org/jansson/

- liblzma-dev - http://tukaani.org/xz/ - for XZ compressed savegames.

- cwebp to create .webp files of the tileset.


When in a [tested system](scripts/install/systems),
you may run `scripts/install/install.sh` and it will fetch and configure what's needed.

Start and stop Freeciv-web with the following commands:  
  start-freeciv-web.sh  
  stop-freeciv-web.sh  
  status-freeciv-web.sh

All software components in Freeciv-web will log to the /logs sub-directory of the Freeciv-web installation.

### Freeciv-Web alternative Docker image

1. Build local dockerfile based on debian - this will take a significant amount of time (10 - 20 minutes)

docker build . -t freeciv-web

2. Run docker image including required ports on your host

docker run -i -t --user docker -p 8888:8080 -p 80:80 -p 7000:7000 -p 7001:7001 -p 7002:7002 -p 6000:6000 -p 6001:6001 -p 6002:6002 freeciv-web

3. Start Freeciv-web with:

./start-freeciv-web.sh

Answer prompt for docker sudo password with "docker"

4. Connect to docker via host machine using standard browser

http://localhost/

Enjoy. The overall dockerfile and required changes to scripts needs some further improvements.

Freeciv-Web continuous integration on Travis CI 
-----------------------------------------------
Freeciv-Web is built on Travis CI on every commit. This is the current build status: [![Build Status](https://api.travis-ci.org/freeciv/freeciv-web.png)](https://travis-ci.org/freeciv/freeciv-web)

Freeciv-web has CasperJS tests which are run by Travis CI on every commit, and by Vagrant when creating a new image. The tests can be found in tests/freeciv-web-tests.js. Please make sure that patches and commits for Freeciv-web don't break the CasperJS tests. Thanks!

Developers interested in Freeciv-web
------------------------------------

If you want to contibute to Freeciv-web, see the [issues](https://github.com/freeciv/freeciv-web/issues) on GibHub and the [TODO file](TODO) for 
some tasks you can work on. Pull requests on Github is welcome! 
  

Contributors to Freeciv-web
---------------------------
Andreas Røsdal  [@andreasrosdal](http://www.twitter.com/andreasrosdal)  
Marko Lindqvist [@cazfi](https://github.com/cazfi)  
Sveinung Kvilhaugsvik [@kvilhaugsvik](https://github.com/kvilhaugsvik)  
Gerik Bonaert [@adaxi](https://github.com/adaxi)  
Lmoureaux [@lmoureaux](https://github.com/lmoureaux)  
Máximo Castañeda [@lonemadmax](https://github.com/lonemadmax)  
and the [Freeciv.org project](http://freeciv.wikia.com/wiki/People)!

