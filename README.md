THE FREECIV-WEB PROJECT
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

Visit [your local host](http://localhost) to interact with Freeciv-Web. To shut down the server, you can either hit **Ctrl+C** in your terminal to end a foreground `docker-compose`, or issue this command in another terminal:

```
docker-compose down
```

Use these commands to test changes.

The `docker-compose.yml` project creates a MySQL server for you.

Configuration
-------------

Visit the [Dockerfile](Dockerfile) to see which environment variables can be set. The most important ones are located at the end of the Dockerfile.

You can specify these environment variables according to the `docker` documentation, or put them into your own `docker-compose.yml`. This includes environment variables like `MYSQL_URL`, `MYSQL_USERNAME` and `MYSQL_PASSWORD`.

Known Issues
------------

 - After 1,000 game starts, the server behaviour is probably undefined. It is recommended as a work around to restart the container near this limit.
 - Save games, score logs, rank logs, application logs, and changes to the long turn server configurations are not persisted. These files will be migrated to use volumes.
 - `freeciv:freeciv-web` containers are not aware of each other (they do not cluster). They cannot be load balanced at the moment.
 
Tips
----

 - Always test by running the docker commands at the top of this document. They ensure your changes will reproduce and work in the production environment.
 - You can use IntelliJ IDEA Community Edition to browse the entire project. Open another project window for just [freeciv-web](freeciv-web) to get a correctly-configured Maven project.
 - To see the git commit corresponding to the version of the freeciv server being used, visit [version.txt](freeciv/version.txt).
 - To get a valid freeciv source tree in your project, navigate to `freeciv` and execute:
   ```$xslt
   dl_freeciv_default.sh
   apply_patches.sh
   ```
 - To create a new patch, make the change to the files in freeciv that you specified. Then, for each file (for example, `server/animals.c`):
   ```
   git diff --patch server/animals.c > ../patches/animals.patch
   ```
   Then, modify [apply_patches.sh](freeciv/apply_patches.sh) `PATCHLIST` variable to include the base name (without the extension) of your patch.
 - To watch the server traffic between the web client and the servers, uncomment the `print('DEBUG ...` lines in [civcom.py](freeciv-proxy/civcom.py).
 - Changes to freeciv usually require rebuilding the entire project.
 - If you add additional, important root files to [freeciv-web](freeciv-web), make sure to include them in the Dockerfile.
 
Learning More
-------------

Visit the [Dockerfile](Dockerfile) to get started with how Freeciv-Web works.

The build process:

 1. Installing various system dependencies and Tomcat, a Java servlet host.
 2. Building freeciv.
 3. Converting freeciv resources like tilesets into a format the HTML-based client understands.
 4. Building the Java servlet project freeciv-web.
 5. Configuring an `nginx` front-end.
 
You can see the details of the build process by navigating from the scripts running in the Dockerfile.

Freeciv-Web uses a variety of services:

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

You can see how these services are run in their respective folders in the [production](production) directory.