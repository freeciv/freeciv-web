 CasperJS tests for Freeciv-web.
===============================

 This test is run automatically when setting up Vagrant or TravisCI installation. 

 Freeciv-web requires CasperJS 1.1-beta3, SlimerJS 0.9.6 or git master, xvfb 
 (for headless tests).

 PhantomJS 2.0 is also supported, but not run automatically since there is no
 Ubuntu package for PhantomJS 2 yet.

 The tests can also be run manually like this:
 xvfb-run ./casperjs --engine=slimerjs test /vagrant/tests/freeciv-web-tests.js

