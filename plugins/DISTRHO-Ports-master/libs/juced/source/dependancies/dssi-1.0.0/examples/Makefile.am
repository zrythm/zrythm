## Process this file with automake to produce Makefile.in

if HAVE_LIBLO
bin_PROGRAMS = dssi_osc_send dssi_osc_update
else
bin_PROGRAMS =
endif

dssi_osc_send_SOURCES = dssi_osc_send.c
dssi_osc_send_CFLAGS = -I$(top_srcdir)/dssi $(AM_CFLAGS) $(LIBLO_CFLAGS)
dssi_osc_send_LDADD = $(AM_LDFLAGS) $(LIBLO_LIBS)

dssi_osc_update_SOURCES = dssi_osc_update.c
dssi_osc_update_CFLAGS = -I$(top_srcdir)/dssi $(AM_CFLAGS) $(LIBLO_CFLAGS)
dssi_osc_update_LDADD = $(AM_LDFLAGS) $(LIBLO_LIBS)

plugindir = $(libdir)/dssi

if BUILD_SAMPLER
plugin_LTLIBRARIES = trivial_synth.la less_trivial_synth.la trivial_sampler.la karplong.la
else
plugin_LTLIBRARIES = trivial_synth.la less_trivial_synth.la karplong.la
endif

trivial_synth_la_SOURCES = \
	trivial_synth.c \
	../dssi/dssi.h

less_trivial_synth_la_SOURCES = \
        less_trivial_synth.c \
	saw.h \
	../dssi/dssi.h

trivial_sampler_la_SOURCES = \
	trivial_sampler.c \
	trivial_sampler.h \
	../dssi/dssi.h

karplong_la_SOURCES = \
	karplong.cpp \
	../dssi/dssi.h

trivial_synth_la_CFLAGS = -I$(top_srcdir)/dssi $(AM_CFLAGS) $(ALSA_CFLAGS)
less_trivial_synth_la_CFLAGS = -I$(top_srcdir)/dssi $(AM_CFLAGS) $(ALSA_CFLAGS)
trivial_sampler_la_CFLAGS = -I$(top_srcdir)/dssi $(AM_CFLAGS) $(ALSA_CFLAGS) $(SNDFILE_CFLAGS) $(SRC_CFLAGS)
karplong_la_CXXFLAGS = -I$(top_srcdir)/dssi $(AM_CFLAGS) $(ALSA_CFLAGS)

trivial_synth_la_LDFLAGS = -module -avoid-version
less_trivial_synth_la_LDFLAGS = -module -avoid-version
trivial_sampler_la_LDFLAGS = -module -avoid-version
karplong_la_LDFLAGS = -module -avoid-version
if DARWIN
less_trivial_synth_la_LIBADD = -lm -lmx
else
less_trivial_synth_la_LIBADD = -lm
endif

trivial_sampler_la_LIBADD = -lpthread $(SNDFILE_LIBS) $(SRC_LIBS)

if HAVE_LIBLO
if HAVE_QT
lts_ui_PROGRAMS = LTS_qt
if BUILD_SAMPLER
trivial_sampler_ui_PROGRAMS = trivial_sampler_qt
else
trivial_sampler_ui_PROGRAMS =
endif
else
lts_ui_PROGRAMS =
trivial_sampler_ui_PROGRAMS =
endif
else
lts_ui_PROGRAMS =
trivial_sampler_ui_PROGRAMS =
endif

lts_uidir = $(libdir)/dssi/less_trivial_synth

LTS_MOC = less_trivial_synth_qt_gui.moc.cpp

LTS_qt_SOURCES = \
	less_trivial_synth_qt_gui.cpp \
	less_trivial_synth_qt_gui.h

nodist_LTS_qt_SOURCES = $(LTS_MOC)

LTS_qt_CXXFLAGS = $(AM_CXXFLAGS) $(QT_CXXFLAGS) $(LIBLO_CFLAGS)
LTS_qt_LDADD = $(AM_LDFLAGS) $(QT_LIBS) $(LIBLO_LIBS)

trivial_sampler_uidir = $(libdir)/dssi/trivial_sampler

trivial_sampler_MOC = trivial_sampler_qt_gui.moc.cpp

trivial_sampler_qt_SOURCES = \
	trivial_sampler_qt_gui.cpp \
	trivial_sampler_qt_gui.h \
	../dssi/dssi.h

nodist_trivial_sampler_qt_SOURCES = $(trivial_sampler_MOC)

trivial_sampler_qt_CXXFLAGS = $(AM_CXXFLAGS) -I$(top_srcdir)/dssi $(ALSA_CFLAGS) $(QT_CXXFLAGS) $(LIBLO_CFLAGS) $(SNDFILE_CFLAGS)
trivial_sampler_qt_LDADD = $(AM_LDFLAGS) $(QT_LIBS) $(LIBLO_LIBS) $(SNDFILE_LIBS)

%.moc.cpp: %.h
	$(QT_MOC) -o $@ $<

if HAVE_QT
BUILT_SOURCES = $(LTS_MOC) $(trivial_sampler_MOC)
else
BUILT_SOURCES =
endif

CLEANFILES = $(BUILT_SOURCES)

# create symlinks for each plugin to jack-dssi-host
install-exec-hook:
	if test -x $(DESTDIR)$(bindir)/jack-dssi-host$(EXEEXT) ; then \
	  cd $(DESTDIR)$(bindir) ; \
	  if ! test -x ./trivial_synth$(EXEEXT) ; then \
	    $(LN_S) jack-dssi-host$(EXEEXT) trivial_synth$(EXEEXT) ; \
	  fi ; \
	  if ! test -x ./less_trivial_synth$(EXEEXT) ; then \
	    $(LN_S) jack-dssi-host$(EXEEXT) less_trivial_synth$(EXEEXT) ; \
	  fi ; \
	  if ! test -x ./trivial_sampler$(EXEEXT) ; then \
	    $(LN_S) jack-dssi-host$(EXEEXT) trivial_sampler$(EXEEXT) ; \
	  fi ; \
	  if ! test -x ./karplong$(EXEEXT) ; then \
	    $(LN_S) jack-dssi-host$(EXEEXT) karplong$(EXEEXT) ; \
	  fi ; \
	fi

