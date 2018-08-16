# We are building some programs to be installed in bin dir
# There is only one program to build, zrythm
#EXTRA_DIST = resources.c
bin_PROGRAMS = zrythm

# sources for zrythm
zrythm_SOURCES = main.c \
                 init.c \
				 project.c \
				 audio/engine.c \
				 gui/main_window.c \
				 gui/widgets/instrument_timeline_view.c \
				 gui/widgets/ruler.c
nodist_zrythm_SOURCES = resources.c
zrythm.$(OBJEXT): resources.c # manually declare dependency

BUILT_SOURCES = resources.c

AM_CFLAGS = -rdynamic -I$(top_srcdir)/inc \
			$(GTK_CFLAGS) \
			$(DAZZLE_CFLAGS) \
			$(JACK_CFLAGS) \
			$(LV2_CFLAGS)
zrythm_LDADD = $(GTK_LIBS) \
			   $(DAZZLE_LIBS) \
			   $(JACK_LIBS) \
			   $(LV2_LIBS) \
			   -lm

# resources is a built source that should be cleaned
CLEANFILES = resources.c

# desktop file
desktopdir = $(datadir)/applications
desktop_DATA = $(top_srcdir)/resources/zrythm.desktop

#the application icon
appicondir=$(datadir)/icons/hicolor/scalable/apps
appicon_DATA = $(top_srcdir)/resources/z.svg

# build rule for resources.c
resources.c:
	glib-compile-resources $(srcdir)/zrythm.gresource.xml --target=resources.c \
	--generate-source --sourcedir=${top_srcdir}/resources
