THE FREECIV-WEB PROJECT
-----------------------
[![Build Status](https://api.travis-ci.org/freeciv/freeciv-web.png)](https://travis-ci.org/freeciv/freeciv-web)

Freeciv-web is an open-source turn-based strategy game. It’s built entirely in HTML5 and features in-depth game-play and a wide variety of game modes and options. Your goal is to build cities, collect resources, organize your government, and build an army, with the ultimate goal of creating the best civilization. You can play online against other players (multiplayer) or play by yourself against the computer. 

Freeciv-web is is free and open source software. The Freeciv C server is released under the GNU General Public License, while the Freeciv-web client is released
under the GNU Affero General Public License. See LICENSE.txt for details.

Freeciv-web is now playable online at http://play.freeciv.org/

Contact: The Freeciv Project - freeciv-dev@gna.org
https://mail.gna.org/listinfo/freeciv-dev


![Freeciv-web](http://upload.wikimedia.org/wikipedia/en/thumb/e/ee/Freeciv-net-screenshot-2011-06-23.png/800px-Freeciv-net-screenshot-2011-06-23.png "Freeciv-web screenshot")


Overview
--------

The Freeciv Web client consists of these components:

* freeciv - a fork of the main Freeciv C client and server.

* freeciv-proxy - a HTTP proxy which allows web client users
  to connect to Freeciv civservers. The proxy is a HTTP and WebSocket
  server, and proxy between web browsers and civservers.

* freeciv-web - a Java web application for the Freeciv web client.
  This application is a Java web application which consists of HTML,
  Javascript, images and JSP files which make up the application
  viewed in each user's web browser. 

* freeciv-img-extract - extracts the images of the Freeciv tileset,
  and generates a version for the web. This produces a tileset PNG 
  and Javascript. Run this to sync with the upstream Freeciv SVN 
  repository.

* publite2 - a simple way to launch multiple civservers. 


Running Freeciv-web with Vagrant on VirtualBox
----------------------------------------------
Freeciv-web can be setup using Vagrant on VirtualBox to quickly create a 
local developer image running Freeciv-web on Ubuntu 13.10 on your host 
operating system such as Windows, OSX or Linux. 

1. Install VirtualBox: https://www.virtualbox.org/
2. Install Vagrant: http://www.vagrantup.com/

3. Install Git: http://git-scm.com/ then check out Freeciv-web from git to 
a directory on your computer, by running these two git commands:
 ```bash
 git config --global core.autocrlf false
 git clone https://github.com/freeciv/freeciv-web.git
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

Note that running Freeciv-web using Vagrant requires about 4Gb of memory
and 3 Gb of harddisk space.


System Requirements for manual install
--------------------------------------

Install this software if you are not running Freeciv-web with Vagrant:

- Resin 4.0.38 - http://www.caucho.com/

- Java 7 JDK - http://www.oracle.com/technetwork/java/javase/downloads/ 

- Python 3.3 - http://www.python.org/

- Pillow v2.3.0 (PIL fork) - http://pillow.readthedocs.org/
  (required for freeciv-img-extract)

- Mysql 5.5.x - http://www.mysql.com/

- Maven 2 - http://maven.apache.org/download.html

- Firebug for debugging - http://getfirebug.com/

- Closure Compiler - http://code.google.com/intl/nb/closure/compiler/
  (Bundled in source code, no need to download.)

- curl-7.19.7 - http://curl.haxx.se/

- OpenSSL - http://www.openssl.org/

- nginx 1.5.10 - http://nginx.org/

- pngcrush, required for freeciv-img-extract.  http://pmt.sourceforge.net/pngcrush/

- Tornado 3.2 - http://www.tornadoweb.org/

- GLib 2.16.0 or newer - http://ftp.gnome.org/pub/GNOME/sources/glib/2.16/

- Jansson 2.5 - http://www.digip.org/jansson/


Compiling and running the Freeciv Web client (old method)
---------------------------------------------------------
When not using Vagrant, or with other Linux distributions, follow this 
manual installation procedure to setup Freeciv-web:

0. Checkout Freeciv-web from github to ~/freeciv-build

1. Install the system requirements.

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

   - Run setup_links.sh and sync.sh in freeciv-img-extract 
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

Freeciv-Web continuous integration on Travis CI 
-----------------------------------------------
Freeciv-Web is built on Travis CI on every commit. This is the current build status: [![Build Status](https://api.travis-ci.org/freeciv/freeciv-web.png)](https://travis-ci.org/freeciv/freeciv-web)


Developers interested in Freeciv-web
------------------------------------

If you want to contibute to Freeciv-web, see the TODO file for 
some tasks you can work on.


Contributors to Freeciv-web
---------------------------
Andreas Røsdal  
Marko Lindqvist  
and the Freeciv.org project!

