<?php

/**
 * @file
 *
 * CiviCRM support module for the Aegir backend.
 *
 * This file contains the verify operations.
 */


/**
 * Implements hook_pre_provision_verify().
 *
 * Keep old values of paths/url so that we can later search and replace
 * in the civicrm_domain table
 */
function drush_provision_civicrm_pre_provision_verify($url = NULL) {
  if (_provision_civicrm_is_site_context()) {
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    drush_set_option('civicrm_old_path', $drupalRoot);

    $baseUrl = 'http://' . drush_get_option('uri', false);
    drush_set_option('civicrm_old_baseurl', $baseUrl);
  }
}

/**
 * Implements hook_provision_verify().
 */
function drush_provision_civicrm_provision_verify($url = NULL) {
  if (! _provision_civicrm_is_site_context()) {
    return;
  }

  drush_log(dt("CiviCRM: Running drush_civicrm_provision_verify"));

  // Precaution to avoid "duplicate entry" that sometimes happens
  // Set to NULL instead of delete, because sometimes the quick write/read might
  // cause Drupal to complicate about duplicate entries.
  if (drush_drupal_major_version() <= 7) {
    variable_set('civicrm_class_loader', NULL);
  }

  // This needs to be done before we initialize CiviCRM,
  // otherwise we will load with an old include path (civicrm_root).
  _provision_civicrm_regenerate_settings();

  // Most importantly, get rid of the Config.IDS.ini
  _provision_civicrm_clear_config_cache();

  // Enabling the upgrade mode (if necessary) will avoid pesky issues
  // during the Drupal upgrade.
  _provision_civicrm_check_upgrade_mode();

  drush_log(dt("CiviCRM: Finished drush_civicrm_provision_verify"));
}

/**
 * Implements hook_post_provision_verify().
 *
 * Persist civicrm settings in the drushrc.php
 */
function drush_provision_civicrm_post_provision_verify($url = NULL) {
  if (! _provision_civicrm_is_site_context(TRUE)) {
    return;
  }

  if (d()->type == 'site') {
    drush_include_engine('drupal', 'environment');
    $modules = drush_get_modules();

    if (! $modules['civicrm']->status) {
      drush_log(dt("CiviCRM: not enabled. Skipping verify operations for CiviCRM. You will need to re-verify the site if you enable CiviCRM in the future."));
      return;
    }
  }
  elseif (d()->type == 'wpsite' && ! function_exists('civicrm_initialize')) {
    drush_log(dt("CiviCRM: not enabled. Skipping verify operations for CiviCRM. You will need to re-verify the site if you enable CiviCRM in the future."));
    return;
  }

  if (d()->type == 'wpsite') {
    drush_log(dt("CiviCRM: WordPress integration not yet fully supported. Skipping post-verify."), 'warning');
    return;
  }

  _civicrm_init();
  _provision_civicrm_check_upgrade_mode();

  // verify might return fail if, for example, this is a very old version of CiviCRM
  // in which case, no point continuing with the upgrade and cache clear.
  if (provision_civicrm_verify_common()) {
    // In some circumstances during migrations, template_c directory can be
    // deleted by a cache clear and the upgrade will fail.
    provision_civicrm_setup_temp_directories();

    // Run the CiviCRM upgrade procedure
    _provision_civicrm_upgradedb();

    // Only flush the cache if we are not upgrading. The upgrader already does a flush, and could
    // lead to odd errors such as: 'The dispatch policy prohibits event "hook_civicrm_permission"'
    if (!_provision_civicrm_check_upgrade_mode()) {
      // Precaution to avoid "duplicate entry" that sometimes happens
      if (drush_drupal_major_version() <= 7) {
        drush_log(dt("CiviCRM: deleting the civicrm_class_loader variable"), 'ok');
        variable_set('civicrm_class_loader', NULL);
      }

      // Aprox. CiviCRM >= 4.2
      if (function_exists('civicrm_api3')) {
        // Run as a shell command, instead of PHP, so that fatal errors do not crash the verify task
        $command = 'cd ' . escapeshellarg(d()->site_root) . ' && drush cc drush && drush @' . escapeshellarg(d()->uri) . ' cvapi --out=json system.flush';
        drush_log(dt("CiviCRM: calling cache clear: %command", ['%command' => $command]), 'ok');
        $json = system($command, $retval);
        $output = json_decode($json, TRUE);
        if ($retval || !empty($output['is_error'])) {
          drush_log(dt("CiviCRM: cache clear failed (the ConfigAndLog should have more details): %output", ['%output' => $json]), 'warning');
        }
        elseif (!preg_match('/is_error.:0/', $json)) {
          drush_log(dt("CiviCRM: cache clear may have failed: %output", ['%output' => $json]), 'warning');
        }
        else {
          drush_log(dt("CiviCRM: cache clear success: %output", ['%output' => $json]), 'ok');
        }
      }
      else {
        drush_log(dt("CiviCRM: Could not find the function to flush the cache. What version of CiviCRM are you using?"), 'warning');
      }

      drush_log(dt("CiviCRM: cache clear finished"));
    }
  }
}

/**
 * Implements hook_provision_apache_vhost_config().
 *
 * Inject a "deny" statement in the Apache vhost on the files/civicrm
 * directory, in order to restrict access to uploaded files, templates,
 * logs, etc.
 *
 * If you have custom CSS or JS to include, you should do that
 * from a Drupal module.
 */
function provision_civicrm_provision_apache_vhost_config($data = null) {
  if (! _provision_civicrm_is_site_context()) {
    return;
  }

  // Block access to user-uploaded sensitive files. See:
  // https://docs.civicrm.org/dev/en/latest/framework/filesystem/
  $cividir = d()->site_path . '/files/civicrm';
  $htaccess = '';

  $block = [
    'ConfigAndLog',
    'custom',
    'upload',
    'templates_c',
  ];

  foreach ($block as $b) {
    $htaccess .=
      "<Directory \"$cividir/$b\">\n"
    . "  Order allow,deny\n"
    . "  Deny from all\n"
    . "</Directory>\n";
  }

  return $htaccess;
}
