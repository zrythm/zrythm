#!/bin/sh
cd
tar xvzf $PACKAGE_NAME.tar.gz
cp $PACKAGE_NAME/zrythm.spec rpmbuild/SPECS/
cp $PACKAGE_NAME.tar.gz rpmbuild/SOURCES/
cd rpmbuild/SPECS/
rpmbuild -ba zrythm.spec
