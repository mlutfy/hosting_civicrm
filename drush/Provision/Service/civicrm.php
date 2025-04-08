<?php

/**
 * The civicrm service class.
 */
class Provision_Service_civicrm extends Provision_Service {
  public $service = 'civicrm';

  /**
   * Add the civicrm_version property to the site context.
   */
  static function subscribe_site($context) {
    $context->setProperty('civicrm_sitekey');
    $context->setProperty('civicrm_ansible_sitekey');
    $context->setProperty('civicrm_ansible_credkey');
    $context->setProperty('civicrm_ansible_signkey');

    // Not used?
    $context->setProperty('civicrm_version');
  }
}

