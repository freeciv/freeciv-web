 CasperJS tests for Freeciv-web.
===============================

 This test is run automatically when setting up Vagrant or TravisCI installation. 

 Freeciv-web requires CasperJS 1.1-beta3, SlimerJS version 0.10.0pre, 
 xvfb (for headless tests) and Firefox 40. 

 PhantomJS 2.0 is also supported.

 The tests can also be run manually like this in the /tests directory:

 ```bash
 xvfb-run casperjs --engine=phantomjs test freeciv-web-tests.js
 ```
 Running the tests will also produce two screenshots.
