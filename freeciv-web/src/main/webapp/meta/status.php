meta-status;<?php
// this status.php file tells publite2 about the number of available freeciv-web servers in the metaserver.
ini_set("include_path", ini_get("include_path") . ":" . $_SERVER["DOCUMENT_ROOT"]);

include_once("php_code/settings.php");

if ($error_msg != NULL) {
  $config_problem = true;
}

if (!isset($config_problem)) {
  include_once("php_code/php_code_find.php");
  // includes for support routines
  include_once(php_code_find("fcdb.php"));
  include_once(php_code_find("versions_file.php"));
  include_once(php_code_find("html.php"));

  fcdb_metaserver_connect();
}
      
$stmt = "select count(*) as count from servers UNION ALL select count(*) as count from servers where "
	. "type = 'singleplayer' and state = 'Pregame' and humans = '0' and stamp >= DATE_SUB(NOW(), INTERVAL 1 MINUTE) "
	. "UNION ALL select count(*) as count from servers where type = 'multiplayer' and state = 'Pregame' and stamp >= DATE_SUB(NOW(), INTERVAL 1 MINUTE) "
        . "UNION ALL select count(*) as count from servers where type = 'pbem' and state = 'Pregame' and stamp >= DATE_SUB(NOW(), INTERVAL 1 MINUTE)";
$res = fcdb_exec($stmt);
$nr = fcdb_num_rows($res);
if ( $nr != 4 ) {
  print "error";
} else {
  $row = fcdb_fetch_next_row($res, 0);
  print db2html($row["count"]);
  print (";");      
  $row = fcdb_fetch_next_row($res, 1);
  print db2html($row["count"]);
  print (";");
  $row = fcdb_fetch_next_row($res, 2);
  print db2html($row["count"]);
  print (";");
  $row = fcdb_fetch_next_row($res, 3);
  print db2html($row["count"]);

} 
?>
