<?php

// This should be invoked with: cv php:script cv/regenerate-settings.php

function is_constant($token) {
  return $token == T_CONSTANT_ENCAPSED_STRING || $token == T_STRING ||
    $token == T_LNUMBER || $token == T_DNUMBER;
}
function strip($value) {
  return preg_replace('!^([\'"])(.*)\1$!', '$2', $value);
}

// We are running inside cv, so all CiviCRM vars are available
$corePath = $GLOBALS['civicrm_root'];

// Grab Drush relevant variables
require_once getcwd() . '/drushrc.php';

\Civi\Setup::assertProtocolCompatibility(1.0);
\Civi\Setup::init([
  // This is just enough information to get going. *.civi-setup.php does more scanning.
  'cms' => CIVICRM_UF,
  // This should not be necessary but otherwise we get very weird results
  // such as: http://crm.example.org/usr/local/bin/usr/local/bin/aegir
  // c.f. civicrm-core/setup/plugins/init/Drupal8.civi-setup.php
  'cmsBaseUrl' => CIVICRM_UF_BASEURL,
  'srcPath' => $corePath,
]);

// init() made the initial guess. Now we can overwrite with user-supplied data.
$setup = \Civi\Setup::instance();
$model = $setup->getModel();
$model->db = [
  'server' => $_SERVER['db_host'],
  'username' => $_SERVER['db_user'],
  'password' => $_SERVER['db_passwd'],
  'database' => $_SERVER['db_name'],
  'dbSSL' => '', // Need to set if relevant later
  'CMSdbSSL' => '', // Need to set if relevant later
];
// if ($lang) {
//  $model->lang = $lang;
//}

/**
 * @var \Civi\Setup\Model $model
 */
$model = $setup->getModel();

// Is there an existing civicrm.settings.php file? If so use existing values
$importSettings = FALSE;
if (file_exists($model->settingsPath)) {
  $settingsOld = file_get_contents($model->settingsPath, true);
  if ($settingsOld !== false) {
    // Using token_get_all ref: https://stackoverflow.com/questions/645862/regex-to-parse-define-contents-possible
    $value = $key = '';
    $state = 0;
    $defines = [];
    $tokens = token_get_all($settingsOld);
    $token = reset($tokens);
     while($token) {
      if (is_array($token)) {
        if ($token[0] == T_WHITESPACE || $token[0] == T_COMMENT || $token[0] == T_DOC_COMMENT) {
          // do nothing
        } else if ($token[0] == T_STRING && strtolower($token[1]) == 'define') {
            $state = 1;
        } else if ($state == 2 && is_constant($token[0])) {
            $key = $token[1];
            $state = 3;
        } else if ($state == 4 && is_constant($token[0])) {
            $value = $token[1];
            $state = 5;
        }
      } else {
        $symbol = trim($token);
        if ($symbol == '(' && $state == 1) {
            $state = 2;
        } else if ($symbol == ',' && $state == 3) {
            $state = 4;
        } else if ($symbol == ')' && $state == 5) {
            $defines[strip($key)] = strip($value);
            $state = 0;
        }
      }
      $token = next($tokens);

    }
  }
  $importSettings = TRUE;
/* For debugging
  foreach($defines as $k => $v) {
    echo "'$k' => '$v'\n";
  }
*/

  // Define imported values
  $model->credKeys = [$defines['_CIVICRM_CRED_KEYS']];
  $model->deployID = $defines['_CIVICRM_DEPLOY_ID'];
  $model->siteKey = $defines['CIVICRM_SITE_KEY'];
  $model->signKeys = [$defines['_CIVICRM_SIGN_KEYS']];
  $model->cms = $defines['CIVICRM_UF'];
  $model->cmsBaseUrl = $defines['CIVICRM_UF_BASEURL'];
  $model->templateCompilePath = $defines['CIVICRM_TEMPLATE_COMPILEDIR'];
}

// Setup CiviCRM settings if not set
if (!$importSettings) {
  // Generate all the relevant variables
  $toAlphanum = function($bits) {
    return preg_replace(';[^a-zA-Z0-9];', '', base64_encode($bits));
  };

  // Setup Cred Keys
  if (empty($model->credKeys)) {
    $model->credKeys = ['aes-cbc:hkdf-sha256:' . $toAlphanum(random_bytes(37))];
  }
  if (is_string($model->credKeys)) {
    $model->credKeys = [$model->credKeys];
  }
  // Setup Deploy Key
  if (empty($model->deployID)) {
    $model->deployID = $toAlphanum(random_bytes(10));
  }
  // Setup Site Key
  if (!empty($model->siteKey)) {
      // skip
  }
  elseif (function_exists('random_bytes')) {
    $model->siteKey = $toAlphanum(random_bytes(32));
  }
  elseif (function_exists('openssl_random_pseudo_bytes')) {
    $model->siteKey = $toAlphanum(openssl_random_pseudo_bytes(32));
  }
  else {
    throw new \RuntimeException("Failed to generate a random site key");
  }
  //Setup Sign Key
  if (empty($model->signKeys)) {
    $model->signKeys = ['jwt-hs256:hkdf-sha256:' . $toAlphanum(random_bytes(40))];
    // toAlpanum() occasionally loses a few bits of entropy, but random_bytes() has significant excess, so it's still more than ample for 256 bit hkdf.
  }
  if (is_string($model->signKeys)) {
    $model->signKeys = [$model->signKeys];
  }
}

// Build params
$params = \Civi\Setup\SettingsUtil::createParams($model);
$parent = dirname($model->settingsPath);
if (!file_exists($parent)) {
  Civi\Setup::log()->info('[InstallSettingsFile.civi-setup.php] mkdir "{path}"', ['path' => $parent]);
  mkdir($parent, 0777, TRUE);
  \Civi\Setup\FileUtil::makeWebWriteable($parent);
}

// Regenerate TPL file and output
$tplPath = implode(DIRECTORY_SEPARATOR,
  [$model->srcPath, 'templates', 'CRM', 'common', 'civicrm.settings.php.template']
);
$str = \Civi\Setup\SettingsUtil::evaluate($tplPath, $params);

// Find Clean URLs section
$pos = strpos($str, "if (!defined('CIVICRM_CLEANURL'))");
if ($pos !== FALSE) {
  $str = substr($str, 0, $pos) .
    "// Added by Aegir: Force Clean URLs to prevent bad Civi multilingual urls\ndefine('CIVICRM_CLEANURL', 1 );\n\n" .
     substr($str, $pos, strlen($str));
} else {
  echo 'Could not set CLEAN URL variable';
}

// Output the file
file_put_contents($model->settingsPath, $str);
chmod($model->settingsPath, 0440);
