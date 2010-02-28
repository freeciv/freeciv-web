<?php

function versions_current() {
  static $retver = "";
  if ($retver == "") {
    $retver = versions_file_nth_line(1);
  }
  return ($retver);
}

function versions_next_likely() {
  static $retver = "";
  if ($retver == "") {
    $retver = versions_file_nth_line(2);
  }
  return ($retver);
}

function versions_file_nth_line($n) {
  global $versions_file;

  $retlin = "?.?.?";
  $fp = fopen($versions_file, 'r');
  if ($fp) {
    $ln = 0;
    while ($line = fgets($fp, 200)) {
      if (!(ereg("^#", $line))) {
	$ln++;
	if ($ln == $n) {
	  $retlin = chop($line);
	  break;
	}
      }
    }
    fclose($fp);
  } else {
    $retlin = "not known";
  }
  return ($retlin);
}

?>
