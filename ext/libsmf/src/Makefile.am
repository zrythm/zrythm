include_HEADERS = smf.h

lib_LTLIBRARIES = libsmf.la
libsmf_la_SOURCES = smf.h smf_private.h smf.c smf_decode.c smf_load.c smf_save.c smf_tempo.c
libsmf_la_CFLAGS = $(GLIB_CFLAGS) -DG_LOG_DOMAIN=\"libsmf\"
libsmf_la_LIBADD = $(GLIB_LIBS)

bin_PROGRAMS = smfsh
smfsh_SOURCES = smfsh.c
smfsh_CFLAGS = $(GLIB_CFLAGS) -DG_LOG_DOMAIN=\"smfsh\"
smfsh_LDADD = libsmf.la $(GLIB_LIBS) $(READLINE_LIBS) -lm

