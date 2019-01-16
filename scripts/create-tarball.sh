#!/bin/sh
mkdir $PACKAGE_NAME
mv autogen.sh debian ext config.guess config.sub \
  configure.ac CONTRIBUTING.md data Doxyfile inc INSTALL.md \
  install-sh LICENSE Makefile.in README.md resources src \
  $PACKAGE_NAME/
tar -zcvf $PACKAGE_NAME.tar.gz $PACKAGE_NAME
