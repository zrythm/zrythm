#!/bin/sh
mkdir $PACKAGE_NAME
mv autogen.sh debian config.guess config.sub \
  configure.ac CONTRIBUTING data Doxyfile inc INSTALL \
  install-sh LICENSE Makefile.in README.md resources src \
  $PACKAGE_NAME/
tar -zcvf $PACKAGE_NAME.tar.gz $PACKAGE_NAME
