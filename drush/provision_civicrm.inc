<?php

/**
 * @file
 * Hookup with drush
 * Common functions used by install and verify.
 */

/**
 * Expose the service type this extension defines to provision.
 *
 * @return
 *   An array with the service type the key, and the default implementation the value.
 */
function provision_civicrm_provision_services() {
  provision_civicrm_provision_register_autoload();
  return array(
    'civicrm' => NULL,
    'civicrm_db' => NULL,
  );
}

/**
 * Wrapper to initialize CiviCRM, depending on the CMS.
 *
 * Returns FALSE if this is not a CiviCRM site (ex: hostmaster).
 */
function provision_civicrm_initialize() {
  if (drush_drupal_major_version() >= 8 && \Drupal::hasService('civicrm')) {
    \Drupal::service('civicrm')->initialize();
    return TRUE;
  }
  elseif (function_exists('civicrm_initialize')) {
    civicrm_initialize();
    return TRUE;
  }

  return FALSE;
}

/**
 * Get the default CiviCRM locale.
 * This is inspired from CRM_Core_Smarty::getLocale().
 */
function _provision_civicrm_get_default_locale() {
  global $tsLocale;

  $config = CRM_Core_Config::singleton();

  if (!empty($config->lcMessages)) {
    return $config->lcMessages;
  }

  return 'en_US';
}

/**
 * Get an array of civicrm related options
 *
 * @param $op
 *   A string describing the task being run, for debugging purposes
 */
function _provision_civicrm_drush_options($op = 'unknown') {
  drush_log(dt("CiviCRM: In @op: Fetching CiviCRM Drush options", array('@op' => $op)));

  $civi_opts = array(
    'civicrm_cron_username' => '',
    'civicrm_cron_password' => '',
    'civicrm_sitekey' => '',
  );

  foreach ($civi_opts as $opt => $val) {
    $civi_opts[$opt] = drush_get_option($opt, NULL);
  }

  return $civi_opts;
}

/**
 * Check we are in an Aegir site context, and that CiviCRM is available in the platform.
 *
 * @param Boolean $load_if_missing Recommended in most cases. See do#2364871.
 * @see _provision_civicrm_get_package_path();
 */
function _provision_civicrm_is_site_context($load_if_missing = FALSE) {
  if (d()->type == 'site') {
    $crmpath = _provision_civicrm_get_package_path($load_if_missing);
    return ($crmpath ? TRUE : FALSE);
  }
  elseif (d()->type == 'wpsite') {
    // Requires hosting_wordpress in Aegir.
    if (function_exists('civicrm_initialize')) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
 * Get the path where the CiviCRM module is installed
 * Ex: /var/aegir/platforms/[...]/sites/all/modules/civicrm
 *
 * NB: if CiviCRM was not detected, verify the platform.
 *
 * @param Boolean $load_if_missing Recommended in most cases. See do#2364871.
 */
function _provision_civicrm_get_package_path($load_if_missing = FALSE) {
  // Issue#2942572 For D8 and D9, we only support having CiviCRM in /vendor
  // Aegir package info functions are not CiviCRM-friendly. This is simpler.
  $drupal_core_version = drush_drupal_major_version();

  if (function_exists('provision_get_drupal_core_version')) {
    if ($v = provision_get_drupal_core_version()) {
      $drupal_core_version = $v;
    }
  }

  if (version_compare($drupal_core_version, 8, '>=')) {
    $root = d()->root;
    // If Drupal lives under a 'web' subdirectory (which is usually the case with composer),
    // we remove the prefix, since the vendor directory is outside 'web'.
    $root = preg_replace('#web/?$#', '', $root);
    $crmpath = $root . '/vendor/civicrm/civicrm-core/';

    if (is_dir($crmpath)) {
      drush_log(dt('CiviCRM: civicrm is in @path', array('@path' => $crmpath)));
      return $crmpath;
    }
  }

  $module = _provision_civicrm_get_package_info();

  if ($module) {
    // Get the parent directory of the module, which is in civicrm/drupal/civicrm.module
    $crmpath = dirname(dirname($module['filename']));
    drush_log(dt('CiviCRM: civicrm is in @path', array('@path' => $crmpath)));

    // do#2364871 Workaround a BOA 2.3 issue
    // When civicrm is in an install profile, the civicrm drush module does not get loaded.
    if ($load_if_missing && ! function_exists('_civicrm_get_crmpath')) {
      drush_log(dt('CiviCRM: loading file: civicrm.drush.inc.'));
      require_once($crmpath . '/drupal/drush/civicrm.drush.inc');
    }

    return $crmpath;
  }

  return FALSE;
}

/**
 * Get the CiviCRM module base information (filename, basename, version, etc)
 */
function _provision_civicrm_get_package_info() {
  $module = NULL;

  // Check if the CiviCRM code base is present in the platform
  // Note: after putting the code there, you need to verify
  // the platform.
  $packages = drush_get_option('packages', array('modules' => NULL));
  $profile = drush_get_option('profile', NULL);

  if (isset($packages['modules']) && isset($packages['modules']['civicrm'])) {
    drush_log(dt('CiviCRM: found civicrm in packages'));
    $module = $packages['modules']['civicrm'];
  }
  elseif (isset($packages['sites-all']) && isset($packages['sites-all']['modules']) && isset($packages['sites-all']['modules']['civicrm'])) {
    $module = $packages['sites-all']['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of sites-all modules'));
  }
  elseif (isset($packages['base']) && isset($packages['base']['modules']) && isset($packages['base']['modules']['civicrm'])) {
    $module = $packages['base']['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of base modules'));
  }
  elseif ($profile && isset($packages['profiles'][$profile]['modules']) && isset($packages['profiles'][$profile]['modules']['civicrm'])) {
    $module = $packages['profiles'][$profile]['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of @profile profile modules', array('@profile' => $profile)));
  }
  else {
    // drush_log(dt('CiviCRM: civicrm not found in packages ' . print_r($packages, 1)));
  }

  return $module;
}

/**
 * CiviCRM verify operations (used by both verify and deploy)
 */
function _provision_civicrm_regenerate_settings($url = NULL) {
  drush_log(dt("CiviCRM: starting provision_civicrm_regenerate_settings()."));

  // We have to manually update the $civicrm_root in the civicrm.settings.php
  // file, because using cv assumes a functionnal CiviCRM, which will not work
  // if we are cloning from a platform to another (duplicate ClassLoader).
  $civicrm_root = _provision_civicrm_get_package_path();
  $filename = d()->site_path . '/civicrm.settings.php';
  $settings = file_get_contents($filename);

  // Relax permissions while we update the file
  provision_file()->chmod(d()->site_path, 0755)
    ->succeed('Changed permissions of <code>@path</code> to @perm')
    ->fail('Could not change permissions <code>@path</code> to @perm');
  provision_file()->chmod($filename, 0640)
    ->succeed('Changed permissions of <code>@path</code> to @perm')
    ->fail('Could not change permissions <code>@path</code> to @perm');

  if ($settings) {
    $replacements = [];
    $replacements[] = [
      'search_regex' => '/^.*?\$civicrm_root =.*\n?/m',
      'replace_line' => "\$civicrm_root = '$civicrm_root';\n",
    ];
    // This is not great because we assume [civicrm.private] points here
    // but .. it should?
    $replacements[] = [
      'search_regex' => "#'CIVICRM_TEMPLATE_COMPILEDIR', '/var/aegir/platforms/[^/]+/(web/)?sites/[^/]+/(private/)?files/civicrm/templates_c'#",
      'replace_line' => "'CIVICRM_TEMPLATE_COMPILEDIR', '" . d()->site_path . "/private/files/civicrm/templates_c'",
    ];

    foreach ($replacements as $desc => $line) {
      // Throw a warning if any of our replacements cannot be found.
      if (!preg_match($line['search_regex'], $settings)) {
        drush_log('drush_provision_civicrm_provision_deploy: Failed to replace: '. $line['search_regex'], 'warning');
      }
      else {
        $count = 0;
        $settings = preg_replace($line['search_regex'], $line['replace_line'], $settings, -1, $count);
        if ($count > 0) {
          drush_log('drush_provision_civicrm_provision_deploy: Succeeded to replace: ' . $line['search_regex'], 'success');
        }
      }
    }

    file_put_contents($filename, $settings);
  }
  else {
    drush_log(dt("drush_provision_civicrm_provision_deploy: Failed to read the civicrm.settings.php file (%file). The civicrm_root value will not be updated, and cloning from one platform to another will fail.", ['%file' => $filename]), 'error');
  }

  // Make sure to write the civicrm options
  $civi_opts = _provision_civicrm_drush_options('post verify');
  foreach ($civi_opts as $opt => $val) {
    drush_set_option($opt, $val, 'site');
  }

  // Since we do not have an upgrade path for existing site key (before it
  // was added to the frontend), be nice about an empty sitekey.
  if (d()->civicrm_sitekey) {
    drush_log('CiviCRM: found a site key from the front-end.', 'ok');
    drush_set_option('civicrm_sitekey', d()->civicrm_sitekey);
  }
  else {
    drush_log('CiviCRM: no site key found from the front-end.');
  }

  // @todo FIXME pass the civicrm_sitekey if set in the frontend
  provision_exec('cd ' . escapeshellarg(d()->site_path) . ' && php ' . dirname(__DIR__) . '/cv/regenerate-settings.php ' . escapeshellarg($url));

  // Set permissions back to strict
  provision_file()->chmod($filename, 0440)
    ->succeed('Changed permissions of <code>@path</code> to @perm')
    ->fail('Could not change permissions <code>@path</code> to @perm');
  provision_file()->chmod(d()->site_path, 0555)
    ->succeed('Changed permissions of <code>@path</code> to @perm')
    ->fail('Could not change permissions <code>@path</code> to @perm');

  drush_log(dt("CiviCRM: Generated config civicrm.settings.php file"), 'ok');
  drush_log(dt("CiviCRM: leaving provision_civicrm_regenerate_settings()."));
}

function provision_civicrm_verify_common() {
  drush_log(dt("CiviCRM: in provision_civicrm_verify_common()."));

  // Set file acls on civicrm.settings.php, if necessary
  if (function_exists('provisionacl_set_acl')) {
    $group = d()->client_name;
    $file = d()->site_path . '/' . 'civicrm.settings.php';
    drush_log(dt('CiviCRM: Attempting to set acls for @group on @file', array('@group' => $group, '@file' => $file)));

    // Check the group
    if (!provision_posix_groupname($group)) {
      drush_log(dt('CiviCRM: not setting acls on @file for non-existent group @group', array('@file' => $file, '@group' => $group)), 'warning');
    }
    // Check the file
    else if (! file_exists($file)) {
      drush_log(dt('@file not found, acls not configured', array('@file' => $file)), 'warning');
    }
    else {
      provisionacl_set_acl('group', $group, 'r--', array($file), TRUE);
    }
  }

  drush_log(dt("CiviCRM: leaving provision_civicrm_verify_common()."));
  return TRUE;
}

/**
 * Check whether to set the upgrade mode
 */
function _provision_civicrm_check_upgrade_mode() {
  drush_log(dt("CiviCRM: in _provision_civicrm_check_upgrade_mode()"));
  // Sets the civicrm upgrade mode of the code/db version do not match
  $codeVer = _provision_civicrm_codebase_version();
  $dbVer = _provision_civicrm_database_version();

  if (version_compare($codeVer, $dbVer) != 0) {
    if (! defined('CIVICRM_UPGRADE_ACTIVE')) {
      define('CIVICRM_UPGRADE_ACTIVE', 1);
      drush_log(dt('CiviCRM: upgrade mode enabled.'));
      return TRUE;
    }
  }

  drush_log(dt("CiviCRM: leaving _provision_civicrm_check_upgrade_mode()"));
  return FALSE;
}

/**
 * Helper function to get the CiviCRM version of the code base.
 *
 * Avoiding any CMS or Aegir methods, because:
 * - using pm_parse_version() to parse the version, because it does not
 *   like versions such as '8.x-5.0.beta1'.
 * - Drupal7 has $packages['modules']['civicrm']['info']['version'],
 *   but Drupal8 only has $packages['modules']['civicrm']['version'] (which D7 also has).
 * - Sometimes the platform info is outdated unless we remember to run verify.
 * - CiviCRM has a somewhat stable interface to fetch this info.
 */
function _provision_civicrm_codebase_version() {
  static $codeVer = 0;

  if ($codeVer) {
    return $codeVer;
  }

  $crmPath = _provision_civicrm_get_package_path();

  if ($crmPath) {
    $xmlPath = $crmPath . '/xml/version.xml';
    drush_log(dt('CiviCRM: version xmlPath !path', ['!path' => $xmlPath]), 'ok');
    $codeVer = system('grep version_no ' . escapeshellarg($xmlPath) . ' | sed "s/.*>\(.*\)<.*/\1/"');
  }

  drush_log(dt('CiviCRM: code version is !ver', ['!ver' => $codeVer]), 'ok');
  return $codeVer;
}

/**
 * Helper function to get the CiviCRM version of the DB
 */
function _provision_civicrm_database_version() {
  static $dbVer = 0;

  if ($dbVer) {
    return $dbVer;
  }

  $command = 'cd ' . escapeshellarg(d()->site_path) . ' && echo "select version from civicrm_domain limit 1" | cv sql | tail -1';
  drush_log(dt("CiviCRM: running: %command", ['%command' => $command]), 'ok');
  $dbVer = system($command, $retval);

  if ($retval) {
    drush_log(dt("CiviCRM: command failed {$retval} (the ConfigAndLog should have more details): %output", ['%output' => $dbVer]), 'warning');
    return NULL;
  }

  drush_log(dt('CiviCRM: db version is !ver', ['!ver' => $dbVer]), 'ok');
  return $dbVer;
}

/**
 * Run a command by invoking a shell instead of running it in the
 * same process as Aegir, because it makes error handling more difficult.
 *
 * You must call escapeshellarg() before calling this function, if there are any args in the $command.
 *
 * Errors are logged as warnings to avoid an Aegir rollback (because those errors
 * should not happen, and if they do, we want to be able to debug).
 */
function _provision_civicrm_run_cv_command($command) {
  // Always non-interactive (cv -n)
  $command = 'cd ' . escapeshellarg(d()->site_path) . ' && cv -n ' . $command;
  drush_log(dt("CiviCRM: running: %command", ['%command' => $command]), 'ok');
  $json = system($command, $retval);
  // Output is always json
  $output = json_decode($json, TRUE);
  if ($retval || (is_array($output) && !empty($output['is_error']))) {
    drush_log(dt("CiviCRM: command failed {$retval} (the ConfigAndLog should have more details): %output", ['%output' => $json]), 'warning');
  }
  else {
    drush_log(dt("CiviCRM: command success: %output", ['%output' => $json]), 'ok');
  }

  return $output;
}
