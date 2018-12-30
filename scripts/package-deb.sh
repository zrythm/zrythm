#!/bin/sh
mkdir -p $_PATH
cp $_PACKAGE_NAME.tar.gz $_PATH/zrythm_0.1.orig.tar.gz
tar xf $_PACKAGE_NAME.tar.gz --directory $_PATH/
cd $_PATH/$_PACKAGE_NAME
sed -e "s/@BRANCH@/0.1/" debian/changelog.in > debian/changelog
rm debian/changelog.in
debuild -us -uc
