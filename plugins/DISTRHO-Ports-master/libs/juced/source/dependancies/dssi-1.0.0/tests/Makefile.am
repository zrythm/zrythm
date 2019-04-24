## Process this file with automake to produce Makefile.in

TESTS = controller

check_PROGRAMS = controller

controller_SOURCES = controller.c ../dssi/dssi.h

controller_CFLAGS = -Wall -Werror -I$(top_srcdir)/dssi $(ALSA_CFLAGS)

