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

  // Full bootstrap is required so that we can use CiviCRM functions
  if (!drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL)) {
    drush_log(dt("CiviCRM: failed to bootstrap Drupal for deploy."), 'warning');
    return;
  }

  drush_log(dt("CiviCRM: Running drush_civicrm_post_provision_deploy"));

  // Check if Mosaico exists, rename paths in the database.
  $mosaico_exists = FALSE;

  if (!provision_civicrm_initialize()) {
    drush_log(dt("CiviCRM: drush_civicrm_post_provision_deploy failed to initialize CiviCRM"));
    return;
  }

  $mosaico_exists = CRM_Core_DAO::checkTableExists('civicrm_mosaico_template');

  if ($mosaico_exists) {
    $new_url = drush_get_option('uri');
    $old_url = drush_get_option('old_uri');

    CRM_Core_DAO::executeQuery('UPDATE civicrm_mosaico_template SET metadata = REPLACE(metadata, %1, %2)', [
      1 => [$old_url, 'String'],
      2 => [$new_url, 'String'],
    ]);

    // Required so that re-using old mailings still works
    CRM_Core_DAO::executeQuery('UPDATE civicrm_mailing SET template_options = REPLACE(template_options, %1, %2)', [
      1 => [$old_url, 'String'],
      2 => [$new_url, 'String'],
    ]);
  }

  // This variable has long been deprecated from CiviCRM, but we are keeping
  // this just in case we are handling an upgrade/import from a very old version.
  // Not checking for Drupal8 because it is a very unlikely use-case.
  if (drush_drupal_major_version() <= 7) {
    drush_variable_delete('civicrm_class_loader');
  }

  drush_log(dt("CiviCRM: Finished drush_civicrm_post_provision_deploy"));
}
