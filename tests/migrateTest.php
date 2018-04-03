<?php

namespace HostingCiviTest;

/**
 * Drupal/CiviCRM migration tests.
 *
 * Not testing every combination possible, since it would be too long.
 */
class migrateTest extends HostingCiviTestCase {
  /**
   * @param string|null $name
   */
  public function __construct($name = NULL) {
    parent::__construct($name);
  }

  /**
   * Called before all test functions will be executed.
   * this function is defined in PHPUnit_TestCase and overwritten
   * here.
   *
   * https://phpunit.de/manual/current/en/fixtures.html#fixtures.more-setup-than-teardown
   */
  public static function setUpBeforeClass() {
    parent::setUpBeforeClass();

    Command\PlatformInstall::run('civicrm46d7');
    Command\PlatformInstall::run('civicrm46d7', 'civicrm46d7_other');
    Command\PlatformInstall::run('civicrm47d7');
    Command\PlatformInstall::run('civicrm47d7', 'civicrm47d7_other');
  }

  /**
   * Called after the test functions are executed.
   * this function is defined in PHPUnit_TestCase and overwritten
   * here.
   */
  public static function tearDownAfterClass() {
    // While in theory we should do this, it makes tests
    // take a really long time, and does have many benefits.
    # Command\PlatformDelete::run('civicrm46d7');
    # Command\PlatformDelete::run('civicrm46d7_other');
    # Command\PlatformDelete::run('civicrm47d7');
    # Command\PlatformDelete::run('civicrm47d7_other');

    parent::tearDownAfterClass();
  }

  /**
   * Test the migration of a 4.7 D7 site.
   */
  public function testMigrate47d7() {
    Command\SiteInstall::run('civicrm47d7', 'civicrm47d7-mig');

    // Enable extensions for testing
    Command\SiteUtils::enableExtension('civicrm47d7-mig', 'com.iatspayments.civicrm');
    Command\SiteUtils::enableExtension('civicrm47d7-mig', 'coop.symbiotic.aegirtests');

    // Start migration
    Command\SiteMigrate::run('civicrm47d7-mig', 'civicrm47d7_other');
    Command\SiteDelete::run('civicrm47d7-mig');
  }

  /**
   * Test the migration of a 4.6 D7 site.
   */
  public function testMigrate46d7() {
    Command\SiteInstall::run('civicrm46d7', 'civicrm46d7-mig');

    // Enable extensions for testing
    Command\SiteUtils::enableExtension('civicrm46d7-mig', 'com.iatspayments.civicrm');
    Command\SiteUtils::enableExtension('civicrm46d7-mig', 'coop.symbiotic.aegirtests');

    // Start migration
    Command\SiteMigrate::run('civicrm46d7-mig', 'civicrm46d7_other');
    Command\SiteDelete::run('civicrm46d7-mig');
  }

  /**
   * Test the migration of a 4.6 D7 site to 4.7 D7.
   */
  public function testMigrate46d7to47d7() {
    Command\SiteInstall::run('civicrm46d7', 'civicrm46d7-mig');

    // Enable extensions for testing
    Command\SiteUtils::enableExtension('civicrm46d7-mig', 'com.iatspayments.civicrm');
    Command\SiteUtils::enableExtension('civicrm46d7-mig', 'coop.symbiotic.aegirtests');

    // Start migration
    Command\SiteMigrate::run('civicrm46d7-mig', 'civicrm47d7');

    // Confirm that we are on CiviCRM 4.6
    $version = Command\SiteUtils::getCiviVersion('civicrm46d7-mig');
    $version = substr($version, 0, 3);
    $this->assertEquals($version, '4.7');

    Command\SiteDelete::run('civicrm46d7-mig');
  }

}
