
these files are a set of git hooks to semi-automate the following:

  * injecting semantic version strings into the program source code
  * signed releases on github
  * packaging for debian
  * packaging on the opensuse build service (OBS)

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
     -> github-post.sh.inc
     -> obs-post.sh.inc
     -> debian-post.sh.inc
```


initial local configuration:

* configure git to use these git-packaging-hooks  
  => $ git config --local core.hooksPath /path/to/git-packaging-hooks/
* define the project-specific constants in project-defs.sh.inc
* ensure that the project debian/ and obs/ directories are on the `$PACKAGING_BRANCH` only
* copy (or adapt) the gbp.conf from this directory to the project debian/ directory
* ensure that `git config user.name` and `git config user.signingkey` are set
* ensure that `curl` and `piuparts` are installed  
  => $ su -c "apt-get install curl piuparts"
* for DEB_BUILD_TOOL='debuild' ensure that `devscripts` is installed  
  => $ su -c "apt-get install devscripts"
* for DEB_BUILD_TOOL='sbuild' ensure that `sbuild` and `schroot` are installed  
  and that your user is in the 'sbuild' group  
  => $ su -c "apt-get install sbuild schroot"  
  => $ su -c "sbuild-adduser $USER"  
  => (optional) set alternate chroot directory  
  => $ su -c "mkdir -p ${$DEB_SBUILD_DIR}  
  => $ su -c "rm -rf /run/schroot"  
  => $ su -c "ln -s $DEB_SBUILD_DIR /run/schroot"
* for DEB_BUILD_TOOL='gbp' ensure that `git-buildpackage` is installed  
  => ensure that your user can sudo  
  => $ sudo apt-get install debhelper fakeroot git-buildpackage cowbuilder  
  => (optional) set alternate chroot directory  
  => $ sudo mkdir -p $DEB_PBUILDER_DIR  
  => $ sudo rm -rf /var/cache/pbuilder  
  => $ sudo ln -s $DEB_PBUILDER_DIR /var/cache/pbuilder


for each release version:

* checkout the `$STAGING_BRANCH` and pull/merge the release state
* ensure that the appropriate tag 'vMAJOR.MINOR' exists on the `$STAGING_BRANCH`  
  or add a new tag 'vMAJOR.MINOR' if major or minor version should change
* amend the `$STAGING_BRANCH` HEAD commit to trigger the git hooks
* verify that the git hook has put a tag of the form 'vMAJOR.MINOR.REV' on the HEAD  
  where REV is n_commits after the nearest 'vMAJOR.MINOR' tag
* checkout the `$PACKAGING_BRANCH` to enable the packaging-specific git hooks
* rebase the `$PACKAGING_BRANCH` onto the previous HEAD
* (optional) in debian/changelog  
  => add detailed entry for this version to suppress the automated entry


if build or install steps have changed:

* in `$OBS_NAME`.spec.in  
  => update '%build' recipe, and/or '%post' install hooks
* in debian.rules  
  => update 'build-stamp:' and 'install:' recipes
* in PKGBUILD.in  
  => update 'build()' and 'package()' recipes  
  => $ gpg --detach-sign PKGBUILD


if output files have changed:

* in `$OBS_NAME`.spec.in  
  => update package '%files'


if dependencies have changed:

* in `$OBS_NAME`.spec.in  
  => update 'BuildRequires' and/or 'Requires'
* in `$OBS_NAME`.dsc.in  
  => update 'Build-Depends'
* in debian.control  
  => update 'Build-Depends' and/or 'Depends'
* in PKGBUILD.in  
  => update 'makedepends' and/or 'depends'  
  => $ gpg --detach-sign PKGBUILD


prepare packaging files:

* add a new commit to trigger the git hooks  
  => $ git add --all  
  => $ git commit --allow-empty --allow-empty-message --message=''


_NOTE: after each commit to the `$STAGING_BRANCH`:_

* the version string will be written into the configure.ac file
* any git tags of the form 'vMAJOR.MINOR.REV' that are not merged into master will be deleted
* a git tag 'vMAJOR.MINOR.REV' will be put on the HEAD  
  where REV is n_commits after the nearest 'vMAJOR.MINOR' tag
* autoreconf will be run, the project will be rebuilt and installchecked
* the commit will be rejected if the re-config/re-build fails
* the commit will be amended with '--gpg-sign' if it was not already GPG signed  
  or if any tracked files had been changed by the re-config/re-build


_NOTE: after each commit to the `$PACKAGING_BRANCH`:_

* all existing git tags are preserved
* rebasing and amend commits are non-eventful and will not trigger any of the following actions
* the \_service, .spec, .dsc, and PKGBUILD files will be created from their corresponding *.in templates
* version strings will be written into the \_service, .spec, .dsc, and PKGBUILD files
* checksums will be written into the .dsc and PKGBUILD files
* a tarball named 'PROJECT_MAJOR.MINOR.REV.orig.tar.gz' will be in the parent directory
* signatures will be generated for the tarball and PKGBUILD
* the .spec and .dsc recipes will be coupled to this tarball
* the .dsc recipe will be coupled to this tarball checksums
* the PKGBUILD recipe will be coupled to this tarball, checksum, and signatures
* a new entry will be added to the debian/changelog if one does not exist for this version
* the new HEAD commit will be amended and signed with the commit message as `$GIT_COMMIT_MSG`
* the `$STAGING_BRANCH`, `$PACKAGING_BRANCH`, and tags will pushed to github
* the PKGBUILD and signatures will be uploaded to the 'vMAJOR.MINOR.REV' github "tag release"
* the local tarball, PKGBUILD, and signatures will be verified as identical to the github "tag release"
* all files in the ./obs/ directory (except *.in) will be copied into the `$OSC_DIR`
* the remote OBS build will be triggered with a checkin
* a debian package will be built, installed, and validated in a clean chroot


finalize the release:

* tweak any debian/ or OSC files and rebuild manually with deb tools and/or osc  
  as necessary until everything is rocking sweetly
* duplicate any manual changes to the OSC files in the ./obs/ files and amend commit  
  => $ git add --all  
  => $ git commit --amend
* checkout the master branch and fast-forward to the `$STAGING_BRANCH`  
  => $ git checkout master && git merge `$STAGING_BRANCH` && git push upstream master
