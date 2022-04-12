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


echo -e "\n==== DEBUG BEGIN ===="

[ "$IS_AMEND_COMMIT"           == "1" ] && echo "!! IS_AMEND_COMMIT"           || echo "!  IS_AMEND_COMMIT"
[ "$IS_PRE_COMMIT_STAGE"       == "1" ] && echo "!! IS_PRE_COMMIT_STAGE"       || echo "!  IS_PRE_COMMIT_STAGE"
[ "$IS_STAGING_BRANCH"         == "1" ] && echo "!! IS_STAGING_BRANCH"         || echo "!  IS_STAGING_BRANCH"
[ "$IS_PACKAGING_BRANCH"       == "1" ] && echo "!! IS_PACKAGING_BRANCH"       || echo "!  IS_PACKAGING_BRANCH"
[ "$SHOULD_INCREMENT_REVISION" == "1" ] && echo "!! SHOULD_INCREMENT_REVISION" || echo "!  SHOULD_INCREMENT_REVISION"
# echo "branch_tags=$branch_tags"
# echo "branch_minor_tags=$branch_minor_tags"
# echo "branch_rev_tags=$branch_rev_tags"
# echo "unmerged_tags=$unmerged_tags"
# echo "unmerged_rev_tags=$unmerged_rev_tags"
echo "HEAD_REF          = $HEAD_REF"
echo "MINOR_VERSION_REF = $MINOR_VERSION_REF"
echo "LAST_REVISION_REF = $LAST_REVISION_REF"
echo "MINOR_VERSION_TAG = $MINOR_VERSION_TAG"
echo "LAST_REVISION_TAG = $LAST_REVISION_TAG"
echo "HIGHEST_REV_TAG   = $HIGHEST_REV_TAG"
echo "N_REVISIONS_AHEAD = $N_REVISIONS_AHEAD # (upon completion)"
echo "VERSION_STRING    = $VERSION_STRING"

[ -f $TAGS_FILE ] && echo "\$TAGS_FILE exists - orphaned tags to restore: '$(cat $TAGS_FILE | sort | uniq | tr "\n" " ")'"

echo -e "==== DEBUG END ====\n"
