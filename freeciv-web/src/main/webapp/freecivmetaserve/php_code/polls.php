<?php
/*
 * You must include the following before including this file:
 *     include_once("php_code/settings.php");
 *     include_once("php_code/fcdb.php");
 */

/* Emit the current poll as a form within a table. */

function polls_emit_poll() {
  global $webmaster_html;

  fcdb_default_connect();

  echo '<TABLE summary="" WIDTH="100%" BORDER="0" CELLPADDING="0">', "\n";
  echo '<TR>', "\n";
  echo '<TD BGCOLOR="#000000">', "\n";
  echo '<TABLE summary="" BORDER="0" WIDTH="100%">', "\n";
  echo '<TR BGCOLOR="#00688B"><TH> Poll </TH></TR>', "\n";
  echo '<TR>', "\n";
  echo '<TD BGCOLOR="#9FB6CD">', "\n";
  echo '<TABLE summary="" WIDTH="100%">', "\n";
  echo '<TR><TD> &nbsp; </TD></TR>', "\n";
  echo '<TR>', "\n";
  echo '<TD>', "\n";

  $stmt="select * from polls where active='y'";
  $res = fcdb_exec($stmt);
  if ( fcdb_num_rows($res) == 1 ) {
    $row = fcdb_fetch_array($res, 0);
    $id=$row["id"];
    echo '<CENTER><B>';
    echo $row["title"];
    echo '</B></CENTER>', "\n";
    echo '<FORM METHOD="POST" ACTION="poll.phtml">', "\n";
    $stmt = "select * from poll_answers where poll_id=$id order by id";
    $res = fcdb_exec($stmt);
    for ( $inx = 0; $inx < fcdb_num_rows($res); $inx++ ) {
      $row = fcdb_fetch_array($res, $inx);
      echo '<INPUT name="value" type="radio" value="', $row["id"], '"> ',
	   stripslashes($row["answer"]), "<BR>\n";
    }
    echo '<CENTER>', "\n";
    echo '<INPUT TYPE="submit" NAME="submit" VALUE="Vote">&nbsp;&nbsp;', "\n";
    echo '<INPUT TYPE="submit" NAME="submit" VALUE="Results">', "\n";
    echo '</CENTER>', "\n";
    echo '</FORM>', "\n";
  } else {
    echo "Poll is broken, contact $webmaster_html.<BR>\n";
  }

  echo '<HR noshade>', "\n";
  echo '<A HREF="oldpolls.phtml">View results of old polls</A>', "\n";
  echo '<P><a href="add_poll.phtml">Submit a poll</a>', "\n";

  echo '</TD>', "\n";
  echo '</TR>', "\n";
  echo '</TABLE>', "\n";
  echo '</TD>', "\n";
  echo '</TR>', "\n";
  echo '</TABLE>', "\n";
  echo '</TD>', "\n";
  echo '</TR>', "\n";
  echo '</TABLE>', "\n";

}

/* Emit the results of the current poll as a table.
   Update with a vote, if one is provided (limit one vote per IP address). */

function polls_emit_results() {
  global $webmaster_html;
  $submit =$_REQUEST['submit'];
  $value = $_REQUEST['value'];
  $num = $_REQUEST['num'];

  fcdb_default_connect();

  if ( ! $num ) {
    $stmt = "select id from polls where active='y'";
    $realnum = fcdb_query_single_value($stmt);
  } else {
    $realnum = $num;
  }

  if ( $submit == "Vote" && $value ) {
    $user_ip = $_SERVER['REMOTE_ADDR'];
    $stmt = "select * from poll_ips where ip='$user_ip'";
    $res = fcdb_exec($stmt);
    if ( fcdb_num_rows($res) > 0 ) {
      $row = fcdb_fetch_array($res, 0);
      $answer=$row["answer_id"];
      $stmt = "select * from poll_answers where id=$answer";
      $res = fcdb_exec($stmt);
      $row = fcdb_fetch_array($res, 0);
      echo "<H2>$user_ip has already voted: ";
      echo stripslashes($row["answer"]);
      echo "</H2>\n";
    } else {
      $stmt = "select * from poll_answers where id=$value and poll_id=$realnum";
      $res = fcdb_exec($stmt);
      $nr = fcdb_num_rows($res);
      if ( $nr == 1 ) {
	$row = fcdb_fetch_array($res, 0);
	echo "<H2>Your vote: ";
	echo $row["answer"];
	echo "</H2>\n";
	$votes=$row["votes"]+1;
	$stmt = "update poll_answers set votes=$votes where id=$value";
	$res = fcdb_exec($stmt);
	$stmt = "insert into poll_ips (ip, answer_id) values ('$user_ip', $value)";
	$res = fcdb_exec($stmt);
	$date=gmdate("Y-m-d H:i:s");
	$stmt = "update polls set datetime='$date' where id=$realnum";
	$res = fcdb_exec($stmt);
      } else {
	echo "Big problems in Little China, contact $webmaster_html.<BR>\n";
      }
    }
  }

  $stmt = "select * from polls where id=$realnum";
  $res = fcdb_exec($stmt);
  $nr = fcdb_num_rows($res);
  if ( $nr == 1 ) {
    $poll = fcdb_fetch_array($res, 0);
    echo '<TABLE summary="" BORDER BGCOLOR="#9FB6CD">' . "\n";
    echo "<TR BGCOLOR=\"#00688B\"><TH COLSPAN=4 ALIGN=CENTER>",
	 "<FONT SIZE=+2>Results of the poll:<BR>";
    echo $poll["title"];
    echo "</TR>\n";
    echo "<TR><TH COLSPAN=2>Votes<TH COLSPAN=2>Percentage</TR>\n";
    echo "<TR></TR>\n";
    $stmt = "select sum(votes) as total from poll_answers where poll_id=$realnum";
    $res = fcdb_exec($stmt);
    $totalarray=fcdb_fetch_array($res, 0);
    $total=$totalarray["total"];
    if ( $total <= 0 ) {
      $totdiv = 1;
    } else {
      $totdiv = $total;
    }
    $stmt = "select votes, ( votes * 100 ) / $totdiv as percent, answer" .
	    " from poll_answers where poll_id=$realnum";
    if ( $num ) {
      $stmt .= " order by votes desc, id";
    } else {
      $stmt .= " order by id";
    }
    $res = fcdb_exec($stmt);
    for ( $inx = 0; $inx < fcdb_num_rows($res); $inx++ ) {
      $row = fcdb_fetch_array($res, $inx);
      $l=round($row["percent"]+.0001);
      echo "<TR><TH ALIGN=\"LEFT\">";
      echo stripslashes($row["answer"]);
      echo "</TH><TH>";
      echo $row["votes"];
      echo "</TH><TH ALIGN=LEFT><IMG SRC=\"/pics/line.png\" height=\"16\" width=\"";
      echo $l*6+1;
      echo "\"> <TH>$l %</TR>\n";
    }
    echo "<TR></TR>\n";
    echo "<TR><TH>Total Votes<TH>$total<TH COLSPAN=2>100 %</TR>\n";
    echo "<TR></TR>\n";
    echo "<TR><TH COLSPAN=2>Last vote<TH COLSPAN=2>";
    echo $poll["datetime"];
    echo " UTC</TR>\n";
    print "</TABLE>\n";
  } else {
    echo "<BR>Poll #$num not found.<BR>\n";
  }
}

/* Emit the results of all old polls as a table. */

function polls_emit_old_polls() {
  global $webmaster_html;

  fcdb_default_connect();

  echo '<TABLE summary="" WIDTH="100%" BORDER="0" CELLPADDING="0">', "\n";
  echo '<TR>', "\n";
  echo '<TD BGCOLOR="#000000">', "\n";
  echo '<TABLE summary="" BORDER="0" WIDTH="100%">', "\n";
  echo '<TR BGCOLOR="#00688B"><TH> Old Polls </TH></TR>', "\n";
  echo '<TR>', "\n";
  echo '<TD BGCOLOR="#9FB6CD">', "\n";
  echo '<TABLE summary="" WIDTH="100%">', "\n";
  echo '<TR><TD> &nbsp </TD></TR>', "\n";
  echo '<TR>', "\n";
  echo '<TD>', "\n";
  echo '<CENTER>', "\n";

  $stmt = "select * from polls where active='n'";
  $res = fcdb_exec($stmt);
  for ( $inx = 0; $inx < fcdb_num_rows($res); $inx++ ) {
    $poll = fcdb_fetch_array($res, $inx);
    $date = explode(" ", $poll["datetime"]);
    echo "<B><A HREF=\"poll.phtml?num=";
	 echo $poll["id"];
	 echo "\">";
	 echo $poll["title"];
	 echo "</A></B><BR>(Last vote: ";
	 echo $date[0];
	 echo " UTC)<br>\n";
  }

  echo '</CENTER>', "\n";
  echo '<P>', "\n";
  echo '<HR noshade>', "\n";
  echo '<P>If you have an interesting suggestion for a poll, mail ', "$webmaster_html.\n";
  echo '</TD>', "\n";
  echo '</TR>', "\n";
  echo '</TABLE>', "\n";
  echo '</TD>', "\n";
  echo '</TR>', "\n";
  echo '</TABLE>', "\n";
  echo '</TD>', "\n";
  echo '</TR>', "\n";
  echo '</TABLE>', "\n";
}

?>
