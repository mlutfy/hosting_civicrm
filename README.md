Hosting CiviCRM
===============

This module provides tools to manage CiviCRM [1] in the Aegir Hosting System
[2]. In other words, it will handle the installation of CiviCRM, generate the
civicrm.settings.php file, handle upgrades, manage the CiviCRM crons and the
CiviCRM API site key, and so on.

[1] https://civicrm.org
[2] http://www.aegirproject.org/

To get the latest version of `hosting_civicrm` or to submit a patch (merge
request), please see the project on Gitlab:  
https://gitlab.com/aegir/hosting_civicrm

Requirements
============

- Required: Aegir >= 3.x
- Recommended: CiviCRM >= 4.4 (CiviCRM 4.2-4.4 are still tested but require a patch, see [14]).
- Supports Drupal 6, Drupal 7 and Drupal 8

[14] https://github.com/mlutfy/hosting_civicrm/wiki/CiviCRM-version-support

Installation
============

As of version 3.2, this module has been included in the Aegir distribution, as
part of the "Golden contrib" initiative. As such, it is automatically available
to be enabled without the need to deploy any code.

* In Aegir, enable CiviCRM and the CiviCRM cron queue from the "Hosting" options (under "experimental" options).
* Create a platform with [CiviCRM](https://civicrm.org/download/list) located in sites/all/modules/ (or in an install profile)
* In Aegir, add the platform inside Aegir (Node -> add -> platform)

When new sites are created in the platform, provision_civicrm will detect that CiviCRM is available and will automatically install it.

For convenience, a "drush make" makefile is available in drush/civicrm.make.yml which can be used to generate a platform for you.

Debugging
=========

If you are having problems running the crons, try:

  drush '@hostmaster' hosting-civicrm_cron --items=5 --debug --strict=0


Support
=======

Please use the issue queue for support:

* https://drupal.org/project/issues/hosting_civicrm

You can also ask questions in either the #aegir or #civicrm IRC channel on
irc.freenode.org, but keep in mind that most active people in those channels do
not necessarely use this module. You can try to ping the module maintaners,
'bgm' or 'ergonlogic'.

Commercial hosting, support and development is also possible:

* Praxis Labs Coop <http://praxis.coop/> (hosting, support, dev)
* Coop SymbioTIC <https://www.symbiotic.coop> (dev)
* Ergon Logic Enterprises <https://www.symbiotic.coop> (dev, support)
* Omega8.cc <https://omega8.cc/> (hosting)
* Civi-go <http://civigo.net/> (hosting, dev via Ixiam.com)
* Koumbit <http://www.koumbit.org> (hosting)
* Progressive Technology Project <http://www.progressivetech.org/> (hosting)

Other Aegir service providers:

* http://community.aegirproject.org/service-providers

If you appreciate this module, please consider donating to either CiviCRM
or the Aegir project.

* https://civicrm.org/participate/support-civicrm
* http://aegirproject.org/donate

You can also send the lead module maintainer a beer:

* https://www.bidon.ca/en/paypal


Patches and testing
===================

You can send a patch attached to an issue on drupal.org [11] or send a
pull-request on Github [12].

Pull-requests are not required, but have the added benefit of being tested
automatically [13] against most CiviCRM versions that are supported.

[11] https://drupal.org/project/issues/hosting_civicrm
[12] https://github.com/mlutfy/hosting_civicrm
[13] https://github.com/mlutfy/hosting_civicrm/wiki/Continuous-integration


Credits
=======

Initial development was by Mathieu Petit-Clair [3] during the CiviCRM code
sprint in San Francisco of spring 2010, with the help of Deepak Srivastava [4]
who wrote the CiviCRM drush module.

Maintenance and development was then continued by Mathieu Lutfy [5], with the
help of many contributors [6] and with great support from the CiviCRM core team
and community. Front-end components were later added by Christopher Gervais [7].

Ongoing development and maintenance by Coop SymbioTIC [8], Praxis Labs Coop [9]
and Ergon Logic Enterprises [10].

[3] https://drupal.org/user/1261
[4] http://civicrm.org/blogs/deepaksrivastava
[5] https://drupal.org/u/bgm
[6] https://drupal.org/node/1063394/committers
[7] https://drupal.org/u/ergonlogic
[8] https://www.symbiotic.coop
[9] http://praxis.coop
[10] http://ergonlogic.com

Thanks to Koumbit, Praxis, Ixiam, PTP, JMA consulting and NDI for financially
supporting the development of this module.


License
=======

(C) 2012-2015 Christopher Gervais <https://www.drupal.org/u/ergonlogic>
(C) 2012-2015 Mathieu Lutfy <mathieu@bidon.ca>
(C) 2012-2015 Coop SymbioTIC <info@symbiotic.coop>

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
