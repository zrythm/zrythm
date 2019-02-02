AM_CFLAGS = \
	-g -Wall -O2 \
	-I$(top_srcdir) \
	$(SNDFILE_CFLAGS) \
	$(FFMPEG_CFLAGS)

noinst_LIBRARIES = libad.a

libad_a_SOURCES = \
	ad_ffmpeg.c \
	ad.h \
	ad_plugin.c \
	ad_plugin.h \
	ad_soundfile.c \
	ffcompat.h
