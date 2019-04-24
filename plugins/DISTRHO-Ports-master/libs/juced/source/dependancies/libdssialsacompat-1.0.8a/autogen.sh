#!/bin/sh

# WANT_AUTOMAKE=1.7
# export WANT_AUTOMAKE

case `uname -s` in
  Linux)
      LIBTOOLIZE=libtoolize
      ACLOCALARGS=''
      ;;
  Darwin)
      LIBTOOLIZE=glibtoolize
      ACLOCALARGS='-I /usr/local/share/aclocal'
      ;;
  *)  echo error: unrecognized OS
      exit
      ;;
esac

echo "=============== running libtoolize --force --copy" &&
    $LIBTOOLIZE --force --copy &&
    echo "=============== running aclocal" &&
    aclocal $ACLOCALARGS &&
    echo "=============== running automake --add-missing --foreign" &&
    automake --add-missing --foreign &&
    echo "=============== running autoconf" &&
    autoconf &&
    echo "=============== done"

