<?php

/**
 * Implements the provision-civicrm-cli command.
 */
function drush_provision_civicrm_cli() {
  // Normally argv will have:
  // 0 = /path/to/drush.php
  // 1 = --php=foo
  // 2 = --php-options=bar
  // 3 = @fooalias
  // 4 = cv
  // 5 = api (etc)
  //
  // We remove the first arguments until 'cv' is first and leave the rest intact.
  do {
    array_shift($GLOBALS['argv']);
  } while (isset($GLOBALS['argv'][0]) && $GLOBALS['argv'][0] != 'cv');

  do {
    array_shift($_SERVER['argv']);
  } while (isset($_SERVER['argv'][0]) && $_SERVER['argv'][0] != 'cv');

  require_once '/usr/local/src/cv/vendor/autoload.php';
  require_once '/usr/local/src/cv/src/Bootstrap.php';
  require_once '/usr/local/src/cv/src/Application.php';

  $application = new \Civi\cv\Application('cv', '@package_version@');
  $application->setAutoExit(FALSE);
  $application->run();
}
