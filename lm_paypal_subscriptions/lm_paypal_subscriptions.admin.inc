<?php
// $Id: lm_paypal_subscriptions.admin.inc,v 1.1.2.2 2009/05/30 17:05:54 pounard Exp $

/**
 * @file
 * Paypal subscriptions interface, admin pages and forms.
 */

/**
 * Main settings form
 */
function lm_paypal_subscriptions_admin_settings($form, $form_state) {
  $form['lm_paypal_subscriptions_inprogress'] = array(
    '#type' => 'textfield',
    '#title' => t('LM PayPal subscription in progress page'),
    '#default_value' => lm_paypal_subscriptions_get_inprogress(),
    '#maxlength' => 100,
    '#required' => TRUE,
    '#description' => t('The page the user is sent to by PayPal after a subscribe. The default is @default but you might want to point it at a page you have created yourself.', array('@default' => LM_PAYPAL_SUBSCRIPTIONS_INPROGRESS_DEFAULT)),
  );
  $form['lm_paypal_subscriptions_menu_rebuild'] = array(
    '#type' => 'checkbox',
    '#title' => t('Rebuild menu cache on subscription change'),
    '#default_value' => lm_paypal_subscriptions_menu_rebuild(),
    '#description' => t('On a user being added to/removed from a subscription rebuild the menu cache. Needed only if user logout/login fails to get the right menu items.'),
  );
  return system_settings_form($form);
}

/////////////////////////// BELOW THIS LINE CODE IS WORKING BUT UGLY




////////////////////////////// (AND UN-MAINTAINABLE)




















/**
 * Main interface to view subscriptions.
 *
 * @return string
 *   (x)html output
 */
function lm_paypal_subscriptions_view_all_page() {
  $business = lm_paypal_api_get_business();

  if (empty($business)) {
    $url = l(t('LM PayPal base settings'), 'admin/config/lm_paypal/settings');
    drupal_set_message(t("LM PayPal not yet configured! An administrator needs to do this at !settings", array("!settings" => $url)));
  }

  $roles = lm_paypal_subscriptions_roles_list(TRUE);
  $groups = lm_paypal_subscribable_groups();
  
  $output = '';
  
  $links = array(array(
      'title' => t('View all subscribers'),
      'href' => 'admin/config/lm_paypal/subscribers',
    ), array(
      'title' => t('Create new subscription'),
      'href' => 'admin/config/lm_paypal/subscriptions/add',
    ));
  $output .= theme('links', array('links' => $links));

  $output .= '<h2>' . t('Subscriptions') . '</h2>';
  
  $header = array(
    array(
      'data' => t('ID'),
      'field' => 'subid',
    ),
    array(
      'data' => t('Type'),
      'field' => 'kind',
    ),
    array(
      'data' => t('Name'),
      'field' => 'item_name',
    ),
    array(
      'data' => t('Description'),
      'field' => 'description',
    ),
    array('data' => t('Role')), // rid
    array('data' => t('Group')), // ?
    array(
      'data' => t('Status'),
      'field' => 'status',
    ),
    array('data' => t('Subscribers')),
    array('data' => t('Actions')),
  );

  // Output a list of all the existing subscriptions 102
  
    $select = db_select('lm_paypal_subscriptions', 'subs')
    ->extend('TableSort')
    ->fields('subs', array('subid', 'item_name', 'description', 'status', 'kind', 'rid'))
    ->orderByHeader($header)
    ->extend('PagerDefault')
    ->limit(20);
    
  $result = $select->execute();
  #$result = pager_query($query, 20, 0);

  $rows = array();
  foreach ($result as $so) {
    $kind = $so->kind;
    $linkso = $so->subid;
    $item_name = l($so->item_name, 'admin/config/lm_paypal/subscriptions/' . $so->subid . '/view');
    $description = $so->description;
    $status = ($so->status ? 'live' : 'defunct');
    $group = '';
    $role = '';
    if ($kind == 0) {
      $kind = t('Role');
      $linksb = l(t('subscribers'), 'admin/config/lm_paypal/subscriptions/subscribers/' . $so->subid);
      $role = l($roles[$so->rid], "admin/people/permissions/$so->rid");
    }
    elseif ($kind == 1) {
      $kind = t('Node');
      $linksb = l(t('subscribed nodes'), 'admin/config/lm_paypal/subscriptions/' . $so->subid . '/view');
    }
    elseif ($kind == 2) {
      $kind = t('Organic Group');
      $linksb = l(t('subscribers'), 'admin/config/lm_paypal/subscriptions/' . $so->subid . '/view');
      $group = $groups[$so->rid];
    }
    $actions_links = array(array(
        'title' => t('edit'),
        'href' => 'admin/config/lm_paypal/subscriptions/' . $so->subid . '/edit',
      ), array(
        'title' => t('delete'),
        'href' => 'admin/config/lm_paypal/subscriptions/' . $so->subid . '/delete',
      ));
    $actions = theme('links', array('links' => $actions_links));
    $rows[] = array('data' => array($linkso, $kind, $item_name, $description, $role, $group, $status, $linksb, $actions));
  }

  if (! empty($rows)) {
    $output .= theme('table', array('header' => $header, 'rows' => $rows));
    $output .= theme('pager', array('tags' => array()));
  }
  else {
    $output .= '<p>' . t('No subscriptions to display.') . '</p>';
  }

  
  return $output;
}

/**
 * Creates or edits a subscription.
 *
 * @param int $subid
 *   If provided then edit this subscription instead creating a new one
 */
function lm_paypal_subscriptions_edit_form($form, $form_state, $subid = NULL) {
  $form = array();

  $subid = (int) $subid;
  $edit = $subid > 0;

  if ($edit) {
    // Look up the default values
    $so = lm_paypal_subscriptions_load_subscription($subid);
    $form['edit_subid'] = array(
      '#type' => 'value',
      '#value' => $subid,
    );
    drupal_set_title(t('Subscription @name', array('@name' => $so->item_name)), PASS_THROUGH);
  }
  else {
    // Initialise a new subscription
    $so = new stdClass;
    $so->item_name = '';
    $so->description = '';
    $so->kind = 0;
    $so->rid = NULL;
    $so->a1 = $so->p1 = $so->t1 = '';
    $so->a2 = $so->p2 = $so->t2 = '';
    $so->a3 = $so->p3 = $so->t3 = '';
    $so->srt = NULL;
    $so->currency_code = NULL;
    $so->return_url = '';


    $so->src = 0;
    $so->uid_admin = LM_PAYPAL_SUBSCRIPTIONS_UID_ADMIN_DEFAULT;
    $so->send_admin_onsub = LM_PAYPAL_SUBSCRIPTIONS_SEND_ADMIN_ONSUB_DEFAULT;
    $so->send_admin_onend = LM_PAYPAL_SUBSCRIPTIONS_SEND_ADMIN_ONEND_DEFAULT;

    $so->send_user_onsub = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONSUB_DEFAULT;
    $so->send_user_onsub_subject = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONSUB_SUBJECT_DEFAULT;
    $so->send_user_onsub_body = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONSUB_BODY_DEFAULT;

    $so->send_user_onnearend = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONNEAREND_DEFAULT;
    $so->nearend_days = LM_PAYPAL_SUBSCRIPTIONS_NEAREND_DAYS_DEFAULT;
    $so->send_user_onnearend_subject = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONNEAREND_SUBJECT_DEFAULT;
    $so->send_user_onnearend_body = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONNEAREND_BODY_DEFAULT;

    $so->send_user_onend = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONEND_DEFAULT;
    $so->send_user_onend_subject = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONEND_SUBJECT_DEFAULT;
    $so->send_user_onend_body = LM_PAYPAL_SUBSCRIPTIONS_SEND_USER_ONEND_BODY_DEFAULT;

    $so->terms = LM_PAYPAL_SUBSCRIPTIONS_TERMS_DEFAULT;
  }

  $form['item_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Subscription Name'),
    '#maxlength' => 127,
    '#default_value' => $so->item_name,
    '#required' => TRUE,
    '#description' => t('The name of the subscription'),
  );

  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#rows' => 5,
    '#default_value' => $so->description,
    '#required' => TRUE,
  );

  $kinds = array(
    0 => 'Role',
    1 => 'Node',
  );
  $descr = t('Kind of subscription. Role subscriptions for users to gain a role. Node subscription to make a private node public.');

  $og = (function_exists('og_subscribe_user'));

  if ($og) {
    $kinds[2] = 'Organic Group';
    $descr .= t('Organic Group for users to gain a group');
    $og = TRUE;
  }

  $form['kind'] = array(
    '#type' => 'select',
    '#title' => t('Subscription Kind'),
    '#options' => $kinds,
    '#default_value' => $so->kind,
    '#required' => TRUE,
    '#description' => $descr,
  );

  $form['rid'] = array(
    '#type' => 'select',
    '#title' => t('Subscribers Role'),
    '#options' => lm_paypal_subscriptions_roles_list(TRUE),
    '#default_value' => $so->rid,
    //'#required' => TRUE, - not required for node-subs only role-subs
    '#description' => t('The role subscribers become members of. Only used when kind is Role'),
  );

  if ($og) {
    // NOTE: Reusing the so->rid field for the Organic Group id
    $form['gid'] = array(
      '#type' => 'select',
      '#title' => t('Subscribers Organic Group'),
      '#options' => lm_paypal_subscribable_groups(),
      '#default_value' => $so->rid,
      //'#required' => TRUE, - not required for node-subs only role-subs
      '#description' => t('The Organic Group subscribers become members of. Only used when kind is Organic Group. Only "Invite Only" groups can be used in subcsriptions.'),
    );
  }

  $form['a3'] = array(
    '#type' => 'textfield',
    '#title' => t('Regular rate'),
    '#maxlength' => 10,
    '#default_value' => $so->a3,
    '#required' => TRUE,
    '#description' => t('This is the price of the subscription. (The currency is specified below.)'),
  );

  $form['p3'] = array(
    '#type' => 'textfield',
    '#title' => t('Regular billing cycle'),
    '#maxlength' => 10,
    '#default_value' => $so->p3,
    '#required' => TRUE,
    '#description' => t('This is the length of the billing cycle. The number is modified by the regular billing cycle units'),
  );

  $form['t3'] = array(
    '#type' => 'select',
    '#title' => t('Regular billing cycle units'),
    '#options' => lm_paypal_api_get_period_unit_list(FALSE),
    '#default_value' => $so->t3,
    '#required' => TRUE,
    '#description' => t('This is the units of the regular billing cycle'),
  );

  $form['src'] = array(
    '#type' => 'checkbox',
    '#title' => t('Recurring payments'),
    '#default_value' => $so->src,
    '#description' => t('If set the payment will recur unless your customer cancels the subscription before the end of the billing cycle. If omitted, the subscription payment will not recur at the end of the billing cycle'),
  );

  $form['srt'] = array(
    '#type' => 'textfield',
    '#title' => t('Recurring Times'),
    '#maxlength' => 10,
    '#default_value' => $so->srt,
    '#description' => t('This is the number of payments which will occur at the regular rate. If omitted, payment will continue to recur at the regular rate until the subscription is cancelled. Requires Recurring payments to be set. <b>If set it must be at least 2</b>.'),
  );

  $form['currency_code'] = array(
    '#type' => 'select',
    '#title' => t('The currency of the payment(s)'),
    '#default_value' => $so->currency_code ? $so->currency_code : lm_paypal_api_get_currency_default(),
    '#options' => lm_paypal_api_get_currency(),
    '#description' => t('The currency to use in all payments relating to this subscription.'),
  );

  $form['return_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Return URL'),
    '#maxlength' => 200,
    '#default_value' => $so->return_url,
    '#description' => t('If set this is the URL the user is returned to after completing the transaction at PayPal. If can be used to override the default "thank you" page with a page of your own.'),
  );

  $form['trial'] = array(
    '#type' => 'fieldset',
    '#title' => t('Optional Trial Periods'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['trial']['a1'] = array(
    '#type' => 'textfield',
    '#title' => t('Trial amount 1'),
    '#maxlength' => 10,
    '#default_value' => $so->a1,
    '#description' => t('This is the price of the first trial period. For a free trial set this to 0'),
  );

  $form['trial']['p1'] = array(
    '#type' => 'textfield',
    '#title' => t('Trial period 1'),
    '#maxlength' => 10,
    '#default_value' => $so->p1,
    '#description' => t('This is the length of the first trial period. The number is modified by the trial period 1 units'),
  );

  $form['trial']['t1'] = array(
    '#type' => 'select',
    '#title' => t('Trial period 1 units'),
    '#options' => lm_paypal_api_get_period_unit_list(),
    '#default_value' => $so->t1,
    '#description' => t('This is the units of trial period 1'),
  );

  $form['trial']['a2'] = array(
    '#type' => 'textfield',
    '#title' => t('Trial amount 2'),
    '#maxlength' => 10,
    '#default_value' => $so->a2,
    '#description' => t('This is the price of the second trial period. For a free trial set this to 0'),
  );

  $form['trial']['p2'] = array(
    '#type' => 'textfield',
    '#title' => t('Trial period 2'),
    '#maxlength' => 10,
    '#default_value' => $so->p2,
    '#description' => t('This is the length of the second trial period. The number is modified by the trial period 2 units'),
  );

  $form['trial']['t2'] = array(
    '#type' => 'select',
    '#title' => t('Trial period 2 units'),
    '#options' => lm_paypal_api_get_period_unit_list(),
    '#default_value' => $so->t2,
    '#description' => t('This is the units of trial period 2'),
  );

  $form['email_admin'] = array(
    '#type' => 'fieldset',
    '#title' => t('Email administrator options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['email_admin']['uid_admin'] = array(
    '#type' => 'textfield',
    '#title' => t('UID Admin'),
    '#maxlength' => 10,
    '#default_value' => $so->uid_admin,
    '#description' => t('UID of this subscriptions administrator'),
  );

  $form['email_admin']['send_admin_onsub'] = array(
    '#type' => 'checkbox',
    '#title' => t('Email admin on subscription'),
    '#default_value' => $so->send_admin_onsub,
    '#description' => t('Email this subscriptions administrator when someone subscribes'),
  );

  $form['email_admin']['send_admin_onend'] = array(
    '#type' => 'checkbox',
    '#title' => t('Email admin on subscription end'),
    '#default_value' => $so->send_admin_onend,
    '#description' => t('Email this subscriptions administrator when someones subscription ends by either cancel or end-of-term'),
  );

  $form['email_user'] = array(
    '#type' => 'fieldset',
    '#title' => t('Email subscriber options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['email_user']['send_user_header1'] = array(
    '#value' => '<p>' . t('The following variables can be used in the subject and body of messages:') . '</p>' .
      '<ul><li>%Subscription - the subscriptions name</li><li>%Username - the subscribers name</li><li>%Login - the subscribers login</li><li>%Site - this sites name</li><li>%Uri - this uri</li><li>%Uri_brief - this uri stripped of http://</li><li>%Mailto - target of this email</li><li>%Date - the curent date/time</li></ul>',
  );

  $form['email_user']['send_user_onsub'] = array(
    '#type' => 'checkbox',
    '#title' => t('Email user on subscription'),
    '#default_value' => $so->send_user_onsub,
    '#description' => t('Email the user when they subscribe'),
  );

  $form['email_user']['send_user_onsub_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('New subscription email subject'),
    '#maxlength' => 80,
    '#default_value' => $so->send_user_onsub_subject,
    '#description' => t('Subject of email sent to a user on a new subscription'),
  );

  $form['email_user']['send_user_onsub_body'] = array(
    '#type' => 'textarea',
    '#title' => t('New subscription email body'),
    '#default_value' => $so->send_user_onsub_body,
    '#description' => t('Body of email sent to a user on a new subscription'),
  );

  $form['email_user']['onnearend_header'] = array(
    '#value' => t('<br/>'),
  );

  $form['email_user']['send_user_onnearend'] = array(
    '#type' => 'checkbox',
    '#title' => t('Email user near subscription end'),
    '#default_value' => $so->send_user_onnearend,
    '#description' => t('Email the user when their subscription ends or is cancelled'),
  );

  $form['email_user']['nearend_days'] = array(
    '#type' => 'textfield',
    '#title' => t('Days near end to email subscriber'),
    '#maxlength' => 10,
    '#validate' => array('lm_paypal_is_integer_between' => array(0)),
    '#default_value' => $so->nearend_days,
    '#description' => t('Days before end of subscription to email subscriber'),
  );

  $form['email_user']['onnearend_note'] = array(
    '#value' => '<p>' . t('An extra variable %Days, days to end of subscription, is also available') . '</p>',
  );

  $form['email_user']['send_user_onnearend_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subscription near end email subject'),
    '#maxlength' => 80,
    '#default_value' => $so->send_user_onnearend_subject,
    '#description' => t('Subject of email sent to a user near end of a subscription'),
  );

  $form['email_user']['send_user_onnearend_body'] = array(
    '#type' => 'textarea',
    '#title' => t('Subscription near end email body'),
    '#default_value' => $so->send_user_onnearend_body,
    '#description' => t('Body of email sent to a user near end of a subscription'),
  );

  $form['email_user']['onend_header'] = array(
    '#value' => t('<br/>'),
  );

  $form['email_user']['send_user_onend'] = array(
    '#type' => 'checkbox',
    '#title' => t('Email user on subscription end'),
    '#default_value' => $so->send_user_onend,
    '#description' => t('Email the user when their subscription ends or is cancelled'),
  );

  $form['email_user']['send_user_onend_note'] = array(
    '#value' => '<p>' . t('An extra variable %End, reason for end of subscription, is also available') . '</p>',
  );

  $form['email_user']['send_user_onend_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subscription end email subject'),
    '#maxlength' => 80,
    '#default_value' => $so->send_user_onend_subject,
    '#description' => t('Subject of email sent to a user on a subscription end or cancellation'),
  );

  $form['email_user']['send_user_onend_body'] = array(
    '#type' => 'textarea',
    '#title' => t('Subscription end email body'),
    '#default_value' => $so->send_user_onend_body,
    '#description' => t('Body of email sent to a user on a subscription end or cancellation'),
  );

  $form['terms'] = array(
    '#type' => 'textarea',
    '#title' => t('Terms and conditions'),
    '#default_value' => $so->terms,
    '#description' => t('Terms and conditions the user must agree to. If empty no terms or conditions are required.'),
    '#format' => isset($edit['format']) ? $edit['format'] : NULL,
  );

  if ($edit) {
    $form['status'] = array(
      '#type' => 'select',
      '#title' => t('The status of this subscription definition'),
      '#default_value' => $so->status,
      '#options' => array(
        0 => 'defunct',
        1 => 'live',
      ),
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
    $form['submit_delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('lm_paypal_subscriptions_edit_form_submit_delete'),
    );
  }
  else {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Create'),
    );
  }
  return $form;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function lm_paypal_subscriptions_edit_form_submit_delete($form, &$form_state) {
  if (isset($form_state['values']['edit_subid'])) {
    $subid = (int) $form_state['values']['edit_subid'];
    drupal_goto('admin/config/lm_paypal/subscriptions/' . $subid . '/delete');
  }
}

/**
 * Checks that all the subscription details make sense
 */
function lm_paypal_subscriptions_edit_form_validate($form, &$form_state) {
  $values = $form_state['values'];

  $a1 = $values['a1'];
  $a2 = $values['a2'];
  $a3 = $values['a3'];
  $p1 = $values['p1'];
  $p2 = $values['p2'];
  $p3 = $values['p3'];
  $src = $values['src'];
  $srt = $values['srt'];
  $uid_admin = $values['uid_admin'];
  $rid = $values['rid'];
  $gid = isset($values['gid']) ? $values['gid'] : 0; // not always set
  $kind = $values['kind'];

  if ($kind == 0 && (!is_numeric($rid) || $rid <= 0)) {
    $perr = t('If Subscription Kind is Role then a Subscription Role must be provided.');
    form_set_error('rid', $perr);
  }

  if ($kind == 2 && (!is_numeric($gid) || $gid <= 0)) {
    $perr = t('If Subscription Kind is Organic Group then a group must be provided.');
    form_set_error('gid', $perr);
  }

  // Shame this doesn't work!
  if ($kind == 0) {
    unset($values['gid']);
  }
  elseif ($kind == 2) {
    unset($values['rid']);
  }

  if (!is_numeric($a3) || $a3 <= 0) {
    $perr = t('The amount must positive number greater than 0');
    form_set_error('a3', $perr);
  }

  $perr = t('A trial amount must either be empty or a positive number greater than or equal to 0');
  if ($a1 != '' && (!is_numeric($a1) || $a1 < 0)) {
    form_set_error('a1', $perr);
  }
  if ($a2 != '' && (!is_numeric($a2) || $a2 < 0)) {
    form_set_error('a2', $perr);
  }

  if (!is_numeric($p3) || intval($p3) != $p3 || $p3 <= 0) {
    $perr = t('Regular period must  be a positive integer greater than 0');
    form_set_error('p3', $perr);
  }

  $perr = t('A trial period must either be empty or a positive integer greater than 0');
  if ($p2 != '' && (!is_numeric($p2) || intval($p2) != $p2 || $p2 <= 0)) {
    form_set_error('p2', $perr);
  }
  if ($p3 != '' && (!is_numeric($p3) || intval($p3) != $p3 || $p3 <= 0)) {
    form_set_error('p3', $perr);
  }

  /* Wrong... an empty amount means free
   $perr = t('If a trial period is set the amount must be set too');
   if ($p1 != '' && $a1 == '') {
   form_set_error('a1', $perr);
   }
   if ($p2 != '' && $a2 == '') {
   form_set_error('a2', $perr);
   }
   */

  $perr = t('If a trial amount is set the period must be set too');
  if ($a1 != '' && $p1 == '') {
    form_set_error('p1', $perr);
  }
  if ($a2 != '' && $p2 == '') {
    form_set_error('p2', $perr);
  }

  if ($srt != '') {
    if (!is_numeric($srt) || intval($srt) != $srt) {
      form_set_error('srt', t('Recurring times, if set, must be an integer'));
    }
    else {
      if ($srt < 2) {
        form_set_error('srt', t('Recurring times, if set, must be at least 2'));
      }
    }
    if (!$src) {
      form_set_error('src', t('If recurring times is set Recurring payments must be set too'));
    }
  }

  if (!is_numeric($uid_admin) || intval($uid_admin) != $uid_admin || $uid_admin <= 0) {
    form_set_error('uid_admin', t('UID number be an integer greater than zero'));
  }
}

/**
 * Process form submission to edit or create new subscription definitions.
 * Updates or inserts subscriptions in the lm_paypal_subscriptions table.
 */
function lm_paypal_subscriptions_edit_form_submit($form, &$form_state) {
  $form_state['redirect'] = array('admin/config/lm_paypal/subscriptions');
  
  $values = $form_state['values'];
  if (isset($values['edit_subid'])) {
    $subid = (int) $values['edit_subid'];
    $edit = (bool) $subid;
  }
  else {
    $edit = FALSE;
  }

  $kind = ($values['kind']);

  static $keys = array(
    'rid',
    'gid',
    'kind',
    'item_name',
    'description',
    'a1',
    'p1',
    't1',
    'a2',
    'p2',
    't2',
    'a3',
    'p3',
    't3',
    'src',
    'srt',
    'currency_code',
    'return_url',
    'uid_admin',
    'send_admin_onsub',
    'send_admin_onend',
    'send_user_onsub',
    'send_user_onsub_subject',
    'send_user_onsub_body',
    'send_user_onnearend',
    'nearend_days',
    'send_user_onnearend_subject',
    'send_user_onnearend_body',
    'send_user_onend',
    'send_user_onend_subject',
    'send_user_onend_body',
    'terms',
    'terms_format',
  );

  static $nulls = array(
    'a1',
    'p1',
    't1',
    'a2',
    'p2',
    't2',
    'a3',
    'p3',
    't3',
    'src',
    'srt',
    'currency_code',
    'return_url',
  );

  static $int_fields = array(
    'subid',
    'kind',
    'rid',
    'a1',
    'p1',
    'a2',
    'p2',
    'a3',
    'p3',
    'src',
    'srt',
    'status',
    'uid_admin',
    'send_admin_onsub',
    'send_admin_onend',
    'send_user_onsub',
    'nearend_days',
    'send_user_onnearend',
    'send_user_onend',
    'terms_format',
  );

  // Prepare drupal_write_record
  $object = array();
  if ($edit) {
    $object['subid'] = $subid;
    $object['status'] = $values['status'];
    $update = 'subid';
  }
  else {
    $object['status'] = 1;
    $update = array();
  }

  foreach ($keys as $key) {
    // Skip rid for OG subscription as it is meaningless
    // Skip rid for Node subscriptions (may be null/empty)
    // Also note that I cheat and reuse the db rid field for the OG gid and
    // insert into it below by renaming gid to rid - yucky but quick to write
    if (($kind == 1 || $kind == 2) && $key == 'rid') {
      continue;
    }
    // Ditto for role and node subs - ignore gid
    if (($kind == 0 || $kind == 1) && $key == 'gid') {
      continue;
    }

    $value = (isset($values[$key])) ? $values[$key] : '';
    // NOTE: Reusing the db rid field for the Organic Group id
    if ($key == 'gid') {
      $key = 'rid';
    }

    // Instead of setting a key to empty, if it is a null field then null it!
    if ($value == '' && in_array($key, $nulls)) {
      $object[$key] = NULL;
    }
    else {
      // TODO: do we need db_escape_string($value); ?
      // pounard: I'm sure drupal_write_record does it
      $object[$key] = $value;
    }
  }

  $message_type = 'status';
  switch (drupal_write_record('lm_paypal_subscriptions', $object, $update)) {
    case SAVED_NEW:
      $message = t('Created subscription <em>@kind @sub</em>', array(
        '@kind' => $kind,
        '@sub' => $object['item_name'],
      ));
      break;
    case SAVED_UPDATED:
      $message = t('Edited subscription <em>@kind @sub</em>', array(
        '@kind' => $kind,
        '@sub' => $object['item_name'],
      ));
      break;
    default:
      $message = t('Failed to create subscription');
      $message_type = 'status';
      form_set_error(NULL, 'Failed to create subscription');
  }

  drupal_set_message($message, $message_type);

  if (lm_paypal_debug()) {
    watchdog(LM_PAYPAL_SUBSCRIPTIONS, $message, array(), ($message_type == 'error' ? WATCHDOG_ERROR : WATCHDOG_DEBUG));
  }
}

// lm_paypal_subscription_delete_confirm_form
function lm_paypal_subscription_delete_confirm_form($form_state, $subid) {
  $subscription = lm_paypal_subscriptions_load_subscription($subid);
  $form = confirm_form(array('subid' => array(
      '#type' => 'value',
      '#value' => $subscription->subid,
    )),
    t('Are you sure you want to delete <em>@subscription</em> subscription ?', array(
    '@subscription' => $subscription->item_name,
  )),
    'admin/config/lm_paypal/subscriptions/' . $subscription->subid . '/edit',
    t('This action cannot be undone.')
  );
    $form_state['redirect'] = array('admin/config/lm_paypal/subscriptions'
    );
  return $form;
}

/**
 * Delete a subscription definition.
 *
 * @param $subid
 *   Required. The subid of the definition to delete.
 * @return
 *   A string describing the result of the deletion.
 */
function lm_paypal_subscription_delete_confirm_form_submit($form, &$form_state) {
  if ($subscription = lm_paypal_subscriptions_load_subscription($form_state['values']['subid'])) {
    /* db_query("DELETE FROM {lm_paypal_subscriptions} WHERE subid = %d", $subscription->subid) */
    db_delete('lm_paypal_subscriptions')
      ->condition('subid', $subscription->subid)
      ->execute();
    $message = t('Deleted subscription @subid - @name', array(
      '@subid' => $subscription->subid,
      '@name' => $subscription->item_name,
    ));
    drupal_set_message($message);
    // TODO: is this good as debug information or should we always display it?
    if (lm_paypal_debug()) {
      watchdog(LM_PAYPAL_SUBSCRIPTIONS, $message, array(), WATCHDOG_DEBUG);
    }
  }
}

/**
 * Fake edit to mark a node as paid by a subscriber.
 *
 * TODO: describe params
 */
function lm_paypal_subscriber_pay_form($form, &$form_state, $subid = NULL, $nid = NULL, $uid = NULL) {
  // if anything is missing, no form presented
  if ($subid == NULL || $nid == NULL || $uid == NULL) {
    $form['Invalid Request'] = array(
      '#value' => '<p>You can mark nodes as paid by going to the node and clicking on the <em>Mark as Paid</em> link. You cannot otherwise directly come here to do the same.</p>',
    );
    return $form;
  }

  $form['subscriber_pay'] = array(
    '#markup' => '<p>Mark node ' . $nid . ' as paid by user ' . $uid . ' for subscription ' . $subid . '</p>',
  );

  if ($destination = drupal_get_destination()) {
    $form_state['redirect'] = array($destination);
  }

  $form['pay_subid'] = array(
    '#type' => 'hidden',
    '#default_value' => $subid,
  );
  $form['pay_nid'] = array(
    '#type' => 'hidden',
    '#default_value' => $nid,
  );
  $form['pay_uid'] = array(
    '#type' => 'hidden',
    '#default_value' => $uid,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Mark subscription as paid'),
  );

  return $form;
}

/**
 * Form processor for subscriber pay form.
 *
 * Create a new subscriber and mark the corresponding node as paid
 * for that subscriber. Then jump to the edit page where the admin
 * can change the user status to OFF.
 *
 * Updates subscribers in the lm_paypal_subscrbers table.
 *
 * @param array $form
 *   the form that was just submitted
 * @param array &$form_state
 *   the current state such as the redirect of the form
 * @return void
 */
function lm_paypal_subscriber_pay_form_submit($form, &$form_state) {
  $values = $form_state['values'];

  $subid = $values['pay_subid'];
  $nid   = $values['pay_nid'];
  $uid   = $values['pay_uid'];

  // retrieve the kind
  $so = NULL;
  $sql = "SELECT kind FROM {lm_paypal_subscriptions} WHERE subid = %d AND status = 1";
  $db_res = db_query("SELECT kind FROM {lm_paypal_subscriptions} WHERE subid = :subid AND status = :status", array(':subid' => $subid, ':status' => 1))->fetchCol();

  if (is_array($db_res)) {
    $so = $db_res[0];
  }
  if (!$so) {
    $msg = t('Failed to mark node !nid as paid by subscriber !uid with subscription !subid (sql: !sql)',
      array(
      '!nid' => check_plain($nid),
      '!uid' => check_plain($uid),
      '!subid' => check_plain($subid),
      '!sql' => check_plain($sql),
    )
    );
    watchdog(LM_PAYPAL_SUBSCRIPTIONS, check_plain($msg), NULL, WATCHDOG_ERROR);
    drupal_set_message($msg);
    form_set_error('Failed to mark node as paid by subscriber');
    return;
  }

  $subscr_id = ':auto:node:' . $nid;
  if (!lm_paypal_add_subscriber($so, $uid, $nid, $subid, $subscr_id, LM_PAYPAL_SUBSCRIPTIONS_STATUS_LIVE)) {
    $msg = t('Failed to mark node as paid by subscriber (by adding subscriber to that node).');
    watchdog(LM_PAYPAL_SUBSCRIPTIONS, check_plain($msg), NULL, WATCHDOG_ERROR);
    drupal_set_message($msg);
    form_set_error(NULL, 'Failed to mark node as paid by subscriber');
    return;
  }

  // retrieve the usid now
  $sb = NULL;
  $sql = "SELECT usid FROM {lm_paypal_subscribers} WHERE subscr_id = '%s'";
  $usid = db_query("SELECT usid FROM {lm_paypal_subscribers} WHERE subscr_id = :subscr_id", array(':subscr_id' => $subscr_id))->fetchField();

  if (!$usid) {
    $msg = t('Could not retrieve the subscribtion identifier sql: %sql', array('%sql' => check_plain($sql)));
    watchdog(LM_PAYPAL_SUBSCRIPTIONS, check_plain($msg), NULL, WATCHDOG_ERROR);
    drupal_set_message($msg);
    form_set_error(NULL, 'Failed to mark node as paid by subscriber');
    return;
  }

  $form_state['redirect'] = array("admin/config/lm_paypal/subscriber_edit/$usid");
}

/**
 * Edit a subscriber.
 *
 * @param array $form_state
 *   Form API junk
 * @param int $usid
 *   The subscriber to edit
 * @return array
 *   Form ready for drupal_get_form
 */
function lm_paypal_subscriptions_subscriber_edit_form($form, $form_state, $usid) {
  $sb = lm_paypal_subscriptions_load_subscriber($usid);
  $user = user_load($sb->uid);
  $form['edit_subscriber'] = array(
    '#markup' => '<p>User: ' . l($user->name, 'user/' . $user->uid) . '</p>',
  );

  $form['edit_usid'] = array(
    '#type' => 'hidden',
    '#default_value' => $usid,
  );

  $form['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => lm_paypal_subscription_get_status(),
    '#default_value' => $sb->status,
    '#required' => TRUE,
    '#description' => t('The status of this subscriber'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  return $form;
}

/**
 * Updates subscribers in the lm_paypal_subscrbers table.
 */
function lm_paypal_subscriptions_subscriber_edit_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $usid = ($values['edit_usid']);
  
  $db_res = db_update('lm_paypal_subscribers')
  ->fields(array(
    'status' => $values['status'],
  ))
  ->condition('usid', $usid)
  ->execute();
  if (!$db_res) {
    $msg = t('Failed to update subscriber sql: %sql', array('%sql' => check_plain($db_res)));
    watchdog(LM_PAYPAL_SUBSCRIPTIONS, check_plain($msg), NULL, WATCHDOG_ERROR);
    drupal_set_message($msg);
    form_set_error(NULL, 'Failed to update subscriber');
    return;
  }

  if (lm_paypal_debug()) {
    $msg = t('Updated subscriber: %usid', array('%usid' => $usid));
    watchdog(LM_PAYPAL_SUBSCRIPTIONS, $msg);
    drupal_set_message($msg);
  }
  
  $form_state['redirect'] = array('admin/config/lm_paypal/subscriber_edit/' . $usid);
}

/**
 * View subscibers
 *
 * @param int $subid = NULL
 *   If NOT NULL then just print out details of that subscription
 * @return string
 *   (x)html output
 */
function lm_paypal_subscriptions_subscribers_view_page($subid = NULL) {
  $limit = 50;
  $output = '';
  
  $links = array(array(
    'title' => t('View all subscriptions'),
    'href' => 'admin/config/lm_paypal/subscriptions',
  ));
  $output .= theme('links', array('links' => $links));

  if ($subid = (int) $subid
     && $so = lm_paypal_subscriptions_load_subscription($subid)) {
    drupal_set_title(t('Subscribers for @name', array('@name' => $so->item_name)), PASS_THROUGH);
  }
  else {
    $output .= '<h2>' . t('Subscribers') . '</h2>';
  }

  $output .= drupal_render(drupal_get_form('lm_paypal_subscriptions_subscriber_filter_form', $subid));

  $header = array(
    array(
      'data' => t('Kind'),
      'field' => 's.kind',
    ),
    array(
      'data' => t('User'),
      'field' => 'u.name',
    ),
    array(
      'data' => t('Node'),
      'field' => 's.nid',
    ),
    array(
      'data' => t('Subid'),
      'field' => 's.subid',
    ),
    array(
      'data' => t('Subscription name'),
      'field' => 'd.item_name',
    ),
    array(
      'data' => t('Started'),
      'field' => 's.started',
      'sort' => 'desc',
    ),
    array(
      'data' => t('Status'),
      'field' => 's.status',
    ),
    array(
      'data' => t('PayPal subscription ID'),
      'field' => 's.subscr_id',
    ),
    array(
      'data' => t('Operations'),
    ),
  );

 /*
 $sql = "SELECT u.name, d.item_name,s.kind, s.usid, s.uid, s.nid,s.subid, s.started, s.status, s.subscr_id FROM {lm_paypal_subscriptions} d JOIN {lm_paypal_subscribers} s ON s.subid = d.subid  JOIN {users} u ON s.uid = u.uid";
 $tablesort = tablesort_sql($header);
 */

  $query = db_select('lm_paypal_subscriptions', 'd');
  $alias = $query->join('lm_paypal_subscribers', 's', 's.subid = d.subid');
  $alias = $query->join('users', 'u', 's.uid = u.uid');
  $query = $query->fields('u', array('name'))
            ->fields('d', array('item_name'))
            ->fields('s', array(
              'kind',
              'usid',
              'uid',
              'nid',
              'subid',
              'started',
              'status',
              'subscr_id'
            ))
            ->extend('TableSort')
            ->extend('PagerDefault')
            ->orderByHeader($header)
            ->limit($limit);

  
  // If not sorting by started then make that the 2nd field to sort on
  //if (strpos($tablesort, 'started') === FALSE) {
  //  $tablesort .= ', started DESC';
  //}

  if ($subid) {
    $query->condition('s.subid', $subid);
  }

  $status_filter = lm_paypal_subscriptions_subscriber_filter_get();
  
  if (! empty($status_filter) && $status_filter != 'all') {
    $query->condition('s.status', $status_filter);
  }
  
  $result = $query->execute();
  
  
  
  $rows = array();
  foreach ($result as $sub) {
    $ss = lm_paypal_subscription_get_status($sub->status);
    if ($ss == '') {
      $ss = $sub->status;
    }
    if ($sub->nid != 0) {
      $l = 'node/' . $sub->nid;
      $nid = l($l, $l);
    }
    else {
      $nid = '';
    }
    

    if ($sub->kind == 0) {
      $kind = t('Role');
    }
    elseif ($sub->kind == 1) {
      $kind = t('Node');
    }
    elseif ($sub->kind == 2) {
      $kind = t('Group');
    }

    $rows[] = array(
      'data' => array(
        $kind,
        l($sub->name, 'user/' . $sub->uid),
        $nid,
        l($sub->subid, 'admin/config/lm_paypal/subscriptions/' . $sub->subid),
        check_plain($sub->item_name),
        format_date($sub->started, 'short'),
        $ss,
        $sub->subscr_id,
        l(t('edit'), 'admin/config/lm_paypal/subscribers/' . $sub->usid . '/edit'),
      ),
    );
  }

  if (! empty($rows)) {
    $output .= theme('table', array('header' => $header, 'rows' => $rows));
    $output .= theme('pager', array('tags' => array()));
  }
  else {
    $output .= '<p>' . t('No subs found.') . '</p>';
  }

  return $output;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function lm_paypal_subscriptions_subscriber_filter_form($form, $form_state, $subid) {
  // WTF subid does here?
  $subid = (int) $subid;
  $current_filter = lm_paypal_subscriptions_subscriber_filter_get();

  $form['#prefix'] = '<div class="container-inline">';
  $form['#suffix'] = '</div>';
  $form['filter'] = array(
    '#type' => 'select',
    '#title' => t('Filter subscribers'),
    '#options' => array('all' => t('all')) + lm_paypal_subscription_get_status(),
    '#default_value' => $current_filter ? $current_filter : 'all',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
  );
  $form['submit_reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
    '#submit' => array('lm_paypal_subscriptions_subscriber_filter_form_reset'),
  );
  return $form;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function lm_paypal_subscriptions_subscriber_filter_form_reset($form, &$form_state) {
  lm_paypal_subscriptions_subscriber_filter_get(NULL, TRUE);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function lm_paypal_subscriptions_subscriber_filter_get($filter = NULL, $reset = FALSE) {
  if ($reset) {
    unset($_SESSION['lm_paypal_subs_filter']);
  }
  elseif ($filter) {
    $_SESSION['lm_paypal_subs_filter'] = $filter;
  }
  return $filter;
}

/**
 * Process the form submission for lm_paypal_subscribers
 */
function lm_paypal_subscriptions_subscriber_filter_form_submit($form, &$form_state) {
  lm_paypal_subscriptions_subscriber_filter_get($form_state['values']['filter']);
}

