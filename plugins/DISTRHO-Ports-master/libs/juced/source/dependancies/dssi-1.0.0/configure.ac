dnl Process this file with autoconf to produce a configure script.

AC_INIT(DSSI, 1.0.0, dssi-devel@lists.sourceforge.net)

AC_CONFIG_SRCDIR(dssi/dssi.h)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_CXX
AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_PROG_LIBTOOL
AC_PROG_LN_S

AC_C_BIGENDIAN

dnl Check for LADSPA
AC_CHECK_HEADERS(ladspa.h)

dnl Require ALSA or libdssialsacompat
PKG_CHECK_MODULES(ALSA, alsa >= 0.9, with_alsa=ALSA, with_alsa=no)
if test "x${with_alsa}" = xno ; then
  PKG_CHECK_MODULES(ALSA, libdssialsacompat, with_alsa=libdssialsacompat,
                    AC_MSG_ERROR([neither ALSA nor libdssialsacompat found]))
fi
AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)

dnl Check for liblo
PKG_CHECK_MODULES(LIBLO, liblo >= 0.12, with_liblo=yes, with_liblo=no)
AC_SUBST(LIBLO_CFLAGS)
AC_SUBST(LIBLO_LIBS)
AM_CONDITIONAL(HAVE_LIBLO, test x$with_liblo = xyes)

dnl Check for JACK
PKG_CHECK_MODULES(JACK, jack >= 0.99.14, with_jack=yes, with_jack=no)
AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)
AM_CONDITIONAL(HAVE_JACK, test x$with_jack = xyes)

dnl Check for libsndfile and libsamplerate for trivial_sampler
PKG_CHECK_MODULES(SNDFILE, sndfile, with_sndfile=yes, with_sndfile=no)
PKG_CHECK_MODULES(SRC, samplerate, with_SRC=yes, with_SRC=no)
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)
AC_SUBST(SRC_CFLAGS)
AC_SUBST(SRC_LIBS)

dnl Check for Qt
AC_MSG_CHECKING([whether QTDIR environment variable is set])
if test "x${QTDIR}" = x ; then
  AC_MSG_RESULT([no])
  AC_MSG_CHECKING([for Qt library])
  for qq in /usr/lib/qt3 /usr/qt/3 /usr/share/qt /usr/share/qt3 /usr/lib/qt-3.1 /usr/lib/qt31 /usr/local/qt31 /usr/local/qt3 /usr/lib/qt /usr/local/qt
  do
    if test "x${QTDIR}" = x -a \( -f $qq/lib/libqt-mt.so -o -f $qq/lib/libqt-mt.a \)
    then
      QTDIR=$qq
    fi
  done
  if test "x${QTDIR}" = x ; then
    with_qt=no
    AC_MSG_RESULT([none found. Perhaps you need to set QTDIR?])
  else
    with_qt=yes
    AC_MSG_RESULT([found one in $QTDIR/lib])
  fi
else
  AC_MSG_RESULT([$QTDIR])
  AC_MSG_CHECKING([for Qt library])
  if test -f $QTDIR/lib/libqt-mt.so -o -f $QTDIR/lib/libqt-mt.a; then
    with_qt=yes
    AC_MSG_RESULT([in $QTDIR/lib])
  else
    with_qt=no
    AC_MSG_RESULT([no. Perhaps QTDIR is not set correctly?])
  fi
fi
if test "x${with_qt}" = xyes ; then
  if test "x${QTINCDIR}" != x ; then
    QT_CXXFLAGS="-I${QTINCDIR}"
  else
    QT_CXXFLAGS="-I${QTDIR}/include"
  fi
  if test "x${QTLIBDIR}" != x ; then
    QT_LIBS="-L${QTLIBDIR} -lqt-mt"
  else
    QT_LIBS="-L${QTDIR}/lib -lqt-mt"
  fi
  if test "x${MOC}" != x ; then
    QT_MOC="${MOC}"
  else
    QT_MOC="${QTDIR}/bin/moc"
  fi
  # dnl Check for Qt moc utility.
  # AC_PATH_PROG(ac_moc, moc, [no])
  # if test "x$ac_moc" = "xno"; then
  #    AC_MSG_ERROR([moc $ac_qtdir_errmsg])
  # fi
  # AC_SUBST(ac_moc)
fi
AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(QT_MOC)
AM_CONDITIONAL(HAVE_QT, test x$with_qt = xyes)

dnl OS specific checks
case "${host_os}" in
darwin*)
  darwin=yes
  AC_DEFINE(MIDI_COREMIDI, 1, [Define for CoreMIDI MIDI support])
  ;;
*)
  darwin=no
  if test "x${with_alsa}" = xALSA ; then
    AC_DEFINE(MIDI_ALSA, 1, [Define for ALSA MIDI support])
  fi
  ;;
esac
AM_CONDITIONAL(DARWIN, test x$darwin = xyes)

dnl Set GCC warning flags
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
    *[\ \	]-Wall[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

jack_dssi_host_needs=''
dssi_osc_tools_needs=''
trivial_sampler_needs=''
less_trivial_synth_gui_needs=''
trivial_sampler_gui_needs=''
if test "x$with_liblo" != xyes ; then
    jack_dssi_host_needs="$jack_dssi_host_needs liblo"
    dssi_osc_tools_needs="$dssi_osc_tools_needs liblo"
    less_trivial_synth_gui_needs="$less_trivial_synth_gui_needs liblo"
    trivial_sampler_gui_needs="$trivial_sampler_gui_needs liblo"
fi
if test "x$with_jack" != xyes ; then
    jack_dssi_host_needs="$jack_dssi_host_needs JACK"
fi
if test "x$with_sndfile" != xyes ; then
    trivial_sampler_needs="$trivial_sampler_needs libsndfile"
    trivial_sampler_gui_needs="$trivial_sampler_gui_needs libsndfile"
fi
if test "x$with_SRC" != xyes ; then
    trivial_sampler_needs="$trivial_sampler_needs libsamplerate"
fi
AM_CONDITIONAL(BUILD_SAMPLER, test -z "$trivial_sampler_needs")
if test "x$with_qt" != xyes ; then
    less_trivial_synth_gui_needs="$less_trivial_synth_gui_needs Qt"
    trivial_sampler_gui_needs="$trivial_sampler_gui_needs Qt"
fi

AC_OUTPUT([
    Makefile
    dssi.pc
    jack-dssi-host/Makefile
    examples/Makefile
    tests/Makefile
])

dnl --- Below here is just pretty output for the user ---

echo "* ======== DSSI ${PACKAGE_VERSION} configured ========"

if test -z "$jack_dssi_host_needs" ; then
    echo "* building jack-dssi-host:            yes"
else
    echo "* building jack-dssi-host:            no (missing$jack_dssi_host_needs)"
fi
if test -z "$dssi_osc_tools_needs" ; then
    echo "* building DSSI OSC tools:            yes"
else
    echo "* building DSSI OSC tools:            no (missing$dssi_osc_tools_needs)"
fi
echo "* building trivial_synth plugin:      yes"
echo "* building less_trivial_synth plugin: yes"
echo "* building karplong plugin:           yes"
if test -z "$trivial_sampler_needs" ; then
    echo "* building trivial_sampler plugin:    yes"
else
    echo "* building trivial_sampler plugin:    no (missing$trivial_sampler_needs)"
fi
if test -z "$less_trivial_synth_gui_needs" ; then
    echo "* building less_trivial_synth GUI:    yes"
else
    echo "* building less_trivial_synth GUI:    no (missing$less_trivial_synth_gui_needs)"
fi
if test -z "$trivial_sampler_gui_needs" ; then
    echo "* building trivial_sampler GUI:       yes"
else
    echo "* building trivial_sampler GUI:       no (missing$trivial_sampler_gui_needs)"
fi
