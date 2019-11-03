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
   @file mac_gl.m OpenGL graphics backend for MacOS.
*/

#include "pugl/detail/implementation.h"
#include "pugl/detail/mac.h"
#include "pugl/pugl_gl_backend.h"

#ifndef __MAC_10_10
#define NSOpenGLProfileVersion4_1Core NSOpenGLProfileVersion3_2Core
typedef NSUInteger NSEventModifierFlags;
typedef NSUInteger NSWindowStyleMask;
#endif

@interface PuglOpenGLView : NSOpenGLView
{
@public
	PuglView* puglview;
}

@end

@implementation PuglOpenGLView

- (id) initWithFrame:(NSRect)frame
{
	const bool compat  = puglview->hints[PUGL_USE_COMPAT_PROFILE];
	const int  samples = puglview->hints[PUGL_SAMPLES];
	const int  major   = puglview->hints[PUGL_CONTEXT_VERSION_MAJOR];
	const int  profile = ((compat || major < 3)
	                      ? NSOpenGLProfileVersionLegacy
	                      : (major >= 4
	                         ? NSOpenGLProfileVersion4_1Core
	                         : NSOpenGLProfileVersion3_2Core));

	NSOpenGLPixelFormatAttribute pixelAttribs[16] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAOpenGLProfile, profile,
		NSOpenGLPFAColorSize,     32,
		NSOpenGLPFADepthSize,     32,
		NSOpenGLPFAMultisample,   samples ? 1 : 0,
		NSOpenGLPFASampleBuffers, samples ? 1 : 0,
		NSOpenGLPFASamples,       samples,
		0};

	NSOpenGLPixelFormat* pixelFormat = [
		[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttribs];

	if (pixelFormat) {
		self = [super initWithFrame:frame pixelFormat:pixelFormat];
		[pixelFormat release];
	} else {
		self = [super initWithFrame:frame];
	}

	if (self) {
		[[self openGLContext] makeCurrentContext];
		[self reshape];
	}
	return self;
}

- (void) reshape
{
	PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

	[super reshape];
	[wrapper setReshaped];
}

- (void) drawRect:(NSRect)rect
{
	PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];
	[wrapper dispatchExpose:rect];
}

@end

static PuglStatus
puglMacGlConfigure(PuglView* PUGL_UNUSED(view))
{
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlCreate(PuglView* view)
{
	PuglInternals*  impl     = view->impl;
	PuglOpenGLView* drawView = [PuglOpenGLView alloc];
	const NSRect    rect     = NSMakeRect(
		0, 0, view->frame.width, view->frame.height);

	drawView->puglview = view;
	[drawView initWithFrame:rect];
	if (view->hints[PUGL_RESIZABLE]) {
		[drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	} else {
		[drawView setAutoresizingMask:NSViewNotSizable];
	}

	impl->drawView = drawView;
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlDestroy(PuglView* view)
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[drawView removeFromSuperview];
	[drawView release];

	view->impl->drawView = nil;
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlEnter(PuglView* view, bool PUGL_UNUSED(drawing))
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[[drawView openGLContext] makeCurrentContext];
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlLeave(PuglView* view, bool drawing)
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	if (drawing) {
		[[drawView openGLContext] flushBuffer];
	}

	[NSOpenGLContext clearCurrentContext];

	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlResize(PuglView* view, int PUGL_UNUSED(width), int PUGL_UNUSED(height))
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[drawView reshape];

	return PUGL_SUCCESS;
}

static void*
puglMacGlGetContext(PuglView* PUGL_UNUSED(view))
{
	return NULL;
}

const PuglBackend* puglGlBackend(void)
{
	static const PuglBackend backend = {
		puglMacGlConfigure,
		puglMacGlCreate,
		puglMacGlDestroy,
		puglMacGlEnter,
		puglMacGlLeave,
		puglMacGlResize,
		puglMacGlGetContext
	};

	return &backend;
}
