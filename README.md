THE FREECIV-WEB PROJECT
-----------------------

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![Build Status](https://github.com/freeciv/freeciv-web/workflows/continuous%20integration/badge.svg)](https://github.com/freeciv/freeciv-web/actions?query=workflow%3A%22continuous+integration%22)
[![Code Quality: Javascript](https://img.shields.io/lgtm/grade/javascript/g/freeciv/freeciv-web.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/freeciv/freeciv-web/context:javascript)
[![Total Alerts](https://img.shields.io/lgtm/alerts/g/freeciv/freeciv-web.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/freeciv/freeciv-web/alerts)

Freeciv-web is an open-source turn-based strategy game. It can be played in any HTML5 capable web-browser and features in-depth game-play and a wide variety of game modes and options. Your goal is to build cities, collect resources, organize your government, and build an army, with the ultimate goal of creating the best civilization. You can play online against other players (multiplayer) or play by yourself against the computer. There is both a HTML5 2D version with isometric graphics and a 3D WebGL version of Freeciv-web. 

Freeciv-web is free and open source software. The Freeciv C server is released under the GNU General Public License, while the Freeciv-web client is released
under the GNU Affero General Public License. See [License](LICENSE.txt) for the full license document.


Live servers
------------
Currently known servers based on Freeciv-web, which are open source in compliance with [the AGPL license](https://github.com/freeciv/freeciv-web/blob/develop/LICENSE.txt):
- [FCIV.NET](https://www.fciv.net) [https://github.com/fciv-net/fciv-net]
- [freecivweb.org](https://www.freecivweb.org) [https://github.com/Lexxie9952/fcw.org-server]
- [moving borders](https://fcw.movingborders.es)  [https://github.com/lonemadmax/freeciv-web] (Everything except longturn and real-Earth)
- [Freeciv Tactics & Triumph](https://www.tacticsandtriumph.com)  [https://github.com/Canik05/freeciv-tnt] Freeciv Games & Mods (No PBEM)

Freeciv-web screenshots:
------------------------
Freeciv WebGL 3D:
![Freeciv-web](https://raw.githubusercontent.com/freeciv/freeciv-web/develop/freeciv-web/src/main/webapp/javascript/webgl/freeciv-webgl.png "Freeciv-web WebGL screenshot")

Freeciv-web HTML5 version:
![Freeciv-web](https://raw.githubusercontent.com/freeciv/freeciv-web/develop/scripts/freeciv-web-screenshot.png "Freeciv-web screenshot")


Overview
--------

Freeciv-Web consists of these components:

* [Freeciv-web](freeciv-web) - a Java web application for the Freeciv-web client.
  This application is a Java web application which make up the application
  viewed in each user's web browser. The Metaserver is also a part of this module.
  Implemented in Javascript, Java, JSP, HTML and CSS. Built with maven and runs 
  on Tomcat 10 and nginx.

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
at [config.dist](config/config.dist),
copy it without the `.dist` extension and edit to your liking.

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

  This will build, compile, install and run Freeciv-web on the virtual server image. Wait for the installation process to complete, watching for any error messages in the logs. If you get an error message about Virtualization (VT) not working, then enable Virtualization in the BIOS.

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

- Tomcat 10 - https://tomcat.apache.org/

- Java 11 JDK - https://adoptopenjdk.net/ 

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

- Tornado 6.1 or later - http://www.tornadoweb.org/

- Jansson 2.6 - http://www.digip.org/jansson/

- liblzma-dev - http://tukaani.org/xz/ - for XZ compressed savegames.


When in a [tested system](scripts/install/systems),
you may run `scripts/install/install.sh` and it will fetch and configure what's needed.

Start and stop Freeciv-web with the following commands:  
  start-freeciv-web.sh  
  stop-freeciv-web.sh  
  status-freeciv-web.sh

All software components in Freeciv-web will log to the /logs sub-directory of the Freeciv-web installation.


### Running Freeciv-web on Docker

Freeciv-web can easily be built and run from Docker using `docker-compose`.

 1. Make sure you have both [Docker](https://www.docker.com/get-started) and [Docker Compose](https://docs.docker.com/compose/install/) installed.

 2. Run the following from the freeciv-web directory:

    ```sh
    docker-compose up -d
    ```

 3. Connect to docker via host machine using standard browser

http://localhost:8080/

Enjoy. The overall dockerfile and required changes to scripts needs some further improvements.


Freeciv-Web continuous integration on GitHub actions
----------------------------------------------------
Freeciv-Web is built on GitHub actions on every commit. This is the current build status: [![Build Status](https://github.com/freeciv/freeciv-web/workflows/continuous%20integration/badge.svg)](https://github.com/freeciv/freeciv-web/actions?query=workflow%3A%22continuous+integration%22)


Developers interested in Freeciv-web
------------------------------------

If you want to contibute to Freeciv-web, see the [issues](https://github.com/freeciv/freeciv-web/issues) on GibHub and the [TODO file](doc/TODO) for 
some tasks you can work on. Pull requests on Github is welcome! 
  

Contributors to Freeciv-web
---------------------------
Andreas Røsdal  [@andreasrosdal](https://github.com/andreasrosdal)  
Marko Lindqvist [@cazfi](https://github.com/cazfi)  
Sveinung Kvilhaugsvik [@kvilhaugsvik](https://github.com/kvilhaugsvik)  
Gerik Bonaert [@adaxi](https://github.com/adaxi)  
Lmoureaux [@lmoureaux](https://github.com/lmoureaux)  
Máximo Castañeda [@lonemadmax](https://github.com/lonemadmax)  
and the [Freeciv.org project](https://www.freeciv.org/wiki/People)!
