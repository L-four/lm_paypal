<?php

/**
 * Handle an incoming IPN.
 *
 * PayPal sends an IPN here for each transaction that takes place. The IPN is
 * present as a form submission which this routine unravels and saves for
 * processing.
 */
function lm_paypal_ipn_in() {
  // file_put_contents('/tmp/ipn-post.txt', serialize($_POST));

  watchdog(LM_PAYPAL, 'lm_paypal_ipn_in called', NULL);
  _lm_paypal_ini();
  global $_lm_paypal_host;

  // Don't bother with these fields - but don't flag them as errors
  static $ignore_fields = array(
    // 'notify_version',
    'receipt_id',
    'charset',
    'mc_gross1',
  );

  // Process the incoming form results to (a) build the validation request, and
  // (b) check for unexpected values.
  $schema = drupal_get_schema('lm_paypal_ipns');
  $req = 'cmd=_notify-validate';
  foreach ($_POST as $key => $value) {
    $req .= "&$key=" . urlencode(stripslashes($value));

    if ($value == '' || in_array($key, $ignore_fields)) {
      continue;
    }

    if (!isset($schema['fields'][$key])) {
      watchdog(LM_PAYPAL, 'IPN unknown field ignored: !key => !value', array('!key' => check_plain($key), '!value' => check_plain($value)), WATCHDOG_ERROR);
    }
  }


  // Validate this incoming IPN by sending it to PayPal to be checked
  $skip_validation = variable_get('lm_paypal_skip_validation', FALSE);
  if ($skip_validation) {
    // Danger, Will Robinson!  Never enable this on a real website.
    $validated = TRUE;
  }
  else {
    watchdog(LM_PAYPAL, 'lm_paypal_ipn_in pre-validate', NULL);
    $validate_result = drupal_http_request(
      'http://'. $_lm_paypal_host . '/cgi-bin/webscr',
      array(
        'Content-Type' => 'application/x-www-form-urlencoded',
      ),
      'POST',
      $req
    );
    $validated = $validate_result->data == 'VERIFIED';
    watchdog(LM_PAYPAL, 'lm_paypal_ipn_in post-validate', NULL);
  }

  // Sanitize and remove risky fields from $_POST to make it suitable for
  // drupal_write_record. Include the entire POST data for debugging.
  $ipn = $_POST;
  $ipn['timestamp'] = time();
  $ipn['ipn'] = serialize($_POST);
  unset($ipn['processed']);

  // Add the IPN to our database (whether it validated or not).
  $wr = drupal_write_record('lm_paypal_ipns', $ipn);
  if ($wr == SAVED_NEW) {
    $last = $ipn['id'];
    watchdog(LM_PAYPAL, 'lm_paypal_ipn_in $last = '. $ipn['id'], NULL);
    $link = l(t('view'), "admin/settings/lm_paypal/id/$ipn[id]");

    if ($validated) {
      watchdog(LM_PAYPAL, 'IPN incoming %type', array('%type' => check_plain($_POST['txn_type'])), WATCHDOG_NOTICE, $link);
      // TODO: replace with actions/triggers. pass $ipn?
      lm_paypal_process_in($ipn['id']);
    }
    else {
      watchdog(LM_PAYPAL, 'IPN incoming NOT VERIFIED %type got %ret', array('%type' => check_plain($_POST['txn_type']), '%ret' => check_plain($validate_result->data)), WATCHDOG_ERROR, $link);
    }
  }
  else {
    watchdog(LM_PAYPAL, 'IPN in failed to run sql: %sql', array('%sql' => check_plain($sql)), WATCHDOG_ERROR);
    // Return an HTTP error and hopefully PayPal will resend the IPN to me
    // later on and then I can try again! Maybe PayPal is very busy
    // or there is a network problem at the moment
  }


  return 'IPN: Only PayPal will ever see this page - humans go away!';
}
