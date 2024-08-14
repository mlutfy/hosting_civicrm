# Hosting CiviCRM - Coop Symbiotic fork

This module provides tools to manage [CiviCRM](https://civicrm.org) in the
[Aegir Hosting System](https://www.aegirproject.org/). In other words, it will
handle the installation of CiviCRM, generate the civicrm.settings.php file,
handle upgrades, manage the CiviCRM crons and the CiviCRM API site key, and so on.

To get the latest version of `hosting_civicrm` or to submit a patch (pull
request), please see the project on Github:  
https://github.com/mlutfy/hosting_civicrm

- Supports CiviCRM 5.0 or later
- Supports Drupal 7 to Drupal 10, WordPress and partial CiviCRM Standalone support

# This is a Coop Symbiotic fork

In August 2024, we forked this module in order to make some more drastic changes.
Coop Symbiotic's hosting does a few non-Aegir-standard things when managing sites.
For example, site crons are configured using SystemD timers/services. We also increasingly
rely on Ansible to configure sites. We support Drupal7, Drupal10+, WordPress and Standalone,
and want a consistent solution.

In 2024, in order to support Drupal 10+ (which requires a newer drush), we
started making some major changes in the `provision` module. Instead of
"bootstrapping" the CMS, our provision fork uses the command line using drush
or cv or wp-cli to setup sites. This broke a lot of things initially, but
ultimately resulted in a simpler model (h/t to work by Jon Pugh who saw the
necessity for this a long time ago, and Omega8 for their early D9/D10 work).

Ultimately our goal is to have as little Aegir code as possible, so that when
we move off Drupal 7, it will not be too complicated. We do not currently
have a roadmap for that. Besides stabilising our forks, our priorities
are to add full CiviCRM Standalone support, and to remove `hosting_https`.

Everything we do is Free Software, and we commit to always do so, but we cannot
garantee it will work for you. You can hire us if you need a hand or would like
to sponsor some improvements. We specialise in Drupal, WordPress and CiviCRM
hosting (and everything that goes with it, such as monitoring and backups).

Here are the main non-standard Aegir repos that we use:

- `provision`: https://github.com/mlutfy/provision/ (fork for Drupal 10+ support and more)
- `provision_symbiotic`: https://github.com/coopsymbiotic/provision_symbiotic (mostly Symbiotic-specific tweaks)
- `hosting_civicrm`: https://github.com/mlutfy/hosting_civicrm (this repo)
- `hosting_civicrm_ansible`: https://github.com/coopsymbiotic/hosting_civicrm_ansible (wrapper for calling Ansible)
- Our Ansible roles: https://github.com/coopsymbiotic/coopsymbiotic-ansible/

## Requirements

- PHP 7.3 or later (and tested up to PHP 8.2)
- Aegir 3.x
- Provision fork by Symbiotic: https://github.com/mlutfy/provision/ ('symbiotic' branch)

## Installation

As of version 3.2, this module has been included in the Aegir distribution, as
part of the "Golden contrib" initiative. As such, it is automatically available
to be enabled without the need to deploy any code.

* In Aegir, enable CiviCRM and the CiviCRM cron queue from the "Hosting" options (under "experimental" options).
* Create a platform with [CiviCRM](https://civicrm.org/download/list) located in sites/all/modules/ (or in an install profile)
* In Aegir, add the platform inside Aegir (Node -> add -> platform)

When new sites are created in the platform, provision_civicrm will detect that CiviCRM is available and will automatically install it.

## Support

Please use the issue queue for support:  
https://github.com/mlutfy/hosting_civicrm/issues

Please note that we provide very limited community support on Github.

For paid support or to sponsor development:  
https://www.symbiotic.coop/en/contact

## Patches and testing

You can send a patch attached to an issue on drupal.org [11] or send a
pull-request on Github [12].

Pull-requests are not required, but have the added benefit of being tested
automatically [13] against most CiviCRM versions that are supported.

[11] https://drupal.org/project/issues/hosting_civicrm
[12] https://github.com/mlutfy/hosting_civicrm
[13] https://github.com/mlutfy/hosting_civicrm/wiki/Continuous-integration


## Credits

Initial development was by Mathieu Petit-Clair [3] during the CiviCRM code
sprint in San Francisco of spring 2010, with the help of Deepak Srivastava [4]
who wrote the CiviCRM drush module.

Maintenance and development was then continued by Mathieu Lutfy [5], with the
help of many contributors [6] and with great support from the CiviCRM core team
and community. Front-end components were later added by Christopher Gervais [7].

Ongoing development and maintenance by Coop SymbioTIC [8].

[3] https://drupal.org/user/1261
[4] http://civicrm.org/blogs/deepaksrivastava
[5] https://drupal.org/u/bgm
[6] https://drupal.org/node/1063394/committers
[7] https://drupal.org/u/ergonlogic
[8] https://www.symbiotic.coop

Thanks to Koumbit, Praxis, Ixiam, PTP, JMA consulting and NDI for financially
supporting the development of this module.


## License

(C) 2012-2024 Mathieu Lu <mathieu@bidon.ca>  
(C) 2012-2024 Coop SymbioTIC <info@symbiotic.coop>  
(C) 2012-2015 Christopher Gervais <https://www.drupal.org/u/ergonlogic>  
(C) 2010-2012 Mathieu Petit-Clair

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

See LICENSE.txt for details.
