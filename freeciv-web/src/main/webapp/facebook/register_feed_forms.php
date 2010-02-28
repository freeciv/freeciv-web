<?php

  /*
   * register_feed_forms.php
   *
   * Access this PHP file once after you have set up your configuration.
   * Copy/paste the resulting bundle ID and put it into config.php.
   */

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

echo render_header();

try {
  verify_facebook_callback();

  $bundle_id = register_feed_forms();

  echo '<div class="bluebox">'
    .'<p>Congratulations! You have registered a feed form.</p>'
    .'<p>Put the following line in lib/config.php</p>'
    .'<pre>'
    .'  $feed_bundle_id = <b>'.$bundle_id.'</b>;'
    .'</pre>'
    .'</div>';
} catch (Exception $e) {
  echo '<div class="error">'
    .'Error while setting up application: <br /><tt>'
    .$e->getMessage().'</tt>';
}

echo render_footer();

/*
 * Make the API call to register the feed forms. This is a setup call that only
 * needs to be made once.
 *
 */
function register_feed_forms() {
  $one_line_stories = $short_stories = $full_stories = array();

  $one_line_stories[] = '{*actor*} went for a {*distance*} run at {*location*}.';

  $form_id = facebook_client()->api_client->feed_registerTemplateBundle($one_line_stories);
  return $form_id;
}


/*
 * Make sure that the settings on Facebook correspond with the settings in the config file.
 * Enforce business rules that need to be in place in order for the iframe communication to work.
 */
function verify_facebook_callback() {
  $app_properties = facebook_client()->api_client->admin_getAppProperties(array('callback_url'));
  $facebook_callback_url = idx($app_properties, 'callback_url');
  $local_callback_url = get_callback_url();

  if (!$facebook_callback_url) {
    throw new Exception('You need to configure your callback URL in the '.
                        '<a href="http://www.facebook.com/developers/">Facebook Developers App</a>');
  }
  if (!$local_callback_url) {
    throw new Exception('Copy your Facebook callback URL into lib/config.php. '.
                        'Your Callback is '.$facebook_callback_url);
  }
  if (strpos($local_callback_url, 'http://') === null) {
    throw new Exception('Your configured callback url must begin with http://. '
                        .'It is currently set to "'.$local_callback_url.'"');
  }
  if (get_domain($facebook_callback_url) != get_domain($local_callback_url)) {
    throw new Exception('Your config file says the callback URL is "'.$local_callback_url.
                        '", but Facebook says "'.$facebook_callback_url.'"');
  }
  if (get_domain($local_callback_url) != get_domain($_SERVER['SCRIPT_URI'])) {
    throw new Exception('Your config file says the callback URL is "'.$local_callback_url.
                        '", but you are on "'.$_SERVER['SCRIPT_URI'].'"');
  }
}
