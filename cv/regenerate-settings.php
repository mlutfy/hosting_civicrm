<?php

// This should be invoked with plain PHP, example:
// php cv/regenerate-settings.php \
//   --host=https://example.org \
//   --civicrm_root=/path/to/civicrm/ \
//   --cms={Drupal,Drupal8,WordPress,Standalone} \
//   --site_key=[...] \
//   [--regen_keys=1]
//
// (previously was run using 'cv' but that did not work well for migrate/clone/restore)
//
// if a bool value to [regen-keys] is passed, then the site key and creds are rotated (ex: for site cloning)
// @todo This is not really implemented at the moment in hosting_civicrm (in the parent functions)
//
// This script assumes that you have a somewhat working CiviCRM installation
// It might fix some settings, but the main objective is to regenerate the settings
// file using the latest CiviCRM settings template.
// Do not rely on the stability of this script, it will likely change a bit in 2024.

// Parse command-line options
// Based on https://stackoverflow.com/a/26520115
$config = [
  // Required values
  'host' => NULL,
  'civicrm_root' => NULL,
  'cms' => NULL,
  // Optional values: set defaults
  'regen_keys' => FALSE,
  'site_key' => FALSE,
  'templates_c' => FALSE,
];

for ($i = 1; $i < count($argv); $i++) {
  if (preg_match('/^--([^=]+)=(.*)/', $argv[$i], $match)) {
    $config[$match[1]] = $match[2];
  }
}

// Check required values
foreach ($config as $key => $val) {
  if ($val === NULL) {
    throw new Exception("$key is a required value. Ex: --$key=VAL");
  }
}

$settingsPath = 'civicrm.settings.php';
if (!file_exists($settingsPath)) {
  if (file_exists('wp-content/uploads/civicrm/civicrm.settings.php')) {
    $settingsPath = 'wp-content/uploads/civicrm/civicrm.settings.php';
  }
}

if (in_array($config['cms'], ['Drupal', 'Drupal8'])) {
  $config['templates_c'] = getcwd() . '/private/files/civicrm/templates_c';
}
elseif ($model->cms == 'WordPress') {
  $config['templates_c'] = getcwd() . '/wp-content/uploads/civicrm/templates_c';
}
else {
  throw new Exception('Unknown CMS (' . $config['cms'] . ') - cannot set templates_c');
}

// Is there an existing civicrm.settings.php file?
if (file_exists($settingsPath)) {
  $settingsOld = file_get_contents($settingsPath, true);
  if ($settingsOld !== false) {
    // Extract existing constants, such as CIVICRM_CRED_KEYS etc into a $defines array
    // Based on: https://stackoverflow.com/q/645862
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

    // We have to manually update the $civicrm_root in the civicrm.settings.php
    // file, because using cv assumes a functionnal CiviCRM, which will not work
    // if we are cloning from a platform to another (duplicate ClassLoader).
    $replacements = [];
    $replacements[] = [
      'search_regex' => '/^.*?\$civicrm_root =.*\n?/m',
      'replace_line' => "\$civicrm_root = '{$config['civicrm_root']}';\n",
    ];
    // This is not great because we assume [civicrm.private] points here
    // but .. it should?
    $replacements[] = [
      'search_regex' => "#'CIVICRM_TEMPLATE_COMPILEDIR', '/var/aegir/platforms/[^/]+/(web/)?sites/[^/]+/(private/)?files/civicrm/templates_c'#",
      'replace_line' => "'CIVICRM_TEMPLATE_COMPILEDIR', '{$config['templates_c']}'",
    ];

    foreach ($replacements as $desc => $line) {
      // Throw a warning if any of our replacements cannot be found.
      if (!preg_match($line['search_regex'], $settingsOld)) {
        echo "[warning] cv/regenerate-settings.php: Failed to replace: {$line['search_regex']}\n";
      }
      else {
        $count = 0;
        $settingsOld = preg_replace($line['search_regex'], $line['replace_line'], $settingsOld, -1, $count);
        if ($count > 0) {
          echo "[success] cv/regenerate-settings.php: Succeeded to replace: {$line['search_regex']}\n";
        }
      }
    }

    chmod($settingsPath, 0640);
    file_put_contents($settingsPath, $settingsOld);
    chmod($settingsPath, 0440);
  }
}

function is_constant($token) {
  return $token == T_CONSTANT_ENCAPSED_STRING || $token == T_STRING ||
    $token == T_LNUMBER || $token == T_DNUMBER;
}
function strip($value) {
  return preg_replace('!^([\'"])(.*)\1$!', '$2', $value);
}

// We are running inside cv, so all CiviCRM vars are available
$corePath = $config['civicrm_root'];

// Grab Drush relevant variables
require_once getcwd() . '/drushrc.php';

// Ensure that https URLs are generated, especially on WordPress
$_SERVER['HTTPS'] = 'on';

// Civi\Setup fails because of bootstrap issues on Drupal9+
// but for now, leaving the others on level=classloader because it is
// less likely to crash if some values are incorrectly set
if ($config['cms'] == 'Drupal8') {
  eval(`cv php:boot --level=cms-full`);
}
else {
  eval(`cv php:boot --level=classloader`);
}

\Civi\Setup::assertProtocolCompatibility(1.0);
\Civi\Setup::init([
  // This is just enough information to get going. *.civi-setup.php does more scanning.
  'cms' => $config['cms'],
  // This should not be necessary but otherwise we get very weird results
  // such as: http://crm.example.org/usr/local/bin/usr/local/bin/aegir
  // c.f. civicrm-core/setup/plugins/init/Drupal8.civi-setup.php
  'cmsBaseUrl' => $config['host'],
  'srcPath' => $corePath,
]);

if (empty($_SERVER['db_user']) || empty($_SERVER['db_passwd'])) {
  throw new Exception("Missing database credentials such as db_user or db_passwd.");
}

// init() made the initial guess. Now we can overwrite with user-supplied data.
$setup = \Civi\Setup::instance();
/**
 * @var \Civi\Setup\Model $model
 */
$model = $setup->getModel();
$model->cmsDb = [
  'server' => $_SERVER['db_host'] . ':' . $_SERVER['db_port'],
  'username' => $_SERVER['db_user'],
  'password' => $_SERVER['db_passwd'],
  'database' => $_SERVER['db_name'],
  'dbSSL' => '', // Need to set if relevant later
  'CMSdbSSL' => '', // Need to set if relevant later
];
$model->db = [
  'server' => $_SERVER['db_host'] . ':' . $_SERVER['db_port'],
  'username' => $_SERVER['db_user'],
  'password' => $_SERVER['db_passwd'],
  'database' => $_SERVER['db_name'],
  'dbSSL' => '', // Need to set if relevant later
  'CMSdbSSL' => '', // Need to set if relevant later
];
// if ($lang) {
//  $model->lang = $lang;
//}

// Define imported values
if (!$config['regen_keys']) {
  $model->credKeys = [$defines['_CIVICRM_CRED_KEYS'] ?? $defines['CIVICRM_CRED_KEYS']];
  $model->deployID = $defines['_CIVICRM_DEPLOY_ID'] ?? $defines['CIVICRM_DEPLOY_ID'];
  $model->siteKey = $config['site_key'] ?: $defines['CIVICRM_SITE_KEY'];
  $model->signKeys = [$defines['_CIVICRM_SIGN_KEYS'] ?? $defines['CIVICRM_SIGN_KEYS']];
}
$model->cms = $config['cms'];
$model->cmsBaseUrl = $config['host'];
$model->templateCompilePath = $config['templates_c'];

// This might not always be necessary, but it was done before for Drupal7
// without this, on d7, it will default to "private/files/civicrm"
// ex: cv ev 'echo \Civi::paths()->getPath("[civicrm.files]/");'
$model->paths = [
  'civicrm.files' => [
    'path' => getcwd() . '/files/civicrm',
  ]
];

// Setup CiviCRM settings if not set
if ($config['regen_keys']) {
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

// Regenerate TPL file and output
$tplPath = implode(DIRECTORY_SEPARATOR,
  [$model->srcPath, 'templates', 'CRM', 'common', 'civicrm.settings.php.template']
);
$str = \Civi\Setup\SettingsUtil::evaluate($tplPath, $params);

// Find Clean URLs section
$pos = strpos($str, "if (!defined('CIVICRM_CLEANURL'))");
if ($pos !== FALSE) {
  $str = substr($str, 0, $pos)
    . "// Added by Aegir: Force Clean URLs to prevent bad Civi multilingual urls\ndefine('CIVICRM_CLEANURL', 1 );\n\n"
    . substr($str, $pos, strlen($str));
}
else {
  echo 'Could not set CLEAN URL variable';
}

// On WordPress, include the drushrc.php for the WP salts
if ($model->cms == 'WordPress') {
  $pos = strpos($str, '// Additional settings generated by installer:');
  if ($pos !== FALSE) {
    $str = substr($str, 0, $pos)
      . "// Added by Aegir: ensures we have the WP salts loaded\n@include_once('" . getcwd() . "/drushrc.php');\n\n"
      . substr($str, $pos, strlen($str));
  }
  else {
    echo "Could not add the drushrc.php include\n";
  }
}

// Output the file
chmod($settingsPath, 0640);
file_put_contents($settingsPath, $str);
chmod($settingsPath, 0440);
