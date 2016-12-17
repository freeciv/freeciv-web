<?php

$debug=0;

// include the php-code finder
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
  include_once(php_code_find("img.php"));
  include_once(php_code_find("html.php"));

  fcdb_metaserver_connect();
}

      $stmt="select distinct statsDate as sd, (select gameCount from games_played_stats where statsDate = sd and gameType = '0') as web_single, (select gameCount from games_played_stats where statsDate = sd and gameType = '1') as web_multi, (select gameCount from games_played_stats where statsDate = sd and gameType = '2') as web_pbem, (select gameCount from games_played_stats where statsDate = sd and gameType = '4') as web_hotseat, (select gameCount from games_played_stats where statsDate = sd and gameType = '3') as desktop_multi, (select gameCount from games_played_stats where statsDate = sd and gameType = '5') as web_single_3d from games_played_stats;";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr > 0 ) {

        for ( $inx = 0; $inx < $nr; $inx++ ) {
 	  $row = fcdb_fetch_next_row($res, $inx);

          print db2html($row["sd"]);
          print ",";
          print db2html($row["web_single"]);
          print ",";
          print db2html($row["web_multi"]);
          print ",";
          print db2html($row["web_pbem"]);
          print ",";
          print db2html($row["desktop_multi"]);
          print ",";
          print db2html($row["web_hotseat"]);
          print ",";
          print db2html($row["web_single_3d"]);
          print ";";


        }
      } else {
        print "ops. error.";
      }

?>
