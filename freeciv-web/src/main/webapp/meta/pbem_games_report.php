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
      
$stmt = "select winner, playerOne as one, playerTwo as two, endDate, (select count(*) from game_results where winner = one) as wins_by_one, (select count(*) from game_results where winner = two) as wins_by_two from game_results  order by id desc limit 20;";
$res = fcdb_exec($stmt);

$nr = fcdb_num_rows($res);
if ( $nr > 0 ) {
  for ( $inx = 0; $inx < $nr; $inx++ ) {
    $row = fcdb_fetch_next_row($res, $inx);
    $endDate = db2html($row["endDate"]);
    $winner = db2html($row["winner"]);
    $one = db2html($row["one"]);
    $two = db2html($row["two"]);
    $wins_one = db2html($row["wins_by_one"]);
    $wins_two = db2html($row["wins_by_two"]);
    print($endDate . "," . $winner . "," . $one . "," . $two . "," . $wins_one . "," . $wins_two . ";\n");
  }
}

?>
