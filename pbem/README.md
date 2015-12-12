Freeciv-PBEM
============

Play-By-Email support for Freeciv-web!

Requires:
https://github.com/mysql/mysql-connector-python

Run like this:
python3.4 freeciv-pbem.py

(also automatically started by start-freeciv-web.sh script)


Required configuration:
 1. Put your Google ReCaptcha (https://www.google.com/recaptcha) 
  secret key in captcha-secret field here:
    freeciv-web\src\main\webapp\WEB-INF\resin-web.xml.dist 
   and Google ReCaptcha client sitekey here:
    freeciv-web/src/main/webapp/javascript/pbem.js

 2. Update settings.ini.dist with
     - SMTP username, password, host,port. The SMTP authentication can be gotten by creating an account at http://www.mailgun.com
     - MySQL username and password for the local MySQL instance. 
    then rename this settings-file to settings.ini.
    


