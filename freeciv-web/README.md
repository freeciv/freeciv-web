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

Tomcat 10 + nginx setup
================================
Freeciv-web supports the Tomcat 10 application server for hosting the Java web application.

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

Flyway migrations of the database are supported. Remember to set the mysql password in flyway.properties.dist, rename the file, and pass the new filename to Maven.
To migrate the database to the latest version, run this Maven command:
mvn -Dflyway.configFiles=<config-file-name> flyway:migrate

The following files contain DB, mail and other configuration, and must be set manually
if you are not using vagrant or the install script:

* freeciv-web/src/main/webapp/META-INF/context.xml  (you can use config/web.context.tmpl as a template)
* freeciv-web/src/main/webapp/WEB-INF/config.properties  (you can use config/web.properties.tmpl as a template)


Copyright (C) 2007-2016 Andreas Røsdal.
Released under the GNU AFFERO GENERAL PUBLIC LICENSE.
