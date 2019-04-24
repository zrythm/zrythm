#!/usr/bin/make -f
OPTIMIZATIONS ?= -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only -DNDEBUG
PREFIX ?= /usr/local
CFLAGS ?= $(OPTIMIZATIONS) -Wall

PKG_CONFIG?=pkg-config
STRIP?=strip
STRIPFLAGS?=-s

midifilter_VERSION?=$(shell git describe --tags HEAD 2>/dev/null | sed 's/-g.*$$//;s/^v//' || echo "LV2")
###############################################################################

LV2DIR ?= $(PREFIX)/lib/lv2
LOADLIBES=-lm
LV2NAME=midifilter
BUNDLE=midifilter.lv2
BUILDDIR=build/
targets=

ifneq ($(MOD),)
  MODBRAND=mod:brand \"x42\";
  GREPEXCL="^^^^"
else
  GREPEXCL=mod:label[^;]*;
  MODBRAND=
endif


UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  LV2LDFLAGS=-dynamiclib
  LIB_EXT=.dylib
  EXTENDED_RE=-E
  STRIPFLAGS=-u -r -arch all -s lv2syms
  targets+=lv2syms
else
  LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic
  LIB_EXT=.so
  EXTENDED_RE=-r
endif

ifneq ($(XWIN),)
  CC=$(XWIN)-gcc
  STRIP=$(XWIN)-strip
  LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed
  LIB_EXT=.dll
  override LDFLAGS += -static-libgcc -static-libstdc++
else
  override CFLAGS += -fPIC -fvisibility=hidden
endif

targets+=$(BUILDDIR)$(LV2NAME)$(LIB_EXT)
ifneq ($(MOD),)
  targets+=$(BUILDDIR)modgui
endif

###############################################################################
# extract versions
LV2VERSION=$(midifilter_VERSION)
include git2lv2.mk

# check for build-dependencies
ifeq ($(shell $(PKG_CONFIG) --exists lv2 || echo no), no)
  $(error "LV2 SDK was not found")
endif

override CFLAGS += -std=c99 `$(PKG_CONFIG) --cflags lv2`

# build target definitions
default: all

all: $(BUILDDIR)manifest.ttl $(BUILDDIR)presets.ttl $(BUILDDIR)$(LV2NAME).ttl $(targets)

FILTERS := $(wildcard filters/*.c)

lv2syms:
	echo "_lv2_descriptor" > lv2syms

filters.c: $(FILTERS)
	echo "#include \"ttf.h\"" > filters.c
	i=0; for file in $(FILTERS); do \
		echo "#define MFD_FILTER(FNX) MFD_FLT($$i, FNX)" >> filters.c; \
		echo "#include \"$${file}\"" >> filters.c; \
		echo "#undef MFD_FILTER" >> filters.c; \
		i=`expr $$i + 1`; \
		done;
	echo "#define LOOP_DESC(FN) \\" >> filters.c;
	i=0; for file in $(FILTERS); do \
		echo "FN($$i) \\" >> filters.c; \
		i=`expr $$i + 1`; \
		done;
	echo >> filters.c;

$(BUILDDIR)manifest.ttl: manifest.ttl.in ttf.h filters.c
	@mkdir -p $(BUILDDIR)
	cat manifest.ttl.in > $(BUILDDIR)manifest.ttl
	gcc -E -I. -DMX_MANIFEST filters.c \
		| grep -v '^\#' \
		| sed "s/HTTPP/http:\//g;s/HASH/#/g;s/@LV2NAME@/$(LV2NAME)/g;s/@LIB_EXT@/$(LIB_EXT)/g" \
		| uniq \
		>> $(BUILDDIR)manifest.ttl
ifneq ($(MOD),)
	gcc -E -I. -DMX_MODGUI filters.c \
		| grep -v '^\#' \
		| sed "s/HTTPP/http:\//g;s/HASH/#/g;s/_DASH_/-/g;s/_DOT_/./g;" \
		| uniq \
		>> $(BUILDDIR)manifest.ttl
endif
	for file in presets/*.ttl; do head -n 3 $$file >> $(BUILDDIR)manifest.ttl; echo "rdfs:seeAlso <presets.ttl> ." >> $(BUILDDIR)manifest.ttl; done

$(BUILDDIR)presets.ttl: presets.ttl.in presets/*.ttl
	@mkdir -p $(BUILDDIR)
	cat presets.ttl.in > $(BUILDDIR)presets.ttl
	cat presets/*.ttl >> $(BUILDDIR)presets.ttl

$(BUILDDIR)$(LV2NAME).ttl: $(LV2NAME).ttl.in ttf.h filters.c
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME).ttl.in > $(BUILDDIR)$(LV2NAME).ttl
	gcc -E -I. -DMX_TTF filters.c \
		| grep -v '^\#' \
		| sed 's/HTTPP/http:\//g;s/@VERSION@/lv2:microVersion $(LV2MIC) ;lv2:minorVersion $(LV2MIN) ;/g;s/@MODBRAND@/$(MODBRAND)/;s/$(GREPEXCL)//' \
		| uniq \
		>> $(BUILDDIR)$(LV2NAME).ttl

$(BUILDDIR)$(LV2NAME)$(LIB_EXT): $(LV2NAME).c midifilter.h filters.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) \
	  -o $(BUILDDIR)$(LV2NAME)$(LIB_EXT) $(LV2NAME).c \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)
	$(STRIP) $(STRIPFLAGS) $(BUILDDIR)$(LV2NAME)$(LIB_EXT)

$(BUILDDIR)modgui: modgui/
	@mkdir -p $(BUILDDIR)/modgui
	cp -r modgui/* $(BUILDDIR)modgui/

# install/uninstall/clean target definitions

install: all
	install -d $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m644 $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME).ttl $(BUILDDIR)presets.ttl $(DESTDIR)$(LV2DIR)/$(BUNDLE)
ifneq ($(MOD),)
	install -d $(DESTDIR)$(LV2DIR)/$(BUNDLE)/modgui
	install -t $(DESTDIR)$(LV2DIR)/$(BUNDLE)/modgui $(BUILDDIR)modgui/*
endif

uninstall:
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/manifest.ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/presets.ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME)$(LIB_EXT)
	rm -rf $(DESTDIR)$(LV2DIR)/$(BUNDLE)/modgui
	-rmdir $(DESTDIR)$(LV2DIR)/$(BUNDLE)

clean:
	rm -f $(BUILDDIR)manifest.ttl $(BUILDDIR)presets.ttl $(BUILDDIR)$(LV2NAME).ttl $(BUILDDIR)$(LV2NAME)$(LIB_EXT) lv2syms filters.c
	rm -rf $(BUILDDIR)modgui
	-test -d $(BUILDDIR) && rmdir $(BUILDDIR) || true

.PHONY: clean all install uninstall
