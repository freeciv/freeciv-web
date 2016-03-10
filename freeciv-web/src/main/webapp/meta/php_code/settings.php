<?php

$error_msg = NULL;

if (file_exists('php_code/local.php')) {
  $localsettings = fopen('php_code/local.php', 'r');
}

if ($localsettings != NULL) {
  include_once('php_code/local.php');
}

if (!isset($metaserver_url_path)) {
  // Server root
  $metaserver_url_path = '';
}

if (!isset($metaserver_root)) {
  $metaserver_root = '.' . $metaserver_url_path;
}

if (!isset($versions_file)) {
  $versions_file = $metaserver_root . 'versions';
}

if (!isset($fcdb_default_db)) {
  $fcdb_default_db = 'freecivmetaserver';
}

if (!isset($fcdb_metaserver_db)) {
  $fcdb_metaserver_db = 'freecivmetaserver';
}

if (!isset($fcdb_username)) {
  $fcdb_username = 'root';
}

if (!isset($fcdb_pw)) {
  $fcdb_pw = 'changeme';
}

if ($pic_paths == NULL) {
  // Search pics under metaserver directory and under server root.
  $pic_paths = array($metaserver_root . 'pics/' => $metaserver_url_path . 'pics/',
                     $_SERVER[DOCUMENT_ROOT] . 'pics/' => 'pics/');
}

if (!isset($webmaster_name)) {
  $webmaster_name = 'Webmaster';
  $webmaster_default = true;
}

//if (!isset($webmaster_addr) && !isset($webmaster_email)) {
//  $webmaster_addr = "mailto:$webmaster_email";
//}

if (!isset($webmaster_addr)) {
  $webmaster_html = $webmaster_name;
} else {
  $webmaster_html = "<A HREF=\"$webmaster_addr\">$webmaster_name</A>";
}

if (!isset($bugs_addr)) {
  //$bugs_html = $bugs_name;
} else {
  if ($bugs_name == NULL) {
    $bugs_link = 'to this address';
  } else {
    $bugs_link = $bugs_name;
  }

  $bugs_html = "<A HREF=\"$bugs_addr\">$bugs_link</A>";
}

if ($metaserver_header == NULL) {
  $metaserver_header = 'Freeciv-web multiplayer games around the world';
}

// Check configuration
if (! file_exists($metaserver_root . '/php_code/php_code_find.php')) {

  $error_msg = "<table border=\"1\" style=\"font-size:xx-small\">\n" .
               "<tr><th>Metaserver installation problem.</th><tr>\n" .
               "<tr><td>" . $lspart . "</td></tr>" . 
               "<tr><td>" .
               "Please contact the maintainer" . $wmpart .
               ".</td></tr>\n</table></font>\n";
}

?>
