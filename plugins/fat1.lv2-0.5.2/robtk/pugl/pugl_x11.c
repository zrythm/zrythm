/*
  Copyright 2012 David Robillard <http://drobilla.net>
  Copyright 2011-2012 Ben Loftis, Harrison Consoles
  Copyright 2013,2015 Robin Gareus <robin@gareus.org>

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
   @file pugl_x11.c X11 Pugl Implementation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "pugl_internal.h"

#ifdef WITH_SOFD
#define HAVE_X11
#include "../sofd/libsofd.h"
#include "../sofd/libsofd.c"
#endif

/* work around buggy re-parent & focus issues on some systems
 * where no keyboard events are passed through even if the
 * app has mouse-focus and all other events are working.
 */
//#define XKEYFOCUSGRAB

/* show messages during initalization
 */
//#define VERBOSE_PUGL

struct PuglInternalsImpl {
	Display*   display;
	int        screen;
	Window     win;
	GLXContext ctx;
	Bool       doubleBuffered;
};

/**
   Attributes for single-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListSgl[] = {
	GLX_RGBA,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	GLX_ARB_multisample, 1,
	None
};

/**
   Attributes for double-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListDbl[] = {
	GLX_RGBA, GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	GLX_ARB_multisample, 1,
	None
};

/**
   Attributes for double-buffered RGBA with multi-sampling
	 (antialiasing)
*/
static int attrListDblMS[] = {
	GLX_RGBA,
	GLX_DOUBLEBUFFER    , True,
	GLX_RED_SIZE        , 4,
	GLX_GREEN_SIZE      , 4,
	GLX_BLUE_SIZE       , 4,
	GLX_ALPHA_SIZE      , 4,
	GLX_DEPTH_SIZE      , 16,
	GLX_SAMPLE_BUFFERS  , 1,
	GLX_SAMPLES         , 4,
	None
};

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
		free(view);
		free(impl);
		return NULL;
	}

	view->impl   = impl;
	view->width  = width;
	view->height = height;
	view->ontop  = ontop;
	view->set_window_hints = true;
	view->user_resizable = resizable;

	impl->display = XOpenDisplay(0);
	if (!impl->display) {
		free(view);
		free(impl);
		return 0;
	}
	impl->screen  = DefaultScreen(impl->display);
	impl->doubleBuffered = True;

	XVisualInfo* vi = glXChooseVisual(impl->display, impl->screen, attrListDblMS);

	if (!vi) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListDbl);
#ifdef VERBOSE_PUGL
		printf("puGL: multisampling (antialiasing) is not available\n");
#endif
	}

	if (!vi) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListSgl);
		impl->doubleBuffered = False;
#ifdef VERBOSE_PUGL
		printf("puGL: singlebuffered rendering will be used, no doublebuffering available\n");
#endif
	}

	int glxMajor, glxMinor;
	glXQueryVersion(impl->display, &glxMajor, &glxMinor);
#ifdef VERBOSE_PUGL
	printf("puGL: GLX-Version : %d.%d\n", glxMajor, glxMinor);
#endif

	impl->ctx = glXCreateContext(impl->display, vi, 0, GL_TRUE);

	if (!impl->ctx) {
		free(view);
		free(impl);
		return 0;
	}

	Window xParent = parent
		? (Window)parent
		: RootWindow(impl->display, impl->screen);

	Colormap cmap = XCreateColormap(
		impl->display, xParent, vi->visual, AllocNone);

	XSetWindowAttributes attr;
	memset(&attr, 0, sizeof(XSetWindowAttributes));
	attr.colormap     = cmap;
	attr.border_pixel = 0;

	attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask
#ifdef XKEYFOCUSGRAB
		| EnterWindowMask
#endif
		| PointerMotionMask | StructureNotifyMask;

	impl->win = XCreateWindow(
		impl->display, xParent,
		0, 0, view->width, view->height, 0, vi->depth, InputOutput, vi->visual,
		CWBorderPixel | CWColormap | CWEventMask, &attr);

	if (!impl->win) {
		free(view);
		free(impl);
		return 0;
	}

	puglUpdateGeometryConstraints(view, min_width, min_height, min_width != width);
	XResizeWindow(view->impl->display, view->impl->win, width, height);

	if (title) {
		XStoreName(impl->display, impl->win, title);
	}

	if (!parent) {
		Atom wmDelete = XInternAtom(impl->display, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(impl->display, impl->win, &wmDelete, 1);
	}

	if (!parent && view->ontop) { /* TODO stay on top  */
		Atom type = XInternAtom(impl->display, "_NET_WM_STATE_ABOVE", False);
		XChangeProperty(impl->display, impl->win,
				XInternAtom(impl->display, "_NET_WM_STATE", False),
				XInternAtom(impl->display, "ATOM", False),
				32, PropModeReplace, (unsigned char *)&type, 1);
	}

	if (transientId > 0) {
		XSetTransientForHint(impl->display, impl->win, (Window)(transientId));
	}

	if (parent) {
		XMapRaised(impl->display, impl->win);
	}

	if (glXIsDirect(impl->display, impl->ctx)) {
#ifdef VERBOSE_PUGL
		printf("puGL: DRI enabled\n");
#endif
	} else {
#ifdef VERBOSE_PUGL
		printf("puGL: No DRI available\n");
#endif
	}

	XFree(vi);
	return view;
}

void
puglDestroy(PuglView* view)
{
	if (!view) {
		return;
	}
#ifdef WITH_SOFD
	x_fib_close(view->impl->display);
#endif

	//glXMakeCurrent(view->impl->display, None, NULL);
	glXDestroyContext(view->impl->display, view->impl->ctx);
	XDestroyWindow(view->impl->display, view->impl->win);
	XCloseDisplay(view->impl->display);
	free(view->impl);
	free(view);
	view = NULL;
}

PUGL_API void
puglShowWindow(PuglView* view) {
	XMapRaised(view->impl->display, view->impl->win);
}

PUGL_API void
puglHideWindow(PuglView* view) {
	XUnmapWindow(view->impl->display, view->impl->win);
}

static void
puglReshape(PuglView* view, int width, int height)
{
	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);

	if (view->reshapeFunc) {
		view->reshapeFunc(view, width, height);
	} else {
		puglDefaultReshape(view, width, height);
	}
	glXMakeCurrent(view->impl->display, None, NULL);

	view->width  = width;
	view->height = height;
}

static void
puglDisplay(PuglView* view)
{
	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);
#if 0
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
#endif

	view->redisplay = false;
	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();
	if (view->impl->doubleBuffered) {
		glXSwapBuffers(view->impl->display, view->impl->win);
	}
	glXMakeCurrent(view->impl->display, None, NULL);
}

static void
puglResize(PuglView* view)
{
	int set_hints = 1;
	view->resize = false;
	if (!view->resizeFunc) { return; }
	/* ask the plugin about the new size */
	view->resizeFunc(view, &view->width, &view->height, &set_hints);

	XSizeHints *hints = XAllocSizeHints();
	hints->min_width = view->width;
	hints->min_height = view->height;
	hints->max_width = view->user_resizable ? 2048 : view->width;
	hints->max_height = view->user_resizable ? 2048 : view->height;
	hints->flags = PMaxSize | PMinSize;

	if (set_hints) {
		XSetWMNormalHints(view->impl->display, view->impl->win, hints);
	}
	XResizeWindow(view->impl->display, view->impl->win, view->width, view->height);
	XFlush(view->impl->display);
	XFree(hints);

#ifdef VERBOSE_PUGL
	printf("puGL: window resize (%dx%d)\n", view->width, view->height);
#endif

	/* and call Reshape in glX context */
	puglReshape(view, view->width, view->height);
}

static PuglKey
keySymToSpecial(KeySym sym)
{
	switch (sym) {
	case XK_F1:        return PUGL_KEY_F1;
	case XK_F2:        return PUGL_KEY_F2;
	case XK_F3:        return PUGL_KEY_F3;
	case XK_F4:        return PUGL_KEY_F4;
	case XK_F5:        return PUGL_KEY_F5;
	case XK_F6:        return PUGL_KEY_F6;
	case XK_F7:        return PUGL_KEY_F7;
	case XK_F8:        return PUGL_KEY_F8;
	case XK_F9:        return PUGL_KEY_F9;
	case XK_F10:       return PUGL_KEY_F10;
	case XK_F11:       return PUGL_KEY_F11;
	case XK_F12:       return PUGL_KEY_F12;
	case XK_Left:      return PUGL_KEY_LEFT;
	case XK_Up:        return PUGL_KEY_UP;
	case XK_Right:     return PUGL_KEY_RIGHT;
	case XK_Down:      return PUGL_KEY_DOWN;
	case XK_Page_Up:   return PUGL_KEY_PAGE_UP;
	case XK_Page_Down: return PUGL_KEY_PAGE_DOWN;
	case XK_Home:      return PUGL_KEY_HOME;
	case XK_End:       return PUGL_KEY_END;
	case XK_Insert:    return PUGL_KEY_INSERT;
	case XK_Shift_L:   return PUGL_KEY_SHIFT;
	case XK_Shift_R:   return PUGL_KEY_SHIFT;
	case XK_Control_L: return PUGL_KEY_CTRL;
	case XK_Control_R: return PUGL_KEY_CTRL;
	case XK_Alt_L:     return PUGL_KEY_ALT;
	case XK_Alt_R:     return PUGL_KEY_ALT;
	case XK_Super_L:   return PUGL_KEY_SUPER;
	case XK_Super_R:   return PUGL_KEY_SUPER;
	}
	return (PuglKey)0;
}

static void
setModifiers(PuglView* view, unsigned xstate, unsigned xtime)
{
	view->event_timestamp_ms = xtime;

	view->mods = 0;
	view->mods |= (xstate & ShiftMask)   ? PUGL_MOD_SHIFT  : 0;
	view->mods |= (xstate & ControlMask) ? PUGL_MOD_CTRL   : 0;
	view->mods |= (xstate & Mod1Mask)    ? PUGL_MOD_ALT    : 0;
	view->mods |= (xstate & Mod4Mask)    ? PUGL_MOD_SUPER  : 0;
}

Window x_fib_window();

PuglStatus
puglProcessEvents(PuglView* view)
{
	XEvent event;
	while (XPending(view->impl->display) > 0) {
		XNextEvent(view->impl->display, &event);

#ifdef WITH_SOFD
		if (x_fib_handle_events(view->impl->display, &event)) {
			const int status = x_fib_status();

			if (status > 0) {
				char* const filename = x_fib_filename();
				x_fib_close(view->impl->display);
				x_fib_add_recent (filename, time(NULL));
				//x_fib_save_recent ("~/.robtk.recent");
				if (view->fileSelectedFunc) {
					view->fileSelectedFunc(view, filename);
				}
				free(filename);
				x_fib_free_recent ();
			} else if (status < 0) {
				x_fib_close(view->impl->display);
				if (view->fileSelectedFunc) {
					view->fileSelectedFunc(view, NULL);
				}
			}
		}
#endif

		if (event.xany.window != view->impl->win) {
			continue;
		}

		switch (event.type) {
		case UnmapNotify:
			if (view->motionFunc) {
				view->motionFunc(view, -1, -1);
			}
			break;
		case MapNotify:
			puglReshape(view, view->width, view->height);
			break;
		case ConfigureNotify:
			if ((event.xconfigure.width != view->width) ||
			    (event.xconfigure.height != view->height)) {
				puglReshape(view,
				            event.xconfigure.width,
				            event.xconfigure.height);
			}
			break;
		case Expose:
			if (event.xexpose.count != 0) {
				break;
			}
			puglDisplay(view);
			break;
		case MotionNotify:
			setModifiers(view, event.xmotion.state, event.xmotion.time);
			if (view->motionFunc) {
				view->motionFunc(view, event.xmotion.x, event.xmotion.y);
			}
			break;
		case ButtonPress:
			setModifiers(view, event.xbutton.state, event.xbutton.time);
			if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
				if (view->scrollFunc) {
					float dx = 0, dy = 0;
					switch (event.xbutton.button) {
					case 4: dy =  1.0f; break;
					case 5: dy = -1.0f; break;
					case 6: dx = -1.0f; break;
					case 7: dx =  1.0f; break;
					}
					view->scrollFunc(view, event.xbutton.x, event.xbutton.y, dx, dy);
				}
				break;
			}
			// nobreak
		case ButtonRelease:
			setModifiers(view, event.xbutton.state, event.xbutton.time);
			if (view->mouseFunc &&
			    (event.xbutton.button < 4 || event.xbutton.button > 7)) {
				view->mouseFunc(view,
				                event.xbutton.button, event.type == ButtonPress,
				                event.xbutton.x, event.xbutton.y);
			}
			break;
		case KeyPress: {
			setModifiers(view, event.xkey.state, event.xkey.time);
			KeySym  sym;
			char    str[5];
			int     n   = XLookupString(&event.xkey, str, 4, &sym, NULL);
			PuglKey key = keySymToSpecial(sym);
			if (!key && view->keyboardFunc) {
				if (n == 1) {
					view->keyboardFunc(view, true, str[0]);
				} else {
					fprintf(stderr, "warning: Unknown key %X\n", (int)sym);
				}
			} else if (view->specialFunc) {
				view->specialFunc(view, true, key);
			}
		} break;
		case KeyRelease: {
			setModifiers(view, event.xkey.state, event.xkey.time);
			bool repeated = false;
			if (view->ignoreKeyRepeat &&
			    XEventsQueued(view->impl->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(view->impl->display, &next);
				if (next.type == KeyPress &&
				    next.xkey.time == event.xkey.time &&
				    next.xkey.keycode == event.xkey.keycode) {
					XNextEvent(view->impl->display, &event);
					repeated = true;
				}
			}

			if (!repeated) {
				KeySym sym = XLookupKeysym(&event.xkey, 0);
				PuglKey special = keySymToSpecial(sym);
#if 0 // close on 'Esc'
				if (sym == XK_Escape && view->closeFunc) {
					view->closeFunc(view);
					view->redisplay = false;
				} else
#endif
				if (view->keyboardFunc) {
					if (!special) {
						view->keyboardFunc(view, false, sym);
					} else if (view->specialFunc) {
						view->specialFunc(view, false, special);
					}
				}
			}
		} break;
		case ClientMessage: {
			char* type = XGetAtomName(view->impl->display,
			                          event.xclient.message_type);
			if (!strcmp(type, "WM_PROTOCOLS")) {
				if (view->closeFunc) {
					view->closeFunc(view);
					view->redisplay = false;
				}
			}
			XFree(type);
		} break;
#ifdef XKEYFOCUSGRAB
		case EnterNotify:
			XSetInputFocus(view->impl->display, view->impl->win, RevertToPointerRoot, CurrentTime);
			break;
#endif
		default:
			break;
		}
	}

	if (view->resize) {
		puglResize(view);
	}

	if (view->redisplay) {
		puglDisplay(view);
	}

	return PUGL_SUCCESS;
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
	return view->impl->win;
}

int
puglOpenFileDialog(PuglView* view, const char *title)
{
#ifdef WITH_SOFD
	//x_fib_cfg_filter_callback (fib_filter_movie_filename);
	if (x_fib_configure (1, title)) {
		return -1;
	}
	//x_fib_load_recent ("~/.robtk.recent");
	if (x_fib_show (view->impl->display, view->impl->win, 300, 300)) {
		return -1;
	}
	return 0;
#else
	return -1;
#endif
}

int
puglUpdateGeometryConstraints(PuglView* view, int min_width, int min_height, bool aspect)
{
	if (!view->set_window_hints) {
		return -1;
	}
	XSizeHints sizeHints;
	memset(&sizeHints, 0, sizeof(sizeHints));
	sizeHints.flags      = PMinSize|PMaxSize;
	sizeHints.min_width  = min_width;
	sizeHints.min_height = min_height;
	sizeHints.max_width  = view->user_resizable ? 2048 : min_width;
	sizeHints.max_height = view->user_resizable ? 2048 : min_height;
	if (aspect) {
		sizeHints.flags |= PAspect;
		sizeHints.min_aspect.x=min_width;
		sizeHints.min_aspect.y=min_height;
		sizeHints.max_aspect.x=min_width;
		sizeHints.max_aspect.y=min_height;
	}
	XSetNormalHints(view->impl->display, view->impl->win, &sizeHints);
	return 0;
}
