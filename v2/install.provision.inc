<?php

/**
 * implementation of hook_pre_provision_install
 */
function drush_civicrm_pre_provision_install($url = NULL) {
  /*
  if (d()->type == 'site') {
    // if necessary ...
  }
  */
}

/**
 * implementation of hook_post_provision_install
 */
function drush_civicrm_post_provision_install($url = null) {
  if (d()->type == 'site') {
    // Check if the CiviCRM code base is present in the platform
    // Note: after putting the code there, you must verify the platform.
    if (! _provision_civicrm_get_package_path()) {
      // drush_log(dt('CiviCRM: not found in package path, skipping installation.'));
      return;
    }

    drush_log(dt("CiviCRM: Starting installation..."));

    $db_user = drush_get_option('db_user', NULL, 'site');
    $db_passwd = drush_get_option('db_passwd', NULL, 'site');
    $db_host = drush_get_option('db_host', NULL, 'site');
    $db_name = drush_get_option('db_name', NULL, 'site');

    // Inherit the language from Drupal
    // see provision_civicrm_language_uf_to_civicrm() for details.
    $lang = '';

    $uf_language = d()->language;
    drush_log(dt("CiviCRM: Drupal language: %lang", array('%lang' => $uf_language)));

    if ($uf_language && $uf_language != 'en') {
      $lang = provision_civicrm_language_uf_to_civicrm($uf_language);
      drush_log(dt("CiviCRM: CiviCRM language: %lang", array('%lang' => $lang)));
    }

    provision_civicrm_install_civicrm($db_user, $db_passwd, $db_host, $db_name, $lang);

    // Deprecated by hosting_civicrm_cron
    // _provision_civicrm_create_civicron_user();

    // Fix some paths (not really necessary since 4.x, works fine after install).
    // _provision_civicrm_fixresourceurls();

    drush_log(dt("CiviCRM: Installation complete!"), 'ok');
  }
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
function provision_civicrm_install_civicrm($dbuser, $dbpass, $dbhost, $dbname, $lang) {
  $crmpath    = _civicrm_get_crmpath();
  $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
  $modPath    = "$drupalRoot/$crmpath";

  $civiPath = _provision_civicrm_get_package_path();
  $sitePath = drush_get_context('DRUSH_DRUPAL_ROOT');

  // include civicrm installer helper file
  $civicrmInstallerHelper = $civiPath . "/install/civicrm.php";

  if (!file_exists($civicrmInstallerHelper)) {
    return drush_set_error('CIVICRM_NOT_PRESENT', dt("CiviCRM installer helper file is missing."));
  }

  // setup all required files/civicrm/* directories
  if (!_civicrm_create_files_dirs($civicrmInstallerHelper, $sitePath)) {
    return FALSE;
  }

  // install database
  // FIXME: the use of $crmPath is very weird. It's necessary for civicrm_source(),
  // which is called from _civicrm_install_db(). Needs refactoring!
  global $crmPath;
  $crmPath = $civiPath;

  // NB: rather silly, but this function expects the parent dir
  // of where the civicrm code base is located.
  drush_log(dt("CiviCRM: Initializing the CiviCRM database."));
  _civicrm_install_db($dbuser, $dbpass, $dbhost, $dbname, $civiPath . '/../', $lang);

  // generate civicrm.settings.php file
  drush_log(dt("CiviCRM: Generating the CiviCRM settings file."));
  _civicrm_generate_settings_file($dbuser, $dbpass, $dbhost, $dbname, $civiPath . '/../');

  drush_log(dt("CiviCRM: Enabling the CiviCRM module."));
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
function provision_civicrm_language_uf_to_civicrm($uf_language) {
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
    'ja' => 'ja_JP', // Japanese/Japan
    'sv' => 'sv_SE', // Swedish
    'sl' => 'sl_SI', // Slovenian
    'sq' => 'sq_AL', // Albenian
    'vi' => 'vi_VN', // Vietnam
    'zh' => 'zh_CN', // Mandarin/China
  );

  if (isset($exceptions[$uf_language])) {
    return $exceptions[$uf_language];
  }

  return $uf_language . '_' . strtoupper($uf_language);
}