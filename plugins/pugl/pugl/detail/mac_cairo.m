/*
  Copyright 2019 David Robillard <http://drobilla.net>

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
   @file mac_cairo.m Cairo graphics backend for MacOS.
*/

#include "pugl/detail/implementation.h"
#include "pugl/detail/mac.h"
#include "pugl/pugl_cairo_backend.h"

#include <cairo-quartz.h>

#import <Cocoa/Cocoa.h>

#include <assert.h>

@interface PuglCairoView : NSView
{
@public
	PuglView*        puglview;
	cairo_surface_t* surface;
	cairo_t*         cr;
}

@end

@implementation PuglCairoView

- (id) initWithFrame:(NSRect)frame
{
	return (self = [super initWithFrame:frame]);
}

- (void) resizeWithOldSuperviewSize:(NSSize)oldSize
{
	PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

	[super resizeWithOldSuperviewSize:oldSize];
	[wrapper setReshaped];
}

- (void) drawRect:(NSRect)rect
{
	PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];
	[wrapper dispatchExpose:rect];
}

@end

static PuglStatus
puglMacCairoConfigure(PuglView* PUGL_UNUSED(view))
{
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoCreate(PuglView* view)
{
	PuglInternals* impl     = view->impl;
	PuglCairoView* drawView = [PuglCairoView alloc];

	drawView->puglview = view;
	[drawView initWithFrame:NSMakeRect(0, 0, view->frame.width, view->frame.height)];
	if (view->hints[PUGL_RESIZABLE]) {
		[drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	} else {
		[drawView setAutoresizingMask:NSViewNotSizable];
	}

	impl->drawView = drawView;
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoDestroy(PuglView* view)
{
	PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;

	[drawView removeFromSuperview];
	[drawView release];

	view->impl->drawView = nil;
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoEnter(PuglView* view, bool drawing)
{
	PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;
	if (!drawing) {
		return PUGL_SUCCESS;
	}

	assert(!drawView->surface);
	assert(!drawView->cr);

	CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];

	drawView->surface = cairo_quartz_surface_create_for_cg_context(
		context, view->frame.width, view->frame.height);

	drawView->cr = cairo_create(drawView->surface);

	return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoLeave(PuglView* view, bool drawing)
{
	PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;
	if (!drawing) {
		return PUGL_SUCCESS;
	}

	assert(drawView->surface);
	assert(drawView->cr);

	CGContextRef context = cairo_quartz_surface_get_cg_context(drawView->surface);

	cairo_destroy(drawView->cr);
	cairo_surface_destroy(drawView->surface);
	drawView->cr      = NULL;
	drawView->surface = NULL;

	CGContextFlush(context);

	return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoResize(PuglView* PUGL_UNUSED(view),
                   int       PUGL_UNUSED(width),
                   int       PUGL_UNUSED(height))
{
	// No need to resize, the surface is created for the drawing context
	return PUGL_SUCCESS;
}

static void*
puglMacCairoGetContext(PuglView* view)
{
	return ((PuglCairoView*)view->impl->drawView)->cr;
}

const PuglBackend* puglCairoBackend(void)
{
	static const PuglBackend backend = {
		puglMacCairoConfigure,
		puglMacCairoCreate,
		puglMacCairoDestroy,
		puglMacCairoEnter,
		puglMacCairoLeave,
		puglMacCairoResize,
		puglMacCairoGetContext
	};

	return &backend;
}
