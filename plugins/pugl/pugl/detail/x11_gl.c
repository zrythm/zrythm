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
   @file x11_gl.c OpenGL graphics backend for X11.
*/

#include "pugl/detail/types.h"
#include "pugl/detail/x11.h"
#include "pugl/pugl.h"
#include "pugl/pugl_gl_backend.h"

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	GLXFBConfig fb_config;
	GLXContext  ctx;
	int         double_buffered;
} PuglX11GlSurface;

static PuglStatus
puglX11GlHintValue(const int value)
{
	return value == PUGL_DONT_CARE ? (int)GLX_DONT_CARE : value;
}

static PuglStatus
puglX11GlGetAttrib(Display* const    display,
                   const GLXFBConfig fb_config,
                   const int         attrib)
{
	int value = 0;
	glXGetFBConfigAttrib(display, fb_config, attrib, &value);
	return value;
}

static PuglStatus
puglX11GlConfigure(PuglView* view)
{
	PuglInternals* const impl    = view->impl;
	const int            screen  = impl->screen;
	Display* const       display = impl->display;

	PuglX11GlSurface* const surface =
		(PuglX11GlSurface*)calloc(1, sizeof(PuglX11GlSurface));
	impl->surface = surface;

	const int attrs[] = {
		GLX_X_RENDERABLE,  True,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_SAMPLES,       view->hints[PUGL_SAMPLES],
		GLX_RED_SIZE,      puglX11GlHintValue(view->hints[PUGL_RED_BITS]),
		GLX_GREEN_SIZE,    puglX11GlHintValue(view->hints[PUGL_GREEN_BITS]),
		GLX_BLUE_SIZE,     puglX11GlHintValue(view->hints[PUGL_BLUE_BITS]),
		GLX_ALPHA_SIZE,    puglX11GlHintValue(view->hints[PUGL_ALPHA_BITS]),
		GLX_DEPTH_SIZE,    puglX11GlHintValue(view->hints[PUGL_DEPTH_BITS]),
		GLX_STENCIL_SIZE,  puglX11GlHintValue(view->hints[PUGL_STENCIL_BITS]),
		GLX_DOUBLEBUFFER,  puglX11GlHintValue(view->hints[PUGL_DOUBLE_BUFFER]),
		None
	};

	int          n_fbc = 0;
	GLXFBConfig* fbc   = glXChooseFBConfig(display, screen, attrs, &n_fbc);
	if (n_fbc <= 0) {
		fprintf(stderr, "error: Failed to create GL context\n");
		return PUGL_CREATE_CONTEXT_FAILED;
	}

	surface->fb_config = fbc[0];
	impl->vi           = glXGetVisualFromFBConfig(impl->display, fbc[0]);

	printf("Using visual 0x%lX: R=%d G=%d B=%d A=%d D=%d"
	       " DOUBLE=%d SAMPLES=%d\n",
	       impl->vi->visualid,
	       puglX11GlGetAttrib(display, fbc[0], GLX_RED_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_GREEN_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_BLUE_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_ALPHA_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_DEPTH_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_DOUBLEBUFFER),
	       puglX11GlGetAttrib(display, fbc[0], GLX_SAMPLES));

	XFree(fbc);

	return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlCreate(PuglView* view)
{
	PuglInternals* const    impl      = view->impl;
	PuglX11GlSurface* const surface   = (PuglX11GlSurface*)impl->surface;
	Display* const          display   = impl->display;
	const GLXFBConfig       fb_config = surface->fb_config;

	const int ctx_attrs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, view->hints[PUGL_CONTEXT_VERSION_MAJOR],
		GLX_CONTEXT_MINOR_VERSION_ARB, view->hints[PUGL_CONTEXT_VERSION_MINOR],
		GLX_CONTEXT_FLAGS_ARB, (view->hints[PUGL_USE_DEBUG_CONTEXT]
		                        ? GLX_CONTEXT_DEBUG_BIT_ARB
		                        : 0),
		GLX_CONTEXT_PROFILE_MASK_ARB, (view->hints[PUGL_USE_COMPAT_PROFILE]
		                               ? GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
		                               : GLX_CONTEXT_CORE_PROFILE_BIT_ARB),
		0};

	typedef GLXContext (*CreateContextAttribs)(
		Display*, GLXFBConfig, GLXContext, Bool, const int*);

	CreateContextAttribs create_context =
		(CreateContextAttribs)glXGetProcAddress(
			(const GLubyte*)"glXCreateContextAttribsARB");

	surface->ctx = create_context(display, fb_config, 0, GL_TRUE, ctx_attrs);
	if (!surface->ctx) {
		surface->ctx =
			glXCreateNewContext(display, fb_config, GLX_RGBA_TYPE, 0, True);
	}

	if (!surface->ctx) {
		return PUGL_CREATE_CONTEXT_FAILED;
	}

	glXGetConfig(impl->display,
	             impl->vi,
	             GLX_DOUBLEBUFFER,
	             &surface->double_buffered);

	return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlDestroy(PuglView* view)
{
	PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;
	if (surface) {
		glXDestroyContext(view->impl->display, surface->ctx);
		free(surface);
		view->impl->surface = NULL;
	}
	return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlEnter(PuglView* view, bool PUGL_UNUSED(drawing))
{
	PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;
	glXMakeCurrent(view->impl->display, view->impl->win, surface->ctx);
	return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlLeave(PuglView* view, bool drawing)
{
	PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;

	if (drawing && surface->double_buffered) {
		glXSwapBuffers(view->impl->display, view->impl->win);
	} else if (drawing) {
		glFlush();
	}

	glXMakeCurrent(view->impl->display, None, NULL);

	return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlResize(PuglView* PUGL_UNUSED(view),
                int       PUGL_UNUSED(width),
                int       PUGL_UNUSED(height))
{
	return PUGL_SUCCESS;
}

static void*
puglX11GlGetContext(PuglView* PUGL_UNUSED(view))
{
	return NULL;
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
	return glXGetProcAddress((const GLubyte*)name);
}

const PuglBackend* puglGlBackend(void)
{
	static const PuglBackend backend = {
		puglX11GlConfigure,
		puglX11GlCreate,
		puglX11GlDestroy,
		puglX11GlEnter,
		puglX11GlLeave,
		puglX11GlResize,
		puglX11GlGetContext
	};

	return &backend;
}
