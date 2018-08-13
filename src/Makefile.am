# We are building some programs to be installed in bin dir
# There is only one program to build, zrythm
#EXTRA_DIST = resources.c
bin_PROGRAMS = zrythm

AM_CFLAGS = -rdynamic -I$(top_srcdir)/inc $(GTK_CFLAGS)

# sources for zrythm
zrythm_SOURCES = main.c init.c gui/main_window.c
nodist_zrythm_SOURCES = resources.c
zrythm.$(OBJEXT): resources.c # manually declare dependency

BUILT_SOURCES = resources.c

zrythm_LDADD = $(GTK_LIBS)

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
