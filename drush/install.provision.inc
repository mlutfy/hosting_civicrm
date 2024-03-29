<?php

/**
 * implementation of hook_post_provision_install
 */
function drush_provision_civicrm_post_provision_install($url = null) {
  if (! _provision_civicrm_is_site_context(TRUE)) {
    return;
  }

  drush_log(dt("CiviCRM: Starting installation..."));

  $db_user = drush_get_option('db_user', NULL, 'site');
  $db_passwd = drush_get_option('db_passwd', NULL, 'site');
  $db_name = drush_get_option('db_name', NULL, 'site');

  // Always setting the port helps support non-standard aegir configs,
  // but also workaround CRM-19407.
  //
  // FIXME: this is not if the hostname is using an IPv6 address
  // but 1- use a hostname instead, and 2- needs fixing in civicrm.drush.inc.
  $db_host = drush_get_option('db_host', NULL, 'site') . ':' . drush_get_option('db_port', 3306, 'site');

  // Inherit the language from Drupal
  // see _provision_civicrm_language_uf_to_civicrm() for details.
  $lang = '';

  $uf_language = d()->language;
  drush_log(dt("CiviCRM: Drupal language: %lang", array('%lang' => $uf_language)));

  if ($uf_language && $uf_language != 'en') {
    $lang = _provision_civicrm_language_uf_to_civicrm($uf_language);
    drush_log(dt("CiviCRM: CiviCRM language: %lang", array('%lang' => $lang)));
  }

  _provision_civicrm_install_civicrm($db_user, $db_passwd, $db_host, $db_name, $lang);

  // Fix some paths (not really necessary since 4.x, works fine after install).
  // _provision_civicrm_fixresourceurls();

  drush_log(dt("CiviCRM: Installation complete!"), 'ok');
}

/**
 * Main function to initialize the CiviCRM database and settings file.
 * This has been tested on CiviCRM 4.4.
 *
 * This is basically a simplified version of drush_civicrm_install()
 * from civicrm.drush.inc shipped with CiviCRM.
 *
 * We had to fork it since it assumes that the civicrmInstallerHelper
 * is located in sites/example.org/modules/civicrm/.
 *
 * Also removed the references to the tar file, since we assume that
 * the platform is already created with drush make.
 */
function _provision_civicrm_install_civicrm($dbuser, $dbpass, $dbhost, $dbname, $lang) {
  // As of D8, the database schema and civicrm.settings.php
  // are generated when the module is first installed.
  $drupal_core_version = drush_drupal_major_version();

  // Transition code, provision might not have this function
  if (function_exists('provision_get_drupal_core_version')) {
    $v = provision_get_drupal_core_version();
    if ($v) {
      $drupal_core_version = $v;
    }
  }

  drush_log(dt('CiviCRM: Detected Drupal core version: %version', ['%version' => $drupal_core_version]), 'ok');

  if (version_compare($drupal_core_version, 10, '>=')) {
    // Install CiviCRM using drush
    $command = 'drush pm:install civicrm -y';
    drush_log(dt("CiviCRM: running: %command", ['%command' => $command]), 'ok');
    provision_file()->chmod(d()->site_path, 0755);
    $output = system($command, $retval);
    drush_log(dt("CiviCRM: retval=%retval, output: %output", ['%retval' => $retval, '%output' => $output]), $retval == 0 ? 'ok' : 'error');
    provision_file()->chmod(d()->site_path, 0555);
    // Fix file permissions so that www-data can write
    drush_log(dt("CiviCRM: running fix-drupal-site-ownership.sh and permissions..."), 'ok');
    drush_shell_exec("sudo --non-interactive /usr/local/bin/fix-drupal-site-ownership.sh --site-path=%s --script-user=%s --web-group=%s", d()->site_path, d()->server->script_user, d()->server->web_group);
    $output = implode('', drush_shell_exec_output());
    drush_log(dt("CiviCRM: fix-drupal-site-ownership.sh output: %output", ['%output' => $output]), 'ok');
    drush_shell_exec("sudo --non-interactive /usr/local/bin/fix-drupal-site-permissions.sh --site-path=%s", d()->site_path);
    $output = implode('', drush_shell_exec_output());
    drush_log(dt("CiviCRM: fix-drupal-site-permissions.sh output: %output", ['%output' => $output]), 'ok');
    drush_log(dt("CiviCRM: finished running fix-drupal-site-ownership.sh and permissions"), 'ok');
    return;
  }
  elseif (drush_drupal_major_version() >= 8) {
    // SystemInstallEvent will be called from here with the first call of CRM_Core_Config,
    // which calls Core_BAO_ConfigSetting::applyLocale(), who will default to calling
    // Civi::settings()->get('lcMessages');
    // Therefore, we need to pass the seedLanguage before that.
    if ($lang) {
      global $civicrm_setting;
      $civicrm_setting['domain']['lcMessages'] = $lang;
    }

    drush_log(dt("CiviCRM: Enabling the CiviCRM module."), 'ok');
    try {
      \Drupal::service('module_installer')->install(['civicrm']);
    }
    catch (Exception $e) {
      // Do whatever is necessary to help debug installation problems
      drush_log(dt("CiviCRM: failed to enable CiviCRM: !error - !trace", ['!error' => $e->getMessage(), '!trace' => print_r($e->getTrace(), 1)]), 'error');
    }
    return;
  }

  // Various civicrm.drush.inc functions expect the $crmPath to be set.
  global $crmPath;
  $crmPath = _provision_civicrm_get_package_path();
  $loadGeneratedData = drush_get_option('load_generated_data', FALSE);

  drush_log('DRUSH_DRUPAL_ROOT: ' . drush_get_context('DRUSH_DRUPAL_ROOT'), 'ok');
  drush_log('DRUSH_SITE_ROOT: ' . drush_get_context('DRUSH_DRUPAL_SITE_ROOT', FALSE), 'ok');

  // Include civicrm installer helper file
  $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";

  if (!file_exists($civicrmInstallerHelper)) {
    return drush_set_error('CIVICRM_NOT_PRESENT', dt("CiviCRM installer helper file is missing."));
  }

  // NB: civicrm drush functions expect the parent dir of the civicrm code base.
  $modPath = dirname($crmPath) . '/';

  // Setup all required files/civicrm/* directories
  if (!_civicrm_create_files_dirs($civicrmInstallerHelper, $modPath)) {
    return FALSE;
  }

  // Install the database
  drush_log(dt("CiviCRM: Initializing the CiviCRM database."));
  _civicrm_install_db($dbuser, $dbpass, $dbhost, $dbname, $modPath, $lang, $loadGeneratedData);

  // Generate civicrm.settings.php file
  drush_log(dt("CiviCRM: Generating the CiviCRM settings file."));
  _civicrm_generate_settings_file($dbuser, $dbpass, $dbhost, $dbname, $modPath);

  // SystemInstallEvent will be called from here with the first call of CRM_Core_Config,
  // which calls Core_BAO_ConfigSetting::applyLocale(), who will default to calling
  // Civi::settings()->get('lcMessages');
  // Therefore, we need to pass the seedLanguage before that.
  if ($lang) {
    global $civicrm_setting;
    $civicrm_setting['domain']['lcMessages'] = $lang;
  }

  drush_log(dt("CiviCRM: Enabling the CiviCRM module."), 'ok');
  _provision_civicrm_run_drush_command('en civicrm -y');
}

/**
 * Returns the CiviCRM language (ex: fr_FR) for a Drupal language (ex: fr).
 *
 * Handles a few exceptions (zh_CN, af_ZA, etc) and can be overridden with a
 * constant in your aegir global.inc of the form:
 * PROVISION_CIVICRM_LANGUAGE_MAPPING_XX, where XX is the short language code
 * from Drupal.
 */
function _provision_civicrm_language_uf_to_civicrm($uf_language) {
  if (defined('PROVISION_CIVICRM_LANGUAGE_MAPPING_' . $uf_language)) {
    return constant('PROVISION_CIVICRM_LANGUAGE_MAPPING_' . $uf_language);
  }

  // Taken from l10n/bin/copy-to-svn.sh
  $exceptions = array(
    'ar' => 'ar_EG', // Arabic/Egypt
    'af' => 'af_ZA', // Afrikaans/South Africa
    'be' => 'be_BY', // Belarus
    'ca' => 'ca_ES', // Catalan/Spain
    'cs' => 'cs_CZ', // Czech
    'da' => 'da_DK', // Danish
    'el' => 'el_GR', // Greek
    'et' => 'et_EE', // Estonian
    'he' => 'he_IL', // Hebrew/Israel
    'hi' => 'hi_IN', // Hindi
    'hy' => 'hy_AM', // Armenian
    'ja' => 'ja_JP', // Japanese/Japan
    'sv' => 'sv_SE', // Swedish
    'sl' => 'sl_SI', // Slovenian
    'sq' => 'sq_AL', // Albenian
    'sr' => 'sr_RS', // Serbian
    'vi' => 'vi_VN', // Vietnam
    'zh' => 'zh_CN', // Mandarin/China
  );

  if (isset($exceptions[$uf_language])) {
    return $exceptions[$uf_language];
  }

  return $uf_language . '_' . strtoupper($uf_language);
}
