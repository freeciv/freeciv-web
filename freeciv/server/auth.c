/**********************************************************************
 Freeciv - Copyright (C) 2005  - M.C. Kaufman
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_AUTH
  #include <mysql/mysql.h>
#endif

/* utility */
#include "fcintl.h"
#include "log.h"
#include "md5.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* common */
#include "connection.h"
#include "packets.h"

/* server */
#include "connecthand.h"
#include "notify.h"
#include "sernet.h"
#include "srv_main.h"

#include "auth.h"

/* where our mysql database is located and how to get to it */
#define DEFAULT_AUTH_HOST     "localhost"
#define DEFAULT_AUTH_PORT     "3306"
#define DEFAULT_AUTH_USER     "anonymous"
#define DEFAULT_AUTH_PASSWORD ""

/* the database where our table is located */
#define DEFAULT_AUTH_DATABASE "test"

/* the tables where we will do our lookups and inserts.
 * the tables can be created with the following:
 *
 * CREATE TABLE auth (
 *   id int(11) NOT NULL auto_increment,
 *   name varchar(32) default NULL,
 *   password varchar(32) default NULL,
 *   email varchar(128) default NULL,
 *   createtime int(11) default NULL,
 *   accesstime int(11) default NULL,
 *   address varchar(255) default NULL,
 *   createaddress varchar(15) default NULL,
 *   logincount int(11) default '0',
 *   PRIMARY KEY  (id),
 *   UNIQUE KEY name (name)
 * ) TYPE=MyISAM;
 * 
 * CREATE TABLE loginlog (
 *   id int(11) NOT NULL auto_increment,
 *   name varchar(32) default NULL,
 *   logintime int(11) default NULL,
 *   address varchar(255) default NULL,
 *   succeed enum('S','F') default 'S',
 *   PRIMARY KEY  (id)
 * ) TYPE=MyISAM;
 *
 * N.B. if the tables are not of this format, then the select,insert,
 *      and update syntax in the auth_db_* functions below must be changed.
 */
#define DEFAULT_AUTH_TABLE           "auth"
#define DEFAULT_AUTH_LOGIN_TABLE     "loginlog"

#define GUEST_NAME "guest"

#define MIN_PASSWORD_LEN  6  /* minimum length of password */
#define MIN_PASSWORD_CAPS 0  /* minimum number of capital letters required */
#define MIN_PASSWORD_NUMS 0  /* minimum number of numbers required */

#define MAX_AUTH_TRIES 3
#define MAX_WAIT_TIME 300   /* max time we'll wait on a password */

/* after each wrong guess for a password, the server waits this
 * many seconds to reply to the client */
static const int auth_fail_wait[] = { 1, 1, 2, 3 };

/**************************************************
 The auth db statuses are:

 1: an error occurred, possibly we couldn't access the database file.
 2: we were able to successfully insert an entry. or we found the entry 
    we were searching for
 3: the user we were searching for was not found.
***************************************************/
enum authdb_status {
  AUTH_DB_ERROR,
  AUTH_DB_SUCCESS,
  AUTH_DB_NOT_FOUND
};

static bool is_good_password(const char *password, char *msg);

static bool authdb_check_password(struct connection *pconn,
                           const char *password, int len);
static enum authdb_status auth_db_load(struct connection *pconn);
static bool auth_db_save(struct connection *pconn);

#ifdef HAVE_AUTH
static char *alloc_escaped_string(MYSQL *mysql, const char *orig);
static void free_escaped_string(char *str);
#endif /* HAVE_AUTH */

#ifdef HAVE_AUTH

enum auth_option_source {
  AOS_DEFAULT,  /* Internal default         */
  AOS_FILE,     /* Read from config file    */
  AOS_SET       /* Set, currently not used  */
};

struct auth_option {
  const char              *name;
  char                    *value;
  enum auth_option_source source;
};

static struct authentication_conf {
  struct auth_option host;
  struct auth_option port;
  struct auth_option user;
  struct auth_option password;
  struct auth_option database;
  struct auth_option table;
  struct auth_option login_table;
} auth_config;

/* How much information dump functions show */
enum show_source_type {
  SST_NONE,
  SST_DEFAULT,
  SST_ALL
};

/**************************************************************************
  Output information about one auth option.
**************************************************************************/
static void print_auth_option(int loglevel, enum show_source_type show_source,
                              bool show_value, const struct auth_option *target)
{
  char buffer[512];
  bool real_show_source;

  buffer[0] = '\0';

  real_show_source = (show_source == SST_ALL
                      || (show_source == SST_DEFAULT && target->source == AOS_DEFAULT));

  if (show_value || real_show_source) {
    /* We will print this line. Begin it. */
    /* TRANS: Further information about option will follow. */
    my_snprintf(buffer, sizeof(buffer), _("Auth option \"%s\":"), target->name);
  }

  if (show_value) {
    cat_snprintf(buffer, sizeof(buffer), " \"%s\"", target->value);
  }
  if (real_show_source) {
    switch(target->source) {
     case AOS_DEFAULT:
       if (show_source == SST_DEFAULT) {
         cat_snprintf(buffer, sizeof(buffer),
		      /* TRANS: After 'Auth option "user":'. Option value
			 may have been inserted between these. */
                      _(" missing from config file (using default)"));
       } else {
         /* TRANS: auth option originates from internal default */
         cat_snprintf(buffer, sizeof(buffer), _(" (default)"));
       }
       break;
     case AOS_FILE:
       /* TRANS: auth option originates from config file */
       cat_snprintf(buffer, sizeof(buffer), _(" (config)"));
       break;
     case AOS_SET:
       /* TRANS: auth option has been set from prompt */
       cat_snprintf(buffer, sizeof(buffer), _(" (set)"));
       break;
    }
  }

  if (buffer[0] != '\0') {
    /* There is line to print */
    freelog(loglevel, "%s", buffer);
  }
}

/**************************************************************************
  Output auth config information.
**************************************************************************/
static void print_auth_config(int loglevel, enum show_source_type show_source,
                              bool show_value)
{
  print_auth_option(loglevel, show_source, show_value, &auth_config.host);
  print_auth_option(loglevel, show_source, show_value, &auth_config.port);
  print_auth_option(loglevel, show_source, show_value, &auth_config.user);
  print_auth_option(loglevel, show_source, FALSE, &auth_config.password);
  print_auth_option(loglevel, show_source, show_value, &auth_config.database);
  print_auth_option(loglevel, show_source, show_value, &auth_config.table);
  print_auth_option(loglevel, show_source, show_value, &auth_config.login_table);
}

/**************************************************************************
  Set one auth option.
**************************************************************************/
static bool set_auth_option(struct auth_option *target, const char *value,
                            enum auth_option_source source)
{
  if (value != NULL
      && !strcmp(target->name, "port")) {
    /* Port value must be all numeric. */
    int i;

    for (i = 0; value[i] != '\0'; i++) {
      if (value[i] < '0' || value[i] > '9') {
        freelog(LOG_ERROR, _("Illegal value for auth port: \"%s\""), value);
        return FALSE;
      }
    }
  }

  if (value == NULL) {
    if (target->value != NULL) {
      free(target->value);
    }
    target->value = NULL;
  } else {
    target->value = fc_realloc(target->value, strlen(value) + 1);
    memcpy(target->value, value, strlen(value) + 1);
  }
  target->source = source;

  return TRUE;
}

/**************************************************************************
  Load value for one auth option from section_file.
**************************************************************************/
static void load_auth_option(struct section_file *file,
                             struct auth_option *target)
{
  const char *value;

  value = secfile_lookup_str_default(file, "", "auth.%s", target->name);
  if (value[0] != '\0') {
    /* We really loaded something from file */
    set_auth_option(target, value, AOS_FILE);
  }
}

/**************************************************************************
  Load auth configuration from file.
  We use filename just like user gave it to us.
  No searching from datadirs, if file with same name exist there!
**************************************************************************/
static bool load_auth_config(const char *filename)
{
  struct section_file file;

  assert(filename != NULL);

  if (!section_file_load_nodup(&file, filename)) {
    freelog(LOG_ERROR, _("Cannot load auth config file \"%s\"!"), filename);
    return FALSE;
  }

  load_auth_option(&file, &auth_config.host);
  load_auth_option(&file, &auth_config.port);
  load_auth_option(&file, &auth_config.user);
  load_auth_option(&file, &auth_config.password);
  load_auth_option(&file, &auth_config.database);
  load_auth_option(&file, &auth_config.table);
  load_auth_option(&file, &auth_config.login_table);

  section_file_check_unused(&file, filename);
  section_file_free(&file);

  return TRUE;
}
#endif /* HAVE_AUTH */

/**************************************************************************
  Initialize authentication system
**************************************************************************/
bool auth_init(const char *conf_file)
{
#ifdef HAVE_AUTH
  static bool first_init = TRUE;

  if (first_init) {
    /* Run just once when program starts */
    auth_config.host.value        = NULL;
    auth_config.port.value        = NULL;
    auth_config.user.value        = NULL;
    auth_config.password.value    = NULL;
    auth_config.database.value    = NULL;
    auth_config.table.value       = NULL;
    auth_config.login_table.value = NULL;

    auth_config.host.name         = "host";
    auth_config.port.name         = "port";
    auth_config.user.name         = "user";
    auth_config.password.name     = "password";
    auth_config.database.name     = "database";
    auth_config.table.name        = "table";
    auth_config.login_table.name  = "login_table";

    first_init = FALSE;
  }

  set_auth_option(&auth_config.host, DEFAULT_AUTH_HOST, AOS_DEFAULT);
  set_auth_option(&auth_config.port, DEFAULT_AUTH_PORT, AOS_DEFAULT);
  set_auth_option(&auth_config.user, DEFAULT_AUTH_USER, AOS_DEFAULT);
  set_auth_option(&auth_config.password, DEFAULT_AUTH_PASSWORD, AOS_DEFAULT);
  set_auth_option(&auth_config.database, DEFAULT_AUTH_DATABASE, AOS_DEFAULT);
  set_auth_option(&auth_config.table, DEFAULT_AUTH_TABLE, AOS_DEFAULT);
  set_auth_option(&auth_config.login_table, DEFAULT_AUTH_LOGIN_TABLE, AOS_DEFAULT);

  if (strcmp(conf_file, "-")) {
    if (!load_auth_config(conf_file)) {
      return FALSE;
    }

    /* Print all options in LOG_DEBUG level... */
    print_auth_config(LOG_DEBUG, SST_ALL, TRUE);

    /* ...and those missing from config file with LOG_NORMAL too */
    print_auth_config(LOG_NORMAL, SST_DEFAULT, FALSE);

  } else {
    freelog(LOG_DEBUG, "No auth config file. Using defaults");
  }

#endif /* HAVE_AUTH */

  return TRUE;
}

/**************************************************************************
  Free resources allocated by auth system.
**************************************************************************/
void auth_free(void)
{
#ifdef HAVE_AUTH
  set_auth_option(&auth_config.host, NULL, AOS_DEFAULT);
  set_auth_option(&auth_config.port, NULL, AOS_DEFAULT);
  set_auth_option(&auth_config.user, NULL, AOS_DEFAULT);
  set_auth_option(&auth_config.password, NULL, AOS_DEFAULT);
  set_auth_option(&auth_config.database, NULL, AOS_DEFAULT);
  set_auth_option(&auth_config.table, NULL, AOS_DEFAULT);
  set_auth_option(&auth_config.login_table, NULL, AOS_DEFAULT);
#endif /* HAVE_AUTH */
}

/**************************************************************************
  handle authentication of a user; called by handle_login_request()
  if authentication is enabled.

  if the connection is rejected right away, return FALSE, otherwise return TRUE
**************************************************************************/
bool authenticate_user(struct connection *pconn, char *username)
{
  char tmpname[MAX_LEN_NAME] = "\0";

  /* assign the client a unique guest name/reject if guests aren't allowed */
  if (is_guest_name(username)) {
    if (srvarg.auth_allow_guests) {

      sz_strlcpy(tmpname, username);
      get_unique_guest_name(username);

      if (strncmp(tmpname, username, MAX_LEN_NAME) != 0) {
        notify_conn(pconn->self, NULL, E_CONNECTION, FTC_WARNING, NULL,
                    _("Warning: the guest name '%s' has been "
                      "taken, renaming to user '%s'."), tmpname, username);
      }
      sz_strlcpy(pconn->username, username);
      establish_new_connection(pconn);
    } else {
      reject_new_connection(_("Guests are not allowed on this server. "
                              "Sorry."), pconn);
      freelog(LOG_NORMAL, _("%s was rejected: Guests not allowed."), username);
      return FALSE;
    }
  } else {
    /* we are not a guest, we need an extra check as to whether a 
     * connection can be established: the client must authenticate itself */
    char buffer[MAX_LEN_MSG];

    sz_strlcpy(pconn->username, username);

    switch(auth_db_load(pconn)) {
    case AUTH_DB_ERROR:
      if (srvarg.auth_allow_guests) {
        sz_strlcpy(tmpname, pconn->username);
        get_unique_guest_name(tmpname); /* don't pass pconn->username here */
        sz_strlcpy(pconn->username, tmpname);

        freelog(LOG_ERROR, "Error reading database; connection -> guest");
        notify_conn(pconn->self, NULL, E_CONNECTION, FTC_WARNING, NULL,
                    _("There was an error reading the user "
                      "database, logging in as guest connection '%s'."), 
                    pconn->username);
        establish_new_connection(pconn);
      } else {
        reject_new_connection(_("There was an error reading the user database "
                                "and guest logins are not allowed. Sorry"), 
                              pconn);
        freelog(LOG_NORMAL, 
                _("%s was rejected: Database error and guests not allowed."),
                pconn->username);
        return FALSE;
      }
      break;
    case AUTH_DB_SUCCESS:
      /* we found a user */
      my_snprintf(buffer, sizeof(buffer), _("Enter password for %s:"),
                  pconn->username);
      dsend_packet_authentication_req(pconn, AUTH_LOGIN_FIRST, buffer);
      pconn->server.auth_settime = time(NULL);
      pconn->server.status = AS_REQUESTING_OLD_PASS;
      break;
    case AUTH_DB_NOT_FOUND:
      /* we couldn't find the user, he is new */
      if (srvarg.auth_allow_newusers) {
        sz_strlcpy(buffer, _("Enter a new password (and remember it)."));
        dsend_packet_authentication_req(pconn, AUTH_NEWUSER_FIRST, buffer);
        pconn->server.auth_settime = time(NULL);
        pconn->server.status = AS_REQUESTING_NEW_PASS;
      } else {
        reject_new_connection(_("This server allows only preregistered "
                                "users. Sorry."), pconn);
        freelog(LOG_NORMAL,
                _("%s was rejected: Only preregistered users allowed."),
                pconn->username);

        return FALSE;
      }
      break;
    default:
      assert(0);
      break;
    }
    return TRUE;
  }

  return TRUE;
}

/**************************************************************************
  Receives a password from a client and verifies it.
**************************************************************************/
bool handle_authentication_reply(struct connection *pconn, char *password)
{
  char msg[MAX_LEN_MSG];

  if (pconn->server.status == AS_REQUESTING_NEW_PASS) {

    /* check if the new password is acceptable */
    if (!is_good_password(password, msg)) {
      if (pconn->server.auth_tries++ >= MAX_AUTH_TRIES) {
        reject_new_connection(_("Sorry, too many wrong tries..."), pconn);
        freelog(LOG_NORMAL, _("%s was rejected: Too many wrong password "
                "verifies for new user."), pconn->username);

	return FALSE;
      } else {
        dsend_packet_authentication_req(pconn, AUTH_NEWUSER_RETRY, msg);
	return TRUE;
      }
    }

    /* the new password is good, create a database entry for
     * this user; we establish the connection in handle_db_lookup */
    sz_strlcpy(pconn->server.password, password);

    if (!auth_db_save(pconn)) {
      notify_conn(pconn->self, NULL, E_CONNECTION, FTC_WARNING, NULL,
		  _("Warning: There was an error in saving to the database. "
                    "Continuing, but your stats will not be saved."));
      freelog(LOG_ERROR, "Error writing to database for: %s", pconn->username);
    }

    establish_new_connection(pconn);
  } else if (pconn->server.status == AS_REQUESTING_OLD_PASS) { 
    if (authdb_check_password(pconn, password, strlen(password)) == 1) {
      establish_new_connection(pconn);
    } else {
      pconn->server.status = AS_FAILED;
      pconn->server.auth_tries++;
      pconn->server.auth_settime = time(NULL)
                                   + auth_fail_wait[pconn->server.auth_tries];
    }
  } else {
    freelog(LOG_VERBOSE, "%s is sending unrequested auth packets", 
            pconn->username);
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
 checks on where in the authentication process we are.
**************************************************************************/
void process_authentication_status(struct connection *pconn)
{
  switch(pconn->server.status) {
  case AS_NOT_ESTABLISHED:
    /* nothing, we're not ready to do anything here yet. */
    break;
  case AS_FAILED:
    /* the connection gave the wrong password, we kick 'em off or
     * we're throttling the connection to avoid password guessing */
    if (pconn->server.auth_settime > 0
        && time(NULL) >= pconn->server.auth_settime) {

      if (pconn->server.auth_tries >= MAX_AUTH_TRIES) {
        pconn->server.status = AS_NOT_ESTABLISHED;
        reject_new_connection(_("Sorry, too many wrong tries..."), pconn);
        freelog(LOG_NORMAL,
                _("%s was rejected: Too many wrong password tries."),
                pconn->username);
        close_connection(pconn);
      } else {
        struct packet_authentication_req request;

        pconn->server.status = AS_REQUESTING_OLD_PASS;
        request.type = AUTH_LOGIN_RETRY;
        sz_strlcpy(request.message,
                   _("Your password is incorrect. Try again."));
        send_packet_authentication_req(pconn, &request);
      }
    }
    break;
  case AS_REQUESTING_OLD_PASS:
  case AS_REQUESTING_NEW_PASS:
    /* waiting on the client to send us a password... don't wait too long */
    if (time(NULL) >= pconn->server.auth_settime + MAX_WAIT_TIME) {
      pconn->server.status = AS_NOT_ESTABLISHED;
      reject_new_connection(_("Sorry, your connection timed out..."), pconn);
      freelog(LOG_NORMAL,
              _("%s was rejected: Connection timeout waiting for password."),
              pconn->username);

      close_connection(pconn);
    }
    break;
  case AS_ESTABLISHED:
    /* this better fail bigtime */
    assert(pconn->server.status != AS_ESTABLISHED);
    break;
  }
}

/**************************************************************************
  see if the name qualifies as a guest login name
**************************************************************************/
bool is_guest_name(const char *name)
{
  return (mystrncasecmp(name, GUEST_NAME, strlen(GUEST_NAME)) == 0);
}

/**************************************************************************
  return a unique guest name
  WARNING: do not pass pconn->username to this function: it won't return!
**************************************************************************/
void get_unique_guest_name(char *name)
{
  unsigned int i;

  /* first see if the given name is suitable */
  if (is_guest_name(name) && !find_conn_by_user(name)) {
    return;
  } 

  /* next try bare guest name */
  mystrlcpy(name, GUEST_NAME, MAX_LEN_NAME);
  if (!find_conn_by_user(name)) {
    return;
  }

  /* bare name is taken, append numbers */
  for (i = 1; ; i++) {
    my_snprintf(name, MAX_LEN_NAME, "%s%u", GUEST_NAME, i);


    /* attempt to find this name; if we can't we're good to go */
    if (!find_conn_by_user(name)) {
      break;
    }
  }
}

/**************************************************************************
 Verifies that a password is valid. Does some [very] rudimentary safety 
 checks. TODO: do we want to frown on non-printing characters?
 Fill the msg (length MAX_LEN_MSG) with any worthwhile information that 
 the client ought to know. 
**************************************************************************/
static bool is_good_password(const char *password, char *msg)
{
  int i, num_caps = 0, num_nums = 0;
   
  /* check password length */
  if (strlen(password) < MIN_PASSWORD_LEN) {
    my_snprintf(msg, MAX_LEN_MSG,
                _("Your password is too short, the minimum length is %d. "
                  "Try again."), MIN_PASSWORD_LEN);
    return FALSE;
  }
 
  my_snprintf(msg, MAX_LEN_MSG,
              _("The password must have at least %d capital letters, %d "
                "numbers, and be at minimum %d [printable] characters long. "
                "Try again."), 
              MIN_PASSWORD_CAPS, MIN_PASSWORD_NUMS, MIN_PASSWORD_LEN);

  for (i = 0; i < strlen(password); i++) {
    if (my_isupper(password[i])) {
      num_caps++;
    }
    if (my_isdigit(password[i])) {
      num_nums++;
    }
  }

  /* check number of capital letters */
  if (num_caps < MIN_PASSWORD_CAPS) {
    return FALSE;
  }

  /* check number of numbers */
  if (num_nums < MIN_PASSWORD_NUMS) {
    return FALSE;
  }

  if (!is_ascii_name(password)) {
    return FALSE;
  }

  return TRUE;
}


/**************************************************************************
  Check if the password with length len matches that given in 
  pconn->server.password.
***************************************************************************/
static bool authdb_check_password(struct connection *pconn, 
                                  const char *password, int len)
{
#ifdef HAVE_AUTH
  bool ok = FALSE;
  char buffer[512] = "";
  const int bufsize = sizeof(buffer);
  char checksum[DIGEST_HEX_BYTES + 1];
  MYSQL *sock, mysql;

  /* do the password checking right here */
  create_md5sum(password, len, checksum);
  ok = (strncmp(checksum, pconn->server.password, DIGEST_HEX_BYTES) == 0)
                                                              ? TRUE : FALSE;

  /* we don't really need the stuff below here to 
   * verify password, this is just logging */
  mysql_init(&mysql);

  /* attempt to connect to the server */
  if ((sock = mysql_real_connect(&mysql, auth_config.host.value,
                                 auth_config.user.value, auth_config.password.value, 
                                 auth_config.database.value,
                                 atoi(auth_config.port.value), NULL, 0))) {
    char *name_buffer = alloc_escaped_string(&mysql, pconn->username);
    int str_result;

    if (name_buffer != NULL) {
      /* insert an entry into our log */
      str_result = my_snprintf(buffer, bufsize,
                               "insert into %s (name, logintime, address, succeed) "
                               "values ('%s',unix_timestamp(),'%s','%s')",
                               auth_config.login_table.value,
                               name_buffer, pconn->server.ipaddr, ok ? "S" : "F");

      if (str_result < 0 || str_result >= bufsize || mysql_query(sock, buffer)) {
        freelog(LOG_ERROR, "check_pass insert loginlog failed for user: %s (%s)",
                pconn->username, mysql_error(sock));
      }
      free_escaped_string(name_buffer);
      name_buffer = NULL;
    }
    mysql_close(sock);
  } else {
    freelog(LOG_ERROR, "Can't connect to server! (%s)", mysql_error(&mysql));
  }

  return ok;
#else  /* HAVE_AUTH */
  return TRUE;
#endif /* HAVE_AUTH */
}

/**************************************************************************
 Loads a user from the database.
**************************************************************************/
static enum authdb_status auth_db_load(struct connection *pconn)
{
#ifdef HAVE_AUTH
  char buffer[512] = "";
  const int bufsize = sizeof(buffer);
  int num_rows = 0;
  MYSQL *sock, mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *name_buffer;
  int str_result;

  mysql_init(&mysql);

  /* attempt to connect to the server */
  if (!(sock = mysql_real_connect(&mysql, auth_config.host.value,
                                  auth_config.user.value,
                                  auth_config.password.value,
                                  auth_config.database.value,
                                  atoi(auth_config.port.value),
                                  NULL, 0))) {
    freelog(LOG_ERROR, "Can't connect to server! (%s)", mysql_error(&mysql));
    return AUTH_DB_ERROR;
  }

  name_buffer = alloc_escaped_string(&mysql, pconn->username);

  if (name_buffer != NULL) {
    /* select the password from the entry */
    str_result = my_snprintf(buffer, bufsize,
                             "select password from %s where name = '%s'",
                             auth_config.table.value, name_buffer);

    if (str_result < 0 || str_result >= bufsize || mysql_query(sock, buffer)) {
      freelog(LOG_ERROR, "db_load query failed for user: %s (%s)",
              pconn->username, mysql_error(sock));
      free_escaped_string(name_buffer);
      mysql_close(sock);
      return AUTH_DB_ERROR;
    }

    res = mysql_store_result(sock);
    num_rows = mysql_num_rows(res);
  
    /* if num_rows = 0, then we could find no such user */
    if (num_rows < 1) {
      mysql_free_result(res);
      free_escaped_string(name_buffer);
      mysql_close(sock);

      return AUTH_DB_NOT_FOUND;
    }
  
    /* if there are more than one row that matches this name, it's an error 
     * continue anyway though */
    if (num_rows > 1) {
      freelog(LOG_ERROR, "db_load query found multiple entries (%d) for user: %s",
              num_rows, pconn->username);
    }

    /* if there are rows, then fetch them and use the first one */
    row = mysql_fetch_row(res);
    mystrlcpy(pconn->server.password, row[0], sizeof(pconn->server.password));
    mysql_free_result(res);

    /* update the access time for this user */
    memset(buffer, 0, bufsize);
    str_result = my_snprintf(buffer, bufsize,
                             "update %s set accesstime=unix_timestamp(), "
                             "address='%s', logincount=logincount+1 "
                             "where strcmp(name, '%s') = 0",
                             auth_config.table.value, pconn->server.ipaddr,
                             name_buffer);

    free_escaped_string(name_buffer);
    name_buffer = NULL;

    if (str_result < 0 || str_result >= bufsize || mysql_query(sock, buffer)) {
      freelog(LOG_ERROR, "db_load update accesstime failed for user: %s (%s)",
              pconn->username, mysql_error(sock));
    }
  }

  mysql_close(sock);
#endif
  return AUTH_DB_SUCCESS;
}

#ifdef HAVE_AUTH
/**************************************************************************
 Creates escaped version of string.
**************************************************************************/
static char *alloc_escaped_string(MYSQL *mysql, const char *orig)
{
  int orig_len = strlen(orig);
  char *escaped = fc_malloc(orig_len*2+1);

  if (escaped == NULL) {
    freelog(LOG_ERROR, "Failed to allocate memory for escaped string %s", orig);
  } else {
    mysql_real_escape_string(mysql, escaped, orig, orig_len);
  }

  return escaped;
}

/**************************************************************************
 Frees escaped string created by alloc_escaped_string()
**************************************************************************/
static void free_escaped_string(char *str)
{
  if (str != NULL) {
    FC_FREE(str);
  }
}
#endif /* HAVE_AUTH */

/**************************************************************************
 Saves pconn fields to the database. If the username already exists, 
 replace the data.
**************************************************************************/
static bool auth_db_save(struct connection *pconn)
{
#ifdef HAVE_AUTH
  char buffer[1024] = "";
  const int bufsize = sizeof(buffer);
  char *name_buffer = NULL;
  char *pw_buffer = NULL;
  MYSQL *sock, mysql;
  int str_result;

  mysql_init(&mysql);

  /* attempt to connect to the server */
  if (!(sock = mysql_real_connect(&mysql, auth_config.host.value,
                                  auth_config.user.value, auth_config.password.value,
                                  auth_config.database.value,
                                  atoi(auth_config.port.value),
                                  NULL, 0))) {
    freelog(LOG_ERROR, "Can't connect to server! (%s)", mysql_error(&mysql));
    return FALSE;
  }

  name_buffer = alloc_escaped_string(&mysql, pconn->username);
  pw_buffer = alloc_escaped_string(&mysql, pconn->server.password);
  if (name_buffer == NULL || pw_buffer == NULL) {
    free_escaped_string(name_buffer);
    mysql_close(sock);
    return FALSE;
  }

  /* insert new user into table. we insert the following things: name
   * md5sum of the password, the creation time in seconds, the accesstime
   * also in seconds from 1970, the users address (twice) and the logincount */
  str_result = my_snprintf(buffer, bufsize,
                           "insert into %s values "
                           "(NULL, '%s', md5('%s'), NULL, "
                           "unix_timestamp(), unix_timestamp(),"
                           "'%s', '%s', 0)",
                           auth_config.table.value, name_buffer, pw_buffer,
                           pconn->server.ipaddr, pconn->server.ipaddr);

  /* Password is not needed for further queries. */
  free_escaped_string(pw_buffer);
  pw_buffer = NULL;

  if (str_result < 0 || str_result >= bufsize || mysql_query(sock, buffer)) {
    freelog(LOG_ERROR, "db_save insert failed for new user: %s (%s)",
                       pconn->username, mysql_error(sock));
    mysql_close(sock);
    return FALSE;
  }

  /* insert an entry into our log */
  memset(buffer, 0, bufsize);
  str_result = my_snprintf(buffer, bufsize,
                           "insert into %s (name, logintime, address, succeed) "
                           "values ('%s',unix_timestamp(),'%s', 'S')",
                           auth_config.login_table.value,
                           name_buffer, pconn->server.ipaddr);

  free_escaped_string(name_buffer);
  name_buffer = 0;

  if (str_result < 0 || str_result >= bufsize || mysql_query(sock, buffer)) {
    freelog(LOG_ERROR, "db_load insert loginlog failed for user: %s (%s)",
                       pconn->username, mysql_error(sock));
  }

  mysql_close(sock);
#endif
  return TRUE;
}
