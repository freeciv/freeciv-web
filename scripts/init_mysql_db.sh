#!/bin/bash

/usr/bin/mysqld_safe &
sleep 5
mysql -u root -e "CREATE DATABASE freeciv_web;"
mysql -u root freeciv_web < /docker/scripts/build_mysql_db.sql
sleep 2
service mysql stop
sleep 2
