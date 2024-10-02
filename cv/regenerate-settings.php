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
  'sign_keys' => FALSE,
  'cred_keys' => FALSE,
  'deploy_id' => FALSE,
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

// Guess the templates_c path
if (in_array($config['cms'], ['Drupal', 'Drupal8'])) {
  $config['templates_c'] = getcwd() . '/private/files/civicrm/templates_c';
}
elseif ($config['cms'] == 'WordPress') {
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
        } elseif ($token[0] == T_STRING && strtolower($token[1]) == 'define') {
            $state = 1;
        } elseif ($state == 2 && is_constant($token[0])) {
            $key = $token[1];
            $state = 3;
        } elseif ($state == 4 && is_constant($token[0])) {
            $value = $token[1];
            $state = 5;
        }
      } else {
        $symbol = trim($token);
        if ($symbol == '(' && $state == 1) {
          $state = 2;
        } elseif ($symbol == ',' && $state == 3) {
          $state = 4;
        } elseif ($symbol == ')' && $state == 5) {
          $value = strip($value);
          $key = strip($key);
	  // Ignore any value such as %%deployID%% or a constant defined by
          // another constant (ex: setting CIVICRM_SIGN_KEYS = _CIVICRM_SIGN_KEYS)
          if ($value && substr($value, 0, 2) == '%%' && substr($value, -2, 2) == '%%' && $key != substr($value, 1)) {
            $defines[$key] = $value;
          }
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

if (empty($_SERVER['db_user']) || empty($_SERVER['db_passwd'])) {
  throw new Exception("Missing database credentials such as db_user or db_passwd.");
}

// Helper
$toAlphanum = function($bits) {
  return preg_replace(';[^a-zA-Z0-9];', '', base64_encode($bits));
};

// Setup CiviCRM settings if not set
if ($config['regen_keys']) {
  $config['cred_keys'] = 'aes-cbc:hkdf-sha256:' . $toAlphanum(random_bytes(37));
  $config['deploy_id'] = $toAlphanum(random_bytes(10));
  $config['site_key'] = $toAlphanum(random_bytes(32));
  $config['sign_keys'] = 'jwt-hs256:hkdf-sha256:' . $toAlphanum(random_bytes(40));
}

// Check for any missing variable
$config['cred_keys'] = $config['cred_keys'] ?: $defines['_CIVICRM_CRED_KEYS'] ?? $defines['CIVICRM_CRED_KEYS'] ?? 'aes-cbc:hkdf-sha256:' . $toAlphanum(random_bytes(37));
$config['deploy_id'] = $config['deploy_id'] ?: $defines['_CIVICRM_DEPLOY_ID'] ?? $defines['CIVICRM_DEPLOY_ID'] ?? $toAlphanum(random_bytes(10));
$config['site_key'] = $config['site_key'] ?: $defines['_CIVICRM_SITE_KEY'] ?? $defines['CIVICRM_SITE_KEY'] ?? $toAlphanum(random_bytes(32));
$config['sign_keys'] = $config['sign_keys'] ?: $defines['_CIVICRM_SIGN_KEYS'] ?? $defines['CIVICRM_SIGN_KEYS'] ?? 'jwt-hs256:hkdf-sha256:' . $toAlphanum(random_bytes(40));

$settingstpl = file_get_contents($config['civicrm_root'] . '/templates/CRM/common/civicrm.settings.php.template');

$tokens = [
  'cms' => $config['cms'],
  'CMSdbUser' => $_SERVER['db_user'],
  'CMSdbPass' => $_SERVER['db_passwd'],
  'CMSdbHost' => $_SERVER['db_host'] . ':' . $_SERVER['db_port'],
  'CMSdbName' => $_SERVER['db_name'],
  // @todo
  'CMSdbSSL' => '',
  'dbUser' => $_SERVER['db_user'],
  'dbPass' => $_SERVER['db_passwd'],
  'dbHost' => $_SERVER['db_host'] . ':' . $_SERVER['db_port'],
  'dbName' => $_SERVER['db_name'],
  // @todo
  'dbSSL' => '',
  'extraSettings' => '',
  'crmRoot' => $config['civicrm_root'],
  'templateCompileDir' => $config['templates_c'],
  'baseURL' => $config['host'],
  'siteKey' => $config['site_key'],
  'credKeys' => $config['cred_keys'],
  'deployID' => $config['deploy_id'],
  'signKeys' => $config['sign_keys'],
  'templateCompileDir' => $config['templates_c'],
];

$extraSettings = [
  'Additional settings generated by hosting_civicrm',
];

if (in_array($config['cms'], ['Drupal', 'Drupal8'])) {
  $urlParts = parse_url($config['host']);
  $extraSettings[] = '$civicrm_paths[\'civicrm.files\'][\'url\'] = \'' . $config['host'] . '/sites/' . $urlParts['host'] . '/files/civicrm\';';
  $extraSettings[] = '$civicrm_paths[\'civicrm.files\'][\'path\'] = \'' . getcwd() . '/files/civicrm\';';
  $extraSettings[] = '$civicrm_paths[\'civicrm.private\'][\'path\'] = \'' . getcwd() . '/private/files/civicrm\';';
}

// Always enable clean URLs
$extraSettings[] = '// Force Clean URLs to prevent bad Civi multilingual urls';
$extraSettings[] = 'define(\'CIVICRM_CLEANURL\', 1);';

// On WordPress, include the drushrc.php for the WP salts
if ($config['cms'] == 'WordPress') {
  $extraSettings[] = '// Ensure we have the WP salts loaded';
  $extraSettings[] = '@include_once(\'' . getcwd() . '/drushrc.php\');';
}

$tokens['extraSettings'] = implode("\n", $extraSettings);

// Interpolate tokens
foreach ($tokens as $token => $value) {
  $settingstpl = str_replace('%%' . $token . '%%', $value, $settingstpl);
}

// Write the file
chmod($settingsPath, 0640);
file_put_contents($settingsPath, $settingstpl);
chmod($settingsPath, 0440);
