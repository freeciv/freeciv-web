<?php

// Return path to any file.
// (Search for "local" file; fallback to "default" file.)

// it turns out include_once() tests for the literal filename,
// not the inode or an internal list of included files,
// so it doesn't actually work.  grr.
//
if (isset($included_fallback_find_file_php)) {
  return(true);
}
$included_fallback_find_file_php = true;

function fallback_find_file($name, $reldir = ".") {
  $dir = dirname($_SERVER[SCRIPT_FILENAME]);
  $fil = "$dir/$reldir/$name";
  if (my_file_exists($fil)) {
    return "$fil";
  } else {
    return $_SERVER[DOCUMENT_ROOT]."/$name";
  }
}

// let's put some additional file finding functions in here - rp

// for unknown reasons, file_exists() fails on relative paths now
// this is the fix

function my_file_exists($filename) {
  if (!strcmp(substr($filename,0,1),'/')) {
    return file_exists($filename);
  } else {
    return file_exists(getcwd() . '/' . $filename);
  }
}

?>
