<?php

/**
 * @file
 *   Aegir service autoloading function and switch based on version and include
 *   the appropriate files to handle install, verify, &c.
 */

require_once('provision_civicrm.inc');
require_once(dirname(__FILE__) . '/deploy.provision.inc');
require_once(dirname(__FILE__) . '/install.provision.inc');
require_once(dirname(__FILE__) . '/migrate.provision.inc');
require_once(dirname(__FILE__) . '/verify.provision.inc');

/**
 * Implements hook_drush_init().
 */
function provision_civicrm_drush_init() {
  // Register our service classes for autoloading.
  provision_civicrm_provision_register_autoload();

  if (_provision_civicrm_is_site_context()) {
    // #2183083 Reset the classloader drupal variable in case we are migrating
    // from a platform to another.
    global $conf;
    $conf['civicrm_class_loader'] = NULL;

    // if other modules use PEAR (ex: print_mail), it may cause a PHP conflict
    // so declare our own include path here. See: #1431874
    $crmPath = _provision_civicrm_get_package_path();
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );

    // _provision_civicrm_check_upgrade_mode();
  }
}

/**
 * Register our directory as a place to find Provision classes.
 *
 * This allows Provision to autoload our classes, so that we don't need to
 * specifically include the files before we use the class.
 */
function provision_civicrm_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    $list = drush_commandfile_list();
    $provision_dir = dirname($list['provision']);
    include_once($provision_dir . '/provision.inc');
    include_once($provision_dir . '/provision.service.inc');
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

/**
 * Implementation of hook_drush_command().
 */
function provision_civicrm_drush_command() {
  $items['provision-civicrm-cli'] = array(
    'description' => 'Run a civicrm cv command.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'aliases' => array('cv'),
  );

  return $items;
}
