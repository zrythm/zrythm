# project-specific constants
#
# NOTE: packaging is strongly coupled to the upstream tarball URL and naming schemes - e.g.
#         TARBALL_FILE:      ${VERSION_STRING}.tar.gz
#         TARBALL_INNER_DIR: ${UPSTREAM_NAME}-${VERSION}/
#         DEB_TARBALL_FILE:  ${DEBIAN_NAME}_${VERSION}.orig.tar.gz
#         DEB_DIFFBALL_FILE: ${DEBIAN_NAME}_${VERSION}-1.diff.gz

readonly GITHUB_LOGIN=''                      # e.g. 'my-github-user-name'
readonly GITHUB_AUTH_TOKEN=''                 # e.g. '0123456789012345678901234567890123456789'
readonly OBS_LOGIN='alextee'                         # e.g. 'my-opensuse-user-name'
readonly UPSTREAM_NAME='zrythm'                     # e.g. 'my-github-project-name'
readonly OBS_NAME=$UPSTREAM_NAME              # e.g. 'my-opensuse-project-name'
readonly DEBIAN_NAME=$UPSTREAM_NAME           # e.g. 'my-debian-project-name'
readonly REMOTE_NAME='origin'                 # e.g. one of `git remote`
readonly STAGING_BRANCH='staging'             # NOTE: must be different than PACKAGING_BRANCH
readonly PACKAGING_BRANCH='packaging'         # NOTE: must be different than STAGING_BRANCH
readonly DEB_BUILD_TOOL='debuild'                 # one of 'debuild', 'sbuild', 'gbp'
readonly DEB_SBUILD_DIR=/var/cache/sbuild     # for DEB_BUILD_TOOL='sbuild'
readonly DEB_PBUILDER_DIR=/var/cache/pbuilder # for DEB_BUILD_TOOL='gbp'
readonly DEB_BUILD_DIST=testing
readonly DEB_BUILD_ARCH=amd64
readonly OBS_DIRNAME='obs'
readonly OSC_DIR=/code/osc/home\:${OBS_LOGIN}/${OBS_NAME}
readonly BUG_URL=https://git.zrythm.org/zrythm/zrythm/issues
readonly CHANGELOG_MESSAGE='updated to new upstream version'
readonly GIT_CLEAN_OPTIONS='-dx --force'
readonly N_MAKE_JOBS=42
readonly LOG_FILE=../gbp-buildlog
