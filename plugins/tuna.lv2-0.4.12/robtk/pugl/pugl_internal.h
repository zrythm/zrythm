/*
  Copyright 2012 David Robillard <http://drobilla.net>

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
   @file pugl_internal.h Private platform-independent definitions.

   Note this file contains function definitions, so it must be compiled into
   the final binary exactly once.  Each platform specific implementation file
   including it once should achieve this.
*/

#include "pugl.h"

typedef struct PuglInternalsImpl PuglInternals;

struct PuglViewImpl {
	PuglHandle       handle;
	PuglCloseFunc    closeFunc;
	PuglDisplayFunc  displayFunc;
	PuglKeyboardFunc keyboardFunc;
	PuglMotionFunc   motionFunc;
	PuglMouseFunc    mouseFunc;
	PuglReshapeFunc  reshapeFunc;
	PuglResizeFunc   resizeFunc;
	PuglScrollFunc   scrollFunc;
	PuglSpecialFunc  specialFunc;
	PuglFileSelectedFunc fileSelectedFunc;

	PuglInternals* impl;

	int      width;
	int      height;
	int      min_width;
	int      min_height;
	int      mods;
	bool     mouse_in_view;
	bool     ignoreKeyRepeat;
	bool     redisplay;
	bool     user_resizable;
	bool     set_window_hints;
	bool     ontop;
	bool     resize;
	uint32_t event_timestamp_ms;
};

void
puglSetHandle(PuglView* view, PuglHandle handle)
{
	view->handle = handle;
}

PuglHandle
puglGetHandle(PuglView* view)
{
	return view->handle;
}

uint32_t
puglGetEventTimestamp(PuglView* view)
{
	return view->event_timestamp_ms;
}

int
puglGetModifiers(PuglView* view)
{
	return view->mods;
}

static void
puglDefaultReshape(PuglView* view, int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho (-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
	view->ignoreKeyRepeat = ignore;
}

void
puglSetCloseFunc(PuglView* view, PuglCloseFunc closeFunc)
{
	view->closeFunc = closeFunc;
}

void
puglSetDisplayFunc(PuglView* view, PuglDisplayFunc displayFunc)
{
	view->displayFunc = displayFunc;
}

void
puglSetKeyboardFunc(PuglView* view, PuglKeyboardFunc keyboardFunc)
{
	view->keyboardFunc = keyboardFunc;
}

void
puglSetMotionFunc(PuglView* view, PuglMotionFunc motionFunc)
{
	view->motionFunc = motionFunc;
}

void
puglSetMouseFunc(PuglView* view, PuglMouseFunc mouseFunc)
{
	view->mouseFunc = mouseFunc;
}

void
puglSetReshapeFunc(PuglView* view, PuglReshapeFunc reshapeFunc)
{
	view->reshapeFunc = reshapeFunc;
}

void
puglSetResizeFunc(PuglView* view, PuglResizeFunc resizeFunc)
{
	view->resizeFunc = resizeFunc;
}

void
puglSetScrollFunc(PuglView* view, PuglScrollFunc scrollFunc)
{
	view->scrollFunc = scrollFunc;
}

void
puglSetSpecialFunc(PuglView* view, PuglSpecialFunc specialFunc)
{
	view->specialFunc = specialFunc;
}

void
puglSetFileSelectedFunc(PuglView* view, PuglFileSelectedFunc fileSelectedFunc)
{
	view->fileSelectedFunc = fileSelectedFunc;
}
