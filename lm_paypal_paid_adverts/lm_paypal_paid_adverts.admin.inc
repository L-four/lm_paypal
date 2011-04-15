<?php

function _lm_paypal_paid_adverts_get_subscriptions() {
  static $cache;
  if (! $cache) {
    $cache = array();
    $result = db_query("SELECT a.*,s.status FROM {lm_paypal_paid_adverts} a JOIN {lm_paypal_subscriptions} s ON a.subid = s.subid");
    foreach ($result as $object) {
      $cache[$object->node_type][$object->subid] = $object;
    }
  }
  return $cache;
}

function lm_paypal_paid_adverts_settings_form() {
  $form['header'] = array(
    '#type' => 'markup',
    '#markup' => '<h2>' . t('Associate node subscriptions with node types.') . '</h2>',
  );
  $form['note'] = array(
    '#type' => 'markup',
    '#markup' => '<p><em>' . t('Once a subscription is associated with a node type when new nodes of that type are created they will be PRIVATE to the author until paid for.') . '</em></p>',
  );

  $node_subs = lm_paypal_subscription_node_subs();

  if (empty($node_subs)) {
    drupal_set_message(t('No node subscriptions defined.'), 'error');
  }
  else {
    $node_types = node_type_get_names();
    $data = _lm_paypal_paid_adverts_get_subscriptions();

    // Send form data as a nested array, rather than flattening it.
    $form['live'] = array('#tree' => TRUE);

    foreach ($node_types as $node_type => $node_name) {
      $options = array();
      $values = array();
      foreach ($node_subs as $subid => $sub_data) {
        $subscription = lm_paypal_subscriptions_load_subscription($subid);
        $options[$subid] = theme('lm_paypal_subscription_name', array('subscription' => $subscription));
        if (isset($data[$node_type][$subid])) {
          $values[$subid] = $data[$node_type][$subid]->subid;
        }
      }
      $form['live'][$node_type] = array(
        '#type' => 'checkboxes',
        '#title' => t('Subscriptions for %node_type nodes', array('%node_type' => $node_name)),
        '#options' => $options,
        '#default_value' => $values,
      );
    }
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  return $form;
}

function lm_paypal_paid_adverts_settings_form_submit($form, &$form_state) {
  db_query("DELETE FROM {lm_paypal_paid_adverts}");
  foreach ($form_state['values']['live'] as $node_type => $subscriptions) {
    foreach ($subscriptions as $subid => $enabled) {
      if ($enabled) {
        $object = array(
          'node_type' => $node_type,
          'subid' => $subid,
        );
        drupal_write_record('lm_paypal_paid_adverts', $object);
      }
    }
  }
}
