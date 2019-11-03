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
   @file x11_cairo.c Cairo graphics backend for X11.
*/

#include "pugl/detail/types.h"
#include "pugl/detail/x11.h"
#include "pugl/pugl.h"
#include "pugl/pugl_cairo_backend.h"

#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct  {
	cairo_surface_t* back;
	cairo_t*         backCr;
	cairo_surface_t* front;
	cairo_t*         frontCr;
} PuglX11CairoSurface;

static PuglStatus
puglX11CairoConfigure(PuglView* view)
{
	PuglInternals* const impl = view->impl;

	XVisualInfo pat;
	int         n;
	pat.screen = impl->screen;
	impl->vi = XGetVisualInfo(impl->display, VisualScreenMask, &pat, &n);

	return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoCreate(PuglView* view)
{
	PuglInternals* const impl    = view->impl;
	const int            width   = (int)view->frame.width;
	const int            height  = (int)view->frame.height;
	PuglX11CairoSurface  surface = { 0 };

	surface.back = cairo_xlib_surface_create(
		impl->display, impl->win, impl->vi->visual, width, height);
	surface.front = cairo_surface_create_similar(
		surface.back, CAIRO_CONTENT_COLOR, width, height);
	surface.backCr  = cairo_create(surface.back);
	surface.frontCr = cairo_create(surface.front);

	cairo_status_t st = CAIRO_STATUS_SUCCESS;
	if (!surface.back || !surface.backCr ||
	    !surface.front || !surface.frontCr ||
	    (st = cairo_surface_status(surface.back)) ||
	    (st = cairo_surface_status(surface.front)) ||
	    (st = cairo_status(surface.backCr)) ||
	    (st = cairo_status(surface.frontCr))) {
		cairo_destroy(surface.frontCr);
		cairo_destroy(surface.backCr);
		cairo_surface_destroy(surface.front);
		cairo_surface_destroy(surface.back);
		return PUGL_CREATE_CONTEXT_FAILED;
	}

	impl->surface = calloc(1, sizeof(PuglX11CairoSurface));
	*(PuglX11CairoSurface*)impl->surface = surface;

	return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoDestroy(PuglView* view)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	cairo_destroy(surface->frontCr);
	cairo_destroy(surface->backCr);
	cairo_surface_destroy(surface->front);
	cairo_surface_destroy(surface->back);
	free(surface);
	impl->surface = NULL;
	return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoEnter(PuglView* view, bool drawing)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	if (drawing) {
		cairo_save(surface->frontCr);
	}

	return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoLeave(PuglView* view, bool drawing)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	if (drawing) {
		cairo_set_source_surface(surface->backCr, surface->front, 0, 0);
		cairo_paint(surface->backCr);
		cairo_restore(surface->frontCr);
	}

	return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoResize(PuglView* view, int width, int height)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	cairo_xlib_surface_set_size(surface->back, width, height);

	cairo_destroy(surface->frontCr);
	cairo_surface_destroy(surface->front);
	if (!(surface->front = cairo_surface_create_similar(
		      surface->back, CAIRO_CONTENT_COLOR, width, height))) {
		return PUGL_CREATE_CONTEXT_FAILED;
	}

	surface->frontCr = cairo_create(surface->front);
	cairo_save(surface->frontCr);

	return PUGL_SUCCESS;
}

static void*
puglX11CairoGetContext(PuglView* view)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	return surface->frontCr;
}

const PuglBackend*
puglCairoBackend(void)
{
	static const PuglBackend backend = {
		puglX11CairoConfigure,
		puglX11CairoCreate,
		puglX11CairoDestroy,
		puglX11CairoEnter,
		puglX11CairoLeave,
		puglX11CairoResize,
		puglX11CairoGetContext
	};

	return &backend;
}
