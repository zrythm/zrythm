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
   @file win_gl.c OpenGL graphics backend for Windows.
*/

#include "pugl/detail/types.h"
#include "pugl/detail/win.h"
#include "pugl/pugl_gl_backend.h"

#include <windows.h>

#include <GL/gl.h>

#include <stdbool.h>
#include <stdlib.h>

#define WGL_DRAW_TO_WINDOW_ARB    0x2001
#define WGL_ACCELERATION_ARB      0x2003
#define WGL_SUPPORT_OPENGL_ARB    0x2010
#define WGL_DOUBLE_BUFFER_ARB     0x2011
#define WGL_PIXEL_TYPE_ARB        0x2013
#define WGL_COLOR_BITS_ARB        0x2014
#define WGL_RED_BITS_ARB          0x2015
#define WGL_GREEN_BITS_ARB        0x2017
#define WGL_BLUE_BITS_ARB         0x2019
#define WGL_ALPHA_BITS_ARB        0x201b
#define WGL_DEPTH_BITS_ARB        0x2022
#define WGL_STENCIL_BITS_ARB      0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB         0x202b
#define WGL_SAMPLE_BUFFERS_ARB    0x2041
#define WGL_SAMPLES_ARB           0x2042

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB   0x2093
#define WGL_CONTEXT_FLAGS_ARB         0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB  0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x00000001

typedef HGLRC (*WglCreateContextAttribs)(HDC, HGLRC, const int*);
typedef BOOL (*WglSwapInterval)(int);
typedef BOOL (*WglChoosePixelFormat)(
	HDC, const int*, const FLOAT*, UINT, int*, UINT*);

typedef struct {
	WglChoosePixelFormat    wglChoosePixelFormat;
	WglCreateContextAttribs wglCreateContextAttribs;
	WglSwapInterval         wglSwapInterval;
} PuglWinGlProcs;

typedef struct {
	PuglWinGlProcs procs;
	HGLRC          hglrc;
} PuglWinGlSurface;

// Struct to manage the fake window used during configuration
typedef struct {
	HWND hwnd;
	HDC  hdc;
} PuglFakeWindow;

static PuglStatus
puglWinError(PuglFakeWindow* fakeWin, const PuglStatus status)
{
	if (fakeWin->hwnd) {
		ReleaseDC(fakeWin->hwnd, fakeWin->hdc);
		DestroyWindow(fakeWin->hwnd);
	}

	return status;
}

static PuglWinGlProcs puglWinGlGetProcs(void)
{
	const PuglWinGlProcs procs = {
		(WglChoosePixelFormat)(
			wglGetProcAddress("wglChoosePixelFormatARB")),
		(WglCreateContextAttribs)(
			wglGetProcAddress("wglCreateContextAttribsARB")),
		(WglSwapInterval)(
			wglGetProcAddress("wglSwapIntervalEXT"))
	};

	return procs;
}

static PuglStatus
puglWinGlConfigure(PuglView* view)
{
	PuglInternals* impl = view->impl;

	const int pixelAttrs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,  view->hints[PUGL_DOUBLE_BUFFER],
		WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
		WGL_SAMPLE_BUFFERS_ARB, view->hints[PUGL_SAMPLES] ? 1 : 0,
		WGL_SAMPLES_ARB,        view->hints[PUGL_SAMPLES],
		WGL_RED_BITS_ARB,       view->hints[PUGL_RED_BITS],
		WGL_GREEN_BITS_ARB,     view->hints[PUGL_GREEN_BITS],
		WGL_BLUE_BITS_ARB,      view->hints[PUGL_BLUE_BITS],
		WGL_ALPHA_BITS_ARB,     view->hints[PUGL_ALPHA_BITS],
		WGL_DEPTH_BITS_ARB,     view->hints[PUGL_DEPTH_BITS],
		WGL_STENCIL_BITS_ARB,   view->hints[PUGL_STENCIL_BITS],
		0,
	};

	PuglWinGlSurface* const surface =
		(PuglWinGlSurface*)calloc(1, sizeof(PuglWinGlSurface));
	impl->surface = surface;

	// Create fake window for getting at GL context
	PuglStatus     st      = PUGL_SUCCESS;
	PuglFakeWindow fakeWin = { 0, 0 };
	if ((st = puglWinCreateWindow(view, "Pugl Configuration",
	                              &fakeWin.hwnd, &fakeWin.hdc))) {
		return puglWinError(&fakeWin, st);
	}

	// Set pixel format for fake window
	const PuglWinPFD fakePfd  = puglWinGetPixelFormatDescriptor(view->hints);
	const int        fakePfId = ChoosePixelFormat(fakeWin.hdc, &fakePfd);
	if (!fakePfId) {
		return puglWinError(&fakeWin, PUGL_SET_FORMAT_FAILED);
	} else if (!SetPixelFormat(fakeWin.hdc, fakePfId, &fakePfd)) {
		return puglWinError(&fakeWin, PUGL_SET_FORMAT_FAILED);
	}

	// Create fake GL context to get at the functions we need
	HGLRC fakeRc = wglCreateContext(fakeWin.hdc);
	if (!fakeRc) {
		return puglWinError(&fakeWin, PUGL_CREATE_CONTEXT_FAILED);
	}

	// Enter fake context and get extension functions
	wglMakeCurrent(fakeWin.hdc, fakeRc);
	surface->procs = puglWinGlGetProcs();

	if (surface->procs.wglChoosePixelFormat) {
		// Choose pixel format based on attributes
		UINT numFormats = 0;
		if (!surface->procs.wglChoosePixelFormat(
			    fakeWin.hdc, pixelAttrs, NULL, 1u, &impl->pfId, &numFormats)) {
			return puglWinError(&fakeWin, PUGL_SET_FORMAT_FAILED);
		}

		DescribePixelFormat(
			impl->hdc, impl->pfId, sizeof(impl->pfd), &impl->pfd);
	} else {
		// Modern extensions not available, use basic pixel format
		impl->pfd  = fakePfd;
		impl->pfId = fakePfId;
	}

	// Dispose of fake window and context
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeRc);
	ReleaseDC(fakeWin.hwnd, fakeWin.hdc);
	DestroyWindow(fakeWin.hwnd);

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinGlCreate(PuglView* view)
{
	PuglInternals* const    impl    = view->impl;
	PuglWinGlSurface* const surface = (PuglWinGlSurface*)impl->surface;
	PuglStatus              st      = PUGL_SUCCESS;

	const int contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, view->hints[PUGL_CONTEXT_VERSION_MAJOR],
		WGL_CONTEXT_MINOR_VERSION_ARB, view->hints[PUGL_CONTEXT_VERSION_MINOR],
		WGL_CONTEXT_FLAGS_ARB, (view->hints[PUGL_USE_DEBUG_CONTEXT]
		                        ? WGL_CONTEXT_DEBUG_BIT_ARB
		                        : 0),
		(view->hints[PUGL_USE_COMPAT_PROFILE]
		 ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB
		 : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB),
		0
	};

	// Create real window with desired pixel format
	if ((st = puglWinCreateWindow(view, "Pugl", &impl->hwnd, &impl->hdc))) {
		return st;
	} else if (!SetPixelFormat(impl->hdc, impl->pfId, &impl->pfd)) {
		ReleaseDC(impl->hwnd, impl->hdc);
		DestroyWindow(impl->hwnd);
		impl->hwnd = NULL;
		impl->hdc  = NULL;
		return PUGL_SET_FORMAT_FAILED;
	}

	// Create GL context
	if (surface->procs.wglCreateContextAttribs &&
	    !(surface->hglrc = surface->procs.wglCreateContextAttribs(
		      impl->hdc, 0, contextAttribs))) {
		return PUGL_CREATE_CONTEXT_FAILED;
	} else if (!(surface->hglrc = wglCreateContext(impl->hdc))) {
		return PUGL_CREATE_CONTEXT_FAILED;
	}

	// Enter context and set swap interval
	wglMakeCurrent(impl->hdc, surface->hglrc);
	if (surface->procs.wglSwapInterval) {
		surface->procs.wglSwapInterval(view->hints[PUGL_SWAP_INTERVAL]);
	}

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinGlDestroy(PuglView* view)
{
	PuglWinGlSurface* surface = (PuglWinGlSurface*)view->impl->surface;
	if (surface) {
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(surface->hglrc);
		free(surface);
		view->impl->surface = NULL;
	}

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinGlEnter(PuglView* view, bool drawing)
{
	PuglWinGlSurface* surface = (PuglWinGlSurface*)view->impl->surface;

	wglMakeCurrent(view->impl->hdc, surface->hglrc);

	if (drawing) {
		PAINTSTRUCT ps;
		BeginPaint(view->impl->hwnd, &ps);
	}

	return PUGL_SUCCESS;
}

static PuglStatus
puglWinGlLeave(PuglView* view, bool drawing)
{
	if (drawing) {
		PAINTSTRUCT ps;
		EndPaint(view->impl->hwnd, &ps);
		SwapBuffers(view->impl->hdc);
	}

	wglMakeCurrent(NULL, NULL);
	return PUGL_SUCCESS;
}

static PuglStatus
puglWinGlResize(PuglView* PUGL_UNUSED(view),
                int       PUGL_UNUSED(width),
                int       PUGL_UNUSED(height))
{
	return PUGL_SUCCESS;
}

static void*
puglWinGlGetContext(PuglView* PUGL_UNUSED(view))
{
	return NULL;
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
	const PuglGlFunc func = (PuglGlFunc)wglGetProcAddress(name);

	/* Windows has the annoying property that wglGetProcAddress returns NULL
	   for functions from OpenGL 1.1, so we fall back to pulling them directly
	   from opengl32.dll */

	return func
		? func
		: (PuglGlFunc)GetProcAddress(GetModuleHandle("opengl32.dll"), name);
}

const PuglBackend*
puglGlBackend()
{
	static const PuglBackend backend = {
		puglWinGlConfigure,
		puglWinGlCreate,
		puglWinGlDestroy,
		puglWinGlEnter,
		puglWinGlLeave,
		puglWinGlResize,
		puglWinGlGetContext
	};

	return &backend;
}
