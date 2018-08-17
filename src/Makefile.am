bin_PROGRAMS = zrythm

# sources for zrythm
zrythm_SOURCES = main.c \
                 init.c \
								 project.c \
								 settings.c \
								 audio/engine.c \
								 audio/timeline.c \
								 audio/region.c \
								 gui/main_window.c \
								 gui/widgets/instrument_timeline_view.c \
								 gui/widgets/ruler.c \
								 gui/widgets/timeline.c \
								 plugins/plugin_manager.c

AM_CFLAGS = -rdynamic -I$(top_srcdir)/inc \
			$(GTK_CFLAGS) \
			$(DAZZLE_CFLAGS) \
			$(JACK_CFLAGS) \
			$(LV2_CFLAGS) \
			$(LILV_CFLAGS)
zrythm_LDADD = $(GTK_LIBS) \
			   $(DAZZLE_LIBS) \
			   $(JACK_LIBS) \
			   $(LV2_LIBS) \
			   $(LILV_LIBS) \
			   -lm

nodist_zrythm_SOURCES = resources.c
zrythm.$(OBJEXT): resources.c # manually declare dependency

BUILT_SOURCES = resources.c

# resources is a built source that should be cleaned
CLEANFILES = resources.c

# build rule for resources.c
resources.c:
	glib-compile-resources $(srcdir)/zrythm.gresource.xml --target=resources.c \
	--generate-source --sourcedir=${top_srcdir}/resources
