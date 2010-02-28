<?php

function get_all_friends($user) {
  $query = 'SELECT uid2 AS uid FROM friend WHERE uid1 = ' . $user->fb_uid;
  return facebook_client()->api_client->fql_query($query);
}

function get_connected_friends($user) {
  $results = array();
  $query = 'SELECT name, pic, uid, email_hashes, has_added_app FROM user WHERE uid IN '.
    '(SELECT uid2 FROM friend WHERE uid1 = '.$user->fb_uid.')';
  try {
    $rows = facebook_client()->api_client->fql_query($query);

    // Do filtering in PHP because the FQL doesn't allow it (yet)
    if (!empty($rows)) {
      foreach ($rows as $row) {
          $results[] = $row;
      }
    }
  }
  catch (Exception $e) {
    error_log("Failure in the api: ". $e->getMessage());
  }

  return $results;
}

