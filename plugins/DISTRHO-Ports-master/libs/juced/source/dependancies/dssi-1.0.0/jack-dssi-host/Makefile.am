## Process this file with automake to produce Makefile.in

if HAVE_LIBLO
if HAVE_JACK
bin_PROGRAMS = jack-dssi-host
else
bin_PROGRAMS =
endif
else
bin_PROGRAMS =
endif

jack_dssi_host_SOURCES = \
	../dssi/dssi.h \
	jack-dssi-host.c \
	jack-dssi-host.h \
	../message_buffer/message_buffer.c \
	../message_buffer/message_buffer.h

jack_dssi_host_CFLAGS = -I$(top_srcdir)/dssi $(AM_CFLAGS) $(ALSA_CFLAGS) $(LIBLO_CFLAGS) $(JACK_CFLAGS)

if DARWIN
jack_dssi_host_LDADD = $(AM_LDFLAGS) -lmx $(ALSA_LIBS) $(LIBLO_LIBS) $(JACK_LIBS)
else
jack_dssi_host_LDADD = $(AM_LDFLAGS) $(ALSA_LIBS) $(LIBLO_LIBS) $(JACK_LIBS)
endif

