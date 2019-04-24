#!/bin/sh

# This works on Gentoo, whose automake wrapper is based on Mandrake:
WANT_AUTOMAKE=1.7
export WANT_AUTOMAKE


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
      exit 1
      ;;
esac

AUTOMAKE_REQ=1.7

# Automake version check from MusE
lessthan () {
  ver1="$1"
  ver2="$2"

  major1=$( echo $ver1 | sed "s/^\([0-9]*\)\..*/\1/");
  minor1=$( echo $ver1 | sed "s/^[^\.]*\.\([0-9]*\).*/\1/" );
  major2=$( echo $ver2 | sed "s/^\([0-9]*\)\..*/\1/");
  minor2=$( echo $ver2 | sed "s/^[^\.]*\.\([0-9]*\).*/\1/" );
  test "$major1" -lt "$major2" || test "$minor1" -lt "$minor2";
}

amver=$( automake --version | head -n 1 | sed "s/.* //" );
if lessthan $amver $AUTOMAKE_REQ ; then
    echo "you must have automake version >= $AUTOMAKE_REQ to proper plugin support"
    exit 1
fi


echo "=============== running libtoolize --force --copy" &&
    $LIBTOOLIZE --force --copy &&
    echo "=============== running aclocal" &&
    aclocal $ACLOCALARGS &&
    echo "=============== running autoheader" &&
    autoheader &&
    echo "=============== running automake --add-missing --foreign" &&
    automake --add-missing --foreign &&
    echo "=============== running autoconf" &&
    autoconf &&
    echo "=============== done"

