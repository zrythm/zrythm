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
   @file implementation.c Platform-independent implementation.
*/

#include "pugl/detail/implementation.h"
#include "pugl/pugl.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void
puglSetString(char** dest, const char* string)
{
	const size_t len = strlen(string);

	*dest = (char*)realloc(*dest, len + 1);
	strncpy(*dest, string, len + 1);
}

void
puglSetBlob(PuglBlob* const dest, const void* const data, const size_t len)
{
	if (data) {
		dest->len  = len;
		dest->data = realloc(dest->data, len + 1);
		memcpy(dest->data, data, len);
		((char*)dest->data)[len] = 0;
	} else {
		dest->len  = 0;
		dest->data = NULL;
	}
}

static void
puglSetDefaultHints(PuglHints hints)
{
	hints[PUGL_USE_COMPAT_PROFILE]    = PUGL_TRUE;
	hints[PUGL_CONTEXT_VERSION_MAJOR] = 2;
	hints[PUGL_CONTEXT_VERSION_MINOR] = 0;
	hints[PUGL_RED_BITS]              = 4;
	hints[PUGL_GREEN_BITS]            = 4;
	hints[PUGL_BLUE_BITS]             = 4;
	hints[PUGL_ALPHA_BITS]            = 4;
	hints[PUGL_DEPTH_BITS]            = 24;
	hints[PUGL_STENCIL_BITS]          = 8;
	hints[PUGL_SAMPLES]               = 0;
	hints[PUGL_DOUBLE_BUFFER]         = PUGL_FALSE;
	hints[PUGL_SWAP_INTERVAL]         = 0;
	hints[PUGL_RESIZABLE]             = PUGL_FALSE;
	hints[PUGL_IGNORE_KEY_REPEAT]     = PUGL_FALSE;
}

PuglWorld*
puglNewWorld(void)
{
	PuglWorld* world = (PuglWorld*)calloc(1, sizeof(PuglWorld));
	if (!world || !(world->impl = puglInitWorldInternals())) {
		free(world);
		return NULL;
	}

	world->startTime = puglGetTime(world);
	puglSetString(&world->className, "Pugl");

	return world;
}

void
puglFreeWorld(PuglWorld* const world)
{
	puglFreeWorldInternals(world);
	free(world->className);
	free(world->views);
	free(world);
}

PuglStatus
puglSetClassName(PuglWorld* const world, const char* const name)
{
	puglSetString(&world->className, name);
	return PUGL_SUCCESS;
}

PuglView*
puglNewView(PuglWorld* const world)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
	if (!view || !(view->impl = puglInitViewInternals())) {
		free(view);
		return NULL;
	}

	view->world        = world;
	view->frame.width  = 640;
	view->frame.height = 480;

	puglSetDefaultHints(view->hints);

	// Add to world view list
	++world->numViews;
	world->views = (PuglView**)realloc(world->views,
	                                   world->numViews * sizeof(PuglView*));
	world->views[world->numViews - 1] = view;

	return view;
}

void
puglFreeView(PuglView* view)
{
	// Remove from world view list
	PuglWorld* world = view->world;
	for (size_t i = 0; i < world->numViews; ++i) {
		if (world->views[i] == view) {
			if (i == world->numViews - 1) {
				world->views[i] = NULL;
			} else {
				memmove(world->views + i, world->views + i + 1,
				        sizeof(PuglView*) * (world->numViews - i - 1));
				world->views[world->numViews - 1] = NULL;
			}
			--world->numViews;
		}
	}

	free(view->title);
	free(view->clipboard.data);
	puglFreeViewInternals(view);
	free(view);
}

PuglWorld*
puglGetWorld(PuglView* view)
{
	return view->world;
}

PuglStatus
puglSetViewHint(PuglView* view, PuglViewHint hint, int value)
{
	if (hint < PUGL_NUM_WINDOW_HINTS) {
		view->hints[hint] = value;
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglSetParentWindow(PuglView* view, PuglNativeWindow parent)
{
	view->parent = parent;
	return PUGL_SUCCESS;
}

PuglStatus
puglSetBackend(PuglView* view, const PuglBackend* backend)
{
	view->backend = backend;
	return PUGL_SUCCESS;
}

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

bool
puglGetVisible(PuglView* view)
{
	return view->visible;
}

PuglRect
puglGetFrame(const PuglView* view)
{
	return view->frame;
}

void*
puglGetContext(PuglView* view)
{
	return view->backend->getContext(view);
}

PuglStatus
puglEnterContext(PuglView* view, bool drawing)
{
	view->backend->enter(view, drawing);
	return PUGL_SUCCESS;
}

PuglStatus
puglLeaveContext(PuglView* view, bool drawing)
{
	view->backend->leave(view, drawing);
	return PUGL_SUCCESS;
}

PuglStatus
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc)
{
	view->eventFunc = eventFunc;
	return PUGL_SUCCESS;
}

/** Return the code point for buf, or the replacement character on error. */
uint32_t
puglDecodeUTF8(const uint8_t* buf)
{
#define FAIL_IF(cond) do { if (cond) return 0xFFFD; } while (0)

	// http://en.wikipedia.org/wiki/UTF-8

	if (buf[0] < 0x80) {
		return buf[0];
	} else if (buf[0] < 0xC2) {
		return 0xFFFD;
	} else if (buf[0] < 0xE0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		return (buf[0] << 6) + buf[1] - 0x3080u;
	} else if (buf[0] < 0xF0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xE0 && buf[1] < 0xA0);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		return (buf[0] << 12) + (buf[1] << 6) + buf[2] - 0xE2080u;
	} else if (buf[0] < 0xF5) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xF0 && buf[1] < 0x90);
		FAIL_IF(buf[0] == 0xF4 && buf[1] >= 0x90);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		FAIL_IF((buf[3] & 0xC0) != 0x80);
		return ((buf[0] << 18) +
		        (buf[1] << 12) +
		        (buf[2] << 6) +
		        buf[3] - 0x3C82080u);
	}
	return 0xFFFD;
}

void
puglDispatchEvent(PuglView* view, const PuglEvent* event)
{
	switch (event->type) {
	case PUGL_NOTHING:
		break;
	case PUGL_CONFIGURE:
		puglEnterContext(view, false);
		view->eventFunc(view, event);
		puglLeaveContext(view, false);
		break;
	case PUGL_EXPOSE:
		if (event->expose.count == 0) {
			puglEnterContext(view, true);
			view->eventFunc(view, event);
			puglLeaveContext(view, true);
		}
		break;
	default:
		view->eventFunc(view, event);
	}
}

const void*
puglGetInternalClipboard(const PuglView* const view,
                         const char** const    type,
                         size_t* const         len)
{
	if (len) {
		*len = view->clipboard.len;
	}

	if (type) {
		*type = "text/plain";
	}

	return view->clipboard.data;
}

PuglStatus
puglSetInternalClipboard(PuglView* const   view,
                         const char* const type,
                         const void* const data,
                         const size_t      len)
{
	if (type && strcmp(type, "text/plain")) {
		return PUGL_UNSUPPORTED_TYPE;
	}

	puglSetBlob(&view->clipboard, data, len);
	return PUGL_SUCCESS;
}

