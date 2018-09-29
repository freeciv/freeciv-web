Freeciv web application 
=======================

This is the Freeciv web application, which consists of the Java servlets 
and filters for running the web client, JSP templates, javascript code
and other web content. 

Derived Sources
===============

Freeciv-web uses packet definitions, tilesets, and other resources
derived from the original freeciv project, keeping these up to date by
generating them during install from freeciv source.

Scripts to generate these files are in `$freeciv-web/scripts` and they
are generated to `$freeciv-web/freeciv-web/src/derived`. See the
README.md in those directories for more info.

Tomcat 8 + nginx setup
================================
Freeciv-web supports the Tomcat 8 application server for hosting the Java web application.

The build scripts are updated to build Freeciv-web for Tomcat by default,
so setting up Freeciv-web with Vagrant will configure this automatically.
Also see the suggested nginx.conf file in publite2/nginx.conf

  https://tomcat.apache.org/  
  http://nginx.org/  

Build script
============
Use maven to build and deploy, by running this build script: 
sh build.sh

There is also a build-js.sh script to build just JavaScript quickly for development work.

The build script will also create a data webapp directory where savegames and scorelogs are stored.

Flyway migrations of the database is supported. Remember to set the mysql password in flyway.properties.dist and rename the file to flyway.properties.
To migrate the database to the latest version, run this Maven command:
mvn flyway:migrate

The following files contains MySQL username and password, and must be set manually
if you are not using vagrant:

* freeciv-web/src/main/webapp/META-INF/context.xml.dist  (rename to context.xml)
* freeciv-web/src/main/webapp/WEB-INF/config.properties.dist  (rename to config.properties, contains capcha secret for pbem)


Copyright (C) 2007-2016 Andreas Røsdal. 
Released under the GNU AFFERO GENERAL PUBLIC LICENSE.

