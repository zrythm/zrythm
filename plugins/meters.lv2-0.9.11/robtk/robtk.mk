RT=$(RW)rtk/
WD=$(RW)widgets/robtk_
PKG_CONFIG?=pkg-config
STRIP?=strip
LIBSTRIPFLAGS?=-s
APPSTRIPFLAGS?=-s
WINDRES=$(XWIN)-windres
UNAME?=$(shell uname)

JACKEXTRA=
OSXJACKWRAP=

ifeq ($(UNAME),Darwin)
  OSXJACKWRAP=$(RW)jackwrap.mm
  USEWEAKJACK=1
  LIBSTRIPFLAGS=-u -r -arch all -s $(RW)lv2uisyms
  APPSTRIPFLAGS=-u -r -arch all
endif

ifneq ($(XWIN),)
  USEWEAKJACK=1
  JACKCFLAGS+=-mwindows
  ifeq ($(shell test -f img/x42.ico && echo yes), yes)
    JACKEXTRA+=win_icon.rc.o
  endif
else
  JACKCFLAGS+=-pthread
endif

ifeq ($(USEWEAKJACK),1)
  JACKCFLAGS+=-DUSE_WEAK_JACK
  JACKEXTRA+=$(RW)weakjack/weak_libjack.c
  ifeq ($(XWIN),)
    JACKLIBS+=-ldl
  endif
else
  JACKLIBS+=`$(PKG_CONFIG) $(PKG_UI_FLAGS) --libs jack`
endif

ifeq ($(shell $(PKG_CONFIG) --exists liblo && echo yes), yes)
  JACKCFLAGS+=`$(PKG_CONFIG) $(PKG_UI_FLAGS) --cflags liblo` -DHAVE_LIBLO
  JACKLIBS+=`$(PKG_CONFIG) $(PKG_UI_FLAGS) --libs liblo`
endif


UITOOLKIT=$(WD)checkbutton.h $(WD)dial.h $(WD)label.h $(WD)pushbutton.h\
          $(WD)radiobutton.h $(WD)scale.h $(WD)separator.h $(WD)spinner.h \
          $(WD)xyplot.h $(WD)selector.h $(WD)multibutton.h \
          $(WD)image.h $(WD)drawingarea.h $(WD)checkimgbutton.h

ROBGL= $(RW)robtk.mk $(UITOOLKIT) $(RW)ui_gl.c $(PUGL_SRC) \
  $(RW)gl/common_cgl.h $(RW)gl/layout.h $(RW)gl/robwidget_gl.h $(RW)robtk.h \
	$(RT)common.h $(RT)style.h \
  $(RW)gl/xternalui.c $(RW)gl/xternalui.h

ROBGTK = $(RW)robtk.mk $(UITOOLKIT) $(RW)ui_gtk.c \
  $(RW)gtk2/common_cgtk.h $(RW)gtk2/robwidget_gtk.h $(RW)robtk.h \
	$(RT)common.h $(RT)style.h

%UI_gtk.so %UI_gtk.dylib:: $(ROBGTK)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(GTKUICFLAGS) \
	  -DPLUGIN_SOURCE="\"gui/$(*F).c\"" \
	  -o $@ $(RW)ui_gtk.c \
	  $(value $(*F)_UISRC) \
	  -shared $(LV2LDFLAGS) $(LDFLAGS) $(GTKUILIBS)
	$(STRIP) ${LIBSTRIPFLAGS} $@

%UI_gl.o:: $(ROBGL)
	@mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) $(GLUICFLAGS) \
	  -DUINQHACK="$(shell date +%s$$$$)" \
	  -DPLUGIN_SOURCE="\"gui/$(*F).c\"" \
	  -DRTK_DESCRIPTOR="$(value gl_$(subst -,_,$(*F))_LV2DESC)" \
	  -o $@ $(RW)ui_gl.c

%pugl.o:: $(ROBGL)
	@mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) $(GLUICFLAGS) \
	  -DUINQHACK="$(shell date +%s$$$$)" \
	  -o $@ $(PUGL_SRC)

%_glui.so %_glui.dylib %_glui.dll::
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(GLUICFLAGS) \
	  -o $@ gui/$(*F).c \
	  $(GLGUIOBJ) \
	  $(value $(*F)_UISRC) \
	  -shared $(LV2LDFLAGS) $(LDFLAGS) $(GLUILIBS)
	$(STRIP) ${LIBSTRIPFLAGS} $@


%UI_gl.so %UI_gl.dylib %UI_gl.dll:: $(ROBGL)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(GLUICFLAGS) \
	  -DUINQHACK="$(shell date +%s$$$$)" \
	  -DPLUGIN_SOURCE="\"gui/$(*F).c\"" \
	  -o $@ $(RW)ui_gl.c \
	  $(PUGL_SRC) \
	  $(value $(*F)_UISRC) \
	  -shared $(LV2LDFLAGS) $(LDFLAGS) $(GLUILIBS)
	$(STRIP) ${LIBSTRIPFLAGS} $@

# ignore man-pages in rule below
x42-%.1:
	@/bin/true

# windows icon
.SUFFFIXES: .rc
win_icon.rc.o: $(RW)win_icon.rc img/x42.ico
	$(WINDRES) -o $@ $<

x42-%.o:: $(ROBGL)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(JACKCFLAGS) \
	  -DXTERNAL_UI -DHAVE_IDLE_IFACE -DDEFAULT_NOT_ONTOP \
	  -DRTK_DESCRIPTOR="$(value x42_$(subst -,_,$(*F))_JACKDESC)" \
	  -DPLUGIN_SOURCE="\"$(value x42_$(subst -,_,$(*F))_JACKGUI)\"" \
	  -o $@ \
	  -c $(RW)ui_gl.c

x42-%-collection x42-%-collection.exe:: $(ROBGL) $(RW)jackwrap.c $(OSXJACKWRAP) $(RW)weakjack/weak_libjack.def $(RW)weakjack/weak_libjack.h $(JACKEXTRA)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(JACKCFLAGS) -DDEFAULT_NOT_ONTOP \
	  -DXTERNAL_UI -DHAVE_IDLE_IFACE \
	  -DJACK_DESCRIPT="\"$(value x42_$(subst -,_,$(*F))_collection_LV2HTTL)\"" \
	  -DAPPNAME="\"$(*F)\"" \
	  -o $@ \
	  $(RW)jackwrap.c $(PUGL_SRC) $(OSXJACKWRAP) $(JACKEXTRA) \
	  $(value x42_$(subst -,_,$(*F))_collection_JACKSRC) \
	  $(LDFLAGS) $(JACKLIBS)
	$(STRIP) ${APPSTRIPFLAGS} $@

x42-% x42-%.exe:: $(ROBGL) $(RW)jackwrap.c $(OSXJACKWRAP) $(RW)weakjack/weak_libjack.def $(RW)weakjack/weak_libjack.h $(JACKEXTRA)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(JACKCFLAGS) -DDEFAULT_NOT_ONTOP \
	  -DXTERNAL_UI -DHAVE_IDLE_IFACE \
	  -DRTK_DESCRIPTOR="$(value x42_$(subst -,_,$(*F))_JACKDESC)" \
	  -DPLUGIN_SOURCE="\"$(value x42_$(subst -,_,$(*F))_JACKGUI)\"" \
	  -DJACK_DESCRIPT="\"$(value x42_$(subst -,_,$(*F))_LV2HTTL)\"" \
	  -DAPPNAME="\"$(*F)\"" \
	  -o $@ \
	  $(RW)jackwrap.c $(RW)ui_gl.c $(PUGL_SRC) $(OSXJACKWRAP) $(JACKEXTRA) \
	  $(value x42_$(subst -,_,$(*F))_JACKSRC) \
	  $(LDFLAGS) $(JACKLIBS)
	$(STRIP) ${APPSTRIPFLAGS} $@
