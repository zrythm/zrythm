# -----------------------------------------
# Compile all the Ports

PREFIX  := /usr/local
LIBDIR  := $(PREFIX)/lib
SRCDIR  := $(PREFIX)/src
DESTDIR :=

# -----------------------------------------
# all

all:
	$(MAKE) -C libs/drowaudio
	$(MAKE) -C libs/juce
	$(MAKE) -C libs/juced
	$(MAKE) -C libs/lv2-ttl-generator
	$(MAKE) -C ports plugins
	$(MAKE) gen

# -----------------------------------------
# install

install:
	# make dirs
	install -d $(DESTDIR)$(LIBDIR)/lv2/
	install -d $(DESTDIR)$(LIBDIR)/vst/
	install -d $(DESTDIR)$(SRCDIR)/distrho/libs/juce/build-juce/
	install -d $(DESTDIR)$(SRCDIR)/distrho/libs/juce/source/
	install -d $(DESTDIR)$(SRCDIR)/distrho/libs/juce-plugin/
	install -d $(DESTDIR)$(SRCDIR)/distrho/scripts/

	# install plugins
ifneq (,$(wildcard bin/lv2/TheFunction.lv2))
	cp -r bin/lv2/*.lv2/        $(DESTDIR)$(LIBDIR)/lv2/
	cp -r static-lv2-ttl/*.lv2/ $(DESTDIR)$(LIBDIR)/lv2/
endif
ifneq (,$(wildcard bin/vst/TheFunction.so))
	cp -r bin/vst/*             $(DESTDIR)$(LIBDIR)/vst/
endif

	# install source needed for extra builds
	install -m 644 libs/libjuce.a           $(DESTDIR)$(SRCDIR)/distrho/libs/
	install -m 755 libs/lv2_ttl_generator   $(DESTDIR)$(SRCDIR)/distrho/libs/
	install -m 644 libs/juce/build-juce/*.h $(DESTDIR)$(SRCDIR)/distrho/libs/juce/build-juce/
	install -m 644 libs/juce-plugin/*.cpp   $(DESTDIR)$(SRCDIR)/distrho/libs/juce-plugin/
	install -m 644 libs/juce-plugin/*.h     $(DESTDIR)$(SRCDIR)/distrho/libs/juce-plugin/
	install -m 644 scripts/*.lua            $(DESTDIR)$(SRCDIR)/distrho/scripts/
	install -m 755 scripts/*.sh             $(DESTDIR)$(SRCDIR)/distrho/scripts/

	find libs/juce/source/ -type f -name "*.h" -exec cp -v --parents {} $(DESTDIR)$(SRCDIR)/distrho/ \;

	cp -v --parents -r libs/juce/source/modules/juce_audio_plugin_client/LV2/*                            $(DESTDIR)$(SRCDIR)/distrho/
	cp -v --parents    libs/juce/source/modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp         $(DESTDIR)$(SRCDIR)/distrho/
	cp -v --parents    libs/juce/source/modules/juce_audio_plugin_client/utility/juce_PluginUtilities.cpp $(DESTDIR)$(SRCDIR)/distrho/

# -----------------------------------------
# gen

gen: gen_lv2 gen_vst

gen_lv2:
	$(MAKE) -C libs/lv2-ttl-generator
	@./scripts/generate-cabbage-lv2.sh
	@./scripts/generate-ttl.sh

gen_vst:
	@./scripts/generate-cabbage-vst.sh

# -----------------------------------------
# clean

clean:
	$(MAKE) clean -C libs/drowaudio
	$(MAKE) clean -C libs/juce
	$(MAKE) clean -C libs/juced
	$(MAKE) clean -C libs/lv2-ttl-generator
	$(MAKE) clean -C ports
	rm -rf bin/lv2/*.lv2/
	rm -rf bin/lv2-extra/
	rm -rf bin/vst-extra/

distclean: clean
	$(MAKE) distclean -C libs/drowaudio
	$(MAKE) distclean -C libs/juce
	$(MAKE) distclean -C libs/juced
	$(MAKE) distclean -C ports

# -----------------------------------------
# mingw

mingw:
	$(MAKE) -C libs/lv2-ttl-generator mingw

# -----------------------------------------
# Custom build types

libs:
	$(MAKE) -C libs/drowaudio
	$(MAKE) -C libs/juce
	$(MAKE) -C libs/juced
	$(MAKE) -C libs/lv2-ttl-generator

lv2:
	$(MAKE) -C libs/drowaudio
	$(MAKE) -C libs/juce
	$(MAKE) -C libs/juced
	$(MAKE) -C libs/lv2-ttl-generator
	$(MAKE) -C ports lv2
	$(MAKE) gen_lv2

lv2_nogen:
	$(MAKE) -C libs/drowaudio
	$(MAKE) -C libs/juce
	$(MAKE) -C libs/juced
	$(MAKE) -C ports lv2

vst:
	$(MAKE) -C libs/drowaudio
	$(MAKE) -C libs/juce
	$(MAKE) -C libs/juced
	$(MAKE) -C ports vst
	$(MAKE) gen_vst

.PHONY: libs

