<?php

/**
 * Override civicrm.settings.php template file search paths.
 *
 * @see provision_civicrm_regenerate_settings_file().
 */
function hook_civicrm_settings_template_files_alter(&$files){}

/**
 * Modify the civicrm.settings.php file template.
 *
 * @see provision_civicrm_regenerate_settings_file().
 */
function hook_civicrm_settings_template_alter(&$template) {}

/**
 * Add to or override the parameters passed to the civicrm.settings.php
 * template.
 *
 * @see provision_civicrm_regenerate_settings_file().
 */
function hook_civicrm_settings_parameters_alter(&$params) {}
