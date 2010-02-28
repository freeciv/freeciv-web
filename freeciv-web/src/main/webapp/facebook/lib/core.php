<?php

define ('PASSWORD_PLACEHOLDER', '********');

// if this is set on by your webserver, then client access doesn't work
ini_set('zend.ze1_compatibility_mode', 0);

function is_config_setup() {
  return (get_api_key() && get_api_secret() &&
          get_api_key() != 'YOUR_API_KEY' &&
          get_api_secret() != 'YOUR_API_SECRET' &&
          get_callback_url() != null);
}

// Whether the site is "connected" or not
function is_fbconnect_enabled() {
  if (!is_config_setup()) {
    return false;
  }

  // Change this if you want to turn off Facebook connect
  return true;
}
function get_api_key() {
  return idx($GLOBALS, 'api_key');
}
function get_api_secret() {
  return idx($GLOBALS, 'api_secret');
}
function get_base_fb_url() {
  return idx($GLOBALS, 'base_fb_url');
}
function get_static_root() {
  return 'http://static.ak.'.get_base_fb_url();
}
function get_feed_bundle_id() {
  return idx($GLOBALS, 'feed_bundle_id');
}
function get_callback_url() {
  return idx($GLOBALS, 'callback_url');
}
function get_current_url() {
  $script_uri = idx($_SERVER, 'SCRIPT_URI');
  return $script_uri ? $script_uri : idx($_SERVER, 'SERVER_NAME');
}

/*
 * Like sprintf, but does some extra stuff to prevent SQL injection.
 *
 */
function squeryf($conn, $sql /*, args ... */) {
  $argv = func_get_args();  // variable length function arguments
  $argc = count($argv);     // number of argumentns total
  $sql_params = array();    // container for sql parameters

  if ($argc > 2) {
    for ($x=2; $x<$argc; $x++) {                 // get all optional params starting from the third parameter to the last (however many)
      if (is_string($argv[$x])) {               // check for string type
        // w/ string quote handlers x1000 = 0.0937 vs 0.0824 without

        $sql_str = $argv[$x];
        $sql_str = mysql_real_escape_string($sql_str, $conn); // use conn_real_escape_string c/api escaping for: \x00, \n, \r, \, ', " and \x1a
        $sql_params[] = '\''.$sql_str.'\'';     // add quotes surrounding string params

      } elseif (is_scalar($argv[$x])) {         // check for int/float/bool

        $sql_params[] = $argv[$x];              // don't do anything to int types, they are safe
      } else {                                  // unsupported type (array, object, resource, null)
        $bad_param = str_replace("\n", '', var_export($argv[$x], true));    // capture variable info for debug msg
        error_log("MYSQL_QUERYF: non-scalar type for SQL query parameter $x: ".$argv[$x].", var: $bad_param, sql: [$sql]");
        return false;
      }
    }


    $ok_sql = vsprintf($sql, $sql_params);    // use vsprintf to merge parameters and the query string

    // error_log("sql = " . $ok_sql . " with sql " . $sql . "  and params " . print_r($sql_params,true));

    if ($ok_sql=='') {  // if blank this is because of a parameter count mismatch (maybe even something else)
      error_log("MYSQL_QUERYF: SQL query parameter missing, sprintf failed on: $sql");
    }
  } else {
    $ok_sql = str_replace('%%', '%', $sql);
  }

  return $ok_sql;
}

/*
 * Run a mysql query.
 */
function queryf($sql /*,  args */) {
  $conn = db_get_conn();
  if (!$conn) {
    return null;
  }

  $args = func_get_args();
  array_unshift($args, $conn);
  $sql = call_user_func_array('squeryf', $args);

  return mysql_query($sql, $conn);
}

function db_get_conn() {
  // Create your own function in lib/config.php to
  // override this guy
  if (function_exists('db_get_custom_conn')) {
    return db_get_custom_conn();
  }

  static $conn = null;
  if ($conn === null) {
    $conn = mysql_connect($GLOBALS['db_host'],
                          $GLOBALS['db_user'],
                          $GLOBALS['db_pass']);
    if (!$conn) {
      error_log('Could not connect to db.');
      return $conn;
    }

    mysql_select_db($GLOBALS['db_name']);
  }
  return $conn;
}

function go_home() {
  header('Location: index.php');
}

/**
 *  Returns $arr[$idx], because php doesn't let you index into
 *  arrays returned from a function.
 *
 *  a()[0] doesn't work
 *
 *  idx(a(), 0) does.
 *
 *  PHP is a pretty stupid language.
 *
 *  @param    array to index into
 *  @param    index. if negative, start at the end.
 *  @param    default to return if $arr[$idx] is not set
 *  @return   array[index]
 */
function idx($arr, $idx, $default=null) {
  if ($idx === null || !is_array($arr)) {
    return $default;
  }
  $idx = $idx >= 0 ? $idx : count($arr) + $idx;
  return array_key_exists($idx, $arr) ? $arr[$idx] : $default;
}

/*
 * Extract the domain from a relatively-well-formed URL
 */
function get_domain($url) {
  $info = parse_url($url);
  if (isset($info['host'])) {
    return $info['host'];
  }
  return $info['path'];
}

function no_magic_quotes($val) {
  if (get_magic_quotes_gpc()) {
    return stripslashes($val);
  } else {
    return $val;
  }
}

function parse_http_args($http_params, $keys) {
  $result = array();
  foreach ($keys as $key) {
    $result[$key] = no_magic_quotes(idx($http_params, $key));
  }
  return $result;
}
