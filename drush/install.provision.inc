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

  // Clear Config.IDS.ini
  _provision_civicrm_clear_config_cache();

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
  if (drush_drupal_major_version() >= 8) {
    // SystemInstallEvent will be called from here with the first call of CRM_Core_Config,
    // which calls Core_BAO_ConfigSetting::applyLocale(), who will default to calling
    // Civi::settings()->get('lcMessages');
    // Therefore, we need to pass the seedLanguage before that.
    if ($lang) {
      global $civicrm_setting;
      $civicrm_setting['domain']['lcMessages'] = $lang;
    }

    drush_log(dt("CiviCRM: Enabling the CiviCRM module."), 'ok');
    \Drupal::service('module_installer')->install(['civicrm']);
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
  $modPath = $crmPath . '/../';

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
  module_enable(array('civicrm'));

  // Save the site key, in case we verify later on, it will regenerate
  // the civicrm.settings.php
  // c.f. duplication from provision_civicrm_regenerate_settings()
  drush_log(dt("CiviCRM: Saving sitekey and other settings."));

  $civi_opts = _provision_civicrm_drush_options('post verify');
  $civi_opts['civicrm_sitekey'] = CIVICRM_SITE_KEY;

  foreach ($civi_opts as $opt => $val) {
    drush_set_option($opt, $val, 'site');
  }
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
