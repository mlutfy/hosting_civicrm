<?php
/**
 * @file
 *  Some tests for provison_civicrm (based on provision's tests).
 */

define('PROVISION_CIVICRM_TESTS_BUILDS_REPO', dirname(__FILE__) . '/makes');

/**
 * Implementation of hook_drush_command().
 */
function provision_civicrm_tests_drush_command() {
  $items['provision-civicrm-tests-run'] = array(
    'description' => dt('Runs provision_civicrm tests'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    // Although we're a provision command, we require hostmaster to be around to
    // run the tests correctly
    'drupal dependencies' => array(
      'hosting',
    ),
  );

  return $items;
}

/**
 * Drush command to run the provision tests.
 */
function drush_provision_civicrm_tests_run() {
  $error_detected = FALSE;

  // Disable the tasks queue, we run them manually instead.
  $queue_status_initial = variable_get('hosting_queue_tasks_enabled', '0');
  variable_set('hosting_queue_tasks_enabled', '0');

  try {
    drush_log(dt('*****'), 'ok');
    drush_log(dt('***** CiviCRM TEST: creating platforms.'), 'ok');
    drush_log(dt('*****'), 'ok');

    provision_civicrm_tests_install_platform('civicrm44d7');
    provision_civicrm_tests_install_platform('civicrm46d7');
    provision_civicrm_tests_install_platform('civicrm47d7');

    provision_civicrm_tests_001_install_and_delete();
    provision_civicrm_tests_002_basic_migrations();
    # provision_civicrm_tests_003_major_upgrades();

    // TODO: test the multi-lingual upgrade
  }
  catch (Exception $e) {
    $error_detected = TRUE;
    drush_log(dt('*****'), 'warning');
    drush_log(dt('***** Caught exception, stopping tests.'), 'warning');
    drush_log(dt('*****'), 'warning');
  }

  // Clean up a little.
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: Removing platforms.'), 'ok');
  drush_log(dt('*****'), 'ok');

  provision_civicrm_tests_remove_platform('civicrm44d7');
  provision_civicrm_tests_remove_platform('civicrm46d7');
  provision_civicrm_tests_remove_platform('civicrm47d7');

  // Restore the tasks queue status:
  variable_set('hosting_queue_tasks_enabled', $queue_status_initial);

  if ($error_detected || drush_get_error() != DRUSH_SUCCESS) {
    drush_set_error(drush_get_error(), 'Running tests failed');
    exit(1);
  }

  drush_log(dt('Tests completed successfully'), 'success');
}

/**
 * Test the installation and deletion of sites in various versions.
 */
function provision_civicrm_tests_001_install_and_delete() {
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: 001 Basic installation and removal tests.'), 'ok');
  drush_log(dt('*****'), 'ok');

  // Install some sites.
  provision_civicrm_tests_install_site('civicrm44d7', 'civicrm44d7-standard');
  provision_civicrm_tests_remove_site('civicrm44d7-standard');

  provision_civicrm_tests_install_site('civicrm46d7', 'civicrm46d7-standard');
  provision_civicrm_tests_remove_site('civicrm46d7-standard');

  provision_civicrm_tests_install_site('civicrm47d7', 'civicrm47d7-standard');
  provision_civicrm_tests_remove_site('civicrm47d7-standard');
}

function provision_civicrm_tests_002_basic_migrations() {
  // Test migrate from 4.4-D7 to 4.4-D7
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.4-D7 to 4.4-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  provision_civicrm_tests_install_platform('civicrm44d7', 'civicrm44d7_other');
  provision_civicrm_tests_install_site('civicrm44d7', 'civicrm44d7-test-migrate');
  provision_civicrm_tests_migrate_site('civicrm44d7-test-migrate', 'civicrm44d7_other');
  provision_civicrm_tests_remove_site('civicrm44d7-test-migrate');
  provision_civicrm_tests_remove_platform('civicrm44d7_other');

  // Test migrate from 4.6-D7 to 4.6-D7
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.6-D7 to 4.6-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  provision_civicrm_tests_install_platform('civicrm46d7', 'civicrm46d7_other');
  provision_civicrm_tests_install_site('civicrm46d7', 'civicrm46d7-test-migrate');
  provision_civicrm_tests_migrate_site('civicrm46d7-test-migrate', 'civicrm46d7_other');
  provision_civicrm_tests_remove_site('civicrm46d7-test-migrate');
  provision_civicrm_tests_remove_platform('civicrm46d7_other');
}

/**
 * Test upgrade from one CiviCRM version to another (ex: 4.4 to 4.6).
 */
function provision_civicrm_tests_003_major_upgrades() {
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.4-D7 to 4.6-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  provision_civicrm_tests_install_site('civicrm44d7', 'civicrm44d7-test-upgrade');
  provision_civicrm_tests_migrate_site('civicrm44d7-test-migrate', 'civicrm46d7');
  provision_civicrm_tests_remove_site('civicrm44d7-test-upgrade');
}

/**
 * Helper function to install a platform.
 */
function provision_civicrm_tests_install_platform($platform_name, $platform_alias = NULL) {
  if (is_null($platform_alias)) {
    $platform_alias = $platform_name;
  }
  drush_log(dt('Building platform: @platform and adding to hostmaster.', array('@platform' => $platform_alias)), 'ok');
  $args = array(
    PROVISION_CIVICRM_TESTS_BUILDS_REPO . "/$platform_name.build",
    "/var/aegir/platforms/$platform_alias"
  );

  drush_invoke_process('@none', 'make', $args);

  $args = array(
    "@platform_$platform_alias",
  );

  $options = array(
    'root' => "/var/aegir/platforms/$platform_alias",
    'context_type' => 'platform',
  );

  drush_invoke_process('@none', 'provision-save', $args, $options);
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@platform_$platform_alias"));
  drush_provision_civicrm_tests_run_remaining_tasks();

  if (drush_get_error() != DRUSH_SUCCESS) {
    throw new Exception('Drush error: ' . drush_get_error());
  }
}

/**
 * Helper function to remove a platform.
 */
function provision_civicrm_tests_remove_platform($platform_name) {
  drush_log(dt('Removing platform: @platform.', array('@platform' => $platform_name)), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'delete'));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Helper function to install a site.
 */
function provision_civicrm_tests_install_site($platform_name, $site, $profile_name = 'standard') {
  drush_log(dt('Installing: @site on platform: @platform with profile: @profile.', array('@site' => "$site.aegir.example.com", '@platform' => $platform_name, '@profile' => $profile_name)), 'ok');

  // For some reason, Aegir3/Debian configured me a 127.0.0.1 host for the db
  // and a server with the localhost name, instead of @server_localhost as
  // hardcoded in the provision tests. The following does a quick check to
  // fetch the db server.
  static $db_server = NULL;

  if ($db_server == NULL) {
    $db_server = db_query('SELECT c.name
      FROM hosting_context c
      INNER JOIN hosting_service s on (s.nid=c.nid AND s.service = :type)
      ORDER BY s.nid ASC LIMIT 1', array(':type' => 'db'))->fetchField();
  }

  if (! $db_server) {
    drush_set_error(drush_get_error(), 'Could not find the db_server for this host.');
    throw new Exception('Could not find the db_server for this host.');
  }

  $args = array(
    "@$site.aegir.example.com",
  );

  $options = array(
    'uri' => "$site.aegir.example.com",
    'context_type' => 'site',
    'platform' => "@platform_$platform_name",
    'profile' => $profile_name,
    'db_server' => "@$db_server",
    'root' => "/var/aegir/platforms/$platform_name",
    'client_email' => 'this.email@is.invalid',
  );

  drush_invoke_process('@none', 'provision-save', $args, $options);

  $args = array();
  $options = array(
    'client_email' => 'this.email@is.invalid',
  );

  provision_backend_invoke("@$site.aegir.example.com", 'provision-install', $args, $options);
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'verify'), array('force' => TRUE));
  drush_provision_civicrm_tests_run_remaining_tasks();

  if (drush_get_error() != DRUSH_SUCCESS) {
    throw new Exception('Drush error: ' . drush_get_error());
  }
}

/**
 * Helper function to delete a site.
 */
function provision_civicrm_tests_remove_site($site) {
  drush_log(dt('Removing: @site.', array('@site' => "$site.aegir.example.com")), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'delete'), array('force' => TRUE));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Migrates a site from one platform to another.
 *
 * @param $site
 *   The site to migrate.
 * @param $target
 *   The target platform to migrate to.
 */
function provision_civicrm_tests_migrate_site($site, $target) {
  drush_log(dt('Migrating: @site to platform: @platform.', array('@site' => "$site.aegir.example.com", '@platform' => $target)), 'ok');
  // Do the migrate.
  provision_backend_invoke("@$site.aegir.example.com", 'provision-migrate', array("@platform_$target", array("debug" => TRUE)));
  // Import the site into the frontend.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com"));
  // Verify the $target platform.
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$target", 'verify'), array('force' => TRUE));
  // Import and verify the site.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com"));
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'verify'), array('force' => TRUE));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Run all remaining hosting tasks.
 */
function drush_provision_civicrm_tests_run_remaining_tasks() {
  $tasks = array();
  $result = db_query("SELECT t.nid FROM {hosting_task} t INNER JOIN {node} n ON t.vid = n.vid WHERE t.task_status = 0 ORDER BY n.changed, n.nid ASC");
  foreach ($result as $node) {
    $tasks[$node->nid] = node_load($node->nid);
  }

  foreach ($tasks as $task) {
    provision_backend_invoke('@hostmaster', "hosting-task", array($task->nid), array('force' => TRUE));
  }
}
