#!/bin/sh
# This script is used to create PKG files in gitlab CI.
mkdir -p $_PATH
sed -e "s/@BRANCH@/$CI_COMMIT_REF_NAME/" PKGBUILD.in > PKGBUILD
mv $_PACKAGE_NAME.tar.gz PKGBUILD $_PATH
cd $_PATH
makepkg
