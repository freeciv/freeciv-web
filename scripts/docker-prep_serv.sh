#!/bin/bash

## Prepare mysql, tomcat and nginx

echo "docker ALL = (root) NOPASSWD: /docker/scripts/docker-build*.sh, /docker/scripts/start-freeciv-web.sh, /usr/sbin/service nginx *, /usr/sbin/service tomcat8 *, /usr/sbin/service *, /etc/init.d/tomcat8 *, /bin/chmod, /bin/ls, /bin/cat, /usr/bin/mysql_install_db, /usr/bin/mysql *  /usr/share/tomcat8/bin/catalina.sh *" >> /etc/sudoers
echo "freeciv ALL = (root) NOPASSWD: /docker/scripts/docker-build*.sh, /docker/scripts/start-freeciv-web.sh, /usr/sbin/service nginx *, /usr/sbin/service tomcat8 *, /usr/sbin/service *, /etc/init.d/tomcat8 *, /bin/chmod, /bin/ls, /bin/cat,  /usr/local/tomcat/bin/catalina.sh *" >> /etc/sudoers

mkdir -p /data /var/lib/mysql
chmod -R 777 /data /var/lib/mysql
chown -R root:root /data /var/lib/mysql

export CATALINA_HOME=/usr/local/tomcat
export PATH=$PATH:$CATALINA_HOME/bin

cp /docker/publite2/nginx.conf /etc/nginx/nginx.conf
ln -s /usr/local/tomcat/ /var/lib/tomcat8
mv /var/lib/tomcat8/webapps/ROOT/ /var/lib/tomcat8/webapps/root-old/

chmod +x /docker/scripts/docker-build.sh
chmod +x /docker/scripts/init_mysql_db.sh

cp /docker/pbem/settings.ini.dist /docker/pbem/settings.ini
cp /docker/publite2/settings.ini.dist /docker/publite2/settings.ini
cp /docker/freeciv-proxy/settings.ini.dist /docker/freeciv-proxy/settings.ini
cp /docker/freeciv-web/flyway.properties.dist /docker/freeciv-web/flyway.properties
cp /docker/freeciv-web/src/main/webapp/WEB-INF/config.properties.dist /docker/freeciv-web/src/main/webapp/WEB-INF/config.properties

echo "mysql setup..."

/bin/bash /docker/scripts/init_mysql_db.sh
