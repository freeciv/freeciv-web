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
      
$stmt = "select winner, count(winner) as wins from game_results group by winner order by wins desc limit 5;";
$res = fcdb_exec($stmt);

$nr = fcdb_num_rows($res);
if ( $nr > 0 ) {
  for ( $inx = 0; $inx < $nr; $inx++ ) {
    $row = fcdb_fetch_next_row($res, $inx);
    $winner = db2html($row["winner"]);
    $wins = db2html($row["wins"]);
    print($winner . "," . $wins . ";\n");
  }
}

?>
