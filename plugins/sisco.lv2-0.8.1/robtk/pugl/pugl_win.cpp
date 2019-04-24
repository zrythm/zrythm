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
   @file pugl_win.cpp Windows/WGL Pugl Implementation.
*/

#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>

#include "pugl_internal.h"

#ifndef WM_MOUSEWHEEL
#    define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#    define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WHEEL_DELTA
#    define WHEEL_DELTA 120
#endif
#ifndef GWL_USERDATA
#    define GWL_USERDATA (-21)
#endif

const int LOCAL_CLOSE_MSG = WM_USER + 50;

struct PuglInternalsImpl {
	HWND     hwnd;
	HDC      hdc;
	HGLRC    hglrc;
	WNDCLASS wc;
	bool     keep_aspect;
	int      win_flags;
};

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              min_width,
           int              min_height,
           int              width,
           int              height,
           bool             resizable,
           bool             ontop,
           unsigned long    transientId)
{
	PuglView*      view = (PuglView*)calloc(1, sizeof(PuglView));
	PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));
	if (!view || !impl) {
		return NULL;
	}

	view->impl   = impl;
	view->width  = width;
	view->height = height;
	view->ontop  = ontop;
	view->user_resizable = resizable && !parent;
	view->impl->keep_aspect = min_width != width;

	// FIXME: This is nasty, and pugl should not have static anything.
	// Should class be a parameter?  Does this make sense on other platforms?
	static int wc_count = 0;
	int retry = 99;
	char classNameBuf[256];
	while (true) {
		_snprintf(classNameBuf, sizeof(classNameBuf), "x%d%s", wc_count++, title);
		classNameBuf[sizeof(classNameBuf)-1] = '\0';

		impl->wc.style         = CS_OWNDC;
		impl->wc.lpfnWndProc   = wndProc;
		impl->wc.cbClsExtra    = 0;
		impl->wc.cbWndExtra    = 0;
		impl->wc.hInstance     = 0;
		impl->wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		impl->wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		impl->wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		impl->wc.lpszMenuName  = NULL;
		impl->wc.lpszClassName = strdup(classNameBuf);

		if (RegisterClass(&impl->wc)) {
			break;
		}
		if (--retry > 0) {
			free((void*)impl->wc.lpszClassName);
			continue;
		}
		/* fail */
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return NULL;
	}

	if (parent) {
		view->impl->win_flags = WS_CHILD;
	} else {
		view->impl->win_flags = WS_POPUPWINDOW | WS_CAPTION | (view->user_resizable ? WS_SIZEBOX : 0);
	}

	// Adjust the overall window size to accomodate our requested client size
	RECT wr = { 0, 0, width, height };
	AdjustWindowRectEx(&wr, view->impl->win_flags, FALSE, WS_EX_TOPMOST);

	RECT mr = { 0, 0, min_width, min_height };
	AdjustWindowRectEx(&mr, view->impl->win_flags, FALSE, WS_EX_TOPMOST);
	view->min_width  = mr.right - mr.left;
	view->min_height = mr.bottom - mr.top;

	impl->hwnd = CreateWindowEx (
		WS_EX_TOPMOST,
		classNameBuf, title, (view->user_resizable ? WS_SIZEBOX : 0) |
		(parent ? (WS_CHILD | WS_VISIBLE) : (WS_POPUPWINDOW | WS_CAPTION)),
		0, 0, wr.right - wr.left, wr.bottom - wr.top,
		(HWND)parent, NULL, NULL, NULL);

	if (!impl->hwnd) {
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return NULL;
	}

	SetWindowLongPtr(impl->hwnd, GWL_USERDATA, (LONG_PTR)view);

	SetWindowPos (impl->hwnd,
			ontop ? HWND_TOPMOST : HWND_TOP,
			0, 0, 0, 0, (ontop ? 0 : SWP_NOACTIVATE) | SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);

	impl->hdc = GetDC(impl->hwnd);

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int format = ChoosePixelFormat(impl->hdc, &pfd);
	SetPixelFormat(impl->hdc, format, &pfd);

	impl->hglrc = wglCreateContext(impl->hdc);
	if (!impl->hglrc) {
		ReleaseDC (impl->hwnd, impl->hdc);
		DestroyWindow (impl->hwnd);
		UnregisterClass (impl->wc.lpszClassName, NULL);
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return NULL;
	}
	wglMakeCurrent(impl->hdc, impl->hglrc);

	view->width  = width;
	view->height = height;

	return view;
}

void
puglDestroy(PuglView* view)
{
	if (!view) {
		return;
	}
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(view->impl->hglrc);
	ReleaseDC(view->impl->hwnd, view->impl->hdc);
	DestroyWindow(view->impl->hwnd);
	UnregisterClass(view->impl->wc.lpszClassName, NULL);
	free((void*)view->impl->wc.lpszClassName);
	free(view->impl);
	free(view);
}

PUGL_API void
puglShowWindow(PuglView* view) {
	ShowWindow(view->impl->hwnd, WS_VISIBLE);
	ShowWindow(view->impl->hwnd, SW_RESTORE);
	//SetForegroundWindow(view->impl->hwnd);
	UpdateWindow(view->impl->hwnd);
}

PUGL_API void
puglHideWindow(PuglView* view) {
	ShowWindow(view->impl->hwnd, SW_HIDE);
	UpdateWindow(view->impl->hwnd);
}

static void
puglReshape(PuglView* view, int width, int height)
{
	wglMakeCurrent(view->impl->hdc, view->impl->hglrc);

	if (view->reshapeFunc) {
		view->reshapeFunc(view, width, height);
	} else {
		puglDefaultReshape(view, width, height);
	}

	wglMakeCurrent(NULL, NULL);

	view->width  = width;
	view->height = height;
}

static void
puglDisplay(PuglView* view)
{
	wglMakeCurrent(view->impl->hdc, view->impl->hglrc);
#if 0
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
#endif

	view->redisplay = false;
	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();
	SwapBuffers(view->impl->hdc);
	wglMakeCurrent(NULL, NULL);
}

static void
puglResize(PuglView* view)
{
	int set_hints; // ignored for now
	view->resize = false;
	if (!view->resizeFunc) { return; }
	/* ask the plugin about the new size */
	view->resizeFunc(view, &view->width, &view->height, &set_hints);

	HWND parent = GetParent (view->impl->hwnd);
	if (parent) {
		puglReshape(view, view->width, view->height);
		SetWindowPos (view->impl->hwnd, HWND_TOP,
				0, 0, view->width, view->height,
				SWP_NOZORDER | SWP_NOMOVE);
		return;
	}

	RECT wr = { 0, 0, (long)view->width, (long)view->height };
	AdjustWindowRectEx(&wr, view->impl->win_flags, FALSE, WS_EX_TOPMOST);
	SetWindowPos (view->impl->hwnd,
			view->ontop ? HWND_TOPMOST : HWND_NOTOPMOST,
			0, 0, wr.right-wr.left, wr.bottom-wr.top,
			SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER /*|SWP_NOZORDER*/);
	UpdateWindow(view->impl->hwnd);

	/* and call Reshape in GL context */
	puglReshape(view, view->width, view->height);
}

static PuglKey
keySymToSpecial(int sym)
{
	switch (sym) {
	case VK_F1:      return PUGL_KEY_F1;
	case VK_F2:      return PUGL_KEY_F2;
	case VK_F3:      return PUGL_KEY_F3;
	case VK_F4:      return PUGL_KEY_F4;
	case VK_F5:      return PUGL_KEY_F5;
	case VK_F6:      return PUGL_KEY_F6;
	case VK_F7:      return PUGL_KEY_F7;
	case VK_F8:      return PUGL_KEY_F8;
	case VK_F9:      return PUGL_KEY_F9;
	case VK_F10:     return PUGL_KEY_F10;
	case VK_F11:     return PUGL_KEY_F11;
	case VK_F12:     return PUGL_KEY_F12;
	case VK_LEFT:    return PUGL_KEY_LEFT;
	case VK_UP:      return PUGL_KEY_UP;
	case VK_RIGHT:   return PUGL_KEY_RIGHT;
	case VK_DOWN:    return PUGL_KEY_DOWN;
	case VK_PRIOR:   return PUGL_KEY_PAGE_UP;
	case VK_NEXT:    return PUGL_KEY_PAGE_DOWN;
	case VK_HOME:    return PUGL_KEY_HOME;
	case VK_END:     return PUGL_KEY_END;
	case VK_INSERT:  return PUGL_KEY_INSERT;
	case VK_SHIFT:   return PUGL_KEY_SHIFT;
	case VK_CONTROL: return PUGL_KEY_CTRL;
	case VK_MENU:    return PUGL_KEY_ALT;
	case VK_LWIN:    return PUGL_KEY_SUPER;
	case VK_RWIN:    return PUGL_KEY_SUPER;
	}
	return (PuglKey)0;
}

static void
processMouseEvent(PuglView* view, int button, bool press, LPARAM lParam)
{
	view->event_timestamp_ms = GetMessageTime();
	if (GetFocus() != view->impl->hwnd) {
		// focus is needed to receive mouse-wheel events
		SetFocus (view->impl->hwnd);
	}

	if (press) {
		SetCapture(view->impl->hwnd);
	} else {
		ReleaseCapture();
	}

	if (view->mouseFunc) {
		view->mouseFunc(view, button, press,
		                GET_X_LPARAM(lParam),
		                GET_Y_LPARAM(lParam));
	}
}

static void
setModifiers(PuglView* view)
{
	view->mods = 0;
	view->mods |= (GetKeyState(VK_SHIFT)   < 0) ? PUGL_MOD_SHIFT  : 0;
	view->mods |= (GetKeyState(VK_CONTROL) < 0) ? PUGL_MOD_CTRL   : 0;
	view->mods |= (GetKeyState(VK_MENU)    < 0) ? PUGL_MOD_ALT    : 0;
	view->mods |= (GetKeyState(VK_LWIN)    < 0) ? PUGL_MOD_SUPER  : 0;
	view->mods |= (GetKeyState(VK_RWIN)    < 0) ? PUGL_MOD_SUPER  : 0;
}

static LRESULT
handleMessage(PuglView* view, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	PuglKey     key;

	setModifiers(view);
	switch (message) {
	case WM_CREATE:
	case WM_SHOWWINDOW:
	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(view->impl->hwnd, &rect);
			puglReshape(view, rect.right, rect.bottom);
			view->width = rect.right;
			view->height = rect.bottom;
		}
		break;
	case WM_SIZING:
		if (view->impl->keep_aspect) {
			float aspect = view->min_width / (float)view->min_height;
			RECT* rect = (RECT*)lParam;
			switch ((int)wParam) {
				case WMSZ_LEFT:
				case WMSZ_RIGHT:
				case WMSZ_BOTTOMLEFT:
				case WMSZ_BOTTOMRIGHT:
					rect->bottom = rect->top + (rect->right - rect->left) / aspect;
					break;
				case WMSZ_TOP:
				case WMSZ_BOTTOM:
				case WMSZ_TOPRIGHT:
					rect->right = rect->left + (rect->bottom - rect->top) * aspect;
					break;
				case WMSZ_TOPLEFT:
					rect->left = rect->right - (rect->bottom - rect->top) * aspect;
					break;
			}
			return TRUE;
		}
		break;
	case WM_GETMINMAXINFO:
		{
		MINMAXINFO *mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = view->min_width;
		mmi->ptMinTrackSize.y = view->min_height;
		}
		break;
	case WM_PAINT:
		BeginPaint(view->impl->hwnd, &ps);
		puglDisplay(view);
		EndPaint(view->impl->hwnd, &ps);
		break;
	case WM_MOUSEMOVE:
		if (view->motionFunc) {
			view->motionFunc(view, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_LBUTTONDOWN:
		processMouseEvent(view, 1, true, lParam);
		break;
	case WM_MBUTTONDOWN:
		processMouseEvent(view, 2, true, lParam);
		break;
	case WM_RBUTTONDOWN:
		processMouseEvent(view, 3, true, lParam);
		break;
	case WM_LBUTTONUP:
		processMouseEvent(view, 1, false, lParam);
		break;
	case WM_MBUTTONUP:
		processMouseEvent(view, 2, false, lParam);
		break;
	case WM_RBUTTONUP:
		processMouseEvent(view, 3, false, lParam);
		break;
	case WM_MOUSEWHEEL:
		if (view->scrollFunc) {
			view->event_timestamp_ms = GetMessageTime();
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(view->impl->hwnd, &pt);
			view->scrollFunc(
				view, pt.x, pt.y,
				0, GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
		}
		break;
	case WM_MOUSEHWHEEL:
		if (view->scrollFunc) {
			view->event_timestamp_ms = GetMessageTime();
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(view->impl->hwnd, &pt);
			view->scrollFunc(
				view, pt.x, pt.y,
				GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0);
		}
		break;
	case WM_KEYDOWN:
		if (view->ignoreKeyRepeat && (lParam & (1 << 30))) {
			break;
		} // else nobreak
	case WM_KEYUP:
		view->event_timestamp_ms = GetMessageTime();
		if ((key = keySymToSpecial(wParam))) {
			if (view->specialFunc) {
				view->specialFunc(view, message == WM_KEYDOWN, key);
			}
		} else if (view->keyboardFunc) {
			static BYTE kbs[256];
			if (GetKeyboardState(kbs) != FALSE) {
				char lb[2];
				UINT scanCode = (lParam >> 8) & 0xFFFFFF00;
				if ( 1 == ToAscii(wParam, scanCode, kbs, (LPWORD)lb, 0)) {
					view->keyboardFunc(view, message == WM_KEYDOWN, (char)lb[0]);
				}
			}
		}
		break;
	case WM_QUIT:
	case LOCAL_CLOSE_MSG:
		if (view->closeFunc) {
			view->closeFunc(view);
		}
		break;
	default:
		return DefWindowProc(
			view->impl->hwnd, message, wParam, lParam);
	}

	return 0;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	MSG msg;
	while (PeekMessage(&msg, view->impl->hwnd, 0, 0, PM_REMOVE)) {
		handleMessage(view, msg.message, msg.wParam, msg.lParam);
	}

	if (view->resize) {
		puglResize(view);
	}

	if (view->redisplay) {
		InvalidateRect(view->impl->hwnd, NULL, FALSE);
	}

	return PUGL_SUCCESS;
}

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PuglView* view = (PuglView*)GetWindowLongPtr(hwnd, GWL_USERDATA);
	switch (message) {
	case WM_CREATE:
		PostMessage(hwnd, WM_SHOWWINDOW, TRUE, 0);
		return 0;
	case WM_CLOSE:
		PostMessage(hwnd, LOCAL_CLOSE_MSG, wParam, lParam);
		return 0;
	case WM_DESTROY:
		return 0;
	default:
		if (view && hwnd == view->impl->hwnd) {
			return handleMessage(view, message, wParam, lParam);
		} else {
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

void
puglPostResize(PuglView* view)
{
	view->resize = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->hwnd;
}

int
puglOpenFileDialog(PuglView* view, const char *title)
{
	char fn[1024] = "";
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFile = fn;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = title;
	ofn.lpstrFilter = "All\0*.*\0";
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = 0;
	ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_HIDEREADONLY | OFN_READONLY;

	// TODO look into async ofn.lpfnHook, OFN_ENABLEHOOK
	// UINT_PTR CALLBACK openFileHook(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	// yet it seems GetOpenFIleName itself won't return anyway.

	ofn.hwndOwner = view->impl->hwnd; // modal

	if (GetOpenFileName (&ofn)) {
		if (view->fileSelectedFunc) {
			view->fileSelectedFunc(view, fn);
		}
	} else {
		if (view->fileSelectedFunc) {
			view->fileSelectedFunc(view, NULL);
		}
	}
	return 0;
}

int
puglUpdateGeometryConstraints(PuglView* view, int min_width, int min_height, bool aspect)
{
	view->min_width = min_width;
	view->min_height = min_height;
	if (view->width < min_width || view->height < min_height) {
		if (!view->resize) {
			if (view->width < min_width) view->width = min_width;
			if (view->height < min_height) view->height = min_height;
			view->resize = true;
			// TODO: trigger resize
			// (not always needed since this fn is usually called in response to a resize)
		}
	}
	return 0;
}
