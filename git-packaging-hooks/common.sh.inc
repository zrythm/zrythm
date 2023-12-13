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
#
# SPDX-License-Identifier: GPL-3.0-or-later

# trace helpers
function TraceStage() { local message=$* ; echo -e "HOOKS: $message" ;                  }
function TraceStep()  { local message=$* ; echo -e "         -> $message" ;             }
function TraceError() { local message=$* ; echo -e "\033[0;31mERROR: $message\033[0m" ; }
function Quit()       { local message=$* ; TraceError $message ; exit 1 ; }

# project-specific constants
readonly PROJECT_DEFS_FILE=$(git config --local core.hooksPath)/project-defs.sh.inc
source $PROJECT_DEFS_FILE
[ ! -f $PROJECT_DEFS_FILE ] && Quit "missing $PROJECT_DEFS_FILE - aborting commit"
[ -z "$UPSTREAM_NAME" -o \
  -z "$REMOTE_NAME"   -o \
  -z "$STAGING_BRANCH" -o -z "$PACKAGING_BRANCH" -o -z "$N_MAKE_JOBS"      ] && \
  Quit "$PROJECT_DEFS_FILE incomplete - aborting commit"

  #[ $(echo $DEB_BUILD_TOOL | grep -E "^(debuild|sbuild|gbp)$") ] || \
  #Quit "invalid \$DEB_BUILD_TOOL '$DEB_BUILD_TOOL' - aborting commit"


function init()
{
  readonly PROJECT_DIR=${PWD}

  # files to modify
  readonly VERSION_FILE=VERSION.txt
  readonly CHANGELOG_FILE=CHANGELOG.md

  # detect commit to staging or packaging branch
  CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
  [ "$CURRENT_BRANCH" == "$STAGING_BRANCH"   ] && readonly IS_STAGING_BRANCH=1   || \
                                                  readonly IS_STAGING_BRANCH=0
  [ "$CURRENT_BRANCH" == "$PACKAGING_BRANCH" ] && readonly IS_PACKAGING_BRANCH=1 || \
                                                  readonly IS_PACKAGING_BRANCH=0

  # detect empty commit message
  readonly GIT_COMMIT_MSG_FILE=.git/COMMIT_EDITMSG
  [ "$(grep --invert-match "^\s*#" $GIT_COMMIT_MSG_FILE)" ] && IS_EMPTY_COMMIT_MSG=0 || \
                                                               IS_EMPTY_COMMIT_MSG=1

  # detect commit message
  commit_msg=`cat "$GIT_COMMIT_MSG_FILE"`

  # misc string constants
  readonly PRE_ALPHA_REGEX='^v[0-9]+\.[0-9]+\.[0-9]+-DEV\.[0-9]+\.[0-9]+$'
  readonly ALPHA_REGEX='^v[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+\.[0-9]+$'
  readonly BETA_REGEX='^v[0-9]+\.[0-9]+\.[0-9]+-beta\.[0-9]+\.[0-9]+$'
  readonly RC_REGEX='^v[0-9]+\.[0-9]+\.[0-9]+-rc\.[0-9]+$'
  readonly STABLE_REGEX='^v[0-9]+\.[0-9]+\.[0-9]+$'
  case "$commit_msg" in
    "release-pre-alpha:"*)
      readonly MINOR_VERSION_REGEX="$PRE_ALPHA_REGEX"
      readonly IS_RC=0
      ;;
    "release-alpha:"*)
      readonly MINOR_VERSION_REGEX="$ALPHA_REGEX"
      readonly IS_RC=0
      ;;
    "release-beta:"*)
      readonly MINOR_VERSION_REGEX="$BETA_REGEX"
      readonly IS_RC=0
      ;;
    "release-rc:"*)
      readonly MINOR_VERSION_REGEX="$RC_REGEX"
      readonly IS_RC=1
      ;;
    "release-stable:"*)
      readonly MINOR_VERSION_REGEX="$STABLE_REGEX"
      readonly IS_RC=0
      ;;
    *)
      readonly MINOR_VERSION_REGEX="$STABLE_REGEX"
      readonly IS_RC=0
      if (($IS_STAGING_BRANCH)); then
        Quit 'missing "release-*:" prefix in commit message - aborting commit'
      fi
      ;;
  esac
  readonly REVISION_REGEX="$MINOR_VERSION_REGEX"'\.[0-9]+$'
  readonly GIT_USER=$(git config user.name      )
  readonly GPG_KEY=$( git config user.signingkey)
  readonly CURL_ERROR_MSG="could not find \`curl\` executable"
  readonly BRANCHES_ERROR_MSG="\$STAGING_BRANCH and \$PACKAGING_BRANCH can not be the same - aborting commit"
  readonly GIT_USER_ERROR_MSG="git config user.name not set - aborting commit"
  readonly GPG_KEY_ERROR_MSG="git config user.signingkey not set - aborting commit"
  readonly VERSION_TAG_ERROR_MSG="could not locate a minor version tag of the form: vMAJOR.MINOR"
  readonly REV_TAG_ERROR_MSG="could not locate a revision tag of the form: vMAJOR.MINOR.REV"
  readonly TAGS_FILE=$(git config --local core.hooksPath)/RESTORE_TAGS
  readonly CURL_RESP_ARGS='--silent'
  readonly CURL_FETCH_ARGS='--silent --location --remote-name'
  readonly CURL_STATUS_ARGS='--silent --output /dev/null --write-out %{http_code}'


  # detect amend commit
  [ "$(ps --pid $PPID --format command= | grep '\-\-amend')" ] && readonly IS_AMEND_COMMIT=1 || \
                                                                  readonly IS_AMEND_COMMIT=0

  # differentiate pre/post commit stages
  [ "$*" == '.git/COMMIT_EDITMSG' ] && readonly IS_PRE_COMMIT_STAGE=1 || \
                                       readonly IS_PRE_COMMIT_STAGE=0

  # get existing revision tags
  local ifs=$IFS ; IFS=' ' ;
  local branch_minor_tags=$(git tag --list --merged    HEAD   | grep -E $MINOR_VERSION_REGEX)
  local branch_rev_tags=$(  git tag --list --merged    HEAD   | grep -E $REVISION_REGEX     )
  local unmerged_rev_tags=$(git tag --list --no-merged master | grep -E $REVISION_REGEX     )
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
       if (($IS_RC)); then
         readonly VERSION_STRING=$MINOR_VERSION_TAG
       else
         readonly VERSION_STRING=$MINOR_VERSION_TAG.$(printf '%d' $N_REVISIONS_AHEAD)
       fi
  else readonly N_REVISIONS_AHEAD='n/a'
       readonly SHOULD_INCREMENT_REVISION='n/a'
       readonly VERSION_STRING=$LAST_REVISION_TAG
  fi
  readonly VERSION=${VERSION_STRING//v}


  # commit log messages
  readonly GIT_COMMIT_MSG="update packaging files to $VERSION_STRING"


  # versioned filenames
  readonly TARBALL_FILENAME=$VERSION_STRING.tar.gz
  readonly TARBALL_FILE=$TARBALL_FILENAME
  readonly TARBALL_INNER_DIR=${UPSTREAM_NAME}-v${VERSION}
  readonly TARBALL_URL=${tarball_download_url}/${TARBALL_FILENAME}

# source $(git config --local core.hooksPath)/debug-constants.sh.inc # DEBUG
# exit 1 # debug


  # sanity checks
  [ -z "$(which curl 2> /dev/null)"           ] && Quit "$CURL_ERROR_MSG"
  [ "$STAGING_BRANCH" == "$PACKAGING_BRANCH"  ] && Quit "$BRANCHES_ERROR_MSG"
  [ -z "$GIT_USER"                            ] && Quit "$GIT_USER_ERROR_MSG"
  [ -z "$GPG_KEY"                             ] && Quit "$GPG_KEY_ERROR_MSG"

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
declare -ar UPLOAD_FILES=(
 "$TARBALL_FILE.sig"  \
                           "$PKGBUILD_FILE"     \
                           "$PKGBUILD_FILE.sig" )
declare -ar REMOTE_FILENAMES=("$TARBALL_FILENAME"  \
                              "$TARBALL_FILENAME.sig")
declare -ar CLEANUP_FILES=( "$TARBALL_FILE.sig" )
