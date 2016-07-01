 CasperJS tests for Freeciv-web.
===============================

 This test is run automatically when setting up Vagrant or TravisCI installation. 

 Freeciv-web requires CasperJS 1.1.2, PhantomJS 2.0 or SlimerJS version 0.10.0pre, 
 and xvfb (for headless tests). 

 The tests can also be run manually like this in the /tests directory:

 ```bash
 xvfb-run casperjs --engine=phantomjs test freeciv-web-tests.js
 ```
 Running the tests will also produce two screenshots.

 The automatic build by Travis-CI uses PhantomJS.

freeciv-web-autogame.js will start a server autogame for 200 turns. This is run by Travis-CI also.
