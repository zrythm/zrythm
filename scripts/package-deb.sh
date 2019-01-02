#!/bin/sh
mkdir -p $_PATH
cp $PACKAGE_NAME.tar.gz $_PATH/$PACKAGE_NAME.orig.tar.gz
tar xf $PACKAGE_NAME.tar.gz --directory $_PATH/
cd $_PATH/$PACKAGE_NAME
debuild -us -uc
