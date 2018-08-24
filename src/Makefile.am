bin_PROGRAMS = zrythm

# sources for zrythm
zrythm_SOURCES = main.c \
                 init.c \
								 project.c \
								 settings_manager.c \
								 zrythm_system.c \
								 audio/channel.c \
								 audio/engine.c \
								 audio/mixer.c \
								 audio/port.c \
								 audio/timeline.c \
								 audio/region.c \
								 gui/main_window.c \
								 gui/widget_manager.c \
								 gui/widgets/instrument_timeline_view.c \
								 gui/widgets/browser.c \
								 gui/widgets/channel.c \
								 gui/widgets/channel_color.c \
								 gui/widgets/fader.c \
								 gui/widgets/knob.c \
								 gui/widgets/ruler.c \
								 gui/widgets/timeline.c \
								 plugins/lv2_gtk.c \
								 plugins/lv2_plugin.c \
								 plugins/plugin.c \
								 plugins/plugin_manager.c \
								 plugins/lv2/control.c \
								 plugins/lv2/log.c \
								 plugins/lv2/lv2_evbuf.c \
								 plugins/lv2/state.c \
								 plugins/lv2/symap.c \
								 plugins/lv2/worker.c \
								 plugins/lv2/zix/ring.c

AM_CFLAGS = -rdynamic -I$(top_srcdir)/inc \
						-I$(src_dir) \
						$(GTK_CFLAGS) \
						$(JACK_CFLAGS) \
						$(LV2_CFLAGS) \
						$(LILV_CFLAGS) \
						$(SUIL_CFLAGS)
zrythm_LDADD = $(GTK_LIBS) \
			   $(JACK_LIBS) \
			   $(LV2_LIBS) \
			   $(LILV_LIBS) \
				 $(SUIL_LIBS) \
			   -lm

zrythm_LDFLAGS = -pthread

nodist_zrythm_SOURCES = resources.c
zrythm.$(OBJEXT): resources.c # manually declare dependency

BUILT_SOURCES = resources.c

# resources is a built source that should be cleaned
CLEANFILES = resources.c

# build rule for resources.c
resources.c:
	glib-compile-resources $(srcdir)/zrythm.gresource.xml --target=resources.c \
	--generate-source --sourcedir=${top_srcdir}/resources
