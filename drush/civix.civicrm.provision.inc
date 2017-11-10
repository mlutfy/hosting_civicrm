<?php

/**
 * Implements the provision-civicrm-civix command.
 */
function drush_provision_civicrm_civix() {
  // Normally argv will have:
  // 0 = /path/to/drush.php
  // 1 = --php=foo
  // 2 = --php-options=bar
  // 3 = @fooalias
  // 4 = civix
  // 5 = civicrm:ping (etc)
  //
  // We remove the first arguments until 'cv' is first and leave the rest intact.
  do {
    array_shift($GLOBALS['argv']);
  } while (isset($GLOBALS['argv'][0]) && $GLOBALS['argv'][0] != 'civix');

  do {
    array_shift($_SERVER['argv']);
  } while (isset($_SERVER['argv'][0]) && $_SERVER['argv'][0] != 'civix');

  require_once '/usr/local/src/civix/vendor/autoload.php';

  $application = new \CRM\CivixBundle\Application('civix', '@package_version@');
  $application->setAutoExit(FALSE);
  $application->run();
}
