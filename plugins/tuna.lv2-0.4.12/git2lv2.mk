
###############################################################################
# extract versions
GIT_REV_REGEXP="([0-9][0-9]*)\.([0-9][0-9]*)(\.([0-9][0-9]*))?(-([0-9][0-9]*))?(-g([a-f0-9]+))?"

override MAJOR=$(shell echo $(LV2VERSION) | sed $(EXTENDED_RE) -e s/$(GIT_REV_REGEXP)/\\1/)
override MINOR=$(shell echo $(LV2VERSION) | sed $(EXTENDED_RE) -e s/$(GIT_REV_REGEXP)/\\2/)
override MICRO=$(shell echo $(LV2VERSION) | sed $(EXTENDED_RE) -e s/$(GIT_REV_REGEXP)/\\4/)
override GITREV=$(shell echo $(LV2VERSION) | sed $(EXTENDED_RE) -e s/$(GIT_REV_REGEXP)/\\6/)

ifeq ($(MAJOR),)
  override MAJOR=0
endif
ifeq ($(MINOR),)
  override MINOR=0
endif
ifeq ($(MICRO),)
  override MICRO=0
endif

$(info Version: $(LV2VERSION) -> $(MAJOR) $(MINOR) $(MICRO) $(GITREV))

# version requirements, see
# http://lv2plug.in/ns/lv2core/#minorVersion
# http://lv2plug.in/ns/lv2core/#microVersion
ifeq ($(GITREV),)
# even numbers for tagged releases
  override LV2MIN = $(shell expr $(MAJOR) \* 65536 + $(MINOR) \* 256 + $(MICRO) \* 2 )
  override LV2MIC = 0
else
# odd-numbers for all non tagged git versions
  override LV2MIN = $(shell expr $(MAJOR) \* 65536 + $(MINOR) \* 256 + $(MICRO) \* 2 + 1 )
  override LV2MIC = $(shell expr $(GITREV) \* 2 + 1)
endif

ifeq ($(LV2MIN),)
  $(error "Cannot extract required LV2 minor-version parameter")
endif
ifeq ($(LV2MIC),)
  $(error "Cannot extract required LV2 micro-version parameter")
endif

$(info LV2 Version: $(LV2MIN) $(LV2MIC))
