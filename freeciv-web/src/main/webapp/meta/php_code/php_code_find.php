<?php

// Return path to PHP code file.
// (Search for "local" file; fallback to "default" file.)

  // includes for support routines
  //include_once("php_code/fallback_find_file.php");
  include_once($metaserver_root . '/php_code/fallback_find_file.php');

function php_code_find($name, $reldir = ".") {

  $file = dirname($_SERVER['SCRIPT_FILENAME']) . "/$reldir/php_code/$name";
  if (my_file_exists($file)) {
    return $file;
  }

  $file = dirname(__FILE__) . "/$name";
  if (my_file_exists($file)) {
    return $file;
  }

  return $_SERVER[DOCUMENT_ROOT]."/php_code/$name";
}

?>
