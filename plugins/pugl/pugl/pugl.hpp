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
   @file pugl.hpp Pugl C++ API wrapper.
*/

#ifndef PUGL_HPP_INCLUDED
#define PUGL_HPP_INCLUDED

#include "pugl/pugl.h"

/**
   @defgroup puglmm Puglmm
   Pugl C++ API wrapper.
   @{
*/

namespace pugl {

class View {
public:
	View(int* pargc, char** argv)
		: _view(puglInit(pargc, argv))
	{
		puglSetHandle(_view, this);
		puglSetEventFunc(_view, _onEvent);
	}

	virtual ~View() { puglDestroy(_view); }

	virtual void initWindowParent(PuglNativeWindow parent) {
		puglInitWindowParent(_view, parent);
	}

	virtual void initWindowSize(int width, int height) {
		puglInitWindowSize(_view, width, height);
	}

	virtual void initWindowMinSize(int width, int height) {
		puglInitWindowMinSize(_view, width, height);
	}

	virtual void initWindowAspectRatio(int min_x, int min_y, int max_x, int max_y) {
		puglInitWindowAspectRatio(_view, min_x, min_y, max_x, max_y);
	}

	virtual void initResizable(bool resizable) {
		puglInitResizable(_view, resizable);
	}

	virtual void initTransientFor(uintptr_t parent) {
		puglInitTransientFor(_view, parent);
	}

	virtual void initBackend(const PuglBackend* backend) {
		puglInitBackend(_view, backend);
	}

	virtual void createWindow(const char* title) {
		puglCreateWindow(_view, title);
	}

	virtual void             showWindow()      { puglShowWindow(_view); }
	virtual void             hideWindow()      { puglHideWindow(_view); }
	virtual PuglNativeWindow getNativeWindow() { return puglGetNativeWindow(_view); }

	virtual void onEvent(const PuglEvent* event) = 0;

	virtual void*      getContext()                 { return puglGetContext(_view); }
	virtual void       ignoreKeyRepeat(bool ignore) { puglIgnoreKeyRepeat(_view, ignore); }
	virtual void       grabFocus()                  { puglGrabFocus(_view); }
	virtual void       requestAttention()           { puglRequestAttention(_view); }
	virtual PuglStatus waitForEvent()               { return puglWaitForEvent(_view); }
	virtual PuglStatus processEvents()              { return puglProcessEvents(_view); }
	virtual void       postRedisplay()              { puglPostRedisplay(_view); }

	PuglView* cobj() { return _view; }

private:
	static void _onEvent(PuglView* view, const PuglEvent* event) {
		((View*)puglGetHandle(view))->onEvent(event);
	}

	PuglView* _view;
};

}  // namespace pugl

/**
   @}
*/

#endif  /* PUGL_HPP_INCLUDED */
