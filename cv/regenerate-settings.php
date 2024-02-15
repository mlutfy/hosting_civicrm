<?php

// This should be invoked with: cv php:script cv/regenerate-settings.php

// FIXME 
$corePath = '/var/aegir/platforms/civicrm-d7/sites/all/modules/civicrm';

require_once getcwd() . '/drushrc.php';

\Civi\Setup::assertProtocolCompatibility(1.0);
\Civi\Setup::init([
  // This is just enough information to get going. Drupal.civi-setup.php does more scanning.
  // FIXME set the correct CMS
  'cms' => 'Drupal',
  'srcPath' => $corePath,
  // 'lang' => $lang,
  'credKeys' => 'aes-cbc:hkdf-sha256:abcd',
  'signKeys' => 'TEST SIGN',
  'db' => [
    'server' => $_SERVER['db_host'],
    'username' => $_SERVER['db_user'],
    'password' => $_SERVER['db_passwd'],
    'database' => $_SERVER['db_name'],
  ],
]);
$ctrl = \Civi\Setup::instance()->createController()->getCtrl();
$ctrl->setUrls([
  'ctrl' => url('civicrm'),
  // 'res' => $coreUrl . '/setup/res/',
  // 'jquery.js' => $coreUrl . '/bower_components/jquery/dist/jquery.min.js',
  // 'font-awesome.css' => $coreUrl . '/bower_components/font-awesome/css/font-awesome.min.css',
]);

/**
 * @var \Civi\Setup\Model $model
 */
$setup = $ctrl->getSetup();
$model = $setup->getModel();
$params = \Civi\Setup\SettingsUtil::createParams($model);

$parent = dirname($model->settingsPath);
if (!file_exists($parent)) {
  Civi\Setup::log()->info('[InstallSettingsFile.civi-setup.php] mkdir "{path}"', ['path' => $parent]);
  mkdir($parent, 0777, TRUE);
  \Civi\Setup\FileUtil::makeWebWriteable($parent);
}

// And persist it...
$tplPath = implode(DIRECTORY_SEPARATOR,
  [$model->srcPath, 'templates', 'CRM', 'common', 'civicrm.settings.php.template']
);
$str = \Civi\Setup\SettingsUtil::evaluate($tplPath, $params);
echo "Path = " . $model->settingsPath . "\n";
file_put_contents($model->settingsPath . '.TEST', $str);
