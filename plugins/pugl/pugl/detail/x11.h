/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file x11.h Shared definitions for X11 implementation.
*/

#include "pugl/detail/implementation.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
	Atom CLIPBOARD;
	Atom UTF8_STRING;
	Atom WM_PROTOCOLS;
	Atom WM_DELETE_WINDOW;
	Atom NET_WM_NAME;
	Atom NET_WM_STATE;
	Atom NET_WM_STATE_DEMANDS_ATTENTION;
} PuglX11Atoms;

struct PuglWorldInternalsImpl {
	Display*     display;
	PuglX11Atoms atoms;
	XIM          xim;
};

struct PuglInternalsImpl {
	Display*     display;
	int          screen;
	XVisualInfo* vi;
	Window       win;
	XIC          xic;
	PuglSurface* surface;
	PuglEvent    pendingConfigure;
	PuglEvent    pendingExpose;
};
