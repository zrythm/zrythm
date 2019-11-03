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
   @file win.h Shared definitions for Windows implementation.
*/

#include "pugl/detail/implementation.h"

#include <windows.h>

#include <stdbool.h>

typedef PIXELFORMATDESCRIPTOR PuglWinPFD;

struct PuglWorldInternalsImpl {
	double timerFrequency;
};

struct PuglInternalsImpl {
	PuglWinPFD   pfd;
	int          pfId;
	HWND         hwnd;
	HDC          hdc;
	PuglSurface* surface;
	DWORD        refreshRate;
	bool         flashing;
	bool         resizing;
	bool         mouseTracked;
};

static inline PuglWinPFD
puglWinGetPixelFormatDescriptor(const PuglHints hints)
{
	const int rgbBits = (hints[PUGL_RED_BITS] +
	                     hints[PUGL_GREEN_BITS] +
	                     hints[PUGL_BLUE_BITS]);

	PuglWinPFD pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize        = sizeof(pfd);
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;
	pfd.dwFlags     |= hints[PUGL_DOUBLE_BUFFER] ? PFD_DOUBLEBUFFER : 0;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cColorBits   = (BYTE)rgbBits;
	pfd.cRedBits     = (BYTE)hints[PUGL_RED_BITS];
	pfd.cGreenBits   = (BYTE)hints[PUGL_GREEN_BITS];
	pfd.cBlueBits    = (BYTE)hints[PUGL_BLUE_BITS];
	pfd.cAlphaBits   = (BYTE)hints[PUGL_ALPHA_BITS];
	pfd.cDepthBits   = (BYTE)hints[PUGL_DEPTH_BITS];
	pfd.cStencilBits = (BYTE)hints[PUGL_STENCIL_BITS];
	pfd.iLayerType   = PFD_MAIN_PLANE;
	return pfd;
}

static inline unsigned
puglWinGetWindowFlags(const PuglView* const view)
{
	const bool resizable = view->hints[PUGL_RESIZABLE];
	return (WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
	        (view->parent
	         ? WS_CHILD
	         : (WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX |
	            (resizable ? (WS_SIZEBOX | WS_MAXIMIZEBOX) : 0))));
}

static inline unsigned
puglWinGetWindowExFlags(const PuglView* const view)
{
	return WS_EX_NOINHERITLAYOUT | (view->parent ? 0u : WS_EX_APPWINDOW);
}

static inline PuglStatus
puglWinCreateWindow(const PuglView* const view,
                    const char* const     title,
                    HWND* const           hwnd,
                    HDC* const            hdc)
{
	const char*    className  = (const char*)view->world->className;
	const unsigned winFlags   = puglWinGetWindowFlags(view);
	const unsigned winExFlags = puglWinGetWindowExFlags(view);

	// Calculate total window size to accommodate requested view size
	RECT wr = { (long)view->frame.x, (long)view->frame.y,
	            (long)view->frame.width, (long)view->frame.height };
	AdjustWindowRectEx(&wr, winFlags, FALSE, winExFlags);

	// Create window and get drawing context
	if (!(*hwnd = CreateWindowEx(winExFlags, className, title, winFlags,
	                             CW_USEDEFAULT, CW_USEDEFAULT,
	                             wr.right-wr.left, wr.bottom-wr.top,
	                             (HWND)view->parent, NULL, NULL, NULL))) {
		return PUGL_CREATE_WINDOW_FAILED;
	} else if (!(*hdc = GetDC(*hwnd))) {
		DestroyWindow(*hwnd);
		*hwnd = NULL;
		return PUGL_CREATE_WINDOW_FAILED;
	}

	return PUGL_SUCCESS;
}
