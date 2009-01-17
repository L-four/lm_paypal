<?php
// $Id$

/**
 * Return the main LM PayPal settings form.
 */
function lm_paypal_settings_form() {
  _lm_paypal_ini();
  global $_lm_paypal_debug;
  global $_lm_paypal_host;
  global $_lm_paypal_business;
  global $_lm_paypal_ipns_max_age;
  global $_lm_paypal_obey_test_ipns;
  global $_lm_paypal_ipns_max_age;

  // $site_name = variable_get('site_name', 'drupal');
  $site_name = url('', array('query' => NULL, 'fragment' => NULL, 'absolute' => TRUE));

  // TODO: add better support for switching to dev/sandbox site : maybe two sets
  // of options (defaulting to sane options), with a radio button to switch
  // between them.

  // Show these in order of most likely to be changed
  $form['lm_paypal_business'] = array(
    '#type' => 'textfield',
    '#title' => t('LM PayPal Business/Premier Email'),
    '#default_value' => $_lm_paypal_business,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#validate' => array('lm_paypal_is_email_shaped' => array(0)),
    '#description' => t('The PayPal Business/Premier Email for the current website: %site_name', array('%site_name' => $site_name)),
  );

  $form['lm_paypal_host'] = array(
    '#type' => 'textfield',
    '#title' => t('LM PayPal Host'),
    '#default_value' => $_lm_paypal_host,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#description' => t('The host to send PayPal requests to usually www.paypal.com (when testing use www.sandbox.paypal.com)'),
  );

  $form['lm_paypal_ipns_max_age'] = array(
    '#type' => 'textfield',
    '#title' => t('LM PayPal Max Age IPNS'),
    '#default_value' => $_lm_paypal_ipns_max_age,
    '#maxlength' => 10,
    '#required' => TRUE,
    '#validate' => array('lm_paypal_is_integer_between' => array(1)),
    '#description' => t('Maximum age of an old IPN record in hours before it is deleted. Minimum is 1.'),
  );

  $form['lm_paypal_obey_test_ipns'] = array(
    '#type' => 'checkbox',
    '#title' => t('LM PayPal Obey Test IPNS'),
    '#default_value' => $_lm_paypal_obey_test_ipns,
    '#description' => t('Obey test IPNS, from PayPal Sandbox, as if real'),
  );

  $form['lm_paypal_skip_validation'] = array(
    '#type' => 'value',
    '#value' => variable_get('lm_paypal_skip_validation', FALSE),
    '#title' => t('Disable validation of payments with paypal.com'),
    '#description' => t('This should NEVER be enabled, except for testing. Hidden from the UI for safety'),
  );

  $form['lm_paypal_debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('LM PayPal Debug'),
    '#default_value' => $_lm_paypal_debug,
    '#description' => t('Enabled verbose debugging output of LM PayPal'),
  );

  return system_settings_form($form);
}



/**
 * View all saved IPNs
 *
 * Mostly borrowed from watchdog.module.
 */
function lm_paypal_ipns() {
  _lm_paypal_ini();
  global $_lm_paypal_debug;

  $ipns_per_page = 50;

  $output = drupal_get_form('lm_paypal_ipns_filter');

  $header = array(
    array('data' => t('Id'), 'field' => 'id'),
    array('data' => t('Date'), 'field' => 'timestamp', 'sort' => 'desc'),
    array('data' => t('Txn Type'), 'field' => 'txn_type'),
    array('data' => t('User'), 'field' => 'custom'), // TODO: not 'custom' anymore
  );

  $sql = "SELECT id, timestamp, txn_type, custom FROM {lm_paypal_ipns}";
  $tablesort = tablesort_sql($header);

  // If not sorting by timestamp then make that the 2nd field to sort on
  if (strpos($tablesort, 'timestamp') === FALSE) {
    $tablesort .= ', timestamp DESC';
  }

  $type = $_SESSION['lm_paypal_ipns_filter'];
  if ($type != 'all') {
    if (strpos($type, '%') === FALSE) {
      $result = pager_query($sql ." WHERE txn_type = '%s'". $tablesort, $ipns_per_page, 0, NULL, $type);
    }
    else {
      // If type contains a '%' use like to match it
      $result = pager_query($sql ." WHERE txn_type like '%s'". $tablesort, $ipns_per_page, 0, NULL, $type);
    }
  }
  else {
    $result = pager_query($sql . $tablesort, $ipns_per_page);
  }

  while ($ipn = db_fetch_object($result)) {
    $custom = unpack_ipn_custom($ipn);
    // $uid = $custom['uid'];
    // $other = ($ipn->custom >> 16) & 0xFFFF;
    $rows[] = array(
      'data' => array(
        l($ipn->id, "admin/settings/lm_paypal/id/$ipn->id"),
        format_date($ipn->timestamp, 'small'),
        check_plain($ipn->txn_type),
        $custom['uid'] . ($custom['other'] == '' ? '' : " ($custom[other])"),
      ),
    );
  }

  if (!$rows) {
    $rows[] = array(array('data' => t('No ipns found.'), 'colspan' => 3));
  }

  $output .= theme('table', $header, $rows);
  $output .= theme('pager', NULL, $ipns_per_page, 0);

  return $output;
}








/**
 * Display a saved IPN.
 *
 * @param $id
 *   Required. The id of the IPN to display.
 * @return
 *   The string containing the details of the IPN.
 */
function lm_paypal_id($id = '') {
  _lm_paypal_ini();
  global $_lm_paypal_debug;

  $id = check_plain($id);

  if ($id == '' || !is_numeric($id) || intval($id) != $id) {
    watchdog(LM_PAYPAL, 'Bad id passed: %id', array('%id' => $id), WATCHDOG_WARNING);
    return t('Huh?');
  }

  // Output the transaction as a table of fields/values (skip the empty ones)
  $output = '<h2>'. t('Transaction %id', array('%id' => $id)) .'</h2>';
  $header = array(t('field'), t('value'));
  $ipns = db_query("SELECT * FROM {lm_paypal_ipns} WHERE id = %d", $id);
  foreach (db_fetch_array($ipns) as $key => $value) {
    if ($value == '' || $key == 'ipn') {
      continue;
    }
    if ($key == 'timestamp') {
      $value = format_date($value);
    }
    else {
      $value = check_plain($value);
    }
    $rows[] = array('data' => array($key, $value));
  }
  $output .= theme('table', $header, $rows);
  return $output;
}



function lm_paypal_ipns_filter() {
  $names = array(
    'all' => t('all messages'),
    'web_accept' => t('donation/sent money (web_accept)'),
    'subscr_%' => t('all subscription IPNs'),
    'subscr_signup' => t('subscription signup'),
    'subscr_payment' => t('subscription payment'),
    'subscr_cancel' => t('subscription cancel'),
    'subscr_eot' => t('subscription eot'),
  );

// TODO: seems like we should be using form state for this?
  if (empty($_SESSION['lm_paypal_ipns_filter'])) {
    $_SESSION['lm_paypal_ipns_filter'] = 'all';
  }

  $form['#submit'] = array('lm_paypal_ipns_submit');

  $form['filter'] = array(
    '#type' => 'select',
    '#title' => t('Filter IPN type'),
    '#options' => $names,
    '#default_value' => $_SESSION['lm_paypal_ipns_filter'],
  );
  $form['#action'] = url('admin/settings/lm_paypal/ipns');

  $form['submit'] = array('#type' => 'submit', '#value' => t('Filter'));

  return $form;
}


/**
 * Process the form submission for lm_paypal_ipns
 */
function lm_paypal_ipns_submit($form, &$form_state) {
  global $form_values;
  $_SESSION['lm_paypal_ipns_filter'] = $form_state['values']['filter'];
}