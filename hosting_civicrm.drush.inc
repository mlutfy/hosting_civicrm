<?php

/**
 * Implementation of hook_hosting_site_context_options().
 */
function hosting_civicrm_hosting_site_context_options(&$task) {
  if (isset($task->ref->civicrm_version) && !empty($task->ref->civicrm_version)) {
    $task->context_options['civicrm_version'] = $task->ref->civicrm_version;
  }
  else {
    $task->context_options['civicrm_version'] = 'null';
  }

  $task->context_options['civicrm_ansible_sitekey'] = $task->ref->civicrm_ansible_sitekey ?? $task->ref->civicrm_sitekey ?? '';
  $task->context_options['civicrm_ansible_credkey'] = $task->ref->civicrm_ansible_credkey ?? '';
  $task->context_options['civicrm_ansible_signkey'] = $task->ref->civicrm_ansible_signkey ?? '';
  // Legacy
  $task->context_options['civicrm_sitekey'] = $task->context_options['civicrm_ansible_sitekey'];
}

/**
 * Implementation of hook_drush_context_import().
 */
function hosting_civicrm_drush_context_import($context, &$node) {
  if ($context->type == 'site') {
    if (isset($context->civicrm_version) && !empty($context->civicrm_version)) {
      $node->civicrm_version = $context->civicrm_version;
    }

    if (isset($context->civicrm_sitekey) && !empty($context->civicrm_sitekey)) {
      $node->civicrm_sitekey = $context->civicrm_sitekey;
    }

    // Not sure what this does, transition towards Aegir-on-Backdrop
    $node->civicrm_ansible_sitekey = $context->civicrm_ansible_sitekey ?? NULL;
    $node->civicrm_ansible_credkey = $context->civicrm_ansible_credkey ?? NULL;
    $node->civicrm_ansible_sitekey = $context->civicrm_ansible_sitekey ?? NULL;
  }
}

/**
 * Implements hook_post_hosting_TASK_TYPE_task().
 */
function hosting_civicrm_post_hosting_install_task($task, $data) {
  if ($task->ref->type == 'site' && isset($task->ref->civicrm_version)) {
    // Verify the site to fix permissions and ownership, if applicable.
    hosting_add_task($task->rid, 'verify');
  }
}
