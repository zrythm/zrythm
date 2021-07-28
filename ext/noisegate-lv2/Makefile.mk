#!/usr/bin/make -f
# Makefile for noisegate.lv2 #
# ----------------------- #
#

AR  ?= ar
CC  ?= gcc
CXX ?= g++

# --------------------------------------------------------------
# Fallback to Linux if no other OS defined

ifneq ($(MACOS),true)
ifneq ($(WIN32),true)
LINUX=true
endif
endif

# --------------------------------------------------------------
# Set build and link flags

BASE_FLAGS = -Wall -Wextra -pipe -Wno-unused-parameter
BASE_OPTS  = -O3 -ffast-math

ifeq ($(MACOS),true)
# MacOS linker flags
LINK_OPTS  = -Wl,-dead_strip -Wl,-dead_strip_dylibs
else
# Common linker flags
LINK_OPTS  = -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifneq ($(WIN32),true)
# not needed for Windows
BASE_FLAGS += -fPIC -DPIC
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -O3 -g
LINK_OPTS   =
else
BASE_FLAGS += -DNDEBUG $(BASE_OPTS) -fvisibility=hidden -O3
CXXFLAGS   += -fvisibility-inlines-hidden
endif

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=c99 -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=c++11 $(CXXFLAGS) $(CPPFLAGS)

ifeq ($(MACOS),true)
# 'no-undefined' is always enabled on MacOS
LINK_FLAGS      = $(LINK_OPTS) $(LDFLAGS)
else
# add 'no-undefined'
LINK_FLAGS      = $(LINK_OPTS) -Wl,--no-undefined $(LDFLAGS)
endif

# --------------------------------------------------------------
# Set shared lib extension

LIB_EXT = .so

ifeq ($(MACOS),true)
LIB_EXT = .dylib
endif

ifeq ($(WIN32),true)
LIB_EXT = .dll
endif

# --------------------------------------------------------------
# Set shared library CLI arg

SHARED = -shared

ifeq ($(MACOS),true)
SHARED = -dynamiclib
endif

# --------------------------------------------------------------
