THE FREECIV-WEB PROJECT
-----------------------
[![Build Status](https://api.travis-ci.org/freeciv/freeciv-web.png)](https://travis-ci.org/freeciv/freeciv-web)

Freeciv-web is an open-source turn-based strategy game. It can be played in any HTML5 capable web-browser and features in-depth game-play and a wide variety of game modes and options. Your goal is to build cities, collect resources, organize your government, and build an army, with the ultimate goal of creating the best civilization. You can play online against other players (multiplayer) or play by yourself against the computer. 

Freeciv-web is is free and open source software. The Freeciv C server is released under the GNU General Public License, while the Freeciv-web client is released
under the GNU Affero General Public License. See LICENSE.txt for details.

Freeciv-web is now playable online at http://play.freeciv.org/

Contact: The Freeciv Project - freeciv-dev@gna.org  
https://mail.gna.org/listinfo/freeciv-dev  
  

![Freeciv-web](https://raw.githubusercontent.com/freeciv/freeciv-web/master/scripts/freeciv-web-screenshot.png "Freeciv-web screenshot")


Overview
--------

Freeciv-Web consists of these components:

* [Freeciv-web](freeciv-web) - a Java web application for the Freeciv-web client.
  This application is a Java web application which make up the application
  viewed in each user's web browser. The Metaserver is also a part of this module.
  Implemented in Javascript, Java, JSP, PHP, HTML and CSS. Built with maven and runs 
  on the Resin application server.

* [Freeciv](freeciv) - the Freeciv C server, which is checked out from the official
  svn repository, and patched to work with a WebSocket/JSON protocol. Implemented in C.

* [Freeciv-proxy](freeciv-proxy) - a WebSocket proxy which allows WebSocket clients in Freeciv-web
  to send socket requests to Freeciv servers. WebSocket requests are sent from Javascript 
  in Freeciv-web to nginx, which then proxies the WebSocket messages to freeciv-proxy, 
  which finally sends Freeciv socket requests to the Freeciv servers. Implemented in Python.

* [Publite2](publite2) - a process launcher for Freeciv C servers, which manages
  multiple Freeciv server processes and checks capacity through the Metaserver. 
  Implemented in Python.


Running Freeciv-web with Vagrant on VirtualBox
----------------------------------------------
Freeciv-web can be setup using Vagrant on VirtualBox to quickly create a 
local developer image running Freeciv-web on Ubuntu 14.04 on your host 
operating system such as Windows, OSX or Linux. 

1. Install VirtualBox: https://www.virtualbox.org/
2. Install Vagrant: http://www.vagrantup.com/

3. Install Git: http://git-scm.com/ then check out Freeciv-web from git to 
a directory on your computer, by running this git command:
 ```bash
  git clone https://github.com/freeciv/freeciv-web.git --depth=10
 ```

4. Run Vagrant with the following commands in your Freeciv-web directory from step 3:
 ```bash
 vagrant plugin install vagrant-vbguest
 vagrant up
 ```

  This will build, compile, install and run Freeciv-web on the virtual server image. Wait for the installation process to complete, watching for any error messages in the logs.

5. (Skip this step if you run Windows) For Linux or OS X, then you 
need to setup a SSH tunnel to port 80 like this:
 ```bash
 sudo ssh -p 2222 -gNfL 80:localhost:80 vagrant@localhost -i ~/.vagrant.d/insecure_private_key
 ```

Then test Freeciv-web by pointing your browser to http://localhost/ on your
host operating system.

To log in to your Vagrant server, run the command: 
 ```bash
 vagrant ssh
 ```

The Vagrant guest machine will mount the Freeciv-web source repository in the /vagrant directory.
Note that running Freeciv-web using Vagrant requires about 4Gb of memory
and 3 Gb of harddisk space.


System Requirements for manual install
--------------------------------------

Install this software if you are not running Freeciv-web with Vagrant:

- Resin 4.0.x - http://www.caucho.com/

- Java 7 JDK - http://www.oracle.com/technetwork/java/javase/downloads/ 

- Python 3.4 - http://www.python.org/

- Pillow v2.3.0 (PIL fork) - http://pillow.readthedocs.org/
  (required for freeciv-img-extract)

- Mysql 5.5.x - http://www.mysql.com/

- Maven 3 - http://maven.apache.org/download.html

- Firebug for debugging - http://getfirebug.com/

- curl-7.19.7 - http://curl.haxx.se/

- OpenSSL - http://www.openssl.org/

- nginx 1.6.0 or later - http://nginx.org/

- pngcrush, required for freeciv-img-extract.  http://pmt.sourceforge.net/pngcrush/

- Tornado 4.x - http://www.tornadoweb.org/

- GLib 2.16.0 or newer - http://ftp.gnome.org/pub/GNOME/sources/glib/2.16/

- Jansson 2.6 - http://www.digip.org/jansson/


Compiling and running the Freeciv Web client (old method)
---------------------------------------------------------
When not using Vagrant, or with other Linux distributions, follow this 
manual installation procedure to setup Freeciv-web:

0. Checkout Freeciv-web from github to ~/freeciv-build

1. Install the system requirements from the previous chapter.

2. Create mysql database called 'freeciv_web'
   Import mysql tables into a MySQL database from:
    freeciv-web/src/main/webapp/meta/private/metaserver.sql

3. Get, prepare, and build freeciv server:
   > cd freeciv
   > ./prepare_freeciv.sh

4. Install Freeciv. This involves running the following commands:
   > cd freeciv/freeciv  
   > make install     (as root user)  

   copy freeciv-web/freeciv/data/fcweb to /usr/local/share/freeciv 

5. Build and configure freeciv-web. 

   - Run setup_links.sh and sync.sh in the scripts/freeciv-img-extract directory.
   - Run sync-js-hand.sh in the scripts/ directory.
   - Copy freeciv-web/src/main/webapp/WEB-INF/resin-web.xml.dist as resin-web.xml
     and update the values for your MySQL database.
   - Copy freeciv-web/src/main/webapp/meta/php_code/local.php.dist as local.php
     and edit to suit your needs.
   - Run 'build.sh' in the freeciv-web directory
   - Copy nginx configuration file from publite2/nginx/freeciv-web
     to /etc/nginx/sites-enabled/ and edit to suit your needs.


6.  Start and stop Freeciv-web with the following commands:  
  scripts/start-freeciv-web.sh  
  scripts/stop-freeciv-web.sh  
  scripts/status-freeciv-web.sh  
(On Vagrant, these scripts are found on /vagrant/scripts)  

Freeciv-Web continuous integration on Travis CI 
-----------------------------------------------
Freeciv-Web is built on Travis CI on every commit. This is the current build status: [![Build Status](https://api.travis-ci.org/freeciv/freeciv-web.png)](https://travis-ci.org/freeciv/freeciv-web)


Developers interested in Freeciv-web
------------------------------------

If you want to contibute to Freeciv-web, see the [TODO file](TODO) for 
some tasks you can work on.


Contributors to Freeciv-web
---------------------------
[Andreas Røsdal](http://github.com/andreasrosdal)  [@andreasrosdal](http://www.twitter.com/andreasrosdal)  
[Marko Lindqvist](https://github.com/cazfi)  
and the [Freeciv.org project](http://freeciv.wikia.com/wiki/People)!

