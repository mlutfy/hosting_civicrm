<?php

/**
 * @file
 *
 * CiviCRM support module for the Aegir backend.
 *
 * This file contains the deploy operations (mainly for migrate/import tasks).
 */

/**
 * Implements hook_pre_provision_deploy().
 *
 * Keep old values of paths/url so that we can later search and replace
 * in the civicrm_domain table
 */
function drush_provision_civicrm_pre_provision_deploy($url = NULL) {
  if (_provision_civicrm_is_site_context()) {
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    drush_set_option('civicrm_old_path', $drupalRoot);

    $baseUrl = 'http://' . drush_get_option('uri', false);
    drush_set_option('civicrm_old_baseurl', $baseUrl);
  }
}

/**
 * Implements hook_provision_deploy().
 */
function drush_provision_civicrm_provision_deploy() {
  if (! _provision_civicrm_is_site_context(TRUE)) {
    return;
  }

  // This needs to be done before we initialize CiviCRM,
  // otherwise we will load with an old include path (civicrm_root).
  //
  // c.f. #1907884 The Drupal cache clear in drupal's post_provision hook
  // causes CiviCRM 4.2 to call civicrm_initialize() before we have had the
  // time to regenerate the settings.
  _provision_civicrm_regenerate_settings();
}

/**
 * Implements hook_post_provision_deploy().
 */
function drush_provision_civicrm_post_provision_deploy() {
  if (! _provision_civicrm_is_site_context(TRUE)) {
    return;
  }

  // Do not check blocked_ips, otherwise it will crash when doing an
  // upgrade from drupal6 to drupal7. We need to bootstrap in order to
  // delete the civicrm_class_loader variable.
  global $conf;
  $conf['blocked_ips'] = array();

  if (! drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_DATABASE)) {
    drush_log(dt("CiviCRM: failed to bootstrap Drupal for deploy."));
    return;
  }

  drush_log(dt("CiviCRM: Running drush_civicrm_post_provision_deploy"));

  // Drupal8 has "cdel", but seems like civicrm on d8 doesn't use this?
  if (drush_drupal_major_version() <= 7) {
    drush_variable_delete('civicrm_class_loader');
  }

  drush_log(dt("CiviCRM: Finished drush_civicrm_post_provision_deploy"));
}
