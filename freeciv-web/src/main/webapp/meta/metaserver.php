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
  "message",
  "type",
  "serverid",
  "available",
  "humans",
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
  "message",
  "type",
  "available",
  "humans",
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

  if (isset($message)) {
    $message = addneededslashes($message); /* escape stuff to go into the database */
  }
  if (isset($type)) {
    $type = addneededslashes($type); /* escape stuff going to database */
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
      if (isset($pln[$i]) ) {
        $pln[$i] = addneededslashes($pln[$i]);
        $ins .= "flag=\"$pln[$i]\", ";
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
      if (isset($assoc_array[$var])) {
        array_push($string, "$var=\"$assoc_array[$var]\"");
      }
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
      $avstmt = "update servers set available=0, humans=-1 where host=\"$host\" and port=\"$port\"";
      $res = fcdb_exec($avstmt);
    }

    for ($i = 0; $i < count($playerstmt); $i++) {
      print "$playerstmt[$i]\n";
      $res = fcdb_exec($playerstmt[$i]);
    }
  }

  /* We've done the database so we're done */

} elseif ( isset($client_cap) || isset($client) ) {
  global $freeciv_versions;
  $output = "";
  $output .= "[versions]\n";
  $output .= "latest_stable=\"" . version_by_tag("stable") . "\"\n";
  $verkeys = array_keys($freeciv_versions);
  foreach ($verkeys as $key) {
    $output .= "$key=\"" . version_by_Tag("$key") . "\"\n";
  }
  $stmt="select * from servers order by host,port asc";
  $res = fcdb_exec($stmt);
  $nr = fcdb_num_rows($res);
  $nservers=0;
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

<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>Freeciv-web - Live Games Metaserver</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="Freeciv is a Free and Open Source empire-building strategy game">
    <meta name="author" content="The Freeciv project">

    <!-- Le styles -->
    <link href="/css/bootstrap.min.css" rel="stylesheet">
    <link href="/css/bootstrap-responsive.min.css" rel="stylesheet">
    <link href="/meta/css/metaserver.css" rel="stylesheet">
<link rel="shortcut icon" href="/images/freeciv-shortcut-icon.png" />
<link type="text/css" href="/meta/css/jquery-ui.custom.min.css" rel="stylesheet" />
<script type="text/javascript" src="/javascript-compressed/jquery.min.js"></script>
<script type="text/javascript" src="/javascript/jquery-ui.custom.min.js"></script>
<script type="text/javascript" src="/meta/js/meta.js"></script>


<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');

  ga('create', 'UA-40584174-1', 'freeciv.org');
  ga('send', 'pageview');
      
</script>


  </head>

  <body>


    <div class="navbar navbar-inverse navbar-fixed-top">
      <div class="navbar-inner">
        <div class="container">
          <button type="button" class="btn btn-navbar" data-toggle="collapse" data-target=".nav-collapse"
		onclick="window.location='/wireframe.jsp?do=menu';">
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
          </button>
	<a href="/" style="padding: 3px; float: left;"><img src="/images/freeciv-web-logo.png" alt="The Freeciv Project"></a>
          <div class="nav-collapse collapse">
            <ul class="nav">
              <li><a href="/">Play Freeciv!</a></li>
              <li><a href="http://www.freeciv.org/wiki/">Wiki</a></li>
              <li class="active"><a href="/meta/metaserver.php">Current Games</a></li>
              <li><a href="http://forum.freeciv.org">Forum</a></li>
              <li><a href="http://github.com/freeciv/freeciv-web">Development</a></li>
              <li><a href="http://freeciv.wikia.com/wiki/Donations">Donate!</a></li>

            </ul>
           </div><!--/.nav-collapse -->
        </div>
      </div>
    </div>

<div id="container">

 <div class="row" >
 <div class="span10 metaspan">


<div>
<?php

  if ($error_msg != NULL) {
    echo $error_msg;
  } else {
    if (isset($server_port)) {
 

      $port = substr(strrchr($server_port, ":"), 1);
      $host = substr($server_port, 0, strlen($server_port) - strlen($port) - 1);
      print "<h1>Freeciv-web server id: " . db2html($port) . "</h1>\n";
      print "Freeciv-web is an Open Source strategy game which can be played online against other players, or in single player mode against AI opponents.<br><br>";

      
      $stmt = "select * from servers where host=\"$host\" and port=\"$port\"";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr != 1 ) {
        print "Cannot find the specified server";
      } else {
        $row = fcdb_fetch_array($res, 0);

	print "</a>";
        if ($row["state"] == "Pregame") {
          print "<div><a class='button' href='/civclientlauncher?civserverport=" . db2html($port) . "&amp;civserverhost=" . db2html($host)
             . "'>Join</a> <b>You can join this game now.</b></div>";
	}
        print "<div><a class='button' href='/civclientlauncher?action=observe&amp;civserverport=" . db2html($port) . "&amp;civserverhost=" . db2html($host)
             . "'>Join/Observe</a> <b>You can observe this game now.</b></div>";

        print "<br/><br/>";
        $msg = db2html($row["message"]);
        print "<table><tr class='meta_header'><th>Version</th><th>Patches</th><th>Capabilities</th>";
        print "<th>State</th>";
        print "<th>Server ID</th></tr>\n";
        print "<tr class='meta_row'><td>";
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
          print "<table><tr class='meta_header'><th>Message</th></tr>\n";
          print "<tr><td>" . $msg . "</td></tr>";
          print "</table></p>\n";
        }
        $stmt="select * from players where hostport=\"$server_port\" order by name";
        $res = fcdb_exec($stmt);
        $nr = fcdb_num_rows($res);
        if ( $nr > 0 ) {
          print "<p><div><table class='metainfotable'>\n";
          print "<tr class='meta_header'><th class=\"left\">Flag</th><th>Leader</th><th>Nation</th>";
          print "<th>User</th><th>Type</th></tr>\n";
          for ( $inx = 0; $inx < $nr; $inx++ ) {
            $row = fcdb_fetch_array($res, $inx);
            print "<tr class='meta_row'><td class=\"left\">";
            print "</td><td>";
            print db2html($row["name"]);
            print "</td><td>";
            print db2html($row["nation"]);
            print "</td><td>";
            print db2html($row["user"]);
            print "</td><td>";
            print db2html($row["type"]);
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
          print "<P><a class='button' href=\"".$_SERVER["PHP_SELF"]."\">Return to games list</a>";
        }

      }
    } else {
	?>

<div id="tabs">
<ul>
<li><a id="singleplr" href="#tabs-1">Single-player Games</a></li>
<li><a id="multiplr" href="#tabs-2">Multi-player Games</a></li>
<li><a id="freecivmeta" href="#tabs-3">Desktop Games</a></li>
</ul>
<div id="tabs-1">
<h2>Freeciv-web Single-player games</h2>
<?
      $stmt="select host,port,version,patches,state,message,unix_timestamp()-unix_timestamp(stamp), IFNULL((select user from players p where p.hostport =  CONCAT(s.host ,':',s.port) and p.type = 'Human' Limit 1 ), 'none') as player, IFNULL((select flag from players p where p.hostport =  CONCAT(s.host ,':',s.port) and p.type = 'Human' Limit 1 ), 'none') as flag, (select value from variables where name = 'turn' and hostport = CONCAT(s.host ,':',s.port)) as turn, (select value from variables where name = 'turn' and hostport = CONCAT(s.host ,':',s.port)) + 0 as turnsort from servers s where type = 'Singleplayer' and state = 'Running' order by turnsort desc";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr > 0 ) {
        print "<br /><table class='metatable'>\n";
        print "<tr class='meta_header'><th>Game Action:</th>";
        print "<th>State</th><th>Players</th>";
        print "<th style='width:45%;'>Message</th>";
        print "<th>Player</th>\n";
        print "<th>Turn:</th></tr>";
        for ( $inx = 0; $inx < $nr; $inx++ ) {
          $row = fcdb_fetch_array($res, $inx);
          print "<tr class='meta_row'><td>";

	  print "<a  class='button' href=\"/civclientlauncher?action=observe&amp;civserverport=" 
		  . db2html($row["port"]) . "&amp;civserverhost=" . db2html($row["host"]) . "\">";
          print "Join/Observe";
          print "</a>";

          print "<a class='button' href=\"/meta/metaserver.php?server_port=" . db2html($row["host"]) . ":" . db2html($row["port"]) . "\">";
	  	  print "Info";
          print "</a>";


	  print "</td><td>";
          print db2html($row["state"]);
          print "</td><td>";
          $stmt="select * from players where hostport=\"".$row['host'].":".$row['port']."\"";
          $res1 = fcdb_exec($stmt);
          print fcdb_num_rows($res1);
          print "</td><td style=\"width: 30%\" >";
          print db2html($row["message"]);
          print "</td><td>";

          print db2html($row["player"]);
	  print "</td><td>"
          print db2html($row["turn"]);
	  print "</td></tr>\n";
        }
        print "</table><br><br><br><br>";
      } else {
        print "<h3><a href='/wireframe.jsp?do=login'>Click here to start a new single player game!</a></h3><br><br><br>";
      }
?>

 </div>
 <div id="tabs-2">
 <h2>Freeciv-web Multiplayer games around the world</h2>

<?
      $stmt="select host,port,version,patches,state,message,unix_timestamp()-unix_timestamp(stamp), (select value from variables where name = 'turn' and hostport = CONCAT(s.host ,':',s.port)) as turn from servers s where type = 'Multiplayer' order by state desc";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr > 0 ) {
	print "<table class='metatable'>\n";
        print "<tr class='meta_header'><th>Game Action:</th>";
        print "<th>State</th><th>Players</th>";
        print "<th style='width:45%;'>Message</th>";
        print "<th>Turn:</th></tr>";

        for ( $inx = 0; $inx < $nr; $inx++ ) {
 	  $row = fcdb_fetch_array($res, $inx);
	  $mystate = db2html($row["state"]);

          print "<tr class='meta_row'><td>";
	  
  
          if ($mystate != "Running") {
           print "<a  class='button' href=\"/civclientlauncher?civserverport=" . db2html($row["port"]) . "&amp;civserverhost=" . db2html($row["host"]) . "\">";
           print "Play";
	   print "</a>";
	  } else {
	   print "<a  class='button' href=\"/civclientlauncher?action=observe&amp;civserverport=" . db2html($row["port"]) . "&amp;civserverhost=" . db2html($row["host"]) . "\">";
           print "Join/Observe";
           print "</a>";
	  }


          print "<a class='button' href=\"/meta/metaserver.php?server_port=" . db2html($row["host"]) . ":" . db2html($row["port"]) . "\">";
	  	  print "Info";
          print "</a>";

	  print "</td>";
          if ($mystate == "Running") {
  	    print "<td style='color: green;'>";
	  } else {
  	    print "<td>";
  	  }

          print db2html($row["state"]);
          print "</td>";
          $stmt="select * from players where type='Human' and hostport=\"".$row['host'].":".$row['port']."\"";
	  $res1 = fcdb_exec($stmt);
	  if (fcdb_num_rows($res1) == 0) {
		  print ("<td>None" );
	  } else if (fcdb_num_rows($res1) == 1) {
		  print ("<td style='color: green;'>" . fcdb_num_rows($res1 ) . " player");
	  } else {
		  print ("<td style='color: green;'>" . fcdb_num_rows($res1 ) . " players");
	  }
          print "</td><td style=\"width: 30%\" >";
          print db2html($row["message"]);
	  print "</td><td>"
          print db2html($row["turn"]);
	  print "</td></tr>\n";


        }
        print "</table> </div> ";
      } else {
        print "<h2>No servers currently listed</h2>";
      }

?>


<div id="tabs-3">
  <h2>Desktop version of Freeciv - meta.freeciv.org </h2>
<iframe src="http://meta.freeciv.org" width="100%" height="1500" frameborder="0"></iframe>
</div>

</div>
</div>



</div>



<?
    }
    
    
  }
 ?>


</div>



      <hr>

      <footer>
        <p>&copy; The Freeciv Project 2013</p>
      </footer>

    </div> <!-- /container -->

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
