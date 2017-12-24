#!/bin/bash

echo "mysql setup..."
/usr/bin/mysql_install_db
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password password ${mysql_pass}"
sudo debconf-set-selections <<< "mysql-server mysql-server/root_password_again password ${mysql_pass}"

/usr/bin/mysqld_safe &
sleep 5
mysql -u root -e "CREATE DATABASE freeciv_web;"
mysql -u root freeciv_web < /docker/scripts/build_mysql_db.sql
sleep 5
service mysql stop
sleep 5
