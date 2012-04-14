<?php
// $Id: lm_paypal_subscriptions.pages.inc,v 1.1.2.4 2009/05/30 17:05:54 pounard Exp $

/**
 * @file
 * Paypal subscriptions interface, user pages.
 */

/////////////////////////// BELOW THIS LINE CODE IS WORKING BUT UGLY




////////////////////////////// (AND UN-MAINTAINABLE)





















/**
 * Print a subscription details
 *
 *  TODO: we should replace this method with a set of different theme functions
 *
 * @param $subid
 *   Required. The subscription to print
 * @param $display
 *   If just showing a single subscription then $diplay lists what to show.
 *   1 = item_name
 *   2 = description
 *   4 = human readable details of subscription
 *   8 = button
 *  16 = brief
 *   These can be added to get combinations (eg: 11 = 1 + 2 + 8)
 */
function lm_paypal_subscriptions_details($subid, $display = 0) {
  $field_names = array(
    'item_name' => 'Subscription Name',
    'description' => 'Description',
    'rid' => 'Role',
    'a1' => 'Trial amount 1',
    'p1' => 'Trial period 1',
    'a2' => 'Trial amount 2',
    'p2' => 'Trial period 2',
    'a3' => 'Regular rate',
    'p3' => 'Regular billing cycle',
    'src' => 'Recurring payments',
    'srt' => 'Recurring times',
    'currency_code' => 'Currency',
    'terms' => 'Terms and Conditions',
    'status' => 'Status',
  );

  $subid = (int) $subid;
  $so = lm_paypal_subscriptions_load_subscription($subid);

  drupal_set_title(t('Subscription @name', array('@name' => $so->item_name)), PASS_THROUGH);

  $roles = lm_paypal_subscriptions_roles_list(TRUE);
  $groups = lm_paypal_subscribable_groups();

  if ($display != 0) {
    $output = '';
    if (($display & 1) != 0) {
      if (($display & 16) != 0) {
        $output .= '<b>' . check_plain($so->item_name) . '</b> ';
      }
      else {
        $output .= '<h3>' . check_plain($so->item_name) . '</h3>';
      }
    }
    if (($display & 2) != 0) {
      if (($display & 16) != 0) {
        // This is "brief"--we may want to make it so by keeping only the first line
        $output .= '"' . $so->description . '"';
      }
      else {
        $output .= '<p>' . $so->description . '</p>';
      }
    }
    if (($display & 4) != 0) {
      if (($display & 16) != 0) {
        $output .= " ";
      }
      else {
        $output .= '<p>';
      }
      if ($so->p1 != '' && $so->p1 > 0) {
        $amount = theme('lm_paypal_amount', array('amount' => $so->a1, 'cc' => $so->currency_code, 'syms' => TRUE));
        $duration = lm_paypal_unit2str($so->p1, $so->t1);
        $output .= t('First trial period %amount for %duration.', array('%amount' => $amount, '%duration' => $duration));
      }
      if ($so->p2 != '' && $so->p2 > 0) {
        $amount = theme('lm_paypal_amount', array('amount' => $so->a2, 'cc' => $so->currency_code, 'syms' => TRUE));
        $duration = lm_paypal_unit2str($so->p2, $so->t2);
        $output .= t('Second trial period %amount for %duration.', array('%amount' => $amount, '%duration' => $duration));
      }
      $amount = theme('lm_paypal_amount', array('amount' => $so->a3, 'cc' => $so->currency_code, 'syms' => TRUE));
      $duration = lm_paypal_unit2str($so->p3, $so->t3);
      $output .= t('Rate !amount for %duration.', array('!amount' => $amount, '%duration' => $duration));
      if ($so->src) {
        if ($so->srt != '') {
          $times = $so->srt;
          $output .= t('Recurs %times times.', array('%times' => $times));
        }
        else {
          $output .= t('Recurs till cancelled') . '. ';
        }
      }
      if ($so->kind == 0) {
        $role = $roles[$so->rid];
        $output .= t('User becomes member of role %role.', array('%role' => $role));
      }
      elseif ($so->kind == 1) {
        $output .= t('Node becomes viewable by others after the payment was confirmed.');
      }
      elseif ($so->kind == 2) {
        $group = $groups[$so->rid];
        $output .= t('User becomes member of Organic Group %group.', array('%group' => $group));
      }

      // Calculate the length of the subscription - just for debugging for now
      $duration = lm_paypal_subscription_days($so);
      if ($duration == 0) {
        $output .= t('(Duration - till cancelled.)');
      }
      else {
        $output .= t('(Duration %duration days.)', array('%duration' => $duration));
      }

      if (($display & 16) == 0) {
        $output .= '</p>';
      }
    }
    return $output;
  }

  $output = '<h2>' . t('Subscription %subid', array('%subid' => $subid)) . '</h2>';
  $header = array(t('field'), t('value'));
  foreach ($so as $key => $value) {
    if ($value == '') {
      continue;
    }
    if ($key == 'status') {
      $value = t($value ? 'live' : 'defunct');
    }
    elseif ($key == 'src') {
      $value = t($value ? 'recurring' : 'not recurring');
    }
    elseif ($key == 'rid') {
      $value = $roles[$value];
    }
    elseif (strncmp($key, 'p', 1) == 0 && strlen($key) == 2) {
      // use 'p' (period-count) and 't' (period-unit) to format a period
      $unit = $so->{'t' . substr($key, 1)};
      $value = lm_paypal_unit2str($value, $unit);
    }
    elseif (strncmp($key, 't', 1) == 0 && strlen($key) == 2) {
      continue; // skip t (period-unit) fields
    }

    $k = isset($field_names[$key]) ? $field_names[$key] : check_plain($key);
    $value = check_plain($value);
    $rows[] = array('data' => array($k, $value));
  }
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  return $output;
}

// These are binary flags
define('LM_PAYPAL_SUBSCRIPTION_DISPLAY_ITEM_NAME',     1);
define('LM_PAYPAL_SUBSCRIPTION_DISPLAY_DESCRIPTION',   2);
define('LM_PAYPAL_SUBSCRIPTION_DISPLAY_DETAILS',       4);
define('LM_PAYPAL_SUBSCRIPTION_DISPLAY_BUTTON',        8);
define('LM_PAYPAL_SUBSCRIPTION_DISPLAY_BRIEF',        16);
define('LM_PAYPAL_SUBSCRIPTION_DISPLAY_NOHEADERS',    32);
define('LM_PAYPAL_SUBSCRIPTION_DISPLAY_LIST',         64);

/**
 * Display either all live subscriptions or a single one with a link to PayPal.
 *
 * TODO: unless this is used by crazy dynamic code, this wrapper is useless.
 *
 * @param int $subid
 *   If given then a subscribe page is returned otherwise a list of
 *   available subscriptions is returned.
 * @param int $display
 *   If just showing a single subscription then $diplay lists what to show.
 *   1 = item_name
 *   2 = description
 *   4 = human readable details of subscription
 *   8 = button
 *  16 = brief
 *  32 = no section header (applies to list of subscriptions, subid=null)
 *   These can be added to get combinations (eg: 11 = 1 + 2 + 8)
 *  64 = output a comma seperated list the current users subscriptions
 *       (or just none)
 * TODO: change this ugly button
 * @param string button_url
 *   The url of the button to 
 * @param int nid
 *   For node subscriptions this is the node id
 * @param object $account
 *   TODO: ??
 */
function lm_paypal_subscribe($subid = NULL, $display = 15, $button_url = NULL, $nid = NULL, $account = NULL) {
  $subid = (int) $subid;
  if ($subid) {
    return lm_paypal_subscribe_subscription($subid, $display, $button_url, $nid);
  }
  else {
    return lm_paypal_subscribe_select($display, $account);
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function lm_paypal_subscriptions_get_agreement($subid = NULL, $set = FALSE) {
  if ($set) {
    $_SESSION['lm_paypal_I_agree'] = $subid;
  }
  else {
    if (isset($_SESSION['lm_paypal_I_agree'])) {
      $accepted = (bool) $_SESSION['lm_paypal_I_agree'] == $subid;
      unset($_SESSION['lm_paypal_I_agree']);
    }
    else {
      $accepted = FALSE;
    }
    return $accepted;
  }
}

/**
 * The user has to agree to the terms and conditions
 */
function lm_paypal_subscriptions_terms_form($form, $form_state, $subid, $terms, $terms_format) {
  $form['#submit'] = array('lm_paypal_subscription_terms_submit');

  // This reset the subid agreement
  lm_paypal_subscriptions_get_agreement(NULL, FALSE);

  $form['terms'] = array(
    '#type' => 'markup',
    '#value' => t('<h2>Terms and Conditions</h2><p>') . check_markup($terms, $terms_format) .
    t('</p><p><em>Before you can subscribe you must agree to these terms and conditions</em></p>'),
  );
  // Some countries made legally mandatory the checkbox to agress before submit
  // TODO: maybe we should put this checkbox in a variable_get from module
  $form['terms_accepted'] = array(
    '#type' => 'checkbox',
    // TODO: and the text is bad too, we should really put it in a variable too
    '#title' => t('Checking this option, I confirm that I read and understood all terms.'),
    '#default_value' => 0,
  );
  $form['terms_subid'] = array(
    '#type' => 'hidden',
    '#value' => $subid,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('I agree'),
  );
  return $form;
}

function lm_paypal_subscription_terms_submit($form, &$form_state) {
  if ($form_state['values']['terms_accepted']) {
    $subid = ($form_state['values']['terms_subid']);
    lm_paypal_subscriptions_get_agreement($subid, TRUE);
  }
  else {
    drupal_set_message(t('You must accept the terms to proceed to your payment.'));
  }
}

/**
 * Display single subscription with a link to PayPal.
 *
 * @param int $subid
 *   If given then a subscribe page is returned otherwise a list of
 *   available subscriptions is returned.
 * @param int $display = 15
 *   If just showing a single subscription then $diplay lists what to show.
 *   1 = item_name
 *   2 = description
 *   4 = human readable details of subscription
 *   8 = button
 *  16 = brief
 *  32 = no section header (applies to list of subscriptions, subid=null)
 *   These can be added to get combinations (eg: 11 = 1 + 2 + 8)
 *  64 = output a comma seperated list the current users subscriptions
 *       (or just none)
 * @param string button_url = NULL
 *   The url of the button to display
 * @param int nid = NULL
 *   For node subscriptions this is the node id
 */
function lm_paypal_subscribe_subscription($subid, $display = 15, $button_url = NULL, $nid = NULL) {
  $output = '';
  $output .= '<p>' . lm_paypal_subscriptions_details($subid, $display) . '</p>';

  $so = lm_paypal_subscriptions_load_subscription($subid);

  if (($display & 8) == 0) {
    // If I am not displaying a button then I am done.
    return $output;
  }

  $terms = trim($so->terms);
  if (! empty($terms)) {
    if (! lm_paypal_subscriptions_get_agreement($subid)) {
      $output .= drupal_render(drupal_get_form('lm_paypal_subscriptions_terms_form', $subid, $so->terms, $so->terms_format));
      return $output;
    }
  }

  if ($button_url == '') {
    // This is the default paypal subscribe button
    $button_url = 'http://images.paypal.com/images/x-click-but20.gif';
    $button_url = 'http://images.paypal.com/images/x-click-butcc-subscribe.gif';
  }
  else {
    $button_url = check_url($button_url);
  }

  if ($so->return_url != '') {
    $return_path = $so->return_url;
  }
  else {
    $return_path = lm_paypal_subscriptions_get_inprogress();
  }

  $item_name = check_plain($so->item_name);

  // Output a form that will redirect the user to PayPal - note all the fields
  // are hidden so only the submit appears
  $hidden = array();
  if (((int) $so->p1) > 0) {
    $hidden['a1'] = check_plain($so->a1);
    $hidden['p1'] = check_plain($so->p1);
    $hidden['t1'] = check_plain($so->t1);
  }
  if (((int) $so->p2) > 0) {
    $hidden['a2'] = check_plain($so->a2);
    $hidden['p2'] = check_plain($so->p2);
    $hidden['t2'] = check_plain($so->t2);
  }
  if (((int) $so->p3) > 0) {
    $hidden['a3'] = check_plain($so->a3);
    $hidden['p3'] = check_plain($so->p3);
    $hidden['t3'] = check_plain($so->t3);
  }
  if (! empty($so->srt)) {
    $hidden['srt'] = check_plain($so->srt);
    // If recurring times is set but recurring isn't then set it otherwise
    // PayPal will reject this request
    if (empty($so->src)) {
      $hidden['src'] = "1";
    }
  }
  if (! empty($so->src)) {
    $hidden['src'] = check_plain($so->src);
  }
  $hidden['sra'] = "1";
  $hidden['no_note'] = "1";
  $hidden['usr_manage'] = "0";
  
  $form = drupal_get_form(
      'lm_paypal_api_payment_form',
      $so->a3,
      array(
        'item_name' => $item_name,
        'item_number' => $so->subid,
        'return_path' => $return_path,
        'currency_code' => $so->currency_code,
        'custom' => array(
          'nid' => $nid,
          'module' => LM_PAYPAL_SUBSCRIPTIONS,
        ),
        'hidden_fields' => $hidden,
      ),
      LM_PAYPAL_FORM_CMD_SUBSCRIBE
    );
  $output .= drupal_render($form);

  return $output;
}

/**
 * Display all live subscriptions.
 *
 * @param int $display = 15
 *   If just showing a single subscription then $diplay lists what to show.
 *   1 = item_name
 *   2 = description
 *   4 = human readable details of subscription
 *   8 = button
 *  16 = brief
 *  32 = no section header (applies to list of subscriptions, subid=null)
 *   These can be added to get combinations (eg: 11 = 1 + 2 + 8)
 *  64 = output a comma seperated list the current users subscriptions
 *       (or just none)
 * @param object $account = NULL
 *   Drupal account for which you want to subscribe for.
 *   If null, use the current user.
 */
// TODO: check this method
function lm_paypal_subscribe_select($display = 15, $account = NULL) {
  if (!$account) {
    global $user;
    $account = $user;
  }
  $logged_in = $account->uid != 0;

  // Output a list of all the live role or group subscriptions
  $output = '';
  $result = db_query('SELECT subid FROM {lm_paypal_subscriptions} WHERE status = :status AND (kind = :kind OR kind = :kind2)', array(':status' => 1, ':kind' => 0, ':kind2' => 2));

  if (($display & 64) != 0) {
    $nsubs = 0;
    foreach ($result as $data) {
      if ($subscription = lm_paypal_subscriptions_load_subscription($data->subid)) {
        $already = lm_paypal_subscriptions_user_subscribed($so, $account);
        if (!$already) {
          continue;
        }
        $item_name = check_plain($subscription->item_name);
        if ($nsubs > 0) {
          $output .= ", ";
        }
        $output .= '"' . $item_name . '"';
        $nsubs++;
      }
      if ($nsubs == 0) {
        $output .= t('<b>none</b>');
      }
    }
    return $output;
  }

  if (($display & 32) == 0) {
    $output .= '<h2>' . t('Subscriptions') . '</h2>';
  }

  $header = array(t('Name'), t('Description'), t('Action'));
  $rows = array();
  foreach ($result as $data) {
    if ($subscription = lm_paypal_subscriptions_load_subscription($data->subid)) {
      $already = lm_paypal_subscriptions_user_subscribed($subscription, $account);

      $item_name = check_plain($subscription->item_name);
      if ($already) {
        $sub = t('already subscribed');
      }
      else {
        if (!$logged_in) {
          $login = l(t('login'), 'user');
          $register = l(t('create new account'), 'user/register');
          $sub = t("You must !login or !register to subscribe", array("!login" => $login, "!register" => $register));
        }
        else {
          $sub = l(
            t('subscribe'),
            "lm_paypal/subscribe/$subscription->subid",
            array('attributes' => array(
              'alt' => t('Link to the subscription page'),
              'title' => t('Click here to subscribe now!'),
            )));
        }
      }

      $rows[] = array('data' => array(
          array(
            'data' => $item_name,
            'class' => 'lm_paypal_paid_adverts_subscriptions_cell',
          ),
          array(
            'data' => $subscription->description,
            'class' => 'lm_paypal_paid_adverts_subscriptions_cell',
          ),
          array(
            'data' => $sub,
            'class' => 'lm_paypal_paid_adverts_subscriptions_cell',
          ),
        ));
    }
  }
  $output .= theme('table', array('header' => $header, 'rows' => $rows));

  $business = variable_get('lm_paypal_business', '');
  $alias = urlencode($business);
  $unsuburl = lm_paypal_api_get_host(TRUE) . "?cmd=_subscr-find&amp;alias=$alias";

  $output .= '<h4>' . t('To Unsubscribe') . '</h4>';
  $output .= '<p>';
  $output .= t('Click unsubscribe to log in to your PayPal account. Click the Details of the subscription in question. Click Cancel Subscription.');
  $output .= "</p><p><a href=\"$unsuburl\">" . t('Unsubscribe') . '</a>';
  $output .= '</p>';

  return $output;
}

/**
 * Returns the default page that users are sent to by PayPal after subscribing.
 *
 * @return
 *   A string containing the page contents.
 */
function lm_paypal_subscriptions_inprogress_page() {
  return drupal_render(theme('lm_paypal_subscription_inprogress_message'));
}
