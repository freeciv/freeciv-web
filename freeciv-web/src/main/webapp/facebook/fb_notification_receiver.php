<?php

  /*
   * When you register a user on Facebook using an email hash, you also register an "account id".
   * If that user responds to the request and connects their accounts, then you can be notified at
   * the "post-authorize-url" as configured in your developer account settings.
   *
   * This is that post-authorize-url.
   */

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

$fb = facebook_client();
$fb_uid = $fb->get_loggedin_user();
$account_ids = idx($fb->fb_params, 'linked_account_ids');

if (!($account_ids && $fb_uid)) {
  exit;
}

// Theoretically possible for a single facebook user to have multiple accounts on this system,
// Since they could have multiple email addresses. so account for that
foreach ($account_ids as $account_id) {
    $user = User::getByUsername($account_id);
    $user->connectWithFacebookUID($fb_uid);
    $user->save();
}
