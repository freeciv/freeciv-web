Freeciv-PBEM
============

Play-By-Email support for Freeciv-web!

Requires:
https://github.com/mysql/mysql-connector-python

Run like this:
python3 pbem.py

Freeciv-pbem is automatically started by start-freeciv-web.sh script
Python unit tests are found in test_pbem.py.

Required configuration:
 1. Put your Google ReCaptcha (https://www.google.com/recaptcha) 
  secret key in captcha-secret field here:
    freeciv-web\src\main\webapp\WEB-INF\web.xml 
   and Google ReCaptcha client sitekey here:
    freeciv-web/src/main/webapp/javascript/pbem.js

 2. Update settings.ini.dist with
     - SMTP username, password, host,port. The SMTP authentication can be gotten by creating an account at http://www.mailgun.com
     - MySQL username and password for the local MySQL instance. 
    then rename this settings-file to settings.ini.
     - savegame_directory and ranklog_directory, which is where PBEM savegames and ranklog files are stored.
       (savegame_directory in this file should be set to the same value as save_directory in publite2/settings.ini.
    

