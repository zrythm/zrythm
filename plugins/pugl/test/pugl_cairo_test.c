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
   @file pugl_cairo_test.c A simple Pugl test that creates a top-level window.
*/

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/pugl_cairo_backend.h"

#include <cairo.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static PuglWorld* world = NULL;
PuglTestOptions   opts  = {0};

static int      quit        = 0;
static bool     entered     = false;
static bool     mouseDown   = false;
static unsigned framesDrawn = 0;

typedef struct {
	int         x;
	int         y;
	int         w;
	int         h;
	const char* label;
} Button;

static Button buttons[] = { { 128, 128, 64, 64, "1"   },
                            { 384, 128, 64, 64, "2"   },
                            { 128, 384, 64, 64, "3"   },
                            { 384, 384, 64, 64, "4"   },
                            { 0,   0,   0,   0,  NULL } };

static void
roundedBox(cairo_t* cr, double x, double y, double w, double h)
{
	static const double radius  = 10;
	static const double degrees = 3.14159265 / 180.0;

	cairo_new_sub_path(cr);
	cairo_arc(cr,
	          x + w - radius,
	          y + radius,
	          radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr,
	          x + w - radius, y + h - radius,
	          radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr,
	          x + radius, y + h - radius,
	          radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr,
	          x + radius, y + radius,
	          radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);
}

static void
buttonDraw(cairo_t* cr, const Button* but, const double time)
{
	cairo_save(cr);
	cairo_translate(cr, but->x, but->y);
	cairo_rotate(cr, sin(time) * 3.141592);

	// Draw base
	if (mouseDown) {
		cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
	} else {
		cairo_set_source_rgba(cr, 0.3, 0.5, 0.1, 1);
	}
	roundedBox(cr, 0, 0, but->w, but->h);
	cairo_fill_preserve(cr);

	// Draw border
	cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
	cairo_set_line_width(cr, 4.0);
	cairo_stroke(cr);

	// Draw label
	cairo_text_extents_t extents;
	cairo_set_font_size(cr, 32.0);
	cairo_text_extents(cr, but->label, &extents);
	cairo_move_to(cr,
	              (but->w / 2.0) - extents.width / 2,
	              (but->h / 2.0) + extents.height / 2);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_show_text(cr, but->label);

	cairo_restore(cr);
}

static void
onDisplay(PuglView* view)
{
	cairo_t* cr = (cairo_t*)puglGetContext(view);

	// Draw background
	const PuglRect frame  = puglGetFrame(view);
	const double   width  = frame.width;
	const double   height = frame.height;
	if (entered) {
		cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	} else {
		cairo_set_source_rgb(cr, 0, 0, 0);
	}
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	// Scale to view size
	const double scaleX = (width - (512 / width)) / 512.0;
	const double scaleY = (height - (512 / height)) / 512.0;
	cairo_scale(cr, scaleX, scaleY);

	// Draw button
	for (Button* b = buttons; b->label; ++b) {
		buttonDraw(cr, b, opts.continuous ? puglGetTime(world) : 0.0);
	}

	++framesDrawn;
}

static void
onClose(PuglView* view)
{
	(void)view;
	quit = 1;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	switch (event->type) {
	case PUGL_KEY_PRESS:
		if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
			quit = 1;
		}
		break;
	case PUGL_BUTTON_PRESS:
		mouseDown = true;
		puglPostRedisplay(view);
		break;
	case PUGL_BUTTON_RELEASE:
		mouseDown = false;
		puglPostRedisplay(view);
		break;
	case PUGL_ENTER_NOTIFY:
		entered = true;
		puglPostRedisplay(view);
		break;
	case PUGL_LEAVE_NOTIFY:
		entered = false;
		puglPostRedisplay(view);
		break;
	case PUGL_EXPOSE:
		onDisplay(view);
		break;
	case PUGL_CLOSE:
		onClose(view);
		break;
	default: break;
	}

	return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
	opts = puglParseTestOptions(&argc, &argv);
	if (opts.help) {
		puglPrintTestUsage("pugl_test", "");
		return 1;
	}

	world = puglNewWorld();
	puglSetClassName(world, "PuglCairoTest");

	PuglRect  frame = { 0, 0, 512, 512 };
	PuglView* view  = puglNewView(world);
	puglSetFrame(view, frame);
	puglSetMinSize(view, 256, 256);
	puglSetViewHint(view, PUGL_RESIZABLE, opts.resizable);
	puglSetBackend(view, puglCairoBackend());

	puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
	puglSetEventFunc(view, onEvent);

	if (puglCreateWindow(view, "Pugl Test")) {
		return 1;
	}

	puglShowWindow(view);

	PuglFpsPrinter fpsPrinter = { puglGetTime(world) };
	while (!quit) {
		if (opts.continuous) {
			puglPostRedisplay(view);
		} else {
			puglPollEvents(world, -1);
		}

		puglDispatchEvents(world);

		if (opts.continuous) {
			puglPrintFps(world, &fpsPrinter, &framesDrawn);
		}
	}

	puglFreeView(view);
	puglFreeWorld(world);
	return 0;
}
