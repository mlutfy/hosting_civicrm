#!/bin/sh

set -e
set -x

# This is to allow phpunit to write the results.xml file as the aegir user,
# so that it gets collected as an artifact.
# Assumes that the 'aegir' user is a member of the 'gitlab-runner' unix group.
chmod g+w "${CI_PROJECT_DIR}/tests/"

# If https is enabled, the vhost might have multiple 'root' entries
# hence piping in uniq and head, as a precaution.
HOSTMASTER_ROOT=`sudo -u aegir grep root /var/aegir/config/server_master/nginx/vhost.d/${HOSTMASTER_SITE} | awk '{print $2}' | grep hostmaster | sed "s/;//" | uniq | head -1`

sudo -u aegir rm -fr "${HOSTMASTER_ROOT}/profiles/hostmaster/modules/aegir/hosting_civicrm/"

# so that jenkins (in the aegir group) can clone/update files.
sudo -u aegir chmod g+w "${HOSTMASTER_ROOT}/profiles/hostmaster/modules/aegir"

cd ${HOSTMASTER_ROOT}/profiles/hostmaster/modules/aegir
sudo -u aegir git clone https://gitlab.com/aegir/hosting_civicrm.git

cd hosting_civicrm
sudo -u aegir git fetch origin +refs/pull/*:refs/remotes/origin/pr/*
sudo -u aegir git checkout "$CI_COMMIT_SHA"

sudo -i -u aegir drush @hm cc drush
sudo -i -u aegir drush @hm cc all

# Allow the aegir user to write the tests (nb: adduser aegir jenkins)
ls -la ${CI_PROJECT_DIR}
chmod g+w ${CI_PROJECT_DIR}/tests

# NB: for xml results, jenkins expects things to be in its workspace (/home/jenkins/workspace)
# hence using /tmp/
cd ${HOSTMASTER_ROOT}/profiles/hostmaster/modules/aegir/hosting_civicrm
sudo -u aegir composer install
sudo -u aegir phpunit --configuration tests --log-junit ${CI_PROJECT_DIR}/tests/results.xml
