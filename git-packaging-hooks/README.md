Git Packaging Hooks
===================

these files are a set of git hooks to semi-automate the following:

  * injecting semantic version strings into the program source code
  * creating a release tag
  * creating a signed release tarball

sources include graph:
```
* commit-msg
  -> common.sh.inc
     -> project-defs.sh.inc
  -> staging-pre.sh.inc
  -> packaging-pre.sh.inc
* post-commit
  -> common.sh.inc
     -> project-defs.sh.inc
  -> staging-post.sh.inc
  -> packaging-post.sh.inc
```


initial local configuration:

* configure git to use these git-packaging-hooks
  => $ git config --local core.hooksPath /path/to/git-packaging-hooks/
* define the project-specific constants in project-defs.sh.inc
* ensure that `git config user.name` and `git config user.signingkey` are set
* ensure that `curl` and `piuparts` are installed
  => $ su -c "apt-get install curl piuparts"


for each release version:

* checkout the `$STAGING_BRANCH` and rebase the release state from master
* ensure that the appropriate tag 'vMAJOR.MINOR' (or version excluding patch) exists on the `$STAGING_BRANCH`
  or add a new tag 'vMAJOR.MINOR' if major or minor version should change
* amend the `$STAGING_BRANCH` HEAD commit to trigger the git hooks
* verify that the git hook has put a tag of the form 'vMAJOR.MINOR.REV' on the HEAD (unless rc)
  where REV is n_commits after the nearest 'vMAJOR.MINOR' tag


_NOTE: after each commit to the `$STAGING_BRANCH`:_

* the version string will be written into the meson.build and CHANGELOG.md files
* any git tags of the form 'vMAJOR.MINOR.REV' that are not merged into master will be deleted
* a git tag 'vMAJOR.MINOR.REV' will be put on the HEAD
  where REV is n_commits after the nearest 'vMAJOR.MINOR' tag
* meson/ninja will be run, the project will be rebuilt and installchecked
* the commit will be rejected if the re-config/re-build fails
* the commit will be amended with '--gpg-sign' if it was not already GPG signed
  or if any tracked files had been changed by the re-config/re-build
* a release tarball will be generated in build/meson-dist


_NOTE: after each commit to the `$PACKAGING_BRANCH`:_


finalize the release:

* checkout the master branch and merge the `$STAGING_BRANCH`
  => $ git checkout master && git merge `$STAGING_BRANCH` && git push upstream master
* push the newly created tag
  => $ git push --tags
