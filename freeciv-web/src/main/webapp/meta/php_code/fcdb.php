<?php

// MySQL is the only one really working at the moment...
$fcdb_sel = "MySQL";

function fcdb_error_handler($errno, $errstr) {

}

// please use these instead of fcdb_connect wherever possible

function fcdb_default_connect() {
  global $fcdb_default_db, $fcdb_username, $fcdb_pw;

  return fcdb_connect($fcdb_default_db, $fcdb_username, $fcdb_pw);
}

function fcdb_metaserver_connect() {
  global $fcdb_metaserver_db, $fcdb_username, $fcdb_pw;

  return fcdb_connect($fcdb_metaserver_db, $fcdb_username, $fcdb_pw);
}

$dbhost = '';

function fcdb_connect($db, $un, $pw) {
  global $fcdb_sel, $fcdb_conn;
  global $dbhost;

  $ok = true;

  set_error_handler("fcdb_error_handler");
  switch ($fcdb_sel) {
    case "PostgreSQL":
      $fcdb_conn = pg_Connect("dbname=$db port=5432 user=$un");
      if (!$fcdb_conn) {
	build_fcdb_error("I cannot make a connection to the database server.");
      }
      $ok = false;
      break;
    case "MySQL":
      $fcdb_conn = new PDO("mysql:host=$dbhost;dbname=$db;charset=utf8",
                           "$un", "$pw", 
                           array(PDO::ATTR_EMULATE_PREPARES => false,
                                 PDO::ATTR_ERRMODE => PDO::ERRMODE_SILENT));
      if (!$fcdb_conn) {
        build_fcdb_error("I cannot open the database.");
	$ok = false;
      }
      break;
  }
  restore_error_handler();

  return $ok;
}

function fcdb_exec($stmt) {
  global $fcdb_sel, $fcdb_conn;
  set_error_handler("fcdb_error_handler");
  switch ($fcdb_sel) {
    case "PostgreSQL":
      $res = pg_Exec($fcdb_conn, $stmt);
      if (!$res) {
	build_fcdb_error("I cannot run a statement: '$stmt'.");
      }
      break;
    case "MySQL":
      $res = $fcdb_conn->query($stmt);
      if (!$res) {
	build_fcdb_error("I cannot run a statement: '$stmt'.");
      }
      break;
  }
  restore_error_handler();
  return ($res);
}

function fcdb_query_single_value($stmt) {
  global $fcdb_sel, $fcdb_conn;
  set_error_handler("fcdb_error_handler");
  switch ($fcdb_sel) {
    case "PostgreSQL":
      $res = pg_Exec($fcdb_conn, $stmt);
      if (!$res) {
	build_fcdb_error("I cannot run a query: '$stmt'.");
      }
      $val = pg_Result($res, 0, 0);
      break;
    case "MySQL":
      $res = $fcdb_conn->query($stmt);
      if (!$res) {
	build_fcdb_error("I cannot run a query: '$stmt'.");
      }
      $res->bindColumn(0, $val);
      $res->fetch(PDO::FETCH_BOUND);
      break;
  }
  restore_error_handler();
  return ($val);
}

function fcdb_num_rows($res) {
  global $fcdb_sel, $fcdb_conn;
  set_error_handler("fcdb_error_handler");
  switch ($fcdb_sel) {
    case "PostgreSQL":
      $rows = pg_NumRows($res);
      break;
    case "MySQL":
      $rows = $res->rowCount();
      break;
  }
  restore_error_handler();
  return ($rows);
}

function fcdb_fetch_next_row($res, $inx) {
  global $fcdb_sel, $fcdb_conn;
  set_error_handler("fcdb_error_handler");
  switch ($fcdb_sel) {
    case "PostgreSQL":
      $arr = pg_Fetch_Array($res, $inx);
      break;
    case "MySQL":
      $arr = $res->fetch(PDO::FETCH_ASSOC);
      break;
  }
  restore_error_handler();
  return ($arr);
}

function fcdb_mktime($time_str) {
  global $fcdb_sel;
  set_error_handler("fcdb_error_handler");
  switch ($fcdb_sel) {
    case "PostgreSQL":
      $part = split("[- :]", $time_str);
      $time_sec =
	mktime($part[3], $part[4], $part[5], $part[1], $part[2], $part[0]);
      break;
    case "MySQL":
      $ok =
	ereg("([0-9]{4})([0-9]{2})([0-9]{2})([0-9]{2})([0-9]{2})([0-9]{2})",
	      $time_str, $part);
      if ($ok) {
	$time_sec =
	  mktime($part[4], $part[5], $part[6], $part[2], $part[3], $part[1]);
      } else {
	$part = split("[- :]", $time_str);
	$time_sec =
	  mktime($part[3], $part[4], $part[5], $part[1], $part[2], $part[0]);
      }
      break;
  }
  restore_error_handler();
  return ($time_sec);
}

function addneededslashes_db($str) {
  global $fcdb_conn;

  if (get_magic_quotes_gpc()) {
    // Get rid of automatically added slashes first. We want to set them correctly!
    $str = stripslashes($str);
  }

  return $fcdb_conn->quote($str);
}

// Store error message. Don't send it yet as HTTP headers must be sent first.
function build_fcdb_error($what_error) {
  global $webmaster_html;
  global $webmaster_default;
  global $error_msg;
  global $fcdb_conn;

  if (! $webmaster_default) {
    $wmpart = ", $webmaster_html";
  } else {
    $wmpart = "";
  }

  // FIXME: There should be no mysql_error() call if we are using
  //        some other db engine.
  $error_msg = "<table border=\"1\" style=\"font-size:xx-small\">\n" .
               "<tr><th>$what_error</th><tr>\n" .
               "<tr><td>" . $fcdb_conn->errorInfo() . "</td></tr>" .
               "<tr><td>" .
               "Please contact the maintainer" . $wmpart .
               ".</td></tr>\n</table></font>\n";
}

?>
