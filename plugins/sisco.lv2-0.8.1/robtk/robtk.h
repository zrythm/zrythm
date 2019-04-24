/* robwidget - gtk2 & GL wrapper
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef _ROBTK_H_
#define _ROBTK_H_

#ifndef VERSION
#define VERSION "0.invalid"
#endif

#include <assert.h>
#include <pthread.h>
#include <string.h>

#include <cairo/cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifdef _WIN32
static LARGE_INTEGER getFILETIMEoffset() {
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return (t);
}
#endif

// monotonic time
static void rtk_clock_gettime(struct timespec *ts) {
#ifdef __APPLE__
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK /*CALENDAR_CLOCK*/, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts->tv_sec = mts.tv_sec;
	ts->tv_nsec = mts.tv_nsec;
#elif defined _WIN32
	LARGE_INTEGER  t;
	FILETIME       f;
	double                  microseconds;
	static LARGE_INTEGER    offset;
	static double           frequencyToMicroseconds;
	static int              initialized = 0;
	static BOOL             usePerformanceCounter = 0;

	if (!initialized) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
		if (usePerformanceCounter) {
			QueryPerformanceCounter(&offset);
			frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
		} else {
			offset = getFILETIMEoffset();
			frequencyToMicroseconds = 10.;
		}
	}
	if (usePerformanceCounter) QueryPerformanceCounter(&t);
	else {
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = microseconds;
	ts->tv_sec = t.QuadPart / 1000000;
	ts->tv_nsec = 1000 * (t.QuadPart % 1000000);
#else
	clock_gettime(CLOCK_MONOTONIC, ts);
#endif
}

// realtime time -- only used for  pthread_cond_timedwait()
static void rtk_clock_systime(struct timespec *ts) {
#ifdef __APPLE__
	rtk_clock_gettime(ts);
#elif defined _WIN32
	struct __timeb64 currSysTime;
	_ftime64(&currSysTime);
	ts->tv_sec = currSysTime.time;
	ts->tv_nsec = 1000 * currSysTime.millitm;
#else
	clock_gettime(CLOCK_REALTIME, ts);
#endif

}
/*****************************************************************************/

typedef struct {
	int x; int y;
	int state;
	int direction; // scroll
	int button;
} RobTkBtnEvent;

#ifndef ROBTK_MOD_SHIFT
#error backend implementation misses ROBTK_MOD_SHIFT
#endif

#ifndef ROBTK_MOD_CTRL
#error backend implementation misses ROBTK_MOD_CTRL
#endif

enum {
	ROBTK_SCROLL_ZERO,
	ROBTK_SCROLL_UP,
	ROBTK_SCROLL_DOWN,
	ROBTK_SCROLL_LEFT,
	ROBTK_SCROLL_RIGHT
};

enum LVGLResize {
	LVGL_ZOOM_TO_ASPECT,
	LVGL_LAYOUT_TO_FIT,
	LVGL_CENTER,
	LVGL_TOP_LEFT,
};

typedef struct _robwidget {
	void *self; // user-handle for the actual (wrapped) widget

	/* required */
	bool (*expose_event) (struct _robwidget* handle, cairo_t* cr, cairo_rectangle_t *ev);
	void (*size_request) (struct _robwidget* handle, int *w, int *h);

	/* optional */
	void (*position_set) (struct _robwidget* handle, int pw, int ph);
	void (*size_allocate) (struct _robwidget* handle, int pw, int ph);
	/* optional -- hybrid GL+cairo scaling */
	void (*size_limit) (struct _robwidget* handle, int *pw, int *ph);
	void (*size_default) (struct _robwidget* handle, int *pw, int *ph);

	/* optional */
	struct _robwidget* (*mousedown)    (struct _robwidget*, RobTkBtnEvent *event);
	struct _robwidget* (*mouseup)      (struct _robwidget*, RobTkBtnEvent *event);
	struct _robwidget* (*mousemove)    (struct _robwidget*, RobTkBtnEvent *event);
	struct _robwidget* (*mousescroll)  (struct _robwidget*, RobTkBtnEvent *event);
	void               (*enter_notify) (struct _robwidget*);
	void               (*leave_notify) (struct _robwidget*);

	/* internal - GL */
#ifndef GTK_BACKEND
	void* top;
	struct _robwidget* parent;
	struct _robwidget **children;
	unsigned int childcount;
	float widget_scale;

	bool redraw_pending; // queue_draw_*() failed (during init or top-levelresize)
	bool resized; // full-redraw --containers after resize
	bool hidden; // don't display, skip in layout and events
	int  packing_opts;
	bool block_events;
#endif
	float xalign, yalign; // unused in GTK
	cairo_rectangle_t area; // allocated pos + size
	cairo_rectangle_t trel; // cached pos + size relative to top widget
	bool cached_position;

	/* internal - GTK */
#ifdef GTK_BACKEND
	GtkWidget *m0;
	GtkWidget *c;
#endif

	char name[12]; // debug with ROBWIDGET_NAME()
} RobWidget;


/*****************************************************************************/
/* provided by host */

static void resize_self(RobWidget *rw); // dangerous :) -- never call from expose_event
static void resize_toplevel(RobWidget *rw, int w, int h); // ditto
static void relayout_toplevel(RobWidget *rw);
static void queue_draw(RobWidget *);
static void queue_draw_area(RobWidget *, int, int, int, int);
static void queue_tiny_area(RobWidget *rw, float x, float y, float w, float h);

static void robwidget_toplevel_enable_scaling (RobWidget* rw);
static void robtk_queue_scale_change (RobWidget *rw, const float ws);

static RobWidget * robwidget_new(void *);
static void robwidget_destroy(RobWidget *rw);

static const char * robtk_info(void *);

/*****************************************************************************/
/* static plugin singletons */

static LV2UI_Handle
instantiate(
		void *const               ui_toplevel,
		const LV2UI_Descriptor*   descriptor,
		const char*               plugin_uri,
		const char*               bundle_path,
		LV2UI_Write_Function      write_function,
		LV2UI_Controller          controller,
		RobWidget**               widget,
		const LV2_Feature* const* features);

static void
cleanup(LV2UI_Handle handle);

static enum LVGLResize
plugin_scale_mode(LV2UI_Handle handle);

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer);

static const void *
extension_data(const char* uri);

#define ROBWIDGET_NAME(RW) ( ((RobWidget*)(RW))->name[0] ? (const char*) (((RobWidget*)(RW))->name) : (const char*) "???")
#define ROBWIDGET_SETNAME(RW, TXT) strcpy(((RobWidget*)(RW))->name, TXT); // max 11 chars

/******************************************************************************/
// utils //
static void rect_intersection(cairo_rectangle_t *r, const cairo_rectangle_t *r1, const cairo_rectangle_t *r2){
	cairo_rectangle_t is;
	is.x      = MAX(r1->x, r2->x);
	is.y      = MAX(r1->y, r2->y);
	is.width  = MAX(0, MIN(r1->x + r1->width,  r2->x + r2->width)  - MAX(r1->x, r2->x));
	is.height = MAX(0, MIN(r1->y + r1->height, r2->y + r2->height) - MAX(r1->y, r2->y));
	memcpy(r, &is, sizeof(cairo_rectangle_t));
}

static bool rect_intersect(const cairo_rectangle_t *r1, const cairo_rectangle_t *r2){
	float dest_x, dest_y;
	float dest_x2, dest_y2;

	dest_x  = MAX (r1->x, r2->x);
	dest_y  = MAX (r1->y, r2->y);
	dest_x2 = MIN (r1->x + r1->width, r2->x + r2->width);
	dest_y2 = MIN (r1->y + r1->height, r2->y + r2->height);

	return (dest_x2 > dest_x && dest_y2 > dest_y);
}

static bool rect_intersect_a(const cairo_rectangle_t *r1, const float x, const float y, const float w, const float h) {
	cairo_rectangle_t r2 = { x, y, w, h};
	return rect_intersect(r1, &r2);
}

static void rect_combine(const cairo_rectangle_t *r1, const cairo_rectangle_t *r2, cairo_rectangle_t *dest) {
	double dest_x, dest_y;
	dest_x = MIN (r1->x, r2->x);
	dest_y = MIN (r1->y, r2->y);
	dest->width  = MAX (r1->x + r1->width,  r2->x + r2->width)  - dest_x;
	dest->height = MAX (r1->y + r1->height, r2->y + r2->height) - dest_y;
	dest->x = dest_x;
	dest->y = dest_y;
}

static float rtk_hue2rgb(const float p, const float q, float t) {
	if(t < 0.f) t += 1.f;
	if(t > 1.f) t -= 1.f;
	if(t < 1.f/6.f) return p + (q - p) * 6.f * t;
	if(t < 1.f/2.f) return q;
	if(t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6.f;
	return p;
}

#include "rtk/style.h"
#include "rtk/common.h"

#ifdef GTK_BACKEND

#include "gtk2/common_cgtk.h"
#include "gtk2/robwidget_gtk.h"

#else

#include "gl/common_cgl.h"
#include "gl/robwidget_gl.h"
#include "gl/layout.h"

#endif

#define C_RAD 5

#include "widgets/robtk_checkbutton.h"
#include "widgets/robtk_checkimgbutton.h"
#include "widgets/robtk_multibutton.h"
#include "widgets/robtk_dial.h"
#include "widgets/robtk_label.h"
#include "widgets/robtk_pushbutton.h"
#include "widgets/robtk_radiobutton.h"
#include "widgets/robtk_scale.h"
#include "widgets/robtk_separator.h"
#include "widgets/robtk_spinner.h"
#include "widgets/robtk_xyplot.h"
#include "widgets/robtk_selector.h"
#include "widgets/robtk_image.h"
#include "widgets/robtk_drawingarea.h"

#endif
