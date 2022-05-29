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
 * Implements hook_civicrm_init()
 */
function provision_civicrm_civicrm_init() {
  return provision_civicrm_setup_temp_directories();
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
 * Setup logs and template_c in a sub-directory to avoid permission
 * conflicts between the www-user and the aegir user.
 */
function provision_civicrm_setup_temp_directories() {
  require_once 'CRM/Core/Config.php';
  $config = CRM_Core_Config::singleton();

  // ConfigAndLog directory
  $exists = provision_file()->exists($config->configAndLogDir)
    ->succeed('CiviCRM: @path exists.')
    ->status();

  if (! $exists) {
    $exists = provision_file()->mkdir($config->configAndLogDir)
      ->succeed('CiviCRM: log path @path has been created.')
      ->fail('CiviCRM: log path @path could not be created.', 'DRUSH_PERM_ERROR')
      ->status();
  }

  // Smarty template directory
  $exists = provision_file()->exists($config->templateCompileDir)
    ->succeed('CiviCRM: @path exists.')
    ->status();

  if (! $exists) {
    $exists = provision_file()->mkdir($config->templateCompileDir)
      ->succeed('CiviCRM: compiled template path @path has been created.')
      ->fail('CiviCRM: compiled template path @path could not be created.', 'DRUSH_PERM_ERROR')
      ->status();
  }

  // Smarty template directory for the default locale.
  $lcMessages = _provision_civicrm_get_default_locale();

  $exists = provision_file()->exists($config->templateCompileDir . '/' . $lcMessages)
    ->succeed('CiviCRM: @path exists.')
    ->status();

  if (! $exists) {
    $exists = provision_file()->mkdir($config->templateCompileDir . '/' . $lcMessages)
      ->succeed('CiviCRM: compiled template path @path has been created.')
      ->fail('CiviCRM: compiled template path @path could not be created.', 'DRUSH_PERM_ERROR')
      ->status();
  }

  return TRUE;
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
 * Clear Config.IDS.ini files from various possible locations.
 */
function _provision_civicrm_clear_config_cache() {
  drush_log(dt("CiviCRM: Clearing config cache."));
  // Remove the Config.IDS.ini file (d.o#1226668)
  // Usually 'verify' will call civicrm_cache_clear() which will
  // delete most of the contents of template_c, but there have been
  // some hard to trace bugs, so we are not taking any chances.
  //
  // Also, civicrm_cache_clear() assumes that _civi_init() has been
  // done, and if we init, then the Config.IDS.ini will be parsed.
  $configFiles = [
    '/files/civicrm/ConfigAndLog/Config.IDS.ini',
    '/files/civicrm/drush/ConfigAndLog/Config.IDS.ini',
    '/files/civicrm/templates_c/en_US/ConfigAndLog/Config.IDS.ini',
  ];

  foreach ($configFiles as $c) {
    $ids = d()->site_path . $c;

    if (provision_file()->exists($ids)->status()) {
      provision_file()->unlink($ids)
        ->succeed('CiviCRM: Removed @path')
        ->fail('CiviCRM: Could not remove @path')
        ->status();
    }
  }
}

/**
 * Update the CiviCRM Resource URLs
 * There must be a much simpler/cleaner way of doing this..
 */
function _provision_civicrm_fixresourceurls() {
  drush_log(dt("CiviCRM: Running civicrm_config_update() to fix URL/path settings."));

  if (function_exists('drush_civicrm_update_cfg')) {
    drush_civicrm_update_cfg_validate();
    drush_civicrm_update_cfg();
  }
  elseif (function_exists('civicrm_config_update')) {
    civicrm_config_update();
  }
  else {
    drush_log(dt("CiviCRM: Could not find a drush function to update the CiviCRM url/path settings."), 'error');
    return;
  }

  drush_log(dt("CiviCRM: Finished civicrm_config_update()."));
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
  if (drush_drupal_major_version() >= 8) {
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
function _provision_civicrm_regenerate_settings() {
  drush_log(dt("CiviCRM: starting provision_civicrm_regenerate_settings()."));

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

  $db_user = drush_get_option('db_user', NULL, 'site');
  $db_passwd = drush_get_option('db_passwd', NULL, 'site');
  $db_host = drush_get_option('db_host', NULL, 'site');
  $db_name = drush_get_option('db_name', NULL, 'site');

  // include civicrm installer helper file (otherwise PEAR DB.php will fail to include parts)
  // FIXME : we should test whether this is still necessary.
  $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
  $crmPath = _provision_civicrm_get_package_path();

  $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";

  if (! file_exists($civicrmInstallerHelper) ) {
    drush_log(dt("CiviCRM installer helper file is missing. Not running provision_civicrm_regenerate_settings()."), 'error');
    return;
  }

  require_once "$civicrmInstallerHelper";

  provision_civicrm_regenerate_settings_file($db_user, $db_passwd, $db_host, $db_name, $crmPath);
  drush_log(dt("CiviCRM: Generated config civicrm.settings.php file"), 'ok');
  drush_log(dt("CiviCRM: leaving provision_civicrm_regenerate_settings()."));
}

/**
 * Regenerates the civicrm.settings.php file.
 * Forked from civicrm.drush.inc's _civicrm_generate_settings_file().
 *
 * Modified mostly so that we can make the sitekey persist,
 * and replaced th $modPath argument by $crmPath.
 */
function provision_civicrm_regenerate_settings_file($dbuser, $dbpass, $dbhost, $dbname, $crmPath) {
  $sitekey = d()->civicrm_sitekey;
  $drupalRoot = d()->root;
  $siteRoot   = d()->site_path;
  $crmPath    = _provision_civicrm_get_package_path();

  $files = array(
    "$crmPath/templates/CRM/common/civicrm.settings.php.template",
    "$crmPath/templates/CRM/common/civicrm.settings.php.tpl"
  );

  // Allow overriding the template file search paths.
  drush_command_invoke_all_ref('civicrm_settings_template_files_alter', $files);

  $settingsTplFile = NULL;
  foreach ($files as $file) {
    if (file_exists($file)) {
      $settingsTplFile = $file;
      break;
    }
  }

  if (!$settingsTplFile) {
    drush_die(dt("Could not find CiviCRM settings template and therefore could not create settings file."));
  }

  drush_log(dt("CiviCRM: Generating the settings file using !file ..", array('!file' => $settingsTplFile)), 'ok');

  $protocol = (d()->ssl_enabled || d()->https_enabled) ? 'https' : 'http';
  $baseUrl = $protocol . '://' . d()->uri;
  $db_spec = drush_civicrm_get_db_spec();

  // Check version: since 4.1, Drupal6 must be used for the UF in D6
  // The file civicrm-version.php appeared around 4.0, so it is safe to assume
  // that if it's not there, it's 3.x, in which case the CMS 'Drupal' is D6.
  $cms = 'Drupal';

  if (file_exists("$crmPath/civicrm-version.php")) {
    require_once "$crmPath/civicrm-version.php";
    $v = civicrmVersion();
    $cms = $v['cms'];
  }

  $params = array(
    'crmRoot' => $crmPath,
    'templateCompileDir' => "$siteRoot/files/civicrm/templates_c",
    'frontEnd' => 0,
    'cms' => $cms,
    'baseURL' => $baseUrl,
    'dbUser' => $dbuser,
    'dbPass' => $dbpass,
    'dbHost' => $dbhost,
    'dbName' => $dbname,
    'CMSdbUser' => $db_spec['username'],
    'CMSdbPass' => $db_spec['password'],
    'CMSdbHost' => $db_spec['host'],
    'CMSdbName' => $db_spec['database'],
    'siteKey' => $sitekey,
  );
  $str = file_get_contents($settingsTplFile);
  // Allow changes to the settings file template.
  drush_command_invoke_all_ref('civicrm_settings_template_alter', $str);
  // Allow addition to and overrides of the settings file parameters.
  drush_command_invoke_all_ref('civicrm_settings_parameters_alter', $params);
  foreach ($params as $key => $value) {
    $str = str_replace('%%' . $key . '%%', $value, $str);
  }
  $str = trim($str);

  $configFile = "$siteRoot/civicrm.settings.php";
  civicrm_write_file($configFile, $str);
  @drush_op('chmod', "$configFile", 0644);
  drush_log(dt("Settings file generated: !file", array('!file' => $configFile)), 'ok');
}

/**
 * Implements hook_civicrm_settings_template_alter().
 *
 * @see provision_civicrm_civicrm_settings_parameters_alter().
 */
function provision_civicrm_civicrm_settings_template_alter(&$template) {
  $replacements = array();
  $replacements['allow uploadDir to be overridden.'] = array(
    'search_regex' => '/^.*?civicrm_setting.*uploadDir.*\n?/m',
    'replace_line' => "\$civicrm_setting['Directory Preferences']['uploadDir'] = '%%uploadDir%%';\n",
  );

  foreach ($replacements as $desc => $line) {
    // Throw a warning if any of our replacements cannot be found.
    if (!preg_match($line['search_regex'], $template)) {
      drush_log(dt('CiviCRM: Failed to ' . $desc), 'warning');
    }
    else {
      $count = 0;
      $template = preg_replace($line['search_regex'], $line['replace_line'], $template, -1, $count);
      if ($count > 0) {
        drush_log(dt('CiviCRM: Succeeded to ' . $desc), 'success');
      }
    }
  }
}

/**
 * Implements hook_civicrm_settings_parameters_alter().
 *
 * @see provision_civicrm_civicrm_settings_template_alter().
 */
function provision_civicrm_civicrm_settings_parameters_alter(&$params) {
  $params['uploadDir'] = d()->site_path . "/files/civicrm/upload";
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

  drush_set_option('oldVal_1', drush_get_option('civicrm_old_path'));
  drush_set_option('newVal_1', drush_get_context('DRUSH_DRUPAL_ROOT'));

  drush_set_option('oldVal_2', drush_get_option('civicrm_old_baseurl'));
  drush_set_option('newVal_2', 'http://' . drush_get_option('uri'));

  // Update database settings for the directory paths
  drush_civicrm_update_cfg();

  // Update Resource URLs
  _provision_civicrm_fixresourceurls();

  // Clear Config.IDS.ini
  _provision_civicrm_clear_config_cache();

  drush_log(dt("CiviCRM: leaving provision_civicrm_verify_common()."));
  return TRUE;
}

/**
 * CiviCRM database upgrade
 *
 * Code forked from the CiviCRM drush module
 * Adapted to check the upgrade result and support multiple CiviCRM versions.
 *
 * @todo We should exec a process to run the upgrade, so that a failed upgrade
 * does not make Aegir crash and/or rollback.
 */
function _provision_civicrm_upgradedb() {
  drush_log(dt("CiviCRM: in _provision_civicrm_upgradedb(), calling _civicrm_init()"));

  // Check for the CiviCRM installer helper, to ignore the upgrade
  // on really old versions of civicrm (verify also does this).
  $crmPath = _provision_civicrm_get_package_path();

  $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";

  if (! file_exists($civicrmInstallerHelper) ) {
    drush_log(dt("CiviCRM installer helper file is missing. Old CiviCRM version?"));
    return;
  }

  _civicrm_init();

  drush_log(dt("CiviCRM: in _provision_civicrm_upgradedb(), _civicrm_init() worked, comparing code and schema versions."));

  // Still need to check if the DB and code versions are the same
  // so that we can avoid calling the upgrade on 'verify'.
  $codeVer = _provision_civicrm_codebase_version();
  $dbVer   = _provision_civicrm_database_version();

  if (version_compare($codeVer, $dbVer) == 0) {
    drush_log(dt("CiviCRM: no upgrade required (@codeVer = @dbVer)", array('@codeVer' => $codeVer, '@dbVer' => $dbVer)), 'ok');
    return;
  }

  if (! defined('CIVICRM_UPGRADE_ACTIVE')) {
    drush_log(dt("CiviCRM: enabling CIVICRM_UPGRADE_ACTIVE, this will disable all extensions.", 'ok'));
    define('CIVICRM_UPGRADE_ACTIVE', 1);
  }

  // This was added around CiviCRM >= 4.2. Use it if possible.
  // Does a few version checks before the upgrade.
  // NB: This function will define CIVICRM_UPGRADE_ACTIVE if not already set.
  // FIXME: should we use drush_invoke or equivalent?
  if (function_exists('drush_civicrm_upgrade_db_validate')) {
    if (! drush_civicrm_upgrade_db_validate()) {
      return FALSE; // above already does a drush_set_error
    }
  }

  drush_log(dt('CiviCRM: Beginning upgrade from @dbVer to @codeVer', array('@codeVer' => $codeVer, '@dbVer' => $dbVer)), 'info');

  ob_start();
  if (function_exists('drush_civicrm_upgrade_db')) {
    drush_civicrm_upgrade_db();
  }
  elseif (function_exists('civicrm_upgrade_db')) {
    civicrm_upgrade_db(); // CiviCRM < 4.2 ?
  }
  else {
    ob_end_clean();
    drush_log(dt("CiviCRM: Could not find a drush function to upgrade CiviCRM."), 'error');
    return;
  }
  ob_end_clean();

  $output = ob_get_contents();

  drush_log(dt("CiviCRM: upgrade output: !output", array('!output' => $output)), 'info');
  drush_log(dt("CiviCRM: finished _provision_civicrm_upgradedb()"));
}

/**
 * Check whether to set the upgrade mode
 */
function _provision_civicrm_check_upgrade_mode() {
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
  $vendorPath = $crmPath . '/vendor/';

  if (drush_drupal_major_version() >= 8) {
    $root = d()->root;
    // If Drupal lives under a 'web' subdirectory (which is usually the case with composer),
    // we remove the prefix, since the vendor directory is outside 'web'.
    // Duplicates code from _provision_civicrm_get_package_path().
    $root = preg_replace('#web/?$#', '', $root);
    $vendorPath = $root . '/vendor/';
  }

  if ($crmPath) {
    require_once $vendorPath . '/autoload.php';
    require_once $crmPath . '/CRM/Utils/System.php';
    require_once $crmPath . '/CRM/Core/Exception.php';
    $codeVer = CRM_Utils_System::version();
  }

  drush_log(dt('CiviCRM: code version is !ver', array('!ver' => $codeVer)));
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

  $civicrmSettingsFile = d()->site_path . "/civicrm.settings.php";
  include_once($civicrmSettingsFile);

  switch (drush_drupal_major_version()) {
    case 6:
      $dbVer = db_result(db_query('select version from civicrm_domain where id = %d', CIVICRM_DOMAIN_ID));
      break;

    case 7:
    case 8:
      $dbVer = db_query('select version from civicrm_domain where id = :id', array(':id' => CIVICRM_DOMAIN_ID))->fetchField();
      break;
    case 9:
      $dbVer = \Drupal::database()->query('select version from civicrm_domain where id = :id', array(':id' => CIVICRM_DOMAIN_ID))->fetchField();
      break;
    default:
      drush_log(dt('_provision_civicrm_database_version: unsupported Drupal version: !version', array('!version' => drush_drupal_major_version())), 'warning');
  }

  drush_log(dt('CiviCRM: db version is !ver', array('!ver' => $dbVer)));
  return $dbVer;
}
