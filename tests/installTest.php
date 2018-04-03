<?php

namespace HostingCiviTest;

/**
 * Drupal/CiviCRM installation tests.
 */
class installTest extends HostingCiviTestCase {
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
    Command\PlatformInstall::run('civicrm46d6');
    Command\PlatformInstall::run('civicrm47d7');
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
    # Command\PlatformDelete::run('civicrm46d6');
    # Command\PlatformDelete::run('civicrm47d7');

    parent::tearDownAfterClass();
  }

  /**
   * Test the toString function.
   */
  public function testHello() {
    $result = 'hello';
    $expected = 'hello';
    $this->assertEquals($result, $expected);
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.6 D7.
   */
  public function testInstallAndDelete46d7() {
    Command\SiteInstall::run('civicrm46d7', 'civicrm46d7-standard');
    Command\SiteDelete::run('civicrm46d7-standard');
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.6 D6.
   */
  public function testInstallAndDelete46d6() {
    Command\SiteInstall::run('civicrm46d6', 'civicrm46d6-default', 'default');
    Command\SiteDelete::run('civicrm46d6-default');
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.7 D7.
   */
  public function testInstallAndDelete47d7() {
    Command\SiteInstall::run('civicrm47d7', 'civicrm47d7-standard');
    Command\SiteDelete::run('civicrm47d7-standard');
  }

}
