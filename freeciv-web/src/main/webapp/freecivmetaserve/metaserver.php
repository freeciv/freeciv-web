<?php

/* do we want debug output to stderr?
 * This is very heavy so never leave it on in production
 */
$debug=0;

// include the php-code finder
ini_set("include_path", ini_get("include_path") . ":" . $_SERVER["DOCUMENT_ROOT"]);

include_once("php_code/settings.php");

if ($error_msg != NULL) {
  $config_problem = true;
}

if (! $config_problem) {
  include_once("php_code/php_code_find.php");
  // includes for support routines
  include_once(php_code_find("fcdb.php"));
  include_once(php_code_find("versions_file.php"));
  include_once(php_code_find("img.php"));
  include_once(php_code_find("html.php"));

  fcdb_metaserver_connect();
}

$fullself="http://".$_SERVER["SERVER_NAME"].$_SERVER["PHP_SELF"];

$posts = array(
  "host",
  "port",
  "bye",
  "version",
  "patches",
  "capability",
  "state",
  "topic",
  "message",
  "serverid",
  "vn",
  "vv",
  "plrs",
  "plt",
  "pll",
  "pln",
  "plu",
  "plh",
  "dropplrs",
  /* URL line cgi parameters */
  "server_port",
  "client",
  "client_cap",
  "rss"
);

/* This is where we store what variables we can collect from the server
 * If we want to add variables, they need to be here, and new columns
 * need to be added to the database. They will also be sent to the client */
$sqlvars = array(
  "version",
  "patches",
  "capability",
  "state",
  "topic",
  "message",
  "serverid"
);

/* this little block of code "changes" the namespace of the variables 
 * we got from the $_REQUEST variable to a local scope */
$assoc_array = array();
foreach($posts as $val) {
  if (isset($_REQUEST[$val])) {
    $assoc_array[$val] = $_REQUEST[$val];
  }
}
extract($assoc_array);


if ( isset($port) ) {
  /* All responses to the server will be text */
  header("Content-Type: text/plain; charset=\"utf-8\"");

  /* garbage port */
  if (!is_numeric($port) || $port < 1024 || $port > 65535) {
    print "exiting, garbage port \"$port\"\n";
    exit(1);
  }

  /* This is to check if the name they have supplied matches their IP */
    /* Maybe they have a proxy they can't get around */
  /* FIXME: these don't work.  - Andreas
  if ( isset($_SERVER["HTTP_X_FORWARDED_FOR"]) ) {
    $ip = $_SERVER["HTTP_X_FORWARDED_FOR"];
  } elseif ( isset($_SERVER["HTTP_CLIENT_IP"]) ) {
    $ip = $_SERVER["HTTP_CLIENT_IP"];
  } else {
    $ip = $_SERVER["REMOTE_ADDR"];
  }

  if (gethostbyname($host) != $ip) {
    $host = @gethostbyaddr($ip);
  }*/

  /* is this server going away? */
  if (isset($bye)) {
    $stmt="delete from servers where host=\"$host\" and port=\"$port\"";
    print "$stmt\n";
    $res = fcdb_exec($stmt);
    $stmt="delete from variables where hostport=\"$host:$port\"";
    print "$stmt\n";
    $res = fcdb_exec($stmt);
    $stmt="delete from players where hostport=\"$host:$port\"";
    print "$stmt\n";
    $res = fcdb_exec($stmt);
    print "Thanks, please come again!\n";
    exit(0); /* toss all entries and exit */
  }

  if (isset($topic)) {
    $topic = addneededslashes($topic); /* escape stuff to go into the database */
  }
  if (isset($message)) {
    $message = addneededslashes($message); /* escape stuff to go into the database */
  }
  if (isset($serverid)) {
    $serverid = addneededslashes($serverid); /* escape stuff to go into the database */
  }


  /* lets get the player information arrays if we were given any */
  $playerstmt = array();
  if (isset($plu)) {
    for ($i = 0; $i < count($plu); $i++) { /* run through all the names */
      $ins = "insert into players set hostport=\"$host:$port\", ";

      if (isset($plu[$i]) ) {
        $plu[$i] = addneededslashes($plu[$i]);
        $ins .= "user=\"$plu[$i]\", ";
      }
      if (isset($pll[$i]) ) {
        $pll[$i] = addneededslashes($pll[$i]);
        $ins .= "name=\"$pll[$i]\", ";
      }
      if (isset($pln[$i]) ) {
        $pln[$i] = addneededslashes($pln[$i]);
        $ins .= "nation=\"$pln[$i]\", ";
      }
      if (isset($plt[$i]) ) {
        $plt[$i] = addneededslashes($plt[$i]);
        $ins .= "type=\"$plt[$i]\", ";
      }
      $ins .= "host=\"$plh[$i]\"";
      /* an array of all the sql statements; save actual db access to the end */
      debug("\nINS = $ins\n\n");
      array_push($playerstmt, $ins); 
    }
  }

  /* delete this variables that this server might have already set. */
  $stmt="delete from variables where hostport=\"$host:$port\"";
  $res = fcdb_exec($stmt);

  /* lets get the variable arrays if we were given any */
  $variablestmt = array();
  if (isset($vn)) {
    for ($i = 0; $i < count($vn); $i++) { /* run through all the names */
      $vn[$i] = addneededslashes($vn[$i]);
      $vv[$i] = addneededslashes($vv[$i]);
      $ins = "insert into variables set hostport=\"$host:$port\", ";
      $ins .= "name=\"$vn[$i]\", ";
      $ins .= "value=\"$vv[$i]\"";
      /* an array of all the sql statements; save actual db access to the end */
      array_push($variablestmt, $ins);
    }
  }

  $stmt = "select * from servers where host=\"$host\" and port=\"$port\"";
  $res = fcdb_exec($stmt);

  /* do we already have an entry for this host:port combo? */
  if (fcdb_num_rows($res) == 1) {
    /* so this is an update */
    $string = array();
    $stmt = "update servers set ";

    /* iterate through the vars to build a list of things to update */
    foreach ($sqlvars as $var) {
      if (isset($assoc_array[$var])) {
        array_push($string, "$var=\"$assoc_array[$var]\"");
      }
    }

    /* we always want to update the timestamp */
    array_push($string, "stamp=now() ");

    $stmt .= join(", ", $string); /* put them all together */
    $stmt .= "where host=\"$host\" and port=\"$port\"";
  } else {
    /* so this is a brand new server and is an insert */
    $string = array();

    foreach($sqlvars as $var) {
        array_push($string, "$var=\"$assoc_array[$var]\"");
    }

    /* we always want to update the timestamp */
    array_push($string, "stamp=now() ");

    $stmt= " insert into servers set host=\"$host\", port=\"$port\", ";
    $stmt .= join(", ", $string); /* put them all together */
  }

  print "$stmt\n"; /* server statement */

  /* Do all the processing above, we now hit the database */
  $res = fcdb_exec($stmt);

  for ($i = 0; $i < count($variablestmt); $i++) {
    print "$variablestmt[$i]\n";
    $res = fcdb_exec($variablestmt[$i]);
  }

  /* if we have a playerstmt array we want to zero out the players
   * and if the server wants to explicitly tell us to drop them all */
  if (count($playerstmt) > 0 || isset($dropplrs)) { 
    $delstmt = "delete from players where hostport=\"$host:$port\"";

    print "$delstmt\n";

    $res = fcdb_exec($delstmt);

    /* if dropplrs=1 then set available back to 0 */
    if (isset($dropplrs)) {
      $avstmt = "update servers set available=0 where host=\"$host\" and port=\"$port\"";
      $res = fcdb_exec($avstmt);
    }

    for ($i = 0; $i < count($playerstmt); $i++) {
      print "$playerstmt[$i]\n";
      $res = fcdb_exec($playerstmt[$i]);
    }
  }

  /* We've done the database so we're done */

} elseif ( isset($client_cap) || isset($client) ) {
  $stmt="select * from servers where message like '%Multiplayer%' order by host,port asc";
  $res = fcdb_exec($stmt);
  $nr = fcdb_num_rows($res);
  $nservers=0;
  $output="";
  if ( $nr > 0 ) {
    for ($inx = 0; $inx < $nr; $inx++) {
      $row = fcdb_fetch_array($res, $inx);
      // debug("db = \"".$row["capability"]." \" vs \"$client_cap\"\n");
      /* we only want to show compatable servers */
      if ( $client == "all"  || 
           (has_all_capabilities(mandatory_capabilities($row["capability"]),$client_cap) &&
            has_all_capabilities(mandatory_capabilities($client_cap),$row["capability"]) ) ) {
        $output .= "[server$nservers]\n";
        $nservers++;
        $output .= sprintf("host = \"%s\"\n", $row["host"]);
        $output .= sprintf("port = \"%s\"\n", $row["port"]);
        /* the rest of the vars from the database */
        foreach($sqlvars as $var) {
          $noquote=str_replace("\"","",$row[$var]); /* some " were messing it up */
          $output .= "$var = \"$noquote\"\n";
        }

        $stmt="select * from players where hostport=\"".$row["host"].":".$row["port"]."\" order by name";
        $res1 = fcdb_exec($stmt);
        $nr1 = fcdb_num_rows($res1);
        $output .= "nplayers = \"$nr1\"\n";
        if ($nr1 > 0) {
          $output .= "player = { \"name\", \"user\", \"nation\", \"type\", \"host\"\n";
          for ($i = 0; $i < $nr1; $i++) {
            $prow = fcdb_fetch_array($res1, $i);
            $output .= sprintf(" \"%s\", ", stripslashes($prow["name"]));
            $output .= sprintf("\"%s\", ", stripslashes($prow["user"]));
            $output .= sprintf("\"%s\", ", stripslashes($prow["nation"]));
            $output .= sprintf("\"%s\", ", stripslashes($prow["type"]));
            $output .= sprintf("\"%s\"\n", stripslashes($prow["host"]));
          }
          $output .= "}\n";
        }

        $stmt="select * from variables where hostport=\"".$row["host"].":".$row["port"]."\"";
        $res2 = fcdb_exec($stmt);
        $nr2 = fcdb_num_rows($res2);
        if ($nr2 > 0) {
          $output .= "vars = { \"name\", \"value\"\n";
          for ($i = 0; $i < $nr2; $i++) {
            $vrow = fcdb_fetch_array($res2, $i);
            $output .= sprintf(" \"%s\", ", $vrow["name"]);
            $output .= sprintf("\"%s\"\n ", $vrow["value"]);
          }
          $output .= "}\n";
        }
        $output .= "\n";
      }
    }
    $output .= "[main]\n";
    $output .= "nservers = $nservers\n\r\n";

    /* All responses to the client will be in Freeciv's ini format */
    //header("Content-Type: text/x-ini");
    header("Content-Type: text/plain; charset=\"utf-8\"");

    header("Content-Length: " . strlen($output));
    print $output;
  }
} else {

  header("Content-Type: text/html; charset=\"utf-8\"");

?>
<html>
<head>
<title>Freeciv.net - online multiplayer strategy game</title>

<link href="/stylesheets/frontpage.css" rel="stylesheet" type="text/css" />

<?php if ($_COOKIE["facebook_mode"] == "true") { ?>
  <link href="/stylesheets/fb_frontpage.css" rel="stylesheet" type="text/css" />
<? } ?>


<link rel="shortcut icon" href="/images/freeciv-forever-icon.png" />

<script type="text/javascript" src="/javascript/jquery-1.3.2.min.js"></script>

</head>
<body link="white" vlink="white" alink="white" bgcolor="black">
<!-- HEADER -->
<div id="container">

<div id="header">

<div id="header_menu"> <a class="menu_link" href="/" title="News about Freeciv.net">News</a> &nbsp;&nbsp; 
<a class="menu_link" href="/wireframe.jsp?do=login" title="Login to play Freeciv.net now">Play Now</a> &nbsp;&nbsp; 
<a class="menu_link" href="/freecivmetaserve/metaserver.php" title="Multiplayer Games">Games</a> &nbsp;&nbsp; 
<a class="menu_link" href="http://www.freeciv.net/tournament/" title="Freeciv Tournament">Tournament</a> &nbsp;&nbsp; 
<a class="menu_link" href="/wiki" title="Documentation">Documentation</a> &nbsp;&nbsp; 
<a class="menu_link" href="/forum/" title="Freeciv.net Forum">Forum</a> &nbsp;&nbsp;
<a class="menu_link" title="Contribute to the Freeciv.net development" href="/wireframe.jsp?do=dev">Development</a> &nbsp;&nbsp;
<a title="About Freeciv.net" class="menu_link" href="/wireframe.jsp?do=about">About</a></div>
</div>

<div id="body_content">
<br>

<div id="main_column">


<div>
<?php

  if ($error_msg != NULL) {
    echo $error_msg;
  } else {
    if (isset($server_port)) {
 

      $port = substr(strrchr($server_port, ":"), 1);
      $host = substr($server_port, 0, strlen($server_port) - strlen($port) - 1);
      print "<h1>Freeciv.net server id: " . db2html($port) . "</h1>\n";
      print "Freeciv.net is an Open Source strategy game which can be played online against other players, or in single player mode against AI opponents.<br><br>";

      
      $stmt = "select * from servers where host=\"$host\" and port=\"$port\"";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr != 1 ) {
        print "Cannot find the specified server";
      } else {
        $row = fcdb_fetch_array($res, 0);
        
        if ($row["state"] == "Pregame") {
          print "<div><a href='/civclientlauncher?civserverport=" . db2html($port) . "&amp;civserverhost=" . db2html($host)
             . "'><img border='0' title='Join this game now' src='/images/join.png'/></a> <b>You can join this game now.</b></div>";
	}
        print "<div><a href='/civclientlauncher?action=observe&civserverport=" . db2html($port) . "&amp;civserverhost=" . db2html($host)
             . "'><img border='0' title='Join this game now' src='/images/observe.png'/></a> <b>You can observe this game now.</b></div>";

        print "<br/><br/>";
        $msg = db2html($row["message"]);
        print "<table><tr id='meta_header'><th>Version</th><th>Patches</th><th>Capabilities</th>";
        print "<th>State</th>";
        print "<th>Server ID</th></tr>\n";
        print "<tr id='meta_row'><td>";
        print db2html($row["version"]);
        print "</td><td>";
        print db2html($row["patches"]);
        print "</td><td>";
        print db2html($row["capability"]);
        print "</td><td>";
        print db2html($row["state"]);
        print "</td><td>";
        print db2html($row["serverid"]);
        print "</td></tr>\n</table></p>\n";
        if ($msg != "") {
          print "<p>";
          print "<table><tr id='meta_header'><th>Message</th></tr>\n";
          print "<tr><td>" . $msg . "</td></tr>";
          print "</table></p>\n";
        }
        $stmt="select * from players where hostport=\"$server_port\" order by name";
        $res = fcdb_exec($stmt);
        $nr = fcdb_num_rows($res);
        if ( $nr > 0 ) {
          print "<p><div><table style=\"width: 60%;\">\n";
          print "<tr id='meta_header'><th class=\"left\">Leader</th><th>Nation</th>";
          print "<th>User</th><th>Type</th><th>Host</th></tr>\n";
          for ( $inx = 0; $inx < $nr; $inx++ ) {
            $row = fcdb_fetch_array($res, $inx);
            print "<tr id='meta_row'><td class=\"left\">";
            print db2html($row["name"]);
            print "</td><td>";
            print db2html($row["nation"]);
            print "</td><td>";
            print db2html($row["user"]);
            print "</td><td>";
            print db2html($row["type"]);
            print "</td><td>";
            print db2html($row["host"]);
            print "</td></tr>\n";
          }
          print "</table></div><p>\n";
        } else {
          print "<p>No players</p>\n";
        }
        $stmt="select * from variables where hostport=\"$server_port\"";
        $res = fcdb_exec($stmt);
        $nr = fcdb_num_rows($res);
        if ( $nr > 0 ) {
          print "<div><table>\n";
          print "<tr><th class=\"left\">Option</th><th>Value</th></tr>\n";
          for ( $inx = 0; $inx < $nr; $inx++ ) {
            $row = fcdb_fetch_array($res, $inx);
            print "<tr><td>";
            print db2html($row["name"]);
            print "</td><td>";
            print db2html($row["value"]);
            print "</td></tr>\n";
          }
          print "</table></div>";
          print "<P><a href=\"".$_SERVER["PHP_SELF"]."\">Return to games list</a>";
        }

      }
    } else {
      print "<h1>Freeciv.net multiplayer games around the world</h1><br />\n";
      $stmt="select host,port,version,patches,state,message,unix_timestamp()-unix_timestamp(stamp),available from servers where message like '%Multiplayer%' or message like '%Tournament%' order by state,host,port asc";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr > 0 ) {
        print "<table>\n";
        print "<tr id='meta_header'><th class=\"left\">Action:</th><th>Server ID:</th>";
        print "<th>State</th><th>Players</th>";
        print "<th style='width:45%;'>Topic</th><th>Last Update</th>";
        print "<th>Players Available</th>\n";
        print "<th>Info:</th></tr>";
        for ( $inx = 0; $inx < $nr; $inx++ ) {
          $row = fcdb_fetch_array($res, $inx);
          print "<tr id='meta_row'><td class=\"left\">";
          print "<a href=\"/civclientlauncher?civserverport=" . db2html($row["port"]) . "&civserverhost=" . db2html($row["host"]) . "\">";
          //print db2html($row["port"]);
          print "<img src='/images/join.png' border='0' title='Join this game now'>";
          print "</a>";
          print "</td><td>";
	  	  print db2html($row["port"]);
          print "</td><td>";
          print db2html($row["state"]);
          print "</td><td>";
          $stmt="select * from players where hostport=\"".$row['host'].":".$row['port']."\"";
          $res1 = fcdb_exec($stmt);
          print fcdb_num_rows($res1);
          print "</td><td style=\"width: 30%\" title='To change the message in the topic, use the command:  /metamessage your-new-message in the game.'>";
          print db2html($row["message"]);
          print "</td><td>";
          $time_sec = $row["unix_timestamp()-unix_timestamp(stamp)"];
          $last_update = sprintf("%ss", $time_sec);
          if ($time_sec >= 60) {
            $last_update = sprintf("%sm", floor($time_sec/60));
          }
          print $last_update;
          print "</td><td>";
          print db2html($row["available"]);
	  	  print "</td>"
	  	  print "<td>";
          print "<a href=\"/freecivmetaserve/metaserver.php?server_port=" . db2html($row["host"]) . ":" . db2html($row["port"]) . "\">";
	  	  print "<img src='/images/info.png' border='0'>";
          print "</a>";
          print "</td>";
	  	  print "</tr>\n";
        }
        print "</table>";
      } else {
        print "<h2>No servers currently listed</h2>";
      }




      print "<br><br> ";
      print "<h1>Freeciv.net single-player games</h1><br />\n";
      $stmt="select host,port,version,patches,state,message,unix_timestamp()-unix_timestamp(stamp),available from servers where message like '%Singleplayer%' and state = 'Running' order by state,host,port asc";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr > 0 ) {
        print "<table>\n";
        print "<tr id='meta_header'><th class=\"left\">Action:</th><th>Server ID:</th>";
        print "<th>State</th><th>Players</th>";
        print "<th style='width:45%;'>Topic</th><th>Last Update</th>";
        print "<th>Players Available</th>\n";
        print "<th>Info:</th></tr>";
        for ( $inx = 0; $inx < $nr; $inx++ ) {
          $row = fcdb_fetch_array($res, $inx);
          print "<tr id='meta_row'><td class=\"left\">";
          print "<a href=\"/civclientlauncher?action=observe&civserverport=" . db2html($row["port"]) . "&civserverhost=" . db2html($row["host"]) . "\">";
          //print db2html($row["port"]);
          print "<img src='/images/observe.png' border='0' title='Observe this game now'>";
          print "</a>";
          print "</td><td>";
	  	  print db2html($row["port"]);
          print "</td><td>";
          print db2html($row["state"]);
          print "</td><td>";
          $stmt="select * from players where hostport=\"".$row['host'].":".$row['port']."\"";
          $res1 = fcdb_exec($stmt);
          print fcdb_num_rows($res1);
          print "</td><td style=\"width: 30%\" title='To change the message in the topic, use the command:  /metamessage your-new-message in the game.'>";
          print db2html($row["message"]);
          print "</td><td>";
          $time_sec = $row["unix_timestamp()-unix_timestamp(stamp)"];
          $last_update = sprintf("%ss", $time_sec);
          if ($time_sec >= 60) {
            $last_update = sprintf("%sm", floor($time_sec/60));
          }
          print $last_update;
          print "</td><td>";
          print db2html($row["available"]);
	  	  print "</td>"
	  	  print "<td>";
          print "<a href=\"/freecivmetaserve/metaserver.php?server_port=" . db2html($row["host"]) . ":" . db2html($row["port"]) . "\">";
	  	  print "<img src='/images/info.png' border='0'>";
          print "</a>";
          print "</td>";
	  	  print "</tr>\n";
        }
        print "</table>";
      } else {
        print "<h2>No single player games currently active</h2>";
      }




?>
<br />
<br />
<p class="center"><em>


<br><br>
Discuss and propose new multiplayer games in <a href="/forum/viewforum.php?f=2">the multiplayer forum</a> on Freeciv.net.
<br><br>



<br />


<?
    }
    echo "<br /><br /><p class=\"center\">";
    
    echo "</p><br /><br />";
    
  }
?>

</div>
</div>


</div>

<div id="footer">
Copyright &copy; 2008-2010 Freeciv.net
</div>

</div>


<!-- Google Analytics Code --!>
<script type="text/javascript">
var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");
document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
</script>
<script type="text/javascript">
var pageTracker = _gat._getTracker("UA-5588010-1");
pageTracker._trackPageview();
</script>


</body>
</html>


<?php 
} 

/* This returns a list of the capabilities that are mandatory in a given capstring
 * i.e. those that begin with a + 
 */
function mandatory_capabilities($capstr) {
  $return=array();
  $elements=preg_split("/\s+/",$capstr);
  foreach ($elements as $element) {
    if ( preg_match("/^\+/", $element) ) {
      array_push($return, ltrim($element,"+"));
    }
  }
  return($return);
}

/* This returns true if a cap is contained in capstr
 */
function has_capability($cap,$capstr) {
  $elements=preg_split("/\s+/",$capstr);
  foreach ($elements as $element) {
    $element=ltrim($element,"+"); /*drop + if there, because it wont match with it*/
    // debug("  comparing \"$cap\" to \"$element\"\n");
    if ( $cap == $element) {
      return(TRUE);
    } 
  }
  return(FALSE);
}

/* This returns true if all caps are contained in capstr
 */
function has_all_capabilities($caps,$capstr) {
  foreach ($caps as $cap) {
    if ( ! has_capability($cap,$capstr) ) {
      return(FALSE);
    }
  }
  return(TRUE);
}

function debug($output) {
  global $debug;
  if ( $debug ) {
    $stderr=fopen("php://stderr","a");
    fputs($stderr, $output);
    fclose($stderr);
  }
}
      
?>
