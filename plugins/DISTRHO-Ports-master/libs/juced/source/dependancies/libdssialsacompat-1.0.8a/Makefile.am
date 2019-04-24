## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	autogen.sh

AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libdssialsacompat.la

libdssialsacompat_la_SOURCES = dssi_alsa_compat.c

libdssialsacompat_la_LDFLAGS = -version-info 0:0:0

library_includedir=$(includedir)/dssi

nobase_library_include_HEADERS = \
	alsa/asoundef.h \
	alsa/asoundlib.h \
	alsa/seq.h \
	alsa/seq_event.h \
	alsa/seq_midi_event.h \
	alsa/sound/asequencer.h

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libdssialsacompat.pc

