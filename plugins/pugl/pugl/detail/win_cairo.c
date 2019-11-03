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
   @file win_cairo.c Cairo graphics backend for Windows.
*/

#include "pugl/detail/types.h"
#include "pugl/detail/win.h"
#include "pugl/pugl_cairo_backend.h"

#include <cairo-win32.h>
#include <cairo.h>

#include <stdlib.h>

typedef struct  {
	cairo_surface_t* surface;
	cairo_t*         cr;
	HDC              drawDc;
	HBITMAP          drawBitmap;
} PuglWinCairoSurface;

static PuglStatus
puglWinCairoCreateDrawContext(PuglView* view)
{
	PuglInternals* const       impl    = view->impl;
	PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

	surface->drawDc     = CreateCompatibleDC(impl->hdc);
	surface->drawBitmap = CreateCompatibleBitmap(
		impl->hdc, (int)view->frame.width, (int)view->frame.height);

	DeleteObject(SelectObject(surface->drawDc, surface->drawBitmap));

	cairo_status_t st = CAIRO_STATUS_SUCCESS;
	if (!(surface->surface = cairo_win32_surface_create(surface->drawDc)) ||
	    (st = cairo_surface_status(surface->surface)) ||
	    !(surface->cr = cairo_create(surface->surface)) ||
	    (st = cairo_status(surface->cr))) {
		return PUGL_CREATE_CONTEXT_FAILED;
	}

	cairo_save(surface->cr);
	return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoDestroyDrawContext(PuglView* view)
{
	PuglInternals* const       impl    = view->impl;
	PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

	DeleteDC(surface->drawDc);
	DeleteObject(surface->drawBitmap);
	cairo_destroy(surface->cr);
	cairo_surface_destroy(surface->surface);

	surface->surface    = NULL;
	surface->cr         = NULL;
	surface->drawDc     = NULL;
	surface->drawBitmap = NULL;

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoConfigure(PuglView* view)
{
	PuglInternals* const impl = view->impl;
	PuglStatus           st   = PUGL_SUCCESS;

	if ((st = puglWinCreateWindow(view, "Pugl", &impl->hwnd, &impl->hdc))) {
		return st;
	}

	impl->pfd  = puglWinGetPixelFormatDescriptor(view->hints);
	impl->pfId = ChoosePixelFormat(impl->hdc, &impl->pfd);

	if (!SetPixelFormat(impl->hdc, impl->pfId, &impl->pfd)) {
		ReleaseDC(impl->hwnd, impl->hdc);
		DestroyWindow(impl->hwnd);
		impl->hwnd = NULL;
		impl->hdc  = NULL;
		return PUGL_SET_FORMAT_FAILED;
	}

	impl->surface = (PuglWinCairoSurface*)calloc(
		1, sizeof(PuglWinCairoSurface));

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoCreate(PuglView* view)
{
	return puglWinCairoCreateDrawContext(view);
}

static PuglStatus
puglWinCairoDestroy(PuglView* view)
{
	PuglInternals* const       impl    = view->impl;
	PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

	puglWinCairoDestroyDrawContext(view);
	free(surface);
	impl->surface = NULL;

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoEnter(PuglView* view, bool drawing)
{
	PuglInternals* const       impl    = view->impl;
	PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;
	if (!drawing) {
		return PUGL_SUCCESS;
	}

	PAINTSTRUCT ps;
	BeginPaint(view->impl->hwnd, &ps);
	cairo_save(surface->cr);

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoLeave(PuglView* view, bool drawing)
{
	PuglInternals* const       impl    = view->impl;
	PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;
	if (!drawing) {
		return PUGL_SUCCESS;
	}

	cairo_restore(surface->cr);
	cairo_surface_flush(surface->surface);
	BitBlt(impl->hdc,
	       0, 0, (int)view->frame.width, (int)view->frame.height,
	       surface->drawDc, 0, 0, SRCCOPY);

	PAINTSTRUCT ps;
	EndPaint(view->impl->hwnd, &ps);
	SwapBuffers(view->impl->hdc);

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoResize(PuglView* view,
                   int       PUGL_UNUSED(width),
                   int       PUGL_UNUSED(height))
{
	PuglStatus st = PUGL_SUCCESS;
	if ((st = puglWinCairoDestroyDrawContext(view)) ||
	    (st = puglWinCairoCreateDrawContext(view))) {
		return st;
	}

	return PUGL_SUCCESS;
}

static void*
puglWinCairoGetContext(PuglView* view)
{
	return ((PuglWinCairoSurface*)view->impl->surface)->cr;
}

const PuglBackend*
puglCairoBackend()
{
	static const PuglBackend backend = {
		puglWinCairoConfigure,
		puglWinCairoCreate,
		puglWinCairoDestroy,
		puglWinCairoEnter,
		puglWinCairoLeave,
		puglWinCairoResize,
		puglWinCairoGetContext
	};

	return &backend;
}
