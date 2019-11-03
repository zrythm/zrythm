/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>
  Copyright 2013 Robin Gareus <robin@gareus.org>
  Copyright 2011-2012 Ben Loftis, Harrison Consoles

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
   @file x11.c X11 implementation.
*/

#define _POSIX_C_SOURCE 199309L

#include "pugl/detail/implementation.h"
#include "pugl/detail/types.h"
#include "pugl/detail/x11.h"
#include "pugl/pugl.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <sys/select.h>
#include <sys/time.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

enum WmClientStateMessageAction {
	WM_STATE_REMOVE,
	WM_STATE_ADD,
	WM_STATE_TOGGLE
};

static const long eventMask =
	(ExposureMask | StructureNotifyMask | FocusChangeMask |
	 EnterWindowMask | LeaveWindowMask | PointerMotionMask |
	 ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask);

PuglWorldInternals*
puglInitWorldInternals(void)
{
	Display* display = XOpenDisplay(NULL);
	if (!display) {
		return NULL;
	}

	PuglWorldInternals* impl = (PuglWorldInternals*)calloc(
		1, sizeof(PuglWorldInternals));

	impl->display = display;

	// Intern the various atoms we will need
	impl->atoms.CLIPBOARD        = XInternAtom(display, "CLIPBOARD", 0);
	impl->atoms.UTF8_STRING      = XInternAtom(display, "UTF8_STRING", 0);
	impl->atoms.WM_PROTOCOLS     = XInternAtom(display, "WM_PROTOCOLS", 0);
	impl->atoms.WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	impl->atoms.NET_WM_NAME      = XInternAtom(display, "_NET_WM_NAME", 0);
	impl->atoms.NET_WM_STATE     = XInternAtom(display, "_NET_WM_STATE", 0);
	impl->atoms.NET_WM_STATE_DEMANDS_ATTENTION =
		XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);

	// Open input method
	XSetLocaleModifiers("");
	if (!(impl->xim = XOpenIM(display, NULL, NULL, NULL))) {
		XSetLocaleModifiers("@im=");
		if (!(impl->xim = XOpenIM(display, NULL, NULL, NULL))) {
			fprintf(stderr, "warning: XOpenIM failed\n");
		}
	}

	XFlush(display);

	return impl;
}

PuglInternals*
puglInitViewInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

PuglStatus
puglPollEvents(PuglWorld* world, const double timeout)
{
	XFlush(world->impl->display);

	const int fd   = ConnectionNumber(world->impl->display);
	const int nfds = fd + 1;
	int       ret  = 0;
	fd_set    fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if (timeout < 0.0) {
		ret = select(nfds, &fds, NULL, NULL, NULL);
	} else {
		const long     sec  = (long)timeout;
		const long     msec = (long)((timeout - (double)sec) * 1e6);
		struct timeval tv   = {sec, msec};
		ret = select(nfds, &fds, NULL, NULL, &tv);
	}

	return ret < 0 ? PUGL_UNKNOWN_ERROR
	               : ret == 0 ? PUGL_FAILURE : PUGL_SUCCESS;
}

static PuglView*
puglFindView(PuglWorld* world, const Window window)
{
	for (size_t i = 0; i < world->numViews; ++i) {
		if (world->views[i]->impl->win == window) {
			return world->views[i];
		}
	}

	return NULL;
}

static XSizeHints
getSizeHints(const PuglView* view)
{
	XSizeHints sizeHints = {0};

	if (!view->hints[PUGL_RESIZABLE]) {
		sizeHints.flags      = PMinSize|PMaxSize;
		sizeHints.min_width  = (int)view->frame.width;
		sizeHints.min_height = (int)view->frame.height;
		sizeHints.max_width  = (int)view->frame.width;
		sizeHints.max_height = (int)view->frame.height;
	} else {
		if (view->minWidth || view->minHeight) {
			sizeHints.flags      = PMinSize;
			sizeHints.min_width  = view->minWidth;
			sizeHints.min_height = view->minHeight;
		}
		if (view->minAspectX) {
			sizeHints.flags        |= PAspect;
			sizeHints.min_aspect.x  = view->minAspectX;
			sizeHints.min_aspect.y  = view->minAspectY;
			sizeHints.max_aspect.x  = view->maxAspectX;
			sizeHints.max_aspect.y  = view->maxAspectY;
		}
	}

	return sizeHints;
}

PuglStatus
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* const impl    = view->impl;
	PuglWorld* const     world   = view->world;
	PuglX11Atoms* const  atoms   = &view->world->impl->atoms;
	Display* const       display = world->impl->display;
	const int            width   = (int)view->frame.width;
	const int            height  = (int)view->frame.height;

	impl->display = display;
	impl->screen  = DefaultScreen(display);

	if (!view->backend || !view->backend->configure) {
		return PUGL_BAD_BACKEND;
	}

	PuglStatus st = view->backend->configure(view);
	if (st || !impl->vi) {
		view->backend->destroy(view);
		return st ? st : PUGL_BACKEND_FAILED;
	}

	Window xParent = view->parent ? (Window)view->parent
	                              : RootWindow(display, impl->screen);

	Colormap cmap = XCreateColormap(
		display, xParent, impl->vi->visual, AllocNone);

	XSetWindowAttributes attr = {0};
	attr.colormap   = cmap;
	attr.event_mask = eventMask;

	const Window win = impl->win = XCreateWindow(
		display, xParent,
		(int)view->frame.x, (int)view->frame.y, width, height,
		0, impl->vi->depth, InputOutput,
		impl->vi->visual, CWColormap | CWEventMask, &attr);

	if ((st = view->backend->create(view))) {
		return st;
	}

	XSizeHints sizeHints = getSizeHints(view);
	XSetNormalHints(display, win, &sizeHints);

	XClassHint classHint = { world->className, world->className };
	XSetClassHint(display, win, &classHint);

	if (title) {
		puglSetWindowTitle(view, title);
	}

	if (!view->parent) {
		XSetWMProtocols(display, win, &atoms->WM_DELETE_WINDOW, 1);
	}

	if (view->transientParent) {
		XSetTransientForHint(display, win, (Window)(view->transientParent));
	}

	// Create input context
	const XIMStyle im_style = XIMPreeditNothing | XIMStatusNothing;
	if (!(impl->xic = XCreateIC(world->impl->xim,
	                            XNInputStyle,   im_style,
	                            XNClientWindow, win,
	                            XNFocusWindow,  win,
	                            NULL))) {
		fprintf(stderr, "warning: XCreateIC failed\n");
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglShowWindow(PuglView* view)
{
	XMapRaised(view->impl->display, view->impl->win);
	puglPostRedisplay(view);
	view->visible = true;
	return PUGL_SUCCESS;
}

PuglStatus
puglHideWindow(PuglView* view)
{
	XUnmapWindow(view->impl->display, view->impl->win);
	view->visible = false;
	return PUGL_SUCCESS;
}

void
puglFreeViewInternals(PuglView* view)
{
	if (view) {
		if (view->impl->xic) {
			XDestroyIC(view->impl->xic);
		}
		view->backend->destroy(view);
		XDestroyWindow(view->impl->display, view->impl->win);
		XFree(view->impl->vi);
		free(view->impl);
	}
}

void
puglFreeWorldInternals(PuglWorld* world)
{
	if (world->impl->xim) {
		XCloseIM(world->impl->xim);
	}
	XCloseDisplay(world->impl->display);
	free(world->impl);
}

static PuglKey
keySymToSpecial(KeySym sym)
{
	switch (sym) {
	case XK_F1:          return PUGL_KEY_F1;
	case XK_F2:          return PUGL_KEY_F2;
	case XK_F3:          return PUGL_KEY_F3;
	case XK_F4:          return PUGL_KEY_F4;
	case XK_F5:          return PUGL_KEY_F5;
	case XK_F6:          return PUGL_KEY_F6;
	case XK_F7:          return PUGL_KEY_F7;
	case XK_F8:          return PUGL_KEY_F8;
	case XK_F9:          return PUGL_KEY_F9;
	case XK_F10:         return PUGL_KEY_F10;
	case XK_F11:         return PUGL_KEY_F11;
	case XK_F12:         return PUGL_KEY_F12;
	case XK_Left:        return PUGL_KEY_LEFT;
	case XK_Up:          return PUGL_KEY_UP;
	case XK_Right:       return PUGL_KEY_RIGHT;
	case XK_Down:        return PUGL_KEY_DOWN;
	case XK_Page_Up:     return PUGL_KEY_PAGE_UP;
	case XK_Page_Down:   return PUGL_KEY_PAGE_DOWN;
	case XK_Home:        return PUGL_KEY_HOME;
	case XK_End:         return PUGL_KEY_END;
	case XK_Insert:      return PUGL_KEY_INSERT;
	case XK_Shift_L:     return PUGL_KEY_SHIFT_L;
	case XK_Shift_R:     return PUGL_KEY_SHIFT_R;
	case XK_Control_L:   return PUGL_KEY_CTRL_L;
	case XK_Control_R:   return PUGL_KEY_CTRL_R;
	case XK_Alt_L:       return PUGL_KEY_ALT_L;
	case XK_ISO_Level3_Shift:
	case XK_Alt_R:       return PUGL_KEY_ALT_R;
	case XK_Super_L:     return PUGL_KEY_SUPER_L;
	case XK_Super_R:     return PUGL_KEY_SUPER_R;
	case XK_Menu:        return PUGL_KEY_MENU;
	case XK_Caps_Lock:   return PUGL_KEY_CAPS_LOCK;
	case XK_Scroll_Lock: return PUGL_KEY_SCROLL_LOCK;
	case XK_Num_Lock:    return PUGL_KEY_NUM_LOCK;
	case XK_Print:       return PUGL_KEY_PRINT_SCREEN;
	case XK_Pause:       return PUGL_KEY_PAUSE;
	default: break;
	}
	return (PuglKey)0;
}

static int
lookupString(XIC xic, XEvent* xevent, char* str, KeySym* sym)
{
	Status status = 0;

#ifdef X_HAVE_UTF8_STRING
	const int n = Xutf8LookupString(xic, &xevent->xkey, str, 7, sym, &status);
#else
	const int n = XmbLookupString(xic, &xevent->xkey, str, 7, sym, &status);
#endif

	return status == XBufferOverflow ? 0 : n;
}

static void
translateKey(PuglView* view, XEvent* xevent, PuglEvent* event)
{
	const unsigned state  = xevent->xkey.state;
	const bool     filter = XFilterEvent(xevent, None);

	event->key.keycode = xevent->xkey.keycode;
	xevent->xkey.state = 0;

	// Lookup unshifted key
	char          ustr[8] = {0};
	KeySym        sym     = 0;
	const int     ufound  = XLookupString(&xevent->xkey, ustr, 8, &sym, NULL);
	const PuglKey special = keySymToSpecial(sym);

	event->key.key = ((special || ufound <= 0)
	                  ? special
	                  : puglDecodeUTF8((const uint8_t*)ustr));

	if (xevent->type == KeyPress && !filter && !special) {
		// Lookup shifted key for possible text event
		xevent->xkey.state = state;

		char      sstr[8] = {0};
		const int sfound  = lookupString(view->impl->xic, xevent, sstr, &sym);
		if (sfound > 0) {
			// Dispatch key event now
			puglDispatchEvent(view, event);

			// "Return" a text event in its place
			event->text.type      = PUGL_TEXT;
			event->text.character = puglDecodeUTF8((const uint8_t*)sstr);
			memcpy(event->text.string, sstr, sizeof(sstr));
		}
	}
}

static uint32_t
translateModifiers(const unsigned xstate)
{
	return (((xstate & ShiftMask)   ? PUGL_MOD_SHIFT  : 0) |
	        ((xstate & ControlMask) ? PUGL_MOD_CTRL   : 0) |
	        ((xstate & Mod1Mask)    ? PUGL_MOD_ALT    : 0) |
	        ((xstate & Mod4Mask)    ? PUGL_MOD_SUPER  : 0));
}

static PuglEvent
translateEvent(PuglView* view, XEvent xevent)
{
	const PuglX11Atoms* atoms = &view->world->impl->atoms;

	PuglEvent event = {0};
	event.any.flags = xevent.xany.send_event ? PUGL_IS_SEND_EVENT : 0;

	switch (xevent.type) {
	case ClientMessage:
		if (xevent.xclient.message_type == atoms->WM_PROTOCOLS) {
			const Atom protocol = (Atom)xevent.xclient.data.l[0];
			if (protocol == atoms->WM_DELETE_WINDOW) {
				event.type = PUGL_CLOSE;
			}
		}
		break;
	case MapNotify: {
		XWindowAttributes attrs = {0};
		XGetWindowAttributes(view->impl->display, view->impl->win, &attrs);
		event.type             = PUGL_CONFIGURE;
		event.configure.x      = attrs.x;
		event.configure.y      = attrs.y;
		event.configure.width  = attrs.width;
		event.configure.height = attrs.height;
		break;
	}
	case ConfigureNotify:
		event.type             = PUGL_CONFIGURE;
		event.configure.x      = xevent.xconfigure.x;
		event.configure.y      = xevent.xconfigure.y;
		event.configure.width  = xevent.xconfigure.width;
		event.configure.height = xevent.xconfigure.height;
		break;
	case Expose:
		event.type          = PUGL_EXPOSE;
		event.expose.x      = xevent.xexpose.x;
		event.expose.y      = xevent.xexpose.y;
		event.expose.width  = xevent.xexpose.width;
		event.expose.height = xevent.xexpose.height;
		event.expose.count  = xevent.xexpose.count;
		break;
	case MotionNotify:
		event.type           = PUGL_MOTION_NOTIFY;
		event.motion.time    = xevent.xmotion.time / 1e3;
		event.motion.x       = xevent.xmotion.x;
		event.motion.y       = xevent.xmotion.y;
		event.motion.xRoot   = xevent.xmotion.x_root;
		event.motion.yRoot   = xevent.xmotion.y_root;
		event.motion.state   = translateModifiers(xevent.xmotion.state);
		event.motion.isHint  = (xevent.xmotion.is_hint == NotifyHint);
		break;
	case ButtonPress:
		if (xevent.xbutton.button >= 4 && xevent.xbutton.button <= 7) {
			event.type           = PUGL_SCROLL;
			event.scroll.time    = xevent.xbutton.time / 1e3;
			event.scroll.x       = xevent.xbutton.x;
			event.scroll.y       = xevent.xbutton.y;
			event.scroll.xRoot   = xevent.xbutton.x_root;
			event.scroll.yRoot   = xevent.xbutton.y_root;
			event.scroll.state   = translateModifiers(xevent.xbutton.state);
			event.scroll.dx      = 0.0;
			event.scroll.dy      = 0.0;
			switch (xevent.xbutton.button) {
			case 4: event.scroll.dy =  1.0; break;
			case 5: event.scroll.dy = -1.0; break;
			case 6: event.scroll.dx = -1.0; break;
			case 7: event.scroll.dx =  1.0; break;
			}
			// fallthru
		}
		// fallthru
	case ButtonRelease:
		if (xevent.xbutton.button < 4 || xevent.xbutton.button > 7) {
			event.button.type   = ((xevent.type == ButtonPress)
			                       ? PUGL_BUTTON_PRESS
			                       : PUGL_BUTTON_RELEASE);
			event.button.time   = xevent.xbutton.time / 1e3;
			event.button.x      = xevent.xbutton.x;
			event.button.y      = xevent.xbutton.y;
			event.button.xRoot  = xevent.xbutton.x_root;
			event.button.yRoot  = xevent.xbutton.y_root;
			event.button.state  = translateModifiers(xevent.xbutton.state);
			event.button.button = xevent.xbutton.button;
		}
		break;
	case KeyPress:
	case KeyRelease:
		event.type       = ((xevent.type == KeyPress)
		                    ? PUGL_KEY_PRESS
		                    : PUGL_KEY_RELEASE);
		event.key.time   = xevent.xkey.time / 1e3;
		event.key.x      = xevent.xkey.x;
		event.key.y      = xevent.xkey.y;
		event.key.xRoot  = xevent.xkey.x_root;
		event.key.yRoot  = xevent.xkey.y_root;
		event.key.state  = translateModifiers(xevent.xkey.state);
		translateKey(view, &xevent, &event);
		break;
	case EnterNotify:
	case LeaveNotify:
		event.type            = ((xevent.type == EnterNotify)
		                         ? PUGL_ENTER_NOTIFY
		                         : PUGL_LEAVE_NOTIFY);
		event.crossing.time   = xevent.xcrossing.time / 1e3;
		event.crossing.x      = xevent.xcrossing.x;
		event.crossing.y      = xevent.xcrossing.y;
		event.crossing.xRoot  = xevent.xcrossing.x_root;
		event.crossing.yRoot  = xevent.xcrossing.y_root;
		event.crossing.state  = translateModifiers(xevent.xcrossing.state);
		event.crossing.mode   = PUGL_CROSSING_NORMAL;
		if (xevent.xcrossing.mode == NotifyGrab) {
			event.crossing.mode = PUGL_CROSSING_GRAB;
		} else if (xevent.xcrossing.mode == NotifyUngrab) {
			event.crossing.mode = PUGL_CROSSING_UNGRAB;
		}
		break;

	case FocusIn:
	case FocusOut:
		event.type = (xevent.type == FocusIn) ? PUGL_FOCUS_IN : PUGL_FOCUS_OUT;
		event.focus.grab = (xevent.xfocus.mode != NotifyNormal);
		break;

	default:
		break;
	}

	return event;
}

PuglStatus
puglGrabFocus(PuglView* view)
{
	XSetInputFocus(
		view->impl->display, view->impl->win, RevertToNone, CurrentTime);
	return PUGL_SUCCESS;
}

bool
puglHasFocus(const PuglView* view)
{
	int    revertTo      = 0;
	Window focusedWindow = 0;
	XGetInputFocus(view->impl->display, &focusedWindow, &revertTo);
	return focusedWindow == view->impl->win;
}

PuglStatus
puglRequestAttention(PuglView* view)
{
	PuglInternals* const      impl  = view->impl;
	const PuglX11Atoms* const atoms = &view->world->impl->atoms;
	XEvent                    event = {0};

	event.type                 = ClientMessage;
	event.xclient.window       = impl->win;
	event.xclient.format       = 32;
	event.xclient.message_type = atoms->NET_WM_STATE;
	event.xclient.data.l[0]    = WM_STATE_ADD;
	event.xclient.data.l[1]    = atoms->NET_WM_STATE_DEMANDS_ATTENTION;
	event.xclient.data.l[2]    = 0;
	event.xclient.data.l[3]    = 1;
	event.xclient.data.l[4]    = 0;

	const Window root = RootWindow(impl->display, impl->screen);
	XSendEvent(impl->display,
	           root,
	           False,
	           SubstructureNotifyMask | SubstructureRedirectMask,
	           &event);

	return PUGL_SUCCESS;
}

PuglStatus
puglWaitForEvent(PuglView* view)
{
	XEvent xevent;
	XPeekEvent(view->impl->display, &xevent);
	return PUGL_SUCCESS;
}

static void
merge_expose_events(PuglEvent* dst, const PuglEvent* src)
{
	if (!dst->type) {
		*dst = *src;
	} else {
		const double max_x = MAX(dst->expose.x + dst->expose.width,
		                         src->expose.x + src->expose.width);
		const double max_y = MAX(dst->expose.y + dst->expose.height,
		                         src->expose.y + src->expose.height);

		dst->expose.x      = MIN(dst->expose.x, src->expose.x);
		dst->expose.y      = MIN(dst->expose.y, src->expose.y);
		dst->expose.width  = max_x - dst->expose.x;
		dst->expose.height = max_y - dst->expose.y;
		dst->expose.count  = MIN(dst->expose.count, src->expose.count);
	}
}

static void
sendRedisplayEvent(PuglView* view)
{
	XExposeEvent ev = { Expose, 0, True, view->impl->display, view->impl->win,
	                    0, 0, (int)view->frame.width, (int)view->frame.height,
	                    0 };

	XSendEvent(view->impl->display, view->impl->win, False, 0, (XEvent*)&ev);
}

PUGL_API PuglStatus
puglDispatchEvents(PuglWorld* world)
{
	const PuglX11Atoms* const atoms = &world->impl->atoms;

	// Send expose events for any views with pending redisplays
	for (size_t i = 0; i < world->numViews; ++i) {
		if (world->views[i]->redisplay) {
			sendRedisplayEvent(world->views[i]);
			world->views[i]->redisplay = false;
		}
	}

	// Flush just once at the start to fill event queue
	Display* display = world->impl->display;
	XFlush(display);

	// Process all queued events (locally, without flushing or reading)
	while (XEventsQueued(display, QueuedAlready) > 0) {
		XEvent xevent;
		XNextEvent(display, &xevent);

		PuglView* view = puglFindView(world, xevent.xany.window);
		if (!view) {
			continue;
		}

		// Handle special events
		PuglInternals* const impl = view->impl;
		if (xevent.type == KeyRelease && view->hints[PUGL_IGNORE_KEY_REPEAT]) {
			XEvent next;
			if (XCheckTypedWindowEvent(display, impl->win, KeyPress, &next) &&
			    next.type == KeyPress &&
			    next.xkey.time == xevent.xkey.time &&
			    next.xkey.keycode == xevent.xkey.keycode) {
				continue;
			}
		} else if (xevent.type == FocusIn) {
			XSetICFocus(impl->xic);
		} else if (xevent.type == FocusOut) {
			XUnsetICFocus(impl->xic);
		} else if (xevent.type == SelectionClear) {
			puglSetBlob(&view->clipboard, NULL, 0);
			continue;
		} else if (xevent.type == SelectionNotify &&
		           xevent.xselection.selection == atoms->CLIPBOARD &&
		           xevent.xselection.target == atoms->UTF8_STRING &&
		           xevent.xselection.property == XA_PRIMARY) {

			uint8_t*      str  = NULL;
			Atom          type = 0;
			int           fmt  = 0;
			unsigned long len  = 0;
			unsigned long left = 0;
			XGetWindowProperty(impl->display, impl->win, XA_PRIMARY,
			                   0, 8, False, AnyPropertyType,
			                   &type, &fmt, &len, &left, &str);

			if (str && fmt == 8 && type == atoms->UTF8_STRING && left == 0) {
				puglSetBlob(&view->clipboard, str, len);
			}

			XFree(str);
			continue;
		} else if (xevent.type == SelectionRequest) {
			const XSelectionRequestEvent* request = &xevent.xselectionrequest;

			XSelectionEvent note = {0};
			note.type            = SelectionNotify;
			note.requestor       = request->requestor;
			note.selection       = request->selection;
			note.target          = request->target;
			note.time            = request->time;

			const char* type = NULL;
			size_t      len  = 0;
			const void* data = puglGetInternalClipboard(view, &type, &len);
			if (data &&
			    request->selection == atoms->CLIPBOARD &&
			    request->target == atoms->UTF8_STRING) {
				note.property = request->property;
				XChangeProperty(impl->display, note.requestor,
				                note.property, note.target, 8, PropModeReplace,
				                (const uint8_t*)data, len);
			} else {
				note.property = None;
			}

			XSendEvent(impl->display, note.requestor, True, 0, (XEvent*)&note);
			continue;
		}

		// Translate X11 event to Pugl event
		const PuglEvent event = translateEvent(view, xevent);

		if (event.type == PUGL_EXPOSE) {
			// Expand expose event to be dispatched after loop
			merge_expose_events(&view->impl->pendingExpose, &event);
		} else if (event.type == PUGL_CONFIGURE) {
			// Expand configure event to be dispatched after loop
			view->impl->pendingConfigure = event;
		} else {
			// Dispatch event to application immediately
			puglDispatchEvent(view, &event);
		}
	}

	// Flush pending configure and expose events for all views
	for (size_t i = 0; i < world->numViews; ++i) {
		PuglView* const  view      = world->views[i];
		PuglEvent* const configure = &view->impl->pendingConfigure;
		PuglEvent* const expose    = &view->impl->pendingExpose;

		if (configure->type || expose->type) {
			const bool mustExpose = expose->type && expose->expose.count == 0;
			puglEnterContext(view, mustExpose);

			if (configure->type) {
				view->frame.x      = configure->configure.x;
				view->frame.y      = configure->configure.y;
				view->frame.width  = configure->configure.width;
				view->frame.height = configure->configure.height;

				view->backend->resize(view,
				                      (int)view->frame.width,
				                      (int)view->frame.height);
				view->eventFunc(view, &view->impl->pendingConfigure);
			}

			if (mustExpose) {
				view->eventFunc(view, &view->impl->pendingExpose);
			}

			puglLeaveContext(view, mustExpose);
			configure->type = 0;
			expose->type    = 0;
		}
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	return puglDispatchEvents(view->world);
}

double
puglGetTime(const PuglWorld* world)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((double)ts.tv_sec + ts.tv_nsec / 1000000000.0) - world->startTime;
}

PuglStatus
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
	return PUGL_SUCCESS;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->win;
}

PuglStatus
puglSetWindowTitle(PuglView* view, const char* title)
{
	Display*                  display = view->world->impl->display;
	const PuglX11Atoms* const atoms   = &view->world->impl->atoms;

	puglSetString(&view->title, title);
	XStoreName(display, view->impl->win, title);
	XChangeProperty(display, view->impl->win, atoms->NET_WM_NAME,
	                atoms->UTF8_STRING, 8, PropModeReplace,
	                (const uint8_t*)title, (int)strlen(title));

	return PUGL_SUCCESS;
}

PuglStatus
puglSetFrame(PuglView* view, const PuglRect frame)
{
	view->frame = frame;

	if (view->impl->win &&
	    XMoveResizeWindow(view->world->impl->display, view->impl->win,
	                      (int)frame.x, (int)frame.y,
	                      (int)frame.width, (int)frame.height)) {
		return PUGL_UNKNOWN_ERROR;
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglSetMinSize(PuglView* const view, const int width, const int height)
{
	Display* display = view->world->impl->display;

	view->minWidth  = width;
	view->minHeight = height;

	if (view->impl->win) {
		XSizeHints sizeHints = getSizeHints(view);
		XSetNormalHints(display, view->impl->win, &sizeHints);
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglSetAspectRatio(PuglView* const view,
                   const int       minX,
                   const int       minY,
                   const int       maxX,
                   const int       maxY)
{
	Display* display = view->world->impl->display;

	view->minAspectX = minX;
	view->minAspectY = minY;
	view->maxAspectX = maxX;
	view->maxAspectY = maxY;

	if (view->impl->win) {
		XSizeHints sizeHints = getSizeHints(view);
		XSetNormalHints(display, view->impl->win, &sizeHints);
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglSetTransientFor(PuglView* view, PuglNativeWindow parent)
{
	Display* display = view->world->impl->display;

	view->transientParent = parent;

	if (view->impl->win) {
		XSetTransientForHint(display, view->impl->win,
		                     (Window)view->transientParent);
	}

	return PUGL_SUCCESS;
}

const void*
puglGetClipboard(PuglView* const    view,
                 const char** const type,
                 size_t* const      len)
{
	PuglInternals* const      impl  = view->impl;
	const PuglX11Atoms* const atoms = &view->world->impl->atoms;

	const Window owner = XGetSelectionOwner(impl->display, atoms->CLIPBOARD);
	if (owner != None && owner != impl->win) {
		// Clear internal selection
		puglSetBlob(&view->clipboard, NULL, 0);

		// Request selection from the owner
		XConvertSelection(impl->display,
		                  atoms->CLIPBOARD,
		                  atoms->UTF8_STRING,
		                  XA_PRIMARY,
		                  impl->win,
		                  CurrentTime);

		// Run event loop until data is received
		while (!view->clipboard.data) {
			puglPollEvents(view->world, -1);
			puglDispatchEvents(view->world);
		}
	}

	return puglGetInternalClipboard(view, type, len);
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
	PuglInternals* const      impl  = view->impl;
	const PuglX11Atoms* const atoms = &view->world->impl->atoms;

	PuglStatus st = puglSetInternalClipboard(view, type, data, len);
	if (st) {
		return st;
	}

	XSetSelectionOwner(impl->display, atoms->CLIPBOARD, impl->win, CurrentTime);
	return PUGL_SUCCESS;
}
