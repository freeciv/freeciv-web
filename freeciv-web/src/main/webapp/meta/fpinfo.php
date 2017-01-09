<?php header('Expires: '.gmdate('D, d M Y H:i:s \G\M\T', time() + 3600)); ?>
<?php
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
  include_once(php_code_find("html.php"));

  fcdb_metaserver_connect();
}
      
$stmt = "select (select count(*) from servers where state = 'Running') as running_count, (select SUM(gameCount) from games_played_stats where gametype IN (0, 5)) as single_count, (select SUM(gameCount) from games_played_stats where gametype IN (1, 2)) as multi_count";
$res = fcdb_exec($stmt);
$row = fcdb_fetch_next_row($res, 0);
print db2html($row["running_count"]);
print(";");
print db2html($row["single_count"]);
print(";");
print db2html($row["multi_count"]);
?>
