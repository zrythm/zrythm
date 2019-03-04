#  git-packaging-hooks - git hooks to semi-automate releases and distro packaging
#
#  Copyright 2017 bill-auger <https://github.com/bill-auger/git-packaging-hooks/issues>
#
#  git-packaging-hooks is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 3 as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  git-packaging-hooks is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License version 3
#  along with git-packaging-hooks.  If not, see <http://www.gnu.org/licenses/>.


# trace helpers
function TraceStage() { local message=$* ; echo -e "HOOKS: $message" ;                  }
function TraceStep()  { local message=$* ; echo -e "         -> $message" ;             }
function TraceError() { local message=$* ; echo -e "\033[0;31mERROR: $message\033[0m" ; }
function Quit()       { local message=$* ; TraceError $message ; exit 1 ; }

# project-specific constants
readonly PROJECT_DEFS_FILE=$(git config --local core.hooksPath)/project-defs.sh.inc
source $PROJECT_DEFS_FILE
[ ! -f $PROJECT_DEFS_FILE ] && Quit "missing $PROJECT_DEFS_FILE - aborting commit"
[ -z "$GITHUB_LOGIN"   -o -z "$OBS_LOGIN"        -o -z "$UPSTREAM_NAME" -o \
  -z "$OBS_NAME"       -o -z "$DEBIAN_NAME"      -o -z "$REMOTE_NAME"   -o \
  -z "$STAGING_BRANCH" -o -z "$PACKAGING_BRANCH" -o -z "$OBS_DIRNAME"   -o \
  -z "$OSC_DIR"        -o -z "$BUG_URL"          -o -z "$N_MAKE_JOBS"      ] && \
  Quit "$PROJECT_DEFS_FILE incomplete - aborting commit"

[ $(echo $DEB_BUILD_TOOL | grep -E "^(debuild|sbuild|gbp)$") ] || \
  Quit "invalid \$DEB_BUILD_TOOL '$DEB_BUILD_TOOL' - aborting commit"


function init()
{
  readonly PROJECT_DIR=${PWD}
  readonly OBS_DIR=${PROJECT_DIR}/${OBS_DIRNAME}

  # files to modify
  readonly VERSION_FILE=configure.ac
  readonly SERVICE_FILE=$OBS_DIR/_service
  readonly SPEC_FILE=$OBS_DIR/$OBS_NAME.spec
  readonly DSC_FILE=$OBS_DIR/$OBS_NAME.dsc
  readonly PKGBUILD_FILE=$OBS_DIR/PKGBUILD

  # misc string constants
  readonly MINOR_VERSION_REGEX='^v[0-9]*\.[0-9]*$'
  readonly REVISION_REGEX='^v[0-9]*\.[0-9]*\.[0-9]*$'
  readonly GIT_USER=$(git config user.name      )
  readonly GPG_KEY=$( git config user.signingkey)
  readonly CURL_ERROR_MSG="could not find \`curl\` executable"
  readonly BRANCHES_ERROR_MSG="\$STAGING_BRANCH and \$PACKAGING_BRANCH can not be the same - aborting commit"
  readonly GIT_USER_ERROR_MSG="git config user.name not set - aborting commit"
  readonly GPG_KEY_ERROR_MSG="git config user.signingkey not set - aborting commit"
  readonly GITHUB_TOKEN_ERROR_MSG="\$GITHUB_AUTH_TOKEN not set in environment"
  readonly OBS_DIR_ERROR_MSG="OBS_DIR does not exist '$OBS_DIR'"
  readonly OSC_DIR_ERROR_MSG="OSC_DIR does not exist '$OSC_DIR'"
  readonly VERSION_TAG_ERROR_MSG="could not locate a minor version tag of the form: vMAJOR.MINOR"
  readonly REV_TAG_ERROR_MSG="could not locate a revision tag of the form: vMAJOR.MINOR.REV"
  readonly TAGS_FILE=$(git config --local core.hooksPath)/RESTORE_TAGS
  readonly FAUX_DSC_MD5SUM='0123456789abcdef0123456789abcdef                                '
  readonly FAUX_DSC_SIZE='1234567'
  readonly GITHUB_API_URL=https://api.github.com/repos/${GITHUB_LOGIN}/${UPSTREAM_NAME}/releases
  local    tarball_download_url=https://github.com/${GITHUB_LOGIN}/${UPSTREAM_NAME}/archive
  local    assets_download_url=https://github.com/${GITHUB_LOGIN}/${UPSTREAM_NAME}/releases/download
  readonly RELEASEID_REGEX='s/^.*\"assets_url\": \"https:\/\/api.github.com\/repos\/'${GITHUB_LOGIN}'\/loopidity\/releases\/\(.*\)\/assets\",.*$/\1/'
  readonly UPLOADURL_REGEX='s/^.*\"upload_url\": \"\(.*\){?.*\",.*$/\1/'
  readonly CURL_RESP_ARGS='--silent'
  readonly CURL_FETCH_ARGS='--silent --location --remote-name'
  readonly CURL_STATUS_ARGS='--silent --output /dev/null --write-out %{http_code}'
  readonly DEBOOTSTRAP_REPO="http://httpredir.debian.org/debian main"
  readonly CHROOTBALL=${DEB_SBUILD_DIR}/${DEB_BUILD_DIST}-${DEB_BUILD_ARCH}.tar.gz
  readonly PIUPARTS_CHROOT=${DEB_PBUILDER_DIR}/base-${DEB_BUILD_DIST}-${DEB_BUILD_ARCH}.cow/


  # detect amend commit
  [ "$(ps --pid $PPID --format command= | grep '\-\-amend')" ] && readonly IS_AMEND_COMMIT=1 || \
                                                                  readonly IS_AMEND_COMMIT=0

  # differentiate pre/post commit stages
  [ "$*" == '.git/COMMIT_EDITMSG' ] && readonly IS_PRE_COMMIT_STAGE=1 || \
                                       readonly IS_PRE_COMMIT_STAGE=0

  # detect empty commit message
  readonly GIT_COMMIT_MSG_FILE=.git/COMMIT_EDITMSG
  [ "$(grep --invert-match "^\s*#" $GIT_COMMIT_MSG_FILE)" ] && IS_EMPTY_COMMIT_MSG=0 || \
                                                               IS_EMPTY_COMMIT_MSG=1

  # detect commit to staging or packaging branch
  CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
  [ "$CURRENT_BRANCH" == "$STAGING_BRANCH"   ] && readonly IS_STAGING_BRANCH=1   || \
                                                  readonly IS_STAGING_BRANCH=0
  [ "$CURRENT_BRANCH" == "$PACKAGING_BRANCH" ] && readonly IS_PACKAGING_BRANCH=1 || \
                                                  readonly IS_PACKAGING_BRANCH=0

  # get existing revision tags
  local ifs=$IFS ; IFS=' ' ;
  local branch_minor_tags=$(git tag --list --merged    HEAD   | grep $MINOR_VERSION_REGEX)
  local branch_rev_tags=$(  git tag --list --merged    HEAD   | grep $REVISION_REGEX     )
  local unmerged_rev_tags=$(git tag --list --no-merged master | grep $REVISION_REGEX     )
  readonly MINOR_VERSION_TAG=$(echo $branch_minor_tags | sort --version-sort | tail --lines=1)
  readonly LAST_REVISION_TAG=$(echo $branch_rev_tags   | sort --version-sort | tail --lines=1)
  readonly HIGHEST_REV_TAG=$(  echo $unmerged_rev_tags | sort --version-sort | tail --lines=1)
  readonly REVISION_TAGS=$(    echo $unmerged_rev_tags | sort --version-sort                 )
  IFS=$ifs

  # get refs
  readonly HEAD_REF="$(         git rev-parse HEAD              )"
  readonly MINOR_VERSION_REF="$(git rev-parse $MINOR_VERSION_TAG)"
  readonly LAST_REVISION_REF="$(git rev-parse $LAST_REVISION_TAG)"

  # compute or extract next revision string
  if (($IS_STAGING_BRANCH))
  then local    n_revisions_ahead=$(git rev-list $MINOR_VERSION_TAG..HEAD --count)
       readonly SHOULD_INCREMENT_REVISION=$((! ($IS_AMEND_COMMIT + $IS_PRE_COMMIT_STAGE) ))
       (($SHOULD_INCREMENT_REVISION)) && readonly N_REVISIONS_AHEAD=$(($n_revisions_ahead + 1)) || \
                                         readonly N_REVISIONS_AHEAD=$n_revisions_ahead
       readonly VERSION_STRING=$MINOR_VERSION_TAG.$(printf '%03d' $N_REVISIONS_AHEAD)
  else readonly N_REVISIONS_AHEAD='n/a'
       readonly SHOULD_INCREMENT_REVISION='n/a'
       readonly VERSION_STRING=$LAST_REVISION_TAG
  fi
  readonly VERSION=${VERSION_STRING//v}


  # commit log messages
  readonly GIT_COMMIT_MSG="update packaging files to $VERSION_STRING"
  readonly OSC_COMMIT_MSG="$VERSION_STRING"

  # github "tag release" params
  readonly GITHUB_RELEASE_JSON="{ \"tag_name\":         \"$VERSION_STRING\" ,
                                  \"target_commitish\": \"\"                ,
                                  \"name\":             \"$VERSION_STRING\" ,
                                  \"body\":             \"\"                ,
                                  \"draft\":            false               ,
                                  \"prerelease\":       true                }"

  # versioned filenames
  readonly TARBALL_FILENAME=$VERSION_STRING.tar.gz
  readonly TARBALL_FILE=../$TARBALL_FILENAME
  readonly OBS_TARBALL_PATH="\/$GITHUB_LOGIN\/$UPSTREAM_NAME\/archive\/$VERSION_STRING\.tar\.gz"
  readonly DEB_PREFIX=${DEBIAN_NAME}_${VERSION}
  readonly DEB_TARBALL_FILENAME=${DEB_PREFIX}.orig.tar.gz
  readonly DEB_DIFFBALL_FILENAME=${DEB_PREFIX}-1.diff.gz
  readonly TARBALL_INNER_DIR=${UPSTREAM_NAME}-${VERSION}
  readonly TARBALL_URL=${tarball_download_url}/${TARBALL_FILENAME}
  readonly TARBALL_SIG_URL=${assets_download_url}/${VERSION_STRING}/${TARBALL_FILENAME}.sig
  readonly PKGBUILD_URL=${assets_download_url}/${VERSION_STRING}/PKGBUILD
  readonly PKGBUILD_SIG_URL=${assets_download_url}/${VERSION_STRING}/PKGBUILD.sig

# source $(git config --local core.hooksPath)/debug-constants.sh.inc # DEBUG
# exit 1 # debug


  # sanity checks
  [ -z "$(which curl 2> /dev/null)"           ] && Quit "$CURL_ERROR_MSG"
  [ "$STAGING_BRANCH" == "$PACKAGING_BRANCH"  ] && Quit "$BRANCHES_ERROR_MSG"
  [ -z "$GIT_USER"                            ] && Quit "$GIT_USER_ERROR_MSG"
  [ -z "$GPG_KEY"                             ] && Quit "$GPG_KEY_ERROR_MSG"
  [ -z "$GITHUB_AUTH_TOKEN"                   ] && Quit "$GITHUB_TOKEN_ERROR_MSG"
  (($IS_PACKAGING_BRANCH)) && [ ! -d $OBS_DIR ] && Quit "$OBS_DIR_ERROR_MSG"
  (($IS_PACKAGING_BRANCH)) && [ ! -d $OSC_DIR ] && Quit "$OSC_DIR_ERROR_MSG"

  # validations
  if   (($IS_STAGING_BRANCH))
  then [ -z "$MINOR_VERSION_TAG" ] && Quit $VERSION_TAG_ERROR_MSG
  elif (($IS_PACKAGING_BRANCH))
  then [ -z "$LAST_REVISION_TAG" ] && Quit $REV_TAG_ERROR_MSG
  else exit 0
  fi
}


init


# files to export
declare -ar DSC_FILES=( "$DSC_FILE"                           \
                        "${DSC_FILE/.dsc/-Debian_8.0.dsc}"    \
                        "${DSC_FILE/.dsc/-xUbuntu_16.04.dsc}" )
declare -ar TEMPLATE_FILES=( "$SERVICE_FILE"  \
                             "$SPEC_FILE"     \
                             ${DSC_FILES[@]}  \
                             "$PKGBUILD_FILE" )
declare -ar DEBIAN_FILES=( "debian/source/format" \
                           "debian/changelog"     \
                           "debian/compat"        \
                           "debian/control"       \
                           "debian/copyright"     \
                           "debian/rules"         )
declare -ar OBS_FILES=( ${TEMPLATE_FILES[@]}        \
                        "$OBS_DIR/debian.changelog" \
                        "$OBS_DIR/debian.compat"    \
                        "$OBS_DIR/debian.control"   \
                        "$OBS_DIR/debian.copyright" \
                        "$OBS_DIR/debian.rules"     )
declare -ar UPLOAD_FILES=( "$TARBALL_FILE.sig"  \
                           "$PKGBUILD_FILE"     \
                           "$PKGBUILD_FILE.sig" )
declare -ar REMOTE_FILENAMES=( "$TARBALL_FILENAME"     \
                               "$TARBALL_FILENAME.sig" \
                               "PKGBUILD"              \
                               "PKGBUILD.sig"          )
declare -ar CLEANUP_FILES=( "$TARBALL_FILE.sig" )
