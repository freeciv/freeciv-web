#!/bin/bash

# Freeciv - Copyright (C) 2007 - Marko Lindqvist
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

# Script to help setup of authentication enabled Freeciv server.
# I know that this will not work with bare sh. Tested with bash,
# so I set it in use above. 

if which basename >/dev/null
then
  PROGRAM_NAME="$(basename $0)"
else
  PROGRAM_NAME="setup_auth_server.sh"
fi
PROGRAM_VERSION="0.5"

#############################################################################
#
# Function definitions
#

# Ask yes/no question
#
# $1 - Question
#
# 0 - Answer 'No'
# 1 - Answer 'Yes'
#

ask_yes_no() {
  while /bin/true
  do
    ANSWER=""

    echo -e $1
    echo -n "(y/n) > "

    read ANSWER

    if test "x$ANSWER" = "xy" || test "x$ANSWER" = "xyes" ||
       test "x$ANSWER" = "xY" || test "x$ANSWER" = "xYES" ||
       test "x$ANSWER" = "xYes"
    then
      return 1
    elif test "x$ANSWER" = "xn" || test "x$ANSWER" = "xno" ||
       test "x$ANSWER" = "xN" || test "x$ANSWER" = "xNO" ||
       test "x$ANSWER" = "xNo"
    then
      return 0
    else
      echo "Please answer 'y' or 'n'"
    fi
  done
}

# Connects to Freeciv database on MySQL server and runs given query
# Determines connection parameters from environment variables.
# Special value "*" for MYSQL_PASSWORD causes mysql to prompt password.
#
# $1 - Query or "-" indicating query from stdin
#
# Output is what MySQL server shows. This function adds nothing.
#

make_query() {
  declare -i MYSQL_RETURN

  if test "x$MYSQL_SERVER" != "x"
  then
   MYSQL_SERVER_PARAM="-h$MYSQL_SERVER"
  else
   MYSQL_SERVER_PARAM=
  fi
  if test "x$MYSQL_PORT" != "x"
  then
   MYSQL_PORT_PARAM="-P$MYSQL_PORT"
  else
   MYSQL_PORT_PARAM=
  fi
  if test "x$MYSQL_DATABASE" != "x"
  then
   MYSQL_DATABASE_PARAM="-D$MYSQL_DATABASE"
  else
   MYSQL_DATABASE_PARAM=
  fi
  if test "x$MYSQL_USER" != "x"
  then
   MYSQL_USER_PARAM="-u$MYSQL_USER"
  else
   MYSQL_USER_PARAM=
  fi
  if test "x$MYSQL_PASSWORD" = "x*"
  then
   MYSQL_PASSWORD_PARAM="-p"
  elif test "x$MYSQL_PASSWORD" != "x"
  then
   MYSQL_PASSWORD_PARAM="-p$MYSQL_PASSWORD"
  else
   MYSQL_PASSWORD_PARAM=
  fi

  if test "x$1" != "x-"
  then
    echo "$1" | mysql $MYSQL_SERVER_PARAM $MYSQL_PORT_PARAM \
                      $MYSQL_USER_PARAM $MYSQL_PASSWORD_PARAM \
                      $MYSQL_DATABASE_PARAM
  else
    mysql $MYSQL_SERVER_PARAM $MYSQL_PORT_PARAM \
          $MYSQL_USER_PARAM $MYSQL_PASSWORD_PARAM \
          $MYSQL_DATABASE_PARAM
  fi

  MYSQL_RETURN=$?

  return $MYSQL_RETURN
}

print_usage() {
  echo "Usage: $PROGRAM_NAME [-v|--version] [-h|--help]"
}

#############################################################################
#
# Script main

# Check for commanline parameters

if test "x$1" = "x-v" || test "x$1" = "x--version"
then
  echo "$PROGRAM_NAME version $PROGRAM_VERSION"

  # If we have several parameters fall through to usage, otherwise exit
  if test "x$2" = "x"
  then
    exit 0
  fi
fi

if test "x$1" != "x"
then
  print_usage

  if test "x$2" = "x"
  then
    if test "x$1" = "x-h" || test "x$1" = "x--help"
    then
      exit 0
    fi
  fi

  exit 1
fi

echo "This script helps in setup of Freeciv server with player authentication."
echo "Most users do not need player authentication."
echo
echo "Authentication uses MySQL database to store player information."
echo "Before running this script you should have MySQL server running"
echo "and that server should have"
echo " - user account to be used by civserver"
echo " - empty database this script will populate"
echo "Password for that account will be stored to config file,"
echo "so you want to create special user account just for civserver use."
echo "This script needs MySQL user account that can create tables in to"
echo "that database. Generated config file for Freeciv server will"
echo "contain MySQL username used by this script, but it's easy to change"
echo "afterwards if you don't want use same account from this script and"
echo "later from Freeciv server."

if ask_yes_no "\nDo you want to continue with this script now?"
then
  echo "Canceling before anything done"
  exit
fi

echo
echo "First we populate player database at MySQL server"

if ! which mysql >/dev/null
then
  echo "mysql command not found. Aborting!"
  exit 1
fi

CONNECTED=no
MYSQL_SERVER="localhost"
MYSQL_PORT="3306"
MYSQL_USER="Freeciv"
MYSQL_PASSWORD=""
# We attempt connection without selecting database first
MYSQL_DATABASE=""

while test $CONNECTED = no
do
  echo "Please answer questions determining how to contact MySQL server."
  echo -n "server ($MYSQL_SERVER)> "
  read MYSQL_SERVER_NEW
  echo -n "port ($MYSQL_PORT)> "
  read MYSQL_PORT_NEW
  echo -n "username ($MYSQL_USER)> "
  read MYSQL_USER_NEW
  echo "If password is not required, say \"-\"."
  echo "If you want MySQL server to prompt it, say \"*\"."
  echo "(you need to type actual password many times if you choose *)"
  echo -n "password > "
  read MYSQL_PASSWORD_NEW

  if test "x$MYSQL_SERVER_NEW" != "x"
  then
    MYSQL_SERVER="$MYSQL_SERVER_NEW"
  fi
  if test "x$MYSQL_PORT_NEW" != "x"
  then
    MYSQL_PORT="$MYSQL_PORT_NEW"
  fi
  if test "x$MYSQL_USER_NEW" != "x"
  then
    export MYSQL_USER="$MYSQL_USER_NEW"
  fi
  if test "x$MYSQL_PASSWORD_NEW" != "x"
  then
    if test "x$MYSQL_PASSWORD_NEW" == "x-"
    then
      # No password
      MYSQL_PASSWORD=""
    else
      MYSQL_PASSWORD="$MYSQL_PASSWORD_NEW"
    fi
  fi

  # Just connect to server and exit if connect succeeded.
  if make_query "exit" ; then
    echo "Connection test to MySQL server succeeded."
    CONNECTED=yes
  else
    echo -e "\nConnection to MySQL server failed."
    if ask_yes_no "\nRetry with changed parameters"
    then
      echo "Aborting!"
      exit 1
    fi
  fi
done

echo "Next we select that database"

MYSQL_DATABASE="Freeciv"
DATABASE_SELECTED=no
while test $DATABASE_SELECTED = no
do
  echo "List of databases this user can see at MySQL server:"

  # Make sure that make_query doesn't try to select any database
  # for this query
  MYSQL_DATABASE_TMP="$MYSQL_DATABASE"
  MYSQL_DATABASE=""

  # List Databases - remove header and internal database.
  DBLIST="$(make_query 'show databases' | grep -v '^Database$' | grep -v '^information_schema$')"

  echo "$DBLIST" | grep "$MYSQL_DATABASE_TMP" > /dev/null
  GREPRESULT=$?

  # See if automatically proposed database is in the list
  if test $GREPRESULT -eq 0
  then
    # Keep current default
    MYSQL_DATABASE_TMP="$MYSQL_DATABASE_TMP"
  else
    # Select first one from the list
    MYSQL_DATABASE_TMP=$(echo "$DBLIST" | head -n 1)
  fi

  # Start lines with " -"
  echo "$DBLIST" | sed 's/^/ -/'
  echo
  echo "Please tell which one to use."
  echo -n "($MYSQL_DATABASE_TMP)> "
  read MYSQL_DATABASE_NEW

  if test "x$MYSQL_DATABASE_NEW" != "x"
  then
    MYSQL_DATABASE="$MYSQL_DATABASE_NEW"
  else
    MYSQL_DATABASE="$MYSQL_DATABASE_TMP"
  fi

  # Try to connect using that database
  if make_query "exit" ; then
    echo "Database successfully selected."
    DATABASE_SELECTED=yes
  else
    echo -e "\nCannot select that database."

    # We could try to create database here.
    # But we don't want anybody to think it's a
    # good idea to use MySQL account with database
    # creation permissions for Freeciv player authentication.

    if ask_yes_no "\nTry selecting some other database?"
    then
      echo "Aborting!"
      exit 1
    fi
  fi
done

# These are hardcoded here, and not prompted
TABLE_AUTH="auth"
TABLE_LOGINLOG="loginlog"

TABLELIST="$(make_query 'show tables' | grep -v '^Tables_in_')"

if test "x$TABLELIST" != "x"
then
  echo "This database already contains some tables."

  echo "$TABLELIST" | grep "$TABLE_AUTH" > /dev/null
  GREPRESULT=$?

  if test $GREPRESULT -eq 0
  then
    AUTH_TABLE_PRESENT=yes
  else
    AUTH_TABLE_PRESENT=no
  fi

  echo "$TABLELIST" | grep "$TABLE_LOGINLOG" > /dev/null
  GREPRESULT=$?

  if test $GREPRESULT -eq 0
  then
    LOGINLOG_TABLE_PRESENT=yes
  else
    LOGINLOG_TABLE_PRESENT=no
  fi

  if test $LOGINLOG_TABLE_PRESENT = yes ||
     test $AUTH_TABLE_PRESENT = yes
  then
    echo "There is even tables with names Freeciv would use."
    if test $AUTH_TABLE_PRESENT = yes
    then
      echo " -$TABLE_AUTH"
    fi
    if test $LOGINLOG_TABLE_PRESENT = yes
    then
      echo " -$TABLE_LOGINLOG"
    fi

    echo "Maybe you have already attempted to create Freeciv tables?"
    echo "We can destroy existing tables and create new ones, if you really want."
    echo "Just be sure that they indeed are Freeciv related tables AND if"
    echo "they are Freeciv tables, they contain nothing important."
    echo "If you have ever run Freeciv server with authentication, these"
    echo "contain player accounts and login information."

    if ask_yes_no "\nDo you want tables destroyed, and DATA IN THEM PERMANENTLY LOST?"
    then
      echo "Can't continue because of conflicting tables."
      exit 1
    fi

    # Drop tables
    if test $AUTH_TABLE_PRESENT = yes
    then
      if ! make_query "drop table $TABLE_AUTH"
      then
        echo "Dropping table $TABLE_AUTH failed!"
        echo "Aborting!"
        exit 1
      fi
    fi
    if test $LOGINLOG_TABLE_PRESENT = yes
    then
      if ! make_query "drop table $TABLE_LOGINLOG"
      then
        echo "Dropping table $TABLE_AUTH failed!"
        echo "Aborting!"
        exit 1
      fi
    fi

    # Updated tablelist
    TABLELIST="$(make_query 'show tables' | grep -v '^Tables_in_')"
    if test "x$TABLELIST" != "x"
    then
      echo "After dropping Freeciv tables, others remain."
    fi
  fi

  # Do we still have tables in that database?
  if test "x$TABLELIST" != "x"
  then
    # Print them, each line starting with " -"
    echo "$TABLELIST" | sed 's/^/ -/'
    echo "Table names do not conflict with tables Freeciv would use."

    if ask_yes_no "\nDo you really want to use this database for Freeciv\nplayer authentication?"
    then
      echo "Aborting!"
      exit 1
    fi
  fi
fi

echo "Now we create the Freeciv tables."

# We have embedded table creation SQL to this script.
# Maybe we should read it from separate file in the future.
(echo \
 "CREATE TABLE $TABLE_AUTH ( \
   id int(11) NOT NULL auto_increment, \
   name varchar(32) default NULL, \
   password varchar(32) default NULL, \
   email varchar(128) default NULL, \
   createtime int(11) default NULL, \
   accesstime int(11) default NULL, \
   address varchar(255) default NULL, \
   createaddress varchar(15) default NULL, \
   logincount int(11) default '0', \
   PRIMARY KEY  (id), \
   UNIQUE KEY name (name) \
 ) TYPE=MyISAM;"
 echo \
 "CREATE TABLE $TABLE_LOGINLOG ( \
   id int(11) NOT NULL auto_increment, \
   name varchar(32) default NULL, \
   logintime int(11) default NULL, \
   address varchar(255) default NULL, \
   succeed enum('S','F') default 'S', \
   PRIMARY KEY  (id) \
 ) TYPE=MyISAM;"
) | make_query "-"

QUERYRESULT=$?

if test $QUERYRESULT -ne 0
then
  echo "Creation of tables failed!"
  echo "Aborting!"
  exit 1
fi

echo "Tables successfully created!"

CONFIG_FILE="./fc_auth.conf"
ACCEPTABLE_FILE=no
while test $ACCEPTABLE_FILE = no
do
  echo "Give name for Freeciv server authentication config file we"
  echo "are about to generate next."

  echo -n "($CONFIG_FILE) > "
  read CONFIG_FILE_NEW

  if test "x$CONFIG_FILE_NEW" != "x"
  then
    CONFIG_FILE="$CONFIG_FILE_NEW"
  fi

  # Default is to test file creation
  TRY_FILE=yes
  if test -e "$CONFIG_FILE"
  then
    echo "$CONFIG_FILE already exist"
    # Default is not to test overwriting
    TRY_FILE=no
    if test -d "$CONFIG_FILE"
    then
      echo "and it's directory. Can't overwrite with file."
    else
      if ! ask_yes_no "\nOK to overwrite?"
      then
        TRY_FILE=yes
      fi
    fi
  fi

  if test $TRY_FILE = "yes"
  then
    if touch "$CONFIG_FILE"
    then
      ACCEPTABLE_FILE=yes
    else
      echo "Can't write to that file"
      if ask_yes_no "\nRetry with other file?"
      then
        echo "Aborting!"
        exit 1
      fi
    fi
  fi

done

SAVE_PASSWORD=no
if test "x$MYSQL_PASSWORD" != "x" &&
   test "x$MYSQL_PASSWORD" != "x-" &&
   test "x$MYSQL_PASSWORD" != "x*"
then
  # User has given password for this script, should it also
  # go to config script? If user has not given password even
  # to this script, (s)he definitely does not want it saved.
  echo "Freeciv server needs MySQL password from config file"
  echo "in order to access database."
  echo "It has to be added to config file before authentication"
  echo "can be used. We can add it automatically now, or you"
  echo "can add it manually later."
  if ! ask_yes_no "\nShould password be added to config file now?"
  then
    SAVE_PASSWORD=yes
  fi
fi

# Generate config file
(
 echo "; Configuration file for Freeciv server player authentication"
 echo "; Generated by \"$PROGRAM_NAME\" version $PROGRAM_VERSION"
 echo
 echo "[auth]"
 echo "; How to connect MySQL server"
 echo "host=\"$MYSQL_SERVER\""
 echo "port=\"$MYSQL_PORT\" ; This is handled as string!"
 echo "user=\"$MYSQL_USER\""
 if test $SAVE_PASSWORD = "yes"
 then
   echo "password=\"$MYSQL_PASSWORD\""
 else
   echo ";password=\"\""
 fi
 echo
 echo "; What database to use in MySQL server"
 echo "database=\"$MYSQL_DATABASE\""
 echo
 echo "; Table names"
 echo "table=\"$TABLE_AUTH\""
 echo "login_table=\"$TABLE_LOGINLOG\""
) > $CONFIG_FILE

echo "Config file generated."
echo "Auth server setup finished."
