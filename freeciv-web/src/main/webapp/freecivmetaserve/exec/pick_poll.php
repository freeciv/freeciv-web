#!/usr/bin/env php

<?php
    // include the php-code finder
    ini_set("include_path",
		    ini_get("include_path") . ":" . $_SERVER["DOCUMENT_ROOT"]
	   );
    include_once("php_code/php_code_find.php");
    // includes for support routines
    include_once(php_code_find("fcdb.php"));
    fcdb_default_connect();

    ////////////////////////////////////////
    // Select a poll from the poll queue //
    //////////////////////////////////////

    // Select the oldest new poll.
    //$stmt="select * from poll_queue where new=1 and approved=1 having queue_num=min(queue_num)";
    $stmt="select * from poll_queue where new=1 and approved=1 order by queue_num limit 1";
    $res=fcdb_exec($stmt);

    // If no new polls are available, don't do anything;
    // old ones may require updating.
    //
    if (fcdb_num_rows ($res) == 0) {
       echo "No new polls are available.  Can't replace the current one.\n";
       exit(0);

       // we used to pick an old poll instead:
       $stmt="select * from poll_queue where new=0 order by queue_num limit 1";
       $res=fcdb_exec($stmt);
    }

    // Get ID of the selected poll
    $row = fcdb_fetch_array ($res, 0);
    $poll_queue_id = $row[id];

    /////////////////////////////////////////////
    // Make the poll available on the website //
    ///////////////////////////////////////////

    // Strip active poll in polls and clear IPs used.
    $stmt="update polls set active='n' where active='y'";
    $res=fcdb_exec($stmt);
    $stmt="delete from poll_ips";
    $res=fcdb_exec($stmt);

    // Insert new poll
    $date=gmdate("Y-m-d H:i:s");
    $stmt="insert into polls (title, active, datetime)" .
          " values ( '$row[title]', 'y', '$date')";
    $res=fcdb_exec($stmt);

    // Get id of new poll
    $stmt="select max(id) from polls";
    $poll_id=fcdb_query_single_value($stmt);

    // push poll_queue_answers to answers
    $stmt="insert into poll_answers (poll_id, answer, votes)" .
          " select $poll_id, answer, 0 from poll_queue_answer" .
	  " where poll_queue_id=$poll_queue_id";
    $res=fcdb_exec($stmt);

    //////////////////////////////////////////////////////////////
    // Push poll in the back of the queue and make it not new. //
    ////////////////////////////////////////////////////////////
    $queue_num = fcdb_query_single_value ("select max(queue_num)+1 from poll_queue");
    fcdb_exec("update poll_queue set queue_num=$queue_num, new=0 where id=$poll_queue_id");
?>
